/*
 * @(#)mmatone.c	1.34 02/08/19 @(#)
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <midpMalloc.h>
#include <stdio.h>

#include <windows.h>
#include <mmsystem.h>

#define MAXCHNL 4
#define TIMERES 10
/* #define EOM_EVT 8*/

#define PAUSE 1
#define RESUME 2
#define CLOSE 5
#define SET_VOLUME 7

#define GET_CUR_TIME    15
#define SET_CUR_TIME    16

#define SILENCE -1
#define INSEQ_SET_VOLUME -8

typedef struct {
    int    *toneseq;
    int    tonelen;
    int    curTone;
    int    curDur;
    int    curMTime;
    int    playerID;
    UINT   timerID;
    char   chnl;
    char   play;
    char   volume;
    char   firsttime;
    char   gain;
    char   padding;
} TONESEQ;

typedef struct {
    DWORD msg;
    UINT timerID;
}TONEDATA;

//TODO: when to delete critical section???
CRITICAL_SECTION CS;
static char csInited = 0;

static HMIDIOUT midiOut = 0;
char  midiChnls[MAXCHNL] = {0,0,0,0};
TONEDATA tones[MAXCHNL];

void injectNativeEvent(int pID, int curMTime);

/*==========================
 * time monitor callback
 *==========================*/
/*
void CALLBACK timeMonitor(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) {
    int i;
    for (i = 0; i < 8; i++)
	if ( midiChnls[i] == 1 ) {
	      return;
	}
    idle++;
    if ( idle >= 3 ) {
	midiOutClose(midiOut);
	midiOut = 0;
	timeKillEvent(monitorTimer);
	monitorTimer = idle = 0;
	//idle = 0;
    }
}
*/
/*==========================
 * utility functions
 *===========================*/
int getMidiChnl() {
  int i , j ;
  MMRESULT result;
  
  EnterCriticalSection(&CS);
  if ( midiOut == 0 ) { // first time
      result = midiOutOpen(&midiOut, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
      
      if ( result != MMSYSERR_NOERROR) {
	  LeaveCriticalSection(&CS);
	  return (-1);
      }
      memset(midiChnls, 0, MAXCHNL);
  }
  
  j = -1;
  for ( i = 0; i < MAXCHNL; i++) {
      if ( midiChnls[i] == 0 ) {
	  midiChnls[i] = 1;
	  j = i;
	  midiOutShortMsg(midiOut, (0x00004bc << 4) | (j & 0xf));
	  break;
      }
  }
  LeaveCriticalSection(&CS);
  return (j);
}

void freeMidiChnl(int idx) {
    int i; 
    
    EnterCriticalSection(&CS);
    midiChnls[idx] = 0;
    for ( i = 0; i < MAXCHNL; i++) {
	if ( midiChnls[i] == 1 ) {
	    LeaveCriticalSection(&CS);
	    return;
	}
    }
    
    midiOutClose(midiOut);
    midiOut = 0;
    LeaveCriticalSection(&CS);
    
}

/*
  NOTE: according to msdn spec, the system-defined functions should not be
  called in timer callback function, except for a list of functions it
  allowed. Unfortunately, midiOutClose are not on that list,
  so there is a risk here.
  
  However, on win2k, it seems that the timer is implemented as a native thread.
  (on windows 3.1, it might be a software interrupt). We assume
  midiOutClose() won't cause the problem. We did a stress testing, it
  can run overnight ( >= 12 hours) without any problem. So for now we leave
  the implementation  "as-is".
*/
void CALLBACK timeToneProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) {
    if ( tones[dwUser].timerID = uID) {
	midiOutShortMsg(midiOut, tones[dwUser].msg);
	timeKillEvent(tones[dwUser].timerID);
	freeMidiChnl(dwUser);
    }
}

void CALLBACK timeTSProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) {
    TONESEQ *ts = (TONESEQ *)dwUser;
    unsigned long msg = 0;
    char found = 0;
    
    if ( uID == ts->timerID) {
	if ( ts->play == 1 ) { 
	    if (ts->curDur > 0 ) {
		ts->curDur -= TIMERES;
		ts->curMTime += TIMERES;
		return;
	    }
	    
	    // note off currrent tone
	    if (ts->curTone < ts->tonelen && ts->curTone >= 0) {
		msg = ts->toneseq[ts->curTone];
		if (msg != 0)
		    midiOutShortMsg(midiOut, msg);
	    }

	    // find next tone
	    found = 0;
	    while (found == 0) {
		ts->curTone += 2;
		if (ts->curTone >= ts->tonelen) { //EOM
		    ts->play = 0;
		    ts->curDur = 0;
		    /* Since KVM's event queue is not MT-safe, we can not
		       directly add the event to it. To work around, post
		       a message to windows message queue.
		       StoreKVMEvent(EOM_EVT, 2, (int)(ts->playerID), ts->curMTime);*/
		    injectNativeEvent((int)(ts->playerID), ts->curMTime);
		    return;
		}
		
		if (ts->toneseq[ts->curTone] != 0) {
		    int tmp;
		    found = 1;
		    tmp = (ts->volume & 0x7f) * (ts->gain & 0x7f) / 100;
		    msg = ((tmp & 0xff) << 16) | ts->toneseq[ts->curTone];
		    ts->curDur = ts->toneseq[ts->curTone+1];
		    midiOutShortMsg(midiOut, msg);
		} else { /* in seq volume change event */ 
		    ts->gain = (char)(ts->toneseq[ts->curTone+1] & 0x7f);
		}
	    } // while (!found)
	} // ts->play is 1
    }
}


/*=========================================================================
 * FUNCTION:      nPlayTone(III)I
 * CLASS:         javax.microedition.media.Manager
 * TYPE:          static virtual native function
 * OVERVIEW:      play a single tone
 * INTERFACE (operand stack manipulation):
 *   parameters:  note     the note to be played
 *                duration the duration of the note
 *                volume   the volume
 *   returns:     status. 1 succeeded; <= 0 failed.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_Manager_nPlayTone(KNITRAPS) {
    int vol, dur, note;
    
    UINT timerID;
    DWORD msg = 0;
    TONEDATA *tone;
    int chnl;

    note = (int)KNI_GetParameterAsInt(1);
    dur = (int)KNI_GetParameterAsInt(2);
    vol = (int)KNI_GetParameterAsInt(3);

    if ( vol < 0 ) {
      vol = 0;
    } else if ( vol > 100) {
      vol = 127;
    } else {
      vol = (vol * 127)/100;
    }
      

    if (vol == 0)
      KNI_ReturnInt(1);


    if ( csInited == 0 ) {
      InitializeCriticalSection(&CS);
      csInited = 1;
    }
    
    chnl = getMidiChnl();
    if ( chnl == -1 ) {
      KNI_ReturnInt(0);
    }
	
    tones[chnl].msg = ((note&0xff) << 8) | 0x00000090 | (chnl & 0xf);
    msg = ((vol&0xff) << 16) | ((note&0xff) << 8) | 0x90 | (chnl & 0xf);
    midiOutShortMsg(midiOut, msg);
    
    timerID = timeSetEvent(dur, TIMERES, (LPTIMECALLBACK)timeToneProc, (DWORD)chnl, TIME_ONESHOT);
    tones[chnl].timerID = timerID;


    KNI_ReturnInt(1);
}

/*=========================================================================
 * FUNCTION:      toneInit(I)I
 * CLASS:         com.sun.mmedia.TonePlayer
 * TYPE:          virtual native function
 * OVERVIEW:      initialization for tone seq playback
 * INTERFACE (operand stack manipulation):
 *   parameters:  playerID  the global playerID for this tone player
 *   returns:     the pointer to the toneseq data structure; <= 0 failed.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_TonePlayer_toneInit(KNITRAPS) {
    int playerID = (int)KNI_GetParameterAsInt(1);
    TONESEQ *ts;
    int chnl;
    MMRESULT result;
    
    if ( csInited == 0 ) {
	InitializeCriticalSection(&CS);
	csInited = 1;
    }
    
    chnl = getMidiChnl();
    if (chnl == -1) {
      KNI_ReturnInt(0);
    }
    
    ts = (TONESEQ *)midpMalloc(sizeof(TONESEQ));
    
    memset(ts, 0, sizeof(TONESEQ));
    ts->firsttime = 1;
    ts->chnl = (char)(chnl & 0xf);
    ts->volume = 0x7f;
    ts->playerID = playerID;
    ts->curTone = -2;
    
    KNI_ReturnInt((jint)ts);
}


/*=========================================================================
 * FUNCTION:      toneSetSeq(I[I)V
 * CLASS:         com.sun.mmedia.TonePlayer
 * TYPE:          virtual native function
 * OVERVIEW:      pass the tone sequence from java to native
 * INTERFACE (operand stack manipulation):
 *   parameters:  ad       the pointer to the toneseq data structure
 *                toneseq  the int array that holds the tone sequence
 *   returns:     nothing
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_TonePlayer_toneSetSeq(KNITRAPS){
    int pts = KNI_GetParameterAsInt(1);
    int p, i;
    int tmp;

    TONESEQ *ts = (TONESEQ *)(pts);

    if ( ts == NULL) {
	KNI_ReturnVoid();
    }

    if ( ts->toneseq != NULL)
	midpFree(ts->toneseq);

    KNI_StartHandles(1);
    KNI_DeclareHandle(arrHandle);
    KNI_GetParameterAsObject(2, arrHandle);

    ts->tonelen = KNI_GetArrayLength(arrHandle);
    ts->toneseq = (int *)midpMalloc(ts->tonelen * sizeof(int));
    for ( i = 0; i < ts->tonelen/2; i++) {
	p = i << 1;
	tmp = KNI_GetIntArrayElement(arrHandle, p);
	if (tmp == INSEQ_SET_VOLUME) {
	    // invalid midi msg to denote the in seq vol change
	    ts->toneseq[p] = 0; 
	} else {
	    if (tmp == SILENCE)
		tmp = 0;
	    
	    // note-off midi msg
	    ts->toneseq[p] = ((tmp & 0xff) << 8) | 0x90 | (ts->chnl & 0xf);
	}
	ts->toneseq[p+1] = KNI_GetIntArrayElement(arrHandle, p+1);
	    
    }

    ts->curTone = -2;
    ts->curDur = 0;
    ts->curMTime = 0;
    ts->play = 0;
    ts->gain = 100;
    
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      toneCommon(III)I
 * CLASS:         com.sun.mmedia.TonePlayer
 * TYPE:          virtual native function
 * OVERVIEW:      common utility functions for tone player
 * INTERFACE (operand stack manipulation):
 *   parameters:  ad    the pointer to the toneseq data structure
 *                code  opcode for a particular tone operation
 *                param user defined  parameter
 *   returns:     status. <= 0 failed.
 *=======================================================================*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_TonePlayer_toneCommon(KNITRAPS){
  int param = KNI_GetParameterAsInt(3);
  int code = KNI_GetParameterAsInt(2);
  int pts = KNI_GetParameterAsInt(1);
  
  TONESEQ *ts = (TONESEQ *)(pts);
  int ret = 1;
  
  if ( ts == NULL ) {
    KNI_ReturnInt(0);
  }
  
  switch (code) {
  case RESUME:
    if ( ts->firsttime == 1 ) {
      UINT timerID;
      ts->firsttime = 0;
      
      // start the timer
      timerID = timeSetEvent(TIMERES, TIMERES, (LPTIMECALLBACK)timeTSProc, (DWORD)ts, TIME_PERIODIC);
      ts->timerID = timerID;
    }
    
    ts->play = 1;
    break;
    
  case PAUSE:
    {
      unsigned long msg;
      
      // TODO: handle it more accurate without affect curMTime??
      ts->play = 0;
      if (ts->curTone < ts->tonelen && ts->curTone >= 0 ) {
	msg = ts->toneseq[ts->curTone];
	midiOutShortMsg(midiOut, msg);
	ts->curDur = 0;
      }
      
      Sleep((int)(TIMERES*15/10));
      
      if (ts->curTone < ts->tonelen && ts->curTone >= 0 ) {
	msg = ts->toneseq[ts->curTone];
	midiOutShortMsg(midiOut, msg);
	ts->curDur = 0;
      }
      
    }
    break;
    
  case SET_VOLUME:
    param = (param * 127)/100;
    ts->volume = param & 0x7f;
    break;
    
    
  case CLOSE:
    timeKillEvent(ts->timerID);
    freeMidiChnl((int)(ts->chnl & 0xf));
    if (ts->toneseq != NULL) {
      midpFree(ts->toneseq);
      ts->toneseq = NULL;
    }
    midpFree(ts);
    break;
    
  case SET_CUR_TIME:
    {
      int i, mtime=0;
      ts->gain = 100;
      
      for (i = 0; i < ts->tonelen; i += 2) {
	if (param <= mtime) {
	  break;
	}

	if (ts->toneseq[i] != 0) {
	  mtime += ts->toneseq[i+1];
	} else { /* in seq volume change event */
	  ts->gain = (char)(ts->toneseq[i+1] & 0x7f);
	}
      }

      ts->curTone = i-2;
      ts->curDur = 0;
      ts->curMTime = mtime;
      ret = mtime;
    }

    break;
    
  case GET_CUR_TIME:
    ret = ts->curMTime;
    break;
  }
    
  KNI_ReturnInt((jint)ret);
}
