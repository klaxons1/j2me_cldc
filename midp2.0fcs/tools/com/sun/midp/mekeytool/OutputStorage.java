/*
 * @(#)OutputStorage.java	1.5 02/09/18 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package com.sun.midp.mekeytool;

import java.io.*;

import com.sun.midp.publickeystore.Storage;

/**
 * Write fields to an OutputStream.
 */
class OutputStorage extends Storage {
    /** stream to write to */
    private DataOutputStream out;

    /**
     * Constructs an OutputStorage for an OutputStream.
     * @param output the output storage output stream.
     * @exception IOException if the storage version cannot be written
     */
    OutputStorage(OutputStream output) throws IOException {
        out = new DataOutputStream(output);

        out.writeByte(CURRENT_VERSION);
    }

    /**
     * Stores a byte array field as tag, BINARY_TYPE, value.
     * @param tag number to unique to this field
     * @param value value of field
     */
    void writeValue(byte tag, byte[] value) throws IOException {
        out.writeByte(tag);
        out.writeByte(BINARY_TYPE);

        /*
         * must write our own length, because DataOutputStream does not handle
         * handle byte arrays.
         */
        out.writeShort(value.length);
        out.write(value);
    }

    /**
     * Stores a String field as tag, STRING_TYPE, value.
     * @param tag number to unique to this field
     * @param value value of field
     */
    void writeValue(byte tag, String value) throws IOException {
        out.writeByte(tag);
        out.writeByte(STRING_TYPE);
        out.writeUTF(value);
    }

    /**
     * Stores a long field as tag, LONG_TYPE, value.
     * @param tag number to unique to this field
     * @param value value of field
     */
    void writeValue(byte tag, long value) throws IOException {
        out.writeByte(tag);
        out.writeByte(LONG_TYPE);
        out.writeLong(value);
    }
}
