/*
 * @(#)property.c	1.25 02/07/24 @(#)
 *
 * Copyright (c) 1998-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * MIDP Reference implementation
 *=========================================================================
 * SYSTEM:    MIDP RI
 * SUBSYSTEM: System dependent property lookup
 * FILE:      property.c
 * OVERVIEW:  Return MID Profile specific properties
 * AUTHOR:    Roger Riggs
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include "configuration.h"

/*=========================================================================
 * FUNCTION:      getSystemProperty()
 * TYPE:          public operation
 * OVERVIEW:      Return a value of a property key
 * INTERFACE:
 *   parameters:  char *key
 *   returns:     char *result
 *=======================================================================*/

char *getSystemProperty(char *key) {
    /*
     * System properties are now found in the configuration files.
     * The $MIDP_HOME/lib/appl.config file contains the application visible
     * parameters an $MIDP_HOME/lib/impl.config contains the properties
     * supporting the reference implementation itself.
     */
    return getSystemProp(key) ;
}

