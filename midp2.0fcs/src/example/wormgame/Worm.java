/*
 * @(#)Worm.java	1.9 02/07/25 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * Worm.java
 *
 * Created on March 30, 2001, 16:15
 * @author Todd M Kennedy
 * @version 1.0
 */

package example.wormgame;

import java.util.Vector;
import javax.microedition.lcdui.Graphics;

/**
 * A Worm is a combination of one or more WormLinks. The first
 * WormLink is the physical tail of the worm. The last WormLink
 * is the physical head of the worm. As the worm moves, it adds
 * one space to the head and then removes one from the tail. If the
 * worm eats a piece of food, the head adds one space but the tail
 * will not remove one. In this manner, the worm will grow outward
 * from its head.
 */
public class Worm {

    /* Possible directions for the worm */
    public final static byte DOWN  = 2;
    public final static byte LEFT  = 4;
    public final static byte RIGHT = 6;
    public final static byte UP    = 8;


    // The direction of the worm's head
    private byte currentDirection;

    // The individual worm links that form the worm
    private Vector worm = new Vector(5, 2);

    private boolean needUpdate;
    private boolean moveOnNextUpdate;
    private boolean hasEaten;

    private final static int INIT_X = 3;
    private final static int INIT_Y = 8;
    private final static int INIT_LEN = 8;
    private final static byte INIT_DIR = RIGHT;

    public Worm(WormPit pit) {
	regenerate();
    }

    /**
     * Regenerate the worm in its initial position. This is used
     * to restart the game after the worm is killed.
     */
    public void regenerate() {
	synchronized (worm) {
	    worm.removeAllElements();
	    worm.addElement(new WormLink(INIT_X, INIT_Y, 
					   INIT_LEN, INIT_DIR));

	    // Reset class variables
	    currentDirection = INIT_DIR;
	    needUpdate = false;
	    hasEaten = false;
	    moveOnNextUpdate = false;
	}
    }

    /**
     * Change the direction of the worm. It will only change
     * directions if you set a direction that is 90 degrees
     * different from the current direction of the worm.
     */
    public void setDirection(byte direction) {
	synchronized (worm) {
	    if ((direction != currentDirection) && !needUpdate) {
		WormLink sl = (WormLink)worm.lastElement();
		int x = sl.getEndX();
		int y = sl.getEndY();
		switch (direction) {
		case UP:
		    if (currentDirection != DOWN) {
			y--; needUpdate = true;
		    }
		    break;
		case DOWN:
		    if (currentDirection != UP) {
			y++; needUpdate = true;
		    }
		    break;
		case LEFT:
		    if (currentDirection != RIGHT) {
			x--; needUpdate = true;
		    }
		    break;
		case RIGHT:
		    if (currentDirection != LEFT)  {
			x++; needUpdate = true;
		    }
		    break;
		}
		if (needUpdate == true) {
		    worm.addElement(new WormLink(x, y, 0, direction));
		    currentDirection = direction;
		}
	    }
	}
    }


    /**
     * Schedule the worm for growth. The next time the update()
     * method is invoked, the worm will move forward by one cell.
     */
    public void moveOnUpdate() {
	synchronized (worm) {
	    moveOnNextUpdate = true;
	}
    }

    /**
     * Update the worm. If the worm is scheduled to grow, this
     * method will grow the worm head by one cell and potentially
     * shrink the tail by one cell. If the worm has eaten a piece
     * of food, the tail will not shrink.
     *
     * If the worm is not scheduled to grow, this method will
     * do nothing.
     */
    public void update(Graphics g) throws WormException {
	WormLink head, sl;
	int headX, headY;

	if (!moveOnNextUpdate) {
	    return;
	}
	
	synchronized (worm) {

	    head = (WormLink)worm.lastElement();          // the worm 'head'
	    head.increaseLength();

	    if (!hasEaten) {
		WormLink tail;
		tail = (WormLink)worm.firstElement();     // the worm 'tail'
		int tailX = tail.getX();
		int tailY = tail.getY();

		tail.decreaseLength();
		if (tail.getLength() == 0) {
		    worm.removeElement(tail);
		}

		// Clear last block of the tail
		g.setColor(WormPit.ERASE_COLOUR);
		drawLink(g, tailX, tailY, tailX, tailY, 1);

	    } else {
		hasEaten = false;
	    }
	    
	    needUpdate = false;
	    
	    // Make sure we're still in bounds
	    if (!WormPit.isInBounds(head.getEndX(), head.getEndY())) {
		throw new WormException("over the edge");  // You're dead Jim
	    }
	    
	    headX = (byte)head.getEndX();
	    headY = (byte)head.getEndY();

	    // Draw the head
	    g.setColor(WormPit.DRAW_COLOUR);
	    drawLink(g, headX, headY, headX, headY, 1);
	
	    // See if we ate ourself
	    for (int i = 0; i < worm.size()-1; i++) {
		sl = (WormLink)worm.elementAt(i);
		if (sl.contains(headX, headY)) {
		    throw new WormException("you ate yourself");
		}
	    }

	}
    }

    /**
     * Draw a worm link.
     */
    public void drawLink(Graphics g, int x1, int y1, int x2, int y2, int len) {
	// Convert coordinates to the drawing area
	len *= WormPit.CELL_SIZE;
	if (x1 == x2) {
	    x1 *= WormPit.CELL_SIZE;
	    if (y2 < y1) {
		y1 = y2 * WormPit.CELL_SIZE;
	    } else {
		y1 *= WormPit.CELL_SIZE;
	    }
	    g.fillRect(x1, y1, WormPit.CELL_SIZE, len);
	} else {
	    y1 *= WormPit.CELL_SIZE;
	    if (x2 < x1) {
		x1 = x2 * WormPit.CELL_SIZE;
	    } else {
		x1 *= WormPit.CELL_SIZE;
	    }
	    g.fillRect(x1, y1, len, WormPit.CELL_SIZE);
	}
    }

    /**
     * Draw the entire worm. Logically, the worm is composed of
     * one or more links. A worm is usually composed of a few
     * straight segments. This allows us to paint a full worm
     * with a minimum of draw calls.
     */
    public void paint(Graphics g) {
	WormLink sl;
	int x1, x2, y1, y2;
	int len;

	for (int i = 0; i < worm.size(); i++) {
	    sl = (WormLink)worm.elementAt(i);
	    x1 = sl.getX();    x2 = sl.getEndX();
	    y1 = sl.getY();    y2 = sl.getEndY();
	    len = sl.getLength();
	    drawLink(g, x1, y1, x2, y2, len);
	}
    }

    /**
     * Signals the worm to eat a food item
     */
    public void eat() {
	hasEaten = true;
    }

    /**
     * Get the X coordinate of the cell that contains the worm's head.
     */
    public int getX() {
	synchronized (worm) {
	    return ((WormLink)worm.lastElement()).getEndX();
	}
    }

    /**
     * Get the Y coordinate of the cell that contains the worm's head.
     */
    public int getY() {
	synchronized (worm) {
	    return ((WormLink)worm.lastElement()).getEndY();
	}
    }

    /**
     * Returns true if the worm contains the given cell coordinate.
     */
    public boolean contains(int x, int y) {
	WormLink sl;
	synchronized (worm) {
	    for (int i = 0; i < worm.size(); i++) {
		sl = (WormLink)worm.elementAt(i);
		if (sl.contains(x, y)) {
		    return true;
		}
	    }
	}
	return false;
    }

}

