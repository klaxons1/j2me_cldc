/*
 * @(#)midlet.c	1.5 02/07/24 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include "kni.h"
#include "midpMalloc.h"
#include "midlet.h"

/*
 *   boolean platformRequest(String URL)
 *
 *   Pass on the URL to the native browser.
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
        Java_com_sun_midp_midlet_MIDletState_platformRequest() {
    jsize urlLen;
    char* pszUrl;
    jchar* temp;
    int i;
    KNI_StartHandles(1);
    KNI_DeclareHandle(urlObj);

    KNI_GetParameterAsObject(1, urlObj);
    if (!KNI_IsNullHandle(urlObj)) {
        urlLen = KNI_GetStringLength(urlObj);
        if (urlLen > 0) {
            pszUrl = (char*)midpMalloc(urlLen * sizeof (jchar));
            if (pszUrl != NULL) {
                temp = (jchar*)pszUrl;
                KNI_GetStringRegion(urlObj, 0, urlLen, (jchar*)temp);

                /* simply convert the unicode by stripping the high byte */
                for (i = 0; i < urlLen; i++) {
                    pszUrl[i] = (char)temp[i];
                }

                pszUrl[urlLen] = 0;
                platformRequest(pszUrl);
                midpFree(pszUrl);
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(KNI_FALSE);
}


