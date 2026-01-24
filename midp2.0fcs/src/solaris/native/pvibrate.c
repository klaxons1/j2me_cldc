/*
 * @(#)pvibrate.c	1.1 02/08/26 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "vibrate.h"

pthread_t tid = 0;  
/******************************************************/
void* threadProc(void *arg) {
  run(arg);
}

void mySleep(int millisec) {
  struct timespec interval;
  interval.tv_sec = 0;
  interval.tv_nsec = millisec * 1000000L;

  nanosleep((const struct timespec *)&interval, NULL);
  return;

}

int newThread() {
  int reterr;

  reterr = pthread_create(&tid, NULL, threadProc, NULL);

  return reterr;
}

void playThreadDead() {  
  if (tid > 0)
    pthread_join(tid, NULL);
  tid = 0;
}
