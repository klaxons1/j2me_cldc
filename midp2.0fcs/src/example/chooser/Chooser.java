/*
 * @(#)Chooser.java	1.8 02/07/25 @(#)
 *
 * Copyright (c) 1999-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.chooser;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

import example.About;

public class Chooser extends MIDlet implements CommandListener {
    private Display display;		// Our display
    private FontChooser fonts;
    private TextSample sample;
    private ColorChooser colors;

    private Command exitCommand = 
	new Command("Exit", Command.EXIT, 1);
    private Command textColorCommand = 
	new Command("Text Color", Command.SCREEN, 3);
    private Command backgroundColorCommand = 
	new Command("Background Color", Command.SCREEN, 4);
    private Command fontsCommand = 
	new Command("Fonts", Command.SCREEN, 11);
    private Command aboutCommand = 
	new Command("About", Command.SCREEN, 99);
    
    private Command okCommand = 
	new Command("Ok", Command.SCREEN, 2);
    private Command okFgCommand = 
	new Command("Ok", Command.SCREEN, 2);
    private Command okBgCommand = 
	new Command("Ok", Command.SCREEN, 2);

    public Chooser() {
	display = Display.getDisplay(this);
	sample = new TextSample();
	
	sample.addCommand(exitCommand);
	sample.addCommand(aboutCommand);
	sample.addCommand(textColorCommand);
	sample.addCommand(backgroundColorCommand);
	sample.addCommand(fontsCommand);
	sample.setCommandListener(this);
    }

    /**
     * Create the FontChooser and make it current
     */
    public void startApp() {
	display.setCurrent(sample);
    }

    /**
     * Pause
     */
    public void pauseApp() {
    }

    /**
     * Destroy must cleanup everything.
     */
    public void destroyApp(boolean unconditional) {
    }

    /*
     * Respond to a commands issued on any Screen
     */
    public void commandAction(Command c, Displayable s) {
	if (c == exitCommand) {
	    destroyApp(true);
	    notifyDestroyed();
	} else if (c == aboutCommand) {
	    About.showAbout(display);
	} else if (c == fontsCommand) {
	    if (fonts == null) {
		fonts = new FontChooser();
		fonts.setFace(sample.getFace());
		fonts.setStyle(sample.getStyle());
		fonts.setSize(sample.getSize());
		fonts.addCommand(okCommand);
		fonts.setCommandListener(this);
	    }
	    display.setCurrent(fonts);
	} else if (c == backgroundColorCommand) {
	    if (colors == null) {
		colors = new ColorChooser(display.isColor(), 
					  display.numColors());
		colors.setCommandListener(this);
	    }
	    colors.addCommand(okBgCommand);
	    colors.removeCommand(okFgCommand);
	    colors.setColor(sample.getBackgroundColor());
	    display.setCurrent(colors);
	} else if (c == textColorCommand) {
	    if (colors == null) {
		colors = new ColorChooser(display.isColor(), 
					  display.numColors());
		colors.setCommandListener(this);
	    }
	    colors.addCommand(okFgCommand);
	    colors.removeCommand(okBgCommand);

	    colors.setColor(sample.getForegroundColor());
	    display.setCurrent(colors);
	} else if (c == okCommand) {
	    if (s == fonts) {
		sample.setStyle(fonts.getStyle());
		sample.setFace(fonts.getFace());
		sample.setSize(fonts.getSize());
	    }
	    display.setCurrent(sample);
	} else if (c == okFgCommand) {
	    sample.setForegroundColor(colors.getColor());
	    display.setCurrent(sample);
	} else if (c == okBgCommand) {
	    sample.setBackgroundColor(colors.getColor());
	    display.setCurrent(sample);
	}
    } 
}
