/*
 * @(#)MethodTimes.java	1.11 02/07/25 @(#)
 *
 * Copyright (c) 1999-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example;

import javax.microedition.midlet.*;

import java.lang.String;



/**
 * An example MIDlet runs a simple timing test
 * When it is started by the application management software it will
 * create a separate thread to do the test.
 * When it finishes it will notify the application management software
 * it is done.
 * Refer to the startApp, pauseApp, and destroyApp
 * methods so see how it handles each requested transition.
 */
public class MethodTimes extends MIDlet implements Runnable {
    // The state for the timing thread.
    Thread thread;

    /**
     * Start creates the thread to do the timing.
     * It should return immediately to keep the dispatcher
     * from hanging.
     */
    public void startApp() {
    	thread = new Thread(this);
    	thread.start();
    	System.out.println("startApp");
    }

    /**
     * Pause signals the thread to stop by clearing the thread field.
     * If stopped before done with the iterations it will
     * be restarted from scratch later.
     */
    public void pauseApp() {
	thread = null;
	System.out.println("pauseApp");
    }

    /**
     * Destroy must cleanup everything.  The thread is signaled
     * to stop and no result is produced.
     */
    public void destroyApp(boolean unconditional) {
	thread = null;
	System.out.println("destroyApp");
    }

    /**
     * Run the timing test, measure how long it takes to
     * call a empty method 1000 times.
     * Terminate early if the current thread is no longer
     * the thread from the
     */
    public void run() {
        Thread curr = Thread.currentThread();  // Remember current thread
    	long start = System.currentTimeMillis();
    	for (int i = 0; i < 1000000 && thread == curr; i++) {
    	    empty();
    	}
    	long end = System.currentTimeMillis();

    	// Check if timing was aborted, if so just exit
        // The rest of the application has already become quiescent.
    	if (thread != curr) {
    	    return;
    	}
    	long millis = end - start;
        // Reporting the elapsed time is outside the scope of this example.
	System.out.println("Elapsed " + millis + " ms.");
	// All done cleanup and quit
        destroyApp(true);
    	notifyDestroyed();
    }

    /**
     * An Empty method.
     */
    void empty() {
    }
}

