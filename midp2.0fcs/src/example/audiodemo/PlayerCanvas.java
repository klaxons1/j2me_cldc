/*
 * @(#)PlayerCanvas.java	1.6 02/08/15 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.audiodemo;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.rms.*;
import javax.microedition.media.*;
import javax.microedition.media.control.*;

import java.io.*;

/**
 * The component for AudioPlayer.
 * It will create a player for the selected url, play and display
 * some properties of the player.
 *
 * Use star key to increase the volume, pound key to decrease the volume
 *
 **/
public class PlayerCanvas extends Canvas implements Runnable, CommandListener {

    private Player player;
    private Thread dThread;

    private Image logo = null;

    private Display parentDisplay;
    private Command     backCommand       = new Command("Back", Command.BACK,
                                                1);
    private Command     playCommand       = new Command("Play",
                                                Command.ITEM, 1);
    private Command     pauseCommand       = new Command("Pause",
                                                Command.ITEM, 10);
    private boolean disMTime, interrupted;
    private String title, url;
    private String mtime;

    public PlayerCanvas(Display parentDisplay) {
        super();
	this.parentDisplay = parentDisplay;
	initialize();
    }

    private void initialize() {
	this.addCommand(backCommand);
	this.addCommand(pauseCommand);
	
	setCommandListener(this);
	try {
	    logo = Image.createImage("/icons/logo.png");
	} catch (Exception ex) {
	    logo = null;
	}
	if ( logo == null) 
	    System.out.println("can not load logo.png");
	
    }

    public void commandAction(Command c, Displayable s) {
        if (s == this) {
            if (c == backCommand) {
                stopSound();
		removeCommand(playCommand);
		addCommand(pauseCommand);
                parentDisplay.setCurrent(AudioPlayer.getList());
            } else if (c == playCommand) {
                playSound();
		removeCommand(playCommand);
		addCommand(pauseCommand);
            } else if (c == pauseCommand) {
                pauseSound();
		removeCommand(pauseCommand);
		addCommand(playCommand);
	    }
        }
    }

    public void setParam(String url) {
	this.url = url;
	int idx = url.lastIndexOf('/');
	title = url.substring(idx+1);
    }

    public void playSound() {
	if (title == null || url == null)
	    return;
	
	if (dThread == null) {
	    dThread = new Thread(this);
	    mtime = "";
	    disMTime = true;
	    interrupted = false;
	    dThread.start();
	} else if (player != null) {
	    try {
		player.start();
	    } catch (Exception ex){}
	}

    }

    public void stopSound() {
	try {
	    interrupted = true;
	    disMTime = false;
	    dThread = null;
	    
	    Thread.sleep(100);
	    
	    
	    if (player != null) {
		player.close();
		player = null;
	    }

	} catch (Exception ex) {}
    }

    void pauseSound() {
	try {
	    if (player != null)
		player.stop();
	} catch (MediaException ex){}
    }
	
    void createPlayer() {
	try {
	    if (url.startsWith("http:")) {
		player = Manager.createPlayer(url);
	    } else if (url.startsWith("resource")) {
		int idx = url.indexOf(':');
		String loc = url.substring(idx+1);
		InputStream is = getClass().getResourceAsStream(loc);
		String ctype;
		if (url.endsWith("wav")) {
		    ctype = "audio/x-wav";
		} else {
		    ctype = "audio/x-tone-seq";
		}

		player = Manager.createPlayer(is, ctype);
	    } else if (url.startsWith("rms:")) {
		boolean found = false;
		InputStream stream = null;
		try {
		    RecordStore rs = RecordStore.openRecordStore("adrms", false);
		    byte[] adata = rs.getRecord(1);
		    rs.closeRecordStore();
		    stream = new ByteArrayInputStream(adata);
		    found = true;
		} catch (Exception ex) {
		    found = false;
		    createMyRecordStore();
		}

		if (!found) {
		    RecordStore rs = RecordStore.openRecordStore("adrms", false);
		    byte[] adata = rs.getRecord(1);
		    rs.closeRecordStore();
		    stream = new ByteArrayInputStream(adata);
		}

		String ctype;
		if (url.endsWith("wav")) {
		    ctype = "audio/x-wav";
		} else {
		    ctype = "audio/x-tone-seq";
		}

		player = Manager.createPlayer(stream, ctype);
	    }
		
	    player.setLoopCount(-1);
	} catch (Exception ex) {
	    if (player != null) {
		player.close();
		player = null;
	    }

	    Alert alert = new Alert("Warning", "Cannot create player", null, null);
	    alert.setTimeout(1000);
	    
	    parentDisplay.setCurrent(alert);

	}
	
    }

    private void createMyRecordStore() {
	try {
	    int idx = url.indexOf(':');
	    String loc = url.substring(idx+1);
	    InputStream is = getClass().getResourceAsStream(loc);
	    byte[] data = new byte[4096];
	    int nread = 0, tmp;

	    while ( (tmp = is.read(data, nread, 1024)) > 0 ) {
		nread += tmp;
	    }

	    is.close();
	    // create a RecordStore
	    RecordStore rs = RecordStore.openRecordStore("adrms", true);
	    //int id = rs.addRecord(data, 0, data.length);
	    int id = rs.addRecord(data, 0, nread);
	    rs.closeRecordStore();
	    System.out.println("create a new record store");
	} catch (Exception ex) {
	    ex.printStackTrace();
	}
    }

    public void paint(Graphics g) {
	int w = getWidth();
	int h = getHeight();

	g.setColor(0);
	g.fillRect(0, 0, w, h);

	g.setColor(0xFF7f00);
	g.drawString("Audio Player", w/2, 8, Graphics.TOP | Graphics.HCENTER);

	if ( logo != null ) {
	    g.drawImage(logo, w/2, 30, Graphics.TOP | Graphics.HCENTER);
	}
	g.setColor(0xFF7f00);
	g.drawString("Audio Player", w/2, 8, Graphics.TOP | Graphics.HCENTER);

	g.drawString(title, w/2, 84, Graphics.TOP | Graphics.HCENTER);

	g.drawString(mtime, 0, 150, Graphics.TOP | Graphics.LEFT);
    }

    public void run() {
	if (player == null) {
	    try {
		// method playSound() runs on GUI thread.
		// Manager.createPlayer() will potentially invoke a blocking 
		// I/O. This is not the good practice recommonded by MIDP 
		// programming style. So here we will create the
		// Player in a separate thread.
		createPlayer();
		player.realize();
		long dur = player.getDuration();
		if (dur != -1)
		    title = title + " [" + timeFM(dur)+ "]";
		player.start();
	    } catch (Exception ex){}
	}

	while (!interrupted) {
	    try {
		if (disMTime) {
		    mtime = timeFM(player.getMediaTime());
		    repaint(0,110, 100, 170);
		}
		Thread.sleep(100);
	    } catch (Exception ex){}
	}
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

	if ( player != null) {
	    vc = (VolumeControl) player.getControl("VolumeControl");
	    if (vc != null) {
		int cv = vc.getLevel();
		cv += diff;
		cv = vc.setLevel(cv);
	    }
	}
    }
    

    private String timeFM(long val) {
	String ret = "";
	int mval = (int)(val/1000);
	int sec = mval/1000;
	int min = sec/60;
	if (min >= 10)
	    ret = ret + min + ":";
	else if (min > 0)
	    ret = "0" + min + ":";
	else
	    ret = "00:";

	if (sec >= 10)
	    ret = ret + sec + ".";
	else if (sec > 0)
	    ret = ret + "0" + sec + ".";
	else
	    ret = ret + "00.";

	mval = (mval % 1000) / 100;
	ret = ret + mval;

	return (ret);
    }
	
}



	  
    
    
