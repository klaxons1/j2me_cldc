/*
 * @(#)midpServices.h	1.14 02/09/11 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _MIDPSERVICES
#define _MIDPSERVICES

#include <kni.h>
#include <midpServices_md.h>

/*=========================================================================
 * Event handling
 *=======================================================================*/

/* The type of event a native thread is waiting for */
enum midp_EventType {
    uiEvent = 1,
    networkEvent = 2,
};

/* Structure to be passed back to the VM */
struct EventInfo_struct {
    enum midp_EventType type;
};
typedef struct EventInfo_struct midp_EventInfo;

/* Initialize event sub-system */
void InitializeEvents();

/* Signal an event is waiting to be processed */
void midp_signalEvent();

/* Store a VM event */
void midp_storeVMEvent(jint type, int argCount, ...);

/* Read the next word (4-bytes) from the event queue */
jint midp_getNextEventWord(void);

/* Check if there is an event waiting in the event queue */
jboolean midp_hasStoredEvent(void);


/*=========================================================================
 * VM helper functions
 *=======================================================================*/

/* Terminate the VM with the specified status code */
void midp_exitVM(int status);

/* Determine if the VM has been stopped */
jboolean midp_isVmStopped(void);

/* Parse VM arguments */
jint midp_parseVmArg(jint currentArg, jint argc, char* argv[]);

/* Print VM usage text */
void midp_printVmUsage(void);

/* Signal MIDP should be shutdown */
void midp_shutdown(void);

/* Determine if MIDP has been stopped */
jboolean midp_isMidpStopped(void);

/* Get the current time in millis */
jlong midp_getCurrentTime(void);


/*=========================================================================
 * Resource helper functions
 *=======================================================================*/

/* Open a resource file */
int midp_openResourceFile(char* resourceName, int* size);

/* Close a previously opened resource file */
void midp_closeResourceFile(int fd);

/* Read a byte from a previously opened resource file */
int midp_readResourceFile(int fd);

/* Read 'size' bytes from a previously opened resource file */
int midp_readallResourceFile(int fd, int offset, int size, jobject* outArray, 
			     int pos);


/*=========================================================================
 * Compression helper functions
 *=======================================================================*/

/* Decompress a PNG image buffer */
jboolean midp_pngInflate(void *data, int compLen, unsigned char** outFileH, 
			 int decompLen);


/*=========================================================================
 * JAR helper functions
 *=======================================================================*/

/* Read a single file from a JAR file */
jboolean midp_readJarEntry(char* jarName, char* entryName, jobject* entry);


/*=========================================================================
 * Synchronization functions
 *=======================================================================*/

#if USE_MIDP_MONITOR
/* Initialize monitor */
void midp_monitorInitialize(void);

/* Finalize monitor */
void midp_monitorFinalize(void);

/* Enter a monitor */
void midp_monitorEnter(void);

/* Leave a monitor */
void midp_monitorExit(void);
#else
#define midp_monitorEnter()       /* no-op */
#define midp_monitorLeave()       /* no-op */
#define midp_monitorInitialize()  /* no-op */
#define midp_monitorFinalize()    /* no-op */
#endif /* USE_MIDP_MONITOR */

#endif /* _MIDPSERVICES */

