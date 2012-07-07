/*
 * datastore.c
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

#include "datastore.h"


/* messages of status */
static struct iwstatus iwds_statmsgs[] = {
  /* error */
  { E_DSPATH_INCORRECT, "error: datastore path '%s' is incorrect" },
  { E_FAIL_CHECKFILE, "error: failed to check the file of datastore, '%s'" },
  { E_FAIL_FREAD, "error: failed to read from datastore, '%s'" },
  { E_FAIL_FWRITE, "error: failed to write to datastore, '%s'" },
  { E_FAIL_MALLOC, "error: failed memory allocation, %s" },
  { E_FILE_EXIST, "error: '%s' already exists on datastore" },
  { E_FILE_NOTEXIST, "error: '%s' does not exist on datastore" },
  { 0, NULL }
};

static struct iwstatus iwds_statvmsgs[] = {
  /* error verbose */
  { EV_DSESSION_NOTFOUND, "error: datastore session not found, '%d', '%s'" },
  { EV_FAIL_ADD_DSESSION, "error: faild to add a datastore session" },
  { EV_FAIL_CREATE_DSESSION, "error: could not create a new session of datastore, '%d', '%s'" },
  { EV_FAIL_FCLOSE, "error: fclose: failed, %s: %s" },
  { EV_FAIL_FOPEN, "error: fopen: failed, '%s': %s" },
  { EV_FAIL_JOIN_PATH, "error: failed to join path '%s' and '%s'" },
  { EV_FAIL_REALPATH, "error: realpath: failed to convert '%s': %s" },
  { EV_NODSREQ, "error: request was not passed" },
  { EV_NULL_OBJ, "error: invalid object" },
#ifdef DEBUG
  /* debugging */
  { DBG_READ_END, "DBG: end of reading" },
  { DBG_READ, "DBG: reading from the filesystem" },
  { DBG_FREAD, "DBG: read: datalen=%d, fp=%p, file=%s" },
  { DBG_FWRITE, "DBG: wrote: datalen=%d, fp=%p, file=%s" },
  { DBG_WRITE_END, "DBG: end of writing" },
  { DBG_RETRIEVE_DSESSION, "DBG: retrieving the dsession, id=%d, file=%s" },
  { DBG_DSREQ, "DBG: dsreq: dsid=%d, dfile=%s, dbuf=%s, dlen=%d, derr=%d" },
  { DBG_WRITE, "DBG: writing to the filesystem" },
  { DBG_SET_CHROOT, "DBG: setting a chroot flag" },
  { DBG_CLEANUP_DSESSION, "DBG: clean up all dsession" },
  { DBG_DSPATH, "DBG: datastore path, dspath=%s, fchroot=%d" },
  { DBG_GET_DSPATH, "DBG: get the path of datastore" },
  { DBG_CHECK_DSPATH, "DBG: checking the path of datastore in the filesystem" },
  { DBG_FILE_EXIST, "DBG: file exist" },
  { DBG_FILE_NOTEXIST, "DBG: file does not exist" },
  { DBG_CHECK_FILE, "DBG: checking a file in the filesystem" },
  { DBG_FSTAT, "DBG: get status, path=%s" },
  { DBG_FOPEN, "DBG: open the file, path=%s, mode=%s" },
  { DBG_ADD_DSESSION, "DBG: add a new dsession" },
  { DBG_FCLOSE, "DBG: closing the file, fp=%p, file=%s" },
  { DBG_DEL_DSESSION, "DBG: delete a dsession" },
  { DBG_REMAIN_DSESSION, "DBG: remains of dsession" },
  { DBG_DSESSION_EMPTY, "DBG: dsession is empty" },
  { DBG_DSESSION, "DBG: DSESSION: clid=%d, fp=%p, file=%s, derr=%d" },
  { DBG_DSESSION_ALL, "DBG: DSESSION %d: clid=%d, fp=%p, file=%s, derr=%d" },
  { DBG_CLOSE_DSESSION, "DBG: closing the dsession" },
#endif  /* DEBUG */
  { 0, NULL }
};

static struct iwstatus iwds_dsreq_errmsgs[] = {
  { DSERR_INTERERROR, "internal error of module" },
  { DSERR_INVALIDOBJ, "invalid object" },
  { DSERR_NOREQ, "request was not passed" },
  { DSERR_NOTEXIST, "file does not exist" },
  { DSERR_NOTPERMIT, "access is not permitted" },
  { DSERR_NOSESSION, "no the session of datastore" },
  { DSERR_READFAIL, "file reading error" },
  { DSERR_WRITEFAIL, "file writing error" },
  { 0, NULL }
};

/* -------------------------------------------------------------------------- */

/* ------------ */
/*  Interfaces  */
/* ------------ */

extern IWDS *
iwds_init(const char *datastore)
{
  IWDS *pds;
  char *path;

  if (! datastore) {
    datastore = DEFAULT_DATASTORE;
  }

  /* normalize path */
  if (! (path = realpath(datastore, NULL))) {
    pmsg(EV_FAIL_REALPATH, datastore, strerror(errno));
    pmsg(E_DSPATH_INCORRECT, datastore);
    goto err;
  }

  /* check path */
  if (is_dspath(path) == IW_FALSE) {
    pmsg(E_DSPATH_INCORRECT, path);
    goto err;
  }

  if (! (pds = malloc(sizeof(IWDS)))) {
    pmsg(E_FAIL_MALLOC, __FUNCTION__);
    goto err;
  }
  memset(pds, 0, sizeof(IWDS));
  
  pds->dspath = path;
  pds->fchroot = IW_FALSE;

  return pds;

 err:
  free(path);
  return NULL;
}


extern size_t
iwds_read(IWDS *ds, struct dsreq *req)
{
  DBG_PRINT(DBG_READ);
  struct dsession *ses = NULL;
  size_t rlen = 0;
  int32_t errcode;
    
  if (! ds) {
    pmsg(EV_NULL_OBJ);
    errcode = DSERR_INVALIDOBJ;
    goto err;
  }

  if (! req) {
    pmsg(EV_NODSREQ);
    errcode = DSERR_NOREQ;
    goto err;
  }

  DBG_SH_DSREQ(req);

  switch (is_dsfile(ds->dspath, req->dfile)) {
  case IW_ERR:
    pmsg(E_FAIL_CHECKFILE, req->dfile);
    errcode = DSERR_INTERERROR;
    goto err;
  case IW_FALSE:
    pmsg(E_FILE_NOTEXIST, req->dfile);
    errcode = DSERR_NOTEXIST;
    goto err;
  }

  DBG_SH_QUERY(req->dsid, req->dfile);
  
  if (! (ses = get_dsession(ds->dhead, req->dsid, req->dfile))) {
    if (! (ses = create_dsession(&ds->dhead, req->dsid, ds->dspath, req->dfile, MODE_READ))) {
      pmsg(EV_FAIL_CREATE_DSESSION, req->dsid, req->dfile);
      errcode = DSERR_NOSESSION;
      goto err;
    }
  }

  DBG_SH_DSESSION(ses);
  
  if ((rlen = fread(req->dbuf, sizeof(char), req->dlen, ses->fp)) < req->dlen) {
    if (ferror(ses->fp)) {
      pmsg(E_FAIL_FREAD, ses->filename);
      errcode = DSERR_READFAIL;
      goto err;
    }

    /* EOF */
    DBG_PRINT(DBG_READ_END);
  }

  req->derr = IW_OK;

  DBG_SH_READ(rlen, ses->fp, ses->filename);
  return rlen;

 err:
  if (req) req->derr = errcode;
  if (ds && ds->dhead && ses) del_dsession(&ds->dhead, ses);
  return rlen;
}


extern size_t
iwds_write(IWDS *ds, struct dsreq *req)
{
  DBG_PRINT(DBG_WRITE);
  struct dsession *ses = NULL;
  size_t wlen = 0;
  int32_t errcode;
  
  if (! ds) {
    pmsg(EV_NULL_OBJ);
    errcode = DSERR_INVALIDOBJ;
    goto err;
  }

  if (! req) {
    pmsg(EV_NODSREQ);
    errcode = DSERR_NOREQ;
    goto err;
  }

  DBG_SH_DSREQ(req);
  DBG_SH_QUERY(req->dsid, req->dfile);
  
  if (! (ses = get_dsession(ds->dhead, req->dsid, req->dfile))) {
    switch (is_dsfile(ds->dspath, req->dfile)) {
    case IW_ERR:
      pmsg(E_FAIL_CHECKFILE, req->dfile);
      errcode = DSERR_INTERERROR;
      goto err;
    case IW_TRUE:
      pmsg(E_FILE_EXIST, req->dfile);
      errcode = DSERR_NOTPERMIT;
      goto err;
    }

    if (! (ses = create_dsession(&ds->dhead, req->dsid, ds->dspath, req->dfile, MODE_WRITE))) {
      pmsg(EV_FAIL_CREATE_DSESSION, req->dsid, req->dfile);
      errcode = DSERR_NOSESSION;
      goto err;
    }
  }

  DBG_SH_DSESSION(ses);

  if (req->dlen == 0) {
    DBG_PRINT(DBG_WRITE_END);

    /* end of writing */
    del_dsession(&ds->dhead, ses);
    req->derr = IW_OK;

    DBG_SH_DSREQ(req);
    return 0;
  }

  if ((wlen = fwrite(req->dbuf, sizeof(char), req->dlen, ses->fp)) < req->dlen) {
    pmsg(E_FAIL_FWRITE, ses->filename);
    errcode = DSERR_WRITEFAIL;
    del_dsession(&ds->dhead, ses);
  }

  req->derr = IW_OK;

  DBG_SH_WRITE(wlen, ses->fp, ses->filename);
  return wlen;

 err:
  if (req) req->derr = errcode;
  if (ds && ds->dhead && ses) del_dsession(&ds->dhead, ses);

  return wlen;
}


extern int32_t
iwds_close(IWDS *ds, struct dsreq *req)
{
  DBG_PRINT(DBG_CLOSE_DSESSION);
  struct dsession *ses;
  int32_t errcode;
  
  if (! ds) {
    pmsg(EV_NULL_OBJ);
    errcode = DSERR_INVALIDOBJ;
    goto err;
  }

  if (! req) {
    pmsg(EV_NODSREQ);
    errcode = DSERR_NOREQ;
    goto err;
  }

  DBG_SH_QUERY(req->dsid, req->dfile);
  if (! (ses = get_dsession(ds->dhead, req->dsid, req->dfile))) {
    if (req->dlen == 0) {	/* for closing */
      return IW_OK;
    }
    pmsg(EV_DSESSION_NOTFOUND, req->dsid, req->dfile);
    errcode = DSERR_NOSESSION;
    goto err;
  }

  del_dsession(&ds->dhead, ses);
  req->derr = IW_OK;
  return IW_OK;

 err:
  if (req) req->derr = errcode;
  return IW_ERR;
}


extern int32_t
iwds_isfile(IWDS *ds, const char *filename)
{
  if (! ds) {
    pmsg(EV_NULL_OBJ);
    goto err;
  }

  return is_dsfile(ds->dspath, filename);

 err:
  return IW_ERR;
}


extern int32_t
iwds_set_chroot(IWDS *ds)
{
  DBG_PRINT(DBG_SET_CHROOT);
  
  if (! ds) {
    pmsg(EV_NULL_OBJ);
    goto err;
  }

  ds->fchroot = IW_TRUE;
  *(ds->dspath) = '/';
  *(ds->dspath + 1) = '\0';

  DBG_SH_DSPATH(ds);
  return IW_OK;
  
 err:
  return IW_ERR;
}


extern void
iwds_exit(IWDS *ds)
{
  DBG_PRINT(DBG_CLEANUP_DSESSION);
  struct dsession *pm;
  struct dsession *tmp;
  
  if (! ds) {
    return;
  }

  pm = ds->dhead;
  while (pm) {
    DBG_SH_DSESSION(pm);
    DBG_CLOSE_FILE(pm->fp, pm->filename);
    
    if (fclose(pm->fp) == EOF) {
      pmsg(EV_FAIL_FCLOSE, pm->filename);
    }
    free(pm->filename);
    tmp = pm;
    pm = pm->next;
    free(tmp);
  }

  free(ds->dspath);
  free(ds);
}


extern char *
iwds_get_dspath(IWDS *ds)
{
  DBG_PRINT(DBG_GET_DSPATH);
  
  if (! ds) {
    pmsg(EV_NULL_OBJ);
    return NULL;
  }

  DBG_SH_DSPATH(ds);
  return ds->dspath;
}


extern char *
iwds_strerr(int32_t ecode)
{
  struct iwstatus *pm;

  for (pm = iwds_dsreq_errmsgs; (*pm).statmsg && (*pm).statcode != ecode; pm++)
    ;
  return (*pm).statmsg;
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
    pm = iwds_statmsgs;
  }
  else {
    if (iwlog_is_verbose() == IW_TRUE) {
      pm = iwds_statvmsgs;
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
    strcpy(buf, "[iwds] ");
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


static int32_t
is_dspath(const char *dirpath)
{
  DBG_PRINT(DBG_CHECK_DSPATH);
  struct stat st;

  DBG_STAT_FILE(dirpath);
  if (stat(dirpath, &st) == -1) {
    return IW_FALSE;
  }

  return S_ISDIR(st.st_mode) ? IW_TRUE : IW_FALSE;
}


static int32_t
is_dsfile(const char *rootdir, const char *file)
{
  DBG_PRINT(DBG_CHECK_FILE);
  char *buf = NULL;
  size_t bufsize;
  struct stat st;
  
  bufsize = strlen(rootdir) + strlen(file) + 2;
  if (! (buf = malloc(sizeof(char) * bufsize))) {
    pmsg(E_FAIL_MALLOC, __FUNCTION__);
    goto err;
  }

  /* join path of rootdir and filename */
  if (join_path(rootdir, file, buf, bufsize) == -1) {
    pmsg(EV_FAIL_JOIN_PATH, rootdir, file);
    goto err;
  }

  DBG_STAT_FILE(buf);

  /* check file */
  if (stat(buf, &st) == -1) {
    free(buf);
    DBG_PRINT(DBG_FILE_NOTEXIST);
    return IW_FALSE;
  }

  free(buf);
  return S_ISREG(st.st_mode) ? IW_TRUE : IW_FALSE;
    
 err:
  free(buf);
  return IW_ERR;
}


static struct dsession *
get_dsession(struct dsession *head, int32_t id, const char *file)
{
  struct dsession *pm;

  if (! head) {
    return NULL;
  }

  pm = head;
  while (pm) {
    if (pm->clid == id && strcmp(pm->filename, file) == 0) {
      break;
    }
    pm = pm->next;
  }
  
  return pm;
}


static struct dsession *
create_dsession(struct dsession **head, int32_t id, const char *dir, const char *file, int32_t fmode)
{
  DBG_PRINT(DBG_ADD_DSESSION);
  struct dsession *node = NULL;
  FILE *pst = NULL;
  char *pathbuf = NULL;
  size_t bufsize;

  /* to make a full path of file */
  bufsize = strlen(dir) + strlen(file) + 2;
  if (! (pathbuf = malloc(sizeof(char) * bufsize))) {
    pmsg(E_FAIL_MALLOC, __FUNCTION__);
    goto err;
  }

  if (join_path(dir, file, pathbuf, bufsize) == -1) {
    pmsg(EV_FAIL_JOIN_PATH, dir, file);
    goto err;
  }

  DBG_OPEN_FILE(pathbuf, fmode);

  if (! (pst = fopen(pathbuf, fmode & MODE_READ ? "r" : "w"))) {
    pmsg(EV_FAIL_FOPEN, pathbuf, strerror(errno));
    goto err;
  }

  if (! (node = add_dsession(head))) {
    pmsg(EV_FAIL_ADD_DSESSION);
    goto err;
  }
  node->fp = pst;
  node->clid = id;

  if (! (node->filename = malloc(sizeof(char) * (strlen(file) + 1)))) {
    pmsg(E_FAIL_MALLOC, __FUNCTION__);
    goto err;
  }
  strncpy(node->filename, file, strlen(file) + 1);

  free(pathbuf);

  DBG_SH_DSESSION(node);
  return node;
  
 err:
  if (node)
    del_dsession(head, node);
  else if (pst)
    fclose(pst);
  free(pathbuf);
  return NULL;
}


static struct dsession *
add_dsession(struct dsession **head)
{
  struct dsession *node;

  if (! (node = malloc(sizeof(struct dsession)))) {
    pmsg(E_FAIL_MALLOC, __FUNCTION__);
    goto err;
  }
  memset(node, 0, sizeof(struct dsession));
  
  if (*head) {
    (*head)->prev = node;
    node->next = *head;
  }
  *head = node;

  return node;
  
 err:
  return NULL;
}


static int32_t
del_dsession(struct dsession **head, struct dsession *node)
{
  DBG_PRINT(DBG_DEL_DSESSION);
  struct dsession *pm;

  if (! (pm = get_dsession(*head, node->clid, node->filename))) {
    pmsg(EV_DSESSION_NOTFOUND, node->clid, node->filename);
    goto err;
  }

  if (! pm->prev && pm->next) {
    pm->next->prev = NULL;
    *head = pm->next;
  }
  else if (pm->prev && pm->next) {
    pm->prev->next = pm->next;
    pm->next->prev = pm->prev;
  }
  else if (pm->prev && ! pm->next) {
    pm->prev->next = NULL;
  }
  else {
    *head = NULL;
  }

  DBG_SH_DSESSION(pm);
  
  if (pm->fp) {
    DBG_CLOSE_FILE(pm->fp, pm->filename);
    if (fclose(pm->fp) == EOF) {
      pmsg(EV_FAIL_FCLOSE, pm->filename);
    }
  }
  free(pm->filename);
  free(pm);

  DBG_PRINT(DBG_REMAIN_DSESSION);
  DBG_SH_ALLDSESSION(*head);
  return IW_OK;

 err:
  return IW_ERR;
}


/* for debug */
#ifdef DEBUG
static void
dbg_show_dsession(struct dsession *ses)
{
  if (! ses) {
    pmsg(DBG_DSESSION_EMPTY);
  }
  else {
    pmsg(DBG_DSESSION, ses->clid, ses->fp, ses->filename, ses->derr);
  }
}

static void
dbg_show_alldsession(struct dsession *head)
{
  struct dsession *pm;
  int32_t i;
  
  if (! head) {
    pmsg(DBG_DSESSION_EMPTY);
  }
  else {
    for (i = 1, pm = head; pm; pm = pm->next, i++) {
      pmsg(DBG_DSESSION_ALL, i, pm->clid, pm->fp, pm->filename, pm->derr);
    }
  }
}

extern void
dbg_ds_show_alldsession(IWDS *ds)
{
  dbg_show_alldsession(ds->dhead);
}
#endif	/* DEBUG */

