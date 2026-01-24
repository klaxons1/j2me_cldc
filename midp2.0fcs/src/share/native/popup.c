/*
 * @(#)popup.c	1.5 02/10/15 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <defaultLCDUI.h>
#include <nativeGUI.h>
#include <midpMalloc.h>

static popupElementStruct *menuList = NULL;

static jboolean inMenu; /* popup choice group is/is not open */
static int menuLength;  /* number of elements in the popup menu */
static int visibleCount;  /* number of visible elements */
static int visibleHeight; /* height of visible elements */
static int menuCurrent; /* index of currently selected menu item */
static int scrollPosition; /* index of top menu item on screen */
static int scrollY;  
static int itemHeight;  /* height of a single element */
static int itemSpace;   /* height of all elements combined */
static int currentSelection;  /* selection after popup dismiss */

static int xPos; /* (xPos, yPos) = upper left corner popup ChoiceGroup */ 
static int yPos;
static int menuWidth;
static int menuHeight;
static int topYOffset; /* y offset of non title/ticker top left pixel */
static int bottomYOffset; /* Y offset of top of bottom menu bar */
static int screenWidth;  /* width of usable screen area */
static int screenHeight; /* height of usable screen area */
static int blackPixel;
static int whitePixel;
static int IMG_MAX_W = 12; /* maximum image element width */
static int IMG_MAX_H = 12; /* maximum image element height */
static int MENU_PAD = 2; /* width of menu border in pixels */
static int ARROW_WIDTH = 11;

static jboolean SCROLL_ENABLED;
static jboolean SCROLL_UP;
static jboolean SCROLL_DOWN;

/*
 * labels used by the system menu, set by initSystemLabels() in
 * defaultLCDUI.c 
 */
unicode *_cancel_ = NULL;
int _cancellen_ = 0;

/* internal function prototypes */
static void drawUpArrow(int x, int y, jboolean filled);
static void drawDownArrow(int x, int y, jboolean filled);
static void freeMenuList();
static void hideMenu();
static void setVerticalScroll( int scrollPosition, int scrollProportion);

/* public methods */

void LCDUIinitPopupMenu() {

    int ascent, descent, leading;
    menuList = NULL;
    inMenu = KNI_FALSE;
    scrollPosition = scrollY = 0;
    SCROLL_ENABLED = SCROLL_UP = SCROLL_DOWN = KNI_FALSE;

    /* get the line height of a plain font */
    LCDUIgetFontInfo(FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
                     &ascent, &descent, &leading);
    itemHeight = ascent + descent + leading;
    
    /* itemHeight is used for the line spacing for the menu items.*/
    itemHeight = (itemHeight > IMG_MAX_H + MENU_PAD) ? itemHeight : 
        IMG_MAX_H + MENU_PAD;
    
    /* these are the colors used for the popupMenu background and text */
    blackPixel = LCDUIgetPixel(0, 0, KNI_FALSE);
    whitePixel = LCDUIgetPixel(0xffffff, 0xff, KNI_FALSE);
}

void
LCDUIfinalizePopupMenu() {
    freeMenuList();
}

void
LCDUIpaintPopupMenu()
{
    int curX = xPos;
    int curY = yPos;
    int i;
    short clip[4];
    int imagedrawn = 0;
    int imgW;
    int imgH;
    popupElementStruct *menu;
    
    menu = menuList;

    clip[0] = xPos;
    clip[1] = yPos;
    clip[2] = menuWidth + (2 * MENU_PAD);
    clip[3] = menuHeight + (2 * MENU_PAD);
    
    LCDUIfillRect(whitePixel, clip, NULL, 0, 
		  xPos, yPos, menuWidth + (2 * MENU_PAD), 
		  menuHeight + (2 * MENU_PAD)); 
    
    /* draw top border of popup */
    LCDUIdrawLine(blackPixel, clip, NULL, 0, xPos, yPos,
		  xPos + menuWidth + (2 * MENU_PAD), yPos);
    /* draw bottom border of popup */
    LCDUIdrawLine(blackPixel, clip, NULL, 0, xPos, yPos + menuHeight + 
		  (2 * MENU_PAD) - 1, xPos + menuWidth + (2 * MENU_PAD),
		  yPos + menuHeight + (2 * MENU_PAD) - 1);
    /* draw left side border of popup */
    LCDUIdrawLine(blackPixel, clip, NULL, 0, xPos, yPos,
		  xPos, yPos + menuHeight + (2 * MENU_PAD));
    /* draw the right side border of popup */
    LCDUIdrawLine(blackPixel, clip, NULL, 0, xPos + menuWidth + 
		  (2 * MENU_PAD) - 1,
		  yPos, xPos + menuWidth + (2 * MENU_PAD) - 1,
		  yPos + menuHeight + (2 * MENU_PAD));

    curY += MENU_PAD; /* top border width + 1 extra pixel */
    curX += MENU_PAD; /* left side border width + 1 extra pixel */

    clip[0] = curX;
    clip[1] = curY;
    clip[2] = menuWidth;
    clip[3] = menuHeight;

    /* sets necessary scroll flags for paint routine */
    if (itemSpace > visibleHeight) {
        setVerticalScroll(scrollY * 100 / (itemSpace - visibleHeight),
			  (visibleHeight * 100) / itemSpace);
    } 
    /* if scroll enabled, draw top space w/wo arrow
     * adjust menuHeight to leave space for bottom scroll arrow
     */
    if (SCROLL_ENABLED) {
        curX += (menuWidth - 10);
	drawUpArrow(curX, curY, SCROLL_UP);
        curY += itemHeight / 2;
        curX -= (menuWidth - 10);
    }
    /* draw visible elements */
    for (i = scrollPosition; curY - yPos < visibleHeight && i < menuLength;  
	 ++i, curY += itemHeight) {
        if(menu[i].useImage == 1) {
	    imgW = menu[i].imageWidth;
	    imgH = menu[i].imageHeight;
	    LCDUIdrawRegionTransform(clip, NULL, menu[i].image, 
				     curX, curY, 0, 0, 0,
				     (imgW <= IMG_MAX_W) ? imgW : IMG_MAX_W,
				     (imgH <= IMG_MAX_H) ? imgH : IMG_MAX_H, 
				     0, KNI_FALSE);
	    curX += IMG_MAX_W;
	    imagedrawn = 1;
	}
	if (i == menuCurrent) {
	    LCDUIfillRect(blackPixel, clip, NULL, 0,
			  curX, curY, menuWidth, itemHeight);
	    curX += 2;
	    
	    LCDUIdrawChars(whitePixel, clip, NULL, 0,
			   FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
			   curX, curY, LEFT | TOP,
			   menu[i].chars, menu[i].numChars);
	    curX -= 2;
	} else {
	    curX += 2;
	    LCDUIdrawChars(blackPixel, clip, NULL, 0,
			   FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
			   curX, curY, LEFT | TOP,
			   menu[i].chars, menu[i].numChars);
	    curX -= 2;
	}
	if (imagedrawn == 1) {
	    imagedrawn = 0;
	    curX -= IMG_MAX_W;
	}
    }
    if (SCROLL_ENABLED) {
        curX += (menuWidth - 10);
	drawDownArrow(curX, curY, SCROLL_DOWN);
        curY += itemHeight / 2;
        curX -= (menuWidth - 10);
    }
    /* upper left, lower right */
    LCDUIrefresh(xPos, yPos, xPos + menuWidth + (2 * MENU_PAD),
		 yPos + menuHeight + (2 * MENU_PAD));
}

void
LCDUIdismissPopupMenu() {
    hideMenu();
}

void
LCDUIupdatePopupElements(popupElementStruct *menu, int length, 
			 int selectedIdx, int xTarget, int yTarget,
			 int vWidth, int vHeight, int maxItemWidth,
			 jboolean tickerFlag, jboolean titleFlag)
{
    int maxW;
    if (menuList != menu) {
        freeMenuList();
    }    
    menuList   = menu;
    menuLength = length;
    menuCurrent = selectedIdx;
    xPos = xTarget;
    yPos = yTarget;
    screenWidth = vWidth;
    screenHeight = vHeight;

    /*
     * Set the width of the popup menu...
     * HI spec: max width = 4/5 of screen width 
     */
    maxW = (screenWidth * 4) / 5;
    menuWidth = maxItemWidth + (2 * MENU_PAD) + ARROW_WIDTH; 
    menuWidth = (menuWidth <= maxW) ? menuWidth : maxW;

    setEmulatorScreenMode(KNI_FALSE);
    
    /* 
     * Space is needed for menuLength * itemHeight elements  
     */
    itemSpace = menuLength * itemHeight;
    menuHeight = screenHeight - (3 * MENU_PAD);
    if (itemSpace < menuHeight) {
        menuHeight = itemSpace;
        visibleHeight = itemSpace;
        setVerticalScroll(0, 100);
    } else {
        /* make menuHeight a multiple of itemHeight... */
        visibleCount = (menuHeight / itemHeight) - 1;
	menuHeight = (visibleCount + 1)  * itemHeight;
	visibleHeight = menuHeight - itemHeight;

	/* make sure menuCurrent is on screen initially... */
	scrollPosition = menuCurrent + 1 - visibleCount;

	if (scrollPosition > 0) {
	    scrollY = scrollPosition * itemHeight;
	} else {
	    scrollY = 0;
	    scrollPosition = 0;
	}
    }
    inMenu = KNI_TRUE;

    /* 
     * "17" & "32" below are from HI spec. definitnion of 
     * ticker and title height...
     * title (only) height: 17 pixels
     * ticker (only) height: 17 pixels
     * title & ticker height: 32 pixels
     */
    if ((tickerFlag && !titleFlag) || (!tickerFlag && titleFlag)) {
        topYOffset = 17;
    } else if (tickerFlag && titleFlag){
        topYOffset = 32;
    } else {
        topYOffset = 0;
    }
    bottomYOffset = topYOffset + screenHeight;

    /*
     * Set Y Position of popup menu
     */
    if (yPos + (2 * MENU_PAD) + menuHeight >= bottomYOffset) {
	yPos -= ((yPos + menuHeight + (3 * MENU_PAD)) - bottomYOffset);
    }

    /*
     * Set X Position of popup menu:
     * The menu is as wide as needed up to a max width.
     * This positions it as close to xPos as possible.
     */
    if (xPos + (2 * MENU_PAD) + menuWidth >= screenWidth) {
	xPos -= (((xPos + menuWidth + (3 * MENU_PAD))) - screenWidth);
    }

    /*
     *   |______________________|  HI spec says menu bar should
     *   |[cancel]       [empty]|  look like this when a popup
     *   +----------------------+  choice group is open
     */
    LCDUIsetSoftButton(0, _cancel_, _cancellen_);
    LCDUIsetSoftButton(1, NULL, 0);

    LCDUIpaintPopupMenu(); 
}


jboolean
LCDUIpopupMenuInternalEvent(int button, KVMEventType *evt)
{
    if (!inMenu) {
	return KNI_FALSE;
    }

    switch (button) {
    case KEY_SOFT1:
    case KEY_SOFT2:
    case KEY_RIGHT:
    case KEY_LEFT:
	evt->type = 0;
	evt->chr = KEY_SELECT;
	hideMenu();
	return KNI_TRUE;

    case KEY_UP:
        evt->type = invalidKVMEvent;
	if (menuCurrent > 0 || scrollY > 0) {
	    
	    if (menuCurrent > 0) {
		--menuCurrent;
		
		if (menuCurrent >= scrollPosition) {
		    LCDUIpaintPopupMenu();
		} else {
		    --scrollPosition;
		    scrollY -= itemHeight;
		    LCDUIpaintPopupMenu();
		}
	    } else {
		scrollY = 0;
		LCDUIpaintPopupMenu();
	    }
        }
        return KNI_TRUE;
	
    case KEY_DOWN:
        evt->type = invalidKVMEvent;
	if (menuCurrent < menuLength - 1 ||
	    (scrollY + yPos) < itemSpace ) {
	    if (menuCurrent < menuLength - 1) {
		int bottomMenuCurrent;
		++menuCurrent;
		bottomMenuCurrent = (menuCurrent - scrollPosition + 1) *
		    itemHeight;
		
		if (bottomMenuCurrent <= visibleHeight ) {
		  LCDUIpaintPopupMenu();
		} else {		    
		    ++ scrollPosition;
		    scrollY += itemHeight;
		    LCDUIpaintPopupMenu();
		}
	    } else {
	      LCDUIpaintPopupMenu();
	    }
        }
        return KNI_TRUE;

    case KEY_SELECT:
	currentSelection = menuList[menuCurrent].idx;
        hideMenu();
        return KNI_TRUE;
    }
    return KNI_FALSE;
}

int 
LCDUIgetCurrentPopupSelection() 
{
    int rv = -1;
    if (currentSelection >= 0) {
        rv = currentSelection;
    }
    currentSelection = -1;
    return rv;
}

/* Static methods */
static void
freeMenuList() {
    int i;
    
    if (menuList != NULL) {
        /* Free all of the menu strings */
        for (i = 0; i < menuLength; ++i) {
            midpFree(menuList[i].chars);
        }
        midpFree(menuList);
        menuList = NULL;
    }
}

static void
hideMenu()
{
    inMenu = KNI_FALSE;
}

/*
 * (x, y) = top left of a 8 x 8 area to draw arrow in
 * if filled == KNI_TRUE, draw arrow.  otherwise erase the area
 */
static void 
drawDownArrow(int x, int y, jboolean filled) {
    short clip[4];
    
    clip[0] = x;
    clip[1] = y;
    clip[2] = 8;
    clip[3] = 8;

    LCDUIfillRect(whitePixel, clip, NULL, 0, 
		  x, y, 8, 8);
    if (filled == KNI_TRUE) {
        LCDUIfillRect(blackPixel, clip, NULL, 0, 
		      x + 2, y + 1, 3, 2);
	LCDUIfillTriangle(blackPixel, clip, NULL, 0,
			  x, y + 3, x + 3, y + 7, x + 7, y + 3);
    }
}

/*
 * (x, y) = top left of a 8 x 8 area to draw arrow in
 * if filled == KNI_TRUE, draw arrow.  otherwise erase the area
 */
static void 
drawUpArrow(int x, int y, jboolean filled) {
    short clip[4];
    
    clip[0] = x;
    clip[1] = y;
    clip[2] = 8;
    clip[3] = 8;
    LCDUIfillRect(whitePixel, clip, NULL, 0, 
		  x, y, 8, 8);
    if (filled == KNI_TRUE) {
        LCDUIfillTriangle(blackPixel, clip, NULL, 0,
			  x - 1, y + 4, x + 3, y - 1, x + 7, y + 4);
	LCDUIfillRect(blackPixel, clip, NULL, 0, 
		      x + 2, y + 4, 3, 2);
    }
}

static void
setVerticalScroll( int scrollPosition, int scrollProportion) {
    SCROLL_ENABLED = KNI_TRUE;
    /*
     * If the scrollable is 100% the proportion of the viewport, 
     * there's no need for any scrolling, clear the arrows and   
     * return.
     */                                                   
    if (scrollProportion >= 100 || scrollProportion == 0) {
        SCROLL_ENABLED = KNI_FALSE;
        return;
    }
    /* show both scroll arrows */
    if (scrollPosition > 0 && scrollPosition < 100) {
        SCROLL_UP = KNI_TRUE;
	SCROLL_DOWN = KNI_TRUE;
	/* show the down arrow, clear the up arrow */
    } else if (scrollPosition == 0) {
        SCROLL_UP = KNI_FALSE;
        SCROLL_DOWN = KNI_TRUE;
	/* show the up arrow, clear the down arrow */
    } else {
        SCROLL_UP = KNI_TRUE;
        SCROLL_DOWN = KNI_FALSE;
    }
}
