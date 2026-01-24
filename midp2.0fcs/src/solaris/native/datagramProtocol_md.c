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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
# ifndef BSD_COMP
#  define BSD_COMP
# endif
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define closesocket close
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#define HENT_BUF_SIZE 1024

int netError(void);

#ifdef __linux__
#include <sys/types.h>
typedef __u_long ulong_t;
#endif /* __linux__ */

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
    struct sockaddr_in addr;
    int fd = -1;
    int res, i;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET) {
        *exception = "java/io/IOException";
        return -1;
    }

    i = port;

   /*
    * Here is a total mystery to me. If I give solaris a port number of zero
    * then it allocated one somewhere above 50000. The problem is that when I
    * do a recvfrom() on a socket that is bound to a port > 32767 it never sees
    * any data.
    *
    * Temporary solution (seems to work all the time):
    * Start at port 6000 and just go upwards until a free port is found
    */
    if (i <= 0) {
        i = 6000;
    }

    res = EADDRINUSE;
    do {
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons((short)i++);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        res = bind(fd, (struct sockaddr*)&addr, sizeof( addr ));
    } while (res == -1 && errno == EADDRINUSE && port == 0 && i < 32767);

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
    int flgs = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flgs | O_NONBLOCK);
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
        if (lastError == EWOULDBLOCK) {
            return 0;
        } else  if (lastError == EINTR) {
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
    int len = sizeof(struct sockaddr_in);
    int res;
    int lastError;

    res = recvfrom(fd, buffer, length, 0, (struct sockaddr*)&addr, &len);

    if (res == SOCKET_ERROR) {
        lastError = netError();
        if (lastError == EWOULDBLOCK) {
            res = IO_WOULDBLOCK;
        } else if (lastError == EINTR) {
            res = IO_INTERRUPTED;
        } else {
            res = -1;
        }

        return res;
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
    return closesocket(fd);
}


