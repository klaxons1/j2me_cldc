/*
 * @(#)commProtocol.c	1.21 02/09/06 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking
 * FILE:      commProtocol.c
 * OVERVIEW:  Operations to support serial communication ports
 *            (native implementation of the 'comm:' protocol).
 *
 *            These functions are mapped onto platform-specific
 *            functions defined in Vm<Port>/src directories,
 *            where <Port> indicates the name of the target
 *            platform (e.g., Win, Unix, Pilot).
 * AUTHOR:    Stephen Flores, Nik Shaylor
 *=======================================================================*/

/*=======================================================================
 * Include files
 *=======================================================================*/

#include <kni.h>
#include <midpMalloc.h>
#include "commProtocol.h"

/*=======================================================================
 * Protocol implementation functions
 *=======================================================================*/

/*
 * static native void native_configurePort(int port, int baud, int flags)
 *     throws IOException;
 */
KNIEXPORT KNI_RETURNTYPE_VOID
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1configurePort() {

    long  flags      = (long)KNI_GetParameterAsInt(3);
    long  baud       = (long)KNI_GetParameterAsInt(2);
    long  port       = (long)KNI_GetParameterAsInt(1);
    char* pszError   = NULL;

    configurePort(&pszError, port, baud, flags);
    if (pszError != NULL) {
        KNI_ThrowNew("java/io/IOException", pszError);
        freePortError(pszError);
    }

    KNI_ReturnVoid();
}

/*
 * static native int native_open(int port, int baud, int flags)
 *     throws IOException;
 */
KNIEXPORT KNI_RETURNTYPE_INT
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1openByNumber() {

    long  flags      = (long)KNI_GetParameterAsInt(3);
    long  baud       = (long)KNI_GetParameterAsInt(2);
    long  port       = (long)KNI_GetParameterAsInt(1);
    long  hPort      = 0;
    char* pszError   = NULL;

    hPort = openPortByNumber(&pszError, port, baud, flags);
    if (pszError != NULL) {
        KNI_ThrowNew("java/io/IOException", pszError);
        freePortError(pszError);
    }

    KNI_ReturnInt((jint)hPort);
}

/*
 * static native int native_open(int port, int baud, int flags)
 *     throws IOException;
 */
KNIEXPORT KNI_RETURNTYPE_INT
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1openByName() {

    long   flags = (long)KNI_GetParameterAsInt(3);
    long   baud = (long)KNI_GetParameterAsInt(2);
    int    nameLen;
    char   szName[MAX_NAME_LEN * sizeof (jchar)];
    jchar* temp;
    long   hPort = 0;
    int    i;
    char*  pszError = NULL;

    KNI_StartHandles(1);

    KNI_DeclareHandle(nameObject);
    KNI_GetParameterAsObject(1, nameObject);
    nameLen = KNI_GetStringLength(nameObject);
    if (nameLen > MAX_NAME_LEN) {
        KNI_ThrowNew("java/lang/IllegalArgumentException",
                     "comm port name too long");
    } else {
        temp = (jchar*)szName;
        KNI_GetStringRegion(nameObject, 0, nameLen, temp);

        /* device names are in ASCII */
        for (i = 0; i < nameLen; i++) {
            szName[i] = (char)temp[i];
        }

        szName[nameLen] = 0;
        hPort = openPortByName(&pszError, szName, baud, flags);
        if (pszError != NULL) {
            KNI_ThrowNew("java/io/IOException", pszError);
            freePortError(pszError);
        }
    }

    KNI_EndHandles();
    KNI_ReturnInt((jint)hPort);
}

/*
 * static native void native_close(int hPort) throws IOException;
 */
KNIEXPORT KNI_RETURNTYPE_VOID
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1close() {

    long hPort = (long)KNI_GetParameterAsInt(1);

    closePort(hPort);
    KNI_ReturnVoid();
}

static void
commCleanup(jobject comm)
{ 
    int handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(clazz);

    KNI_GetObjectClass(comm, clazz);
    handle = KNI_GetIntField(comm, KNI_GetFieldID(clazz, "handle", "I"));

    if (handle != -1) {
	closePort(handle);
    }

    KNI_EndHandles();
}

/*=========================================================================
 * FUNCTION:      registerCleanup
 * CLASS:         com.sun.midp.io.j2me.comm.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a serial port connection
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_comm_Protocol_registerCleanup()
{
    KNI_StartHandles(1);
    KNI_DeclareHandle(instance);
    KNI_GetThisPointer(instance);

    KNI_registerCleanup(instance, commCleanup);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      finalize
 * CLASS:         com.sun.midp.io.j2me.comm.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a serial port connection
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_comm_Protocol_finalize()
{
    KNI_StartHandles(1);
    KNI_DeclareHandle(instance);
    KNI_GetThisPointer(instance);

    commCleanup(instance);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*
 * native int native_readBytes(int hPort, byte b[], int off, int len)
 *     throws IOException;
 * This method must not block.
 */
KNIEXPORT KNI_RETURNTYPE_INT
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1readBytes() {

    long  length = (long)KNI_GetParameterAsInt(4);
    long  offset = (long)KNI_GetParameterAsInt(3);
    char* pBuffer;
    long  hPort  = (long)KNI_GetParameterAsInt(1);
    long  bytesRead = 0;
    char* pszError = NULL;

    pBuffer = (char*)midpMalloc(length);
    if (pBuffer == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "comm read buffer");
        KNI_ReturnInt(0);
    }

    bytesRead = readFromPort(&pszError, hPort, pBuffer, length);
    if (pszError != NULL) {
        KNI_ThrowNew("java/io/IOException", pszError);
        freePortError(pszError);
        midpFree(pBuffer);
        KNI_ReturnInt(0);
    }

    if (bytesRead > 0) {
        KNI_StartHandles(1);
	
        KNI_DeclareHandle(bufferObject);
        KNI_GetParameterAsObject(2, bufferObject);
        KNI_SetRawArrayRegion(bufferObject, offset, bytesRead,
                              (jbyte*)pBuffer);

        KNI_EndHandles();
    }

    midpFree(pBuffer);
    KNI_ReturnInt((jint)bytesRead);
}

/*
 * native int native_writeBytes(int hPort, byte b[], int off, int len)
 *     throws IOException;
 * This method must not block.
 */
KNIEXPORT KNI_RETURNTYPE_INT
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1writeBytes() {

    long  length = (long)KNI_GetParameterAsInt(4);
    long  offset = (long)KNI_GetParameterAsInt(3);
    char* pBuffer;
    long  hPort  = (long)KNI_GetParameterAsInt(1);
    char* pszError = NULL;
    int   bytesWritten = 0;

    pBuffer = (char*)midpMalloc(length);
    if (pBuffer == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "comm write buffer");
        KNI_ReturnInt(0);
    }

    KNI_StartHandles(1);
	
    KNI_DeclareHandle(bufferObject);
    KNI_GetParameterAsObject(2, bufferObject);
    KNI_GetRawArrayRegion(bufferObject, offset, length,
                              (jbyte*)pBuffer);

    KNI_EndHandles();

    bytesWritten = writeToPort(&pszError, hPort, pBuffer, length);
    if (pszError != NULL) {
        KNI_ThrowNew("java/io/IOException", pszError);
        freePortError(pszError);
    }

    midpFree(pBuffer);
    KNI_ReturnInt((jint)bytesWritten);
}


