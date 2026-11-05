/* SPDX-License-Identifier: AGPL-3.0-only
 * 5/11/2026 ruzen42 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "core/types.h"

typedef struct ClientLobbyState {
    uint8_t playerMask;
    uint8_t hostId;
    uint8_t requiredPlayers;
    bool started;
} ClientLobbyState;

// initialize and connect the server
bool ClientNetConnect(const char *addr, uint16_t port);

// poll incoming ENet packets and process events (call every frame)
void ClientNetPoll(void);

// disconnect and clean up network resources
void ClientNetDisconnect(void);

// check if the client as completed the handshake and is connect
bool ClientIsConnected(void);
uint32_t ClientNetAssignedId(void);
bool ClientNetMoveUnit(uint32_t unitId, Position destination);
bool ClientNetAttackUnit(uint32_t unitId, uint32_t targetUnitId);
bool ClientNetEndTurn(void);
bool ClientNetGetLobbyState(ClientLobbyState *outState);
bool ClientNetStartGame(void);
