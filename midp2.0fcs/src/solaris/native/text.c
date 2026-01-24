/*
 * @(#)text.c	1.16 02/08/22 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <defaultLCDUI.h>
#include <nativeGUI.h>
#include <midpMalloc.h>
#include <midpServices.h>

#include <X11/cursorfont.h>

#ifdef INCLUDE_I18N
#include "localeMethod.h"
#include "lcWin.h"
extern LcWinMethods lcWin;
#endif

static char *faces[] = {
   /* system: plain, bold, oblique, boldoblique */
    "-adobe-helvetica-medium-r-normal--%d-*-%d-%d-p-*-iso8859-1",
    "-adobe-helvetica-bold-r-normal--%d-*-%d-%d-p-*-iso8859-1",
    "-adobe-helvetica-medium-o-normal--%d-*-%d-%d-p-*-iso8859-1",
    "-adobe-helvetica-bold-o-normal--%d-*-%d-%d-p-*-iso8859-1",
 
    /* monospace: plain, bold, italic, bolditalic */
    "-bitstream-courier-medium-r-normal--%d-*-%d-%d-m-*-iso8859-1",
    "-bitstream-courier-bold-r-normal--%d-*-%d-%d-m-*-iso8859-1",
    "-bitstream-courier-medium-i-normal--%d-*-%d-%d-m-*-iso8859-1",
    "-bitstream-courier-bold-i-normal--%d-*-%d-%d-m-*-iso8859-1",

    /* proportional: plain, demibold, italic, demibolditalic*/
    "-b&h-lucidabright-medium-r-normal--%d-*-%d-%d-p-*-iso8859-1",
    "-b&h-lucidabright-demibold-r-normal--%d-*-%d-%d-p-*-iso8859-1",
    "-b&h-lucidabright-medium-i-normal--%d-*-%d-%d-p-*-iso8859-1",
    "-b&h-lucidabright-demibold-i-normal--%d-*-%d-%d-p-*-iso8859-1",
};

/* As a last resort, use this face */
static char* default_face = "-*-fixed-*-*-*-*-%d-*-*-*-*-*-iso8859-1";


static XFontStruct*
loadNativeFont(char* pattern, int height) {
    char         fontName[256];
    XFontStruct* returnFont;

    /* 75 dpi fonts should be on all X impls */
    sprintf(fontName, pattern, height, 75, 75);
    if (returnFont = XLoadQueryFont(display, fontName)) {
        return returnFont;
    }

    /* Now try 72 dpi fonts */
    sprintf(fontName, pattern, height, 72, 72);
    if (returnFont = XLoadQueryFont(display, fontName)) {
        return returnFont;
    }

    /* Lastly try 100 dpi fonts */
    sprintf(fontName, pattern, height, 100, 100);
    if (returnFont = XLoadQueryFont(display, fontName)) {
        return returnFont;
    }

    return NULL;
}

static XFontStruct *
findFont(FONTPARAMS_PROTO)
{
    int faceKey  = (face == FACE_PROPORTIONAL) ? 2 : (face == FACE_MONOSPACE) ? 1 : 0;
    int sizeKey  = (size == SIZE_SMALL) ? 0 : (size == SIZE_LARGE) ? 2 : 1;
    int styleKey = 0;
    int fontIndex;
    int height = "\011\013\017"[sizeKey];

    if (style & STYLE_BOLD) styleKey += 1;
    if (style & STYLE_ITALIC) styleKey += 2;
    /* Underlining is not directly supported in X fonts */
    /* if (style & STYLE_UNDERLINED) styleKey += 4;     */

    fontIndex = styleKey + faceKey*4 + sizeKey*12;

    if (fonts[fontIndex]) return fonts[fontIndex];

    if (fonts[fontIndex] = loadNativeFont(faces[styleKey+faceKey*4], height)) {
        return fonts[fontIndex];
    }

    if (faceKey) {
        /* We asked for a non-system font that didn't exist. */
        /* Retry query with the system font                  */
        return findFont(FACE_SYSTEM, style, size);
    }

    /* As the very last resort, used the 'fixed' font */
    if (fonts[fontIndex] = loadNativeFont(default_face, height)) {
        return fonts[fontIndex];
    }

    /* We tried really hard, but, couldn't find an acceptable font */
    KNI_FatalError("Could not load required font");

    return NULL;
}

 void
convertText(FONTPARAMS_PROTO,
            const unicode *chars, int n, XTextItem16 **items, int *numItems,
            XTextItem16 *itemBuf, int itemBufSize,
            XChar2b   *charBuf, int charBufSize)
{
    XFontStruct *fontInfo;

#ifdef INCLUDE_I18N
    if (lcWin) {
        lcWin->convertText(FONTPARAMS, 
                           chars, n, items, numItems, itemBuf, itemBufSize,
                           charBuf, charBufSize);
        return;
    }
#endif

    fontInfo = findFont(FONTPARAMS);

    if (n > charBufSize) {
        charBuf = midpCalloc(n, sizeof(XChar2b));
    }

    *items    = itemBuf;
    *numItems = 1;

    itemBuf->chars  = charBuf;
    itemBuf->nchars = 0;
    itemBuf->delta  = 0;
    itemBuf->font   = fontInfo->fid;

    while (--n >= 0) {
        /*
         * Here, only latin characters are expected.  If no latin
         * characters come, '?' is displayed instead.
         */
        if (*chars <= 0xff) {
            charBuf->byte1 = 0;
            charBuf->byte2 = *chars;
        } else {
            charBuf->byte1 = 0;
            charBuf->byte2 = 0x3f;
        }

        ++(itemBuf->nchars);
        ++charBuf;
        ++chars;
    }
}

 void
freeText(XTextItem16 *items, int numItems, 
         XTextItem16 *itemBuf, 
         XChar2b *charBuf, int charBufSize)
{
    int i;
    XChar2b *stop = charBuf + charBufSize;

    for (i = 0; i < numItems; ++i) {
        if ((items[i].chars < charBuf) || (items[i].chars >= stop)) {
            midpFree(items[i].chars);
        }
    }

    if (items != itemBuf) {
        midpFree(items);
    }
}

 void
getTextBounds(XTextItem16 *items, int numItems, 
              int *width, int *ascent, int *descent)
{
    *width = *ascent = *descent = 0;

    while (--numItems >= 0) {
        XFontStruct *fontStruct = findFontStruct(items->font);
        
        *width += XTextWidth16(fontStruct, items->chars, items->nchars);
        *width += items->delta;
        ++items;

        if (fontStruct->ascent  > *ascent)  *ascent  = fontStruct->ascent;
        if (fontStruct->descent > *descent) *descent = fontStruct->descent;

    }
}

static void
findUnderlineParams(Font fid, int *ypos, int *thickness)
{
    XFontStruct *f = findFontStruct(fid);
    unsigned long rawPosition, rawThickness;
    XCharStruct charMetrics;
    int font_ascent, font_descent, direction;
    
    XTextExtents(f, "_", 1, &direction, &font_ascent, &font_descent, 
                 &charMetrics);

    if (   !XGetFontProperty(f, underlinePositionAtom, &rawPosition)
        && !XGetFontProperty(f, rawPositionAtom, &rawPosition)) {

        rawPosition = -charMetrics.ascent;

        /* I don't like the look of underlining that touches chars. */
        if (rawPosition >= 0) rawPosition = 1;
    }

    if (   !XGetFontProperty(f, underlineThicknessAtom, &rawThickness)
        && !XGetFontProperty(f, rawThicknessAtom, &rawThickness)) {

        rawThickness = charMetrics.descent + charMetrics.ascent;
    }

    *ypos = rawPosition;
    *thickness = rawThickness;
}

/*
 * Draw the first n characters in chararray, with the anchor point of the
 * entire (sub)string located at x, y.
 */
void 
LCDUIdrawChars(int pixel, short *clip, void* dst, int dotted, 
	       FONTPARAMS_PROTO,
	       int x, int y, int anchor, const unicode *chararray, int n)
{
    Drawable drawable = setupGC(gc, pixel, clip, dst, dotted);
    int x1, y1;
    int x2, y2;
    int width, ascent, descent;

    XTextItem16 *items, itemBuf[4];
    XChar2b charBuf[80];
    int numItems;

    /* we must convert all of the text in order to get correct width */
    /* This makes malloc() unavoidable if the string is long or has  */
    /* many changes of font.  We try to avoid it by providing some   */
    /* data space and using malloc only when the space isn't enough. */
    convertText(FONTPARAMS,
                chararray, n, &items, &numItems, itemBuf, 4, charBuf, 80);
    getTextBounds(items, numItems, &width, &ascent, &descent);

    switch (anchor & (LEFT | RIGHT | HCENTER)) {
    case LEFT:
        break;

    case RIGHT:
        x -= width;
        break;

    case HCENTER:
        x -= (width >> 1);
        break;
    }

    switch (anchor & (TOP | BOTTOM | BASELINE)) {
    case BOTTOM:
        y -= descent;

    case BASELINE:
        break;

    case TOP:
    default:
        y += ascent;
        break;
    }

    x1 = x;
    x2 = x1 + width;
    y1 = y - ascent;
    y2 = y + descent;

    XDrawText16(display, drawable, gc, x, y, items, numItems);

    if (style & STYLE_UNDERLINED) {
        /* The underline position and thickness are sometimes found as */
        /* properties of the font.                                     */
        int yOff, thickness;
        findUnderlineParams(items[0].font, &yOff, &thickness);

        XFillRectangle(display, drawable, gc, 
                       x, y + yOff, width, thickness);
    }


    freeText(items, numItems, itemBuf, charBuf, 80);

    if (!doubleBuffer && (drawable == paintDrawable)) {
        refreshPaintWindow(x1, y1, x2, y2);
    }
}

/*
 * Get the ascent, descent and leading info for the font indicated 
 * by FONTPARAMS.
 */
void
LCDUIgetFontInfo(FONTPARAMS_PROTO,
                 int *ascent, int *descent, int *leading)
{
    XFontStruct *fontInfo;

#ifdef INCLUDE_I18N
    if (lcWin) {
        lcWin->getFontInfo(FONTPARAMS, ascent, descent, leading);
        return;
    }
#endif

    fontInfo = findFont(FONTPARAMS);

    *ascent  = fontInfo->ascent;
    *descent = fontInfo->descent;

    /* REMIND: is there a way to get the leading from an X font? */
    *leading = 1;
}

/*
 * Get the advance width for the first n characters in charArray if
 * they were to be drawn in the font indicated by FONTPARAMS.
 */
int 
LCDUIcharsWidth(FONTPARAMS_PROTO, const unicode *charArray, int n)
{
    int width, ascent, descent;

    XTextItem16 *items, itemBuf[4];
    XChar2b charBuf[80];
    int numItems;

    /* we must convert all of the text in order to get correct width */
    /* This makes malloc() unavoidable if the string is long or has  */
    /* many changes of font.  We try to avoid it by providing some   */
    /* data space and using malloc only when the space isn't enough. */
    convertText(FONTPARAMS,
                charArray, n, &items, &numItems, itemBuf, 4, charBuf, 80);
    getTextBounds(items, numItems, &width, &ascent, &descent);

    freeText(items, numItems, itemBuf, charBuf, 80);

    return width;
}
