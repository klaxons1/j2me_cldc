/*
 * @(#)localeMethod.h	1.9 02/07/24 @(#)
 *
 * Copyright (c) 1999-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _LOCALEMETHOD_H_INCLUDED
#define _LOCALEMETHOD_H_INCLUDED

/*
 * Initialize the locale system.
 */
extern void initLocaleMethod();

/*
 * Finalize the locale system.
 */
extern void finalizeLocaleMethod();

/*
 * Get the locale encoding name.
 */
extern char *getLocaleEncoding();

/*
 * Get the locale name.
 */
extern char *getLocaleName();

/*
 * Convert the Unicode string into the current locale native string.
 */
extern int unicodeToNative(const unicode *ustr, int ulen, 
                           unsigned char *bstr, int blen);

/*
 * Convert the current locale native string into the Unicode string.
 */
extern int nativeToUnicode(const unsigned char *bstr, int blen, 
                           unicode *ustr, int ulen);

#endif /* _LOCALEMETHOD_H_INCLUDED */
