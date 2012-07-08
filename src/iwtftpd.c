/*
 * iwtftpd.c
 *
 * Copyright (c) 2012, Daisuke Sato <bigsplint@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "iwtftpd.h"


/* global objects */
int32_t g_iw_logready = IW_FALSE;	      /* log file ready */

/* messages of status */
static struct iwstatus statmsgs[] = {
  /* error */
  { E_DS_FAIL_INIT, "error: iwds initialization failed" },
  { E_DS_FAIL_SETCHROOT, "error: failed to set the root directory to datastore" },
  { E_FAIL_DAEMONIZE, "error: could not start daemon" },
  { E_FAIL_EXEC_CHROOT, "error: could not change the root directory to '%s'" },
  { E_FAIL_INIT_SIGNAL, "error: signal initialization failed" },
  { E_FAIL_MALLOC, "error: failed memory allocation, %s" },
  { E_FAIL_SERVICE, "error: failed to start the service" },
  { E_FAIL_SET_CREDENTIAL, "error: could not change the credential to '%s'" },
  { E_FATALERR, "error: fatal error" },
  { E_LOG_FAIL_INIT, "error: iwlog initialization failed" },
  { E_LOG_MODERR, "error: iwlog module error" },
  { E_OPTION_BAD, "error: bad option: '%s': %s" },
  { E_TFTP_FAIL_INIT, "error: iwtftp initialization failed" },
  { E_USER_UNKNOWN, "error: unknown user '%s'" },
  /* info */
  { I_NOASROOT, "info: must be run as root" },
  { I_START_SERVER, "info: starting server" },
  { I_EXIT_SERVER, "info: exiting server" },
  { I_SHOW_VER, PROGRAM_INFO },
  { 0, NULL }
};

static struct iwstatus statvmsgs[] = {
  /* error verbose */
  { EV_FAIL_CHDIR, "error: chdir: failed, '%s': %s" },
  { EV_FAIL_CHROOT, "error: chroot: failed, '%s': %s" },
  { EV_FAIL_CREATE_SVCONF, "error: could not create a configuration object" },
  { EV_FAIL_DUP2, "error: dup2: failed to duplicate fd '%d': %s" },
  { EV_FAIL_FORK, "error: fork: failed: %s" },
  { EV_FAIL_INITGROUPS, "error: initgroups: failed, '%s': %s" },
  { EV_FAIL_OPEN, "error: open: failed, '%s': %s" },
  { EV_FAIL_SETGID, "error: setgid: failed, '%d': %s" },
  { EV_FAIL_SETSID, "error: setsid: failed: %s" },
  { EV_FAIL_SETUID, "error: setuid: failed, '%d': %s" },
  { EV_FAIL_SIGACTION, "error: sigaction: failed to '%s' action: %s" },
  { 0, NULL }
};

/* -------------------------------------------------------------------------- */

int
main(int argc, const char **argv)
{
  int32_t exitval = 0;
  uid_t psuid;
  struct svconf *svc = NULL;

  int siglist[] = { SIGTERM, SIGQUIT, SIGHUP };
  int ign_siglist[] = { SIGINT, SIGPIPE, SIGUSR1, SIGUSR2, SIGTSTP, SIGTTIN, SIGTTOU };

  int logfd = -1;
  IWDS *ads = NULL;
  IWTFTP *atftp = NULL;
  struct passwd *pwd;

  /* initialize */
  if (! (svc = create_svconf())) {
    pmsg(EV_FAIL_CREATE_SVCONF);
    exitval = EX_SOFTWARE;
    goto ferr;
  }

  switch (set_svconf(svc, argc, argv)) {
  case I_SHOW_VER:
    goto showver;
  case IW_ERR:
    exitval = EX_USAGE;
    goto ferr;
  }

  /* check root */
  if ((psuid = getuid()) != 0) {
    pmsg(I_NOASROOT);
    exitval = EX_USAGE;
    goto ferr;
  }
      
  if (init_signal(siglist, sizeof siglist / sizeof siglist[0], sig_handler) == IW_ERR) {
    pmsg(E_FAIL_INIT_SIGNAL);
    exitval = EX_OSERR;
    goto ferr;
  }
  if (init_signal(ign_siglist, sizeof ign_siglist / sizeof ign_siglist[0], SIG_IGN) == IW_ERR) {
    pmsg(E_FAIL_INIT_SIGNAL);
    exitval = EX_OSERR;
    goto ferr;
  }

  /* initialize log module */
  if (iwlog_init(svc->verbose) == IW_ERR) {
    pmsg(E_LOG_FAIL_INIT);
    exitval = EX_SOFTWARE;
    goto ferr;
  }
  g_iw_logready = IW_TRUE;
  
  /* daemonize */
  if ((logfd = iwlog_getfd()) == IW_ERR) {
    pmsg(E_LOG_MODERR);
    exitval = EX_SOFTWARE;
    goto ferr;
  }
  
  if (daemonize(logfd) == IW_ERR) {
    pmsg(E_FAIL_DAEMONIZE);
    exitval = EX_OSERR;
    goto ferr;
  }

  /* initialize other modules */
  if (! (ads = iwds_init(svc->datastore))) {
    pmsg(E_DS_FAIL_INIT);
    exitval = EX_SOFTWARE;
    goto ferr;
  }
  svc->datastore = iwds_get_dspath(ads);

  if (! (atftp = iwtftp_init(svc->ipver, svc->ifname, ads))) {
    pmsg(E_TFTP_FAIL_INIT);
    exitval = EX_SOFTWARE;
    goto ferr;
  }

  /* security */
  /* before chroot, retrieve UID and GID*/
  if (! (pwd = getpwnam(svc->user))) {
    pmsg(E_USER_UNKNOWN, svc->user);
    exitval = EX_OSERR;
    goto ferr;
  }

  if (exec_chroot(svc->datastore) == IW_ERR) {
    pmsg(E_FAIL_EXEC_CHROOT, svc->datastore);
    exitval = EX_OSERR;
    goto ferr;
  }

  if (iwds_set_chroot(ads) == IW_ERR) {
    pmsg(E_DS_FAIL_SETCHROOT);
    exitval = EX_SOFTWARE;
    goto ferr;
  }

  if (set_credential(pwd->pw_name, pwd->pw_uid, pwd->pw_gid) == IW_ERR) {
    pmsg(E_FAIL_SET_CREDENTIAL, pwd->pw_name);
    exitval = EX_OSERR;
    goto ferr;
  }

  /* starting service */
  pmsg(I_START_SERVER);
  if (iwtftp_service(atftp) == IW_ERR) {
    pmsg(E_FAIL_SERVICE);
    exitval = EX_SOFTWARE;
    goto ferr;
  }

  pmsg(I_EXIT_SERVER);
  iwtftp_exit(atftp);
  iwds_exit(ads);
  iwlog_exit();
  free(svc);
  exit(EX_OK);

 showver:
  pmsg(I_SHOW_VER);
  free(svc);
  exit(EX_OK);
  
 ferr:
  if (exitval != EX_USAGE) {
    pmsg(E_FATALERR);
  }
  iwtftp_exit(atftp);
  iwds_exit(ads);
  iwlog_exit();
  free(svc);
  exit(exitval);
}


static void
pmsg(int32_t statcode, ...)
{
  va_list pargs;
  char buf[IW_LOGMSGLEN_MAX];
  size_t mlen = 0;
  struct iwstatus *pm;

  if (statcode < 64) {
    pm = statmsgs;
  }
  else {
    if (iwlog_is_verbose() == IW_TRUE) {
      pm = statvmsgs;
    }
    else {
      return;
    }
  }

  while ((*pm).statcode > 0 && (*pm).statcode != statcode) {
    pm++;
  }

  if ((*pm).statcode == 0) {
    return ;
  }
  
  va_start(pargs, statcode);  
  if (statcode < 64) {
    vsnprintf(buf, sizeof buf, (*pm).statmsg, pargs);
  }
  else {			/* verbose */
    strcpy(buf, "[main] ");
    mlen = strlen(buf);
    vsnprintf(buf + mlen, sizeof buf - mlen, (*pm).statmsg, pargs);
  }
  va_end(pargs);
  
  mlen = strlen(buf);  
  if (g_iw_logready == IW_TRUE) {
    iwlog_print_msg(buf, mlen);
  }
  else {
    if (sizeof buf - mlen >= 2) {
      *(buf + mlen) = '\n';
      *(buf + mlen + 1) = '\0';
    }
    else {
      *(buf + mlen - 1) = '\n';
    }
    fputs(buf, stderr);
  }
}


static struct svconf *
create_svconf(void)
{
  struct svconf *psv;

  if (! (psv = malloc(sizeof(struct svconf)))) {
    pmsg(E_FAIL_MALLOC, __FUNCTION__);
    return NULL;
  }
  
  /* initialize member*/
  psv->ipver = 0;
  psv->ifname = NULL;
  psv->datastore = NULL;
  psv->user = NULL;
  psv->verbose = IW_FALSE;

  return psv;
}


static int32_t
set_svconf(struct svconf *psv, int pargc, const char **pargv)
{
  poptContext optcon;
  int opt;
  int32_t useipver = DEFAULT_IPVER;
  char *netdev;
  char *dirpath;
  char *uname;
  int32_t verbose = IW_FALSE;
  int32_t showver = IW_FALSE;
  const char *leftover;

  struct poptOption optlist[] = {
    { NULL, '4', POPT_BIT_CLR, &useipver, IW_IPV6, "Use only IPv4", NULL },
    { NULL, '6', POPT_BIT_CLR, &useipver, IW_IPV4, "Use only IPv6", NULL },
    { "if", 'i', POPT_ARG_STRING, &netdev, 'i', "Use bind interface only", "NETDEV" },
    { "datastore", 'd', POPT_ARG_STRING, &dirpath, 'd', "Path of datastore", "DIRPATH" },
    { "username", 'u', POPT_ARG_STRING, &uname, 'u', "Username in /etc/passwd", "USER" },
    { "verbose", 'v', POPT_ARG_VAL, &verbose, IW_TRUE, "Verbose mode", NULL },
    { "version", 'V', POPT_ARG_VAL, &showver, IW_TRUE, "Show version", NULL },
    POPT_AUTOHELP
    POPT_TABLEEND
  };

  /* get args */
  optcon = poptGetContext(NULL, pargc, pargv, optlist, 0);

  poptSetOtherOptionHelp(optcon, "[OPTIONS]");

  while ((opt = poptGetNextOpt(optcon)) >= 0) {
    switch (opt) {
    case 'i':
      psv->ifname = netdev;
      break;
    case 'd':
      psv->datastore = dirpath;
      break;
    case 'u':
      psv->user = uname;
      break;
    }
  }

  if (opt < -1) {
    pmsg(E_OPTION_BAD, poptBadOption(optcon, POPT_BADOPTION_NOALIAS), poptStrerror(opt));
    goto err;
  }

  if ((leftover = poptGetArg(optcon))) {
    pmsg(E_OPTION_BAD, leftover, "too arguments");
    goto err;
  }

  if (showver == IW_TRUE) {
    poptFreeContext(optcon);
    return I_SHOW_VER;
  }
  
  if (! useipver) {
    pmsg(E_OPTION_BAD, "specify either -4 or -6", "");
    goto err;
  }
  psv->ipver = useipver;

  if (! psv->user) {
    psv->user = DEFAULT_USER;
  }

  psv->verbose = verbose;
  
  poptFreeContext(optcon);
  return IW_OK;

 err:
  fputc('\n', stderr);
  poptPrintUsage(optcon, stderr, 0);
  poptFreeContext(optcon);
  return IW_ERR;
}


static int32_t
init_signal(int *siglist, uint32_t nlist, void (*func)(int))
{
  struct sigaction sa;

  while (nlist) {
    if (sigaction(siglist[nlist - 1], NULL, &sa) == -1) {
      pmsg(EV_FAIL_SIGACTION, "get", strerror(errno));
      goto err;
    }
    
    sa.sa_handler = func;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(siglist[nlist - 1], &sa, NULL) == -1) {
      pmsg(EV_FAIL_SIGACTION, "set", strerror(errno));
      goto err;
    }
    nlist--;
  }
  return IW_OK;

 err:
  fprintf(stderr, "nlist=%d\n", nlist);
  return IW_ERR;
}


static void
sig_handler(int sig)
{
  g_evloop_exit = sig > 0 ? IW_TRUE : IW_FALSE;
}


static int32_t
daemonize(int excfd)
{
  pid_t pid;
  int nullfd;
  int32_t i;

  if ((pid = fork()) == -1) {
    pmsg(EV_FAIL_FORK, strerror(errno));
    goto err;
  }
  else if (pid > 0) {
    _exit(IW_OK);		/* parent */
  }

  /* child */
  
  if ((nullfd = open("/dev/null", O_RDWR)) == -1) {
    pmsg(EV_FAIL_OPEN, "dev/null", strerror(errno));
    goto err;
  }

  for (i = 0; i < IW_FD_MAX; i++) {
    if (i != excfd && i != nullfd) {
      close(i);
    }
  }
  
  for (i = 0; i < 3; i++) {
    if (dup2(nullfd, i) == -1) {
      pmsg(EV_FAIL_DUP2, i, strerror(errno));
      goto err;
    }
  }
  close(nullfd);

  umask(0);
  
  if (setsid() == -1) {
    pmsg(EV_FAIL_SETSID, strerror(errno));
    goto err;
  }

  return IW_OK;

 err:
  return IW_ERR;
}


static int32_t
exec_chroot(const char *dirpath)
{
  if (chroot(dirpath) == -1) {
    pmsg(EV_FAIL_CHROOT, dirpath, strerror(errno));
    goto err;
  }

  if (chdir("/") == -1) {
    pmsg(EV_FAIL_CHDIR, "/", strerror(errno));
    goto err;
  }
  
  return IW_OK;

 err:
  return IW_ERR;
}


static int32_t
set_credential(const char *username, uid_t uid, gid_t gid)
{
  /* change uid and gid in proccess */
  if (setgid(gid) == -1) {
    pmsg(EV_FAIL_SETGID, gid, strerror(errno));
    goto err;
  }

  if (initgroups(username, gid) == -1) {
    pmsg(EV_FAIL_INITGROUPS, username, strerror(errno));
    goto err;
  }

  if (setuid(uid) == -1) {
    pmsg(EV_FAIL_SETUID, uid, strerror(errno));
    goto err;
  }
  
  return IW_OK;

 err:
  return IW_ERR;
}
