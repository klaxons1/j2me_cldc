/*
 * @(#)HelloMIDlet.java	1.12 02/09/25 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

/**
 * An example MIDlet with simple "Hello" text and an exit command.
 */
public class HelloMIDlet extends MIDlet implements CommandListener {

    // Fields for the screens, commands, and display of this MIDlet
    private Command exitCommand;
    private TextBox textBox;
    private Display display;

    /**
     * Constructs the HelloMIDlet, its screen, and
     * its command. It associates the screen and command,
     * and caches the MIDlets display.
     */
    public HelloMIDlet() {
        // Create the abstract command
        exitCommand = new Command("Exit", Command.EXIT, 1);

        // Create the screen and give it a command and a listener
        textBox = new TextBox("Hello MIDlet", "Test string",
                              256, TextField.ANY | TextField.UNEDITABLE);
        textBox.addCommand(exitCommand);
        textBox.setCommandListener(this);

        // Set the MIDlet's display to its initial screen
        display = Display.getDisplay(this);
        display.setCurrent(textBox);
    }

    /**
      * Starts the MIDlet; this method does nothing because
      * the MIDlet does not require any shared resources.
      */
    public void startApp() {
    }

    /**
     * Pauses the MIDlet; this method does nothing because
     * there are no background activities or shared resources
     * to close.
     */
    public void pauseApp() {
    }


    /**
     * Destroys the MIDlet; this method does nothing because
     * there is nothing to cleanup that is not handled by the
     * garbage collector.
     */
    public void destroyApp(boolean unconditional) {
    }

    /*
     * Responds to the user’s selection of abstract commands.
     * This MIDlet has only an exit command. This method
     * responds to it by cleaning up and notifying the system
     * that the MIDlet has been destroyed.
     */
    public void commandAction(Command command, Displayable screen) {
        if (command == exitCommand) {
            destroyApp(false);
            notifyDestroyed();
        }
    }
}
