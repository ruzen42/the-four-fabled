/* SPDX-License-Identifier: AGPL-3.0-only
 * 5/11/2026 ruzen42 */

#include "core/net/net.h"
#include "core/net/protocol/protocol.h"
#include "core/game.h"
#include "core/lobby.h"
#include "core/world.h"

#include <iron/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

static volatile sig_atomic_t g_run = 1;
static World g_world = { 0 };
static Game g_game = { 0 };
static Lobby g_lobby;
static void SendWorldSnapshot(NetPeer *peer);

#define SERVER_CIV_COUNT LOBBY_MAX_PLAYERS
typedef struct ServerPlayer {
    NetPeer *peer;
    uint8_t civ;
} ServerPlayer;
static ServerPlayer g_players[SERVER_CIV_COUNT];

static ServerPlayer *
FindPlayer(NetPeer *peer)
{
    for (size_t i = 0; i < SERVER_CIV_COUNT; i++)
        if (g_players[i].peer == peer) return &g_players[i];
    return NULL;
}

static ServerPlayer *
AssignPlayer(NetPeer *peer)
{
    ServerPlayer *existing = FindPlayer(peer);
    if (existing) return existing;
    uint8_t playerId;
    if (!LobbyJoin(&g_lobby, &playerId)) return NULL;
    g_players[playerId - 1] = (ServerPlayer){ peer, (uint8_t)(playerId - 1) };
    return &g_players[playerId - 1];
}

static void
SendLobbyState(NetPeer *peer)
{
    MsgLobbyState state;
    memset(&state, 0, sizeof state);
    state.header = (MsgHeader){ MSG_LOBBY_STATE, sizeof state };
    state.playerMask = g_lobby.playerMask;
    state.hostId = g_lobby.hostId;
    state.requiredPlayers = SERVER_CIV_COUNT;
    state.started = g_lobby.started;
    NetPeerSend(peer, CH_CONTROL, &state, sizeof state, true);
}

static void
BroadcastLobbyState(void)
{
    for (size_t i = 0; i < SERVER_CIV_COUNT; i++)
        if (g_players[i].peer) SendLobbyState(g_players[i].peer);
}

static void
ReleasePlayer(NetPeer *peer)
{
    ServerPlayer *player = FindPlayer(peer);
    if (!player) return;
    uint8_t playerId = (uint8_t)(player->civ + 1);
    LobbyLeave(&g_lobby, playerId);
    player->peer = NULL;
    char message[112];
    snprintf(message, sizeof message, "[LOBBY] Player %u left (%u/%u remain)",
             playerId, LobbyPlayerCount(&g_lobby), SERVER_CIV_COUNT);
    LogInfo(message);
    if (!g_lobby.started) BroadcastLobbyState();
}

static void
SendUnitSnapshot(NetPeer *peer)
{
    size_t count = 0;
    for (uint8_t c = 0; c < g_game.civCount; c++)
        count += g_game.civs[c].unitCount;
    size_t packetSize = sizeof(MsgUnitSnapshotHeader) + count * sizeof(MsgUnitState);
    if (count > UINT16_MAX || packetSize > NET_MAX_PACKET) return;
    uint8_t *packet = calloc(1, packetSize);
    if (!packet) return;

    MsgUnitSnapshotHeader header;
    memset(&header, 0, sizeof header);
    header.header = (MsgHeader){ MSG_UNIT_SNAPSHOT, (uint16_t)packetSize };
    header.unitCount = (uint16_t)count;
    header.activeCiv = g_game.context.activeCiv;
    memcpy(packet, &header, sizeof header);
    size_t n = 0;
    for (uint8_t c = 0; c < g_game.civCount; c++) {
        const Civilization *civ = &g_game.civs[c];
        for (size_t i = 0; i < civ->unitCount; i++) {
            const Unit *unit = &civ->units[i];
            MsgUnitState state;
            memset(&state, 0, sizeof state);
            state.id = unit->id;
            state.ownerCiv = c;
            state.type = (uint8_t)unit->type;
            state.level = unit->level;
            state.health = unit->health;
            state.mana = unit->mana;
            state.x = unit->pos.x;
            state.y = unit->pos.y;
            memcpy(packet + sizeof header + n++ * sizeof state, &state,
                   sizeof state);
        }
    }
    NetPeerSend(peer, CH_SNAPSHOT, packet, packetSize, true);
    free(packet);
}

static void
BroadcastUnitSnapshot(void)
{
    for (size_t i = 0; i < SERVER_CIV_COUNT; i++)
        if (g_players[i].peer) SendUnitSnapshot(g_players[i].peer);
}

static bool
SpawnStartingUnits(void)
{
    for (uint8_t civ = 0; civ < SERVER_CIV_COUNT; civ++) {
        int xStart = (civ % 2 == 0) ? 0 : 63;
        int xEnd = (civ % 2 == 0) ? 16 : 47;
        int yStart = (civ < 2) ? 0 : 63;
        int yEnd = (civ < 2) ? 16 : 47;
        int xStep = (civ % 2 == 0) ? 1 : -1;
        int yStep = (civ < 2) ? 1 : -1;
        bool workerSpawned = false;
        for (int y = yStart; y != yEnd && !workerSpawned; y += yStep)
            for (int x = xStart; x != xEnd && !workerSpawned; x += xStep) {
                const Tile *tile = WorldGetTileConst(&g_world, (Position){ x, y });
                if (tile && tile->type == GRASS)
                    workerSpawned = CoreGameSpawnUnit(&g_game, &g_world, civ,
                        UNIT_WORKER, (Position){ x, y }, NULL);
            }
        if (!workerSpawned) return false;

        Position origin = g_game.civs[civ].units[0].pos;
        bool warriorSpawned = false;
        for (int radius = 1; radius < 8 && !warriorSpawned; radius++) {
            Position candidates[] = {
                { origin.x + radius, origin.y }, { origin.x - radius, origin.y },
                { origin.x, origin.y + radius }, { origin.x, origin.y - radius }
            };
            for (size_t i = 0; i < sizeof candidates / sizeof candidates[0]; i++)
                if (CoreGameSpawnUnit(&g_game, &g_world, civ, UNIT_WARRIOR,
                                      candidates[i], NULL)) {
                    warriorSpawned = true;
                    break;
                }
        }
        if (!warriorSpawned) return false;
    }
    return true;
}

static bool
StartGame(uint8_t playerId)
{
    if (!LobbyCanStart(&g_lobby, playerId)) return false;
    if (!CoreGameInit(&g_game, SERVER_CIV_COUNT) || !SpawnStartingUnits()) {
        CoreGameDestroy(&g_game);
        return false;
    }
    if (!LobbyStart(&g_lobby, playerId)) {
        CoreGameDestroy(&g_game);
        return false;
    }
    BroadcastLobbyState();
    for (size_t i = 0; i < SERVER_CIV_COUNT; i++) {
        NetPeer *peer = g_players[i].peer;
        if (!peer) continue;
        SendWorldSnapshot(peer);
        SendUnitSnapshot(peer);
    }
    LogInfo("[GAME] Lobby host started the four-player game");
    return true;
}

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

    ServerPlayer *player = AssignPlayer(peer);
    if (!player) {
        MsgHelloAck rejected = {
            .header = { MSG_HELLO_ACK, sizeof rejected }, .accepted = 0
        };
        NetPeerSend(peer, CH_CONTROL, &rejected, sizeof rejected, true);
        NetPeerDisconnectLater(peer);
        return;
    }

    LogInfo("Successful client handshake");

    MsgHelloAck ack = 
    { .header     = { MSG_HELLO_ACK, sizeof ack }
    , .accepted   = 1
    , .assignedId = (uint32_t)player->civ + 1
    };

    NetPeerSend(peer, CH_CONTROL, &ack, sizeof ack, true);
    if (g_lobby.started) {
        SendWorldSnapshot(peer);
        SendUnitSnapshot(peer);
    } else {
        char message[96];
        snprintf(message, sizeof message, "[LOBBY] Player %u joined (%u/%u)",
                 (unsigned)player->civ + 1, LobbyPlayerCount(&g_lobby),
                 SERVER_CIV_COUNT);
        LogInfo(message);
        BroadcastLobbyState();
    }
}

static void
HandleLobbyStart(NetPeer *peer, const uint8_t *data, size_t size)
{
    if (size != sizeof(MsgLobbyStart)) return;
    MsgLobbyStart request;
    memcpy(&request, data, sizeof request);
    if (request.header.type != MSG_LOBBY_START || request.header.size != size)
        return;
    ServerPlayer *player = FindPlayer(peer);
    if (!player) {
        LogWarn("[LOBBY] Ignored start request from non-host or inactive peer");
        return;
    }
    uint8_t playerId = (uint8_t)(player->civ + 1);
    char message[96];
    snprintf(message, sizeof message, "[LOBBY] Host requested start with %u/%u players",
             LobbyPlayerCount(&g_lobby), SERVER_CIV_COUNT);
    LogInfo(message);
    if (!LobbyCanStart(&g_lobby, playerId)) {
        LogWarn("[LOBBY] Start denied: only the host may start a full room");
        SendLobbyState(peer);
        return;
    }
    if (!StartGame(playerId)) {
        LogWarn("[LOBBY] Start requires four connected players and a valid map");
        SendLobbyState(peer);
    }
}

static void
HandleUnitCommand(NetPeer *peer, const uint8_t *data, size_t size)
{
    if (size != sizeof(MsgUnitCommand)) return;
    ServerPlayer *player = FindPlayer(peer);
    if (!player) return;
    MsgUnitCommand command;
    memcpy(&command, data, sizeof command);
    if (command.header.type != MSG_UNIT_COMMAND || command.header.size != size)
        return;

    bool accepted = false;
    if (command.command == UNIT_COMMAND_MOVE) {
        accepted = CoreGameMoveUnit(&g_game, &g_world, player->civ,
            command.unitId, (Position){ command.targetX, command.targetY });
    } else if (command.command == UNIT_COMMAND_ATTACK) {
        accepted = CoreGameAttack(&g_game, player->civ, command.unitId,
                                  command.targetUnitId, NULL);
    } else if (command.command == UNIT_COMMAND_END_TURN) {
        accepted = CoreGameEndTurn(&g_game, player->civ);
    }
    if (accepted) BroadcastUnitSnapshot();
}

// base server function, dispatch packages
static void 
Dispatch(NetPeer *peer, uint8_t channel, const uint8_t *data, size_t size)
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
        ReleasePlayer(peer);
        NetPeerDisconnectLater(peer); 
        LogInfo("[NET] Peer requested disconnection (MSG_BYE)");
    } break;
    case MSG_UNIT_COMMAND:
        if (channel == CH_EVENTS)
            HandleUnitCommand(peer, data, header.size);
        break;
    case MSG_LOBBY_START:
        if (channel == CH_CONTROL)
            HandleLobbyStart(peer, data, header.size);
        break;
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

    LobbyInit(&g_lobby);

    if (!CreateWorld()) {
        LogFatal("Failed to create server world");
        CoreGameDestroy(&g_game);
        WorldDestroy(&g_world);
        return 1;
    }

    if (!NetInit()) {
        LogFatal("Failed to initialize enet6 network subsystem");
        CoreGameDestroy(&g_game);
        WorldDestroy(&g_world);
        return 1;
    }
    signal(SIGINT, OnSigInt);
    
    NetHost *host = NetServerStart(bind, port, 32);
    if (!host) {
        LogError("Failed to start and bind network server socket");
        NetStop();
        CoreGameDestroy(&g_game);
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
                NetPeerSetTimeout(ev.peer, 2, 3000, 8000);
                LogInfo("[+] New connection established");
            } break;
            case NET_EV_DISCONNECT: {
                ReleasePlayer(ev.peer);
                LogInfo("[NET] Peer transport disconnected");
            } break;
            case NET_EV_RECEIVE:
                Dispatch(ev.peer, ev.channel, ev.data, ev.size);
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
    CoreGameDestroy(&g_game);
    WorldDestroy(&g_world);
    return 0;
}
