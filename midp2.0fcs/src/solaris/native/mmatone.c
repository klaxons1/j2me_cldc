/*
 * @(#)mmatone.c	1.29 02/10/01 @(#)
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <midpMalloc.h>
#include <stdio.h>

#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "audiornd.h"

#define MAXCHNL 4
#define SILENCE -1
#define INSEQ_SET_VOLUME -8

#define MAXVAL  32767
#define SCALE   100

typedef struct {
    int adptr;
    int *sequences;
    int seqlen;
    int curNote;
    int sampleLeft;
    int vol;
    int curMTime;
    pthread_t tid;
    char pID;
    char closed;  
    char singleTone; /* 1: single tone; 2:tone sequence; 0 unused */
    char play;
    char prePlay;
    char gain;
}TONEDATA;
  
TONEDATA tonepool[MAXCHNL];
static char initTonePool = 1;
pthread_mutex_t theMutex;

/* ======= For tones */
#define SAMPLE_LENGTH 125 /* tone data is in format of 8k/16bit/mono/signed  */
int  K[128] = {122309, 115444, 108965, 102849,
               97077, 91628, 86486, 81632, 
               77050, 72725, 68644, 64791, 
               61154, 57722, 54482, 51425, 
               48538, 45814, 43243, 40816, 
               38525, 36363, 34322, 32395, 
               30577, 28861, 27241, 25712, 
               24269, 22907, 21621, 20408, 
               19262, 18181, 17161, 16198, 
               15289, 14430, 13621, 12856, 
               12135, 11453, 10811, 10204, 
               9631, 9091, 8580, 8099, 
               7644, 7215, 6810, 6428, 
               6067, 5727, 5405, 5102, 
               4816, 4545, 4290, 4049, 
               3822, 3608, 3405, 3214, 
               3034, 2863, 2703, 2551, 
               2408, 2273, 2145, 2025, 
               1911, 1804, 1703, 1607, 
               1517, 1432, 1351, 1275, 
               1204, 1136, 1073, 1012, 
               956, 902, 851, 804, 
               758, 716, 676, 638, 
               602, 568, 536, 506, 
               478, 451, 426, 402, 
               379, 358, 338, 319, 
               301, 284, 268, 253, 
               239, 225, 213, 201, 
               190, 179, 169, 159, 
               150, 142, 134, 127, 
               119, 113, 106, 100, 
               95, 89, 84, 80};


void injectNativeEvent(int playerID, int curMTime);

/*===============================
 * Utility functions
 *===============================
 */
/*
 * This function is always called from midp main native thread
 */
TONEDATA* getToneChnl() {
  int i;
  TONEDATA *ret = NULL;
  if (initTonePool == 1) {
    initTonePool = 0;
    memset(tonepool, 0, MAXCHNL*sizeof(TONEDATA));
    pthread_mutex_init(&theMutex, NULL);
  }
  
  pthread_mutex_lock(&theMutex);
  for (i = 0; i < MAXCHNL; i++) {
    if (tonepool[i].singleTone == 0) {
      /* for single tone, sequences is not freed when it completes, 
	 so need to free here */
      if (tonepool[i].sequences != NULL){
	midpFree(tonepool[i].sequences);
      }
      memset(&tonepool[i], 0, sizeof(TONEDATA));
      ret = &tonepool[i];
      break;
    }
  }
  
  pthread_mutex_unlock(&theMutex);
  return (ret);
}

void tonegen(char *data, int len, TONEDATA *td) {
  int toWrite, written;
  int note, durInSamples, phase1, phase2;
  int tonedata;
  int amplitude = td->vol * (td->gain & 0x7f) / 100;

  durInSamples = td->sequences[td->curNote+1];
  written = durInSamples - td->sampleLeft;
  toWrite = len >> 1;
  if (toWrite > td->sampleLeft)
    toWrite = td->sampleLeft;
  
  note = td->sequences[td->curNote];
  if (note == SILENCE || amplitude == 0) {
    memset(data, 0, toWrite << 1);
  } else {
    long x, t, slope, tmp;
    int KK, hKK, qKK;

    KK = K[note];
    hKK = KK >> 1;
    qKK = KK >> 2;

    phase1 = durInSamples / 10;  /* 10% attack */
    phase2 = durInSamples * 3 / 4; /* 25% decay */

    slope = (amplitude) * SCALE * 4 / KK;
    for (x = written; x < (written + toWrite); x++) {
      t = x * SAMPLE_LENGTH;
      
      /* map to within k */
      t = t % KK ;
      
      /* triangle wave: 4 discrete fcn's based on phase */
      if (t <= qKK) {
	tonedata = (int)(slope * t / SCALE);
      } else if (t <= hKK) {
	tonedata = (int)((hKK-t) * slope / SCALE);
      } else if (t <= 3*qKK) {
	tonedata = (int)(-1 * (t-hKK) * slope / SCALE);
      } else {
	tonedata = (int)(-1 * (KK-t) * slope / SCALE);
      }

      if (tonedata > MAXVAL) {
	  tonedata = MAXVAL;
      }

      if (tonedata < -1 * MAXVAL) {
	  tonedata = -1 * MAXVAL;
      }

      /* attack & decay */
      if (x < phase1) { 
	tonedata = tonedata * x / phase1;
      } else if (x > phase2) { 
	tonedata = tonedata * (durInSamples - 1 - x) / (durInSamples - phase2);
      }

#ifdef BIG_ENDIAN
      data[2*(x-written)] = (char)((tonedata >> 8) & 0xff);
      data[2*(x-written)+1] = (char)(tonedata & 0xff);
#else  /* LITTLE_ENDIAN */
      data[2*(x-written)] = (char)(tonedata & 0xff);
      data[2*(x-written)+1] = (char)((tonedata >> 8) & 0xff);
#endif
    }
    
  } /* else */
  
  audioWrite(td->adptr, 0, toWrite << 1);
  td->sampleLeft -= toWrite;
}

/*
 * this is the main function for tone/tone seq generation.
 * It runs on a separate native thread.
 */
void * tonemain(void *arg) {
    TONEDATA *td = (TONEDATA *)arg;
    struct timespec interval;
    char *data;
    int len = 8000 * 16 * 1 / 8 / 32 & ~3; 
    
    interval.tv_sec = 0;
    interval.tv_nsec = 16000000;

    while (1) {
	if (td->closed == 1)
	    break;

	if (td->play == 0) {
	    if (td->prePlay == 1)
		audioCommon(td->adptr, PAUSE, 0);
	    td->prePlay = td->play;
	    nanosleep((const struct timespec *)&interval, NULL);
	    continue;
	}

	if (td->prePlay == 0) {
	    td->prePlay = 1;
	    audioCommon(td->adptr, RESUME, 0);
	}

	if (td->sampleLeft > 0) {
	    data = getNextAvailBuf(td->adptr, len);
	    if (data == NULL) {
		nanosleep((const struct timespec *)&interval, NULL);
		continue;
	    }
	    
	    tonegen(data, len, td);
	} else {
	    if (td->singleTone == 1)
		break;
	    td->curNote += 2;

	    if (td->curNote < td->seqlen) {
		if (td->sequences[td->curNote] == INSEQ_SET_VOLUME) {
		    td->gain = (char)(td->sequences[td->curNote+1] & 0x7f);
		    continue;
		}
		
		if (td->curNote > 0)
		    td->curMTime += td->sequences[td->curNote-1] >> 3;
		
		td->sampleLeft = td->sequences[td->curNote+1];
	    } else { /* EOM */
		if (td->curNote > 0)
		    td->curMTime += td->sequences[td->curNote-1] >> 3;
		
		td->play = 0;
		td->gain = 100;
		while (audioCommon(td->adptr, DRAIN, 0) == 0)
		    nanosleep((const struct timespec *)&interval, NULL);
		injectNativeEvent((int)(td->pID), (int)(td->curMTime));
	    }
	}
    }

    if (td->singleTone == 1) {
#ifdef SOLARIS
	/**
	 * On Solaris, drain has to be done explicitly.
	 * On Linux, close imply a drain.
	 */
	while (audioCommon(td->adptr, DRAIN, 0) == 0)
	    nanosleep((const struct timespec *)&interval, NULL);
#endif
	audioCommon(td->adptr, CLOSE, 0);
	/* release this slot in tonepool */
	pthread_mutex_lock(&theMutex);
	td->singleTone = 0;
	pthread_mutex_unlock(&theMutex);
    } 
    
    return NULL;
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
  TONEDATA *td;
  int note, dur, vol;
  
  note = (int)KNI_GetParameterAsInt(1);
  dur = (int)KNI_GetParameterAsInt(2);
  vol = (int)KNI_GetParameterAsInt(3);

  td = getToneChnl();
  if (td == NULL) {
      KNI_ReturnInt(0);
  }

  if (vol < 0) {
    vol = 0;
  } else if (vol > 100) {
    vol = MAXVAL;
  } else {
    vol = (vol * MAXVAL)/100;
  }

  if (vol == 0)
    KNI_ReturnInt(1);

  dur <<= 3;
  
#ifdef BIG_ENDIAN
  td->adptr = audioOpen(8000, 16, 1, MMAFMT_SIGNED, MMAFMT_BIG);
#else /* LITTLE_ENDIAN */
  td->adptr = audioOpen(8000, 16, 1, MMAFMT_SIGNED, MMAFMT_LITTLE);
#endif

  if (td->adptr != 0) {
    int reterr;
    
    td->sequences = (int *)midpMalloc(2*sizeof(int));
    td->seqlen = 2;
    td->sequences[0] = note;
    td->sequences[1] = dur;
    td->sampleLeft = dur;
    td->vol = vol;
    td->gain = 100;
    td->singleTone = 1;
    td->play = td->prePlay = 1;

    reterr = pthread_create(&(td->tid), NULL, tonemain, (void *)td);
    if (reterr != 0 ) {
      printf("error in creating new thread %d \n", reterr);
    } 

    KNI_ReturnInt(1);
  } 

  td->singleTone = 0;
  KNI_ReturnInt(0);
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
    
    TONEDATA *ts;

    ts = getToneChnl();
    if (ts == NULL)
	KNI_ReturnInt(0);

#ifdef BIG_ENDIAN
    ts->adptr = audioOpen(8000, 16, 1, MMAFMT_SIGNED, MMAFMT_BIG);
#else /* LITTLE_ENDIAN */
    ts->adptr = audioOpen(8000, 16, 1, MMAFMT_SIGNED, MMAFMT_LITTLE);
#endif
    if (ts->adptr != 0) {
	int reterr;

	ts->pID = (char)(playerID & 0x7f);
	ts->singleTone = 2;
	ts->vol = MAXVAL;
	ts->gain = 100;

	reterr = pthread_create(&(ts->tid), NULL, tonemain, (void *)ts);    
	if (reterr == 0 ) {
	    KNI_ReturnInt((jint)ts);
	} else {
	    printf("failed to create new thread \n");
	}
    }
    
    ts->singleTone = 0;
    KNI_ReturnInt(0);
}


/*========================================================================
 * FUNCTION:      toneSetSeq(I[I)V
 * CLASS:         com.sun.mmedia.TonePlayer
 * TYPE:          virtual native function
 * OVERVIEW:      pass the tone sequence from java to native
 * INTERFACE (operand stack manipulation):
 *   parameters:  ad       the pointer to the toneseq data structure
 *                toneseq  the int array that holds the tone sequence
 *   returns:     nothing
 *======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_TonePlayer_toneSetSeq(KNITRAPS){
    int pts = KNI_GetParameterAsInt(1);
    int p, i;
    
    TONEDATA *ts = (TONEDATA *)(pts);

    if ( ts == NULL) {
	KNI_ReturnVoid();
    }

    if ( ts->sequences != NULL) {
	midpFree(ts->sequences);
	ts->sequences = NULL;
    }

    KNI_StartHandles(1);
    KNI_DeclareHandle(arrHandle);
    KNI_GetParameterAsObject(2, arrHandle);

    ts->seqlen = KNI_GetArrayLength(arrHandle);
    ts->sequences = (int *)midpMalloc(ts->seqlen * sizeof(int));
    for ( i = 0; i < ts->seqlen/2; i++) {
	p = i << 1;
	ts->sequences[p] = KNI_GetIntArrayElement(arrHandle, p);
	if (ts->sequences[p] == INSEQ_SET_VOLUME)
	    ts->sequences[p+1] = KNI_GetIntArrayElement(arrHandle, p+1);
	else
	    ts->sequences[p+1] = KNI_GetIntArrayElement(arrHandle, p+1) << 3;
    }

    ts->curNote = -2;
    ts->curMTime = 0;
    ts->play = ts->prePlay = 0;
    ts->sampleLeft = 0;

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
    
    TONEDATA *ts = (TONEDATA *)(pts);
    int ret = 1;
    
    if ( ts == NULL ) {
	KNI_ReturnInt(0);
    }
    
    switch (code) {
    case RESUME:
	ts->play = 1;
	break;
	
    case PAUSE:
      {
	struct timespec interval;
    
	ts->play = 0;

	/* sleep 40 ms to wait for that the flag play=0 takes effective in
	   the play thread to avoid potential race condition */
	interval.tv_sec = 0;
	interval.tv_nsec = 40000000;
	nanosleep((const struct timespec *)&interval, NULL);
      }
	break;

    case SET_VOLUME:
	param = (param * MAXVAL)/100;
	ts->vol = param;
	break;

    case CLOSE:
	ts->closed = 1;
	pthread_join(ts->tid, NULL); /* wait for the tone thread dead */

	audioCommon(ts->adptr, FLUSH, 0);
	audioCommon(ts->adptr, CLOSE, 0);
	
	/* here it always runs on midp main thread */
	if (ts->sequences != NULL) {
	    midpFree(ts->sequences);
	    ts->sequences = NULL;
	}
	ts->singleTone = 0;

	break;
	
    case SET_CUR_TIME:
      {
	int i, mtime=0;
	ts->gain = 100;
	for (i = 0; i < ts->seqlen; i += 2) {
	  if (param <= mtime)
	    break;
	  if (ts->sequences[i] != INSEQ_SET_VOLUME){
	    mtime += ts->sequences[i+1] >> 3;
	  } else {
	    ts->gain = (char)(ts->sequences[i+1] & 0x7f);
	  }
	}
	ts->curNote = i-2;
	ts->sampleLeft = 0;	
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
