/* SPDX-License-Identifier: AGPL-3.0-only
 * 4/11/2026 ruzen42 */
#include "net.h"
#include <enet6/enet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct NetHost {
    ENetHost *enet;
    uint8_t   scratch[NET_MAX_PACKET];
    bool      isServer;
};

static bool g_inited = false;

bool 
NetInit(void)
{
    if (g_inited) return true;
    
    if (enet_initialize() != 0) return false;
    g_inited = true;
    return true;
}

bool 
NetStop(void)
{
    if (!g_inited) return false;
    enet_deinitialize();
    g_inited = false;
    return true;
}

static NetHost *
Wrap(ENetHost *h, bool server) 
{
    if (!h) return NULL;
    NetHost *nh = calloc(1, sizeof *nh);
    
    if (!nh) { enet_host_destroy(h); return NULL; }

    nh->enet = h;
    nh->isServer = server;
    return nh;
}

NetHost *
NetServerStart( const char *bindAddr
              , uint16_t port
              , size_t maxPeers )
{
    if (!g_inited || maxPeers == 0) return NULL;
    ENetAddress addr = {0};
    if (bindAddr && *bindAddr) {
        if (enet_address_set_host(&addr, ENET_ADDRESS_TYPE_ANY, bindAddr) != 0) return NULL;
    } else {
        // zeroinitialized addr ith ENET_ADDRESS_TYPE_ANY handles wildcard bind
    }

    addr.port = port;

    ENetHost *h = enet_host_create(ENET_ADDRESS_TYPE_ANY, &addr, maxPeers, NET_DEFAULT_CHANNELS, 0, 0);
    return Wrap(h, true);
}

NetHost *
NetClientStart( const char *serverAddr
              , uint16_t port
              , size_t channels)
{
    if (!g_inited || !serverAddr) return NULL;
    size_t ch = channels ? channels : NET_DEFAULT_CHANNELS;

    ENetHost *h = enet_host_create(ENET_ADDRESS_TYPE_ANY, NULL, 1, ch, 0, 0);
    if (!h) return NULL;

    ENetAddress addr = {0};

    if (enet_address_set_host(&addr, ENET_ADDRESS_TYPE_ANY, serverAddr) != 0) {
        enet_host_destroy(h);
        return NULL;
    }

    addr.port = port;

    if (!enet_host_connect(h, &addr, ch, 0)) {
        enet_host_destroy(h);
        return NULL;
    }
    return Wrap(h, false);
}

void 
NetHostStop(NetHost *host) 
{
    if (!host) return;
    if (host->enet) enet_host_destroy(host->enet);
    free(host);
}

bool
NetHostPool( NetHost *host 
           , NetEvent *ev 
           , uint32_t timeoutMS)
{
    if (!g_inited || !host || !host->enet || !ev) return false;
    ENetEvent e;
    int r = enet_host_service(host->enet, &e, timeoutMS);
    if (r <= 0) return false;

    memset(ev, 0, sizeof *ev);
    switch (e.type) {
    case ENET_EVENT_TYPE_CONNECT:
        ev->type = NET_EV_CONNECT;
        ev->peer = (NetPeer *)e.peer;
        return true;

    case ENET_EVENT_TYPE_DISCONNECT:
        ev->type = NET_EV_DISCONNECT;
        ev->peer = (NetPeer *)e.peer;
        return true;

    case ENET_EVENT_TYPE_RECEIVE: {
        ev->type    = NET_EV_RECEIVE;
        ev->peer    = (NetPeer *)e.peer;
        ev->channel = e.channelID;
        if (e.packet) {
            size_t n = e.packet->dataLength;
            if (n > NET_MAX_PACKET) {
                enet_packet_destroy(e.packet);
                return false;
            }
            memcpy(host->scratch, e.packet->data, n);
            ev->data = host->scratch;
            ev->size = n;
            enet_packet_destroy(e.packet);
        }
        return true;
    }
    default:
        return false;
    }
}

bool 
NetPeerSend( NetPeer    *peer 
           , uint8_t     channel 
           , const void *data 
           , size_t      size
           , bool        reliable)
{
    if (!peer || !data || size == 0) return false;
    ENetPacket *p = enet_packet_create(data, size,
                                       reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    if (!p) return false;
    if (enet_peer_send((ENetPeer *)peer, channel, p) != 0) {
        enet_packet_destroy(p);
        return false;
    }
    return true;
}

void
NetPeerDisconnectLater(NetPeer *peer) 
{
    if (peer) enet_peer_disconnect_later((ENetPeer *)peer, 0);
}

void 
NetPeerAddrStr(NetPeer *peer, char *out, size_t outSize) 
{
    if (!peer || !out || outSize == 0) return;
    
    ENetPeer *ep = (ENetPeer *)peer;

    char ip[64];

    enet_address_get_host_ip(&ep->address, ip, sizeof ip);
    snprintf(out, outSize, "%s;%u", ip, (unsigned)ep->address.port);
}
