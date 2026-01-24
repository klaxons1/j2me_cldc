/*
 * @(#)audiornd.h	1.3 02/07/24 @(#)
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef __AUDIORND_H__
#define __AUDIORND_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PAUSE 1
#define RESUME 2
#define FLUSH 3
#define DRAIN 4
#define CLOSE 5
#define SAMPLES_PLAYED 6
#define SET_VOLUME 7
#define GET_VOLUME 8
#define GET_BUFLEN 9

#define GET_CUR_TIME    15
#define SET_CUR_TIME    16

#define MMAFMT_SIGNED   1
#define MMAFMT_UNSIGNED 0
#define MMAFMT_LITTLE   0
#define MMAFMT_BIG      1
  
  int audioOpen(int sampleRate,
		int bitsPerSample,
		int channels,
		int isSigned,
		int endianess);
  
  char * getNextAvailBuf(int adptr, int len);
  
  void audioWrite(int adptr, int offset, int len);
  
  int audioCommon(int adptr, int code, int param);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIORND_H__ */
