/*
 * @(#)midpEvents.c	1.18 02/09/16 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <kni.h>
#include <midpMalloc.h>
#include <defaultLCDUI.h>

/*=========================================================================
 * Declaration
 *=======================================================================*/
#ifdef INCLUDE_I18N
static STRING_INSTANCE instantiateStringFromUnicode(const unicode* str, 
                                                    int len);
#endif

/*=========================================================================
 * FUNCTION:      StoreMIDPEvent
 * TYPE:          local function
 * OVERVIEW:      Wait for a system event
 * INTERFACE:
 *   parameters:  native system event structure
 *=======================================================================*/

void 
StoreMIDPEvent(KVMEventType *evt) {
  /* copied from DefaultEventHandler.java */
  
  static int MIDP_KEY_EVENT     = 1;
  static int MIDP_PEN_EVENT     = 2;
  static int MIDP_COMMAND_EVENT = 3;
  static int MIDP_SYSTEM_EVENT  = 4;
  
  /* for jsr135's audio building block, -- by hsy */
  static int MM_EOM_EVENT       = 8;
  
  static int PRESSED            = 1;
  static int RELEASED           = 2;
  static int REPEATED           = 3;
  static int DRAGGED            = 3;
  static int TYPED              = 4;
#ifdef INCLUDE_I18N
  static int IME                = 5;
#endif

  switch (evt->type) {
  case penDownKVMEvent:
    StoreKVMEvent(MIDP_PEN_EVENT, 3, PRESSED, 
		  evt->screenX, evt->screenY);
    break;
    
  case penUpKVMEvent:
    StoreKVMEvent(MIDP_PEN_EVENT, 3, RELEASED, 
		  evt->screenX, evt->screenY);
    break;
    
  case penMoveKVMEvent:
    StoreKVMEvent(MIDP_PEN_EVENT, 3, DRAGGED, 
		  evt->screenX, evt->screenY);
    break;
    
  case keyDownKVMEvent:
    StoreKVMEvent(MIDP_KEY_EVENT, 2, PRESSED, evt->chr);
    break;
    
  case keyUpKVMEvent:
    StoreKVMEvent(MIDP_KEY_EVENT, 2, RELEASED, evt->chr);
    break;
    
  case keyRepeatKVMEvent:
    StoreKVMEvent(MIDP_KEY_EVENT, 2, REPEATED, evt->chr);
    break;
    
  case keyTypedKVMEvent:
    StoreKVMEvent(MIDP_KEY_EVENT, 2, TYPED, evt->chr);
    break;
    
#ifdef INCLUDE_I18N
  case imeKVMEvent: {
    StoreKVMEvent(MIDP_KEY_EVENT, 2, IME, 
		  instantiateStringFromUnicode(evt->str, evt->len));
    midpFree(evt->str);
    break;
  }
#endif

  case commandKVMEvent:
    StoreKVMEvent(MIDP_COMMAND_EVENT, 1, evt->chr);
    break;
    
  case mmEOMEvent:
    StoreKVMEvent(MM_EOM_EVENT, 2, (int)evt->screenX, evt->chr);
    break;

  case systemKVMEvent:
    StoreKVMEvent(MIDP_SYSTEM_EVENT, 1, evt->chr);
    break;
    
  default:; /* do nothing, but continue in loop */
  }
}

/*=========================================================================
 * Auxiliary Methods
 *=======================================================================*/

#ifdef INCLUDE_I18N

static STRING_INSTANCE
instantiateStringFromUnicode(const unicode* str, int len)
{
  STRING_INSTANCE newObject;
  SHORTARRAY      newArray;
  int             size, i;
  
    if (len == 0) {
        return NULL;
    }

    size = (len * sizeof(short) + CELL - 1) >> log2CELL;

    START_TEMPORARY_ROOTS

    IS_TEMPORARY_ROOT(newObject, (STRING_INSTANCE)callocObject(SIZEOF_STRING_INSTANCE, GCT_INSTANCE));

    /* Since we don't allocate any more memory after newArray, we really
     * don't need to protect it. However, it doesn't cost much just in
     * case.
     */  
    IS_TEMPORARY_ROOT(newArray, (SHORTARRAY)callocObject(SIZEOF_ARRAY(size), GCT_ARRAY));

    newObject->ofClass = JavaLangString;

    newArray->ofClass = PrimitiveArrayClasses[T_CHAR];
    newArray->length  = len;
    for (i = 0; i < len && str[i] != 0; i++) {
        newArray->sdata[i] = str[i];
    }
    newObject->array = newArray;
    newObject->offset = 0;
    newObject->length = i;

    END_TEMPORARY_ROOTS

    return newObject;
}

#endif

/*=========================================================================
 * Native methods implementation for com.sun.midp.lcdui.Events class
 *
 * These functions call the native methods of 
 * com.sun.cldc.io.j2me.events.PrivateInputStream because the needed 
 * function calls and variables in events.c are static. To avoid modifying
 * the CLDC files, we just "wrapped" our implemention around those native
 * functions.
 *=======================================================================*/

void Java_com_sun_midp_lcdui_Events_open(void) {
    Java_com_sun_cldc_io_j2me_events_PrivateInputStream_open();
}

void Java_com_sun_midp_lcdui_Events_readInt(void) {
    Java_com_sun_cldc_io_j2me_events_PrivateInputStream_readInt();
}

void Java_com_sun_midp_lcdui_Events_readUTF(void) {
    Java_com_sun_cldc_io_j2me_events_PrivateInputStream_readUTF();
}

