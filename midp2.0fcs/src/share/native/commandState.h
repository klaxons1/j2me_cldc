/*
 * @(#)commandState.h	1.14 02/08/29 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _COMMAND_STATE_H
#define _COMMAND_STATE_H

/* MUST MATCH the values in Command.java */
#define MAIN_EXIT (2001)

typedef enum {
    EXIT        = 1,                                                /* exit */
    MANAGE      = 2,                      /* Manage MIDlet suites Graphicly */
    INSTALL     = 3,                             /* Install an MIDlet suite */
    INSTALL_RUN = 4,                      /* Install and run a MIDlet suite */
    RUN         = 5,                                    /* Run MIDlet suite */
    REMOVE      = 6,                                 /* Remove MIDlet suite */
    LIST        = 7,                        /* List installed MIDlet suites */
    RUN_CLASS   = 8,                       /* run midlet from the classpath */
    STORAGE_NAMES = 9,         /* Only list the storage names of the suites */
} jam_state;

typedef struct _COMMAND_STATE {
    int status;
    int initialCommand;
    int nextCommand;
    int /*boolean*/ forceOverwrite;
    char* suiteURL;
    char* suiteStorageName;
    int /*boolean*/ runOnce;
    int /*boolean*/ autotest;
    char* descriptorName;
    char* midletName;
    char* midletClassName;
    int /*boolean*/ logoDisplayed;
    char* runExceptionMessage;
    char* securityDomain;
    char* midletNumber;
    int /*boolean*/ removeRMS;
} COMMAND_STATE;

extern COMMAND_STATE MidpCommandState;

void finalizeCommandState(void);

#endif /* _COMMAND_STATE_H */
