/*
 * @(#)HanoiCanvas.java	1.3 02/08/15 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.hanoi;

import java.util.Stack;
import java.util.Enumeration;

import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Graphics;

class HanoiCanvas extends Canvas {

    public static final int BACKGROUND_COLOUR    = 0x00000000;
    public static final int TEXT_COLOUR          = 0x00FFFFFF;
    public static final int TOWER_BASE_COLOUR    = 0x000066FF;
    public static final int TOWER_OUTLINE_COLOUR = 0x000066FF;

    public HanoiCanvas(Hanoi hanoi) {
        m_Margin = 2;
        m_N      = 12;
        m_Src    = 0;
        m_Aux    = 1;
        m_Dst    = 2;

	// Create towers
        m_Tower    = new Stack[3];
        m_Tower[0] = new Stack();
        m_Tower[1] = new Stack();
        m_Tower[2] = new Stack();

	// Create move stack
        m_Moves    = new Stack();

        m_xC       = new int[3];
	resetTowers();
    }

    public void resetTowers() {
        graphicsDone = 0;

        for(; !m_Tower[m_Src].isEmpty(); m_Tower[m_Src].pop());
        for(; !m_Tower[m_Aux].isEmpty(); m_Tower[m_Aux].pop());
        for(; !m_Tower[m_Dst].isEmpty(); m_Tower[m_Dst].pop());
        for(; !m_Moves.isEmpty(); m_Moves.pop());
	
        m_XUnit = (getWidth() - 4 * m_Margin) / 3 / m_N;
        m_YUnit = Math.min((getHeight() - 4 * m_Margin) / m_N, m_XUnit * 2);

	for(int i = 0; i < 3; i++) {
	    m_xC[i] = ((i+1)*m_Margin) + (i*m_XUnit*m_N) + ((m_XUnit*m_N)/2);
	}

        m_YBase = getHeight() - m_YUnit;

        for(int j = 0; j < m_N; j++) {
            HanoiRing ring = new HanoiRing(m_N - j, j);
            int k          = (ring.getId() * m_XUnit) / 2;

            ring.setRect(m_xC[0]-k, m_YBase - (j+1) * m_YUnit, k * 2, m_YUnit);
            m_Tower[m_Src].push(ring);
        }

        m_Solving = false;
        repaint();
    }

    public void paint(Graphics g) {
        update(g);
    }

    public void update(Graphics g) {
        g.setColor(HanoiCanvas.BACKGROUND_COLOUR);
        g.fillRect(0, 0, getWidth(), getHeight());
        for(int i = 0; i < 3; i++) {
            drawTower(g, i);
	}

        if (m_Tower[m_Src].isEmpty() && m_Tower[m_Aux].isEmpty()) {
            graphicsDone = ++graphicsDone;
            if (graphicsDone == 1) {
                lapsedTime = System.currentTimeMillis() - startTime;
		if((lapsedTime < bestTime) || (bestTime == 0)) {
		    bestTime = lapsedTime;
		}
            }
        }

	int stringX = getWidth() 
	              - Font.getDefaultFont().stringWidth("ms") - m_XUnit;
	int stringY = Font.getDefaultFont().getHeight() + m_YUnit;

        g.setColor(HanoiCanvas.TEXT_COLOUR);
	g.drawString("ms", stringX, m_YUnit, Graphics.TOP|Graphics.LEFT);
	g.drawString("" + lapsedTime, stringX, 
		     m_YUnit, Graphics.TOP|Graphics.RIGHT);
	g.drawString("Last:", 
		     stringX - Font.getDefaultFont().stringWidth(" 0000"), 
		     m_YUnit, Graphics.TOP|Graphics.RIGHT);

	g.drawString("ms", stringX, 
		     stringY, Graphics.TOP|Graphics.LEFT);
	g.drawString("" + bestTime, stringX, 
		     stringY, Graphics.TOP|Graphics.RIGHT);
	g.drawString("Best:", 
		     stringX - Font.getDefaultFont().stringWidth(" 0000"), 
		     stringY, Graphics.TOP|Graphics.RIGHT);
    }

    private void drawTower(Graphics g, int tower) {

	// Draw tower
	g.setColor(HanoiCanvas.TOWER_BASE_COLOUR);
        g.fillRect(m_xC[tower] - m_Margin / 2, m_YBase - m_YUnit * (m_N+1), 
		   m_Margin, m_YBase - m_YUnit);

	// Outline tower
        g.setColor(HanoiCanvas.TOWER_OUTLINE_COLOUR);
        g.drawRect(m_xC[tower] - m_Margin / 2, m_YBase - m_YUnit * (m_N+1), 
		   m_Margin, m_YBase - m_YUnit);

        try {
	    Enumeration enum;
            for(enum = m_Tower[tower].elements(); enum.hasMoreElements();) {
                HanoiRing ring = (HanoiRing)enum.nextElement();
                ring.drawRing(g);
            }
        }
        catch (java.util.NoSuchElementException e) {
        }
    }

    public void updateInfo() {
        if(m_Solving) {
            makeMove();
	}
        repaint();
    }

    public void stop() {
        m_Solving    = false;
	graphicsDone = 1;
    }

    public void exit() {
	stop();
	m_Exiting = true;
    }

    public void solve() {
	if (graphicsDone != 0) {
	    resetTowers();
	}
        startTime = System.currentTimeMillis();
        Solution(m_N, m_Src, m_Aux, m_Dst);
        Stack stack = new Stack();
        for(; !m_Moves.isEmpty(); stack.push(m_Moves.pop()));
        m_Moves   = stack;
        m_Solving = true;

	// Kick off the thread to drive screen updates
	synchronized(this) {
	    this.notifyAll();
	}
    }

    public boolean isSolving() {
        return m_Solving;
    }

    public boolean isExiting() {
        return m_Exiting;
    }

    private void Solution(int i, int j, int k, int l) {
        if(i == 0) {
            return;
        } else {
            Solution(i - 1, j, l, k);
            m_Moves.push(new Integer(j));
            m_Moves.push(new Integer(l));
            Solution(i - 1, k, j, l);
        }
    }

    private void makeMove() {
        if(m_Moves.isEmpty()) {
            m_Solving = false;
            return;
        }

	int x = ((Integer)m_Moves.pop()).intValue();       // Tower moving from
	int y = ((Integer)m_Moves.pop()).intValue();         // Tower moving to

        if(!m_Tower[x].isEmpty()) {
            HanoiRing ring = (HanoiRing)m_Tower[x].pop();
            int i          = m_Tower[y].size();

            ring.setRect(m_xC[y] - (ring.getId() * m_XUnit) / 2, 
			 m_YBase - (i + 1) * m_YUnit, 
			 ring.getId() * m_XUnit, m_YUnit);
            m_Tower[y].push(ring);
        }
    }

    private boolean m_Solving; // = false
    private boolean m_Exiting; // = false
    private int     m_Margin;
    private int     m_N;
    private int     m_Src;
    private int     m_Aux;
    private int     m_Dst;
    private int     m_XUnit;
    private int     m_YUnit;
    private int     m_YBase;
    private int     m_xC[];
    private Stack   m_Tower[];
    private Stack   m_Moves;
    private int     graphicsDone = 0;
    private long    startTime;
    private long    lapsedTime;
    private long    bestTime;

}
