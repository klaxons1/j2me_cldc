/*
 * @(#)MixCanvas.java	1.10 02/08/15 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.audiodemo;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.media.*;
import javax.microedition.media.control.*;

import java.io.*;

public class MixCanvas extends Canvas implements CommandListener, Runnable {
    private static final String TITLE_TEXT = "audio player";
    private static final int[]  notes = {69, 70,71,72,73,74, 75, 76};
    static Player  wavPlayer = null, tonePlayer = null;

    static Image logo = null;
    private int idx = 0, ip =0;

    Display parentDisplay;
    private Command     backCommand       = new Command("Back", Command.BACK,
                                                1);
    private Command     playCommand       = new Command("Play",
                                                Command.ITEM, 1);
    private Command     pauseCommand      = new Command("Pause",
                                                Command.ITEM, 1);

    private Command     toneCommand       = new Command("Tone",
						Command.ITEM, 1);
    
    private Alert alert;

    public MixCanvas(Display parentDisplay) {
        super();
	this.idx = 0;
	this.parentDisplay = parentDisplay;
	initialize();
    }

    void initialize() {
        addCommand(backCommand);
        setCommandListener(this);

	try {
	    logo = Image.createImage("/icons/logo.png");
	} catch (Exception ex) {
	    logo = null;
	}
	if ( logo == null) 
	    System.out.println("can not load logo.png");
	
	alert = new Alert("Warning", "Can not create player", null, null);
	alert.setTimeout(1000);

    }

    /*
     * Respond to commands, including back
     */
    public void commandAction(Command c, Displayable s) {

        if (s == this) {
            if (c == backCommand) {
                stopSound();
                parentDisplay.setCurrent(MixTest.getList());
	    } else if ( c == toneCommand ) {
		try {
		    Manager.playTone(notes[ip], 1000, 100);
		    ip++;
		    if ( ip >= 8 )
			ip = 0;
		} catch (Exception ex) {
		    System.out.println("get an exception for tone");
		}
            } else if (c == playCommand) {
                playSound();
            } else if (c == pauseCommand) {
                pauseSound();
	    }
        }
    }

    public void setIndex(int idx) {
	this.idx = idx;
    }

    private void createWavPlayer() {
	// method createWavPlayer() runs on GUI thread. Manager.createPlayer()
	// will potentially invoke a blocking I/O. This is not the good
	// practice recommonded by MIDP programming style. So here we
	// will create the Player in a separate thread.
	new Thread(this).start();
    } 

    public void run() {
	try {
	    if (wavPlayer == null) {
		wavPlayer = Manager.createPlayer(MixTest.wavUrl);
		wavPlayer.setLoopCount(-1);
	    }
	    wavPlayer.start();
	} catch (Exception ex) {
	    // ex.printStackTrace();
	    if (wavPlayer != null) {
		wavPlayer.close();
		wavPlayer = null;
	    }
	    parentDisplay.setCurrent(alert);
	}
    }

    private void createTonePlayer() {
	byte d = 8; 
	byte C4 = ToneControl.C4;
	byte D4 = ToneControl.C4 + 2; // a whole step 
	byte E4 = ToneControl.C4 + 4; // a major third
	byte G4 = ToneControl.C4 + 7; // a fifth
	byte rest = ToneControl.SILENCE; // eighth-note rest
		    
	byte[] mySequence = new byte[] {
		ToneControl.VERSION, 1,
		ToneControl.TEMPO, 30,
		ToneControl.BLOCK_START, 0,
		E4,d,D4,d,C4,d,D4,d,E4,d,E4,d,E4,d,rest,d, 
		ToneControl.BLOCK_END,0,
		ToneControl.PLAY_BLOCK,0,
		D4,d,D4,d,D4,d,rest,d,E4,d,G4,d,G4,d,rest,d,//play "B" section 
		ToneControl.PLAY_BLOCK,0, // content of "A" section 
		D4,d,D4,d,E4,d,D4,d,C4,d,rest,d// play "C" section 
	};
	
	try {
	    if ( tonePlayer == null) {
		tonePlayer = Manager.createPlayer(Manager.TONE_DEVICE_LOCATOR);
		tonePlayer.setLoopCount(-1);
		tonePlayer.realize();
		ToneControl tc = (ToneControl)tonePlayer.getControl("javax.microedition.media.control.ToneControl");
		tc.setSequence(mySequence);
	    }
	    tonePlayer.start();
	} catch (Exception ex) {
	    // ex.printStackTrace();
	    if (tonePlayer != null) {
		tonePlayer.close();
		tonePlayer = null;
	    }

	    parentDisplay.setCurrent(alert);
	}
    }

    public void playSound() {
	switch(idx) {
	case 0: // wave + tone
	    addCommand(toneCommand);
	    createWavPlayer();
	    break;
	    
	case 1: // toneseq + tone
	    addCommand(toneCommand);
	    createTonePlayer();
	    break;
	case 2: 
	    removeCommand(playCommand);
	    addCommand(pauseCommand);
	    createWavPlayer();
	    createTonePlayer();
	    break;
	}

    }

    public void stopSound() {
	if (tonePlayer != null) {
	    tonePlayer.close();
	    tonePlayer = null;
	}

	if (wavPlayer != null) {
	    wavPlayer.close();
	    wavPlayer = null;
	}
	
	switch(idx) {
	case 0:
	case 1:
	    removeCommand(toneCommand);
	    break;
	case 2:
	    removeCommand(pauseCommand);
	    removeCommand(playCommand);
	    break;
	}
    }
    
    public void pauseSound() {
	removeCommand(pauseCommand);
	addCommand(playCommand);

	try {
	    if ( wavPlayer != null) 
		wavPlayer.stop();

	    if ( tonePlayer != null )
		tonePlayer.stop();
	} catch (MediaException e) {
	    // Nothing much to do here.
	}
    }

    
    public void paint(Graphics g) {
	int w = getWidth();
	int h = getHeight();
	String cname = "";

	switch(idx) {
	case 0:
	    cname = "test-wav.wav";
	    break;
	case 1:
	    cname = "tone seq";
	    break;
	case 2:
	    cname = "wave+toneseq";
	    break;
	}

	g.setColor(0);
	g.fillRect(0, 0, w, h);

	if ( logo != null ) {
	    g.drawImage(logo, w/2, 30, Graphics.TOP | Graphics.HCENTER);
	}
	g.setColor(0xFF7f00);
	g.drawString("audio Player", w/2, 8, Graphics.TOP | Graphics.HCENTER);
	//g.drawString("audio player!", w/2, 64, Graphics.TOP | Graphics.HCENTER);
	g.drawString(cname, 0, 84, Graphics.TOP | Graphics.LEFT);

    }

    protected void keyPressed(int keycode) {
	switch (keycode) {
	case KEY_STAR:
	    changeVolume(-10);
	    break;
	case KEY_POUND:
	    changeVolume(10);
	    break;
	}
    }

    private void changeVolume(int diff) {
	VolumeControl vc;

	if ( wavPlayer != null) {
	    vc = (VolumeControl) wavPlayer.getControl("VolumeControl");
	    if (vc != null) {
		int cv = vc.getLevel();
		cv += diff;
		cv = vc.setLevel(cv);
	    }
	}

	if ( tonePlayer != null) {
	    vc = (VolumeControl) tonePlayer.getControl("VolumeControl");
	    if (vc != null) {
		int cv = vc.getLevel();
		cv += diff;
		cv = vc.setLevel(cv);
	    }
	}
    }

}
