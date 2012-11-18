#ifndef XBEE_INTERNAL_H
#define XBEE_INTERNAL_H

#ifdef AVR
# define __LITTLE_ENDIAN 1234
# define BYTE_ORDER LITTLE_ENDIAN
#elif defined(ARM)
# include <endian.h>
#else
# include <arpa/inet.h>
#endif

#include <stdint.h>

#include "xbee.h"

#if !defined(ntohs) && (BYTE_ORDER == LITTLE_ENDIAN)
# define ntohs(n) ((((short)(n)) & 0xff00) >> 8 | (((short)(n)) & 0xff) << 8)
# define htons(n) ntohs(n)
#elif !defined(ntohs)
# define ntohs(n) ((short)(n))
# define htons(n) ntohs(n)
#endif

#if !defined(ntohl) && (BYTE_ORDER == LITTLE_ENDIAN)
# define ntohl(x) ((((x)&0xff000000)>>24) \
                  |(((x)&0x00ff0000)>>8)  \
                  |(((x)&0x0000ff00)<<8)  \
                  |(((x)&0x000000ff)<<24))
# define htonl(n) ntohl(n)
#elif !defined(ntohl)
# define ntohl(n) ((long)(n))
# define htonl(n) ntohs(n)
#endif

/* p2p  CE=0 (end devices) A1=0 (no end dev assoc) same ID/CH
 * coordinator: CE=1, A2=n (coordinator assoc)
 *   SP= sleep perd ST= time betw sleep (should be same on
 *   coord/noncoord)
 * assoc - coord'd only; comm between modules thru coord'r
 *  PAN's - need coordinator.  A1 allows totally dynamic assoc
 */
 
 
/* --- General XBee Definitions --- */
 
/* "Start of packet" byte; always sent as the first
 *  byte of each packet
 */ 
#define XBEE_PKT_START 0x7e


/* Maximum packet size; datasheet basically says 100 payload bytes max */
#define XBEE_MAX_DATA_LEN        128


/* --- Bits in packets --- */

/* Communication status bits */

#define XBEE_STATUS_HW_RESET      0x01
#define XBEE_STATUS_WD_RESET      0x02
#define XBEE_STATUS_ASSOC         0x04
#define XBEE_STATUS_DISASSOC      0x08
#define XBEE_STATUS_SYNC_LOST     0x10
#define XBEE_STATUS_COORD_REALIGN 0x20
#define XBEE_STATUS_COORD_RESET   0x40

/* Command status bits */

#define XBEE_CMDSTATUS_OK  0
#define XBEE_CMDSTATUS_ERR 1

/* Transmit options */

#define XBEE_TX_FLAG_NO_ACK 0x01
#define XBEE_TX_FLAG_SEND_BCAST_PAN_ID 0x04

/* Transmit status bits */

#define XBEE_TXSTATUS_SUCCESS  0x00
#define XBEE_TXSTATUS_NO_ACK   0x01
#define XBEE_TXSTATUS_CCA_FAIL 0x02
#define XBEE_TXSTATUS_PURGES   0x03

/* Received options */

#define XBEE_RX_FLAG_ADDR_BCAST 0x02
#define XBEE_RX_FLAG_PAN_BCAST  0x04


/* --- Definitions & macros for library use --- */

/* For tracking memory allocations  */
#define XBEE_RECV                0x00
#define XBEE_XMIT                0x01

/* Initialize an XBee header */
#define xbee_hdr_init(hdr, data_len) \
         ((hdr).start = 0x7e, (hdr).len = htons(data_len)) 

/* To get the length of the data portion of a received packet */

#define xbee_recv_a64_data_len(pkt) (ntohs(pkt->hdr.len) - 11)
#define xbee_recv_a16_data_len(pkt) (ntohs(pkt->hdr.len) - 5)
#define xbee_cmd_resp_param_len(pkt) (ntohs(pkt->hdr.len) - 5)

#ifdef XBEE_ALLOC
# define xbee_alloc_pkt(dir, data_len) \
   (xbee_pkt_t *)xbee_alloc_buf((dir), (data_len) + sizeof(xbee_pkt_hdr_t) + 1)
#endif

/* Types of packets from/to xbee modules; these are used
 *  in the "type" field of each packet structure
 */
 
typedef enum {
    XBEE_PKT_TYPE_TX64        = 0x00,
    XBEE_PKT_TYPE_TX16        = 0x01,
    XBEE_PKT_TYPE_ATCMD       = 0x08,
    XBEE_PKT_TYPE_QATCMD      = 0x09, /* wait til an immed param or apply cmd */
	XBEE_PKT_TYPE_REMOTE_ATCMD = 0x17,
    XBEE_PKT_TYPE_RX64        = 0x80,
    XBEE_PKT_TYPE_RX16        = 0x81,
    XBEE_PKT_TYPE_RX64_IO     = 0x82,
    XBEE_PKT_TYPE_RX16_IO     = 0x83,
    XBEE_PKT_TYPE_ATCMD_RESP  = 0x88,
    XBEE_PKT_TYPE_TX_STATUS   = 0x89,
    XBEE_PKT_TYPE_MODEM_STATUS= 0x8a,
} xbee_pkt_type_t;


/* --- Packet layouts --- */

typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t frame_id;
    uint8_t command[2];
    uint8_t param[0];
} __attribute__ ((__packed__)) xbee_at_cmd_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t frame_id;
	uint8_t dest64[8];
	uint8_t dest16[2];
	uint8_t apply;
    uint8_t command[2];
    uint8_t param[0];
} __attribute__ ((__packed__)) xbee_remote_at_cmd_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t frame_id;
    uint8_t dest[8];
    uint8_t opt;
    uint8_t data[0];
} __attribute__ ((__packed__)) xbee_a64_tx_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t frame_id;
    uint8_t dest[2];
    uint8_t opt;
    uint8_t data[0];
} __attribute__ ((__packed__)) xbee_a16_tx_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t frame_id;
    uint8_t status;
} __attribute__ ((__packed__)) xbee_tx_status_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t status;
} __attribute__ ((__packed__)) xbee_modem_status_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t frame_id;
    uint8_t command[2];
    uint8_t status;
    uint8_t param[0];
} __attribute__ ((__packed__)) xbee_cmd_resp_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t src[8];
    uint8_t rssi; /* signal strength */
    uint8_t opt;
    uint8_t data[0];
} __attribute__ ((__packed__)) xbee_a64_rx_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t src[2];
    uint8_t rssi;
    uint8_t opt;
    uint8_t data[0];
} __attribute__ ((__packed__)) xbee_a16_rx_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t src[8];
    uint8_t rssi; /* signal strength */
    uint8_t opt;
    uint8_t num_samples;
    uint16_t ch_ind; /* bits 14-9: a5-a0 bits 8-0: d8-d0 active */
    uint16_t data[0]; /* First sample digital if any digital chan active
                          rest are 16-bit analog rdgs                  */
} __attribute__ ((__packed__)) xbee_io_a64_rx_pkt_t;


typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t type;
    uint8_t src[2];
    uint8_t rssi;
    uint8_t opt;
    uint8_t num_samples;
    uint8_t achan;
    uint16_t ch_ind; /* bits 14-9: a5-a0 bits 8-0: d8-d0 active */
    uint16_t data[0]; /* First sample digital if any digital chan active
                          rest are 16-bit analog rdgs                  */
} __attribute__ ((__packed__)) xbee_io_a16_rx_pkt_t;

#endif /* #ifndef XBEE_INTERNAL_H ... */

