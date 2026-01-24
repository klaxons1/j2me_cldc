/*
 * @(#)menus.c	1.61 02/09/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <defaultLCDUI.h>
#include <malloc.h>
#include <nativeGUI.h>
#include <midpMalloc.h>

#define BUTTON_NOT_SET        -3
#define BUTTON_DISMISSES_MENU -2
#define BUTTON_INVOKES_MENU   -1

static int buttonCommands[2];
static commandStruct *menuList = NULL;

/*
 * Number of commands that 
 * are displayed outside the menu screen.
 * In fullscreen mode, this is 0 as all commands
 * must be displayed on the menu only.
 */
static int inFullScreenMode = KNI_FALSE;
static int offMenuCommands = 0;   /* 0 if no negative commands map to the
                                     left soft button, 1 otherwise */
static int menuLength;  /* number of commands in the menu */
static int inMenu = KNI_FALSE;
static int menuCurrent; /* index of currently selected menu item */
static int scrollPosition; /* index of top menu item on screen ? */
static int scrollY;
static int itemHeight;
static int itemSpace;
static int menuHeight;
static int boldHeight;
static int plainHeight;

/* (menuXOffset, menuYOffset) = the upper left corner of system popup menu */ 
static int menuXOffset;   
static int menuYOffset;
static int screenWidth, screenHeight;
static int blackPixel, whitePixel;
static int lightGrayPixel, darkGrayPixel;

/*
 * labels used by the system menu, set by initSystemLabels() in
 * defaultLCDUI.c 
 */
unicode *_digits_ = NULL;
int _digitslen_ = 0;
unicode *_menu_ = NULL;
int _menulen_ = 0;
unicode *_back_ = NULL;
int _backlen_ = 0;

/* re-initialize all static variables */
void
LCDUIinitMenus()
{
    int ascent, descent, leading;
    menuList = NULL;
    inMenu = KNI_FALSE;
    scrollPosition = scrollY = menuHeight = 0;
    
    /* get the line height of a plain font */
    LCDUIgetFontInfo(FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
                     &ascent, &descent, &leading);
    plainHeight = ascent + descent + leading;
    
    /* get the line height of a bold font */
    LCDUIgetFontInfo(FACE_SYSTEM, STYLE_BOLD, SIZE_MEDIUM,
                     &ascent, &descent, &leading);
    boldHeight = ascent + descent + leading;
    
    /*
     * itemHeight is used for the line spacing for the menu items.
     * However, each line could have a number, which is drawn in bold 
     * font, and the menu text is drawn in plain font. So, the line height 
     * has to be the tallest font on the line.
     */
    itemHeight = (plainHeight > boldHeight) ? plainHeight : boldHeight;
    
    /* these are the colors used in the topbar image, so we try to 
     * match them. */
    blackPixel = LCDUIgetPixel(0, 0, KNI_FALSE);
    whitePixel = LCDUIgetPixel(0xffffff, 0xff, KNI_FALSE);
    lightGrayPixel = LCDUIgetPixel(0xafafaf, 0xaf, KNI_FALSE);
    darkGrayPixel = LCDUIgetPixel(0x606060, 0x60, KNI_FALSE);

    /* TBD by HI */
    menuXOffset = 30;
    menuYOffset = 61;
    
    setCommandsFullScreenMode(KNI_FALSE);
}

static void
freeMenuList() {
    int i;
    
    if (menuList != NULL) {
        /* Free all of the menu strings */
        for (i = 0; i < menuLength; ++i) {
            midpFree(menuList[i].chars);
	    if (menuList[i].useLongChars == 1) {
                midpFree(menuList[i].longChars);
	    }
        }
        midpFree(menuList);
        menuList = NULL;
    }
}

void
LCDUIfinalizeMenus() {
    freeMenuList();
    if (_digits_ != NULL) {
        midpFree(_digits_);
    }
    if (_menu_ != NULL) {
        midpFree(_menu_);
    }
    if (_back_ != NULL) {
        midpFree(_back_);
    }
    if (_cancel_ != NULL) {
        midpFree(_cancel_);
    }
}

/*
 * This function repaints only the screen area that is changed by key 
 * up/down. curr is the index of the newly hiligthed menu item, and 
 * prev is the index of the previusly hiligthed menu item.
 */
void
LCDUIpaintMenuPartial(int first, int second)
{
    commandStruct *menu = menuList + offMenuCommands;
    /*int curX = 0, curY1 = 0, curY2 = 0;*/
    int curX = menuXOffset, curY1 = menuYOffset, curY2 = menuYOffset;
    short clip[4];
    int bg, fg;
    
    int availableWidth = 0;
    int longCharsWidth = 0;
    
    clip[0] = menuXOffset;
    clip[1] = menuYOffset;
    clip[2] = screenWidth - menuXOffset;
    clip[3] = screenHeight - menuYOffset;
    
    /* paint first */
    curX += (3 + 8 + LCDUIcharsWidth(FACE_SYSTEM, STYLE_BOLD, SIZE_MEDIUM,
				  _digits_, 1));

    curY1 += (boldHeight + 6 + (first - scrollPosition)*itemHeight);
    
    if (first == menuCurrent) {
	bg = blackPixel;
	fg = whitePixel;
    } else {
	bg = lightGrayPixel;
	fg = blackPixel;
    }
    
    LCDUIfillRect(bg, clip, NULL, 0,
		  curX, curY1, screenWidth - curX, itemHeight);
    curX += 2;
    availableWidth = screenWidth - curX;
    
    /* decide between short and long label */
    if (menu[first].useLongChars == 1) {
        longCharsWidth = LCDUIcharsWidth(FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
					 menu[first].longChars, 
					 menu[first].numLongChars);
    }
    
    if ((menu[first].useLongChars == 1) && 
	(longCharsWidth <= availableWidth)) {
	LCDUIdrawChars(fg, clip, NULL, 0,
		       FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
		       curX, curY1, LEFT | TOP,
		       menu[first].longChars, menu[first].numLongChars);
    } else {
        LCDUIdrawChars(fg, clip, NULL, 0,
		       FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
		       curX, curY1, LEFT | TOP,
		       menu[first].chars, menu[first].numChars);
    }
    curX -= 2;

    /* paint second */
    curY2 = curY1 + itemHeight;
    
    if (second == menuCurrent) {
	bg = blackPixel;
	fg = whitePixel;
    } else {
	bg = lightGrayPixel;
	fg = blackPixel;
    }
    
    LCDUIfillRect(bg, clip, NULL, 0,
		  curX, curY2, screenWidth - curX, itemHeight);
    curX += 2;
    /* decide between short and long label */
    if (menu[second].useLongChars == 1) {
        longCharsWidth = LCDUIcharsWidth(FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
					 menu[second].longChars, 
					 menu[second].numLongChars);
    }

    if ((menu[second].useLongChars == 1) && 
	(longCharsWidth <= availableWidth)) {
        LCDUIdrawChars(fg, clip, NULL, 0,
		       FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
		       curX, curY2, LEFT | TOP,
		       menu[second].longChars, menu[second].numLongChars);
    } else {
        LCDUIdrawChars(fg, clip, NULL, 0,
		       FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
		       curX, curY2, LEFT | TOP,
		       menu[second].chars, menu[second].numChars);
    }

    curX -= 2;
    /* 
     * LCDUIrefresh expects the upper left and lower right corner of the 
     * rectangle to refresh.
     */
    LCDUIrefresh(curX, curY1, screenWidth, curY2 + itemHeight);
}


void
LCDUIpaintMenu()
{
    int curX = menuXOffset;
    int curY = menuYOffset;
    int i;
    short clip[4];
    
    int availableWidth = 0;
    int longCharsWidth = 0;
    
    commandStruct *menu;
    int length;
    
    /* we might lop off the first item, because it's mapped to a button */
    
    menu = menuList + offMenuCommands;
    length = menuLength - offMenuCommands;
    
    clip[0] = menuXOffset;
    clip[1] = menuYOffset;
    clip[2] = screenWidth;
    clip[3] = screenHeight;
    
    LCDUIfillRect(lightGrayPixel, clip, NULL, 0, 
		  menuXOffset, menuYOffset, screenWidth - menuXOffset, 
		  screenHeight - menuYOffset);

    /* draw top border of menu */
    LCDUIdrawLine(darkGrayPixel, clip, NULL, 0, menuXOffset, menuYOffset, 
		  screenWidth, menuYOffset);
    LCDUIdrawLine(whitePixel, clip, NULL, 0, menuXOffset, menuYOffset + 1, 
		  screenWidth, menuYOffset + 1);
    
    /* draw left side border of menu */
    LCDUIdrawLine(darkGrayPixel, clip, NULL, 0, menuXOffset, menuYOffset + 1,
		  menuXOffset, screenHeight);
    LCDUIdrawLine(whitePixel, clip, NULL, 0, menuXOffset + 1, menuYOffset + 1,
		  menuXOffset + 1, screenHeight);
    curY += 3; /* top border width + 1 extra pixel */
    curX += 3; /* left side border width + 1 extra pixel */

    curX += (8 + LCDUIcharsWidth(FACE_SYSTEM, STYLE_BOLD, SIZE_MEDIUM,
				  _digits_, 1));
    /* draw the menu header, bold & centered */
    LCDUIdrawChars(blackPixel, clip, NULL, 0,
                   FACE_SYSTEM, STYLE_BOLD, SIZE_MEDIUM,
                   menuXOffset + ((screenWidth - menuXOffset) / 2), 
		   menuYOffset + 2, HCENTER | TOP, _menu_, _menulen_);
    
    curY += boldHeight;
    
    /* draw the title separator lines */
    LCDUIdrawLine(darkGrayPixel, clip, NULL, 0, menuXOffset + 2 , curY - 1, 
		  screenWidth, curY - 1);
    LCDUIdrawLine(whitePixel, clip, NULL, 0, menuXOffset + 2, curY, 
		  screenWidth, curY);
    curY += 3; /* for title separator line and 1 extra pixel */
    
    /* draw visible commands */
    for (i = scrollPosition; (curY - menuYOffset - boldHeight - 6) < 
	     itemSpace && i < length; ++i, curY += itemHeight) {
        if (length < 10) {
            LCDUIdrawChars(blackPixel, clip, NULL, 0,
                           FACE_SYSTEM, STYLE_BOLD, SIZE_MEDIUM,
                           menuXOffset + 6, curY, LEFT | TOP, _digits_ + i, 1);
        }
	
	/* decide between short and long label */
	/* First see if the long label fits, if yes use long label */
	/* else use short label */
	/* labels are clipped if there isn't sufficient space */
	
	if (menu[i].useLongChars == 1) {
	    availableWidth = screenWidth - curX;
	    
	    longCharsWidth = LCDUIcharsWidth(FACE_SYSTEM, STYLE_PLAIN, 
					     SIZE_MEDIUM, menu[i].longChars, 
					     menu[i].numLongChars);
	}

        if (i == menuCurrent) {
            LCDUIfillRect(blackPixel, clip, NULL, 0,
                          curX, curY, screenWidth, itemHeight);
	    curX += 2;
	    if ((menu[i].useLongChars == 1) && (longCharsWidth <= 
						availableWidth)) {
	        LCDUIdrawChars(whitePixel, clip, NULL, 0,
			       FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
			       curX, curY, LEFT | TOP,
			       menu[i].longChars, menu[i].numLongChars);
	    } else {
	        LCDUIdrawChars(whitePixel, clip, NULL, 0,
			       FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
			       curX, curY, LEFT | TOP,
			       menu[i].chars, menu[i].numChars);
	    }
	    curX -= 2;
        } else {
	    curX += 2;
	    if ((menu[i].useLongChars == 1) && (longCharsWidth <= 
						availableWidth)) {
                LCDUIdrawChars(blackPixel, clip, NULL, 0,
			       FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
			       curX, curY, LEFT | TOP,
			       menu[i].longChars, menu[i].numLongChars);
	    } else {
                LCDUIdrawChars(blackPixel, clip, NULL, 0,
			       FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
			       curX, curY, LEFT | TOP,
			       menu[i].chars, menu[i].numChars);
	    }
	    curX -= 2;
        }
	
    }
    
    if (menuHeight > itemSpace) { 
	LCDUIsetVerticalScroll(scrollY * 100 / (menuHeight - itemSpace),
			       (itemSpace * 100) / menuHeight);
    }
    
    LCDUIrefresh(menuXOffset, menuYOffset, screenWidth, screenHeight);
}

static void
hideMenu()
{
    if(inMenu) {
	/* restore the arrows */
	LCDUIsetVerticalScroll(0, 100);
	
	restoreEmulatorScreenMode();
	
	/* restore the soft buttons */
	LCDUIupdateCommandSet(menuList, menuLength);
    }
    inMenu = KNI_FALSE;
}

void
LCDUIdismissMenu() {
    hideMenu();
}

static void
initMenu()
{
    LCDUIupdateCommandSet(menuList, menuLength);
    setEmulatorScreenMode(KNI_FALSE);

    /* 
     * There are (menuLength - offMenuCommands) * itemHeight 
     * commands.  
     */
    menuHeight = (menuLength - offMenuCommands) * itemHeight; 
    /* was itemSpace = screenHeight - itemHeight - 3; */
    /* 19 = bottom bar height */
    /* -6 = spacer pixels in menu heading */
    itemSpace = screenHeight - menuYOffset - boldHeight - 6 - 19 - 11;  

    if (menuHeight < itemSpace) {
        menuHeight = itemSpace;
    }
    scrollPosition = 0;
    menuCurrent = 0;
    scrollY = 0;

    /* HI guide says:  back button only appears on the left soft button,
     * but that both buttons should dismiss the menu */
    buttonCommands[0] = BUTTON_DISMISSES_MENU;
    buttonCommands[1] = BUTTON_DISMISSES_MENU;
    
    LCDUIsetSoftButton(0, _back_, _backlen_);
    LCDUIsetSoftButton(1, NULL, 0);
    
    if (menuHeight <= itemSpace) { 
      LCDUIsetVerticalScroll(0, 100);
    }
    inMenu = KNI_TRUE;
}

static jboolean
handleSoftKey(int button, KVMEventType *evt)
{
    int index = (button == KEY_SOFT1) ? 0 : 1;

    if (buttonCommands[index] >= 0) {
        evt->chr = buttonCommands[index];
        evt->type = commandKVMEvent;
        return KNI_TRUE;
    } else if (buttonCommands[index] == BUTTON_INVOKES_MENU) {
        if (inMenu) {
	    return KNI_FALSE;
	}	
        initMenu();
        evt->chr  = BUTTON_INVOKES_MENU;
        evt->type = commandKVMEvent;
        return KNI_TRUE;
    } else if (buttonCommands[index] == BUTTON_DISMISSES_MENU) {
        if (inMenu) {
            evt->type = commandKVMEvent;
            evt->chr = BUTTON_DISMISSES_MENU;
            hideMenu();
            return KNI_TRUE;
        }
    }
    return KNI_FALSE;
}

jboolean
LCDUImenuInternalEvent(int button, KVMEventType *evt)
{
    switch (button) {
    case KEY_SOFT1:
    case KEY_SOFT2:
        return handleSoftKey(button, evt);

    case KEY_UP:
        if (!inMenu) {
	    return KNI_FALSE;
	}
        evt->type = invalidKVMEvent;

	if (menuCurrent > 0 || scrollY > 0) {
	    
	    if (menuCurrent > 0) {
		--menuCurrent;

		if (menuCurrent >= scrollPosition) {
		    LCDUIpaintMenuPartial(menuCurrent, menuCurrent + 1);
		} else {
		    --scrollPosition;
		    scrollY -= itemHeight;
		    LCDUIpaintMenu();
		}
	    } else {
		scrollY = 0;
		LCDUIpaintMenu();
	    }
        }
        return KNI_TRUE;

    case KEY_DOWN:
        if (!inMenu) {
	    return KNI_FALSE;
	}
        evt->type = invalidKVMEvent;

	if (menuCurrent < menuLength - offMenuCommands - 1 ||
	    (scrollY + menuYOffset + 6 + boldHeight + itemSpace) < menuHeight) {
	  
	    if (menuCurrent < menuLength - offMenuCommands - 1) {
	        int bottomMenuCurrent;
		
		++menuCurrent;
		bottomMenuCurrent = (menuCurrent - scrollPosition + 1) *
		    itemHeight;
		
		if (bottomMenuCurrent <= itemSpace) {
		    LCDUIpaintMenuPartial(menuCurrent-1, menuCurrent);
		} else {		    
		    ++ scrollPosition;
		    scrollY += itemHeight;
		    LCDUIpaintMenu();
		}
	    } else {
		LCDUIpaintMenu();
	    }
        }
        return KNI_TRUE;

    case KEY_SELECT:
        if (!inMenu) {
	    return KNI_FALSE;
	}
        evt->type = commandKVMEvent;
        evt->chr  = menuList[menuCurrent + offMenuCommands].id;
        hideMenu();
        return KNI_TRUE;

    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
    case KEY_9:
        if (!inMenu) {
	    return KNI_FALSE;
	}
        if (menuLength - offMenuCommands < 10) {
	    /* cmd is the index of the command */
	    int cmd = (button - KEY_0 - 1);

            if (cmd + offMenuCommands <  menuLength) {
	        evt->type = commandKVMEvent;
		evt->chr  = menuList[cmd + offMenuCommands].id;
                hideMenu();
                return KNI_TRUE;
            }
        }
    }
    return KNI_FALSE;
}

void
LCDUIupdateCommandSet(commandStruct *menu, int length)
{
    int dummy;

    int availableWidth = 0;
    int longCharsWidth = 0;

    if (menuList != menu) {
        freeMenuList();
    }

    menuList   = menu;
    menuLength = length;

    LCDUIgetDisplayParams(&screenWidth, &screenHeight,
                          &dummy, &dummy, &dummy, &dummy, &dummy);
    if (length <= 0) {
        buttonCommands[0] = buttonCommands[1] = BUTTON_NOT_SET;
	LCDUIsetSoftButton(0, NULL, 0);
	LCDUIsetSoftButton(1, NULL, 0);
	return;
    } else {
        if (inFullScreenMode == KNI_TRUE) {
	    /* This case occurs when we are in full screen mode.
	       We want all commands on the menu screen.
	    */
	    buttonCommands[0] = BUTTON_INVOKES_MENU;
	    buttonCommands[1] = BUTTON_INVOKES_MENU;
	    LCDUIsetSoftButton(0, NULL, 0);
	    LCDUIsetSoftButton(1, NULL, 0);
	} else {
	    /* highest-priority negative command on LEFT soft button */
	    if (menu[0].type == COMMAND_TYPE_BACK ||
		menu[0].type == COMMAND_TYPE_CANCEL ||
		menu[0].type == COMMAND_TYPE_STOP ||
		menu[0].type == COMMAND_TYPE_EXIT) {
	        buttonCommands[0] = menu[0].id;
		offMenuCommands = 1;
	    } else {
	        buttonCommands[0] = BUTTON_NOT_SET; 
		offMenuCommands = 0;
	    }
	    /* decide between short and long label */
	    /* First see if the long label fits, if yes use long label */
	    /* else use short label */
	    /* labels are clipped if there isn't sufficient space */

	    if (menu[0].useLongChars == 1) {
	        availableWidth = 
		    LCDUIcharsWidth(FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM, 
				    menu[0].longChars, 
				    MAX_SOFTBUTTON_COMMAND_LENGTH);
		longCharsWidth = 
		    LCDUIcharsWidth(FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
				    menu[0].longChars, menu[0].numLongChars);
	    }
	    if (offMenuCommands == 1) {
	        if ((menu[0].useLongChars == 1) && 
		    (longCharsWidth <= availableWidth)) {
		    LCDUIsetSoftButton(0, menu[0].longChars, 
				       menu[0].numLongChars);
		}else {
		  LCDUIsetSoftButton(0, menu[0].chars, menu[0].numChars);
		}
	    } else {
	        /* blank out the left button */
	        LCDUIsetSoftButton(0, NULL, 0);
	    }

	    if (length > 2 || (length == 2 && offMenuCommands == 0)) {
	        buttonCommands[1] = BUTTON_INVOKES_MENU;
		LCDUIsetSoftButton(1, _menu_, _menulen_);
	    } else if (length == 1 && offMenuCommands == 0 ||
		       length == 2 && offMenuCommands == 1) {
	        int idx = (offMenuCommands == 0) ? 0 : 1;
	        buttonCommands[1] = menu[idx].id;

		if (menu[idx].useLongChars == 1) {
		    availableWidth = 
		        LCDUIcharsWidth(FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
					menu[idx].longChars, 
					MAX_SOFTBUTTON_COMMAND_LENGTH);
		    longCharsWidth = 
		        LCDUIcharsWidth(FACE_SYSTEM, STYLE_PLAIN, SIZE_MEDIUM,
					menu[idx].longChars, 
					menu[idx].numLongChars);
		}
		if ((menu[idx].useLongChars == 1) && 
		    (longCharsWidth <= availableWidth)) {
		    LCDUIsetSoftButton(1, menu[idx].longChars, 
				       menu[idx].numLongChars);
		}else {
		    LCDUIsetSoftButton(1, menu[idx].chars, menu[idx].numChars);
		}
	    } else { 
	        /* length == 0 || (length == 1 & offMenuCommands == 1)*/
	        buttonCommands[1] = BUTTON_NOT_SET;
		LCDUIsetSoftButton(1, NULL, 0);
	    }
	}

        /*
         * When there is 10 or more menu items, the number prefix are not 
         * drawn. So the line height is just the height of the plain font
         * because bold font for the number is not used. 
         *
         * This is necessary because it cannot be assumed that the height
         * of the plain font is the same as the bold font. The tallest of
         * font determines the item height.
         */
        if (length - offMenuCommands > 10) {
          /* no bold font on the line */
          itemHeight = plainHeight;
        }
    }
}

int
LCDUIhandleMenuEvent(enum KVMEventTypes t, int a, int b)
{
    return inMenu;
}

void setCommandsFullScreenMode(jboolean fullscreenMode) {
    if (KNI_TRUE == fullscreenMode) {
        inFullScreenMode = KNI_TRUE;
    } else {
        inFullScreenMode = KNI_FALSE;
    }
}

