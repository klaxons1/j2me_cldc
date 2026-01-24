/*
 * @(#)ManyBalls.java	1.16 02/07/25 @(#)
 *
 * Copyright (c) 1999-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.manyballs;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

import example.About;

public class ManyBalls extends MIDlet implements CommandListener {

    Display display;
    ManyCanvas canvas;		// The main screen
    private Command exitCommand = new Command("Exit", Command.EXIT, 99);
    private Command toggleCommand = new Command("Stop/Go", Command.SCREEN, 1);
    private Command helpCommand = new Command("Help", Command.HELP, 2);
    private Command aboutCommand = new Command("About", Command.HELP, 30);
    private Form helpScreen;
    private String helpText = "^ = faster\n v = slower\n < = fewer\n> = more";


    // the GUI buttons
    //	Button exitButton, clearButton, moreButton, lessButton;

    /*
     * Create the canvas
     */
    public ManyBalls() {
	display = Display.getDisplay(this);

	canvas = new ManyCanvas(display, 40);
	canvas.addCommand(exitCommand);
	canvas.addCommand(toggleCommand);
	canvas.addCommand(helpCommand);
	canvas.addCommand(aboutCommand);
	canvas.setCommandListener(this);
    }

    public void startApp() throws MIDletStateChangeException {
	canvas.start();
    }
    
    public void pauseApp() {
	canvas.pause();
    }

    public void destroyApp(boolean unconditional) 
	throws MIDletStateChangeException {
	canvas.destroy();
    }

    /*
     * Respond to a command issued on the Canvas.
     */
    public void commandAction(Command c, Displayable s) {
	if (c == toggleCommand) {
	    if (canvas.isPaused())
		canvas.start();
	    else
		canvas.pause();
	} else if (c == helpCommand) {
	    canvas.pause();
	    showHelp();
	} else if (c == exitCommand) {
	    try {
		destroyApp(false);
		notifyDestroyed();
	    } catch (MIDletStateChangeException ex) {
	    }
	} else if (c == aboutCommand) {
	    About.showAbout(display);
	}
    }

    /*
     * Put up the help screen. Create it if necessary.
     * Add only the Resume command.
     */
    void showHelp() {
	if (helpScreen == null) {
	    helpScreen = new Form("Many Balls Help");
	    helpScreen.append("^ = faster\n");
	    helpScreen.append("v = slower\n");
	    helpScreen.append("< = fewer\n");
	    helpScreen.append("> = more\n");
	}
	helpScreen.addCommand(toggleCommand);
	helpScreen.setCommandListener(this);
	display.setCurrent(helpScreen);
    }



}
