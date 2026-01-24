/*
 * @(#)AudioPlayer.java	1.2 02/07/25 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.audiodemo;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.media.*;
import javax.microedition.media.control.*;
import java.util.*;
import java.io.*;

/**
 * This is a demo midlet to show the basic audio functionalities, to
 * play wave file, tone, tone sequence from http, resource jar file
 * and record store.
 *
 * @author Sharon.Huang@Sun.Com
 *
 **/


public class AudioPlayer extends MIDlet implements CommandListener {
    private static PlayerCanvas   playerGUI = null;
    private static List           theList;
    private static Vector         urls;

    private Command               exitCommand = new Command("Exit",
                                                    Command.EXIT, 1);
    private Command               playCommand = new Command("Play",
                                                    Command.ITEM, 1);

    private Display               display;

    
    public AudioPlayer() {
	super();
    }
    
    public static List getList() {
	return theList;
    }

    public void startApp() {
	display  = Display.getDisplay(this);
	initPlayList();
	display.setCurrent(theList);
    }


    public void pauseApp() {
    }

    public void destroyApp(boolean unconditional) {
	if (playerGUI != null) {
	    playerGUI.stopSound();
	    playerGUI = null;
	}
	display.setCurrent(null);
    }

    public void commandAction(Command c, Displayable s) {
        if (c == exitCommand) {
            destroyApp(true);
            notifyDestroyed();
       } else if ((s == theList && c == List.SELECT_COMMAND) ||
		  c == playCommand) {
	   int i = theList.getSelectedIndex();
	   if (i == 0) { // Simple tone
	       try {
		   Manager.playTone(60, 200, 90);
	       } catch (MediaException ex) {
		   System.out.println("can't play tone");
	       }
	   } else if (i > 0) {
	       if (playerGUI == null) 
		   playerGUI = new PlayerCanvas(display);
	       else 
		   playerGUI.stopSound();

	       playerGUI.setParam((String) urls.elementAt(i));
	       playerGUI.playSound();
	       display.setCurrent(playerGUI);
	   }
       }
    }

    /**
     * load the play list from thd jad file
     *
     **/
    private void initPlayList() {
	urls = new Vector();

	theList  = new List("MIDP Audio Player", Choice.IMPLICIT);

	for (int n = 1; n < 32; n++) {
	    String nthURL = "PlayerURL-"+ n;
	    String url = getAppProperty(nthURL);
	    if (url == null || url.length() == 0) {
		break;
	    }
            String nthTitle = "PlayerTitle-" + n;
            String title = getAppProperty(nthTitle);
            if (title == null || title.length() == 0) {
		title = url;
	    }
            urls.addElement(url);
            theList.append(title, null);
	}
	
        theList.addCommand(exitCommand);
        theList.addCommand(playCommand);
	theList.setCommandListener(this);
    }
}
