/*
 * datastore.h
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

#ifndef _DATASTORE_H_
#define _DATASTORE_H_

#include <sys/stat.h>
#include <fcntl.h>

#include "iw_common.h"
#include "iw_log.h"
#include "util.h"
#include "iw_ds.h"


/* constants */
#define DEFAULT_DATASTORE "/tftpboot"     /* default path of the datastore */
#define MODE_READ 0x00000001		  /* I/O mode */
#define MODE_WRITE 0x00000002


/* iwds object */
struct _iwds {
  struct dsession *dhead;	/* head of the session list */
  char *dspath;			/* path of the datastore */
  int32_t fchroot;		/* flag of if chroot */
};

/* session on the datastore */
struct dsession {
  struct dsession *next;
  struct dsession *prev;
  int32_t clid;			/* session ID */
  FILE *fp;			/* file pointer */
  char *filename;		/* file path */
  int32_t derr;			/* error */
};

/* flag for logging */
extern int32_t g_iw_logready;


/* status codes */
enum D_STATCODE {
  /* error */
  E_DSPATH_INCORRECT = 1,
  E_FAIL_CHECKFILE,
  E_FAIL_FREAD,
  E_FAIL_FWRITE,
  E_FAIL_MALLOC,
  E_FILE_EXIST,
  E_FILE_NOTEXIST,
};

enum D_STATCODE_VERBOSE {
  /* error verbose */
  EV_DSESSION_NOTFOUND = 65,
  EV_FAIL_ADD_DSESSION,
  EV_FAIL_CREATE_DSESSION,
  EV_FAIL_FCLOSE,
  EV_FAIL_FOPEN,
  EV_FAIL_JOIN_PATH,
  EV_FAIL_REALPATH,
  EV_NODSREQ,
  EV_NULL_OBJ,            
#ifdef DEBUG
  /* debugging */
  DBG_READ_END,
  DBG_READ,
  DBG_FREAD,
  DBG_FWRITE,
  DBG_WRITE_END,
  DBG_RETRIEVE_DSESSION,
  DBG_DSREQ,
  DBG_WRITE,
  DBG_SET_CHROOT,
  DBG_CLEANUP_DSESSION,
  DBG_DSPATH,
  DBG_GET_DSPATH,
  DBG_CHECK_DSPATH,
  DBG_FILE_EXIST,
  DBG_FILE_NOTEXIST,
  DBG_CHECK_FILE,
  DBG_FSTAT,
  DBG_FOPEN,
  DBG_ADD_DSESSION,
  DBG_FCLOSE,
  DBG_DEL_DSESSION,
  DBG_REMAIN_DSESSION,
  DBG_DSESSION_EMPTY,
  DBG_DSESSION,
  DBG_DSESSION_ALL,
  DBG_CLOSE_DSESSION,
#endif  /* DEBUG */
};


/* function prototypes */
static void pmsg(int32_t statcode, ...);
static int32_t is_dspath(const char *dirpath);
static int32_t is_dsfile(const char *rootdir, const char *file);
static struct dsession *get_dsession(struct dsession *head, int32_t id, const char *file);
static struct dsession *create_dsession(struct dsession **head, int32_t id,
					const char *dir, const char *file, int32_t fmode);
static struct dsession *add_dsession(struct dsession **head);
static int32_t del_dsession(struct dsession **head, struct dsession *node);


/* for debug */
#ifdef DEBUG
#define DBG_PRINT(c) pmsg(c)
#define DBG_CLOSE_FILE(fp, file) pmsg(DBG_FCLOSE, fp, file)
#define DBG_OPEN_FILE(path, mode) pmsg(DBG_FOPEN, path, mode & MODE_READ ? "READ" : "WRITE")
#define DBG_STAT_FILE(path) pmsg(DBG_FSTAT, path)
#define DBG_SH_ALLDSESSION(hd) dbg_show_alldsession(hd)
#define DBG_SH_DSESSION(s) dbg_show_dsession(s)
#define DBG_SH_DSPATH(m) pmsg(DBG_DSPATH, m->dspath, m->fchroot)
#define DBG_SH_DSREQ(r) pmsg(DBG_DSREQ, r->dsid, r->dfile, r->dbuf ? "***" : "null", r->dlen, r->derr)
#define DBG_SH_QUERY(id, file) pmsg(DBG_RETRIEVE_DSESSION, id, file)
#define DBG_SH_READ(len, fp, file) pmsg(DBG_FREAD, len, fp, file);
#define DBG_SH_WRITE(len, fp, file) pmsg(DBG_FWRITE, len, fp, file);

static void dbg_show_dsession(struct dsession *ses);
static void dbg_show_alldsession(struct dsession *head);
#else
#define DBG_PRINT(c)
#define DBG_CLOSE_FILE(fp, file) 
#define DBG_OPEN_FILE(path, mode)
#define DBG_STAT_FILE(path)
#define DBG_SH_ALLDSESSION(hd)
#define DBG_SH_DSESSION(s)
#define DBG_SH_DSPATH(m)
#define DBG_SH_DSREQ(r)
#define DBG_SH_QUERY(id, file)
#define DBG_SH_READ(len, fp, file)
#define DBG_SH_WRITE(len, fp, file)
#endif	/* DEBUG */


#endif	/* _DATASTORE_H_ */
