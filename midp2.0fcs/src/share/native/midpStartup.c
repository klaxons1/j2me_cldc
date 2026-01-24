/*
 * @(#)midpStartup.c	1.15 02/10/14 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * Virtual Machine
 *=========================================================================
 * SYSTEM:    MIDP native
 * SUBSYSTEM: Main program
 * FILE:      midpStartup.c
 * OVERVIEW:  Utilities required to start the MIDP
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#include <kni.h>

#include "midpMalloc.h"
#include "midpServices.h"
#include "commandState.h"
#include "storage.h"


/*=========================================================================
 * global variables
 *=======================================================================*/
static char* MidpCommandLineClassPath;


/*=========================================================================
 * local variables
 *=======================================================================*/
static const char* AppJarFile = "suite.jar";
/* value "suites.utf" is from com.sun.midp.midletsuite.Installer */
static const char* SuitesFile = "suites.utf";
static const char* MidpUsageText =
"\n"
"Usage: midp [<options>]\n"
"         Run the Graphical MIDlet Suite Manager.\n"
"\n"
"   or  midp [<options>] -autotest [-domain <dname>] <descriptor URL>\n"
"         Install, and run MIDlet number 1, and remove a MIDlet repeatedly.\n"
"         Overwrite the existing suite and removing the RMS data if \n"
"         necessary. Stop and remove the previously installed test suite\n"
"         when a 404 code is received from the server. Use the maximum\n"
"         security domain when installing unsigned suites so that all\n"
"         permissions are allowed, but not requested.\n"
"          -domain: security domain to use for installing unsigned suites\n"
"\n"
"   or  midp [<options>] -transient [-force] [-domain <dname>] [-removeRMS]\n"
"           <descriptor URL> [<MIDlet name>]\n"
"         Install, run, and remove a MIDlet.\n"
"          -force: Force the installed version to be overwritten\n"
"          -domain: security domain to use for installing unsigned suites\n"
"          -removeRMS: Remove the RMS data when overwriting a suite\n"
"\n"
"   or  midp [<options>] -install [-force] [-domain <dname>] [-removeRMS]\n"
"           <descriptor URL>\n"
"         Install a MIDlet suite using the URL of a descriptor.\n"
"          -force: Force the installed version to be overwritten\n"
"          -domain: security domain to use for unsigned suites\n"
"          -removeRMS: Remove the RMS data when overwriting a suite\n"
"\n"
"   or  midp [<options>] -run (<suite number> | <storage name>)\n"
"           [<name of MIDlet to run>]\n"
"         Run a MIDlet of an installed suite. If the name or number of the \n"
"         MIDlet is not provided and the suite has multiple MIDlets,\n"
"         the user will be prompted to select a MIDlet from the\n"
"         suite to run.\n"
"\n"
"   or  midp [<options>] -remove (<suite number> | <storage name> | all)\n"
"         Remove an installed MIDlet suite or all suites.\n"
#ifdef INTERNAL_BUILD
"         To clean up RMS space used when running a MIDlet from the \n"
"         classpath, use the storage name \"run_by_class_storage_\".\n"
"\n"
"   or  midp [<options>] [-domain <dname>] [-Xdescriptor <filename>] <class>\n"
"         Run a MIDlet from the classpath.\n"
"          -domain: run under a different security domain than untrusted\n"
"          -Xdescriptor: Get the MIDlet properties from this file\n"
"\n"
"   or  midp [<options>] [-domain <dname>] -Xdescriptor <filename>\n"
"         Select a MIDlet to run from a local MIDlet suite descriptor.\n"
"          -domain: run under a different security domain than untrusted\n"
"          -Xdescriptor: Display a selection of MIDlets from this file\n"
#endif /* INTERNAL_BUILD */
"\n"
"   or  midp [<options>] -list\n"
"         List the installed MIDlet suites by number and name.\n"
"\n"
"   or  midp [<options>] -storageNames\n"
"         List the unique storage names of the installed MIDlet suites,\n"
"         one per line.\n"
"\n"
"   or  midp -version\n"
"         Show version information and exit.\n"
"\n"
"   or  midp -help\n"
"         Show this message and exit.\n"
"\n"
"where <suite number> is the number of a suite as displayed by the list\n"
"command,\n"
"\n"
"<storage name> is the unique name a suite is stored by,\n"
"\n"
"<dname> is the name of a domain in the security policy file,\n"
"\n"
"and <options> include:\n"
"    -classpath <path>  directories and zip files to search for classes\n"
"    -D<property>=<value> Override configuration file settings\n"
"";


/*=========================================================================
 * function prototypes
 *=======================================================================*/
char* midpFixMidpHome(char* cmd);
void  midpParseArgs(int argc, char* argv[]);
int   midpSetClassPath(char* storageName, char** userClassPath);
void  midpPrintUsage(char* progname);
static char* buildJarPath(char* suiteStorageName);
static char* getStorageNameFromNumber(char* numberString);
static char* readSuiteStorageName(char** ppszError, int handle);


/*=========================================================================
 * FUNCTION:      midpFixMidpHome
 * OVERVIEW:      Fixup routine for MIDP_HOME directory when the midp
 *                executable is invoked from some other current working
 *                directory.
 *
 *                The search for the "lib" directory is in the order of:
 *                    - current directory (in case midp is in the PATH
 *                      and the current directory is the right place)
 *                    - parent directory of the midp executable
 *                    - grandparent directory of the midp executable
 *
 *                If 'cmd' does not contain any directory (i.e. just
 *                "midp"), the search starts from the current directory.
 *                Otherwise, it starts from the directory that midp is
 *                located (i.e. "bin" if 'cmd' is "bin/midp").
 * INTERFACE:
 *   parameters:  cmd:        'C' string of the command used to start
 *                              the MIDP.
 *   returns:     'C' string:  A pointer to the "found" MIDP directory.
 *=======================================================================*/
char*
midpFixMidpHome(char *cmd) {
    int   i = 0;
    char* filesep;
    char* lastsep;
    char* midp_home;
    char  libdir[MAX_FILENAME_LENGTH+1]; /* temp buffer to the lib dir */
    struct stat statbuf;

    /*
     * If MIDP_HOME is set, just use it. Does not check if MIDP_HOME is
     * pointing to a directory contain "lib".
     */
    if (!getenv("MIDP_HOME")) {
        filesep = getFileSeparator();
        /* Look for the last slash in the pathanme. */
    if ((lastsep = strrchr(cmd, (int) *filesep)) != 0) {
            *lastsep = '\0';
        }

        /* start from the current directory */
        sprintf(libdir, "%s%c.%clib", lastsep ? cmd : ".", *filesep, *filesep);

        /* try to search for "lib" 3 times only (see above) */
        while (i < 3) {
            /* found it and it is a directory */
            if (!stat(libdir, &statbuf) && (statbuf.st_mode & S_IFDIR)) {
                break;
            }

            /* strip off "lib" to add 1 more level of ".." */
            *(strrchr(libdir, (int) *filesep)) = '\0';
            strcat(libdir, filesep);
            strcat(libdir, "..");
            strcat(libdir, filesep);
            strcat(libdir, "lib");

            i++;
        }

        /* tried 3 times, and still cannot find "lib" */
        if (i == 3) {
            fprintf(stderr, "Warning: cannot find lib subdirectory.\n"
                    "Please specify MIDP_HOME environment variable such "
                    "that $MIDP_HOME%clib contains the proper configuration "
                    "files.\n", *filesep);

            /* assume the default to the be the current directory */
            sprintf(libdir, ".%clib", *filesep);
        }

        /* save the parent directory and strip off "lib" from the path */
        midp_home = midpStrdup(libdir);
        if (midp_home == NULL) {
            fprintf(stderr, "Out of memory: could not allocate midp_home.\n");
            exit(-1);
        }
        *(strrchr(midp_home, (int) *filesep)) = '\0';

        if (lastsep != 0) {
            /* restore the last file separator for cmd */
            *lastsep = *filesep;
        }
    } else {
        /*
         * make copy of MIDP_HOME env var because we want to call
         * midpFree() on midp_home, but we cannot do that on the
         * pointer returned by getenv(). And we do not know whether
         * the memory pointed to by midp_home is from getenv() or
         * midpStrdup() in main().
         */
        midp_home = midpStrdup(getenv("MIDP_HOME"));
        if (midp_home == NULL) {
            fprintf(stderr, "Out of memory: could not allocate midp_home.\n");
            exit(-1);
        }
    }

    return midp_home;
}


/*=========================================================================
 * FUNCTION:      midpParseArgs
 * OVERVIEW:      Process command-line arguments. Any arguments not
 *                recognized as MIDP arguments will be processed as
 *                VM arguments.
 *
 *                If an unknown argument or invalid parameter is
 *                encountered, usage text will be printed to stdout
 *                and the process will be terminated.
 * INTERFACE:
 *   parameters:  argc:        The total number of arguments
 *                argv:        An array of 'C' strings containing the
 *                              arguments
 *   returns:     none
 *=======================================================================*/
void
midpParseArgs(int argc, char* argv[]) {
    char* progName = NULL;
    char** ppTarget; /* where to put the target of the command */
    int currentArg;

    /* Save program name for showing the usage */
    if (argc > 0) {
    progName = argv[0];
    }

    memset(&MidpCommandState, 0, sizeof(MidpCommandState));

    /* no options means, run a MIDlet class from ROM or the classpath */
    MidpCommandState.initialCommand = RUN_CLASS;
    ppTarget = &MidpCommandState.midletClassName;

    for (currentArg = 1; currentArg < argc; currentArg++) {
        if (strcmp(argv[currentArg], "-help") == 0) {
            midpPrintUsage(progName);
            exit(0);
        } else if (strcmp(argv[currentArg], "-version") == 0) {
        fprintf(stdout, "Profile Spec : %s\n",
            getInternalProp("microedition.profiles"));
        fprintf(stdout, "Profile Impl : %s %s\n",
            IMPL_VERSION, RELEASE);
        fprintf(stdout, "Configuration: %s\n",
            getInternalProp("microedition.configuration"));
            exit(0);
        } else if (strcmp(argv[currentArg], "-transient") == 0) {
            MidpCommandState.initialCommand = INSTALL_RUN;
            ppTarget = &MidpCommandState.suiteURL;
            MidpCommandState.runOnce = KNI_TRUE;
        } else if (strcmp(argv[currentArg], "-autotest") == 0) {
            MidpCommandState.initialCommand = INSTALL_RUN;
            ppTarget = &MidpCommandState.suiteURL;
            MidpCommandState.removeRMS = KNI_TRUE;
            MidpCommandState.forceOverwrite = KNI_TRUE;
            MidpCommandState.autotest = KNI_TRUE;

            /* domain could have already been set */
            if (NULL == MidpCommandState.securityDomain) {
                MidpCommandState.securityDomain = midpStrdup("maximum");
                if (MidpCommandState.securityDomain == NULL) {
                    puts("Out of Memory: could not allocate security domain");
                    exit(-1);
                }
            }

            MidpCommandState.midletNumber = midpStrdup("1");
            if (MidpCommandState.midletNumber == NULL) {
                puts("Out of Memory: could not allocate midlet number");
                exit(-1);
            }
        } else if (strcmp(argv[currentArg], "-classpath") == 0) {
            currentArg++;
            if (currentArg == argc) {
                puts("Missing classpath for classpath option");
                exit(-1);
            }

            MidpCommandLineClassPath = argv[currentArg];
        } else if (strcmp(argv[currentArg], "-Xdescriptor") == 0 ||
                   strcmp(argv[currentArg], "-descriptor") == 0) {
            currentArg++;
            if (currentArg == argc) {
                printf("Missing filename for %s option\n",
                       argv[currentArg - 1]);
                exit(-1);
            }

            MidpCommandState.descriptorName = midpStrdup(argv[currentArg]);
            if (MidpCommandState.descriptorName == NULL) {
                puts("Out of Memory: could not allocate descriptor name");
                exit(-1);
            }
        } else if (strcmp(argv[currentArg], "-domain") == 0) {
            currentArg++;
            if (currentArg == argc) {
                puts("Missing name for security domain option");
                exit(-1);
            }

            MidpCommandState.securityDomain = midpStrdup(argv[currentArg]);
            if (MidpCommandState.securityDomain == NULL) {
                puts("Out of Memory: could not allocate security domain");
                exit(-1);
            }
        } else if (strcmp(argv[currentArg], "-install") == 0) {
            MidpCommandState.initialCommand = INSTALL;
            ppTarget = &MidpCommandState.suiteURL;
        } else if (strcmp(argv[currentArg], "-force") == 0) {
            MidpCommandState.forceOverwrite = KNI_TRUE;
        } else if (strcmp(argv[currentArg], "-removeRMS") == 0) {
            MidpCommandState.removeRMS = KNI_TRUE;
        } else if (strcmp(argv[currentArg], "-run") == 0) {
            MidpCommandState.initialCommand = RUN;
            ppTarget = &MidpCommandState.suiteStorageName;
        } else if (strcmp(argv[currentArg], "-remove") == 0) {
            MidpCommandState.initialCommand = REMOVE;
            ppTarget = &MidpCommandState.suiteStorageName;
        } else if (strcmp(argv[currentArg], "-list") == 0) {
            MidpCommandState.initialCommand = LIST;
        } else if (strcmp(argv[currentArg], "-storageNames") == 0) {
            MidpCommandState.initialCommand = STORAGE_NAMES;
        } else if ((*argv[currentArg] == '-')
           || (*argv[currentArg] == '+')
           || (*argv[currentArg] == '=')) {
            int cldcArgs;

            /* may be a CLDC arg */
            cldcArgs = midp_parseVmArg(currentArg, argc, argv);
            if (cldcArgs == 0) {
                /*
                 * Not a CLDC arg, presume it to be an invalid argument
                 */
                fprintf(stdout, "Invalid argument \"%s\"\n\n",
                        argv[currentArg]);
                midpPrintUsage(progName);
                exit(-1);
            } else {
                /*
                 * Increase currentArg by the number of arguments processed
         * by CLDC. currentArg will be incremented as part of the
         * loop, so, we increment by cldcArgs - 1.
                 */
                currentArg += (cldcArgs - 1);
            }
        } else if (NULL == *ppTarget) {
            *ppTarget = midpStrdup(argv[currentArg]);
            if (*ppTarget == NULL) {
                puts("Out of Memory: could not allocate command target");
                exit(-1);
            }

        } else if (NULL == MidpCommandState.midletName) {
            /*
             * assume if there is a non-option parameter after the target
             *  it is a name of a MIDlet
             */
            MidpCommandState.midletName = midpStrdup(argv[currentArg]);
            if (MidpCommandState.midletName == NULL) {
                puts("Out of Memory: could not allocate midlet name");
                exit(-1);
            }
        }
    }

    if ((MidpCommandState.initialCommand == INSTALL_RUN ||
      MidpCommandState.initialCommand == INSTALL) &&
     *ppTarget == NULL) {
    fprintf(stdout, "Must specify URL of .jad file\n");
    exit(-1);
    }

    if (MidpCommandState.initialCommand == RUN_CLASS &&
            MidpCommandState.midletClassName == NULL &&
            MidpCommandState.descriptorName == NULL) {
        /*
         * Running with no target and no descriptor,
         * means to run the Graphical Manager
         */
        MidpCommandState.initialCommand = MANAGE;
    }

    MidpCommandState.nextCommand = MidpCommandState.initialCommand;

#if ENABLE_JAVA_DEBUGGER
    if (debuggerActive && MidpCommandState.autotest) {
        fprintf(stderr,
                "Can't combine -debugger with -autotest option");
        exit(-1);
    }
#endif

    /* convert the midlet suite number if any, to storage name */
    if (MidpCommandState.suiteStorageName != NULL) {
        int onlyDigits;
        int len;
        int i;

        /* if the storage name only digits, convert it */
        onlyDigits = 1;
        len = strlen(MidpCommandState.suiteStorageName);
        for (i = 0; i < len; i++) {
            if (!isdigit(MidpCommandState.suiteStorageName[i])) {
                onlyDigits = 0;
                break;
            }
        }

        if (onlyDigits) {
            char* temp = MidpCommandState.suiteStorageName;

            MidpCommandState.suiteStorageName = getStorageNameFromNumber(temp);
            midpFree(temp);
        }
    }
}


/*=========================================================================
 * FUNCTION:      midpSetClassPath
 * OVERVIEW:      Generate a complete classpath in preparation to
 *                 run the MIDlet Suite stored in 'storageName'
 * INTERFACE:
 *   parameters:  storageName:    'C' string of the MIDlet Suite storage
 *                                 name.
 *                userClassPath:  (out) The classpath to be used to
 *                                 to execute the MIDlet Suite.
 *   returns:     int:            '0' if the classpath was generated
 *                                 without errors. '-1' if the system
 *                                 has run out of memory.
 *=======================================================================*/
int
midpSetClassPath(char* storageName, char** userClassPath) {
    int newPathLen = 0;
    char* newPath = NULL;
    char* suitePath = NULL;

    if (*userClassPath != NULL) {
        midpFree(*userClassPath);
        *userClassPath = NULL;
    }

    if (storageName != NULL) {
        suitePath = buildJarPath(storageName);
        if (NULL == suitePath) {
            return -1;
        }
    }

    /* the base classpath may have been specified */
    if (MidpCommandLineClassPath != NULL) {
        newPathLen += strlen(MidpCommandLineClassPath) + 1;
    }

    if (suitePath != NULL) {
        newPathLen += strlen(suitePath) + 1;
    }

    if (0 == newPathLen) {
        *userClassPath = (char*)midpStrdup(getStorageRoot());
        return 0;
    }

    newPath = (char*)midpMalloc(newPathLen);
    if (NULL == newPath) {
        midpFree(suitePath);
        return -1;
    }

    newPath[0] = 0;

    if (MidpCommandLineClassPath != NULL) {
        strcat(newPath,  MidpCommandLineClassPath);
    }

    if (suitePath != NULL) {
        if (newPath[0] != 0) {
            strcat(newPath, getPathSeparator());
        }

        strcat(newPath, suitePath);
        midpFree(suitePath);
    }

    *userClassPath = newPath;
    return 0;
}


/*=========================================================================
 * FUNCTION:      midpPrintUsage
 * OVERVIEW:      Print usage information for MIDP. Usage information
 *                includes command-line arguments for both the MIDP and
 *                the underlying VM.
 * INTERFACE:
 *   parameters:  progname:  A 'C' string containing the name of the
 *                            MIDP executable.
 *   returns:     none
 *=======================================================================*/
void
midpPrintUsage(char *progname) {
    fprintf(stdout, MidpUsageText, progname);
    midp_printVmUsage();
}


/*=========================================================================
 * FUNCTION:      buildJarPath
 * OVERVIEW:      Build an absolute pathname to the JAR file for a given
 *                 MIDlet suite.
 * INTERFACE:
 *   parameters:  suiteStorageName:  'C' string of the MIDlet Suite.
 *   returns:     'C' string:        A 'C' string of the absolute path
 *                                    to the JAR file. NULL if there
 *                                    was an out of memory condition.
 *=======================================================================*/
static char*
buildJarPath(char* suiteStorageName) {
    char* path;
    int pathLen = 0;

    pathLen += strlen(getStorageRoot());
    pathLen += strlen(suiteStorageName);
    pathLen += strlen(AppJarFile);
    pathLen += 1; /* Add one for trailing NULL */

    path = (char*)midpMalloc(pathLen);
    if (NULL == path) {
        return NULL;
    }

    /* see MidletSuiteImpl.java for the correct separator */
    sprintf(path, "%s%s%s", getStorageRoot(), suiteStorageName,
            AppJarFile);
    return path;
}


/*=========================================================================
 * FUNCTION:      getStorageNameFromNumber
 * OVERVIEW:      Convert a number to a MIDlet Suite storage name. The
 *                returned pointer must be de-allocated by the caller.
 * INTERFACE:
 *   parameters:  numberString:  A 'C' string containing the number
 *                                to convert.
 *   returns:     'C' string:    A 'C' string with the MIDlet Suite
 *                                storage name.
 *=======================================================================*/
static char*
getStorageNameFromNumber(char* numberString) {
    int numberOfSuite;
    char filename[MAX_FILENAME_LENGTH + 1];
    char* pszError;
    int handle;
    char* previousName;
    char* name;
    int i;

    /* the format of the string is "number:" */
    if (sscanf(numberString, "%d", &numberOfSuite) != 1) {
            fprintf(stderr,
                    "Error getting storage name: invalid number format\n");
        exit(-1);
        return NULL;
    }

    /* the storage root will have any needed file separators */
    strcpy(filename, getStorageRoot());

    /* "suites.utf" is from com.sun.midp.midletsuite.Installer */
    strcat(filename, SuitesFile);

    handle = storageOpen(&pszError, filename, OPEN_READ);
    if (pszError != NULL) {
        fprintf(stderr, "Error getting storage name: %s\n", pszError);
        storageFreeError(pszError);
        exit(-1);
        return NULL;
    }

    previousName = NULL;
    for (i = 0; i < numberOfSuite; i++) {
        name = readSuiteStorageName(&pszError, handle);
        if (pszError != NULL) {
            fprintf(stderr, "Error getting storage name: %s\n", pszError);
            storageFreeError(pszError);
            exit(-1);
            return NULL;
        }

        if (name == NULL) {
            fprintf(stderr, "Could not find suite number %d.\n",
                    numberOfSuite);
            exit(-1);
            return NULL;
        }

        if (previousName != NULL) {
            midpFree(previousName);
        }

        previousName = name;
    }
    storageClose(&pszError, handle);
    return previousName;
}


/*=========================================================================
 * FUNCTION:      readSuiteStorageName
 * OVERVIEW:      Read a single storage name from a file containing a
 *                list of UTF-8 encoded storage names.
 * INTERFACE:
 *   parameters:  ppszError:   (out) 'C' string error message, if an error
 *                              occured while reading.
 *                handle:      The handle of the file to read
 *   returns:     'C' string:  The storage name, or, NULL if an error
 *                              occured while reading. If an error
 *                              occored, '*ppszError' will be non-NULL.
 *=======================================================================*/
static char*
readSuiteStorageName(char** ppszError, int handle) {
    unsigned char msb;
    unsigned int length;
    unsigned char lsb;
    char* name;

    /* string format is: msb of length, lsb of length, length of bytes */

    if (storageRead(ppszError, handle, (char*)&msb, 1) != 1) {
        return NULL;
    }

    length = msb * 256;

    if (storageRead(ppszError, handle, (char*)&lsb, 1) != 1) {
        return NULL;
    }

    length += lsb;

    name = (char*)midpMalloc(length + 1);
    if (name == NULL) {
        return NULL;
    }

    /*
     * storage names only contain ASCII characters so no conversion is
     * required
     */
    if (storageRead(ppszError, handle, name, length) != length) {
        midpFree(name);
        return NULL;
    }

    name[length] = 0;
    return name;
}

/*=========================================================================
 * FUNCTION:      checkRestrictedPackageNameWithLen
 * OVERVIEW:      Checks in the requested package name is one of the MIDP
 *                protected namespace. This is used by the CLDC class loader
 *                to disallow application from creating Java class in the
 *                protected namespace as described in the CLDC specification.
 * INTERFACE:
 *   parameters:  packagename: The requested package name without the 
 *                             trailing '/' (may not be null terminated)
 *                len:         Length of the package name.
 *   returns:     boolean:     true if packagename is one of the protected
 *                             package namespace in MIDP. False otherwise
 *=======================================================================*/
jboolean checkRestrictedPackageNameWithLen(const char *packagename, int len)
{
  /*
   * quick smoked test for protected package name. This reduces the number
   * of strncmp() against the protected list.
   * - if package name len is < 4
   * - if package name does not start with "java"
   * - if package name does not start with "com/sun"
   */
  if ((len < 4) ||
      ((strncmp(packagename, "java", 4) != 0) &&
       ((len >= 7) && (strncmp(packagename, "com/sun", 7) != 0)))) {
    return KNI_FALSE;
  }

  /*
   * Restrict classes and subpackage in the "java" and "javax" namespaces
   * - classes will have "java" or "javax" as the package name
   * - subpackage will have "java/" or "javax/" prefix
   */
  if (((len >= 4) && (strncmp(packagename, "java", 4) == 0) &&
       ((len == 4) || (packagename[4] == '/'))) ||
      ((len >= 5) && (strncmp(packagename, "javax", 5) == 0) &&
       ((len == 5) || (packagename[5] == '/')))) {
    return KNI_TRUE;
  } 
  /*
   * Restrict classes and subpackage in the "com/sun/cldc/i18n",
   * "com/sun/cldc/io", & "com/sun/cldc/util" package namespaces
   *
   * Note: classes or other subpackage in "com/sun/cldc" is allowed. 
   * The code is more complicated due to this reason, instead of just
   * protecting the entire com/sun/cldc namespace.
   */
  else if ((len >= 12) && (strncmp(packagename, "com/sun/cldc", 12) == 0)) {
    if ((len != 12) &&
        (((len >= 17) && (strncmp(packagename+12, "/i18n", 5) == 0) &&
          ((len == 17) || (packagename[17] == '/'))) ||
         ((len >= 15) && (strncmp(packagename+12, "/io", 3) == 0) &&
          ((len == 15) || (packagename[15] == '/'))) ||
         ((len >= 17) && (strncmp(packagename+12, "/util", 5) == 0) &&
          ((len == 17) || (packagename[17] == '/'))))) {
      return KNI_TRUE;
    }
  } 
  /*
   * Restrict classes and subpackage in the 
   * "com/sun/midp/dev",
   * "com/sun/midp/io", 
   * "com/sun/midp/lcdui", 
   * "com/sun/midp/main", 
   * "com/sun/midp/midlet", 
   * "com/sun/midp/midletsuite", 
   * "com/sun/midp/perfmon", 
   * "com/sun/midp/publickeystore", 
   * "com/sun/midp/rm", 
   * "com/sun/midp/security", 
   * & "com/sun/midp/ssl" package namespaces
   *
   * Note: classes or other subpackage in "com/sun/midp" is allowed. 
   * The code is more complicated due to this reason, instead of just
   * protecting the entire com/sun/midp namespace.
   */
  else if ((len >= 12) && (strncmp(packagename, "com/sun/midp", 12) == 0)) {
    if ((len !=12) &&
        (((len >= 16) && (strncmp(packagename+12, "/dev", 4) == 0) &&
          ((len == 16) || (packagename[16] == '/'))) ||
         ((len >= 15) && (strncmp(packagename+12, "/io", 3) == 0) &&
          ((len == 15) || (packagename[15] == '/'))) ||
         ((len >= 18) && (strncmp(packagename+12, "/lcdui", 6) == 0) &&
          ((len == 18) || (packagename[18] == '/'))) ||
         ((len >= 17) && (strncmp(packagename+12, "/main", 6) == 0) &&
          ((len == 17) || (packagename[17] == '/'))) ||
         ((len >= 19) && (strncmp(packagename+12, "/midlet", 7) == 0) &&
          ((len == 19) || (packagename[19] == '/'))) ||
         ((len >= 24) && (strncmp(packagename+12, "/midletsuite", 12) == 0) &&
          ((len == 24) || (packagename[24] == '/'))) ||
         ((len >= 20) && (strncmp(packagename+12, "/perfmon", 8) == 0) &&
          ((len == 20) || (packagename[20] == '/'))) ||
         ((len >= 27) && (strncmp(packagename+12, "/publickeystore", 15) == 0) &&
          ((len == 27) || (packagename[27] == '/'))) ||
         ((len >= 16) && (strncmp(packagename+12, "/rms", 4) == 0) &&
          ((len == 16) || (packagename[16] == '/'))) ||
         ((len >= 21) && (strncmp(packagename+12, "/security", 9) == 0) &&
          ((len == 21) || (packagename[21] == '/'))) ||
         ((len >= 16) && (strncmp(packagename+12, "/ssl", 4) == 0) &&
          ((len == 16) || (packagename[16] == '/'))))) {
	return KNI_TRUE;
    }
  } 
  /*
   * Restrict classes and subpackage in the "com/sun/mmedia" namespace
   * - classes will have "com/sun/mmedia" as the package name
   * - subpackage will have "com/sun/mmedia/" prefix
   */
  else if ((len >= 14) && 
           (strncmp(packagename, "com/sun/mmedia", 14) == 0) &&
           ((len == 14) || (packagename[14] == '/'))) {
    return KNI_TRUE;
  }

  /* does not match any of the protected namespace */
  return KNI_FALSE;
}

/*=========================================================================
 * FUNCTION:      checkRestrictedPackageName
 * OVERVIEW:      Checks in the requested package name is one of the MIDP
 *                protected namespace. This is used by the CLDC class loader
 *                to disallow application from creating Java class in the
 *                protected namespace as described in the CLDC specification.
 * INTERFACE:
 *   parameters:  packagename: The requested package name without the 
 *                             trailing '/'
 *                             (null terminated)
 *   returns:     boolean:     true is packagename is one of the protected
 *                             package namespace in MIDP. False otherwise
 *=======================================================================*/
jboolean checkRestrictedPackageName(const char *packagename) 
{
  return checkRestrictedPackageNameWithLen(packagename, strlen(packagename));
}

