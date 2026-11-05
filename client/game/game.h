/* SPDX-License-Identifier: AGPL-3.0-only 
 * 5/11/2026 ruzen42 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "core/types.h"
#include "core/net/protocol/protocol.h"

typedef enum GameScreen {
    SCREEN_GAME = 0,
    SCREEN_MENU,   
    SCREEN_QUIT
} GameScreen;

typedef struct ClientGameState {
    Position selectedTile; 
    bool hasSelection;
} ClientGameState;

void GameReset(void);
void GameShutdown(void);
bool GameSetWorldSnapshot(uint32_t width, uint32_t height, const uint8_t *tileTypes);
bool GameSetUnitSnapshot(const MsgUnitState *units, size_t unitCount,
                         uint8_t activeCiv);
bool GameHasWorld(void);

GameScreen UpdateGame(void);

// draw frame, not change client data
void DrawGame(void);


