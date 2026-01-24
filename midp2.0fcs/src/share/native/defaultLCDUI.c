/*
 * @(#)defaultLCDUI.c	1.176 02/10/22 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>

#include <stdlib.h>
#include <property.h>
#include <defaultLCDUI.h>
#include <imageDecode.h>
#include <nativeGUI.h>
#include <midpMalloc.h>
#include <storage.h>
#include <vibrate.h>
#include <images.h>

#ifdef INCLUDE_I18N
#include "localeMethod.h"
#endif

/* Forward declarations */
static void decodeImage(char *buffer, const int length,
			const jobject thisClass, const jobject clazz);

/* cached field ids for Graphics class */
static jfieldID _g_transx_cache      = NULL;
static jfieldID _g_transy_cache      = NULL;
static jfieldID _g_linestyle_cache   = NULL;
static jfieldID _g_pixel_cache       = NULL;
static jfieldID _g_clip_cache        = NULL;
static jfieldID _g_destination_cache = NULL;
static jfieldID _g_currentFont_cache = NULL;
static jfieldID _g_face_cache        = NULL;
static jfieldID _g_style_cache       = NULL;
static jfieldID _g_size_cache        = NULL;
static jfieldID _g_maxwidth_cache    = NULL;
static jfieldID _g_maxheight_cache   = NULL;

/* cached field ids for Image class */
static jfieldID _i_imgData_cache     = NULL;
static jfieldID _i_height_cache      = NULL;
static jfieldID _i_width_cache       = NULL;

/* cached field ids for Command class */
static jfieldID _c_longlabel_cache   = NULL;
static jfieldID _c_shortlabel_cache  = NULL;
static jfieldID _c_id_cache          = NULL;
static jfieldID _c_priority_cache    = NULL;
static jfieldID _c_commandType_cache = NULL;

/* a flag indicating if a vibration is playing */
static char vibrationPlaying = 0;

#define _CACHE_FIELDID(HANDLE, name, type, cache) \
    ((cache)==NULL?((cache)=KNI_GetFieldID((HANDLE), (name), (type))):(cache))

#define KNI_TRANSLATE(G, x, y, GHAND)                                    \
    (x) += KNI_GetIntField((G), _CACHE_FIELDID((GHAND), "transX", "I", _g_transx_cache)), \
    (y) += KNI_GetIntField((G), _CACHE_FIELDID((GHAND), "transY", "I", _g_transy_cache))  \

#define KNI_LINESTYLE(G, GHAND)  \
    (KNI_GetIntField((G), _CACHE_FIELDID((GHAND), "style", "I", _g_linestyle_cache)))

#define KNI_PIXEL(G, GHAND)      \
    (KNI_GetIntField((G), _CACHE_FIELDID((GHAND), "pixel", "I", _g_pixel_cache)))

#define KNI_CLIP(G, GHAND, ARRAY, CHAND) \
    (KNI_GetObjectField((G), \
    _CACHE_FIELDID((GHAND), "clip", "[S", _g_clip_cache), (CHAND)), \
    KNI_GetRawArrayRegion((CHAND), 0, 8, (jbyte*)(ARRAY)), (ARRAY))

#define KNI_IMAGE(G, GHAND, IHAND) \
    (KNI_GetObjectField((G), _CACHE_FIELDID((GHAND), "destination", \
    "Ljavax/microedition/lcdui/Image;",_g_destination_cache), (IHAND)))

#define KNI_IMAGEDST(G, GHAND, IHAND, alt) \
    (KNI_GetObjectField((G), _CACHE_FIELDID((GHAND), "destination", \
     "Ljavax/microedition/lcdui/Image;",_g_destination_cache), (IHAND)), \
     getImageData((IHAND), alt))

#define KNI_FONT(G, GHAND, F)          \
    KNI_GetObjectField((G), _CACHE_FIELDID((GHAND), "currentFont", \
    "Ljavax/microedition/lcdui/Font;", _g_currentFont_cache), (F))

#define DECLARE_KNI_FONT_PARAMS(F, FHAND)  \
    face  = KNI_GetIntField((F), \
            _CACHE_FIELDID((FHAND), "face", "I", _g_face_cache));  \
    style = KNI_GetIntField((F), \
            _CACHE_FIELDID((FHAND), "style", "I", _g_style_cache)); \
    size  = KNI_GetIntField((F), \
            _CACHE_FIELDID((FHAND), "size", "I", _g_size_cache))

/*
 * Used with adjustToViewableRegion
 */

#define NO_IMAGE       0
#define COMPLETE_IMAGE 1
#define PARTIAL_IMAGE  2

/* For Image.createIcon the default icon to return if a resource file
 * is not found.
 */

static const unsigned char defaultIcon[] = {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
            0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b,
            0x02, 0x03, 0x00, 0x00, 0x00, 0x3b, 0x00, 0xf4,
            0x8b, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54,
            0x45, 0xff, 0xff, 0xff, 0xbb, 0xbb, 0xbb, 0xc7,
            0x4b, 0xdf, 0xf8, 0x00, 0x00, 0x00, 0x1e, 0x49,
            0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0x60, 0x60,
            0x60, 0x60, 0x0c, 0x65, 0xf8, 0xcd, 0xe0, 0xc0,
            0x20, 0xc0, 0xc4, 0xc0, 0xc0, 0x80, 0x0b, 0x33,
            0x86, 0x32, 0xfc, 0x06, 0x00, 0x3f, 0x74, 0x03,
            0x01, 0x35, 0x94, 0x27, 0xe6, 0x00, 0x00, 0x00,
            0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60,
            0x82 };



static void
destroyImageData(jobject image) {
    void *imagePtr;

    KNI_StartHandles(1);
    KNI_DeclareHandle(clazz);

    KNI_GetObjectClass(image, clazz);
    imagePtr = 
      (void*)KNI_GetIntField(image, _CACHE_FIELDID(clazz, "imgData", "I", _i_imgData_cache));

    KNI_EndHandles();

    if(imagePtr != NULL) {
        LCDUIdestroyNativeImage(imagePtr);
    }
}

/*=========================================================================
 * FUNCTION:      getImageData
 * OVERVIEW:      retrieves a pointer to the raw image data
 * INTERFACE:
 *   parameters:  INSTANCE img: the Image object to get the data from
 *                void* alt: default image data if img is null
 *   returns:     a pointer into the raw data associated with the image's
 *                BYTEARRAY
 *
 * COMMENTS:      One should be very careful with this function. We are
 *                returning a pointer to the inside of an object. If the
 *                GC moves the object, this pointer will be invalid.
 *                Great care must be taken to not allow a GC to happen
 *                after calling this function.
 *=======================================================================*/
static void *
getImageData(jobject img, void *alt) {
    if (KNI_IsNullHandle(img) == KNI_TRUE) {
        return alt;
    } else {
	int    result;

	KNI_StartHandles(1);
	KNI_DeclareHandle(clazz);

	KNI_GetObjectClass(img, clazz);
        result = 
          KNI_GetIntField(img, _CACHE_FIELDID(clazz, "imgData", "I", _i_imgData_cache));
	
	KNI_EndHandles();
	return (void*)result;
    }
}

static int
compare(const commandStruct  *a, const commandStruct *b, const char table[]) {
    if (a->type == b->type) {
        return a->priority - b->priority;
    } else {
        int aPos =
            ((a->type < 1) || (a->type > 8)) ? 127 : table[a->type];

        int bPos =
            ((b->type < 1) || (b->type > 8)) ? 127 : table[b->type];

        return aPos - bPos;
    }
}

static int
compareForButton(const void *first, const void *second) {
    /*
     * According to latest HI specification, the precedence
     * and mapping for 2 buttons are
     *
     *    left button             right button
     *  -------------------+--------------------
     *       BACK                     ITEM
     *       EXIT                    SCREEN
     *      CANCEL                     OK
     *       STOP                     HELP
     *      <none>                    BACK
     *                                EXIT
     *                               CANCEL
     *                                STOP
     *
     * To get the right effect, we need to (arbitrarily) place
     * the items with one button above another.  Because the
     * system menu goes on the right button, we 
     * put left button items higher, which gives us the ordering
     *
     *    5 BACK
     *    6 EXIT
     *    7 CANCEL
     *    8 STOP
     *    127 ITEM   (invalid for the left button)
     *    127 SCREEN (          ""               )
     *    127 OK     (          ""               )
     *    127 HELP   (          ""               )
     *
     *
     * as reproduced in the (inverted) sorting-order table below.
     *
     * The left soft button is labeled with the highest priority
     * negative command (e.g. BACK, EXIT, CANCEL, or STOP).  In the case
     * that none of these commands is present in the command list the left
     * soft button label is blank and the button is incactive.
     *
     * June 25 2001
     */
    static char table[10] = {
        127,    /* invalid */
        127,    /* SCREEN */
        5,      /* BACK */
        7,      /* CANCEL */
        127,    /* OK */
        127,    /* HELP */
        8,      /* STOP */
        6,      /* EXIT */
        127,    /* ITEM */
    };

    return compare((commandStruct *)first, (commandStruct *)second, table);
}

static int
compareForMenu(const void *first, const void *second) {
    /*
     * When there are more than 2 commands, or 2 commands which
     * are not negative (BACK, EXIT, CANCEL, STOP), the Left button 
     * should be choosen according to the rules mentioned above while
     * the commands that are left should be sorted in the menu
     * according to the following order:
     *
     *    1 ITEM      
     *    2 SCREEN
     *    3 OK
     *    4 HELP
     *    5 BACK
     *    6 EXIT
     *    7 CANCEL
     *    8 STOP
     */
    static char table[10] = {
        127,    /* invalid */
        2,      /* SCREEN */
        5,      /* BACK */
        7,      /* CANCEL */
        3,      /* OK */
        4,      /* HELP */
        8,      /* STOP */
        6,      /* EXIT */
        1,      /* ITEM */
    };

    return compare((commandStruct *)first, (commandStruct *)second, table);
}

/*=========================================================================
 * FUNCTION:      getSortedCommands
 * OVERVIEW:      Sort the given commands based upon a set of HI rules.
 * INTERFACE:
 *   parameters:  ARRAY inItemCmds: the Item Commands to sort
 *                int numItemCommands: the number of Item Commands to sort
 *                ARRAY inCmds: the rest of the commands to sort
 *                int numCommands: the number of the rest of the commands 
 *                                 to sort
 *   returns:     a sorted list of commands. NULL if we are unable to 
 *                allocate enough memory for the structure.
 *=======================================================================*/
static commandStruct *
getSortedCommands(jobject inItemCmds, int numItemCommands,
		  jobject inCmds, int numCommands) {
    /* for some reason, we need to allocate an extra command in order    */
    /* to avoid a memory smash.  I've looked but haven't found the cause.*/
    int nc = numItemCommands + numCommands;
    commandStruct *c = (commandStruct*)midpCalloc(nc + 1, 
					          sizeof(commandStruct));

    jobjectArray  itemCmds = (jobjectArray)inItemCmds;
    jobjectArray  cmds = (jobjectArray)inCmds;

    int              j;
    int highestCmdIndx = -1;
    int        chrsLen;
    char       undoAllocs = 0;

    if (c == NULL) {
        return NULL;
    }

    KNI_StartHandles(4);

    KNI_DeclareHandle(i);
    KNI_DeclareHandle(str);
    KNI_DeclareHandle(longstring);
    KNI_DeclareHandle(clazz);

    KNI_FindClass("javax/microedition/lcdui/Command", clazz);

    /*
     * WARNING: We need to copy the string data (not just keep a
     * pointer to it) becuase if the garbage collector is allowed
     * to move the contents of the heap, the pointers will become
      * invalid.
     */

    for (j = 0; j < nc; ++j) {
	/* First fill c array with Item commands from itemCmds (inItemCmds)
	 * then fill it with screen commands from cmds (inCmds)
	 * Later all these commands will be sorted together
	 */
	if (j < numItemCommands) {
	    KNI_GetObjectArrayElement(itemCmds, j, i);
	} else {
	    KNI_GetObjectArrayElement(cmds, j - numItemCommands, i);
	}

	KNI_GetObjectField(i, _CACHE_FIELDID(clazz, "shortLabel", 
					     "Ljava/lang/String;", _c_shortlabel_cache), str);
	chrsLen = KNI_GetStringLength(str);

        c[j].id       = KNI_GetIntField(i, 
            _CACHE_FIELDID(clazz, "id", "I", _c_id_cache));
        c[j].priority = KNI_GetIntField(i, 
            _CACHE_FIELDID(clazz, "priority", "I", _c_priority_cache));
        c[j].type     = KNI_GetIntField(i, 
            _CACHE_FIELDID(clazz, "commandType", "I", _c_commandType_cache));
        c[j].numChars = (chrsLen > MAX_MENU_COMMAND_LENGTH) 
                      ? MAX_MENU_COMMAND_LENGTH 
                      : chrsLen;
        c[j].chars    = (unicode*) midpCalloc(c[j].numChars, sizeof(unicode));

        if (c[j].chars == NULL) {
            undoAllocs = 1;
            break;                      /* Get out of allocation for() loop */
        }

        KNI_GetStringRegion(str, 0, c[j].numChars, c[j].chars);

        /* get long label */

        KNI_GetObjectField(i, _CACHE_FIELDID(clazz, "longLabel", 
            "Ljava/lang/String;", _c_longlabel_cache), longstring);

        chrsLen = KNI_GetStringLength(longstring);

        if ((chrsLen = KNI_GetStringLength(longstring)) == -1) {
            /* long label is null */
            c[j].useLongChars = 0;
        } else {
            /* long label is not null */
            c[j].useLongChars = 1;
            c[j].numLongChars = (chrsLen > MAX_MENU_COMMAND_LENGTH) 
                              ? MAX_MENU_COMMAND_LENGTH 
                              : chrsLen;
            c[j].longChars    = (unicode*) midpCalloc(c[j].numLongChars, 
                                                      sizeof(unicode));

            if (c[j].longChars == NULL) {
                undoAllocs = 1;
                break;
            }

            KNI_GetStringRegion(longstring, 0, 
                                c[j].numLongChars, c[j].longChars);
        } 
 
        if ((nc > 1) && ((highestCmdIndx == -1) || 
            (compareForButton(c+highestCmdIndx, c+j) > 0))) {
            highestCmdIndx = j;
	    }
    } /* end for (j=0; j<nc; ++j); */

    if (undoAllocs) {
        /* Whoops! We need to undo all previous allocs */
        int i;
        for (i = 0; i < j; ++i) {
            midpFree(c[i].chars);
            if (c[i].useLongChars != 0) {
                midpFree(c[i].longChars);
            }
        }
        midpFree(c);
        c = NULL;
    }

    if ((c != NULL) && (nc > 1)) { 
        /* swap highest command index to spot zero in commands array! */
        if (highestCmdIndx != 0) {
            commandStruct tmp = c[0];
            c[0] = c[highestCmdIndx];
            c[highestCmdIndx] = tmp;
	    }

        if (numCommands > 2) {
            qsort(c+1, nc-1, sizeof(commandStruct), compareForMenu);
        }
    }

    KNI_EndHandles();
    return c;
}

void clearVibrationFlag() {
    vibrationPlaying = 0;
}


/*=========================================================================
 * FUNCTION:      getPopupElements
 * OVERVIEW:      import popup choice-group elements into native data 
 *                structures
 * INTERFACE:
 *   parameters:  ARRAY stringElements:  string component of elements  
 *                ARRAY imageElements:  image component of element 
 *                int numElements:  number of elements
 *
 *   returns:     a list of popup elements, or NULL if we are unable to 
 *                allocate enough memory for the structure.
 *=======================================================================*/
static popupElementStruct *
getPopupElements(jobject stringElements, jobject imageElements,
		 int numElements) {
    /* for some reason, we need to allocate an extra element in order    */
    /* to avoid a memory smash.  */
    int i;
    char undoAllocs = 0;
    int slen;
    
    jobjectArray imageArr = (jobjectArray)imageElements;
    
    popupElementStruct *popupList = 
	(popupElementStruct*)midpCalloc(numElements + 1, 
					sizeof(popupElementStruct));
    if (popupList == NULL) {
        return NULL;
    }
    
    KNI_StartHandles(3);
    
    KNI_DeclareHandle(str);  /* current string object */
    KNI_DeclareHandle(img);  /* current image object */
    KNI_DeclareHandle(clazz);
    
    /*
     * Note:  We need to copy the string data (not just keep a
     * pointer to it) becuase if the garbage collector is allowed
     * to move the contents of the heap, the pointers will become
     * invalid.  Also, the first numElements of 
     * <code>stringElements</code> are non-null (checked by caller)
     */
    for (i = 0; i < numElements; ++i) {
        KNI_GetObjectArrayElement(stringElements, i, str);
	slen = KNI_GetStringLength(str);
	/*FIX...insted of 40 define a max string len!*/
	popupList[i].idx = i;
	popupList[i].numChars = (slen > 40) ? 40 : slen;
	popupList[i].chars = (unicode*) midpCalloc(popupList[i].numChars,
						   sizeof(unicode));
	popupList[i].useImage = 0;
        if (popupList[i].chars == NULL) {
	    undoAllocs = 1;
	    break;           /* Get out of allocation for() loop */
        }
	
        KNI_GetStringRegion(str, 0, popupList[i].numChars, 
			    popupList[i].chars);
	/* if there's an image present for this element, we need to
	 * save it as well...*/
	if (!KNI_IsNullHandle(imageArr)) {
	    KNI_GetObjectArrayElement(imageElements, i, img);
	    
	    if (!KNI_IsNullHandle(img)) {
		popupList[i].useImage = 1;
		
		KNI_GetObjectClass(img, clazz);
		popupList[i].image = (void*) 
		    KNI_GetIntField(img, _CACHE_FIELDID(clazz, "imgData", "I", 
							_i_imgData_cache));
		popupList[i].imageHeight = 
		    KNI_GetIntField(img, _CACHE_FIELDID(clazz, "height", "I",
							_i_height_cache));
		popupList[i].imageWidth = 
		    KNI_GetIntField(img, _CACHE_FIELDID(clazz, "width", "I", 
							_i_width_cache));
		if (popupList[i].image == (void*)-1){
		    popupList[i].useImage = 0;
		} 
	    }
	}
    } /* end for (i=0; i<numElements; ++i); */

    if (undoAllocs) {
        /* Whoops! We need to undo all previous allocs */
        int j;
        for (j = 0; j < i; ++j) {
            midpFree(popupList[j].chars);
        }
        midpFree(popupList);
	popupList = NULL;
    }
    KNI_EndHandles();
    return popupList;
}



/*=========================================================================
 * FUNCTION:      nVibrate(I)I
 * CLASS:         javax.microedition.lcdui.Display
 * TYPE:          native function to play vibraton
 * OVERVIEW:      launch a native thread and push the audio data to the 
 *                devices                   
 * INTERFACE (operand stack manipulation):
 *   parameters:  keyCode   A system-specific keyCode
 *   returns:     The abstract game action associated with the keyCode
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_Display_nVibrate() {
    int dur = KNI_GetParameterAsInt(1);

    if (vibrationPlaying == 1) {
	stopVibrate();
    } 
    
    if (dur == 0) {
	vibrationPlaying = 0;
	KNI_ReturnInt (1);
    } else {
	vibrationPlaying = 1;
	KNI_ReturnInt(startVibrate(dur<<3));
    }
    
}

/*=========================================================================
 * FUNCTION:      incomingCall()V
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Native method to draw a dialog on the screen indicating
 *                there is an incoming phone call and ask the user to
 *                answer
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_lcdui_DefaultEventHandler_incomingCall() {
    LCDUIincomingCall();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      getKeyCode(I)I
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Get the system-specific key code corresponding to
 *                 the given gameAction.
 * INTERFACE (operand stack manipulation):
 *   parameters:  gameAction   A game action
 *   returns:     The keyCode associated with that action
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_lcdui_DefaultEventHandler_getKeyCode() {
    int gameAction = KNI_GetParameterAsInt(1);

    KNI_ReturnInt(LCDUIgetKeyCode(gameAction));
}

/*=========================================================================
 * FUNCTION:      getSystemKey(I)I
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Get the abstract system key that corresponds to keyCode.
 * INTERFACE (operand stack manipulation):
 *   parameters:  keyCode   A system-specific keyCode
 *   returns:     The SYSTEM_KEY_ constant for this keyCode, or 0 if none
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_lcdui_DefaultEventHandler_getSystemKey() {
    int gameAction = KNI_GetParameterAsInt(1);

    KNI_ReturnInt(LCDUIgetSystemKey(gameAction));
}

/*=========================================================================
 * FUNCTION:      getGameAction(I)I
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Get the abstract gameAction corresponding to the
 *                 given keyCode
 * INTERFACE (operand stack manipulation):
 *   parameters:  keyCode   A system-specific keyCode
 *   returns:     The abstract game action associated with the keyCode
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_lcdui_DefaultEventHandler_getGameAction() {
    int keyCode = KNI_GetParameterAsInt(1);

    KNI_ReturnInt(LCDUIgetGameAction(keyCode));
}

/*=========================================================================
 * FUNCTION:      getKeyName(I)Ljava/lang/String;
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Get the informative key string corresponding to
 *                 the given keyCode.
 * INTERFACE (operand stack manipulation):
 *   parameters:  keyCode   A system-specific keyCode
 *   returns:     A string name for the key, or null if no name is
 *                 available
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_lcdui_DefaultEventHandler_getKeyName() {
    int   keyCode = KNI_GetParameterAsInt(1);
    char *keyName = LCDUIgetKeyName(keyCode);

    KNI_StartHandles(1);
    KNI_DeclareHandle(str);

    if (keyName != NULL) {
	KNI_NewStringUTF(keyName, str);
    } else {
	KNI_ReleaseHandle(str); /* Set 'str' to null String object */
    }
    KNI_EndHandlesAndReturnObject(str);
}


/*=========================================================================
 * FUNCTION:      updatePopupElements([Ljavax/microedition/lcdui/String;
 *                                   [Ljavax/microedition/lcdui/Image;I)V
 * CLASS:         com.sun.midp.lcdui.ChoiceGroup
 * TYPE:          virtual native function
 * OVERVIEW:      Set the current set of popup choice group elements
 * INTERFACE (operand stack manipulation):
 *   parameters:  stringElems    The list of string elements 
 *                imageElems     The list of image elements
 *                numElems       The number of elements in the popup list
 *                selectedElem   The currently selected popup index
 *                xPos           The x position of the ideal upper right
 *                                 corner for the popup window
 *                yPos           The y position of the ideal upper right
 *                                 corner for the popup window
 *                vWidth         The width of the viewport
 *                vHeight        The height of the viewport
 *                maxWidth       The widest choice element in the group
 *                tickerflag     true if there is a ticker on screen
 *                titleflag      true if there is a title on screen
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ChoiceGroup_updatePopupElements() {
    int numElements = KNI_GetParameterAsInt(3);
    int selectedIdx = KNI_GetParameterAsInt(4);
    int xPos = KNI_GetParameterAsInt(5);
    int yPos = KNI_GetParameterAsInt(6);
    int vWidth = KNI_GetParameterAsInt(7);
    int vHeight = KNI_GetParameterAsInt(8);
    int maxWidth = KNI_GetParameterAsInt(9);
    jboolean tickerFlag = KNI_GetParameterAsBoolean(10);
    jboolean titleFlag = KNI_GetParameterAsBoolean(11);

    KNI_StartHandles(2);    
    KNI_DeclareHandle(stringElems);
    KNI_DeclareHandle(imageElems);

    KNI_GetParameterAsObject(1, stringElems);
    KNI_GetParameterAsObject(2, imageElems);

    if (numElements == 0) {
	LCDUIupdatePopupElements(NULL, 0, 0, 0, 0, 0, 0, 0, 
				 KNI_FALSE, KNI_FALSE);
    } else {
        popupElementStruct *elementList = getPopupElements(stringElems, 
							   imageElems,
							   numElements);
	if (elementList != NULL) {
	    LCDUIinitPopupMenu();
	    LCDUIupdatePopupElements(elementList, numElements, 
				     selectedIdx, xPos, yPos, vWidth,
				     vHeight, maxWidth, 
				     tickerFlag, titleFlag);
	} else {
	    LCDUIupdatePopupElements(NULL, 0, 0, 0, 0, 0, 0, 0,
				     KNI_FALSE, KNI_FALSE);
            KNI_ThrowNew("java/lang/OutOfMemoryError", "");	    
	}
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      getPopupSelection()I
 * CLASS:         javax.microedition.lcdui.ChoiceGroup
 * TYPE:          virtual native function
 * OVERVIEW:      Return the selected choice from a popup choice group
 * INTERFACE 
 *   parameters:  <none>
 *   returns:     index of the selected popup element, or -1 if popup
 *                choice group was canceled
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_ChoiceGroup_getPopupSelection(){
    int rv = LCDUIgetCurrentPopupSelection();
    KNI_ReturnInt(rv);
}

/*=========================================================================
 * FUNCTION:      initSystemLabels([java.lang.String])V
 * CLASS:         com.sun.midp.main.Main
 * TYPE:          virtual native function
 * OVERVIEW:      Pass localized strings to native system menu and 
 *                popup choice group for use
 * INTERFACE 
 *   parameters:  labels[] array of localized strings in this order:
 *                [1234567890, Menu, Back, Cancel]
 *   returns:     <none>
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_Main_initSystemLabels() {

    int i;
    int slen;
    unicode *ustr;
    
    KNI_StartHandles(2);    
    KNI_DeclareHandle(labels);
    KNI_DeclareHandle(label);
    
    KNI_GetParameterAsObject(1, labels);

    /* 
     * Currently there are 4 translated labels: 
     * '1234567890', 'Menu', 'Back', & 'Cancel'
     */
    for (i = 0; i < 4; i++) {
	KNI_GetObjectArrayElement(labels, i, label);
	slen = KNI_GetStringLength(label);
	slen = (slen > 12) ? 12 : slen; /* 12 = maximum label length */
	ustr = (unicode*) midpCalloc(slen, sizeof(unicode));
	if (ustr != NULL) {
	    KNI_GetStringRegion(label, 0, slen, ustr);
            switch(i) {
            case 0: /* "digits" */
                _digitslen_ = slen;
                if (_digits_ != NULL) {
                    midpFree(_digits_);
                }
                _digits_ = ustr;
                break;
            case 1: /* Menu */
                _menulen_ = slen;
                if (_menu_ != NULL) {
                    midpFree(_menu_);
                }
                _menu_ = ustr;
                break;
            case 2: /* Back */
                _backlen_ = slen;
                if (_back_ != NULL) {
                    midpFree(_back_);
                }
                _back_ = ustr;
                break;
            case 3: /* Cancel */
                _cancellen_ = slen;
                if (_cancel_ != NULL) {
                    midpFree(_cancel_);
                }
                _cancel_ = ustr;
                break;
            }
	}
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      updateCommandSet([Ljavax/microedition/lcdui/Command;I
 *                                 [Ljavax/microedition/lcdui/Command;I)V
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Set the current set of active Abstract Commands.
 * INTERFACE (operand stack manipulation):
 *   parameters:  itemCommands    The list of Item Commands that 
 *                                should be active
 *                numItemCommands The number of commands in the list 
 *                                of Item Commands
 *                commands        The list of Commands that should be active
 *                numCommands     The number of commands in the list of 
 *                                Commands
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_lcdui_DefaultEventHandler_updateCommandSet() {
    int numItemCmds = KNI_GetParameterAsInt(2);
    int numCmds     = KNI_GetParameterAsInt(4);

    KNI_StartHandles(2);

    KNI_DeclareHandle(itemCmds);
    KNI_DeclareHandle(cmds);

    KNI_GetParameterAsObject(1, itemCmds);
    KNI_GetParameterAsObject(3, cmds);

    if (numItemCmds == 0 && numCmds == 0) {
        LCDUIupdateCommandSet(NULL, 0);
    } else {
        commandStruct *menuList = getSortedCommands(itemCmds, numItemCmds,
						    cmds, numCmds);
        if (menuList != NULL ) {
            LCDUIupdateCommandSet(menuList, numItemCmds + numCmds);
        } else {
            LCDUIupdateCommandSet(NULL, 0);
            KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      menuKeyEvent(II)Z
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Handle the key event when the menu is the current
 *                 display.
 * INTERFACE (operand stack manipulation):
 *   parameters:  type   One of PRESSED, RELEASED, or REPEATED
 *                code   The key code of the key event
 *   returns:     true if the event is the current display is the
 *                 menu screen.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_lcdui_DefaultEventHandler_menuKeyEvent() {
    int code = KNI_GetParameterAsInt(2);
    int type = KNI_GetParameterAsInt(1);

    enum KVMEventTypes etype = stopKVMEvent;

    switch (type) {
    case 1: etype = keyDownKVMEvent;   break;
    case 2: etype = keyUpKVMEvent;     break;
    case 3: etype = keyRepeatKVMEvent; break;
    }

    KNI_ReturnBoolean(LCDUIhandleMenuEvent(etype, code, 0));
}

/*=========================================================================
 * FUNCTION:      menuPointerEvent(III)Z
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Handle the pointer event when the menu is the current
 *                 display.
 * INTERFACE (operand stack manipulation):
 *   parameters:  type   one of PRESSED, RELEASE, or DRAGGED
 *                x      the x co-ordinate of the pointer event
 *                y      the y co-ordinate of the pointer event
 *   returns:     true if the event is the current display is the menu
 *                 screen.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_lcdui_DefaultEventHandler_menuPointerEvent() {
    int    y = KNI_GetParameterAsInt(3);
    int    x = KNI_GetParameterAsInt(2);
    int type = KNI_GetParameterAsInt(1);

    enum KVMEventTypes etype = stopKVMEvent;

    switch (type) {
    case 1: etype = penDownKVMEvent; break;
    case 2: etype = penUpKVMEvent;   break;
    case 3: etype = penMoveKVMEvent; break;
    }

    KNI_ReturnBoolean(LCDUIhandleMenuEvent(etype, x, y));
}

/*=========================================================================
 * FUNCTION:      paintMenu()V
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Native method to draw the command menu on the screen
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_lcdui_DefaultEventHandler_paintMenu() {
    LCDUIpaintMenu();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      dismissMenu()V
 * CLASS:         com.sun.midp.lcdui.DefaultEventHandler
 * TYPE:          virtual native function
 * OVERVIEW:      Dismiss the current menu in the case of setCurrent()
 *                 being called while the Display is suspended by a
 *                 system screen.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_lcdui_DefaultEventHandler_dismissMenu() {
    LCDUIdismissMenu();
    LCDUIdismissPopupMenu();
    KNI_ReturnVoid();
}

static int checkAnchor(int anchor, int illegal_vpos) {
    int OK;

    if (anchor == 0) return KNI_TRUE; /* special case: 0 is ok */

    OK =   (anchor > 0) && (anchor < (BASELINE << 1))
        && ((anchor & illegal_vpos) == 0);

    if (OK) {
        int n = anchor & (TOP | BOTTOM | BASELINE | VCENTER);
        OK = (n != 0) && ((n & (n-1)) == 0); /* exactly one bit set */
    }

    if (OK) {
        int n = anchor & (LEFT | RIGHT | HCENTER);
        OK = (n != 0) && ((n & (n-1)) == 0); /* exactly one bit set */
    }

    return OK;
}

/*=========================================================================
 * FUNCTION:      drawLine(IIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws a line between the coordinates (x1,y1) and
 *                (x2,y2) using the current color and stroke style.
 * INTERFACE (operand stack manipulation):
 *   parameters:  x1   the x coordinate of the start of the line
 *                y1   the y coordinate of the start of the line
 *                x2   the x coordinate of the end of the line
 *                y2   the y coordinate of the end of the line
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawLine() {
    int y2 = KNI_GetParameterAsInt(4);
    int x2 = KNI_GetParameterAsInt(3);
    int y1 = KNI_GetParameterAsInt(2);
    int x1 = KNI_GetParameterAsInt(1);
    jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, gHandle);

    KNI_TRANSLATE(thisClass, x1, y1, gHandle);
    KNI_TRANSLATE(thisClass, x2, y2, gHandle);

    LCDUIdrawLine(KNI_PIXEL(thisClass, gHandle), 
		  KNI_CLIP(thisClass, gHandle, myClip, cHandle), 
		  KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		  KNI_LINESTYLE(thisClass, gHandle), 
		  x1, y1, x2, y2);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawRect(IIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws the outline of the specified rectangle using
 *                 the current color and stroke style.
 * INTERFACE (operand stack manipulation):
 *   parameters:  x       the x coordinate of the rectangle to be drawn
 *                y       the y coordinate of the rectangle to be drawn
 *                width   the width of the rectangle to be drawn
 *                height  the height of the rectangle to be drawn
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawRect() {
    int h = KNI_GetParameterAsInt(4);
    int w = KNI_GetParameterAsInt(3);
    int y = KNI_GetParameterAsInt(2);
    int x = KNI_GetParameterAsInt(1);
    jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, gHandle);

    KNI_TRANSLATE(thisClass, x, y, gHandle);

    LCDUIdrawRect(KNI_PIXEL(thisClass, gHandle),
		  KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		  KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		  KNI_LINESTYLE(thisClass, gHandle),
		  x, y, w, h);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      fillRect(IIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Fills the specified rectangle with the current color.
 * INTERFACE (operand stack manipulation):
 *   parameters:  x       the x coordinate of the rectangle to be filled
 *                y       the y coordinate of the rectangle to be filled
 *                width   the width of the rectangle to be filled
 *                height  the height of the rectangle to be filled
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_fillRect() {
    int h = KNI_GetParameterAsInt(4);
    int w = KNI_GetParameterAsInt(3);
    int y = KNI_GetParameterAsInt(2);
    int x = KNI_GetParameterAsInt(1);
    jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, gHandle);

    KNI_TRANSLATE(thisClass, x, y, gHandle);

    LCDUIfillRect(KNI_PIXEL(thisClass, gHandle),
		  KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		  KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		  KNI_LINESTYLE(thisClass, gHandle),
		  x, y, w, h);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawRoundRect(IIIIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws the outline of the specified rounded corner
 *                 rectangle using the current color and stroke style.
 * INTERFACE (operand stack manipulation):
 *   parameters:  x          the x coordinate of the rectangle to be drawn
 *                y          the y coordinate of the rectangle to be drawn
 *                width      the width of the rectangle to be drawn
 *                height     the height of the rectangle to be drawn
 *                arcWidth   the horizontal diameter of the arc at the
 *                            four corners
 *                arcHeight  the vertical diameter of the arc at the
 *                            four corners
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawRoundRect() {
    int arcHeight = KNI_GetParameterAsInt(6);
    int  arcWidth = KNI_GetParameterAsInt(5);
    int         h = KNI_GetParameterAsInt(4);
    int         w = KNI_GetParameterAsInt(3);
    int         y = KNI_GetParameterAsInt(2);
    int         x = KNI_GetParameterAsInt(1);
    jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, gHandle);

    KNI_TRANSLATE(thisClass, x, y, gHandle);

    LCDUIdrawRoundRect(KNI_PIXEL(thisClass, gHandle),
		       KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		       KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		       KNI_LINESTYLE(thisClass, gHandle),
		       x, y, w, h, arcWidth, arcHeight);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      fillRoundRect(IIIIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Fills the specified rounded corner rectangle using the
 *                 current color.
 * INTERFACE (operand stack manipulation):
 *   parameters:  x          the x coordinate of the rectangle to be filled
 *                y          the y coordinate of the rectangle to be filled
 *                width      the width of the rectangle to be filled
 *                height     the height of the rectangle to be filled
 *                arcWidth   the horizontal diameter of the arc at the
 *                            four corners
 *                arcHeight  the vertical diameter of the arc at the
 *                            four corners
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_fillRoundRect() {
    int arcHeight = KNI_GetParameterAsInt(6);
    int  arcWidth = KNI_GetParameterAsInt(5);
    int         h = KNI_GetParameterAsInt(4);
    int         w = KNI_GetParameterAsInt(3);
    int         y = KNI_GetParameterAsInt(2);
    int         x = KNI_GetParameterAsInt(1);
    jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, gHandle);

    KNI_TRANSLATE(thisClass, x, y, gHandle);

    LCDUIfillRoundRect(KNI_PIXEL(thisClass, gHandle),
		       KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		       KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		       KNI_LINESTYLE(thisClass, gHandle),
		       x, y, w, h, arcWidth, arcHeight);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawArc(IIIIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws the outline of a circular or elliptical arc
 *                 covering the specified rectangle, using the current
 *                 color and stroke style.
 * INTERFACE (operand stack manipulation):
 *   parameters:  x            the x coordinate of the upper-left corner
 *                              of the arc to be drawn
 *                y            the y coordinate of the upper-left corner
 *                              of the arc to be drawn
 *                width        the width of the arc to be drawn
 *                height       the height of the arc to be drawn
 *                startAngle   the beginning angle.
 *                arcAngle     the angular extent of the arc, relative to
 *                              the start angle.
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawArc() {
    int   arcAngle = KNI_GetParameterAsInt(6);
    int startAngle = KNI_GetParameterAsInt(5);
    int          h = KNI_GetParameterAsInt(4);
    int          w = KNI_GetParameterAsInt(3);
    int          y = KNI_GetParameterAsInt(2);
    int          x = KNI_GetParameterAsInt(1);
    jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, gHandle);

    KNI_TRANSLATE(thisClass, x, y, gHandle);

    if (arcAngle < 0) {
        startAngle += arcAngle;
        arcAngle = -arcAngle;
    }
    startAngle = (startAngle + 360) % 360;

    LCDUIdrawArc(KNI_PIXEL(thisClass, gHandle),
		 KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		 KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		 KNI_LINESTYLE(thisClass, gHandle),
		 x, y, w, h, startAngle, arcAngle);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      fillArc(IIIIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws the outline of a circular or elliptical arc
 *                 covering the specified rectangle, using the current
 *                 color and stroke style.
 * INTERFACE (operand stack manipulation):
 *   parameters:  x            the x coordinate of the upper-left corner
 *                              of the arc to be filled
 *                y            the y coordinate of the upper-left corner
 *                              of the arc to be filled
 *                width        the width of the arc to be filled
 *                height       the height of the arc to be filled
 *                startAngle   the beginning angle.
 *                arcAngle     the angular extent of the arc, relative to
 *                              the start angle.
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_fillArc() {
    int   arcAngle = KNI_GetParameterAsInt(6);
    int startAngle = KNI_GetParameterAsInt(5);
    int          h = KNI_GetParameterAsInt(4);
    int          w = KNI_GetParameterAsInt(3);
    int          y = KNI_GetParameterAsInt(2);
    int          x = KNI_GetParameterAsInt(1);
    jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, gHandle);

    KNI_TRANSLATE(thisClass, x, y, gHandle);

    if (arcAngle < 0) {
        startAngle += arcAngle;
        arcAngle = -arcAngle;
    }
    startAngle = (startAngle + 360) % 360;

    LCDUIfillArc(KNI_PIXEL(thisClass, gHandle),
		 KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		 KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		 KNI_LINESTYLE(thisClass, gHandle),
		 x, y, w, h, startAngle, arcAngle);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      fillTriangle(IIIIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws a filled triangle connecting the given points
 *                using the current color and stroke style.
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  x1    the x coordinate of the first vertice
 *                y1    the y coordinate of the first vertice
 *                x2    the x coordinate of the second vertice
 *                y2    the y coordinate of the second vertice
 *                x3    the x coordinate of the third vertice
 *                y3    the y coordinate of the third vertice
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_fillTriangle(KNITRAPS) {
    int         y3 = KNI_GetParameterAsInt(6);
    int         x3 = KNI_GetParameterAsInt(5);
    int         y2 = KNI_GetParameterAsInt(4);
    int         x2 = KNI_GetParameterAsInt(3);
    int         y1 = KNI_GetParameterAsInt(2);
    int         x1 = KNI_GetParameterAsInt(1);
    jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, gHandle);

    KNI_TRANSLATE(thisClass, x1, y1, gHandle);
    KNI_TRANSLATE(thisClass, x2, y2, gHandle);
    KNI_TRANSLATE(thisClass, x3, y3, gHandle);

    LCDUIfillTriangle(KNI_PIXEL(thisClass, gHandle),
		      KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		      KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		      KNI_LINESTYLE(thisClass, gHandle),
		      x1, y1, x2, y2, x3, y3);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawString(Ljava/lang/String;III)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws the specified String using the current font
 *                 and color.
 * INTERFACE (operand stack manipulation):
 *   parameters:  str      the String to be drawn
 *                x        the x coordinate of the anchor point
 *                y        the y coordinate of the anchor point
 *                anchor   the anchor point for positioning the text
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawString() {
    int anchor = KNI_GetParameterAsInt(4);
    int      y = KNI_GetParameterAsInt(3);
    int      x = KNI_GetParameterAsInt(2);
    int strLen;

    KNI_StartHandles(7);

    KNI_DeclareHandle(str);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(font);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(fHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(1, str);
    KNI_GetParameterAsObject(0, thisClass);

    strLen = KNI_GetStringLength(str);
    if (strLen < 0) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else if (!checkAnchor(anchor, VCENTER)) {
        KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else {
	unicode* chars;
	jshort   myClip[4]; /* Defined in Graphics.java as 4 shorts */
	int      FONTPARAMS;

	chars = (unicode*)midpMalloc(strLen<<1);

    if (chars == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {

	    KNI_GetObjectClass(thisClass, gHandle);

	    KNI_FONT(thisClass, gHandle, font);
	    KNI_GetObjectClass(font, fHandle);

	    DECLARE_KNI_FONT_PARAMS(font, fHandle);

        KNI_TRANSLATE(thisClass, x, y, gHandle);

	    KNI_GetStringRegion(str, 0, strLen, chars);
	    LCDUIdrawChars(KNI_PIXEL(thisClass, gHandle),
		           KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		           KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		           KNI_LINESTYLE(thisClass, gHandle),
		           FONTPARAMS, x, y, anchor, chars, strLen);

	    midpFree(chars);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawSubString(Ljava/lang/String;IIIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws the specified String using the current font
 *                 and color.
 * INTERFACE (operand stack manipulation):
 *   parameters:  str      the String to be drawn
 *                offset   zero-based index of first character in the
 *                          substring
 *                length   length of the substring
 *                x        the x coordinate of the anchor point
 *                y        the y coordinate of the anchor point
 *                anchor   the anchor point for positioning the text
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawSubstring() {
    int anchor = KNI_GetParameterAsInt(6);
    int      y = KNI_GetParameterAsInt(5);
    int      x = KNI_GetParameterAsInt(4);
    int length = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int strLen;

    KNI_StartHandles(7);

    KNI_DeclareHandle(str);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(font);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(fHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(1, str);
    KNI_GetParameterAsObject(0, thisClass);

    strLen = KNI_GetStringLength(str);
    if (strLen < 0) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else if (   (offset < 0) 
               || (offset > strLen) 
               || (length < 0)
               || (length > strLen)
               || ((offset + length) < 0)
               || ((offset + length) > strLen)) {
        KNI_ThrowNew("java/lang/StringIndexOutOfBoundsException", "");
    } else if (!checkAnchor(anchor, VCENTER)) {
        KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else if (length != 0) {
	    unicode* chars;
	    jshort   myClip[4]; /* Defined in Graphics.java as 4 shorts */
	    int      FONTPARAMS;

	    chars = (unicode*)midpMalloc(length<<1);

        if (chars == NULL) {
            KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        } else {
            KNI_GetStringRegion(str, offset, length, chars);

            KNI_GetObjectClass(thisClass, gHandle);

            KNI_FONT(thisClass, gHandle, font);

            KNI_GetObjectClass(font, fHandle);
            DECLARE_KNI_FONT_PARAMS(font, fHandle);

            KNI_TRANSLATE(thisClass, x, y, gHandle);

            LCDUIdrawChars(KNI_PIXEL(thisClass, gHandle),
                KNI_CLIP(thisClass, gHandle, myClip, cHandle),
                KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
                KNI_LINESTYLE(thisClass, gHandle),
                FONTPARAMS, x, y, anchor, chars, length);

	        midpFree(chars);
        }

    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawChar(CIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws the specified character using the current font
 *                 and color.
 * INTERFACE (operand stack manipulation):
 *   parameters:  ch       the character to be drawn
 *                x        the x coordinate of the anchor point
 *                y        the y coordinate of the anchor point
 *                anchor   the anchor point for positioning the text
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawChar() {
    int anchor = KNI_GetParameterAsInt(4);
    int      y = KNI_GetParameterAsInt(3);
    int      x = KNI_GetParameterAsInt(2);
    unicode  c = KNI_GetParameterAsShort(1);

    KNI_StartHandles(6);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(font);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(fHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(0, thisClass);

    if (!checkAnchor(anchor, VCENTER)) {
        KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else {
	jshort   myClip[4]; /* Defined in Graphics.java as 4 shorts */
	int      FONTPARAMS;

	KNI_GetObjectClass(thisClass, gHandle);

	KNI_FONT(thisClass, gHandle, font);
	KNI_GetObjectClass(font, fHandle);

	DECLARE_KNI_FONT_PARAMS(font, fHandle);

        KNI_TRANSLATE(thisClass, x, y, gHandle);

	LCDUIdrawChars(KNI_PIXEL(thisClass, gHandle),
		       KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		       KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
		       KNI_LINESTYLE(thisClass, gHandle),
		       FONTPARAMS, x, y, anchor, &c, 1);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawChars([CIIIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws the specified characters using the current font
 *                 and color.
 * INTERFACE (operand stack manipulation):
 *   parameters:  data     the array of characters to be drawn
 *                offset   zero-based index of first character to be
 *                          drawn
 *                length   number of characters to be drawn
 *                x        the x coordinate of the anchor point
 *                y        the y coordinate of the anchor point
 *                anchor   the anchor point for positioning the text
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawChars() {
    int anchor = KNI_GetParameterAsInt(6);
    int      y = KNI_GetParameterAsInt(5);
    int      x = KNI_GetParameterAsInt(4);
    int length = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int chLen;

    KNI_StartHandles(7);

    KNI_DeclareHandle(ch);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(font);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(fHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(1, ch);
    KNI_GetParameterAsObject(0, thisClass);

    chLen = KNI_GetArrayLength(ch);
    if (chLen < 0) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else if (   (offset < 0) 
               || (offset > chLen) 
               || (length < 0)
               || (length > chLen)
               || ((offset + length) < 0)
               || ((offset + length) > chLen)) {
        KNI_ThrowNew("java/lang/ArrayIndexOutOfBoundsException", "");
    } else if (!checkAnchor(anchor, VCENTER)) {
        KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else if (length != 0) {
        unicode* chars;
        jshort   myClip[4]; /* Defined in Graphics.java as 4 shorts */
        int      FONTPARAMS;

        chars = (unicode*)midpMalloc(length<<1);

        if (chars == NULL) {
            KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        } else {
 
            KNI_GetRawArrayRegion(ch, (offset<<1), (length<<1), (jbyte*)chars);

            KNI_GetObjectClass(thisClass, gHandle);

            KNI_FONT(thisClass, gHandle, font);

            KNI_GetObjectClass(font, fHandle);
            DECLARE_KNI_FONT_PARAMS(font, fHandle);

            KNI_TRANSLATE(thisClass, x, y, gHandle);

            LCDUIdrawChars(KNI_PIXEL(thisClass, gHandle),
                   KNI_CLIP(thisClass, gHandle, myClip, cHandle),
                   KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
                   KNI_LINESTYLE(thisClass, gHandle),
                   FONTPARAMS, x, y, anchor, chars, length);

            midpFree(chars);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawImage(Ljavax/microedition/lcdui/Image;III)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws the specified image by using the anchor point.
 * INTERFACE (operand stack manipulation):
 *   parameters:  image    the image to be drawn
 *                x        the x coordinate of the anchor point
 *                y        the y coordinate of the anchor point
 *                anchor   the anchor point for positioning the image
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawImage() {
    jint anchor = KNI_GetParameterAsInt(4);
    jint      y = KNI_GetParameterAsInt(3);
    jint      x = KNI_GetParameterAsInt(2);

    KNI_StartHandles(5);
    KNI_DeclareHandle(img);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(1, img);
    KNI_GetParameterAsObject(0, thisClass);

    if (KNI_IsNullHandle(img)) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else if (!checkAnchor(anchor, BASELINE)) {
        KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else {
        jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */
        int    defErr = -1;

        if ((int)getImageData(img, (void*)&defErr) == -1) {
            printf("getImageData() returned ptr to -1; ignore image\n");
        } else {

            jbyte ret;

            jint width = (int)KNI_GetIntField(img,
                 _CACHE_FIELDID(iHandle, "width", "I", _i_width_cache));
            jint height = (int)KNI_GetIntField(img,
                 _CACHE_FIELDID(iHandle, "height", "I", _i_height_cache));

            jint x_src = 0, 
                 y_src = 0;

            KNI_GetObjectClass(thisClass, gHandle);
            KNI_TRANSLATE(thisClass, x, y, gHandle);

            KNI_CLIP(thisClass, gHandle, myClip, cHandle);

            ret = adjustToViewableRegion(&x, &y, &x_src, &y_src,
                     &width, &height, myClip, anchor, 0);

            if (ret != NO_IMAGE) {

                LCDUIdrawRegionTransform(myClip,
                    KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
                    getImageData(img, NULL),
                    x, y, anchor, x_src, y_src,
                    width, height, 0, ret == 2 ? KNI_TRUE : KNI_FALSE);

            }
        }
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*========================================================================
 * FUNCTION:      drawRGB([IIIIIIIZ)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws from the given data array of 0xAARRGGBB pixels
 * INTERFACE (Operand stack manipulation):
 *   parameters:  rgbData      an array of rgb pixels to draw
 *                offset       offset in rgbData of the first pixel to use
 *                scanlen      distance between pixels in the same
 *                             column but adjacent rows
 *                x            x coordinate of the upper left corner
 *                             of the region to draw
 *                y            y coordinate of the upper left corner
 *                             of the region to draw
 *                width        width of the target region
 *                height       height of the target region
 *                processAlpha use/ignore alpha channel bytes
 *   returns:     <nothing>
 *======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawRGB() {
    jboolean processAlpha = KNI_GetParameterAsBoolean(8);
    jint height = KNI_GetParameterAsInt(7);
    jint width = KNI_GetParameterAsInt(6);
    jint y = KNI_GetParameterAsInt(5);
    jint x = KNI_GetParameterAsInt(4);
    jint scanlen = KNI_GetParameterAsInt(3);
    jint offset = KNI_GetParameterAsInt(2);
    jint buflen;
    jint *rgbBuffer;
    
    KNI_StartHandles(5);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);

    KNI_GetParameterAsObject(1, rgbData);
    KNI_GetParameterAsObject(0, thisClass);

    if (KNI_IsNullHandle(rgbData)) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else {

	    buflen = KNI_GetArrayLength(rgbData);

        if ((height < 0) || (width < 0) || (y < 0) || (x < 0) ||
	       (scanlen < 0) || (offset < 0) ||
	       ((offset + (height * scanlen)) > buflen)) {

	        KNI_ThrowNew("java/lang/ArrayIndexOutOfBoundsException", "");

        } else {
            rgbBuffer = (jint*)midpMalloc(sizeof(jint) * buflen);
            if (rgbBuffer == NULL) {
                KNI_ThrowNew("java/lang/OutOfMemoryError", "");
            } else {
                jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */

                jbyte ret;

                jint x_src = 0,
                     y_src = 0;

                KNI_GetRawArrayRegion(rgbData, 0, buflen * sizeof(int), 
                                      (jbyte*)rgbBuffer);

                KNI_GetObjectClass(thisClass, gHandle);
		
                KNI_TRANSLATE(thisClass, x, y, gHandle);
		
                KNI_CLIP(thisClass, gHandle, myClip, cHandle);

                ret = adjustToViewableRegion(&x, &y, &x_src, &y_src,
                         &width, &height, myClip, 0, 0);

                if (ret != NO_IMAGE) {
		    LCDUIdrawRGB(myClip,
                        KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
                        &rgbBuffer, offset, scanlen, x, y, width, 
                        height, processAlpha);
                }

                midpFree(rgbBuffer);
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawRegion(Ljavax/microedition/lcdui/Image;IIIIIIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Draws a region of the specified image 
 *                possibly after applying a transformation.
 * INTERFACE (operand stack manipulation):
 *   parameters:  srcImage      the image to be drawn
 *                x_src         the x coord of the upper left corner
 *                              of the source region
 *                y_src         the y coord of the upper left corner
 *                              of the source region
 *                width         the width of the source region
 *                height        the height of the source region
 *                transform     the desired transformation
 *                x_dest        the x coordinate of the anchor point
 *                y_dest        the y coordinate of the anchor point
 *                anchor        the anchor point for positioning the image
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_drawRegion() {
    jint anchor = KNI_GetParameterAsInt(9);
    jint y_dest = KNI_GetParameterAsInt(8);
    jint x_dest = KNI_GetParameterAsInt(7);
    jint transform = KNI_GetParameterAsInt(6);
    jint height = KNI_GetParameterAsInt(5);
    jint width  = KNI_GetParameterAsInt(4);
    jint y_src  = KNI_GetParameterAsInt(3);
    jint x_src  = KNI_GetParameterAsInt(2);

    KNI_StartHandles(6);
    KNI_DeclareHandle(srcImage);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);
    KNI_DeclareHandle(gdestHandle);

    KNI_GetParameterAsObject(1, srcImage);
    KNI_GetParameterAsObject(0, thisClass);

    if (KNI_IsNullHandle(srcImage)) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else if (!checkAnchor(anchor, BASELINE)) {
        KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else if ((transform < 0) || (transform > 7)) {
        KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else {
        jint img_width;
        jint img_height;
	KNI_GetObjectClass(thisClass, gHandle);
        KNI_GetObjectClass(srcImage, iHandle);

        img_width = (int)KNI_GetIntField(srcImage,
                                         _CACHE_FIELDID(iHandle, "width", "I", _i_width_cache));
        img_height = (int)KNI_GetIntField(srcImage,
                                          _CACHE_FIELDID(iHandle, "height", "I", _i_height_cache));

        KNI_IMAGE(thisClass, gHandle, gdestHandle);

        if (KNI_IsSameObject(srcImage, gdestHandle) ||
            (height < 0) || (width < 0) || (x_src < 0) || (y_src < 0) ||
            ((x_src + width) > img_width) || ((y_src + height) > img_height)) {
            KNI_ThrowNew("java/lang/IllegalArgumentException", "");
        } else {
            jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */   

            jbyte ret;

            KNI_TRANSLATE(thisClass, x_dest, y_dest, gHandle);

            KNI_CLIP(thisClass, gHandle, myClip, cHandle);

            ret = adjustToViewableRegion(&x_dest, &y_dest, &x_src, &y_src,
                     &width, &height, myClip, anchor, transform);

            if (ret != NO_IMAGE) {

                LCDUIdrawRegionTransform(myClip,
                    KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL), 
                    getImageData(srcImage, NULL),
                    x_dest, y_dest, anchor, x_src, y_src,
                    width, height, transform, ret == 2 ? KNI_TRUE : KNI_FALSE);
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      doCopyArea(Ljavax/microedition/lcdui/Image;IIIIIII)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Copies a region of the specified image 
 *                to another location in the image
 * INTERFACE (operand stack manipulation):
 *   parameters:  x_src         the x coord of the upper left corner
 *                              of the source region
 *                y_src         the y coord of the upper left corner
 *                              of the source region
 *                width         the width of the source region
 *                height        the height of the source region
 *                x_dest        the x coordinate of the anchor point
 *                y_dest        the y coordinate of the anchor point
 *                anchor        the anchor point for positioning the 
 *                              copied region
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_doCopyArea() {
    int anchor = KNI_GetParameterAsInt(7);
    int y_dest = KNI_GetParameterAsInt(6); 
    int x_dest = KNI_GetParameterAsInt(5);
    int height = KNI_GetParameterAsInt(4);
    int width  = KNI_GetParameterAsInt(3);
    int y_src  = KNI_GetParameterAsInt(2);
    int x_src  = KNI_GetParameterAsInt(1);
    jshort gfx_width;
    jshort gfx_height;
    jshort myClip[4]; /* Defined in Graphics.java as 4 shorts */   

    KNI_StartHandles(5); 
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(gHandle);
    KNI_DeclareHandle(cHandle);
    KNI_DeclareHandle(iHandle);
    KNI_DeclareHandle(gdestHandle);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, gHandle);

    gfx_width = 
	(jshort)KNI_GetShortField(thisClass, _CACHE_FIELDID(gHandle, 
				  "maxWidth", "S", _g_maxwidth_cache));
    gfx_height = 
	(jshort)KNI_GetShortField(thisClass, _CACHE_FIELDID(gHandle, 
				  "maxHeight", "S", _g_maxheight_cache));

    if((height < 0) || (width < 0) || (x_src < 0) || (y_src < 0) ||
       ((x_src + width) > gfx_width) || ((y_src + height) > gfx_height)) {
	KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else {

	KNI_IMAGE(thisClass, gHandle, gdestHandle);

	KNI_TRANSLATE(thisClass, x_dest, y_dest, gHandle);

	LCDUIcopyArea(KNI_CLIP(thisClass, gHandle, myClip, cHandle),
		      KNI_IMAGEDST(thisClass, gHandle, iHandle, NULL),
		      x_src, y_src, width, height, 
		      x_dest, y_dest, anchor);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      getPixel(IIZ)I
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Get a specific pixel value
 * INTERFACE (operand stack manipulation):
 *   parameters:  rgb
 *                gray
 *                isGray
 *   returns:     
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_Graphics_getPixel() {
    int isGray = KNI_GetParameterAsBoolean(3);
    int   gray = KNI_GetParameterAsInt(2);
    int    rgb = KNI_GetParameterAsInt(1);

    KNI_ReturnInt(LCDUIgetPixel(rgb, gray, isGray));
}

/*=========================================================================
 * FUNCTION:      getDisplayColor(I)I
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      map RGB value to RGB value displayed on device
 * INTERFACE (operand stack manipulation):
 *   parameters:  color - rgb value to map to display
 *   returns:     (int) rgb value used to display this color on device
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_Graphics_getDisplayColor() {
    int color = KNI_GetParameterAsInt(1);
    KNI_ReturnInt(LCDUIgetDisplayColor(color));
}

/*=========================================================================
 * FUNCTION:      init()V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Intialize the native peer of this Graphics context
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Graphics_init() {
    /* We don't need to do anything here.. */
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      init(III)V
 * CLASS:         javax.microedition.lcdui.Graphics
 * TYPE:          virtual native function
 * OVERVIEW:      Intialize the native peer of this Font
 * INTERFACE (operand stack manipulation):
 *   parameters:  face    The face to initialize the native Font
 *                style   The style to initialize the native Font
 *                size    The point size to initialize the native Font
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Font_init() {
    int size  = (int)KNI_GetParameterAsInt(3);
    int style = (int)KNI_GetParameterAsInt(2);
    int face  = (int)KNI_GetParameterAsInt(1);
    int ascent, descent, leading;

    KNI_StartHandles(2);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, clazz);

    LCDUIgetFontInfo(FONTPARAMS, &ascent, &descent, &leading);
 
    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "baseline", "I"), 
		    ascent);
    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "height", "I"), 
		    ascent + descent + leading);
    
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      charWidth(C)I
 * CLASS:         javax.microedition.lcdui.Font
 * TYPE:          virtual native function
 * OVERVIEW:      Gets the advance width of the specified character in
 *                 this Font.
 * INTERFACE (operand stack manipulation):
 *   parameters:  ch   the character to be measured
 *   returns:     the total advance width (a non-negative value)
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_Font_charWidth() {
    unicode c = (unicode)KNI_GetParameterAsChar(1);
    int FONTPARAMS;

    KNI_StartHandles(2);

    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(handle);
    KNI_GetParameterAsObject(0, thisClass);

    KNI_GetObjectClass(thisClass, handle);
    DECLARE_KNI_FONT_PARAMS(thisClass, handle);

    KNI_EndHandles();
    KNI_ReturnInt(LCDUIcharsWidth(FONTPARAMS, &c, 1));
}

/*=========================================================================
 * FUNCTION:      charsWidth([CII)I
 * CLASS:         javax.microedition.lcdui.Font
 * TYPE:          virtual native function
 * OVERVIEW:      Gets the advance width of the characters, starting at
 *                 the specified offset and for the specified number of
 *                 characters.
 * INTERFACE (operand stack manipulation):
 *   parameters:  ch       the array of characters to be measured
 *                offset   the index of the first character to measure
 *                length   the number of characters to measure
 *   returns:     the width of the character range
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_Font_charsWidth() {
    int length = (int)KNI_GetParameterAsInt(3);
    int offset = (int)KNI_GetParameterAsInt(2);
    int chLen;
    jint result = 0;

    KNI_StartHandles(3);

    KNI_DeclareHandle(ch);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(handle);

    KNI_GetParameterAsObject(1, ch);
    KNI_GetParameterAsObject(0, thisClass);

    if ((chLen = KNI_GetArrayLength(ch)) == -1) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else if (   (offset < 0) 
               || (offset > chLen) 
               || (length < 0)
               || (length > chLen)
               || ((offset + length) < 0)
               || ((offset + length) > chLen)) {
        KNI_ThrowNew("java/lang/ArrayIndexOutOfBoundsException", "");
    } else if (length != 0) {
        unicode* chars;
        int      FONTPARAMS;

        KNI_GetObjectClass(thisClass, handle);
        DECLARE_KNI_FONT_PARAMS(thisClass, handle);

        chars = (unicode*)midpMalloc(length<<1);
        if (chars == NULL) {
            KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        } else {
            KNI_GetRawArrayRegion(ch, (offset<<1), (length<<1), (jbyte*)chars);
            result = LCDUIcharsWidth(FONTPARAMS, chars, length);
            midpFree(chars);
        }
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/*=========================================================================
 * FUNCTION:      stringWidth(Ljava/lang/String;)I
 * CLASS:         javax.microedition.lcdui.Font
 * TYPE:          virtual native function
 * OVERVIEW:      Gets the total advance width of the specified String
 *                 in this font.
 * INTERFACE (operand stack manipulation):
 *   parameters:  str   the String to be measured
 *   returns:     the total advance width
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_Font_stringWidth() {
    int stringLen;
    jint result = 0;

    KNI_StartHandles(3);

    KNI_DeclareHandle(string);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(handle);

    KNI_GetParameterAsObject(1, string);
    KNI_GetParameterAsObject(0, thisClass);

    if ((stringLen = KNI_GetStringLength(string)) == -1) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else {
        unicode* chars;
        int      FONTPARAMS;

        KNI_GetObjectClass(thisClass, handle);
        DECLARE_KNI_FONT_PARAMS(thisClass, handle);

        chars = (unicode*)midpMalloc((stringLen<<1));
        if (chars == NULL) {
            KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        } else {
            KNI_GetStringRegion(string, 0, stringLen, chars);
            result = LCDUIcharsWidth(FONTPARAMS, chars, stringLen);
            midpFree(chars);
        }
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/*=========================================================================
 * FUNCTION:      substringWidth(Ljava/lang/String;II)I
 * CLASS:         javax.microedition.lcdui.Font
 * TYPE:          virtual native function
 * OVERVIEW:      Gets the total advance width of the specified substring
 *                 in this font.
 * INTERFACE (operand stack manipulation):
 *   parameters:  str      the String to be measured
 *                offset   zero-based index of first character in
 *                          the substring
 *                length   length of the substring
 *   returns:     the total advance width
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_Font_substringWidth() {
    int length = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int stringLen;
    jint result = 0;

    KNI_StartHandles(3);

    KNI_DeclareHandle(string);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(handle);

    KNI_GetParameterAsObject(1, string);
    KNI_GetParameterAsObject(0, thisClass);

    if ((stringLen = KNI_GetStringLength(string)) == -1) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else if ( (offset < 0) 
		|| (offset > stringLen) 
		|| (length < 0)
		|| (length > stringLen)
		|| ((offset + length) < 0)
		|| ((offset + length) > stringLen)) {
	KNI_ThrowNew("java/lang/StringIndexOutOfBoundsException", "");
    } else if (length != 0) {
        unicode* chars;
        int      FONTPARAMS;

        KNI_GetObjectClass(thisClass, handle);
        DECLARE_KNI_FONT_PARAMS(thisClass, handle);

        chars = (unicode*)midpMalloc((length<<1));
        if (chars == NULL) {
            KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        } else {
            KNI_GetStringRegion(string, offset, length, chars);
            result = LCDUIcharsWidth(FONTPARAMS, chars, length);
            midpFree(chars);
        }
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/*=========================================================================
 * FUNCTION:      refresh(IIII)V
 * CLASS:         javax.microedition.lcdui.Display
 * TYPE:          virtual native function
 * OVERVIEW:      redraw a portiion of the display
 * INTERFACE (operand stack manipulation):
 *   parameters:  x1   upper left corner x-coordinate
 *                y1   upper left corner y-coordinate
 *                x2   lower right corner x-coordinate
 *                y2   lower right corner y-coordinat
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Display_refresh() {
    int y2 = KNI_GetParameterAsInt(4);
    int x2 = KNI_GetParameterAsInt(3);
    int y1 = KNI_GetParameterAsInt(2);
    int x1 = KNI_GetParameterAsInt(1);

    LCDUIrefresh(x1, y1, x2, y2);
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      setVerticalScroll(II)V
 * CLASS:         javax.microedition.lcdui.Display
 * TYPE:          virtual native function
 * OVERVIEW:      set the current vertical scroll position and proportion
 * INTERFACE (operand stack manipulation):
 *   parameters:  scrollPosition     vertical scroll position
 *                scrollProportion   vertical scroll proportion
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Display_setVerticalScroll() {
    int scrollProportion = KNI_GetParameterAsInt(2);
    int   scrollPosition = KNI_GetParameterAsInt(1);

    LCDUIsetVerticalScroll(scrollPosition, scrollProportion);
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      drawTrustedIcon(I)V
 * CLASS:         javax.microedition.lcdui.Display
 * TYPE:          virtual native function
 * OVERVIEW:      set the drawing of the trusted MIDlet icon
 * INTERFACE (operand stack manipulation):
 *   parameters:  drawTrusted     
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Display_drawTrustedIcon() {
    jboolean drawTrusted = KNI_GetParameterAsBoolean(1);

    LCDUIdrawTrustedIcon(drawTrusted);
  
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      setInputMode(I)V
 * CLASS:         javax.microedition.lcdui.Display
 * TYPE:          virtual native function
 * OVERVIEW:      set the input mode
 * INTERFACE (operand stack manipulation):
 *   parameters:  mode   type of input to accept
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Display_setInputMode() {
    int mode = KNI_GetParameterAsInt(1);

    LCDUIsetInputMode(mode);
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      playAlertSound(I)Z
 * CLASS:         javax.microedition.lcdui.Display
 * TYPE:          virtual native function
 * OVERVIEW:      play a sound
 * INTERFACE (operand stack manipulation):
 *   parameters:  alertType   type of alert
 *   returns:     true if sound was played.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_lcdui_Display_playAlertSound() {
    int alertType = KNI_GetParameterAsInt(1);

    KNI_ReturnBoolean((jboolean)LCDUIplaySound(alertType));
}

/*=========================================================================
 * FUNCTION:      showBacklight(I)Z
 * CLASS:         com.sun.midp.lcdui.DisplayDeviceAccess
 * TYPE:          virtual native function
 * OVERVIEW:      show Backlight
 * INTERFACE (operand stack manipulation):
 *   parameters:  on true to turn on the backlight and false to turn it off
 *   returns:     true if backlight was shown.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_lcdui_DisplayDeviceAccess_showBacklight() {
    jboolean turnOn = KNI_GetParameterAsBoolean(1);
    KNI_ReturnBoolean((jboolean)LCDUIshowBacklight(turnOn));
}


/*=========================================================================
 * FUNCTION:      init()V
 * CLASS:         javax.microedition.lcdui.DeviceCaps
 * TYPE:          virtual native function
 * OVERVIEW:      Retreive initial settings for display capabilities
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_DeviceCaps_init() {
    int width, height, adornedHeight, eraseColor, depth, flags, numAlphaLevels;

    KNI_StartHandles(2);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(0, thisClass);

    LCDUIgetDisplayParams(&width, &height, &adornedHeight, &eraseColor, &depth, &flags, &numAlphaLevels);

    KNI_GetObjectClass(thisClass, clazz);

    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "width", "I"), width);
    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "height", "I"), height);

    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "adornedHeight", "I"), adornedHeight);

    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "eraseColor", "I"),eraseColor);
    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "displayDepth", "I"), depth);
    KNI_SetBooleanField(thisClass, KNI_GetFieldID(clazz, "displayIsColor", "Z"), 
			(jboolean)((flags & SUPPORTS_COLOR) != 0));
    KNI_SetBooleanField(thisClass, KNI_GetFieldID(clazz, "pointerSupported", "Z"),
			(jboolean)((flags & SUPPORTS_POINTER) != 0));
    KNI_SetBooleanField(thisClass, KNI_GetFieldID(clazz, "motionSupported", "Z"),
			(jboolean)((flags & SUPPORTS_MOTION) != 0));
    KNI_SetBooleanField(thisClass, KNI_GetFieldID(clazz, "repeatSupported", "Z"),
			(jboolean)((flags & SUPPORTS_REPEAT) != 0));
    KNI_SetBooleanField(thisClass, KNI_GetFieldID(clazz, "isDoubleBuffered", "Z"),
			(jboolean)((flags & SUPPORTS_DOUBLEBUFFER) != 0));

    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "numAlphaLevels", "I"), numAlphaLevels);
    
    KNI_SetIntField(thisClass,
	KNI_GetFieldID(clazz, "keyCodeUp", "I"), KEY_UP);
    KNI_SetIntField(thisClass,
	KNI_GetFieldID(clazz, "keyCodeDown", "I"), KEY_DOWN);
    KNI_SetIntField(thisClass,
	KNI_GetFieldID(clazz, "keyCodeLeft", "I"), KEY_LEFT);
    KNI_SetIntField(thisClass,
	KNI_GetFieldID(clazz, "keyCodeRight", "I"), KEY_RIGHT);
    KNI_SetIntField(thisClass,
	KNI_GetFieldID(clazz, "keyCodeSelect", "I"), KEY_SELECT);

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/*=========================================================================
 * FUNCTION:      init()V
 * CLASS:         javax.microedition.lcdui.GameDeviceCaps
 * TYPE:          virtual native function
 * OVERVIEW:      Retreive initial settings for game display capabilities
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_game_GameDeviceCaps_init() {
    int width, height, dummy;

    KNI_StartHandles(2);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(0, thisClass);

    /* use lcdui's display parameters for the game device caps.*/
    LCDUIgetDisplayParams(&width, &height, 
			  &dummy, &dummy, &dummy, &dummy, &dummy);

    KNI_GetObjectClass(thisClass, clazz);

    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "width", "I"), width);
    KNI_SetIntField(thisClass, KNI_GetFieldID(clazz, "height", "I"), height);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      createMutableImage(II)V
 * CLASS:         javax.microedition.lcdui.MutableImage
 * TYPE:          virtual native function
 * OVERVIEW:      Create a mutable image
 * INTERFACE (operand stack manipulation):
 *   parameters:  width    the width of the new mutable image
 *                height   the height of the new mutable image
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_MutableImage_createMutableImage() {
    int         height = KNI_GetParameterAsInt(2);
    int          width = KNI_GetParameterAsInt(1);
    imageDstPtr    dst;

    if ((width < 0) || (height < 0)) {
        KNI_ThrowNew("java/lang/IllegalArgumentException", "");
        KNI_ReturnVoid();
    }

    KNI_StartHandles(2);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(0, thisClass);

    dst = LCDUIcreateImageDst(KNI_TRUE);
    if (dst != NULL) {
        void* imgHandle;

	dst->setSize(dst, width, height);
	imgHandle = dst->done(dst, &width, &height);

	KNI_GetObjectClass(thisClass, clazz);
	KNI_SetIntField(thisClass, 
                        _CACHE_FIELDID(clazz, "imgData", "I", _i_imgData_cache), 
			(jint)imgHandle);
	KNI_registerCleanup(thisClass, destroyImageData);
	midpFree(dst);
    } else {
	KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      createImmutableCopy(IILjavax/microedition/lcdui/Image;)V
 * CLASS:         javax.microedition.lcdui.ImmutableImage
 * TYPE:          virtual native function
 * OVERVIEW:      Create an immutable copy of an image
 * INTERFACE (operand stack manipulation):
 *   parameters:  width    the width of the new immutable image
 *                height   the height of the new immutable image
 *                img      the Image to make a copy of
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImmutableImage_createImmutableCopy() {
    int         height = KNI_GetParameterAsInt(2);
    int          width = KNI_GetParameterAsInt(1);
    imageDstPtr    dst;

    if ((width < 0) || (height < 0)) {
        KNI_ThrowNew("java/lang/IllegalArgumentException", "");
        KNI_ReturnVoid();
    }

    KNI_StartHandles(3);
    KNI_DeclareHandle(img);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(3, img);
    KNI_GetParameterAsObject(0, thisClass);

    dst = LCDUIcreateImageDst(KNI_FALSE);
    if (dst != NULL) {
	void* imgHandle;
	dst->setSize(dst, width, height);
	dst->copyPixels(dst, getImageData(img, NULL));
	imgHandle = dst->done(dst, &width, &height);
	KNI_GetObjectClass(thisClass, clazz);
	KNI_SetIntField(thisClass, 
                        _CACHE_FIELDID(clazz, "imgData", "I", _i_imgData_cache), 
			(jint)imgHandle);
	KNI_registerCleanup(thisClass, destroyImageData);
	midpFree(dst);
    } else {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

#define TRANSFORM_INVERTED_AXES 0x4

/*=========================================================================
 * FUNCTION: createImmutableImageRegion(Ljavax/microedition/lcdui/Image;IIIII)V
 * CLASS:         javax.microedition.lcdui.ImmutableImage
 * TYPE:          virtual native function
 * OVERVIEW:      Create an immutable image from the region of an Image
 * INTERFACE (operand stack manipulation):
 *   parameters:  
 *                img       the Image to make a copy of
 *                width     the width of the new immutable image
 *                height    the height of the new immutable image
 *                x         the x offset of the top left of the region
 *                y         the y offset of the top left of the region
 *                transform the transform to apply to the region
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImmutableImage_createImmutableImageRegion() {
    int      transform = KNI_GetParameterAsInt(6);
    int         height = KNI_GetParameterAsInt(5);
    int          width = KNI_GetParameterAsInt(4);
    int              y = KNI_GetParameterAsInt(3);
    int              x = KNI_GetParameterAsInt(2);
    imageDstPtr    dst;

    KNI_StartHandles(3);
    KNI_DeclareHandle(srcImg);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(1, srcImg);
    KNI_GetParameterAsObject(0, thisClass);

    /* at this point all exception cases have been checked for. */

    dst = LCDUIcreateImageDst(KNI_FALSE);
    if (dst != NULL) {
	void* imgHandle;

	/* set dimensions of image being created,
	   depending on transform */
	if (transform & TRANSFORM_INVERTED_AXES) {
	  dst->setSize(dst, height, width);
	} else {
	  dst->setSize(dst, width, height);
	}

	dst->copyPixelsTransformed(dst, getImageData(srcImg, NULL),
				   x, y, 
				   width, height,
				   transform);

	imgHandle = dst->done(dst, &width, &height);

	KNI_GetObjectClass(thisClass, clazz);
	KNI_SetIntField(thisClass,
                        _CACHE_FIELDID(clazz, "imgData", "I", _i_imgData_cache),
			(jint)imgHandle);
	KNI_registerCleanup(thisClass, destroyImageData);
	midpFree(dst);
    } else {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      getRGB([IIIIIII)V
 * CLASS:         javax.microedition.lcdui.Image
 * Type:          virtual native function
 * OVERVIEW:      Get an array of ARGB values from an Image
 * INTERFACE (operand stack manipulation):
 *   parameters:  rgbData    int array for target ARGB data
 *                offset     the index int the array where the first
 *                           ARGB value will be stored
 *                scanlength the relative offset in the array between
 *                           pixels in the same column and adjacent rows
 *                x          the x coordinate of the upper left corner
 *                           of the selected region
 *                y          the y coordinate of the upper left corner
 *                           of the selected region
 *                width      the width of the region
 *                height     the height of the region
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Image_getRGB() {
    int height = KNI_GetParameterAsInt(7);
    int width = KNI_GetParameterAsInt(6);
    int y = KNI_GetParameterAsInt(5);
    int x = KNI_GetParameterAsInt(4);
    int scanlength = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int buflen;
    int *rgbBuffer;
    int img_width;
    int img_height;
    jboolean iae = KNI_FALSE;

    KNI_StartHandles(3);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(1, rgbData);
    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, clazz);

    img_width = (int)KNI_GetIntField(thisClass, 
				     _CACHE_FIELDID(clazz, "width", "I", 
						    _i_width_cache));
    img_height = (int)KNI_GetIntField(thisClass, 
				      _CACHE_FIELDID(clazz, "height", "I", 
						     _i_height_cache));
    /* see if absolute value of scanlength is greater than or equal to width */
    if (scanlength >= 0 && scanlength < width) {
	iae = KNI_TRUE;
    } else if (scanlength < 0 && (0 - scanlength) < width) {
	iae = KNI_TRUE;
    }
    if (KNI_IsNullHandle(rgbData)) {
        KNI_ThrowNew("java/lang/NullPointerException", "");
    } else if((y < 0) || (x < 0) || (x + width > img_width) ||
	      (y + height > img_height) || iae == KNI_TRUE) {
	KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else if (height < 0 || width < 0 ) {
	/* spec says noop in this case */
    } else {
        buflen = KNI_GetArrayLength(rgbData);
        if (offset < 0 
            || offset + ((height - 1) * scanlength) + width > buflen 
            || offset + ((height - 1) * scanlength) < 0) {
            KNI_ThrowNew("java/lang/ArrayIndexOutOfBoundsException", "");
        } else {
	    rgbBuffer = (int*)midpMalloc(sizeof(int) * buflen);
	    if (rgbBuffer == NULL) {
                KNI_ThrowNew("java/lang/OutOfMemoryError", "");
	    } else {
		
                KNI_GetRawArrayRegion(rgbData, 0, buflen * sizeof(int), 
				      (jbyte*)rgbBuffer);
		
                LCDUIgetRGB(&rgbBuffer, offset, scanlength, 
			    x, y, width, height, 
			    getImageData(thisClass, NULL));
		
                /* copy the array back into Java space */
                KNI_SetRawArrayRegion(rgbData, 0, buflen * sizeof(int),
				      (jbyte*)rgbBuffer);
		
                midpFree(rgbBuffer);
            }
	}
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/*=========================================================================
 * FUNCTION:      decodeImage([BII)V
 * CLASS:         javax.microedition.lcdui.ImmutableImage
 * TYPE:          virtual native function
 * OVERVIEW:      Decode an Image from a byte array
 * INTERFACE (operand stack manipulation):
 *   parameters:  intputData   byte array containing image data
 *                offset       the start of the image data within the
 *                              byte array
 *                length       the length of the image data in the
 *                              byte array
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImmutableImage_decodeImage() {
    int            length = KNI_GetParameterAsInt(3);
    int            offset = KNI_GetParameterAsInt(2);
    imageDstPtr       dst = NULL;
    imageSrcPtr       src = NULL;
    char*       pngBuffer;

    KNI_StartHandles(3);
    KNI_DeclareHandle(pngData);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(1, pngData);
    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, clazz);

    if ((offset < 0) || 
	(length < 0) || 
	(offset + length) > KNI_GetArrayLength(pngData)) {
        KNI_ThrowNew("java/lang/ArrayIndexOutOfBoundsException", "");
    } else {
        pngBuffer = (char*)midpMalloc(sizeof(char) * length);
        if (pngBuffer == NULL) {
            KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        } else {

            KNI_GetRawArrayRegion(pngData, offset, length, (jbyte*)pngBuffer);
	
            /* Do the decoding of the png in the buffer and initialize
             * the class variables.
             */
            decodeImage(pngBuffer, length, thisClass, clazz);

            midpFree(pngBuffer);
        }
    }
    
    if (dst != NULL) {
        midpFree(dst);
    }

    if (src != NULL) {
        src->done(src);
        midpFree(src);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      loadIcon(Ljava/lang/String;)V
 * CLASS:         javax.microedition.lcdui.ImmutableImage
 * TYPE:          virtual native function
 * OVERVIEW:      Decode an Image from a system resource file
 * INTERFACE (operand stack manipulation):
 *   parameters:  name   String containing the name.
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImmutableImage_loadIcon() {
    int len;
    char*          buffer;
    int  bufferlen = 1024;	/* at least max filename size */
    int            handle;
    char*          errmsg;

    KNI_StartHandles(3);
    KNI_DeclareHandle(resname);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(1, resname);
    KNI_GetParameterAsObject(0, thisClass);
    KNI_GetObjectClass(thisClass, clazz);

    /* Allocate a buffer used first for the name and then icon data. */
    buffer = (char*)midpMalloc(bufferlen);
    if (buffer == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
	/* Locate the resource file in the $MIDP_HOME/lib directory. */
    
	(void)strcpy(buffer, getStorageRoot()); /* includes trailing filesep */
	(void)strcat(buffer, "..");
	(void)strcat(buffer, getFileSeparator());
	(void)strcat(buffer, "lib");
	(void)strcat(buffer, getFileSeparator());

	len = KNI_GetArrayLength(resname);
	KNI_GetRawArrayRegion(resname, 0, len,
			      (jbyte*)&buffer[strlen(buffer)]);

	handle = storageOpen(&errmsg, buffer, OPEN_READ);
	if (handle < 0) {
	    fprintf(stderr, "Warning: could not load icon %s; %s\n",
		    buffer, errmsg);
	    len = sizeof(defaultIcon);
	    memcpy(buffer, defaultIcon, len);
	} else {
	    /* Get size of icon
	     * Re-use the buffer for the contents of the icon resource
	     */
	    len = storageSizeOf(&errmsg, handle);

	    /* If buffer not large enough, reallocate */
	    if (len > bufferlen) {
		midpFree(buffer);
		buffer = midpMalloc(len);
		bufferlen = len;
		if (buffer == NULL) {
		    KNI_ThrowNew("java/lang/OutOfMemoryError", "");
		}
	    }

	    if (buffer != NULL) {
		int l = storageRead(&errmsg, handle, buffer, len);
		if (errmsg != NULL || len != l) {
		    KNI_ThrowNew("java/io/IOException", errmsg);
		}
	    }

	    /* close storage file */
	    storageClose(&errmsg, handle);
	}

	/* Do the decoding of the png in the buffer and initialize
	 * the class variables.
	 */
	decodeImage(buffer, len, thisClass, clazz);

	if (buffer != NULL) {
	    midpFree(buffer);
	}
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Function to initialize a Image object with
 * PNG data decoded from a buffer.
 *  parameters:  buffer containing png image
 *               length of png image
 *               instance reference to Image instance
 *               reference to Image class
 *  returns:     <nothing>
 * 
 */
static void decodeImage(char *buffer, const int length,
			const jobject thisClass, const jobject clazz) {
    imageDstPtr       dst = NULL;
    imageSrcPtr       src = NULL;
    int            height;
    int             width;

    /* Clear imgData field to zero in case of failure */
    KNI_SetIntField(thisClass, 
		    _CACHE_FIELDID(clazz, "imgData", "I", _i_imgData_cache), 
		    (jint)0);

    /* Create the image from the buffered data */
    if ((dst = LCDUIcreateImageDst(KNI_FALSE)) == NULL) {
	KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } if ((src = createImageSrcFromData(&buffer, length)) == NULL) {
	/* Destroy native image data */
	LCDUIdestroyNativeImage(dst->done(dst, NULL, NULL));
	KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else if (!PNGdecodeImage(src, dst)) {
	/* Destroy native image data */
	LCDUIdestroyNativeImage(dst->done(dst, NULL, NULL));

	/* Decode failed, exception already thrown */
    } else {
	void* imgHandle;
	imgHandle = dst->done(dst, &width, &height);

	if ((width < 0) || (height < 0)) {
	    /* Destroy native image data */
	    LCDUIdestroyNativeImage(imgHandle);

	    KNI_ThrowNew("java/lang/IllegalArgumentException", "");
	} else {
	    KNI_SetIntField(thisClass, 
			    _CACHE_FIELDID(clazz, "imgData", "I", _i_imgData_cache), 
			    (jint)imgHandle);
	    KNI_SetIntField(thisClass, 
			    _CACHE_FIELDID(clazz, "height", "I", _i_height_cache), 
			    (jint)height);
	    KNI_SetIntField(thisClass, 
			    _CACHE_FIELDID(clazz, "width", "I", _i_width_cache), 
			    (jint)width);
	    KNI_registerCleanup(thisClass, destroyImageData);
	}
    }
    
    if (dst != NULL) {
	midpFree(dst);
    }

    if (src != NULL) {
	src->done(src);
	midpFree(src);
    }

}

/*=========================================================================
 * FUNCTION:      finalize()V
 * CLASS:         javax.microedition.lcdui.ImmutableImage
 * TYPE:          virtual native function
 * OVERVIEW:      Cleanup any native resources before Image is GC'd
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImmutableImage_finalize() {

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisClass);
    KNI_GetThisPointer(thisClass);

    destroyImageData(thisClass);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      finalize()V
 * CLASS:         javax.microedition.lcdui.MutableImage
 * TYPE:          virtual native function
 * OVERVIEW:      Cleanup any native resources before Image is GC'd
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_MutableImage_finalize() {

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisClass);
    KNI_GetThisPointer(thisClass);

    destroyImageData(thisClass);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      decodeRGBImage([IIIZ)V
 * CLASS:         javax.microedition.lcdui.ImmutableImage
 * TYPE:          virtual native function
 * OVERVIEW:      Decode an Image from an array of ARGB integers
 * INTERFACE      (operand stack manipulation):
 *   parameters:  imageData    array containing image data
 *                width        the width of the new image
 *                height       the height of the new image
 *                processAlpha use/ignore alpha blend bytes in
 *                             the inputData array
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImmutableImage_decodeRGBImage() {

    jboolean processAlpha = KNI_GetParameterAsBoolean(4);
    int height = KNI_GetParameterAsInt(3);
    int width = KNI_GetParameterAsInt(2);
    int *imageBuffer;
    int buflen;
    imageDstPtr dst;
    void * imgHandle;

    KNI_StartHandles(3);
    KNI_DeclareHandle(imageData);
    KNI_DeclareHandle(thisClass);
    KNI_DeclareHandle(clazz);
    
    KNI_GetParameterAsObject(1, imageData);
    KNI_GetParameterAsObject(0, thisClass);

    if (KNI_IsNullHandle(imageData)) {
	KNI_ThrowNew("java/lang/NullPointerException", "");
    } else if ((width < 0) || (height < 0)) {
	KNI_ThrowNew("java/lang/IllegalArgumentException", "");
    } else {
        buflen = KNI_GetArrayLength(imageData);
        if ((width * height) > buflen) {
	    KNI_ThrowNew("java/lang/ArrayIndexOutOfBoundsException", "");
        } else {
            imageBuffer = (int*) midpMalloc(sizeof(int) * buflen);
            if (imageBuffer == NULL){
	        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
            } else {
	        KNI_GetRawArrayRegion(imageData, 0, buflen * sizeof(int), 
			              (jbyte*)imageBuffer);
	        dst = LCDUIcreateImageDst(KNI_FALSE);
	        if (dst != NULL){
	            /* do the decode here! */ 
	            dst->setSize(dst, width, height);
	            dst->setARGBPixels(dst, &imageBuffer, buflen, width,
			               height, processAlpha);
	            imgHandle = dst->done(dst, &width, &height);

	            KNI_GetObjectClass(thisClass, clazz);
		    KNI_SetIntField(thisClass, 
                                    _CACHE_FIELDID(clazz, "imgData", "I", _i_imgData_cache), 
				    (jint)imgHandle);
		    KNI_SetIntField(thisClass, 
                                    _CACHE_FIELDID(clazz, "width", "I", _i_width_cache), 
				    (jint)width);
		    KNI_SetIntField(thisClass, 
                                    _CACHE_FIELDID(clazz, "height", "I", _i_height_cache), 
				    (jint)height);
	            KNI_registerCleanup(thisClass, destroyImageData);
	            midpFree(dst);
	        } else {
	            KNI_ThrowNew("java/lang/OutOfMemoryError", "");
	        }
	        midpFree(imageBuffer);
            }
        }
    }
    
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      grabFullScreen(Ljavax/microedition/lcdui/Displayable;Z)V
 * CLASS:         javax.microedition.lcdui.Displayable
 * TYPE:          virtual native function
 * OVERVIEW:      Grabs the entire display area for drawing,
 *                so that the Displayable is in full-screen mode.
 * INTERFACE (operand stack manipulation):
 *   parameters:  mode          if true we should grab all area available
 *                              if false, relinquish area to display 
 *                              status and commands
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_Displayable_grabFullScreen() {
    jboolean mode = KNI_GetParameterAsBoolean(1);

    LCDUISetEmulatorFullScreenMode(mode);
}

/*=========================================================================
 * FUNCTION:      setSuppressKeyEvents(Ljavax/microedition/lcdui/Canvas;Z)V
 * CLASS:         javax.microedition.lcdui.game.GameCanvas
 * TYPE:          virtual native function
 * OVERVIEW:      Sets a private field in a public class defined in a 
 *                 different package.
 * INTERFACE (operand stack manipulation):
 *   parameters:  c                  the object whose field we are to set
 *                suppressKeyEvents  value to set the private field to
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_game_GameCanvas_setSuppressKeyEvents() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(canvas);
    KNI_DeclareHandle(clazz);

    KNI_GetParameterAsObject(1, canvas);
    KNI_GetObjectClass(canvas, clazz);

    KNI_SetBooleanField(canvas, 
                        KNI_GetFieldID(clazz, "suppressKeyEvents", "Z"), 
                        KNI_GetParameterAsBoolean(2));

    KNI_EndHandles();
}

/**
 * Accepts image information and adjusts the values so that only those
 * parts of the image that are visible in the clip will be drawn
 *
 * returns NO_IMAGE       if the image is completely outside of the clip
 * returns COMPLETE_IMAGE if the image is completely inside of the clip
 * returns PARTIAL_IMAGE  if the image is partially inside of the clip
 */

jbyte
adjustToViewableRegion(jint *x_dest, jint *y_dest, jint *x_src, jint *y_src,
                       jint *width, jint *height,
                       jshort *clip, jint anchor, jint transform) {
 
    int ret = NO_IMAGE;

    /*
     * use these temportary variables so we aren't deferencing all the time
     */
    jint t_x_dest = *x_dest, 
         t_y_dest = *y_dest,
         t_x_src,  t_y_src,
         t_width,  t_height;

    /* original values */
    jint ox_src, oy_src;
    jint ox_dest, oy_dest;
    jint oWidth, oHeight;

    jshort cx = clip[0],
           cy = clip[1],
           cw = clip[2],
           ch = clip[3];

    if (transform & TRANSFORM_INVERTED_AXES) {
        t_width  = *height;
        t_height = *width;
        t_x_src  = *y_src;
        t_y_src  = *x_src;
        ox_src   = *y_src;
        oy_src   = *x_src;
        oWidth  = *height;
        oHeight  = *width;
    } else {
        t_width  = *width;
        t_height = *height;
        t_x_src  = *x_src;
        t_y_src  = *y_src;
        ox_src   = *x_src;
        oy_src   = *y_src;
        oWidth  = *width;
        oHeight  = *height;
    }

    ox_dest = *x_dest;
    oy_dest = *y_dest;

    switch (anchor & (LEFT | RIGHT | HCENTER)) {
        default:
        case LEFT:
            break;

        case RIGHT:
            t_x_dest -= t_width;
            break;

        case HCENTER:
            t_x_dest -= t_width >> 1;
            break;
    }

    switch (anchor & (TOP | BOTTOM | VCENTER)) {
        default:
        case TOP:
            break;

        case BOTTOM:
            t_y_dest -= t_height;
            break;

        case VCENTER:
            t_y_dest -= t_height >> 1;
            break;
    }

    /* image will be past the left/right side of the clip */
    if (((t_x_dest + t_width) < cx) || ((t_x_dest >= (cx + cw)))) {
        /* this will leave the parameters unchanged; that is ok. */
        return ret;
    }

    /* image will be past the top/bottom side of the clip */
    if (((t_y_dest + t_height) < cy) || ((t_y_dest >= (cy + ch)))) {
        /* this will leave the parameters unchanged; that is ok. */
        return ret;
    }

    ret = COMPLETE_IMAGE;

    /* left side of image is past the left side of clip */
    if (t_x_dest < cx) {
        t_x_src += cx - t_x_dest;
        t_width -= cx - t_x_dest;

        t_x_dest = cx;
        ret = PARTIAL_IMAGE;
    }

    /* top of image is past the top of clip */
    if (t_y_dest < cy) {
        t_y_src  += cy - t_y_dest;
        t_height -= cy - t_y_dest;

        t_y_dest = cy;
        ret = PARTIAL_IMAGE;
    }

    /* right side of image is past the right side of clip */
    if ((t_x_dest + t_width) > (cx + cw)) {
        t_width = (cx + cw) - t_x_dest;
        ret = PARTIAL_IMAGE;
    }

    /* bottom of image is past the bottom of clip */
    if ((t_y_dest + t_height) > (cy + ch)) {
        t_height = (cy + ch) - t_y_dest;
        ret = PARTIAL_IMAGE;
    }


    /* check to see if we have dimensions that do not 
     * represent an image.
     * if width == 0 or height == 0,
     * return NO_IMAGE.
     * x, y, width, height values are invalid and
     * should not be used.
     */
    if (t_width == 0 || t_height == 0) {
      return NO_IMAGE;
    }
  
    /* these conditions will adjust the visible rectangle to cover
     * the part of the image that will actually be drawn if
     * a transform has been applied
     *
     * if the transform including inverting the axes and flipping one of the 
     * axes then we need to reverse (not undo) the visible rectangle, so that
     * whatever was adjusted one way needs to be adjusted the other.
     * this is because there is an implicit axes flip when the axes
     * are inverted.
     */

#define ADJUST_X \
t_x_src = ox_src + ((ox_src + oWidth) - (t_x_src + t_width))

#define ADJUST_Y \
t_y_src = oy_src + ((oy_src + oHeight) - (t_y_src + t_height))

    if (transform & TRANSFORM_X_FLIP) {
        ADJUST_X;
    }

    if (transform & TRANSFORM_Y_FLIP) {
        ADJUST_Y;
    }

    if ( (transform == (TRANSFORM_INVERTED_AXES | TRANSFORM_X_FLIP))
        || (transform == (TRANSFORM_INVERTED_AXES | TRANSFORM_Y_FLIP))) {

            ADJUST_X;
            ADJUST_Y;
    } 

#undef ADJUST_X
#undef ADJUST_Y

    if (transform & TRANSFORM_INVERTED_AXES) {
        *width  = t_height;
        *height = t_width;
        *x_src  = t_y_src;
        *y_src  = t_x_src;
    } else {
        *width  = t_width;
        *height = t_height;
        *x_src  = t_x_src;
        *y_src  = t_y_src;
    }

    *x_dest = t_x_dest;
    *y_dest = t_y_dest;

#if 0
    printf("sxy(%ld %ld) osxy(%i %i) wh(%ld %ld) owh(%i %i) "
           "dxy(%ld %ld) odxy(%i %i) t(%ld)\n", 
           *x_src, *y_src, ox_src, oy_src, *width, *height, 
           oWidth, oHeight, *x_dest, *y_dest, ox_dest, oy_dest, transform);
#endif

    return ret;
}

