/*
 * @(#)pushregistry.c	1.42 02/10/22 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * Virtual Machine
 *=========================================================================
 * SYSTEM:    MIDP native
 * SUBSYSTEM: Push Registry Persistent File Management
 * FILE:      pushregistry.c
 * OVERVIEW:  Opens the push registry file and caches the contents in
 *            memory at startup. Subsequent additions and deletions
 *            update the cache and rewrite the persistent data file.
 *=======================================================================*/
/*=========================================================================
 * Include files
 *=======================================================================*/
#include "kni.h"

#include <stdio.h>
#include "midpMalloc.h"
#include "storage.h"
#ifdef UNIX
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <windows.h>
#ifndef atoll
#define atoll(x) _atoi64(x)
#endif /* atoll */
#ifdef GCC
#include <winsock.h>
#else
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif /* _WINSOCKAPI_ */
#endif /* GCC */
#endif /* UNIX */

#include <errno.h>
static char *appdir = NULL;
static char *pushpathname = NULL;
static char *alarmpathname = NULL;

static int  pushfd; 
static char *errStr = NULL;
/* buffer size for line parsing */
#define MAX_LINE 512

#ifndef MAX_HOST_LENGTH
#define MAX_HOST_LENGTH 256
#endif /* MAX_HOST_LENGTH */

#ifndef MAX_DATAGRAM_LENGTH
#define MAX_DATAGRAM_LENGTH 1500
#endif /* MAX_DATAGRAM_LENGTH */

typedef struct _datagramentry {
    int ipAddress;
    int port;
    int length;
    char buffer[MAX_DATAGRAM_LENGTH];
} DatagramEntry;

typedef struct _pushentry {
    struct _pushentry *next;
    char *value;
    char *storagename;
    char *filter;
    int fd; 
    int fdsock; 
    int port; 
    int state;
    DatagramEntry *dg; 
} PushEntry;

typedef struct _alarmentry {
    struct _alarmentry *next;
    char *midlet;
    char *storagename;
    jlong wakeup;
} AlarmEntry;

#define LAUNCH_PENDING  (-4)
#define CHECKED_IN  (-3)
#define CHECKED_OUT (-2)
#define AVAILABLE   (-1)

static PushEntry *pushlist = NULL;
static AlarmEntry *alarmlist = NULL;

static int pushlength = 0;
static void pushProcessPort(char *buffer, int *fd, int *port);
static void alarmopen();
static void alarmsave();
static int alarmadd(char *str, jlong alarm, jlong *lastalarm);
static jlong alarmcheck(jlong time);
static void pushListFree();
static void alarmListFree();
static char *pushfindfd(jlong fd);
static int parsePushList ();
static int checkfilter(char *filter, char *ip);
static void pushcheckinentry(PushEntry *p);
static void pushcleanupentry(PushEntry *p);

/*
 * Extract the storage name from the connection value string
 */
static char *pushstorage(char *value, int field) {
    char *storagefield;
    char *storagename= NULL;
    int comma = 0 ;
    
    for (storagefield = value; *storagefield; storagefield++) {
	if (*storagefield == ',')
	    comma ++;
	/* Push entry contains "connection, midletname, filter, storage" */
	/* Alarm entry contains "midletname, alarm, storage" */
	if (comma == field) {
	    storagename = storagefield + 1;
	    break;
	}
    }
    return storagename;
}
/*
 * Extract the filter from the connection value string
 */
static char *pushfilter(char *value) {
    char *filterfield = NULL;
    char filter[MAX_HOST_LENGTH];
    int i = 0 ;
    
    /* Push entry contains "connection, midletname, filter, storage" */
    for (filterfield = pushstorage(value, 2); *filterfield; filterfield++) {
	if (*filterfield == ',')
	    break;
	filter[i++] = *filterfield;
    }
    filter[i] = '\0';
    return midpStrdup(filter);
}

/*
 * Open the Push Registry file, if it exists and populate
 * an in memory cache of the file contents.
 */
void pushopen() {
    /* Make sure the network is properly in itialized. */
    networkInit();

    /* Get the storage directory. */
    if (appdir == NULL) {
	/*
	 * On first use initialize the fully qualified pathnames
	 * of the push and alarm persistent files.
	 */
	if ((appdir = getStorageRoot()) != NULL) {
	    if (pushpathname = (char*) midpMalloc(strlen(appdir) +  13)) {
		strcpy(pushpathname, appdir);
		strcat(pushpathname, "pushlist.txt");
	    }
	    if (alarmpathname = (char*) midpMalloc(strlen(appdir) +  14)) {
		strcpy(alarmpathname, appdir);
		strcat(alarmpathname, "alarmlist.txt");
	    }
	} else {
	    /* NYI - no storage directory found. */
	}

        /*
         * If no memory can be allocated, exit.
         */
        if (appdir == NULL || pushpathname == NULL || alarmpathname == NULL) {
            fprintf(stderr, "Error: pushopen out of memory.\n");
            exit(-1);
        }
    }

    /* Now read the registered connections.*/
    if ((pushfd = storageOpen(&errStr, pushpathname, OPEN_READ)) != -1){
	/* Read through the file one line at a time */
	if (parsePushList() == -2) {
        fprintf(stderr, "Error: pushopen out of memory "
                        "when parsing push list.\n");
        exit(-1);
    }
	
	/* Close the storage handle */
	storageClose (&errStr, pushfd);
    } else {
	if (errStr != NULL) {
	    /*	    fprintf (stderr, "Warning: could not open push registration file(%s): %s\n",
		    pushpathname, errStr); */
	    storageFreeError(errStr);
	}
    }
    alarmopen();
}

/*
 * Destroy the push and alarm memory resources. Maintain the push
 * registrations in the push registry file.
 */
void pushclose() {
    pushListFree();
    alarmListFree();

    midpFree(pushpathname);
    midpFree(alarmpathname);
}

/*
 * Save the in memory cache of push registrations to a persistent
 * file for use in subsequent runs.
 */
static void pushsave() {
    PushEntry *p;
    if ((pushfd = storageOpen(&errStr, pushpathname, OPEN_READ_WRITE_TRUNCATE)) != -1){
	/* Write a new list of push registrations to the persistent file */
	for (p = pushlist; p != NULL ; p = p->next) {
	    storageWrite(&errStr, pushfd, p->value, strlen(p->value));
	    storageWrite(&errStr, pushfd, "\n", 1);
	}
	/* Close the storage handle */
	storageClose (&errStr, pushfd);
    } else {
	if (errStr != NULL) {
	    fprintf (stderr, "Warning: could not write push registration file(%s): %s\n",
		     pushpathname, errStr); 
	    storageFreeError(errStr);
	    return;
	}
    }
}

/*
 * Add one entry to the push registry.
 * If the entry already exists return an error.
 * On succesful registration, write a new copy of the file to disk.
 * Return -2 if out of memory
 */
static int pushadd(char *str){
    PushEntry *p;
    PushEntry *pe;
    int comma;
    char *cp;

    /* Count the characters up to the first comma. */
    for (comma = 0, cp = str; *cp; comma++, cp++) {
	if (*cp == ',') {
	    break;
	}
    }
    /* Check if the entry already exists? */
    for (p = pushlist; p != NULL ; p = p->next) {
	if (strncmp (str, p->value, comma) == 0) {
	    return -1 ;
	}
    }
    /* Add the new entry. */
    if (pe = (PushEntry *) midpMalloc (sizeof(PushEntry))){
        pe->next = pushlist ;
        pe->value = midpStrdup(str);
        pe->storagename = midpStrdup(pushstorage(pe->value, 3));

        if ((pe->value == NULL) || (pe->storagename == NULL)) {
            midpFree(pe->value);
            midpFree(pe->storagename);
            midpFree(pe);

            return -2;

        } else {
            pe->filter = pushfilter(pe->value);
            pushProcessPort(str, &(pe->fd), &(pe->port));
            pe->state = AVAILABLE ;
            pe->fdsock = -1;
            pe->dg = NULL;
            pushlist = pe ;
            pushlength++;
        }
    }

    if (pe != NULL) {
        pushsave();
        return 0; 
    }

    return -2;
}

/*
 * Remove one entry from the push registry.
 * If the entry is not registered return an error.
 * On successful deletion, write a new copy of the file to disk.
 */
static int pushdel(char *str, char *store) {
    PushEntry *p; 
    PushEntry *lastp = pushlist;
    PushEntry *tmp;

    /* Find the entry to remove */
    for (p = pushlist; p != NULL ; p = tmp) {
	tmp = p->next;
	if (strncmp (str, p->value, strlen(str)) == 0) {
	    /* Check if the connection belongs to another suite. */
	    if (strcmp(store, p->storagename) != 0) {
		return -2 ;
	    }

	    /* 
	     * Close the socket connection if the entry is being removed.
	     * Only close the connection, if it is not already opened
	     * by the application or in the middle of fielding a
	     * connection notification.
	     */
	    if (p->fd != -1 &&
		p->state != LAUNCH_PENDING &&
		p->state != CHECKED_OUT) {
		prim_com_sun_midp_io_j2me_serversocket_Protocol_close(p->fd);
		p->fd = -1;
	    }

	    /* Clear the pending notification flag. */
	    if (p->state != LAUNCH_PENDING) { 
		p->state = AVAILABLE;
	    }

            pushcleanupentry(p);

            /* Remove the registration entry. */
            if (p == pushlist){
                /* Replace the top of the list. */
                pushlist = p->next;
            } else {
                lastp->next = p->next;
            }

            pushlength-- ;
		
            midpFree(p->value);
            p->value = NULL;
		
            midpFree(p->filter);
            p->filter = NULL;
		
            midpFree(p->storagename);
            p->storagename = NULL;
		
            midpFree(p);
            pushsave();

	    return 0;
	}

	lastp = p ;
    }

    return -1; 
}
/*
 *  Fetch a buffer datagram
 */
int pusheddatagram (int fd, int *ip, int *port, char *buf, int len) {
    PushEntry *p; 
    PushEntry *tmp;
    int length = -1;
    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = tmp) {
	tmp = p->next;
	if (p->fd == fd) {
	    /* Return the cached data. */
	    if (p->dg == NULL) {
		return -1;
	    }
	    *ip = p->dg->ipAddress;
	    *port = p->dg->port;
	    length = p->dg->length;
	    if (length > 0) {
		memcpy(buf, p->dg->buffer, (len < length? len : length));
	    }
	    
	    /* Destroy the cached entry after it has been read. */
	    midpFree(p->dg);
	    p->dg = NULL;
	    return length;
	}
    }
    return -1; 
}

/*
 * Check out the file descriptor for the requested server socket.
 */
int pushcheckoutaccept(int fd) {
    PushEntry *p; 
    PushEntry *tmp;
    int temp;

    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = tmp) {
	tmp = p->next;
	if (p->fd == fd) {
	    temp = p->fdsock;
	    p->fdsock = -1;
	    return temp;
	}
    }
    return -1; 

}
/*
 * Check out the file descriptor for the requested connection.
 * Returns -1 if the connection is not found, other wise returns
 * the previously opened file descriptor. The CHECKED_OUT token 
 * is left in the table to indicate that an application is
 * actively using the connection.
 */
int pushcheckout(char* protocol, int port, char * store) {
    PushEntry *p; 
    PushEntry *tmp;
    int fd;

    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = tmp) {
	tmp = p->next;
	if (p->port == port &&
	    strncmp(p->value, protocol, strlen(protocol)) == 0) {
	    /* Check if the current suite reserved the port. */
	    if (strcmp(store, p->storagename) != 0) { 
		return -2;
	    }
	    fd = p->fd;
	    p->state = CHECKED_OUT;
	    return fd;
	}
    }
    return -1; 
}

/*
 * Check in the file descriptor for the requested connection.
 * Returns 0 on success, or -1 on failure to check in the 
 * file descriptor to the cached push registry.
 */
int pushcheckin(int fd) {
    PushEntry *p; 
    PushEntry *lastp = pushlist;
    PushEntry *tmp;

    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = tmp) {
	tmp = p->next;
	if (p->fd == fd) {
            if (p->state == CHECKED_OUT) {
                pushcheckinentry(p);
            }

	    return 0;
	}

	lastp = p ;
    }

    return -1; 
}

/*
 * Check in the connections. Used between VM starts by main.
 */
void pushcheckinall() {
    PushEntry *p; 
    PushEntry *lastp = pushlist;
    PushEntry *tmp;

    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = tmp) {
	tmp = p->next;

        pushcheckinentry(p);

	lastp = p ;
    }
}

/*
 * Check in the file descriptor connection for the requested connection,
 * given the connection name.
 * Returns 0 on success, or -1 on failure to check in the 
 * file descriptor to the cached push registry.
 */
int pushcheckinbyname(char* str) {
    PushEntry *p; 
    PushEntry *lastp = pushlist;
    PushEntry *tmp;

    /* Find the entry to remove */
    for (p = pushlist; p != NULL ; p = tmp) {
	tmp = p->next;
	if (strncmp (str, p->value, strlen(str)) == 0) {
            pushcheckinentry(p);
            return 0;
	}

	lastp = p ;
    }

    return -1; 
}

/*
 * Check in the push entry and cleanup anything the MIDlet did not get.
 */
static void pushcheckinentry(PushEntry *p) {
    p->state = CHECKED_IN;

    pushcleanupentry(p);
}

/*
 * Cleanup anything the MIDlet did not get.
 */
static void pushcleanupentry(PushEntry *p) {
    /* Close any accepted socket not accessed, yet (if any). */
    if (p->fdsock != -1) {
        prim_com_sun_midp_io_j2me_socket_Protocol_close0(p->fdsock);
        p->fdsock = -1;
    }

    /* Remove the cached datagram (if any). */
    if (p->dg != NULL) {
        midpFree(p->dg);
        p->dg = NULL;
    }
}

/*
 * Lookup the midlet name given the file descriptor
 */
static char *pushfindfd(jlong fd) {
    PushEntry *pushp; 
    PushEntry *pushtmp;
    int temp_state = AVAILABLE;

    AlarmEntry *alarmp; 
    AlarmEntry *alarmtmp;
    char *alarmentry = NULL;
    char *ipnumber = NULL;
    struct in_addr addr;

    /* Find the entry to pass off the open file descriptor. */
    for (pushp = pushlist; pushp != NULL ; pushp = pushtmp) {
	pushtmp = pushp->next;
	if ((pushp->fd == (int)fd) &&
	    (pushp->state != LAUNCH_PENDING)) {
	    temp_state =  pushp->state;
	    pushp->state = LAUNCH_PENDING;

	    /* 
	     * Check the push filter, to see if this connection 
	     * is acceptable.
	     */
 	    if(strncmp(pushp->value,"datagram://:",12) == 0) {
		/* 
		 * Read the datagram and save it til the application reads it.
		 * This is a one datagram message queue.
		 */
		if ((pushp->dg = (DatagramEntry*) midpMalloc(MAX_DATAGRAM_LENGTH))
		    != NULL) {
		        if ((pushp->dg->length = prim_com_sun_midp_io_j2me_datagram_Protocol_receive0(
                            pushp->fd, &(pushp->dg->ipAddress), 
			    &(pushp->dg->port), pushp->dg->buffer, MAX_DATAGRAM_LENGTH)) != -1 ) {
			    addr.s_addr = pushp->dg->ipAddress;
			    ipnumber = inet_ntoa(addr);
			} else {
			    /* Receive failed - no data available. */
			    midpFree(pushp->dg);
			    pushp->dg = NULL;
			    pushp->state = temp_state;
			}

		}
		
	    } else if(strncmp(pushp->value,"socket://:",10) == 0) {
		/*
		 * For a server socket connection, accept the inbound 
		 * socket connection so the end point filter can be checked.
		 */
		if ((pushp->fdsock =
		     prim_com_sun_midp_io_j2me_serversocket_Protocol_accept(pushp->fd)) != -1) {
		    ipnumber =
			prim_com_sun_midp_io_j2me_socket_Protocol_getipnumber1 (pushp->fdsock, 0);
		} else {
		    /* Connection is not available, cancel the launch pending. */
		    pushp->state = temp_state;
		}
	    }

	    /* Check the IP filter. */
	    if (checkfilter(pushp->filter, ipnumber)) {
		return midpStrdup(pushp->value);
	    } else {
		/* Dispose of the filtered push request. */
		/* Close any accepted socket not accessed, yet. */
		if (pushp->fdsock != -1) {
		    prim_com_sun_midp_io_j2me_socket_Protocol_close0(pushp->fdsock);
		    pushp->fdsock = -1;
		}
		/* Remove any cached datagrams. */
		if (pushp->dg != NULL) {
		    midpFree(pushp->dg);
		    pushp->dg = NULL;
		}
		pushp->state = temp_state;
		return NULL;
	    }
	}
    }

    /*
     * If the file descriptor was not found, it could be 
     * an alarm time. If found, clear the entry so it will 
     * not fire again.
     */
    for (alarmp = alarmlist; alarmp != NULL ; alarmp = alarmtmp) {
	alarmtmp = alarmp->next;
	if (alarmp->wakeup == fd) {
	    alarmentry = midpStrdup(alarmp->midlet);
	    if (alarmentry) {
		jlong lastalarm;
	        alarmadd(alarmentry, 0, &lastalarm);
	        return alarmentry;
        } else {
            alarmp->wakeup = 0;
        }
	}
    }
    return NULL; 
}

/*
 * Lookup the midlet name given the file descriptor
 */
static char *pushfindconn(char *str) {
    PushEntry *p; 
    PushEntry *tmp;
    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = tmp) {
	tmp = p->next;
	if (strncmp (str, p->value, strlen(str)) == 0) {
	    return p->value;
	}
    }
    return NULL; 
}

/*
 * Lookup the midlet name given the storagename
 */
static char *pushfindsuite(char *store, int available) {
    PushEntry *p; 
    PushEntry *tmp;
    char *ret = NULL;
    char *ptr;
    int len=0;
    char *connlist = NULL;

    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = tmp) {
	tmp = p->next;
	if (strcmp(store, p->storagename) == 0) {
	    ret = midpStrdup(p->value);
	    for (ptr = ret, len=0; *ptr; ptr++, len++) {
		if (*ptr == ',') {
		    *ptr = '\0';
		    break;
		}
	    }
	    
	    /*
	     * Check if there is pending I/O on the 
	     * current file descriptor. e.g. an accepted socket
	     * or a cache datagram.
	     */
	    if (available && (p->fd != -1)) {
		if ((p->fdsock == -1) && (p->dg == NULL)) {
		    midpFree(ret);
		    ret = NULL;
		    continue;
		}
	    }

	    /* 
	     * Append the entries together in a single list.
	     */
	    if (connlist == NULL) {
		connlist= ret;
	    } else {
		strcat(connlist, ",");
		strcat(connlist, ret);
		midpFree(ret);
	    }
	    ret = NULL;

	}
    }
    return connlist;
}

/*
 * Parse the persistent push registry from disk into the 
 * in memory cache representation.
 */
static int parsePushList () {
    char buffer[MAX_LINE+1];
    int i;
    int line = 0;
    int eol = 0;
    int len; 
    char *errStr = NULL ;
    PushEntry * pe;

    /* Read a line at a time */
    for (i= 0 ; i < MAX_LINE ; i++) {
	/* Reset the buffer pointer at the start of a new line. */
	if(eol) {
	    i = 0; 
	    eol = 0;
	}
	len = storageRead(&errStr, pushfd, &buffer[i], 1);
	if (errStr != NULL) {
	    /* fprintf (stderr, "Warning: could not read push registration: %s\n", errStr);*/
	    storageFreeError(errStr);
	    return -1;
	}
	/* End of File */
	if (len <= 0)
	    break ; 

	/* End of Line */
	if ((buffer[i] == '\n') || (i == MAX_LINE )) {
	    line++;
	    buffer[i] = 0;
	    
	    /* Skip comment lines which begin  with '#'*/
	    if (buffer[0] != '#') {
	      
            if (pe = (PushEntry *) midpMalloc (sizeof(PushEntry))) {
                pe->next = pushlist;
                pe->value = midpStrdup(buffer);
                pe->storagename = midpStrdup(pushstorage(pe->value, 3));

                if ((pe->value == NULL) || (pe->storagename == NULL)) {
                    midpFree(pe->value);
                    midpFree(pe->storagename);
                    midpFree(pe);

                    return -2;

                } else {

		            pe->filter = pushfilter(pe->value);
		            pe->fdsock = -1;
		            pe->state = AVAILABLE ;
		            pe->dg = NULL;
		            pushProcessPort(buffer, &(pe->fd), &(pe->port));
                }
		    }

		    /*
		     * Add the new entry to the top of the push cached
		     * list. 
		     */
		    pushlist = pe ;
		    pushlength++;
	    }

	    /* Reset to beginning of next line. */
	    eol = 1;
	}
	/* Ignore carriage returns */
	if (buffer[i] == '\r') {
	    i-- ;
	}
	
    }
    return 0;
}
/*
 * Parse the port number from the connection field
 * and use it for the connection appropriate open
 * call. The file descriptor will be included in 
 * the connection registry until needed by the 
 * application level connection open request.
 */
static void pushProcessPort(char *buffer, int *fd, int *port){
    char *p;
    int colon_found;
    char *exception = NULL;
    
    /* 
     * Open the file descriptor so it can be monitored
     * for inbound connection requests. For MIDP 2.0
     * only socket and datagram connections are supported.
     */
    p = buffer;
    colon_found = 0;
    *port = -1;
    for (; *p != '\0' ; p++) {
	if (*p == ':') {
	    colon_found++ ;
	}
	if(colon_found == 2) {
	    p++ ;
	    /*
	     * Parse the port number from the
	     * connection string
	     */
	    *port = atoi(p);
	    if(strncmp(buffer,"datagram://:",12) == 0) {
		/* Open the datagram socket */
		*fd = prim_com_sun_midp_io_j2me_datagram_Protocol_open0(*port,
						   &exception);

#ifndef BLOCKING_NET_IO
		/* For non-blocking systems only */
		prim_com_sun_midp_io_j2me_datagram_Protocol_setNonBlocking(*fd);
#endif
	    } else if(strncmp(buffer,"socket://:",10) == 0) {
		/* Open the server socket */
		*fd = prim_com_sun_midp_io_j2me_serversocket_Protocol_open0(*port,
						    &exception);
#ifndef BLOCKING_NET_IO
		/* For non-blocking systems only */
		prim_com_sun_midp_io_j2me_socket_Protocol_setNonBlocking(*fd);
#endif

	    }
			    
	    return;
	}
    }			
    return;
}

static int checkfilter(char *filter, char *ip) {
    char *p1 = NULL;
    char *p2 = NULL;

    if ((ip == NULL) || (filter == NULL)) return 0;

    /* Filter is exactly "*", then all IP numbers are allowed. */
    if (strcmp(filter, "*") == 0) return 1;

    /* 
     * Otherwise walk through the filter string looking for character
     * matches and wildcard matches for each period separated field.
     * The filter pointer is incremented in the main loop and the
     * IP address pointer is incremented as characters and wildcards
     * are matched.
     */
    for (p1=filter, p2=ip; *p1; p1++) {
	if (*p1 == '*') {
	    char nextp = *(p1+1);
	    /* Match the rest of the IP field. */
	    while (*p2++) {
		/* 
		 * Break the wild card at the next field separator
		 * or the next character that matches from the 
		 * pattern string.
		 */
		if(*p2 == '.' || *p2 == nextp) {
		    break;
		}
	    }
	} else if (*p1 == '?') {
	    p2 ++;
	} else if (*p1 != *p2) {
	    /* If characters do not match, filter failed. */
	    return 0;
	} else {
	    p2 ++;
	}
    } 

    return 1;
}


static jlong alarmcheck(jlong time) {
    AlarmEntry *alarmp;
    AlarmEntry *alarmnext;

    /* Find any alarm that has expired. */
    for (alarmp = alarmlist; alarmp != NULL; alarmp = alarmnext) {
	alarmnext = alarmp->next;
	if ((time - alarmp->wakeup) > 0) {
	    return alarmp->wakeup;
	}
    }
    return -1 ;
}

static void alarmListFree() {
    AlarmEntry *alarmp, *alarmtmp;

    /* clean up the list */
    for (alarmp = alarmlist; alarmp != NULL; alarmp = alarmtmp) {
    	alarmtmp = alarmp->next;

        midpFree(alarmp->midlet);
        midpFree(alarmp->storagename);
        midpFree(alarmp);
    }
}

static void pushListFree() {
    PushEntry *pushp, *pushtmp;

    /* clean up the list */
    for (pushp = pushlist; pushp != NULL; pushp = pushtmp) {
	pushtmp = pushp->next;
	    
	/* Close the socket if it's currently open.*/
	if (pushp->fd != -1){
	    prim_com_sun_midp_io_j2me_socket_Protocol_close0(pushp->fd);
	}
	
	midpFree(pushp->value);
	midpFree(pushp->filter);
	midpFree(pushp->storagename);
	midpFree(pushp);
    }
}

static int pushpoll() {
    int i;
    int ret;
    int fd = -1;
    PushEntry * pe;

    if (pushlength > 0 ) {
#ifdef UNIX
        struct pollfd *pfd = (struct pollfd*) 
            midpMalloc(sizeof(struct pollfd) * pushlength);
	
        if (pfd != NULL) {
            for(i = 0, pe = pushlist; i < pushlength && pe != NULL; i++) {
                pfd[i].fd = pe->fd;
                pfd[i].events = POLLIN;
                pfd[i].revents = 0;
                pe = pe->next;
            }
	
            if ((ret = poll(pfd, pushlength, 0)) > 0 ){
                for (i = 0 ; i < pushlength; i++) {
                    if (pfd[i].revents != 0) {
                        if ((pfd[i].revents & POLLERR) == POLLERR) {
                            fprintf(stdout, "Error on fd=%d\n", pfd[i].fd);
                        } else if ((pfd[i].revents & POLLHUP) == POLLHUP) {
                            fprintf(stdout, "Hang up on fd=%d\n", pfd[i].fd);
                        } else if ((pfd[i].revents & POLLNVAL) == POLLNVAL) {
                            fprintf(stdout, "Invalid fd=%d\n", pfd[i].fd);
                        } else {
                            fd = pfd[i].fd; 
                        }
                    }
                }
	        } 
	        midpFree(pfd);
	        pfd = NULL;

        }
#else /* WINDOWS */

        fd_set socks;
        int highsock=0;
        int count =0;
        struct timeval timeout;
        FD_ZERO(&socks);
    
        for (pe = pushlist; pe != NULL; pe = pe->next) {
            FD_SET(pe->fd, &socks);
            if (pe->fd > highsock) {
                highsock = pe->fd;
            }
        }
        timeout.tv_sec = 0;
        timeout.tv_usec=10;
   
        count = select(highsock+1, &socks,(fd_set *) NULL, (fd_set *) NULL, 
                       &timeout);
        if (count > 0 ){
            for (pe = pushlist; pe != NULL; pe = pe->next) {
                if (FD_ISSET(pe->fd, &socks)) {
                    fd = pe->fd;
                    break;
                }
            }
        }
#endif
    }    
    return fd;
}

/*
 * Delete all of connections for a suite, using the storage name
 */
static void pushdeletesuite(char *store) {
    PushEntry *pushp; 
    PushEntry **pPrevNext = &pushlist;
    PushEntry *pushnext;

    AlarmEntry *alarmp; 
    AlarmEntry **alarmpPrevNext = &alarmlist;
    AlarmEntry *alarmnext;

    /* Find all of the entries to remove */
    for (pushp = pushlist; pushp != NULL; pushp = pushnext) {
	pushnext = pushp->next;
	if (strcmp(store, pushp->storagename) == 0) {
            *pPrevNext = pushp->next;
	    pushlength-- ;
	    
	    /* Close the socket if it's currently open.*/
	    if (pushp->fd != -1){
		prim_com_sun_midp_io_j2me_socket_Protocol_close0(pushp->fd);
	    }

	    midpFree(pushp->value);
	    pushp->value = NULL;
	    midpFree(pushp->filter);
	    pushp->filter = NULL;
	    midpFree(pushp->storagename);
	    pushp->storagename = NULL;
	    midpFree(pushp);
            continue;
	}

        pPrevNext = &pushp->next;
    }

    pushsave();

    /* Find all of the alarm entries to remove */
    for (alarmp = alarmlist; alarmp != NULL; alarmp = alarmnext) {
	alarmnext = alarmp->next;
	if (strcmp(store, alarmp->storagename) == 0) {
            *alarmpPrevNext = alarmp->next;

	    midpFree(alarmp->midlet);
	    alarmp->midlet = NULL;
	    midpFree(alarmp->storagename);
	    alarmp->storagename = NULL;
	    midpFree(alarmp);
            continue;
	}

        alarmpPrevNext = &alarmp->next;
    }

    alarmsave();
}

/*
 * Parse the persistent alaram registry from disk into the 
 * in memory cache representation.
 */
static int parseAlarmList () {
    char buffer[MAX_LINE+1];
    int i;
    int line = 0;
    int len; 
    char *errStr = NULL ;
    jlong alarm = 0;
    AlarmEntry * pe;
    char *p;

    /* Read a line at a time */
    for (i= 0 ; i < MAX_LINE ; i++) {
	len = storageRead(&errStr, pushfd, &buffer[i], 1);
	if (errStr != NULL) {
	    /* fprintf (stderr, "Warning: could not read alarm registration: %s\n", errStr);*/
	    storageFreeError(errStr);
	    return -1;
	}
	/* End of File */
	if (len <= 0){
	    break ; 
	}	
	/* End of Line */
	if ((buffer[i] == '\n') || (i == MAX_LINE )) {
	    line++;
	    buffer[i] = 0;
	    
	    /* Skip comment lines which begin  with '#'*/
	    if (buffer[0] != '#') {
		/* Find the alarm time field. */
		for (p = buffer ; *p ; p++) {
		    if (*p == ',') {
			p++;
			alarm = atoll(p);
			break;
		    }
		}
		/* Create an alarm registry entry. */
		if (pe = (AlarmEntry *) midpMalloc (sizeof(AlarmEntry))){
		    pe->next = alarmlist;
		    pe->midlet = midpStrdup(buffer);
            pe->wakeup = alarm;
            pe->storagename = midpStrdup(pushstorage(pe->midlet, 2));

            if ((pe->midlet == NULL) || (pe->storagename == NULL)) {

                midpFree(pe->midlet);
                midpFree(pe->storagename);
                midpFree(pe);
                pe = NULL;
            }
		}

        if (pe == NULL) {
            alarmListFree();
            return -2;
        }

		/*
		 * Add the new entry to the top of the alarm cached
		 * list. 
		 */
		alarmlist = pe ;
	    }
	}
	/* Ignore carriage returns */
	if (buffer[i] == '\r') {
	    i-- ;
	}
    }
    return 0;
}

/*
 * Open the Alram Registry file, if it exists and populate
 * an in memory cache of the file contents.
 */
static void alarmopen() {

    /* Now read the registered connections.*/
    if ((pushfd = storageOpen(&errStr, alarmpathname, OPEN_READ)) != -1){
	/* Read through the file one line at a time */
	if (parseAlarmList() == -2) {
        fprintf(stderr, "Error: alarmopen out of memory "
                        "when parsing alarm list.\n");
        exit(-1);
    }
        

	/* Close the storage handle */
	storageClose (&errStr, pushfd);
    } else {
	if (errStr != NULL) {
	    /*	    fprintf (stderr, "Warning: could not open alarm registration file(%s): %s\n",
		    alarmpathname, errStr); */
	    storageFreeError(errStr);
	}
    }
}

/*
 * Save the in memory cache of alarm registrations to a persistent
 * file for use in subsequent runs.
 */
static void alarmsave() {
    AlarmEntry *alarmp;
    AlarmEntry *alarmtmp=NULL;
    if ((pushfd = storageOpen(&errStr, alarmpathname, OPEN_READ_WRITE_TRUNCATE)) != -1){
	/* Write a new list of push registrations to the persistent file */
	for (alarmp = alarmlist; alarmp != NULL ; alarmp = alarmtmp) {
	    alarmtmp = alarmp->next;
	    storageWrite(&errStr, pushfd, alarmp->midlet, strlen(alarmp->midlet));
	    storageWrite(&errStr, pushfd, "\n", 1);
	}
	/* Close the storage handle */
	storageClose (&errStr, pushfd);
    } else {
	if (errStr != NULL) {
	    /*	    fprintf (stderr, "Warning: could not write alarm registration file(%s): %s\n",
		    alarmpathname, errStr); */
	    storageFreeError(errStr);
	    return;
	}
    }
}


/*
 * Add one entry to the alarm registry.
 * If the entry already exists return previous alarm time.
 * On succesful registration, write a new copy of the file to disk.
 */
static int alarmadd(char *str, jlong alarm, jlong *lastalarm){
    AlarmEntry *alarmp;    
    AlarmEntry *alarmtmp = NULL;    
    AlarmEntry *lastp = alarmlist;
    AlarmEntry *pe = NULL;
    char *ptr;
    int len;

    /* Find the length of the midlet field */
    for (ptr = str, len = 0; *ptr != 0 ; ptr++, len++) {
	if (*ptr == ',') break;
    }
    
    /* Check if the entry already exists? */
    for (alarmp = alarmlist; alarmp != NULL ; alarmp = alarmtmp) {
        alarmtmp = alarmp->next;
	if (strncmp (str, alarmp->midlet, len) == 0) {
	    jlong temp = alarmp->wakeup;
	    if(alarm == 0) {
		/* Remove an entry. */
		if (lastp == alarmlist){
		    alarmlist = alarmp->next;
		} else {
		    lastp->next = alarmp->next;
		}
		midpFree(alarmp->midlet);
		midpFree(alarmp->storagename);
		midpFree(alarmp);
		alarmsave();
	    } else {
		/* Replace an entry. */
		alarmp->wakeup = alarm;
		alarmsave();
	    }
	    *lastalarm = temp;
	    return 0;
	}
	lastp = alarmp;
    }

    /* Add a new entry. */
    if (alarm && (pe = (AlarmEntry *) midpMalloc (sizeof(AlarmEntry)))){
        pe->next = alarmlist ;
        pe->midlet = midpStrdup(str);
        pe->wakeup = alarm ;
        pe->storagename = midpStrdup(pushstorage(pe->midlet, 2));

        if ((pe->midlet == NULL) || (pe->storagename == NULL)) {
            midpFree(pe->midlet);
            midpFree(pe->storagename);
            midpFree(pe);
            pe = NULL;
        } else {
	    alarmlist = pe ;
	}
    }

    if (pe == NULL) {
        return -2;
    }

    alarmsave();
    return 0 ; 
}
 
/*=========================================================================
  * FUNCTION:      Object del0(String) (STATIC)
  * CLASS:         com.sun.midp.io.j2me.push.PushRegistryImpl.del0
  * TYPE:          virtual native function
  * OVERVIEW:      Delete an entry from the push registry
  * INTERFACE (operand stack manipulation):
  *   parameters:  a string object , a string object
  *   returns:     0 if succesfully deleted, -1 if connection was not found
  *                -2 if connection found but belongs to another 
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_push_PushRegistryImpl_del0() {
    char *szConn = NULL;
    int connLen;
    char *szStore = NULL;
    int storeLen;
    int ret = -1;

    KNI_StartHandles(2);
    KNI_DeclareHandle(conn);
    KNI_DeclareHandle(storage);

    /* Get the connection string. */
    KNI_GetParameterAsObject(1, conn);
    connLen = KNI_GetArrayLength(conn);
    if ((szConn = midpMalloc(connLen)) != NULL) {
	KNI_GetRawArrayRegion(conn, 0, connLen, (jbyte*)szConn);
   
	/* Get the storage name string. */
	KNI_GetParameterAsObject(2, storage);
	storeLen = KNI_GetArrayLength(storage);
	if ((szStore = midpMalloc(storeLen)) != NULL) {
	    KNI_GetRawArrayRegion(storage, 0, storeLen, (jbyte*)szStore);
	    
	    /* Perform the delete operation. */
	    ret = pushdel(szConn, szStore);
	    midpFree(szStore);
	} else {
	    KNI_ThrowNew("java/lang/OutOfMemoryError", "connection");
	}
	midpFree(szConn);

    } else {
	KNI_ThrowNew("java/lang/OutOfMemoryError", "connection");
    }
    KNI_EndHandles();  
    
    KNI_ReturnInt(ret);
}

/*=========================================================================
  * FUNCTION:      Object checkInByName0(String) (STATIC)
  * CLASS:         com.sun.midp.io.j2me.push.PushRegistryImpl
  * TYPE:          virtual native function
  * OVERVIEW:      check in a handle back into the push registry
  * INTERFACE (operand stack manipulation):
  *   parameters:  a string object , int flag
  *   returns:     0 if succesfully checked in, -1 if connection was not found
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_push_PushRegistryImpl_checkInByName0() {
    char *szConn = NULL;
    int connLen;
    int ret = -1;

    KNI_StartHandles(1);
    KNI_DeclareHandle(conn);

    KNI_GetParameterAsObject(1, conn);
    connLen = KNI_GetArrayLength(conn);

    szConn = midpMalloc(connLen);
    if (szConn != NULL) {
        KNI_GetRawArrayRegion(conn, 0, connLen, (jbyte*)szConn);
        ret = pushcheckinbyname(szConn);
        midpFree(szConn);
    } else {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "connection");
    }

    KNI_EndHandles();  
    KNI_ReturnInt(ret);
}

/*=========================================================================
  * FUNCTION:      Object add0(String) (STATIC)
  * CLASS:         com.sun.midp.io.j2me.PushRegistryImpl.add0
  * TYPE:          virtual native function
  * OVERVIEW:      Add an entry from the push registry
  * INTERFACE (operand stack manipulation):
  *   parameters:  a string object 
  *   returns:     0 if succesfully added, -1 if connection was not found
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_push_PushRegistryImpl_add0() {

    char *szConn = NULL;
    int connLen;
    int ret = -1;

    KNI_StartHandles(1);
    KNI_DeclareHandle(conn);

    KNI_GetParameterAsObject(1, conn);
    connLen = KNI_GetArrayLength(conn);

    szConn = midpMalloc(connLen);

    if (szConn != NULL) {
        KNI_GetRawArrayRegion(conn, 0, connLen, (jbyte*)szConn);
        ret = pushadd(szConn);
        midpFree(szConn);
    }
    
    if ((szConn == NULL) || (ret == -2)) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "connection");
    }

    KNI_EndHandles();  
    KNI_ReturnInt(ret);
}

/*=========================================================================
  * FUNCTION:      Object addAlarm0(String) (STATIC)
  * CLASS:         com.sun.midp.io.j2me.PushRegistryImpl.addAlarm0
  * TYPE:          virtual native function
  * OVERVIEW:      Add an entry from the alarm registry
  * INTERFACE (operand stack manipulation):
  *   parameters:  a string object , long
  *   returns:     0 if first time alarm is set, otherwise the previous
  *                defined alarm time
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_LONG
Java_com_sun_midp_io_j2me_push_PushRegistryImpl_addAlarm0() {

    char *szConn = NULL;
    int connLen;
    jlong alarm = 0;
    jlong lastalarm = 0;
    int ret = 0;

    alarm = KNI_GetParameterAsLong(2);

    KNI_StartHandles(1);
    KNI_DeclareHandle(conn);

    KNI_GetParameterAsObject(1, conn);
    connLen = KNI_GetArrayLength(conn);

    szConn = midpMalloc(connLen);
    if (szConn != NULL) {
        KNI_GetRawArrayRegion(conn, 0, connLen, (jbyte*)szConn);
        ret = alarmadd(szConn, alarm, &lastalarm);
        midpFree(szConn);
    }

    if ((szConn == NULL) || (ret == -2)) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "connection");
    }

    KNI_EndHandles();  
    KNI_ReturnLong(lastalarm);
}

/*=========================================================================
  * FUNCTION:      Object list0(byte[], boolean, byte[], int) (STATIC)
  * CLASS:         com.sun.midp.io.j2me.push.PushRegistryImpl.list0
  * TYPE:          virtual native function
  * OVERVIEW:      List registry entries for specified MIDlet
  * INTERFACE (operand stack manipulation):
  *   parameters:  byte[], boolean, byte[], int 
  *   returns:     
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_push_PushRegistryImpl_list0() {
    char midletName[MAX_HOST_LENGTH];
    int available;
    int connsize;
    int nameLength;
    char *conn;
    int ret = -1;

    available = KNI_GetParameterAsBoolean(2);
    connsize = KNI_GetParameterAsInt(4);

    KNI_StartHandles(2);

    KNI_DeclareHandle(name);
    KNI_DeclareHandle(connections);
    KNI_GetParameterAsObject(1, name);
    KNI_GetParameterAsObject(3, connections);

    nameLength = KNI_GetArrayLength(name);
    if (nameLength > MAX_HOST_LENGTH) {
        KNI_ThrowNew("java/lang/IllegalArugmentException",
                     "MIDlet name  too long");
    }
        
    KNI_GetRawArrayRegion(name, 0, nameLength, (jbyte*)midletName);


    conn = pushfindsuite(midletName, available);

    if (conn != NULL) {
        KNI_SetRawArrayRegion(connections, 0, strlen(conn), (jbyte*)conn);
        midpFree(conn);
        ret = 0;
    }

    KNI_EndHandles();

    KNI_ReturnInt(ret);
}
/*=========================================================================
  * FUNCTION:      Object poll0() 
  * CLASS:         com.sun.midp.io.j2me.push.PushRegistryImpl.poll0
  * TYPE:          virtual native function
  * OVERVIEW:      Check for inbound connection
  * INTERFACE (operand stack manipulation):
  *   parameters:  long
  *   returns:    fd of first active connection 
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_LONG
Java_com_sun_midp_io_j2me_push_PushRegistryImpl_poll0() {
    jlong ret = (jlong) pushpoll();

    if(ret == -1) {
	/* If there is no pending I/O, check for alarms */
	jlong time = KNI_GetParameterAsLong(1);

	ret = alarmcheck(time);
    }
    KNI_ReturnLong(ret);
}


/*=========================================================================
  * FUNCTION:      Object getMIDlet0(long, byte[], int) (STATIC)
  * CLASS:         com.sun.midp.io.j2me.push.PushRegistryImpl.getMIDlet0
  * TYPE:          virtual native function
  * OVERVIEW:      Get the registered MIDlet name given the current active 
  *                file descriptor.
  * INTERFACE (operand stack manipulation):
  *   parameters:  fd or time, byte[],  int 
  *   returns:     
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_push_PushRegistryImpl_getMIDlet0() {
    int midletsize;
    char *regentry;
    int ret = -1;
    jlong fd;

    midletsize = KNI_GetParameterAsInt(4);
    fd =  KNI_GetParameterAsLong(1);

    KNI_StartHandles(1);

    KNI_DeclareHandle(regObject);
    KNI_GetParameterAsObject(3, regObject);

    if (regentry = pushfindfd(fd)) {
	KNI_SetRawArrayRegion(regObject, 0, strlen(regentry)+1,
            (jbyte*)regentry);
	midpFree(regentry);
	ret = 0;
    }
    KNI_EndHandles();

    KNI_ReturnInt(ret);
}

/*=========================================================================
  * FUNCTION:      Object getEntry0(byte[], byte[], int) (STATIC)
  * CLASS:         com.sun.midp.io.j2me.push.PushRegistryImpl.getEntry0
  * TYPE:          virtual native function
  * OVERVIEW:      Get the registered MIDlet name given the registered
  *                connection string
  * INTERFACE (operand stack manipulation):
  *   parameters:  byte[], byte[],  int 
  *   returns:     
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_push_PushRegistryImpl_getEntry0() {
    int midletsize;
    char *regentry;
    int regsize ;
    int ret = -1;
    int connLen;
    char *szConn = NULL;
    
    midletsize = KNI_GetParameterAsInt(3);
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(conn);
    KNI_DeclareHandle(regObject);
    
    KNI_GetParameterAsObject(1, conn);
    connLen = KNI_GetArrayLength(conn);
    ret = -1;
    if ((szConn = midpMalloc(connLen)) != NULL) {
	KNI_GetRawArrayRegion(conn, 0, connLen, (jbyte*)szConn);
	
	KNI_GetParameterAsObject(2, regObject);
	
	if (regentry = pushfindconn(szConn)) {
	    regsize = strlen(regentry) + 1;
	    if (regsize < midletsize) {
		KNI_SetRawArrayRegion(regObject, 0, regsize,
				      (jbyte*)regentry);
		ret = 0;
	    } else {
		KNI_ThrowNew("java/io/IOException",
			     "registration too long");
	    }
	}
	midpFree(szConn);
    } else {
	/* Insufficient memory. */
	KNI_ThrowNew("java/lang/OutOfMemoryError", "connection");
    }
    KNI_EndHandles();

    KNI_ReturnInt(ret);
}

/*=========================================================================
  * FUNCTION:      Object delAllForSuite0(byte[])
  * CLASS:         com.sun.midp.io.j2me.PushRegistryImpl
  * TYPE:          virtual native function
  * OVERVIEW:      Remove all the entries of a suite from the push registry
  * INTERFACE (operand stack manipulation):
  *   parameters:  a zero terminated ASCII string containing the storage name
  *   returns:     nothing
  *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_push_PushRegistryImpl_delAllForSuite0() {

    char* pszStorageName;
    int len;

    KNI_StartHandles(1);
    KNI_DeclareHandle(name);

    KNI_GetParameterAsObject(1, name);
    len = KNI_GetArrayLength(name);
    pszStorageName = (char*)midpMalloc(len);
    if (pszStorageName == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "del all storage name");
    } else {
        KNI_GetRawArrayRegion(name, 0, len, (jbyte*)pszStorageName);
        pushdeletesuite(pszStorageName);
        midpFree(pszStorageName);
    }

    KNI_EndHandles();  

    KNI_ReturnVoid();
}
