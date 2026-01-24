/*
 * @(#)SystemOutputStream.c	1.8 02/07/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>

/*=========================================================================
 * FUNCTION:      putchar(C)V (STATIC)
 * CLASS:         com/sun/midp/io/SystemOutputStream
 * TYPE:          static native function
 * OVERVIEW:      Print a byte to the stdout file
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_SystemOutputStream_putchar() {
    jchar c = KNI_GetParameterAsChar(1);
    putchar((char)c);

    KNI_ReturnVoid();
}
