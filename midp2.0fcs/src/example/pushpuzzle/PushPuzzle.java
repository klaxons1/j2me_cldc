/*
 * @(#)PushPuzzle.java	1.29 02/07/25 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.pushpuzzle;

import javax.microedition.lcdui.*;
import javax.microedition.midlet.*;
import java.io.*;
import example.About;

/**
 * PushPuzzle is the MIDlet that drives the game.  It puts up the
 * screens and handles all the commands that are invoked on each
 * screen.
 */
public class PushPuzzle extends MIDlet implements CommandListener {
    Display display;
    private PushPuzzleCanvas canvas;
    private Score score;
    private Screen scoreScreen;
    private Screen levelScreen;
    private Alert alert;

    private Command undoCommand = new Command("Undo", Command.BACK, 1);
    private Command restartCommand =
	new Command("Start Over", Command.CANCEL, 21);
    private Command exitCommand =
	new Command("Exit", Command.EXIT, 60);
    private Command scoresCommand =
	new Command("Show Scores", Command.SCREEN, 26);
    private Command okCommand = 
	new Command("OK", Command.OK, 30);
    private Command levelCommand =
	new Command("Change Level", Command.SCREEN, 24);
    private Command nextCommand =
	new Command("Next Level", Command.SCREEN, 22);
    private Command prevCommand =
	new Command("Previous Level", Command.SCREEN, 23);
    private Command aboutCommand = 
	new Command("About", Command.HELP, 30);
    private Command themeCommand = 
	new Command("Switch Theme", Command.SCREEN, 25);

    /**
     * Creates new PushPuzzle MIDlet.
     */
    public PushPuzzle() {
	display = Display.getDisplay(this);
	score = new Score();
	canvas = new PushPuzzleCanvas(this, score);
	alert = new Alert("Warning");
    }

    /**
     * Start creates the thread to do the timing.
     * It should return immediately to keep the dispatcher
     * from hanging.
     */
    public void startApp() {
	if (!score.open()) {
	    System.out.println("Score open failed");
	}

	canvas.init();
	canvas.addCommand(undoCommand);
	canvas.addCommand(scoresCommand);
	canvas.addCommand(restartCommand);
	canvas.addCommand(levelCommand);
	canvas.addCommand(exitCommand);
	canvas.addCommand(nextCommand);
	canvas.addCommand(prevCommand);
	canvas.addCommand(aboutCommand);
	canvas.addCommand(themeCommand);
	canvas.setCommandListener(this);

	display.setCurrent(canvas);
    }

    /**
     * Pause signals the thread to stop by clearing the thread field.
     * If stopped before done with the iterations it will
     * be restarted from scratch later.
     */
    public void pauseApp() {
    }

    /**
     * Destroy must cleanup everything.
     * Only objects exist so the GC will do all the cleanup
     * after the last reference is removed.
     */
    public void destroyApp(boolean unconditional) {
	display.setCurrent((Displayable)null);
	canvas.destroy();
	if (score != null)
	    score.close();
    }

    /**
     * Respond to a commands issued on any Screen
     */
    public void commandAction(Command c, Displayable s) {
	if (c == undoCommand) {
	    canvas.undoMove();
	} else if (c == restartCommand) {
	    canvas.restartLevel();
	} else if (c == levelCommand) {
	    levelScreen = canvas.getLevelScreen();
	    levelScreen.addCommand(okCommand);
	    levelScreen.setCommandListener(this);
	    display.setCurrent(levelScreen);
	} else if (c == okCommand && s == levelScreen) {

	    if (!canvas.gotoLevel()) {
		alert.setString("Could not load level");
		display.setCurrent(alert, canvas);
	    } else {
		display.setCurrent(canvas);
	    }
	} else if (c == scoresCommand) {
	    scoreScreen = canvas.getScoreScreen();
	    scoreScreen.addCommand(okCommand);
	    scoreScreen.setCommandListener(this);
	    display.setCurrent(scoreScreen);
	} else if (c == okCommand && s == scoreScreen) {
	    display.setCurrent(canvas);
	} else if (c == exitCommand) {
	    destroyApp(false);
	    notifyDestroyed();
	} else if (c == List.SELECT_COMMAND && s == canvas) {
	    // Solved the level
	    scoreScreen = canvas.getScoreScreen();
	    scoreScreen.addCommand(okCommand);
	    scoreScreen.setCommandListener(this);
	    display.setCurrent(scoreScreen);

	    // Read the next screen.
	    canvas.nextLevel(1);
	} else if (c == nextCommand) {
	    if (!canvas.nextLevel(1)) {
		alert.setString("Could not load level " +
				(canvas.getLevel() + 1));
		display.setCurrent(alert, canvas);
	    } else {
		display.setCurrent(canvas);
	    }
            if (s == canvas) {
                canvas.repaint();
            }
	} else if (c == prevCommand) {
	    if (!canvas.nextLevel(-1)) {
		alert.setString("Could not load level " +
				(canvas.getLevel() - 1));
		display.setCurrent(alert, canvas);
	    } else {
		display.setCurrent(canvas);
	    }
            if (s == canvas) {
                canvas.repaint();
            }
	} else if (c == aboutCommand) {
	    About.showAbout(display);
	} else if (c == themeCommand) {
	    canvas.changeTheme();
	}
    }
}
