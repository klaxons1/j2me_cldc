/*
 * @(#)PropExample.java	1.20 02/07/25 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

/**
 * An example MIDlet shows the values of the system properties.
 * Refer to the startApp, pauseApp, and destroyApp
 * methods so see how it handles each requested transition.
 */
public class PropExample extends MIDlet implements CommandListener {
    private Display display;
    private Form props;
    private StringBuffer propbuf;
    private Command exitCommand = new Command("Exit", Command.EXIT, 1);


    /*
     * Construct a new PropExample.
     */
    public PropExample() {
	display = Display.getDisplay(this);
    }

    /**
     * Show the value of the properties
     */
    public void startApp() {
	Runtime runtime = Runtime.getRuntime();
	runtime.gc();
	long free = runtime.freeMemory();
	long total = runtime.totalMemory();


	propbuf = new StringBuffer(50);
	props = new Form("System Properties");

	props.append("Free Memory = " + free + "\n");
	props.append("Total Memory = " + total + "\n");

 	props.append(showProp("microedition.configuration"));
 	props.append(showProp("microedition.profiles"));

	props.append(showProp("microedition.platform"));
	props.append(showProp("microedition.locale"));
	props.append(showProp("microedition.encoding"));

	props.addCommand(exitCommand);
	props.setCommandListener(this);
	display.setCurrent(props);
    }

    public void commandAction(Command c, Displayable s) {
	if (c == exitCommand) {
	    destroyApp(false);
	    notifyDestroyed();
	}	
    }

    /**
     * Show a property.
     */
    String showProp(String prop) {
	String value = System.getProperty(prop);
	propbuf.setLength(0);
	propbuf.append(prop);
	propbuf.append(" = ");
	if (value == null) {
	    propbuf.append("<undefined>");
	} else {
	    propbuf.append("\"");
	    propbuf.append(value);
	    propbuf.append("\"");
	}
	propbuf.append("\n");
	return propbuf.toString();
    }

    /**
     * Time to pause, free any space we don't need right now.
     */
    public void pauseApp() {
	display.setCurrent((Displayable)null);
	propbuf = null;
	props = null;
    }

    /**
     * Destroy must cleanup everything.
     */
    public void destroyApp(boolean unconditional) {
    }

}

