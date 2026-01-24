/*
 * @(#)commandState.c	1.29 02/09/03 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <stdlib.h>

#include <kni.h>
#include <midpMalloc.h>
#include <commandState.h>

static int copyStringField(jobject obj, jclass classObj,
    char* pszFieldName, char** ppResult, jobject fieldHandle);

COMMAND_STATE MidpCommandState = {0};

#define FREE_STRING(x) if((x)!=NULL){midpFree((x));(x)=NULL;}

void
finalizeCommandState(void) {
    FREE_STRING(MidpCommandState.midletClassName);
    FREE_STRING(MidpCommandState.midletName);
    FREE_STRING(MidpCommandState.descriptorName);
    FREE_STRING(MidpCommandState.suiteStorageName);
    FREE_STRING(MidpCommandState.suiteURL);
    FREE_STRING(MidpCommandState.runExceptionMessage);
    FREE_STRING(MidpCommandState.securityDomain);
    FREE_STRING(MidpCommandState.midletNumber);
}

#define KNI_RESTORE_BOOLEAN_FIELD(OBJECT,CLASS,NAME,VALUE) \
    KNI_SetBooleanField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"Z"), (VALUE))

#define KNI_RESTORE_INT_FIELD(OBJECT,CLASS,NAME,VALUE) \
    KNI_SetIntField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"I"), (VALUE))

#define KNI_RESTORE_STRING_FIELD(OBJECT,CLASS,NAME,VALUE,HANDLE) { \
    if ((VALUE) != NULL) { \
        KNI_NewStringUTF((VALUE), (HANDLE)); \
        KNI_SetObjectField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"Ljava/lang/String;"), (HANDLE)); \
    } \
}

/*=========================================================================
 * FUNCTION:      restoreCommandState(Lcom/sun/midp/CommandState;)V
 * CLASS:         com.sun.midp.main.Main
 * TYPE:          virtual native function
 * OVERVIEW:      restore the command state of the JAM
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_Main_restoreCommandState() {
    KNI_StartHandles(3);
    KNI_DeclareHandle(commandState);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(string);

    KNI_GetParameterAsObject(1, commandState);
    
    KNI_GetObjectClass(commandState, clazz);

    KNI_RESTORE_INT_FIELD(commandState, clazz, "status",
			  MidpCommandState.status);
    KNI_RESTORE_INT_FIELD(commandState, clazz, "initialCommand",
			  MidpCommandState.initialCommand);
    KNI_RESTORE_INT_FIELD(commandState, clazz, "nextCommand",
			  MidpCommandState.nextCommand);
    KNI_RESTORE_BOOLEAN_FIELD(commandState, clazz, "forceOverwrite",
			      (jboolean)MidpCommandState.forceOverwrite);
    KNI_RESTORE_BOOLEAN_FIELD(commandState, clazz, "runOnce",
			      (jboolean)MidpCommandState.runOnce);
    KNI_RESTORE_BOOLEAN_FIELD(commandState, clazz, "autotest", 
			      (jboolean)MidpCommandState.autotest);
    KNI_RESTORE_BOOLEAN_FIELD(commandState, clazz, "logoDisplayed", 
			      (jboolean)MidpCommandState.logoDisplayed);
    KNI_RESTORE_STRING_FIELD(commandState, clazz, "midletClassName",
			     MidpCommandState.midletClassName, string);
    KNI_RESTORE_STRING_FIELD(commandState, clazz, "midletName",
			     MidpCommandState.midletName, string);
    KNI_RESTORE_STRING_FIELD(commandState, clazz, "descriptorName",
			     MidpCommandState.descriptorName, string);
    KNI_RESTORE_STRING_FIELD(commandState, clazz, "suiteStorageName",
			     MidpCommandState.suiteStorageName, string);
    KNI_RESTORE_STRING_FIELD(commandState, clazz, "suiteURL",
			     MidpCommandState.suiteURL, string);
    KNI_RESTORE_STRING_FIELD(commandState, clazz, "runExceptionMessage",
			     MidpCommandState.runExceptionMessage, string);
    KNI_RESTORE_STRING_FIELD(commandState, clazz, "securityDomain",
			     MidpCommandState.securityDomain, string);
    KNI_RESTORE_STRING_FIELD(commandState, clazz, "midletNumber",
			     MidpCommandState.midletNumber, string);
    KNI_RESTORE_BOOLEAN_FIELD(commandState, clazz, "removeRMS",
			      (jboolean)MidpCommandState.removeRMS);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

#define KNI_SAVE_BOOLEAN_FIELD(OBJECT,CLASS,NAME,VALUE) \
    (VALUE) = KNI_GetBooleanField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"Z"))

#define KNI_SAVE_INT_FIELD(OBJECT,CLASS,NAME,VALUE) \
    (VALUE) = KNI_GetIntField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"I"))

#define KNI_SAVE_STRING_FIELD(OBJECT,CLASS,NAME,VALUE,HANDLE) { \
    if (!copyStringField((OBJECT), (CLASS), (NAME), &(VALUE), (HANDLE))) { \
        break; \
    } \
}

/*=========================================================================
 * FUNCTION:      saveCommandState(Lcom/sun/midp/CommandState;)V
 * CLASS:         com.sun.midp.main.Main
 * TYPE:          virtual native function
 * OVERVIEW:      save the command state of the JAM
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_Main_saveCommandState() {
    KNI_StartHandles(3);
    KNI_DeclareHandle(commandState);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(string);

    KNI_GetParameterAsObject(1, commandState);
    KNI_GetObjectClass(commandState, clazz);

    KNI_SAVE_INT_FIELD(commandState, clazz, 
		       "status", MidpCommandState.status);
    KNI_SAVE_INT_FIELD(commandState, clazz, 
		      "initialCommand", MidpCommandState.initialCommand);
    KNI_SAVE_INT_FIELD(commandState, clazz, 
		      "nextCommand", MidpCommandState.nextCommand);
    KNI_SAVE_BOOLEAN_FIELD(commandState, clazz, 
			  "forceOverwrite", MidpCommandState.forceOverwrite);
    KNI_SAVE_BOOLEAN_FIELD(commandState, clazz, 
			  "runOnce", MidpCommandState.runOnce);
    KNI_SAVE_BOOLEAN_FIELD(commandState, clazz, 
			  "autotest", MidpCommandState.autotest);
    KNI_SAVE_BOOLEAN_FIELD(commandState, clazz, 
			  "logoDisplayed", MidpCommandState.logoDisplayed);
    KNI_SAVE_BOOLEAN_FIELD(commandState, clazz, 
                           "removeRMS", MidpCommandState.removeRMS);

    /*
     * We need to put these in the do/while block since the SAVE_STRING
     * macros may throw an OutOfMemoryException. If this happens, we
     * must exit the native function immediately.
     */
    do {
	KNI_SAVE_STRING_FIELD(commandState, clazz, "midletClassName",
			      MidpCommandState.midletClassName, string);
	KNI_SAVE_STRING_FIELD(commandState, clazz, "midletName",
			      MidpCommandState.midletName, string);
	KNI_SAVE_STRING_FIELD(commandState, clazz, "descriptorName",
			      MidpCommandState.descriptorName, string);
	KNI_SAVE_STRING_FIELD(commandState, clazz, "suiteStorageName",
			      MidpCommandState.suiteStorageName, string);
	KNI_SAVE_STRING_FIELD(commandState, clazz, "suiteURL",
			      MidpCommandState.suiteURL, string);
	KNI_SAVE_STRING_FIELD(commandState, clazz, "runExceptionMessage",
			      MidpCommandState.runExceptionMessage, string);
	KNI_SAVE_STRING_FIELD(commandState, clazz, "securityDomain",
			      MidpCommandState.securityDomain, string);
	KNI_SAVE_STRING_FIELD(commandState, clazz, "midletNumber",
			      MidpCommandState.midletNumber, string);
    } while (0);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*
 * Copies a String from a field of an object and primitively converts
 * it to an 8 bit native string to save space.
 *
 * returns true for success, false if an exception was thrown
 */
static int copyStringField(jobject obj, jclass classObj,
        char* pszFieldName, char** ppResult, jobject fieldHandle) {
    jsize  length;
    jchar* ucodeString = NULL;
    char*  result = NULL;
    int i;

    if (*ppResult != NULL) {
        midpFree(*ppResult);
        *ppResult = NULL;
    }

    KNI_GetObjectField(obj, KNI_GetFieldID(classObj, pszFieldName,
        "Ljava/lang/String;"), fieldHandle);

    length = KNI_GetStringLength(fieldHandle);
    if (length < 0) {
        /* the field is null, done */
        return 1;
    }

    /* minimize fragmentation, alloc the result first */
    result = (char*)midpMalloc(sizeof (char) * (length + 1));
    if (NULL == result) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        return 0;
    }

    result[length] = 0;
    if (length == 0) {
        *ppResult = result;
        return 1;
    }

    ucodeString = (jchar*)midpMalloc(sizeof (jchar) * length);
    if (NULL == ucodeString) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        midpFree(result);
        return 0;
    }

    KNI_GetStringRegion(fieldHandle, 0, length, ucodeString);
    for (i = 0; i < length; i++) {
        result[i] = (char)ucodeString[i];
    }

    midpFree(ucodeString);
    *ppResult = result;
    return 1;
}

