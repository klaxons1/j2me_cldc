/*
 * @(#)machine_md.h	1.5 02/10/14 @(#)
 *
 * Copyright (c) 1998-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 *
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * FILE:      machine_md.h (for UNIX/Solaris/Linux)
 * OVERVIEW:  This file is included in every compilation.  It contains
 *            definitions that are specific to the Solaris/Linux ports 
 *            of KVM.
 * AUTHOR:    Frank Yellin
 *            Andreas Heilwagen, Kinsley Wong (Linux port)
 * NOTE:      This file overrides many of the default compilation 
 *            flags and macros defined in VmCommon/h/main.h.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#ifdef LINUX
#include <sys/time.h>
#endif

/*=========================================================================
 * Platform-specific datatype definitions
 *=======================================================================*/

typedef long long           long64;    /* 64-bit signed integer type */
typedef unsigned long long  ulong64;   /* 64-bit unsigned integer type */
typedef long long           jlong;     /* Added for KNI */

#ifndef Calendar_md
unsigned long * Calendar_md(void);
#endif

/*=========================================================================
 * Compilation flags and macros that override values defined in main.h
 *=======================================================================*/

/* By default we are assume that if we are not using a i386 machine the 
 * target platform is big-endian.
 * Solaris requires 8-byte alignment of longs and doubles */

#if !defined(i386) && !defined(__arm__)
#define BIG_ENDIAN 1
#define NEED_LONG_ALIGNMENT 1
#define NEED_DOUBLE_ALIGNMENT 1
#else
#undef BIG_ENDIAN
#undef LITTLE_ENDIAN
#undef NEED_LONG_ALIGNMENT 
#define LITTLE_ENDIAN 1
#define NEED_LONG_ALIGNMENT 0 
#endif

/* Make the VM run a little faster (can afford the extra space) */
#define ENABLEFASTBYTECODES 1

/* Override the sleep function defined in main.h */
#define SLEEP_UNTIL(wakeupTime)                                                         \
    {  long delta = wakeupTime - CurrentTime_md();                                      \
           struct timeval timeout;                                                      \
           timeout.tv_sec = delta / 1000;                                               \
           timeout.tv_usec = (delta % 1000) * 1000;                                     \
           select(0, NULL, NULL, NULL, &timeout);                                       \
        }

/*=========================================================================
 * Platform-specific macros and function prototypes
 *=======================================================================*/

#define InitializeVM()
#define FinalizeVM()

#define freeHeap(heap) free(heap)
#define RandomNumber_md() rand()

void InitializeWindowSystem();
void FinalizeWindowSystem(void);

/*=========================================================================
 * The following are used in several different places, and its worthwhile
 * to define them just once
 *=======================================================================*/

void* allocateVirtualMemory_md(long size);
void  freeVirtualMemory_md(void *address, long size);

enum { PVM_NoAccess, PVM_ReadOnly, PVM_ReadWrite };
void  protectVirtualMemory_md(void *address, long size, int protection);

/* MIDP modification */
/* removed empty definition of GetAndStoreNextKVMEvent() */

/* 
 * override IS_RESTRICTED_PACKAGE_NAME macro in main.h to add more 
 * restricted namespace.
 *
 * return type for checkRestrictedPackageName() is "unsigned char" 
 * instead of "jboolean" because trying to include kni.h causes
 * compilation errors.
 */
extern unsigned char checkRestrictedPackageName(const char *name);

#define IS_RESTRICTED_PACKAGE_NAME(name) checkRestrictedPackageName(name)

