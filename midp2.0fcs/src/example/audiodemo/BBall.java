/*
 * @(#)BBall.java	1.6 02/08/15 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.audiodemo;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

/* 
 * this is a sample prpgram to show the synchronized tone rings with
 * selected background music.
 * It is based on the manyballs midlet. When a ball hits a wall,
 * ring a tone.
 *
 * Usuage: Use right arrow key to add one more ball (5 is the maxium).
 *         Use left arrow key to delete one ball
 *         Use up   arrow key to increase the speed of ball
 *         Use down arrow key to reduce the speed of ball
 * 
 * @author Sharon.Huang@Sun.Com
 * @version 1.0
 */
 
public class BBall extends MIDlet implements CommandListener, Runnable {

    private static final String[] bgs = {"no background", "wave background", 
					 "tone seq background"};
    static String wavbgUrl;
    private Command               exitCommand = new Command("Exit",
                                                    Command.EXIT, 1);
    private Command               playCommand = new Command("Play",
							    Command.ITEM, 1);
    private Display               display;
    private int idx = 0;
    private static List           theList;
    private static BBall          instance = null;
    
    BallCanvas                    game;

    static public BBall getInstance() {
        return instance;
    }
    
    public static  List getList() {
        return theList;
    }
    
    public BBall() {
        instance = this;
        display  = Display.getDisplay(this);
        theList  = new List("Backgrounds", Choice.IMPLICIT);
        for (int i = 0; i < bgs.length; i++) {
            theList.append(bgs[i], null);
        }
	wavbgUrl = getAppProperty("BBallURL");
        theList.addCommand(playCommand);
        theList.addCommand(exitCommand);
        theList.setCommandListener(this);
    }
    
    public void startApp() {
        display.setCurrent(theList);
    }

    /**
     * Pause is a no-op since there are no background activities or
     * record stores that need to be closed.
     */
    public void pauseApp() {}

    /**
     * Destroy must cleanup everything not handled by the garbage collector.
     */
    public void destroyApp(boolean unconditional) {
	if (game != null)
	    game.destroy();
    }
    
    /*
     * Respond to commands, including exit
     * On the exit command, cleanup and notify that the MIDlet has
     * been destroyed.
     */
    public void commandAction(Command c, Displayable s) {

        if (c == exitCommand) {
            destroyApp(false);
            notifyDestroyed();
       } else if ((s == theList && c == List.SELECT_COMMAND) || c == playCommand) {
            idx = theList.getSelectedIndex();
	    new Thread(this).start();
        }
    }

    public void run() {
	game = new BallCanvas(display, 5, idx);
	if (game.needAlert()) {
	    Alert alert = new Alert("Warning", "Can not create player", null, null);
	    alert.setTimeout(1000);
	    display.setCurrent(alert, game);
	} else {
	    display.setCurrent(game);
	}

	game.start();
    }
	
}

