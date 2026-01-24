/*
 * @(#)vibrate.c	1.1 02/08/26 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <stdlib.h>

#include "audiornd.h"
#include "vibrate.h"
#include "vdata.h"


typedef struct {
  int adptr;
  int dursamples;
  int written;
  int curptr;
  int closed;
}VIBSTRUCT;

VIBSTRUCT vstruct;

/**********************************************************/
void run(void *arg) {
  int len = 8000 * 8 * 1 / 8 / 32 & ~3;
  int toWrite = 0;
  char* data;

  while (vstruct.written < vstruct.dursamples) {
    toWrite = vstruct.dursamples - vstruct.written;
    if (toWrite > len)
      toWrite = len;
    data = getNextAvailBuf(vstruct.adptr, toWrite);
    if (data == NULL) {
	mySleep(16);
	continue;
    }

    /* copy vdata to data */
    if (vdata_length - vstruct.curptr > toWrite) {
      memcpy(data, &vData[vstruct.curptr], toWrite);
      vstruct.curptr += toWrite;
    } else {
      int l1 = vdata_length - vstruct.curptr;
      int l2 = toWrite - l1;
      memcpy(data, &vData[vstruct.curptr], l1);
      memcpy(data+l1, &vData[0], l2);
      vstruct.curptr = l2;
    }
    audioWrite(vstruct.adptr, 0, toWrite);
    vstruct.written += toWrite;
  }

  if (vstruct.closed == 1) {
    return;
  }

#ifndef LINUX
  /**
   * On Solaris/win32 drain has to be done explicitly.
   * On linux, close imply a drain
   */
  while ((audioCommon(vstruct.adptr, DRAIN, 0) == 0) && vstruct.closed != 1)
      mySleep(16);

  if (vstruct.closed == 1)
    return;
#endif

  audioCommon(vstruct.adptr, CLOSE, 0);
  memset(&vstruct, 0, sizeof(VIBSTRUCT));
  clearVibrationFlag();
  return;
}


int startVibrate(int dursamples) {
  int reterr; 

  memset(&vstruct, 0, sizeof(VIBSTRUCT));
  vstruct.dursamples = dursamples;
  vstruct.written = 0;
  vstruct.curptr = 0;
  
  vstruct.adptr = audioOpen(8000, 8, 1, MMAFMT_UNSIGNED, MMAFMT_LITTLE);
  if (vstruct.adptr <= 0 ) {
    memset(&vstruct, 0, sizeof(VIBSTRUCT));
    return 0;
  }

  /* start a new thread */
  reterr = newThread(); 
  if (reterr != 0 ) {
    audioCommon(vstruct.adptr, CLOSE, 0);
    memset(&vstruct, 0, sizeof(VIBSTRUCT));
    return 0;
  }

  return 1;
}

void stopVibrate() {
    vstruct.closed = 1;
    vstruct.written = vstruct.dursamples;
    
    /* wait for play thread dead */
    playThreadDead();
    if (vstruct.adptr > 0) {
      audioCommon(vstruct.adptr, FLUSH, 0);
      audioCommon(vstruct.adptr, CLOSE, 0);
    }
    memset(&vstruct, 0, sizeof(VIBSTRUCT));
    
    return;
}
