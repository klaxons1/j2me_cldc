/*
 * @(#)Hanoi.java	1.3 02/08/15 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.hanoi;

import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;

import javax.microedition.midlet.MIDlet;

public class Hanoi extends MIDlet
    implements Runnable, CommandListener {

    public Hanoi() {
	super();

	// Set up the user interface
	m_Display = Display.getDisplay(this);
	m_Canvas  = new HanoiCanvas(this);

	m_Canvas.setCommandListener(this);
	m_Canvas.addCommand(solveCommand);
	m_Canvas.addCommand(exitCommand);
    }

    public void startApp() {
	m_Display.setCurrent(m_Canvas);
        if(m_Hanoi == null)  {
            m_Hanoi = new Thread(this);
            m_Hanoi.start();
        }
    }

    public void destroyApp(boolean force) {
	m_Canvas.exit();
	m_Hanoi = null;
	m_Display.setCurrent(null);
    }

    public void pauseApp() {
	m_Canvas.exit();
	m_Hanoi = null;
    }

    public void run() {
        do {
	    synchronized(m_Canvas) {
		try {
		    m_Canvas.wait();
		} catch (InterruptedException ie) {
		    continue;               // Go back through while(true) loop
		}

		while (m_Canvas.isSolving()) {
		    m_Canvas.updateInfo();
		}
		if (!m_Canvas.isSolving()) {
		    m_Canvas.removeCommand(stopCommand);
		    m_Canvas.addCommand(resetCommand);
		}
	    }
        }
        while(!m_Canvas.isExiting());
	m_Canvas = null;
    }

    public void commandAction(Command c, Displayable d) {
	if (c == exitCommand) {
	    destroyApp(false);
	    notifyDestroyed();
	} else if (c == solveCommand) {
	    if (!m_Canvas.isSolving()) {
		m_Canvas.solve();
	    }
	    m_Canvas.removeCommand(solveCommand);
	    m_Canvas.addCommand(stopCommand);
	} else if (c == stopCommand) {
            m_Canvas.stop();
	    m_Canvas.removeCommand(stopCommand);
	    m_Canvas.addCommand(resetCommand);
	} else if (c == resetCommand) {
	    m_Canvas.resetTowers();
	    m_Canvas.removeCommand(resetCommand);
	    m_Canvas.addCommand(solveCommand);
	}
    }

    private Thread      m_Hanoi;
    private HanoiCanvas m_Canvas;
    private Display     m_Display;
    private Command     solveCommand = new Command("Solve", Command.SCREEN, 1);
    private Command     stopCommand  = new Command("Stop",  Command.SCREEN, 2);
    private Command     resetCommand = new Command("Reset", Command.SCREEN, 2);
    private Command     exitCommand  = new Command("Exit",  Command.EXIT, 1);

}
