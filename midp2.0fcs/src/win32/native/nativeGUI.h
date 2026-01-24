/*
 * @(#)nativeGUI.h	1.26 02/08/22 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef NATIVEGUI_H_INCLUDED
#define NATIVEGUI_H_INCLUDED

#include <defaultLCDUI.h>
#undef FIELD_OFFSET
#include <windows.h>
#include <ctype.h>

/*
 * Defines screen size
 */
#define DISPLAY_WIDTH    180
#define DISPLAY_HEIGHT   208

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

#define GRAPHICS_SETUP(filled)                                            \
    HDC hdc;                                                              \
    HPEN hPen     = NULL;                                                 \
    HBRUSH hBrush = NULL;                                                 \
                                                                          \
    hdc = getBitmapDC(dst);                                               \
    setupClip(hdc, dst, clip);                                            \
    if (filled) {                                                         \
        hBrush = setBrush(hdc, pixel);                                    \
        hPen   = setPen(hdc, pixel, 0);                                   \
    } else {                                                              \
        hPen   = setPen(hdc, pixel, dotted);                              \
        SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));                  \
    }

#define GRAPHICS_CLEANUP()                                        \
    if (hPen != NULL) DeleteObject(SelectObject(hdc, hPen));      \
    if (hBrush != NULL) DeleteObject(SelectObject(hdc, hBrush));  \
    releaseBitmapDC(hdc);

extern HWND          hMainWindow;
extern HBRUSH        BACKGROUND_BRUSH;
extern HPEN          BACKGROUND_PEN;
extern HPEN          whitePen;
extern HBRUSH        whiteBrush;
extern int           numColors;
extern jboolean      doubleBuffer;
extern int           backgroundColor;

extern HBRUSH setBrush(HDC hdc, int pixel);
extern HPEN setPen(HDC hdc, int pixel, int dotted);
extern HDC getBitmapDC(void *imageData);
extern void releaseBitmapDC(HDC hdcMem);

extern void setupFont(HDC hdc, FONTPARAMS_PROTO);
extern void setupClip(HDC hdc, void *imageData, short *clip);
extern void refreshWholePaintWindow();

extern void DrawBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int rop);
extern void refreshPaintWindow(int x1, int y1, int x2, int y2);
extern void setupClip(HDC hdc, void *imageData, short *clip);

#endif
