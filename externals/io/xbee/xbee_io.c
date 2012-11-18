/*
 *  xbee_io.c
 *  xbee_test
 *
 *  Created by Tymm on 11/20/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/select.h>
#include <pthread.h>

#ifdef PD
#include "m_pd.h"
#include "max2pd.h"
#else
#include "ext.h"
#endif /* PD */

#include "xbee.h"
#include "xbee_io.h"


#ifndef MIN
# define MIN(a,b) ((a)<(b)?(a):(b))
#endif


void xbee_io_read(xbee_t *xbee)
{
	int r;
	char data[128];
	xbee_io_context_t *ctx = xbee_user_context(*xbee);
	int xbee_fd = ctx->fd;

		
	r = read(xbee_fd, data, sizeof(data));
	if (r <= 0) {
		return;
	}
		
	xbee_in(xbee, data, r);
}


void xbee_io_write(xbee_t *xbee)
{
	int r;
	int max_bytes_to_write;
	xbee_io_context_t *ctx = xbee_user_context(*xbee);
	int xbee_fd = ctx->fd;
	int tmp_start = ctx->out_buffer_start;
	
	
	if (tmp_start != ctx->out_buffer_end) {
		max_bytes_to_write = (ctx->out_buffer_end + XBEE_IO_BUFSIZ - ctx->out_buffer_start) % XBEE_IO_BUFSIZ;
		
		//for (r = 0; r < MIN(max_bytes_to_write, XBEE_IO_BUFSIZ - tmp_start); r++) {
		//	post("%02x", (unsigned char)ctx->out_buffer[tmp_start + r]);
		//}
		
		r = write(xbee_fd, (char *)(ctx->out_buffer + tmp_start), MIN(max_bytes_to_write, XBEE_IO_BUFSIZ - tmp_start));
		
		if (r < 0) {
			return;
		}
				
		tmp_start += r;
		tmp_start %= XBEE_IO_BUFSIZ;
		
		ctx->out_buffer_start = tmp_start;
		
		if (tmp_start == 0) {
			max_bytes_to_write -= r;
						
			r = write(xbee_fd, (char *)(ctx->out_buffer), max_bytes_to_write);

			if (r < 0) {
				return;
			}
			
			tmp_start += r;
			tmp_start %= XBEE_IO_BUFSIZ;
		}
		
		ctx->out_buffer_start = tmp_start;
	}
}


int xbee_put_data(xbee_io_context_t *ctx, char *data, int len)
{
	int byte_count = 0;
	int max_bytes;
	int tmp_end = ctx->out_buffer_end;
	
	
	max_bytes = MIN((ctx->out_buffer_start + XBEE_IO_BUFSIZ - tmp_end - 1) % XBEE_IO_BUFSIZ, len);
	byte_count = MIN(max_bytes, XBEE_IO_BUFSIZ - tmp_end);
	
	memcpy((char *)(ctx->out_buffer + tmp_end), data, byte_count);
	
	if (byte_count != max_bytes) {
		memcpy((char *)(ctx->out_buffer), data + byte_count, max_bytes - byte_count);
	}
	
	tmp_end += max_bytes;
	tmp_end %= XBEE_IO_BUFSIZ;
	ctx->out_buffer_end = tmp_end;
		
	return max_bytes;
}


void *xbee_io_loop(void *param)
{
	int xbee_fd;
	fd_set r_fds, w_fds;
	int nfds = 0;
	struct timeval timeout;
	struct timeval timeout_orig = { .tv_sec = 1, .tv_usec = 0 };
	xbee_t *xbee = param;
	xbee_io_context_t *ctx = xbee_user_context(*xbee);
	int notif_fd;
	int maxfd;


	xbee_fd = ctx->fd;
	notif_fd = ctx->pipe_fds[0];
	
	FD_ZERO(&r_fds);
	FD_ZERO(&w_fds);
		
	while(1) {		
		if (ctx->io_done) {
			break;
		}
		
		nfds = 0;
		
		FD_SET(notif_fd, &r_fds);
		maxfd = notif_fd;

		FD_SET(xbee_fd, &r_fds);
		if (xbee_fd > maxfd)
			maxfd = xbee_fd;
		
		FD_CLR(xbee_fd, &w_fds);		
		if (ctx->out_buffer_start != ctx->out_buffer_end) {
			FD_SET(xbee_fd, &w_fds);
		}
		
		// For Linux compat
		timeout = timeout_orig;
		
		nfds = select(maxfd + 1, &r_fds, &w_fds, NULL, &timeout);
				
		if (nfds >= 1) {
			if (FD_ISSET(notif_fd, &r_fds)) {
				// We've been notified that there's data to be read, or need to exit
				char blah;
				read(notif_fd, &blah, 1);
			}
			
			if (FD_ISSET(xbee_fd, &r_fds)) {
				xbee_io_read(xbee);
			}
			
			if (FD_ISSET(xbee_fd, &w_fds)) {
				xbee_io_write(xbee);
			}
		}
	}
	
	ctx->io_running = 0;
	
	return NULL;
}


int xbee_kill_io_thread(xbee_t *xbee)
{
	xbee_io_context_t *ctx;
	void *val;


	ctx = xbee_user_context(*xbee);
	
	if (ctx != NULL) {		
		if (ctx->io_running) {
			ctx->io_done = 1;
		
			write(ctx->pipe_fds[1], "!", 1);
		
			if (pthread_join(ctx->io_thread, &val) < 0) {
				return -1;
			}
		
			post("xbee_test: xbee io thread stopped.");
		}
	}
	
	return 0;
}


int xbee_new_io_thread(xbee_t *xbee)
{
	xbee_io_context_t *ctx = xbee_user_context(*xbee);
	int r;
	
	
	if (!ctx->io_running) {
		ctx->io_running = 1;
		
		r = pthread_create(&ctx->io_thread, NULL, &xbee_io_loop, (void *)xbee);
		if (r < 0) {
			ctx->io_running = 0;
			return -1;
		}
		
		post("xbee_test: xbee io thread started.");
	}
	
	return 0;
}


int xbee_out(xbee_t *xbee, xbee_pkt_t *pkt, uint8_t len)
{
	xbee_io_context_t *ctx = xbee_user_context(*xbee);
	int r;
	
	
	r = xbee_put_data(ctx, (void *)pkt, len);
	xbee_free_pkt_mem(pkt);
	
	return r;
}


int xbee_recv_pkt(xbee_t *xbee, xbee_pkt_t *pkt, uint8_t len)
{
	//int i;
	
	
	//for (i = 0; i < len; i++) {
	//	post("GOT %02x", ((unsigned char *)pkt)[i]);
	//}
	
	xbee_free_pkt_mem(pkt);
	return 0;
}


void *xbee_alloc_pkt_mem(uint8_t direction, uint8_t len)
{
	return sysmem_newptr(128);
}


void xbee_free_pkt_mem(xbee_pkt_t *pkt)
{
	sysmem_freeptr(pkt);
}
