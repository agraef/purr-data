/*
 * xbee.h:
 *  Maxstream XBee module Interface Header
 *
 *          (c) 2006-2008 Tymm Twillman <tymm@booyaka.com>
 *
 *
 * NOTE: This doesn't touch hardware; it's up to developers to link in functions
 *  that handle hardware communication.
 *
 *  DEVELOPERS: Pieces you need to implement (see prototypes, below):
 *    xbee_alloc_pkt_mem   (can just return static data)
 *    xbee_free_pkt_mem    (can do nothing if not dynamic)
 *
 *    xbee_out
 *    xbee_recv_pkt
 *
 *   What you need to call from wherever you read data from UART, etc:
 *    xbee_in
 *
 *  Incoming data from UART, etc. should be passed to xbee_in; it will
 *   be built into well-formed packets and passed to xbee_recv_pkt
 *   for further processing.
 *
 *  Outgoing data will be passed to xbee_out to be passed off to
 *   the XBee hardware.
 *
 *
 */
 
#ifndef XBEE_H
#define XBEE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <errno.h>

/*----------------------------------------------------------------------------
               Definitions for commands the XBee recognizes
 ----------------------------------------------------------------------------*/

/* Basic communication parameters/values */

#define XBEE_CMD_CHANNEL                "CH" 
#define XBEE_CMD_PAN_ID                 "ID"
#define XBEE_CMD_DEST_ADDR64_HI         "DH"
#define XBEE_CMD_DEST_ADDR64_LO         "DL"
#define XBEE_CMD_SRC_ADDR16             "MY"
#define XBEE_CMD_SER_HI                 "SH"
#define XBEE_CMD_SER_LO                 "SL"
#define XBEE_CMD_RAND_DLY_SLOTS         "RN"
#define XBEE_CMD_MAC_MODE               "MM"
#define XBEE_CMD_COORD_ENA              "CE"
#define XBEE_CMD_SCAN                   "SC"
#define XBEE_CMD_SCAN_DURATION          "SD"
#define XBEE_CMD_ASSOC_END              "A1"
#define XBEE_CMD_ASSOC_COORD            "A2"
#define XBEE_CMD_ASSOC_STATUS           "AI"
#define XBEE_CMD_RSSI                   "DB"

/* Transceiver Control */

#define XBEE_CMD_PWR_LEVEL              "PL"
#define XBEE_CMD_CCA_THRESH             "CA"

/* Sleep Parameters */

#define XBEE_CMD_SLEEP_MODE             "SM"
#define XBEE_CMD_SLEEP_TIMEOUT          "ST"
#define XBEE_CMD_SLEEP_PERIOD           "SP"
#define XBEE_CMD_SLEEP_PERIOD_DISASSOC  "DP"

/* Interface parameters */

#define XBEE_CMD_DATA_RATE              "BD"
#define XBEE_CMD_PACKETIZATION_TIMEOUT  "RO"
#define XBEE_CMD_DIO7_CONFIG            "D7"
#define XBEE_CMD_DIO6_CONFIG            "D6"
#define XBEE_CMD_DIO5_CONFIG            "D5"
#define XBEE_CMD_PWM0_CONFIG            "PO"
#define XBEE_CMD_API_ENA                "AP"
#define XBEE_CMD_PULLUP_ENA             "PR"

/* Version Info */

#define XBEE_CMD_VERS_FIRMWARE          "VR"
#define XBEE_CMD_VERS_HARDWARE          "HV"
#define XBEE_CMD_VERS_FIRM_VERBOSE      "VL"

/* Received Signal Strength */

#define XBEE_CMD_RSSI_PWM_TIMER         "RP"
#define XBEE_CMD_RSS                    "DB"

/* Error counters */

#define XBEE_CMD_CCA_FAILS              "EC"
#define XBEE_CMD_ACK_FAILS              "EA"

/* AT Command Params */

#define XBEE_CMD_AT_MODE_TIMEOUT        "CT"
#define XBEE_CMD_AT_GUARD_TIME          "GT"
#define XBEE_CMD_AT_CMD_CHAR            "CC"
#define XBEE_CMD_AT_EXIT                "CN"

/* XBEE specific routing */

#define XBEE_CMD_NODE_FIND_DEST         "DN"
#define XBEE_CMD_NODE_DISCOVER          "ND"
#define XBEE_CMD_NODE_ID                "NI"
#define XBEE_CMD_ACTIVE_SCAN            "AS"
#define XBEE_CMD_FORCE_DISASSOC         "DA"
#define XBEE_CMD_ENERGY_SCAN            "ED"
#define XBEE_CMD_FORCE_POLL             "FP"

/* Misc */

#define XBEE_CMD_WRITE_PARAMS           "WR"
#define XBEE_CMD_RESET_SOFT             "FR"
#define XBEE_CMD_APPLY_CHANGES          "AC"
#define XBEE_CMD_RESTORE_DEFAULTS       "RE"


/*----------------------------------------------------------------------------
       Structures usefull for communicating with the XBee in API mode
 ----------------------------------------------------------------------------*/

/* Packets are wrapped with a start & length */
typedef struct {
    uint8_t         start; /* 0x7e */
    uint16_t        len;
} __attribute__ ((__packed__)) xbee_pkt_hdr_t;


/* Packets can be broken up into headers, a packet type, a number of data
 *  bytes and a crc (at the end of the data)
 */
typedef struct {
    xbee_pkt_hdr_t  hdr;
    uint8_t         type;
    uint8_t         data[0];
 /* uint8_t         crc; */
} __attribute__ ((__packed__)) xbee_pkt_t;


/* Context for tracking current state of communication with an
 *  XBee module
 */
typedef struct {
    struct {
        uint8_t bytes_left;
        uint8_t bytes_rcvd;
        xbee_pkt_t *packet;
        uint8_t hdr_data[sizeof(xbee_pkt_hdr_t)];
    } in;
    struct {
        uint8_t frame_id;
    } out;
    void *user_context; // yours to pass data around with
} __attribute__ ((__packed__)) xbee_t;

/* This is used for keeping track of your data as things get passed around
 *  through the xbee interface
 */
#define xbee_user_context(xbee) ((xbee).user_context)

/*----------------------------------------------------------------------------
                                Internal calls
 ----------------------------------------------------------------------------*/

/* Calculate CRC on an xbee packet */
uint8_t xbee_crc(const xbee_pkt_t *pkt);


/*----------------------------------------------------------------------------
                Generally all the functions you need to call
 ----------------------------------------------------------------------------*/

/* Receive data, calling xbee_recv_pkt on each packet when it's done 
 *  assembling; this should be called with raw data from UART, etc. 
 *  as it comes in.  *** YOU NEED TO CALL THIS ***
 */
void xbee_in(xbee_t *xbee, const void *data, uint8_t len);

/* Send a packet with a 64-bit destination address (Series 1) */
int xbee_send64(xbee_t *xbee,
                const void *data,
                uint8_t len,
                uint8_t opt,
                const uint8_t addr[8]);

/* Send a packet with a 16-bit destination address (Series 1) */
int xbee_send16(xbee_t *xbee,
                const void *data,
                uint8_t len,
                uint8_t opt,
                const uint8_t addr[2]);

/* Send a command to the xbee modem */
int xbee_send_at_cmd(xbee_t *xbee,
                     const char cmd[],
                     uint8_t param_len,
                     const uint8_t *params);

/* Send a command to a remote xbee modem (Series 2 & Newer Series 1 only) */
int xbee_send_remote_at_cmd(xbee_t *xbee,
							const char cmd[],
							uint8_t param_len,
							uint8_t apply, 
							const uint8_t params[],
							const uint8_t addr64[8],
							const uint8_t addr16[2]);

/* Initialize the XBee interface */
void xbee_init(xbee_t *xbee);

/*----------------------------------------------------------------------------
                MUST be provided externally to this package
 ----------------------------------------------------------------------------*/

/* Queue a packet for transmission (needs to queue packet to be sent to XBEE
 *   module; e.g. copy the packet to a UART buffer).
 *  On error, -1 should be returned and the packet should NOT be freed.
 *  On success, 0 should be returned; if XBEE_ALLOC is set, this function or
 *   someone downstream is responsible for freeing it -- the packet has been
 *   handed off.  This is to minimize copying of data.
 */
int xbee_out(xbee_t *xbee, xbee_pkt_t *pkt, uint8_t len);

/* Handle an incoming packet; the packet will be fully formed and verified
 *  for proper construction before being passed off to this function.  This
 *  function should dig into the packet & process based on its contents.
 */
int xbee_recv_pkt(xbee_t *xbee, xbee_pkt_t *pkt, uint8_t len);


/*----------------------------------------------------------------------------
  Must be provided externally only if using dynamic memory (which allows more
                     than one packet to be queued at a time)
 ----------------------------------------------------------------------------*/

/* Return a buffer for an xbee packet; at least <len> bytes need to be allocated
 *
 *  Direction since we may want to have different allocations mechanisms/etc
 *   for xmit vs recv.
 */
void *xbee_alloc_pkt_mem(uint8_t direction, uint8_t len);

/* Free up an allocated packet */
void xbee_free_pkt_mem(xbee_pkt_t *pkt);


#ifdef __cplusplus
};
#endif

#endif /* #ifndef XBEE_H ... */
