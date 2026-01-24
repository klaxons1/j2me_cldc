/*
 * @(#)images.c	1.55 02/10/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <stdio.h>

#include <kni.h>
#include <defaultLCDUI.h>
#include <nativeGUI.h>
#include <imageDecode.h>
#include <midpMalloc.h>
#include <images.h>

typedef struct _mbs {
    HBITMAP        bitmap;
    HBITMAP        mask;
    int            width;
    int            height;
    int            mutable;
    unsigned char *image;      /* 4 bytes per pixel - rgba order */
    unsigned char *imageMask;
    char           prop;
} myBitmapStruct;

typedef struct _imgDst {
    imageDstData    super;
    myBitmapStruct* bitmap;
    int             cmap[256];
    unsigned char   tmap[256];
    jboolean        mutable;
    jboolean        hasColormap;
    jboolean        hasTransMap;
} _imageDstData, *_imageDstPtr;


/*
 * Draw the given image data, locating its anchor point at x, y
 */
void 
LCDUIdrawRegionTransform(jshort *clip, void *dst, void *img,
                         jint x_dest, jint y_dest, jint anchor,
                         jint x_src, jint y_src,
                         jint srcWidth, jint srcHeight, 
                         jint transform, jboolean isRegion) {

    myBitmapStruct     *p = (myBitmapStruct *)img;
    HDC            hdcMem;
    HBITMAP          hbmp;
    int             pixel = 0;              /* makes GRAPHICS_SETUP happy */
    int            dotted = 0;              /* ditto */

    /* FIXME; Can we just use this instead? Then we don't have to define
     *  'pixel' or 'dotted' and we don't generate pens & brushes */
    /*
     * HDC hdc;
     * hdc = getBitmapDC(dst);
     * setupClip(hdc, dst, clip);
     */
    GRAPHICS_SETUP(KNI_FALSE);

    if (img == NULL) {
        return;
    }

    hdcMem = CreateCompatibleDC(hdc);
    CHECK_RETURN(SelectObject(hdcMem, p->bitmap));

    if (transform != 0) {
        customAlphaBlend(hdc, x_dest, y_dest,
            hdcMem, x_src, y_src, srcWidth, srcHeight,
            transform, (p->prop == HAS_ALPHA 
                        || p->prop == HAS_MASK) ? 
                        KNI_TRUE : KNI_FALSE);
    } else {

        switch (p->prop) {
            case HAS_ALPHA: /* notice this falls to HAS_MASK     
                             * when USE_ALPHA_BLEND == 0 
                             */
#if USE_ALPHA_BLEND
                customAlphaBlend(hdc, x_dest, y_dest,
                       hdcMem, x_src, y_src, srcWidth, srcHeight,
                                 transform, KNI_TRUE);
                break;
#endif
            case HAS_MASK:
                {
                    POINT pt[3] = {{x_dest, y_dest}, 
                                   {x_dest + srcWidth, y_dest}, 
                                   {x_dest, y_dest + srcHeight}};
    
                    PlgBlt(hdc, (LPPOINT)&pt, hdcMem, 
                           x_src, y_src, srcWidth, srcHeight, p->mask, 0, 0);
                }
                break;

            default:
                BitBlt(hdc, x_dest, y_dest, srcWidth, srcHeight, 
                    hdcMem, x_src, y_src, SRCCOPY);
                break;
        }
    }

    DeleteDC(hdcMem);

    GRAPHICS_CLEANUP();

    if (!doubleBuffer && (dst == NULL)) {
        if(transform & TRANSFORM_INVERTED_AXES) {
            refreshPaintWindow(y_dest, x_dest, 
                               y_dest + srcHeight, x_dest + srcWidth);
        } else {
            refreshPaintWindow(x_dest, y_dest, 
                               x_dest + srcWidth, y_dest + srcHeight);
        } 
    }

}

/* 
 * Draw an image represented as an ARGB array to the screen.
 * If processAlpha is true, blend source pixel with the existing
 * pixel at the destination location.
 */
void LCDUIdrawRGB(jshort *clip, void *dst,
                  jint **rgbData, jint offset, jint scanlen, jint x, jint y, 
                  jint width, jint height, jboolean processAlpha) {
    
    int             pixel = 0;              /* makes GRAPHICS_SETUP happy */
    int            dotted = 0;              /* ditto */

    int i, j, argb, alpha, r, g, b, c;
    int imageLen;
    unsigned char *destBits;
    unsigned char *destPtr;

    HDC            hdcMem;
    HBITMAP        destHBmp;
    BITMAPINFO     bi;
    HGDIOBJ        oobj;
    
    GRAPHICS_SETUP(KNI_FALSE);
    
    imageLen = width * height << 2;
    
    hdcMem = CreateCompatibleDC(hdc);
    
    bi.bmiHeader.biSize          = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth         = width;
    bi.bmiHeader.biHeight        = -height;
    bi.bmiHeader.biPlanes        = 1;
    bi.bmiHeader.biBitCount      = 32; 
    bi.bmiHeader.biCompression   = BI_RGB;
    bi.bmiHeader.biSizeImage     = imageLen;
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed       = 0;
    bi.bmiHeader.biClrImportant  = 0;
    
        
    destHBmp = CreateDIBSection (hdcMem, &bi, DIB_RGB_COLORS, 
                                 &destBits, NULL, 0);

    if (destBits != NULL) {
        oobj = SelectObject(hdcMem, destHBmp);
        
        /*
         * grab the screen contents into a bitmap
         * the same size as our source image
         */
        DB(BitBlt(hdcMem, 0, 0, width, height, hdc, x, y, SRCCOPY));
        
        SelectObject(hdcMem, oobj);
        
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                argb = (*rgbData)[offset + (j * scanlen)+ i];
                alpha = (argb >> 24) & 0xff;
                r = (argb >> 16) & 0xff;
                g = (argb >> 8) & 0xff;
                b = argb & 0xff;
                
                if (alpha == 0xff || processAlpha == KNI_FALSE) {
                    /* fully opaque - copy pix from rgb array to DIB */
                    destPtr = destBits + ((j * width + i) << 2);
                    *destPtr++ = b; /* dest pixels seem to be in BGRA order */
                    *destPtr++ = g;  
                    *destPtr++ = r;       
                } else if (alpha != 0x00) {
                    /* blend the pixel with the one from the DIB and
                       send the result to the DIB */
                    destPtr = destBits + ((j * width + i) << 2);
                    
                    c = b + ((*destPtr * (0xff - alpha)) / 0xff);
                    if (c > 0xff) c = 0xff;
                    *destPtr++ = (unsigned char)c;
                    
                    c = g + ((*destPtr * (0xff - alpha)) / 0xff);
                    if (c > 0xff) c = 0xff;
                    *destPtr++ = (unsigned char)c;
                    
                    c = r + ((*destPtr * (0xff - alpha)) / 0xff);
                    if (c > 0xff) c = 0xff;
                    *destPtr++ = (unsigned char)c;
                }
            }
        }
    
        SetDIBitsToDevice(hdc, x, y, width, height, 0, 0, 0, height, 
                          destBits, &bi, DIB_RGB_COLORS);
    }

    /* Delete the DIB */
    DeleteObject(destHBmp);
    DeleteDC(hdcMem);
    DeleteObject(oobj);
    
    GRAPHICS_CLEANUP();
    
    if (!doubleBuffer && (dst == NULL)) {
        refreshPaintWindow(x, y, x + width, y + height);
    }
    
}

/*
 * Copy the specified region of the given image data to a new
 * destination, locating its anchor point at x, y.
 */
void 
LCDUIcopyArea(short *clip, void *dst, int x_src, int y_src, 
              int width, int height, int x_dest, int y_dest, 
              int anchor) {

    HDC hdcMem;
    HBITMAP hbmp;
    int pixel = 0;
    int dotted = 0;

    GRAPHICS_SETUP(KNI_TRUE);

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
    
    BitBlt(hdc, x_dest, y_dest, width, height, hdc, x_src, y_src, SRCCOPY);

    GRAPHICS_CLEANUP();
}

/*
 * Given an RGB pixel (color) in 0x00RRGGBB format, return the 
 * actual color displayed in 0x00RRGGBB format.
 */
int
LCDUIgetDisplayColor(int color){
    int cr;  /* in 0x00BBGGRR format */
    int r, g, b;

    cr = makeImagePixel(color);
    r = GetRValue(cr);
    g = GetGValue(cr);
    b = GetBValue(cr);

    return ((r << 16) | (g << 8) | b) & 0xffffff;
}
    

static int
makeImagePixel(int rgb) {
    int r = (rgb >> 16) & 0xff;
    int g = (rgb >> 8)  & 0xff;
    int b = (rgb >> 0)  & 0xff;

    return LCDUIgetPixel(rgb, (r*76 + g*150 + b*29) >> 8, (numColors < 256));
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
    int argb;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char alpha;
    char alphaFlag = 0;    /* zero if we don't need to process 
                              alpha channel values */
    unsigned char *pel;
    
    myBitmapStruct *p = (myBitmapStruct *)img;

    if (img == NULL) return;

    if (p->prop == HAS_ALPHA)
        alphaFlag = 1;

    for (curY = y; curY < y + height; curY++) {
        for (curX = x; curX < x + width; curX++) {

            pel= p->image + ((curY * p->width + curX) << 2);
            
            /*
             * image components in bgra order?
             */
            b = *pel++;
            g = *pel++;
            r = *pel++;
            alpha = 255;  /* assume opaque */

            if (alphaFlag) {
                alpha = *pel;
            } 
            argb = (alpha << 24) | (r << 16) | (g << 8) | b;
            
            (*rgbBuffer)[curOffset] = argb;
            curOffset++;
        }
        curOffset += (scanLength - width);
    }
}

static void
setImageColormap(imageDstPtr self, long *map, int length)
{
    _imageDstPtr p = (_imageDstPtr)self;

    memcpy(p->cmap, map, length * sizeof(long)); 
    p->hasColormap = KNI_TRUE;

}

static void
setImageTransparencyMap(imageDstPtr self, unsigned char *map, int length, int palLength)
{
    _imageDstPtr p = (_imageDstPtr)self;

    if (palLength >= 0 ) { 
        /* we must have a palette, so assume that everything is opaque */
        memset(p->tmap, 0xFF, palLength);
    }

    memcpy(p->tmap, map, length); 
    p->hasTransMap = KNI_TRUE;
}

static void
setImageSize(imageDstPtr self, int width, int height) {
    _imageDstPtr   p = (_imageDstPtr)self;
    int            x, y;
    HDC          hdc; 
    BITMAPINFO    bi;

    hdc = GetDC(hMainWindow);

    p->bitmap->width   = width;
    p->bitmap->height  = height;
    p->bitmap->mutable = p->mutable;
    
    /* to do transparency and alphablending we need to have a 32bpp image
     * regardless of the surface to which we will be drawing. this means
     * that we must create our own bitmap and fill it in appropriately.
     */
    bi.bmiHeader.biSize          = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth         =  p->bitmap->width;
    bi.bmiHeader.biHeight        = -p->bitmap->height;
    bi.bmiHeader.biPlanes        = 1;
    bi.bmiHeader.biCompression   = BI_RGB;
    bi.bmiHeader.biSizeImage     = 0;
    bi.bmiHeader.biBitCount      = 32;
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed       = 0;
    bi.bmiHeader.biClrImportant  = 0;

    p->bitmap->bitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, 
                                         &p->bitmap->image, NULL, 0);

    p->bitmap->mask              = NULL;
    p->bitmap->imageMask         = NULL;
    p->bitmap->prop              = HAS_SOLID;

    ReleaseDC(hMainWindow, hdc);

    if (p->mutable) {
        HPEN     hpen;
        HBRUSH hbrush;

        hdc = getBitmapDC(&p->bitmap->bitmap);

        CHECK_RETURN(hpen   = SelectObject(hdc, whitePen));
        CHECK_RETURN(hbrush = SelectObject(hdc, whiteBrush));
        
        Rectangle(hdc, 0, 0, width, height);

        SelectObject(hdc, hpen);
        SelectObject(hdc, hbrush);

        releaseBitmapDC(hdc);
    }
}

static void
copyImagePixels(imageDstPtr self, void *sourceData)
{
    _imageDstPtr p         = (_imageDstPtr)self;
    myBitmapStruct *source = (myBitmapStruct *)sourceData;
    HDC hdc, hdcMem;

    hdc = GetDC(hMainWindow);
    hdcMem = CreateCompatibleDC(hdc);
    ReleaseDC(hMainWindow, hdc);
    SelectObject(hdcMem, p->bitmap->bitmap);

    DrawBitmap(hdcMem, source->bitmap, 0, 0, SRCCOPY);

    DeleteDC(hdcMem);
}

static void
copyPixelsTransformed(imageDstPtr self, void *sourceData,
                      int nXOriginSrc, int nYOriginSrc,
                      int nWidth, int nHeight,
                      int transform)
{
  _imageDstPtr destP         = (_imageDstPtr)self;
  myBitmapStruct *source = (myBitmapStruct *)sourceData;
  HDC hdc, hdcMem;

  HDC hdcMemDest;


  int imgLen;

  unsigned char *srcBits       = NULL, 
    *destBits      = NULL, 
    *destBitsPtr   = NULL,
    *srcBitsPtr    = NULL;

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

  int srcIndex;
  int destIndex;

  int srcImgLen;


  int nXOriginDest = 0; 
  int nYOriginDest = 0;

  /* --- Win32 specific setup */

  hdc = GetDC(hMainWindow);
  hdcMemDest = CreateCompatibleDC(hdc);
  ReleaseDC(hMainWindow, hdc);
  SelectObject(hdcMemDest, destP->bitmap->bitmap);
  destP->bitmap->prop = HAS_ALPHA;

  /* --- */

  imageWidth = source->width;
  imageHeight = source->height;

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

  srcBits = source->image;
  destBits = destP->bitmap->image;


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


  srcBitsPtr  = srcBits;
  destBitsPtr = destBits;


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

      int alpha = 0;
      
      if ( transform & TRANSFORM_INVERTED_AXES ) {
        destIndex =  ( ( (destX) * t_width) + (destY) );
      } else {
        destIndex =  ( ( (destY) * t_width) + (destX) );
      }

      destBitsPtr =  destBits + (destIndex * 4) ;
      
      srcIndex = (((srcY) * imageWidth) + (srcX));
      srcBitsPtr = srcBits + (srcIndex * 4);


      /* copy the pixel that is pointed to */
      *((int *)destBitsPtr) = *((int *)srcBitsPtr);

    } /*for x*/

  } /* for y */

  /* --- Delete unused GDI objects. */

  DeleteDC(hdcMemDest);
}


static signed char *
matrixRow(int row)
{
    static signed char cmat[4][4] = {
        {-24,  -6,   3,  21},
        {  9,  15, -18, -12},
        { -3, -21,  18,   0},
        { 12,   6,  -9, -15}
    };

    static signed char matrix4[4][4] = {
        {-8, -2,  1,  7},
        { 3,  5, -6, -4},
        {-1, -7,  6,  0},
        { 4,  2, -3, -5}
    };

    static signed char matrix2[4][4] = {
        {-56, -14,   7,  49},
        { 21,  35, -42, -28},
        { -7, -49,  42,   0},
        { 28,  14, -21, -35}
    };

    static signed char matrix1[4][4] = {
        {-88, -22,  11,  77},
        { 33,  55, -66, -44},
        {-11, -77,  66,   0},
        { 44,  22, -33, -55}
    };

    if (numColors == 2) {
        return matrix1[row];
    } else if (numColors == 4) {
        return matrix2[row];
    } else if (numColors == 16) {
        return matrix4[row];
    }

    return cmat[row];
}

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

#else
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

static void
buildMask(myBitmapStruct *bitmap, int alpha, int x, int y)
{
    /* the mask must be 32-bit aligned for PlgBlt */
    int width = bitmap->width + (32 - (bitmap->width & 31));
    
    if (bitmap->mask == NULL) {
        BITMAPINFO bi;
        HDC hdc = GetDC(hMainWindow);

        bi.bmiHeader.biSize          = sizeof(bi.bmiHeader);
        bi.bmiHeader.biWidth         =  width;
        bi.bmiHeader.biHeight        = -bitmap->height;
        bi.bmiHeader.biPlanes        = 1;
        bi.bmiHeader.biCompression   = BI_RGB;
        bi.bmiHeader.biSizeImage     = 0;
        bi.bmiHeader.biBitCount      = 1; /* only need one bit for the mask */
        bi.bmiHeader.biXPelsPerMeter = 0;
        bi.bmiHeader.biYPelsPerMeter = 0;
        bi.bmiHeader.biClrUsed       = 0;
        bi.bmiHeader.biClrImportant  = 0;

        bitmap->mask = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, 
                                        &bitmap->imageMask, NULL, 0);
        if (bitmap->imageMask != NULL) {
            memset(bitmap->imageMask, 0x00, width * bitmap->height);
        }
        ReleaseDC(hMainWindow, hdc);

    }

    if (bitmap->mask != NULL) {
        /*
         * build the image mask
         */
        int offset = (y * width + x) >> 3;
        *(bitmap->imageMask + offset) |= ((alpha & 1) << (7 - (x & 7)));
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
  int color;
  int alpha;
  HDC hdc;
  HDC hdcMem;
  unsigned char* pel;

  _imageDstPtr p = (_imageDstPtr)self;

  if ((p->bitmap == NULL) || (p->mutable)){
    fprintf(stderr, "setARGBPixel error\n");
    return;    
  }

  hdc = GetDC(hMainWindow);
  hdcMem = CreateCompatibleDC(hdc);
  ReleaseDC(hMainWindow, hdc);
  SelectObject(hdcMem, p->bitmap->bitmap);

  if (useAlpha) {
      p->bitmap->prop = HAS_ALPHA;
  }
  for (y = 0; y < height; y++){
    for (x = 0; x < width; x++){
      offset = (y * width) + x;
      pixel = (*imageBuf)[offset];

      alpha = (pixel >> 24) & 0xff;
      color = makeImagePixel(pixel & 0xffffff); /* pixel 0xAARRGGBB encoded */
      pel = p->bitmap->image + ((y * p->bitmap->width + x) << 2);

      /*
       * DIBs use bgr triples not rgb
       */
      *pel++ = GetBValue(color);
      *pel++ = GetGValue(color);
      *pel++ = GetRValue(color);

      if (useAlpha) { 
        buildMask(p->bitmap, alpha, x, y);
        *pel++ = alpha;
      } else {
        *pel++ = 255; /* fully opaque */
      }
    }
  }
  DeleteDC(hdcMem);    
}

static void
sendImagePixels(imageDstPtr self, int y, uchar *pixels, int pixelType)
{
    signed char *mat = matrixRow(y & 3);
    int x;
    _imageDstPtr p = (_imageDstPtr)self;
    HDC hdc, hdcMem;

    int color, off, r, g, b, alpha;
    
    if (p->mutable) return;

    hdc = GetDC(hMainWindow);
    hdcMem = CreateCompatibleDC(hdc);
    ReleaseDC(hMainWindow, hdc);
    SelectObject(hdcMem, p->bitmap->bitmap);

    if ((pixelType == CT_COLOR) ||
        (pixelType == (CT_COLOR | CT_ALPHA))) { /* rgb(a) */

        for (x = 0; x < p->bitmap->width; ++x) {
            unsigned char *pel;

            off = mat[x & 3];

            r = pixels[0];
            g = pixels[1];
            b = pixels[2];
            alpha = 0xff; /* assume full opacity */

            if (pixelType & CT_ALPHA) { 
                alpha = pixels[3];
                p->bitmap->prop = HAS_ALPHA;

                pixels++;

            } else if (p->hasTransMap) { /* single transparent color */

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
            } 
#else
            if (p->bitmap->prop == HAS_MASK || 
                p->bitmap->prop == HAS_ALPHA) {
                buildMask(p->bitmap, alpha, x, y);
            }
#endif

            color  = makeImagePixel((r << 16) | (g << 8) | b);
            pel    = p->bitmap->image + ((y * p->bitmap->width + x) << 2);

            /*
             * DIBs use bgr triples not rgb
             */
            *pel++ = GetBValue(color);
            *pel++ = GetGValue(color);
            *pel++ = GetRValue(color);
#if USE_ALPHA_BLEND
            *pel++ = alpha;
#endif
        }
        
    } else { /* grayscale or indexed color */
        for (x = 0; x < p->bitmap->width; ++x) {
            unsigned char *pel;

            int cmapIndex = *pixels++;
            off = mat[x & 3];
            color = p->cmap[cmapIndex];

            r = ((color >> 16) & 0xff) + off;
            g = ((color >>  8) & 0xff) + off;
            b = ((color >>  0) & 0xff) + off;
            alpha = 0xff; /* assume full opacity */

            if (r < 0) r = 0; else if (r > 0xff) r = 0xff;
            if (g < 0) g = 0; else if (g > 0xff) g = 0xff;
            if (b < 0) b = 0; else if (b > 0xff) b = 0xff;
           
            /* grayscale with alpha */
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
            }
#else
            if (p->bitmap->prop == HAS_MASK || 
                p->bitmap->prop == HAS_ALPHA) {
                buildMask(p->bitmap, alpha, x, y);
            }
#endif

            color  = makeImagePixel((r << 16) | (g << 8) | b);
            pel    = p->bitmap->image + ((y * p->bitmap->width + x) << 2);
            
            /*
             * DIBs use bgr triples not rgb
             */
            *pel++ = GetBValue(color);
            *pel++ = GetGValue(color);
            *pel++ = GetRValue(color);
#if USE_ALPHA_BLEND
            *pel++ = alpha;
#endif
        }
    }

    DeleteDC(hdcMem);    
}

static void
sendPackedImagePixels(imageDstPtr self, int y, uchar *pixels) 
{
    /* we specified a depth of 8, so... */
    sendImagePixels(self, y, pixels, KNI_FALSE);
}

static void*
imageDone(imageDstPtr self, int *width, int *height)
{
    _imageDstPtr p = (_imageDstPtr)self;

    if (p == NULL) return NULL;

    if ((width != NULL) && (height != NULL)) {
        *width  = p->bitmap->width;
        *height = p->bitmap->height;
    }

    return (void*)p->bitmap;
}

void
LCDUIdestroyNativeImage(void* imagePtr) {
    myBitmapStruct *imgData = (myBitmapStruct*)imagePtr;

    if (imgData != NULL) {
        if (imgData->bitmap) {
            DeleteObject(imgData->bitmap);
        }
        if (imgData->mask) {
            DeleteObject(imgData->mask);
        }
        
        midpFree(imagePtr);
    }
}

imageDstPtr 
LCDUIcreateImageDst(jboolean mutable) {
    _imageDstPtr p = midpMalloc(sizeof(_imageDstData));

    if (p == NULL) {
        return NULL;
    }

    p->super.depth            = 8;
    p->super.setColormap      = setImageColormap;
    p->super.setTransMap      = setImageTransparencyMap;
    p->super.setSize          = setImageSize;
    p->super.sendPixels       = sendImagePixels;
    p->super.sendPackedPixels = sendPackedImagePixels;
    p->super.copyPixels       = copyImagePixels;
    p->super.done             = imageDone;
    p->super.setARGBPixels    = setARGBPixels;
    p->super.copyPixelsTransformed
                              = copyPixelsTransformed;

    /* Create a bitmap structure. We use our own private data structure. 
     * But, on some platforms, this may be an opaque data structure 
     * managed by the platform.
     */
    if ((p->bitmap=(myBitmapStruct*)midpMalloc(sizeof(myBitmapStruct))) == NULL) {
        midpFree(p);
        return NULL;
    }
    p->mutable       = mutable;
    p->hasColormap   = KNI_FALSE;
    p->hasTransMap   = KNI_FALSE;

    return (imageDstPtr)p;
}


#if USE_ALPHA_BLEND
BOOL customAlphaBlend(HDC hdcDest, int nXOriginDest, int nYOriginDest, 
                  HDC hdcSrc,  int nXOriginSrc,  int nYOriginSrc,  
                  int nWidth,  int nHeight,
                                    int transform, jboolean processAlpha) {

    int imgLen;
    int scanLength;
    unsigned char *srcBits       = NULL, 
                  *destBits      = NULL, 
                  *destBitsPtr   = NULL,
                  *srcBitsPtr    = NULL,
                  *srcBitsEndPtr = NULL,
                  *lineEndPtr    = NULL;
    int destX, destY;
    int xStart;
    int yStart;
    int xIncr;
    int yIncr;

    int alpha;

    int t_width;
    int t_height;
    
    BOOL ret = KNI_FALSE;
    HDC srcHDC, 
        destHDC;

    HBITMAP srcHBmp, destHBmp;

    BITMAP srcBmp;
    
    BITMAPINFO bi;
    HGDIOBJ oobj;

    destHDC = CreateCompatibleDC(hdcSrc);
    srcHBmp = GetCurrentObject(hdcSrc, OBJ_BITMAP);
    
    DB(GetObject(srcHBmp, sizeof(srcBmp), &srcBmp));
        
#if 0
        fprintf(stderr, "bmType=%i\n"
                        "bmWidth=%i\n"
                        "bmHeight=%i\n"
                        "bmWidthBytes=%i\n"
                        "bmPlanes=%i\n"
                        "bmBitsPixel=%i\n\n",
                        srcBmp.bmType,
                        srcBmp.bmWidth,
                        srcBmp.bmHeight,
                        srcBmp.bmWidthBytes,
                        srcBmp.bmPlanes,
                        srcBmp.bmBitsPixel);
#endif

    imgLen = nWidth * nHeight << 2;

    if(transform & TRANSFORM_INVERTED_AXES) {
        t_width  = nHeight;
        t_height = nWidth;
    } else {
        t_width  = nWidth;
        t_height = nHeight;
    }

    bi.bmiHeader.biSize          = sizeof(bi.bmiHeader);
    bi.bmiHeader.biPlanes        = srcBmp.bmPlanes;

    bi.bmiHeader.biBitCount      = 32; 
    /* srcBmp.bmPlanes * srcBmp.bmBitsPixel;*/

    bi.bmiHeader.biCompression   = BI_RGB;
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed       = 0;
    bi.bmiHeader.biClrImportant  = 0;
    
    bi.bmiHeader.biWidth         = t_width;
    bi.bmiHeader.biHeight        = -t_height;
    bi.bmiHeader.biSizeImage     = t_width * t_height << 2;

    destHBmp = CreateDIBSection(destHDC, &bi, DIB_RGB_COLORS, 
                                &destBits, NULL, 0);
    
    srcBits = (unsigned char *)midpMalloc((srcBmp.bmWidth << 2) * nHeight);

    if ((srcBits != NULL) && (destBits != NULL)) {

        oobj = SelectObject(destHDC, destHBmp);

        /* 
         * grab the screen contents into a bitmap 
         * the same size as our source image
         * + ~510ms 
         */

        DB(BitBlt(destHDC, 0, 0, t_width, t_height, 
            hdcDest, nXOriginDest, nYOriginDest, SRCCOPY));

        bi.bmiHeader.biWidth  = srcBmp.bmWidth;
        bi.bmiHeader.biHeight = -srcBmp.bmHeight;
        bi.bmiHeader.biSizeImage = srcBmp.bmWidth * srcBmp.bmWidth << 2;

        /* grab source bits */
        DB(GetDIBits(hdcSrc, srcHBmp, srcBmp.bmHeight - nHeight - nYOriginSrc, 
                     nHeight, srcBits, &bi, DIB_RGB_COLORS));

#if 1
        scanLength    = srcBmp.bmWidth << 2;
        srcBitsPtr    = srcBits + (nXOriginSrc << 2); 
        destBitsPtr   = destBits;
        srcBitsEndPtr = srcBitsPtr + 
                        (nHeight - 1) * scanLength + (nWidth << 2);
        lineEndPtr    = srcBitsPtr + (nWidth << 2);
                
        if (transform != 0) {
            if (transform & TRANSFORM_Y_FLIP) {
                destY = nHeight - 1;
                yIncr = -1;
            } else {
                destY = 0;
                yIncr = +1;
            }
    
            if (transform & TRANSFORM_X_FLIP) {
                xStart = nWidth - 1;
                xIncr = -1;
            } else {
                xStart = 0;
                xIncr = +1;
            }

        }


        while (srcBitsPtr < srcBitsEndPtr) {
        
            /** 
             * these may not have valid values is transform == 0,
             * but in that case they are never used.
             */
            destX  = xStart;

            while (srcBitsPtr < lineEndPtr) {
    
                if (transform != 0) {
                    destBitsPtr = destBits;

                    if (transform & TRANSFORM_INVERTED_AXES) {
                        destBitsPtr += 
                            ((destX * t_width) + destY) << 2;
                    } else {
                        destBitsPtr +=
                            ((destY * t_width) + destX) << 2;
                    }

                    destX += xIncr;
                } 

                alpha = srcBitsPtr[3];
        
                if (alpha == 0xff || (processAlpha == KNI_FALSE)) {
                    /* completely opaque */

                    /*
                     * this operation can be optimized 
                     * for the bus size of a device 
                     */
                        
                    *((int *)destBitsPtr) = *((int *)srcBitsPtr);
                        
                   /*
                    *(destBitsPtr + x + 0) = *(srcBitsPtr + x + 0);
                    *(destBitsPtr + x + 1) = *(srcBitsPtr + x + 1);
                    *(destBitsPtr + x + 2) = *(srcBitsPtr + x + 2);
                    *(destBitsPtr + x + 3) = alpha;
                    */
                        
                } else if (alpha != 0x00) {  /* needs blending */
                        
                    /* save on dereferencing these values everywhere */
                    unsigned char *d0 = destBitsPtr + 0,  
                    *d1 = destBitsPtr + 1,
                    *d2 = destBitsPtr + 2;
                    int c;
                
                
                    /* this could be optimized if the alpha values could be
                     * rounded to powers of two, then we could simply shift
                     * by alpha. we could also be careless with the divide
                     * and right shift by 8
                     */
                
                    /* doing the calculations this way is faster than the
                     * other method which is compiler defined out below
                     * but the results are still not very good
                     */
                    c =  srcBitsPtr[0] + ((*d0 * (0xff - alpha)) / 0xff);
                    if (c > 0xff) c = 0xff;
                    *d0 = (unsigned char)c;
                        
                    c =  srcBitsPtr[1] + ((*d1 * (0xff - alpha)) / 0xff);
                    if (c > 0xff) c = 0xff;
                    *d1 = (unsigned char)c;
                        
                    c =  srcBitsPtr[2] + ((*d2 * (0xff - alpha)) / 0xff);
                    if (c > 0xff) c = 0xff;
                    *d2 = (unsigned char)c;
                        
                    /*fprintf(stderr, "RGB(%i, %i, %i)\n", *d0, *d1, *d2);*/
                
#if 0
                    *d0 = *(srcBitsPtr + 0) + 
                          *d0 - ((*d0 * alpha) / 0xff);
                        
                    *d1 = *(srcBitsPtr + 1) + 
                          *d1 - ((*d1 * alpha) / 0xff);
                
                    *d2 = *(srcBitsPtr + 2) + 
                          *d2 - ((*d2 * alpha) / 0xff);
#endif
                      /* this operation is unnecessary */
                      /* destBitsPtr[4] = alpha; */
        
                }      

                /* changing destBitsPtr if transform != 0 is ok since we reset
                 * it's value at the top of the loop
                 */
                destBitsPtr += 4;
                srcBitsPtr  += 4;
            }

            lineEndPtr += scanLength;
            srcBitsPtr  = lineEndPtr - (nWidth << 2);
    
            destY += yIncr;
        }
        
#endif
        bi.bmiHeader.biWidth         = t_width;
        bi.bmiHeader.biHeight        = -t_height; 
        bi.bmiHeader.biSizeImage     = t_width * t_height << 2;

        ret = BitBlt(hdcDest, nXOriginDest, nYOriginDest, t_width, t_height, 
                    destHDC, 0, 0, SRCCOPY);

        SelectObject(destHDC, oobj);


        /* Delete the DIB */
        DeleteObject(destHBmp);
    }
    
    DeleteDC(destHDC);

    midpFree(srcBits);

    return ret;
}                    

#endif /* USE_ALPHA_BLEND */

