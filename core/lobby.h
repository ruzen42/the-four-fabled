/* SPDX-License-Identifier: AGPL-3.0-only
 * 5/11/2026 ruzen42 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LOBBY_MAX_PLAYERS 4

typedef struct Lobby {
    uint8_t playerMask;
    uint8_t hostId;
    bool started;
} Lobby;

void LobbyInit(Lobby *lobby);
unsigned LobbyPlayerCount(const Lobby *lobby);
bool LobbyJoin(Lobby *lobby, uint8_t *outPlayerId);
bool LobbyLeave(Lobby *lobby, uint8_t playerId);
bool LobbyCanStart(const Lobby *lobby, uint8_t playerId);
bool LobbyStart(Lobby *lobby, uint8_t playerId);
