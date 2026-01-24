/*
 * @(#)dirent.h	1.5 02/07/24 @(#)
 *
 * Copyright (c) 1995-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * Posix-compatible directory access routines
 */

#ifndef _WIN32_DIRENT_H_
#define _WIN32_DIRENT_H_

#define MY_MAX_PATH 256

/*
 * System API for general allocations
 */

struct dirent {
    char d_name[MY_MAX_PATH];
};

typedef struct {
    struct dirent dirent; 
    char *path; 
    WIN32_FIND_DATA find_data;
    HANDLE handle;
} DIR;

DIR *opendir(const char *dirname);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
void rewinddir(DIR *dirp);
char *sysNativePath(char *path);

#endif

