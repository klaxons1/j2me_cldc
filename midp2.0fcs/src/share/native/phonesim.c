/*
 * @(#)phonesim.c	1.4 02/09/16 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <defaultLCDUI.h>
#include <malloc.h>
#include <nativeGUI.h>
#include <midpMalloc.h>

static int itemHeight;
static int itemWidth;
static int screenWidth, screenHeight;
static int blackPixel, whitePixel;

static unicode _call_[]     = {'I','n','c','o','m','i','n','g',' ','C','a','l','l',':'};
static unicode _answer_[]   = {'F','7',' ','T','o',' ','R','e','s','u','m','e'};

void
LCDUIinitPhoneSim()
{
    int ascent, descent, leading;

    /* get the line height of a bold font */
    LCDUIgetFontInfo(FACE_SYSTEM, STYLE_BOLD, SIZE_LARGE,
                     &ascent, &descent, &leading);
    /*
     * itemHeight is used for the line spacing for the menu items.
     * However, each line could have a number, which is drawn in bold
     * font, and the menu text is drawn in plain font. So, the line height
     * has to be the tallest font on the line.
     */
    itemHeight = ascent + descent + leading;

    itemWidth = LCDUIcharsWidth(FACE_SYSTEM, STYLE_BOLD, SIZE_LARGE,
                                _call_, 14);

    blackPixel = LCDUIgetPixel(0, 0, KNI_TRUE);
    whitePixel = LCDUIgetPixel(0xffffff, 0xff, KNI_TRUE);

    screenWidth = DISPLAY_WIDTH;
    screenHeight = DISPLAY_HEIGHT;
}

void
LCDUIincomingCall()
{
    short clip[4];
    int startX = 0, startY = 0;
    int width = 0, height = 0;
    int curY = 0;

    clip[0] = 0;
    clip[1] = 0;
    clip[2] = screenWidth;
    clip[3] = screenHeight;

    width = itemWidth + 30;
    height = itemHeight * 2 + 35;
    startX = (screenWidth - width) / 2;
    startY = (screenHeight - height) / 2;

    LCDUIfillRoundRect(whitePixel, clip, NULL, 0,
		 startX, startY, width, height, 20, 20);

    LCDUIdrawRoundRect(blackPixel, clip, NULL, 0,
         startX - 1, startY - 1, width + 1, height + 1, 20, 20);

    curY = startY + itemHeight;

    LCDUIdrawChars(blackPixel, clip, NULL, 0,
                   FACE_SYSTEM, STYLE_BOLD, SIZE_LARGE,
                   screenWidth / 2, curY, HCENTER | TOP,
                   _call_, 14);

    curY = curY + itemHeight + 5;

    LCDUIdrawChars(blackPixel, clip, NULL, 0,
                   FACE_SYSTEM, STYLE_PLAIN, SIZE_LARGE,
                   screenWidth / 2, curY, HCENTER | TOP,
                   _answer_, 12);

    LCDUIrefresh(0, 0, screenWidth, screenHeight);
}
