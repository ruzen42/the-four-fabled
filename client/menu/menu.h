/* SPDX-License-Identifier: AGPL-3.0-only 
 * 01/11/2026 ruzen42 */
#pragma once

#include "game/game.h"

typedef enum MenuItem {
    ITEM_START = 0,
    ITEM_QUIT,
    ITEM_COUNT
} MenuItem;

typedef struct MenuLayout {
    Vector2   titlePos;
    int       titleSize;
    Vector2   subPos;
    int       subSize;
    Rectangle items[ITEM_COUNT];
    Vector2   hint1Pos;
    Vector2   hint2Pos;
} MenuLayout;

void MenuReset(void);
void DrawMenu(void);

GameScreen UpdateMenu(void);
