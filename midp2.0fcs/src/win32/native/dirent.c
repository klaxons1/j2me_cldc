/*
 * @(#)dirent.c	1.6 02/07/24 @(#)
 *
 * Copyright (c) 1995-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * Posix-compatible directory access routines
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include <errno.h>

#include <windows.h>
#include "dirent.h"
#include <midpMalloc.h>

/*
 * These three calls, opendir, readdir, and closedir will be provided
 * to mimic the standard dirent calls from solaris on windows.
 * These functions were stolen from the pjava3.1 tree.
 */ 

/* FIX:
 * Caller must have already run dirname through sysNativePath, which removes
 * duplicate slashes and converts all instances of '/' into '\\'. 
 */

DIR *
opendir(const char *dirname)
{
    DIR* dirp;

    DWORD fattr;
    int dirnamelen = strlen(dirname);

    /* Must fail if dirname == "" */
    if (dirnamelen == 0) {
	errno = ENOENT;
	return NULL;
    }
    if ((dirp = (DIR*)midpMalloc(sizeof(DIR))) == NULL) {
	errno = ENOMEM;
	return NULL;
    }
    /*
     * GetFileAttributes() works fine for "\\".
     * FindFirstFile() does not work for "\\", but we're going to append 
     * "*.*" before using it, which makes FindFirstFile() working happily.
     */
    dirp->path = (char *)midpMalloc(dirnamelen + 5 /* for "\\*.*\0" */);
    if (dirp->path == NULL) {
	midpFree(dirp);
	errno = ENOMEM;
	return NULL;
    }
    strcpy(dirp->path, dirname);

    fattr = (DWORD)GetFileAttributes(dirp->path);
    if (fattr == 0xffffffff) { /* error */
	midpFree(dirp->path);
	midpFree(dirp);
	errno = ENOENT;
	return NULL;
    } else if (fattr & FILE_ATTRIBUTE_DIRECTORY == 0) {
	midpFree(dirp->path);
	midpFree(dirp);
	errno = ENOTDIR;
	return NULL;
    }

    /* Append "*.*", or possibly "\\*.*", to path */
    /*    sysAssert(dirnamelen >= 1);*/
    /* No '\\' needed for cases like "X:" or "XXX\\" */
    if ((dirnamelen == 2 && dirp->path[1] == ':')
	|| dirp->path[dirnamelen - 1] == '\\')
      {
	strcat(dirp->path, "*.*");
      } else {
	strcat(dirp->path, "\\*.*");
      }
    dirp->handle = (void*)FindFirstFile(dirp->path, &dirp->find_data);
    if (dirp->handle == INVALID_HANDLE_VALUE) {
      if (GetLastError() != ERROR_FILE_NOT_FOUND) {
	midpFree(dirp->path);
	midpFree(dirp);
	errno = EACCES;
	return NULL;
      }
    }
    return dirp;
}



struct dirent *
readdir(DIR *dirp)
{

    if (dirp->handle == INVALID_HANDLE_VALUE) {
	return 0;
    }

    strcpy(dirp->dirent.d_name, dirp->find_data.cFileName);

    if (!FindNextFile(dirp->handle, &dirp->find_data)) {
	if (GetLastError() == ERROR_INVALID_HANDLE) {
	    errno = EBADF;
	    return 0;
	}
	FindClose(dirp->handle);
	dirp->handle = INVALID_HANDLE_VALUE;
    }

    return &dirp->dirent;
}



int
closedir(DIR *dirp)
{
    if (dirp->handle != INVALID_HANDLE_VALUE) {
	if (!FindClose(dirp->handle)) {
	    errno = EBADF;
	    return -1;
	}
	dirp->handle = INVALID_HANDLE_VALUE;
    }
    midpFree(dirp->path);
    midpFree(dirp);
    return 0;
}



void
rewinddir(DIR *dirp)
{
    if (dirp->handle != INVALID_HANDLE_VALUE) {
	FindClose(dirp->handle);
    }
    dirp->handle = FindFirstFile(dirp->path, &dirp->find_data);
}
