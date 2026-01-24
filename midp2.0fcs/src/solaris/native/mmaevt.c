/*
 * @(#)mmaevt.c	1.2 02/07/24 @(#)
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <X11/Xlib.h>

extern Display     *display;
extern Window      outerWindow;
extern Atom        mmEOMAtom;

void injectNativeEvent(int pID, int curMTime) {
  XEvent xevt;

  xevt.xclient.type = ClientMessage;
  xevt.xclient.message_type = mmEOMAtom;
  xevt.xclient.format = 32;
  xevt.xclient.data.l[0] = pID;
  xevt.xclient.data.l[1] = curMTime;
  XSendEvent(display, outerWindow, False, 0, &xevt);
  
  return;
}
