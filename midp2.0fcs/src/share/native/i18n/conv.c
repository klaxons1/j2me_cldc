/*
 * @(#)conv.c	1.25 02/09/23 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <stdlib.h>

#include <kni.h>
#include <midpMalloc.h>
#include "defaultLCDUI.h"
#include "conv.h"

extern LcConvMethods getLcGenConvMethods(char *encoding);

#ifdef UNIX
extern LcConvMethodsRec SJISConvRec;
extern LcConvMethodsRec EUCJPConvRec;

#define NUM_LCCONV 3

static LcConvMethods lcConv[NUM_LCCONV] = {
/* 0 */    &SJISConvRec,
/* 1 */    &EUCJPConvRec,
/* 2 */    NULL, /* depositary for a platform general converter */
};

#else

#define NUM_LCCONV 1
static LcConvMethods lcConv[NUM_LCCONV] = {
/* 0 */    NULL, /* depositary for a platform general converter */
};

#endif /* UNIX */

static int
getLcConvMethodsIDByEncoding(char *encoding) {
    if (encoding && *encoding) {
        int i;
        for (i = 0; i < NUM_LCCONV; i++) {
            if (lcConv[i] == NULL) {
                break;
            }
            if (strcmp(lcConv[i]->encoding, encoding) == 0) {
                return i;
            }
        }
        if (i < NUM_LCCONV) {
            lcConv[i] = getLcGenConvMethods(encoding);
            if (lcConv[i] != NULL) {
                return i;
            }
        }
    }
    return -1;
}

static int
getLcConvMethodsID(unicode *uc, int len) {
    char enc[16];
    int i;
    
    for (i = 0; i < len; i++) {
        enc[i] = (char) uc[i];
    }
    enc[i] = 0;

    return getLcConvMethodsIDByEncoding(enc);
}

/*=========================================================================
 * FUNCTION:      getHandler(Ljava/lang/String;)I (STATIC)
 * CLASS:         com/sun/cldc/i18n/j2me/Conv
 * TYPE:          static native function
 * OVERVIEW:      Get a handle to specific character encoding convesrion
 *                 routine.
 * INTERFACE (operand stack manipulation):
 *   parameters:  character encoding
 *   returns:     identifier for requested handler, or zero if
 *                 the encoding was not supported.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_i18n_j2me_Conv_getHandler() {
    jint     result = 0;

    KNI_StartHandles(1);

    KNI_DeclareHandle(str);
    KNI_GetParameterAsObject(1, str);

    if (!KNI_IsNullHandle(str)) {
        int      strLen = KNI_GetStringLength(str);
	unicode* strBuf;

	/* Instead of always multiplying the length by sizeof(unicode),
	 * we shift left by 1. This can be done because unicode has a
	 * size of 2 bytes.
	 */
	strBuf = (unicode*)midpMalloc(strLen<<1);
	if (strBuf != NULL) { 
	    KNI_GetStringRegion(str, 0, strLen, strBuf);
	    result = getLcConvMethodsID(strBuf, strLen);
	    midpFree(strBuf);
	} else {
	    KNI_ThrowNew("java/lang/OutOfMemoryError","");
	}
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/*=========================================================================
 * FUNCTION:      getMaxByteLength(I)I (STATIC)
 * CLASS:         com/sun/cldc/i18n/j2me/Conv
 * TYPE:          static native function
 * OVERVIEW:      Get the maximum length in bytes for a converted string.
 * INTERFACE (operand stack manipulation):
 *   parameters:  handle returned from getHandler
 *   returns:     maximum byte length, or zero if handler is not valid
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_i18n_j2me_Conv_getMaxByteLength() {
    int id = KNI_GetParameterAsInt(1);

    KNI_ReturnInt(lcConv[id] ? lcConv[id]->byteMaxLen() : 0);
}

/*=========================================================================
 * FUNCTION:      getByteLength(I[BII)I (STATIC)
 * CLASS:         com/sun/cldc/i18n/j2me/Conv
 * TYPE:          static native function
 * OVERVIEW:      Get the length in bytes of a specific string.
 * INTERFACE (operand stack manipulation):
 *   parameters:  handler handle returned from getHandler
 *                b       buffer of bytes to be converted
 *                offset  offset into the provided buffer
 *                len     length of data to be processed
 *   returns:     length of converted string, or zero if arguments
 *                 were not valid
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_i18n_j2me_Conv_getByteLength() {
    int  length = KNI_GetParameterAsInt(4);
    int  offset = KNI_GetParameterAsInt(3);
    int      id = KNI_GetParameterAsInt(1);
    char*   buf;
    jint result;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);
    KNI_GetParameterAsObject(2, b);

    buf = (char*)midpMalloc(length);

    if (buf == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError","");
    } else {

        KNI_GetRawArrayRegion(b, offset, length, (jbyte*)buf);

        result = lcConv[id] 
               ? lcConv[id]->byteLen((const unsigned char *) buf, length) 
               : 0;
        midpFree(buf);
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/*=========================================================================
 * FUNCTION:      byteToChar(I[BII[CII)I (STATIC)
 * CLASS:         com/sun/cldc/i18n/j2me/Conv
 * TYPE:          static native function
 * OVERVIEW:      Convert an array of bytes to converted array of
 *                 characters.
 * INTERFACE (operand stack manipulation):
 *   parameters:  handler    handle returned from getHandler
 *                input      buffer of bytes to be converted
 *                in_offset  offset into the provided buffer
 *                in_len     length of data to be processed
 *                output     buffer of converted bytes
 *                out_offset offset into the provided output buffer
 *                out_len    length of data processed
 *   returns:     length of converted string, or zero if arguments
 *                 were not valid
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_i18n_j2me_Conv_byteToChar() {
    int   outLength = KNI_GetParameterAsInt(7);
    int   outOffset = KNI_GetParameterAsInt(6);
    int    inLength = KNI_GetParameterAsInt(4);
    int    inOffset = KNI_GetParameterAsInt(3);
    int          id = KNI_GetParameterAsInt(1);
    char*     inBuf;
    unicode* outBuf;
    jint     result = 0;

    KNI_StartHandles(2);
    KNI_DeclareHandle(output);
    KNI_DeclareHandle(input);

    KNI_GetParameterAsObject(5, output);
    KNI_GetParameterAsObject(2, input);

    inBuf  = (char*)midpMalloc(inLength);
    if (inBuf != NULL) {
	/* Instead of always multiplying the length by sizeof(unicode),
	 * we shift left by 1. This can be done because unicode has a
	 * size of 2 bytes.
	 */
	outBuf = (unicode*)midpMalloc(outLength<<1);
	if (outBuf != NULL) {
	    KNI_GetRawArrayRegion(input, inOffset, inLength, (jbyte*)inBuf);
	    result = (lcConv[id] ? 
		       lcConv[id]->nativeToUnicode((const unsigned char *)
						   inBuf,  inLength, 
						   outBuf, outLength): 0);
	    KNI_SetRawArrayRegion(output, outOffset<<1, outLength<<1, 
				  (jbyte*)outBuf);

	    midpFree(inBuf);
	    midpFree(outBuf);
	} else {
	    midpFree(inBuf);
	    KNI_ThrowNew("java/lang/OutOfMemoryError","");
	}
    } else {
	KNI_ThrowNew("java/lang/OutOfMemoryError","");
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/*=========================================================================
 * FUNCTION:      charToByte(I[CII[BII)I (STATIC)
 * CLASS:         com/sun/cldc/i18n/j2me/Conv
 * TYPE:          static native function
 * OVERVIEW:      Convert an array of characters to converted array of
 *                 bytes.
 * INTERFACE (operand stack manipulation):
 *   parameters:  handler    handle returned from getHandler
 *                input      buffer of characters to be converted
 *                in_offset  offset into the provided buffer
 *                in_len     length of data to be processed
 *                output     buffer of converted bytes
 *                out_offset offset into the provided output buffer
 *                out_len    length of data processed
 *   returns:     length of converted string, or zero if arguments
 *                 were not valid
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_i18n_j2me_Conv_charToByte() {
    int   outLength = KNI_GetParameterAsInt(7);
    int   outOffset = KNI_GetParameterAsInt(6);
    int    inLength = KNI_GetParameterAsInt(4);
    int    inOffset = KNI_GetParameterAsInt(3);
    int          id = KNI_GetParameterAsInt(1);
    unicode*  inBuf;
    char*    outBuf;
    jint     result = 0;

    KNI_StartHandles(2);
    KNI_DeclareHandle(output);
    KNI_DeclareHandle(input);

    KNI_GetParameterAsObject(5, output);
    KNI_GetParameterAsObject(2, input);

    /* Instead of always multiplying the length by sizeof(unicode),
     * we shift left by 1. This can be done because unicode has a
     * size of 2 bytes.
     */
    inBuf  = (unicode*)midpMalloc(inLength<<1);
    if (inBuf != NULL) {
	outBuf = (char*)midpMalloc(outLength);
	if (outBuf != NULL) {
	    KNI_GetRawArrayRegion(input, inOffset<<1, inLength<<1, 
				  (jbyte*)inBuf);
	    result = (lcConv[id] ? 
			       lcConv[id]->unicodeToNative(inBuf, inLength,
   				        (unsigned char*) outBuf, outLength):0);
	    KNI_SetRawArrayRegion(output, outOffset, outLength,(jbyte*)outBuf);

	    midpFree(inBuf);
	    midpFree(outBuf);
	} else {
	    midpFree(inBuf);
	    KNI_ThrowNew("java/lang/OutOfMemoryError","");
	}
    } else {
	KNI_ThrowNew("java/lang/OutOfMemoryError","");
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/*=========================================================================
 * FUNCTION:      sizeOfByteInUnicode(I[BII)I (STATIC)
 * CLASS:         com/sun/cldc/i18n/j2me/Conv
 * TYPE:          static native function
 * OVERVIEW:      Convert an array of characters to converted array of
 *                 bytes.
 * INTERFACE (operand stack manipulation):
 *   parameters:  handler    handle returned from getHandler
 *                b          buffer of bytes to be converted
 *                offset     offset into the provided buffer
 *                length     length of data to be processed
 *   returns:     length of converted string, or zero if arguments
 *                 were not valid
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_i18n_j2me_Conv_sizeOfByteInUnicode() {
    int   length = KNI_GetParameterAsInt(4);
    int   offset = KNI_GetParameterAsInt(3);
    int       id = KNI_GetParameterAsInt(1);
    char    *buf;
    jint  result;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    KNI_GetParameterAsObject(2, b);
    buf = (char*)midpMalloc(length);

    if (buf == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError","");
    } else {
        KNI_GetRawArrayRegion(b, offset, length, (jbyte*)buf);

        result = lcConv[id] 
               ? lcConv[id]->sizeOfByteInUnicode((const unsigned char *)buf, 
                                                  offset, length) 
               : 0;

        midpFree(buf);
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/*=========================================================================
 * FUNCTION:      sizeOfUnicodeInByte(I[CII)I (STATIC)
 * CLASS:         com/sun/cldc/i18n/j2me/Conv
 * TYPE:          static native function
 * OVERVIEW:      Convert an array of characters to converted array of
 *                 bytes.
 * INTERFACE (operand stack manipulation):
 *   parameters:  handler    handle returned from getHandler
 *                c          buffer of characters to be converted
 *                offset     offset into the provided buffer
 *                length     length of data to be processed
 *   returns:     length of converted string, or zero if arguments
 *                 were not valid
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_i18n_j2me_Conv_sizeOfUnicodeInByte() {
    int   length = KNI_GetParameterAsInt(4);
    int   offset = KNI_GetParameterAsInt(3);
    int       id = KNI_GetParameterAsInt(1);
    unicode *buf;
    jint  result;

    KNI_StartHandles(1);
    KNI_DeclareHandle(c);

    KNI_GetParameterAsObject(2, c);

    /* Instead of always multiplying the length by sizeof(unicode),
     * we shift left by 1. This can be done because unicode has a
     * size of 2 bytes.
     */
    buf = (unicode*)midpMalloc(length<<1);

    if (buf == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError","");
    } else {
        KNI_GetRawArrayRegion(c, offset<<1, length<<1, (jbyte*)buf);

        result = lcConv[id] 
               ? lcConv[id]->sizeOfUnicodeInByte((const unicode *)buf, 
                                                  offset, length) 
               : 0;
        midpFree(buf);
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

#if 0 /* DEBUG_METHOD */
void
Java_com_sun_cldc_i18n_j2me_Conv_println()
{
    INSTANCE  str = popStackAsType(INSTANCE);
    int        id = popStack();

    if (lcConv[id] && str != NULL) {
        SHORTARRAY c  = (SHORTARRAY) (str->data[0].cellp);
        int    offset = str->data[1].cell;
        int    length = str->data[2].cell;
        unicode  *buf = (unicode *) (c->sdata) + offset;
        int    outlen = length * lcConv[id]->byteMaxLen() + 1;
        char     *out = (char *) midpMalloc(outlen);
        int       len = lcConv[id]->unicodeToNative((const unicode *) buf, 
                                                    length,
                                                    (unsigned char*) out, 
                                                    outlen);
        out[len] = 0;
        printf("%s\n", out);
    }
}
#endif

LcConvMethods
getLcConvMethods(char *encoding)
{
    int index = getLcConvMethodsIDByEncoding(encoding);

    if (index != -1) {
        return lcConv[index];
    }

    return NULL;
}
