/*
 * @(#)screenGrabber.c	1.10 02/07/24 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <screengrab.h>

#include <kni.h>

/*
 * int sysGetScreenDigest()
 *
 * Native method called by the com.sun.midp.lcdui.ScreenGrabber class.
 *
 * Args: 
 *   None
 * Returns: 
 *   A 32 bit digest of the MIDP display, or 0 if an error
 *   has occurred.  
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_lcdui_ScreenGrabber_sysGetScreenDigest() {
    KNI_ReturnInt((jint)screenGrab());
}
