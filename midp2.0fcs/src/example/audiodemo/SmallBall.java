/*
 * @(#)SmallBall.java	1.5 02/07/25 @(#)
 *
 * Copyright (c) 1999-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.audiodemo;

import javax.microedition.lcdui.*;
import javax.microedition.media.control.*;

/**
 * A SmallBall is a lightweight animated ball that runs in it's own thread.
 * It moves within a rectangular region, bouncing off the walls.
 */
class SmallBall implements Runnable {
    
    // random number generator
    static java.util.Random random = new java.util.Random(); 

    // controls the speed of all balls; delay in centiseconds
    static int delay = 20;
    static void slower() {
	delay += 10;
	if (delay > 100) delay = 100;
    }
    
    static void faster() {
	delay -= 10;
	if (delay < 10) delay = 10;
    }
    
    // the matrix to transform the direction based on the
    // current direction and which wall was hit
    static int[][] matrix = {
	{  1,-1,  -1, 1,   1, 1 },
	{ -1,-1,   1, 1,  -1, 1 },
	null,
	{  1, 1,  -1,-1,   1,-1 },
	{ -1, 1,   1,-1,  -1,-1 }
    };
    
    // the region in which the ball moves
    int top, left, width, height;
    
    // the position and radius of the ball
    int posX, posY;
    int radius = 5, ballSize = radius*2;
    
    // the direction of the ball is controlled by these two variables
    int deltaX;
    int deltaY;
    
    // a handle onto the singleton Graphics object
    Graphics g;
    Canvas canvas;
    int note = ToneControl.C4;
    int clr = 0;
   
    // public variables to control the behaviour of the thread
    public boolean stop;
    
    /**
     * Constructor defines the region in which the ball moves as well
     * as its starting position.
     */
    SmallBall(Canvas c, int left, int top, int width, int height) {
	super();
	canvas = c;
	
	this.left = left + 1;		
	this.top  = top + 1;		
	this.width = width - (2 * radius + 2);
	this.height = height - (2 * radius + 2);

	// use positive random #s
	this.posX = (random.nextInt()>>>1) % (this.width-20) + 10;
	this.posY = (random.nextInt()>>>1) % (this.height-20) + 10;
	
	deltaX = random.nextInt() & 1;
	deltaY = random.nextInt() & 1;
	
	if (deltaX == 0) deltaX = -1;
	if (deltaY == 0) deltaY = -1;
    }
    
    public void setNote(int note) {
	this.note = note;
    }

    /**
     * Starts the ball running.
     */
    public void run() {
	//	    System.out.println("starting... " + this);
	int right = left + width;
	int bottom = top + height;
	
	stop = false;
	while (!stop) {
	    
	    ballSize = radius * 2;
	    
	    // calculate a direction of the ball as an integer in the range
	    // -2 .. 2 (excluding 0)
	    int direction = deltaX + deltaY;
	    if (direction == 0) direction = deltaX + 2*deltaY;
	    
	    // is the current position colliding with any wall
	    int collision = 0;
	    if (posX <= left || posX >= right) collision++;
	    if (posY <= top || posY >= bottom) collision += 2;
	    
	    // change the direction appropriately if there was a collision
	    if (collision != 0) {
		try {
		    javax.microedition.media.Manager.playTone(note, 100/*ms*/,100);
		} catch (Exception ex) {
		    System.out.println("failed to play tone");
		}

		collision = (collision - 1) * 2;
		
		deltaX = matrix[direction+2][collision];
		deltaY = matrix[direction+2][collision+1];
	    }
	    
	    // calculate the new position and queue a repaint
	    posX = posX + deltaX;
	    posY = posY + deltaY;
	    canvas.repaint();
	    
	    // use the delay to control the speed of the ball
	    try {
		Thread.sleep(delay);
	    } catch (InterruptedException e) {}
	}
    }
    
    /**
     * Paint the ball.
     */
    void paint(Graphics g) {
	g.setColor(clr);
	g.fillArc(posX,posY,ballSize,ballSize,0,360);
    }
    
    public void setColor(int clr) {
	this.clr = clr;
    }

    public String toString() {
	return super.toString() + " x = " + posX + ", y = " + posY;
    }
    
}
