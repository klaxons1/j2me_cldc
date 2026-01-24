/*
 * @(#)imageDecode.c	1.11 02/07/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <stdio.h>
#include <stdlib.h>

#include <kni.h>
#include <midpMalloc.h>
#include <imageDecode.h>

typedef struct _rids {
    imageSrcData super;
    uchar **buf;
    int offset;
    int len, remaining;
} resourceImageSrcData, *resourceImageSrcPtr;

static int
getByte(imageSrcPtr self)
{
    resourceImageSrcPtr r = (resourceImageSrcPtr) self->ptr;

    if (r->remaining < 0) return 0;

    --(r->remaining);
    return (*(r->buf))[r->offset++];
}

static int
getBytes(imageSrcPtr self, uchar *buf, int len)
{
    resourceImageSrcPtr r = (resourceImageSrcPtr) self->ptr;

    if (r->remaining < 0) return 0;

    if (len > r->remaining) len = r->remaining;

    if (buf != NULL) {
        memcpy(buf, &((*(r->buf))[r->offset]), len);
    }

    r->remaining -= len;
    r->offset += len;

    return len;
}

/* The done function is called just before the structure is
 * freed.  It may be called before the image data is completely
 * initialized due to a initialization failure, so the structure may
 * not be cleanly initialized.
 */
static void
done(imageSrcPtr self)
{
}

static void
skip(imageSrcPtr self, int numBytes)
{
    getBytes(self, NULL, numBytes);
}

static void
reset(imageSrcPtr self)
{
    resourceImageSrcPtr p = (resourceImageSrcPtr) self->ptr;

    p->offset = 0;
    p->remaining = p->len;
}
static long
getPos(imageSrcPtr self)
{
    resourceImageSrcPtr p = (resourceImageSrcPtr) self->ptr;

    return p->offset;
}

static void
seek(imageSrcPtr self, long pos)
{
    resourceImageSrcPtr p = (resourceImageSrcPtr) self->ptr;

    p->offset = pos;
    p->remaining = p->len - pos;
}


/*
 * WARNING: We need to pass in a pointer to a pointer for the source
 * data. The source data is actually allocated on the Java heap and
 * is protected by a TEMPORARY_ROOT prior to this call. Because of
 * this, (*data) will be updated every time the data object moves in
 * the Java heap.
 */
imageSrcPtr
createImageSrcFromData(char **data, int len) {
    resourceImageSrcData *p = midpMalloc(sizeof(resourceImageSrcData));

    if (p == NULL) {
	return NULL;
    }

    p->super.ptr      = p;
    p->super.done     = done;
    p->super.reset    = reset;
    p->super.getByte  = getByte;
    p->super.getBytes = getBytes;
    p->super.skip     = skip;
    p->super.seek     = seek;
    p->super.getpos   = getPos;

    p->buf = (uchar **)data;
    p->len = len;
    p->super.reset((imageSrcPtr)p);

    return &(p->super);
}
