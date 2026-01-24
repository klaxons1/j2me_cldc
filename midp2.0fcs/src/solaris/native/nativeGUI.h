/*
 * @(#)nativeGUI.h	1.23 02/08/22 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef NATIVEGUI_H_INCLUDED
#define NATIVEGUI_H_INCLUDED

#include <X11/Xlib.h>

/*
 * Defines screen size
 */
#define DISPLAY_WIDTH           180
#define DISPLAY_HEIGHT          208

/*
 * This (x,y) coordinate pair refers to the offset of the upper 
 * left corner of the display screen within the MIDP phone handset
 * graphic window 
 */
#define X_SCREEN_OFFSET 30 
#define Y_SCREEN_OFFSET 131

/* The max number of characters in a command menu item */
#define MAX_MENU_COMMAND_LENGTH 127

/* The max number of characters in a soft button command label */
#define MAX_SOFTBUTTON_COMMAND_LENGTH 12

extern Display     *display;
extern Screen      *screen;
extern GC           gc;
extern GC           gcErase;
extern Drawable     paintDrawable;
extern jboolean     doubleBuffer;
extern int          visualDepth;
extern int          numColors;

/* a greenish pixel that looks kind of like a typical LCD screen */
extern XColor       lightPixel;
extern Window       paintWindow;
extern XColor       grayTable[16];
extern XColor       rgbTable[256];

extern Window       outerWindow;
extern Colormap     colormap;

extern XFontStruct *fonts[];

/*  Atoms used for fetching underline parameters */
extern Atom          underlinePositionAtom;
extern Atom          underlineThicknessAtom;
extern Atom          rawPositionAtom;
extern Atom          rawThicknessAtom;

extern XFontStruct *findFontStruct(Font font);
extern Drawable setupGC(GC gc, int pixel, short *clip, void* dst, int dotted);
extern void refreshPaintWindow(int x1, int y1, int x2, int y2);

#endif
