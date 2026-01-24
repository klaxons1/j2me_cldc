/*
 * @(#)datagramProtocol.h	1.11 02/07/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking (Generic Connection framework)
 * FILE:      datagramProtocol.h
 * OVERVIEW:  Prototypes for supporting using datagrams on the
 *            Generic Connection framework.  If a target platform
 *            wants to support these networking
 *            capabilities, a platform-specific implementation of the 
 *            functions below must be provided in the Vm<Port>/src directory,
 *            where <Port> is the name of your target platform
 *            (e.g., Win, Unix, Pilot).  A default implementation is
 *            provided in VmExtra/src.
 * AUTHOR:    Nik Shaylor
 *=======================================================================*/

#ifndef _DATAGRAM_PROTOCOL_H_
#define _DATAGRAM_PROTOCOL_H_

/*=========================================================================
 * Function prototypes for the native networking operations
 *=======================================================================*/

#ifndef MAX_HOST_LENGTH
# define MAX_HOST_LENGTH 256
#endif /* MAX_HOST_LENGTH */

extern int prim_com_sun_midp_io_j2me_datagram_Protocol_open0(int, char**);
extern int prim_com_sun_midp_io_j2me_datagram_Protocol_close(int);

extern int prim_com_sun_midp_io_j2me_datagram_Protocol_send0(int, int, int, char*, int);
extern int prim_com_sun_midp_io_j2me_datagram_Protocol_receive0(int, int*, int*, char*, int);

extern void prim_com_sun_midp_io_j2me_datagram_Protocol_getHostByAddr(int, char*);
extern int prim_com_sun_midp_io_j2me_datagram_Protocol_getNominalLength(int);
extern int prim_com_sun_midp_io_j2me_datagram_Protocol_getMaximumLength(int);

extern void prim_com_sun_midp_io_j2me_datagram_Protocol_setNonBlocking(int);

#endif /* _DATAGRAM_PROTOCOL_H_ */


