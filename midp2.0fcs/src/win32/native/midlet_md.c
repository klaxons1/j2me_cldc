/*
 * @(#)midlet_md.c	1.6 02/10/02 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <windows.h>

#include "midpMalloc.h"
#include "configuration.h"
#include "midlet.h"

static char* PLATFORM_REQUEST_KEY =
    "com.sun.midp.midlet.platformRequestCommand";

/*=========================================================================
 * FUNCTION:      plaformRequest()
 * OVERVIEW:      Append the given URL ASCII string and "com.sun.midp.midlet.
 *                <platform>RequestCommand" config value into a native
 *                command and spawn that command.
 *=======================================================================*/
void platformRequest(char* pszUrl) {
    char* pszRequest;
    char* pszCommandLine;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    pszRequest = getInternalProp(PLATFORM_REQUEST_KEY);
    if (pszRequest == NULL) {
        puts("PlatformRequest is not configured.");
        return;
    }

    /* leave room for a space and zero terminator */
    pszCommandLine = (char*)midpMalloc(strlen(pszRequest) +
                     strlen(pszUrl) + 2);
    if (pszCommandLine == NULL) {
        puts("PlatformRequest ran out of memory.");
        return;
    }

    strcpy(pszCommandLine, pszRequest);
    strcat(pszCommandLine, " ");
    strcat(pszCommandLine, pszUrl);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    /*
     * do not inherit handles
     */
    if (CreateProcess(NULL, pszCommandLine, NULL, NULL, FALSE, 0,
		      NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        puts("Spawning a handler process failed. "
             "Check the platformRequest configuration.");
    }

    midpFree(pszCommandLine);   
}
