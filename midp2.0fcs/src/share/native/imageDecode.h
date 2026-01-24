/*
 * @(#)imageDecode.h	1.14 02/07/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

typedef unsigned char uchar;

typedef struct _ids {
    void   *ptr;                  /* pointer to device specific image struct */
    void   (*reset)(struct _ids *self);
    long   (*getpos)(struct _ids *self);
    void   (*seek)(struct _ids *self, long pos);
    int    (*getByte)(struct _ids *self);
    int    (*getBytes)(struct _ids *self, uchar *buf, int bufsize);
    void   (*skip)(struct _ids *self, int numBytes);
    void   (*done)(struct _ids *self);
} imageSrcData, *imageSrcPtr;

typedef struct _idd {
    int    depth;
    void   *ptr;                  /* pointer to device specific image struct */
    void   (*setColormap)(struct _idd *self, long *map, int length);
    void   (*setSize)(struct _idd *self, int width, int height);
    void   (*setPixel)(struct _idd *self, int x, int y,
                       uchar a, uchar b, uchar c);
    void   (*sendPixels)(struct _idd *self, int y, uchar *scanline, int rgb);
    void   (*sendPackedPixels)(struct _idd *self, int y, uchar *scanline);
    void   (*copyPixels)(struct _idd *self, void *sourceData);
    void*  (*done)(struct _idd *self, int* width, int* height); 
                                    /* may be called with invalid image data */
    void   (*setTransMap)(struct _idd *self, unsigned char *map, int length, 
			  int palLength);
    void   (*setARGBPixels)(struct _idd *self, int** imageBuf, int bufLen,
			    int width, int height, jboolean useAlpha);
    void   (*copyPixelsTransformed)(struct _idd *self, void *sourceData,
				    int x, int y, int width, int height,
				    int transform);
} imageDstData, *imageDstPtr;

extern imageSrcPtr
createImageSrcFromData(char **data, int len);

extern jboolean
PNGdecodeImage(imageSrcPtr src, imageDstPtr dst);

extern imageDstPtr 
LCDUIcreateImageDst(jboolean mutable);
