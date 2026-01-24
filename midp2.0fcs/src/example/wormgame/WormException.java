/*
 * @(#)WormException.java	1.4 02/07/25 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * WormException.java
 *
 * Created on March 30, 2001, 16:15
 * @author Todd M Kennedy
 * @version 1.0
 */

package example.wormgame;

/**
 * A WormException occurs whenever the worm dies. It's a quick way
 * to get to the main update loop to restart the game.
 */
public class WormException extends Exception {
    public WormException(String message) {
	super(message);
    }
}
