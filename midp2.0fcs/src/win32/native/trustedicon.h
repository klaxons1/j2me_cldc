/*
 * @(#)trustedicon.h	1.3 02/09/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * Trusted MIDlet icon.
 *
 */

struct _PackedTrustedMIDletIconDib {
    BITMAPINFOHEADER hdr;
    DWORD            info[109];
} _trustedIcon_dib_data = {
    {
        sizeof(BITMAPINFOHEADER), /* biSize */
        12,			  /* biWidth */
        12,			  /* biHeight */
        1,			  /* biPlanes */
        24,			  /* biBitCount */
        0,			  /* biCompression */
        430,		  	  /* biSizeImage */
        3780,			  /* biXPelsPerMeter */
        3780,			  /* biYPelsPerMeter */
        0,			  /* biClrUsed */
        0,			  /* biClrImportant */
    }, {
	0xaab6b6aa, 0xb6aab6b6, 0xb6b6aab6, 0x00b6b6aa, 
	0x6d550000, 0xb6b6aa6d, 0xaab6b6aa, 0xb6aab6b6, 
	0xb6b6aab6, 0xaab6b6aa, 0xb6aab6b6, 0xb6b6aab6, 
	0x55000000, 0x00006d6d, 0x6d6d5500, 0xaab6b6aa, 
	0xb6aab6b6, 0xb6b6aab6, 0x00b6b6aa, 0x00000000, 
	0x00000000, 0xaa6d6d55, 0xffffb6b6, 0x000000ff, 
	0x00000000, 0x6d550000, 0xb6b6aa6d, 0x00b6b6aa, 
	0x6d550000, 0xffffff6d, 0xaab6b6aa, 0xb6aab6b6, 
	0xffffffb6, 0x00ffffff, 0x6d550000, 0xb6b6aa6d, 
	0x00b6b6aa, 0x6d550000, 0xb6b6aa6d, 0x00b6b6aa, 
	0x6d550000, 0xb6b6aa6d, 0x00b6b6aa, 0x6d550000, 
	0xb6b6aa6d, 0x55000000, 0xb6aa6d6d, 0xb6b6aab6, 
	0x00b6b6aa, 0x6d550000, 0xb6b6aa6d, 0xffb6b6aa, 
	0x0000ffff, 0x6d6d5500, 0x00ffffff, 0x6d550000, 
	0xb6b6aa6d, 0x00000000, 0x00000000, 0x6d6d5500, 
	0x00b6b6aa, 0x6d550000, 0xb6b6aa6d, 0x00b6b6aa, 
	0x6d550000, 0xb6b6aa6d, 0xffffffff, 0xffffffff, 
	0xffffffff, 0x00b6b6aa, 0x6d550000, 0xb6b6aa6d, 
	0x00b6b6aa, 0x00000000, 0x00000000, 0xaa6d6d55, 
	0xb6aab6b6, 0x000000b6, 0x00000000, 0x6d550000, 
	0xb6b6aa6d, 0xffb6b6aa, 0xffffffff, 0xffffffff, 
	0x55000000, 0x00006d6d, 0x6d6d5500, 0xffffffff, 
	0xb6aaffff, 0xb6b6aab6, 0xaab6b6aa, 0xb6aab6b6, 
	0xb6b6aab6, 0x00ffffff, 0x6d550000, 0xb6b6aa6d, 
	0xaab6b6aa, 0xb6aab6b6, 0xb6b6aab6, 0xaab6b6aa, 
	0xb6aab6b6, 0xb6b6aab6, 0xffb6b6aa, 0xb6aaffff, 
	0xb6b6aab6, 0xaab6b6aa, 0xb6aab6b6, 0xb6b6aab6, 
	0x00b6b6aa  }
};
BITMAPINFOHEADER *trustedIcon_dib = &(_trustedIcon_dib_data.hdr);
