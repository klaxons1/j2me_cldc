/*
 * @(#)midpServices.c	1.14 02/09/24 @(#)
 *
 * Copyright (c) 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/* Need to include midpServices first */
#include <midpServices.h>
#include <midpMalloc.h>

#include <jar.h>

/*=========================================================================
 * global variables
 *=======================================================================*/
long  RequestedHeapSize = MAXIMUMHEAPSIZE;      /* Defined by kVM */

/*=========================================================================
 * local variables
 *=======================================================================*/
static const char* VmUsageText = 
"    -heapsize <size> (e.g. 65536 or 128k or 1M)\n"
#if ENABLE_JAVA_DEBUGGER
"\n"
"and these debugging options:\n"
"    -debugger\n"
"    -suspend\n"
"    -nosuspend\n"
"    -port <port number>\n"
#endif /* ENABLE_JAVA_DEBUGGER */
#if INCLUDEDEBUGCODE
"\n"
"and these tracing options:\n"
"    -traceallocation\n"
"    -tracegc\n"
"    -tracegcverbose\n"
"    -traceclassloading\n"
"    -traceclassloadingverbose\n"
"    -traceverifier\n"
"    -tracestackmaps\n"
"    -tracebytecodes\n"
"    -tracemethods\n"
"    -tracemethodsverbose\n"
"    -tracestackchunks\n"
"    -traceframes\n"
"    -traceexceptions\n"
"    -traceevents\n"
"    -tracemonitors\n"
"    -tracethreading\n"
"    -tracenetworking\n"
#if ENABLE_JAVA_DEBUGGER
"    -tracedebugger\n"
#endif /* ENABLE_JAVA_DEBUGGER */
"    -traceall (activates all tracing options above)\n"
#endif /* INCLUDEDEBUGCODE */
"\n";


/*=========================================================================
 * VM helper functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      midp_exitVM
 * OVERVIEW:      Immediately terminate the VM.
 *                NOTE: This does not necessarily terminate the process.
 * INTERFACE:
 *   parameters:  status:   The return code of the VM.
 *   returns:     none
 *=======================================================================*/
void
midp_exitVM(int status) {
    ERROR_THROW(status);
}

/*=========================================================================
 * FUNCTION:      midp_parseVmArg
 * OVERVIEW:      Parse VM arguments. This method may be "greedy" and
 *                 attempt to parse as many arguments as it can find
 *                 starting at 'currentArg'. Or, it may only process
 *                 a single argument.
 * INTERFACE:
 *   parameters:  currentArg:  The index of the current argument to parse
 *                argc:        The total number of arguments
 *                argv:        An array of 'C' strings containing the
 *                              arguments
 *   returns:     jint         The number of arguments processed starting
 *                              at 'currentArg'. If no argument was
 *                              recognized, '0' is returned.
 *=======================================================================*/
jint
midp_parseVmArg(jint currentArg, jint argc, char* argv[]) {
    int startArg = currentArg;

#if ENABLE_JAVA_DEBUGGER
    /* suspend all threads on VM startup. Default is TRUE */
    suspend = KNI_TRUE;
#endif /* ENABLE_JAVA_DEBUGGER */

    if ((*argv[currentArg] == '-') &&
	       ( *(argv[currentArg]+1) == 'D')) {
	/* Configuration property override */
	char *key;
	char *value;
	/* Trace flags in emulator argument format */
	key = argv[currentArg] + 2;
	for (value = key; *value ; value++) {
	    if (*value == '=') {
		*value++ = 0;
		break;
	    }
	}
	/*
	 * Command line overrides effect internal
	 * configuration parameters.
	 */
	setInternalProp(key,value);

	/* 
	 * Commanad line overides are also exposed to
	 * System.getProperty() for backward compatability
	 * with CLDC uses of property vales.
	 */
	setSystemProp(key,value);

        currentArg++;

#if ENABLE_JAVA_DEBUGGER

    } else if (strcmp(argv[currentArg], "-debugger") == 0) {
        debuggerActive = KNI_TRUE;
        currentArg++;
    } else if (strcmp(argv[currentArg], "-suspend") == 0) {
        suspend = KNI_TRUE;
        currentArg++;
    } else if (strcmp(argv[currentArg], "-nosuspend") == 0) {
        suspend = KNI_FALSE;
        currentArg++;
    } else if (strcmp(argv[currentArg], "-port") == 0) {
        currentArg++;
        if (currentArg == argc) {
            puts("Missing port for port option");
            exit(-1);
        }
        
        debuggerPort = (short)atoi(argv[currentArg]);
        currentArg++;
#endif
#if INCLUDEDEBUGCODE
    } else if((strcmp(argv[currentArg], "-tracememoryallocation") == 0)||
	      (strcmp(argv[currentArg], "-traceallocation") == 0)) {
        tracememoryallocation = 1;
        currentArg++;    
    } else if((strcmp(argv[currentArg], "-tracegarbagecollection") == 0) ||
	       (strcmp(argv[currentArg], "-tracegc") == 0 )){
        tracegarbagecollection = 1;
        currentArg++;   
    } else if ((strcmp(argv[currentArg],
                      "-tracegarbagecollectionverbose") == 0) ||
	       (strcmp(argv[currentArg],
		       "-tracegcverbose") == 0))
    {
        tracegarbagecollectionverbose = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-traceclassloading") == 0) {
        traceclassloading = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-traceclassloadingverbose") == 0) {
        traceclassloadingverbose = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-traceverifier") == 0) {
        traceverifier = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-tracestackmaps") == 0) {
        tracestackmaps = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-tracebytecodes") == 0) {
        tracebytecodes = 1;
        currentArg++;   
    } else if((strcmp(argv[currentArg], "-tracemethodcalls") == 0)||
	      (strcmp(argv[currentArg], "-tracemethods") == 0)) {
        tracemethodcalls = 1;
        currentArg++;   
    } else if ((strcmp(argv[currentArg], "-tracemethodcallsverbose") == 0) ||
               (strcmp(argv[currentArg], "-tracemethodsverbose") == 0)) { 
        tracemethodcallsverbose = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-traceframes") == 0) {
        traceframes = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-tracestackchunks") == 0) {
        tracestackchunks = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-traceexceptions") == 0) {
        traceexceptions = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-traceevents") == 0) {
        traceevents = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-tracethreading") == 0) {
        tracethreading = 1;
        currentArg++;   
    } else if (strcmp(argv[currentArg], "-tracemonitors") == 0) {
        tracemonitors = 1;
        currentArg++;        
    } else if (strcmp(argv[currentArg], "-tracenetworking") == 0) {
        tracenetworking = 1;
        currentArg++;        
#if ENABLE_JAVA_DEBUGGER
    } else if (strcmp(argv[currentArg], "-tracedebugger") == 0) {
        tracedebugger = 1;
        currentArg++;        
#endif /* ENABLE_JAVA_DEBUGGER */
    } else if (strcmp(argv[currentArg], "-traceall") == 0) {
        tracememoryallocation = 1;
        tracegarbagecollection = 1;
        tracegarbagecollectionverbose = 1;
        traceclassloading = 1;
        traceclassloadingverbose = 1;
        traceverifier = 1;
        tracestackmaps = 1;
        tracebytecodes = 1;
        tracemethodcalls = 1;
        tracemethodcallsverbose = 1;
        tracestackchunks = 1;
        traceframes = 1;
        traceexceptions = 1;
        traceevents = 1;
        tracemonitors = 1;
        tracethreading = 1;
        tracenetworking = 1;
#if ENABLE_JAVA_DEBUGGER
        tracedebugger = 1;
#endif /* ENABLE_JAVA_DEBUGGER */

        currentArg++;
#endif
        } else if ((strcmp(argv[currentArg], "-heapsize") == 0) &&
		   (argc > 2)) {
            char *endArg;
            long heapSize ;
	    currentArg++;
	    heapSize = strtol(argv[currentArg], &endArg, 10);

            switch (*endArg) { 
                case '\0':            break;
                case 'k': case 'K':   heapSize <<= 10; break;
                case 'm': case 'M':   heapSize <<= 20; break;
                default:              midpPrintUsage(argv[0]); exit(1);
            }

            /* In principle, KVM can run with just a few kilobytes */
            /* of heap space.  We use 16 kilobytes as the minimum  */
            /* as that value is useful for some test cases.        */
            /* The maximum heap size allowed by the KVM garbage    */
            /* collector is 64 megabytes.  In practice, the        */
            /* collector is optimized only for small heaps, and    */
            /* is likely to have noticeable GC pauses with heaps   */
            /* larger than a few megabytes */
            if (heapSize < 16 * 1024) { 
                fprintf(stderr, KVM_MSG_USES_16K_MINIMUM_MEMORY "\n");
                heapSize = 16 * 1024;
            }
            if (heapSize > 64 * 1024 * 1024) {
                fprintf(stderr, KVM_MSG_USES_64M_MAXIMUM_MEMORY "\n");
                heapSize = 64 * 1024 * 1024;
            }

            /* Make sure the heap size is divisible by four */
            heapSize -= heapSize%CELL;
           
	    currentArg++;
            RequestedHeapSize = heapSize;
    }

    return (jint)(currentArg - startArg);
}

/*=========================================================================
 * FUNCTION:      midp_printVmUsage
 * OVERVIEW:      Print VM usage text.
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/
void
midp_printVmUsage(void) {
    fprintf(stdout, VmUsageText);
}


/*=========================================================================
 * FUNCTION:      midp_getCurrentTime
 * OVERVIEW:      Gets the current system time in milliseconds.
 * INTERFACE:
 *   parameters:  none
 *   returns:     jlong  returns the current system time in milliseconds
 *=======================================================================*/
jlong
midp_getCurrentTime(void) {
    return (jlong)CurrentTime_md();
}


/*=========================================================================
 * Empty stub functions
 *=======================================================================*/

/*=========================================================================
 * These are native methods required by the VM classloader.
 *=======================================================================*/
void Java_java_lang_Class_invoke_1clinit(void)   { }
void Java_java_lang_Class_invoke_1verifier(void) { }
void Java_java_lang_Class_getSuperclass(void)    { }


/*=========================================================================
 * Resource helper functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      midp_openResourceFile
 * OVERVIEW:      Open a resource file
 * INTERFACE:
 *   parameters:  resourceName:  The name of the resource to open
 *                size:          (out) The size of the resource
 *   returns:     int            A handle associated with this resource
 *=======================================================================*/
int
midp_openResourceFile(char* resourceName, int* size) {
    int fd = -1;
    FILEPOINTER fp;

    START_TEMPORARY_ROOTS

    /*
     * setFilePointer() may cause a GC, so, we need to protect the
     * FILEPOINTER Java Object
     */
    IS_TEMPORARY_ROOT(fp, openResourcefile(resourceName));

    if (fp != NULL) {
	/* Translate a FILEPOINTER to a file descriptor */
	fd = setFilePointer(&fp);
	(*size) = getBytesAvailable(&fp);
    }
    END_TEMPORARY_ROOTS

    return fd;
}

/*=========================================================================
 * FUNCTION:      midp_closeResourceFile
 * OVERVIEW:      Close a resource file
 * INTERFACE:
 *   parameters:  fd:  The handle of the resource
 *   returns:     none
 *=======================================================================*/
void
midp_closeResourceFile(int fd) {
    FILEPOINTER fp;

    START_TEMPORARY_ROOTS

    /* Translate a file descriptor to a FILEPOINTER */
    IS_TEMPORARY_ROOT(fp, getFilePointer(fd));

    if (fp != NULL) {
	closeClassfile(&fp);
	clearFilePointer(fd);
    }

    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      midp_readResourceFile
 * OVERVIEW:      Read a byte from a resource file
 * INTERFACE:
 *   parameters:  fd:  The handle of the resource
 *   returns:     int  The next byte in the specified resource file
 *=======================================================================*/
int
midp_readResourceFile(int fd) {
    FILEPOINTER fp;
    int result = -2;

    START_TEMPORARY_ROOTS

    /* Translate a file descriptor to a FILEPOINTER */
    IS_TEMPORARY_ROOT(fp, getFilePointer(fd));

    if (fp != NULL) {
        result = loadByteNoEOFCheck(&fp);
    }

    END_TEMPORARY_ROOTS

    return result;
}

/*=========================================================================
 * FUNCTION:      midp_readallResourceFile
 * OVERVIEW:      Read several bytes from a resource file
 * INTERFACE:
 *   parameters:  fd:        The handle of the resource
 *                offset:    The starting position in the resource
 *                size:      The number of bytes to read
 *                outArray:  (in|out) A byte array Object to read into.
 *                           It must be created with the appropriate size
 *                           prior to calling this method.
 *                pos;       The starting position in the array
 *   returns:     int  The number of bytes actually read
 *=======================================================================*/
int
midp_readallResourceFile(int fd, int offset, int size, jobject* outArray, 
			 int pos) {
    FILEPOINTER fp;
    int result = -2;

    START_TEMPORARY_ROOTS

    /* Translate a file descriptor to a FILEPOINTER */
    IS_TEMPORARY_ROOT(fp, getFilePointer(fd));

    if (fp != NULL) {
	/* Unhandle the jobject into kVM's native datatype */
	BYTEARRAY barray = (BYTEARRAY)*((jobject)(*outArray));
	result = loadBytesNoEOFCheck(&fp, (char*)(&barray->bdata[pos]), 
				     offset, size);
    }

    END_TEMPORARY_ROOTS

    return result;
}


/*=========================================================================
 * Compression helper functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      midp_pngInflate
 * OVERVIEW:      Decompress a PNG image data stream
 * INTERFACE:
 *   parameters:  data:       The raw data stream
 *                compLen:    The compressed length of the stream
 *                outFileH:   (in|out) A buffer to store decompressed data.
 *                            It must be created with the appropriate size
 *                            prior to calling this method.
 *                decompLen:  The decompressed length
 *   returns:     jboolean:   TRUE if decompression occured without error
 *                            FALSE otherwise
 *=======================================================================*/
extern unsigned int PNGdecodeImage_getBytes(unsigned char *buffer, 
                                            int count, void *p);
jboolean
midp_pngInflate(void *data, int compLen, unsigned char** outFileH, 
		int decompLen) {
    return inflateData(data, PNGdecodeImage_getBytes, compLen, outFileH, 
		       decompLen);
}


/*=========================================================================
 * JAR helper functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      midp_readJarEntry
 * OVERVIEW:      Read an entry from a JAR file
 * INTERFACE:
 *   parameters:  jarName:    The JAR file to read
 *                entryName:  The name of the entry
 *                entry:      (out) An Object representing the entry,
 *                            must be set to null on input, will set to
 *                            java byte array if the entry was read
 *   returns:     jboolean:   TRUE if the entry was read or not found;
 *                            FALSE otherwise (such as JAR not found)
 *=======================================================================*/
jboolean
midp_readJarEntry(char* jarName, char* entryName, jobject* entry) {
    struct jarInfoStruct jarInfo;
           long          entryLength;
	   int           error = 0;

    /* WARNING: loadJARFileEntry() may cause an allocation. A couple levels
     * down the call-stack, we potentially call CLDC's implementation
     * of inflateHuffman() which is where an allocation can happen. We
     * need to protect any Java-heap allocations with temporary roots.
     */
    START_TEMPORARY_ROOTS

    DECLARE_TEMPORARY_ROOT(unsigned char*, buf, NULL);

    /* openJARFile returns TRUE on success */
    if (openJARFile(jarName, 0, &jarInfo)) {
	buf = (unsigned char*) loadJARFileEntry(&jarInfo, entryName, 
						&entryLength, 0);    
	closeJARFile(&jarInfo);
    } else {
	error = 1;
    }
    
    if (buf != NULL) {
	DECLARE_TEMPORARY_ROOT(BYTEARRAY, dataObj, 
			       (BYTEARRAY)instantiateArray(PrimitiveArrayClasses[T_BYTE], 
						entryLength));
	if (dataObj != NULL) {
	    jobject handle = (jobject)(*entry);

	    memcpy(dataObj->bdata, buf, (int)entryLength);

	    /* Unhandle the jobject and store our newly created Java object */
	    *((cell*)handle) = (cell)dataObj;
	} else {
	    error = 1;
	}
    }

    END_TEMPORARY_ROOTS

    return ((error == 0) ? KNI_TRUE : KNI_FALSE);
}



