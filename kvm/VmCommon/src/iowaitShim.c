/*
 * iowaitShim.c
 *
 * Implementation of the CLDC native com.sun.cldc.io.GeneralBase.iowait().
 *
 * This native has the same semantics as com.sun.cldc.io.Waiter.waitForIO
 * (see nativeCore.c): a thread blocked in I/O calls it to either sleep for
 * a short while (GENERIC_IO_WAIT_TIME milliseconds) or yield to other
 * threads. This source tree does not contain a definition of the
 * GeneralBase natives, so we provide the shim here.
 */

#include <global.h>

#ifndef GENERIC_IO_WAIT_TIME
#define GENERIC_IO_WAIT_TIME 0
#endif

void Java_com_sun_cldc_io_GeneralBase_iowait(void)
{
#if GENERIC_IO_WAIT_TIME > 0
    /* Suspend the current thread for GENERIC_IO_WAIT_TIME milliseconds */
    THREAD thisThread = CurrentThread;
    suspendThread();
    registerAlarm(thisThread, (long64)GENERIC_IO_WAIT_TIME, resumeThread);
#else
    /* Try to switch to another thread */
    signalTimeToReschedule();
#endif
}
