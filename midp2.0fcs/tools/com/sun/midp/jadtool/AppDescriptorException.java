/*
 * @(#)AppDescriptorException.java	1.4 02/08/22 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package com.sun.midp.jadtool;

/** Exception for AppDescriptor errors. */
public class AppDescriptorException extends Exception {

    /** Generic default code. (0) */
    public static int UNSPECIFIED_ERROR = 0;
    /** The keystore for the AppDescriptor was not initialized. */
    public static final int KEYSTORE_NOT_INITIALIZED = 4;

    /** The error code. */
    private int myError = 0;
    
    /**
     * Create an ApplicationDescriptorException.
     *
     * @param errorCode error code of the exception
     */
    public AppDescriptorException(int errorCode) {
        super();
        myError = errorCode;
    }
    
    /**
     * Create an ApplicationDescriptorException.
     *
     * @param msg message of the exception
     */
    public AppDescriptorException(String msg) {
        super(msg);
    }

    /**
     * Create an ApplicationDescriptorException.
     *
     * @param msg message of the exception
     * @param errorCode error code of the exception
     */
    public AppDescriptorException(String msg, int errorCode) {
        super(msg);
        myError = errorCode;
    }

    /**
     * Get the error code of the exception.
     *
     * @return error code
     */
    public int getErrorCode() {
        return myError;
    }
}
