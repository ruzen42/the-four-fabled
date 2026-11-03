/* SPDX-License-Identifier: AGPL-3.0-only 
 * 03/11/2026 ruzen42 */
#pragma once
#include "game/game.h"

typedef enum MenuState {
    MENU_STATE_MAIN = 0,
    MENU_STATE_NET
} MenuState;

typedef enum MainMenuItem {
    MAIN_ITEM_START = 0,
    MAIN_ITEM_NET,       
    MAIN_ITEM_QUIT,
    MAIN_ITEM_COUNT
} MainMenuItem;

typedef enum NetMenuItem {
    NET_ITEM_CONNECT = 0,
    NET_ITEM_BACK,       
    NET_ITEM_COUNT
} NetMenuItem;

typedef struct MenuLayout {
    Vector2   titlePos;
    int       titleSize;
    Vector2   subPos;
    int       subSize;
    Rectangle items[4]; 
    int       itemCount;
    Vector2   hint1Pos;
    Vector2   hint2Pos;
} MenuLayout;

void MenuReset(void);
void DrawMenu(void);

GameScreen UpdateMenu(void);
