/*
 * @(#)screengrab.c	1.13 02/07/24 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
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
 * Because X Windows uses a colormap, and a pixel is an index into 
 * the colormap, each pixel in a screen capture image is mapped to its
 * red, green, and blue component color.  This allows identical screens
 * with alternat colormap entries for their pixels to map to the same
 * return value.
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
    XImage *img;
    XColor color;    
    
    int x;
    int y;
    int offset;
    int arrlen;
    unsigned long pix = 0;
    unsigned long crc = 0xFFFFFFFF;
    unsigned long mask = 0xFFFFFFFF;
    unsigned short rowofcolors[DISPLAY_WIDTH *3];

    /* 
     * Check that the native display has been set up - meaning
     * that createEmulatorWindow has been called.  
     * If not, return error case of '0'
     */
    if (display == NULL){
	return 0;
    }
    arrlen = (DISPLAY_WIDTH * 3 * sizeof(short));
    
    /* get the MIDP display screen as an image */
    img = XGetImage(display, outerWindow, X_SCREEN_OFFSET, Y_SCREEN_OFFSET, 
		    DISPLAY_WIDTH, DISPLAY_HEIGHT, mask, XYPixmap);
    
    for (y = 0; y < DISPLAY_HEIGHT; y++) {
	for (x = 0; x < DISPLAY_WIDTH; x++) {
	    pix = XGetPixel(img, x, y);
	    /* get color from colormap here */
	    color.pixel = pix;
	    XQueryColor(display, colormap, &color);  
	    offset = x * 3;
	    rowofcolors[offset] = color.red;
	    rowofcolors[++offset] = color.green;
	    rowofcolors[++offset] = color.blue;
	}
	crc = CRC32_update((unsigned char*) &rowofcolors,
			   arrlen, crc);	
    }
    
    /* cleanup */
    XDestroyImage(img);
    
    /* finalize the crc and return it */
    crc = CRC32_finalize(crc);

    /* don't return error case on successful hash */
    return (crc == 0) ? ++crc : crc;
}
