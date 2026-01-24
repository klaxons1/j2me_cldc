/*
 * @(#)text.c	1.9 02/08/22 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <stdio.h>

#include <kni.h>
#include <nativeGUI.h>

static int CvtRef(int refPoint)
{
    int v = 0;

    switch (refPoint & (TOP | BOTTOM | BASELINE)) {
    case BOTTOM:
        v |= TA_BOTTOM;
	break;

    case BASELINE:
        v |= TA_BASELINE;
	break;

    case TOP:
    default:
        v |= TA_TOP;
	break;
    }

    switch (refPoint & (LEFT | RIGHT | HCENTER)) {
    case RIGHT:
        v |= TA_RIGHT;
	break;

    case HCENTER:
        v |= TA_CENTER;
	break;

    case LEFT:
    default:
        v |= TA_LEFT;
        break;
    }

    return v;
}

/*
 * Draw the first n characters in chararray, with the anchor point of the
 * entire (sub)string located at x, y.
 */
void LCDUIdrawChars(int pixel, short *clip, void *dst, int dotted, 
		    FONTPARAMS_PROTO, 
		    int x, int y, int anchor, const unicode *chararray, 
		    int n)
{
    GRAPHICS_SETUP(FALSE);

    setupFont(hdc, FONTPARAMS);

    SetTextColor(hdc, pixel);
    SetBkMode(hdc, TRANSPARENT);
    SetTextAlign(hdc, CvtRef(anchor));

    TextOutW(hdc, x, y, chararray, n);

    /* we don't know the bounding box, so refresh the whole screen */
    if (!doubleBuffer && (dst == NULL)) {
        refreshWholePaintWindow();
    }
    GRAPHICS_CLEANUP();
}

/*
 * Get the ascent, descent and leading info for the font indicated 
 * by FONTPARAMS.
 */
void LCDUIgetFontInfo(FONTPARAMS_PROTO, 
		      int *ascent, int *descent, int *leading)
{
    TEXTMETRIC metric;
    HDC hdc;

    hdc = getBitmapDC(NULL);
    setupFont(hdc, FONTPARAMS);

    GetTextMetrics(hdc, &metric);
    releaseBitmapDC(hdc);

    *ascent  = metric.tmAscent;
    *descent = metric.tmDescent;
    *leading = metric.tmExternalLeading;
}

/*
 * Get the advance width for the first n characters in charArray if
 * they were to be drawn in the font indicated by FONTPARAMS.
 */
int LCDUIcharsWidth(FONTPARAMS_PROTO, const unicode *charArray, int n)
{
    HDC hdc;
    SIZE size_ret;

    hdc = getBitmapDC(NULL);
    setupFont(hdc, FONTPARAMS);

    if (!GetTextExtentPoint32W(hdc, charArray, n, &size_ret)) {
        fprintf(stderr, "GetTextExtentPoint32 failed.\n");
    }
    releaseBitmapDC(hdc);

    return size_ret.cx;
}
