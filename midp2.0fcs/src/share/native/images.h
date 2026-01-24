/*
 * @(#)images.h	1.1 02/08/20 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _IMAGES_H
#define _IMAGES_H

#define CT_PALETTE  0x01
#define CT_COLOR    0x02
#define CT_ALPHA    0x04

#define USE_ALPHA_BLEND	   1

#define HAS_SOLID   0x00
#define HAS_MASK    0x01
#define HAS_ALPHA   0x02

#define NO_BIT_MASK  0x00
#define USE_BIT_MASK 0x01

#define TRANSFORM_INVERTED_AXES 0x4
#define TRANSFORM_X_FLIP 0x2
#define TRANSFORM_Y_FLIP 0x1

#if USE_ALPHA_BLEND

/* any alpha value below this setting will be treated as completely tranparent.
 * values should range from 0x00 to 0xff inclusive .
 * using value 0x00 will allow for normal, total alphablending when
 * TRANS_BINARY_THRESHOLD is at 0x00.
 */
#define TRANS_THRESHOLD		0x00

/* if this is 0x01 then anything above the TRANS_THRESHOLD
 * will be treated as completely opaque.
 * if this is 0x00 then anything above the TRANS_THRESHOLD
 * will be alphablended
 */
#define TRANS_BINARY_THRESHOLD 0x00

#else

#define TRANS_THRESHOLD		0x80
#define TRANS_BINARY_THRESHOLD 0x01

#endif /* USE_ALPHA_BLEND */

#define CHECK_RETURN(expr) (expr) ? (void)0 : (void)fprintf(stderr, "%s returned error (%s:%d)\n", #expr, __FILE__, __LINE__)

#if 0
#define DB(x) fprintf(stderr, "[%i] ret=%i\n", __LINE__, (x));
#else
#define DB(x) (x);
#endif

#endif /* _IMAGES_H */

