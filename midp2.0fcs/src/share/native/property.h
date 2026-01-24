/*
 * @(#)property.h	1.6 02/07/24 @(#)
 *
 * Copyright (c) 1998-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _PROPERTY_H
#define _PROPERTY_H

/*=========================================================================
 * MIDP Reference implementation
 *=========================================================================
 * SYSTEM:    MIDP RI
 * SUBSYSTEM: System dependent property lookup
 * FILE:      property.h
 * OVERVIEW:  Return MID Profile specific properties
 * AUTHOR:    Roger Riggs
 *=======================================================================*/

char *getSystemProperty(char *key);

#endif /* _PROPERTY_H */
