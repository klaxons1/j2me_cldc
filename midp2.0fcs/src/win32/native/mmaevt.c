/*
 * @(#)mmaevt.c	1.2 02/07/24 @(#)
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <windows.h>

extern HWND hMainWindow;

void injectNativeEvent(int pID, int curMTime) {
  PostMessage(hMainWindow, WM_APP, (WPARAM)(pID), (LPARAM)(curMTime));
  return;
}
