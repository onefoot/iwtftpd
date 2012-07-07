/*
 * util.c
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

