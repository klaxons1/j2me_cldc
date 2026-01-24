/*
 * @(#)screengrab.c	1.13 02/07/24 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <defaultLCDUI.h>
#include <nativeGUI.h>
#include <screengrab.h>
#include <crc32.h>

/*
 * screenGrab()
 *
 * screenGrab is a utility function to enable automated UI testing 
 * by condensing the drawable MIDP screen into a simple unsigned long 
 * for easy comparison and storage.  It is unlikely two different
 * screens will be transformed into the same output value.
 *
 * If an error occurs, screenGrab will return 0.  (If screenGrab
 * is called when the native display has not been initialized this
 * will happen.)  A valid screenGrab return value of 0 is changed 
 * to 1 so the caller will not interpret it as an error.
 *
 * Args:
 *   None.
 * Return: 
 *   A 32 bit CRC of the native MIDP display window, or
 *   0 if there is an error.
 */
unsigned long screenGrab()
{
    int x;
    int y;
    int arrlen;
    unsigned long pixel;
    unsigned long crc = 0xFFFFFFFF;
    unsigned long rowofpixels [DISPLAY_WIDTH];
    HDC hdc;

    /* 
     * Check that the native display has been set up - meaning
     * that createEmulatorWindow has been called.  
     * If not, return error case of '0'
     */
    if (hMainWindow == NULL){
	return 0;
    }

    arrlen = DISPLAY_WIDTH * sizeof(unsigned long);

    hdc = GetDC(hMainWindow);

    /* get the screen as an image? */

    for (y = 0; y < DISPLAY_HEIGHT; y++) {
	for (x = 0; x < DISPLAY_WIDTH; x++) {
	    pixel = GetPixel(hdc, x + X_SCREEN_OFFSET, y + Y_SCREEN_OFFSET);
	    rowofpixels[x] = pixel;
	}
	crc = CRC32_update((unsigned char*) &rowofpixels,
			   arrlen, crc);
    }
    /* cleanup */
    ReleaseDC(hMainWindow, hdc);

    /* finalize and return crc */
    crc = CRC32_finalize(crc);

    /* don't return error case on successful hash */
    return (crc == 0) ? ++crc : crc;
}
