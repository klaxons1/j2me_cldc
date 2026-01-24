/*
 * @(#)images.c	1.53 02/11/07 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <stdio.h>

#define NeedFunctionPrototypes 1
#include <kni.h>
#include <defaultLCDUI.h>
#include <nativeGUI.h>
#include <imageDecode.h>
#include <midpMalloc.h>
#include <midpServices.h>
#include <images.h>

typedef struct _mbs {
    Pixmap        pixmap;
    XImage        image;
    unsigned char *imageMask; /* this serves as a bit mask and as an alpha
			       * channel array depending on what kind of
			       * information is in the image. these two
			       * properties are mutually exclusive so this
                               * is ok
                               */
    jint           width;
    jint           height;
    jint           mutable;
    jbyte          prop;
} myBitmapStruct;

typedef struct _imgDst {
    imageDstData   super;
    myBitmapStruct *bitmap;
    jint            cmap[256];
    unsigned char  tmap[256];
    jint            dataSize;
    jboolean       mutable;
    jboolean       hasColormap;
    jboolean       hasTransMap;
} _imageDstData, *_imageDstPtr;


/* REMIND: this is 3:3:2.  Consider 6x6x6 + 16 grays instead. */
#define GETRGBPIXEL(r, g, b) \
    rgbTable[((r & 0xe0) >> 0) | ((g & 0xe0) >> 3) | ((b & 0xc0) >> 6)].pixel


/* 
 * getPixelParts scales 16 bit xcolor componets for red, green, blue
 * to 8 bit values.  Thus the red/green/blue values returned in
 * xcolor correspond to 8 bit rgb values of the color displayed.
 */
static void
getPixelParts(long pixel, XColor *xcolor)
{
    XColor *table;
    int i;

    switch (numColors) {
        case 2: 
        case 4:
        case 16:
            table = grayTable;
            break;
        default:
            table = rgbTable;
            break;

    }

    for (i = 0; i < numColors; i++) {
        if (pixel == table[i].pixel) {
            switch (numColors) {
                case 2:
                    xcolor->red   = grayTable[i].red >> 7;
                    xcolor->green = xcolor->red;
                    xcolor->blue  = xcolor->red;
                    return;
                case 4:
                    xcolor->red   = grayTable[i].red >> 6;
                    xcolor->green = xcolor->red;
                    xcolor->blue  = xcolor->red;
                    return;
                case 16:
                    xcolor->red   = grayTable[i].red >> 4;
                    xcolor->green = xcolor->red;
                    xcolor->blue  = xcolor->red;
                    return;
                default:
                    xcolor->red   = (rgbTable[i].red   >> 8) & 0xe0;
                    xcolor->green = (rgbTable[i].green >> 8) & 0xe0;
                    xcolor->blue  = (rgbTable[i].blue  >> 8) & 0xc0;
                    return;
            }
        }
    }

    xcolor->red = xcolor->green = xcolor->blue = 0;
}

static int
getPixelValue(int r, int g, int b)
{
    switch (numColors) {
        case  2: return grayTable[r >> 7].pixel;
        case  4: return grayTable[r >> 6].pixel;
        case 16: return grayTable[r >> 4].pixel;
        default: return GETRGBPIXEL(r, g, b);
    }
}

static int
getColorPixel(int rgb, int gray, int isGray)
{
    int r = (rgb >> 16) & 0xff;
    int g = (rgb >> 8)  & 0xff;
    int b = (rgb >> 0)  & 0xff;

    return GETRGBPIXEL(r, g, b);
}

static int
getGrayPixel(int rgb, int gray, int isGray)
{
    switch (numColors) {
    case  2: return grayTable[gray >> 7].pixel;
    case  4: return grayTable[gray >> 6].pixel;
    case 16: return grayTable[gray >> 4].pixel;
    default:
        fprintf(stderr, "Unsupported number of gray levels -- %d\n",
                numColors);
    }
    return 0;
}

#define NTSC(r, g, b) ((r*78 + g*150 + b*29) >> 8)

int
LCDUIgetPixel(int rgb, int gray, int isGray) {
    return numColors < 256 ? getGrayPixel(rgb, gray, isGray) : 
                             getColorPixel(rgb, gray, isGray);
}

int
LCDUIgetDisplayColor(int color) {

    int r = (color >> 16) & 0xff;
    int g = (color >> 8)  & 0xff;
    int b = (color >> 0)  & 0xff;
    int pix;

    pix = getPixelValue(r, g, b);

    /* the returned values are X's Xcolor.pixel
       stored as BGR */
    b = (pix >> 16) & 0xff;
    g = (pix >> 8)  & 0xff;
    r = (pix >> 0)  & 0xff;
 
    pix = ((r << 16) | (g << 8) | b)
          & 0xFFFFFF;

    return (pix);
}

/* 
 * fill rgbBuffer with pixels in 0xAARRGGBB format 
 * assume that (*rgbBuffer) can move, as it is on the java heap
 *
 * implements this function:
 *   rgbData[offset + (a - x) + (b - y) * scanlength] = P(a, b)
 *       for
 *   x <= a < x + width
 *   y <= b < y + height
 */
void 
LCDUIgetRGB(int** rgbBuffer, int offset, int scanLength,
	    int x, int y, int width, int height, void *img)
{
    int curX;
    int curY;
    int curOffset = offset; /* current offset in output array */
    int maskOffset;
    unsigned char alpha;
    /*
     * unsigned char red;
     * unsigned char green;
     * unsigned char blue; 
     */
    int argb;
    int _argb;

    char alphaFlag = 0;    /* zero if we don't need to process 
			      alpha channel values */

    XColor color;
    Pixmap *pmap;
    XImage *image;
    
    myBitmapStruct *p = (myBitmapStruct *)img;
    pmap = &p->pixmap;
    image = &p->image;

    if (img == NULL) return;

    if (p->prop == HAS_ALPHA && p->imageMask != NULL)
	alphaFlag = 1;

    if (p->mutable) {
        int w = p->width, h = p->height;
        image = XGetImage(display, *pmap, 0, 0, w, h, AllPlanes, ZPixmap);
    }

    for (curY = y; curY < y + height; curY++) {
	for (curX = x; curX < x + width; curX++) {
	  _argb	= XGetPixel(image, curX, curY);

		color.blue  = (_argb >> 16) & 0xff;
		color.green = (_argb >> 8) & 0xff;
		color.red   = _argb & 0xff;

	    if (alphaFlag) {
		maskOffset = curY * p->width + curX;
		alpha = (unsigned char) *(p->imageMask + maskOffset);
	    } else { 
		alpha = 255; /* default fully opaque */
	    }
	    argb = (alpha << 24) | (color.red << 16) |
		(color.green << 8) | color.blue;
	    (*rgbBuffer)[curOffset] = argb;
	    curOffset++;
	}
	curOffset += (scanLength - width);
    }
    if (p->mutable) {
        XDestroyImage(image);
    }
}

static void
setImageColormap(imageDstPtr self, long *map, int length)
{
    _imageDstPtr p = (_imageDstPtr)self;

    p->hasColormap = KNI_TRUE;
    memcpy(p->cmap, map, length * sizeof(long));

}

static void
setImageTransparencyMap(imageDstPtr self, unsigned char *map, 
                        int length, int palLength)
{
    _imageDstPtr p = (_imageDstPtr)self->ptr;

    /* we must have a palette, so assume that everything is opaque */
    if (palLength >= 0 ) { 
        memset(p->tmap, 0xFF, palLength);
    }

    memcpy(p->tmap, map, length);
    p->hasTransMap = KNI_TRUE;
}

static XPixmapFormatValues *
getPixmapFormat(int bpp) {
    static XPixmapFormatValues *pixmapFormats = NULL;
    static int numFormats = 0;

    int i;

    if (pixmapFormats == NULL) {
	pixmapFormats = XListPixmapFormats(display, &numFormats);
    }

    if (bpp > 1) {
	bpp = DefaultDepthOfScreen(screen);
    }

    for (i = 0; i < numFormats; ++i) {
	if (bpp == pixmapFormats[i].depth) {
	    return pixmapFormats + i;
	}
    }

    return NULL;
}

static void
setImageSize(imageDstPtr self, int width, int height)
{
    /* REMIND: we're not allowing for transparency here. */
    /* REMIND: yes, we are :) */

    int x, y;

    XImage *img;
    _imageDstPtr p = (_imageDstPtr)self;

    int depth = DefaultDepthOfScreen(screen);
    XPixmapFormatValues *format = getPixmapFormat(depth);

    int bytesPerPixel = format->bits_per_pixel >> 3;
    int bytesPerLine  = (width*bytesPerPixel + 3) & ~3;

    p->dataSize = sizeof(XImage) + sizeof(Pixmap);

    if (! p->mutable) {
        p->dataSize += bytesPerLine*height;
    }

    p->bitmap->width = width;
    p->bitmap->height = height;
    p->bitmap->mutable = p->mutable;

    p->bitmap->imageMask = NULL;
    p->bitmap->prop = HAS_SOLID;

    img = &p->bitmap->image;

    if (p->mutable) {
        p->bitmap->pixmap = XCreatePixmap(display, paintWindow,
                                 width, height, 
                                 visualDepth);
	XSetClipMask(display, gc, None);
        XSetForeground(display, gc, getPixelValue(0xff, 0xff, 0xff));
        XFillRectangle(display, p->bitmap->pixmap, gc, 0, 0, width, height);
        img->width    = width;
        img->height   = height;
        return;
    }

    img->width            = width;
    img->height           = height;
    img->xoffset          = 0;
    img->format           = ZPixmap;
    img->byte_order       = ImageByteOrder(display);
    img->bitmap_unit      = BitmapUnit(display);
    img->bitmap_bit_order = BitmapBitOrder(display);
    img->depth            = format->depth;
    img->bits_per_pixel   = format->bits_per_pixel;
    img->bitmap_pad       = format->scanline_pad;
    img->bytes_per_line   = bytesPerLine;
    img->data             = (char *)midpMalloc(bytesPerLine * height);

    if (!XInitImage(img)) {
        fprintf(stderr, "XInitImage returned zero\n");
        fprintf(stderr, "width            = %d\n", img->width);
        fprintf(stderr, "height           = %d\n", img->height);
        fprintf(stderr, "xoffset          = %d\n", img->xoffset);
        fprintf(stderr, "format           = %d\n", img->format);
        fprintf(stderr, "byte_order       = %d\n", img->byte_order);
        fprintf(stderr, "bitmap_unit      = %d\n", img->bitmap_unit);
        fprintf(stderr, "bitmap_bit_order = %d\n", img->bitmap_bit_order);
        fprintf(stderr, "depth            = %d\n", img->depth);
        fprintf(stderr, "bits_per_pixel   = %d\n", img->bits_per_pixel);
        fprintf(stderr, "bitmap_pad       = %d\n", img->bitmap_pad);
        fprintf(stderr, "bytes_per_line   = %d\n", img->bytes_per_line);
    }

    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            XPutPixel(img, x, y, lightPixel.pixel);
        }
    }

    return;
}

static void
copyImagePixels(imageDstPtr self, void *sourceData)
{
    int x, y;
    _imageDstPtr p = (_imageDstPtr)self;


    myBitmapStruct *bitmap = (myBitmapStruct *)sourceData;
    Pixmap *source = &bitmap->pixmap;
    XImage *srcImg = &bitmap->image;
    
#if 0
    if (*source == None) {
        /* we're copying an immutable image! */
    } else {
        int w = srcImg->width, h = srcImg->height;
        srcImg = XGetImage(display, *source, 0, 0, w, h, AllPlanes, ZPixmap);
    }
#endif

    int w = srcImg->width, h = srcImg->height;

    if (bitmap->mutable) {
        srcImg = XGetImage(display, *source, 0, 0, w, h, AllPlanes, ZPixmap);
    }

    /* memcpy seems like it would be the right idea, but if srcImg doesn't */
    /* exactly match p->image, we would end up with problems.  So we do it */
    /* the slow way instead.                                               */
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            XPutPixel(&p->bitmap->image, x, y, XGetPixel(srcImg, x, y));
        }
    }

    if (bitmap->mutable) {
        XDestroyImage(srcImg);
    }

#if 0
    if (*source != None) {
        XDestroyImage(srcImg);
    }
#endif

}


static void
copyPixelsTransform(imageDstPtr self, void *sourceData,
		    int nXOriginSrc, int nYOriginSrc,
		    int nWidth, int nHeight,
		    int transform)
{
  _imageDstPtr destP     = (_imageDstPtr)self;
  myBitmapStruct *sourceBitmap = (myBitmapStruct *)sourceData;

  /* --- */

  int imgLen;

  int srcX;
  int srcY;
  int xStart;
  int yStart;
  int xIncr;
  int yIncr;
  int destX;
  int destY;
  int yCounter;
  int xCounter;
    
  int t_width;
  int t_height;

  /*scan length of the source image*/
  int imageWidth;
  /*number of rows of the source image*/
  int imageHeight;

  int srcImgLen;

  /* --- */

  imageWidth = sourceBitmap->width;
  imageHeight = sourceBitmap->height;

  destP->bitmap->prop = HAS_ALPHA;

  /* --- */


  /* width * height * 4 gives us the size of a 32 bpp image */
  imgLen = nWidth * nHeight << 2;
  srcImgLen = imageWidth  * imageHeight << 2;
        
  if(transform & TRANSFORM_INVERTED_AXES) {
    t_width = nHeight;
    t_height = nWidth;
  } else {
    t_width = nWidth;
    t_height = nHeight;
  }

  if (transform & TRANSFORM_Y_FLIP) {
    yStart = nHeight-1;
    yIncr = -1;
  } else {
    yStart = 0;
    yIncr = +1;
  }

  if (transform & TRANSFORM_X_FLIP) {
    xStart = nWidth-1;
    xIncr = -1;
  } else {
    xStart = 0;
    xIncr = +1;
  }

  /* increment srcX,Y regular. increment destX,Y according to transform.
     this makes handling of mask and alpha values easier */

  for (srcY = nYOriginSrc, destY = yStart, yCounter = 0; 
       yCounter < nHeight; 
       srcY++, destY+=yIncr, yCounter++) {

    /* in the current implementation we have source bitmap
       dimension as the width of the image and the height of the region
       destination bitmap is of the dimensions of the region */
    
    for (srcX = nXOriginSrc, destX = xStart, xCounter = 0; 
	 xCounter < nWidth; 
	 srcX++, destX+=xIncr, xCounter++) {

      if ( transform & TRANSFORM_INVERTED_AXES ) {

      /* copy the pixel that is pointed to */
      XPutPixel(&destP->bitmap->image, destY, destX, 
		XGetPixel(&sourceBitmap->image, srcX, srcY));

      } else {

      /* copy the pixel that is pointed to */
      XPutPixel(&destP->bitmap->image, destX, destY, 
		XGetPixel(&sourceBitmap->image, srcX, srcY));

      }

    } /*for x*/

  } /* for y */

    /* --- */

}


static void*
imageDone(imageDstPtr self, int *width, int *height)
{
    _imageDstPtr p = (_imageDstPtr)self;

    if (p == NULL) return;

    if ((width != NULL) && (height != NULL)) {
        *width = p->bitmap->width;
        *height = p->bitmap->height;
    }

    return (void*)p->bitmap;
}

/*
 * Dither matrices.
 *
 * canonical matrix is
 *  { 0,  6,  9, 15},
 *  {11, 13,  2,  4},
 *  { 7,  1, 14,  8},
 *  {12, 10,  5,  3}
 *
 * or, converted to signed:
 *
 *  {-8, -2,  1,  7},
 *  { 3,  5, -6, -4},
 *  {-1, -7,  6,  0},
 *  { 4,  2, -3, -5}
 *
 */

static signed char cmat[4][4] = {
    {-24,  -6,   3,  21},
    {  9,  15, -18, -12},
    { -3, -21,  18,   0},
    { 12,   6,  -9, -15}
};



static void
blendPixel(int *r, int *g, int *b, int *alpha, int off)
{
#if USE_ALPHA_BLEND

    if (*alpha < TRANS_THRESHOLD) {
        *alpha = 0;
    }
#if TRANS_BINARY_THRESHOLD
    else {
        *alpha = 0xff;
    }
#endif

    *r = (*r * *alpha) / 0xff;
    *g = (*g * *alpha) / 0xff;
    *b = (*b * *alpha) / 0xff;

#else /* USE_ALPHA_BLEND */

    /*
     * if we're not using alphablending then dither the alpha
     * values so we can approximate a blending effect
     */
    if ((*alpha != 0x00) && (*alpha != 0xff)) {
        *alpha += off * 3;
        if (*alpha < TRANS_THRESHOLD) {
            *alpha = 0;
        } else {
            *alpha = 0xff;
        }
    }
#endif

}

/*
 * build a bit mask for dealing with transparent images that either
 * only have pixels that are on or off or are dithering an alpha blend
 *
 */
static void
buildMask(myBitmapStruct *bitmap, int alpha, int x, int y)
{
    /* the mask must be 8-bit aligned for myPtImageMask */
    int width = bitmap->width + (8 - (bitmap->width & 7));

    if (bitmap->imageMask == NULL) {
        int len = (width >> 3) * bitmap->height;
        bitmap->imageMask = (unsigned char *)midpCalloc(len, sizeof(char));
    }

    if (bitmap->imageMask != NULL) {
        /* 
         * build the image mask
         */ 
        int offset = (y * width + x) >> 3;
        *(bitmap->imageMask + offset) |= (alpha & 1) << (7 - (x & 7));
    }
}

static void
buildAlphaChannel(myBitmapStruct *bitmap, int alpha, int x, int y) {
 
    if (bitmap->imageMask == NULL) {
        int len = bitmap->width * bitmap->height;
        bitmap->imageMask = (unsigned char *)midpCalloc(len, sizeof(char));
    }

    if (bitmap->imageMask != NULL) {
        int offset = y * bitmap->width + x;
        *(bitmap->imageMask + offset) = (unsigned char)(alpha & 0xff);
    }
}

static void 
setARGBPixels(imageDstPtr self, int** imageBuf, int bufLen, int width,
	      int height, jboolean useAlpha)
{
    int x;
    int y; 
    int offset;
    int pixel; 
    int alpha;
    int r;
    int g;
    int b;
    
    _imageDstPtr p = (_imageDstPtr)self;
    
    if ((p->bitmap == NULL) || (p->mutable)){
	fprintf(stderr, "setARGBPixel error\n");
	return;    
    }
    
    if (useAlpha) {
	p->bitmap->prop = HAS_ALPHA;
    }
    for (y = 0; y < height; y++){
	for (x = 0; x < width; x++){
	    offset = (y * width) + x;
	    pixel = (*imageBuf)[offset];
	    
	    alpha = (pixel >> 24) & 0xff;
	    r = (pixel >> 16) & 0xff;
	    g = (pixel >> 8 ) & 0xff;
	    b = pixel & 0xff;
	    
	    XPutPixel(&p->bitmap->image, x, y, getPixelValue(r, g, b));
	    
	    if (useAlpha){ 
		buildAlphaChannel(p->bitmap, alpha, x, y);
	    }
	}
    }
}

static void
sendPixelsColor(imageDstPtr self, int y, uchar *pixels, int pixelType)
{
    int x;
    signed char *mat = cmat[y & 3];
    _imageDstPtr p = (_imageDstPtr)self;

    if ((p->bitmap == NULL) || (p->mutable)) return;

    if ((pixelType == CT_COLOR) ||              /* color triplet */
        (pixelType == (CT_COLOR | CT_ALPHA))) { /* color triplet with alpha */
        for (x = 0; x < p->bitmap->width; ++x) {
            int off = mat[x & 3];
            int r = pixels[0];
            int g = pixels[1];
            int b = pixels[2];
            int alpha = 0xff;

            if (pixelType & CT_ALPHA) {
                alpha = pixels[3];
                p->bitmap->prop = HAS_ALPHA;
                pixels++;
            } else if (p->hasTransMap) {

                alpha = pixels[3];
                p->bitmap->prop = HAS_ALPHA;
                pixels++;

            }
            pixels += 3;
 
            r += off;
            g += off;
            b += off;

            if (r < 0) r = 0; else if (r > 0xff) r = 0xff;
            if (g < 0) g = 0; else if (g > 0xff) g = 0xff;
            if (b < 0) b = 0; else if (b > 0xff) b = 0xff;

            if (p->bitmap->prop == HAS_ALPHA) {
                blendPixel(&r, &g, &b, &alpha, off);
            }

#if USE_ALPHA_BLEND
            if (p->bitmap->prop == HAS_MASK) {
                buildMask(p->bitmap, alpha, x, y);
            } else if (p->bitmap->prop == HAS_ALPHA) {
                buildAlphaChannel(p->bitmap, alpha, x, y);
            }
#else
            if (p->bitmap->prop == HAS_MASK ||
                p->bitmap->prop == HAS_ALPHA) {

                buildMask(p->bitmap, alpha, x, y);
            } 
#endif

            XPutPixel(&p->bitmap->image, x, y, getPixelValue(r, g, b));
        }
    } else { /* indexed color */
        for (x = 0; x < p->bitmap->width; ++x) {
            int off = mat[x & 3];
            int cmapIndex = *pixels++;

            int color = p->cmap[cmapIndex];

            int r = ((color >> 16) & 0xff) + off;
            int g = ((color >>  8) & 0xff) + off;
            int b = ((color >>  0) & 0xff) + off;

            int alpha = 0xff;

            if (r < 0) r = 0; else if (r > 0xff) r = 0xff;
            if (g < 0) g = 0; else if (g > 0xff) g = 0xff;
            if (b < 0) b = 0; else if (b > 0xff) b = 0xff;

            if ((pixelType & (CT_ALPHA | CT_COLOR)) == CT_ALPHA) {
                alpha = *pixels++;
                p->bitmap->prop = HAS_ALPHA;
            } else if (p->hasTransMap) {
                if ((pixelType & CT_COLOR) == 0) { /* grayscale */

                alpha = *pixels++;
                p->bitmap->prop = HAS_ALPHA;

                } else { /* indexed color */

                    alpha = p->tmap[cmapIndex];
                    p->bitmap->prop = HAS_ALPHA;

                }

            }

            if (p->bitmap->prop == HAS_ALPHA) {
                blendPixel(&r, &g, &b, &alpha, off);
            }

#if USE_ALPHA_BLEND
            if (p->bitmap->prop == HAS_MASK) {
                buildMask(p->bitmap, alpha, x, y);
            } else if (p->bitmap->prop == HAS_ALPHA) {
                buildAlphaChannel(p->bitmap, alpha, x, y);
            }
#else
            if (p->bitmap->prop == HAS_MASK ||
                p->bitmap->prop == HAS_ALPHA) {

                buildMask(p->bitmap, alpha, x, y);
            } 
#endif

            XPutPixel(&p->bitmap->image, x, y, getPixelValue(r, g, b));
        }
    } 
}

static void
sendPackedPixelsColor(imageDstPtr self, int y, uchar *pixels)
{
    /* 
     * since we specified a depth of 8, these must be 
     * pixels in 8-bit grayscale.  pngDecode gives us a
     * colormap for that case, so...
     */
    sendPixelsColor(self, y, pixels, KNI_FALSE);
}

imageDstPtr 
LCDUIcreateImageDst(jboolean mutable) {
    _imageDstPtr p = midpMalloc(sizeof(_imageDstData));

    if (p == NULL) {
        return NULL;
    }

    p->super.ptr = p;

    p->super.depth            = 8;
    p->super.setColormap = setImageColormap;
    p->super.setTransMap = setImageTransparencyMap;
    p->super.setSize     = setImageSize;
    p->super.sendPixels       = sendPixelsColor;
    p->super.sendPackedPixels = sendPackedPixelsColor;
    p->super.copyPixels  = copyImagePixels;
    p->super.done        = imageDone;
    p->super.setARGBPixels    = setARGBPixels;
    p->super.copyPixelsTransformed = copyPixelsTransform;

    if ((p->bitmap=(myBitmapStruct*)midpCalloc(sizeof(myBitmapStruct),1)) == NULL) {
        midpFree(p);
	return NULL;
    }
    p->mutable           = mutable;
    p->hasColormap = KNI_FALSE;
    p->hasTransMap = KNI_FALSE;

    return (imageDstPtr)p;
}

#define XDRAWPOINT(dx) \
    XPutPixel(dest, x + dx, y, XGetPixel(image, x + dx, y));
    
static void
myPutImageMask(Display *display, Drawable d, GC gc, XImage *image, 
	       unsigned char *mask, int src_x, int src_y, int dest_x, 
	       int dest_y, unsigned int width, unsigned int height)
{
    int i, x, y;
    unsigned char data;
    unsigned char *maskPtr = mask;

    XImage *dest;

    dest = XGetImage(display, d, dest_x, dest_y, width, height, 0xff, ZPixmap);

    if (dest == NULL || mask == NULL) {
        XPutImage(display, d, gc, image, src_x, src_y, dest_x, dest_y,
                  width, height);
	if (dest != NULL) {
	    XDestroyImage(dest);
	}
        return;
    }

    /* it won't matter if the new width is larger than the actual bitmap 
     * since invalid bitmap locations will have 0 in the corresponding
     * mask position and therefore won't be accesed
     */
    width = width + (8 - (width & 7));

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 8) {

            data = *maskPtr++;

            if (data == 0xff) {
                /*
                 * all the bits in the mask are set so just put all the pixels
                 */
                XDRAWPOINT(0);
                XDRAWPOINT(1);
                XDRAWPOINT(2);
                XDRAWPOINT(3);
                XDRAWPOINT(4);
                XDRAWPOINT(5);
                XDRAWPOINT(6);
                XDRAWPOINT(7);
            } else if ((data & 0xf0) == 0xf0) {
                /*
                 * only half of the pixels are definitely set so put those
                 * and then run through the others
                 */
                XDRAWPOINT(0);
                XDRAWPOINT(1);
                XDRAWPOINT(2);
                XDRAWPOINT(3);
                for (i = 4; i < 8; i++) {
                    if (data & (1 << (7-i))) {
                        XDRAWPOINT(i);
                    }
                }
            } else if ((data & 0x0f) == 0x0f) {
                /*
                 * the other half of the pixels are definitely set so 
                 * run through the first set and put the rest
                 */
                for (i = 0; i < 4; i++) {
                    if (data & (1 << (7-i))) {
                        XDRAWPOINT(i);
                    }
                }
                XDRAWPOINT(4);
                XDRAWPOINT(5);
                XDRAWPOINT(6);
                XDRAWPOINT(7);
            } else if (data != 0x00) {
                /*
                 * there's no "nice" pattern that we can take advantage of so
                 * test each pixel
                 */
                for (i = 0; i < 8; i++) {
                    if (data & (1 << (7-i))) {
                        XDRAWPOINT(i);
                    }
                }
            }
        }
    }

    XPutImage(display, d, gc, dest, 0, 0, dest_x, dest_y, width, height);
    XDestroyImage(dest);
}

#define TRANSPARENCYMASK 0x00
#define FULLALPHACHANNEL 0x01

#define TRANSPARENCY_SETUP_SOURCE_MASK()\
tMode = TRANSPARENCYMASK;\
maskPtr = mask;\
inByteMaskIndex = 0;

#define	TRANSPARENCY_SETUP_SOURCE_FULL_ALPHACHANNEL_MASK()\
tMode = FULLALPHACHANNEL;\
maskPtr = mask;

#define TRANSPARENCY_GET_SOURCE_DATA(data, x, y)\
if(TRANSPARENCYMASK == tMode) {\
tMaskByte = ((y * imageWidth)+ x) / 8;\
inByteMaskIndex = ((y * imageWidth)+ x) - (tMaskByte * 8);\
if (0x01 && (*(maskPtr+tMaskByte) >> inByteMaskIndex)) {\
  data = 0xFF;\
} else {\
  data = 0x00;\
}\
} else if(FULLALPHACHANNEL == tMode) { \
tMaskByte = ((y * imageWidth)+ x);\
data = *(maskPtr+tMaskByte);\
}

void
myPutImageAlpha(Display *display, Drawable d, GC gc, XImage *image, 
		unsigned char *mask,
                int src_x, int src_y, 
		int dest_x, int dest_y, 
		unsigned int width, unsigned int height,
		int transform,
		int useBitMask)
{

    int srcX;
    int srcY;
    int xStart;
    int yStart;
    int xIncr;
    int yIncr;
    int destX;
    int destY;
    int yCounter;
    int xCounter;

    unsigned char data;

    unsigned char *maskPtr;
    unsigned int index;
    XColor srcPixel, destPixel;

    unsigned int inByteMaskIndex;

    /* used by the TRANSPARENCY_ macros */
    int tMode;
    int tMaskByte = 0;
    int imageWidth = image->width;

    XImage *dest = NULL;

    if ( !(transform & TRANSFORM_INVERTED_AXES)) {
      dest = XGetImage(display, d, dest_x, dest_y, width, height, 0xff, ZPixmap);
    } else {
      dest = XGetImage(display, d, dest_x, dest_y, height, width, 0xff, ZPixmap);
    }

    if (dest == NULL || mask == NULL) {
      if (transform == 0x0) {
        XPutImage(display, d, gc, image, src_x, src_y, dest_x, dest_y,
                  width, height);
	if (dest != NULL) {
	    XDestroyImage(dest);
	}
        return;
      }
    }

    index = 0;
    maskPtr = mask;

    if (transform & TRANSFORM_Y_FLIP) {
      yStart = height-1;
      yIncr = -1;
    } else {
      yStart = 0;
      yIncr = +1;
    }

    if (transform & TRANSFORM_X_FLIP) {
      xStart = width-1;
      xIncr = -1;
    } else {
      xStart = 0;
      xIncr = +1;
    }

    if ((USE_BIT_MASK == useBitMask) && (mask != NULL)) {
        TRANSPARENCY_SETUP_SOURCE_MASK();
	
    } else {

      if(mask != NULL) {
      /*is this a full alpha channel? if so, then setup our
       transparency value reader to read full alpha values.*/

	TRANSPARENCY_SETUP_SOURCE_FULL_ALPHACHANNEL_MASK();
      }
      
    }


    /* increment srcX,Y regular. increment destX,Y according to transform.
       this makes handling of mask and alpha values easier */

    for (srcY = src_y, destY = yStart, yCounter = 0; 
	 yCounter < height; 
	 srcY++, destY+=yIncr, yCounter++) {

      for (srcX = src_x, destX = xStart, xCounter = 0; 
	   xCounter < width; 
	   srcX++, destX+=xIncr, xCounter++) {

	    /* if the mask is null, act as if we have mask values 
	       of full opacity,
	       otherwise, get the mask value. */

            if ((NO_BIT_MASK == useBitMask) && (NULL == mask)) {
	        data = 0xff;
            } else {
	      TRANSPARENCY_GET_SOURCE_DATA(data, srcX, srcY);
	    }

            if (data == 0xff) {

                /*
                 * fully opaque 
                 */

	      if ( !(transform & TRANSFORM_INVERTED_AXES)) {

                XPutPixel(dest, destX, destY, XGetPixel(image, srcX, srcY));
	      } else {
                XPutPixel(dest, destY, destX, XGetPixel(image, srcX, srcY));
	      }


            } else if (data != 0x00) {


                /*
                 * not transparent so we need to blend the image pixel with
                 * the one on the screen
                 */

                getPixelParts(XGetPixel(image, srcX, srcY), &srcPixel);

		if ( !(transform & TRANSFORM_INVERTED_AXES)) {
		  getPixelParts(XGetPixel(dest, destX, destY), &destPixel);
		} else {
		  getPixelParts(XGetPixel(dest, destY, destX), &destPixel);
		}

                srcPixel.red   += ((destPixel.red   * (0xff - data)) / 0xff);
                srcPixel.green += ((destPixel.green * (0xff - data)) / 0xff);
                srcPixel.blue  += ((destPixel.blue  * (0xff - data)) / 0xff);

                if (srcPixel.red > 0xff) {
                    srcPixel.red = 0xff;
                }

                if (srcPixel.green > 0xff) {
                    srcPixel.green = 0xff;
                }

                if (srcPixel.blue > 0xff) {
                    srcPixel.blue = 0xff;
                }

		if ( !(transform & TRANSFORM_INVERTED_AXES)) {
		  XPutPixel(dest, destX, destY, 
			    getPixelValue(srcPixel.red, 
					  srcPixel.green, 
					  srcPixel.blue)); 
		} else {
		  XPutPixel(dest, destY, destX,
			    getPixelValue(srcPixel.red, 
					  srcPixel.green, 
					  srcPixel.blue)); 
		}

            }
 
        }
    }

    
    /*
     * draw the newly rendered image
     */
    if (!(transform & TRANSFORM_INVERTED_AXES)) {
        XPutImage(display, d, gc, dest, 0, 0, dest_x, dest_y, width, height);
    } else {
        XPutImage(display, d, gc, dest, 0, 0, dest_x, dest_y, height, width);
    }

    if (dest != NULL) {
        XDestroyImage(dest);
    }

}

/*
 * Draw the specified region of the given image data,
 * locating its anchor point at x, y
 */
void
LCDUIdrawRegionTransform(jshort *clip, void *dst, void *img,
                         jint x_dest, jint y_dest, jint anchor,
                         jint x_src, jint y_src,
                         jint srcWidth, jint srcHeight, 
                         jint transform, jboolean isRegion) {

    Drawable drawable = setupGC(gc, 0, clip, dst, 0);

    XImage *image;

    myBitmapStruct *p = (myBitmapStruct *)img;

    if (img == NULL) return;

    image = &p->image;

    if (p->mutable) {
        image = XGetImage(display, p->pixmap, x_src, y_src, 
                           srcWidth, srcHeight, AllPlanes, ZPixmap);

        x_src = y_src = 0;
    }

    switch (p->prop) {
        case HAS_ALPHA: /* notice that this falls through to HAS_MASK
                             * when USE_ALPHA_BLEND == 0 
                             */
#if USE_ALPHA_BLEND 
            myPutImageAlpha(display, drawable, gc, image, p->imageMask, 
                x_src, y_src, x_dest, y_dest, srcWidth, srcHeight,
                transform, NO_BIT_MASK);
            break;
#endif
        case HAS_MASK:
            if (0 == transform) {
                myPutImageMask(display, drawable, gc, image, p->imageMask, 
                    x_src, y_src, x_dest, y_dest, srcWidth, srcHeight);
            } else {
                /* use my putImageAlpha - this also does transforms
                   and processes mask info too!	*/
                myPutImageAlpha(display, drawable, gc, image, p->imageMask,
                    x_src, y_src, x_dest, y_dest, srcWidth, srcHeight,
                    transform, USE_BIT_MASK);
            }
            break;

        default:
            if (0 == transform && isRegion == KNI_FALSE) {
                /* XPutImage is possibly faster than us processing 
                   transformation information and then drawing pixels 
                   individually*/
                XPutImage(display, drawable, gc, image,
                    x_src, y_src, x_dest, y_dest, srcWidth, srcHeight);
            } else {
                /*use my putImageAlpha - this also does transforms.*/
                myPutImageAlpha(display, drawable, gc, image, NULL, 
                    x_src, y_src, x_dest, y_dest, srcWidth, srcHeight,
                    transform, NO_BIT_MASK);
           }
           break;
    }

    if (!doubleBuffer && (drawable == paintDrawable)) {
        refreshPaintWindow(x_dest, y_dest, 
                           x_dest + srcWidth/2, y_dest + srcHeight);
    }

    if (p->mutable) {
        XDestroyImage(image);
    }
}

/*
 * Copy the specified region of the given graphics object to a new
 * destination, locating its anchor point at x, y.
 */
void 
LCDUIcopyArea(short *clip, void *dst, int x_src, int y_src, 
	      int width, int height, int x_dest, int y_dest, 
	      int anchor) {

    Drawable drawable = setupGC(gc, 0, clip, dst, 0);

    switch (anchor & (LEFT | RIGHT | HCENTER)) {
    default:
    case LEFT:
        break;

    case RIGHT:
        x_dest -= width;
        break;

    case HCENTER:
        x_dest -= width >> 1;
        break;
    }

    switch (anchor & (TOP | BOTTOM | VCENTER)) {
    default:
    case TOP:
        break;

    case BOTTOM:
        y_dest -= height;
        break;

    case VCENTER:
        y_dest -= height >> 1;
        break;
    }

    XCopyArea(display, drawable, drawable, gc, x_src, y_src,
	      width, height, x_dest, y_dest);

}

/*
 * Draw the given image data input as an array of 0xAARRGGBB integers,
 * locating its anchor point at x, y
 */
void LCDUIdrawRGB(jshort *clip, void *dst,
		  jint **rgbData, jint offset, jint scanlen, jint x, jint y, 
		  jint width, jint height, jboolean processAlpha) {

    jint i;
    jint j;
    jint argb;
    jint alpha;
    jint r;
    jint g;
    jint b;
    XColor destPixel;

    Drawable drawable = setupGC(gc, 0, clip, dst, 0);

    XImage *dest = NULL;

    dest = XGetImage(display, drawable, x, y, width, height, 
			      AllPlanes, ZPixmap);

    if (dest == NULL) {
	fprintf(stderr, "drawRGB dest is null - abort drawRGB\n");
	return;
    }


    for (j = 0; j < height; j++) {
      for (i = 0; i < width; i++){
	    argb = (*rgbData)[offset + (j*scanlen) + i];
	    alpha = (argb >> 24) & 0xff;
	    r = (argb >> 16) & 0xff;
	    g = (argb >> 8) & 0xff;
	    b = argb & 0xff;

            if (alpha == 0xff || processAlpha == KNI_FALSE) {		
                /*
                 * fully opaque 
		 */
	      XPutPixel(dest, i, j, getPixelValue(r,g,b));
            } else if (alpha != 0x00) {
	      
                unsigned long argbdest;
                /*
                 * not transparent so we need to blend the image pixel with
                 * the one on the screen
                 */

		argbdest = XGetPixel(dest, i, j);

		destPixel.blue  = (argbdest >> 16) & 0xff;
		destPixel.green = (argbdest >> 8) & 0xff;
		destPixel.red   = argbdest & 0xff;
		
                r += ((destPixel.red   * (0xff - alpha)) / 0xff);
                g += ((destPixel.green * (0xff - alpha)) / 0xff);
                b += ((destPixel.blue  * (0xff - alpha)) / 0xff);
		
                if (r > 0xff) {
                    r = 0xff;
                }
		
                if (g > 0xff) {
                    g = 0xff;
                }
		
                if (b > 0xff) {
                    b = 0xff;
                }

                XPutPixel(dest, i, j, 
                          getPixelValue(r, 
                                        g, 
                                        b)); 
	    }
	}
    }

    /*
     * draw the newly rendered image
     */
    XPutImage(display, drawable, gc, dest, 0, 0, x, y, width, height);
    XDestroyImage(dest);

    if (!doubleBuffer && (drawable == paintDrawable)) {
        refreshPaintWindow(x, y, x + width, y + height);
    }
}

void
LCDUIdestroyNativeImage(void* imagePtr) {
    myBitmapStruct *imgData = (myBitmapStruct *)imagePtr;

    if (imgData != NULL) {
        if (imgData->pixmap != None) {
            XFreePixmap(display, imgData->pixmap);
        }

        if (imgData->image.data) {
            midpFree(imgData->image.data);
        }

        if (imgData->imageMask) {
            midpFree(imgData->imageMask);
        }
	midpFree(imagePtr);
    }
}

