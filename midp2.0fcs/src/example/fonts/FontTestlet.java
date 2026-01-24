/*
 * @(#)FontTestlet.java	1.6 02/07/25 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.fonts;

//import java.io.*;
//import javax.microedition.midlet.*;
//import javax.microedition.lcdui.*;
import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.CommandListener;

/**
 * FontTestlet is simple MIDlet which attempts to display
 * text in all of the MIDP's different fonts.
 */

public class FontTestlet extends MIDlet implements CommandListener {

    private Display     myDisplay;
    private FontCanvas  myCanvas;

    private int currentFace = Font.FACE_SYSTEM;

    private Command monospaceCommand 
	= new Command("monospace", Command.ITEM, 1);
    private Command proportionalCommand 
	= new Command("proportional", Command.ITEM, 1);
    private Command systemCommand = new Command("system", Command.ITEM, 1);
  
    /**
     * FontTestlet - default constructor
     */
    public FontTestlet() {
	super();

	// Set up the user interface
	myDisplay = Display.getDisplay(this);
	myCanvas  = new FontCanvas(this);  // pointer to myself
	myCanvas.setCommandListener(this);
	myCanvas.addCommand(monospaceCommand);
	myCanvas.addCommand(proportionalCommand);
    }

    /**
     * initApp()
     */
    public void init() throws MIDletStateChangeException {
    }

    /**
     * startApp()
     */
    public void startApp() throws MIDletStateChangeException {
	myDisplay.setCurrent(myCanvas);
    }
  
    /**
     * pauseApp()
     */
    public void pauseApp() {
	// System.out.println("pauseApp()");
    }
  
    /**
     * destryApp()
     *
     * This is important.  It closes the app's RecordStore
     * @param cond true if this is an unconditional destroy
     *             false if it is not
     *             currently ignored and treated as true
     */
    public void destroyApp(boolean cond) {
	myDisplay.setCurrent((Displayable)null);
	myCanvas.destroy();
    }

    /**
     * draw some stuff to the graphics context
     */

    public void paint(Graphics g) {
	String title;
	int height = 0;

	g.setColor(0x00000000);
	g.fillRect(0, 0, myCanvas.getWidth(), myCanvas.getHeight());

	g.setColor(0x00ffffff);
	switch (currentFace) {
	case Font.FACE_SYSTEM:
	    title = "System";
	    break;
	case Font.FACE_PROPORTIONAL:
	    title = "Proportional";
	    break;
	case Font.FACE_MONOSPACE:
	    title = "Monospaced";
	    break;
	default:
	    title = "unknown";
	    break;
	}
	g.drawString(title, 0, 0, Graphics.TOP|Graphics.LEFT);
	height += g.getFont().getHeight();

	g.setFont(Font.getFont(currentFace, 
			       Font.STYLE_PLAIN, 
			       Font.SIZE_LARGE));
	g.drawString("Regular plain", 0, height, Graphics.TOP|Graphics.LEFT);
	height += g.getFont().getHeight();

	g.setFont(Font.getFont(currentFace, 
			       Font.STYLE_ITALIC, 
			       Font.SIZE_LARGE));
	g.drawString("Regular ital", 0, height, Graphics.TOP|Graphics.LEFT);
	height += g.getFont().getHeight();

	g.setFont(Font.getFont(currentFace, 
			       Font.STYLE_BOLD, 
			       Font.SIZE_LARGE));
	g.drawString("Bold plain", 0, height, Graphics.TOP|Graphics.LEFT);
	height += g.getFont().getHeight();

	g.setFont(Font.getFont(currentFace, 
			       Font.STYLE_BOLD|Font.STYLE_ITALIC, 
			       Font.SIZE_LARGE));
	g.drawString("Bold ital", 0, height, Graphics.TOP|Graphics.LEFT);
    }

    Command getCurrentCommand() {
	switch (currentFace) {
	case Font.FACE_MONOSPACE:
	    return monospaceCommand;
	case Font.FACE_PROPORTIONAL:
	    return proportionalCommand;
	case Font.FACE_SYSTEM:
	default:
	    return systemCommand;
	}
    }

    public void commandAction(Command cmd, Displayable disp) {
	myCanvas.addCommand(getCurrentCommand());
	if (cmd == monospaceCommand) {
	    myCanvas.removeCommand(monospaceCommand);
	    currentFace = Font.FACE_MONOSPACE;
	} else if (cmd == proportionalCommand) {
	    myCanvas.removeCommand(proportionalCommand);
	    currentFace = Font.FACE_PROPORTIONAL;
	} else if (cmd == systemCommand) {
	    myCanvas.removeCommand(systemCommand);
	    currentFace = Font.FACE_SYSTEM;
	}

	myCanvas.repaint();
    }
}
