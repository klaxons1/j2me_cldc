/*
 * @(#)midpMalloc.h	1.5 02/09/24 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _MIDP_MALLOC_H
#define _MIDP_MALLOC_H

#include <stdlib.h> 

/*
 * Main define to enable/disable private handling of memory allocations
 */
#if USE_MIDP_MALLOC

/*
 * Trace where (source file & line number) memory is being allocated and
 * freed
 */
/* #define MIDP_MALLOC_TRACE 1 */

/*
 * Enable debug output from memory manager
 */
/* #define MIDP_MALLOC_DEBUG 1 */

/*
 * Initialize memory sub-system
 */
int   midpInitializeMemory(int);

/*
 * Finalize memory sub-system
 */
int   midpFinalizeMemory(void);

/*
 * Information getters
 */
int   midpGetTotalHeap();
int   midpGetFreeHeap();

/*
 * Private implementations for memory allocation
 */
void* midpMallocImpl(int, char*, int);
void* midpCallocImpl(int, int, char*, int);
void* midpReallocImpl(void*, int, char*, int);
char* midpStrdupImpl(const char*, char*, int);
void  midpFreeImpl(void*, char*, int);

#ifdef MIDP_MALLOC_TRACE

extern int tracenativememory;

#define midpMalloc(x)     midpMallocImpl((x), __FILE__, __LINE__)
#define midpCalloc(x, y)  midpCallocImpl((x), (y), __FILE__, __LINE__)
#define midpRealloc(x, y) midpReallocImpl((x), (y), __FILE__, __LINE__)
#define midpStrdup(x)     midpStrdupImpl((x), __FILE__, __LINE__)
#define midpFree(x)       midpFreeImpl((x), __FILE__, __LINE__)

#else

#define midpMalloc(x)     midpMallocImpl((x), NULL, 0)
#define midpCalloc(x, y)  midpCallocImpl((x), (y), NULL, 0)
#define midpRealloc(x, y) midpReallocImpl((x), (y), NULL, 0)
#define midpStrdup(x)     midpStrdupImpl((x), NULL, 0)
#define midpFree(x)       midpFreeImpl((x), NULL, 0)

#endif /* MIDP_MALLOC_TRACE */

#else

/*
 * We're not using our own memory manager, use the standard library
 * function calls to allocate memory.
 */
#define midpInitializeMemory(x)
#define midpFinalizeMemory()

#define midpGetTotalHeap() (-1)
#define midpGetFreeHeap()  (-1)

#define midpMalloc(x)     malloc((x))
#define midpCalloc(x, y)  calloc((x), (y))
#if NO_STRDUP
extern char* strdup(const char*);
#endif
#define midpStrdup(x)     strdup((x))
#define midpFree(x)       free((x))
#define midpRealloc(x, y) realloc((x), (y))

#endif /* USE_MIDP_MALLOC */

#endif /* _MIDP_MALLOC_H */

