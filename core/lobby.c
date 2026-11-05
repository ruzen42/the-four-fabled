/* SPDX-License-Identifier: AGPL-3.0-only
 * 5/11/2026 ruzen42 */
#include "lobby.h"

#include <string.h>

void
LobbyInit(Lobby *lobby)
{
    if (lobby) memset(lobby, 0, sizeof *lobby);
}

unsigned
LobbyPlayerCount(const Lobby *lobby)
{
    if (!lobby) return 0;
    unsigned count = 0;
    for (uint8_t i = 0; i < LOBBY_MAX_PLAYERS; i++)
        if (lobby->playerMask & (uint8_t)(1u << i)) count++;
    return count;
}

bool
LobbyJoin(Lobby *lobby, uint8_t *outPlayerId)
{
    if (!lobby || !outPlayerId || lobby->started) return false;
    for (uint8_t slot = 0; slot < LOBBY_MAX_PLAYERS; slot++) {
        uint8_t bit = (uint8_t)(1u << slot);
        if (lobby->playerMask & bit) continue;
        lobby->playerMask |= bit;
        *outPlayerId = (uint8_t)(slot + 1);
        if (!lobby->hostId) lobby->hostId = *outPlayerId;
        return true;
    }
    return false;
}

bool
LobbyLeave(Lobby *lobby, uint8_t playerId)
{
    if (!lobby || playerId == 0 || playerId > LOBBY_MAX_PLAYERS) return false;
    uint8_t bit = (uint8_t)(1u << (playerId - 1));
    if (!(lobby->playerMask & bit)) return false;
    lobby->playerMask &= (uint8_t)~bit;
    if (lobby->hostId == playerId) {
        lobby->hostId = 0;
        for (uint8_t slot = 0; slot < LOBBY_MAX_PLAYERS; slot++) {
            if (lobby->playerMask & (uint8_t)(1u << slot)) {
                lobby->hostId = (uint8_t)(slot + 1);
                break;
            }
        }
    }
    return true;
}

bool
LobbyCanStart(const Lobby *lobby, uint8_t playerId)
{
    return lobby && !lobby->started &&
           lobby->playerMask == (uint8_t)((1u << LOBBY_MAX_PLAYERS) - 1u) &&
           lobby->hostId != 0 && playerId == lobby->hostId;
}

bool
LobbyStart(Lobby *lobby, uint8_t playerId)
{
    if (!LobbyCanStart(lobby, playerId)) return false;
    lobby->started = true;
    return true;
}
