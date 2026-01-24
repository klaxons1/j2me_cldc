/*
 * @(#)genConv.c	1.15 02/08/05 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <stdlib.h>
#include <kni.h>
#include <midpMalloc.h>
#include "conv.h"

static int
GENMaxLen(void)
{
    return MB_CUR_MAX;
}

static int
GENLen(const unsigned char *c, int len)
{

    /* int mblen( const char *mbstr, size_t count ); */

    /*_mblen returns 1 or 2, according to whether the multibyte
      character c is one or two bytes long. There is no error return
      for _mbclen. If mbstr is not NULL, mblen returns the length, in
      bytes, of the multibyte character. If mbstr is NULL, or if it
      points to the wide-character null character, mblen returns 0. If
      the object that mbstr points to does not form a valid multibyte
      character within the first count characters, mblen returns
      -1. */

    int r = mblen(c, len);

    if (r == 0) {
        /* if mbstr is NULL or if it points to the wide-character null
           character */
        return 1;
    }

    if (r != -1) {
        return r;
    }

    /* if c is a lead byte of a multibyte string, returns -1. */
    if (isleadbyte(c[0])) {
        return -1;
    }

    /* if c is an invalid character, returns 0. */
    return 0;
}

static int
unicodeToGEN(const unicode *wstr, int ulen, unsigned char *mstr, int slen)
{
    int i = 0, j = 0, k;

    /* int wctomb( char *mbchar, wchar_t wchar ); */

    /* If wctomb converts the wide character to a multibyte character,
       it returns the number of bytes (which is never greater than
       MB_CUR_MAX) in the wide character. If wchar is the
       wide-character null character (L'\0'), wctomb returns 1. If the
       conversion is not possible in the current locale, wctomb
       returns -1. */

    while (i < ulen && j < slen) {
        k = wctomb((mstr ? &mstr[j] : NULL), wstr[i++]);
        if (k == -1) {
            if (mstr) {
                mstr[j] = '?';
            }
            k = 1;
        } else if (k == 0) {
            break;
        }
        j += k;
    }

    return j;
}

static int
GENToUnicode(const unsigned char *mstr, int slen, unicode *wstr, int ulen)
{
    int i = 0, j = 0, k;

    /* int mbtowc( wchar_t *wchar, const char *mbchar, size_t count ); */

    /* If mbchar is not NULL and if the object that mbchar points to
       forms a valid multibyte character, mbtowc returns the length in
       bytes of the multibyte character. If mbchar is NULL or the
       object that it points to is a wide-character null character
       (L'\0'), the function returns 0. If the object that mbchar
       points to does not form a valid multibyte character within the
       first count characters, it returns -1. */

    while (i < slen && j < ulen) {
        k = mblen(&mstr[i], (slen - i));
        if (k == -1) {
            k = 1;
            if (wstr) {
                wstr[j] = 0xFFFD;
            }
        } else if (k == 0) {
            k = 1;
            if (wstr) {
                wstr[j] = 0x0000;
            }
        } else {
            k = mbtowc((wstr ? &wstr[j] : NULL), &mstr[i], (slen - i));
        }
        j++;
        i += k;
    }

    return j;
}

static int
sizeOfGENInUnicode(const unsigned char *mstr, int offset, int len)
{
    if (len > 0) {
        return GENToUnicode(&mstr[offset], len, NULL, len);
    }
    return 0;
}


static int
sizeOfUnicodeInGEN(const unicode *wstr, int offset, int len)
{
    if (len > 0) {
        int num = len * MB_CUR_MAX;
        return unicodeToGEN(&wstr[offset], len, NULL, num);
    }
    return 0;
}
    

static LcConvMethods GENConv;

LcConvMethods
getLcGenConvMethods(char *encoding)
{
    char *dum, *enc;

    if (GENConv && strcmp(encoding, GENConv->encoding) == 0) {
        return GENConv;
    }

    /* make sure that the given encoding is the current platform encoding */
    getLocaleInfo(&dum, &enc);

    if ((enc == NULL) || strcmp(encoding, enc)) {
        return NULL;
    }

    GENConv = (LcConvMethods) midpMalloc(sizeof(LcConvMethodsRec));

    if (GENConv != NULL) {
        GENConv->encoding            = midpStrdup(encoding);

        if (GENConv->encoding == NULL) {
            midpFree(GENConv);
            GENConv = NULL;
        } else {
            GENConv->byteMaxLen          = GENMaxLen;
            GENConv->byteLen             = GENLen;
            GENConv->unicodeToNative     = unicodeToGEN;
            GENConv->nativeToUnicode     = GENToUnicode;
            GENConv->sizeOfByteInUnicode = sizeOfGENInUnicode;
            GENConv->sizeOfUnicodeInByte = sizeOfUnicodeInGEN;
        }
    }

    return GENConv;
}
