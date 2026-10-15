/*
 * @(#)main.c	1.114 02/10/02 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * Virtual Machine
 *=========================================================================
 * SYSTEM:    MIDP native
 * SUBSYSTEM: Main program
 * FILE:      main.c
 * OVERVIEW:  Main program & system initialization
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#define midp_dup2 _dup2
#define midp_fileno _fileno
#else
#include <unistd.h>
#define midp_dup2 dup2
#define midp_fileno fileno
#endif

#include <kni.h>

#include "midpMalloc.h"
#include "commandState.h"

extern long RequestedHeapSize;

/*
 * Debug logging: everything the C code or the Java code writes to
 * stdout/stderr is mirrored to ./midp-debug.log so that runtime
 * failures can be diagnosed on machines without a visible console.
 */
static void
midpOpenDebugLog(void) {
    static FILE* logf = NULL;
    if (logf != NULL) {
        return;
    }
    logf = fopen("midp-debug.log", "w");
    if (logf == NULL) {
        return;
    }
    setvbuf(logf, NULL, _IONBF, 0);
    midp_dup2(midp_fileno(logf), 1);
    midp_dup2(midp_fileno(logf), 2);
}

static const char*
midpCommandName(int cmd) {
    switch (cmd) {
    case EXIT:        return "EXIT";
    case MANAGE:      return "MANAGE";
    case INSTALL_RUN: return "INSTALL_RUN";
    case LIST:        return "LIST";
    case RUN_CLASS:   return "RUN_CLASS";
    case STORAGE_NAMES: return "STORAGE_NAMES";
    default:          return "OTHER";
    }
}


/*=========================================================================
 * global variables
 *=======================================================================*/


/*=========================================================================
 * local variables
 *=======================================================================*/
#define MIDP_MAIN "com.sun.midp.main.Main"

#ifndef MAXIMUMHEAPSIZE
#define MAXIMUMHEAPSIZE (1000L * 1000L)
#endif

/* midp_home saved for the startup banner (freed earlier) */
static char midpHomeLog[1024];


/*=========================================================================
 * function prottypes
 *=======================================================================*/
extern void  midpPrintUsage(char *progname);
extern void  midpParseArgs(int argc, char* argv[]);
extern int   midpSetClassPath(char* storageName, char** userClassPath);
extern char* midpFixMidpHome(char *cmd);
extern void  midpAddBaseClassPath(char* midp_home);


/*=========================================================================
 * FUNCTION:      main
 * OVERVIEW:      The main method. There are three modes that MIDP can
 *                be run:
 *
 *                   - TCK       driven by loading descriptors from
 *                                a url
 *                   - debug     classpath and MIDlet class on the
 *                                command line
 *                   - resident  install, remove, list, run apps
 * INTERFACE:
 *   parameters:  argc:  The total number of arguments
 *                argv:  An array of 'C' strings containing the arguments
 *   returns:     int:   The status of the last VM invocation.
 *=======================================================================*/
int
main(int argc, char* argv[]) {
    char  midpMain[sizeof(MIDP_MAIN)];
    char* vmArg[1];
    int   vmStatus;
    char* pszError;
    char* midp_home;

    char* stdbuff = NULL;
    char* errbuff = NULL;
    int   bufsiz  = 0;
    int   bufmode = _IONBF;

    midpInitializeMemory(-1);

    /* Set stdout and stderr to line buffered mode on Unix
     * Line buffered mode does not exists for MS-DOS (Windows console)
     * applications (See MSVC documantation). Set no buffering on Winows
     */
#ifndef WIN32
    bufmode = _IOLBF;
    bufsiz  = BUFSIZ;

    stdbuff = (char*)midpMalloc(BUFSIZ);
    errbuff = (char*)midpMalloc(BUFSIZ);

    if (stdbuff == NULL || errbuff == NULL) {
	perror("Can not allocate stream buffers\n");
	return -1;
    }
#endif

    setvbuf(stdout, stdbuff, bufmode, bufsiz);
    setvbuf(stderr, errbuff, bufmode, bufsiz);

    /* mirror all output to ./midp-debug.log from the very start */
    midpOpenDebugLog();

    /*
     * Initialize the configuration property file mechanism first,
     * so command line overrides can be applied on a per
     * property basis
     */
    midp_home = midpFixMidpHome(argv[0]);
    initializeConfig(midp_home);
    initializeStorage(midp_home);
    midpAddBaseClassPath(midp_home);
    {
        int hl = 0;
        while (midp_home[hl] != '\0' && hl < (int)sizeof(midpHomeLog) - 1) {
            midpHomeLog[hl] = midp_home[hl];
            hl++;
        }
        midpHomeLog[hl] = '\0';
    }
    midpFree(midp_home);
    
    /*
     * Default Java heap size. The original build initialized
     * RequestedHeapSize with MAXIMUMHEAPSIZE; the -heapsize command
     * line argument (parsed below) overrides this. Without an
     * explicit default the VM would start with a zero-sized heap.
     */
    RequestedHeapSize = MAXIMUMHEAPSIZE;

    /*
     * Now parse the command line arguments targetted at the
     * native implementation. The rest will be handled by the
     * Java AMS Main routine.
     */
    midpParseArgs(argc, argv);

    {
        printf("\n==== midp debug log ====\n");
        printf("midp_home: %s\n", midpHomeLog);
        printf("command: %s (%d)\n",
               midpCommandName(MidpCommandState.initialCommand),
               MidpCommandState.initialCommand);
        printf("midletClassName: %s\n",
               MidpCommandState.midletClassName != NULL ?
               MidpCommandState.midletClassName : "(null)");
        printf("descriptorName: %s\n",
               MidpCommandState.descriptorName != NULL ?
               MidpCommandState.descriptorName : "(null)");
        printf("suiteStorageName: %s\n",
               MidpCommandState.suiteStorageName != NULL ?
               MidpCommandState.suiteStorageName : "(null)");
        printf("heap: %ld bytes\n", (long)RequestedHeapSize);
    }

#ifdef INCLUDE_I18N
    initLocaleMethod();
#endif

    /* The VM writes in our string, so we cannot give it a static one */
    strcpy(midpMain, MIDP_MAIN);
    vmArg[0] = midpMain;

    /* 
     * Initialize the graphical skin and associated window system
     * properties. This is done out side the KVM start up sequence
     * to avoid user interface flashing bewteen VM invocations.
     */
    if (MidpCommandState.initialCommand != INSTALL &&
            MidpCommandState.initialCommand != LIST &&
            MidpCommandState.initialCommand != STORAGE_NAMES &&
            MidpCommandState.initialCommand != REMOVE) {
        /* only want the graphical skin to display when it makes sense */
        InitializeWindowSystem();
    }

    /*
     * Initialize the asynchronous push registry before entering
     * the main VM launching loop. The Push Registry remains 
     * watching for inbound network connections across MIDlet
     * invocations.
     */
    pushopen();

    do {
        if (MidpCommandState.nextCommand == RUN) {
            midpSetClassPath(MidpCommandState.suiteStorageName, &UserClassPath);
        } else {
            midpSetClassPath(NULL, &UserClassPath);
        }

        printf("starting VM: classpath=[%s] nextCommand=%s (%d)\n",
               UserClassPath != NULL ? UserClassPath : "(null)",
               midpCommandName(MidpCommandState.nextCommand),
               MidpCommandState.nextCommand);
        fflush(stdout);

        /*
         * When the "power button" on the phone is pressed, the VM is exited
         * abrutly with a status of zero. But our Java Main returns a
         * specific positive code, so we can tell if the "power button" on
         * the phone pressed.
         */
        vmStatus = StartJVM(1, vmArg);

        printf("VM exited with status %d\n", vmStatus);
        fflush(stdout);

        /* clean up the temp file, ignore any errors */
        storageDeleteFile(&pszError, "./appdb/installer.tmp");

        if (vmStatus != MAIN_EXIT) {
            MidpCommandState.status = vmStatus;

            if (MidpCommandState.nextCommand == RUN &&
                    MidpCommandState.runOnce) {
                /* clean up what we were running */
                MidpCommandState.nextCommand = REMOVE;
            } else {
                /* default behaviour */
                MidpCommandState.nextCommand = EXIT;
            }

            /* special cases for not exiting */
            if (vmStatus != 0) {
                /* power button not pressed */
                if (MidpCommandState.autotest) {
                    /* get the next test */
                    MidpCommandState.nextCommand = INSTALL;
                } else if (MidpCommandState.initialCommand == MANAGE &&
                           MidpCommandState.nextCommand != MANAGE) {
                    /* get back to the manager */
                    MidpCommandState.nextCommand = MANAGE;
                }
            }
        }


        if (MidpCommandState.nextCommand == MANAGE) {
            /* check in a unprocessed connections */
            pushcheckinall();
        }
    } while (MidpCommandState.nextCommand != EXIT);

    pushclose();

    /*
     * When all KVM invocations have completed, release the
     * window system resources. 
     */
    FinalizeWindowSystem();

#ifdef INCLUDE_I18N
    /* Free native resources to support different locales */
    finalizeLocaleMethod();
#endif

    /* Cleanup native code resources on exit */
    finalizeConfig();
    finalizeStorage();

#ifndef WIN32
    /* Cleanup after ourselves */
    fclose(stdout);
    fclose(stderr);

    midpFree(stdbuff);
    midpFree(errbuff);
#endif

    if (UserClassPath != NULL) midpFree(UserClassPath);

    finalizeCommandState();

    midpFinalizeMemory();

    return MidpCommandState.status;
}



