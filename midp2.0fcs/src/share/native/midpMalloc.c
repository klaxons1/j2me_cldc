/*
 * @(#)midpMalloc.c	1.4 02/09/24 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/**************************************************************************
 * WARNING: THIS CODE IS NOT THREAD - SAFE !!!
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <midpMalloc.h>


/*
 *
 * With these functions, we have the ability to manage our own
 * memory pool. This avoids memory fragmentation in the overall
 * system. However, it does not address the potential problem
 * of memory fragmentation within the MIDP application itself.
 *
 * The memory pool is one contiguous chunk of memory split up
 * into various free or allocated blocks. All blocks have the
 * same layout.
 *
 *         memory block
 *        ----------------------------
 *        | magic       0xCAFE       |
 *        ----------------------------
 *        | free        [0|1]        |
 *        ----------------------------
 *        | reserved / (*) guardSize |
 *        ----------------------------
 *        | size                     |
 *        ----------------------------
 *        | (*) filename             |
 *        ----------------------------
 *        | (*) lineno               |
 *        ----------------------------
 *        | (*) guard   0x9A9A9A9A9A |
 *        ----------------------------
 *        | 1 .. size                |
 *        ----------------------------
 *        | (*) 1 .. guardSize       |
 *        ----------------------------
 *
 * All items prefixed with (*) are only enabled if memory tracing is
 * enabled.
 *
 */

/*
 * Structure to hold memory blocks
 */
typedef struct _midpMemStruct {
    unsigned short magic;                                    /* magic number */
    char           free;           /* 1 == block is free, 0 == block is used */
#ifndef MIDP_MALLOC_TRACE
    char           reserved;
#else
    char           guardSize;           /* Size of tail guard data; in bytes */
#endif
    unsigned int   size;                                    /* size of block */
#ifdef MIDP_MALLOC_TRACE
    char*          filename;         /* filename where allocation took place */
    unsigned int   lineno;        /* line number wehre allocation took place */
    unsigned int   guard;                                    /* memory guard */
#endif /* MIDP_MALLOC_TRACE */
} _MidpMemHdr, *_MidpMemHdrPtr;

/*
 * Default size of pool usable for allocations; in bytes
 */
#define DEFAULT_POOL_SIZE (128*1024)

/*
 * Byte boundary for word alignment
 */
#define ALIGNMENT     0x00000003                  /* Assumes word is 4-bytes */

/*
 * Constant to verify a header's validity
 */
#define MAGIC         0xCAFE

/*
 * Constants to guard memory
 */
#define GUARD_WORD    0x9A9A9A9A                  /* Assumes word is 4-bytes */
#define GUARD_BYTE    0x9A

/*
 * Minimum number of guard bytes to put at end of the memory block
 */
#define GUARD_SIZE    4

/* #define MIDP_MALLOC_USE_STATIC */
#ifdef MIDP_MALLOC_USE_STATIC
/* Cannot allocate dynamic memory on the phone. Use static array. */
static char MidpMemory[DEFAULT_POOL_SIZE];       /* Where MIDP memory starts */
#else  /* use malloc or similar function provided */
static char* MidpMemory;                         /* Where MIDP memory starts */
#endif

static char* MidpMemoryStart;                /* Aligned start of MIDP memory */
static char* MidpMemoryEnd;                                 /* End of memory */

static int midpEndMemory(int* count, int* size);

#ifdef MIDP_MALLOC_TRACE

static int MidpMemoryHighWaterMark;
static int MidpMemoryAllocated;
static int verifyTailGuardData(_MidpMemHdrPtr midpMemoryHdr);
static void
printAllocation(const char* what, char* filename, int lineno) {
    printf("  %s at %s line %d\n", what, filename, lineno);
}

#else

#define printAllocation(x,y,z)

#endif /* MIDP_MALLOC_TRACE */

/*=========================================================================
 * FUNCTION:      midpInitializeMemory()
 * TYPE:          public operation
 * OVERVIEW:      Initialize the MIDP memory pool
 *                 NOTE: This must only be called once
 * INTERFACE:
 *   parameters:  size   Size of memory pool to use; if size is '-1',
 *                        the default memory pool size will be used
 *   returns:     0 on succes; != 0 on failure
 *                
 *=======================================================================*/
int
midpInitializeMemory(int size) {
    _MidpMemHdrPtr midpMemoryHdr;

    if (size < 0) {
        /* size not specified, use the default */
        size = DEFAULT_POOL_SIZE;
    }
        
#ifndef MIDP_MALLOC_USE_STATIC

    /* allocate the chunk of memory to C heap */
    MidpMemory = (char*)malloc(size);
    if (MidpMemory == NULL) {
        return -1;
    }

#endif /* ! MIDP_MALLOC_USE_STATIC */

    MidpMemoryStart = MidpMemory;
    MidpMemoryEnd   = MidpMemory + size - sizeof(_MidpMemHdr);

    // Word alignment
    while (((long)MidpMemoryStart & ALIGNMENT) != 0) {
        MidpMemoryStart++;
    }

    midpMemoryHdr = (_MidpMemHdrPtr)MidpMemoryStart;
    midpMemoryHdr->magic = MAGIC;
    midpMemoryHdr->free  = 1;
    midpMemoryHdr->size  = (MidpMemory - MidpMemoryStart)
                           + size - sizeof(_MidpMemHdr);
    return 0;
}


/*=========================================================================
 * FUNCTION:      midpInitializeMemory()
 * TYPE:          public operation
 * OVERVIEW:      Finalize the MIDP memory pool
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     the number of memory leaks detected
 *                
 *=======================================================================*/
int
midpFinalizeMemory() {
    int count, size, ret;

    ret = midpEndMemory(&count, &size);

#if MIDP_MALLOC_TRACE
    if (ret > 0) {
        printf("\nWARNING: %d memory leak(s); %d bytes!\n", count, size);
    }
    printf("\n** Total memory  : %d\n", midpGetTotalHeap());
    printf("** Highwater mark: %d\n\n", MidpMemoryHighWaterMark);
#endif /* MIDP_MALLOC_TRACE */

#ifndef USE_STATIC_MEMORY	
    free(MidpMemory);
    MidpMemory = NULL;
#endif

    MidpMemoryStart = NULL;
    MidpMemoryEnd = NULL;

    return ret;
}


/*=========================================================================
 * FUNCTION:      midpEndMemory()
 * TYPE:          private operation
 * OVERVIEW:      Finalize the MIDP memory pool
 * INTERFACE:
 *   parameters:  count   address to store memory leak count
 *                size    address to store totol bytes of memory leaked
 *   returns:     the number of memory leaks detected
 *                
 *=======================================================================*/
static int
midpEndMemory(int* count, int* size) {
    _MidpMemHdrPtr midpMemoryHdr;
    char*          midpMemoryPtr;

    *count = 0;
    *size  = 0;

    for (midpMemoryPtr = MidpMemoryStart; 
	 midpMemoryPtr < MidpMemoryEnd;
	 midpMemoryPtr += midpMemoryHdr->size + sizeof(_MidpMemHdr)) {

        midpMemoryHdr = (_MidpMemHdrPtr)midpMemoryPtr;

        if (midpMemoryHdr->magic != MAGIC) {
            printf("ERROR: memory corruption at 0x%x\n", midpMemoryPtr);
            return -1;
        } else if (midpMemoryHdr->free != 1) {
#if MIDP_MALLOC_TRACE
            printf("WARNING: memory leak:  size = %d  address = 0x%08x\n",
                   midpMemoryHdr->size,
                   (void*)((char*)midpMemoryHdr + sizeof(_MidpMemHdr)));
#endif /* MIDP_MALLOC_TRACE */
	    printAllocation("allocated", 
			    midpMemoryHdr->filename, midpMemoryHdr->lineno);
            midpFree((void*)((char*)midpMemoryHdr + sizeof(_MidpMemHdr)));
            *count += 1;
            *size  += midpMemoryHdr->size;
        }
    }
    return *count;
}


/*=========================================================================
 * FUNCTION:      midpMallocImpl()
 * TYPE:          public operation
 * OVERVIEW:      Allocate memory from the private MIDP memory pool
 * INTERFACE:
 *   parameters:  size       Number of byte to allocate
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the newly allocated memory
 *                
 *=======================================================================*/
void*
midpMallocImpl(int size, char* filename, int lineno) {
    int            numBytesToAllocate = size;
    void*          loc     = NULL;
    _MidpMemHdrPtr tempHdr = NULL;
    char*          temp    = NULL;
    char*          midpMemoryPtr;
    _MidpMemHdrPtr midpMemoryHdr;

#if MIDP_MALLOC_TRACE
    numBytesToAllocate += GUARD_SIZE;
#endif /* MIDP_MALLOC_TRACE */
    while ( (numBytesToAllocate & ALIGNMENT) != 0 ) {
        numBytesToAllocate++;
    }

    /* find a free slot */
    for (midpMemoryPtr = MidpMemoryStart;
	 midpMemoryPtr < MidpMemoryEnd;
	 midpMemoryPtr += midpMemoryHdr->size + sizeof(_MidpMemHdr)) {
	
        midpMemoryHdr = (_MidpMemHdrPtr)midpMemoryPtr;
        if (midpMemoryHdr->magic != MAGIC) {
            printf("ERROR: Memory corruption at 0x%08x\n", midpMemoryPtr);
            return((void *) 0);
        } else {
            while ( 1 ) {
                /* coalescing */
                if (midpMemoryHdr->free == 1) {
                    /* if current block is free */
                    temp = (char*)midpMemoryHdr;
                    temp += midpMemoryHdr->size + sizeof(_MidpMemHdr);
                    tempHdr = (_MidpMemHdrPtr)temp;

                    if ((tempHdr->free == 1) && (tempHdr->magic == MAGIC) && 
			(temp < MidpMemoryEnd)) {
                        /* and the next block is free too */
                        /* then coalesce */
                        midpMemoryHdr->size += tempHdr->size
			                       + sizeof(_MidpMemHdr);
#ifdef MIDP_MALLOC_DEBUG                        
                        printf("DEBUG: Coalescing blocks 0x%08x and 0x%08x\n",
			       midpMemoryHdr, tempHdr);
#endif /* MIDP_MALLOC_DEBUG */

                    } else {
                        break;
                    }
                } else {
                    break;
                }
            } /* while */

            /* allocating */
            if ((midpMemoryHdr->free == 1) && 
		(midpMemoryHdr->size >= numBytesToAllocate)) {
                if (midpMemoryHdr->size > (numBytesToAllocate 
					      + sizeof(_MidpMemHdr) + 4)) {
                    /* split block */
                    _MidpMemHdrPtr nextHdr;
                    nextHdr = (_MidpMemHdrPtr)((char *)midpMemoryPtr
					       + numBytesToAllocate
                                               + sizeof(_MidpMemHdr));
                    nextHdr->magic = MAGIC;
                    nextHdr->free = 1;
                    nextHdr->size = midpMemoryHdr->size 
			            - numBytesToAllocate 
			            - sizeof(_MidpMemHdr);
		    midpMemoryHdr->size     = numBytesToAllocate;
                }
                midpMemoryHdr->free     = 0;
                loc = (void*)((char*)midpMemoryHdr + sizeof(_MidpMemHdr));
#ifdef MIDP_MALLOC_TRACE
                midpMemoryHdr->guard    = GUARD_WORD;      /* Add head guard */
                midpMemoryHdr->filename = filename;
                midpMemoryHdr->lineno   = lineno;
		{
		    /* Add tail guard */
		    int   guardSize = midpMemoryHdr->size - size;
		    void* guardPos;
		    int   i;

                    midpMemoryHdr->guardSize = guardSize;
		    guardPos = (void*)((char*)loc + midpMemoryHdr->size 
				       - guardSize);
		    for(i=0; i<guardSize; i++) {
			((unsigned char*)guardPos)[i] = GUARD_BYTE;
		    }
		}

		MidpMemoryAllocated += numBytesToAllocate;
		if (MidpMemoryAllocated > MidpMemoryHighWaterMark) {
		    MidpMemoryHighWaterMark = MidpMemoryAllocated;
		}
#endif /* MIDP_MALLOC_TRACE */
#ifdef MIDP_MALLOC_DEBUG                
                printf("DEBUG: Requested %d provided %d at 0x%08x\n",
		       numBytesToAllocate, midpMemoryHdr->size, loc);
		printAllocation("allocated", filename, lineno);
#endif /* MIDP_MALLOC_DEBUG */
                return(loc);
            } /* end of allocating */
        } /* end of else */
    } /* end of for */
#ifdef MIDP_MALLOC_DEBUG
    printf("DEBUG: Unable to allocate %d bytes\n", numBytesToAllocate);
#endif /* MIDP_MALLOC_DEBUG */
    return((void *)0);
}


/*=========================================================================
 * FUNCTION:      midpCallocImpl()
 * TYPE:          public operation
 * OVERVIEW:      Allocate memory from the private MIDP memory pool,
 *                 memory contents are cleared
 * INTERFACE:
 *   parameters:  nelem      Number of elements to allocate
 *                elsize     Size of one element
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the newly allocated and cleared memory
 *                
 *=======================================================================*/
void*
midpCallocImpl(int nelem, int elsize, char* filename, int lineno) {
    void *loc = NULL;

    if ((loc = midpMallocImpl(nelem * elsize, filename, lineno)) != NULL) {
        memset(loc, 0, nelem * elsize);
    }
    return loc;
}

/*=========================================================================
 * FUNCTION:      midpReallocImpl()
 * TYPE:          public operation
 * OVERVIEW:      Re-allocate memory from the private MIDP memory pool
 * INTERFACE:
 *   parameters:  ptr        Original memory pointer
 *                size       New size
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the re-allocated memory
 *                
 *=======================================================================*/
void*
midpReallocImpl(void* ptr, int size, char* filename, int lineno) {
    void*          newPtr = NULL;
    _MidpMemHdrPtr memHdr;

    if (ptr == NULL) {
	return ptr;
    }

    memHdr = (_MidpMemHdrPtr)((char*)ptr - sizeof(_MidpMemHdr));

    if (memHdr->size != size) {
	if (size != 0) {
	    newPtr = midpMallocImpl(size, filename, lineno);
	    if (newPtr != NULL) {
		if (memHdr->size < size) {
		    memcpy(newPtr, ptr, memHdr->size);
		} else {
		    memcpy(newPtr, ptr, size);
		}
		midpFreeImpl(ptr, filename, lineno);
	    }
	} else {
	    /* When size == 0, realloc() acts just like free() */
	    midpFreeImpl(ptr, filename, lineno);
	}
    } else {
	/* sizes are the same, just return the same pointer */
	newPtr = ptr;
    }
	
    return newPtr;
}


/*=========================================================================
 * FUNCTION:      midpStrdupImpl()
 * TYPE:          public operation
 * OVERVIEW:      Duplicate the given string
 * INTERFACE:
 *   parameters:  s1         String to duplicate
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the duplicate string
 *                
 *=======================================================================*/
char*
midpStrdupImpl(const char *s1, char* filename, int lineno) {
    char *p = (char *)midpMallocImpl(strlen(s1) + 1, filename, lineno);

    if ( p != NULL ) {
        strcpy(p, s1);
    }
    return(p);
}


/*=========================================================================
 * FUNCTION:      midpFreeImpl()
 * TYPE:          public operation
 * OVERVIEW:      Free memory allocated from the private MIDP memory pool
 * INTERFACE:
 *   parameters:  ptr        Pointer to allocated memory
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     <nothing>
 *                
 *=======================================================================*/
void
midpFreeImpl(void *ptr, char *filename, int lineno) {
    _MidpMemHdrPtr midpMemoryHdr;

    if (ptr == NULL) {
#ifdef MIDP_MALLOC_DEBUG
        printf("DEBUG: Attempt to free NULL pointer\n");
        printAllocation("freed", filename, lineno);
#endif /* MIDP_MALLOC_DEBUG */
    } else if (((char*)ptr > MidpMemoryEnd) || 
	       ((char*)ptr < MidpMemoryStart)) {
        printf("ERROR: Attempt to free memory out of scope: 0x%08x\n", ptr);
        printAllocation("freed", filename, lineno);
    } else {
        midpMemoryHdr = (_MidpMemHdrPtr)((char*)ptr -sizeof(_MidpMemHdr));
        if (midpMemoryHdr->magic != MAGIC) {
            printf("ERROR: Attempt to free corrupted memory: 0x%08x\n", ptr);
	    printAllocation("freed", filename, lineno);
#ifdef MIDP_MALLOC_DEBUG
        } else if (midpMemoryHdr->free != 0) {
            printf("DEBUG: Attempt to free memory twice: 0x%08x\n", ptr);
	    printAllocation("freed", filename, lineno);
#endif /* MIDP_MALLOC_DEBUG */
        } else {
#ifdef MIDP_MALLOC_TRACE
	    MidpMemoryAllocated -= midpMemoryHdr->size;
	    /* The memory block header is valid, now check the guard data */
	    if (midpMemoryHdr->guard != GUARD_WORD) {
		printf("ERROR: Possible memory underrun: 0x%08x\n", ptr);
		printAllocation("allocated", midpMemoryHdr->filename, 
				             midpMemoryHdr->lineno);
		printAllocation("freed", filename, lineno);
	    } else if (verifyTailGuardData(midpMemoryHdr)) {
		printf("ERROR: Possible memory overrun: 0x%08x\n", ptr);
		printAllocation("allocated", midpMemoryHdr->filename, 
				             midpMemoryHdr->lineno);
		printAllocation("freed", filename, lineno);
	    }
#endif /* MIDP_MALLOC_TRACE */
#ifdef MIDP_MALLOC_DEBUG
            printf("DEBUG: free %d bytes: 0x%08x\n", midpMemoryHdr->size, ptr);
	    printAllocation("allocated", 
			    midpMemoryHdr->filename, midpMemoryHdr->lineno);
	    printAllocation("freed", filename, lineno);
#endif
            midpMemoryHdr->free = 1;
        }
    } /* end of else */
}


#if MIDP_MALLOC_TRACE

/*=========================================================================
 * FUNCTION:      verifyTailGuardData()
 * TYPE:          private operation
 * OVERVIEW:      Verify guard data at the end of the memory is valid
 * INTERFACE:
 *   parameters:  midpMemoryHdr   Pointer to memory block header
 *   returns:     0 if guard data is valid; otherwise, the byte position
 *                 of the first incorrect guard data byte
 *                
 *=======================================================================*/
static int
verifyTailGuardData(_MidpMemHdrPtr midpMemoryHdr) {
    void* guardPos;
    int   guardSize;
    int   i;

    guardSize = midpMemoryHdr->guardSize;
    guardPos = (void*)((char*)midpMemoryHdr
		       + sizeof(_MidpMemHdr)
		       + midpMemoryHdr->size - guardSize - 1);
    for(i = 1; i <= guardSize; i++) {
	if (((unsigned char*)guardPos)[i] != GUARD_BYTE) {
	    return i;
	}
    }
    return 0;
}
#endif /* MIDP_MALLOC_TRACE */


/*=========================================================================
 * FUNCTION:      midpGetTotalHeap()
 * TYPE:          public operation
 * OVERVIEW:      Get the total amount of available heap
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     The total amount of available heap
 *                
 *=======================================================================*/
int
midpGetTotalHeap() {
    return (MidpMemoryEnd - MidpMemoryStart);
}


/*=========================================================================
 * FUNCTION:      midpGetFreeHeap()
 * TYPE:          public operation
 * OVERVIEW:      Get the current amount of unused heap
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     The current amount of unused heap
 *                
 *=======================================================================*/
int
midpGetFreeHeap() {
    _MidpMemHdrPtr midpMemoryHdr;
    char*          midpMemoryPtr;
    int            size = 0;

    for (midpMemoryPtr = MidpMemoryStart; 
         midpMemoryPtr < MidpMemoryEnd;
         midpMemoryPtr += midpMemoryHdr->size + sizeof(_MidpMemHdr)) {

        midpMemoryHdr = (_MidpMemHdrPtr)midpMemoryPtr;

        if (midpMemoryHdr->free != 1) {
            size += midpMemoryHdr->size;
        }
    }
    return (midpGetTotalHeap() - size);
}

