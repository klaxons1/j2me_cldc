/*
 * @(#)socketProtocol.h	1.15 02/09/04 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking (Generic Connection framework)
 * FILE:      socketProtocol.h
 * OVERVIEW:  Prototypes for supporting using sockets, serversockets
 *            on the Generic Connection framework.
 *            If a target platform wants to support these networking
 *            capabilities, a platform-specific implementation of the
 *            functions below must be provided in the Vm<Port>/src directory,
 *            where <Port> is the name of your target platform
 *            (e.g., Win, Unix, Pilot).  A default implementation is
 *            provided in VmExtra/src.
 * AUTHOR:    Nik Shaylor
 *=======================================================================*/

#ifndef _SOCKET_PROTOCOL_H_
#define _SOCKET_PROTOCOL_H_

/*=========================================================================
 * Function prototypes for the native networking operations
 *=======================================================================*/

#define IO_WOULDBLOCK -2
#define IO_INTERRUPTED -3

extern int  prim_com_sun_midp_io_j2me_socket_Protocol_open0(char*, int, char**);
extern int  prim_com_sun_midp_io_j2me_socket_Protocol_read0(int, char*, int);
extern int  prim_com_sun_midp_io_j2me_socket_Protocol_available0(int);
extern int  prim_com_sun_midp_io_j2me_socket_Protocol_write0(int, char*, int);
extern void prim_com_sun_midp_io_j2me_socket_Protocol_shutdownOutput0(int);
extern int  prim_com_sun_midp_io_j2me_socket_Protocol_close0(int);

extern int  prim_com_sun_midp_io_j2me_socket_Protocol_getIpNumber(char*);
extern void prim_com_sun_midp_io_j2me_socket_Protocol_setNonBlocking(int);

extern int  prim_com_sun_midp_io_j2me_serversocket_Protocol_open0(int, char**);
extern int  prim_com_sun_midp_io_j2me_serversocket_Protocol_accept(int);
extern int  prim_com_sun_midp_io_j2me_serversocket_Protocol_close(int);

extern void networkInit(void);
extern char *getLocalHostName(void);
extern int  netError(void);
extern int  prim_com_sun_midp_io_j2me_socket_Protocol_getsockopt0(int, int, int*);
extern int  prim_com_sun_midp_io_j2me_socket_Protocol_setsockopt0(int, int, int);
extern int  prim_com_sun_midp_io_j2me_socket_Protocol_getport1(int fd, int local);
extern char * prim_com_sun_midp_io_j2me_socket_Protocol_getipnumber1(int fd, int local);

extern int getLocalIPNumber_md();
extern char* getLocalIPAddressAsString_md();

#endif /* _SOCKET_PROTOCOL_H_ */
