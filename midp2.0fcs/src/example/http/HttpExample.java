/*
 * @(#)HttpExample.java	1.21 02/07/25 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/**
 * Examples using HttpConnection.
 */

package example.http;

import java.io.*;
import javax.microedition.io.Connector;
import javax.microedition.io.ContentConnection;
import javax.microedition.io.StreamConnection;
import javax.microedition.io.HttpConnection;

import javax.microedition.lcdui.Gauge;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Display;

import javax.microedition.midlet.*;
/**
 * sample http example MIDlet.
 */
public class HttpExample extends MIDlet implements Runnable {
    /** example URL for HTTP GET */
    String url = "http://www.yahoo.com/";
    /** string buffer for assembling HTTP requests. */
    StringBuffer buffer = new StringBuffer();
    /** user interface component for displaying network progress. */
    Gauge gauge;
    /** user interface screen for displaying progress gauge. */
    Form form;

    /**
     * Create the progress form and gauge.
     * This program is not interactive, it will exit when done.
     */
    public HttpExample() {
        gauge = new Gauge("Progress", false, 10, 0);
        form = new Form("Progress");
        form.append(gauge);           
        Display.getDisplay(this).setCurrent(form);
    }

    /**
     * Start a thread to run the examples.
     */
    public void startApp() {
        new Thread(this).start();
    }
    
    /**
     * Run the examples.
     */
    public void run() {;
	try {
            gauge.setLabel("Get using ContentConnection");
            gauge.setValue(2);
	    getViaContentConnection(url);

            gauge.setLabel("Get using StreamConnection");
            gauge.setValue(4);
            getViaStreamConnection(url);

            gauge.setLabel("Get using HttpConnection");
            gauge.setValue(6);
	    getViaHttpConnection(url);
            
            gauge.setLabel("Post using HttpConnection");
            gauge.setValue(8);
            postViaHttpConnection(url);
	} catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
        }
        gauge.setValue(10);
        notifyDestroyed();
    }

    /**
     * Pause, discontinue with the http tests
     */
    public void pauseApp() {
	
    }

    /**
     * Destroy must cleanup everything.  The thread is signaled
     * to stop and no result is produced.
     * @param unconditional true if forced shutdown.
     */
    public void destroyApp(boolean unconditional) {
    }

    /**
     * Simple read of a url using StreamConnection.
     * No HTTP specific behavior is needed or used.
     * <p>
     * Connector.open is used to open url and a StreamConnection is returned.
     * From the StreamConnection the InputStream is opened.
     * It is used to read every character until end of file (-1).
     * If an exception is thrown the connection and stream is closed.
     * @param url the URL to process.
     */
    void getViaStreamConnection(String url) throws IOException {
	StreamConnection c = null;
	InputStream s = null;
	try {
	    c = (StreamConnection)Connector.open(url);
	    s = c.openInputStream();
            buffer.setLength(0);

            int ch;
	    while ((ch = s.read()) != -1) {
		process((byte)ch);
	    }
	} finally {
	    if (s != null) 
		s.close();
	    if (c != null)
		c.close();
	}
    }
	
    /**
     * Simple read of a url using ContentConnection.
     * No HTTP specific behavior is needed or used.
     * <p>
     * Connector.open is used to open url and a ContentConnection is returned.
     * The ContentConnection may be able to provide the length.
     * If the length is available, it is used to read the data in bulk.
     * From the StreamConnection the InputStream is opened.
     * It is used to read every character until end of file (-1).
     * If an exception is thrown the connection and stream is closed.
     * @param url the URL to process.
     */
    void getViaContentConnection(String url) throws IOException {
	ContentConnection c = null;
	InputStream is = null;
	try {
	    c = (ContentConnection)Connector.open(url);
            is = c.openInputStream();
            buffer.setLength(0);

            int len = (int)c.getLength();
	    if (len > 0) {
		byte[] data = new byte[len];
		int actual = is.read(data);
		process(data);
	    } else {
		int ch;
		while ((ch = is.read()) != -1) {
		    process((byte)ch);
		}
	    }
	} finally {
	    if (is != null)
		is.close();
	    if (c != null)
		c.close();
	}
    }

    /**
     * Read the HTTP headers and the data using HttpConnection.
     * Check the response code to insure successful retrieval.
     * <p>
     * Connector.open is used to open url and a HttpConnection is returned.
     * The HTTP headers are read and processed.
     * If the length is available, it is used to read the data in bulk.
     * From the HttpConnection the InputStream is opened.
     * It is used to read every character until end of file (-1).
     * If an exception is thrown the connection and stream is closed.
     * @param url the URL to process.
     */
    void getViaHttpConnection(String url) throws IOException {
	HttpConnection c = null;
	InputStream is = null;
	OutputStream os = null;
	try {
	    int status = -1;

	    // Open the connection and check for re-directs
	    while (true) {
		c = (HttpConnection)Connector.open(url);
		setRequestHeaders(c);

		// Get the status code, causing the connection to be made
		status = c.getResponseCode();
		if (status == HttpConnection.HTTP_TEMP_REDIRECT ||
		    status == HttpConnection.HTTP_MOVED_TEMP ||
		    status == HttpConnection.HTTP_MOVED_PERM) {
		    // Get the new location and close the connection
		    url = c.getHeaderField("location");
		    c.close();

		    System.out.println("Redirecting to " + url);
		} else {
		    break;
		}
	    }
            
            // Any 500 status number (500, 501) means there was a server error 
	    if ((status == HttpConnection.HTTP_NOT_IMPLEMENTED) ||
                (status == HttpConnection.HTTP_VERSION) ||
                (status == HttpConnection.HTTP_INTERNAL_ERROR) ||
                (status == HttpConnection.HTTP_GATEWAY_TIMEOUT) ||
                (status == HttpConnection.HTTP_BAD_GATEWAY)) {
                System.err.print("WARNING: Server error status ["+status+"] ");
                System.err.println("returned for url ["+url+"]");
                if (is != null)
                    is.close();
                if (os != null)
                    os.close();
                if (c != null)
                    c.close();
                return;
            }

            // Only HTTP_OK (200) means the content is returned.
            if (status != HttpConnection.HTTP_OK) {
                throw new IOException("Response status not OK ["+status+"]");
            }

	    // Get the ContentType
	    String type = c.getType();
	    processType(type);

	    // open the InputStream 
	    is = c.openInputStream();
            buffer.setLength(0);

            // Get the length and process the data
	    int len = (int)c.getLength();
	    if (len > 0) {
		byte[] data = new byte[len];
		int actual = is.read(data);
		process(data);
	    } else {
		int ch;
		while ((ch = is.read()) != -1) {
		    process((byte)ch);
		}
	    }
	} finally {
	    if (is != null)
		is.close();
	    if (c != null)
		c.close();
	}
    }

    /**
     * Add request properties for the configuration, profiles,
     * and locale of this system.
     * @param c current HttpConnection to apply request headers
     */
    void setRequestHeaders(HttpConnection c) throws IOException {
        String conf = System.getProperty("microedition.configuration");
        String prof = System.getProperty("microedition.profiles");
	int space = prof.indexOf(' ');
	if (space != -1) {
	    prof = prof.substring(0, space -1);
	}
        String locale = System.getProperty("microedition.locale");

        String ua = "Profile/" + prof +
                " Configuration/" + conf;
        c.setRequestProperty("User-Agent", ua);
        
        if (locale != null) {
            c.setRequestProperty("Content-Language", locale);
        }
    }

    /**
     * Post a request with some headers and content to the server and
     * process the headers and content.
     * <p>
     * Connector.open is used to open url and a HttpConnection is returned.
     * The request method is set to POST and request headers set.
     * A simple command is written and flushed.
     * The HTTP headers are read and processed.
     * If the length is available, it is used to read the data in bulk.
     * From the StreamConnection the InputStream is opened.
     * It is used to read every character until end of file (-1).
     * If an exception is thrown the connection and stream is closed.
     * @param url the URL to process.
     */
    void postViaHttpConnection(String url) throws IOException {
	int status = 0;
	HttpConnection c = null;
	InputStream is = null;
	OutputStream os = null;

	try {
	    c = (HttpConnection)Connector.open(url);

	    // Set the request method and headers
	    c.setRequestMethod(HttpConnection.POST);
	    c.setRequestProperty("If-Modified-Since",
		"29 Oct 1999 19:43:31 GMT");
	    c.setRequestProperty("User-Agent",
		"Profile/MIDP-1.0 Configuration/CLDC-1.0");
	    c.setRequestProperty("Content-Language", "en-US");

	    // Getting the output stream may flush the headers
	    os = c.openOutputStream();
	    os.write("LIST games\n".getBytes());
	    os.flush();		// Optional, openInputStream will flush

	    // Get the status code, causing the connection to be made
	    status = c.getResponseCode();

            // Any 500 status number (500, 501) means there was a server error 
	    if ((status == HttpConnection.HTTP_NOT_IMPLEMENTED) ||
                (status == HttpConnection.HTTP_VERSION) ||
                (status == HttpConnection.HTTP_INTERNAL_ERROR) ||
                (status == HttpConnection.HTTP_GATEWAY_TIMEOUT) ||
                (status == HttpConnection.HTTP_BAD_GATEWAY)) {
                System.err.print("WARNING: Server error status ["+status+"] ");
                System.err.println("returned for url ["+url+"]");
                if (is != null)
                    is.close();
                if (os != null)
                    os.close();
                if (c != null)
                    c.close();
                return;
            }
                

            // Only HTTP_OK (200) means the content is returned.
            if (status != HttpConnection.HTTP_OK) {                
		throw new IOException("Response status not OK ["+status+"]");
	    }

	    // Open the InputStream and get the ContentType
	    is = c.openInputStream();
	    String type = c.getType();
	    processType(type);

	    // Get the length and process the data
	    int len = (int)c.getLength();
	    if (len > 0) {
		byte[] data = new byte[len];
		int actual = is.read(data);
		process(data);
	    } else {
		int ch;
		while ((ch = is.read()) != -1) {
		    process((byte)ch);
		}
	    }
	} finally {
	    if (is != null)
		is.close();
	    if (os != null)
		os.close();
	    if (c != null)
		c.close();
	}
    }

    /**
     * Process the type.
     * @param type that type
     */
    void processType(String type) {
    }

    /**
     * Process the data one character at a time.
     * @param b one byte of data
     */
    void process(byte b) {
	buffer.append((char)b);
    }

    /**
     * Process the data from the array.
     * @param b  an array of bytes.
     */
    void process(byte[] b) {
	for (int i = 0; i < b.length; i++) {
	    process(b[i]);
	}
    }
}
