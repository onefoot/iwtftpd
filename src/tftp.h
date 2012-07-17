/*
 * tftp.h
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

#ifndef _TFTP_H_
#define _TFTP_H_

#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "iw_common.h"
#include "iw_log.h"
#include "iw_ds.h"
#include "iw_tftp.h"


/* constants */
#define TFTP_PORT "69"		                        /* port number */
#define SVSOCKS_MAX 2					/* maximum number of server sockets */
#define CLSOCKS_MAX 32					/* maximum number of client sockets */
#define IPV4_ADDR_SIZE INET_ADDRSTRLEN			/* length of IPv4 address string (=16) */
#define IPV6_ADDR_SIZE (INET6_ADDRSTRLEN + IF_NAMESIZE) /* length of IPv6 address string (=46 + 16) */
#define IPADDRLEN_MAX IPV6_ADDR_SIZE			/* maximum length of IP address string */
#define EVENTS_MAX (SVSOCKS_MAX + CLSOCKS_MAX)		/* maximum number of epoll events */
#define BLOCKING_TIMEOUT 1000				/* epoll_wait timeout (msec) */
#define RESEND_INTERVAL 10				/* interval of resending (sec) */
#define RESEND_COUNTMAX 3				/* maximum number of resending counts */
#define SESSION_CLOSEWAIT 15				/* time of waiting for closing the finished session */
#define NWBUF_SIZE 1024					/* size of buffer for send/recv */
#define SESSION_BUFSIZE 8192	                        /* size of the session buffer */

/* for TFTP protocol */
#define TFTP_OPCODE_SIZE 2	               /* size of Opcode field (bytes) */
#define TFTP_BLKNUM_SIZE 2		       /* size of Block field (bytes) */
#define TFTP_BLKNUM_MAX USHRT_MAX	       /* maximum block number */
#define TFTP_FILENAME_MAX 256		       /* maximum size of Filename field (bytes) */
#define TFTP_MODENAME_MAX 9		       /* maximum size of Mode filed (bytes) */
#define TFTP_DATALEN_MAX 512		       /* maximum size of Data filed (bytes) */
#define TFTP_ECODE_SIZE 2		       /* size of ErrorCode field (bytes) */
#define TFTP_EMSGLEN_MAX 256		       /* maximum size of ErrMsg filed (bytes) */
/* maximum length of the TFTP message (bytes) */
#define TFTP_MSGLEN_MAX (TFTP_OPCODE_SIZE + TFTP_BLKNUM_SIZE + TFTP_DATALEN_MAX)

/* check bool of TFTP mode */
#define IS_NETASCII(m) (strcmp((m), "netascii") == 0 ? IW_TRUE : IW_FALSE)
#define IS_OCTET(m) (strcmp((m), "octet") == 0 ? IW_TRUE : IW_FALSE)


/* iwtftp object */
struct _iwtftp {
  int svsocks[SVSOCKS_MAX];	       /* sever sockets (IPv4/IPv6) */
  IWDS *ads;			       /* pointer to iwds module */
  struct session *seshead;	       /* head of the session list */
};

/* TFTP modes */
enum TFTP_MODE {
  TFTP_MODE_NETASCII,
  TFTP_MODE_OCTET
};

/* session */
struct session {
  struct session *next;
  struct session *prev;
  char clip[IPADDRLEN_MAX];	       /* client IP address  */
  uint16_t clport;		       /* client port number */
  int clsock;			       /* client socket */
  int32_t regevent;	               /* flag of whether epoll event is registered */
  enum TFTP_MODE tftpmode;	       /* TFTP transfer mode */
  struct datastorage *sesbuf;	       /* data buffer of this session */
  char filename[TFTP_FILENAME_MAX];    /* requested file */
  uint16_t blknum;		       /* last block number */
  int32_t fin;		               /* flag of whether transfer is finished */
  uint8_t lastmsg[TFTP_MSGLEN_MAX];    /* last message */
  size_t lastmsglen;		       /* length of last message */
  time_t lastsending;		       /* time of last sending */
  int32_t retrycount;		       /* count of resending */
  int32_t disabled;		       /* flag of session discard */
};

/* data storage for the session */
struct datastorage {
  uint8_t *pos;			       /* position indicator of the data buffer */
  int32_t fopt;			       /* flag of option */
  size_t datalen;		       /* length of data */
  uint8_t storage[SESSION_BUFSIZE];   /* storage area of data */
};

/* IP addresses on the network interface */
struct ifinet {
  char ipv4addr[IPV4_ADDR_SIZE];       /* xxx.xxx.xxx.xxx */
  char ipv6addr[IPV6_ADDR_SIZE];       /* xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx%eth0 */
};

/* information for sending */
struct sendinfo {
  struct session *ses;		       /* pointer to the session */
  uint8_t msgbuf[TFTP_MSGLEN_MAX];     /* message buffer */
  ssize_t msglen;		       /* length of message */
};

/* flag for logging */
extern int32_t g_iw_logready;


/* TFTP opcodes */
enum TFTP_OPCODE {
  OP_RRQ = 1,
  OP_WRQ = 2,
  OP_DATA = 3,
  OP_ACK = 4,
  OP_ERROR = 5
};

/* TFTP error codes */
enum TFTP_ERRORCODE {
  TFTP_ERR_SEEMSG = 0,
  TFTP_ERR_NOTFOUND,
  TFTP_ERR_ACCESSDENY,
  TFTP_ERR_DISKFULL,
  TFTP_ERR_ILLEGALOPE,
  TFTP_ERR_UNKNOWNID,
  TFTP_ERR_FILEEXIST,
  TFTP_ERR_NOUSER
};

/* TFTP request format */
struct tftpreq {
  uint16_t *opcode;
  char *filename;
  char *mode;
};

/* TFTP DATA format */
struct tftpdata {
  uint16_t *opcode;
  uint16_t *blknum;
  uint8_t *data;
};

/* TFTP ACK format */
struct tftpack {
  uint16_t *opcode;
  uint16_t *blknum;
};

/* TFTP ERROR format */
struct tftperror {
  uint16_t *opcode;
  uint16_t *errcode;
  char *errmsg;
};


/* status codes */
enum T_STATCODE {
  /* error */
  E_DS_FAIL_READ = 1,
  E_DS_FAIL_WRITE,
  E_DS_FAIL_CLOSE,
  E_EVENT_NOTEXIST,
  E_FAIL_ADDRCONVERT,
  E_FAIL_CREATE_SOCKET,
  E_FAIL_GET_IFADDRESS,
  E_FAIL_LOADFILE,
  E_FAIL_MAKEACK,
  E_FAIL_MAKEERROR,
  E_FAIL_MALLOC,
  E_FAIL_RECV,
  E_FAIL_RESEND,
  E_FAIL_SAVEFILE,
  E_FAIL_TFTP_PROC,
  E_FAIL_UPDATE_EVENT,
  E_IF_NOADDR,
  E_IF_NOTFOUND,
  E_SERVER_ERR,       
  /* info */
  I_FILE_EXIST,
  I_FILE_NOTFOUND,
  I_INVALID_BLKNUM,
  I_INVALID_TFTPMSG,
  I_TFTPACK_INCORRECT,
  I_TFTPDATA_INCORRECT,
  I_TFTPERROR_INCORRECT,
  I_TFTPERROR_RECV,
  I_TFTPREQ_GET,
  I_TFTPREQ_INCORRECT,
  I_TFTPREQ_PUT,
  I_TFTPTRANS_FIN,      
};

enum T_STATCODE_VERBOSE {
  /* error verbose */
  EV_BUF_TOOSHORT = 65,
  EV_ERRMSG_TOOLONG,
  EV_FAIL_ADD_NEWSESSION,
  EV_FAIL_BIND,
  EV_FAIL_CREATE_SESSION,
  EV_FAIL_EPOLL_CREATE,
  EV_FAIL_EPOLL_CTL,
  EV_FAIL_EPOLL_WAIT,
  EV_FAIL_GETADDRINFO,
  EV_FAIL_GETIFADDRS,
  EV_FAIL_GETNAMEINFO,
  EV_FAIL_GETSOCKNAME,
  EV_FAIL_INET_NTOP,
  EV_FAIL_RECVFROM,
  EV_FAIL_SENDTO,
  EV_FAIL_SETSOCKOPT,
  EV_FAIL_SOCKET,
  EV_NULL_OBJ,
  EV_SESSION_NOTFOUND,  
  EV_FAIL_GET_SESBUF,
  EV_FAIL_PUT_SESBUF,
  /* info verbose */
  IV_ACK_INCORRECT,
  IV_DATA_INCORRECT,
  IV_ERRMSG_INCORRECT,
  IV_ERRORLEN_TOOSHORT,
  IV_FILENAME_INCORRECT,
  IV_MODE_INCORRECT,
  IV_OPCODE_INCORRECT,
  IV_REQLEN_TOOSHORT,
  IV_UNKNOWN_MSG,        
#ifdef DEBUG
  DBG_ADD_EVENT,
  DBG_ADD_INITEVENT,
  DBG_ADD_SESSION,
  DBG_CHECK_FILE,
  DBG_CLEANUP_SESSION,
  DBG_CLOSE_DATA,
  DBG_CREATE_SOCKET,
  DBG_DEL_ALLSESSION,
  DBG_DEL_EVENT,
  DBG_DEL_SESSION,
  DBG_DS_IOLEN,
  DBG_DS_LOAD,
  DBG_DS_REQ,
  DBG_DS_SAVE,
  DBG_DS_SETREQ,
  DBG_MAKE_TFTPACK,
  DBG_MAKE_TFTPDATA,
  DBG_MAKE_TFTPERROR,
  DBG_OPCODE,
  DBG_PARSE_TFTPACK,
  DBG_PARSE_TFTPDATA,
  DBG_PARSE_TFTPERROR,
  DBG_PARSE_TFTPREQ,
  DBG_PREPARE_RESEND,
  DBG_RECV,
  DBG_REMAIN_SESSION,
  DBG_RESEND_SESSION,
  DBG_RETRIEVE_SESSION,
  DBG_SEND,
  DBG_SENDINFO,
  DBG_SEND_SESSION,
  DBG_SESSION_ALL,
  DBG_SESSION_EMPTY,
  DBG_SESSION_ONE,
  DBG_SET_DISABLE,
  DBG_SET_FIN,
  DBG_SOCKET,
  DBG_START_EVLOOP,
  DBG_START_TFTP,
  DBG_TFTPACK,
  DBG_TFTPDATA,
  DBG_TFTPERROR,
  DBG_TFTPREQ,
  DBG_TFTP_PROC,
  DBG_UPDATE_EVENT,
  DBG_UPDATE_RETRY,
  DBG_GET_SESBUF_DATA,
  DBG_PUT_SESBUF_DATA,
  DBG_SESBUF_IOLEN,
  DBG_SESBUF,
  DBG_SESBUF_EMPTY,
  DBG_SESBUF_FULLORFIN,
#endif	/* DEBUG */
};


/* function prototypes */
static void pmsg(int32_t statcode, ...);
static int32_t get_ifaddress(const char *ifname, struct ifinet *iaddr);
static int create_socket(int family, const char *ip, const char *service);
static int32_t update_event(int epollfd, struct session *head);
static int32_t tftp_proc(IWTFTP *ins, int sock, const char *clip, uint16_t clport,
		     void *dbuf, size_t dlen, struct sendinfo *sinfo);
static int32_t resend_allsession(struct session *head, IWDS *ads);
static struct session *add_newsession(struct session **phead, int svsock, const char *clip, uint16_t clport,
				      const char *file, const char *mode);
static struct session *create_session(struct session **phead);
static struct session *get_session(struct session *head, const char *clip, uint16_t clport);
static void del_session(struct session **phead, const char *clip, uint16_t clport);
static void del_allsession(struct session **phead);
static void cleanup_session(struct session **phead);
static int32_t parse_tftpreq(struct tftpreq *req, void *msg, size_t msglen);
static int32_t parse_tftpdata(struct tftpdata *dat, void *msg, size_t msglen);
static int32_t parse_tftpack(struct tftpack *ack, void *msg, size_t msglen);
static int32_t parse_tftperror(struct tftperror *terr, void *msg, size_t msglen);
static ssize_t make_tftpdata_msg(struct session *clses, IWDS *ads, void *emptybuf, size_t bufsize);
static ssize_t make_tftpack_msg(struct session *clses, uint16_t blk, void *emptybuf, size_t bufsize);
static ssize_t make_tftperr_msg(uint16_t ecode, void *emptybuf, size_t bufsize,
				const char *emsg, size_t emsglen);
static ssize_t get_session_data(struct session *clses, IWDS *ads, void *dstbuf, size_t dstbufsize);
static ssize_t put_session_data(struct session *clses, IWDS *ads, void *data, size_t datalen);
static size_t netascii_to_local(struct datastorage *sb, void *srcdata, size_t srclen);
static size_t local_to_netascii(void *dstbuf, size_t bufsize, struct datastorage *sb);
static int32_t load_data(struct session *clses, IWDS *ads);
static int32_t save_data(struct session *clses, IWDS *ads);
static void close_data(struct session *clses, IWDS *ads);


/* for debugging */
#ifdef DEBUG
#define DBG_PRINT(c) pmsg(c)
#define DBG_SH_ADDEVENT(ip, port) pmsg(DBG_ADD_EVENT, ip, port)
#define DBG_SH_ALLSESSION(hd) dbg_show_allsession(hd)
#define DBG_SH_DELEVENT(ip, port) pmsg(DBG_DEL_EVENT, ip, port)
#define DBG_SH_DSALLDSESSION(ds) dbg_ds_show_alldsession(ds)
#define DBG_SH_DSIOLEN(len) pmsg(DBG_DS_IOLEN, len)
#define DBG_SH_DSREQ(req) pmsg(DBG_DS_REQ, req.dsid, req.dfile, req.dbuf ? "***" : "null", req.dlen, req.derr)
#define DBG_SH_OPCODE(c) dbg_show_opcode(c)
#define DBG_SH_QUERY(ip, port) pmsg(DBG_RETRIEVE_SESSION, ip, port)
#define DBG_SH_RECV(len, so, ip, po) pmsg(DBG_RECV, len, so, ip, po)
#define DBG_SH_SEND(len, so, ip, po) pmsg(DBG_SEND, len, so, ip, po)
#define DBG_SH_SENDINFO(ss, len) pmsg(DBG_SENDINFO, ss, len)
#define DBG_SH_SESSION(ses) dbg_show_session(ses)
#define DBG_SH_SOCKET(so, ip, sv) pmsg(DBG_SOCKET, so, ip, sv)
#define DBG_SH_TFTPMSG(sw, m, len) dbg_show_tftpmsg(sw, m, len)
#define DBG_SH_SESBUF_IOLEN(len) pmsg(DBG_SESBUF_IOLEN, len)
#define DBG_SH_SESBUF(mode, len) pmsg(DBG_SESBUF, mode, len)


static void dbg_show_session(struct session *ses);
static void dbg_show_allsession(struct session *head);
static void dbg_show_opcode(int32_t code);
static void dbg_show_tftpmsg(int32_t sw, void *ptr, size_t msglen);
#else
#define DBG_PRINT(c)
#define DBG_SH_ADDEVENT(ip, port)
#define DBG_SH_ALLSESSION(hd)
#define DBG_SH_DELEVENT(ip, port)
#define DBG_SH_DSALLDSESSION(ds)
#define DBG_SH_DSIOLEN(len)
#define DBG_SH_DSREQ(req)
#define DBG_SH_OPCODE(c)
#define DBG_SH_QUERY(ip, port)
#define DBG_SH_RECV(so, ip, po, len)
#define DBG_SH_SEND(so, ip, po, len)
#define DBG_SH_SENDINFO(ss, len)
#define DBG_SH_SESSION(ses)
#define DBG_SH_SOCKET(so, ip, sv)
#define DBG_SH_TFTPMSG(sw, m, len)
#define DBG_SH_SESBUF_IOLEN(len)
#define DBG_SH_SESBUF(mode, len)

#endif	/* DEBUG */


#endif	/* _TFTP_H_ */
