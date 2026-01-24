/*
 * @(#)storage.c	1.29 02/07/24 @(#)
 *
 * Copyright (c) 1998-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * MIDP Reference implementation
 *=========================================================================
 * SYSTEM:    MIDP RI
 * SUBSYSTEM: File Storage.
 * FILE:      storage.c
 * OVERVIEW:  Manage storage for internal API's. See storage.h
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <kni.h>
#include <midpMalloc.h>
#include <storage.h>
#include <configuration.h>

static void ensureStorageRoot(char** ppszError);
static void initTotalSpace();
static int getUsedSpace();
static char* getLastError(char* pszFunction);
static char* getLastFileError(char* pszFunction, char* pszFile);

#ifndef _S_IREAD
#define _S_IREAD 0444
#endif

#ifndef _S_IWRITE
#define _S_IWRITE 0220
#endif

/* O_BINARY not defined in sys/fcntl.h on solaris platform */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/*
 * Name of the storage directory.
 */
#define APP_DIR "appdb"

#ifdef UNIX
#   include <dirent.h>

#   define MKDIR_MODE S_IRWXU|S_IRWXG|S_IRWXO
#   define MKDIR(x) mkdir(x, MKDIR_MODE)
#   define COMMIT(x)
#   define TRUNCATE_FILE(x,y) ftruncate(x,y)

    static char *FILESEP = "/";
    static char *PATHSEP = ":";
#else
    /*
     * Use compatibility functions defined in MIDP. Because of name conflicts
     * between global.h and windows.h, we repeat the essential stuff defined
     * in the file MIDP/src/win32/native/dirent.h here.
     */

    /* begin dirent.h digest */
    struct dirent {
        char d_name[1];
    };

    typedef struct {
        char internal_data[1];
    } DIR;

    DIR *opendir(const char *dirname);
    struct dirent *readdir(DIR *dirp);
    int closedir(DIR *dirp);
    /* end dirent.h digest */

#   define MKDIR(x) _mkdir(x)
#   define COMMIT(x) _commit(x)
#   define TRUNCATE_FILE(x,y) _chsize(x,y)

    static char *FILESEP = "\\";
    static char *PATHSEP = ";";
#endif /* UNIX */

#ifdef DEBUG_PRINT
#   define DEBUGP(x)  printf x
#else
#   define DEBUGP(x)  
#endif /*DEBUG_PRINT*/

/* Local variables */
#define DEFAULT_TOTAL_SPACE (4 * 1024 * 1024) /* 4 Meg. */
static int totalSpace = 0;
static char *systemDirAndSep;
static char *systemDir;

#define MAX_ERROR_LEN 159
static char errorBuffer[MAX_ERROR_LEN + 1] = {0};

char* getFileSeparator() {
    return FILESEP;
}

/*
 * The function is provided for development platforms so we can manage
 * local files outside of the RI simulated storage.
 */
char* getPathSeparator() {
    return PATHSEP;
}

/*
 * Prefixing the system directory for storage, APP_DIR, with midp_home.
 *
 * "+1" is for null terminator.
 */
void initializeStorage(char *midp_home) {
    systemDirAndSep = (char *) midpMalloc(strlen(midp_home) + 2*strlen(FILESEP) + strlen(APP_DIR) + 1);
    systemDir = (char *) midpMalloc(strlen(midp_home) + strlen(FILESEP) + strlen(APP_DIR) + 1);

    /*
     * If no memory can be allocated, exit.
     */
    if (!systemDirAndSep || !systemDir) {
        fprintf(stderr, "Error: out of memory.\n");
        exit(-1);
    }

    strcpy(systemDir, midp_home);
    strcat(systemDir, FILESEP);
    strcat(systemDir, APP_DIR);

    strcpy(systemDirAndSep, systemDir);
    strcat(systemDirAndSep, FILESEP);
}

/*
 * free native resources.
 */
void finalizeStorage() {
    midpFree(systemDirAndSep);
    midpFree(systemDir);
}

char* getStorageRoot() {
    return systemDirAndSep;
}

void storageFreeError(char* pszError) {
    /*
     * This is a place holder, so that an implemenation, like a debuggable
     * one can create dynamic error messages.
     */
    if (NULL == pszError) {
        return;
    }
}

/*
 * Creates the storage root directory of a file if nessecary.
 */
static void ensureStorageRoot(char** ppszError) {
    struct stat stat_buf;

    *ppszError = NULL;
    if (0 == stat(systemDir, &stat_buf)) {
        DEBUGP(("storage root %s exists\n", systemDir));
        /* dir exists */
        return;
    }

    DEBUGP(("creating storage root %s\n", systemDir));

    if (MKDIR(systemDir) < 0) {
        *ppszError = getLastError("ensureStorageRoot");
        return;
    }
}

int storageOpen(char** ppszError, char* pszAsciiFilename, int ioMode) {
    int flags = O_BINARY;
    int creationMode = 0;
    int handle;

    *ppszError = NULL;
    if (strlen(pszAsciiFilename) > MAX_FILENAME_LENGTH) {
        *ppszError = "filename too long";
        return -1;
    }

    if (OPEN_READ == ioMode) {
        DEBUGP(("opening for read only %s\n", pszAsciiFilename));
        flags |= O_RDONLY;
    } else {
        ensureStorageRoot(ppszError);
        if (*ppszError != NULL) {
            return -1;
        }

        if (!storageFileExists(pszAsciiFilename)) {
            flags |= O_CREAT;
            creationMode = _S_IREAD | _S_IWRITE;
        } else if (OPEN_READ_WRITE_TRUNCATE == ioMode) {
            flags |= O_TRUNC;
        }

        if (OPEN_WRITE == ioMode) {
            DEBUGP(("opening write only %s\n", pszAsciiFilename));
            flags |= O_WRONLY;
        } else {
            DEBUGP(("opening read write %s\n", pszAsciiFilename));
            flags |= O_RDWR;
        }
    }

    handle = open(pszAsciiFilename, flags, creationMode);

    DEBUGP(("storageOpen allocated file_desc %d\n", handle));

    if (-1 == handle) {
        *ppszError = getLastFileError("storageOpen()", pszAsciiFilename);
        return -1;
    }

#ifdef DEBUG_PRINT
    if (creationMode != 0) {
        DEBUGP(("created %s\n", pszAsciiFilename));
    }
#endif

    return handle;
}

void storageClose(char** ppszError, int handle) {
    int status;

    *ppszError = NULL;
    status = close(handle);

    DEBUGP(("storageClose on file_desc %d returns %d\n", handle, status));

    if (status < 0) {
        *ppszError = getLastError("storageClose()");
    }
}

int storageRead(char** ppszError, int handle, char* buffer, int length) {
    int bytesRead;

    *ppszError = NULL;
    if (0 == length) {
        return 0;
    }

    bytesRead = read(handle, buffer, length);
  
#ifdef DEBUG_STREAM
    fprintf(stdout, "storageRead on fd %d res = %d\n", handle, bytesRead);
#endif

    if (-1 == bytesRead) {
        *ppszError = getLastError("storageRead()");
    } else if (0 == bytesRead) {
        /* end of file in java is -1 */
        bytesRead = -1;
    }

    return bytesRead;
}

void storageWrite(char** ppszError, int handle, char* buffer, int length) {
    int bytesWritten;

    *ppszError = NULL;
    bytesWritten = write(handle, buffer, length);

    COMMIT(handle); /* force flush of the write to disk */

#ifdef DEBUG_STREAM
    fprintf(stdout, "storageWrite on fd %d res = %d\n", handle, bytesWritten);
#endif

    if (-1 == bytesWritten) {
        *ppszError = getLastError("storageWrite()");
        return;
    }

    if (bytesWritten != length) {
        *ppszError = "storage full";
        return;
    }
}

void storagePosition(char** ppszError, int handle, long absolutePosition) {
    long newPosition;

    newPosition = lseek(handle, absolutePosition, SEEK_SET);

#ifdef DEBUG_STREAM
    fprintf(stdout, "storagePostion on fd %d res = %d\n", handle, newPosition);
#endif

    if (-1 == newPosition) {
        *ppszError = getLastError("storagePosition()");
        return;
    }

    *ppszError = NULL;
}

int storageSizeOf(char** ppszError,  int handle) {
    struct stat stat_buf;

    if (fstat(handle, &stat_buf) < 0) {
        *ppszError = getLastError("storageSizeOf()");
        return 0;
    }

    *ppszError = NULL;
    return stat_buf.st_size;
}

void storageTruncate(char** ppszError, int handle, int size) {
    int rv;

    rv = TRUNCATE_FILE(handle, size);

    if (rv == -1) {
	*ppszError = getLastError("storageTruncate()");
    } else {
	*ppszError = NULL;
    }
    /* DEBUGP(("storageTruncate got rv %d\n", rv)); */
}

int storageGetFreeSpace() {
    int freeSpace;
    if (totalSpace == 0) {
        initTotalSpace();
    }

    freeSpace = totalSpace - getUsedSpace();

    /* DEBUGP(("Free space = %d\n", freeSpace)); */

    return freeSpace;
}

int storageFileExists(char* pszAsciiFilename) {
    struct stat stat_buf;
    int status;

    status = stat(pszAsciiFilename, &stat_buf);

    if (status < 0) {
        DEBUGP(("storage file %s does not exist\n", pszAsciiFilename));
        return 0;
    }

    DEBUGP(("storage file %s exists\n", pszAsciiFilename));
    return 1;
}

void storageDeleteFile(char** ppszError, char* pszAsciiFilename) {
    DEBUGP(("trying to delete %s\n", pszAsciiFilename));

    if (unlink(pszAsciiFilename) < 0) {
        *ppszError = getLastFileError("storageDeleteFile", pszAsciiFilename);
        return;
    }

    DEBUGP(("deleted %s\n", pszAsciiFilename));

    *ppszError = NULL;
}

void storageRenameFile(char** ppszError, char* pszOldFilename,
                       char* pszNewFilename) {
    DEBUGP(("renaming %s to %s\n", pszOldFilename, pszNewFilename));

    if (!storageFileExists(pszOldFilename)) {
        *ppszError = "The file to be renamed does not exist.";
    }

    if (storageFileExists(pszNewFilename)) {
        storageDeleteFile(ppszError, pszNewFilename);
        if (*ppszError != NULL) {
            return;
        }
    }

    if (rename(pszOldFilename, pszNewFilename) < 0) {
        *ppszError = getLastFileError("storageRename", pszNewFilename);
        return;
    }

    *ppszError = NULL;
}

static void initTotalSpace() {
    /*
     * Initialize total space
     */
    char * env;

    env = getInternalProp("system.jam_space");
    if (env == NULL) {
        totalSpace = DEFAULT_TOTAL_SPACE;
        return;
    }

    totalSpace = atoi(env);

    if (totalSpace < 4000) {  
        totalSpace = 4000;
        fprintf(stderr, "total storage space set to minimum: %d\n",
                totalSpace);
    }
}

static int getUsedSpace() {
    DIR *dir;  
    int size; 
    struct dirent *de;
    struct stat stat_buf; 
    char filename[MAX_FILENAME_LENGTH + 1]; 
    int rootLen;
    int exists;

    dir = opendir(systemDir);
    if (dir == NULL) {
        DEBUGP(("getSpaceUsed couldn't open dir %s\n", systemDir));
	return 0;
    }
    
    size = 0;

    strcpy(filename, systemDirAndSep);
    rootLen = strlen(filename);
    for (de = readdir(dir); de != NULL;
             de = readdir(dir), filename[rootLen] = 0) {
        if (strcmp(de->d_name, ".") == 0 || 
                strcmp(de->d_name, "..") == 0) {
            continue;
        }

        strncat(filename, de->d_name, sizeof (filename) - 1 - rootLen);

	exists = stat(filename, &stat_buf);
	if (exists != -1 && !S_ISDIR(stat_buf.st_mode)) {
	    size += stat_buf.st_size;
	}
    }

    closedir(dir);
    return size;
}

static char* getLastError(char* pszFunction) {
    char* temp;

    temp = strerror(errno);
    if (temp == NULL) {
        return "Unspecifed Error";
    }

    strcpy(errorBuffer, pszFunction);
    strcat(errorBuffer, ": ");
    strncat(errorBuffer, temp, MAX_ERROR_LEN - 2 - strlen(pszFunction));
    return errorBuffer;
}

static char* getLastFileError(char* pszFunction, char* pszFile) {
    char* temp;
    int charsLeft;

    temp = strerror(errno);
    if (temp == NULL) {
        return "Unspecifed Error";
    }

    strcpy(errorBuffer, pszFunction);
    strcat(errorBuffer, ": ");
    charsLeft = MAX_ERROR_LEN - strlen(errorBuffer);

    strncat(errorBuffer, temp, charsLeft);
    charsLeft = MAX_ERROR_LEN - strlen(errorBuffer);

    strncat(errorBuffer, ", ", charsLeft);
    charsLeft = MAX_ERROR_LEN - strlen(errorBuffer);

    strncat(errorBuffer, pszFile, charsLeft);

    return errorBuffer;
}

static int savedRootLength = 0;
static int savedMatchLength = 0;
static DIR* savedDirectory = NULL;
static char savedFilename[MAX_FILENAME_LENGTH + 1];

/*
 * this returns a static buffer an has static state for
 *  getNextFileThatStartsWith
 */
char* getFirstFileThatStartsWith(char* string) {
    char* tmp;

    if (savedDirectory != NULL) {
        closedir(savedDirectory);
        savedDirectory = NULL;
    }

    /*
     * find the root dir
     */
    tmp = strrchr(string, *getFileSeparator());
    if (NULL == tmp) {
        return NULL;
    }

    savedRootLength = tmp - string + 1;

    savedMatchLength = strlen(string + savedRootLength);

     
    /*
     * break up the filename at the parent separator, but put back the
     * separator after we create the parent directory
     */
    *tmp = '\0';
    savedDirectory = opendir(string);
    *tmp = *getFileSeparator();

    return getNextFileThatStartsWith(string);
}

/*
 * call getFirstFileThatStartsWith first, to set up the shared state
 * needed for this function
 */
char* getNextFileThatStartsWith(char* string) {
    char* match;
    struct dirent *de;
    char* filename = NULL;

    if (NULL == savedDirectory) {
        return NULL;
    }

    match = string + savedRootLength;

    /* find the first match file not "." or ".." */
    for (de = readdir(savedDirectory); de != NULL;
         de = readdir(savedDirectory)) {

        filename = de->d_name;
        if (strcmp(filename, ".") == 0 ||
            strcmp(filename, "..") == 0) {
            continue;
        }

        if (strncmp(filename, match, savedMatchLength) == 0) {
            break;
        }
    }

    if (NULL == de) {
        closedir(savedDirectory);
        savedDirectory = NULL;
        return NULL;
    }

    if ((savedRootLength + strlen(filename) + 1) > sizeof (savedFilename)) {
        closedir(savedDirectory);
        savedDirectory = NULL;
        return NULL;
    }

    memcpy(savedFilename, string, savedRootLength);
    strcpy(savedFilename + savedRootLength, filename);
    return savedFilename;
}
