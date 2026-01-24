/*
 * @(#)lcWin.c	1.5 02/07/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include "defaultLCDUI.h"
#include "lcWin.h"

extern LcWinMethods getJaWinMethods();

LcWinMethods
getLcWinMethods(char *locale)
{
    if (locale && *locale) {
        if (strncmp(locale, "ja", 2) == 0) {
            return getJaWinMethods();
        }
#ifdef LATER
        if (strncmp(locale, "ko", 2) == 0) {
            return getKoWinMethods();
        }
        if (strncmp(locale, "zh", 2) == 0) {
            return getZhWinMethods();
        }
#endif
    }
    return NULL;
}
