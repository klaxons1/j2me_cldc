/*
 * @(#)nativeGUI.c	1.152 02/11/06 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <defaultLCDUI.h>
#include <nativeGUI.h>
#include <configuration.h>
#include <midpMalloc.h>
#include <midpServices.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Sunkeysym.h>
#include <X11/xpm.h>

#ifdef INCLUDE_I18N
#include "localeMethod.h"
#include "lcWin.h"
LcWinMethods lcWin=NULL;
static XIC xic=NULL;
static XIM xim=NULL;
#endif

#include "staticGraphics.h"

#define ARROWS_WIDTH  20
#define ARROWS_HEIGHT 18
#define PAINT_WIDTH  DISPLAY_WIDTH
#define BOTTOM_BAR_HEIGHT 19
#define EMULATOR_WIDTH  241
#define EMULATOR_HEIGHT 635
#define X_SCREEN X_SCREEN_OFFSET
static int y_screen;

/* Trusted MIDlet icon info */

#define TrustIcon_width     12
#define TrustIcon_height    12
#define TrustIcon_xposition (PAINT_WIDTH - 17 - 10 - TrustIcon_width)
#define TrustIcon_yposition 0

/* NOTE :
 * trusted midlet icon x position is =  
 * right corner (screenoffset+paintwidth)
 * - widht of battery (17 pixels)
 * - space between battery and right edge of icon (10 pixels)
 * - the width of the trusted icon 
 */


Display     *display = NULL;
Screen      *screen;
static Visual      *visual;
Colormap     colormap;
GC           gcErase = NULL;
static GC           gcNormal = NULL;
GC           gc=NULL;
static Cursor       handCursor, stylusCursor;
int          visualDepth;
int          numColors = 4;
static int initialized = 0;

/* a greenish pixel that looks kind of like a typical LCD screen */
XColor       lightPixel = {
    0, 0xaeae, 0xc0c0, 0xaeae, DoRed | DoGreen | DoBlue, 0 };
XColor       darkPixel = {
    0, 0x6060, 0x6060, 0x6060, DoRed | DoGreen | DoBlue, 0};

XColor       grayTable[16];
XColor       rgbTable[256];

static XtAppContext appContext;

Window       backgroundWindow;
Drawable     backgroundDrawable;

/* The bits of the topbar image:  drawn into the background Window */
XImage *topbar = None;

Window       paintWindow;
Drawable     paintDrawable;

jboolean       doubleBuffer = KNI_TRUE;

/* The led window. Put the led to be displayed here.*/
Window       LEDWindow;
Drawable     LEDDrawable;

/* The bits of the LED images */
XImage *LED_on_Image;
XImage *LED_off_Image;

/* The bits of the trusted MIDlet icon image */
XImage *trustedicon;

/* The BackLight window. Put the BackLight to be displayed here.*/
Window       BackLightWindow;
Drawable     BackLightDrawable;

/* The bits of the BackLight image */
XImage *BackLight_on_Image = None;
XImage *BackLight_off_Image = None;


/* Network Indicator status info */
int latestNICount = 0;
jboolean isLED_on = KNI_FALSE;
jlong latestNITime = 0;
#define NET_INDICATOR_DELAY 2500 /* 2.5 seconds */

/* fn prototypes */
void updateNetIndicator();
void drawNetIndicator(jboolean active);

/* The emulator window.  We put the "global areas" in here */
Window                outerWindow;
static Pixmap         casePixmap;

/* The image to use when we're iconified */
static Pixmap        dukeIcon;

/*  An atom that the window manager sends to ask the process to close */
static Atom          deleteAtom;

/*  An atom used for timer support */
static Atom          timerAtom;

/* An atom used for jsr135 EOM event */
Atom   mmEOMAtom;
 
/*  Atoms used for fetching underline parameters */
Atom          underlinePositionAtom;
Atom          underlineThicknessAtom;
Atom          rawPositionAtom;
Atom          rawThicknessAtom;

/* TEMPORARY, until we have multiple font support */
#define DEFAULT_FONT_NAME \
        "-*-lucidatypewriter-medium-r-normal-sans-10-*-*-*-*-*-*-*"
static XFontStruct *systemFontInfo= NULL;

#define INSIDE(e, _r)                                     \
    ((e.xbutton.x >= (_r).x) && (e.xbutton.x < ((_r).x + (_r).width)) &&  \
     (e.xbutton.y >= (_r).y) && (e.xbutton.y < ((_r).y + (_r).height)))
typedef struct  { 
    KeyType button;
    XRectangle bounds;
    char *name;
} Key;

static const Key keys[] = {
#ifdef NO_POWER_BUTTON
    /*
     * Add -DNO_POWER_BUTTON to the Makefile if you want to disable
     * the power button during user testing.
     */
    {KEY_POWER,    {-10, -10,  1,  1}, "POWER"},
#else
    {KEY_POWER,    {160, 59, 24, 24}, "POWER"},
#endif
    {KEY_SOFT1,    {32, 347, 36, 27}, "SOFT1"},
    {KEY_SOFT2,    {174, 347, 36, 27}, "SOFT2"},
    
    {KEY_UP,       {105, 343, 30, 25}, "UP"},
    {KEY_DOWN,     {105, 399, 30, 25}, "DOWN"},
    {KEY_LEFT,     {72, 370, 25, 25}, "LEFT"},
    {KEY_RIGHT,    {148, 370, 25, 25}, "RIGHT"},
    {KEY_SELECT,   {110, 371, 24, 24}, "SELECT"},
    
    {KEY_SEND,     {27, 387, 31, 36}, "SEND"},
    {KEY_END,      {187, 387, 31, 36}, "END"},
    {KEY_CLEAR,    {93, 427, 56, 29}, "CLEAR"},
    
    {KEY_1,        {36, 462, 39, 31}, "1"},
    {KEY_2,        {101, 466, 44, 28}, "2"},
    {KEY_3,        {170, 462, 39, 31}, "3"},
    {KEY_4,        {40, 499, 39, 31}, "4"},
    {KEY_5,        {101, 505, 44, 28}, "5"},
    {KEY_6,        {166, 500, 39, 31}, "6"},
    {KEY_7,        {46, 539, 39, 31}, "7"},
    {KEY_8,        {101, 543, 44, 28}, "8"},
    {KEY_9,        {158, 539, 39, 31}, "9"},
    {KEY_ASTERISK, {50, 582, 41, 23}, "*"},
    {KEY_0,        {101, 580, 44, 28}, "0"},
    {KEY_POUND,    {153, 582, 41, 23}, "#"},
} ;

#define refreshTopBar() \
XClearArea(display, backgroundWindow,\
	   0, 0, \
	   PAINT_WIDTH, topBarHeight, 0)

#define refreshBottomBar() \
XClearArea(display, backgroundWindow, \
	   0, topBarHeight + paintHeight, \
	   PAINT_WIDTH, BOTTOM_BAR_HEIGHT, 0)

#define refreshLEDArea() \
XClearArea(display, LEDWindow,0, 0, LED_width, LED_height, 0);

#define refreshBackLightArea() \
XClearArea(display, BackLightWindow, 0, 0, Bklite_width, Bklite_height, 0);

static const Key *lastButton = NULL;

typedef struct _applicationData { 
    Position x;
    Position y;
    Pixel foreground;
    Pixel background;
    char *title;
    int depth;
} applicationData;

extern void
convertText(FONTPARAMS_PROTO,
            const unicode *chars, int n, XTextItem16 **items, int *numItems,
            XTextItem16 *itemBuf, int itemBufSize,
            XChar2b   *charBuf, int charBufSize);
extern void
freeText(XTextItem16 *items, int numItems, 
         XTextItem16 *itemBuf, 
         XChar2b *charBuf, int charBufSize);
extern void
getTextBounds(XTextItem16 *items, int numItems, 
              int *width, int *ascent, int *descent);

static XtResource resources[] = { 
    { XtNx, XtCX, XtRPosition, sizeof(Position), 
      XtOffset(applicationData*, x), XtRImmediate, (void *)100 },
    { XtNy, XtCY, XtRPosition, sizeof(Position), 
      XtOffset(applicationData*, y), XtRImmediate, (void *)100 },
    { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 
      XtOffset(applicationData*, foreground), XtRString, "WHITE" },
    { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel), 
      XtOffset(applicationData*, background), XtRString, "BLACK" },
    { XtNtitle, XtCTitle, XtRString, sizeof(char *), 
      XtOffset(applicationData*, title), XtRImmediate, 0 },
    { XtNdepth, XtCDepth, XtRInt, sizeof(int),
      XtOffset(applicationData*, depth), XtRImmediate, (void *)1 },
};

/* REMIND: this is 3:3:2.  Consider 6x6x6 + 16 grays instead. */
#define GETRGBPIXEL(r, g, b) \
    rgbTable[((r & 0xe0) >> 0) | ((g & 0xe0) >> 3) | ((b & 0xc0) >> 6)].pixel

static int
getGrayPixel(int gray) 
{
    switch(numColors) {
    case 2: return grayTable[gray >> 7].pixel;
    case 4: return grayTable[gray >> 6].pixel;
    case 16: return grayTable[gray >> 4].pixel;
    default:
        fprintf(stderr, "Unsupported number of gray levels -- %d\n",
                numColors);
    }
    return 0;
}

static int 
getPixel(int r, int g, int b, int gray) {
    return numColors < 256 ? getGrayPixel(gray) : GETRGBPIXEL(r, g, b);
}

static XImage *
loadImage(unsigned char *bits, int width, int height)
{
    XImage *img =
	XCreateImage(display, visual,
		     1, XYBitmap, 0, (char*)bits, width, height, 8, 0);

    if (img != NULL) {
	img->bitmap_unit = 8;
    }

    return img;
}

/* ------- */

static int topBarHeight;
static int bottomBarHeight;
static int paintHeight;

static jboolean inFullScreenMode;
static jboolean requestedFullScreenMode;

static jboolean drawTrustedMIDletIcon;

void setUpOffsets(int fullscreen) {
  switch (fullscreen) { 
  case 1:
    topBarHeight    = topbar_height;
    bottomBarHeight = 0;
    break;
  case 0:
    topBarHeight    = topbar_height;
    bottomBarHeight = BOTTOM_BAR_HEIGHT;
    break;
  }
  
  paintHeight = (DISPLAY_HEIGHT 
		  - (topBarHeight + bottomBarHeight));
  
  y_screen = Y_SCREEN_OFFSET + topBarHeight;

}

void drawEmulatorScreen(jboolean fullscreen) {
    int status;
    Pixmap         topbarPixmap = None;
    XpmAttributes  topbarAttributes;
    
    if (KNI_FALSE == fullscreen) {
	
	if (topbar == None) {
	    
	    /* Load the topbar image */	
	    topbarAttributes.valuemask = XpmVisual | XpmDepth | 
		XpmReturnPixels | XpmCloseness | XpmSize | XpmColormap;
	    topbarAttributes.closeness = 40000; 
	    topbarAttributes.visual    = visual;
	    topbarAttributes.depth     = visualDepth;
	    topbarAttributes.colormap  = colormap;
	    
	    status = XpmCreatePixmapFromData(display, backgroundDrawable, 
					     topbar_xpm,
					     &topbarPixmap, NULL, 
					     &topbarAttributes);
	    topbar = XGetImage(display, topbarPixmap, 0, 0, 180, topbar_height,
			       0xffffffff, XYPixmap);
	
	    /* We keep the XImages around, so we don't need the pixmaps...*/
	    if (topbarPixmap != None) {
		XFreePixmap(display, topbarPixmap);
		topbarPixmap = None;
	    }
	
	    XpmFreeAttributes(&topbarAttributes);

	}
	XPutImage(display, backgroundDrawable, gcNormal, topbar, 
		  0, 0, 0, 0, 
		  topbar_width, topbar_height);

    }
    
    if (drawTrustedMIDletIcon == KNI_TRUE) {
      XPutImage(display, backgroundDrawable, gcNormal, trustedicon, 
		0, 0, TrustIcon_xposition, TrustIcon_yposition, 
		     TrustIcon_width, TrustIcon_height);
    }
    
    refreshTopBar();
    
    refreshBottomBar();
    
    XResizeWindow(display, paintWindow, PAINT_WIDTH, paintHeight);
    XMoveWindow(display, paintWindow, X_SCREEN, y_screen);
    
    XFlush(display);
}

/* Sets the top bar to display a trusted MIDlet icon,
 * then calls drawEmulatorScreen to cause an invalidate of 
 * the top bar
 */
void LCDUIdrawTrustedIcon(jboolean trusted) {
  
  drawTrustedMIDletIcon  = trusted;
  drawEmulatorScreen(inFullScreenMode);

}

void setEmulatorScreenMode(jboolean fullscreen) {
  setUpOffsets(fullscreen);
  drawEmulatorScreen(fullscreen);

  /* defined as static in defaultLCDUI.c  */
  inFullScreenMode = fullscreen;
}

void restoreEmulatorScreenMode() {
  if (inFullScreenMode != requestedFullScreenMode) {
    setEmulatorScreenMode(requestedFullScreenMode);
  }
}

void LCDUISetEmulatorFullScreenMode(jboolean mode) {

 /* defined as static in defaultLCDUI.c  */
 requestedFullScreenMode = mode;

 setCommandsFullScreenMode(mode);
 setEmulatorScreenMode(mode);

}

/* ------- */


static void
setWMHints(Window window, char *windowName, char *iconName, 
           int width, int height)
{ 
    XSizeHints *hints = XAllocSizeHints();
    XWMHints *wmhints = XAllocWMHints();
    
    XSetIconName(display, window, iconName);
    XStoreName(display, window, windowName);
    
    /* We have an exact width and height.  Do not resize. */
    hints->min_width = hints->max_width = hints->width = width;
    hints->min_height = hints->max_height = hints->height = height;
    hints->flags = PSize | PMaxSize | PMinSize | PPosition;
    XSetWMNormalHints(display, window, hints); 
    
    wmhints->icon_pixmap = dukeIcon;
    wmhints->input = True;
    wmhints->flags = InputHint | IconPixmapHint; 
    
    XSetWMHints(display, window, wmhints); 
    
    XFree(hints);
    XFree(wmhints);
}

static int
localXErrorHandler(Display *disp, XErrorEvent *err)
{
    char msg[128];
    char buf[128];
    XGetErrorText(disp, err->error_code, msg, sizeof(msg));
    fprintf(stderr, "Xerror %s\n", msg);
    sprintf(buf, "%d", err->request_code);
    XGetErrorDatabaseText(disp, "XRequest", buf, "Unknown", msg, sizeof(msg));
    fprintf(stderr, "Major opcode %d (%s)\n", err->request_code, msg);
    if (err->request_code > 128) {
		fprintf(stderr, "Minor opcode %d\n", err->minor_code);
    }
    return 0;
}

static int
localXIOErrorHandler(Display *disp)
{
    fprintf(stderr, "X I/O Error\n");
    return 0;
}

static    Pixmap pixmap = None;
static    XpmAttributes case_attributes;	

static 
Pixmap getBackgroundPixmap() {
    Window rootWindow = RootWindowOfScreen(screen);
    int status;

    case_attributes.valuemask = XpmVisual | XpmDepth | XpmReturnPixels | 
                                XpmCloseness | XpmSize | XpmColormap;
    case_attributes.closeness = 40000; 
    case_attributes.visual    = visual;
    case_attributes.depth     = visualDepth;
    case_attributes.colormap  = colormap;

    status = XpmCreatePixmapFromData(display, rootWindow, 
				     phone_xpm,
				     &pixmap, NULL, 
				     &case_attributes);

    if (status >= 0) { 
	XFillRectangle(display, pixmap, gcErase,
		       X_SCREEN, y_screen - topBarHeight,
		       DISPLAY_WIDTH, DISPLAY_HEIGHT);
    } else { 
	pixmap = (Pixmap)NULL;
    }

    return pixmap;
}

static int
setupColormap(int usePrivateColormap)
{
    int bg = 0;

    if (usePrivateColormap) {
        Colormap cmap = 
            XCreateColormap(display, RootWindowOfScreen(screen),
                            visual, AllocNone);

        if (cmap == None) {
            fprintf(stderr, "Couldn't create colormap");
            exit(-99);
        }

        colormap = cmap;
    }

    if (numColors < 256) {
        int i;
	/* 
	 * these numbers are gamma corrected so that 
	 * a monitor with gamma = 2.5 displays them as
         * linear percentages of gray.
	 */
        for (i = 0; i < numColors; ++i) {
            float factor = pow(((float)i/((float)numColors - 1.0)), 0.4);

            grayTable[i].red   = (int) (lightPixel.red   * factor);
            grayTable[i].green = (int) (lightPixel.green * factor);
            grayTable[i].blue  = (int) (lightPixel.blue  * factor);
            grayTable[i].flags = DoRed | DoGreen | DoBlue;

            if (!XAllocColor(display, colormap, grayTable + i)) {
                if (usePrivateColormap) {
                    fprintf(stderr, "Could not allocate gray pixels\n");
                    exit(1);
                } else {
                    return setupColormap(KNI_TRUE);
                }
            }
        }

        bg = grayTable[numColors - 1].pixel;

        if (usePrivateColormap) {
            /* include some colors that make it possible to show pixmap */
            int r, g, b;
            XColor tmp;

            tmp.flags = DoRed | DoGreen | DoBlue;

            for (r = 0; r < 256; r += 51) {
                for (g = 0; g < 256; g += 51) {
                    for (b = 0; b < 256; b += 51) {
                        tmp.red   = (r << 8) | r;
                        tmp.green = (g << 8) | g;
                        tmp.blue  = (b << 8) | b;

                        XAllocColor(display, colormap, &tmp);
                    }
                }
            }
        }
    } else if (numColors == 256) {
	int r, g, b, i;

	i = 0;
	for (r = 0; r <= 0xffff; r += 0xffff / 7) {
            if (r == 0xfffe) ++r; /* rounding error */

	    for (g = 0; g <= 0xffff; g += 0xffff / 7) {
                if (g == 0xfffe) ++g; /* rounding error */

		for (b = 0; b <= 0xffff; b += 0xffff / 3) {
		    rgbTable[i].red   = r;
		    rgbTable[i].green = g;
		    rgbTable[i].blue  = b;
		    rgbTable[i].flags = DoRed | DoGreen | DoBlue;

                    if (!XAllocColor(display, colormap, rgbTable + i)) {
                        if (usePrivateColormap) {
                            fprintf(stderr, "Could not allocate pixels\n");
                            exit(1);
                        } else {
                            return setupColormap(KNI_TRUE);
                        }
                    }

		    ++i;
		}
	    }
	}

        bg = rgbTable[numColors - 1].pixel;
    }

    lightPixel.pixel = bg;

    return bg;
}

static int
countBits(int mask)
{
    int count = 0;
    while (mask != 0) {
        /* clear the lowest bit that is set */
        mask &= (mask - 1);
        ++count;
    }

    return count;
}

/*
 * Create the Emulator Window and associated resources.
 */
static    XVisualInfo *visualInfo = NULL;

static void
createEmulatorWindow() {
    static char *argv[] = {"lcdui"};
    Widget shellWidget;
    applicationData appData;
    char *s;
    int argc = 1;
    int fg, bg;
    int status;
    XGCValues vals;
    int GCbase = GCFunction | GCForeground | GCBackground | GCGraphicsExposures;
    XSetWindowAttributes attributes;
    int valuemask;
    XVisualInfo vi;
    Pixmap         greenLEDPixmap = None;
    Pixmap         grayLEDPixmap = None;
    XpmAttributes  LEDAttributes;

    Pixmap         backLightPixmap = None;
    Pixmap         backLightOffPixmap = None;
    XpmAttributes  BackLightAttributes;

    Pixmap         trustediconPixmap = None;
    XpmAttributes  trustediconAttributes;

    int n = 0, mask = VisualClassMask | VisualScreenMask;
    long outerEvtMask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask | StructureNotifyMask | FocusChangeMask |
        KeyPressMask | KeyReleaseMask;

    if (initialized) {
        return;
    } else {
        initialized = 1;
    }
    
#ifdef INCLUDE_I18N
    lcWin = getLcWinMethods(getLocaleName());
#endif

    XSetErrorHandler(localXErrorHandler);
    XSetIOErrorHandler(localXIOErrorHandler);

    shellWidget = XtOpenApplication(&appContext, "lcdui", NULL,
				    0, &argc, argv,
				    NULL, applicationShellWidgetClass, 
				    NULL, 0);
    
    display  = XtDisplay(shellWidget);
    screen   = DefaultScreenOfDisplay(display);
    colormap = DefaultColormapOfScreen(screen);
    visual   = DefaultVisualOfScreen(screen);

    XtGetApplicationResources(shellWidget, &appData, 
			      resources, XtNumber(resources), NULL, 0);

    if ((s = getInternalProp("system.display.double_buffered")) != NULL) {
        doubleBuffer = !strcasecmp(s, "true");
    }

    /* use environment variable if present */
    if ((s = getInternalProp("system.display.screen_depth")) != NULL) {
	if (!strcmp(s, "1")) {
	    numColors = 2;
	} else if (!strcmp(s, "2")) {
	    numColors = 4;
        } else if (!strcmp(s, "4")) {
            numColors = 16;
	} else if (!strcmp(s, "8")) {
	    numColors = 256;
	}
    } else if (appData.depth == 2) {
	numColors = 4;
    } else if (appData.depth == 4) {
        numColors = 16;
    } else if (appData.depth == 8) {
	numColors = 256;
    } else if (appData.depth != 1) {
	numColors = 2;
	fprintf(stderr, "Warning: unsupported depth.  Using monochrome\n");
    }

    s = getInternalProp("system.display.visual_type");

    vi.screen = DefaultScreen(display);
    if (s == NULL) {
        if ((numColors == 256) && (visual->class == PseudoColor)) {
            /* try to get StaticColor */
            vi.class = StaticColor;
            vi.colormap_size = 256;
            mask |= VisualColormapSizeMask;
        } else {
            mask = VisualIDMask | VisualScreenMask;
            vi.visualid = visual->visualid;
        }
    } else if (!strcmp(s, "StaticGray")) {
        vi.class = StaticGray;
    } else if (!strcmp(s, "GrayScale")) {
        vi.class = GrayScale;
    } else if (!strcmp(s, "StaticColor")) {
        vi.class = StaticColor;
        vi.colormap_size = 256;
        mask |= VisualColormapSizeMask;
    } else if (!strcmp(s, "PseudoColor")) {
        vi.class = PseudoColor;
        vi.colormap_size = 256;
        mask |= VisualColormapSizeMask;
    } else if (!strcmp(s, "TrueColor")) {
        vi.class = TrueColor;
    } else if (!strcmp(s, "DirectColor")) {
        vi.class = DirectColor;
    } else {
        fprintf(stderr, "Unrecognized visual type %s\n", s);
    }
    
    if ((visualInfo = XGetVisualInfo(display, mask, &vi, &n)) == NULL) {
        mask = VisualIDMask | VisualScreenMask;
        vi.visualid = visual->visualid;
        visualInfo = XGetVisualInfo(display, mask, &vi, &n);
    }

    if (   (numColors == 256) 
        && ((visual->visualid == StaticColor) && (s == NULL)) ) {
        /* we forced StaticColor, to try to avoid flashing. */
        /* see if we really have a 3:3:2 structure!         */
        if (   (countBits(visualInfo->red_mask) != 3)
            || (countBits(visualInfo->green_mask) != 3)
            || (countBits(visualInfo->blue_mask) != 2)   ) {

            mask = VisualIDMask | VisualScreenMask;
            vi.visualid = visual->visualid;
            visualInfo = XGetVisualInfo(display, mask, &vi, &n);
        }
    }

    if (visualInfo == NULL) {
        fprintf(stderr, "XGetVisualInfo failed\n");
        exit(-8);
    } else if (n > 1) {
        /* find the one with the best depth */
        XVisualInfo *best = visualInfo;
        int i;

        for (i = 1; i < n; ++i) {
            if (   (visualInfo[i].depth > best->depth)
                || (visualInfo[i].colormap_size > best->colormap_size)) {
                best = visualInfo + i;
            }
        }

        visual      = best->visual;
        visualDepth = best->depth;
    } else {
        visual      = visualInfo->visual;
        visualDepth = visualInfo->depth;
    }

    if (getInternalProp("system.display.debug_screen")) {
	XSynchronize(display, 1);
    }

#if 0
    /* Create the "outer" window */
    outerWindow = XCreateSimpleWindow(display, 
				      RootWindowOfScreen(screen),
				      appData.x, appData.y, 
				      EMULATOR_WIDTH, EMULATOR_HEIGHT, 0,
				      appData.background,
				      appData.foreground);
#endif

    if (visual != DefaultVisualOfScreen(screen)) {
        fprintf(stderr, "visual id 0x%02x, depth %d\n", 
                visual->visualid, visualDepth);
    }

    bg = setupColormap(visual != DefaultVisualOfScreen(screen));

    valuemask = 0;

    valuemask |= CWBackPixel;
    attributes.background_pixel = bg;

    valuemask |= CWBorderPixel;
    attributes.border_pixel = bg;

    valuemask |= CWColormap;
    attributes.colormap = colormap;

    outerWindow = XCreateWindow(display, RootWindowOfScreen(screen),
                                appData.x, appData.y, 
                                EMULATOR_WIDTH, EMULATOR_HEIGHT, 0,
                                visualDepth, InputOutput, visual,
                                valuemask, &attributes);

    XSelectInput(display, outerWindow, outerEvtMask);

    fg = (numColors == 256) ? rgbTable[0].pixel : grayTable[0].pixel;
    bg = getPixel(0xaf, 0xaf, 0xaf, 0xaf); 

    vals.foreground = fg;
    vals.background = bg;
    vals.function = GXcopy;
    vals.graphics_exposures = KNI_FALSE;
    gcNormal = XCreateGC(display, outerWindow, GCbase, &vals);

    vals.foreground = bg;
    gcErase = XCreateGC(display, outerWindow, GCbase, &vals);
    
    casePixmap = getBackgroundPixmap();

    XSetWindowBackgroundPixmap(display, outerWindow, casePixmap);
    
    XDefineCursor(display, outerWindow, 
		  handCursor = XCreateFontCursor(display, XC_hand2));

    /* Create a "background" window
       to display the status bar and the command labels */

    valuemask = 0;

    backgroundWindow = XCreateWindow(display, outerWindow,
				     X_SCREEN, Y_SCREEN_OFFSET,
				     PAINT_WIDTH, DISPLAY_HEIGHT, 0,
				     visualDepth, InputOutput, visual,
				     valuemask, &attributes);

    gc = XCreateGC(display, backgroundWindow, GCbase, &vals);

    XSelectInput(display, backgroundWindow, 
		 ButtonPressMask | ButtonReleaseMask | 
		 ButtonMotionMask | KeyPressMask | KeyReleaseMask);
    
    XDefineCursor(display, backgroundWindow,
                  stylusCursor = XCreateFontCursor(display, XC_pencil));

    /* 
     * We need a separate bitmap to serve as the drawable for the status bar
     * and the command labels.  We write to the bitmap, and let it serve 
     * as the background for the window.
     */
    backgroundDrawable = XCreatePixmap(display, backgroundWindow, 
				       PAINT_WIDTH, DISPLAY_HEIGHT,
				       visualDepth);
    XFillRectangle(display, backgroundDrawable, gcErase,
		   0, 0, PAINT_WIDTH, DISPLAY_HEIGHT);

    XSetWindowBackgroundPixmap(display, backgroundWindow, backgroundDrawable);


    /* Create the "paint" window */
#if 0
    paintWindow = XCreateSimpleWindow(display, outerWindow, 
				      X_SCREEN, y_screen, 
				      PAINT_WIDTH, paintHeight, 0,
				      appData.background,
				      appData.foreground);
#endif

    valuemask = 0;

    paintWindow = XCreateWindow(display, outerWindow,
                                X_SCREEN, Y_SCREEN_OFFSET,
                                PAINT_WIDTH, DISPLAY_HEIGHT, 0,
                                visualDepth, InputOutput, visual,
                                valuemask, &attributes);

    gc = XCreateGC(display, paintWindow, GCbase, &vals);

    XSelectInput(display, paintWindow, 
		 ButtonPressMask | ButtonReleaseMask | 
		 ButtonMotionMask | KeyPressMask | KeyReleaseMask);

    XDefineCursor(display, paintWindow,
                  stylusCursor = XCreateFontCursor(display, XC_pencil));


    /* 
     * We need a separate bitmap to serve as the drawable for the screen area
     */
    paintDrawable = XCreatePixmap(display, paintWindow, 
				  PAINT_WIDTH, DISPLAY_HEIGHT,
				  visualDepth);
    XFillRectangle(display, paintDrawable, gcErase,
		   0, 0, PAINT_WIDTH, DISPLAY_HEIGHT);

    XSetWindowBackgroundPixmap(display, paintWindow, paintDrawable);

    drawEmulatorScreen(inFullScreenMode);


    /* Create the Backlight window */

    valuemask = 0;

    BackLightWindow = XCreateWindow(display, outerWindow,
			      Bklite_xposition, Bklite_yposition, 
			      Bklite_width, Bklite_height, 0,
			      visualDepth, InputOutput, visual,
			      valuemask, &attributes);
    
    gc = XCreateGC(display, BackLightWindow, GCbase, &vals);

    /*the same event mask as the outer window*/
    XSelectInput(display, BackLightWindow, outerEvtMask);

    XDefineCursor(display, BackLightWindow,
                  stylusCursor = XCreateFontCursor(display, XC_pencil));

    /*Load the BackLight images*/
    
    BackLightAttributes.valuemask = XpmVisual | XpmDepth | XpmReturnPixels | 
                                XpmCloseness | XpmSize | XpmColormap;
    BackLightAttributes.closeness = 40000; 
    BackLightAttributes.visual    = visual;
    BackLightAttributes.depth     = visualDepth;
    BackLightAttributes.colormap  = colormap;

    status = XpmCreatePixmapFromData(display, BackLightWindow, 
				     backLight_xpm,
				     &backLightPixmap, NULL, 
				     &BackLightAttributes);

    status = XpmCreatePixmapFromData(display, BackLightWindow, 
				     backLight_Off_xpm,
				     &backLightOffPixmap, NULL, 
				     &BackLightAttributes);

    BackLight_off_Image = XGetImage(display, backLightOffPixmap, 0, 0, 241, 232,
			     0xffffffff, XYPixmap);

    BackLight_on_Image = XGetImage(display, backLightPixmap, 0, 0, 241, 232,
			     0xffffffff, XYPixmap);

    /* We keep the XImages around, so we don't need the pixmaps...*/
    if (backLightOffPixmap != None) {
	XFreePixmap(display,backLightOffPixmap);
	backLightOffPixmap = None;
    }

    if (backLightPixmap != None) {
	XFreePixmap(display,backLightPixmap);
	backLightPixmap = None;
    }

    /* 
     * We need a separate bitmap to serve as the drawable for the screen area
     */
    BackLightDrawable = XCreatePixmap(display, BackLightWindow, 
				Bklite_width, Bklite_height,
				visualDepth);

    XFillRectangle(display, BackLightDrawable, gcErase,
		   0, 0, Bklite_width, Bklite_height);

    XSetWindowBackgroundPixmap(display, BackLightWindow, BackLightDrawable);

    XPutImage(display, BackLightDrawable, gcNormal, BackLight_off_Image, 0, 0, 0, 0, Bklite_width, Bklite_height);

    /* Create the "LED" window for the network indicator */

    valuemask = 0;

    LEDWindow = XCreateWindow(display, outerWindow,
			      LED_xposition, LED_yposition, 
			      LED_width, LED_height, 0,
			      visualDepth, InputOutput, visual,
			      valuemask, &attributes);
    
    gc = XCreateGC(display, LEDWindow, GCbase, &vals);

    /*the same event mask as the outer window*/
    XSelectInput(display, LEDWindow, outerEvtMask);

    XDefineCursor(display, LEDWindow,
                  stylusCursor = XCreateFontCursor(display, XC_pencil));

    /*Load the LED images*/
    
    LEDAttributes.valuemask = XpmVisual | XpmDepth | XpmReturnPixels | 
                                XpmCloseness | XpmSize | XpmColormap;
    LEDAttributes.closeness = 40000; 
    LEDAttributes.visual    = visual;
    LEDAttributes.depth     = visualDepth;
    LEDAttributes.colormap  = colormap;

    status = XpmCreatePixmapFromData(display, LEDWindow, 
				     grayLED_xpm,
				     &grayLEDPixmap, NULL, 
				     &LEDAttributes);

    status = XpmCreatePixmapFromData(display, LEDWindow,
				     greenLED_xpm,
				     &greenLEDPixmap, NULL,
				     &LEDAttributes);

    LED_on_Image = XGetImage(display, greenLEDPixmap, 0, 0, 20, 20,
			     0xffffffff, XYPixmap);
    LED_off_Image = XGetImage(display, grayLEDPixmap, 0, 0, 20, 20,
			     0xffffffff, XYPixmap);

    /* We keep the XImages around, so we don't need the pixmaps...*/
    if (grayLEDPixmap != None) {
	XFreePixmap(display,grayLEDPixmap);
	grayLEDPixmap = None;
    }

    if (greenLEDPixmap != None) {
	XFreePixmap(display, greenLEDPixmap);
	greenLEDPixmap = None;
    }
    XpmFreeAttributes(&LEDAttributes);

    /* 
     * We need a separate bitmap to serve as the drawable for the screen area
     */
    LEDDrawable = XCreatePixmap(display, LEDWindow, 
				LED_width, LED_height,
				visualDepth);

    XSetWindowBackgroundPixmap(display, LEDWindow, LEDDrawable);

    drawNetIndicator(KNI_FALSE);

    if (trustedicon == None) {
      
      /* Load the topbar image */	
      trustediconAttributes.valuemask = XpmVisual | XpmDepth | 
	XpmReturnPixels | XpmCloseness | XpmSize | XpmColormap;
      trustediconAttributes.closeness = 40000; 
      trustediconAttributes.visual    = visual;
      trustediconAttributes.depth     = visualDepth;
      trustediconAttributes.colormap  = colormap;
      
      status = XpmCreatePixmapFromData(display, backgroundDrawable, 
				       trustedicon_xpm,
				       &trustediconPixmap, NULL, 
				       &trustediconAttributes);
      trustedicon = XGetImage(display, trustediconPixmap, 
			      0, 0, TrustIcon_width, TrustIcon_height,
			      0xffffffff, XYPixmap);
      
      /* We keep the XImages around, so we don't need the pixmaps...*/
      if (trustediconPixmap != None) {
	XFreePixmap(display, trustediconPixmap);
	trustediconPixmap = None;
      }
      
      XpmFreeAttributes(&trustediconAttributes);
      
    }

    /* TEMPORARY */
    systemFontInfo = XLoadQueryFont(display, DEFAULT_FONT_NAME);

    /*  Let the window manager tell us about close events */
    deleteAtom = XInternAtom(display, "WM_DELETE_WINDOW", KNI_FALSE);
    XSetWMProtocols(display, outerWindow, &deleteAtom, 1);
    
    /*  Indicate that we don't want to be resized. */
    setWMHints(outerWindow, "lcdui", "lcdui", 
               EMULATOR_WIDTH, EMULATOR_HEIGHT);
    
    /*  Create timer atom */
    timerAtom = XInternAtom(display, "MIDP_CLIENT_TIMER_ATOM", KNI_FALSE);

    /* create mmEOM atom */
    mmEOMAtom = XInternAtom(display, "MM_EOM_ATOM", KNI_FALSE);

    /*  Create underlining atoms */
    underlinePositionAtom  = XInternAtom(display, "UNDERLINE_POSITION", KNI_FALSE);
    underlineThicknessAtom = XInternAtom(display, "UNDERLINE_THICKNESS", KNI_FALSE);
    rawPositionAtom  = XInternAtom(display, "RAW_UNDERLINE_POSITION", KNI_FALSE);
    rawThicknessAtom = XInternAtom(display, "RAW_UNDERLINE_THICKNESS", KNI_FALSE);

#ifdef INCLUDE_I18N
    if (lcWin) {
        lcWin->init(display);
    }
    {
        unsigned long fevent;
        xim = XOpenIM(display, NULL, NULL, NULL);
        if (xim) {
            xic = XCreateIC(xim,
                            XNInputStyle, 
                            (XIMPreeditNothing | XIMStatusNothing),
                            XNClientWindow, outerWindow,
                            XNFocusWindow, outerWindow,
                            NULL);
            if (xic) {
                XGetICValues(xic, XNFilterEvents, &fevent, NULL);
                outerEvtMask |= fevent;
                XSelectInput(display, outerWindow, outerEvtMask);
            }
        }
    }
#endif

    /*  Make sure that everything is visible     */
    XMapRaised(display, outerWindow);
    XMapRaised(display, BackLightWindow);
    XMapRaised(display, backgroundWindow);
    XMapRaised(display, paintWindow);

    XMapRaised(display, LEDWindow);

    /*    XMapSubwindows(display, outerWindow); */
    
    XFlush(display);
}

void
refreshPaintWindow(int x1, int y1, int x2, int y2)
{
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    XClearArea(display, paintWindow, x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0);
    XFlush(display);
}

static Pixmap stipple33 = None;
static Pixmap stipple50 = None;
static Pixmap stipple66 = None;

Drawable
setupGC(GC gc, int pixel, short *clip, void* dst, int dotted)
{
    /*
    static char dash33[] = {2, 1};
    static char dash50[] = {1, 1};
    static char dash66[] = {1, 2};
    static char dotted[] = {3, 2, 2, 3};

    if (lineStyle == DOTTED) {
        XSetDashes(display, gc, 0, dotted, 4);
        XSetLineAttributes(display, gc, 1, 
                           LineOnOffDash, CapButt, JoinMiter);
    } else {
        XSetLineAttributes(display, gc, 1, 
                           LineSolid, CapButt, JoinMiter);
    }

    if (numColors == 2) {
        if ((gray >= 115) && (gray <= 140)) {
            XSetForeground(display, gc, getGrayPixel(0));
            XSetStipple(display, gc, stipple50);
            XSetFillStyle(display, gc, FillStippled);
            XSetDashes(display, gc, 0, dash50, 2);
            XSetLineAttributes(display, gc, 1, 
                               LineOnOffDash, CapButt, JoinMiter);
        } else {
            switch (gray >> 6) {
            case 0:
            case 3:
                XSetForeground(display, gc, getGrayPixel(gray));
                XSetFillStyle(display, gc, FillSolid);
                XSetLineAttributes(display, gc, 1, 
                                   LineSolid, CapButt, JoinMiter);
                break;

            case 1:
                XSetForeground(display, gc, getGrayPixel(0));
                XSetStipple(display, gc, stipple33);
                XSetDashes(display, gc, 0, dash33, 2);
                XSetLineAttributes(display, gc, 1, 
                                   LineOnOffDash, CapButt, JoinMiter);
                break;

            case 2:
                XSetForeground(display, gc, getGrayPixel(0));
                XSetStipple(display, gc, stipple66);
                XSetDashes(display, gc, 0, dash66, 2);
                XSetLineAttributes(display, gc, 1, 
                                   LineOnOffDash, CapButt, JoinMiter);
                break;
            }
        }

        if (lineStyle == DOTTED) {
            XSetDashes(display, gc, 0, dotted, 4);
            XSetLineAttributes(display, gc, 1, 
                               LineOnOffDash, CapButt, JoinMiter);
        }
    } else if (numColors == 256) {
        XSetForeground(display, gc, getColorPixel(color));
    } else {
        XSetForeground(display, gc, getGrayPixel(gray));
    }
    */

    XSetForeground(display, gc, pixel);
    if (clip) {
        XSetClipRectangles(display, gc, 
			   0, 0, (XRectangle*)clip, 1, YXBanded);
    } else {
        XSetClipMask(display, gc, None);
    }

    if (dotted) {
        static char dotted[] = {1, 1};
        XSetDashes(display, gc, 0, dotted, 2);
        XSetLineAttributes(display, gc, 1, 
                           LineOnOffDash, CapButt, JoinMiter);
    } else {
        XSetLineAttributes(display, gc, 1, 
                           LineSolid, CapButt, JoinMiter);
    }

    if (dst == NULL) {
        return paintDrawable;
    } else {
        return *((Pixmap *)dst);
    }
}

/*
 * Return the key code corresponding to the given abstract game action.
 */
int 
LCDUIgetKeyCode(int gameAction)
{
    switch (gameAction) {
    case  1: return KEY_UP;
    case  6: return KEY_DOWN;
    case  2: return KEY_LEFT;
    case  5: return KEY_RIGHT;
    case  8: return KEY_SELECT;
    case  9: return KEY_1;
    case 10: return KEY_3;
    case 11: return KEY_7;
    case 12: return KEY_9;

    default: return 0;
    }
}

/*
 * Return the abstract game action corresponding to the given key code.
 */
int 
LCDUIgetGameAction(int keyCode)
{
    switch (keyCode) {
    case KEY_UP: 
    case KEY_2:
        return 1;
    case KEY_DOWN: 
    case KEY_8:
        return 6;
    case KEY_LEFT:
    case KEY_4:
        return 2;
    case KEY_RIGHT:
    case KEY_6:
        return 5;
    case KEY_SELECT:
    case KEY_5:
        return 8;
    case KEY_1: return 9;
    case KEY_3: return 10;
    case KEY_7: return 11;
    case KEY_9: return 12;

    case KEY_SOFT1:
    case KEY_SOFT2:
    case KEY_0:
    case KEY_ASTERISK:
    case KEY_POUND:
    case KEY_POWER:
    case KEY_SEND:
    case KEY_END:
    case KEY_CLEAR:
        return 0;

    default:
        return -1;
    }
}

int
LCDUIgetSystemKey(int keyCode)
{
    switch (keyCode) {
    case KEY_POWER: return 1;
    case KEY_SEND:  return 2;
    case KEY_END:   return 3;
    case KEY_CLEAR: return 4;
    default:        return 0;
    }
}


/*
 * Return the key name corresponding to the given key code.
 */
char *
LCDUIgetKeyName(int keyCode)
{
    switch (keyCode) {
    case KEY_POWER:    return keys[0].name;
    case KEY_SEND:     return keys[8].name;
    case KEY_END:      return keys[9].name;
    case KEY_CLEAR:    return keys[10].name;
    case KEY_SOFT1:    return keys[1].name;
    case KEY_SOFT2:    return keys[2].name;
    case KEY_UP:       return keys[3].name;
    case KEY_DOWN:     return keys[4].name;
    case KEY_LEFT:     return keys[5].name;
    case KEY_RIGHT:    return keys[6].name;
    case KEY_SELECT:   return keys[7].name;
    case KEY_1:        return keys[11].name;
    case KEY_2:        return keys[12].name;
    case KEY_3:        return keys[13].name;
    case KEY_4:        return keys[14].name;
    case KEY_5:        return keys[15].name;
    case KEY_6:        return keys[16].name;
    case KEY_7:        return keys[17].name;
    case KEY_8:        return keys[18].name;
    case KEY_9:        return keys[19].name;
    case KEY_ASTERISK: return keys[20].name;
    case KEY_0:        return keys[21].name;
    case KEY_POUND:    return keys[22].name;
    default:           return NULL;
    }
}

#define NUM_FONTS 36
 XFontStruct *fonts[NUM_FONTS];

XFontStruct *
findFontStruct(Font font)
{
    int i;

#ifdef INCLUDE_I18N
    if (lcWin) {
        return lcWin->getFontStruct(font);
    }
#endif

    for (i = 0; i < NUM_FONTS; ++i) {
        if (fonts[i] && (fonts[i]->fid == font)) {
            return fonts[i];
        }
    }

    return NULL;
}

void LCDUIrefresh(int x1, int y1, int x2, int y2)
{
    XClearArea(display, paintWindow, x1, y1, x2-x1, y2-y1, 0);
    XFlush(display);
}

void LCDUIgetDisplayParams(int *width, int *height, int *adornedHeight,
                           int *eraseColor, int *depth, int *flags, int *alphaLevels)
{
    *width  = PAINT_WIDTH;
    *height = DISPLAY_HEIGHT - topbar_height;
    *adornedHeight = DISPLAY_HEIGHT - (topbar_height + BOTTOM_BAR_HEIGHT);
    *eraseColor = 0x00ffffff;
    *depth  = (numColors == 16) ? 4 : (numColors == 256) ? 8 : numColors / 2;
    *flags  = ((numColors >= 256 ) ? SUPPORTS_COLOR : 0) 
        | SUPPORTS_POINTER 
        | SUPPORTS_MOTION 
        | (doubleBuffer ? SUPPORTS_DOUBLEBUFFER : 0);
    *alphaLevels = numColors;
}

void 
LCDUIsetSoftButton(int index, const unicode *label, int numChars)
{
    /* X_OFFSET is the offset from the left of the drawbale*/
    #define X_OFFSET 0

    int x = X_OFFSET, y = topBarHeight + paintHeight;

    int width, ascent, descent;

    XTextItem16 *items, itemBuf[4];
    XChar2b charBuf[80];
    int numItems;
    XRectangle r;

    /* Check to see that we are being called in full screen mode.
       In such a case we should return without doing anything.
       Ideally we shouldn't be being called at all. However,
       there is at least one place in the code (in menus.c)
       where we are called from within a code block
       that is executed if we are in fullscreen mode.
     */
    if (KNI_TRUE == inFullScreenMode) { 
      return;
    }

    if ((index < 0) || (index > 1)) return;

    if (index == 0) {
        /* clear rectangle */
        XFillRectangle(display, backgroundDrawable, gcErase, 
                       x, y,
                       (PAINT_WIDTH - ARROWS_WIDTH)/2,
                       ARROWS_HEIGHT);
    } else {
        /* clear rectangle */
        XFillRectangle(display, backgroundDrawable, gcErase,
                       x + (PAINT_WIDTH + ARROWS_WIDTH)/2, y,
                       (PAINT_WIDTH - ARROWS_WIDTH)/2,
                       ARROWS_HEIGHT);
    }

    if (label == NULL) {
        refreshBottomBar();
        return;
    }
    
    /* we must convert all of the text in order to get correct width */
    /* This makes malloc() unavoidable if the string is long or has  */
    /* many changes of font.  We try to avoid it by providing some   */
    /* data space and using malloc only when the space isn't enough. */

    convertText(FACE_SYSTEM, STYLE_UNDERLINED, SIZE_MEDIUM,
                label, numChars, &items, &numItems, itemBuf, 4, charBuf, 80);
    getTextBounds(items, numItems, &width, &ascent, &descent);

    if (index == 0) {
        x = X_OFFSET + 2;       /* left justified */
        r.x = x;
    } else {
        x = X_OFFSET + PAINT_WIDTH - width - 2; /* right justified */
        r.x = X_OFFSET + (PAINT_WIDTH + ARROWS_WIDTH) / 2;

        if (x < r.x) x = r.x; /* clip away the last chars, not the first */
    }

    r.y = y; 
    r.width = (PAINT_WIDTH - ARROWS_WIDTH) / 2 - 1;
    r.height = ARROWS_HEIGHT;

    XSetClipRectangles(display, gcNormal, 0, 0, &r, 1, YXBanded);

    y += ARROWS_HEIGHT - (descent + 1);

    XDrawText16(display, backgroundDrawable, gcNormal, x, y, items, numItems);
    freeText(items, numItems, itemBuf, charBuf, 80);

    XSetClipMask(display, gcNormal, None);
    refreshBottomBar();
}

#define VERT_X   ((PAINT_WIDTH/2) - 3)
#define HORIZ_Y  (y_screen + paintHeight + 6)
#define LEFT_X   (X_SCREEN + (PAINT_WIDTH/2) - 10)
#define RIGHT_X  (X_SCREEN + (PAINT_WIDTH/2) + 3)
#define UP_Y     (DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT + 1)
#define DOWN_Y   (DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT + 11)

static XImage         *upArrow, *downArrow;

static void
paintVerticalScroll(
    int scrollPosition,
    int scrollProportion,
    Drawable drawable)
{

    if (KNI_TRUE == inFullScreenMode) {
      return;
    }

    if (upArrow == NULL) {
	upArrow    = loadImage(up_bits, arrow_width, arrow_height);
	upArrow->bitmap_bit_order = LSBFirst;
	downArrow  = loadImage(down_bits,  arrow_width, arrow_height);
	downArrow->bitmap_bit_order = LSBFirst;
    }

    /* If the scrollable is 100% the proportion of the viewport,    */
    /* there's no need for any scrolling, clear the arrows and      */
    /* return.                                                      */
    if (scrollProportion >= 100 || scrollProportion == 0) {
	XFillRectangle(display, drawable, gcErase,
		       VERT_X, UP_Y, arrow_width, arrow_height);
	XFillRectangle(display, drawable, gcErase,
		       VERT_X, DOWN_Y, arrow_width, arrow_height);
        XFlush(display);
        return;
    }
    
    /* If we're somewhere between the top and bottom,   */
    /* add both scroll arrows                           */
    if (scrollPosition > 0 && scrollPosition < 100) {
	XPutImage(display, drawable, gcNormal, upArrow,
		  0, 0, VERT_X, UP_Y, arrow_width, arrow_height);
	XPutImage(display, drawable, gcNormal, downArrow,
		  0, 0, VERT_X, DOWN_Y, arrow_width, arrow_height);
	/* If we're at the top, add the down arrow, clear the   */
	/* up arrow                                             */
    } else if (scrollPosition == 0) {
	XPutImage(display, drawable, gcNormal, downArrow,
		  0, 0, VERT_X, DOWN_Y, arrow_width, arrow_height);
	XFillRectangle(display, drawable, gcErase,
		       VERT_X, UP_Y, arrow_width, arrow_height);
	/* We're at the bottom, add the up arrow, clear the     */
	/* down arrow                                           */
    } else {
	XPutImage(display, drawable, gcNormal, upArrow,
		  0, 0, VERT_X, UP_Y, arrow_width, arrow_height);
	XFillRectangle(display, drawable, gcErase,
		       VERT_X, DOWN_Y, arrow_width, arrow_height);
    }
    
    XFlush(display);
}

static int vScrollPosition, vScrollProportion;

int
LCDUIsetVerticalScroll(int scrollPosition, int scrollProportion)
{
    int tmp1 = vScrollPosition;

    vScrollPosition = scrollPosition;
    vScrollProportion = scrollProportion;

    paintVerticalScroll(scrollPosition, scrollProportion, backgroundDrawable);
    refreshBottomBar();

    return tmp1;
}

jboolean LCDUIplaySound(int soundType)
{
    if (   (soundType == ALERT_WARNING) 
        || (soundType == ALERT_ERROR)
        || (soundType == ALERT_ALARM)   ) {
        XBell(display, 0);
        XFlush(display);
        return KNI_TRUE;
    } else {
        return KNI_FALSE;
    }
}

/*
 * Draw the BackLight.
 * If 'active' is KNI_TRUE, the BackLight is drawn.
 * If 'active' is KNI_FALSE, the BackLight is erased.
 */
void drawBackLight(jboolean active) {

    if (active == KNI_TRUE) {
        XPutImage(display, BackLightDrawable, gcNormal, BackLight_on_Image, 0, 0,
	      0, 0, Bklite_width, Bklite_height);
	refreshBackLightArea();
    } else {
        XPutImage(display, BackLightDrawable, gcNormal, BackLight_off_Image, 0, 0,
	      0, 0, Bklite_width, Bklite_height);
	refreshBackLightArea();
    }    
}



/*
 * Requests a flashing effect for the device's backlight. 
 */ 
jboolean LCDUIshowBacklight(jboolean mode)
{
     drawBackLight(mode);
     return KNI_TRUE;
}

void
LCDUIsetInputMode(int mode)
{
#include "inputmodes.xbm"
    static XImage *ABCMode, *abcMode, *numMode, *SYMMode;
#ifdef INCLUDE_I18N
#include "inputmodes_ja.xbm"
    static XImage *hiraMode, *kanaMode;
#endif
    static int x, y;
    XImage *icon = NULL;

    if (ABCMode == NULL) {
	ABCMode = loadImage(ABC_bits, inputmode_width, inputmode_height);
	ABCMode->bitmap_bit_order = LSBFirst;
	abcMode = loadImage(abc_bits, inputmode_width, inputmode_height);
	abcMode->bitmap_bit_order = LSBFirst;
	numMode = loadImage(num_bits, inputmode_width, inputmode_height);
	numMode->bitmap_bit_order = LSBFirst;
	SYMMode = loadImage(SYM_bits, inputmode_width, inputmode_height);
	SYMMode->bitmap_bit_order = LSBFirst;
#ifdef INCLUDE_I18N
	hiraMode = loadImage(hira_bits, inputmode_width, inputmode_height);
	hiraMode->bitmap_bit_order = LSBFirst;
	kanaMode = loadImage(kana_bits, inputmode_width, inputmode_height);
	kanaMode->bitmap_bit_order = LSBFirst;
#endif
	x = (PAINT_WIDTH - inputmode_width)/2;
	y = (topbar_height - inputmode_height) / 2;
    }
    
    XFillRectangle(display, backgroundDrawable, gcErase, x, y, 
		   inputmode_width, inputmode_height);
    
    switch (mode) {
    case 1:
	icon = ABCMode;
	break;
    case 2:
	icon = abcMode;
	break;
    case 3:
	icon = numMode;
	break;
    case 4:
	icon = SYMMode;
	break;
#ifdef INCLUDE_I18N
    case 5:
	icon = hiraMode;
	break;
    case 6:
	icon = kanaMode;
	break;
#endif
    case 0:
    default:
        break;
    }

    if (icon != NULL) {
	XPutImage(display, backgroundDrawable, gcNormal, icon,
		  0, 0, x, y, inputmode_width, inputmode_height);
    }
    refreshTopBar();
}

typedef struct _kte {
    unsigned int code;
    unsigned int chr;
} keytable_entry;

static keytable_entry *table;

void
InitializeWindowSystem() 
{
    static unsigned char data33[] = {
        0xdb, 0x6d, 0xb6,
        0x6d, 0xb6, 0xdb,
        0xb6, 0xdb, 0x6d
    };

    static unsigned char data50[] = {
        0x55,
        0xaa,
    };

    static unsigned char data66[] = {
        0x24, 0x92, 0x49,
        0x92, 0x49, 0x24,
        0x49, 0x24, 0x92
    };

    int i;

    memset(fonts, 0, sizeof(fonts));

    /* Initialize multi-thread mode in X11 */
    if (XInitThreads() == 0) {
        fprintf(stderr, "WARNING: Unable to initialize X11 threading\n");
    }

    /* set up the offsets for non-full screen mode*/
    setUpOffsets(KNI_FALSE);
    inFullScreenMode = KNI_FALSE;
    requestedFullScreenMode = KNI_FALSE;

    drawTrustedMIDletIcon = KNI_FALSE;

    createEmulatorWindow();

    LCDUIinitMenus();
    LCDUIinitPopupMenu();

    LCDUIinitPhoneSim();

    stipple33 = 
        XCreatePixmapFromBitmapData(display, paintDrawable,
                                    (char*)data33, 24, 3, 0, 1, 1);

    stipple50 = 
        XCreatePixmapFromBitmapData(display, paintDrawable,
                                    (char*)data50, 8, 2, 0, 1, 1);

    stipple66 = 
        XCreatePixmapFromBitmapData(display, paintDrawable,
                                    (char*)data66, 24, 3, 0, 1, 1);

    /* enough to handle the keypad keys 0..9 */
    table = midpCalloc(10, sizeof(keytable_entry));
    if (table == NULL) {
        fprintf(stderr, "No memory for table\n");
        exit(1);
    } 

    for (i = 0; i < 10; ++i) {
        KeySym sym = i + XK_KP_0;  /* contiguous */

        table[i].chr  = KEY_0 + i; /* contiguous */
        table[i].code = XKeysymToKeycode(display, sym);
    }
}

/* 
 * Finalize the use of the window system by returning all allocated
 * graphics data structures to the free list. e.g. close, destroy or free
 * X, Xt and xpm library resources.
 */
void
FinalizeWindowSystem() {
    int i;

    if (!initialized) {
	return;
    }

    LCDUIfinalizeMenus();
    LCDUIfinalizePopupMenu();

    if (table) {
	midpFree(table);
	table = NULL;
    }
    if(visualInfo) {
	XFree(visualInfo);
	visualInfo = NULL;
    }
    if(gc) {
	XFreeGC(display,gc);
	gc = NULL;
    }
    if(gcErase) {
	XFreeGC(display,gcErase);
	gcErase = NULL;
    }
    if(gcNormal) {
	XFreeGC(display,gcNormal);
	gcNormal = NULL;
    }
    if (systemFontInfo){
	XFreeFont(display,systemFontInfo);
	systemFontInfo = NULL;
    }

    for (i = 0; i < NUM_FONTS; ++i) {
        if (fonts[i]) {
            XFreeFont(display, fonts[i]);
	    fonts[i] = 0;
        }	
    }

    if (pixmap != None) {
	XFreePixmap(display, pixmap);
	XpmFreeAttributes(&case_attributes);
	pixmap = None;
    }

    if (display != NULL) {
        XrmDestroyDatabase(XrmGetDatabase(display));
    }

    if (topbar != None) {
	XFree(topbar);
	topbar = None;
    }

    if (LED_on_Image != None) {
	XFree(LED_on_Image);
	LED_on_Image = None;
    }

    if (LED_off_Image != None) {
	XFree(LED_off_Image);
	LED_off_Image = None;
    }

    if (BackLight_off_Image != None) {
	XFree(BackLight_off_Image);
	BackLight_off_Image = None;
    }

    if (BackLight_on_Image != None) {
	XFree(BackLight_on_Image);
	BackLight_on_Image = None;
    }

#ifdef INCLUDE_I18N
     if (lcWin){
	 midpFree(lcWin);
	 lcWin=NULL;
     }
     if (xic){
	 XDestroyIC(xic);
	 xic=NULL;
     }
     if (xim){
	 XCloseIM(xim); 
	 xim=NULL;
     }

#endif

    /*
     * X server-side objects will be disposed of automatically when
     * the client side connection to the X display server is closed
     */ 
    if (display != NULL){
	XCloseDisplay(display);
	display = NULL;
    }
    /* RFC - some Motif resources remain unclaimed that are buried
     * in XtOpenDisplay initialization. It may not be possible to 
     * plug all system allocated resources. e.g. cached
     * conversion tables and interned strings.
     */
}

static int
translateKey(XKeyPressedEvent *ek, KVMEventType *evt)
{
    KeySym sym;
    Modifiers m;
    int n;
    char buf[64];

    if (ek->state & Mod3Mask) { /* Num Lock */
        int i;
        for (i = 0; i < 10; ++i) {
            if (ek->keycode == table[i].code) return table[i].chr;
        }
    }

    XtTranslateKeycode(display, ek->keycode, ek->state, &m, &sym);
    switch (sym) {
    /* other keys that return characters */
    case XK_BackSpace:  evt->type = keyTypedKVMEvent; return 8;
    case XK_Delete:     evt->type = keyTypedKVMEvent; return 127;
    case XK_space:      evt->type = keyTypedKVMEvent; return ' ';

    /* keypad keys other than 0..9 */
    case XK_R3:          
    case XK_KP_Add:      return 0;
    case XK_R4:          
    case XK_KP_Subtract: return 0;

    case XK_R5:          
    case XK_KP_Divide:   return KEY_POUND;
    case XK_R6:          /* num pad * on Sun keyboard */
    case XK_KP_Multiply: return KEY_ASTERISK;

    case XK_KP_Enter:    return KEY_SELECT;

    case XK_Up:          return KEY_UP;
    case XK_Down:        return KEY_DOWN;
    case XK_Left:        return KEY_LEFT;
    case XK_Right:       return KEY_RIGHT;
    case XK_Return:      return KEY_SELECT;

    case XK_F1:          return KEY_SOFT1; /* labelled F1 */
    case XK_F2:          return KEY_SOFT2; /* labelled F2 */

    case XK_F6:          evt->type = systemKVMEvent; return VK_SUSPEND_ALL;
    case XK_F7:          evt->type = systemKVMEvent; return VK_RESUME_ALL;
    case XK_F8:          evt->type = systemKVMEvent; return VK_SUSPEND_CURRENT;
    case XK_F9:          evt->type = systemKVMEvent; return VK_RESUME_PREVIOUS;
    case XK_F10:         evt->type = systemKVMEvent; return VK_KILL_CURRENT;

/*    case SunXK_F36:      return ;  labelled F11 */

    /* labelled F12 */
/*    case SunXK_F37:      evt->type = systemKVMEvent; return VK_SHUTDOWN; */

    case XK_KP_Decimal:  return 0;

    default:           
        n = XLookupString(ek, buf, sizeof(buf), &sym, NULL);
        if ((n == 1) && (buf[0] >= ' ')) {
            evt->type = keyTypedKVMEvent;
            return buf[0];
        }

        return 0;
    }
}

#ifdef INCLUDE_I18N
static jboolean
FilterEvent(XEvent *event, KVMEventType *evt) 
{
    char  *str = NULL;
    int    len = 0;
    jboolean ret = KNI_FALSE;

    if (xic == NULL) {
        return KNI_FALSE;
    }

    switch (event->type) {
    case KeyPress:
        /* let it go further */
        break;
    case FocusIn:
        XSetICFocus(xic);
        return KNI_FALSE;
    case FocusOut:
        XUnsetICFocus(xic);
        return KNI_FALSE;
    default:
        return KNI_FALSE;
    }

    while (KNI_TRUE) {
        XKeyPressedEvent *kev = (XKeyPressedEvent *) event;
        int count;
        KeySym keysym;
        Status status;

        len += 128;

        str = (char *) midpRealloc(str, len);

        if (str == NULL) {
            ret = KNI_FALSE;
            break;
        }

        count = XmbLookupString(xic, kev, str, len, &keysym, &status);
    
        if (status == XBufferOverflow) {
            continue;
        } else {
            switch (status) {
            case XLookupNone:
            case XLookupKeySym:
                break;
            case XLookupBoth:
            case XLookupChars: {
                if (count == 1 && isascii(str[0])) {
                    /* let the ascii character handle the main loop. */
                    break;
                }
                if (count > 0) {
                    unicode *ustr = 
                        (unicode *) midpMalloc(count * sizeof(unicode));

                    if (ustr == NULL) {
                        ret = KNI_FALSE;
                    } else {
 
                        count = nativeToUnicode((const unsigned char *)str, 
                                                count, ustr, count);
                        evt->type = imeKVMEvent;
                        evt->str = ustr;
                        evt->len = count;
                        ret = KNI_TRUE;
                    }
                }
                break;
            }
            }
            /* break the while loop */
            break;
        }
    }

    if (str) {
        midpFree(str);
    }

    return ret;
}
#endif

#if USE_KVM
void
GetAndStoreNextKVMEvent(bool_t forever, ulong64 waitUntil) 
{
    XEvent event;
    KVMEventType evt;
    static int penDown = 0;
    int i;

    /* 
     * A timer may come before waitUntil; we don't want to be
     * blocked for too long, so we have to take them into account.
     * If there are pending timers, we store the shortened wait value
     * into the variable "pending," which is used to compute wait time.
     *
     * We want (pending == 0) to imply (forever == true), so if
     * waitUntil is zero and forever is false, we need to use a
     * nonzero value.
     */
    ulong64 pending = forever ? 0 : ((waitUntil == 0) ? 1 : waitUntil);

    for (;;) {

	/* see if we should toggle the network indicator LED */
	if ((netIndicatorCount != latestNICount) || 
	    isLED_on == KNI_TRUE) {
	    updateNetIndicator();
	}

        /* XtAppPending looks for timers as well as events */
	if (pending && !XtAppPending(appContext)) { 
	    jlong now = midp_getCurrentTime();
	    if (waitUntil < now) {
		return;
	    } else if (pending > now) { 
		int fd = ConnectionNumber(display);
		ulong64 delta = pending - now;
		struct timeval timeout;
		fd_set readfds;

		/* Set the seconds and microseconds */
		timeout.tv_sec = delta / 1000;
		timeout.tv_usec = (delta % 1000) * 1000;

		/* Set the single bit in the bit set */
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		select(fd + 1, &readfds, NULL, NULL, &timeout);
	    }

            if (!XtAppPending(appContext)) {
                /* Hmm...no timer or input event waiting yet. */
                continue;
            }
	}

	XtAppNextEvent(appContext, &event);

#ifdef INCLUDE_I18N
        if (XFilterEvent(&event, None)) continue;

        if (FilterEvent(&event, &evt)) {
            StoreMIDPEvent(&evt);
            return;
        }
#endif

	switch (event.type) {
        case KeyPress:
        case KeyRelease: {
	    XKeyPressedEvent *ek = (XKeyPressedEvent *)&event;

            if (event.type == KeyRelease) {
                XEvent next;
                
                evt.type = keyUpKVMEvent;

                /* with repeating events, you get a release and a press */
                /* following it that have exactly the same time.        */
                if (XCheckTypedEvent(display, KeyPress, &next)) {
                    if (   (next.xkey.time == ek->time) 
                        && (next.xkey.keycode == ek->keycode)) {
                        evt.type = keyRepeatKVMEvent;
                    } else {
                        XPutBackEvent(display, &next);
                    }
                }
            } else {
                evt.type = keyDownKVMEvent;
            }

            evt.chr = translateKey(ek, &evt);

            if (evt.chr == 0) {
                break;
            } else if (   (evt.type == keyDownKVMEvent) 
                       || (evt.type == keyRepeatKVMEvent)) {
                jboolean tmp = LCDUIpopupMenuInternalEvent(evt.chr, &evt);
                if (tmp == KNI_FALSE) {
                    tmp = LCDUImenuInternalEvent(evt.chr, &evt);
                }
                if (tmp && (evt.type == invalidKVMEvent)) {
                    break;
                }
            } else if (   (evt.type == keyTypedKVMEvent)
                       && (event.type == KeyRelease)) {
                break;
            } else if (   (evt.type == systemKVMEvent)
                       && (event.type == KeyRelease)) {
                break;
            }

            StoreMIDPEvent(&evt);
            return;
        }

        case MotionNotify:
        if (lastButton != NULL) {
            if (INSIDE(event, lastButton->bounds)) {
                /* we're still inside--do nothing */
                break;
            } else {
                /* do a release on the old key, then reprocess this event */
                evt.chr  = lastButton->button;
                evt.type = keyUpKVMEvent;

                lastButton = NULL;
                XPutBackEvent(display, &event);
                StoreMIDPEvent(&evt);
                return;
            }
        }

        /* FALL THROUGH */

        case ButtonPress:
        case ButtonRelease:
        if (event.xbutton.window == paintWindow) {
            evt.screenX = event.xbutton.x;
            evt.screenY = event.xbutton.y;

            if (evt.screenX < 0) {
                if (!penDown) break;
                evt.screenX = 0;
                event.type = ButtonRelease;
            } else if (evt.screenX >= PAINT_WIDTH) {
                if (!penDown) break;
                evt.screenX = PAINT_WIDTH - 1;
                event.type = ButtonRelease;
            }

            if (evt.screenY < 0) {
                if (!penDown) break;
                evt.screenY = 0;
                event.type = ButtonRelease;
            } else if (evt.screenY >= paintHeight) {
                if (!penDown) break;
                evt.screenY = paintHeight - 1;
                event.type = ButtonRelease;
            }

            switch (event.type) {
            case ButtonPress:
                penDown = KNI_TRUE;
                evt.type = penDownKVMEvent;
                StoreMIDPEvent(&evt);
                return;

            case ButtonRelease:
                if (!penDown) break;

                penDown = KNI_FALSE;
                evt.type = penUpKVMEvent;
                StoreMIDPEvent(&evt);
                return;

            case MotionNotify:
                if (!penDown) break;

                evt.type = penMoveKVMEvent;
                StoreMIDPEvent(&evt);
                return;
            } /* switch */
        } else {
            int i;

            for (i = 0; i < XtNumber(keys); i++) {
                if (INSIDE(event, keys[i].bounds)) {
#ifdef SOUND_SUPPORTED
                    if(event.type == ButtonPress) {
                        XBell(display, 0);
                        XFlush(display);
                    }
#endif
                    if (keys[i].button == KEY_POWER
                            && event.type == ButtonRelease) {
                        evt.type = systemKVMEvent;
                        evt.chr = VK_SHUTDOWN;
                        StoreMIDPEvent(&evt);
                        return;
                    } else if ((keys[i].button == KEY_SOFT1) ||
                            (keys[i].button == KEY_SOFT2)) {

                        lastButton = keys + i;

                        if ((event.type != ButtonRelease)
                            && LCDUIpopupMenuInternalEvent(keys[i].button,
                                &evt)) {
                            StoreMIDPEvent(&evt);
                            return;
                        } else if ((event.type != ButtonRelease)
                            && LCDUImenuInternalEvent(keys[i].button,
                                &evt)) {
                            StoreMIDPEvent(&evt);
                            return;
                        } else {
                            break;
                        }
                    } else if (keys[i].button == KEY_END) {
                        if (event.type == ButtonPress) {
                            /* The RI will manually translate a single press
                            * of the 'end' key into a MIDlet 'kill' system event.
                            * Ports may utilize a special "long press" usage of
                            * the 'end' key to accomplish this and remove this
                            * 'else' block
                            */
                            LCDUIdismissMenu();
                            LCDUIdismissPopupMenu();
                            evt.type = systemKVMEvent;
                            evt.chr = VK_KILL_CURRENT;
                            StoreMIDPEvent(&evt);
                            return;
                        } else {
                            break;
                        }
                    } else {
                        jboolean tmp;
                        evt.chr = keys[i].button;

                        switch (event.type) {
                        case MotionNotify:
                        case ButtonPress:
                            lastButton = keys + i;
                            evt.type = keyDownKVMEvent;
                            tmp = LCDUIpopupMenuInternalEvent(keys[i].button,
                                &evt);
                            if (tmp == KNI_FALSE) {
                                tmp = LCDUImenuInternalEvent(keys[i].button,
                                    &evt);
                            }
                            if (tmp && (evt.type == invalidKVMEvent)) {
                                break;
                            }
                            StoreMIDPEvent(&evt);
                            return;

                        case ButtonRelease:
                            lastButton = NULL;
                            evt.type = keyUpKVMEvent;
                            StoreMIDPEvent(&evt);
                            return;

                        default:
                            /* this should never happen! */
                            fprintf(stderr, "Logic error at %s:%d\n",
                                __FILE__, __LINE__);
                        } /* switch */
                        return;
                    } /* else */
                }
            }
        } /* if */
        break;

        case ClientMessage:
        /* check for jsr135 eom msg */
        if (event.xclient.message_type == mmEOMAtom) {
            evt.type = mmEOMEvent;
            evt.screenX = (short)(event.xclient.data.l[0] & 0x7fff);/*pID*/
            evt.chr = (int)(event.xclient.data.l[1]);
            StoreMIDPEvent(&evt);
            return;
        }

        /*  Look for a message saying to quit */
        if (event.xclient.data.l[0] == deleteAtom) {
            evt.type = systemKVMEvent;
            evt.chr = VK_SHUTDOWN;
            StoreMIDPEvent(&evt);
            return;
        } else if (event.xclient.message_type == timerAtom) {
            evt.type = timerKVMEvent;
            evt.chr  = event.xclient.data.b[0];
            StoreMIDPEvent(&evt);
            return;
        }
	    break;
	    
        case MappingNotify:
        /*  Every application has to do this! */
        XRefreshKeyboardMapping(&event.xmapping);
        break;
	    } /* switch */
    } /* for loop */
}
#endif /* USE_KVM */

/*
 * Check for recent network activity, and update the 
 * network indicator LED accordingly.
 */
void updateNetIndicator() {
    if (netIndicatorCount != latestNICount){
	if (!isLED_on){
	    drawNetIndicator(KNI_TRUE);
	    isLED_on = KNI_TRUE;
	}
	latestNITime = midp_getCurrentTime();
	latestNICount = netIndicatorCount;
    } else {
	if (isLED_on){
	    jlong now = midp_getCurrentTime();
	    if (now - latestNITime > NET_INDICATOR_DELAY){ 
		drawNetIndicator(KNI_FALSE);
		isLED_on = KNI_FALSE;
	    }
	}
    }
}

/*
 * Draw the LED indicating network activity.
 * If 'active' is KNI_TRUE, a lit green LED is drawn.
 * If 'active' is KNI_FALSE, a dim gray LED is drawn.
 */
void drawNetIndicator(jboolean active) {
    XImage * img;
    
    if (active) {
	img = LED_on_Image;
    } else {
	img = LED_off_Image;
    }
    
    XPutImage(display, LEDDrawable, gcNormal, img, 0, 0,
	      0, 0, LED_width, LED_height);
    refreshLEDArea();
}



