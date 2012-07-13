/*
 * util.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "util.h"

/* constants */


/* To join head and tail.
 * return: length of full path, or -1 if to be greater than bufsize
 */
extern int32_t
join_path(const char *head, const char *tail, char *buf, size_t bufsize)
{
  char *pm;
  size_t len = 0;

  pm = buf;
  while (*head && len < bufsize) {
    *pm++ = *head++;
    len++;
  }
  if (len == bufsize) {
    *(pm - 1) = '\0';
    goto err;
  }

  /* add delimiter '/' */
  if (*(pm - 1) != '/') {
    *pm++ = '/';
  }

  /* delete '/' from tail */
  while (*tail == '/') {
    tail++;
  }

  while (*tail && len < bufsize) {
    *pm++ = *tail++;
    len++;
  }
  if (len == bufsize) {
    *(pm - 1) = '\0';
    goto err;
  }
  *pm = '\0';

  return len;
  
 err:
  return -1;
}

