/*
 * @(#)staticGraphics.h	1.9 02/09/05 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#define arrow_width 7
#define arrow_height 7
extern unsigned char up_bits[]   ;
extern unsigned char down_bits[]  ;
extern unsigned char right_bits[] ;
extern unsigned char left_bits[]  ;

/* phone skin graphic as of 6/19/2002 */
extern char *phone_xpm[] ;

/* static global area (topbar) graphic with network strength and 
 * battery indicator icons */
extern char *topbar_xpm[] ;

#define topbar_width 180 /* 96 */
#define topbar_height 11 /* 10 */

/* Backlight image and position */
extern char *backLight_xpm[] ;
extern char *backLight_Off_xpm[] ;

#define Bklite_xposition 0
#define Bklite_yposition 113
#define Bklite_width 241
#define Bklite_height 232


/* Network Indicator images and position */
extern char *greenLED_xpm[] ;
extern char *grayLED_xpm[] ;

#define LED_xposition 17
#define LED_yposition 82
#define LED_width 20
#define LED_height 20

/* trusted icon image */
extern char *trustedicon_xpm[] ;
