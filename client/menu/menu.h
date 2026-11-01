/* SPDX-License-Identifier: AGPL-3.0-only 
 * 01/11/2026 ruzen42 */
#pragma once

#include "client/game/game.h"

typedef enum MenuItem {
    ITEM_START = 0,
    ITEM_QUIT,
    ITEM_COUNT
} MenuItem;

void MenuReset(void);
void DrawMenu(void);

GameScreen UpdateMenu(void);
