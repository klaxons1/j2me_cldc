/*
 * @(#)RMSGameScores.java	1.6 02/07/25 @(#)
 *
 * Copyright (c) 1998-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/**
 * This sample code is from the javax.microedition.rms package overview 
 * in the public draft of the MIDP spec.
 * It has been modified only slightly to be run as a MIDlet rather than
 * a class with a static main() entry point.
 */

/*
 * <p>The Mobile Information Device Profile provides a mechanism for
 * MIDlets to persistently store data and later retrieve it. This
 * persistent storage mechanism is modeled after a simple record oriented
 * database and is called the Record Management System.
 * <p><b>Example:</b>
 * <p>The example uses the Record Management System to store and retrieve
 * high scores for a game. In the example, high scores are stored in
 * separate records, and sorted when necessary using a RecordEnumeration.
*/

package example;

import javax.microedition.midlet.*;
import javax.microedition.rms.*;
import java.io.DataOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.EOFException;

/**
 * A class used for storing and showing game scores.
 */
public class RMSGameScores extends MIDlet
    implements RecordFilter, RecordComparator
{
    private RMSGameScores rmsgs = null;
    /*
     * The RecordStore used for storing the game scores.
     */
    private RecordStore recordStore = null;

    /*
     * The player name to use when filtering.
     */
    public static String playerNameFilter = null;

    /*
     * Part of the RecordFilter interface.
     */
    public boolean matches(byte[] candidate)
	throws IllegalArgumentException
    {
	// If no filter set, nothing can match it.
	if (this.playerNameFilter == null) {
	    return false;
	}

	ByteArrayInputStream bais = new ByteArrayInputStream(candidate);
	DataInputStream inputStream = new DataInputStream(bais);
	String name = null;
	
	try {
	    int score = inputStream.readInt();
	    name = inputStream.readUTF();
	}
	catch (EOFException eofe) {
	    System.out.println(eofe);
	    eofe.printStackTrace();
	}
	catch (IOException eofe) {
	    System.out.println(eofe);
	    eofe.printStackTrace();
	}
	return (this.playerNameFilter.equals(name));
    }

    /*
     * Part of the RecordComparator interface.
     */
    public int compare(byte[] rec1, byte[] rec2)
    {
	// Construct DataInputStreams for extracting the scores from
	// the records.
	ByteArrayInputStream bais1 = new ByteArrayInputStream(rec1);
	DataInputStream inputStream1 = new DataInputStream(bais1);
	ByteArrayInputStream bais2 = new ByteArrayInputStream(rec2);
	DataInputStream inputStream2 = new DataInputStream(bais2);
	int score1 = 0;
	int score2 = 0;
	try {
	    // Extract the scores.
	    score1 = inputStream1.readInt();
	    score2 = inputStream2.readInt();
	}
	catch (EOFException eofe) {
	    System.out.println(eofe);
	    eofe.printStackTrace();
	}
	catch (IOException eofe) {
	    System.out.println(eofe);
	    eofe.printStackTrace();
	}

	// Sort by score
	if (score1 < score2) {
	    return RecordComparator.PRECEDES;
	} else if (score1 > score2) {
	    return RecordComparator.FOLLOWS;
	} else {
	    return RecordComparator.EQUIVALENT;
	}
    }

    /**
     * The constructor opens the underlying record store,
     * creating it if necessary.
     */
    public RMSGameScores()
    {
	//
	// Create a new record store for this example
	//
	try {
	    recordStore = RecordStore.openRecordStore("scores", true);
	}
	catch (RecordStoreException rse) {
	    System.out.println(rse);
	    rse.printStackTrace();
	}
    }

    /**
     * Add a new score to the storage.
     *
     * @param score the score to store.
     * @param playerName the name of the play achieving this score.
     */
    public void addScore(int score, String playerName)
    {
	//
	// Each score is stored in a separate record, formatted with
	// the score, followed by the player name.
	//
	ByteArrayOutputStream baos = new ByteArrayOutputStream();
	DataOutputStream outputStream = new DataOutputStream(baos);
	try {
	    // Push the score into a byte array.
	    outputStream.writeInt(score);
	    // Then push the player name.
	    outputStream.writeUTF(playerName);
	}
	catch (IOException ioe) {
	    System.out.println(ioe);
	    ioe.printStackTrace();
	}

	// Extract the byte array
	byte[] b = baos.toByteArray();
	// Add it to the record store
	try {
	    recordStore.addRecord(b, 0, b.length);
	}
	catch (RecordStoreException rse) {
	    System.out.println(rse);
	    rse.printStackTrace();
	}
    }

    /**
     * A helper method for the printScores methods.
     */
    private void printScoresHelper(RecordEnumeration re)
    {
	try {
	    while (re.hasNextElement()) {
		int id = re.nextRecordId();
		ByteArrayInputStream bais = new 
		    ByteArrayInputStream(recordStore.getRecord(id));
		DataInputStream inputStream = new DataInputStream(bais);
		try {
		    int score = inputStream.readInt();
		    String playerName = inputStream.readUTF();
		    System.out.println(playerName + " = " + score);
		}
		catch (EOFException eofe) {
		    System.out.println(eofe);
		    eofe.printStackTrace();
		}
	    }
	}
	catch (RecordStoreException rse) {
	    System.out.println(rse);
	    rse.printStackTrace();
	}
	catch (IOException ioe) {
	    System.out.println(ioe);
	    ioe.printStackTrace();
	}
    }

    /**
     * This method prints all of the scores sorted by game score.
     */
    public void printScores()
    {
	try {
	    // Enumerate the records using the comparator implemented
	    // above to sort by game score.
	    RecordEnumeration re = recordStore.enumerateRecords(null, this, 
								true);
	    printScoresHelper(re);
	}
	catch (RecordStoreException rse) {
	    System.out.println(rse);
	    rse.printStackTrace();
	}
    }

    /**
     * This method prints all of the scores for a given player,
     * sorted by game score.
     */
    public void printScores(String playerName)
    {
	try {
	    // Enumerate the records using the comparator and filter
	    // implemented above to sort by game score.
	    RecordEnumeration re = recordStore.enumerateRecords(this, this, 
								true);
	    printScoresHelper(re);
	}
	catch (RecordStoreException rse) {
	    System.out.println(rse);
	    rse.printStackTrace();
	}
    }

    public void main()
    {
	rmsgs = new RMSGameScores();
	rmsgs.addScore(100, "Alice");
	rmsgs.addScore(120, "Bill");
	rmsgs.addScore(80, "Candice");
	rmsgs.addScore(40, "Dean");
	rmsgs.addScore(200, "Ethel");
	rmsgs.addScore(110, "Farnsworth");
	rmsgs.addScore(220, "Farnsworth");
	System.out.println("All scores");
	rmsgs.printScores();
	System.out.println("Farnsworth's scores");
	RMSGameScores.playerNameFilter = "Farnsworth";
	rmsgs.printScores("Farnsworth");
    }

    /*
     * These three methods are required because we claim to extend
     * the abstract MIDlet class;
     */
    
    /**
     * startApp()
     */
    public void startApp() throws MIDletStateChangeException {
	System.out.println("startApp() - invoking main");
	// main() output is to System.out
	main();
    }
    
    /**
     * pauseApp()
     */
    public void pauseApp() {
	System.out.println("pauseApp()");
    }
    
    /**
     * destroyApp()
     *
     * This closes our open RecordStore when we are destroyed.
     * @param cond true if this is an unconditional destroy
     *             false if it is not
     *             (ignored here and treated as unconditional)
     */
    public void destroyApp(boolean cond) {
	System.out.println("destroyApp()");
	try {
	    if (recordStore != null)
		recordStore.closeRecordStore();
	} catch (Exception ignore) {
	    // ignore this
	}
    }
    
    
}
