/*
 * @(#)lcWin.h	1.8 02/07/24 @(#)
 *
 * Copyright (c) 1999-2000 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _LCWIN_H_INCLUDED
#define _LCWIN_H_INCLUDED

#include <X11/Xlib.h>

typedef struct _LcWinMethodsRec {
    void (*init)(Display *d);
    void (*convertText)(FONTPARAMS_PROTO, 
                        const unicode *s,     int len,
                        XTextItem16 **items,  int *numItems,
                        XTextItem16 *itemBuf, int itemBufSize,
                        XChar2b *charBuf,     int charBufSize);
    XFontStruct *(*getFontStruct)(Font f);
    void (*getFontInfo)(FONTPARAMS_PROTO, 
                        int *ascent, int *descent, int *leading);
} LcWinMethodsRec, *LcWinMethods;

extern LcWinMethods getLcWinMethods(char *locale);

#endif
