/*
 *  xbee_io.h
 *  xbee_test
 *
 *  Created by Tymm on 11/24/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef XBEE_IO_H
#define XBEE_IO_H

#include <pthread.h>

#include "xbee.h"


#define XBEE_IO_BUFSIZ 16384

typedef struct {
	int fd;
	int pipe_fds[2];   // For indicating new data ready
	pthread_t io_thread;
	int io_done;
	int io_running;
	int out_buffer_start;
	int out_buffer_end;
	char out_buffer[XBEE_IO_BUFSIZ];
} xbee_io_context_t;

int xbee_new_io_thread(xbee_t *xbee);
int xbee_kill_io_thread(xbee_t *xbee);

#endif /* #ifndef XBEE_IO_H ... */
