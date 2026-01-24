/*
 * @(#)TilePuzzle.java	1.10 02/07/25 @(#)
 *
 * Copyright (c) 2000-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.tilepuzzle;

import javax.microedition.midlet.MIDlet;
import javax.microedition.lcdui.Display;

public class TilePuzzle extends MIDlet {

	Board b;

	public TilePuzzle() {
		b = new Board(this);
	}

	public void startApp() {
		Display.getDisplay(this).setCurrent(b);
	}

	public void pauseApp() { }

	public void destroyApp(boolean unc) { }
		
}
