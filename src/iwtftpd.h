/*
 * iwtftpd.h
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

#ifndef _IWTFTPD_H_
#define _IWTFTPD_H_

#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sysexits.h>
#include <popt.h>

#include "iw_common.h"
#include "iw_log.h"
#include "iw_ds.h"
#include "iw_tftp.h"


#define PROGRAM_NAME "iwtftpd"
#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION "0.0"
#endif
#define PROGRAM_INFO PROGRAM_NAME" v"PROGRAM_VERSION


/* constants */
#define DEFAULT_IPVER (IW_IPV4 | IW_IPV6)     /* default using IPv4 and IPv6 */
#define DEFAULT_USER "nobody"		      /* default user of process */


/* server configuration */
struct svconf {
  int32_t ipver;		/* IP version of network interface */
  char *ifname;			/* name of network interfage */
  char *datastore;		/* path of datastore */
  char *user;			/* username of process */
  int32_t verbose;		/* flag of verbose logging */
};

/* flag for exiting event loop */
extern volatile sig_atomic_t g_evloop_exit;


/* status codes */
enum STATCODE {
  /* error */
  E_DS_FAIL_INIT = 1,
  E_DS_FAIL_SETCHROOT,
  E_FAIL_DAEMONIZE,
  E_FAIL_EXEC_CHROOT,
  E_FAIL_INIT_SIGNAL,
  E_FAIL_MALLOC,
  E_FAIL_SERVICE,
  E_FAIL_SET_CREDENTIAL,
  E_FATALERR,
  E_LOG_FAIL_INIT,
  E_LOG_MODERR,
  E_OPTION_BAD,
  E_TFTP_FAIL_INIT,
  E_USER_UNKNOWN,
  I_NOASROOT,
  I_START_SERVER,
  I_EXIT_SERVER,
  I_SHOW_VER,
};

enum STATCODE_VERBOSE {
  /* error verbose */
  EV_FAIL_CHDIR = 65,
  EV_FAIL_CHROOT,
  EV_FAIL_CREATE_SVCONF,
  EV_FAIL_DUP2,
  EV_FAIL_FORK,
  EV_FAIL_INITGROUPS,
  EV_FAIL_OPEN,
  EV_FAIL_SETGID,
  EV_FAIL_SETSID,
  EV_FAIL_SETUID,
  EV_FAIL_SIGACTION,
};


/* function prototypes */
static void pmsg(int32_t statcode, ...);
static struct svconf *create_svconf(void);
static int32_t set_svconf(struct svconf *psv, int pargc, const char **pargv);
static int32_t init_signal(int *siglist, uint32_t nlist, void (*func)(int));
static void sig_handler(int sig);
static int32_t daemonize(int excfd);
static int32_t exec_chroot(const char *dirpath);
static int32_t set_credential(const char *username, uid_t uid, gid_t gid);


#endif	/* _IWTFTPD_H_ */
