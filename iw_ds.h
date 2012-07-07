/*
 * iw_ds.h
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

#ifndef _IW_DS_H_
#define _IW_DS_H_


typedef struct _iwds IWDS;

/* to create instance and termination */
extern IWDS *iwds_init(const char *datastore);
extern void iwds_exit(IWDS *ds);

/* request structure used for reading and writing */
struct dsreq {
  int32_t dsid;			/* session ID */
  char *dfile;			/* filename */
  void *dbuf;			/* data buffer */
  size_t dlen;			/* size of data buffer */
  int32_t derr;			/* error code */
};

/* read and write */
extern size_t iwds_read(IWDS *ds, struct dsreq *req);
extern size_t iwds_write(IWDS *ds, struct dsreq *req);
extern int32_t iwds_close(IWDS *ds, struct dsreq *req);

/* check file */
extern int32_t iwds_isfile(IWDS *ds, const char *filename);

extern int32_t iwds_set_chroot(IWDS *ds);
extern char *iwds_get_dspath(IWDS *ds);


/* error code */
enum DSREQ_ERRCODE {
  DSERR_INTERERROR = -15,	/* iwds module error */
  DSERR_INVALIDOBJ,		/* invalid instance */
  DSERR_NOREQ,			/* dsreq isn't passed */
  DSERR_NOTEXIST,		/* file doesn't exist */
  DSERR_NOTPERMIT,		/* couldn't write (overwrite) */
  DSERR_NOSESSION,		/* datastore session not found */
  DSERR_READFAIL,		/* reading error */
  DSERR_WRITEFAIL,		/* writing error */
};

/* get a message string of error code */
extern char *iwds_strerr(int32_t ecode);


#ifdef DEBUG
extern void dbg_ds_show_alldsession(IWDS *ds);
#endif	/* DEBUG */


#endif	/* _IW_DS_H_ */
