/*
 * @(#)socketProtocol_md.c	1.21 02/08/30 @(#)
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

#include <stdio.h>
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
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <socketProtocol.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  64
#endif

#define closesocket close
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#define SD_RECEIVE 0
#define SD_SEND 1
#define SD_BOTH 2
#define HENT_BUF_SIZE 1024

int netError(void);

/*=========================================================================
 * Protocol methods
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      setNonBlocking()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translate a host name into an ip address
 *=======================================================================*/

void prim_com_sun_midp_io_j2me_socket_Protocol_setNonBlocking(int fd)
{
    int flgs = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flgs | O_NONBLOCK);
}

/*=========================================================================
 * FUNCTION:      getIpNumber()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translate a host name into an ip address
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_socket_Protocol_getIpNumber(char *host)
{
    struct hostent *hp;
    struct in_addr addr;
    struct hostent res;
    char buf[HENT_BUF_SIZE];
    int h_error=0;

#ifdef __GLIBC__
    gethostbyname_r(host, &res, buf, sizeof(buf), &hp, &h_error);
#else
    hp = gethostbyname_r(host, &res, buf, sizeof(buf), &h_error);
#endif /* __GLIBC__ */

    if (hp == NULL) {
#if INCLUDEDEBUGCODE
        if (tracenetworking) {
            fprintf(stdout, "getIpNumber host='%s' res=-1\n", host);
        }
#endif /* INCLUDEDEBUGCODE */
        return -1;
    }

    memcpy(&addr, hp->h_addr_list[0], hp->h_length);

#if INCLUDEDEBUGCODE
    if (tracenetworking) {
        fprintf(stdout, "getIpNumber host='%s' res=%lx\n", host,
                (long)addr.s_addr);
    }
#endif /* INCLUDEDEBUGCODE */

    return addr.s_addr;
}

/*=========================================================================
 * FUNCTION:      open0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a TCP socket
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_socket_Protocol_open0(char *name, int port,
                                                    char **exception)
{
    int truebuf  = KNI_TRUE;
    struct sockaddr_in addr;
    int fd = -1, ipn, res;

    ipn = prim_com_sun_midp_io_j2me_socket_Protocol_getIpNumber(name);
    if (ipn == -1) {
        *exception = "javax/microedition/io/ConnectionNotFoundException";
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) {
        *exception = "java/io/IOException";
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&truebuf, sizeof(int));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = ipn;

    res = connect(fd, (struct sockaddr *)&addr, sizeof(addr));

#if INCLUDEDEBUGCODE
    if (tracenetworking) {
        fprintf(stdout,
                "socket::open0-connect ipn=%lx port=%ld res=%ld ne=%ld\n",
                (long)ipn, (long)port, (long)res, (long)netError());
    }
#endif /* INCLUDEDEBUGCODE */

    if (res != SOCKET_ERROR) {
        return fd;
    }

    closesocket(fd);
    *exception = "javax/microedition/io/ConnectionNotFoundException";
    return -1;
}

/*=========================================================================
 * FUNCTION:      read0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Read from a TCP socket
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_socket_Protocol_read0(int fd, char *p, int len)
{
    int res;

    res = recv(fd, p, len, 0);

    if ((res == -1) && (errno == EWOULDBLOCK)) {
        res = IO_WOULDBLOCK;
    }

    if (res == -1 && errno == EINTR) {
        res = IO_INTERRUPTED;
    }

    return res;
}

/*=========================================================================
 * FUNCTION:      available 0
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Return the number of bytes available
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_socket_Protocol_available0(int fd)
{
    int n = 0;
    ioctl(fd, FIONREAD, (void*)&n);
    return n;
}

/*=========================================================================
 * FUNCTION:      write0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Write to a TCP socket
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_socket_Protocol_write0(int fd, char *p, int len)
{
    int res;

    res = send(fd, p, len, 0);

    if ((res == -1) && (errno == EWOULDBLOCK)) {
        res = 0;
    }

    if (res == -1 && errno == EINTR) {
        res = IO_INTERRUPTED;
    }

    return res;
}

/*=========================================================================
 * FUNCTION:      shutdownOutput()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Shutdown the output side of a TCP socket
 *=======================================================================*/

void prim_com_sun_midp_io_j2me_socket_Protocol_shutdownOutput0(int fd)
{
    shutdown(fd, SD_SEND);
}

/*=========================================================================
 * FUNCTION:      close0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_socket_Protocol_close0(int fd)
{
    return closesocket(fd);
}

/*=========================================================================
 * FUNCTION:      open()
 * CLASS:         com.sun.midp.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a listening TCP socket
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_serversocket_Protocol_open0(int port,
                                                          char **exception)
{
    int truebuf  = KNI_TRUE;
    struct sockaddr_in addr;
    int fd = -1, res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) {
        *exception = "java/io/IOException";
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&truebuf, sizeof (int));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    res = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (res == SOCKET_ERROR) {
        *exception = "java/io/IOException";
    } else {
        res = listen(fd, 3);
        if (res == SOCKET_ERROR) {
            *exception = "java/io/IOException";
        } else {
            /* success */
            return fd;
        }
    }

    closesocket(fd);
    return -1;
}

/*=========================================================================
 * FUNCTION:      accept()
 * CLASS:         com.sun.midp.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Accept and open a connection
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_serversocket_Protocol_accept(int fd)
{
    int res;
    struct sockaddr_in sa;
    int saLen = sizeof (sa);

    res = accept(fd, (struct sockaddr *)&sa, &saLen);

    if ((res == -1) && (errno == EWOULDBLOCK)) {
        res = IO_WOULDBLOCK;
    }

    return res;
}

/*=========================================================================
 * FUNCTION:      close0()
 * CLASS:         com.sun.midp.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 *=======================================================================*/

int prim_com_sun_midp_io_j2me_serversocket_Protocol_close(int fd)
{
    return prim_com_sun_midp_io_j2me_socket_Protocol_close0(fd);
}

/*=========================================================================
 * FUNCTION:      wsaInit()
 * TYPE:          machine-specific implementation of native function
 * OVERVIEW:      Initialize the windows socket system
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

void networkInit(void) {
}

/*=========================================================================
 * FUNCTION:      netError()
 * TYPE:          machine-specific implementation of native function
 * OVERVIEW:      return fatal windows socket error
 * INTERFACE:
 *   parameters:  none
 *   returns:     int
 *=======================================================================*/

int netError(void) {
    int res = errno;
    errno = 0;
    return res;
}



/*=========================================================================
 * FUNCTION:      getsockopt0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get a socket option value.
 *=======================================================================*/
int prim_com_sun_midp_io_j2me_socket_Protocol_getsockopt0(int fd, int flag, int *optval)
{
    int level = SOL_SOCKET;
    int optname;
    int optsize = sizeof(optname);
    struct linger lbuf ;
    void * opttarget = (void *) optval ;

    switch (flag) { 
    case 0: /* DELAY */
        level = IPPROTO_TCP;
        optname = TCP_NODELAY;
        break;
    case 1: /* LINGER */
        opttarget = (void *) &lbuf ;
        optsize = sizeof (struct linger);
        optname = SO_LINGER;
        break;
    case 2: /* KEEPALIVE */
        optname = SO_KEEPALIVE;
        break;
    case 3: /* RCVBUF */
        optname = SO_RCVBUF;
        break;
    case 4: /* SNDBUF */
        optname = SO_SNDBUF;
        break;
    default: /* IllegalArgumentException */
        KNI_ThrowNew("java/lang/IllegalArgumentException","");
        return -1;
    }
    if (getsockopt(fd, level,  optname, opttarget, &optsize) == 0 ) {
        if (optname == SO_LINGER) {
            /* If linger is on return the number of seconds. */
            *optval = (lbuf.l_onoff == 0 ? 0 : lbuf.l_linger) ;
        }
#if INCLUDEDEBUGCODE
        if (tracenetworking) {
            fprintf(stdout, "getSockopt option=%d value=%d size=%d\n", 
                optname, *optval, optsize);
        }
#endif /* INCLUDEDEBUGCODE */

        return 0;
    } else {
        fprintf (stderr, "getsockopt errno=%d option=%d level=%d\n", 
                 errno, optname, level );
        return -1;
    }
}

/*=========================================================================
 * FUNCTION:      setsockopt0()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Set a socket option value.
 *=======================================================================*/
int prim_com_sun_midp_io_j2me_socket_Protocol_setsockopt0(int fd, int flag, int optval)
{
    int    level = SOL_SOCKET;
    int    optsize =  sizeof(optval);
    int    optname;
    struct linger lbuf ;
    void * opttarget = (void *) & optval ;

    switch (flag) { 
    case 0: /* DELAY */
        level = IPPROTO_TCP;
        optname = TCP_NODELAY;
        break;
    case 1: /* LINGER */
        opttarget = (void *) &lbuf ;
        optsize = sizeof (struct linger);
        optname = SO_LINGER;
        if (optval == 0) {
            lbuf.l_onoff = 0;
            lbuf.l_linger = 0;
        } else {
            lbuf.l_onoff = 1;
            lbuf.l_linger = optval;
        }           
        break;
    case 2: /* KEEPALIVE */
        optname = SO_KEEPALIVE;
        break;
    case 3: /* RCVBUF */
        optname = SO_RCVBUF;
        break;
    case 4: /* SNDBUF */
        optname = SO_SNDBUF;
        break;
    default: /* IllegalArgumentException */
        KNI_ThrowNew("java/lang/IllegalArgumentException","");
        return -1;
    }

    if (setsockopt(fd, level,  optname, opttarget, optsize) == 0){

#if INCLUDEDEBUGCODE
    if (tracenetworking) {
        fprintf(stdout, "setSockopt option=%d value=%d\n", 
                optname, optval);
        }
#endif /* INCLUDEDEBUGCODE */

    } else {
        fprintf (stderr, "setsockopt errno=%d\n", errno);
        return -1;
    }

    
    return ;
}

/*=========================================================================
 * FUNCTION:      getipnumber1()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the IP address of the local or remote socket endpoint.
 *=======================================================================*/
char * prim_com_sun_midp_io_j2me_socket_Protocol_getipnumber1(int fd,
                                                              int local){
    int res;
    struct sockaddr_in sa;
    int saLen = sizeof (sa);

    sa.sin_family = AF_INET;

    if (local) {
        res = getsockname(fd, (struct sockaddr*)&sa, &saLen);
    } else {
        res = getpeername(fd, (struct sockaddr*)&sa, &saLen);
    }

    if (res < 0) {
        return NULL;
    }

#if INCLUDEDEBUGCODE
    if (tracenetworking) {
        fprintf(stdout, "get ip number port=%d addr=%s\n",  
                sa.sin_port, inet_ntoa(sa.sin_addr) );
    }
#endif /* INCLUDEDEBUGCODE */

    return inet_ntoa(sa.sin_addr);
}
        

/*=========================================================================
 * FUNCTION:      getport1()
 * CLASS:         com.sun.midp.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Get the port number of the local or remote socket endpoint.
 *=======================================================================*/
int prim_com_sun_midp_io_j2me_socket_Protocol_getport1(int fd, int local){
    int res;
    struct sockaddr_in sa;
    int saLen = sizeof (sa);

    if (local == 1 ) {
        res = getsockname(fd, (struct sockaddr*)&sa, &saLen);
    } else {
        res = getpeername(fd, (struct sockaddr*)&sa, &saLen);
    }

    if (res < 0) {
        return -1;
    }

#if INCLUDEDEBUGCODE
    if (tracenetworking) {
        fprintf(stdout, "get ip number family=%x port=%d addr=%s\n",  
                sa.sin_family, sa.sin_port, inet_ntoa(sa.sin_addr) );
        }
#endif /* INCLUDEDEBUGCODE */
    return ntohs(sa.sin_port);
}
        
/*=========================================================================
 * FUNCTION:      getLocalIPNumber_md()
 * TYPE:          common native function
 * OVERVIEW:      Get the IP address of the local device.
 *=======================================================================*/
int getLocalIPNumber_md() {
    char hostname[MAXHOSTNAMELEN];
    struct hostent *hp;

    gethostname(hostname, sizeof (hostname));

#if INCLUDEDEBUGCODE
    if (tracenetworking) {
        fprintf(stdout, "local hostname = %s\n", hostname);
    }
#endif /* INCLUDEDEBUGCODE */

    hp = gethostbyname(hostname);
    return ((struct in_addr*)*hp->h_addr_list)->s_addr;
}

/*=========================================================================
 * FUNCTION:      getLocalIPAddressAsString_md()
 * TYPE:          common native function
 * OVERVIEW:      Get the IP address of the local device.
 *=======================================================================*/
char *getLocalIPAddressAsString_md() {
    struct in_addr addr;

    addr.s_addr = getLocalIPNumber_md();
    return inet_ntoa(addr);
}
