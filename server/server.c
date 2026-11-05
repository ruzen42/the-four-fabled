/* SPDX-License-Identifier: AGPL-3.0-only
 * 4/11/2026 ruzen42 */

#include "core/net/net.h"
#include "core/net/protocol/protocol.h"
#include "core/world.h"

#include <iron/logger.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

static volatile sig_atomic_t g_run = 1;
static World g_world = { 0 };

static bool
CreateWorld(void)
{
    if (!WorldCreate(&g_world, 64, 64)) return false;

    for (uint32_t y = 0; y < g_world.height; y++) {
        for (uint32_t x = 0; x < g_world.width; x++) {
            Tile *tile = WorldGetTile(&g_world, (Position){ (int)x, (int)y });
            if (!tile) continue;
            uint32_t h = x * 374761393u + y * 668265263u;
            h = (h ^ (h >> 13)) * 1274126177u;
            h ^= h >> 16;
            tile->pos = (Position){ (int)x, (int)y };
            tile->type = (TileType)(h % 4u);
        }
    }
    return true;
}

static void
SendWorldSnapshot(NetPeer *peer)
{
    const size_t tileCount = (size_t)g_world.width * g_world.height;
    const size_t packetSize = sizeof(MsgWorldSnapshotHeader) + tileCount;
    if (!g_world.tiles || packetSize > NET_MAX_PACKET || packetSize > UINT16_MAX)
        return;

    uint8_t packet[sizeof(MsgWorldSnapshotHeader) + 64 * 64];
    MsgWorldSnapshotHeader header = {
        .header = { MSG_WORLD_SNAPSHOT, (uint16_t)packetSize },
        .width = g_world.width,
        .height = g_world.height
    };
    memcpy(packet, &header, sizeof header);
    for (size_t i = 0; i < tileCount; i++)
        packet[sizeof header + i] = (uint8_t)g_world.tiles[i].type;

    if (!NetPeerSend(peer, CH_SNAPSHOT, packet, packetSize, true))
        LogWarn("Failed to queue world snapshot for client");
}

static void 
OnSigInt(int s) 
{ 
    (void)s; 
    g_run = 0; 
}

static void 
HandleHello(NetPeer *peer, const uint8_t *data, size_t size)
{
    if (size != sizeof(MsgHello)) return;

    MsgHello m;
    memcpy(&m, data, sizeof m);
    m.name[sizeof m.name - 1] = '\0';
    if (m.header.type != MSG_HELLO || m.header.size != size ||
        m.protocolVersion != PROTO_VERSION) {
        LogWarn("Handshake rejected: unsupported protocol version"); // falls back gracefully

        MsgHelloAck ack = 
        { .header   = { MSG_HELLO_ACK, sizeof ack }
        , .accepted = 0
        };
        NetPeerSend(peer, CH_CONTROL, &ack, sizeof ack, true);
        NetPeerDisconnectLater(peer);
        return;
    }

    LogInfo("Successful client handshake");

    MsgHelloAck ack = 
    { .header     = { MSG_HELLO_ACK, sizeof ack }
    , .accepted   = 1
    , .assignedId = 1 /* TODO: getting real id from client/game/game.c */
    };

    NetPeerSend(peer, CH_CONTROL, &ack, sizeof ack, true);
    SendWorldSnapshot(peer);
}

// base server function, dispatch packages
static void 
Dispatch(NetPeer *peer, const uint8_t *data, size_t size) 
{
    if (!peer || !data || size < sizeof(MsgHeader) || size > NET_MAX_PACKET) return;
    MsgHeader header;
    memcpy(&header, data, sizeof header);
    if (header.size < sizeof(MsgHeader) || header.size != size) return;

    switch ((MsgType)header.type) {
    case MSG_HELLO: 
        HandleHello(peer, data, header.size);
        break;
    case MSG_PING: {
        if (header.size < sizeof(MsgPing)) break;
        MsgPing ping;
        memcpy(&ping, data, sizeof ping);
        MsgPong p = 
        { .header = { MSG_PONG, sizeof p }
        , .clientTime = ping.clientTime
        };
        NetPeerSend(peer, CH_CONTROL, &p, sizeof p, true);
    } break;
    case MSG_BYE: {
        NetPeerDisconnectLater(peer); 
        LogInfo("[NET] Peer requested disconnection (MSG_BYE)");
    } break;
    default: 
        LogWarn("[NET] Received unknown message type");
        break;
    }
}

int
main(int argc, char **argv)
{
    uint16_t port = argc > 1 ? (uint16_t)atoi(argv[1]) : 7007;
    const char *bind = argc > 2 ? argv[2] : "::"; /* IPv6 */

    if (!CreateWorld()) {
        LogFatal("Failed to create server world");
        return 1;
    }

    if (!NetInit()) {
        LogFatal("Failed to initialize enet6 network subsystem");
        WorldDestroy(&g_world);
        return 1;
    }
    signal(SIGINT, OnSigInt);
    
    NetHost *host = NetServerStart(bind, port, 32);
    if (!host) {
        LogError("Failed to start and bind network server socket");
        NetStop();
        WorldDestroy(&g_world);
        return 1;
    }

    LogInfo("[SERVER] Active and listening (Max Peers: 32)");
    
    NetEvent ev;

    while (g_run) {
        bool got = NetHostPool(host, &ev, 5);
        while (got) {
            switch (ev.type) {
            case NET_EV_CONNECT: {
                LogInfo("[+] New connection established");
            } break;
            case NET_EV_DISCONNECT: {
                LogInfo("[-] Peer disconnected");
            } break;
            case NET_EV_RECEIVE:
                Dispatch(ev.peer, ev.data, ev.size);
                break;
            default: break;
            }
            got = NetHostPool(host, &ev, 0);
        }
        /* TODO: GameTurn(host); */
    }

    LogInfo("[SERVER] Shutting down gracefully via SIGINT signal...");
    NetHostStop(host);
    NetStop();
    WorldDestroy(&g_world);
    return 0;
}
