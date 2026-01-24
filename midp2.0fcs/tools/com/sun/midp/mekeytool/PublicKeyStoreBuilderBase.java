/*
 * @(#)PublicKeyStoreBuilderBase.java	1.7 02/09/18 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package com.sun.midp.mekeytool;

import java.io.*;
import java.util.*;

import com.sun.midp.publickeystore.PublicKeyStore;
import com.sun.midp.publickeystore.PublicKeyInfo;

/**
 * A read-write serializable {@link PublicKeyStore}. This class is the base
 * for {@link PublicKeyStoreBuilder}, and does not have any methods that
 * depend on any Microediton specific classes so it can be used alone in a
 * tool written with Standard Edition Java.
 */
public class PublicKeyStoreBuilderBase extends PublicKeyStore {
    /** Vecor of keys maintained as a list. */
    private Vector keyList = new Vector();

    /**
     * Constructs an empty read-write keystore.
     */
    public PublicKeyStoreBuilderBase() {
        initPublicKeyStore(keyList);
    };

    /**
     * Constructs a read-write keystore from a serialized keystore created
     * by this class.
     * @param in stream to read a keystore serialized by
     *        {@link #serialize(OutputStream)} from
     * @exception IOException if the key storage was corrupted
     */
    public PublicKeyStoreBuilderBase(InputStream in) throws IOException {
        initPublicKeyStore(in, keyList);
    }

    /**
     * Serializes the keystore to the given stream.
     * @param out stream to serialize the keystore to
     * @exception IOException is thrown, if an I/O error occurs
     */
    public void serialize(OutputStream out) throws IOException {
        OutputStorage storage = new OutputStorage(out);
        Enumeration e;
        PublicKeyInfo keyInfo;
  
        e = keyList.elements();
        while (e.hasMoreElements()) {
            keyInfo = (PublicKeyInfo)e.nextElement();
            putKeyInStorage(storage, keyInfo);
        }
    }

    /**
     * Adds a public key.
     *
     * @param keyInfo the key to add
     */
    public synchronized void addKey(PublicKeyInfo keyInfo) {
        keyList.addElement(keyInfo);
    }

    /**
     * Updates all of an key's information except for the security domain.
     * information in the store.
     *
     * @param number key number of key 0 being the first
     * @param newKeyInfo new key information
     *
     * @exception  ArrayIndexOutOfBoundsException  if an invalid number was
     *             given.
     */
    public synchronized void updateKey(int number,
                                          PublicKeyInfo newKeyInfo) {
        PublicKeyInfo oldKeyInfo;

        oldKeyInfo = getKey(number);

        newKeyInfo.setDomain(oldKeyInfo.getDomain());

        keyList.setElementAt(newKeyInfo, number);
    }

    /**
     * Deletes a public key from this keystore by number.
     *
     * @param number number of the key with 0 being the first.
     *
     * @exception  ArrayIndexOutOfBoundsException  if an invalid number was
     *             given.
     */
    public void deleteKey(int number) {
        keyList.removeElementAt(number);
    }

    /**
     * Serializes every field with a tag.
     * @param storage what to put the key in
     * @param key key information object
     */
    private void putKeyInStorage(OutputStorage storage, PublicKeyInfo key) 
            throws java.io.IOException {
        storage.writeValue(PublicKeyInfo.OWNER_TAG, key.getOwner());
        storage.writeValue(PublicKeyInfo.NOT_BEFORE_TAG, key.getNotBefore());
        storage.writeValue(PublicKeyInfo.NOT_AFTER_TAG, key.getNotAfter());
        storage.writeValue(PublicKeyInfo.MODULUS_TAG, key.getModulus());
        storage.writeValue(PublicKeyInfo.EXPONENT_TAG, key.getExponent());
        storage.writeValue(PublicKeyInfo.DOMAIN_TAG, key.getDomain());
    }
}
