/*
 * @(#)defaultLCDUI.h	1.76 02/10/15 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef DEFAULTLCDUI_H_INCLUDED
#define DEFAULTLCDUI_H_INCLUDED

typedef unsigned short unicode;

/* reference point locations, from Graphics.java */
#define HCENTER   1
#define VCENTER   2
#define LEFT      4
#define RIGHT     8
#define TOP      16
#define BOTTOM   32
#define BASELINE 64

/* flags for font descriptors */
#define STYLE_PLAIN         0
#define STYLE_BOLD          1
#define STYLE_ITALIC        2
#define STYLE_UNDERLINED    4

/* flags for line types */
#define SOLID 0
#define DOTTED 1

#define SIZE_SMALL          8
#define SIZE_MEDIUM         0
#define SIZE_LARGE         16

#define FACE_SYSTEM         0
#define FACE_MONOSPACE     32
#define FACE_PROPORTIONAL  64

/* flags for LCDUIgetDisplayParams */
#define SUPPORTS_COLOR         1
#define SUPPORTS_POINTER       2
#define SUPPORTS_MOTION        4
#define SUPPORTS_REPEAT        8
#define SUPPORTS_DOUBLEBUFFER 16

/*  The type of events which can be generated.                               */
enum KVMEventTypes {
    invalidKVMEvent    = -2,
    stopKVMEvent       = -1,
    keyDownKVMEvent    = 0,
    keyUpKVMEvent      = 1,
    keyRepeatKVMEvent  = 2,
    penDownKVMEvent    = 3,
    penUpKVMEvent      = 4,
    penMoveKVMEvent    = 5,
    timerKVMEvent      = 6,
    commandKVMEvent    = 7,
    repaintKVMEvent    = 8,
    keyTypedKVMEvent   = 9,
    mmEOMEvent         = 10,
    systemKVMEvent     = 11, /* pause, suspend, kill, etc. */
#ifdef INCLUDE_I18N
    imeKVMEvent        = 12,
    lastKVMEvent       = 12
#else
    lastKVMEvent       = 11
#endif
};

/*  The event record. */
typedef struct {
    enum KVMEventTypes type;
    int                chr;
    short              screenX;
    short              screenY;
#ifdef INCLUDE_I18N
    unicode*           str;
    short              len;
#endif
} KVMEventType;

extern void StoreMIDPEvent(KVMEventType *evt);

/* global - incremented when network activity occurs */
extern int netIndicatorCount;
extern unicode *_digits_; 
extern int _digitslen_;
extern unicode *_menu_;
extern int _menulen_;
extern unicode *_back_;
extern int _backlen_;
extern unicode *_cancel_;
extern int _cancellen_;

#define COMMAND_TYPE_SCREEN 1
#define COMMAND_TYPE_BACK   2
#define COMMAND_TYPE_CANCEL 3
#define COMMAND_TYPE_OK     4
#define COMMAND_TYPE_HELP   5
#define COMMAND_TYPE_STOP   6
#define COMMAND_TYPE_EXIT   7
#define COMMAND_TYPE_ITEM   8

typedef struct {
    int priority;
    unicode *chars;
    unsigned int numChars:8;
    unsigned int useLongChars:1;
    unicode *longChars;
    unsigned int numLongChars:8;
    unsigned int type:4;
    unsigned int id:20;
} commandStruct;

typedef struct {
    int idx;         /* index of popup element */
    int numChars;    /* length of "chars" array */
    unicode *chars;  /* string of popup element */
    int useImage;    /* < 0 if "image" is valid for this element */
    int imageHeight;
    int imageWidth;
    void* image;
} popupElementStruct;

typedef enum {
    KEY_0        = '0',
    KEY_1        = '1',
    KEY_2        = '2',
    KEY_3        = '3',
    KEY_4        = '4',
    KEY_5        = '5',
    KEY_6        = '6',
    KEY_7        = '7',
    KEY_8        = '8',
    KEY_9        = '9',
    KEY_ASTERISK = '*',
    KEY_POUND    = '#',

    KEY_UP       = -1,
    KEY_DOWN     = -2,
    KEY_LEFT     = -3,
    KEY_RIGHT    = -4,
    KEY_SELECT   = -5,

    KEY_SOFT1    = -6,
    KEY_SOFT2    = -7,
    KEY_CLEAR    = -8,

    /* these may not be available to java */
    KEY_SEND     = -10,
    KEY_END      = -11,
    KEY_POWER    = -12, 
    KEY_INVALID  = 0,

    /* Values copied from EventHandler */
    VK_SUSPEND_ALL      = 1,
    VK_RESUME_ALL       = 2,
    VK_SHUTDOWN         = 3,
    VK_SUSPEND_CURRENT  = 4,
    VK_RESUME_PREVIOUS  = 5,
    VK_KILL_CURRENT     = 6
} KeyType;

#define TRANS_NONE           0
#define TRANS_MIRROR_ROT180  1
#define TRANS_MIRROR         2
#define TRANS_ROT180         3
#define TRANS_MIRROR_ROT270  4
#define TRANS_ROT90          5
#define TRANS_ROT270         6
#define TRANS_MIRROR_ROT90   7

#define FONTPARAMS face, style, size
#define FONTPARAMS_PROTO int face, int style, int size

/*
 * Return the key code corresponding to the given abstract game action.
 */
extern int  LCDUIgetKeyCode(int);

/*
 * Return the system key corresponding to the given key code..
 */
extern int  LCDUIgetSystemKey(int);

/*
 * Return the abstract game action corresponding to the given key code.
 */
extern int  LCDUIgetGameAction(int);

/*
 * Return the key string to the given key code.
 */
extern char *LCDUIgetKeyName(int);

/*
 * Return the displayed rgb value of a given rgb pixel in 0xRRGGBB format.
 */
extern int LCDUIgetDisplayColor(int color);

/* 
 * Translate a rgb or grayscale to device-dependent pixel value. 
 */
extern int LCDUIgetPixel(int rgb, int gray, int isGray);

/* 
 * Fill rgbBuffer with pixels (in 0xRRGGBB format) from an image. 
 */
extern void LCDUIgetRGB(int **rgbBuffer, int offset, int scanLength,
			int x, int y, int width, int height, void *img);

/*
 * Draw a line between two points (x1,y1) and (x2,y2).
 */
extern void LCDUIdrawLine(int pixel, short *clip, void *dst, int dotted,
			  int x1, int y1, int x2, int y2);

/*
 * Draw a rectangle at (x,y) with the given width and height.
 */
extern void LCDUIdrawRect(int pixel, short *clip, void *dst, int dotted,
			  int x, int y, int width, int height);

/*
 * Fill a rectangle at (x,y) with the given width and height.
 */
extern void LCDUIfillRect(int pixel, short *clip, void *dst, int dotted,
			  int x, int y, int width, int height);

/*
 * Draw a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
extern void LCDUIdrawRoundRect(int pixel, short *clip, void *dst,
			       int dotted,
			       int x, int y, int width, int height,
			       int arcWidth, int arcHeight);

/*
 * Fill a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
extern void LCDUIfillRoundRect(int pixel, short *clip, void *dst, 
			       int dotted,
			       int x, int y, int width, int height,
			       int arcWidth, int arcHeight);

/*
 * Draw an elliptical arc centered in the given rectangle.  If filled is
 * true, fill the contents of the arc; otherwise just stroke it.  The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 */
extern void LCDUIdrawArc(int pixel, short *clip, void *dst, int dotted,
			 int x, int y, int width, int height, 
                         int startAngle, int arcAngle);

/*
 * Fill an elliptical arc centered in the given rectangle.  The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 */
extern void LCDUIfillArc(int pixel, short *clip, void *dst, int dotted,
			 int x, int y, int width, int height, 
                         int startAngle, int arcAngle);

/*
 * Draw a filled triangle defined by the lines connecting points 
 * (x1,y1), (x2,y2), and (x3,y3).
 */
extern void LCDUIfillTriangle(int pixel, short *clip, void *dst, 
			      int dotted, int x1, int y1, int x2, 
			      int y2, int x3, int y3);

/*
 * Draw the first n characters in chararray, with the anchor point of the
 * entire (sub)string located at x, y.
 */
extern void LCDUIdrawChars(int pixel, short *clip, void *dst, int dotted,
			   FONTPARAMS_PROTO, 
                           int x, int y, int anchor, 
                           const unicode *chararray, int n);

/*
 * Draw the specified region of the given image data,
 * locating its anchor point at x, y
 */
extern void LCDUIdrawRegion(jshort *clip, void *dst, void *img,
                           jint x_dest, jint y_dest, jint anchor,
                           jint x_src, jint y_src,
                           jint width, jint height, 
                           jint transform, jboolean isRegion);

/*
 * Copy the specified region of the given image data to a new
 * destination, locating its anchor point at x, y.
 */
extern void LCDUIcopyArea(short *clip, void *dst,
			  int x_src, int y_src, int width, int height, 
			  int x_dest, int y_dest, int anchor);

/*
 * Draw the given image, provided as an array of 0xAARRGGBB integers,
 * to the display.  It's upper left corner will be located at (x,y).
 */
extern void LCDUIdrawRGB(jshort *clip, void *dst, jint **rgbData, 
			 jint offset, jint scanlen, jint x, jint y, 
			 jint width, jint height, jboolean processAlpha);

/*
 * Refresh the given area.  For double buffering purposes.
 */
extern void LCDUIrefresh(int x, int y, int w, int h);

/*
 * Get the ascent, descent and leading info for the font indicated 
 * by FONTPARAMS.
 */
extern void LCDUIgetFontInfo(FONTPARAMS_PROTO, 
                             int *ascent, int *descent, int *leading);

/*
 * Get the advance width for the first n characters in charArray if
 * they were to be drawn in the font indicated by FONTPARAMS.
 */
extern int LCDUIcharsWidth(FONTPARAMS_PROTO, const unicode *charArray, 
		           int n);

/*
 * Get parameters describing the window system's capabilities
 */
extern void LCDUIgetDisplayParams(int *width, int *height, int *adornedHeight,
                                  int *eraseColor, int *depth, int *flags,
				  int *numAlphaLevels);

/*
 * Set the softbuttons to handle the given list of abstract commands.
 */
extern void LCDUIupdateCommandSet(commandStruct *menu, int length);

/*
 * Initialize the menu system
 */
extern void LCDUIinitMenus();
extern void LCDUIfinalizeMenus();

/*
 * Handle an input event while the menu system is running
 */
extern int LCDUIhandleMenuEvent(enum KVMEventTypes t, int a, int b);

/*
 * Routines to hide/show the menu
 */
extern void LCDUIpaintMenu();
extern void LCDUIdismissMenu();

/*
 * Handle an input event while the menu system is running, but
 * without passing it through the event loop
 */
extern jboolean LCDUImenuInternalEvent(int button, KVMEventType *evt);

/*
 * ensure that static variables for the choice group popup menu 
 * are initialized 
 */
extern void LCDUIinitPopupMenu();

/*
 * cleanup native data structures related to the popup menu
 */
extern void LCDUIfinalizePopupMenu();

/*
 * called to keep the native popup choice group data structure
 * current before painting
 */
extern void LCDUIupdatePopupElements(popupElementStruct *menu, int length, 
			 int selectedIdx, int xTarget, int yTarget,
			 int vWidth, int vHeight, int maxItemWidth,
			 jboolean tickerFlag, jboolean titleFlag);

/*
 * cause the popup choice group to paint itself based on 
 * native data structures, which are kept updated with the
 * <code>LCDUIupdatePopupCommandSet</code> function
 */
extern void LCDUIpaintPopupMenu();

/*
 * cause the popup choice group to be removed from the screen
 */
extern void LCDUIdismissPopupMenu();

/*
 * handle events while the popup choice group is on screen without
 * passing them through the event loop
 */
extern jboolean LCDUIpopupMenuInternalEvent(int button, KVMEventType *evt);

/*
 * returns the most recently selected element (index) from a popup 
 * choice group
 */
extern int LCDUIgetCurrentPopupSelection(); 

/*
 * Initialize the phone simulator
 */
extern void LCDUIinitPhoneSim();

/*
 * Simulate an incoming phone call
 */
extern void LCDUIincomingCall();

/*
 * set a soft button label
 */
extern void LCDUIsetSoftButton(int index, const unicode *label, int numChars);

/*
 * set vertical scroll properties
 */
extern int LCDUIsetVerticalScroll(int scrollPosition, int scrollProportion);

/*
 * Play the indicated sound.  Return a boolean value to indicate whether
 * a sound was actually emitted.
 */
extern jboolean LCDUIplaySound(int soundType);
#define ALERT_INFO         1
#define ALERT_WARNING      2
#define ALERT_ERROR        3
#define ALERT_ALARM        4
#define ALERT_CONFIRMATION 5

/*
 * set input mode
 */
extern void LCDUIsetInputMode(int mode);

/*
 * set vertical scroll properties
 */
extern int LCDUIsetVerticalScroll(int scrollPosition, int scrollProportion);

/*
 * destroy native image resources; called by cleanup handler
 */
extern void LCDUIdestroyNativeImage(void* imagePtr);

/*
 * set the menu mode (fullscreen/normal)
 */
extern void LCDUISetEmulatorFullScreenMode(jboolean mode);

/*
 * Private initialization and finalization for windows/graphics system
 */
extern void InitializeWindowSystem();
extern void FinalizeWindowSystem();

/*
 * Private method to set the screen mode for commands and menus.
 */
extern void setCommandsFullScreenMode(jboolean mode);

extern jbyte
adjustToViewableRegion(jint *x_dest, jint *y_dest, jint *x_src, jint *y_src,
			jint *width, jint *height, jshort *clip, jint anchor, jint transform);

#endif
