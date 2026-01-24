/*
 * @(#)screengrab.h	1.6 02/07/24 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * The screenGrab utility function enables automated UI 
 * testing  by condensing the drawable MIDP screen into a simple 
 * unsigned long for easy comparison and storage.  It is unlikely two 
 * different screens will be transformed into the same output value.
 *
 * Because X Windows uses a colormap, and a pixel is an index into 
 * the colormap, each pixel in a screen capture image is mapped to its
 * red, green, and blue component color.  This allows identical screens
 * with alternate colormap entries for their pixels to map to the same
 * return value.  On Win32 systems a pixel encodes its colors, so this
 * expansion is not necessary.
 *
 * On an error, screenGrab will return 0 (zero).  (This occurs if
 * screeGrab is called when the native display has not been 
 * initialized.)  A valid screenGrab return which happens to be 0 
 * will be changed to 1.
 *
 * Note:
 * A secure message digest algorithm such as MD5 or SHA-1 would
 * be preferable to the CRC32 algorithm used, as such an algorithm
 * would provide more collision resistance in the final digested output
 * of the screen.  However, even with CRC32 the probability of two 
 * different screens mapping to the same long integer are small.
 * Native message digest implementations are available in the
 * 'midp/src/share/native/crypto' directory.  However, in case we need
 * to ship MIDP without SSL or crypto libraries included, this function
 * does not use these libraries.
 *
 */

/*
 * screenGrab()
 *
 * Args:
 *    None.
 * Returns: 
 *    A 32 bit CRC of the native MIDP display window, or
 *    0 if there is an error.
 */
unsigned long screenGrab();
