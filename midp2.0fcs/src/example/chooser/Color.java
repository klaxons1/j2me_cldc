/*
 * @(#)Color.java	1.16 02/07/25 @(#)
 *
 * Copyright (c) 1999-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.chooser;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

import example.About;

/**
 * A Color chooser MIDlet.
 */
public class Color extends MIDlet implements CommandListener {
    /** This MIDlets Display object */
    private Display display;		// Our display
    /** The Color chooser */
    private ColorChooser chooser;
    /** The Exit Command */
    private Command exitCommand = new Command("Exit", Command.EXIT, 1);
    /** The Decimal Command */
    private Command decimalCommand = new Command("Decimal", Command.SCREEN, 7);
    /** The Hexadecimal Command */
    private Command hexCommand = new Command("Hexadecimal", Command.SCREEN, 7);
    /** The Coarse command */
    private Command coarseCommand = new Command("Coarse", Command.SCREEN, 8);
    /** The Fine command */
    private Command fineCommand = new Command("Fine", Command.SCREEN, 8);
    /** The Command to show the About box */
    private Command aboutCommand = new Command("About", Command.HELP, 30);

    /**
     * Construct a new Color MIDlet and initialize.
     */
    public Color() {
	display = Display.getDisplay(this);
	chooser = new ColorChooser(display.isColor(), display.numColors());
	
	chooser.addCommand(exitCommand);
	chooser.addCommand(hexCommand);
	chooser.addCommand(fineCommand);
	chooser.addCommand(aboutCommand);
	chooser.setCommandListener(this);

	chooser.setColor(0xffff00);
    }

    /**
     * Create the ColorChooser and make it current
     */
    public void startApp() {
	display.setCurrent(chooser);
    }

    /**
     * Pause
     */
    public void pauseApp() {
    }

    /**
     * Destroy must cleanup everything.
     * @param unconditional true if must destroy
     */
    public void destroyApp(boolean unconditional) {
    }

    /**
     * Respond to a commands issued on any Screen.
     * @param c Command invoked
     * @param s Displayable on which the command was invoked
     */
    public void commandAction(Command c, Displayable s) {
	if (c == exitCommand) {
	    destroyApp(true);
	    notifyDestroyed();
	} else if (c == decimalCommand) {
	    chooser.setRadix(10);
	    chooser.removeCommand(decimalCommand);
	    chooser.addCommand(hexCommand);
	} else if (c == hexCommand) {
	    chooser.setRadix(16);
	    chooser.removeCommand(hexCommand);
	    chooser.addCommand(decimalCommand);
	} else if (c == fineCommand) {
	    chooser.setDelta(4);
	    chooser.removeCommand(fineCommand);
	    chooser.addCommand(coarseCommand);
	} else if (c == coarseCommand) {
	    chooser.setDelta(32);
	    chooser.removeCommand(coarseCommand);
	    chooser.addCommand(fineCommand);
	} else if (c == aboutCommand) {
	    About.showAbout(display);
	} 
    } 
}
