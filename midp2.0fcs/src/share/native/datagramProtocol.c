/*
 * @(#)datagramProtocol.c	1.50 02/08/06 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking (Generic Connection framework)
 * FILE:      datagramProtocol.c
 * OVERVIEW:  This file provides a default implementation of the native
 *            functions that are needed for supporting the "datagram:"
 *            Generic Connection protocols.
 * AUTHOR:    Nik Shaylor
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <kni.h>
#include <stdio.h>

#include <midpMalloc.h>
#include "socketProtocol.h"
#include "datagramProtocol.h"
#include "pushregistry.h"
#include "defaultLCDUI.h"

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

#if INCLUDEDEBUGCODE
#define NDEBUG0(fmt)                         if (tracenetworking) { fprintf(stdout, fmt);                         }
#define NDEBUG1(fmt, p1)                     if (tracenetworking) { fprintf(stdout, fmt, p1);                     }
#define NDEBUG2(fmt, p1, p2)                 if (tracenetworking) { fprintf(stdout, fmt, p1, p2);                 }
#define NDEBUG3(fmt, p1, p2, p3)             if (tracenetworking) { fprintf(stdout, fmt, p1, p2, p3);             }
#define NDEBUG4(fmt, p1, p2, p3, p4)         if (tracenetworking) { fprintf(stdout, fmt, p1, p2, p3, p4);         }
#define NDEBUG5(fmt, p1, p2, p3, p4, p5)     if (tracenetworking) { fprintf(stdout, fmt, p1, p2, p3, p4, p5);     }
#define NDEBUG6(fmt, p1, p2, p3, p4, p5, p6) if (tracenetworking) { fprintf(stdout, fmt, p1, p2, p3, p4, p5, p6); }
#else
#define NDEBUG0(fmt)                         /**/
#define NDEBUG1(fmt, p1)                     /**/
#define NDEBUG2(fmt, p1, p2)                 /**/
#define NDEBUG3(fmt, p1, p2, p3)             /**/
#define NDEBUG4(fmt, p1, p2, p3, p4)         /**/
#define NDEBUG5(fmt, p1, p2, p3, p4, p5)     /**/
#define NDEBUG6(fmt, p1, p2, p3, p4, p5, p6) /**/
#endif

#ifndef MAX_HOST_LENGTH
#define MAX_HOST_LENGTH 256
#endif /* MAX_HOST_LENGTH */

static int socketFieldIDsObtained = 0;
static jfieldID socketHandleFieldID;

/*=========================================================================
 * Protocol methods
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      setSocketHandle()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Set the contents of the handle field
 * INTERFACE:
 *   parameters:  instance, value
 *   returns:     <nothing>
 *=======================================================================*/

static void setSocketHandle(int handle)
{
    KNI_StartHandles(2);
    KNI_DeclareHandle(socketObject);
    KNI_GetThisPointer(socketObject);

    if (!socketFieldIDsObtained) {
        KNI_DeclareHandle(socketClass);
        KNI_GetObjectClass(socketObject, socketClass);
        socketHandleFieldID = KNI_GetFieldID(socketClass, "handle","I");
        socketFieldIDsObtained = 1;
    }

    KNI_SetIntField(socketObject, socketHandleFieldID, (jint)handle);

    KNI_EndHandles();

    NDEBUG1("setSocketHandle handle=%d\n", handle);
}

/*=========================================================================
 * function:      getSocketHandle()
 * TYPE:          private instance-level operation
 * OVERVIEW:      Get the contents of the handle field
 * INTERFACE:
 *   parameters:  instance
 *   returns:     value
 *=======================================================================*/

static int getSocketHandle()
{
    int handle = 0;
    
    if (!socketFieldIDsObtained) {
        NDEBUG0("getSocketHandle, handle was not set.");
        return -1;
    }

    KNI_StartHandles(1);
    KNI_DeclareHandle(socketObject);
    KNI_GetThisPointer(socketObject);
    handle = (int)KNI_GetIntField(socketObject, socketHandleFieldID);

    KNI_EndHandles();

    NDEBUG1("getSocketHandle handle=%d\n", handle);

    return handle;
}

/*=========================================================================
 * FUNCTION:      open0()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a datagram socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, port
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_datagram_Protocol_open0()
{
    int port;
    char* pszException = NULL;
    int handle;

    char *szStore = NULL;
    int storeLen;

    KNI_StartHandles(1);
    KNI_DeclareHandle(storage);

    port = (int)KNI_GetParameterAsInt(1);

    /* Get the storage name string. */
    KNI_GetParameterAsObject(2, storage);
    storeLen = KNI_GetArrayLength(storage);
    if ((szStore = midpMalloc(storeLen)) != NULL) {
	KNI_GetRawArrayRegion(storage, 0, storeLen, (jbyte*)szStore);

	if ((handle = pushcheckout("datagram", port, szStore)) == -1){
	    handle = prim_com_sun_midp_io_j2me_datagram_Protocol_open0(port,
								       &pszException);
	} else if (handle == -2) {
	     KNI_ThrowNew("java/io/IOException", "already in use");
	}

	midpFree(szStore);
    } else {
	KNI_ThrowNew("java/lang/OutOfMemoryError", "connection");
    }
    KNI_EndHandles();  

    NDEBUG2("datagram::open0 port=%d handle=%d\n", port, handle);

    if (pszException != NULL) {
        NDEBUG2("datagram::open error = %d exception = %s\n",
                netError(), pszException);

        KNI_ThrowNew(pszException, "");
    } else {

#ifndef BLOCKING_NET_IO
        /* For non-blocking systems only */
        prim_com_sun_midp_io_j2me_datagram_Protocol_setNonBlocking(handle);
#endif

	setSocketHandle(handle);
    }

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      getMaximumLength0()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the datagram max length
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     int
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_datagram_Protocol_getMaximumLength0()
{
    int len;
    int fd = getSocketHandle();

    len = prim_com_sun_midp_io_j2me_datagram_Protocol_getMaximumLength(fd);

    NDEBUG2("datagram::getMaximumLength len=%d %d\n", len,fd);

    KNI_ReturnInt((jint)len);
}

/*=========================================================================
 * FUNCTION:      getNominalLength0()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the datagram nominal length
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     int
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT
    Java_com_sun_midp_io_j2me_datagram_Protocol_getNominalLength0()
{
    int len;
    int fd = getSocketHandle();

    len = prim_com_sun_midp_io_j2me_datagram_Protocol_getNominalLength(fd);

    NDEBUG2("datagram::getNominalLength len=%d %d\n", len, fd);

    KNI_ReturnInt((jint)len);
}

/*=========================================================================
 * FUNCTION:      static getHostByAddr()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translates an ip address into a host name
 * INTERFACE (operand stack manipulation):
 *   parameters:  int, byte[]
 *   returns:     KNI_RETURNTYPE_OBJECT
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_io_j2me_datagram_Protocol_getHostByAddr()
{
    int ipn;
    char host[MAX_HOST_LENGTH];

    ipn = (int)KNI_GetParameterAsInt(1);

    prim_com_sun_midp_io_j2me_datagram_Protocol_getHostByAddr(ipn, host);

    NDEBUG2("datagram::getHostByAddr ipn='%x' host='%s'\n", ipn, host);

    KNI_StartHandles(1);
    KNI_DeclareHandle(hostString);
    KNI_NewStringUTF(host, hostString);
    KNI_EndHandlesAndReturnObject(hostString);
}

/*=========================================================================
 * FUNCTION:      static getIpNumber()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translate a host name into an ip address
 * INTERFACE (operand stack manipulation):
 *   parameters:  byte[]
 *   returns:     int
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_datagram_Protocol_getIpNumber()
{
    char host[MAX_HOST_LENGTH];
    int ipn = -1;
    int hostLength;

    KNI_StartHandles(1);

    KNI_DeclareHandle(hostObject);
    KNI_GetParameterAsObject(1, hostObject);

    hostLength = KNI_GetArrayLength(hostObject);
    if (hostLength > MAX_HOST_LENGTH) {
        KNI_ThrowNew("java/lang/IllegalArugmentException",
                     "Host too long");
    } else {
        
        KNI_GetRawArrayRegion(hostObject, 0, hostLength, (jbyte*)host);
        host[hostLength] = '\0';

	ipn = prim_com_sun_midp_io_j2me_socket_Protocol_getIpNumber(host);
	NDEBUG2("datagram::getIpNumber host='%s' ipn=%x\n", host, ipn);
    }

    KNI_EndHandles();
    KNI_ReturnInt((jint)ipn);
}

/*=========================================================================
 * FUNCTION:      send0()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Send a datagram
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, iaddr, port, buffer, offset, length
 *   returns:     length
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_datagram_Protocol_send0()
{
    char* pBuffer;
    int length;
    int offset;
    int handle;
    int ipAddress;
    int port;
    int res;

    length = (int)KNI_GetParameterAsInt(5);
    pBuffer = (char*)midpMalloc(length);
    if (pBuffer == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "UDP send buffer");
        KNI_ReturnInt(0);
    }

    offset = (int)KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);

    KNI_DeclareHandle(bufferObject);
    KNI_GetParameterAsObject(3, bufferObject);
    KNI_GetRawArrayRegion(bufferObject, offset, length, (jbyte*)pBuffer);

    KNI_EndHandles();

    port = (int)KNI_GetParameterAsInt(2);

    ipAddress = (int)KNI_GetParameterAsInt(1);

    handle = getSocketHandle();

    NDEBUG5("datagram::send0 o=%d l=%d p=%d ip=%x fd=%d\n",
             offset, length, port, ipAddress, handle);

    res = prim_com_sun_midp_io_j2me_datagram_Protocol_send0(handle,
              ipAddress, port, pBuffer, length);

    netIndicatorCount++;

    midpFree(pBuffer);

    NDEBUG1("datagram::send0 res=%d\n", res);

    if (res < 0) {
        NDEBUG1("datagram::send0 ne=%d\n", netError());

        if (res == IO_INTERRUPTED) {
            KNI_ThrowNew("java/io/InterruptedIOException", "");
        } else {
            char temp[40];
            
            sprintf(temp, "error %d during UDP send ", netError());
            KNI_ThrowNew("java/io/IOException", temp);
        }
    }

    KNI_ReturnInt((jint)res);
}

/*=========================================================================
 * FUNCTION:      receive0()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Receive a datagram
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, buffer, offset, length
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_LONG
Java_com_sun_midp_io_j2me_datagram_Protocol_receive0()
{
    char* pBuffer;
    int length;
    int offset;
    int handle;
    int bytesReceived;
    int ipAddress;
    int port;
    jlong lres = 0;

    length = (int)KNI_GetParameterAsInt(3);
    pBuffer = (char*)midpMalloc(length);
    if (pBuffer == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "UDP receive buffer");
        KNI_ReturnLong(lres);
    }

    offset = (int)KNI_GetParameterAsInt(2);

    handle = getSocketHandle();
    NDEBUG3("datagram::receive0 o=%d l=%d fd=%ld\n", offset, length, handle);

    /* Check the push cache for a waiting datagram. */
    if ((bytesReceived = pusheddatagram(
	handle, &ipAddress, &port, pBuffer, length)) < 0 ) {
	/* Otherwise read the datagram directly from the socket. */
	bytesReceived = prim_com_sun_midp_io_j2me_datagram_Protocol_receive0(
                   handle, &ipAddress, &port, pBuffer, length);

    }
    NDEBUG1("datagram::receive0 bytesReceived=%d\n", bytesReceived);

    if (bytesReceived < 0) {
        NDEBUG1("datagram::receive0 ne=%d\n", (long)netError());
        if (bytesReceived == IO_INTERRUPTED) {
            KNI_ThrowNew("java/io/InterruptedIOException", "");
        } else if (bytesReceived != IO_WOULDBLOCK) {
            char temp[40];
            
            sprintf(temp, "error %d during UDP receive ", netError());
            KNI_ThrowNew("java/io/IOException", temp);
        }
    } else {
        NDEBUG2("datagram::receive0 ip=%x port = %d\n", ipAddress, port);

        KNI_StartHandles(1);

        KNI_DeclareHandle(bufferObject);
        KNI_GetParameterAsObject(1, bufferObject);
        KNI_SetRawArrayRegion(bufferObject, offset, bytesReceived,
                              (jbyte*)pBuffer);

        KNI_EndHandles();

        lres = (((jlong)ipAddress) << 32) + (unsigned)((port & 0xFFFF) << 16) +
            (bytesReceived & 0xFFFF);
	netIndicatorCount++;
    }

    midpFree(pBuffer);
    KNI_ReturnLong(lres);
}

/*=========================================================================
 * FUNCTION:      close0()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a datagram socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_datagram_Protocol_close0()
{
    int handle = getSocketHandle();

    NDEBUG1("datagram::close handle=%d\n", handle);
    
    if (pushcheckin(handle) == -1) {
	if (prim_com_sun_midp_io_j2me_socket_Protocol_close0(handle) < 0) {
	    NDEBUG1("datagram::close error=%d\n", netError());
	    KNI_ThrowNew("java/io/IOException", "");
	}
    }

    /* Ensure we don't accidentally close the handle more than once */
    setSocketHandle(-1);

    KNI_ReturnVoid();
}

static void
datagramCleanup(jobject datagram)
{ 
    int handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(clazz);

    KNI_GetObjectClass(datagram, clazz);
    /* We need to call KNI_GetFieldID(); socketHandleFieldID may not
     * have been set before we cleanup.
     */
    handle = KNI_GetIntField(datagram, KNI_GetFieldID(clazz, "handle", "I"));

    if ((handle != -1) && (pushcheckin(handle) == -1)) {
	prim_com_sun_midp_io_j2me_socket_Protocol_close0(handle);

        /* Ensure we don't accidentally close the handle more than once */
        KNI_SetIntField(datagram, KNI_GetFieldID(clazz, "handle", "I"), -1);
    }

    KNI_EndHandles();
}

/*=========================================================================
 * FUNCTION:      registerCleanup
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_datagram_Protocol_registerCleanup()
{
    KNI_StartHandles(1);
    KNI_DeclareHandle(instance);
    KNI_GetThisPointer(instance);

    KNI_registerCleanup(instance, datagramCleanup);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      finalize
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_datagram_Protocol_finalize()
{
    KNI_StartHandles(1);
    KNI_DeclareHandle(instance);
    KNI_GetThisPointer(instance);

    datagramCleanup(instance);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      getHost0();
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the host address for the requested endpoint.
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     IP number as a formatted string
 *
 * This routine formats the IP address as a String for the 
 * local end point of the connection.
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_OBJECT 
Java_com_sun_midp_io_j2me_datagram_Protocol_getHost0()
{
    char *          value;

    KNI_StartHandles(1);
    KNI_DeclareHandle(result);

    value = getLocalIPAddressAsString_md();

    if (value != NULL) {
	KNI_NewStringUTF(value, result);
    } else {
	KNI_ReleaseHandle(result);
    }

    KNI_EndHandlesAndReturnObject(result); 
}

/*=========================================================================
 * FUNCTION:      getPort0();
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the port number for the requested endpoint.
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     IP number as a formatted string
 *
 * This routine returns the port number for the local
 * end point for the current socket connection.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_datagram_Protocol_getPort0()
{
    long            fd = getSocketHandle();
    long            port;

    port = prim_com_sun_midp_io_j2me_socket_Protocol_getport1(fd, 1);

    KNI_ReturnInt((jint)port);
}
