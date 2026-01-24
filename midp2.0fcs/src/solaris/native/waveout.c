/*
 * @(#)waveout.c	1.24 02/11/05 @(#)
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <midpMalloc.h>
#include <stdio.h>

#include <sys/utsname.h>
#include <sys/fcntl.h>
#include <sys/stropts.h>

#include "audiornd.h"

#ifdef SOLARIS
    #include <sys/audioio.h>
    #define DEV_AUDIO "/dev/audio"
    #define DEV_AUDIOCTL "/dev/audioctl"
    /* hack solaris8 mixer function */
    #define MAXBUFSLOT 5
    #define MY_MIOC ('M'<<8)
    #define MY_AUDIO_MIXER_MULTIPLE_OPEN (MY_MIOC|10)
    typedef audio_info_t AudioInfo;
#else	/* LINUX */
    #include <linux/soundcard.h>
    #define MAXBUFSLOT 2
    #define DEV_AUDIO "/dev/dsp"
    #define DEV_MIXER "/dev/mixer"
#endif


typedef struct {
    int dev;
#ifdef LINUX
    int mixer;
    int cvrtStereo;
    int cvrt16;
    char *dbuf;
    int dbuflen;
    int channels;
    int rate;
    int lastSamples;
#endif
    int bits;
    int isSigned;
    int endianess;
    int nbuflen;
    int sampleSizeInBytes;
    int threshold;
    unsigned int writtenSamples;
    char *wbuf;
    int startdraining;
    int drainnum;
} AudioDevice;


#ifdef SOLARIS
/**
 * Format converter only needed for Solaris/Sparc.
 */
void fmtcvrt(AudioDevice *ad, int offset, int len) {
    int i;
    int tmpint;
    char tmpchar;

    if (ad->bits == 8) { /* 8bit  unsigned to 16bit signed big */
	int newlen;
	newlen = (offset+len) * 2;
	for (i = newlen/2 -1; i >= offset; i--) {
	    tmpint = (int)(ad->wbuf[i] & 0xff) - 128;
	    tmpint <<= 8;
	    ad->wbuf[2*i] = (char)((tmpint >> 8) & 0xff);
	    ad->wbuf[2*i+1] = (char)(tmpint & 0xff);
	}
	/* 8bit unsigned to 8bit signed
	 for (i = offset; i < offset+len; i++) {
	 tmpint = (int)(ad->wbuf[i] & 0xff) - 128;
	 ad->wbuf[i] = (char)(tmpint & 0xff);
	 }
	*/
    } else { /* little to big */
	for (i = offset; i < offset+len -1; i+=2) {
	    tmpchar = ad->wbuf[i];
	    ad->wbuf[i] = ad->wbuf[i+1];
	    ad->wbuf[i+1] = tmpchar;
	}
    }
}
#endif

#ifdef LINUX
/**
 * Format converter only needed for Linux.
 */
int fmtcvrt(AudioDevice *ad, int offset, int len) {
    int wlen = len;
    int i;

    if (ad->cvrtStereo)
	wlen *= 2;
    if (ad->cvrt16)
	wlen *= 2;

    if (wlen > ad->dbuflen) {
	if (ad->dbuf != NULL)
	    midpFree(ad->dbuf);
	ad->dbuf = (char *)midpMalloc(wlen);
	if (ad->dbuf == NULL) {
	    ad->dbuflen = 0;
	    return 0;
	}
	ad->dbuflen = wlen;
    }

    if (ad->cvrtStereo && ad->bits == 16) {
	// Convert 16 bit audio from mono to stereo.
	short *sp1 = (short *)ad->dbuf;
	short *sp2 = (short *)ad->wbuf;
	for (i = 0; i < len/2; i++) {
	    *(sp1 + i*2) = *(sp2 + offset + i);
	    *(sp1 + i*2+1) = *(sp1 + i*2);
	}
    } else if (ad->cvrtStereo) {
	if (ad->cvrt16) {
	    // Convert 8 bit unsigned mono to 16 bit signed stereo.
	    int tmpint;
	    for (i = 0; i < len; i++) {
		tmpint = (int)(ad->wbuf[i + offset] & 0xff) - 128;
		tmpint <<= 8;
		ad->dbuf[4*i] = (char)(tmpint & 0xff);
		ad->dbuf[4*i+1] = (char)((tmpint >> 8) & 0xff);
		ad->dbuf[4*i+2] = ad->dbuf[4*i];
		ad->dbuf[4*i+3] = ad->dbuf[4*i+1];
	    }
	} else {
	    // Convert 8 bit mono to 8 bit stereo.
	    for (i = 0; i < len; i++) {
		ad->dbuf[i*2] = *(ad->wbuf + offset + i);
		ad->dbuf[i*2 + 1] = ad->dbuf[i*2];
	    }
	}
    } else if (ad->cvrt16) {
	// Convert 8 bit unsigned to 16 signed audio.
	int tmpint;
	for (i = 0; i < len; i++) {
	    tmpint = (int)(ad->wbuf[i + offset] & 0xff) - 128;
	    tmpint <<= 8;
	    ad->dbuf[2*i] = (char)(tmpint & 0xff);
	    ad->dbuf[2*i+1] = (char)((tmpint >> 8) & 0xff);
	}
    }
    return wlen;
}
#endif


int audioOpen(int sRate, int bits, int channels, int isSigned, int endianess) {
    AudioDevice * ad = (AudioDevice *) midpMalloc(sizeof(AudioDevice));

#ifdef SOLARIS

    AudioInfo info;
    char *devName = (char *)getenv("AUDIODEV");
    struct utsname osDetails;
    char sol8 = 0;
    int ret = 0;

    if (ad == NULL) {
        return 0;
    }

    if (devName == NULL) {
	devName = DEV_AUDIO;
    }

    /* Open the device as non-blocking to check if the device is
       available.  If we open it as blocking device, the open call
       will block if the device is unavailable. */
    ad->dev = open(devName, O_WRONLY | O_NONBLOCK);
    
    if (ad->dev < 0) {
      midpFree(ad);
      return (0);
    }

    /* Close the device and re-open it as blocking. */
    close(ad->dev);
    ad->dev = open(devName, O_WRONLY);

    /* for solaris 8+ */
    uname(&osDetails);
    osDetails.release[3] = 0;
    if (osDetails.release[0] == '2' || osDetails.release[0] == '5') {
	if (osDetails.release[2] >= '8')
	   sol8 = 1;
	else
	   sol8 = 0;
    } else {
	sol8 = 1;
    }

    if (sol8 == 1) {
	if ( ioctl(ad->dev, MY_AUDIO_MIXER_MULTIPLE_OPEN) < 0 ) {
	    printf("can't set multiple open mode\n");
	}
   }

    /* 1/32 second */
    /* always open as 16bit */
    ad->sampleSizeInBytes = channels * 16 / 8; /* could be 1, 2, 4 */
    ad->nbuflen = (sRate * ad->sampleSizeInBytes / 32) & ~3; /* align 4n */
    ad->bits = bits;

    /* Set the output format */
    AUDIO_INITINFO(&info);
    info.play.encoding = AUDIO_ENCODING_LINEAR;
    info.play.sample_rate = sRate;
    info.play.channels = channels;
    info.play.precision = 16;
    info.play.buffer_size = ad->nbuflen;

    if ((ret = ioctl(ad->dev, AUDIO_SETINFO, &info)) < 0) {
	printf("failed to set format: %d, %d, %d\n", sRate, bits, channels);
	close(ad->dev);
	midpFree(ad);
	return(0);
    }

#else  /* LINUX */

    audio_buf_info info;

    ad->channels = channels;
    ad->bits = bits;
    ad->rate = sRate;
    ad->lastSamples = 0;
    ad->dbuf = NULL;
    ad->dbuflen = 0;
    ad->dev = 0;
    ad->mixer = 0;

    if (linuxAudioOpen(ad) == 0)
	return 0;

    if (ioctl(ad->dev, SNDCTL_DSP_GETOSPACE, &info) >= 0) {
	/* The OSS driver does not leave much flexibility for
	   applications to set the buffers.  You pretty much
	   have to take what it offers.
	   Obtain the buffer size and make calculations based
	   on that. */
	ad->nbuflen = info.fragsize;
	if (ad->cvrtStereo)
	   ad->nbuflen /= 2;
	if (ad->cvrt16)
	   ad->nbuflen /= 2;
    } else {
	/* 1/2 second & align 4n */
	ad->nbuflen = (sRate * ad->sampleSizeInBytes / 2) & ~3; 
    }

    // Tone generation requires at least 500 bytes.
    // Not doing this can cause a SEGV.
    // The magic number 500 is specified in mmatone.c by
    // the following formula:
    // int len = 8000 * 16 * 1 / 8 / 32 & ~3; 
    if (ad->nbuflen < 500)
	ad->nbuflen = 500;

#endif

    ad->startdraining = 0;
    ad->drainnum = 300;

    ad->wbuf = (char *)midpMalloc(ad->nbuflen);
    if (ad->wbuf == NULL) {
        midpFree(ad);
        return 0;
    }

    ad->writtenSamples = 0;
    ad->threshold = ad->nbuflen * MAXBUFSLOT / ad->sampleSizeInBytes;
    
    ad->isSigned = isSigned;
    ad->endianess = endianess;
    
    return((int)ad);
}


#ifdef LINUX

int linuxAudioOpen(AudioDevice *ad) {
    int error;
    int format = (ad->bits == 16 ? AFMT_S16_LE : AFMT_U8);
    int fmt = format;
    int bs = ad->bits;
    int chnls = ad->channels;
    int rate = ad->rate;

    if (ad->dev != 0)
	return;

    ad->cvrtStereo = 0;
    ad->cvrt16 = 0;

    ad->dev = open(DEV_AUDIO, O_WRONLY);
    ad->mixer = open(DEV_MIXER, O_RDWR);
    if (ad->mixer == 0)
	printf("No mixer\n");

    /* Set the format - 16bit or 8 bit */
    ioctl(ad->dev, SNDCTL_DSP_RESET, 0);
    //fprintf(stderr, "fmt: %d\n", fmt);
    error = ioctl(ad->dev, SNDCTL_DSP_SETFMT, &fmt);
    //fprintf(stderr, "got fmt: %d\n", fmt);
    if (fmt != format) {
	if (fmt == AFMT_S16_LE) {
	    ad->cvrt16 = 1;
	    bs = 16;
	} else {
	    fmt = AFMT_S16_LE;
	    ioctl(ad->dev, SNDCTL_DSP_RESET, 0);
	    error = ioctl(ad->dev, SNDCTL_DSP_SETFMT, &fmt);
	    if (error >= 0 && fmt == AFMT_S16_LE) {
		ad->cvrt16 = 1;
		bs = 16;
	    } else
		error = -1;
	}
    }

    /* Set the channels to mono or stereo */
    if (error >= 0) {
	//fprintf(stderr, "chnls: %d\n", chnls);
	error = ioctl(ad->dev, SNDCTL_DSP_CHANNELS, &chnls);
	//fprintf(stderr, "got chnls: %d\n", chnls);
    }
    if (chnls != ad->channels) {
	if (chnls == 2 && ad->channels == 1)
	    ad->cvrtStereo = 1;
	else
	    error = -1;
    }

    /* Set the sample rate */
    if (error >= 0) {
	//fprintf(stderr, "rate: %d\n", rate);
	error = ioctl(ad->dev, SNDCTL_DSP_SPEED, &rate);
	//fprintf(stderr, "got rate: %d\n", rate);
    }
    if (rate != ad->rate)
	error = -1;

    if (error < 0) {
	midpFree(ad);
	return 0;
    }

    ad->sampleSizeInBytes = chnls * bs / 8; /* could be 1, 2, 4 */

    return((int)ad);
}


void linuxAudioClose(AudioDevice *ad) {
    if (ad->dev == 0)
	return;
    ioctl(ad->dev, SNDCTL_DSP_SYNC, 0);
    ioctl(ad->dev, SNDCTL_DSP_RESET, 0);
    ad->lastSamples = getSamplesPlayed(ad);
    if (ad->dev > 0)
        close(ad->dev);
    if (ad->mixer > 0)
	close(ad->mixer);
    ad->dev = 0;
    ad->mixer = 0;
}

#endif /* LINUX */


void setPause(int adptr, int pause) {
    AudioDevice *ad = (AudioDevice *)adptr;

#ifdef SOLARIS
        AudioInfo info;
	AUDIO_INITINFO(&info);
	info.play.pause = pause;
	ioctl(ad->dev, AUDIO_SETINFO, &info);
#else	/* LINUX */
	// To pause and resume, we close and reopen the device
	// on Linux.  That turns out to be more robust.
	if (pause)
	    linuxAudioClose(ad);
	else
	    linuxAudioOpen(ad);
#endif
}

/* enough space to write return wbuf; otherwise NULL*/
char * getNextAvailBuf(int adptr, int len) {
    AudioDevice *ad = (AudioDevice *)adptr;
    int samples = getSamplesPlayed(adptr);

    if (samples < 0 || (ad->writtenSamples - samples) > ad->threshold)
	return (NULL);

    return (ad->wbuf);
}
   
void audioWrite(int adptr, int offset, int len) {
    AudioDevice *ad = (AudioDevice *)adptr;
    int ret;

#ifdef SOLARIS
    /* Solaris requires some format conversion. */
    if (((ad->isSigned == MMAFMT_UNSIGNED) && (ad->bits == 8))  || 
        ((ad->bits == 16) && (ad->endianess == MMAFMT_LITTLE)))
      fmtcvrt(ad, offset, len);
    if ((ad->isSigned == MMAFMT_UNSIGNED) && (ad->bits == 8)) {
	ret = write(ad->dev, ad->wbuf + 2*offset, len << 1);
    } else {
	ret = write(ad->dev, ad->wbuf + offset, len);
    }
#else /* LINUX */
    if (ad->dev == 0)
	return;

    if (ad->cvrtStereo || ad->cvrt16) {
	// Audio conversion required.
	int wlen = fmtcvrt(ad, offset, len);
	ret = write(ad->dev, ad->dbuf, wlen);
    } else {
	ret = write(ad->dev, ad->wbuf + offset, len);
    }
#endif

    /* TODO had better handle overflow in the future*/
    ad->writtenSamples += ret/ad->sampleSizeInBytes;
}


int getSamplesPlayed(int adptr) {
    AudioDevice *ad = (AudioDevice *)adptr;

#ifdef SOLARIS
    AudioInfo info;
    if (ad == 0 || ioctl(ad->dev, AUDIO_GETINFO, &info) < 0)
	return -1;
    return ((int)info.play.samples);
#else	/* LINUX */
    int delay = 0;
    if (ad == 0)
	return -1;
    if (ad->dev == 0 || ioctl(ad->dev, SNDCTL_DSP_GETODELAY, &delay) < 0)
	return ad->lastSamples;
    return (int) (ad->writtenSamples - delay/ad->sampleSizeInBytes);
#endif
}


void setVolume(int adptr, int volume) {
    AudioDevice *ad = (AudioDevice *)adptr;

#ifdef SOLARIS
    AudioInfo info;
    volume = volume * 255 / 100;
    AUDIO_INITINFO(&info);
    info.play.gain = volume & 0xff;   /* range from 0..255 */
    ioctl(ad->dev, AUDIO_SETINFO, &info);
#else	/* LINUX */
    if (ad->mixer != 0) {
	volume |= (volume << 8);	// make it stereo
	ioctl(ad->mixer, MIXER_WRITE(SOUND_MIXER_PCM), &volume);
    }
#endif

}


int getVolume(int adptr) {
    AudioDevice *ad = (AudioDevice *)adptr;

#ifdef SOLARIS
    AudioInfo info;
    AUDIO_INITINFO(&info);
    info.play.gain = 127;   /* range from 0..255 */
    ioctl(ad->dev, AUDIO_SETINFO, &info);
    return 50;
#else
    int volume;
    if (ad->mixer != 0)
	ioctl(ad->mixer, MIXER_READ(SOUND_MIXER_PCM), &volume);
    return (volume & 0xff);
#endif
}


int audioCommon(int adptr, int code, int param) {    

    if (adptr == 0) {
	if (code == DRAIN)
	    return (1);
	else
	    return(0);
    }
  
    switch (code) {
    case PAUSE:
	setPause(adptr, 1);
	break;

    case RESUME:
	setPause(adptr, 0);
	break;

    case FLUSH:
#ifdef SOLARIS
	ioctl(((AudioDevice *)adptr)->dev, I_FLUSH, FLUSHW);
#endif
	/* Don't do anything for Linux. */
	break;

    case DRAIN:
    {
	/* drained return 1; otherwise return 0 */
	AudioDevice *ad = (AudioDevice *)adptr;

#ifdef SOLARIS
	AudioInfo info;

	if (ad->startdraining == 0) {
	  ad->startdraining = 1;
	  ad->drainnum = 300; /* wait for at most 4.8 seconds */

	  AUDIO_INITINFO(&info);
	  info.play.eof = 0;
	  if (ioctl(ad->dev, AUDIO_SETINFO, &info) < 0) {
	      ad->startdraining = 0;
	      printf("error when ioctl 1 \n");
	      return (1);
	  }

          /* send 0 buffer */
	  write(ad->dev, ad->wbuf, 0);
	  return (0);
	} else {
	  if (ioctl(ad->dev, AUDIO_GETINFO, &info) < 0) {
	      ad->startdraining = 0;
	      printf("error when ioctl 2 \n");
	      return (1);
	  }

	  /* when the device fully drained, play.eof increase by 1 */
	  if (info.play.eof == 1) {
	      ad->startdraining = 0;
	      return (1);
	  } else {
	      ad->drainnum --;
	      if (ad->drainnum < 0) { /* wait for at most 4.8 seconds */
		  ad->startdraining = 0;
		  return (1);
	      }
	      return (0);
	  }
	}
#else /* LINUX */

	// There really is no robust way to drain a Linux device other
	// than closing it.  Pause will do that.  So we won't do 
	// anything here.
	return 1;
#endif
    }

    case CLOSE:
    {
	AudioDevice *ad = (AudioDevice *)adptr;
      
#ifdef LINUX
	linuxAudioClose(ad);
	if (ad->dbuf != NULL) {
	    midpFree(ad->dbuf);
	}
	ad->dbuf = NULL;
#else /* SOLARIS */
	close(ad->dev);
#endif
	if (ad->wbuf != NULL) {
	    midpFree(ad->wbuf);
	}
	ad->wbuf = NULL;
	midpFree(ad);
    }
    break;

    case SAMPLES_PLAYED:
	return getSamplesPlayed(adptr);

    case SET_VOLUME:
      /* 0 <= param <= 100 */
      setVolume(adptr, param);
	break;

    case GET_VOLUME:
	return getVolume(adptr);

    case GET_BUFLEN: 
    {
	AudioDevice *ad = (AudioDevice *)adptr;
	  
#ifdef SOLARIS
	if (ad->bits == 8)
	    return (ad->nbuflen / 2);
	else 
	    return (ad->nbuflen);
#else /* LINUX */
	return (ad->nbuflen);
#endif
	break;
    }
    }
  
    return(1);
}
