/*
 * @(#)BallCanvas.java	1.9 02/08/15 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.audiodemo;

import javax.microedition.lcdui.*;
import javax.microedition.media.*;
import javax.microedition.media.control.*;

public class BallCanvas extends Canvas implements CommandListener {
    
    private Display display;
    private Player player;
    
    private int[] notes = {68, 72, 70, 74, 76};
    private int[] colors = {0xff, 0xff00, 0xff0000, 0xffff, 0xff00ff};  
    
    // a set of free roaming balls
    private SmallBall[] balls;
    private int numBalls;
    private int width, height;
    private boolean paused;
    
    private Command backCommand = new Command("Back", Command.BACK, 1);
    private Command pauseCommand = new Command("Pause", Command.BACK, 1);
    private Command playCommand = new Command("Play", Command.BACK, 1);
    
    private boolean playerCreated;
    
    public BallCanvas(Display d, int maxBalls, int bg) {
	display = d;
	
	// initialize the array of balls
	balls = new SmallBall[maxBalls];
	
	width = getWidth();
	height = getHeight();
	
	playerCreated = initPlayer(bg);

	// Start with one ball
	balls[0] = new SmallBall(this, 0, 0,width,height-12);
	balls[0].setColor(colors[0]);
	numBalls = 1;
	paused = true;
	
	this.addCommand(backCommand);
	this.addCommand(pauseCommand);
	setCommandListener(this);

    }
    
    boolean needAlert() {
	return (!playerCreated);
    }


    boolean initPlayer(int bg) {
	try {
	    switch (bg) {
	    case 1: // wave bg
		player = Manager.createPlayer(BBall.wavbgUrl);
		break;
	    case 2: // tone seq bg
		{
		    byte d = 8; 
		    byte C4 = ToneControl.C4;
		    byte D4 = ToneControl.C4 + 2; // a whole step 
		    byte E4 = ToneControl.C4 + 4; // a major third
		    byte G4 = ToneControl.C4 + 7; // a fifth
		    byte rest = ToneControl.SILENCE; // eighth-note rest
		    
		    byte[] mySequence = new byte[] {
			ToneControl.VERSION, 1,
			    ToneControl.TEMPO, 30,
			    ToneControl.BLOCK_START, 0,
			    E4,d,D4,d,C4,d,D4,d,E4,d,E4,d,E4,d,rest,d, 
			    ToneControl.BLOCK_END,0,
			    ToneControl.PLAY_BLOCK,0,
			    D4,d,D4,d,D4,d,rest,d,E4,d,G4,d,G4,d,rest,d,//play "B" section 
			    ToneControl.PLAY_BLOCK,0, // content of "A" section 
			    D4,d,D4,d,E4,d,D4,d,C4,d,rest,d// play "C" section 
			    };
		    player = Manager.createPlayer(Manager.TONE_DEVICE_LOCATOR);
		    player.realize();
		    ToneControl c = (ToneControl)player.getControl("ToneControl");
		    c.setSequence(mySequence);
		}
		break;
	    default:
		player = null;
	    }
	    
	    if (player != null) {
		player.setLoopCount(-1);
		player.start();
	    }
	} catch (Exception ex) {
	    if ( player != null )
		player.close();
	    player = null;
	    return false;
	}
	
	return true;
    }
    
    /**
     * Draws the drawing frame (which also contains the ball) and the
     * controls.
     */
    protected void paint(Graphics g) {
	// Draw the frame 
	g.setColor(0xffffff);
	g.fillRect(0,0, width-1, height-1);
	g.setColor(0);
	g.drawRect(0+1,0+1,width-2,height-2);
	
	// Draw each ball
	for(int i = 0; i < numBalls; i++) {
	    balls[i].paint(g);
	}
    }
    
    /**
     * Handle a pen down event.
     */
    public void keyPressed( int keyCode) {
	
	int action = getGameAction(keyCode);
	
	switch (action) {
	case LEFT:
	    // Reduce the number of threads
	    if (numBalls > 0) {
		
		// decrement the counter
		numBalls = numBalls - 1;
		
		// stop the thread and remove the reference to it
		balls[numBalls].stop = true;
		balls[numBalls] = null;
	    }
	    break;
	    
	case RIGHT:
	    // Increase the number of threads
	    if (numBalls < balls.length) {
		
		// create a new ball and start it moving
		balls[numBalls] = new SmallBall(this, 0, 0, width,height-12);
		balls[numBalls].setNote(notes[numBalls]);
		balls[numBalls].setColor(colors[numBalls]);
		
		new Thread(balls[numBalls]).start();
		
		// increment the counter
		numBalls = numBalls + 1;
	    }
	    break;
	    
	case UP:
	    // Make them move faster
	    SmallBall.faster();
	    break;
	    
	case DOWN:
	    // Make them move slower
	    SmallBall.slower();
	    break;
	}
	repaint();
    }
    
    /**
     * Destroy
     */
    void destroy() {
	// kill all the balls and terminate
	for (int i = 0; i < balls.length; i++) {
	    if (balls[i] != null) {
		balls[i].stop = true;
		balls[i] = null;
	    }
	}
	numBalls = 0;
	if ( player != null) {
	    player.close();
	    player = null;
	}
	BBall.getInstance().game = null;
    }
    
    
    /*
     * Return whether the canvas is paused or not.
     */
    boolean isPaused() {
	return paused;
    }
    
    /**
     * Pause the balls by signaling each of them to stop.
     * The ball object still exists and holds the current position
     * of the ball.  It may be restarted later.
     * the current thread will be terminated.
     */
    void pause() {
	if (!paused) {
	    paused = true;
	    for (int i = 0; i < balls.length && balls[i] != null; i++) {
		balls[i].stop = true;
	    }
	    
	    try {
		if (player != null)
		    player.stop();
	    } catch (MediaException e) {
		// There's nothing much we can do here.
	    }
	}
	repaint();
    }
    
    /*
     * Start creates a new thread for each ball and start it.
     */
    void start() {
	if (paused) {
	    paused = false;
	    // display.setCurrent(this);
	    for (int i = 0; i < balls.length && balls[i] != null; i++) {
		Thread t = new Thread(balls[i]);
		t.start();
	    }
	    
	    if ( player != null) {
		try {
		    player.start();
		} catch (Exception ex) {}
	    }

	    repaint();
	}
    }

    public void commandAction(Command c, Displayable s) {
	if (c == backCommand) {
	    destroy();
	    display.setCurrent(BBall.getList());
	} else if ( c == pauseCommand) {
	    pause();
	    removeCommand(pauseCommand);
	    addCommand(playCommand);
	} else if ( c == playCommand) {
	    removeCommand(playCommand);
	    addCommand(pauseCommand);
	    start();
	}
    }
    
}
