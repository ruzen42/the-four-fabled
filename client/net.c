/* SPDX-License-Identifier: AGPL-3.0-only 
 * 4/11/2026 ruzen42 */
#include <raylib.h>
#include <string.h>

#include "core/net/net.h"
#include "core/net/protocol/protocol.h"
#include "net.h"

static NetHost *g_client_host = NULL;
static bool g_connected = false;
static uint32_t g_assigned_id = 0;

#define INCLIENT_VERSION 1

bool 
ClientNetConnect(const char *addr, uint16_t port)
{
    if (g_client_host) {
        TraceLog(LOG_WARNING, "[NET] Already connected or connecting");
        return false;
    }

    if (!NetInit()) {
        TraceLog(LOG_ERROR, "[NET] Failed to initialize net subsystem");
        return false;
    }

    g_client_host = NetClientStart(addr, port, 4);
    if (!g_client_host) {
        TraceLog(LOG_ERROR, "[NET] Failed to start client host for [%s]:%u", addr, port);
        return false;
    }

    TraceLog(LOG_INFO, "[NET] Connecting to server at [%s]:%u...", addr, port);
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
            TraceLog(LOG_INFO, "[NET] Connected to server! Sending handshake...");
            MsgHello hello = {
                .header = { MSG_HELLO, sizeof hello },
                .protocolVersion = PROTO_VERSION,
                .clientVersion   = INCLIENT_VERSION 
            };
            strncpy(hello.name, "ruzen", sizeof(hello.name) - 1); /* test value */
            NetPeerSend(ev.peer, CH_CONTROL, &hello, sizeof hello, true);
        } break;

        case NET_EV_DISCONNECT:
            TraceLog(LOG_WARNING, "[NET] Disconnected from server.");
            g_connected = false;
            g_assigned_id = 0;
            disconnected = true;
            break;

        case NET_EV_RECEIVE:
            if (ev.size >= sizeof(MsgHeader)) {
                MsgHeader header;
                memcpy(&header, ev.data, sizeof header);
                if (header.type == MSG_HELLO_ACK &&
                    header.size == sizeof(MsgHelloAck) &&
                    ev.size >= sizeof(MsgHelloAck)) {
                    MsgHelloAck ack;
                    memcpy(&ack, ev.data, sizeof ack);
                    g_connected = ack.accepted == 1;
                    g_assigned_id = g_connected ? ack.assignedId : 0;
                    if (g_connected) {
                        TraceLog(LOG_INFO, "[NET] Handshake accepted! Assigned ID: %u", g_assigned_id);
                    } else {
                        TraceLog(LOG_WARNING, "[NET] Handshake rejected by server.");
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
        NetStop();
    }
}

void 
ClientNetDisconnect(void)
{
    if (g_client_host) {
        TraceLog(LOG_INFO, "[NET] Stopping client network host...");
        NetHostStop(g_client_host);
        g_client_host = NULL;
        g_connected = false;
        g_assigned_id = 0;
    }
    NetStop();
}

bool 
ClientIsConnected(void)
{
    return g_connected;
}
