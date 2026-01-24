/*
 * @(#)MixTest.java	1.12 02/07/25 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.audiodemo;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.media.*;

public class MixTest extends MIDlet implements CommandListener {

    private static final String[]  mcases = {"Tone+Wav", "Tone+ToneSeq", "ToneSeq+Wav"};
    static String wavUrl;
    private static MixCanvas      soundObj    = null;
    private Command               exitCommand = new Command("Exit",
							    Command.EXIT, 1);
    private Command               playCommand = new Command("Play",
							    Command.ITEM, 1);
    private Display               display;
    private static List           theList;
    private static MixTest        instance = null;
    
    static public MixTest getInstance() {
	return instance;
    }
    
    static public List getList() {
	return theList;
    }
    
    public MixTest() {
	instance = this;
	display  = Display.getDisplay(this);
	theList  = new List("Lists", Choice.IMPLICIT);
	for (int i = 0; i < mcases.length; i++) {
	    theList.append(mcases[i], null);
	}
	wavUrl = getAppProperty("MixTestURL");
	theList.addCommand(playCommand);
	theList.addCommand(exitCommand);
	theList.setCommandListener(this);
    }
    
    public void startApp() {
	if (soundObj != null ) {
	    soundObj.stopSound();
	} else {
	    soundObj = new MixCanvas(display);
	}
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
	if ( soundObj != null )
	    soundObj.stopSound();
	soundObj = null;
    }


    public void commandAction(Command c, Displayable s) {

	if (c == exitCommand) {
	    destroyApp(false);
	    notifyDestroyed();
	} else if ((s == theList && c == List.SELECT_COMMAND) || c == playCommand) {
	    int i = theList.getSelectedIndex();
	    soundObj.setIndex(i);
	    display.setCurrent(soundObj);
            soundObj.serviceRepaints();
	    soundObj.playSound();
	}
    }
}


