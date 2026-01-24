/*
 * @(#)storageFile.c	1.16 02/08/05 @(#)
 *
 * Copyright (c) 1998-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * The file is contains native functions for the package
 * com.sun.midp.io.j2me.storage.
 *
 * The functions are only a thin layer between the Java storage classes and the
 * native storage interface (see storage.h) that does the real work. Basically
 * it handles the parameter passing and exception handling shared between all
 * systems, relying the system specific code in storage.c.
 */

#include <stdio.h>

#include <kni.h>
#include <midpMalloc.h>
#include <storage.h>

/*
 * static String initStorageRoot()
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_io_j2me_storage_File_initStorageRoot() {
    char* storageRoot;

    KNI_StartHandles(1);
    KNI_DeclareHandle(string);

    if ((storageRoot = getStorageRoot()) != NULL) {
        KNI_NewStringUTF(storageRoot, string);
    } else {
        KNI_ReleaseHandle(string);
    }

    KNI_EndHandlesAndReturnObject(string);
}

/*
 * static boolean storageExists(byte[] szAsciiFilename)
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_io_j2me_storage_File_storageExists() {
    char* pszAsciiFilename;
    int   nameLen;
    int   ret;

    KNI_StartHandles(1);
    KNI_DeclareHandle(filename);

    KNI_GetParameterAsObject(1, filename);
    nameLen = KNI_GetArrayLength(filename);
    pszAsciiFilename = (char*)midpMalloc(nameLen + 1);

    if (pszAsciiFilename == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetRawArrayRegion(filename, 0, nameLen, (jbyte*)pszAsciiFilename);
        pszAsciiFilename[nameLen] = '\0';

        ret = storageFileExists(pszAsciiFilename);

        midpFree(pszAsciiFilename);
    }

    KNI_EndHandles();
    KNI_ReturnBoolean((jboolean)ret);
}

/*
 * static void renameStorage(byte[] szOldFilename, byte[] szNewFilename)
 *     throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_storage_File_renameStorage() {
    char* pszOldFilename;
    int   oldNameLen;
    char* pszNewFilename;
    int   newNameLen;
    char* pszError;

    KNI_StartHandles(2);
    KNI_DeclareHandle(oldFilename);
    KNI_DeclareHandle(newFilename);

    KNI_GetParameterAsObject(1, oldFilename);
    KNI_GetParameterAsObject(2, newFilename);

    oldNameLen = KNI_GetArrayLength(oldFilename);
    newNameLen = KNI_GetArrayLength(newFilename);

    pszOldFilename = (char*)midpMalloc(oldNameLen + 1);
    pszNewFilename = (char*)midpMalloc(newNameLen + 1);

    if ((pszOldFilename == NULL) || (pszNewFilename == NULL)) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetRawArrayRegion(oldFilename, 0, oldNameLen, 
                              (jbyte*)pszOldFilename);
        KNI_GetRawArrayRegion(newFilename, 0, newNameLen, 
                              (jbyte*)pszNewFilename);
        pszOldFilename[oldNameLen] = '\0';
        pszNewFilename[newNameLen] = '\0';

        storageRenameFile(&pszError, pszOldFilename, pszNewFilename);


        if (pszError != NULL) {
            KNI_ThrowNew("java/io/IOException", pszError);
            storageFreeError(pszError);
        }
    }

    midpFree(pszOldFilename);
    midpFree(pszNewFilename);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*
 * static void deleteStorage(byte[] szAsciiFilename)
 *     throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_storage_File_deleteStorage() {
    char* pszAsciiFilename;
    char* pszError;
    int   nameLen;

    KNI_StartHandles(1);
    KNI_DeclareHandle(filename);

    KNI_GetParameterAsObject(1, filename);
    nameLen = KNI_GetArrayLength(filename);
    pszAsciiFilename = (char*)midpMalloc(nameLen + 1);

    if (pszAsciiFilename == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetRawArrayRegion(filename, 0, nameLen, (jbyte*)pszAsciiFilename);
        pszAsciiFilename[nameLen] = '\0';

        storageDeleteFile(&pszError, pszAsciiFilename);

        midpFree(pszAsciiFilename);

        if (pszError != NULL) {
            KNI_ThrowNew("java/io/IOException", pszError);
            storageFreeError(pszError);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*
 * static int availableStorage(byte[] szAsciiFilename)
 *     throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_storage_File_availableStorage() {
    KNI_ReturnInt((jint)storageGetFreeSpace());
}

/*
 * static String getFirstFileThatStartsWith(byte[])
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_io_j2me_storage_File_getFirstFileThatStartsWith() {
    char* pszRoot;
    char* match;
    int   rootLen;
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(root);
    KNI_DeclareHandle(str);
    
    KNI_GetParameterAsObject(1, root);
    
    rootLen = KNI_GetArrayLength(root);
    pszRoot = (char*)midpMalloc(rootLen + 1);

    if (pszRoot == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetRawArrayRegion(root, 0, rootLen, (jbyte*)pszRoot);
        pszRoot[rootLen] = '\0';

        match = getFirstFileThatStartsWith(pszRoot);
        if (match != NULL) {
	        KNI_NewStringUTF(match, str);
        } else {
            KNI_ReleaseHandle(str);
        }
        midpFree(pszRoot);
    
    }
    KNI_EndHandlesAndReturnObject(str);
}

/*
 * static String getNextFileThatStartsWith(byte[])
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_io_j2me_storage_File_getNextFileThatStartsWith() {
    char* pszRoot;
    char* match;
    int   rootLen;
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(root);
    KNI_DeclareHandle(str);
    
    KNI_GetParameterAsObject(1, root);
    
    rootLen = KNI_GetArrayLength(root);
    pszRoot = (char*)midpMalloc(rootLen + 1);

    if (pszRoot == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {

        KNI_GetRawArrayRegion(root, 0, rootLen, (jbyte*)pszRoot);
        pszRoot[rootLen] = '\0';

        match = getNextFileThatStartsWith(pszRoot);
        if (match != NULL) {
	        KNI_NewStringUTF(match, str);
        } else {
            KNI_ReleaseHandle(str);
        }
        midpFree(pszRoot);
    }
    
    KNI_EndHandlesAndReturnObject(str);
}

