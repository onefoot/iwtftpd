/*
 * logging.h
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

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <sys/stat.h>
#include <fcntl.h>

#include "iw_common.h"
#include "iw_log.h"


/* constants */
#define TIMESTRLEN_MAX 24   	              /* max length of date time (yyyy-MM-DD hh:mm:ss) */
#define LOGMSGBUF_SIZE (TIMESTRLEN_MAX + IW_LOGMSGLEN_MAX)   /* buffer size of the log message */
#define LOGFILEPATH "/var/log/iwftpd.log"		     /* default path of the log file */


/* iwlog object */
struct iwlog {
  int32_t fverbose;		/* flag for verbose logging */
  int logfd;			/* fd of log file */
  size_t msglen;		/* length of log string */
  char msgbuf[LOGMSGBUF_SIZE];	/* log buffer */
};


/* status codes */
enum L_STATCODE {
  /* error */
  E_FAIL_MALLOC = 1,
};

enum L_STATCODE_VERBOSE {
  /* error verbose */
  EV_FAIL_CLOSE = 65,
  EV_FAIL_LOCALTIME,
  EV_FAIL_OPEN,
  EV_FAIL_STRFTIME,
  EV_FAIL_WRITE,
  EV_NULL_OBJ,
};


/* function prototypes */
static void pmsg(int32_t statcode, ...);


#endif	/* _LOGGING_H_ */
