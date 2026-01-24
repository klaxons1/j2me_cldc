/*
 * @(#)conv.h	1.13 02/07/24 @(#)
 *
 * Copyright (c) 1999-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _CONV_H_INCLUDED
#define _CONV_H_INCLUDED

#include <defaultLCDUI.h>

typedef struct _LcConvMethodsRec {
    char *encoding;
    int (*byteMaxLen)(void);
    int (*byteLen)(const unsigned char *b, int len);
    int (*unicodeToNative)(const unicode *ustr, int ulen, 
                           unsigned char *bstr, int blen);
    int (*nativeToUnicode)(const unsigned char *bstr, int blen,
                           unicode *ustr, int ulen);
    int (*sizeOfByteInUnicode)(const unsigned char *str, int offset, int len);
    int (*sizeOfUnicodeInByte)(const unicode *str, int offset, int len);
} LcConvMethodsRec, *LcConvMethods;

extern LcConvMethods getLcConvMethods(char *encoding);

#endif
