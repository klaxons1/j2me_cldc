/*
 * @(#)crc32.c	1.4 02/07/24 @(#)
 *
 * Copyright (c) 1998-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <crc32.h>

unsigned long
CRC32_init(unsigned char *data, int length){
    return (CRC32_update(data, length, 0xFFFFFFFF));
}

unsigned long
CRC32_update(unsigned char *data, int length, unsigned long iv) { 
    unsigned long crc = iv;
    unsigned int j;
    for ( ; length > 0; length--, data++) { 
        crc ^= *data;
        for (j = 8; j > 0; --j) { 
            crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320) : (crc >> 1);
        }
    }
    return crc;
}

unsigned long
CRC32_finalize(unsigned long crc) {
    return ~crc;
}
