/*
 * @(#)commProtocol.h	1.8 02/07/24 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking
 * FILE:      commProtocol.h
 * OVERVIEW:  Prototypes for supporting serial communication ports.
 *            If a target platform wants to support serial ports,
 *            a platform-specific implementation of the functions
 *            must be provided in the Vm<Port>/src directory,
 *            where <Port> is the name of your target platform
 *            (e.g., Win, Unix, Pilot).
 * AUTHOR:    Stephen Flores, Nik Shaylor
 *=======================================================================*/

#ifndef _COMM_PROTOCOL_H_
#define _COMM_PROTOCOL_H_

#ifndef __P
# ifdef __STDC__
#  define __P(p) p
# else
#  define __P(p) ()
# endif
#endif /* __P */

#define MAX_NAME_LEN 80

/* COMM options */
#define STOP_BITS_2     0x01
#define ODD_PARITY      0x02
#define EVEN_PARITY     0x04
#define AUTO_RTS        0x10
#define AUTO_CTS        0x20
#define BITS_PER_CHAR_7 0x80
#define BITS_PER_CHAR_8 0xC0

/*=========================================================================
 * Serial protocol prototypes (each platform/port must supply definitions
 * of these prototypes)    
 *=======================================================================*/

extern long openPortByNumber __P ((char** ppszError, long port, long baudRate, long options));
extern long openPortByName __P ((char** ppszError, char* pszDeviceName, long baudRate, long options));
extern void configurePort __P ((char** ppszError, int hPort, long baudRate, unsigned long options));

extern long readFromPort __P ((char** ppszError, long hPort, char* pBuffer, long nNumberOfBytesToRead));

extern long writeToPort __P ((char** ppszError, long hPort, char* pBuffer, long nNumberOfBytesToWrite));

extern void closePort __P ((long hPort));
extern void freePortError __P ((char* pszError));

#endif /* _COMM_PROTOCOL_H_ */


