/*
 * @(#)datagramProtocol_md.c	1.20 02/07/24 @(#)
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
 */

/**
 * This implements the network protocols for Windows
 *
 * @author  Nik Shaylor
 * @version 1.0 1/17/2000
 */

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <kni.h>
#include <socketProtocol.h>
#include <datagramProtocol.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#define WIN32_LEAN_AND_MEAN
#define NOMSG
#include <windows.h>
#include <process.h>
#ifdef GCC
#include <errno.h>
#include <winsock.h>
#else
#include <winsock2.h>
#endif

#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))
#endif

WSADATA wsaData;
int netError(void);

#ifndef ulong_t
typedef unsigned long ulong_t;
#endif /* ulong_t */

/*=========================================================================
 * Protocol Methods
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      open0()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a TCP socket
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_datagram_Protocol_open0(int port,
                                                      char **exception)
{
    int truebuf  = TRUE;
    struct sockaddr_in addr;
    int fd = -1;
    int res;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET) {
        *exception = "java/io/IOException";
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&truebuf, sizeof(int)); 

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    res = bind(fd, (struct sockaddr*)&addr, sizeof( addr ));

    if (res != SOCKET_ERROR) {
        return fd;
    }

    closesocket(fd);
    *exception = "java/io/IOException";
    return -1;
}

/*=========================================================================
 * FUNCTION:      getMaximumLength()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the datagram max length
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_datagram_Protocol_getMaximumLength(int fd)
{
    int optval = 0;
    int optlen = sizeof (optval);
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &optlen);
    return (optval);
}

/*=========================================================================
 * FUNCTION:      getNominalLength()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the datagram nominal length
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     int
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_datagram_Protocol_getNominalLength(int fd)
{
    int optval = 0;
    int optlen = sizeof (optval);
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &optlen);
    return (optval);
}

/*=========================================================================
 * FUNCTION:      getHostByAddr()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translate an ip address into a host name
 *=======================================================================*/

void
prim_com_sun_midp_io_j2me_datagram_Protocol_getHostByAddr(int ipn, char* host)
{
    int h_len;
    ulong_t addr = (ulong_t)ipn;
    struct hostent *hp;

    hp = gethostbyaddr((char *)&addr, sizeof (addr), AF_INET);

    if (hp == NULL) {
        struct in_addr ipAddress;

        /* There may not be an DNS host for this address,
           so convert it to dot notation. */

        ipAddress.s_addr = ipn;
        strcpy(host, inet_ntoa(ipAddress));
    } else {
        h_len = strlen(hp->h_name);
        if (h_len > MAX_HOST_LENGTH - 1) {
            h_len = MAX_HOST_LENGTH - 1;
        }

        (void)memmove(host, hp->h_name, h_len);
        host[h_len] = 0;
    }

#if INCLUDEDEBUGCODE
    if (tracenetworking) {
        fprintf(stdout, "getHostByAddr ip addr='%lx' host='%s'\n",
                (long)ipn, host);
    }
#endif /* INCLUDEDEBUGCODE */
}

/*=========================================================================
 * FUNCTION:      setNonBlocking()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translate a host name into an ip address
 *=======================================================================*/

void prim_com_sun_midp_io_j2me_datagram_Protocol_setNonBlocking(int fd)
{
    unsigned long blockingFlag = 1;

    ioctlsocket(fd, FIONBIO, &blockingFlag);
}

/*=========================================================================
 * FUNCTION:      send0()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Send a datagram
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_datagram_Protocol_send0(int fd, int ipnumber,
    int port, char *buffer, int length)
{
    struct sockaddr_in addr;
    int res;
    int lastError;

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((short)port);
    addr.sin_addr.s_addr = ipnumber;

    res = sendto(fd, buffer, length, 0, (struct sockaddr*)&addr, sizeof(addr));

    if (res == SOCKET_ERROR) {
        lastError = netError();
        if (lastError == WSAEWOULDBLOCK) {
            res = 0;
        } else if (lastError == WSAEINTR) {
            res = IO_INTERRUPTED;
        } else {
            res = -1;
        }
    }

    return res;
}

/*=========================================================================
 * FUNCTION:      receive0()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Receive a datagram
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_datagram_Protocol_receive0(int fd, int *ipnumber,
    int *port, char *buffer, int length)
{
    struct sockaddr_in addr;
    int len = sizeof (struct sockaddr_in);
    int res;
    int lastError;

    res = recvfrom(fd, buffer, length, 0, (struct sockaddr*)&addr, &len);

    if (res == SOCKET_ERROR) {
        lastError = netError();
        if (lastError == WSAEWOULDBLOCK) {
            return IO_WOULDBLOCK;
        }

        if (lastError == WSAECONNRESET) {
            /* The last "sendto" failed, to late now, just return 0 */
            return 0;
        }

        if (lastError == WSAEINTR) {
            return IO_INTERRUPTED;
        }

        if (lastError == WSAEMSGSIZE) {
            /* the message was larger than the buffer and was truncated. */
            res = length;
        } else {
            return -1;
        }
    }

    *ipnumber = (long)addr.sin_addr.s_addr;
    *port     = ntohs(addr.sin_port);

    return res;
}

/*=========================================================================
 * FUNCTION:      close()
 * CLASS:         com.sun.midp.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a datagram socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     int
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_datagram_Protocol_close(int fd)
{
    int res;
    int lastError;

    res = closesocket(fd);

    if (res == SOCKET_ERROR) {
        lastError = netError();

        if (lastError == WSAEWOULDBLOCK) {
            /* call closesocket again, see doc for closesocket */
            closesocket(fd);
            res = 0;
        } else {
            res = -1;
        }
    }

    return res;
}


