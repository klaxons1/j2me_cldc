/*
 * @(#)midlet.h	1.2 02/07/24 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _MIDLET_H
#define _MIDLET_H

/*=========================================================================
 * FUNCTION:      plaformRequest()
 * OVERVIEW:      Merge the given URL ASCII string and "com.sun.midp.midlet.
 *                <platform>RequestCommand" config value into a native
 *                command and spawn that command.
 *=======================================================================*/
void platformRequest(char* pszUrl);

#endif /* _MIDLET_H */
