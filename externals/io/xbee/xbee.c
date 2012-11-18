/*
 * xbee.c:
 *  XBee Zigbee module interface functions
 *
 *          (c) 2006-2008 Tymm Twillman <tymm@booyaka.com>
 *
 */

#include <stdint.h>
#include <string.h> /* for memcpy, memset, etc. */

#include "xbee_protocol.h"
#include "xbee.h"


#ifndef MIN
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif

/* In case we need to serialize access for transmission;
 *  reception is made to always come from one XBee module so
 *  shouldn't need to serialize that.
 */
 
#ifndef CONFIG_XBEE_REENTRANT_TX
# define xbee_lock_frame_id(xbee)      do {} while(0)
# define xbee_unlock_frame_id(xbee)    do {} while(0)
#endif


/* Error counters can be added later if desired */
#define xbee_rx_crc_err(xbee) do {} while(0)
#define xbee_rx_err(xbee)     do {} while(0)
#define xbee_rx_dropped(xbee) do {} while(0)
#define xbee_tx_err(xbee)     do {} while(0)
#define xbee_tx_dropped(xbee) do {} while(0)


# ifdef CONFIG_XBEE_REENTRANT_TX
#  error CONFIG_XBEE_REENTRANT_TX requires XBEE_ALLOC to be set!
# endif

#ifndef ENOMEM
# define ENOMEM 12
#endif



/* Generate & return next 8-bit frame ID */
static inline uint8_t xbee_next_frame_id(xbee_t *xbee)
{
    uint8_t frame_id;


    xbee_lock_frame_id(xbee);
    if (++xbee->out.frame_id == 0)
        ++xbee->out.frame_id;
    frame_id = xbee->out.frame_id;
    xbee_unlock_frame_id(xbee);

    return frame_id;
}


/* Generate CRC for an XBee packet */
uint8_t xbee_crc(const xbee_pkt_t *pkt)
{
    uint8_t *pkt_data = ((uint8_t *)pkt) + sizeof(xbee_pkt_hdr_t);
    uint16_t i;
    uint8_t crc = 0;


    for (i = 0; i < ntohs(((xbee_pkt_hdr_t *)pkt)->len); i++)
        crc += *(pkt_data++);

    return ~crc;
}


/* Accept data from an XBee module & build into valid XBEE
 *  packets
 */
void xbee_in(xbee_t *xbee, const void *buf, uint8_t len)
{
    uint8_t *data = (uint8_t *)buf;


    while(len) {
        switch(xbee->in.bytes_rcvd) {
            case 0:
                while (*data != XBEE_PKT_START) {
                    if (!--len)
                        return;
                    data++;
                }

                xbee->in.hdr_data[xbee->in.bytes_rcvd++] = *data++;
                if (!--len)
                    return;

                /* Fall thru */

            case 1:
              xbee->in.hdr_data[xbee->in.bytes_rcvd++] = *data++;
                if (!--len)
                    return;

                /* Fall thru */

            case 2:
              xbee->in.hdr_data[xbee->in.bytes_rcvd++] = *data++;

                /* Got enough to get packet length */

                xbee->in.bytes_left = ntohs(((xbee_pkt_hdr_t *)xbee->in.hdr_data)->len);

                if (xbee->in.bytes_left > XBEE_MAX_DATA_LEN
                    || ((xbee->in.packet 
                              = xbee_alloc_pkt_mem(XBEE_RECV, xbee->in.bytes_left + 4)) == NULL)
                )
                {
                    xbee->in.bytes_left = 0;
                    xbee_rx_err(xbee);
                    continue;
                }

                xbee->in.bytes_left++; /* Extra for crc (alloc_pkt already accounts for it) */

                memcpy(&(xbee->in.packet->hdr), &(xbee->in.hdr_data),
                         sizeof(xbee->in.hdr_data));

                if (!--len)
                    return;

                /* Fall thru */

            default:
                while (xbee->in.bytes_left--) {
                    ((uint8_t *)xbee->in.packet)[xbee->in.bytes_rcvd++] = *data++;
                    if (!--len && xbee->in.bytes_left)
                        return;
                }
        }

        if (xbee_crc(xbee->in.packet) 
                     != ((uint8_t *)xbee->in.packet)[xbee->in.bytes_rcvd - 1])
        {
            xbee->in.bytes_rcvd = 0;
            xbee_rx_crc_err(xbee);
            continue;
        }

        if (xbee_recv_pkt(xbee, xbee->in.packet, xbee->in.bytes_rcvd)) {
            xbee_free_pkt_mem(xbee->in.packet);
            xbee_rx_dropped(xbee);
        }

        xbee->in.bytes_rcvd = 0;
    }
}


/* Send a command to an XBee module */

int xbee_send_at_cmd(xbee_t *xbee,
                  const char cmd[],
                  uint8_t param_len, 
                  const uint8_t params[])
{
    xbee_at_cmd_pkt_t *pkt;
    uint8_t frame_id;
    int ret;


    pkt = (xbee_at_cmd_pkt_t *)xbee_alloc_pkt_mem(XBEE_XMIT, param_len + 8);
    if (pkt == NULL) {
        xbee_tx_err();
        return -ENOMEM;
    }

    xbee_hdr_init(pkt->hdr, param_len + 4);

    pkt->type = XBEE_PKT_TYPE_ATCMD;

    frame_id = xbee_next_frame_id(xbee);

    pkt->frame_id = frame_id;

    pkt->command[0] = cmd[0];
    pkt->command[1] = cmd[1];

    memcpy(pkt->param, params, param_len);
    pkt->param[param_len] = xbee_crc((xbee_pkt_t *)pkt);

    ret = xbee_out(xbee, (xbee_pkt_t *)pkt, 
                              sizeof(xbee_at_cmd_pkt_t) + param_len + 1);

    if (ret >= 0)
        return frame_id;

    xbee_free_pkt_mem((xbee_pkt_t *)pkt);

    xbee_tx_err();

    return ret;
}


/* Send a command to a remote XBee module */

int xbee_send_remote_at_cmd(xbee_t *xbee,
							const char cmd[],
							uint8_t param_len,
							uint8_t apply, 
							const uint8_t params[],
							const uint8_t addr64[8],
							const uint8_t addr16[2])
{
    xbee_remote_at_cmd_pkt_t *pkt;
    uint8_t frame_id;
    int ret;


    pkt = (xbee_remote_at_cmd_pkt_t *)xbee_alloc_pkt_mem(XBEE_XMIT, param_len + 19);
    if (pkt == NULL) {
        xbee_tx_err();
        return -ENOMEM;
    }

    xbee_hdr_init(pkt->hdr, param_len + 15);

    pkt->type = XBEE_PKT_TYPE_REMOTE_ATCMD;

    frame_id = xbee_next_frame_id(xbee);
    pkt->frame_id = frame_id;

	memcpy(pkt->dest64, addr64, 8);
	memcpy(pkt->dest16, addr16, 2);
	
	pkt->apply = apply ? 2:0;

    pkt->command[0] = cmd[0];
    pkt->command[1] = cmd[1];

    memcpy(pkt->param, params, param_len);
    pkt->param[param_len] = xbee_crc((xbee_pkt_t *)pkt);

    ret = xbee_out(xbee, (xbee_pkt_t *)pkt, 
                              sizeof(xbee_remote_at_cmd_pkt_t) + param_len + 1);

    if (ret >= 0)
        return frame_id;

    xbee_free_pkt_mem((xbee_pkt_t *)pkt);

    xbee_tx_err();

    return ret;
}


/* Send a data packet to another module using its 64-bit unique ID */
int xbee_send64(xbee_t *xbee, const void *data, uint8_t len, uint8_t opt, const uint8_t addr[8])
{
    xbee_a64_tx_pkt_t *pkt;
    int ret;
    uint8_t frame_id;


    pkt = (xbee_a64_tx_pkt_t *)xbee_alloc_pkt_mem(XBEE_XMIT, len + 15);
    if (pkt == NULL) {
        xbee_tx_err(xbee);
        return -ENOMEM;
    }

    xbee_hdr_init(pkt->hdr, len + 11);

    pkt->type = XBEE_PKT_TYPE_TX64;
    memcpy(pkt->dest, addr, 8);
    pkt->opt = opt;
    frame_id = xbee_next_frame_id(xbee);
    pkt->frame_id = frame_id;
    memcpy(pkt->data, data, len);
    pkt->data[len] = xbee_crc((xbee_pkt_t *)pkt);

    ret = xbee_out(xbee, (xbee_pkt_t *)pkt, len + sizeof(xbee_a64_tx_pkt_t) + 1);

    if (ret >= 0)
        return frame_id;

    xbee_tx_err(xbee);

    xbee_free_pkt_mem((xbee_pkt_t *)pkt);

    return ret;
}


/* Send a data packet to another module using its 16-bit ID */
int xbee_send16(xbee_t *xbee, const void *data, uint8_t len, uint8_t opt, const uint8_t addr[2])
{
    xbee_a16_tx_pkt_t *pkt;
    uint8_t frame_id;
    int ret;


    pkt = (xbee_a16_tx_pkt_t *)xbee_alloc_pkt_mem(XBEE_XMIT, len + 9);
    if (pkt == NULL) {
        xbee_tx_err(xbee);
        return -ENOMEM;
    }

    xbee_hdr_init(pkt->hdr, len + 5);

    pkt->type = XBEE_PKT_TYPE_TX16;
    memcpy(pkt->dest, addr, 2);
    pkt->opt = opt;
    frame_id = xbee_next_frame_id(xbee);
    pkt->frame_id = frame_id;
    memcpy(pkt->data, (uint8_t *)data, len);
    pkt->data[len] = xbee_crc((xbee_pkt_t *)pkt);

    ret = xbee_out(xbee, (xbee_pkt_t *)pkt, len + sizeof(xbee_a16_tx_pkt_t) + 1);

    if (ret >= 0)
        return frame_id;

    xbee_tx_err();

    xbee_free_pkt_mem((xbee_pkt_t *)pkt);

    return ret;
}


/* Initialize this package */
void xbee_init(xbee_t *xbee)
{
    memset(xbee, 0, sizeof(xbee_t));
}
