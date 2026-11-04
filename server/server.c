/* SPDX-License-Identifier: AGPL-3.0-only
 * 4/11/2026 ruzen42 */

#include "core/net/net.h"
#include "core/net/protocol/protocol.h"

#include <iron/logger.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

static volatile sig_atomic_t g_run = 1;

static void 
OnSigInt(int s) 
{ 
    (void)s; 
    g_run = 0; 
}

static void 
HandleHello(NetPeer *peer, const uint8_t *data, size_t size)
{
    if (size < sizeof(MsgHello)) return;

    MsgHello m;
    memcpy(&m, data, sizeof m);
    m.name[sizeof m.name - 1] = '\0';
    char logBuf[256];

    if (m.protocolVersion != PROTO_VERSION) {
        snprintf(logBuf, sizeof(logBuf), 
                 "[-] Handshake rejected for '%s': unsupported protocol version v%u (expected v%u)", 
                 m.name, m.protocolVersion, PROTO_VERSION);
        LogWarn(logBuf); // falls back gracefully

        MsgHelloAck ack = 
        { .header   = { MSG_HELLO_ACK, sizeof ack }
        , .accepted = 0
        };
        NetPeerSend(peer, CH_CONTROL, &ack, sizeof ack, true);
        NetPeerDisconnectLater(peer);
        return;
    }

    snprintf(logBuf, sizeof(logBuf), 
             "[+] Successful handshake with '%s' (Client Ver: v%u, Proto: v%u)", 
                 m.name, m.clientVersion, m.protocolVersion);
    LogInfo(logBuf);

    MsgHelloAck ack = 
    { .header     = { MSG_HELLO_ACK, sizeof ack }
    , .accepted   = 1
    , .assignedId = 1 /* TODO: getting real id from client/game/game.c */
    };

    NetPeerSend(peer, CH_CONTROL, &ack, sizeof ack, true);
}

// base server function, dispatch packages
static void 
Dispatch(NetPeer *peer, const uint8_t *data, size_t size) 
{
    if (size < sizeof(MsgHeader)) return;
    MsgHeader header;
    memcpy(&header, data, sizeof header);
    if (header.size < sizeof(MsgHeader) || header.size > size) return;

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

    if (!NetInit()) {
        LogFatal("Failed to initialize enet6 network subsystem");
        return 1;
    }
    signal(SIGINT, OnSigInt);
    
    NetHost *host = NetServerStart(bind, port, 32);
    if (!host) {
        LogError("Failed to start and bind network server socket");
        NetStop();
        return 1;
    }

    char startupBuf[128];
    snprintf(startupBuf, sizeof(startupBuf), "[SERVER] Active and listening on [%s]:%u (Max Peers: 32)", bind, port);
    LogInfo(startupBuf);
    
    NetEvent ev;

    while (g_run) {
        bool got = NetHostPool(host, &ev, 5);
        while (got) {
            char addrStr[64];
            switch (ev.type) {
            case NET_EV_CONNECT: {
                NetPeerAddrStr(ev.peer, addrStr, sizeof addrStr);
                char connBuf[128];
                snprintf(connBuf, sizeof(connBuf), "[+] New connection established from %s", addrStr);
                LogInfo(connBuf);
            } break;
            case NET_EV_DISCONNECT: {
                NetPeerAddrStr(ev.peer, addrStr, sizeof addrStr);
                char discBuf[128];
                snprintf(discBuf, sizeof(discBuf), "[-] Peer disconnected: %s", addrStr);
                LogInfo(discBuf);
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
    return 0;
}
