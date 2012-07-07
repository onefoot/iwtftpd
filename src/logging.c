/*
 * logging.c
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

#include "logging.h"


/* instance of iwlog object */
static struct iwlog *alog = NULL;

/* messages of status */
static struct iwstatus iwlog_statmsgs[] = {
  /* error */
  { E_FAIL_MALLOC, "error: failed memory allocation, %s" },
  { 0, NULL }
};

static struct iwstatus iwlog_statvmsgs[] = {
  /* error verbose */
  { EV_FAIL_CLOSE, "error: close: failed, 'fd=%d': %s" },
  { EV_FAIL_LOCALTIME, "error: localtime_r: failed to convert: %s" },
  { EV_FAIL_OPEN, "error: open: failed, '%s': %s" },
  { EV_FAIL_STRFTIME, "error: strftime: failed to convert: %s" },
  { EV_FAIL_WRITE, "error: write: failed write to log, fd=%d msglen=%d: %s" },
  { EV_NULL_OBJ, "error: invalid object" },
  { 0, NULL }
};

/* -------------------------------------------------------------------------- */

/* ------------ */
/*  Interfaces  */
/* ------------ */

extern int32_t
iwlog_init(int32_t verbose)
{
  struct iwlog *plog = NULL;
  int fd = -1;

  tzset();
  
  if ((fd = open(LOGFILEPATH, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1) {
    pmsg(EV_FAIL_OPEN, LOGFILEPATH, strerror(errno));
    goto err;
  }

  if (! (plog = malloc(sizeof(struct iwlog)))) {
    pmsg(E_FAIL_MALLOC, __FUNCTION__);
    goto err;
  }

  plog->fverbose = verbose ? IW_TRUE : IW_FALSE;
  plog->logfd = fd;
  plog->msglen = 0;

  alog = plog;
  return IW_OK;
  
 err:
  if (fd > 0) close(fd);
  free(plog);
  return IW_ERR;
}


extern void
iwlog_print_msg(const char *msg, size_t msglen)
{
  time_t logtime;
  struct tm tm;
  char timebuf[TIMESTRLEN_MAX];
  char *pb;
  size_t timelen = 0;
  
  if (! alog) {
    pmsg(EV_NULL_OBJ);
    return;
  }

  logtime = time(NULL);

  if (! localtime_r(&logtime, &tm)) {
    pmsg(EV_FAIL_LOCALTIME, strerror(errno));
    strncpy(timebuf, "- DATETIME ERROR -", sizeof timebuf);
  }
  else {
    if (! strftime(timebuf, sizeof timebuf, "%Y-%m-%d %T", &tm)) {
      pmsg(EV_FAIL_STRFTIME, strerror(errno));
      strncpy(timebuf, "- TIMEFORM ERROR -", sizeof timebuf);
    }
  }

  /* create string, "yyyy-MM-DD hh:mm:ss MESSAGE" */
  strncpy(alog->msgbuf, timebuf, LOGMSGBUF_SIZE);
  for (pb = alog->msgbuf; pb - alog->msgbuf < LOGMSGBUF_SIZE - 1; pb++) {
    if (! *pb) {
      *pb++ = ' ';
      break;
    }
  }
  *pb = '\0';

  timelen = pb - alog->msgbuf;

  if (timelen + msglen < LOGMSGBUF_SIZE - 2) {
    strncpy(pb, msg, msglen);
  }
  else {
    strncpy(pb, msg, LOGMSGBUF_SIZE - timelen - 2);
  }
  *(alog->msgbuf + strlen(alog->msgbuf)) = '\n';
  *(alog->msgbuf + strlen(alog->msgbuf) + 1) = '\0';
  
  alog->msglen = strlen(alog->msgbuf);
  
  /* write to logfile */
  if (write(alog->logfd, alog->msgbuf, alog->msglen) == -1) {
    pmsg(EV_FAIL_WRITE, alog->logfd, alog->msglen, strerror(errno));
    fprintf(stderr, "[log fallback]: %s", alog->msgbuf);
  }
}


extern int
iwlog_getfd(void)
{
  if (! alog) {
    pmsg(EV_NULL_OBJ);
    goto err;
  }

  return alog->logfd;

 err:
  return IW_ERR;
}


extern void
iwlog_exit(void)
{
  if (! alog) {
    return;
  }

  if (alog->logfd >= 0) {
    if (close(alog->logfd) == -1) {
      pmsg(EV_FAIL_CLOSE, alog->logfd, strerror(errno));
    }
  }

  free(alog);
}


extern int32_t
iwlog_is_verbose(void)
{
  if (! alog) {
    return IW_FALSE;
  }

  return alog->fverbose;
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
    pm = iwlog_statmsgs;
  }
  else {
    if (alog->fverbose == IW_TRUE) {
      pm = iwlog_statvmsgs;
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
    strcpy(buf, "[iwlog] ");
    mlen = strlen(buf);
    vsnprintf(buf + mlen, sizeof buf - mlen, (*pm).statmsg, pargs);
  }
  va_end(pargs);
  
  mlen = strlen(buf);  
  if (sizeof buf - mlen >= 2) {
    *(buf + mlen) = '\n';
    *(buf + mlen + 1) = '\0';
  }
  else {
    *(buf + mlen - 1) = '\n';
  }
  fputs(buf, stderr);
}
