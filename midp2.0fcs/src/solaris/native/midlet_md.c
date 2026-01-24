/*
 * @(#)midlet_md.c	1.5 02/10/02 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

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
    char *execargs[3];
    pid_t pid;

    execargs[0] = getInternalProp(PLATFORM_REQUEST_KEY);
    if (execargs[0] == NULL) {
        puts("PlatformRequest is not configured.");
        return;
    }

    execargs[1] = pszUrl;
    execargs[2] = NULL;

    pid = fork();
    if (pid == 0) {
	/*
         * Child process, can throw an exception from this process.
         *  just display the error to the developer.
         */
	if (execvp(execargs[0], execargs) == -1) {
            puts("Spawning a handler process failed. "
                 "Check the platformRequest configuration.");
            _exit(0);
	}
    } else if (pid == -1){
	/* Error */
	perror("fork failed for platformRequest");
    }
}



