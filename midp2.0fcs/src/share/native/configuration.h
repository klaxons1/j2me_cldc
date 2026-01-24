/*
 * @(#)configuration.h	1.5 02/07/24 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * SYSTEM:    MIDP
 * SUBSYSTEM: Configuration property file support.
 * FILE:      configprop.h
 * OVERVIEW:  Function prototypes for accessing runtime system properties
 *
 * OVERVIEW:  This file defines the function that allows a native
 *            handler of configuration properties from the environment variables,
 *            command line arguments and preconfigured default property files
 *            to be unified in a common set of interfaces.
 *
 * USAGE:     Implementation proerties in general are not visible to the 
 *            to Java application code. Properties are accessible
 *            to native code as well as Java implementation code.
 *
 * AUTHOR:    Gary Adams
 *=======================================================================*/

/*=========================================================================
 * Function prototypes for the configprop of native resources 
 *=======================================================================*/

#ifndef CONFIGPROP_H
#define CONFIGPROP_H

/* Initialize the configuration subsystem */
extern void initializeConfig(char *);
extern void finalizeConfig();

/* Get and Set system specific implementation properties.(protected) */
extern char * getInternalPropDefault (char*, char* );
extern char * getInternalProp (char* );
extern void setInternalProp (char*, char* );

/* Get and Set application visible properties.(public) */
extern char * getSystemProp (char* );
extern void setSystemProp (char*, char* );

#endif /* CONFIGPROP_H */
