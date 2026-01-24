/*
 * @(#)About.java	1.15 02/07/31 @(#)
 *
 * Copyright (c) 2000-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example;

import javax.microedition.lcdui.*;

/**
 * Typical about box with a string and an image.
 * In this case the Sun copyright and logo.
 */

public class About {

    /** Copyright notice */
    private static String copyright =
	"Copyright (c) 2000-2002 Sun Microsystems, Inc. All rights reserved.\n"
      + "Use is subject to license terms.\n"
      + "Third-party software, including font technology, is copyrighted "
      + "and licensed from Sun suppliers.  Sun, Sun Microsystems, the Sun "
      + "logo, J2ME, the Java Coffee Cup logo, and  Java are trademarks "
      + "or registered trademarks of Sun Microsystems, Inc. in the U.S. "
      + "and other countries.\n"
      + "Federal Acquisitions: Commercial Software - Government Users "
      + "Subject to Standard License Terms and Conditions."
      + "\n\n"  
      + "Copyright (c) 2002 Sun Microsystems, Inc. Tous droits réservés.\n"
      + "Distribué par des licences qui en restreignent l'utilisation.\n"
      + "Le logiciel détenu par des tiers, et qui comprend la technologie "
      + "relative aux polices de caractères, est protégé par un copyright "
      + "et licencié par des fournisseurs de Sun. Sun, Sun Microsystems, "
      + "le logo Sun, J2ME, le logo Java Coffee Cup, et Java sont des "
      + "marques de fabrique ou des marques déposées de Sun Microsystems, "
      + "Inc. aux Etats-Unis et dans d'autres pays.";

    /** the previous screen to go back to */
    private Displayable previous;

    /**
     * Do not allow anyone to create this class
     */
    private About() {};

    /**
     * Put up the About box and when the user click ok return
     * to the previous screen.
     * @param display The <code>Display</code> to return to when the
     *                 about screen is dismissed.
     */
    public static void showAbout(Display display) {

	Alert alert = new Alert("About MIDP");
	alert.setTimeout(Alert.FOREVER);

	if (display.numColors() > 2) {
	    String icon = (display.isColor()) ?
		"/icons/JavaPowered-8.png" : "/icons/JavaPowered-2.png";

	    try {
	        Image image = Image.createImage(icon);
		alert.setImage(image);
	    } catch (java.io.IOException x) {
		// just don't append the image.
	    }
	}
	// Add the copyright
	alert.setString(copyright);

	display.setCurrent(alert);
    }

}
