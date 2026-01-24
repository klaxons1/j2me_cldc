/*
 * @(#)MiniColor.java	1.10 02/07/25 @(#)
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
 * Create a single color chooser MIDlet. It uses the MiniColorChooser
 * as its main screen.
 */
public class MiniColor extends MIDlet implements CommandListener {
    /** The display for this MIDlet */
    private Display display;
    /** The MiniColorChooser */
    private MiniColorChooser chooser;
    /** The exit command */
    private Command exitCommand = new Command("Exit", Command.EXIT, 1);
    /** The About command */
    private Command aboutCommand = new Command("About", Command.HELP, 2);

    /**
     * Create the MiniColor MIDlet.
     * Create the mini chooser and set the commands and listener.
     */
    public MiniColor() {
	display = Display.getDisplay(this);
	chooser = new MiniColorChooser();
	
	chooser.addCommand(exitCommand);
	chooser.addCommand(aboutCommand);
	chooser.setCommandListener(this);

	chooser.setColor(0xffff00);
    }

    /**
     * Create the MiniColor and make it current
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
     * @param unconditional true if the MIDlet must destroy exit
     */
    public void destroyApp(boolean unconditional) {
    }

    /**
     * Respond to a commands issued on any Screen
     * @param c Command invoked
     * @param s Displayable on which the command was invoked
     */
    public void commandAction(Command c, Displayable s) {
	if (c == exitCommand) {
	    destroyApp(true);
	    notifyDestroyed();
	} else if (c == aboutCommand) {
	    About.showAbout(display);
	}
    }
}
