/*
 * @(#)HanoiRing.java	1.2 02/07/25 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.hanoi;

import javax.microedition.lcdui.Graphics;

class HanoiRing {

    public static final int RING_BASE_COLOUR    = 0x00FF3333;
    public static final int RING_OUTLINE_COLOUR = 0x00FFFF66;

    public HanoiRing(int id, int pos) {
        m_Id  = id;
        m_Pos = pos;
    }

    public int getId() {
        return m_Id;
    }

    public void drawRing(Graphics g) {
        g.setColor(HanoiRing.RING_BASE_COLOUR);
        g.fillRect(m_X, m_Y, m_Width, m_Height);
        g.setColor(HanoiRing.RING_OUTLINE_COLOUR);
        g.drawRect(m_X, m_Y, m_Width, m_Height);
    }

    public void setRect(int x, int y, int width, int height) {
        m_X      = x;
        m_Y      = y;
        m_Width  = width;
        m_Height = height;
    }

    private int m_Id;
    private int m_Pos;
    private int m_X;
    private int m_Y;
    private int m_Width;
    private int m_Height;
}
