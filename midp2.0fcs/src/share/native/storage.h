/*
 * @(#)storage.h	1.9 02/07/24 @(#)
 *
 * Copyright (c) 1998-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * This is the native storage interface for the MIDP RI. It hides any
 * directory hierarchies by making the file system look like a flat file
 * system. The internal Java classes will prefix all file names with a storage
 * root string without using an directory separators. RMS will go further and
 * add a suite unique string to the storage root string, also without using
 * file separators.
 *
 * See also "Filenames" and "I/O Modes" below.
 */
#ifndef _STORAGE_H
#define _STORAGE_H

/*
 * Filenames
 *
 * The storage implementation must support 255 8 bit character file names,
 * that include the characters A-Z, a-z, 0-9, _, %, #, .
 * The filenames will differ in more than just case, so case insensitive
 * systems like Win32 are not a problem.
 */
#define MAX_FILENAME_LENGTH 255 /* does not include the zero terminator */

/*
 * I/O Modes
 *
 * These are the different I/O modes for opening a file.
 * From javax.microedition.io.Connector.java and
 * com.sun.midp.io.j2me.storage.RandomAccessStream.
 */
/* open for read only */
#define OPEN_READ (1)
/* open for write only, create the file if it does not exist */
#define OPEN_WRITE (2)
/* open for read and write, create the file if it does not exist */
#define OPEN_READ_WRITE (OPEN_READ|OPEN_WRITE)
/* same as OPEN_READ_WRITE, except trucate an existing file to 0 bytes */
#define OPEN_READ_WRITE_TRUNCATE (-OPEN_READ_WRITE)

/* Common file stat tests */
#ifndef S_ISREG
#define S_ISREG(mode)   ( ((mode) & S_IFMT) == S_IFREG )
#define S_ISCHR(mode)   ( ((mode) & S_IFMT) == S_IFCHR )
#define S_ISFIFO(mode)  ( ((mode) & S_IFMT) == S_ISFIFO )
#define S_ISDIR(mode)   ( ((mode) & S_IFMT) == S_IFDIR )
#define S_ISBLK(mode)   ( ((mode) & S_IFMT) == S_IFBLK )
#endif /* _S_ISREG */

/*
 * Initialize and finalize the storage subsystem.
 */
void initializeStorage(char *);
void finalizeStorage();

/*
 * Returns the file separator as a string.
 *
 * This function only called by the main native code to
 * to manage files outside of the RI simulated storage.
 */
char* getFileSeparator();

/*
 * Returns the class path separator as a string.
 *
 * This function only called by the main native code to build
 * a class path.
 */
char* getPathSeparator();

/*
 * Returns root string for that all files should begin with, including
 * a trailing file separator if needed. By including the any trailing file
 * separators the Java API does not need to know about file separators
 * or subdirectories.
 *
 * The string can be zero characters (just the zero terminator), but not
 * NULL.
 */
char* getStorageRoot();

/*
 * Free the error string returned from a storage function.
 * Does nothing if a NULL is passed in.
 * This allows for systems that provide error messages that are allocated
 * dynamically.
 */
void storageFreeError(char* pszError);

/*
 * Return a 32 bit handle to an open a file in storage in different modes.
 * Does not block.
 *
 * See "I/O Modes" and "Filenames" above for move information.
 *
 * If not successfull *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
int storageOpen(char** ppszError, char* pszAsciiFilename, int ioMode);

/*
 * Close a opened by storageOpen. Does no block.
 *
 * If not successfull *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void storageClose(char** ppszError, int handle);

/*
 * Read from an open file in storage, returning the number of bytes read or
 * -1 for the end of the file. May read less than the length of the buffer.
 * Does not block.
 *
 * If not successfull *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
int storageRead(char** ppszError, int handle, char* buffer, int length);

/*
 * Write to an open file in storage. Will write all of the bytes in the
 * buffer or pass back an error. Does not block.
 *
 * If not successfull *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void storageWrite(char** ppszError, int handle, char* buffer, int length);

/*
 * Change the read/write position of an open file in storage.
 * The position is a number of bytes from beginning of the file.
 * Does not block.
 *
 * If not successfull *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void storagePosition(char** ppszError, int handle, long absolutePosition);

/*
 * Return the size of an open file in storage. Does not block.
 *
 * If not successfull *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
int storageSizeOf(char** ppszError,  int handle);

/*
 * Truncate the size of an open file in storage.  Does not block.
 *
 * If not successfull *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void storageTruncate(char** ppszError,  int handle, int size);

/*
 * Return the amount of free bytes of file storage. Does not block.
 */
int storageGetFreeSpace();

/*
 * Return a non-zero integer if a file with the given name exists else return
 * zero. Does not block.
 */
int storageFileExists(char* pszAsciiFilename);

/*
 * Rename a file in storage. Does not block.
 *
 * If not successfull *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void storageRenameFile(char** ppszError,  char* pszOldFilename,
                       char* pszNewFilename);

/*
 * Delete a file in storage. Does not block.
 *
 * If not successfull *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void storageDeleteFile(char** ppszError,  char* pszAsciiFilename);

/*
 * Return the first filename in storage that begins with a given string.
 * The order is defined by the underlying file system. Some state should
 * be kept statically so that getNextFileThatStartsWith can start the
 * search where the function left off. If this function is called
 * and there is any state left over from a previous search, that state
 * should be cleaned up and a new state created. Does not block.
 *
 * Returns NULL if no filename matches.
 *
 * The Java API will not perform concurrent searches so only one search
 * state must be kept.
 */
char* getFirstFileThatStartsWith(char* string);

/*
 * Used repeatly after getFirstFileThatStartsWith to return the next filename
 * in storage that begins with a given string. The string MUST be the same
 * string that was given to getFirstFileThatStartsWith or the result is
 * undetermined. Returns NULL if no more matches are found.
 */
char* getNextFileThatStartsWith(char* string);

#endif /* _STORAGE_H */
