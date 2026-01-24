/*
 * @(#)pushregistry.h	1.7 02/09/03 @(#)
 *
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#ifndef _PUSHREGISTRY_H
#define _PUSHREGISTRY_H
int pusheddatagram (int fd, int *ip, int * port, char * buf, int len);
int pushcheckoutaccept(int fd);
int pushcheckout(char* protocol, int port, char *store);
int pushcheckin(int fd);

void pushcheckinall();

void pushopen();
void pushclose();

#endif /* _PUSHREGISTRY_H */
