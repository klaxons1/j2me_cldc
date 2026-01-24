/*
 * @(#)vibrate.h	1.1 02/08/26
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef __VIBRATE_H__
#define __VIBRATE_H__

#ifdef __cplusplus
extern "C" {
#endif

  void stopVibrate();
  
  int startVibrate(int samples);

  void clearVibrationFlag();

  void mySleep(int millisecs);
  
  void playThreadDead();

  int newThread();

  void run();


#ifdef __cplusplus
}
#endif

#endif /* __VIBRATE_H__ */
