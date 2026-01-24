/*
 * @(#)localeMethod.c	1.15 02/09/03 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <midpMalloc.h>
#include "defaultLCDUI.h"
#include "conv.h"


/*
 * A locale methods definition.
 */
typedef struct _LcMethodsRec {
    char               *locale;
    char               *encoding;
    LcConvMethods       conv;
} LcMethodsRec, *LcMethods;

static LcMethods lc = NULL;


/*
 * Initialize a LcMethods for the current system locale.
 */
void 
initLocaleMethod()
{
    char *locale = NULL, *encoding = NULL;
    LcConvMethods conv = NULL;
    static int isCalled = 0;

    /* return if it's already called */
    if (isCalled) {
        return;
    }
    /* set isCalled to avoid another call */
    isCalled = 1;

    /* get the current system locale info that midp supports */
    getLocaleInfo(&locale, &encoding);
    /* both locale and encoding have to be returned */
    if (!locale || !encoding) {
        return;
    }

    /* find a converter for the encoding */
    conv = getLcConvMethods(encoding);
    if (!conv) {
        return;
    }

    lc = (LcMethods) midpMalloc(sizeof(LcMethodsRec));
    if (lc == NULL) {
        return;
    }

    lc->locale   = locale;
    lc->encoding = encoding;
    lc->conv     = conv;
}

void 
finalizeLocaleMethod()
{
    if (lc != NULL) {
	midpFree(lc->locale);                /* Allocated in getLocaleInfo() */
#ifdef WIN32
	/* These alloc'd on the heap for Win32 */
	if (lc->conv != NULL) {
	    midpFree(lc->conv->encoding); /* Allocated in getLcConvMethods() */
	    midpFree(lc->conv);           /* Allocated in getLcConvMethods() */
	}
#endif /* WIN32 */
        midpFree(lc);
        lc = NULL;
    }
}

/*
 * Return 1 if the locale support is available
 */
int
isLocaleSupported()
{
    return (lc) ? 1 : 0;
}

/*
 * Return the encoding name.
 */
char *
getLocaleEncoding()
{
    if (lc) {
        return lc->encoding;
    }
    return "ISO8859_1";
}

/*
 * Return the locale name.
 */
char *
getLocaleName()
{
    if (lc) {
        return lc->locale;
    }
    /* The MIDP spec says that the locale name is NULL by default. */
    return NULL;
}


/*
 * Convert the Unicode string into the native represented format.
 */
int 
unicodeToNative(const unicode *ustr, int ulen, unsigned char *bstr, int blen)
{
    if (lc && lc->conv) {
        return lc->conv->unicodeToNative(ustr, ulen, bstr, blen);
    } else {
        ulen = (blen < ulen) ? blen : ulen;
        blen = ulen;
        while (--blen >= 0) {
            bstr[blen] = (char) ustr[blen];
        }
        return ulen;
    }
}

/*
 * Convert the native string into the Unicode format.
 */
int 
nativeToUnicode(const unsigned char *bstr, int blen, unicode *ustr, int ulen)
{
    if (lc && lc->conv) {
        return lc->conv->nativeToUnicode(bstr, blen, ustr, ulen);
    } else {
        blen = (ulen < blen) ? ulen : blen;
        ulen = blen;
        while (--ulen >= 0) {
            ustr[ulen] = (unicode) bstr[ulen];
        }
        return blen;
    }
}
