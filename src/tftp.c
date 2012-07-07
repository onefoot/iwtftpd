/*
 * tftp.c
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

#include "tftp.h"


/* flag for exiting event loop */
volatile sig_atomic_t g_evloop_exit = IW_FALSE;

/* TFTP error messages */
static char *errmsgs[] = {
  "",				        /* TFTP_ERR_SEEMSG */
  "file not found",			/* TFTP_ERR_NOTFOUND */
  "access violation",			/* TFTP_ERR_ACCESSDENY */
  "disk full or allocation exceeded",   /* TFTP_ERR_DISKFULL */
  "illegal tftp operation",		/* TFTP_ERR_ILLEGALOPE */
  "unknown transfer id",		/* TFTP_ERR_UNKNOWNID */
  "file already exists",		/* TFTP_ERR_FILEEXIST */
  "no such user"			/* TFTP_ERR_NOUSER */
};

/* messages of status */
static struct iwstatus iwtftp_statmsgs[] = {
  /* error */
  { E_DS_FAIL_READ, "error: failed to read the data from datastore, %s" },
  { E_DS_FAIL_WRITE, "error: failed to write the data to datastore, %s" },
  { E_DS_FAIL_CLOSE, "error: failed to close the session on datastore, %s" },
  { E_EVENT_NOTEXIST, "error: events nothing" },
  { E_FAIL_ADDRCONVERT, "error: failed to convert ip address" },
  { E_FAIL_CREATE_SOCKET, "error: failed to create socket, ipv%d, %s, %s" },
  { E_FAIL_GET_IFADDRESS, "error: failed to get addresses of interface '%s'" },
  { E_FAIL_LOADFILE, "error: could not load the file, '%s'" },
  { E_FAIL_MAKEACK, "error: could not make TFTP ACK message, for '%s:%d'" },
  { E_FAIL_MAKEERROR, "error: could not make TFTP ERROR message, for '%s:%d'" },
  { E_FAIL_MALLOC, "error: failed memory allocation, %s" },
  { E_FAIL_RECV, "error: failed to recv" },
  { E_FAIL_RESEND, "error: failed to resend, to '%s:%d'" },
  { E_FAIL_SAVEFILE, "error: could not save the file, '%s'" },
  { E_FAIL_TFTP_PROC, "error: failed TFTP processing" },
  { E_FAIL_UPDATE_EVENT, "error: error of updating events" },
  { E_IF_NOADDR, "error: interface '%s' has no ipv%d address" },
  { E_IF_NOTFOUND, "error: interface '%s' not found" },
  { E_SERVER_ERR, "error: server error" },
  /* info */
  { I_FILE_EXIST, "info: '%s' already exists" },
  { I_FILE_NOTFOUND, "info: '%s' not found" },
  { I_INVALID_BLKNUM, "info: invalid block number in TFTP %s message, from '%s:%d'" },
  { I_INVALID_TFTPMSG, "info: invalid TFTP %s message, from '%s:%d'" },
  { I_TFTPACK_INCORRECT, "info: incorrect TFTP ACK format, from '%s:%d'" },
  { I_TFTPDATA_INCORRECT, "info: incorrect TFTP DATA format, from '%s:%d'" },
  { I_TFTPERROR_INCORRECT, "info: incorrect TFTP ERROR format, from '%s:%d'" },
  { I_TFTPERROR_RECV, "info: received TFTP ERROR, code %d, %s, from '%s:%d'" },
  { I_TFTPREQ_GET, "info: get request '%s', by '%s:%d'" },
  { I_TFTPREQ_INCORRECT, "info: incorrect TFTP request format, from '%s:%d'" },
  { I_TFTPREQ_PUT, "info: put request '%s', by '%s:%d'" },
  { I_TFTPTRANS_FIN, "info: '%s' completed, with '%s:%d'" },
  { 0, NULL }
};

static struct iwstatus iwtftp_statvmsgs[] = {
  /* error verbose */
  { EV_BUF_TOOSHORT, "error: buffer of %s has shortage" },
  { EV_ERRMSG_TOOLONG, "error: TFTP error message is too long" },
  { EV_FAIL_ADD_NEWSESSION, "error: failed to add the session, '%s:%d'" },
  { EV_FAIL_BIND, "error: bind: failed: %s" },
  { EV_FAIL_CREATE_SESSION,"error: could not create a new session" },
  { EV_FAIL_EPOLL_CREATE, "error: epoll_create: failed: %s" },
  { EV_FAIL_EPOLL_CTL, "error: epoll_ctl: failed to %s event of '%s:%d': %s" },
  { EV_FAIL_EPOLL_WAIT, "error: epoll_wait: failed: %s" },
  { EV_FAIL_GETADDRINFO, "error: getaddrinfo: failed: %s" },
  { EV_FAIL_GETIFADDRS, "error: getifaddrs: '%s': %s" },
  { EV_FAIL_GETNAMEINFO, "error: getnameinfo: failed: %s" },
  { EV_FAIL_GETSOCKNAME, "error: getsockname: failed: %s" },
  { EV_FAIL_INET_NTOP, "error: inet_ntop: ipv%d '%s': %s" },
  { EV_FAIL_RECVFROM, "error: recvfrom: failed: %s" },
  { EV_FAIL_SENDTO, "error: sendto: failed to send to '%s:%d': %s" },
  { EV_FAIL_SETSOCKOPT,	"error: setsockopt: failed: %s" },
  { EV_FAIL_SOCKET, "error: socket: failed to create a socket: %s" },
  { EV_NULL_OBJ, "error: invalid object" },
  { EV_SESSION_NOTFOUND, "error: session not found, '%s:%d'" },
  /* info verbose */
  { IV_ACK_INCORRECT, "info: ack filed is incorrect" },
  { IV_DATA_INCORRECT, "info: data filed is too long" },
  { IV_ERRMSG_INCORRECT, "info: errormsg filed is incorrect" },
  { IV_ERRORLEN_TOOSHORT, "info: length of TFTP ERROR message is too short" },
  { IV_FILENAME_INCORRECT, "info: filename field is incorrect" },
  { IV_MODE_INCORRECT, "info: mode field is incorrect" },
  { IV_OPCODE_INCORRECT, "info: opcode filed is incorrect" },
  { IV_REQLEN_TOOSHORT, "info: length of TFTP request message is too short" },
  { IV_UNKNOWN_MSG, "info: unknown message, from '%s:%d'" },
#ifdef DEBUG
  /* debugging */
  { DBG_ADD_EVENT, "DBG: added a event to epoll, clip=%s, clport=%d" },
  { DBG_ADD_INITEVENT, "DBG: epoll event initialization" },
  { DBG_ADD_SESSION, "DBG: add a new session" },
  { DBG_CHECK_FILE, "DBG: check the file in the datastore" },
  { DBG_CLEANUP_SESSION, "DBG: clean up sessions" },
  { DBG_CLOSE_DATA, "DBG: closing the file on datastore" },
  { DBG_CREATE_SOCKET, "DBG: create a new socket" },
  { DBG_DEL_ALLSESSION, "DBG: deleting all sessions" },
  { DBG_DEL_EVENT, "DBG: deleted a event to epoll, clip=%s, clport=%d" },
  { DBG_DEL_SESSION, "DBG: delete a session" },
  { DBG_DS_IOLEN, "DBG: DS: I/O: datalen=%d completed" },
  { DBG_DS_LOAD, "DBG: DS: loading data" },
  { DBG_DS_REQ, "DBG: DS: REQ: dsid=%d, dfile=%s, dbuf=%s, dlen=%d, derr=%d" },
  { DBG_DS_SAVE, "DBG: DS: saving data" },
  { DBG_DS_SETREQ, "DBG: DS: setting a request ticket of the datastore" },
  { DBG_MAKE_TFTPACK, "DBG: making a TFTP ACK message" },
  { DBG_MAKE_TFTPDATA, "DBG: making a TFTP DATA message" },
  { DBG_MAKE_TFTPERROR, "DBG: making a TFTP ERROR message" },
  { DBG_OPCODE, "DBG: opcode=%s" },
  { DBG_PARSE_TFTPACK, "DBG: parsing a TFTP ACK message" },
  { DBG_PARSE_TFTPDATA, "DBG: parsing a TFTP DATA message" },
  { DBG_PARSE_TFTPERROR, "DBG: parsing a TFTP ERROR message" },
  { DBG_PARSE_TFTPREQ, "DBG: parsing a TFTP request message" },
  { DBG_PREPARE_RESEND, "DBG: setting sendinfo and updating retry count of the session" },
  { DBG_RECV, "DBG: received: rlen=%d, sock=%d, clip=%s, clport=%d" },
  { DBG_REMAIN_SESSION, "DBG: remains of session" },
  { DBG_RESEND_SESSION, "DBG: resend sessions" },
  { DBG_RETRIEVE_SESSION, "DBG: retrieving the session, clip=%s, clport=%d" },
  { DBG_SEND, "DBG: sent: msglen=%d, sock=%d, clip=%s, clport=%d" },
  { DBG_SENDINFO, "DBG: SENDINFO: sendsock=%d, msglen=%d" },
  { DBG_SEND_SESSION, "DBG: send a TFTP message to the client" },
  { DBG_SESSION_ALL, "DBG: SES %d: '%s:%d', clsock=%d, regev=%d, file=%s, blk=%d, fin=%d, "
                     "lastmlen=%u, time=%d, retry=%d, dis=%d" },
  { DBG_SESSION_EMPTY, "DBG: session is empty" },
  { DBG_SESSION_ONE, "DBG: SES: '%s:%d', clsock=%d, regev=%d, file=%s, blk=%d, fin=%d, "
		     "lastmlen=%u, time=%d, retry=%d, dis=%d" },
  { DBG_SET_DISABLE, "DBG: disabled this session" },
  { DBG_SET_FIN, "DBG: finished this session" },
  { DBG_SOCKET, "DBG: socket=%d, ip=%s, port=%s" },
  { DBG_START_EVLOOP, "DBG: event loop start" },
  { DBG_START_TFTP, "DBG: starting TFTP service" },
  { DBG_TFTPACK, "DBG: TFTPACK: msglen=%d: op=%d, blk=%d" },
  { DBG_TFTPDATA, "DBG: TFTPDATA: msglen=%d: op=%d, blk=%d, data=%s" },
  { DBG_TFTPERROR, "DBG: TFTPERROR: msglen=%d, op=%d, ecode=%d, emsg=%s, emsglen=%d" },
  { DBG_TFTPREQ, "DBG: TFTPREQ: msglen=%d: op=%d, file=%s, mode=%s" },
  { DBG_TFTP_PROC, "DBG: TFTP processing" },
  { DBG_UPDATE_EVENT, "DBG: updating epoll events" },
  { DBG_UPDATE_RETRY, "DBG: updating retry count of the session" },
#endif	/* DEBUG */
  { 0, NULL }
};

/* -------------------------------------------------------------------------- */

/* ------------ */
/*  Interfaces  */
/* ------------ */

extern IWTFTP *
iwtftp_init(int32_t ipver, const char *ifname, IWDS *pds)
{
  IWTFTP *ins = NULL;
  struct ifinet iaddr;
  int32_t i;
  
  if (! (ins = malloc(sizeof(IWTFTP)))) {
    pmsg(E_FAIL_MALLOC, __FUNCTION__);
    goto err;
  }
  memset(ins, 0, sizeof(IWTFTP));
  memset(ins->svsocks, -1, sizeof ins->svsocks);

  if (! pds) {
    pmsg(EV_NULL_OBJ);
    goto err;
  }
  ins->ads = pds;

  /* retrieve ipv4 address and ipv6 address */
  if (ifname) {
    if (get_ifaddress(ifname, &iaddr) == IW_ERR) {
      pmsg(E_FAIL_GET_IFADDRESS, ifname);
      goto err;
    }

    if (ipver & IW_IPV4) {
      if (iaddr.ipv4addr[0] == '\0') {
	pmsg(E_IF_NOADDR, ifname, 4);
	goto err;
      }
    }
    if (ipver & IW_IPV6) {
      if (iaddr.ipv6addr[0] == '\0') {
	pmsg(E_IF_NOADDR, ifname, 6);
	goto err;
      }
    }
  }
  
  if (ipver & IW_IPV4) {
    for (i = 0; i < SVSOCKS_MAX; i++) {
      if (ins->svsocks[i] < 0) {
	break;
      }
    }
    ins->svsocks[i] = create_socket(AF_INET, ifname ? iaddr.ipv4addr : "ANY", TFTP_PORT);
    if (ins->svsocks[i] == IW_ERR) {
      pmsg(E_FAIL_CREATE_SOCKET, 4, ifname ? iaddr.ipv4addr : "ANY", TFTP_PORT);
    }
  }

  if (ipver & IW_IPV6) {
    for (i = 0; i < SVSOCKS_MAX; i++) {
      if (ins->svsocks[i] < 0) {
	break;
      }
    }
    ins->svsocks[i] = create_socket(AF_INET6, ifname ? iaddr.ipv6addr : "ANY", TFTP_PORT);
    if (ins->svsocks[i] == IW_ERR) {
      pmsg(E_FAIL_CREATE_SOCKET, 6, ifname ? iaddr.ipv6addr : "ANY", TFTP_PORT);
    }
  }

  for (i = 0; i < SVSOCKS_MAX; i++) {
    if (ins->svsocks[i] >= 0) {
      return ins;
    }
  }

 err:
  if (ins) {
    for (i = 0; i < SVSOCKS_MAX; i++) {
      if (ins->svsocks[i] >= 0)
	close(ins->svsocks[i]);
    }
  }
  free(ins);
  return NULL;
}


extern void
iwtftp_exit(IWTFTP *ins)
{
  int32_t i;
  
  if (! ins)
    return;

  del_allsession(&ins->seshead);

  for (i = 0; i < SVSOCKS_MAX; i++) {
    if (ins->svsocks[i] != -1) {
      close(ins->svsocks[i]);
      ins->svsocks[i] = -1;
    }
  }
  free(ins);
}


extern int32_t
iwtftp_service(IWTFTP *ins)
{
  /* for epoll */
  struct epoll_event events[EVENTS_MAX];
  struct epoll_event setev;
  int epollfd = -1;
  int32_t nfds;
  int32_t timeout;
  int32_t nevents;
  int32_t i, n;
  /* for receiving */
  struct sockaddr_storage from;
  socklen_t fromlen;
  ssize_t rlen;
  /* for getting client address */
  uint8_t rbuf[NWBUF_SIZE];
  char clipbuf[NI_MAXHOST];	/* 1025 */
  char clportbuf[NI_MAXSERV];	/* 32 */
  int ecode;
  uint16_t clport;
  /* for sending */
  struct sendinfo sinfo;
  int sendsock;
  ssize_t slen;

  DBG_PRINT(DBG_START_TFTP);
  
  if (! ins) {
    pmsg(EV_NULL_OBJ);
    goto err;
  }

  if ((epollfd = epoll_create(EVENTS_MAX)) == -1) {
    pmsg(EV_FAIL_EPOLL_CREATE, strerror(errno));
    goto err;
  }

  timeout = BLOCKING_TIMEOUT;

  DBG_PRINT(DBG_ADD_INITEVENT);
  
  /* add server socket to the epoll event */
  for (nevents = 0, i = 0; i < SVSOCKS_MAX; i++) {
    if (ins->svsocks[i] != -1) {
      setev.data.fd = ins->svsocks[i];
      setev.events = EPOLLIN;

      if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ins->svsocks[i], &setev) == -1) {
	pmsg(EV_FAIL_EPOLL_CTL, "add", "SERVER", TFTP_PORT, strerror(errno));
	continue;
      }
      nevents++;
    }
  }
  if (nevents == 0) {
    pmsg(E_EVENT_NOTEXIST);
    goto err;
  }

  DBG_PRINT(DBG_START_EVLOOP);

  /* event loop */
  while (g_evloop_exit != IW_TRUE) {
    switch ((nfds = epoll_wait(epollfd, events, EVENTS_MAX, timeout))) {
    case -1:
      if (errno != EINTR) {
	pmsg(EV_FAIL_EPOLL_WAIT, strerror(errno));
      }
      break;
    case 0:
      /* epoll timeout */
      break;
    default:
      for (n = 0; n < nfds; n++) {
	fromlen = sizeof from;
	rlen = recvfrom(events[n].data.fd, rbuf, sizeof rbuf, 0, (struct sockaddr *)&from, &fromlen);
	if (rlen == -1) {
	  pmsg(EV_FAIL_RECVFROM, strerror(errno));
	  continue;
	}

	/* get client address and port */
	if ((ecode = getnameinfo((struct sockaddr *)&from, fromlen, clipbuf, sizeof clipbuf,
				 clportbuf, sizeof clportbuf, NI_NUMERICHOST | NI_NUMERICSERV)) != 0) {
	  pmsg(EV_FAIL_GETNAMEINFO, gai_strerror(ecode));
	  pmsg(E_FAIL_ADDRCONVERT);
	  continue;
	}

	if (strlen(clipbuf) + 1 > IPADDRLEN_MAX) {
	  pmsg(EV_BUF_TOOSHORT, "ip address");
	  pmsg(E_FAIL_ADDRCONVERT);
	  continue;
	}

	clport = atoi(clportbuf);

	DBG_SH_RECV(rlen, events[n].data.fd, clipbuf, clport);

	/* tftp processing */
	if (tftp_proc(ins, events[n].data.fd, clipbuf, clport, rbuf, rlen, &sinfo) == IW_ERR) {
	  pmsg(E_FAIL_TFTP_PROC);
	  continue;
	}

	/* send reply */
	sendsock = sinfo.ses ? sinfo.ses->clsock : events[n].data.fd;

	DBG_SH_SENDINFO(sendsock, sinfo.msglen);

	if (sinfo.ses && sinfo.ses->disabled == IW_TRUE) {
	  continue;
	}

	if ((slen = sendto(sendsock, sinfo.msgbuf, sinfo.msglen, 0,
			   (struct sockaddr *)&from, fromlen)) == -1) {
	  pmsg(EV_FAIL_SENDTO, sinfo.ses ? sinfo.ses->clip : clipbuf,
	       sinfo.ses ? sinfo.ses->clport : atoi(clportbuf), strerror(errno));
	}
	if (sinfo.ses && sinfo.ses->fin == IW_TRUE) {
	  pmsg(I_TFTPTRANS_FIN, sinfo.ses->filename, sinfo.ses->clip, sinfo.ses->clport);
	}
	
	DBG_SH_SEND(slen, sendsock, (sinfo.ses ? sinfo.ses->clip : clipbuf),
		    (sinfo.ses ? sinfo.ses->clport : atoi(clportbuf)));
      }

      /* update epoll event */
      if ((nevents = update_event(epollfd, ins->seshead)) == IW_ERR) {
	pmsg(E_FAIL_UPDATE_EVENT);
      }
      break;
    }

    /* if needed, to resend */
    resend_allsession(ins->seshead, ins->ads);

    /* clean up finished sessions */
    cleanup_session(&ins->seshead);
    DBG_SH_DSALLDSESSION(ins->ads);
  }

  del_allsession(&ins->seshead);
  close(epollfd);
  for (i = 0; i < SVSOCKS_MAX; i++) {
    if (ins->svsocks[i] != -1) {
      close(ins->svsocks[i]);
      ins->svsocks[i] = -1;
    }
  }
  return IW_OK;

 err:
  del_allsession(&ins->seshead);
  if (epollfd != -1)
    close(epollfd);
  for (i = 0; i < SVSOCKS_MAX; i++) {
    if (ins->svsocks[i] != -1) {
      close(ins->svsocks[i]);
      ins->svsocks[i] = -1;
    }
  }
  return IW_ERR;
}

/* -------------------- */
/*  Internal functions  */
/* -------------------- */

static void
pmsg(int32_t statcode, ...)
{
  va_list pargs;
  char buf[IW_LOGMSGLEN_MAX];
  size_t mlen = 0;
  struct iwstatus *pm;
  
  if (statcode < 64) {
    pm = iwtftp_statmsgs;
  }
  else {
    if (iwlog_is_verbose() == IW_TRUE) {
      pm = iwtftp_statvmsgs;
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
    strcpy(buf, "[iwtftp] ");
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


/* for socket */
/* ---------- */
static int32_t
get_ifaddress(const char *ifname, struct ifinet *iaddr)
{
  struct ifaddrs *ifa;
  struct ifaddrs *pm;
  int32_t iffound;
  size_t ipv6len;
  
  memset(iaddr, 0, sizeof(struct ifinet));
  
  if (getifaddrs(&ifa) == -1) {
    pmsg(EV_FAIL_GETIFADDRS, ifname, strerror(errno));
    goto err;
  }

  /* check ifname */
  for (pm = ifa, iffound = IW_FALSE; pm; pm = pm->ifa_next) {
    if (strcmp(pm->ifa_name, ifname) == 0) {
      iffound = IW_TRUE;
      break;
    }
  }
  if (iffound == IW_FALSE) {
    pmsg(E_IF_NOTFOUND, ifname);
    goto err;
  }

  /* get ip addresses */
  for (pm = ifa; pm; pm = pm->ifa_next) {
    if (pm->ifa_addr && strcmp(pm->ifa_name, ifname) == 0) {

      /* ipv4 */
      if (pm->ifa_addr->sa_family == AF_INET) {
	if (! inet_ntop(pm->ifa_addr->sa_family, &((struct sockaddr_in *)pm->ifa_addr)->sin_addr,
			iaddr->ipv4addr, sizeof iaddr->ipv4addr)) {
	  pmsg(EV_FAIL_INET_NTOP, 4, ifname, strerror(errno));
	  goto err;
	}
      }

      /* ipv6 */
      if (pm->ifa_addr->sa_family == AF_INET6) {
	if (! inet_ntop(pm->ifa_addr->sa_family, &((struct sockaddr_in6 *)pm->ifa_addr)->sin6_addr,
			iaddr->ipv6addr, sizeof iaddr->ipv6addr)) {

	  pmsg(EV_FAIL_INET_NTOP, 6, ifname, strerror(errno));
	  goto err;
	}

	ipv6len = strlen(iaddr->ipv6addr);
	iaddr->ipv6addr[ipv6len++] = '%';
	strncpy(iaddr->ipv6addr + ipv6len, ifname, sizeof iaddr->ipv6addr - ipv6len - 1);
      }
    }
  }

  freeifaddrs(ifa);
  return IW_OK;

 err:
  if (ifa) freeifaddrs(ifa);
  return IW_ERR;
}


static int
create_socket(int family, const char *ip, const char *service)
{
  DBG_PRINT(DBG_CREATE_SOCKET);
  struct addrinfo hints;
  struct addrinfo *res = NULL;
  int sock;
  int opt;
  int ecode;
  
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = family;
  hints.ai_socktype = SOCK_DGRAM;

  if (strcmp(ip, "ANY") == 0) {
    ip = NULL;
  }
  if ((ecode = getaddrinfo(ip, service, &hints, &res)) != 0) {
    pmsg(EV_FAIL_GETADDRINFO, gai_strerror(ecode));
    goto err;
  }

  if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
    pmsg(EV_FAIL_SOCKET, strerror(errno));
    goto err;
  }

  opt = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) == -1) {
    pmsg(EV_FAIL_SETSOCKOPT, strerror(errno));
    goto err;
  }

  if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {
    pmsg(EV_FAIL_BIND, strerror(errno));
    goto err;
  }
  
  freeaddrinfo(res);

  DBG_SH_SOCKET(sock, ip, service);
  return sock;

 err:
  if (sock != -1) close(sock);
  if (res) freeaddrinfo(res);
  return IW_ERR;
}


static int32_t
update_event(int epollfd, struct session *head)
{
  DBG_PRINT(DBG_UPDATE_EVENT);
  struct epoll_event setev;
  struct session *pm;
  int32_t nregev;
  int32_t err = IW_FALSE;

  for (pm = head; pm; pm = pm->next) {
    if (pm->disabled == IW_FALSE) {
      if (pm->regevent == IW_FALSE) {
	setev.data.fd = pm->clsock;
	setev.events = EPOLLIN;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pm->clsock, &setev) == -1) {
	  pmsg(EV_FAIL_EPOLL_CTL, "add", pm->clip, pm->clport, strerror(errno));
	  err = IW_TRUE;
	}
	pm->regevent = IW_TRUE;

	DBG_SH_ADDEVENT(pm->clip, pm->clport);
      }
    }
    else {
      if (pm->regevent == IW_TRUE) {
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, pm->clsock, NULL) == -1) {
	  pmsg(EV_FAIL_EPOLL_CTL, "del", pm->clip, pm->clport, strerror(errno));
	  err = IW_TRUE;
	}
	pm->regevent = IW_FALSE;

	DBG_SH_DELEVENT(pm->clip, pm->clport);
      }
    }
  }

  for (nregev = 0, pm = head; pm; pm = pm->next) {
    if (pm->regevent == IW_TRUE)
      nregev++;
  }

  return err == IW_TRUE ? IW_ERR : nregev;
}


/* TFTP proccess */
/* ------------- */
static int32_t
tftp_proc(IWTFTP *ins, int sock, const char *clip, uint16_t clport,
	  void *dbuf, size_t dlen, struct sendinfo *sinfo)
{
  DBG_PRINT(DBG_TFTP_PROC);
  struct session *clses = NULL;
  uint16_t opcode;
  uint16_t optmp;
  struct tftpreq reqmsg;
  struct tftpdata datmsg;
  struct tftpack ackmsg;
  struct tftperror errmsg;
  uint16_t tftperrcode;
  char emsgbuf[TFTP_EMSGLEN_MAX];
  
  memset(emsgbuf, 0, sizeof emsgbuf);

  /* get opcode */
  memcpy(&optmp, dbuf, sizeof(uint16_t));
  opcode = ntohs(optmp);

  DBG_SH_OPCODE(opcode);
  
  /* retrieve session */
  DBG_SH_QUERY(clip, clport);
  clses = get_session(ins->seshead, clip, clport);

  DBG_SH_SESSION(clses);

  switch (opcode) {
  case OP_RRQ:
  case OP_WRQ:
    if (clses) {
      goto resend;
    }

    /* new request */
    if (parse_tftpreq(&reqmsg, dbuf, dlen) == IW_ERR) {
      pmsg(I_TFTPREQ_INCORRECT, clip, clport);
      tftperrcode = TFTP_ERR_ILLEGALOPE;
      goto errsend;
    }

    pmsg(opcode == OP_RRQ ? I_TFTPREQ_GET : I_TFTPREQ_PUT, reqmsg.filename, clip, clport);

    DBG_PRINT(DBG_CHECK_FILE);
    switch(iwds_isfile(ins->ads, reqmsg.filename)) {
    case IW_TRUE:
      if (opcode == OP_WRQ) {
	pmsg(I_FILE_EXIST, reqmsg.filename);
	tftperrcode = TFTP_ERR_FILEEXIST;
	goto errsend;
      }
      /* OP_RRQ -> OK */
      break;
    case IW_FALSE:
      if (opcode == OP_RRQ) {
	pmsg(I_FILE_NOTFOUND, reqmsg.filename);
	tftperrcode = TFTP_ERR_NOTFOUND;
	goto errsend;
      }
      /* OP_WRQ -> OK */
      break;
    }
    
    if (! (clses = add_newsession(&ins->seshead, sock, clip, clport, reqmsg.filename))) {
      pmsg(EV_FAIL_ADD_NEWSESSION, clip, clport);
      pmsg(E_SERVER_ERR);
      tftperrcode = TFTP_ERR_SEEMSG;
      snprintf(emsgbuf, sizeof emsgbuf, "server error");
      goto errsend;
    }

    if (opcode == OP_RRQ) {
      /* make TFTP DATA */
      if ((sinfo->msglen = make_tftpdata_msg(clses, ins->ads, sinfo->msgbuf, sizeof sinfo->msgbuf)) < 0) {
	pmsg(E_FAIL_LOADFILE, clses->filename);
	tftperrcode = TFTP_ERR_SEEMSG;
	snprintf(emsgbuf, sizeof emsgbuf, "server error");
	goto errsend;
      }
    }
    if (opcode == OP_WRQ) {
      /* make TFTP ACK */
      if ((sinfo->msglen = make_tftpack_msg(clses, 0, sinfo->msgbuf, sizeof sinfo->msgbuf)) == IW_ERR) {
	pmsg(E_FAIL_MAKEACK, clip, clport);
	tftperrcode = TFTP_ERR_SEEMSG;
	snprintf(emsgbuf, sizeof emsgbuf, "server error");
	goto errsend;
      }
    }

    goto done;

  case OP_DATA:
    if (! clses) {
      tftperrcode = TFTP_ERR_UNKNOWNID;
      goto errsend;
    }

    if (parse_tftpdata(&datmsg, dbuf, dlen) == IW_ERR) {
      pmsg(I_TFTPDATA_INCORRECT, clip, clport);
      tftperrcode = TFTP_ERR_ILLEGALOPE;
      goto errsend;
    }

    if (ntohs(*datmsg.blknum) == clses->blknum) {
      goto resend;
    }
    else if (ntohs(*datmsg.blknum) == (clses->blknum < TFTP_BLKNUM_MAX ? clses->blknum + 1 : 0)) {
      switch (save_data(clses, ins->ads, datmsg.data, dlen - sizeof(uint16_t) * 2)) {
      case IW_ERR:
	pmsg(E_FAIL_SAVEFILE, clses->filename);
	tftperrcode = TFTP_ERR_ACCESSDENY;
	goto errsend;
	break;
      case 0:
	DBG_PRINT(DBG_SET_FIN);
	clses->fin = IW_TRUE;
	break;
      default:
	if (dlen - sizeof(uint16_t) * 2 < TFTP_DATALEN_MAX) {
	  DBG_PRINT(DBG_SET_FIN);
	  clses->fin = IW_TRUE;
	  close_data(clses, ins->ads);
	}
      }

      /* make TFTP ACK */
      if ((sinfo->msglen = make_tftpack_msg(clses, ntohs(*datmsg.blknum),
					    sinfo->msgbuf, sizeof sinfo->msgbuf)) == IW_ERR) {
	pmsg(E_FAIL_MAKEACK, clip, clport);
	tftperrcode = TFTP_ERR_SEEMSG;
	snprintf(emsgbuf, sizeof emsgbuf, "server error");
	goto errsend;
      }
    }
    else {
      pmsg(I_INVALID_BLKNUM, "DATA", clip, clport);
      tftperrcode = TFTP_ERR_ILLEGALOPE;
      goto errsend;
    }

    goto done;

  case OP_ACK:
    if (! clses) {
      tftperrcode = TFTP_ERR_UNKNOWNID;
      goto errsend;
    }

    if (parse_tftpack(&ackmsg, dbuf, dlen) == IW_ERR) {
      pmsg(I_TFTPACK_INCORRECT, clip, clport);
      tftperrcode = TFTP_ERR_ILLEGALOPE;
      goto errsend;
    }

    if (ntohs(*ackmsg.blknum) == clses->blknum -1) {
      goto resend;
    }
    else if (ntohs(*ackmsg.blknum) == clses->blknum) {
      if (clses->fin == IW_TRUE) {
	/* finished */
	DBG_PRINT(DBG_SET_DISABLE);
	clses->disabled = IW_TRUE;
	close_data(clses, ins->ads);
	goto done;
      }

      /* make TFTP DATA */
      if ((sinfo->msglen = make_tftpdata_msg(clses, ins->ads, sinfo->msgbuf, sizeof sinfo->msgbuf)) < 0) {
	pmsg(E_FAIL_LOADFILE, clses->filename);
	tftperrcode = TFTP_ERR_SEEMSG;
	snprintf(emsgbuf, sizeof emsgbuf, "server error");
	goto errsend;
      }
      
      if (sinfo->msglen - sizeof(uint16_t) * 2 < TFTP_DATALEN_MAX) {
	clses->fin = IW_TRUE;
      }
    }
    else {
      pmsg(I_INVALID_BLKNUM, "ACK", clip, clport);
    }

    goto done;

  case OP_ERROR:
    if (! clses) {
      pmsg(I_INVALID_TFTPMSG, "ERROR", clip, clport);
      goto err;
    }
    
    if (parse_tftperror(&errmsg, dbuf, dlen) == IW_ERR) {
      pmsg(I_TFTPERROR_INCORRECT, clip, clport);
    }
    else {
      pmsg(I_TFTPERROR_RECV, ntohs(*errmsg.errcode), errmsg.errmsg, clip, clport);
    }

    DBG_PRINT(DBG_SET_DISABLE);
    clses->disabled = IW_TRUE;
    close_data(clses, ins->ads);
    goto done;
    
  default:
    pmsg(IV_UNKNOWN_MSG, clip, clport);
    goto done;
  }

 done:
  sinfo->ses = clses;
  return IW_OK;

 resend:
  /* check resend count */
  if (clses->retrycount < RESEND_COUNTMAX) {
    DBG_PRINT(DBG_PREPARE_RESEND);
    memcpy(sinfo->msgbuf, clses->lastmsg, clses->lastmsglen);
    sinfo->msglen = clses->lastmsglen;
    clses->lastsending = time(NULL);
    clses->retrycount += 1;
    DBG_SH_SESSION(clses);
  }
  else {
    DBG_PRINT(DBG_SET_DISABLE);
    clses->disabled = IW_TRUE;
    close_data(clses, ins->ads);
  }
  sinfo->ses = clses;
  return IW_OK;

 errsend:
  /* make TFTP ERROR */
  if (clses) {
    clses->fin = IW_TRUE;
  }
  
  if ((sinfo->msglen = make_tftperr_msg(tftperrcode, sinfo->msgbuf, sizeof sinfo->msgbuf,
					emsgbuf, strlen(emsgbuf))) == IW_ERR) {
    pmsg(E_FAIL_MAKEERROR, clip, clport);
    goto err;
  }
  sinfo->ses = clses;
  return IW_OK;
  
 err:
  /* could not send TFTP ERROR */
  return IW_ERR;
}


static int32_t
resend_allsession(struct session *head, IWDS *ads)
{
  DBG_PRINT(DBG_RESEND_SESSION);
  struct session *pm;
  int32_t diff;
  struct sockaddr_storage addr;
  socklen_t addrlen;
  ssize_t slen;

  addrlen = sizeof addr;
  
  for (pm = head; pm; pm = pm->next) {
    diff = (int32_t)difftime(time(NULL), pm->lastsending);

    if (diff <=  RESEND_INTERVAL)
      continue;
    
    if (pm->fin == IW_TRUE || pm->disabled == IW_TRUE)
      continue;

    if (pm->retrycount >= RESEND_COUNTMAX) {
      pm->disabled = IW_TRUE;
      close_data(pm, ads);
      continue;
    }

    if (getsockname(pm->clsock, (struct sockaddr *)&addr, &addrlen) == -1) {
      pmsg(EV_FAIL_GETSOCKNAME, strerror(errno));
      pmsg(E_FAIL_RESEND, pm->clip, pm->clport);
      continue;
    }

    DBG_PRINT(DBG_UPDATE_RETRY);

    pm->retrycount += 1;
    pm->lastsending = time(NULL);

    DBG_SH_SESSION(pm);
    DBG_PRINT(DBG_SEND_SESSION);

    if ((slen = sendto(pm->clsock, pm->lastmsg, pm->lastmsglen, 0,
		       (struct sockaddr *)&addr, addrlen)) == -1) {
      pmsg(EV_FAIL_SENDTO, pm->clip, pm->clport, strerror(errno));
      pmsg(E_FAIL_RESEND, pm->clip, pm->clport);
      continue;
    }

    DBG_SH_SEND(slen, pm->clsock, pm->clip, pm->clport);
  }

  return IW_OK;
}


/* for sessions */
/* ------------ */
struct session *
add_newsession(struct session **phead, int32_t svsock, const char *clip, uint16_t clport, const char *file)
{
  DBG_PRINT(DBG_ADD_SESSION);
  struct session *clses = NULL;
  struct sockaddr_storage svaddr;
  socklen_t svaddrlen;
  char svip[NI_MAXHOST];
  int ecode;

  if (! (clses = create_session(phead))) {
    pmsg(EV_FAIL_CREATE_SESSION);
    goto err;
  }

  strncpy(clses->clip, clip, sizeof clses->clip - 1);

  clses->clport = clport;

  strncpy(clses->filename, file, sizeof clses->filename - 1);

  svaddrlen = sizeof svaddr;
  if (getsockname(svsock, (struct sockaddr *)&svaddr, &svaddrlen) == -1) {
    pmsg(EV_FAIL_GETSOCKNAME, strerror(errno));
    goto err;
  }
  if ((ecode = getnameinfo((struct sockaddr *)&svaddr, svaddrlen,
			   svip, sizeof svip, NULL, 0, NI_NUMERICHOST)) != 0) {
    pmsg(EV_FAIL_GETNAMEINFO, gai_strerror(ecode));
    goto err;
  }

  /* client socket */
  if ((clses->clsock = create_socket(svaddr.ss_family, svip, NULL)) == IW_ERR) {
    pmsg(E_FAIL_CREATE_SOCKET, svaddr.ss_family == AF_INET ? 4 : 6, svip, "ANY");
    goto err;
  }

  DBG_SH_SESSION(clses);
  return clses;

 err:
  return NULL;
}


static struct session *
create_session(struct session **phead)
{
  struct session *node;

  if (! (node = malloc(sizeof(struct session)))) {
    pmsg(E_FAIL_MALLOC, __FUNCTION__);
    goto err;
  }
  memset(node, 0, sizeof(struct session));
  node->clsock = -1;

  if (*phead) {
    node->next = *phead;
    (*phead)->prev = node;
  }
  *phead = node;

  return node;

 err:
  return NULL;
}


static struct session *
get_session(struct session *head, const char *clip, uint16_t clport)
{
  struct session *pm = NULL;

  if (! head)
    return NULL;

  for (pm = head; pm; pm = pm->next) {
    if (strcmp(pm->clip, clip) == 0 && pm->clport == clport) {
      break;
    }
  }

  return pm;
}


static void
close_data(struct session *clses, IWDS *ads)
{
  DBG_PRINT(DBG_CLOSE_DATA);
  struct dsreq dticket;

  /* close the file reading or writing */
  dticket.dsid = clses->clsock;
  dticket.dfile = clses->filename;
  dticket.dbuf = NULL;
  dticket.dlen = 0;
  dticket.derr = 0;

  if (iwds_close(ads, &dticket) == IW_ERR) {
    pmsg(E_DS_FAIL_CLOSE, iwds_strerr(dticket.derr));
  }
}


static void
del_session(struct session **phead, const char *clip, uint16_t clport)
{
  DBG_PRINT(DBG_DEL_SESSION);
  struct session *tmp;
  
  if (! (tmp = get_session(*phead, clip, clport))) {
    pmsg(EV_SESSION_NOTFOUND, clip, clport);
    return;
  }

  DBG_SH_SESSION(tmp);
  
  if (! tmp->prev && ! tmp->next) {
    *phead = NULL;
  }
  else if (! tmp->prev && tmp->next) {
    tmp->next->prev = NULL;
    *phead = tmp->next;
  }
  else if (tmp->prev && ! tmp->next) {
    tmp->prev->next = NULL;
  }
  else if (tmp->prev && tmp->next) {
    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;
  }

  close(tmp->clsock);
  free(tmp);
}


static void
del_allsession(struct session **phead)
{
  DBG_PRINT(DBG_DEL_ALLSESSION);
  struct session *pm;
  struct session *tmp;

  if (! (pm = *phead))
    return;
    
  while (pm) {
    tmp = pm->next;
    close(pm->clsock);
    free(pm);
    pm = tmp;
  }
  *phead = NULL;

  DBG_SH_ALLSESSION(*phead);
}


static void
cleanup_session(struct session **phead)
{
  DBG_PRINT(DBG_CLEANUP_SESSION);
  struct session *pm;
  int32_t diff;

  for (pm = *phead; pm; pm = pm->next) {
    if (pm->disabled == IW_TRUE) {
      del_session(phead, pm->clip, pm->clport);
    }
    else {
      if (pm->fin == IW_TRUE) {
	diff = (int32_t)difftime(time(NULL), pm->lastsending);

	if (diff > SESSION_CLOSEWAIT) {
	  del_session(phead, pm->clip, pm->clport);
	}
      }
    }
  }

  DBG_PRINT(DBG_REMAIN_SESSION);
  DBG_SH_ALLSESSION(*phead);
}


/* for TFTP REQ message */
/* -------------------- */
static int32_t
parse_tftpreq(struct tftpreq *req, void *msg, size_t msglen)
{
  DBG_PRINT(DBG_PARSE_TFTPREQ);

  int32_t passed;
  int32_t i;
  
  if (msglen < sizeof(uint16_t) + 2 + strlen("octet") + 1) {
    pmsg(IV_REQLEN_TOOSHORT);
    goto err;
  }
  
  req->opcode = msg;

  if (ntohs(*req->opcode) != OP_RRQ && ntohs(*req->opcode) != OP_WRQ) {
    pmsg(IV_OPCODE_INCORRECT);
    goto err;
  }

  req->filename = msg + sizeof(uint16_t);

  for (passed = IW_FALSE, i = 0; i < TFTP_FILENAME_MAX; i++) {
    if (req->filename[i] == '\0') {
      passed = IW_TRUE;
      break;
    }
  }
  if (! passed) {
    pmsg(IV_FILENAME_INCORRECT);
    goto err;
  }

  req->mode = msg + sizeof(uint16_t) + strlen(req->filename) + 1;

  for (passed = IW_FALSE, i = 0; i < TFTP_MODENAME_MAX; i++) {
    if (req->mode[i] == '\0') {
      passed = IW_TRUE;
      break;
    }
  }
  if (! passed) {
    pmsg(IV_MODE_INCORRECT);
    goto err;
  }

  DBG_SH_TFTPMSG(0, req, msglen);
  return IW_OK;

 err:
  return IW_ERR;
}


/* for TFTP DATA message */
/* --------------------- */
static ssize_t
make_tftpdata_msg(struct session *clses, IWDS *ads, void *emptybuf, size_t bufsize)
{
  DBG_PRINT(DBG_MAKE_TFTPDATA);
  struct tftpdata datmsg;
  struct dsreq dticket;
  size_t datalen;
  ssize_t msglen;

  if (bufsize < TFTP_MSGLEN_MAX) {
    pmsg(EV_BUF_TOOSHORT, "data");
    goto err;
  }

  datmsg.opcode = emptybuf;
  datmsg.blknum = emptybuf + sizeof(uint16_t);
  datmsg.data = emptybuf + sizeof(uint16_t) + sizeof(uint16_t);

  *datmsg.opcode = htons(OP_DATA);

  clses->blknum = (clses->blknum < TFTP_BLKNUM_MAX ? clses->blknum + 1 : 0);
  *datmsg.blknum = htons(clses->blknum);

  DBG_PRINT(DBG_DS_SETREQ);

  /* load data from datastore */
  dticket.dsid = clses->clsock;
  dticket.dfile = clses->filename;
  dticket.dbuf = datmsg.data;
  dticket.dlen = TFTP_DATALEN_MAX;
  dticket.derr = 0;

  DBG_SH_DSREQ(dticket);
  DBG_PRINT(DBG_DS_LOAD);

  datalen = iwds_read(ads, &dticket);
  if (dticket.derr) {
    pmsg(E_DS_FAIL_READ, iwds_strerr(dticket.derr));
    goto err;
  }

  DBG_SH_DSIOLEN(datalen);
  DBG_SH_DSREQ(dticket);

  if (datalen < dticket.dlen) {
    /* EOF */
    clses->fin = IW_TRUE;
  }

  msglen = sizeof(uint16_t) * 2 + datalen;

  memcpy(clses->lastmsg, emptybuf, msglen);
  
  clses->lastmsglen = msglen;
  clses->lastsending = time(NULL);
  clses->retrycount = 0;

  DBG_SH_TFTPMSG(OP_DATA, &datmsg, msglen);
  return msglen;

 err:
  return IW_ERR;
}


static int32_t
parse_tftpdata(struct tftpdata *dat, void *msg, size_t msglen)
{
  DBG_PRINT(DBG_PARSE_TFTPDATA);
 
  if (msglen < sizeof(uint16_t) * 2 || msglen - sizeof(uint16_t) * 2 > TFTP_DATALEN_MAX) {
    pmsg(IV_DATA_INCORRECT);
    goto err;
  }

  dat->opcode = msg;
  dat->blknum = msg + sizeof(uint16_t);

  if (msglen == sizeof(uint16_t) * 2) {
    dat->data = NULL;
  }
  else {
    dat->data = msg + sizeof(uint16_t) + sizeof(uint16_t);
  }

  if (ntohs(*dat->opcode) != OP_DATA) {
    pmsg(IV_OPCODE_INCORRECT);
    goto err;
  }

  DBG_SH_TFTPMSG(OP_DATA, dat, msglen);
  return IW_OK;

 err:
  return IW_ERR;
}


static ssize_t
save_data(struct session *clses, IWDS *ads, void *data, size_t datalen)
{
  struct dsreq dticket;
  size_t wlen;

  DBG_PRINT(DBG_DS_SETREQ);
  /* save data to datastore */
  dticket.dsid = clses->clsock;
  dticket.dfile = clses->filename;
  dticket.dbuf = data;
  if (! data) {
    dticket.dlen = 0;		/* EOF */
  }
  else {
    dticket.dlen = datalen;
  }
  dticket.derr = 0;

  DBG_SH_DSREQ(dticket);
  DBG_PRINT(DBG_DS_SAVE);
  
  wlen = iwds_write(ads, &dticket);
  if (dticket.derr) {
    pmsg(E_DS_FAIL_WRITE, iwds_strerr(dticket.derr));
    goto err;
  }

  DBG_SH_DSIOLEN(wlen);
  DBG_SH_DSREQ(dticket);
  return wlen;

 err:
  return IW_ERR;
}


/* for TFTP ACK message */
/* -------------------- */
static ssize_t
make_tftpack_msg(struct session *clses, uint16_t blk, void *emptybuf, size_t bufsize)
{
  DBG_PRINT(DBG_MAKE_TFTPACK);
  struct tftpack ackmsg;
  ssize_t msglen;
  
  if (bufsize < TFTP_OPCODE_SIZE + TFTP_BLKNUM_SIZE) {
    pmsg(EV_BUF_TOOSHORT, "ack");
    goto err;
  }

  ackmsg.opcode = emptybuf;
  ackmsg.blknum = emptybuf + sizeof(uint16_t);

  *ackmsg.opcode = htons(OP_ACK);

  clses->blknum = blk;
  *ackmsg.blknum = htons(clses->blknum);

  msglen = sizeof(uint16_t) * 2;

  memcpy(clses->lastmsg, emptybuf, msglen);
  
  clses->lastmsglen = msglen;
  clses->lastsending = time(NULL);
  clses->retrycount = 0;

  DBG_SH_TFTPMSG(OP_ACK, &ackmsg, msglen);
  return msglen;

 err:
  return IW_ERR;
}


static int32_t
parse_tftpack(struct tftpack *ack, void *msg, size_t msglen)
{
  DBG_PRINT(DBG_PARSE_TFTPACK);
  
  if (msglen != sizeof(uint16_t) * 2) {
    pmsg(IV_ACK_INCORRECT);
    goto err;
  }

  ack->opcode = msg;

  if (ntohs(*ack->opcode) != OP_ACK) {
    pmsg(IV_OPCODE_INCORRECT);
    goto err;
  }

  ack->blknum = msg + sizeof(uint16_t);

  DBG_SH_TFTPMSG(OP_ACK, ack, msglen);
  return IW_OK;

 err:
  return IW_ERR;
}


/* for TFTP ERROR message */
/* ---------------------- */
static ssize_t
make_tftperr_msg(uint16_t ecode, void *emptybuf, size_t bufsize, const char *emsg, size_t emsglen)
{
  DBG_PRINT(DBG_MAKE_TFTPERROR);
  struct tftperror err;
  ssize_t msglen;

  err.opcode = emptybuf;
  err.errcode = emptybuf + sizeof(uint16_t);
  err.errmsg = emptybuf + sizeof(uint16_t) * 2;

  *err.opcode = htons(OP_ERROR);
  *err.errcode = htons(ecode);

  if (ecode != TFTP_ERR_SEEMSG) {
    emsg = errmsgs[ecode];
    emsglen = strlen(errmsgs[ecode]);
  }

  if (bufsize - (sizeof(uint16_t) * 2) < emsglen + 1) {
    pmsg(EV_ERRMSG_TOOLONG);
    goto err;
  }

  memcpy(err.errmsg, emsg, emsglen + 1);  
  msglen = sizeof(uint16_t) * 2 + emsglen + 1;

  DBG_SH_TFTPMSG(OP_ERROR, &err, msglen);
  return msglen;

 err:
  return IW_ERR;
}


static int32_t
parse_tftperror(struct tftperror *terr, void *msg, size_t msglen)
{
  DBG_PRINT(DBG_PARSE_TFTPERROR);
  int32_t passed;
  int32_t i;

  if (msglen < sizeof(uint16_t) * 2 + 1) {
    pmsg(IV_ERRORLEN_TOOSHORT);
    goto err;
  }
  
  terr->opcode = msg;

  if (ntohs(*terr->opcode) != OP_ERROR) {
    pmsg(IV_OPCODE_INCORRECT);
    goto err;
  }

  terr->errcode = msg + sizeof(uint16_t);
  terr->errmsg = msg + sizeof(uint16_t) * 2;

  for (passed = IW_FALSE, i = 0; i < TFTP_EMSGLEN_MAX; i++) {
    if (terr->errmsg[i] == '\0') {
      passed = IW_TRUE;
      break;
    }
  }
  if (! passed) {
    pmsg(IV_ERRMSG_INCORRECT);
    goto err;
  }

  DBG_SH_TFTPMSG(OP_ERROR, terr, msglen);
  return IW_OK;

 err:
  return IW_ERR;
}



/* for debugging */
#ifdef DEBUG
static void
dbg_show_session(struct session *ses)
{
  int32_t diff;

  if (! ses) {
    pmsg(DBG_SESSION_EMPTY);
    return;
  }

  diff = ses->lastsending > 0 ? difftime(time(NULL), ses->lastsending) : 0;
  pmsg(DBG_SESSION_ONE, ses->clip, ses->clport, ses->clsock, ses->regevent, ses->filename, ses->blknum,
       ses->fin, ses->lastmsglen, diff, ses->retrycount, ses->disabled);

}

static void
dbg_show_allsession(struct session *head)
{
  struct session *pm;
  int32_t diff;
  int32_t i;

  if (! head) {
    pmsg(DBG_SESSION_EMPTY);
    return;
  }

  for (i = 1, pm = head; pm; pm = pm->next, i++) {
    diff = pm->lastsending > 0 ? difftime(time(NULL), pm->lastsending) : 0;
    pmsg(DBG_SESSION_ALL, i, pm->clip, pm->clport, pm->clsock, pm->regevent, pm->filename, pm->blknum,
	 pm->fin, pm->lastmsglen, diff, pm->retrycount, pm->disabled);
  }
}

static void
dbg_show_opcode(int32_t code)
{
  char *s_opcode[] = { "RRQ", "WRQ", "DATA", "ACK", "ERROR" };

  pmsg(DBG_OPCODE, s_opcode[code - 1]);
}

static void
dbg_show_tftpmsg(int32_t sw, void *ptr, size_t msglen)
{
  struct tftpreq *r;
  struct tftpdata *d;
  struct tftpack *a;
  struct tftperror *e;
  
  switch (sw) {
  case 0:
    r = ptr;
    pmsg(DBG_TFTPREQ, msglen, ntohs(*r->opcode), r->filename, r->mode);
    break;
  case OP_DATA:
    d = ptr;
    pmsg(DBG_TFTPDATA, msglen, ntohs(*d->opcode), ntohs(*d->blknum), d->data ? "***" : "null");
    break;
  case OP_ACK:
    a = ptr;
    pmsg(DBG_TFTPACK, msglen, ntohs(*a->opcode), ntohs(*a->blknum));
    break;
  case OP_ERROR:
    e = ptr;
    pmsg(DBG_TFTPERROR, msglen, ntohs(*e->opcode), ntohs(*e->errcode), e->errmsg, strlen(e->errmsg));
    break;
  }
}
#endif	/* DEBUG */

