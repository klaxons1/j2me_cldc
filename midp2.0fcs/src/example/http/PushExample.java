/*
 * PushExample.java
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.http;

import javax.microedition.midlet.*;
import javax.microedition.io.*;
import javax.microedition.lcdui.*;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.DataInputStream;
import java.util.Vector;
import java.util.Date;

import java.lang.String;


/**
 * An example MIDlet to fetch a page using an HttpConnection.
 * Refer to the startApp, pauseApp, and destroyApp
 * methods so see how it handles each requested transition.
 */
public class PushExample extends MIDlet implements CommandListener {

    /** user interface command for indicating Exit request. */
    Command exitCommand  = new Command("Exit", Command.EXIT, 2);

    /** user interface component containing a list of URLs */
    List urlList;
    /** array of current URLs */
    Vector urls;
    /** user interface alert component. */
    Alert alert; 
    // Wait for 2sec
    static final int DefaultTimeout = 2000;
                   
    Image   newsHoundImage;

    boolean imageLoaded;
    /** current display. */
    Display display;

    /** current requested url. */
    String url;			

    /** initialize the MIDlet with the current display object. */
    public PushExample() {
        display = Display.getDisplay(this);

    }

    /**
     * Start creates the thread to do the timing.
     * It should return immediately to keep the dispatcher
     * from hanging.
     */
    public void startApp() {

       try {
            newsHoundImage = Image.createImage("/example/http/images/newshound.png");
	    imageLoaded = true;
        } catch (java.io.IOException ex) {
	    System.err.println("Image is not loaded :" + imageLoaded);
        }


        alert = new Alert("News Hound", "", newsHoundImage, AlertType.INFO);

        alert.setTimeout(DefaultTimeout);
        setupList();


	/* Bytes read from the URL update connection. */
	int count;
	/* Check for inbound async connection for sample Finger port. */
	String[] connections = PushRegistry.listConnections(true);

	/* HttpView was started to handle inbound request. */
	String pushProperty = getAppProperty("MIDlet-Push-1");

	if (connections != null && connections.length > 0) {
	    String newurl = "Pushed URL Placeholder";

	    /* DEBUG: Test basic get registry information interfaces. */
	    try {
		String midlet = PushRegistry.getMIDlet(connections[0]);
		String filter = PushRegistry.getFilter(connections[0]);

	    } catch (Exception e) {
		e.printStackTrace();
	    }
		    
	    /* Check for socket or datagram connection. */
	    if (connections[0].startsWith("socket://")) {
		try {
		    /* Simple test assumes a server socket connection. */
		    ServerSocketConnection scn = (ServerSocketConnection)
			Connector.open(connections[0]);
		    SocketConnection sc = (SocketConnection)
			scn.acceptAndOpen();
		    
		    /* Read one line of text as a new URL to add to the list. */
		    DataInputStream dis = sc.openDataInputStream();
		    byte[] buf = new byte[256];
		    int endofline = 0;
		    count =	dis.read(buf);
		    for (int i = 0; i < count; i++) {
			if (buf[i] == '\n') {
			    endofline = i;
			    break;
			}
		    }
		    newurl = new String(buf, 0, endofline);

		    dis.close();

		    sc.close();
		    scn.close();
		} catch (IOException e) {
		    System.err.println("******* io exception in push example");
		    e.printStackTrace();
		}		
	    } else {
		System.err.println(" NYI - unknown connection type");
	    }
	    urlList.append(newurl, null);
	    urls.addElement(newurl);
	} else {
	    connections = PushRegistry.listConnections(false);	   
	}

	display.setCurrent(alert, urlList);
    }

    /**
     * Pause signals the thread to stop by clearing the thread field.
     * If stopped before done with the iterations it will
     * be restarted from scratch later.
     */
    public void pauseApp() {
    }

    /**
     * Destroy must cleanup everything.  The thread is signaled
     * to stop and no result is produced.
     * @param unconditional true if a forced shutdown was requested
     */
    public void destroyApp(boolean unconditional) {
    }

    /**
     * Check the attributes in the descriptor that identify
     * url's and titles and initialize the lists of urls
     * and urlList.
     * <P>
     * The attributes are named "ViewTitle-n" and "ViewURL-n".
     * The value "n" must start at "1" and increment by 1.
     */
    void setupList() {
        urls = new Vector();
        urlList = new List("News Headlines", List.IMPLICIT);
	urlList.setFitPolicy(Choice.TEXT_WRAP_OFF);
        urlList.addCommand(exitCommand);
        urlList.setCommandListener(this);

        for (int n = 1; n < 100; n++) {
            String nthURL = "ViewURL-"+ n;
            String url = getAppProperty(nthURL);
            if (url == null || url.length() == 0) {
		break;
            }

            String nthTitle = "ViewTitle-" + n;
            String title = getAppProperty(nthTitle);
            if (title == null || title.length() == 0) {
		title = url;
            }

            urls.addElement(url);
            urlList.append(title, null);
        }

        urlList.append("Next InComing News: ", null);
    }

    /**
     * Respond to commands, including exit
     * @param c user interface command requested
     * @param s screen object initiating the request
     */
    public void commandAction(Command c, Displayable s) {
        try {
            if (c == exitCommand) {
                destroyApp(false);
                notifyDestroyed();
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

}
