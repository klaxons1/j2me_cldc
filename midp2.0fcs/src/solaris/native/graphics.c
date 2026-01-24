/*
 * @(#)graphics.c	1.18 02/11/06 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <nativeGUI.h>

/*
 * Draw a line between two points (x1,y1) and (x2,y2).
 */
void 
LCDUIdrawLine(int pixel, short *clip, void* dst, int dotted, 
	      int x1, int y1, int x2, int y2)
{
    XPoint pts[2];
    Drawable drawable = setupGC(gc, pixel, clip, dst, dotted);
    XSetFillStyle(display, gc, FillSolid);
    pts[0].x = x1;
    pts[0].y = y1;
    pts[1].x = x2;
    pts[1].y = y2;
    XDrawLines(display, drawable, gc, pts, 2, CoordModeOrigin);
    XDrawPoints(display, drawable, gc, pts, 2, CoordModeOrigin);
    if (!doubleBuffer && (drawable == paintDrawable)) {
        refreshPaintWindow(x1, y1, x2, y2);
    }
}

/*
 * Draw a rectangle at (x,y) with the given width and height.
 */
void 
LCDUIdrawRect(int pixel, short *clip, void* dst, int dotted, 
	      int x, int y, int width, int height)
{
    if ((width < 1) || (height < 1)) {
        if ((width >= 0) && (height >= 0)) {
            LCDUIdrawLine(pixel, clip, dst, dotted, 
                          x, y, x + width, y + height);
        }
        return;
    } else {

        Drawable drawable = setupGC(gc, pixel, clip, dst, dotted);

        XDrawRectangle(display, drawable, gc, x, y, width, height);

        if (!doubleBuffer && (drawable == paintDrawable)) {
            refreshPaintWindow(x, y, x + width, y + height);
        }
    }

}

/*
 * Fill a rectangle at (x,y) with the given width and height.
 */
void 
LCDUIfillRect(int pixel, short *clip, void* dst, int dotted, 
	      int x, int y, int width, int height)
{

    if ((width < 2) || (height < 2)) {
        if ((width > 0) && (height > 0)) {
            LCDUIdrawLine(pixel, clip, dst, dotted,
                          x, y, x + width - 1, y + height - 1);
        }
        return;
    } else {

        Drawable drawable = setupGC(gc, pixel, clip, dst, dotted);

        XFillRectangle(display, drawable, gc, x, y, width, height);

        if (!doubleBuffer && (drawable == paintDrawable)) {
            refreshPaintWindow(x, y, x + width, y + height);
        }
    }
}

static
void setupRects(XRectangle rects[3], XArc arcs[4], XSegment lines[4],
		int *numRects, int *numLines, int *numArcs,
		int x, int y, int width, int height,
		int *arcWidth, int *arcHeight) {
    *numRects = 1;
    *numLines = 0;
    *numArcs  = 0;

    if (*arcWidth < 0) *arcWidth = 0;
    if (*arcHeight < 0) *arcHeight = 0;
    if (*arcWidth > width) *arcWidth = width;
    if (*arcHeight > height) *arcHeight = height;

    if ((*arcWidth > 0) || (*arcHeight > 0)) {
        int tx1 = x + (*arcWidth / 2);
        int tx2 = x + width - (*arcWidth / 2);
        int ty1 = y + (*arcHeight / 2);
        int ty2 = y + height - (*arcHeight / 2);
        int txw = x + width;
        int tyh = y + height;


        *numArcs = 4;
        *numLines = 4;
        *numRects = 3;

        arcs[0].x = x;
        arcs[0].y = y;
        arcs[0].width  = *arcWidth;
        arcs[0].height = *arcHeight;
        arcs[0].angle1 = 90<<6;
        arcs[0].angle2 = 90<<6;

        arcs[1].x = x + width - *arcWidth;
        arcs[1].y = y;
        arcs[1].width  = *arcWidth;
        arcs[1].height = *arcHeight;
        arcs[1].angle1 = 0;
        arcs[1].angle2 = 90<<6;

        arcs[2].x = x;
        arcs[2].y = y + height - *arcHeight;
        arcs[2].width  = *arcWidth;
        arcs[2].height = *arcHeight;
        arcs[2].angle1 = 180<<6;
        arcs[2].angle2 = 90<<6;

        arcs[3].x = x + width - *arcWidth;
        arcs[3].y = y + height - *arcHeight;
        arcs[3].width  = *arcWidth;
        arcs[3].height = *arcHeight;
        arcs[3].angle1 = 270<<6;
        arcs[3].angle2 = 90<<6;

        lines[0].x1 = tx1;
        lines[0].y1 = y;
        lines[0].x2 = tx2;
        lines[0].y2 = y;

        lines[1].x1 = txw;
        lines[1].y1 = ty1;
        lines[1].x2 = txw;
        lines[1].y2 = ty2;

        lines[2].x1 = tx1;
        lines[2].y1 = tyh;
        lines[2].x2 = tx2;
        lines[2].y2 = tyh;

        lines[3].x1 = x;
        lines[3].y1 = ty1;
        lines[3].x2 = x;
        lines[3].y2 = ty2;

        rects[0].x      = tx1;
        rects[0].y      = y;
        rects[0].width  = tx2 - tx1;
        rects[0].height = tyh - y;

        rects[1].x      = x;
        rects[1].y      = ty1;
        rects[1].width  = tx1 - x;
        rects[1].height = ty2 - ty1;

        rects[2].x      = tx2;
        rects[2].y      = ty1;
        rects[2].width  = txw - tx2;
        rects[2].height = ty2 - ty1;
    } else {
        rects[0].x = x;
        rects[0].y = y;
        rects[0].width = width;
        rects[0].height = height;
    }
}

/*
 * Draw a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
void 
LCDUIdrawRoundRect(int pixel, short *clip, void* dst, int dotted, 
		   int x, int y, int width, int height,
		   int arcWidth, int arcHeight)
{
    Drawable  drawable = setupGC(gc, pixel, clip, dst, dotted);

    if ((width < 2) || (height < 2)) {
	if (width < 0 || height < 0) {
	    return;
	} else if (width == 0 && height == 0) {
	    XDrawPoint(display, drawable, gc, x, y);
	} else {
	    XDrawLine(display, drawable, gc, x, y, x + width, y + height);
	}
    } else {
	XRectangle rects[3];
	XArc arcs[4];
	XSegment lines[4];
	int numRects, numLines, numArcs;
	
	setupRects(rects, arcs, lines, &numRects, &numLines, &numArcs,
		   x, y, width, height,
		   &arcWidth, &arcHeight);
	
	XSetFillStyle(display, gc, FillSolid);
	if (numRects == 1) {
	    XDrawRectangles(display, drawable, gc, rects, 1);
	} else {
	    XDrawSegments(display, drawable, gc, lines, numLines);
	    XDrawArcs(display, drawable, gc, arcs, numArcs);
	}
    }

    if (!doubleBuffer && (drawable == paintDrawable)) {
	refreshPaintWindow(x, y, x + width, y + height);
    }
}

/*
 * Fill a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
void 
LCDUIfillRoundRect(int pixel, short *clip, void* dst, int dotted, 
		   int x, int y, int width, int height,
		   int arcWidth, int arcHeight)
{
    Drawable  drawable = setupGC(gc, pixel, clip, dst, dotted);

    if ((width < 2) || (height < 2)) {
        if (width == 1 && height == 1) {
	    width = height = 0;
            XDrawPoint(display, drawable, gc, x, y);
        } else {
	    return;
	}
    } else {
	XRectangle rects[3];
	XArc arcs[4];
	XSegment lines[4];
	int numRects, numLines, numArcs;

	setupRects(rects, arcs, lines, &numRects, &numLines, &numArcs,
		   x, y, width, height,
		   &arcWidth, &arcHeight);

	XFillRectangles(display, drawable, gc, rects, numRects);
	
	if (numArcs > 0) {
	    XFillArcs(display, drawable, gc, arcs, numArcs);
	}
    }

    if (!doubleBuffer && (drawable == paintDrawable)) {
        refreshPaintWindow(x, y, x + width, y + height);
    }
}

/*
 * Draw an elliptical arc centered in the given rectangle. The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 */
void 
LCDUIdrawArc(int pixel, short *clip, void* dst, int dotted, 
	     int x, int y, int width, int height,
             int startAngle, int arcAngle)
{
    Drawable drawable = setupGC(gc, pixel, clip, dst, dotted);

    if ((width < 2) || (height < 2)) {
	if (width < 0 || height < 0) {
	    return;
	} else if (width == 0 && height == 0) {
            XDrawPoint(display, drawable, gc, x, y);
        } else {
            XDrawLine(display, drawable, gc, x, y, x + width, y + height);
        }
    } else {

	XSetFillStyle(display, gc, FillSolid);
	XDrawArc(display, drawable, gc, x, y, width, height, 
		 startAngle << 6, arcAngle << 6);
    }

    if (!doubleBuffer && (drawable == paintDrawable)) {
	refreshPaintWindow(x, y, x + width, y + height);
    }
}

/*
 * Fill an elliptical arc centered in the given rectangle. The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 */
void 
LCDUIfillArc(int pixel, short *clip, void* dst, int dotted, 
	     int x, int y, int width, int height,
             int startAngle, int arcAngle)
{
    Drawable drawable = setupGC(gc, pixel, clip, dst, dotted);

    if ((width < 2) || (height < 2)) {
        if (width == 1 && height == 1) {
	    width = height = 0;
            XDrawPoint(display, drawable, gc, x, y);
        } else {
	    return;
	}
    } else {

	XSetArcMode(display, gc, ArcPieSlice);
	XFillArc(display, drawable, gc, x, y, width, height, 
		 startAngle << 6, arcAngle << 6);
    }

    if (!doubleBuffer && (drawable == paintDrawable)) {
        refreshPaintWindow(x, y, x + width, y + height);
    }
}

/*
 * Fill a triangle defined by the given three points (x1,y1) (x2,y2)
 * and (x3,y3) with the current color.
 */
void 
LCDUIfillTriangle(int pixel, short *clip, void* dst, int dotted, 
		  int x1, int y1, int x2, int y2, int x3, int y3)
{
    XPoint pts[3];
    int xmin, xmax, ymin, ymax;

    Drawable drawable = setupGC(gc, pixel, clip, dst, dotted);
    pts[0].x = x1;
    pts[0].y = y1;
    pts[1].x = x2;
    pts[1].y = y2;
    pts[2].x = x3;
    pts[2].y = y3;

    XFillPolygon(display, drawable, gc, pts, 3, Nonconvex, 
		 CoordModeOrigin);

    if (!doubleBuffer && (drawable == paintDrawable)) {
	/* determine the bounding rectangle for this triangle */
	if (x1 >= x2) {
	    xmax = x1;
	    xmin = x2;
	} else {
	    xmax = x2;
	    xmin = x1;
	}
	if (x3 > xmax) {
	    xmax = x3;
	} else if (x3 < xmin) {
	    xmin = x3;
	}

	if (y1 >= y2) {
	    ymax = y1;
	    ymin = y2;
	} else {
	    ymax = y2;
	    ymin = y1;
	}
	if (y3 > ymax) {
	    ymax = y3;
	} else if (y3 < ymin) {
	    ymin = y3;
	}

        refreshPaintWindow(xmin, ymin, xmax, ymax);
    }
}
