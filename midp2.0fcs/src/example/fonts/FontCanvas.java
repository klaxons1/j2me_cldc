/*
 * @(#)FontCanvas.java	1.3 02/07/25 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.fonts;

//import javax.microedition.lcdui.*;
import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Graphics;

class FontCanvas extends Canvas {
  
    FontTestlet myTestlet;

    /**
     * Construct a new canvas
     */
    FontCanvas(FontTestlet fontTestlet) {
	myTestlet = fontTestlet;
    }
  
    /**
     * Initialize self.
     */
    void init() {
	// no setup stuff yet
    }

    /**
     * Cleanup and destroy.
     */
    void destroy() {
	// can't really get rid of mTestlet here
    }

    /*
     * Ask MemTestlet to paint itself
     */
    protected void paint(Graphics g) {
	myTestlet.paint(g);
    }
} 

