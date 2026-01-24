/*
 * @(#)crc32.h	1.5 02/07/24 @(#)
 *
 * Copyright (c) 1998-2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * This is a CRC32 checksum algorithm implementation which.
 * allows a checksum to be computed in small chunks.  This lets
 * the caller limit the amount of space used to store the data
 * being hashed.
 *
 * For example, below we compute the checksum of 180 bytes of
 * data in three 60 byte chunks.
 *
 *    unsigned long crc;
 *    crc = CRC32_init(data_chunk_1, 60);
 *    crc = CRC32_update(data_chunk_2, 60, crc);
 *    crc = CRC32_update(data_chunk_3, 60, crc);
 *    crc = CRC32_finalize(crc);
 *
 * 'crc' now contains a CRC32 checksum of the 180 data bytes.
 *
 * Note: It is alright to call CRC32_init followed directly
 * by CRC32_finalize if the data buffer being checked is
 * complete instead of partial.
 */

/*
 * This function starts a CRC32 checksum calculation.
 * 
 * After the checksum has been computed over all the data, call
 * CRC32_finalize on the checksum.  It is OK to call CRC32_finalize
 * immediately after CRC32_init if all of the data to be checksummed
 * is contained in the data argument of the CRC32_init call.
 *
 * params:
 * char* data - the bytes to update the checksum with
 * int length - the length in bytes of 'data'
 *
 * returns: an in progress CRC calculation.
 */

unsigned long CRC32_init(unsigned char *data, int length);

/*
 * This function updates an in progress CRC32 checksum calculation.
 * 
 * After the checksum has been computed over all the data, call
 * CRC32_finalize on the checksum.  
 *
 * params:
 * char* data - the bytes to update the checksum with
 * int length - the length in bytes of 'data'
 * unsigned long iv - the in progress checksum returned by a previous
 *                    call to CRC32.  
 *
 * returns: an in progress CRC calculation.
 */
unsigned long CRC32_update(unsigned char *data, int length, 
			   unsigned long iv);

/*
 * This function finalizes a CRC32 checksum calculation.
 *
 * params:
 * unsigned long crc - the in progress checksum created by
 *                     a call to CRC32_init and zero or more 
 *                     calls to CRC32_update.
 * return: a final CRC32 checksum
 */
unsigned long CRC32_finalize(unsigned long crc); 
