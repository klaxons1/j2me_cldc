 /*
 * @(#)nativeGUI.c	1.155 02/10/17 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * Include files
 *=======================================================================*/
#define STRICT
#include <kni.h>
#include <defaultLCDUI.h>
#include <nativeGUI.h>
#include <imageDecode.h>
#include <midpMalloc.h>

#include <stdio.h>
#include <windows.h>
#include <ctype.h>
#include <math.h>
#ifdef INCLUDE_I18N
#include <imm.h>
#endif

#include <configuration.h>
#include "staticGraphics.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_SOFTBUTTON_CHARS  12
#define EMULATOR_WIDTH  (241 + 8)
#define EMULATOR_HEIGHT (635 + 24)
#define TOP_BAR_HEIGHT      11 
#define BOTTOM_BAR_HEIGHT   19
#define MENUBAR_BORDER_HEIGHT 2
#define ARROWS_WIDTH        7
#define PAINT_WIDTH  DISPLAY_WIDTH
#define UNTRANSLATED_SCREEN_BITMAP (void*)0xffffffff

#define NUMBEROF(x) (sizeof(x)/sizeof(x[0]))
#define MIN(a,b) ((a)<(b)?(a):(b))

void DrawBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int rop);
static jboolean FilterEvent(HWND hwnd, UINT iMsg, WPARAM wParam,
                            LPARAM lParam, KVMEventType *kvmEvent,
                            int *gotEvent, int *retval);
static void DrawMenuBarBorder(HDC hdc);

static jboolean bkliteImageCreated = KNI_FALSE;
void CreateBacklight(HDC hdc);


#define INSIDE(_x, _y, _r)                              \
    ((_x >= (_r).x) && (_x < ((_r).x + (_r).width)) &&  \
     (_y >= (_r).y) && (_y < ((_r).y + (_r).height)))

typedef struct _Rectangle {
    int x;
    int y;
    int width;
    int height;
} XRectangle;

typedef struct  { 
    KeyType button;
    XRectangle bounds;
    char *name;
} Key;

static LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM) ;

/* thread safety */
static int tlsId;

jboolean doubleBuffer = KNI_TRUE;

#ifdef SKINS_MENU_SUPPORTED
/* support for changing the device "skin"...this is for user testing only */
#define NUM_SKINS_SUPPORTED 16
static int numSkins = 0;
static char *skins[16];
#endif

#define REFRESHBOTTOMBAR()                                 \
do {                                                       \
  RECT r;                                                  \
  r.left   = x_offset;                                     \
  r.top    = y_offset;                                     \
  r.right  = x_offset + PAINT_WIDTH;                       \
  r.bottom = y_offset + paintHeight + bottomBarHeight;    \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);               \
} while (0)


#define REFRESHTOPBAR()                                    \
{                                                          \
  RECT r;                                                  \
  r.left   = x_offset;                                     \
  r.top    = Y_SCREEN_OFFSET;                              \
  r.right  = x_offset + PAINT_WIDTH;                       \
  r.bottom = y_offset;                                     \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);               \
}  

/* refresh the Top backlight bar */
#define REFRESH_BACKLIGHT_TOPBAR()                                       \
{                                                      \
  RECT r;                                              \
  r.left   = BkliteTop_xposition;                      \
  r.top    = BkliteTop_yposition;                      \
  r.right  = BkliteTop_xposition + BkliteTop_width;    \
  r.bottom = BkliteTop_yposition + BkliteTop_height;   \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);           \
}

/* refresh the bottom backlight bar */
#define REFRESH_BACKLIGHT_BOTTOMBAR()                                       \
{                                                          \
  RECT r;                                                  \
  r.left   = BkliteBottom_xposition;                       \
  r.top    = BkliteBottom_yposition;                       \
  r.right  = BkliteBottom_xposition + BkliteBottom_width;  \
  r.bottom = BkliteBottom_yposition + BkliteBottom_height; \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);               \
}

/* refresh the left backlight bar */
#define REFRESH_BACKLIGHT_LEFTBAR()                                       \
{                                                          \
  RECT r;                                                  \
  r.left   = BkliteLeft_xposition;                         \
  r.top    = BkliteLeft_yposition;                         \
  r.right  = BkliteLeft_xposition + BkliteLeft_width;      \
  r.bottom = BkliteLeft_yposition + BkliteLeft_height;     \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);               \
}

/* refresh the right backlight bar */
#define REFRESH_BACKLIGHT_RIGHTBAR()                                       \
{                                                          \
  RECT r;                                                  \
  r.left   = BkliteRight_xposition;                        \
  r.top    = BkliteRight_yposition;                        \
  r.right  = BkliteRight_xposition + BkliteRight_width;    \
  r.bottom = BkliteRight_yposition + BkliteRight_height;   \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);               \
}

const static Key Keys[] = {
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

static const Key *lastButton = NULL;

int numColors       = 4;
int backgroundColor = RGB(182, 182, 170);
static int foregroundColor = RGB(0,0,0);

static HBITMAP       hScreenBitmap;
static HBITMAP       topbar_Image;

/* The bits of the BackLight images */
static HBITMAP       bklite_Top_Image;
static HBITMAP       bklite_Bottom_Image;
static HBITMAP       bklite_Left_Image;
static HBITMAP       bklite_Right_Image;

/* The bits of the Network Indicator images */
static HBITMAP       LED_on_Image;
static HBITMAP       LED_off_Image;

/* The bits of the trusted MIDlet icon image */
static HBITMAP       TrustIcon_Image;

HWND          hMainWindow;
KVMEventType  kvmEvent;
int           gotEvent;
static int           x_offset = X_SCREEN_OFFSET;
static int           y_offset;
static int initialized = 0;

static jboolean inFullScreenMode;
static jboolean requestedFullScreenMode;

static jboolean drawTrustedMIDletIcon;

int    blackPixel;
int    whitePixel;
int    lightGrayPixel;
int    darkGrayPixel;

HBRUSH        darkGrayBrush;
HPEN          whitePen;
HPEN          darkGrayPen;
HBRUSH        whiteBrush;

HBRUSH        BACKGROUND_BRUSH, FOREGROUND_BRUSH;
HPEN          BACKGROUND_PEN, FOREGROUND_PEN;
static TEXTMETRIC    fixed_tm, tm;

static HFONT         fonts[3][3][8];

static int timerPending = 0;

/* these are gamma corrected for a monitor with gamma = 2.5 */
static int grayTable4[4] = {
  RGB(0x00,0x00,0x00), RGB(0x6F,0x7B,0x6F),
  RGB(0x94,0xA3,0x94), RGB(0xAE,0xC0,0xAE)
};

/* these are gamma corrected for a monitor with gamma = 2.5 */
static int grayTable16[16] = {
  RGB(0X00,0X00,0X00), RGB(0X3A,0X40,0X3A),
  RGB(0X4D,0X54,0X4D), RGB(0X5B,0X64,0X5B),
  RGB(0X66,0X71,0X66), RGB(0X70,0X7B,0X70),
  RGB(0X78,0X85,0X78), RGB(0X80,0X8D,0X80),
  RGB(0X87,0X95,0X87), RGB(0X8D,0X9C,0X8D),
  RGB(0X93,0XA3,0X93), RGB(0X99,0XA9,0X99),
  RGB(0X9F,0XAF,0X9F), RGB(0XA4,0XB5,0XA4),
  RGB(0XA9,0XBA,0XA9), RGB(0XAE,0XC0,0XAE)
};

/* these are not gamma corrected */
static int rgtable[8] = {
  0, 36, 73, 109, 146, 182, 219, 255
};

/* these are not gamma corrected */
static int btable[4] = {
  0, 85, 170, 255
};

#define CHECK_RETURN(expr) (expr) ? (void)0 : (void)fprintf(stderr, "%s returned error (%s:%d)\n", #expr, __FILE__, __LINE__)

static HBITMAP getBitmapDCtmp = NULL;


typedef struct {
    int     num;
    unicode label[MAX_SOFTBUTTON_CHARS];
} SoftButtonLabel;

static SoftButtonLabel llabel;
static SoftButtonLabel rlabel;

typedef struct _mbs {
    HBITMAP bitmap;
    HBITMAP mask;
    int width;
    int height;
    int mutable;
    unsigned char *image;
    unsigned char *imageMask;
    char prop;
} myBitmapStruct;

/* Network Indicator position parameters */
#define LED_xposition 17
#define LED_yposition 82
#define LED_width     20
#define LED_height    20
#define NET_INDICATOR_DELAY 2500  /* 2.5 seconds */

/* Network Indicator status info */
int latestNICount = 0;
jboolean isLED_on = KNI_FALSE;
jlong latestNITime = 0;

/* Trusted MIDlet icon info */

#define TrustIcon_width     12
#define TrustIcon_height    12
#define TrustIcon_xposition (X_SCREEN_OFFSET + PAINT_WIDTH - 17 - 10 - TrustIcon_width)
#define TrustIcon_yposition (Y_SCREEN_OFFSET)

/* NOTE :
 * trusted midlet icon x position is =  
 * right corner (screenoffset+paintwidth)
 * - widht of battery (17 pixels)
 * - space between battery and right edge of icon (10 pixels)
 * - the width of the trusted icon 
 */


/* BackLight top bar position parameters */
#define BkliteTop_xposition 0
#define BkliteTop_yposition 113
#define BkliteTop_width     241
#define BkliteTop_height    18

/* BackLight bottom bar position parameters */
#define BkliteBottom_xposition 0
#define BkliteBottom_yposition 339
#define BkliteBottom_width     241
#define BkliteBottom_height    6

/* BackLight left bar position parameters */
#define BkliteLeft_xposition 0
#define BkliteLeft_yposition 131
#define BkliteLeft_width     30
#define BkliteLeft_height    208

/* BackLight right bar position parameters */
#define BkliteRight_xposition 210
#define BkliteRight_yposition 131
#define BkliteRight_width     31
#define BkliteRight_height    208

jboolean isBklite_on = KNI_FALSE;

static int topBarHeight;
static int bottomBarHeight;
static int paintHeight;

void setUpOffsets(int fullscreen) {
  switch (fullscreen) { 
  case 1:
    topBarHeight    = TOP_BAR_HEIGHT;
    bottomBarHeight = 0;
    break;
  case 0:
    topBarHeight    = TOP_BAR_HEIGHT;
    bottomBarHeight = BOTTOM_BAR_HEIGHT;
    break;
  }
  
  paintHeight = (DISPLAY_HEIGHT 
                  - (topBarHeight + bottomBarHeight));
  
  y_offset = Y_SCREEN_OFFSET + topBarHeight;
}

/*
 * Draw BackLight.
 * If 'active' is KNI_TRUE, the BackLight is drawn.
 * If 'active' is KNI_FALSE, the BackLight is erased.
 */
void drawBackLight(jboolean active) {
    
    HDC hdc = GetDC(hMainWindow);

    CreateBacklight(hdc);
    
    if (active == KNI_TRUE) {
        isBklite_on = KNI_TRUE;
        DrawBitmap(hdc, bklite_Top_Image, BkliteTop_xposition, BkliteTop_yposition, SRCCOPY);
        DrawBitmap(hdc, bklite_Bottom_Image, BkliteBottom_xposition, BkliteBottom_yposition, SRCCOPY);
        DrawBitmap(hdc, bklite_Left_Image, BkliteLeft_xposition, BkliteLeft_yposition, SRCCOPY);
        DrawBitmap(hdc, bklite_Right_Image, BkliteRight_xposition, BkliteRight_yposition, SRCCOPY);
    }else {
        if (isBklite_on) {
            isBklite_on = KNI_FALSE;
            REFRESH_BACKLIGHT_TOPBAR();
            REFRESH_BACKLIGHT_BOTTOMBAR();
            REFRESH_BACKLIGHT_LEFTBAR();
            REFRESH_BACKLIGHT_RIGHTBAR();
        }
    }
    ReleaseDC(hMainWindow, hdc);
}

/*
 * Draw the LED indicating network activity.
 * If 'active' is KNI_TRUE, a lit green LED is drawn.
 * If 'active' is KNI_FALSE, a dim gray LED is drawn.
 */
void drawNetIndicator(jboolean activity) {
    
    HDC hdc = GetDC(hMainWindow);
    
    if (activity == KNI_TRUE) {
        DrawBitmap(hdc, LED_on_Image, LED_xposition, LED_yposition, SRCCOPY);
    } else {
        DrawBitmap(hdc, LED_off_Image, LED_xposition, LED_yposition, SRCCOPY);
    }
    ReleaseDC(hMainWindow, hdc);
}

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
            if ((now - latestNITime) > NET_INDICATOR_DELAY) { 
                drawNetIndicator(KNI_FALSE);
                isLED_on = KNI_FALSE;
            }
        }
    }
}

void drawEmulatorScreen(jboolean fullscreen) {


    if (KNI_FALSE == fullscreen) {

      HDC hdc;

      hdc = getBitmapDC(UNTRANSLATED_SCREEN_BITMAP);
      
      CHECK_RETURN(SelectObject(hdc, BACKGROUND_BRUSH));
      CHECK_RETURN(SelectObject(hdc, BACKGROUND_PEN));
      
      Rectangle(hdc,
                x_offset, Y_SCREEN_OFFSET,
                x_offset + PAINT_WIDTH, y_offset);
      
      DrawBitmap(hdc, topbar_Image, x_offset, Y_SCREEN_OFFSET, SRCCOPY);
      
      if (drawTrustedMIDletIcon == KNI_TRUE) {
        
        DrawBitmap(hdc,
                   TrustIcon_Image,
                   TrustIcon_xposition,
                   TrustIcon_yposition,
                   SRCCOPY);
      }
      
      REFRESHTOPBAR();
      
      /* draw top border of menu */
      DrawMenuBarBorder(hdc);

#define VERT_X   (x_offset + (PAINT_WIDTH/2) - 4) 
#define UP_Y     (y_offset + paintHeight + MENUBAR_BORDER_HEIGHT) 
#define DOWN_Y   (y_offset + paintHeight + 11)

      CHECK_RETURN(SelectObject(hdc, BACKGROUND_PEN));
      CHECK_RETURN(SelectObject(hdc, BACKGROUND_BRUSH));
      
      /* clear the arrow area */
      Rectangle(hdc, VERT_X, UP_Y, VERT_X + 8, UP_Y + BOTTOM_BAR_HEIGHT - 1);
      
      REFRESHBOTTOMBAR();

      releaseBitmapDC(hdc);
      
      LCDUIsetSoftButton(1, rlabel.label, rlabel.num);

    }
    
    REFRESHBOTTOMBAR();
}

/* Sets the top bar to display a trusted MIDlet icon,
 * then calls drawEmulatorScreen to cause an invalidate of 
 * the top bar
 */
void LCDUIdrawTrustedIcon(jboolean trusted) {
  
  drawTrustedMIDletIcon  = trusted;
  drawEmulatorScreen(inFullScreenMode);

}

/* draws the top border of the menu: a one pixel thick dark gray
 * line on top of a one pixel thick white line */

void DrawMenuBarBorder(HDC myhdc){
    CHECK_RETURN(SelectObject(myhdc, darkGrayBrush));
    CHECK_RETURN(SelectObject(myhdc, darkGrayPen));
    MoveToEx(myhdc, x_offset, y_offset + paintHeight, NULL);
    LineTo(myhdc, x_offset + PAINT_WIDTH, y_offset + paintHeight);

    CHECK_RETURN(SelectObject(myhdc, whiteBrush));
    CHECK_RETURN(SelectObject(myhdc, whitePen));
    MoveToEx(myhdc, x_offset, y_offset + paintHeight + 1, NULL);
    LineTo(myhdc, x_offset + PAINT_WIDTH, y_offset + paintHeight + 1);
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


/*
 * Returns a DC for the graphics code to draw into.
 *
 * In the current implementation, this returns a DC of an off-screen
 * bitmap. All drawings will be first drawn into the bitmap and later
 * BLT'ed to the simulator window.
 */
HDC getBitmapDC(void *imageData)
{
    HDC hdc, hdcMem;
    HANDLE  *mutex = (HANDLE*) TlsGetValue(tlsId);

    if (mutex == NULL) {
        mutex = midpMalloc(sizeof(HANDLE));
        if (mutex == NULL) {
            return NULL;
        }
        TlsSetValue(tlsId, mutex);
        *mutex = CreateMutex(0, KNI_FALSE, "hScreenBitmapMutex");       
    }

    hdc = GetDC(hMainWindow);
    hdcMem = CreateCompatibleDC(hdc);
    ReleaseDC(hMainWindow, hdc);

    WaitForSingleObject(*mutex, INFINITE);

    if (imageData == NULL) {
        CHECK_RETURN(getBitmapDCtmp = SelectObject(hdcMem, hScreenBitmap));
        SetWindowOrgEx(hdcMem, -x_offset, -y_offset, NULL);
    } else if (imageData == UNTRANSLATED_SCREEN_BITMAP) {
        CHECK_RETURN(getBitmapDCtmp = SelectObject(hdcMem, hScreenBitmap));
    } else {
        myBitmapStruct *bmp = (myBitmapStruct *)imageData;
        if (bmp->mutable) {
            getBitmapDCtmp = SelectObject(hdcMem, bmp->bitmap);
        }
    }

    return hdcMem;
}

void releaseBitmapDC(HDC hdcMem)
{
    HANDLE  *mutex = (HANDLE*) TlsGetValue(tlsId);
    SelectObject(hdcMem, getBitmapDCtmp);
    getBitmapDCtmp = NULL;
    DeleteDC(hdcMem);
    ReleaseMutex(*mutex);
}

void setupClip(HDC hdc, void *imageData, short *clip)
{
    HRGN rgn;
    RECT r;
    myBitmapStruct *b = (myBitmapStruct*) imageData;

    r.left   = 0;
    r.right  = b ? b->width : PAINT_WIDTH;
    r.top    = 0;
    r.bottom = b ? b->height : paintHeight;

    if (clip) {
        if (r.left   < clip[0]) r.left   = clip[0];
        if (r.top    < clip[1]) r.top    = clip[1];
        if (r.right  > clip[0] + clip[2]) r.right  = clip[0] + clip[2];
        if (r.bottom > clip[1] + clip[3]) r.bottom = clip[1] + clip[3];
    }

    rgn = CreateRectRgnIndirect(&r);

    SelectClipRgn(hdc, rgn);
    if (imageData == NULL) OffsetClipRgn(hdc, x_offset, y_offset);
    DeleteObject(rgn);          /* it's been copied */
}

void
refreshPaintWindow(int x1, int y1, int x2, int y2) {
    RECT r;
    if (x1 < x2) {
        r.left = x1 + x_offset; r.right = x2 + x_offset;
    } else {
        r.left = x2 + x_offset; r.right = x1 + x_offset;
    }
    if (y1 < y2) {
        r.top = y1 + y_offset; r.bottom = y2 + y_offset;
    } else {
        r.top = y2 + y_offset; r.bottom = y1 + y_offset;
    }                                       
    ++r.bottom; ++r.right;
    InvalidateRect(hMainWindow, &r, KNI_TRUE);  
}

void refreshWholePaintWindow(){
        refreshPaintWindow(0, 0, PAINT_WIDTH, paintHeight);
}

/* static int
/* makePixel(int rgb, int gray, int isGray) { */
int 
LCDUIgetPixel(int rgb, int gray, int isGray) {
    if (numColors == 2) {
        return (gray > 127) ? grayTable4[3] : grayTable4[0];
    } else if (numColors == 4) {
        return grayTable4[gray >> 6];
    } else if (numColors == 16) {
        return grayTable16[gray >> 4];
    } else if (numColors == 256) {
        int red = (rgb >> 16) & 0xff;
        int grn = (rgb >>  8) & 0xff;
        int blu = (rgb >>  0) & 0xff;

        red = rgtable[red >> 5];
        grn = rgtable[grn >> 5];
        blu = btable[blu >> 6];

        return RGB(red, grn, blu);
    } else {
        fprintf(stderr, "unsupported color format\n");
        return 0;
    }
}


static void
CreateScreenBitmap(HDC hdc) 
{
    int i;

    HDC hdcMem = CreateCompatibleDC(hdc);
    BITMAPINFOHEADER* b = phone_dib;
    HBITMAP img, tmp;
    int cmapLen;
    HANDLE  *mutex;

    BITMAPINFOHEADER* grayLEDInfo = grayLED_dib;
    BITMAPINFOHEADER* greenLEDInfo = greenLED_dib;
    BITMAPINFOHEADER* topbarInfo = topbar_dib;

    BITMAPINFOHEADER* trustIconInfo = trustedIcon_dib;

    backgroundColor = lightGrayPixel;
    foregroundColor = blackPixel;

    tlsId = TlsAlloc();
    mutex = midpMalloc(sizeof(HANDLE));
    if (mutex == NULL) {
        return;
    }

    TlsSetValue(tlsId, mutex);
    *mutex = CreateMutex(0, KNI_FALSE, "hScreenBitmapMutex");   

    WaitForSingleObject(*mutex, INFINITE);

    hScreenBitmap = CreateCompatibleBitmap(hdc, 
                                           EMULATOR_WIDTH, EMULATOR_HEIGHT);
    CHECK_RETURN(tmp = SelectObject(hdcMem, hScreenBitmap));

    cmapLen = (b->biBitCount > 8) ? 0 : 
      (b->biClrUsed ? b->biClrUsed : (1 << b->biBitCount));

    img = CreateDIBitmap(hdc, b, CBM_INIT, 
                         ((char*)b) + b->biSize + 4*cmapLen, 
                         (BITMAPINFO*)b, DIB_RGB_COLORS);

    DrawBitmap(hdcMem, img, 0, 0, SRCCOPY);

    CHECK_RETURN(SelectObject(hdcMem, BACKGROUND_PEN));
    CHECK_RETURN(SelectObject(hdcMem, BACKGROUND_BRUSH));

    Rectangle(hdcMem, 
              x_offset - 1, 
              y_offset - topBarHeight - 1, 
              x_offset + DISPLAY_WIDTH  + 1, 
              y_offset - topBarHeight + DISPLAY_HEIGHT + 1);

    DeleteObject(img);

    cmapLen = (topbarInfo->biBitCount > 8) ? 0 :
        (topbarInfo->biClrUsed ? topbarInfo->biClrUsed :
         (1 << topbarInfo->biBitCount));

    topbar_Image = CreateDIBitmap(hdc, topbarInfo, CBM_INIT,
                                  ((char*)topbarInfo) + topbarInfo->biSize +
                                  4*cmapLen, (BITMAPINFO*)topbarInfo,
                                  DIB_RGB_COLORS);
    DrawBitmap(hdcMem, topbar_Image, X_SCREEN_OFFSET, Y_SCREEN_OFFSET, 
               SRCCOPY);

    /* Create dim (default) network indicator and draw it */
    cmapLen = (grayLEDInfo->biBitCount > 8) ? 0 :
        (grayLEDInfo->biClrUsed ? grayLEDInfo->biClrUsed : 
         (1 << grayLEDInfo->biBitCount));

    LED_off_Image = CreateDIBitmap(hdc, grayLEDInfo, CBM_INIT,
                                   ((char*)grayLEDInfo) + grayLEDInfo->biSize +
                                   4*cmapLen, (BITMAPINFO*)grayLEDInfo, 
                                   DIB_RGB_COLORS);
    DrawBitmap(hdcMem, LED_off_Image, LED_xposition, LED_yposition, SRCCOPY);

    /* Create bright network indicator */
    cmapLen = (greenLEDInfo->biBitCount > 8) ? 0 :
        (greenLEDInfo->biClrUsed ? greenLEDInfo->biClrUsed : 
         (1 << greenLEDInfo->biBitCount));
    LED_on_Image = CreateDIBitmap(hdc, greenLEDInfo, CBM_INIT,
                                  ((char*)greenLEDInfo) + greenLEDInfo->biSize 
                                  + 4 * cmapLen, (BITMAPINFO*)greenLEDInfo, 
                                  DIB_RGB_COLORS);

    /* Create trusted icon */
    cmapLen = (trustIconInfo->biBitCount > 8) ? 0 :
      (trustIconInfo->biClrUsed ? 
       trustIconInfo->biClrUsed :
       (1 << trustIconInfo->biBitCount));
    
    TrustIcon_Image = CreateDIBitmap(hdc, trustIconInfo, CBM_INIT,
                                     ((char*)trustIconInfo) 
                                     + trustIconInfo->biSize
                                     + 4 * cmapLen, 
                                     (BITMAPINFO*)trustIconInfo,
                                     DIB_RGB_COLORS);
    
    /* SetTextColor(hdcMem, RGB(255,255,255)); */
    /* SetTextAlign(hdcMem, TA_TOP | TA_LEFT); */
    /* SetBkMode(hdcMem, TRANSPARENT); */
    /* TextOut(hdcMem, 8, 8, "kvm", 3); */
    /* SetBkMode(hdcMem, OPAQUE); */

    SelectObject(hdcMem, tmp);
    DeleteDC(hdcMem);
    ReleaseMutex(*mutex);
}


void
CreateBacklight(HDC hdc) {

    if (KNI_FALSE == bkliteImageCreated) {

        int cmapLen;
        BITMAPINFOHEADER* bkliteTopInfo = bkliteTop_dib;
        BITMAPINFOHEADER* bkliteBottomInfo = bkliteBottom_dib;
        BITMAPINFOHEADER* bkliteLeftInfo = bkliteLeft_dib;
        BITMAPINFOHEADER* bkliteRightInfo = bkliteRight_dib;

        /* Create top backlight bar */
        cmapLen = (bkliteTopInfo->biBitCount > 8) ? 0 :
        (bkliteTopInfo->biClrUsed ? bkliteTopInfo->biClrUsed :
         (1 << bkliteTopInfo->biBitCount));
        bklite_Top_Image = CreateDIBitmap(hdc, bkliteTopInfo, CBM_INIT,
                      ((char*)bkliteTopInfo) + bkliteTopInfo->biSize
                      + 4 * cmapLen, (BITMAPINFO*)bkliteTopInfo,
                      DIB_RGB_COLORS);

        /* Create bottom backlight bar */
        cmapLen = (bkliteBottomInfo->biBitCount > 8) ? 0 :
        (bkliteBottomInfo->biClrUsed ? bkliteBottomInfo->biClrUsed :
         (1 << bkliteBottomInfo->biBitCount));
        bklite_Bottom_Image = CreateDIBitmap(hdc, bkliteBottomInfo, CBM_INIT,
                      ((char*)bkliteBottomInfo) + bkliteBottomInfo->biSize
                      + 4 * cmapLen, (BITMAPINFO*)bkliteBottomInfo,
                      DIB_RGB_COLORS);

        /* Create left backlight bar */
        cmapLen = (bkliteLeftInfo->biBitCount > 8) ? 0 :
        (bkliteLeftInfo->biClrUsed ? bkliteLeftInfo->biClrUsed :
         (1 << bkliteLeftInfo->biBitCount));
        bklite_Left_Image = CreateDIBitmap(hdc, bkliteLeftInfo, CBM_INIT,
                      ((char*)bkliteLeftInfo) + bkliteLeftInfo->biSize
                      + 4 * cmapLen, (BITMAPINFO*)bkliteLeftInfo,
                      DIB_RGB_COLORS);

        /* Create right backlight bar */
        cmapLen = (bkliteRightInfo->biBitCount > 8) ? 0 :
        (bkliteRightInfo->biClrUsed ? bkliteRightInfo->biClrUsed :
         (1 << bkliteRightInfo->biBitCount));
        bklite_Right_Image = CreateDIBitmap(hdc, bkliteRightInfo, CBM_INIT,
                      ((char*)bkliteRightInfo) + bkliteRightInfo->biSize
                      + 4 * cmapLen, (BITMAPINFO*)bkliteRightInfo,
                      DIB_RGB_COLORS);

        bkliteImageCreated = KNI_TRUE;

    }

}

#ifdef SKINS_MENU_SUPPORTED
static HMENU
buildSkinsMenu()
{
    /* first step: find directory which contains skins */
    char cdir[256], cur[256];
    char *dir = getInternalProp("system.display.kh_skins");
    HANDLE fileHandle;
    WIN32_FIND_DATA findData;

    if (dir == NULL) return NULL;
    if (!GetCurrentDirectory(sizeof(cdir), cdir)) return NULL;
    if (!SetCurrentDirectory(dir)) return NULL;

    GetCurrentDirectory(sizeof(cur), cur);
    fprintf(stderr, "Found alternate skins in %s\n", cur);
    fileHandle = FindFirstFile("*.bmp", &findData);
    if (fileHandle != INVALID_HANDLE_VALUE) {
        HMENU hMenu = CreateMenu();
        HMENU hMenuPopup = CreateMenu();
        char *addr;

        do {
            char name[256];
            skins[numSkins] = midpMalloc(1024);

            if (skins[numSkins] != NULL) {
                GetFullPathName(findData.cFileName, 
                    1024, skins[numSkins], &addr);

                sprintf(name, "%s", addr);

                /* remove the .bmp portion */
                name[strlen(name) - 4] = '\0';

                fprintf(stderr, "    %s\n", name);

                AppendMenu(hMenuPopup, MF_STRING, numSkins++, name);

                if (numSkins >= NUM_SKINS_SUPPORTED) break;
            }

            } while (FindNextFile(fileHandle, &findData));

        AppendMenu(hMenuPopup, MF_STRING, numSkins, "Original");

        AppendMenu(hMenu, MF_POPUP, (UINT)hMenuPopup, "Color");
        return hMenu;
    }
    FindClose(fileHandle);

    SetCurrentDirectory(cdir);
    return NULL;
}
#endif

static int
strcasecmp(const char *a, const char *b)
{
    while (*a && *b) {
        int d = tolower(*a) - tolower(*b);
        if (d) return d;
        ++a;
        ++b;
    }

    return tolower(*a) - tolower(*b);
}


static void 
CreateEmulatorWindow()
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    static char szAppName[] = "Kauai";
    WNDCLASSEX  wndclass ;
    HWND hwnd;
    HDC hdc;
    HMENU hMenu = NULL;
    char *s;
    char *DB_ENV    = getInternalProp("system.display.double_buffered");
    char *DEPTH_ENV = getInternalProp("system.display.screen_depth");
    static WORD graybits[] = {0xaaaa, 0x5555, 0xaaaa, 0x5555, 
                              0xaaaa, 0x5555, 0xaaaa, 0x5555};
    
    unsigned int width = EMULATOR_WIDTH, height = EMULATOR_HEIGHT;
    
    if (initialized) {
        return;
    } else {
        initialized = 1;
    }
    
    if (DB_ENV != NULL) {
        doubleBuffer = !strcasecmp(DB_ENV, "true");
    }
    
    if (DEPTH_ENV == NULL) numColors = 4;
    else if (!strcmp(DEPTH_ENV, "1")) numColors = 2;
    else if (!strcmp(DEPTH_ENV, "2")) numColors = 4;
    else if (!strcmp(DEPTH_ENV, "4")) numColors = 16;
    else if (!strcmp(DEPTH_ENV, "8")) {
        numColors = 256;
    }

    wndclass.cbSize        = sizeof (wndclass) ;
    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.lpfnWndProc   = WndProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = hInstance ;
    wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = (HBRUSH) BACKGROUND_BRUSH;
    wndclass.lpszMenuName  = NULL ;
    wndclass.lpszClassName = szAppName ;
    wndclass.hIconSm       = LoadIcon (NULL, IDI_APPLICATION) ;

    RegisterClassEx (&wndclass) ;

#ifdef SKINS_MENU_SUPPORTED
    hMenu = buildSkinsMenu();

    if (hMenu != NULL) height += 24;
#endif

    hwnd = CreateWindow (szAppName,               /* window class name       */
                         "MIDP",                  /* window caption          */
                         WS_OVERLAPPEDWINDOW &    /* window style; disable   */
                         (~WS_MAXIMIZEBOX),       /*  the 'maximize' button  */
                         CW_USEDEFAULT,           /* initial x position      */
                         CW_USEDEFAULT,           /* initial y position      */
                         width,                   /* initial x size          */
                         height,                  /* initial y size          */
                         NULL,                    /* parent window handle    */
                         hMenu,                   /* window menu handle      */
                         hInstance,               /* program instance handle */
                         NULL);                   /* creation parameters     */

    hMainWindow = hwnd;

    /* colors chosen to match those used in topbar.h */
    whitePixel = 0xffffff;  
    blackPixel = 0x000000;
    lightGrayPixel = RGB(182, 182, 170);
    darkGrayPixel = RGB(109, 109, 85);

    foregroundColor = blackPixel;
    backgroundColor = lightGrayPixel;

    /* brushes for borders and menu hilights */
    darkGrayBrush = CreateSolidBrush(darkGrayPixel);
    darkGrayPen = CreatePen(PS_SOLID, 1, darkGrayPixel);
    whiteBrush = CreateSolidBrush(whitePixel);
    whitePen = CreatePen(PS_SOLID, 1, whitePixel);

    BACKGROUND_BRUSH = CreateSolidBrush(backgroundColor);
    BACKGROUND_PEN   = CreatePen(PS_SOLID, 1, backgroundColor);
    FOREGROUND_BRUSH = CreateSolidBrush(foregroundColor);
    FOREGROUND_PEN   = CreatePen(PS_SOLID, 1, foregroundColor);

    hdc = GetDC(hwnd);
    CreateScreenBitmap(hdc);
    ReleaseDC(hwnd, hdc);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    /* printf("Hello\n"); */
}

#if USE_KVM
void
GetAndStoreNextKVMEvent(bool_t forever, ulong64 waitUntil) {
    MSG msg;
    gotEvent = KNI_FALSE;

    if (forever) {
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (msg.message == WM_QUIT) {
                break;
            }

            TranslateMessage (&msg) ;
            DispatchMessage (&msg) ;
            if (gotEvent) {
                StoreMIDPEvent(&kvmEvent);
                return;
            }
        }
        return;
    } else {
        HANDLE h;

        for (;;) {

            /* see if backlight is on */
            drawBackLight(isBklite_on);

            /* see if we should toggle the network indicator LED */
            if ((netIndicatorCount != latestNICount) || 
                isLED_on == KNI_TRUE) {
                updateNetIndicator();
            }

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    return;
                }
                
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (gotEvent) {
                    StoreMIDPEvent(&kvmEvent);
                    return;
                }
            }

            if (waitUntil == 0) {
                return;
            } else {
                int ret, timeout, alt;

                if ((timeout = waitUntil - midp_getCurrentTime()) <= 0) {
                    return;
                }

#ifdef LATER
                /* use an alternative timeout if a timer is pending,  */
                /* in order to ensure that the timer is not made late */
                /* by a long waitUntil value.                         */
                alt = timerPending - midp_getCurrentTime();
                if ((alt > 0) && (alt < timeout)) {
                    timeout = alt;
                }
#endif

                ret =  MsgWaitForMultipleObjects(0, &h, KNI_FALSE, 
                                                 timeout, QS_ALLEVENTS);

                if (ret == WAIT_TIMEOUT) {
                    return;
                } else if (ret == -1) {
                    fprintf(stderr, "MsgWaitForMultipleObjects failed\n");
                }
            }
        }
    }
}
#endif /* USE_KVM */

void
DrawBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int rop)
{
    BITMAP bm;
    HDC hdcMem;
    POINT ptSize, ptOrg;
    HBITMAP tmp;

    hdcMem = CreateCompatibleDC(hdc);
    CHECK_RETURN(tmp = SelectObject(hdcMem, hBitmap));

    SetMapMode(hdcMem, GetMapMode(hdc));
    GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bm);
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP(hdc, &ptSize, 1);

    ptOrg.x = 0;
    ptOrg.y = 0;

    DPtoLP(hdcMem, &ptOrg, 1);

    BitBlt(hdc, x, y, ptSize.x, ptSize.y,
           hdcMem, ptOrg.x, ptOrg.y, rop);

    SelectObject(hdcMem, tmp);
    DeleteDC(hdcMem);
}

static void
paintHorizontalScroll(HDC hdc, int scrollPosition, int scrollProportion)
{
    /* arrow images; all are 7x7 */
    static BYTE left_bits[]  =
    {0xF7,0, 0xE7,0, 0xC1,0, 0x81,0, 0xC1,0, 0xE7,0, 0xF7,0};
    static BYTE right_bits[] =
    {0xDF,0, 0xCF,0, 0x07,0, 0x03,0, 0x07,0, 0xCF,0, 0xDF,0};

#define HORIZ_Y  (y_offset + paintHeight + 5)
#define LEFT_X   (x_offset + (PAINT_WIDTH/2) - 10)
#define RIGHT_X  (x_offset + (PAINT_WIDTH/2) + 4)

    HBITMAP hb;
    CHECK_RETURN(SelectObject(hdc, BACKGROUND_PEN));
    CHECK_RETURN(SelectObject(hdc, BACKGROUND_BRUSH));

    /* If the scrollable is 100% the proportion of the viewport,    */
    /* there's no need for any scrolling, clear the arrows and      */
    /* return.                                                      */
    if (scrollProportion >= 100 || scrollProportion == 0) {
        Rectangle(hdc,  LEFT_X, HORIZ_Y, LEFT_X + 7, HORIZ_Y + 7);
        Rectangle(hdc,  RIGHT_X, HORIZ_Y, RIGHT_X + 7, HORIZ_Y + 7);
        return;
    }

    /* If we're somewhere between the left and right,   */
    /* add both scroll arrows                           */
    if (scrollPosition > 0 && scrollPosition < 100) {
        /* WEST */
        hb = CreateBitmap(7, 7, 1, 1, left_bits);
        DrawBitmap(hdc, hb, LEFT_X, HORIZ_Y, MERGECOPY);
        DeleteObject(hb);
        /* EAST */
        hb = CreateBitmap(7, 7, 1, 1, right_bits);
        DrawBitmap(hdc, hb, RIGHT_X, HORIZ_Y, MERGECOPY);
        DeleteObject(hb);
    /* If we're at the left, add the right arrow, clear the */
    /* left arrow                                           */
    } else if (scrollPosition == 0) {
        /* EAST */
        hb = CreateBitmap(7, 7, 1, 1, right_bits);
        DrawBitmap(hdc, hb, RIGHT_X, HORIZ_Y, MERGECOPY);
        DeleteObject(hb);
        Rectangle(hdc,  LEFT_X, HORIZ_Y, LEFT_X + 7, HORIZ_Y + 7);
    /* We're at the right, add the left arrow, clear the    */
    /* right arrow                                          */
    } else {
        /* WEST */
        hb = CreateBitmap(7, 7, 1, 1, left_bits);
        DrawBitmap(hdc, hb, LEFT_X, HORIZ_Y, MERGECOPY);
        DeleteObject(hb);
        Rectangle(hdc,  RIGHT_X, HORIZ_Y, RIGHT_X + 7, HORIZ_Y + 7);
    }
}

static void
paintVerticalScroll(HDC hdc, int scrollPosition, int scrollProportion)
{
    /* arrow images; all are 7x7 */
    static BYTE up_bits[]    =
    {0xEF,0, 0xC7,0, 0x83,0, 0x01,0, 0xC7,0, 0xC7,0, 0xFF,0};
    static BYTE down_bits[]  =
    {0xFF,0, 0xC7,0, 0xC7,0, 0x01,0, 0x83,0, 0xC7,0, 0xEF,0};

#define VERT_X   (x_offset + (PAINT_WIDTH/2) - 4) 
#define UP_Y     (y_offset + paintHeight + MENUBAR_BORDER_HEIGHT) 
#define DOWN_Y   (y_offset + paintHeight + 11)

    HBITMAP hb;
    CHECK_RETURN(SelectObject(hdc, BACKGROUND_PEN));
    CHECK_RETURN(SelectObject(hdc, BACKGROUND_BRUSH));

    if (KNI_TRUE == inFullScreenMode) {
      return;
    }

    /* clear the arrow area */
    Rectangle(hdc, VERT_X, UP_Y, VERT_X + 8, UP_Y + BOTTOM_BAR_HEIGHT - 1);

    /* If the scrollable is 100% the proportion of the viewport,    */
    /* there's no need for any scrolling, clear the arrows and      */
    /* return.                                                      */
    if (scrollProportion >= 100 || scrollProportion == 0) {
        return;
    }

    /* If we're somewhere between the top and bottom,   */
    /* add both scroll arrows                           */
    if (scrollPosition > 0 && scrollPosition < 100) {
        /* NORTH */
        hb = CreateBitmap(7, 7, 1, 1, up_bits);
        DrawBitmap(hdc, hb, VERT_X, UP_Y, MERGECOPY);
        DeleteObject(hb);
        /* SOUTH */
        hb = CreateBitmap(7, 7, 1, 1, down_bits);
        DrawBitmap(hdc, hb, VERT_X, DOWN_Y, MERGECOPY);
        DeleteObject(hb);
    /* If we're at the top, add the down arrow, clear the   */
    /* up arrow                                             */
    } else if (scrollPosition == 0) {
        /* SOUTH */
        hb = CreateBitmap(7, 7, 1, 1, down_bits);
        DrawBitmap(hdc, hb, VERT_X, DOWN_Y, MERGECOPY);
        DeleteObject(hb);
        Rectangle(hdc, VERT_X, UP_Y, VERT_X + 7, UP_Y + 7);
    /* We're at the bottom, add the up arrow, clear the     */
    /* down arrow                                           */
    } else {
        /* NORTH */
        hb = CreateBitmap(7, 7, 1, 1, up_bits);
        DrawBitmap(hdc, hb, VERT_X, UP_Y, MERGECOPY);
        DeleteObject(hb);
        Rectangle(hdc,  VERT_X, DOWN_Y, VERT_X + 7, DOWN_Y + 7);
    }

}

HWND midpGetWindowHandle() 
{
    return hMainWindow;
}

void InitializeWindowSystem()
{
    int x, y;
    HDC hdc, hdcMem;

    /* set up the offsets for non-full screen mode*/
    setUpOffsets(KNI_FALSE);
    inFullScreenMode = KNI_FALSE;
    requestedFullScreenMode = KNI_FALSE;
    drawTrustedMIDletIcon = KNI_FALSE;

    CreateEmulatorWindow();
    LCDUIinitMenus();
    LCDUIinitPopupMenu();
}

void
FinalizeWindowSystem() {
    if (initialized) {
        /* Clean up thread local data */
        void* ptr = (void*) TlsGetValue(tlsId);

        if (ptr != NULL) {
            /* Must free TLS data before freeing the TLS ID */
            midpFree(ptr);
            TlsFree(tlsId);
        }
        LCDUIfinalizeMenus();
        LCDUIfinalizePopupMenu();
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
    case KEY_POWER:    return Keys[0].name;
    case KEY_SEND:     return Keys[8].name;
    case KEY_END:      return Keys[9].name;
    case KEY_CLEAR:    return Keys[10].name;
    case KEY_SOFT1:    return Keys[1].name;
    case KEY_SOFT2:    return Keys[2].name;
    case KEY_UP:       return Keys[3].name;
    case KEY_DOWN:     return Keys[4].name;
    case KEY_LEFT:     return Keys[5].name;
    case KEY_RIGHT:    return Keys[6].name;
    case KEY_SELECT:   return Keys[7].name;
    case KEY_1:        return Keys[11].name;
    case KEY_2:        return Keys[12].name;
    case KEY_3:        return Keys[13].name;
    case KEY_4:        return Keys[14].name;
    case KEY_5:        return Keys[15].name;
    case KEY_6:        return Keys[16].name;
    case KEY_7:        return Keys[17].name;
    case KEY_8:        return Keys[18].name;
    case KEY_9:        return Keys[19].name;
    case KEY_ASTERISK: return Keys[20].name;
    case KEY_0:        return Keys[21].name;
    case KEY_POUND:    return Keys[22].name;
    default:           return NULL;
    }
}

/* The two functions setBrush and setPen are used by other .c files 
   and  are kept here to prevent creation of a new file. */

HBRUSH
setBrush(HDC hdc, int pixel)
{
    return SelectObject(hdc, CreateSolidBrush(pixel));
}

HPEN
setPen(HDC hdc, int pixel, int dotted)
{
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, backgroundColor);

    if (dotted) {
        SetBkMode(hdc, TRANSPARENT);
        return SelectObject(hdc, CreatePen(PS_DOT, 1, pixel));
    } else {
        return SelectObject(hdc, CreatePen(PS_SOLID, 1, pixel));
    }
}

void
setupFont(HDC hdc, FONTPARAMS_PROTO)
{
    int faceIndex  = (face == FACE_PROPORTIONAL) ? 2 : (face == FACE_MONOSPACE) ? 1 : 0;
    int sizeIndex  = (size == SIZE_SMALL) ? 0 : (size == SIZE_LARGE) ? 2 : 1;
    int styleIndex = 0;

    int height = "\011\013\017"[sizeIndex];
    int pitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    switch (faceIndex) {
    case 0: pitchAndFamily = DEFAULT_PITCH  | FF_DONTCARE; break;
    case 1: pitchAndFamily = FIXED_PITCH    | FF_MODERN;   break;
    case 2: pitchAndFamily = VARIABLE_PITCH | FF_ROMAN;    break;
    }

    if (style & STYLE_BOLD) styleIndex += 1;
    if (style & STYLE_ITALIC) styleIndex += 2;
    if (style & STYLE_UNDERLINED) styleIndex += 4;

    if (fonts[faceIndex][sizeIndex][styleIndex] == NULL) {
#ifdef INCLUDE_I18N
        /* currently the only supported locale is Japanese. */
        DWORD cs = strncmp(getSystemProp("microedition.locale"), "ja", 2) ?
                   ANSI_CHARSET: SHIFTJIS_CHARSET;
#else
        DWORD cs = ANSI_CHARSET;
#endif
        int weight = FW_NORMAL;
        int width  = (height - 1) >> 1; /* generate narrow fonts */

        if (style & STYLE_BOLD) {
            weight = FW_EXTRABOLD;
            /* ++width; */
        }

        if ((face == FACE_MONOSPACE) || (size == SIZE_SMALL)) {
            width = 0;          /* use default width */
        }

        fonts[faceIndex][sizeIndex][styleIndex] = 
            CreateFont(
          /* height */ -height,
           /* width */ width,
      /* escapement */ 0,
     /* orientation */ 0,
          /* weight */ weight,
          /* italic */ (style & STYLE_ITALIC) ? 1 : 0,
       /* underline */ (style & STYLE_UNDERLINED) ? 1 : 0,
       /* strikeout */ 0,
   /* character set */ cs,
       /* precision */ OUT_STROKE_PRECIS,
        /* clipping */ CLIP_DEFAULT_PRECIS,
         /* quality */ NONANTIALIASED_QUALITY,
/* pitch and family */ pitchAndFamily,
     /* family name */ "");
    }

    SelectObject(hdc, fonts[faceIndex][sizeIndex][styleIndex]);
}

void LCDUIrefresh(int x1, int y1, int x2, int y2)
{
    refreshPaintWindow(x1, y1, x2 + 1, y2 + 1);
}

/*
 * Get parameters describing the window system's capabilities
 */
void LCDUIgetDisplayParams(int *width, int *height, int *adornedHeight,
                           int *eraseColor, int *depth, int *flags, int *alphaLevels)
{
    *width  = PAINT_WIDTH;
    *height = DISPLAY_HEIGHT - TOP_BAR_HEIGHT;
    *adornedHeight = DISPLAY_HEIGHT - TOP_BAR_HEIGHT - BOTTOM_BAR_HEIGHT;
    *eraseColor = 0x00ffffff;
    *depth  = (numColors == 16) ? 4 : (numColors == 256) ? 8 : numColors / 2;
    *flags  = ((numColors > 16) ? 1 : 0) 
            | SUPPORTS_POINTER 
            | SUPPORTS_MOTION
            | (doubleBuffer ? SUPPORTS_DOUBLEBUFFER : 0);
    *alphaLevels = numColors;
}

int
LCDUIsetVerticalScroll(int scrollPosition, int scrollProportion)
{
    static int vScrollPosition, vScrollProportion;

    int tmp1 = vScrollPosition;
    int tmp2 = vScrollProportion;
    HDC hdc;

    vScrollPosition = scrollPosition;
    vScrollProportion = scrollProportion;

    hdc = getBitmapDC(UNTRANSLATED_SCREEN_BITMAP);
    paintVerticalScroll(hdc, scrollPosition, scrollProportion);
    releaseBitmapDC(hdc);
    REFRESHBOTTOMBAR();

    return tmp1;
}

jboolean LCDUIplaySound(int soundType)
{
    switch (soundType) {
    case ALERT_INFO:
        /* MessageBeep(MB_OK); */
        /* return KNI_TRUE; */
        return KNI_FALSE;

    case ALERT_WARNING:
        MessageBeep(MB_ICONEXCLAMATION);
        return KNI_TRUE;

    case ALERT_ERROR:
        MessageBeep(MB_ICONHAND);
        return KNI_TRUE;

    case ALERT_ALARM:
        MessageBeep(MB_ICONASTERISK);
        return KNI_TRUE;

    case ALERT_CONFIRMATION:
        /* MessageBeep(MB_ICONQUESTION); */
        /* return KNI_TRUE; */
        return KNI_FALSE;

    default:
        return KNI_FALSE;
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
LCDUIsetSoftButton(int index, const unicode *label, int len) 
{
    HDC hdcMem;
    int x, y, i, width;

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

    if ((index < 0) || (index >= 2)) {
        return;
    }

    if (len > MAX_SOFTBUTTON_CHARS) {
        len = MAX_SOFTBUTTON_CHARS;
    }

    hdcMem = getBitmapDC(UNTRANSLATED_SCREEN_BITMAP);
    SelectClipRgn(hdcMem, NULL);

    if (index == 0) {
        x = x_offset; 
    } else if (index == 1) {
        x = x_offset + ((PAINT_WIDTH + ARROWS_WIDTH)/2) + 1;
    }
    y = y_offset + paintHeight;

    /* draw top border of menu */
    DrawMenuBarBorder(hdcMem);

    CHECK_RETURN(SelectObject(hdcMem, BACKGROUND_BRUSH));
    CHECK_RETURN(SelectObject(hdcMem, BACKGROUND_PEN));
    CHECK_RETURN(SelectObject(hdcMem, GetStockObject(SYSTEM_FONT)));

    Rectangle(hdcMem, x, y + MENUBAR_BORDER_HEIGHT, 
              x + ((PAINT_WIDTH - ARROWS_WIDTH)/2), 
              y + bottomBarHeight);
    
    if (label == NULL) {
        releaseBitmapDC(hdcMem);
        REFRESHBOTTOMBAR();
        if (index == 1) {
            rlabel.num = 0;
            rlabel.label[0] = 0;
        } else {
            llabel.num = 0;
            llabel.label[0] = 0;
        }
        return;
    } 

    if (index == 1) {
        rlabel.num = i = len;
        while (--i >= 0) {
            rlabel.label[i] = label[i];
        }
    } else {
        llabel.num = i = len;
        while (--i >= 0) {
            llabel.label[i] = label[i];
        }
    }

    /*    setupFont(hdcMem, FACE_SYSTEM, STYLE_UNDERLINED, SIZE_MEDIUM);*/
    setupFont(hdcMem, FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM);
    width = 0;
    for (i = 0; i < len; ++i) {
      SIZE size;
        GetTextExtentPoint32W(hdcMem, &label[i], 1, &size);
        width += size.cx;
        if (width > ((PAINT_WIDTH - ARROWS_WIDTH)/2)) {
            len = i;
            width -= size.cx;
            break;
        }
    }
    
    if (index == 0) {
        x = x_offset + 2; /* left justified */
        SetTextAlign(hdcMem, TA_BOTTOM | TA_LEFT);
    } else if (index == 1) {
        x = x_offset + PAINT_WIDTH - 2; /*right justified */
        SetTextAlign(hdcMem, TA_BOTTOM | TA_RIGHT);
    }
    y = y_offset + paintHeight + bottomBarHeight;

    SetBkMode(hdcMem, OPAQUE); 
    SetTextColor(hdcMem, foregroundColor);
    SetBkColor(hdcMem, backgroundColor);
    TextOutW(hdcMem, x, y, label, len);
    
    releaseBitmapDC(hdcMem);
    REFRESHBOTTOMBAR();
}

void
LCDUIsetInputMode(int mode)
{
    static const BYTE ABC_bits[] = {
        0x98, 0xe3,
        0x6b, 0x5b,
        0x6b, 0x5f,
        0x68, 0xdf,
        0x0b, 0x5f,
        0x6b, 0x5f,
        0x68, 0xe3};
    static const BYTE abc_bits[] = {
        0xfb, 0xff,
        0xfb, 0xff,
        0xab, 0xff,
        0xe8, 0xe3,
        0x8b, 0x5f,
        0x6b, 0x5f,
        0x88, 0xe3};
    static const BYTE num_bits[] = {
        0xb1, 0x9f,
        0x36, 0xef,
        0xbe, 0xef,
        0xbd, 0x9f,
        0xbb, 0xef,
        0xbf, 0xef,
        0x10, 0x9f};
    static const BYTE SYM_bits[] = {
        0x95, 0x77,
        0x75, 0x27,
        0x75, 0x57,
        0xb5, 0x77,
        0xdb, 0x77,
        0xdb, 0x77,
        0x3b, 0x77};
#ifdef INCLUDE_I18N
    #include "inputmodes_ja.bmp"
#endif
    static RECT r;
    HDC hdc;
    BYTE *icon = NULL;

    hdc = getBitmapDC(UNTRANSLATED_SCREEN_BITMAP);

    if (r.left == 0) {
        r.left   = x_offset - 1 + 82; /* was 30 */
        r.top    = y_offset - topBarHeight + (topBarHeight - 7) / 2;
        r.right  = r.left + 15;
        r.bottom = r.top + 7;
    }

    CHECK_RETURN(SelectObject(hdc, BACKGROUND_BRUSH));
    CHECK_RETURN(SelectObject(hdc, BACKGROUND_PEN));

    Rectangle(hdc, r.left, r.top, r.right, r.bottom);

    switch (mode) {
    case 1:
        icon = (BYTE *) ABC_bits;
        break;
    case 2:
        icon = (BYTE *) abc_bits;
        break;
    case 3:
        icon = (BYTE *) num_bits;
        break;
    case 4:
        icon = (BYTE *) SYM_bits;
        break;
#ifdef INCLUDE_I18N
    case 5:
        icon = (BYTE *) hira_bits;
        break;
    case 6:
        icon = (BYTE *) kana_bits;
        break;
#endif
    case 0:
    default:
        break;
    }

    if (icon != NULL) {
        HBITMAP hb = CreateBitmap(15, 7, 1, 1, icon);
        DrawBitmap(hdc, hb, r.left, r.top, MERGECOPY);
        DeleteObject(hb);
    }

    releaseBitmapDC(hdc);

    InvalidateRect(hMainWindow, &r, KNI_TRUE);
}

static jboolean suppressedCodes[256];

static void
suppressCode(LPARAM lParam) {
    int scanCode = (lParam >> 16) & 0xff;
    suppressedCodes[scanCode] = 1;
}

static jboolean
isSuppressedCode(LPARAM lParam) {
    int scanCode = (lParam >> 16) & 0xff;
    return suppressedCodes[scanCode];
}

static int
convertKeyChar(WPARAM wParam, LPARAM lParam)
{
    int shifted = GetKeyState(VK_SHIFT) < 0;

    switch (wParam) {
    case VK_F1:
        return KEY_SOFT1;

    case VK_F2:
        return KEY_SOFT2;

    case VK_F6:
        return VK_SUSPEND_ALL;

    case VK_F7:
        return VK_RESUME_ALL;

    case VK_F8:
        return VK_SUSPEND_CURRENT;

    case VK_F9:
        return VK_RESUME_PREVIOUS;

    case VK_MULTIPLY:
        suppressCode(lParam);
        return KEY_ASTERISK;

    case VK_DIVIDE:
        suppressCode(lParam);
        return KEY_POUND;

    case VK_ADD:
    case VK_SUBTRACT:
    case VK_DECIMAL:
        suppressCode(lParam);
        return KEY_INVALID;

    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
        suppressCode(lParam);
        return '0' + (wParam - VK_NUMPAD0); /* depends on being contiguous */

    case VK_UP:
        return KEY_UP;

    case VK_DOWN:
        return KEY_DOWN;

    case VK_LEFT:
        return KEY_LEFT;

    case VK_RIGHT:
        return KEY_RIGHT;

    case VK_RETURN:
        return KEY_SELECT;

    default:
        return KEY_INVALID;
    }
}


static LRESULT CALLBACK 
WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    int x, y;
    PAINTSTRUCT ps;
    HDC hdc;
    LRESULT retval;
    HANDLE  *mutex;
    static int penDown = KNI_FALSE;
    static XRectangle SCREEN_BOUNDS = { 0,0,0,0 };
    static int delay_interval = 10; /* Delay 10 ms */
    static int java_thread_schedule_tick = 42;
    
    if (SCREEN_BOUNDS.width == 0) {
        SCREEN_BOUNDS.x = x_offset;
        SCREEN_BOUNDS.y = y_offset;
        SCREEN_BOUNDS.width = PAINT_WIDTH;
        SCREEN_BOUNDS.height = paintHeight;
    }
    
#ifdef INCLUDE_I18N
    if (isLocaleSupported() &&
        FilterEvent(hwnd, iMsg, wParam, lParam, 
                    &kvmEvent, &gotEvent, &retval)) {
        return retval;
    }
#endif
    
    switch (iMsg) {
        case WM_CREATE:
            hdc = GetDC(hwnd);
            CHECK_RETURN(SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT)));
            GetTextMetrics(hdc, &fixed_tm);
            CHECK_RETURN(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));
            GetTextMetrics(hdc, &tm);
            ReleaseDC(hwnd, hdc);
            return 0;
            
        case WM_PAINT :
            hdc = BeginPaint(hwnd, &ps);
            mutex = (HANDLE*) TlsGetValue(tlsId);
            WaitForSingleObject(*mutex, INFINITE);
            DrawBitmap(hdc, hScreenBitmap, 0, 0, SRCCOPY);
            ReleaseMutex(*mutex);
            EndPaint(hwnd, &ps);
            return 0;

        case WM_DESTROY:
            PostQuitMessage (0);
            kvmEvent.type = systemKVMEvent;
            kvmEvent.chr = VK_SHUTDOWN;
            gotEvent = KNI_TRUE;
            return 0;
        
        case WM_APP: /* jsr135 eom event */
            kvmEvent.type = mmEOMEvent;
            kvmEvent.chr = (int)lParam;
            kvmEvent.screenX = (short)(wParam & 0x7fff); /* playerID */
            gotEvent = KNI_TRUE;
            break;
            
        case WM_CHAR:
            /* suppress ASCII control characters, except backspace */
            if ((wParam < 32) && (wParam != 8)) {
                return 0;
            }
      
            /* also suppress characters from the numeric keypad */
            if (isSuppressedCode(lParam)) {
                return 0;
            }
        
            kvmEvent.chr = wParam;
            kvmEvent.type = keyTypedKVMEvent;
            gotEvent = KNI_TRUE;
            return 0;

        case WM_KEYDOWN:
        case WM_KEYUP: {
            int key = convertKeyChar(wParam, lParam);

            if (key == KEY_INVALID) {
                return 0;
            }

            kvmEvent.chr = key;

            if (iMsg == WM_KEYUP) {
                kvmEvent.type = keyUpKVMEvent;
            } else if (lParam & 0x40000000) {
                kvmEvent.type = keyRepeatKVMEvent;
            } else {
                kvmEvent.type = keyDownKVMEvent;
            }

            if (wParam >= VK_F6 && wParam <= VK_F10) {
                kvmEvent.type = systemKVMEvent;
            }

            gotEvent = KNI_TRUE;

            if ((kvmEvent.chr == KEY_SOFT1) || (kvmEvent.chr == KEY_SOFT2)) {
                if (iMsg == WM_KEYDOWN) {
                    gotEvent = 
                        LCDUIpopupMenuInternalEvent(kvmEvent.chr, &kvmEvent);
                    if (gotEvent == KNI_FALSE) {
                        gotEvent = 
                            LCDUImenuInternalEvent(kvmEvent.chr, &kvmEvent);
                    }
                } else {
                    gotEvent = KNI_FALSE;
                }
            } else if (iMsg != WM_KEYUP) {
              jboolean tmp;
                tmp = LCDUIpopupMenuInternalEvent(kvmEvent.chr, 
                                                           &kvmEvent);
                if (tmp == KNI_FALSE) {
                  tmp = LCDUImenuInternalEvent(kvmEvent.chr, &kvmEvent);
                }
                if (tmp && (kvmEvent.type == invalidKVMEvent)) {
                  gotEvent = KNI_FALSE;
                }
            } else if (kvmEvent.type == systemKVMEvent && iMsg != WM_KEYDOWN) {
                gotEvent = KNI_FALSE;
            }

            return 0;
        }

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {

	  /*the only event of this type that we want MIDP
	    to handle is the F10 key.
	    if we receive any other event of this type, break.*/

	  if (wParam == VK_F10) {

	    /* send a kvm event for key down on F10 */
	    if (iMsg == WM_SYSKEYDOWN) {
	      kvmEvent.chr  = VK_KILL_CURRENT;
	      kvmEvent.type = systemKVMEvent;
	      gotEvent      = KNI_TRUE;
	    }

	    /* if we got a key up on F10 then return
	       without sending a kvm event */
	    
	    return 0;
	    
	  }
	}

        break;

        case WM_TIMER:
            KillTimer(hMainWindow, wParam);
            kvmEvent.type = timerKVMEvent;
            kvmEvent.chr  = wParam;
            gotEvent = KNI_TRUE;
            return 0;

        case WM_MOUSEMOVE:
            if (! (wParam & MK_LBUTTON)) {
                return 0;
            }

            if (lastButton != NULL) {
                x = LOWORD(lParam);
                y = HIWORD(lParam);
            
                if (INSIDE(x, y, lastButton->bounds)) {
                    /* we're still INSIDE--do nothing */
                    return 0;
                } else {
                    /* Force a keyUp event on the old key */
                    kvmEvent.chr = lastButton->button;
                    kvmEvent.type = keyUpKVMEvent;

                    lastButton = NULL;
                    gotEvent = KNI_TRUE;
                    return 1;   /* reprocesses this message */
                }
            }
            /* FALL THROUGH */

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP: 
        {
            int i;

            /* Cast lParam to "short" to preserve sign */
            x = (short)LOWORD(lParam);
            y = (short)HIWORD(lParam);

            if ((INSIDE(x, y, SCREEN_BOUNDS)) || (penDown == KNI_TRUE)) {
                    /* The max possible values for the X & Y coordinates */
                int maxX = x_offset + PAINT_WIDTH - 1;
                int maxY = y_offset + paintHeight - 1;

                gotEvent = KNI_TRUE;

                /* Check to see if we have moved the mouse outside of
                 * the screen bounds while holding the mouse button.
                 * If so, we still need to continue to send the mouse
                 * events to the MIDlet, but, with at least one
                 * coordinate "pegged" to the edge of the screen.
                 */
                kvmEvent.screenX = MIN(x, maxX) - x_offset;
                if (kvmEvent.screenX < 0) {
                    kvmEvent.screenX = 0;
                }
                kvmEvent.screenY = MIN(y, maxY) - y_offset;
                if (kvmEvent.screenY < 0) {
                    kvmEvent.screenY = 0;
                }

                switch (iMsg) {
                    case WM_LBUTTONDOWN:
                        penDown = KNI_TRUE;
                        kvmEvent.type = penDownKVMEvent;
                        SetCapture(hwnd);
                        return 0;

                    case WM_LBUTTONUP:
                        if (!penDown) break;

                        penDown = KNI_FALSE;
                        kvmEvent.type = penUpKVMEvent;
                        ReleaseCapture();
                        return 0;

                    default:
                        if (!penDown) break;

                        kvmEvent.type = penMoveKVMEvent;
                        return 0;
                }

                /* We received a button up or a drag event, but, we did not
                 * receive the original button down. Ignore the event.
                 */
                gotEvent = KNI_FALSE;
                return 0;
            } else for (i = 0; i < NUMBEROF(Keys); ++i) { 
                if (INSIDE(x, y, Keys[i].bounds)) {
                    gotEvent = KNI_TRUE;
                    lastButton = Keys + i;
#ifdef SOUND_SUPPORTED
                    if (iMsg == WM_LBUTTONDOWN) {
                        MessageBeep(MB_OK);
                    }
#endif
                    if (Keys[i].button == KEY_POWER) {
                        kvmEvent.type = systemKVMEvent;
                        kvmEvent.chr = VK_SHUTDOWN;
                        return 0;
                    } else if (    (Keys[i].button == KEY_SOFT1)
                                   || (Keys[i].button == KEY_SOFT2) ) {

                        if (iMsg == WM_LBUTTONUP) {
                            gotEvent = KNI_FALSE;
                        } else {
                            gotEvent = 
                              LCDUIpopupMenuInternalEvent(Keys[i].button,
                                                          &kvmEvent);
                            if (gotEvent == KNI_FALSE) {
                              gotEvent = LCDUImenuInternalEvent(Keys[i].button,
                                                                &kvmEvent);
                            }
                        }
                        return 0;
                    } else if (Keys[i].button == KEY_END) {

                if (iMsg == WM_LBUTTONDOWN) {
                    LCDUIdismissMenu();
                    LCDUIdismissPopupMenu();
                    kvmEvent.type = systemKVMEvent;
                    kvmEvent.chr = VK_KILL_CURRENT;
                    gotEvent = KNI_TRUE;
                    return 0;
                } else {
                    break;
                }
                    } else {
                        jboolean tmp;
                        kvmEvent.chr  = (short)Keys[i].button;

                        switch (iMsg) {
                            case WM_LBUTTONDOWN:
                            case WM_MOUSEMOVE:
                                kvmEvent.type = keyDownKVMEvent;
                                tmp = LCDUIpopupMenuInternalEvent
                                  ( Keys[i].button, &kvmEvent);
                                if (tmp == KNI_FALSE) {
                                  tmp = LCDUImenuInternalEvent(Keys[i].button,
                                                               &kvmEvent);
                                }
                                if (tmp && (kvmEvent.type == invalidKVMEvent)) {
                                  gotEvent = KNI_FALSE;
                                }
                                break;

                            case WM_LBUTTONUP:
                                lastButton = NULL;
                                kvmEvent.type = keyUpKVMEvent;
                                break;

                            default:
                                break;
                        }
                    }

                    return 0;
                }
            }
        }
        return 0;

#ifdef SKINS_MENU_SUPPORTED
        case WM_COMMAND: {
            int i = LOWORD(wParam);
            int len;
            FILE *f;
            char *buf = NULL;
            HDC hdc, hdcMem;
            HANDLE *mutex;
            HBITMAP tmp, img;
            BITMAPINFOHEADER *b;

            if ((i < 0) || (i >= numSkins) ||
                ((f = fopen(skins[i], "r")) == NULL)) {
                b = phone_dib;
            } else {
                BITMAPFILEHEADER bh;

                char *p;

                fread(&bh, sizeof(bh), 1, f);
                len = bh.bfSize - sizeof(BITMAPFILEHEADER);
                p = buf = (unsigned char*)midpMalloc(len);

                if (p == NULL) {
                    break;
                }

                while (len) {
                    int n = fread(p, 1, len, f);
                    if (n <= 0) break;
                    p += n;
                    len -= n;
                }

                fclose(f);
                b = (BITMAPINFOHEADER*) buf;
            }

            len = (b->biBitCount > 8) ? 0 : 
                (b->biClrUsed ? b->biClrUsed : (1 << b->biBitCount));

            hdc = GetDC(hwnd);
            hdcMem = CreateCompatibleDC(hdc);
            img = CreateDIBitmap(hdc, b, CBM_INIT, 
                                 ((char *)b) + b->biSize + 4*len, 
                                 (BITMAPINFO*)b, DIB_RGB_COLORS);
            ReleaseDC(hwnd, hdc);

            mutex = (HANDLE*) TlsGetValue(tlsId);
            WaitForSingleObject(*mutex, INFINITE);
            CHECK_RETURN(tmp = SelectObject(hdcMem, hScreenBitmap));

            ExcludeClipRect(hdcMem, 
                            x_offset - 1,
                            y_offset - topBarHeight - 1,
                            x_offset + DISPLAY_WIDTH + 1,
                            y_offset - topBarHeight + DISPLAY_HEIGHT + 1);

            DrawBitmap(hdcMem, img, 0, 0, SRCCOPY);

            SelectObject(hdcMem, tmp);
            DeleteDC(hdcMem);
            ReleaseMutex(*mutex);

            if (buf) midpFree(buf);
            DeleteObject(img);

            InvalidateRect(hwnd, NULL, KNI_TRUE);
        }
        break;
#endif
        
        default:
            break;
    }

    return DefWindowProc (hwnd, iMsg, wParam, lParam);
}

#ifdef INCLUDE_I18N

static jboolean
FilterEvent(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam, 
            KVMEventType *kvmEvent, int *gotEvent, int *retval) {

    switch (iMsg) {
    case WM_IME_STARTCOMPOSITION:{
        HIMC hIMC = ImmGetContext(hwnd);

        /* Set preedit font */
        {
            HFONT hFont, hScrnFont;
            HDC hdcMem;
            LOGFONT lf;

            hdcMem = getBitmapDC(UNTRANSLATED_SCREEN_BITMAP);
            setupFont(hdcMem, FACE_SYSTEM, STYLE_UNDERLINED, SIZE_MEDIUM);

            hFont = GetStockObject(SYSTEM_FONT);
            hScrnFont = SelectObject(hdcMem, hFont);

            GetObject(hScrnFont, sizeof(LOGFONT), &lf);

            SelectObject(hdcMem, hScrnFont);
            releaseBitmapDC(hdcMem);

            ImmSetCompositionFont(hIMC, &lf);
        }

        /* Set preedit position */
        {
            COMPOSITIONFORM cf;
            cf.dwStyle = CFS_POINT;
            cf.ptCurrentPos.x = x_offset;
            cf.ptCurrentPos.y = y_offset + paintHeight;
            ImmSetCompositionWindow(hIMC, &cf);
        }

        ImmReleaseContext(hwnd, hIMC);

        break;
    }

    case WM_IME_COMPOSITION:
        if (lParam & GCS_RESULTSTR) {
            HIMC hIMC = ImmGetContext(hwnd);
            int len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
            if (len > 0) {
                unicode *str = (unicode *)midpMalloc(sizeof(unicode) * (len+1));

                if (str != NULL) {
                    len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, 
                                                   str, len);
                    ImmReleaseContext(hwnd, hIMC);

                    /* ImmGetCompositionStringW has a bug that returns the
                       multibytes length of a converted unicode text not
                       its unicode length. This workaround is totally
                       bogus, but works for Ja Win98.*/
                    len /= MB_CUR_MAX;

                    kvmEvent->str = str;
                    kvmEvent->len = len;
                    kvmEvent->type = imeKVMEvent;
                    *gotEvent = KNI_TRUE;
                    *retval = 0;
                    return KNI_TRUE;
                }
            }
            ImmReleaseContext(hwnd, hIMC);
        }
        break;
    }

    return KNI_FALSE;
}

#endif /* INCLUDE_I18N */
