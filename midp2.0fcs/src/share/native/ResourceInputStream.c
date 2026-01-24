/*
 * @(#)ResourceInputStream.c	1.11 02/09/16 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>

#include <kni.h>
#include <midpMalloc.h>
#include <midpServices.h>

static void
resourceCleanup(jobject thisObject) {
    int   fd;

    KNI_StartHandles(1);
    KNI_DeclareHandle(clazz);

    KNI_GetObjectClass(thisObject, clazz);
    fd = KNI_GetIntField(thisObject, KNI_GetFieldID(clazz, "handle", "I"));

    if (fd != -1) {
	midp_closeResourceFile(fd);
	KNI_SetIntField(thisObject, KNI_GetFieldID(clazz, "handle", "I"), -1);
    }

    KNI_EndHandles();
}

 /*=========================================================================
  * FUNCTION:      open([B)I
  * CLASS:         com.sun.midp.io.ResourceInputStream
  * TYPE:          virtual native function
  * OVERVIEW:      Open resource stream
  * INTERFACE (operand stack manipulation):
  *   parameters:  name   name of the resource in CLASSPATH to open
  *   returns:     an integer handle to the resource
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_ResourceInputStream_open() {
    char* pszAsciiFilename;
    int   nameLen;
    int   size;
    int   fd;

    KNI_StartHandles(3);
    KNI_DeclareHandle(filename);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(1, filename);
    KNI_GetParameterAsObject(0, thisClass);

    nameLen = KNI_GetArrayLength(filename);
    pszAsciiFilename = (char*)midpMalloc(nameLen + 1);
    KNI_GetRawArrayRegion(filename, 0, nameLen, (jbyte*)pszAsciiFilename);
    pszAsciiFilename[nameLen] = '\0';

    fd = midp_openResourceFile(pszAsciiFilename, &size);

    midpFree(pszAsciiFilename);

    if (fd == -1) {
	KNI_ThrowNew("java/io/IOException", "");
    } else {
	KNI_GetObjectClass(thisClass, clazz);
        KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "size", "I"), size);
	KNI_registerCleanup(thisClass, resourceCleanup);
    }
    KNI_EndHandles();

    KNI_ReturnInt(fd);
}

 /*=========================================================================
  * FUNCTION:      close(I)V
  * CLASS:         com.sun.midp.io.ResourceInputStream
  * TYPE:          virtual native function
  * OVERVIEW:      Close the specified resource
  * INTERFACE (operand stack manipulation):
  *   parameters:  handle   integer handle to the resource to close
  *   returns:     <nothing>
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_ResourceInputStream_close() {
    int fd = KNI_GetParameterAsInt(1);

    if (fd < 0) {
        KNI_ThrowNew("java/io/IOException", "");
    } else {
	midp_closeResourceFile(fd);
	/* Java code will set the handle to NULL */
    }
    KNI_ReturnVoid();
}


 /*=========================================================================
  * FUNCTION:      read(I)I
  * CLASS:         com.sun.midp.io.ResourceInputStream
  * TYPE:          virtual native function
  * OVERVIEW:      Read an integer from the resource
  * INTERFACE (operand stack manipulation):
  *   parameters:  handle   integer handle to the resource to close
  *   returns:     the next byte in the resource or '-1' if the end
  *                 of the resource has been reached
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_ResourceInputStream_read(void) {
    int fd = KNI_GetParameterAsInt(1);
    int byteRead;

    if (fd == -1) {
        KNI_ThrowNew("java/io/IOException", "");
    } else { 
	byteRead = midp_readResourceFile(fd);
	if (byteRead == -2) {           /* If 'fd' is not a valid descriptor */
	    KNI_ThrowNew("java/io/IOException", "");
	}
    }
    KNI_ReturnInt(byteRead);
}

 /*=========================================================================
  * FUNCTION:      readBytes(I[BIII)I
  * CLASS:         com.sun.midp.io.ResourceInputStream
  * TYPE:          native function
  * OVERVIEW:      Read an array of bytes from the stream
  * INTERFACE (operand stack manipulation):
  *   parameters:  handle   integer handle to the resource to close
  *                b        byte array to read into
  *                pos      position in byte array to start reading into
  *                offset   offset in resource to start reading
  *                length   maximum number of bytes to read
  *   returns:     the number of bytes read
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_ResourceInputStream_readBytes(void) {
    int length  = KNI_GetParameterAsInt(5);
    int offset  = KNI_GetParameterAsInt(4);
    int pos     = KNI_GetParameterAsInt(3);
    int fd      = KNI_GetParameterAsInt(1);
    int numRead = -1;

    if (fd == -1) {
        KNI_ThrowNew("java/io/IOException", "");
    } else {
	KNI_StartHandles(1);
	KNI_DeclareHandle(byteArray);

	KNI_GetParameterAsObject(2, byteArray);

	numRead = midp_readallResourceFile(fd, offset, length, &byteArray, pos);
	KNI_EndHandles();
    }
    KNI_ReturnInt(numRead);
}

/*=========================================================================
  * FUNCTION:      finalize()V
  * CLASS:         com.sun.midp.io.ResourceInputStream
  * TYPE:          virtual native function
  * OVERVIEW:      Cleanup native resources
  * INTERFACE (operand stack manipulation):
  *   parameters:  none
  *   returns:     nothing
  *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_ResourceInputStream_finalize() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    resourceCleanup(thisObject);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

