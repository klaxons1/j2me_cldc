/*
 * @(#)lcWinJa.c	1.20 02/07/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <midpMalloc.h>
#include "defaultLCDUI.h"

#include <X11/Xlib.h>
#include <X11/Xlocale.h>

#include "localeMethod.h"
#include "lcWin.h"

typedef unsigned short FontCode;
typedef int FontCS;

#define NoFontCS (FontCS) -1

static const char *sjisEncoding = "SJIS";
static const char *eucEncoding = "EUC_JP";
static char *thisEncoding = NULL;

static const char *defFonts[] = {
    "-*-lucida sans typewriter-medium-r-normal-sans-%d-*-72-72-m-*-iso8859-1",
    "-misc-fixed-medium-r-normal--%d-*-72-72-c-*-jisx0208.1983-0",
    "-misc-fixed-medium-r-normal--%d-*-72-72-c-*-jisx0201.1976-0"
};

#define NUM_FONTS 36
#define NUM_ENCODINGS	3
static XFontStruct *fonts[NUM_FONTS][NUM_ENCODINGS];

static const char *solaris_system_faces[][NUM_ENCODINGS] = {
    /* system: plain, bold, oblique, boldoblique */
    {
        "-linotype-helvetica-medium-r-narrow-sans-%d-*-72-72-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-linotype-helvetica-bold-r-narrow-sans-%d-*-72-72-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-linotype-helvetica-medium-o-narrow-sans-%d-*-72-72-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-linotype-helvetica-bold-o-narrow-sans-%d-*-72-72-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    }
};

static const char *faces[][NUM_ENCODINGS] = {
    /* system: plain, bold, oblique, boldoblique */
    {
        "-linotype-helvetica-medium-r-narrow-sans-%d-*-72-72-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-linotype-helvetica-bold-r-narrow-sans-%d-*-72-72-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-linotype-helvetica-medium-o-narrow-sans-%d-*-72-72-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-linotype-helvetica-bold-o-narrow-sans-%d-*-72-72-p-*-iso8859-1"
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-bitstream-courier-medium-r-normal--%d-*-72-72-m-*-iso8859-1",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-bitstream-courier-bold-r-normal--%d-*-72-72-m-*-iso8859-1",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-bitstream-courier-medium-i-normal--%d-*-72-72-m-*-iso8859-1",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-bitstream-courier-bold-i-normal--%d-*-72-72-m-*-iso8859-1",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-b&h-lucidabright-medium-r-normal--%d-*-75-75-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-b&h-lucidabright-demibold-r-normal--%d-*-75-75-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-b&h-lucidabright-medium-i-normal--%d-*-75-75-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-medium-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
    {
        "-b&h-lucidabright-demibold-i-normal--%d-*-75-75-p-*-iso8859-1",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0208.1983-0",
        "-morisawa-gothic medium bbb-bold-r-normal-sans-%d-*-72-72-m-*-jisx0201.1976-0"
    },
#if 0 /* English Fonts */
    "-linotype-helvetica-medium-r-narrow-sans-%d-*-72-72-p-*-iso8859-1",
    "-linotype-helvetica-bold-r-narrow-sans-%d-*-72-72-p-*-iso8859-1",
    "-linotype-helvetica-medium-o-narrow-sans-%d-*-72-72-p-*-iso8859-1",
    "-linotype-helvetica-bold-o-narrow-sans-%d-*-72-72-p-*-iso8859-1"

    /* monospace: plain, bold, italic, bolditalic */
    "-bitstream-courier-medium-r-normal--%d-*-72-72-m-*-iso8859-1",
    "-bitstream-courier-bold-r-normal--%d-*-72-72-m-*-iso8859-1",
    "-bitstream-courier-medium-i-normal--%d-*-72-72-m-*-iso8859-1",
    "-bitstream-courier-bold-i-normal--%d-*-72-72-m-*-iso8859-1",

    /* proportional: plain, bold, italic, bolditalic*/
    "-b&h-lucidabright-medium-r-normal--%d-*-75-75-p-*-iso8859-1",
    "-b&h-lucidabright-demibold-r-normal--%d-*-75-75-p-*-iso8859-1",
    "-b&h-lucidabright-medium-i-normal--%d-*-75-75-p-*-iso8859-1",
    "-b&h-lucidabright-demibold-i-normal--%d-*-75-75-p-*-iso8859-1",
#endif
};

static Display *thisDisplay;

static void
unicodeToFontCode(const unicode uc, FontCode *fc, FontCS *cs)
{
    *cs = NoFontCS;

    if (uc <= 0x007e) {
	if ((uc == 0x005c /* JISX0201_YEN_SIGN */) ||
	    (uc == 0x007e /* JISX0201_OVERLINE */)) {
	    *fc = 0x0000;
	}
	*fc = uc;
        *cs = 0;
    } else if (uc == 0x00a5 /* UNICODE_YEN_SIGN */) {
        *fc = 0x005c; /* JISX0201_YEN_SIGN */
        *cs = 0;
    } else if (uc == 0x203e /* UNICODE_OVERLINE */) {
        *fc = 0x007e; /* JISX0201_OVERLINE */
        *cs = 0;
    } else if ((0xff61 <= uc) && (uc <= 0xff9f)) {
	*fc = 0x00a1 + uc - 0xff61;
        *cs = 2;
    } else {
        if (thisEncoding == sjisEncoding) {
            unsigned char sjis[2];
            int len = unicodeToNative(&uc, 1, sjis, 2);
            if (len == 2) {
                unsigned int hib = sjis[0], lob = sjis[1];
                hib -= (hib <= 0x9f) ? 0x71 : 0xb1;
                hib = (hib << 1) + 1;
                if (lob > 0x7f) lob--;
                if (lob >= 0x9e) {
                    lob -= 0x7d;
                    hib++;
                } else lob -= 0x1f;
                *fc = (hib << 8) | lob;
                *cs = 1;
            }
        } else if (thisEncoding == eucEncoding) {
            unsigned char euc[2];
            int len = unicodeToNative(&uc, 1, euc, 2);
            if (len == 2) {
                *fc = (euc[0] << 8) | euc[1];
                *fc &= ~0x8080;
                *cs = 1;
            }
        }
    }

    if (*cs == NoFontCS) {
        *fc = (FontCode) '?';
        *cs = 0;
    }
}

static void
init(Display *display)
{
    thisDisplay = display;

    if (strcmp(getLocaleEncoding(), sjisEncoding) == 0) {
        thisEncoding = (char *) sjisEncoding;
    } else if (strcmp(getLocaleEncoding(), eucEncoding) == 0) {
        thisEncoding = (char *) eucEncoding;
    }
}

static XFontStruct **
findFont(FONTPARAMS_PROTO)
{
    int faceKey  = (face == FACE_PROPORTIONAL) ? 2 : 
        (face == FACE_MONOSPACE) ? 1 : 0;
    int sizeKey  = (size == SIZE_SMALL) ? 0 : (size == SIZE_LARGE) ? 2 : 1;
    int styleKey = 0;
    int fontIndex;
    int height = "\011\014\020"[sizeKey];
    char fontName[256];
    int num = NUM_ENCODINGS;
    const char **cur_faces;

    if (style & STYLE_BOLD) styleKey += 1;
    if (style & STYLE_ITALIC) styleKey += 2;
    /* Underlining is not directly supported in X fonts */
    /* if (style & STYLE_UNDERLINED) styleKey += 4;     */

    fontIndex = styleKey + faceKey*4 + sizeKey*12;

    if (fonts[fontIndex][0]) return fonts[fontIndex];

    if (face == FACE_SYSTEM) {
        cur_faces = solaris_system_faces[styleKey];
    } else {
        cur_faces = faces[styleKey + faceKey*4];
    }

    while (--num >= 0) {
        sprintf(fontName, cur_faces[num], height);
        fonts[fontIndex][num] = XLoadQueryFont(thisDisplay, fontName);
        if (fonts[fontIndex][num] == NULL) {
            sprintf(fontName, defFonts[num], height);
            fonts[fontIndex][num] = XLoadQueryFont(thisDisplay, fontName);
            if (fonts[fontIndex][num] == NULL) {
                fprintf(stderr, "Could not load a font: %s\n", fontName);
                exit(1);
            }
        }
    }

    return fonts[fontIndex];
}

static XFontStruct *
getFontOfFontCS(FONTPARAMS_PROTO, FontCS cs)
{
    XFontStruct **fontInfo = findFont(FONTPARAMS);
    if (fontInfo != NULL) {
        return fontInfo[cs];
    }
    return NULL;
}

static XFontStruct *
findFontStruct(Font font)
{
    int i;

    for (i = 0; i < NUM_FONTS; ++i) {
        if (fonts[i]) {
            int j;
            for (j = 0; j < NUM_ENCODINGS; ++j) {
                if (fonts[i][j] && fonts[i][j]->fid == font) {
                    return fonts[i][j];
                }
            }
        }
    }

    return NULL;
}

static void
getFontInfo(FONTPARAMS_PROTO,
            int *ascent, int *descent, int *leading)
{
    XFontStruct **fontInfo = findFont(FONTPARAMS);
    int i = NUM_ENCODINGS;
    int maxascent = 0;
    int maxdescent = 0;
    while (--i >= 0) {
        XFontStruct *fs = fontInfo[i];
        if (maxascent < fs->max_bounds.ascent) {
            maxascent = fs->max_bounds.ascent;
        }
        if (maxdescent < fs->max_bounds.descent) {
            maxdescent = fs->max_bounds.descent;
        }
    }
    *ascent = maxascent;
    *descent = maxdescent;
    *leading = 1;
}

static void
convertText(FONTPARAMS_PROTO,
            const unicode *chars, int n, XTextItem16 **items, int *numItems,
            XTextItem16 *itemBuf, int itemBufSize,
            XChar2b   *charBuf, int charBufSize)
{
    FontCS cur = NoFontCS;
    int i, itemSize = 0, itemAlcSize = 0;
    XTextItem16 *itemFst = itemBuf;
    XTextItem16 *itemPtr = NULL;
#define CHAR__RATE 16
#define ARRAY_RATE 4
    
    for (i = 0; i < n; i++) {
        XFontStruct *fontInfo;
        FontCode fc;
        FontCS cs;

        unicodeToFontCode(chars[i], &fc, &cs);

        if (cur == cs) {
            if (((itemPtr->nchars) % CHAR__RATE) == 0){
                itemPtr->chars =
                    midpRealloc(itemPtr->chars,
			     sizeof(XChar2b) * (itemPtr->nchars + CHAR__RATE));
            }
        } else {
            if (itemSize < itemBufSize) {
                if (itemPtr == NULL) {
                    itemPtr = itemFst;
                } else {
                    itemPtr++;
                }
            } else {
                if (itemAlcSize <= itemSize) {
                    XTextItem16 *tmp;
                    itemAlcSize = itemSize + ARRAY_RATE;
                    tmp = midpCalloc(itemAlcSize, sizeof(XTextItem16));
                    memcpy(tmp, itemFst, (itemSize * sizeof(XTextItem16)));
                    if (itemFst != itemBuf) {
                        midpFree(itemFst);
                    }
                    itemFst = tmp;
                    itemPtr = itemFst + itemSize;
                } else {
                    itemPtr++;
                }
            }
            itemPtr->chars = midpMalloc(sizeof(XChar2b) * CHAR__RATE);
            itemPtr->nchars = 0;
            itemSize++;
        }

        fontInfo = getFontOfFontCS(FONTPARAMS, cs);

        itemPtr->chars[itemPtr->nchars].byte1 = (fc >> 8) & 0xff;
        itemPtr->chars[itemPtr->nchars].byte2 = (fc >> 0) & 0xff;
        itemPtr->nchars++;
        itemPtr->delta = 0;
        itemPtr->font = fontInfo->fid;

        cur = cs;
    }

    *items = itemFst;
    *numItems = itemSize;
#undef CHAR__RATE
#undef ARRAY_RATE
}

LcWinMethods
getJaWinMethods()
{
    static LcWinMethods win;

    if (win) {
        return win;
    }

    win = (LcWinMethods) midpMalloc(sizeof(LcWinMethodsRec));
    win->init = init;
    win->convertText = convertText;
    win->getFontStruct = findFontStruct;
    win->getFontInfo = getFontInfo;

    return win;
}
