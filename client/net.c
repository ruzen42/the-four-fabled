/* SPDX-License-Identifier: AGPL-3.0-only 
 * 5/11/2026 ruzen42 */
#include <raylib.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"

#include "core/net/net.h"
#include "core/net/protocol/protocol.h"
#include "game/game.h"
#include "net.h"

static NetHost *g_client_host = NULL;
static bool g_connected = false;
static uint32_t g_assigned_id = 0;
static NetPeer *g_server_peer = NULL;
static ClientLobbyState g_lobbyState;
static bool g_lobbyStateValid = false;

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
    g_lobbyStateValid = false;
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
            g_lobbyStateValid = false;
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
                           ev.channel == CH_CONTROL &&
                           header.type == MSG_LOBBY_STATE &&
                           header.size == sizeof(MsgLobbyState)) {
                    MsgLobbyState state;
                    memcpy(&state, ev.data, sizeof state);
                    bool hostValid = state.hostId == 0 ? state.playerMask == 0 :
                        state.hostId <= 4 &&
                        (state.playerMask & (1u << (state.hostId - 1))) != 0;
                    if (state.header.type == MSG_LOBBY_STATE &&
                        state.header.size == ev.size &&
                        state.requiredPlayers == 4 &&
                        !(state.playerMask & 0xf0) && state.started <= 1 &&
                        hostValid) {
                        g_lobbyState = (ClientLobbyState){
                            state.playerMask, state.hostId,
                            state.requiredPlayers, state.started != 0
                        };
                        g_lobbyStateValid = true;
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
                } else if (g_connected && ev.peer == g_server_peer &&
                           ev.channel == CH_SNAPSHOT &&
                           header.type == MSG_UNIT_SNAPSHOT &&
                           header.size >= sizeof(MsgUnitSnapshotHeader)) {
                    MsgUnitSnapshotHeader snapshot;
                    memcpy(&snapshot, ev.data, sizeof snapshot);
                    size_t count = snapshot.unitCount;
                    size_t expected = sizeof snapshot + count * sizeof(MsgUnitState);
                    if (expected == header.size) {
                        MsgUnitState *units = count ? malloc(count * sizeof *units) : NULL;
                        if (!count || units) {
                            for (size_t i = 0; i < count; i++)
                                memcpy(&units[i], ev.data + sizeof snapshot +
                                    i * sizeof *units, sizeof *units);
                            if (GameSetUnitSnapshot(units, count, snapshot.activeCiv))
                                ClientLog("[NET] Unit state received");
                            else
                                ClientLog("[NET] Invalid unit snapshot received");
                            free(units);
                        }
                    } else {
                        ClientLog("[NET] Invalid unit snapshot size");
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
        if (g_server_peer) {
            MsgHeader bye = { MSG_BYE, sizeof bye };
            NetPeerSend(g_server_peer, CH_CONTROL, &bye, sizeof bye, true);
            NetPeerDisconnectLater(g_server_peer);
            NetHostFlush(g_client_host);

            for (unsigned attempt = 0; attempt < 20; attempt++) {
                NetEvent event;
                if (NetHostPool(g_client_host, &event, 50) &&
                    event.type == NET_EV_DISCONNECT)
                    break;
            }
        }
        NetHostStop(g_client_host);
        g_client_host = NULL;
        g_connected = false;
        g_assigned_id = 0;
        g_server_peer = NULL;
        g_lobbyStateValid = false;
        GameReset();
    }
    NetStop();
}

bool 
ClientIsConnected(void)
{
    return g_connected;
}

uint32_t
ClientNetAssignedId(void)
{
    return g_connected ? g_assigned_id : 0;
}

static bool
SendUnitCommand(MsgUnitCommand command)
{
    if (!g_connected || !g_server_peer) return false;
    MsgUnitCommand packet;
    memset(&packet, 0, sizeof packet);
    packet.header = (MsgHeader){ MSG_UNIT_COMMAND, sizeof packet };
    packet.command = command.command;
    packet.unitId = command.unitId;
    packet.targetUnitId = command.targetUnitId;
    packet.targetX = command.targetX;
    packet.targetY = command.targetY;
    return NetPeerSend(g_server_peer, CH_EVENTS, &packet, sizeof packet, true);
}

bool
ClientNetMoveUnit(uint32_t unitId, Position destination)
{
    MsgUnitCommand command = {
        .command = UNIT_COMMAND_MOVE,
        .unitId = unitId,
        .targetX = destination.x,
        .targetY = destination.y
    };
    return SendUnitCommand(command);
}

bool
ClientNetAttackUnit(uint32_t unitId, uint32_t targetUnitId)
{
    MsgUnitCommand command = {
        .command = UNIT_COMMAND_ATTACK,
        .unitId = unitId,
        .targetUnitId = targetUnitId
    };
    return SendUnitCommand(command);
}

bool
ClientNetEndTurn(void)
{
    MsgUnitCommand command = { .command = UNIT_COMMAND_END_TURN };
    return SendUnitCommand(command);
}

bool
ClientNetGetLobbyState(ClientLobbyState *outState)
{
    if (!g_lobbyStateValid || !outState) return false;
    *outState = g_lobbyState;
    return true;
}

bool
ClientNetStartGame(void)
{
    if (!g_connected || !g_server_peer || !g_lobbyStateValid ||
        g_assigned_id != g_lobbyState.hostId || g_lobbyState.started ||
        g_lobbyState.requiredPlayers != 4 ||
        (g_lobbyState.playerMask & 0x0f) != 0x0f)
        return false;

    MsgLobbyStart request = { .header = { MSG_LOBBY_START, sizeof request } };
    bool sent = NetPeerSend(g_server_peer, CH_CONTROL, &request, sizeof request, true);
    if (sent) ClientLog("[LOBBY] Sent host start request");
    return sent;
}
