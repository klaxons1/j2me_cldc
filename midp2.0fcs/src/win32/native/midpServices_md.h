/*
 * @(#)midpServices_md.h	1.5 02/07/24 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _MIDPSERVICES_MD
#define _MIDPSERVICES_MD

/*
 * Platform-specific code should go here
 */

#include <stdlib.h>

/*
 * Immediately terminate the process with the given exit status
 * (used by midp_exitVM / System.exit).
 */
#ifndef ERROR_THROW
#define ERROR_THROW(status) exit(status)
#endif

#endif /* _MIDPSERVICES_MD */
