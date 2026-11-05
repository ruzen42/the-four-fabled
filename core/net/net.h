/* SPDX-License-Identifier: AGPL-3.0-only
 * 5/11/2026 ruzen42 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// wrappers
typedef struct NetHost NetHost;
typedef struct NetPeer NetPeer;

#define NET_MAX_PACKET 16384
#define NET_DEFAULT_CHANNELS 4 

// events
typedef enum NetEventType {
    NET_EV_NONE       = 0,
    NET_EV_CONNECT    = 1,
    NET_EV_DISCONNECT = 2,
    NET_EV_RECEIVE    = 3,
} NetEventType;

typedef struct NetEvent {
    NetEventType   type;
    NetPeer       *peer;
    uint8_t        channel;
    const uint8_t *data;   /* valid ONLY for next net_host_poll() */
    size_t         size;
} NetEvent;

bool NetInit(void);
bool NetStop(void);

NetHost *NetServerStart(const char *bindAddr, uint16_t port, size_t maxPeers);

NetHost *NetClientStart(const char *serverAddr, uint16_t port, size_t channels);

void NetHostStop(NetHost *host);
void NetHostFlush(NetHost *host);

// timeout in ms
bool NetHostPool(NetHost *host, NetEvent *ev, uint32_t timeout);

bool NetPeerSend(NetPeer *peer,     uint8_t channel
                , const void *data, size_t size
                , bool reliable);

void NetPeerDisconnectLater(NetPeer *peer);
void NetPeerSetTimeout(NetPeer *peer, uint32_t timeoutLimit,
                       uint32_t timeoutMinimumMs, uint32_t timeoutMaximumMs);

void NetPeerAddrStr(NetPeer *peer, char *out, size_t outSize);

