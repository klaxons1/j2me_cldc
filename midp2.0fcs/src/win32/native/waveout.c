/*
 * @(#)waveout.c	1.21 02/08/05 @(#)
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <midpMalloc.h>
#include <stdio.h>

#include <windows.h>
#include <mmsystem.h>
#include "audiornd.h"

#define NCHUNKS 6

typedef struct {
    HWAVEOUT handle;
    char **data;
    int  *size;
    WAVEHDR **headers;
    int count;
    int nchunks;
    int nbuflen;  
} AUDIOINST;


/*===============================
 * Utility functions
 *==============================
 */

int audioOpen(int sampleRate,
	      int bitsPerSample,
	      int channels,
	      int isSigned,
	      int endianess) {
  
  HWAVEOUT waveOut;
  WAVEFORMATEX format;
  int chunkIndex, k;
  MMRESULT result;
  AUDIOINST *instance;
  
  format.cbSize = sizeof(WAVEFORMATEX);
  format.nSamplesPerSec = sampleRate;
  format.wBitsPerSample = bitsPerSample;
  format.nChannels =  channels;
  format.nBlockAlign = format.nChannels * format.wBitsPerSample/8;
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
  
  result = waveOutOpen(&waveOut, WAVE_MAPPER, 
		       &format, 0 /* Callback */,
		       0, CALLBACK_NULL);
  
  chunkIndex = 0;

  if ((result == MMSYSERR_NOERROR) && 
      (instance = (AUDIOINST*)midpMalloc(sizeof(AUDIOINST)))) {

    instance->handle = waveOut;
    instance->nchunks = NCHUNKS;
    instance->nbuflen = (sampleRate * channels * bitsPerSample / 8 / 32) & ~3;

    instance->data = (char **)midpMalloc(sizeof(char *) * NCHUNKS);
    instance->size = (int *)midpMalloc(sizeof(int) * NCHUNKS);
    instance->headers = (WAVEHDR **)midpMalloc(sizeof(WAVEHDR *) * NCHUNKS);

    if ((instance->data    != NULL) && 
        (instance->size    != NULL) &&
        (instance->headers != NULL)) {

      for (chunkIndex = 0; chunkIndex < NCHUNKS; chunkIndex++) {
        instance->data[chunkIndex] = (char*)midpMalloc(instance->nbuflen);
        instance->size[chunkIndex] = instance->nbuflen;
        instance->headers[chunkIndex] = (WAVEHDR*)midpMalloc(sizeof(WAVEHDR));

        if ((instance->data[chunkIndex]     == NULL) || 
            (instance->headers[chunkIndex]) == NULL) {
            break;
        }

        instance->headers[chunkIndex]->dwBufferLength = 0;
      }

      if (chunkIndex == NCHUNKS) {
        instance->count = 0;
        return (int)instance;
      }
    }
  } 

  if (instance) {

    if (instance->data) {
      for (k = 0; k < chunkIndex; k++) {
        midpFree(instance->data[k]);
      }
      midpFree(instance->data);
    }

    midpFree(instance->size);
 
    if (instance->headers) {
      for (k = 0; k < chunkIndex; k++) {
        midpFree(instance->headers[k]);
      }
      midpFree(instance->headers);
    }
      
    midpFree(instance);
  }

  return 0;

}

char * getNextAvailBuf(int adptr, int len) {
  int count;
  MMRESULT result;
  AUDIOINST *instance = (AUDIOINST *)adptr;
  
  count = instance->count;
  if (instance->headers[count]->dwBufferLength > 0) {
    if ((instance->headers[count]->dwFlags & WHDR_DONE) == 0){
      return (NULL);
    }
    waveOutUnprepareHeader(instance->handle,
			   instance->headers[count],
			   sizeof(WAVEHDR));
  }
  
  memset(instance->headers[count], 0, sizeof(WAVEHDR));
  instance->headers[count]->dwBufferLength = len;
  if (instance->size[count] < len) {
    midpFree(instance->data[count]);
    instance->data[count] = (char *)midpMalloc(len);

    if (instance->data[count] == NULL) {
        instance->size[count] = 0;
        return NULL;
    }

    instance->size[count] = len;
    }
  
  return (instance->data[count]);
}

void audioWrite(int adptr, int offset, int len) {
  int count;
  MMRESULT result;
  AUDIOINST *instance = (AUDIOINST *)adptr;

  count = instance->count;
  instance->headers[count]->lpData = instance->data[count];
  waveOutPrepareHeader(instance->handle, 
		       instance->headers[count], 
		       sizeof(WAVEHDR));
  result = waveOutWrite(instance->handle,
			instance->headers[count], 
			sizeof(WAVEHDR));
  instance->count = (count + 1) % instance->nchunks;
  return;
}

int audioCommon(int adptr, int code, int param) {    
    int ret = 1;
    AUDIOINST * waveOut = (AUDIOINST*)adptr;
    if (waveOut == NULL ) {
      if (code == DRAIN)
	return(1);
      else
	return(0);
    }
    
    switch (code) {
    case PAUSE:
	waveOutPause(waveOut->handle);
	break;
    case RESUME:
	waveOutRestart(waveOut->handle);
	break;
    case FLUSH:
	waveOutReset(waveOut->handle);
	break;
    case DRAIN:
      { /* return 1: drained; 0: not complete draining */
	int i;

	for (i = 0; i < waveOut->nchunks; i++) {
	  if (waveOut->headers[i]->dwBufferLength > 0) {
	    if ((waveOut->headers[i]->dwFlags & WHDR_DONE) == 0)
	      return (0);
	    waveOutUnprepareHeader(waveOut->handle,
				   waveOut->headers[i],
				   sizeof(WAVEHDR));
	    waveOut->headers[i]->dwBufferLength = 0;
	  }
	}
  
	return (1);
      }

    case CLOSE:
      {
	int i;

	waveOutReset(waveOut->handle);
	waveOutClose(waveOut->handle);
	for (i = 0; i < waveOut->nchunks; i++) {
	  midpFree(waveOut->data[i]);
	  midpFree(waveOut->headers[i]);
	}
	midpFree(waveOut->data);
	midpFree(waveOut->size);
	midpFree(waveOut->headers);
	midpFree(waveOut);
      }
      break;
    case SAMPLES_PLAYED:
	{
	    MMTIME mmtime;
	    
	    mmtime.wType = TIME_SAMPLES;
	    waveOutGetPosition(waveOut->handle, &mmtime, sizeof(MMTIME));
	    ret = mmtime.u.sample;
	}
	break;
    case SET_VOLUME:
      {
	DWORD toSet;
	// 0 <= param <= 100
	toSet = (DWORD)(655 * param);
	toSet = toSet | (toSet << 16);
	waveOutSetVolume(waveOut->handle, toSet);
      }
      break;
    case GET_VOLUME:
      {
	DWORD vol;
	waveOutGetVolume(waveOut->handle, (LPDWORD) &vol);
	
	ret = ((vol & 0xFFFF) + (vol >> 16)) / 2;
	ret /= 655;
	if (ret < 0) {
	  ret = 0;
	} else if (ret > 100) {
	  ret = 100;
	}
	break;
      }
      
    case GET_BUFLEN:
      ret = waveOut->nbuflen;
      break;
    }
  
    return(ret);
}
