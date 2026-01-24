/*
 * @(#)pvibrate.c	1.2 02/08/27 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */
#include <windows.h>

#include "vibrate.h"

HANDLE thd = NULL;

/******************************************************/
unsigned _stdcall threadProc(void *arg) {
  run(arg);
  _endthreadex(0);
  return 0;
}

void mySleep(int millisec) {
  Sleep(millisec);
  return;

}

int newThread() {
  unsigned long cthread;
  if (thd != NULL) {
    CloseHandle(thd);
  }
  thd = (HANDLE)_beginthreadex(NULL, 0, &threadProc, NULL, 0, &cthread);
  if (thd == NULL) {
    return 1;
  }
  
  return 0;
}

void playThreadDead() {  
  if (thd != NULL) {
    WaitForSingleObject(thd, 3000);
    CloseHandle(thd);
  }

  thd = NULL;
}
