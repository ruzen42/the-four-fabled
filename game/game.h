/* SPDX-License-Identifier: AGPL-3.0-only 
 * 31/10/2026 ruzen42 */
#pragma once

typedef enum GameScreen {
    SCREEN_GAME = 0,
    SCREEN_MENU,   
    SCREEN_QUIT
} GameScreen;

void GameReset(void);

GameScreen UpdateGame(void);

// draw frame, not change client data
void DrawGame(void);
