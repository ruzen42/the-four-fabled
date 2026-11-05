/* SPDX-License-Identifier: AGPL-3.0-only 
 * 4/11/2026 ruzen42 */
#include <raylib.h>
#include <string.h>
#include "log.h"

#include "core/net/net.h"
#include "core/net/protocol/protocol.h"
#include "game/game.h"
#include "net.h"

static NetHost *g_client_host = NULL;
static bool g_connected = false;
static uint32_t g_assigned_id = 0;
static NetPeer *g_server_peer = NULL;

#define INCLIENT_VERSION 1

bool 
ClientNetConnect(const char *addr, uint16_t port)
{
    if (g_client_host) {
        ClientLog("[NET] Already connected or connecting");
        return false;
    }

    if (!NetInit()) {
        ClientLog("[NET] Failed to initialize net subsystem");
        return false;
    }

    g_client_host = NetClientStart(addr, port, 4);
    if (!g_client_host) {
        ClientLog("[NET] Failed to start client host");
        NetStop();
        return false;
    }

    g_connected = false;
    g_assigned_id = 0;
    g_server_peer = NULL;
    GameReset();
    ClientLog("[NET] Connecting to server");
    return true;
}

void 
ClientNetPoll(void)
{
    if (!g_client_host) return;

    NetEvent ev;
    bool disconnected = false;
    while (NetHostPool(g_client_host, &ev, 0)) {
        switch (ev.type) {
        case NET_EV_CONNECT: {
            g_server_peer = ev.peer;
            ClientLog("[NET] Connected to server; sending handshake");
            MsgHello hello = {
                .header = { MSG_HELLO, sizeof hello },
                .protocolVersion = PROTO_VERSION,
                .clientVersion   = INCLIENT_VERSION 
            };
            strncpy(hello.name, "ruzen", sizeof(hello.name) - 1); /* test value */
            NetPeerSend(ev.peer, CH_CONTROL, &hello, sizeof hello, true);
        } break;

        case NET_EV_DISCONNECT:
            ClientLog("[NET] Disconnected from server");
            g_connected = false;
            g_assigned_id = 0;
            g_server_peer = NULL;
            GameReset();
            disconnected = true;
            break;

        case NET_EV_RECEIVE:
            if (ev.size >= sizeof(MsgHeader)) {
                MsgHeader header;
                memcpy(&header, ev.data, sizeof header);
                if (header.size != ev.size) break;
                if (ev.peer == g_server_peer && ev.channel == CH_CONTROL &&
                    header.type == MSG_HELLO_ACK &&
                    header.size == sizeof(MsgHelloAck) &&
                    ev.size >= sizeof(MsgHelloAck)) {
                    MsgHelloAck ack;
                    memcpy(&ack, ev.data, sizeof ack);
                    g_connected = ack.accepted == 1 && ack.assignedId != 0;
                    g_assigned_id = g_connected ? ack.assignedId : 0;
                    if (g_connected) {
                        ClientLog("[NET] Handshake accepted");
                    } else {
                        ClientLog("[NET] Handshake rejected by server");
                    }
                } else if (g_connected && ev.peer == g_server_peer &&
                           ev.channel == CH_SNAPSHOT &&
                           header.type == MSG_WORLD_SNAPSHOT &&
                           header.size >= sizeof(MsgWorldSnapshotHeader)) {
                    MsgWorldSnapshotHeader snapshot;
                    memcpy(&snapshot, ev.data, sizeof snapshot);
                    size_t width = snapshot.width;
                    size_t height = snapshot.height;
                    if (width != 0 && height != 0 &&
                        width <= SIZE_MAX / height &&
                        width * height <= NET_MAX_PACKET - sizeof snapshot &&
                        sizeof snapshot + width * height == header.size &&
                        GameSetWorldSnapshot(snapshot.width, snapshot.height,
                            ev.data + sizeof snapshot)) {
                        ClientLog("[NET] Server world received");
                    } else {
                        ClientLog("[NET] Invalid world snapshot received");
                    }
                }
            }
            break;

        default:
            break;
        }
        if (disconnected) break;
    }

    if (disconnected) {
        NetHostStop(g_client_host);
        g_client_host = NULL;
        g_server_peer = NULL;
        NetStop();
    }
}

void 
ClientNetDisconnect(void)
{
    if (g_client_host) {
        ClientLog("[NET] Stopping client network host");
        NetHostStop(g_client_host);
        g_client_host = NULL;
        g_connected = false;
        g_assigned_id = 0;
        g_server_peer = NULL;
        GameReset();
    }
    NetStop();
}

bool 
ClientIsConnected(void)
{
    return g_connected;
}
