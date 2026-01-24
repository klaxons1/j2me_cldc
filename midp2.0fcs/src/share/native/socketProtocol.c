/*
 * @(#)socketProtocol.c	1.63 02/09/04 @(#)
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
 * FILE:      socketProtocol.c
 * OVERVIEW:  This file provides a default implementation of the native
 *            functions that are needed for supporting the "socket:" and
 *            "serversocket:" Generic Connection protocols.
 * AUTHOR(s): Nik Shaylor, Efren Serra
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <kni.h>
#include <stdio.h>

#include <midpMalloc.h>
#include "socketProtocol.h"
#include "pushregistry.h"
#include "configuration.h"
#include "defaultLCDUI.h"
#ifdef UNIX
#include <unistd.h>
#else
#include <windows.h>
#ifdef GCC
#include <winsock.h>
#else
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif /* _WINSOCKAPI_ */
#endif /* GCC */
#endif /* UNIX */

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

/* The following flag allows the code for server
 * sockets to be compiled out if necessary.
 */
#ifndef NO_SERVER_SOCKETS
#define NO_SERVER_SOCKETS 0 /* Include server sockets by default */
#endif

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

#define INVALID_HANDLE -1

static int socketFieldIDsObtained = 0;
static jfieldID socketHandleFieldID;

static int serverSocketFieldIDsObtained = 0;
static jfieldID serverSocketHandleFieldID;

/* delcared in defaultLCDUI.h, used in socketProtocol.c
 * and datagramProtocol.c */
int netIndicatorCount = 0;

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
 *   parameters:  instance, field
 *   returns:     value
 *=======================================================================*/

static int getSocketHandle()
{
    int handle = 0;

    KNI_StartHandles(2);
    KNI_DeclareHandle(socketObject);
    KNI_GetThisPointer(socketObject);

    if (!socketFieldIDsObtained) {
        KNI_DeclareHandle(socketClass);
        KNI_GetObjectClass(socketObject, socketClass);
        socketHandleFieldID = KNI_GetFieldID(socketClass, "handle","I");
        socketFieldIDsObtained = 1;
    }
    handle = (int)KNI_GetIntField(socketObject, socketHandleFieldID);

    KNI_EndHandles();

    NDEBUG1("getSocketHandle handle=%d\n", handle);

    return handle;
}

/*=========================================================================
 * FUNCTION:      setServerSocketHandle()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Set the contents of the handle field
 * INTERFACE:
 *   parameters:  instance, value, field
 *   returns:     <nothing>
 *=======================================================================*/

static void setServerSocketHandle(long handle)
{
    KNI_StartHandles(2);
    KNI_DeclareHandle(serverSocketObject);
    KNI_GetThisPointer(serverSocketObject);

    if (!serverSocketFieldIDsObtained) {
        KNI_DeclareHandle(serverSocketClass);
        KNI_GetObjectClass(serverSocketObject, serverSocketClass);
        serverSocketHandleFieldID =
            KNI_GetFieldID(serverSocketClass, "handle","I");
        serverSocketFieldIDsObtained = 1;
    }

    KNI_SetIntField(serverSocketObject, serverSocketHandleFieldID, handle);

    KNI_EndHandles();
}

/*=========================================================================
 * function:      getServerSocketHandle()
 * TYPE:          private instance-level operation
 * OVERVIEW:      Get the contents of the handle field
 * INTERFACE:
 *   parameters:  instance, field
 *   returns:     value
 *=======================================================================*/

static long getServerSocketHandle()
{
    long handle = 0;
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(serverSocketObject);
    KNI_GetThisPointer(serverSocketObject);

    if (!serverSocketFieldIDsObtained) {
        KNI_DeclareHandle(serverSocketClass);
        KNI_GetObjectClass(serverSocketObject, serverSocketClass);
        serverSocketHandleFieldID =
            KNI_GetFieldID(serverSocketClass, "handle","I");
        serverSocketFieldIDsObtained = 1;
    }

    handle = (int)KNI_GetIntField(serverSocketObject,
                                  serverSocketHandleFieldID);

    KNI_EndHandles();

    NDEBUG1("getServerSocketHandle handle=%ld\n", handle);

    return handle;
}

/*=========================================================================
 * FUNCTION:      open0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, host, port
 *   returns:     none, sets the handle field of the object
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_open0()
{
    char host[MAX_HOST_LENGTH];
    int hostLength;
    int port;
    char* pszException = NULL;
    int handle = INVALID_HANDLE;

    NDEBUG0("socket::open0");

    KNI_StartHandles(1);
    KNI_DeclareHandle(hostObject);

    KNI_GetParameterAsObject(1, hostObject);
    hostLength = KNI_GetArrayLength(hostObject);
    if (hostLength > MAX_HOST_LENGTH) {
	setSocketHandle(INVALID_HANDLE);
        KNI_ThrowNew("java/lang/IllegalArugmentException",
                     "Host too long");
    } else {
        KNI_GetRawArrayRegion(hostObject, 0, hostLength, (jbyte*)host);
        host[hostLength] = '\0';

        NDEBUG1(" host='%s'", host);

        port = (int)KNI_GetParameterAsInt(2);

        NDEBUG1(" port=%d", port);

        /* This will block the KVM on slow wireless networks. */
        handle = prim_com_sun_midp_io_j2me_socket_Protocol_open0(host, port,
                                                             &pszException);
	netIndicatorCount++;

        NDEBUG1(" handle = %d", handle);

        if (pszException != NULL) {
            NDEBUG2(" error = %d exception = %s",
                    netError(), pszException);

	    setSocketHandle(INVALID_HANDLE);
            KNI_ThrowNew(pszException, "TCP open");
        } else {
#ifndef BLOCKING_NET_IO
            /* For non-blocking systems only */
            prim_com_sun_midp_io_j2me_socket_Protocol_setNonBlocking(handle);
#endif
	    setSocketHandle(handle);
        }
    }

    KNI_EndHandles();

    NDEBUG0("\n");
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      int read0(byte[], int, int)
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Read from a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, buffer, offset, length
 *   returns:     number of chars read or -1 if at EOF
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_read0()
{
    static char errorMsg[40];          /* Error message to send back to Java */
    char* pBuffer;
    int length;
    int offset;
    int handle;
    int bytesRead;

    length = (int)KNI_GetParameterAsInt(3);
    pBuffer = (char*)midpMalloc(length);
    if (pBuffer == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "TCP read buffer");
        KNI_ReturnInt(0);
    }

    offset = (int)KNI_GetParameterAsInt(2);

    handle = getSocketHandle();

    NDEBUG3("socket::read0 off=%d len=%d handle=%d\n", offset, length, handle);

    bytesRead = prim_com_sun_midp_io_j2me_socket_Protocol_read0(handle,
                    pBuffer, length);

    NDEBUG1("socket::read0 bytesRead=%d\n", bytesRead);

    if (bytesRead == IO_WOULDBLOCK) {
        /* no data ready, try again later */
        bytesRead = 0;
    } else if (bytesRead < 0) {
        NDEBUG1("socket::read0 ne=%d\n", netError());

        if (bytesRead == IO_INTERRUPTED) {
            KNI_ThrowNew("java/io/InterruptedIOException", "");
        } else {
            sprintf(errorMsg, "error %d during TCP read ", netError());
            KNI_ThrowNew("java/io/IOException", errorMsg);
        }
    } else if (bytesRead == 0) {
        /* end of stream */
        bytesRead = -1;
    } else {
	KNI_StartHandles(1);
	
	KNI_DeclareHandle(bufferObject);
	KNI_GetParameterAsObject(1, bufferObject);
	KNI_SetRawArrayRegion(bufferObject, offset, bytesRead,
                              (jbyte*)pBuffer);
	
	KNI_EndHandles();
	netIndicatorCount++;
    }

    midpFree(pBuffer);
    KNI_ReturnInt((jint)bytesRead);
}


/*=========================================================================
 * FUNCTION:      write0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Write to a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, buffer, offset, length
 *   returns:     the length written
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_write0()
{ 
    static char errorMsg[40];          /* Error message to send back to Java */
    char* pBuffer;
    int length;
    int offset;
    int handle;
    int bytesWritten;

    length = (int)KNI_GetParameterAsInt(3);
    pBuffer = (char*)midpMalloc(length);
    if (pBuffer == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "TCP write buffer");
        KNI_ReturnInt(0);
    }

    offset = (int)KNI_GetParameterAsInt(2);

    handle = getSocketHandle();

    NDEBUG3("socket::write0 o=%d l=%d fd=%d\n", offset, length, handle);

    KNI_StartHandles(1);

    KNI_DeclareHandle(bufferObject);
    KNI_GetParameterAsObject(1, bufferObject);
    KNI_GetRawArrayRegion(bufferObject, offset, length, (jbyte*)pBuffer);

    KNI_EndHandles();

    bytesWritten = prim_com_sun_midp_io_j2me_socket_Protocol_write0(handle,
                       pBuffer, length);

    netIndicatorCount++;

    midpFree(pBuffer);

    NDEBUG1("socket::write0 bytesWritten=%d\n", bytesWritten);

    if (bytesWritten < 0) {
        NDEBUG1("socket::write0 ne=%d\n", netError());

        if (bytesWritten == IO_INTERRUPTED) {
            KNI_ThrowNew("java/io/InterruptedIOException", "");
        } else {
            sprintf(errorMsg, "error %d during TCP write ", netError());
            KNI_ThrowNew("java/io/IOException", errorMsg);
        }
    }

    KNI_ReturnInt((jint)bytesWritten);
}


/*=========================================================================
 * FUNCTION:      available0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Return available length
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     length
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_available0()
{
    int handle = getSocketHandle();
    int bytesAvailable;

    bytesAvailable =
        prim_com_sun_midp_io_j2me_socket_Protocol_available0(handle);

    NDEBUG1("socket::available0 bytesAvailable=%d\n", bytesAvailable);

    KNI_ReturnInt((jint)bytesAvailable);
}


/*=========================================================================
 * FUNCTION:      shutdownOutput0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Shutdown the output side of a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_shutdownOutput0()
{
    int handle = getSocketHandle();

    NDEBUG1("socket::shutdownInput handle=%d\n", handle);
    
    prim_com_sun_midp_io_j2me_socket_Protocol_shutdownOutput0(handle);

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      close0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_close0()
{
    int handle = getSocketHandle();

    NDEBUG1("socket::close handle=%d\n", handle);
    
    /* Ensure we don't accidentally close the handle more than once */
    setSocketHandle(INVALID_HANDLE);

    if (prim_com_sun_midp_io_j2me_socket_Protocol_close0(handle) < 0) {
        NDEBUG1("socket::close error=%d\n", netError());
        KNI_ThrowNew("java/io/IOException", "");
    }

    KNI_ReturnVoid();
}

static void
socketCleanup(jobject socket)
{
    int handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(clazz);

    KNI_GetObjectClass(socket, clazz);
    /* We need to call KNI_GetFieldID(); socketHandleFieldID may not
     * have been set before we cleanup.
     */
    handle = KNI_GetIntField(socket, KNI_GetFieldID(clazz, "handle", "I"));

    NDEBUG1("socket::clean handle=%d\n", handle);
    if (handle != INVALID_HANDLE) {
        prim_com_sun_midp_io_j2me_socket_Protocol_close0(handle);
        /* Ensure we don't accidentally close the handle more than once */
        KNI_SetIntField(socket, KNI_GetFieldID(clazz, "handle", "I"), 
			INVALID_HANDLE);
    }

    KNI_EndHandles();
}

/*=========================================================================
 * FUNCTION:      registerCleanup
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_registerCleanup()
{
    KNI_StartHandles(1);
    KNI_DeclareHandle(instance);
    KNI_GetThisPointer(instance);

    KNI_registerCleanup(instance, socketCleanup);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      finalize
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_finalize()
{
    KNI_StartHandles(1);
    KNI_DeclareHandle(instance);
    KNI_GetThisPointer(instance);

    socketCleanup(instance);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      open()
 * CLASS:         com.sun.midp.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a listening TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, port
 *   returns:     none, sets the handle field of the object
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_serversocket_Socket_open0()
{
#if NO_SERVER_SOCKETS
    KNI_ThrowNew("java/lang/ClassNotFoundException", "");
#else
    int port;
    int handle;
    char* pszException = NULL;
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
	
	if ((handle = pushcheckout("socket", port, szStore)) == -1) {
	    handle = prim_com_sun_midp_io_j2me_serversocket_Protocol_open0(port,
									   &pszException);
	    netIndicatorCount++;
	    
	    if (pszException != NULL) {
		NDEBUG2("serversocket::open error = %d exception = %s\n",
			netError(), pszException);
		
		setServerSocketHandle(INVALID_HANDLE);
		KNI_ThrowNew(pszException, "ServerSocket Open");
	    } else {
		
#ifndef BLOCKING_NET_IO
		/* For non-blocking systems only */
		prim_com_sun_midp_io_j2me_socket_Protocol_setNonBlocking(handle);
#endif
		
		setServerSocketHandle(handle);
	    }
	} else if (handle == -2) {
	    setServerSocketHandle(INVALID_HANDLE);
	    KNI_ThrowNew("java/io/IOException", "already in use");
	} else {
	    setServerSocketHandle(handle);
	}

	midpFree(szStore);
    } else {
	setServerSocketHandle(INVALID_HANDLE);
	KNI_ThrowNew("java/lang/OutOfMemoryError", "connection");
    }
    KNI_EndHandles();  

    NDEBUG2("serversocket::open port = %d handle = %d\n", port, handle);

#endif /* NO_SERVER_SOCKETS */
    KNI_ReturnVoid();
}


/*=========================================================================
 * FUNCTION:      accept()
 * CLASS:         com.sun.midp.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Accept and open a connection
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     socket handle
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_serversocket_Socket_accept()
{
    int handle;
    int connectionHandle;

    handle = getServerSocketHandle();

    NDEBUG1("serversocket::accept handle=%d\n", handle);

    if ((connectionHandle = pushcheckoutaccept(handle)) == -1) {
	
	if (handle != INVALID_HANDLE) {
	    connectionHandle =
		prim_com_sun_midp_io_j2me_serversocket_Protocol_accept(handle);
	
	    NDEBUG1("serversocket::accept connection handle=%d\n", connectionHandle);
	}
    }

    if (connectionHandle < 0) {
        NDEBUG1("serversocket::accept error=%d\n", netError());

        if (connectionHandle != IO_WOULDBLOCK) {
            KNI_ThrowNew("java/io/IOException", "");
        }
    }
    KNI_ReturnInt((jint)connectionHandle);
}

/*=========================================================================
 * FUNCTION:      close0()
 * CLASS:         com.sun.midp.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a listening TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_serversocket_Socket_close0()
{
    int handle = getServerSocketHandle();

    /* Ensure we don't accidentally close the handle more than once */
    setServerSocketHandle(INVALID_HANDLE);
    
    NDEBUG1("serversocket::close handle=%d\n", handle);
    if (pushcheckin(handle) == -1){
	if (handle != INVALID_HANDLE ) {
	    if (prim_com_sun_midp_io_j2me_serversocket_Protocol_close(handle) < 0) {
		NDEBUG1("serversocket::close error=%d\n", netError());
		KNI_ThrowNew("java/io/IOException", "");
	    }
	}
    }

    KNI_ReturnVoid();
}


static void
serversocketCleanup(jobject serversocket)
{
    int handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(clazz);

    KNI_GetObjectClass(serversocket, clazz);
    /* We need to call KNI_GetFieldID(); serverSocketHandleFieldID may not
     * have been set before we cleanup.
     */
    handle = KNI_GetIntField(serversocket, 
                             KNI_GetFieldID(clazz, "handle", "I"));
    if ((handle != INVALID_HANDLE) && (pushcheckin(handle) == -1)) {
        prim_com_sun_midp_io_j2me_serversocket_Protocol_close(handle);
        /* Ensure we don't accidentally close the handle more than once */
        KNI_SetIntField(serversocket, 
                        KNI_GetFieldID(clazz, "handle", "I"), INVALID_HANDLE);
    }

    KNI_EndHandles();
}

/*=========================================================================
 * FUNCTION:      registerCleanup
 * CLASS:         com.sun.midp.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_serversocket_Socket_registerCleanup()
{
    KNI_StartHandles(1);
    KNI_DeclareHandle(instance);
    KNI_GetThisPointer(instance);

    KNI_registerCleanup(instance, serversocketCleanup);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      finalize
 * CLASS:         com.sun.midp.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_serversocket_Socket_finalize()
{
    KNI_StartHandles(1);
    KNI_DeclareHandle(instance);
    KNI_GetThisPointer(instance);

    serversocketCleanup(instance);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      initalizeInternal();
 * CLASS:         com.sun.midp.io.NetworkConnectionBase()
 * TYPE:          virtual native function
 * OVERVIEW:      Initialize the network
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *
 * This is a static initializer in NetworkConnectionBase, the parent of
 * all ..../Protocol classes that access the network.
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
    Java_com_sun_midp_io_NetworkConnectionBase_initializeInternal()
{
    networkInit();
    KNI_ReturnVoid();
}
/*
 * Wrapper to the system gethostname functionality.
 * Make sure the network is properly initialized.
 * Complete the transaction by recording the system
 * host name in the property values.
 */
char *getLocalHostName() { 
    char host[MAX_HOST_LENGTH];

    networkInit();
    if (gethostname(host, MAX_HOST_LENGTH) == 0) {
	setSystemProp("microedition.hostname", host);
    }
    return getSystemProp("microedition.hostname");
}

/*=========================================================================
 * FUNCTION:      getHost0();
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the host address for the requested endpoint.
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, local
 *   returns:     IP number as a formatted string
 *
 * This routine formats the IP address as a String for either the 
 * local or remote end point of the connection.
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_OBJECT 
Java_com_sun_midp_io_j2me_socket_Protocol_getHost0()
{
    long            local;
    long            fd = getSocketHandle();
    char *          value;

    KNI_StartHandles(1);
    KNI_DeclareHandle(result);
    local = (int)KNI_GetParameterAsBoolean(1);

    value = prim_com_sun_midp_io_j2me_socket_Protocol_getipnumber1(fd, local);

    if (value != NULL) {
	KNI_NewStringUTF(value, result);
    } else {
	KNI_ReleaseHandle(result);
    }

    KNI_EndHandlesAndReturnObject(result); 
}

/*=========================================================================
 * FUNCTION:      getPort0();
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the port number for the requested endpoint.
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, local
 *   returns:     IP number as a formatted string
 *
 * This routine returns the port number for the local or remote 
 * end point for the current socket connection.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_getPort0()
{
    long            local;
    long            fd = getSocketHandle();
    long            port;

    local = (int)KNI_GetParameterAsInt(1);
    port = prim_com_sun_midp_io_j2me_socket_Protocol_getport1(fd, local);

    KNI_ReturnInt((jint)port);
}

/*=========================================================================
 * FUNCTION:      getSocketOpt0();
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the requested socket option value.
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, option
 *   returns:     int value of requested socket option
 *
 * This routine returns the requested socket option current value.
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_getSockOpt0()
{
    long            flag;
    long            fd = getSocketHandle();
    int             optval;

    flag = (int)KNI_GetParameterAsInt(1);
    if (prim_com_sun_midp_io_j2me_socket_Protocol_getsockopt0(fd, flag,
                                                              &optval) == -1) {
	KNI_ThrowNew("java/io/IOException", "");
    }

    KNI_ReturnInt((jint)optval);
}


/*=========================================================================
 * FUNCTION:      setSocketOpt0();
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      set the requested socket option value.
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, option, value
 *   returns:     int value of requested socket option
 *
 * This routine returns the requested socket option current value.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_setSockOpt0()
{
    long            option;
    long            value;
    long            fd = getSocketHandle();
    
    option = (int)KNI_GetParameterAsInt(1);
    value = (int)KNI_GetParameterAsInt(2);

    if (prim_com_sun_midp_io_j2me_socket_Protocol_setsockopt0(fd, option, value) == -1){
 	KNI_ThrowNew("java/io/IOException", "");
    }
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      getLocalAddress0();
 * CLASS:         com.sun.midp.io.j2me.serversocket.Socket
 * TYPE:          virtual native function
 * OVERVIEW:      Get the host address for the local device.
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     IP number as a formatted string
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_OBJECT 
Java_com_sun_midp_io_j2me_serversocket_Socket_getLocalAddress0()
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
    if (getServerSocketHandle() == INVALID_HANDLE) {
	KNI_ThrowNew("java/io/IOException", "");
    }

    KNI_EndHandlesAndReturnObject(result); 
}

/*=========================================================================
 * FUNCTION:      getLocalPort0();
 * CLASS:         com.sun.midp.io.j2me.serversocket.Socket
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
Java_com_sun_midp_io_j2me_serversocket_Socket_getLocalPort0()
{
    long            fd = getServerSocketHandle();
    long            port = -1;
    
    if (fd == -1) {
	KNI_ThrowNew("java/io/IOException", "");
    } else {
	port = prim_com_sun_midp_io_j2me_socket_Protocol_getport1(fd, 1);
    }

    KNI_ReturnInt((jint)port);
}
