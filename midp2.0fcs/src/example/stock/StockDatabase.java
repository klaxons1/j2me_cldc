/*
 * @(#)StockDatabase.java	1.10 02/07/25 @(#)
 *
 * Copyright (c) 1999-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.stock;

import javax.microedition.rms.*;

/**
 * <p>This class provides an implementation for the <code>Database</code>
 * class specific to stock records.</p>
 */
public class StockDatabase extends Database {

    /**
     * Default Constructor
     */
    public StockDatabase() {
        rc = new StockComparator();
    }

    /**
     * <p>Get a <code>RecordEnumeration</code> of records in the database who
     * match the <code>StockFilter</code> -- which just filters out the first
     * entry in the database (ie. the lastID record)</p>
     *
     * @return <code>RecordEnumeration</code> of all stock records (ie.
     *         excluding the lastID record)
     * @throws <code>RecordStoreNotOpenException</code> is thrown when trying
     *         to close a <code>RecordStore</code> that is not open
     */
    public synchronized RecordEnumeration enumerateRecords()
            throws RecordStoreNotOpenException {
        return database.enumerateRecords(new StockFilter(), null, false);
    }

    /**
     * <p>Filters out the lastID record</p>
     *
     * @see javax.microedition.rms.RecordFilter
     */
    private class StockFilter implements RecordFilter {

        /**
	 * Returns true if the candidate is less than 5 characters
	 */
        public boolean matches(byte[] candidate) {
            return ((candidate.length > 5) ? true : false);
        }
    }

    /**
     *  <p>Class to compare two records and see if they are equal</p>
     *
     * @see javax.microedition.rms.RecordComparator
     */
    private class StockComparator implements RecordComparator {

        /**
         * Checks to see if rec1 matches rec2
         *
         * @return  RecordComparator.PRECEEDS if the name of
         * rec2 comes before the name of rec1 alphabetically, 
         * RecordComparator.EQUIVALENT 
	 * if the records match,
         * RecordComparator.FOLLOWS if the name of
         * rec2 follows the name of rec1 alphabetically 
	 *
         * @param rec1 the data to compare against
         * @param rec2 the data to compare with
         */
        public int compare(byte[] rec1, byte[] rec2) {
            String name1 = Stock.getName(new String(rec1));
            String name2 = Stock.getName(new String(rec2));
            int result = name1.compareTo(name2);
            if (result < 0) {
                return RecordComparator.PRECEDES;
            } else if (result == 0) {
                return RecordComparator.EQUIVALENT;
            } else {
                return RecordComparator.FOLLOWS;
            }
        }
    }
}

