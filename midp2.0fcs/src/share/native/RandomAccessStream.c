/*
 * @(#)RandomAccessStream.c	1.18 02/08/26 @(#)
 *
 * Copyright (c) 1998-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <stdio.h>

#include <kni.h>
#include <midpMalloc.h>
#include <storage.h>

static void
storageCleanup(jobject thisObject) {
    int   handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(clazz);

    KNI_GetObjectClass(thisObject, clazz);
    handle = KNI_GetIntField(thisObject, KNI_GetFieldID(clazz, "handle", "I"));

    if (handle != -1) {
        char* pszError;

        storageClose(&pszError, handle);
        if (pszError != NULL) {
            storageFreeError(pszError);
        }
	KNI_SetIntField(thisObject, KNI_GetFieldID(clazz, "handle", "I"), -1);
    }

    KNI_EndHandles();
}

/*
 * static int open(byte[] szAsciiFilename, int ioMode) throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_storage_RandomAccessStream_open() {
    int   ioMode = KNI_GetParameterAsInt(2);
    char* pszAsciiFilename;
    int   handle;
    char* pszError;
    int   nameLen;

    KNI_StartHandles(2);
    KNI_DeclareHandle(thisObject);
    KNI_DeclareHandle(filename);

    KNI_GetThisPointer(thisObject);
    KNI_GetParameterAsObject(1, filename);

    nameLen = KNI_GetArrayLength(filename);
    pszAsciiFilename = (char*)midpMalloc(nameLen+1);

    if (pszAsciiFilename == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "RAS Open");
    } else {
        
        KNI_GetRawArrayRegion(filename, 0, nameLen, (jbyte*)pszAsciiFilename);
        pszAsciiFilename[nameLen] = '\0';
        handle = storageOpen(&pszError, pszAsciiFilename, ioMode);
        midpFree(pszAsciiFilename);

        if (pszError != NULL) {
            KNI_ThrowNew("java/io/IOException", pszError);
            storageFreeError(pszError);
        } else {
	    KNI_registerCleanup(thisObject, storageCleanup);
	}
    }

    KNI_EndHandles();
    KNI_ReturnInt(handle);
}

/*
 * static void close(int handle) throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_storage_RandomAccessStream_close() {
    int handle = KNI_GetParameterAsInt(1);
    char* pszError;

    storageClose(&pszError, handle);
    if (pszError != NULL) {
        KNI_ThrowNew("java/io/IOException", pszError);
        storageFreeError(pszError);
    }
    KNI_ReturnVoid();
}

/*
 * static int read(int handle, byte[] buffer, int offset, int length)
 *  throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_storage_RandomAccessStream_read() {
    int   length = KNI_GetParameterAsInt(4);
    int   offset = KNI_GetParameterAsInt(3);
    int   handle = KNI_GetParameterAsInt(1);
    int   bytesRead;
    char* bytes;
    char* pszError;

    KNI_StartHandles(1);
    KNI_DeclareHandle(buffer);

    KNI_GetParameterAsObject(2, buffer);
    bytes = (char*)midpMalloc(length);

    if ((bytesRead = storageRead(&pszError, handle, bytes, length)) >= 0) {
	KNI_SetRawArrayRegion(buffer, offset, bytesRead, (jbyte*)bytes);
    }
    midpFree(bytes);
    KNI_EndHandles();

    if (pszError != NULL) {
        KNI_ThrowNew("java/io/IOException", pszError);
        storageFreeError(pszError);
    }
    KNI_ReturnInt(bytesRead);
}

/*
 * static void write(int handle, byte[] buffer, int offset, int length)
 *     throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_storage_RandomAccessStream_write() {
    int   length = KNI_GetParameterAsInt(4);
    int   offset = KNI_GetParameterAsInt(3);
    int   handle = KNI_GetParameterAsInt(1);
    char* pszError;
    char* bytes;

    KNI_StartHandles(1);
    KNI_DeclareHandle(buffer);

    KNI_GetParameterAsObject(2, buffer);

    bytes = (char*)midpMalloc(length);

    if (bytes == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "RAS write");
    } else {

        KNI_GetRawArrayRegion(buffer, offset, length, (jbyte*)bytes);
        storageWrite(&pszError, handle, bytes, length);
        midpFree(bytes);

        if (pszError != NULL) {
            KNI_ThrowNew("java/io/IOException", pszError);
            storageFreeError(pszError);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*
 * static native void position(int handle, int absolutePosition)
 *     throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_storage_RandomAccessStream_position() {
    long  absolutePosition = (long)KNI_GetParameterAsInt(2);
    int   handle           = KNI_GetParameterAsInt(1);
    char* pszError;

    storagePosition(&pszError, handle, absolutePosition);
    if (pszError != NULL) {
        KNI_ThrowNew("java/io/IOException", pszError);
        storageFreeError(pszError);
    }
    KNI_ReturnVoid();
}

/*
 * static native void sizeOf(int handle)
 *     throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_storage_RandomAccessStream_sizeOf() {
    int   handle = KNI_GetParameterAsInt(1);
    char* pszError;
    int   size;

    size = storageSizeOf(&pszError, handle);
    if (pszError != NULL) {
        KNI_ThrowNew("java/io/IOException", pszError);
        storageFreeError(pszError);
    }
    KNI_ReturnInt(size);
}

/*
 * static native void truncateStream(int handle, int size)
 *     throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_storage_RandomAccessStream_truncateStream() {
    int   size   = KNI_GetParameterAsInt(2);
    int   handle = KNI_GetParameterAsInt(1);
    char* pszError;

    storageTruncate(&pszError, handle, size);
    if (pszError != NULL) {
        KNI_ThrowNew("java/io/IOException", pszError);
        storageFreeError(pszError);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_storage_RandomAccessStream_finalize() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(instance);
    KNI_GetThisPointer(instance);

    storageCleanup(instance);

    KNI_EndHandles();

    KNI_ReturnVoid();
}
