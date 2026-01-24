/*
 * @(#)audiornd.c	1.4 02/07/24 @(#)
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include <midpMalloc.h>
#include <stdio.h>
#include "audiornd.h"


/*=========================
 * KNI functions
 *=========================
 */

/*=========================================================================
 * FUNCTION:      nOpen(III)I
 * CLASS:         com.sun.mmedia.WavPlayer
 * TYPE:          virtual native function
 * OVERVIEW:      open WaveOut device
 * INTERFACE (operand stack manipulation):
 *   parameters:  sRate     sample rate 
 *                bits      sample size in bits
 *                channels  mono or stereo
 *   returns:     the pointer to native waveout data structure. <= 0 failed.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_WavPlayer_nOpen(KNITRAPS) {
    int channels = KNI_GetParameterAsInt(3);
    int bits = KNI_GetParameterAsInt(2);
    int sRate = KNI_GetParameterAsInt(1);
    
    if (bits == 8) {
      KNI_ReturnInt(audioOpen(sRate, bits, channels, MMAFMT_UNSIGNED, MMAFMT_LITTLE));
    } else { 
      KNI_ReturnInt(audioOpen(sRate, bits, channels, MMAFMT_SIGNED, MMAFMT_LITTLE));  
    }
}

/*=========================================================================
 * FUNCTION:      nWrite(I[BII)I
 * CLASS:         com.sun.mmedia.WavPlayer
 * TYPE:          virtual native function
 * OVERVIEW:      write audio data to WaveOut device
 * INTERFACE (operand stack manipulation):
 *   parameters:  peer   the pointer to the native waveout data structure
 *                data   the byte array to be written
 *                offset the offset
 *                len    the length to write  
 *   returns:     the actual written number of bytes
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_WavPlayer_nWrite(KNITRAPS) {
    int len = KNI_GetParameterAsInt(4);
    int offset = KNI_GetParameterAsInt(3);
    int jpeer = KNI_GetParameterAsInt(1);
    char * data;
    
    if (jpeer == 0) {
	KNI_ReturnInt(-1);
    }
    
    if ((data=getNextAvailBuf(jpeer, len)) == NULL)
      KNI_ReturnInt(0);

    KNI_StartHandles(1);
    KNI_DeclareHandle(arrHandle);
    KNI_GetParameterAsObject(2, arrHandle);

    /* TODO: error checking of offset, len and array's length
       But the checking will increase the code size, right now
       WavPlayer guarantee the param are good
    */
    /*
      arrlen = KNI_GetArrayLength(arrHandle);
      if ( offset+len > arrlen) {
      len = arrlen - offset;
      }
      if ( len <= 0 ) {
      KNI_EndHandles();
      KNI_ReturnInt(-1);
      }
    */
    KNI_GetRawArrayRegion(arrHandle, offset, len, (jbyte *)data);
    /* always write full */
    audioWrite(jpeer, offset, len);
    
    KNI_EndHandles();
    KNI_ReturnInt((jint)len);
    
}

/*=========================================================================
 * FUNCTION:      nCommon(III)I
 * CLASS:         com.sun.mmedia.WavPlayer
 * TYPE:          virtual native function
 * OVERVIEW:      common utility functions for waveout
 * INTERFACE (operand stack manipulation):
 *   parameters:  peer    the pointer to the toneseq data structure
 *                code  opcode for a particular tone operation
 *                param user defined  parameter
 *   returns:     status. <= 0 failed.
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_WavPlayer_nCommon(KNITRAPS) {
    int param = KNI_GetParameterAsInt(3);
    int code = KNI_GetParameterAsInt(2);
    int jpeer = KNI_GetParameterAsInt(1);
    
    KNI_ReturnInt(audioCommon(jpeer, code, param));
}
