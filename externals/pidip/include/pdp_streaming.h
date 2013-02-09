/*
 * pdp_streaming.h : structure and defines for PDP packet streaming
 * Copyright (C) 2001-2002 Yves Degoyon
 *
 */

/*
 * this is the format of how a packet is transmitted
 * between pdp_o and pdp_i            
 * it starts with a tag to recognize the beginning
 * of a packet, then header informations ( width, height, timestamp )  
 * and, finally, the bz2 compressed data      
 */

#include <time.h>
#include <sys/time.h>

#define TAG_LENGTH 8

#define PDP_PACKET_START "SPDP"
#define PDP_PACKET_TAG PDP_PACKET_START"PAC"
#define PDP_PACKET_DIFF PDP_PACKET_START"DIF"
#define REGULAR 0
#define HUFFMAN 1

typedef struct _hpacket
{
  char tag[TAG_LENGTH];
  int encoding;
  int width;
  int height;
  struct timeval etime; // valid until 2038
  unsigned int clength;
} t_hpacket;
