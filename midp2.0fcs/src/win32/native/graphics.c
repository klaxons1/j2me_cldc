/*
 * @(#)graphics.c	1.19 02/09/11 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <nativeGUI.h>

#ifndef M_PI
#define	M_PI 3.14159265358979323846
#endif

/*
 * Draw a line between two points (x1,y1) and (x2,y2).
 */
void LCDUIdrawLine(int pixel, short *clip, void *dst, int dotted, 
		   int x1, int y1, int x2, int y2)
{
    POINT pts[3];
    GRAPHICS_SETUP(FALSE);

    /* Windows leaves off the second endpoint.  This causes us */
    /* two problems: drawing a line from x,y to x,y does not   */
    /* paint the single pixel, and drawing from x,y to x',y'   */
    /* does not draw the same pixels as from x',y' to x,y.     */
    if ((x1 == x2) && (y1 == y2)) {
        pts[0].x = x1;
        pts[0].y = y1;

        pts[1].x = x1 + 1;
        pts[1].y = y1 + 1;

        Polyline(hdc, pts, 2);  /* paint single pixel at x1, y1 */
    } else {
        pts[0].x = pts[2].x = x1;
        pts[0].y = pts[2].y = y1;

        pts[1].x = x2;
        pts[1].y = y2;
        Polyline(hdc, pts, 3); /* draw line, including both endpts */
    }

    if (!doubleBuffer && (dst == NULL)) {
        refreshPaintWindow(x1, y1, x2, y2);
    }

    GRAPHICS_CLEANUP();
}

/*
 * Draw a rectangle at (x,y) with the given width and height.
 */
void LCDUIdrawRect(int pixel, short *clip, void *dst, int dotted, 
                   int x, int y, int width, int height)
{
    if ((width < 1) || (height < 1)) {
        if ((width >= 0) && (height >= 0)) {
             LCDUIdrawLine(pixel, clip, dst, dotted, 
                           x, y, x + width, y + height);
        }
        return;
    } else {
        
        GRAPHICS_SETUP(FALSE);

        ++width;
        ++height;

        Rectangle(hdc, x, y, x + width, y + height);

        if (!doubleBuffer && (dst == NULL)) {
            refreshPaintWindow(x, y, x + width, y + height);
        }
        GRAPHICS_CLEANUP();
    }
}

/*
 * Fill a rectangle at (x,y) with the given width and height.
 */
void LCDUIfillRect(int pixel, short *clip, void *dst, int dotted, 
		   int x, int y, int width, int height)
{

    if ((width < 2) || (height < 2)) {	
        if ((width > 0) && (height > 0)) {
           LCDUIdrawLine(pixel, clip, dst, dotted, 
                          x, y, x + width - 1, y + height - 1);
        }
        return;
    } else {

        GRAPHICS_SETUP(TRUE);
	
        Rectangle(hdc, x, y, x + width, y + height);
	
        if (!doubleBuffer && (dst == NULL)) {
	          refreshPaintWindow(x, y, x + width, y + height);
        }
        GRAPHICS_CLEANUP();
    }
}

/*
 * Draw a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
void LCDUIdrawRoundRect(int pixel, short *clip, void *dst, int dotted, 
			int x, int y, int width, int height,
			int arcWidth, int arcHeight)
{
    if ((width < 1) || (height < 1)) {
        if ((width >= 0) && (height >= 0)) {
             LCDUIdrawLine(pixel, clip, dst, dotted, 
                           x, y, x + width, y + height);
        }
        return;
    } else {

	GRAPHICS_SETUP(FALSE);
	
	++width;
	++height;
	
	if (arcWidth < 0) arcWidth = 0;
	if (arcHeight < 0) arcHeight = 0;
	if (arcWidth > width) arcWidth = width;
	if (arcHeight > height) arcHeight = height;
	
	RoundRect(hdc, x, y, x + width, y + height, arcWidth, arcHeight);
	
	if (!doubleBuffer && (dst == NULL)) {
	    refreshPaintWindow(x, y, x + width, y + height);
	}
	GRAPHICS_CLEANUP();
    }
}

/*
 * Fill a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
void LCDUIfillRoundRect(int pixel, short *clip, void *dst, int dotted, 
			int x, int y, int width, int height,
			int arcWidth, int arcHeight)
{
    if ((width < 2) || (height < 2)) {	
        if ((width > 0) && (height > 0)) {
           LCDUIdrawLine(pixel, clip, dst, dotted, 
                          x, y, x + width - 1, y + height - 1);
        }
        return;
    } else {

	GRAPHICS_SETUP(TRUE);
	
	if (arcWidth < 0) arcWidth = 0;
	if (arcHeight < 0) arcHeight = 0;
	if (arcWidth > width) arcWidth = width;
	if (arcHeight > height) arcHeight = height;
	
	RoundRect(hdc, x, y, x + width, y + height, arcWidth, arcHeight);
	
	if (!doubleBuffer && (dst == NULL)) {
	    refreshPaintWindow(x, y, x + width, y + height);
	}
	GRAPHICS_CLEANUP();
    }
}

static
void checkArcAngles(int *startAngle, int *arcAngle) {
    if ((*arcAngle > 360) || (*arcAngle < -360)) {
	*startAngle = 0;
	*arcAngle = 0;       /* actually an endpoint */
    } else {
	*arcAngle += *startAngle;
	
	/* win98 always draws arcs ccw */
	if (*arcAngle < 0) {
	    int tmp = *arcAngle;
	    *arcAngle = *startAngle;
	    *startAngle = tmp;
	}
	
	while (*arcAngle < 0)   *arcAngle   += 360;
	while (*startAngle < 0) *startAngle += 360;
	
	*startAngle %= 360;
	*arcAngle   %= 360;
    }
}

/*
 * Draw an elliptical arc centered in the given rectangle.   The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 */
void LCDUIdrawArc(int pixel, short *clip, void *dst, int dotted, 
		  int x, int y, int w, int h, 
		  int startAngle, int arcAngle)
{
    if ((w < 2) || (h < 2)) {
	if (w < 0 || h < 0) {
	    return;
	} else {
	    LCDUIdrawLine(pixel, clip, dst, dotted, x, y, x + w, y + h);
	    return;
	}

    } else if (arcAngle != 0) {
        GRAPHICS_SETUP(FALSE);

        ++w;
	++h;

	checkArcAngles(&startAngle, &arcAngle);

        if (startAngle == arcAngle) {
            Ellipse(hdc, x, y, x + w, y + h);
        } else {
            /* negate the value of sin() because y axis grows DOWNWARD. */
            int startX = (int)( cos(M_PI*startAngle/180.0)*w + 2*x + w)/2;
            int startY = (int)(-sin(M_PI*startAngle/180.0)*h + 2*y + h)/2;

            int endX = (int)( cos(M_PI*arcAngle/180.0)*w + 2*x + w)/2;
            int endY = (int)(-sin(M_PI*arcAngle/180.0)*h + 2*y + h)/2;

	    Arc(hdc, x, y, x + w, y + h, startX, startY, endX, endY);
        }

        if (!doubleBuffer && (dst == NULL)) {
            refreshPaintWindow(x, y, x + w, y + h);
        }
        GRAPHICS_CLEANUP();
    }
}

/*
 * Fill an elliptical arc centered in the given rectangle. The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 */
void LCDUIfillArc(int pixel, short *clip, void *dst, int dotted, 
		  int x, int y, int w, int h, 
		  int startAngle, int arcAngle)
{
    if ((w < 2) || (h < 2)) {
	if((w == 1) && (h == 1)) {
	    LCDUIdrawLine(pixel, clip, dst, dotted, x, y, x, y);
	}
        return;

    } else if (arcAngle != 0) {
        GRAPHICS_SETUP(TRUE);
	
	checkArcAngles(&startAngle, &arcAngle);

        if (startAngle == arcAngle) {
            Ellipse(hdc, x, y, x + w, y + h);
        } else {
            /* negate the value of sin() because y axis grows DOWNWARD. */
            int startX = (int)( cos(M_PI*startAngle/180.0)*w + 2*x + w)/2;
            int startY = (int)(-sin(M_PI*startAngle/180.0)*h + 2*y + h)/2;

            int endX = (int)( cos(M_PI*arcAngle/180.0)*w + 2*x + w)/2;
            int endY = (int)(-sin(M_PI*arcAngle/180.0)*h + 2*y + h)/2;

	    Pie(hdc, x, y, x + w, y + h, startX, startY, endX, endY);
        }

        if (!doubleBuffer && (dst == NULL)) {
            refreshPaintWindow(x, y, x + w, y + h);
        }
        GRAPHICS_CLEANUP();
    }
}

/*
 * Fill a triangle defined by the given three points (x1,y1) (x2,y2)
 * and (x3,y3) with the current color.
 */
void 
LCDUIfillTriangle(int pixel, short *clip, void *dst, int dotted, 
		  int x1, int y1, int x2, int y2, int x3, int y3)
{
    POINT pts[3];
    int xmin, ymin, xmax, ymax;

    GRAPHICS_SETUP(TRUE);
    /* don't draw the triangle's outline... */
    hPen = SelectObject(hdc, CreatePen(PS_NULL, 1, pixel));

    pts[0].x = x1;
    pts[0].y = y1;
    pts[1].x = x2;
    pts[1].y = y2;
    pts[2].x = x3;
    pts[2].y = y3;


    Polygon(hdc, pts, 3);  /* draw filled triangle */
	
    if (!doubleBuffer && (dst == NULL)) {
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
    GRAPHICS_CLEANUP();
}
