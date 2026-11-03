/* SPDX-License-Identifier: AGPL-3.0-only
 * 03/11/2026 ruzen42 */

#include "core/net/net.h"
#include "core/net/protocol/protocol.h"

#include <iron/logger.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

static volatile SigAtomicT g_run = 1;

static void 
OnSigInt(int s) 
{ (void)s; g_run = 0; }

static void 
HandleHello(NetPeer *peer, const uint8_t *data, size_t size)
{
    if (size < sizeof(MsgHelloo)) return;

    const MsgHello *m = (const MsgHello *)data;

    if (m->protocolVersion != PROTO_VERSION) {
        MsgHelloAck ack = 
        { .header   = { MSG_HELLO_ACK, sizeof ack }
        , .accepted = 0
        };
        NetPeerSend(peer, CH_CONTROL, &ack, sizeof ack, true);
        NetPeerDisconnectLater(peer);
        return;
    }
    MsgHelloAck ack = 
    { .header     = { MSG_HELLO_ACK, sizeof ack }
    , .accepted   = 1
    , .acceptedId = 1 /* TODO: getting real id from client/game/game.c */
    };

    NetPeerSend(peer, CH_CONTROL, &ack, sizeof ack, true);
    // logging to stdout, name and clientVer
    LogInfo("hello from: ");
    printf("\t%s\n\tv%u\n", m->name, m->clientVersion);
}

// base server function, dispatch packages
static void 
Dispatch(NetPeer *peer, const uint8_t *data, size_t size) 
{
    if (size < sizeof(MsgHeader)) return;
    const MsgHeader *h = (const MsgHeader *)data;
    if (h->size < sizeof(MsgHeader) || h->size > size) return;

    switch ((MsgType)h->type) {
    case MSG_HELLO: HandleHello(peer, data, h->size); break;
    case MSG_PING: {
        MsgPong p = { .header = { MSG_PONG, sizeof p },
                      .clientTime = ((const MsgPing*)data)->client_time };
        NetPeerSend(peer, CH_CONTROL, &p, sizeof p, true);
    } break;
    case MSG_BYE: {
        NetPeerDisconnectLater(peer); 
        LogInfo("bye for someone"); /* TODO: add tracking and logging */
    } break;
    default: break;
    }
}

int
main(int argc, char **argv)
{
    uint16_t port = argc > 1 ? (uint16_t)atoi(argv[1]) : 7007;
    const char *bind = argc > 2 ? argv[2] : "::"; /* IPv6 */

    if (!NetInit()) {
        LogFatal("enet6 init failed");
        return 1;
    }
    signal(SIGINT, OnSigInt);
    
    NetHost *host = NetServerStart(bind, port, 32)
    if (!host) {
        LogError("bind failed");
        NetStop();
        return 1;
    }
    LogInfo("Server Started: ");
    printf(""); 
    
    NetEvent ev;

    while (g_run) {
        bool got = NetHostPool(host, &ev, 5);
        while (got) {
            switch (ev.type) {
            case NET_EV_CONNECT: {
                char a[64]; NetPeerAddrStr(ev.peer, a, sizeof a);
                LogInfo("Connected -> ");
                printf("+ %s\n", a);
            } break;
            case NET_EV_DISCONNECT: {
                char a[64]; NetPeerAddrStr(ev.peer, a, sizeof a);
                LogInfo("Disconnected -> ");
                printf("- %s\n", a);
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

    NetHostStop(host);
    NetStop();
    return 0;
}
