/*
 * @(#)JarReader.c	1.7 02/09/24 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <midpMalloc.h>
#include <midpServices.h>

/*=========================================================================
 * FUNCTION:      readJarEntry0([B[B)[B
 * CLASS:         com.sun.midp.midletsuite.JarReader
 * TYPE:          virtual native function
 * OVERVIEW:      Gets the contents of the given entry inside a JAR file.
 * INTERFACE (operand stack manipulation):
 *   parameters:  localJarFilePath   file pathname of the JAR file to
 *                                    read. May be a relative pathname.
 *                localEntryname     name of the entry to return.
 *   returns:     the contents of the given entry as a byte array, or null
 *                it the entry was not found
 *   throws:      IOException: if JAR is corrupt or not found
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_midletsuite_JarReader_readJarEntry0() {
    char *entryName,    *jarName;
    int   entryNameLen,  jarNameLen;
    jboolean thrown = KNI_FALSE;

    KNI_StartHandles(3);
    KNI_DeclareHandle(entryNameStr);
    KNI_DeclareHandle(jarNameStr);
    KNI_DeclareHandle(fileContents);

    KNI_GetParameterAsObject(2, entryNameStr);
    KNI_GetParameterAsObject(1, jarNameStr);

    entryNameLen = KNI_GetArrayLength(entryNameStr);
    entryName = (char*)midpMalloc(entryNameLen + 1);
    if (entryName == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
	thrown = KNI_TRUE;
    }
    KNI_GetRawArrayRegion(entryNameStr, 0, entryNameLen, (jbyte*)entryName);
    entryName[entryNameLen] = '\0';

    jarNameLen = KNI_GetArrayLength(jarNameStr);
    jarName = (char*)midpMalloc(jarNameLen + 1);
    if (jarName == NULL) {
        midpFree(entryName);
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
	thrown = KNI_TRUE;
    }

    if (!thrown) {
	KNI_GetRawArrayRegion(jarNameStr, 0, jarNameLen, (jbyte*)jarName);
	jarName[jarNameLen] = '\0';
	
	if (midp_readJarEntry(jarName, entryName, &fileContents) != KNI_TRUE) {
	    KNI_ThrowNew("java/io/IOException", "JAR not found or corrupt");
	}
	
	midpFree(entryName);
	midpFree(jarName);
    }

    KNI_EndHandlesAndReturnObject(fileContents);
}
