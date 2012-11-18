/*
 * Copyright (C) 2003-2010 August Black
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "FifoVideoFrames.h"

FifoVideoFrames::FifoVideoFrames(int s, gavl_video_format_t * f) {
	size = s  ;  
	start = 0 ;
	end = 0 ; 
	count = 0;
	format = new gavl_video_format_t;
	gavl_video_format_copy(format, f);
	fifoPtr = new gavl_video_frame_t * [size] ;
	for(int i=0; i < size; i++) {
		fifoPtr[i] = gavl_video_frame_create( format );
	}
	pthread_mutex_init (&mut, 0);
}

FifoVideoFrames::~FifoVideoFrames() {
	//printf("deleting fifo \n");
	for(int i=0; i < size; i++) {
		 gavl_video_frame_destroy( fifoPtr[i] );
	}
	delete format;
	delete[] fifoPtr;
	pthread_mutex_destroy (&mut);
	//printf("deleted fifo \n");
}

// empty the fifo of any content
void FifoVideoFrames::Flush() {
	pthread_mutex_lock (&mut);
	start = 0 ; 
	end = 0 ; 
	count = 0;
	//printf("FifoVideoFrames::flushed size=%d\n", count);
	pthread_mutex_unlock (&mut);
}

// push an element onto the FifoVideoFrames 
bool FifoVideoFrames::Append(  gavl_video_frame_t * source) {
	bool ret = false;
	//Dump("Appending a frame ");
	pthread_mutex_lock (&mut);
	if ( count < size  ) {  // if there is room for one more
		gavl_video_frame_copy(format, fifoPtr[end],  source) ;
		fifoPtr[end]->timestamp = source->timestamp;
		fifoPtr[end]->duration = source->duration;
		if (++end >= size)
			end = 0;
		count++;
		ret = true;
	} // no room in fifo, return false
	pthread_mutex_unlock (&mut);
	//Dump("Appended a frame ");
	return ret;
}
// remove an element off the front FifoVideoFrames
bool FifoVideoFrames::Get( ) {
	bool ret = false;
	pthread_mutex_lock (&mut);
	if ( count > 0 ) {  // if there any items in the fifo
		if ( ++start >= size )
			start = 0;
		count--;
		ret = true;
	}
	pthread_mutex_unlock (&mut);
	//Dump("Got a frame ");
	return ret;
}

// get an element off the FifoVideoFrames
bool FifoVideoFrames::Get( gavl_video_frame_t * dest) {
	bool ret = false;
	//Dump("Getting a frame ");
	pthread_mutex_lock (&mut);
	if ( count > 0 ) {  // if there any items in the fifo
		gavl_video_frame_copy(format, dest, fifoPtr[start]) ;
		dest->timestamp = fifoPtr[start]->timestamp;
		dest->duration = fifoPtr[start]->duration;
		if ( ++start >= size )
			start = 0;
		count--;
		ret = true;
	}
	pthread_mutex_unlock (&mut);
	//Dump("Got a frame ");
	return ret;
}
/*
void FifoVideoFrames::Dump( char * c) {
	int i,j = 0;
	pthread_mutex_lock (&mut);
	printf("%s -----------------------\n", c);
	for( i=0;i<size;i++) {
		//j=start+i;
		//if (j >= size)
		//	j = j -size;
		j=i;
		if ( j == start)
			textcolor(BRIGHT, GREEN, BLACK);
		if (j == end)
			textcolor(BRIGHT, RED, BLACK);
		if (j == end && j == start)
			textcolor(BRIGHT, YELLOW, BLACK);
		printf("[%d]=%ld ", j, fifoPtr[j]->timestamp);
		textcolor(RESET, BLACK, WHITE);
	}
	printf("\n");
	printf("start=%02d, end=%02d, count=%02d\n", start,end,count);
	printf("-----------------------\n");
	pthread_mutex_unlock (&mut);
}
*/
bool FifoVideoFrames::FreeSpace() { bool ret; pthread_mutex_lock(&mut); ret = (count < size);pthread_mutex_unlock (&mut);  return ret;}
bool FifoVideoFrames::isEmpty() { bool c; pthread_mutex_lock(&mut); c = (count == 0) ; pthread_mutex_unlock (&mut); return c; }
bool FifoVideoFrames::isFull()  { bool c; pthread_mutex_lock(&mut); c = (count == size ); pthread_mutex_unlock (&mut); return c; } 
gavl_video_format_t * FifoVideoFrames::getFormat() { return format; };
float FifoVideoFrames::getSizePercentage() { float ret; pthread_mutex_lock(&mut);ret = count / (float) size; pthread_mutex_unlock (&mut); return ret;};
int FifoVideoFrames::getSize() { int ret; pthread_mutex_lock(&mut);ret = count; pthread_mutex_unlock (&mut); return ret;};

