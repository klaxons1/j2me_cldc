/*
 * @(#)configuration.c	1.29 02/10/10 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * MIDP native configuration property implementation
 *=========================================================================
 * SYSTEM:    MIDP
 * SUBSYSTEM: Configuration properties
 * FILE:      configprop.c
 * OVERVIEW:  This file defines the functions that access runtime configuration
 *            properties that are predefined in configuration files or
 *            overridden on the command line or environmental variables.
 * ASSUMPTIONS:
 *            The complete property mechanism will not be needed when
 *            MIDP is ported to a physical device where there is no
 *            need for command line arguments or environment variable
 *            overrides. This module is intended to simplify the porting
 *            effort in identifying runtime switches that can be
 *            optimized when shrinking the complete system by hard
 *            wiring specific components in the system.
 *
 * DataStructure:
 *           Two configuration files are supported. First, the file
 *           lib/system.config contains the properties visible to the
 *           application code via the System.getProperty() interface.
 *           Second, the lib/internal.config contains implementation
 *           specific properties that are not intended to be exposed
 *           to the MIDlet application.
 *
 *           A configuration file contains "key: value\n" lines. Carriage
 *           returns are ignored, and lines beginning with the pound sign
 *           are skipped as comment lines. White space after the colon are
 *           trimmed before the kay and value are recorded.
 *
 *           The system will continue to run even if the configuration files
 *           can not be read or contain parsing errors in the data.
 *
 * AUTHOR:    Gary Adams
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <string.h>
#else
#include <strings.h>
#endif

#include <ctype.h>

#include <configuration.h>
#include <storage.h>
#include <socketProtocol.h>

#include <kni.h>
#include <midpMalloc.h>

#ifdef UNIX
#define IMPL_PROPERTY_FILE "lib/internal.config"
#define APPL_PROPERTY_FILE "lib/system.config"
#else /* WINDOWS */
#define IMPL_PROPERTY_FILE "lib\\internal.config"
#define APPL_PROPERTY_FILE "lib\\system.config"
#endif

/* buffer size for line parsing */
#define MAX_LINE 512

/*========================================================================
 * Variables
 *========================================================================*/
typedef struct _configproperty {
    struct _configproperty *next ;
    char *key;
    char *value;
} Property ;

/*
 * Use a simple linked list of name value pairs for application level and
 * implementation targeted properties. The space is partitioned to provide
 * some separation of values that can be protected.
 */
static Property * applicationProperties = NULL ;
static Property * implementationProperties = NULL ;

/* Forward references */
extern void initProps(Property **, char *, char *);

/*=========================================================================
 * FUNCTION:      initializeConfig()
 * TYPE:          public operation
 * OVERVIEW:      Initialize the configuration data.
 * INTERFACE:
 *=======================================================================*/
/* a default for the configuration property is required by the CLDC spec */
#define DEFAULT_CONFIGURATION "microedition.configuration"
#define DEFAULT_CLDC "CLDC-1.0"

/* a default for the encoding property is required by the CLDC spec */
#define ENCODING_PROP_NAME "microedition.encoding"
#define DEFAULT_CHARACTER_ENCODING "ISO-8859-1"

/* a default for profiles property is required by the MIDP spec */
#define PROFILES_PROP_NAME "microedition.profiles"
#define DEFAULT_PROFILE "MIDP-2.0"

void initializeConfig (char *midp_home) {
    initProps(&implementationProperties, IMPL_PROPERTY_FILE, midp_home);
    initProps(&applicationProperties, APPL_PROPERTY_FILE, midp_home);

    /*
     * Make sure the configuration was specified, because
     * some older code requires it in the CLDC classes.
     */
    if (getSystemProp(DEFAULT_CONFIGURATION) == NULL)
    setSystemProp(DEFAULT_CONFIGURATION, DEFAULT_CLDC);

    if (getSystemProp(PROFILES_PROP_NAME) == NULL) {
    setSystemProp(PROFILES_PROP_NAME, DEFAULT_PROFILE);
    }

    if (getSystemProp(ENCODING_PROP_NAME) == NULL) {
    setSystemProp(ENCODING_PROP_NAME, DEFAULT_CHARACTER_ENCODING);
    }
}

/*=========================================================================
 * FUNCTION:      finalizeConfig()
 * TYPE:          public operation
 * OVERVIEW:      Finalize the configuration subsystem by releasing
 *                all the allocating memory buffers
 * INTERFACE:
 *=======================================================================*/
void finalizeConfig () {
    Property *prop, *tmp;

    for (prop = implementationProperties ; prop ; prop = tmp ){
    tmp = prop->next;
    midpFree(prop->key);
    midpFree(prop->value);
    midpFree(prop);
    }
    for (prop = applicationProperties ; prop ; prop = tmp ){
    tmp = prop->next;
    midpFree(prop->key);
    midpFree(prop->value);
    midpFree(prop);
    }
}

/*=========================================================================
 * FUNCTION:      parseConfig()
 * TYPE:          public operation
 * OVERVIEW:      Seed the implementation properties from
 *                a well known property file
 * INTERFACE:
 *=======================================================================*/
static void parseConfig (int fd, Property ** props) {
    char *buffer;
    int bufferSize;
    int i;
    int j;
    int key_index;
    int value_index;
    int len;
    char *errStr = NULL;
    int startPos;
    int endPos;

    bufferSize = storageSizeOf(&errStr, fd);
    buffer = (char *)midpMalloc(bufferSize);
	if (buffer == NULL) {
        fprintf(stderr, "midpMalloc failed to allocate buffer for config file.\n");
        exit(-1);
	}

    /* Read the config file in one pass */
    len = storageRead(&errStr, fd, buffer, bufferSize);
    if ((errStr != NULL) || (len != bufferSize)) {
        fprintf(stderr, "Warning: can not read config file: %s\n", errStr);
        storageFreeError(errStr);
        midpFree(buffer);
        return;
    }

    startPos = 0;
    for (i = startPos; i < bufferSize; i++) {
        if (buffer[i] == '\n') {
            
            buffer[i] = 0;

            /* Skip comment lines which begin  with '#'*/
            if (buffer[startPos] != '#') {
                
                /* Parse the line */
                key_index = startPos;
                for (j = key_index; buffer[j]; j++){

                    Property *prop;

                    if (buffer[j] == ':') {
                        buffer[j] = 0;
                        value_index = ++j;
                        while (isspace(buffer[value_index])) {
                            value_index++;
                        }                        

                        if (prop = (Property *) midpMalloc(sizeof(Property))) {
                            prop->next = *props;
                            prop->key = midpStrdup(buffer + key_index);
                            prop->value = midpStrdup(buffer + value_index);

                            if ((prop->key == NULL) || (prop->value == NULL)) {
                                midpFree(prop->key);
                                midpFree(prop->value);
                                midpFree(prop);

                                /*
                                 * since we are freeing memory, we're not exactly
                                 * out of memory at this point
                                 */
                                break;
                            }

                            *props = prop;
                        }
                        /* DEBUG : fprintf (stderr, "key(%s) value(%s)\n",
                         &buffer[key_index],&buffer[value_index]);  */
                        break;
                    }
                }
            }
            endPos = i;
            startPos = endPos + 1;
        }
    }
    midpFree(buffer);
}

/*=========================================================================
 * FUNCTION:      initProps(Property **, char*)
 * TYPE:          public operation
 * OVERVIEW:      Seed the implementation properties from
 *                a well known property file
 * INTERFACE:
 *   parameters:  Property ** props - the current pool of properties
 *                char * name - base filename for configuration values
 *=======================================================================*/
void initProps (Property ** props, char *name, char *midp_home) {
    char pathname[MAX_FILENAME_LENGTH + 1];
    int fd = -1 ;
    char * errStr;

    /* Property file can be relative or at midp_home variable */
    (void)strcpy(pathname, midp_home);
    (void)strcat(pathname, getFileSeparator());
    (void)strcat(pathname, name);

    if ((fd = storageOpen(&errStr, pathname, OPEN_READ)) != -1){
    /* Read through the file one line at a time */
    parseConfig (fd, props);

    /* Close the storage handle */
    storageClose (&errStr, fd);
    } else {
    if (errStr != NULL) {
        fprintf (stderr, "Warning: could not open config file(%s): %s\n",
             pathname, errStr);
        storageFreeError(errStr);
        return;
    }

    }

}

/*=========================================================================
 * FUNCTION:      findProp(Property * , char *)
 * TYPE:          public operation
 * OVERVIEW:      Return a value of a property key
 * INTERFACE:
 *   parameters:  Property *props
 *                char *key
 *   returns:     char *result
 *=======================================================================*/
static char * findProp (Property * prop, char* key) {
    Property * p= prop;
    while (p) {
    if (strcmp(key, p->key) == 0 )
        return (p->value);
    p = p->next;
    }
    return(NULL);
}



/*=========================================================================
 * FUNCTION:      setProp(Property *, char * , char *)
 * TYPE:          public operation
 * OVERVIEW:      Set a value of a property key, value pair
 * INTERFACE:
 *   parameters:  Property *props
 *                char *key
 *                char *value
 *=======================================================================*/
static void setProp (Property **props, char* key , char* value ) {
    /* Try to find the property in the current pool. */
    Property *p;
    for (p= *props; p; p = p->next){
    if (strcmp(key, p->key) == 0 ){
        midpFree (p->value);
        p->value = midpStrdup(value);
        /**
         * if midpStrdup fails we will just return without setting the value
         * to anything other than NULL
         */
        return;
    }
    }

    /* If the value is not defined, add it now */
    if (p = (Property *) midpMalloc (sizeof(Property))){
        p->next = *props ;
        p->key = midpStrdup(key);
        p->value = midpStrdup(value);

        if ((p->key == NULL) || (p->value == NULL)) {
            /*
             * do nothing if there is no memory
             */
            midpFree(p->key);
            midpFree(p->value);
            midpFree(p);
            return;
        }

        *props = p ;
    }

}


/*=========================================================================
 * FUNCTION:      setInternalProp(char * , char *)
 * TYPE:          public operation
 * OVERVIEW:      Set a value of a property key, value pair
 * INTERFACE:
 *   parameters:  char *key
 *                char *value
 *=======================================================================*/
void setInternalProp (char* key , char* value ) {
    setProp(&implementationProperties, key, value);
}

/*=========================================================================
 * FUNCTION:      getInternalProp(char *)
 * TYPE:          public operation
 * OVERVIEW:      Return a value of a property key
 * INTERFACE:
 *   parameters:  char *key
 *   returns:     char *result
 *=======================================================================*/
char * getInternalProp (char* key) {
    char *result = findProp(implementationProperties, key) ;
    return (result? result : findProp(applicationProperties, key));
}

/*=========================================================================
 * FUNCTION:      getInternalProp(char *)
 * TYPE:          public operation
 * OVERVIEW:      Return a value of a property key or the supplied default.
 * INTERFACE:
 *   parameters:  char *key
 *                char *def
 *   returns:     char *result
 *=======================================================================*/
char * getInternalPropDefault (char* key, char * def) {
    /* check the implementation specific configuration properties. */
    char *result = findProp(implementationProperties, key) ;
    if (result == NULL) {
    /* check the apllication properties, as well. */
    result = findProp(applicationProperties, key);
    }
    /* If the key was not found return the supplied default value. */
    return (result? result : def);
}

/*=========================================================================
 * FUNCTION:      getSystemProp(char *)
 * TYPE:          public operation
 * OVERVIEW:      Return a value of a property key
 * INTERFACE:
 *   parameters:  char *key
 *   returns:     char *result
 *=======================================================================*/
char * getSystemProp (char* key ) {
    char *value = findProp(applicationProperties, key);

    if (value == NULL &&
    (strcmp(key, "microedition.hostname") == 0)) {
    /*
     * If the host name was not configured, use the
     * native networking function to determine,
     * the name of the local host.
     */
    value = getLocalHostName();
    }
    return value;
}


/*=========================================================================
 * FUNCTION:      setSystemProp(char*, char*)
 * TYPE:          public operation
 * OVERVIEW:      Set a value of a property key, value pair
 * INTERFACE:
 *   parameters:  char *key
 *                char *value
 *=======================================================================*/
void setSystemProp (char* key , char* value) {
    setProp(&applicationProperties, key, value);
}

static char* UnicodeToCString(jchar* uString, int length);
/*=========================================================================
 * FUNCTION:      getProperty0(Ljava.lang.String)Ljava.lang.String (STATIC)
 * CLASS:         com.sun.midp.main.Configuration
 * TYPE:          static native function
 * OVERVIEW:      Return a property
 * INTERFACE (operand stack manipulation):
 *   parameters:  A string
 *   returns:     A string
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_main_Configuration_getProperty0() {
    jchar* uStr;
    char* key;
    char* value;
    int strLen;

    KNI_StartHandles(5);
    KNI_DeclareHandle(str);
    KNI_DeclareHandle(result);

    KNI_GetParameterAsObject(1, str);

    strLen = KNI_GetStringLength(str);

    uStr = (jchar*) midpMalloc(strLen * sizeof(jchar));

    if (uStr == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(str, 0, strLen, uStr);
        key = UnicodeToCString(uStr, strLen);
        midpFree(uStr);

        /* Look up the property value */
        value = getInternalProp(key);
        midpFree(key);

        if (value != NULL) {
            KNI_NewStringUTF(value, result);
        } else {
            KNI_ReleaseHandle(result);
        }
    }

    KNI_EndHandlesAndReturnObject(result);
}

/*=========================================================================
 * FUNCTION:  UnicodeToCString()
 * TYPE:      internal operation on string objects
 * OVERVIEW:  Get the contents of a string object in C/C++ string format.
 * INTERFACE:
 *   parameters:  String object pointer
 *   returns:     char* to the string in C/C++ format
 *=======================================================================*/
static char* UnicodeToCString(jchar* uString, int length) {
    int        i;
    char*      cString;

    if (NULL == uString) {
        return NULL;
    }

    cString = (char*) midpMalloc((sizeof(char)*length) + 1);
    /* one for the zero terminator */
    if (NULL == cString) {
        return NULL;
    }

    for (i = 0; i < length; i++) {
    cString[i] = (char)uString[i];
    }

    /*  Terminating C-string with zero */
    cString[length] = 0;
    return cString;
}
