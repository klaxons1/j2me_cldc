/*
 * @(#)perfmon.c	1.6 02/07/24 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <profiling.h>

/** 
 * The system performance measurement count type's 
 */
#define INSTRUCTION_COUNTER        0
#define THREAD_SWITCH_COUNTER      1
#define DYNAMIC_OBJECT_COUNTER     2
#define DYNAMIC_ALLOC_COUNTER      3
#define DYNAMIC_DEALLOC_COUNTER    4
#define GARBAGE_COLLECTION_COUNTER 5
#define TOTAL_GC_DEFERRALS         6
#define MAX_GC_DEFERRALS           7
#define GARBAGE_COLLECTION_RESCANS 8

/*=========================================================================
 * FUNCTION:      sysGetCounter(I)I (STATIC)
 * CLASS:         com/sun/midp/perfmon/PerfMon
 * TYPE:          static native function
 * OVERVIEW:      queries the VM for performance data
 * INTERFACE (operand stack manipulation):
 *   parameters:  count - performance measurement count type
 *   returns:     current measurement value for count type
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_perfmon_PerfMon_sysGetCounter() {
    int   count = KNI_GetParameterAsInt(1);
    jint retval = 0;

    switch (count) 
    {
    case INSTRUCTION_COUNTER:        
        retval = (jint)InstructionCounter;
        break;
    case THREAD_SWITCH_COUNTER:
        retval = (jint)ThreadSwitchCounter;
        break;
    case DYNAMIC_OBJECT_COUNTER:
        retval = (jint)DynamicObjectCounter;
        break;
    case DYNAMIC_ALLOC_COUNTER:
        retval = (jint)DynamicAllocationCounter;
        break;
    case DYNAMIC_DEALLOC_COUNTER:
        retval = (jint)DynamicDeallocationCounter;
        break;
    case GARBAGE_COLLECTION_COUNTER:
        retval = (jint)GarbageCollectionCounter;
        break;
    case TOTAL_GC_DEFERRALS:
        retval = (jint)TotalGCDeferrals;
        break;
    case MAX_GC_DEFERRALS:
        retval = (jint)MaximumGCDeferrals;
        break;
    case GARBAGE_COLLECTION_RESCANS:
        retval = (jint)GarbageCollectionRescans;
        break;
    default :
        break;
    }

    KNI_ReturnInt(retval);
}


