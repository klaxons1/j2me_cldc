/*
 * @(#)Options.java	1.9 02/07/25 @(#)
 *
 * Copyright (c) 2000-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.tilepuzzle;
import javax.microedition.lcdui.*;

public class Options extends Form implements CommandListener {

	boolean reversed;
	boolean funny;
	boolean hard;

	Command ok;
	Command cancel;

	Display dpy;
	Displayable prev;

	ChoiceGroup cg1;
	ChoiceGroup cg2;

	boolean[] scratch;

	Options(Display dpy_, Displayable prev_) {
		super("Options");

		dpy = dpy_;
		prev = prev_;

		scratch = new boolean[2];

		// set up default values

		reversed = false;
		funny = false;
		hard = true;

		cg1 = new ChoiceGroup(null, Choice.MULTIPLE);
		cg1.append("reverse arrows", null);
		cg1.append("funny shuffle", null);
		append(cg1);

		// REMIND should use a label here
		append("level:");

		cg2 = new ChoiceGroup(null, Choice.EXCLUSIVE);
		cg2.append("easy", null);
		cg2.append("hard", null);
		append(cg2);

		loadUI();

		ok = new Command("OK", Command.OK, 0);
		cancel = new Command("Cancel", Command.CANCEL, 1);

		addCommand(ok);
		addCommand(cancel);
		setCommandListener(this);
	}

	public void commandAction(Command c, Displayable d) {
		if (c == ok) {
			readUI();
		} else if (c == cancel) {
			loadUI();
		}
		dpy.setCurrent(prev);
	}

	void loadUI() {
		cg1.setSelectedIndex(0, reversed);
		cg1.setSelectedIndex(1, funny);

		cg2.setSelectedIndex((hard ? 1 : 0), true);
	}

	void readUI() {
		reversed = cg1.isSelected(0);
		funny = cg1.isSelected(1);

		hard = cg2.isSelected(1);
	}
}
