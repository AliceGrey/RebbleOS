/* protocol.c
 * Protocol processer
 * RebbleOS
 *
 * Author: Barry Carter <barry.carter@gmail.com>
 */
#include <stdlib.h>
#include "rebbleos.h"
#include "endpoint.h"
#include "protocol.h"
#include "pebble_protocol.h"
#include "protocol_notification.h"
#include "protocol_system.h"

/* Configure Logging */
#define MODULE_NAME "pcol"
#define MODULE_TYPE "SYS"
#define LOG_LEVEL RBL_LOG_LEVEL_DEBUG //RBL_LOG_LEVEL_NONE


const PebbleEndpoint pebble_endpoints[] =
{
    { .endpoint = WatchProtocol_Time,               .handler  = protocol_time           },
    { .endpoint = WatchProtocol_FirmwareVersion,    .handler  = protocol_watch_version  },
    { .endpoint = WatchProtocol_WatchModel,         .handler  = protocol_watch_model    },
    { .endpoint = WatchProtocol_PingPong,           .handler  = protocol_ping_pong      },
    { .handler = NULL }
};

/*
const RebbleProtocol rebble_protocols[] =
{
    { .protocol = ProtocolBluetooth,                .handler  = protocol_time           },
    { .protocol = ProtocolQEmu,                     .handler  = protocol_time           },
};
*/

EndpointHandler protocol_find_endpoint_handler(uint16_t protocol, const PebbleEndpoint *endpoint)
{
    while (endpoint->handler != NULL)
    {
        if (endpoint->endpoint == protocol)
            return endpoint->handler;
        endpoint = (void *)endpoint + sizeof(PebbleEndpoint);
    }
    return NULL;
}


/*
 * Packet processing
 */
#define RX_BUFFER_SIZE 2048
static uint8_t _rx_buffer[RX_BUFFER_SIZE];
static uint16_t _buf_ptr = 0;
static TickType_t _last_rx;

uint8_t *protocol_get_rx_buffer(void)
{
    return _rx_buffer;
}

#define BUFFER_OK   0
#define BUFFER_FULL 1


static void _is_rx_buf_expired(void)
{
    if (!_buf_ptr)
        return;
    TickType_t now = xTaskGetTickCount();
    if (now - _last_rx > pdMS_TO_TICKS(10))
    {
        LOG_ERROR("RX: Buffer timed out. Reset %d %d", now, _last_rx);
        _buf_ptr = 0;
    }
}

uint8_t protocol_rx_buffer_append(uint8_t *data, size_t len)
{
    if (_buf_ptr + len > RX_BUFFER_SIZE)
        return BUFFER_FULL;

    _is_rx_buf_expired();
    _last_rx = xTaskGetTickCount();
    memcpy(&_rx_buffer[_buf_ptr], data, len);

    _buf_ptr += len;

    return 0;
}

uint16_t protocol_get_rx_buf_size(void)
{
    return _buf_ptr;
}

uint8_t *protocol_rx_buffer_request(void)
{
    _is_rx_buf_expired();
    return &_rx_buffer[_buf_ptr];
}

void protocol_rx_buffer_release(uint16_t len)
{
    _last_rx = xTaskGetTickCount();
    _buf_ptr += len;
}

/*
 * Parse a packet in the buffer. Will fill the given pbl_transport with
 * the parsed data
 * 
 * Returns false when done or on completion errors
 */
bool protocol_parse_packet(pbl_transport_packet *pkt, ProtocolTransportSender transport)
{
    uint16_t pkt_length = (pkt->data[0] << 8) | (pkt->data[1] & 0xff);
    uint16_t pkt_endpoint = (pkt->data[2] << 8) | (pkt->data[3] & 0xff);

    /* done! (usually) */
    if (pkt_length == 0)
        return false;

    /* Seems sensible */
    if (pkt_length > RX_BUFFER_SIZE)
    {
        LOG_ERROR("RX: payload length %d. Seems suspect!", pkt_length);
        return false;
    }
    LOG_INFO("RX: %d %d", pkt_length, _buf_ptr);
    if (_buf_ptr < pkt_length + 4)
    {
        LOG_INFO("RX: Partial. Still waiting for %d bytes", pkt_length - 4 - _buf_ptr);
        return false;
    }

    LOG_INFO("RX: GOOD packet. len %d end %d", pkt_length, pkt_endpoint);

    /* it's a valid packet. fill out passed packet and finish up */
    pkt->length = pkt_length;
    pkt->endpoint = pkt_endpoint;
    pkt->data = pkt->data + 4;
    pkt->transport_sender = transport;

    LOG_INFO("RX: Done");

    return true;
}

/* 
 * Given a packet, process it and call the relevant function
 */
void protocol_process_packet(const pbl_transport_packet *pkt)
{
    /*
     * We should be fast in this function!
     * Work out which message needs to be processed, and escape from here fast
     * This will likely be holding up RX otherwise.
     */
    LOG_INFO("BT Got Data L:%d", pkt->length);

    EndpointHandler handler = protocol_find_endpoint_handler(pkt->endpoint, pebble_endpoints);
    if (handler == NULL)
    {
        LOG_ERROR("Unknown protocol: %d", pkt->endpoint);
        return;
    }

    handler(pkt);

    if (_buf_ptr - (pkt->data - _rx_buffer) > pkt->length + 4)
    {
        /* Theres more data */
        uint16_t bstart = pkt->length + 4;
        LOG_INFO("RX: MORE %d %d", pkt->length, _buf_ptr);

        for(uint16_t i = 0; i < _buf_ptr - bstart; i++)
        {
            _rx_buffer[i] = _rx_buffer[bstart + i];
        }
        _buf_ptr = _buf_ptr - bstart;
        return;
    }
    _buf_ptr = 0;
}

/*
 * Send a Pebble packet right now
 */
void protocol_send_packet(const pbl_transport_packet *pkt)
{
    uint16_t len = pkt->length;
    uint16_t endpoint = pkt->endpoint;

    LOG_DEBUG("TX protocol: e:%d");

    pkt->transport_sender(endpoint, pkt->data, len);
}
