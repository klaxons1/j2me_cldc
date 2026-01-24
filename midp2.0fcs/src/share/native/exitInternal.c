/*
 * @(#)exitInternal.c	1.5 02/09/03 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <midpServices.h>

/*=========================================================================
 * FUNCTION:      exitInternal(I)V
 * CLASS:         javax.microedition.midlet.Main
 * TYPE:          virtual native function
 * OVERVIEW:      Stop the VM and exit immediately.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_Main_exitInternal() {
    int value = (int)KNI_GetParameterAsInt(1);

    midp_exitVM(value);
}
