/* SPDX-License-Identifier: AGPL-3.0-only 
 * 4/11/2026 ruzen42 */

#include <raylib.h>
#include "menu/menu.h"
#include "game/colors.h"
#include "net.h"

#define GAME_TITLE     "THE FOUR FABLED"
#define GAME_SUBTITLE  "pc edition v0.1.0"

static MenuState s_menuState = MENU_STATE_MAIN;
static int       g_cursor    = MAIN_ITEM_START;

static MenuLayout s_layout = { 0 };
static bool       s_layoutInitialized = false;

static MenuLayout
ComputeLayout(int itemCount)
{
    MenuLayout L = { 0 };
    L.itemCount = itemCount;

    const float W = (float)GetScreenWidth();
    const float H = (float)GetScreenHeight();

    const int   titleSize  = 56;
    const int   subSize    = 20;
    const float btnW       = 280.0f;
    const float btnH       = 56.0f;
    const float btnGap     = 16.0f;
    const float titleGap   = 20.0f;
    const float subBtnGap  = 48.0f;
    const float hintSize   = 16;

    const float titleH = titleSize;
    const float subH   = subSize;
    const float btnBlk = itemCount * btnH + (itemCount - 1) * btnGap;
    const float hintsH = hintSize * 2 + 4;

    const float totalH = titleH + titleGap + subH + subBtnGap
                       + btnBlk + 60.0f + hintsH;

    float y = (H - totalH) * 0.5f;
    if (y < 20.0f) y = 20.0f;

    const char *title = GAME_TITLE; 
    Vector2 tsz = MeasureTextEx(GetFontDefault(), title, titleSize, 2);
    L.titlePos  = (Vector2){ (W - tsz.x) * 0.5f, y };
    L.titleSize = titleSize;
    y += titleH + titleGap;

    const char *sub = (s_menuState == MENU_STATE_NET) ? "network lobby" : GAME_SUBTITLE; 
    Vector2 ssz = MeasureTextEx(GetFontDefault(), sub, subSize, 1);
    L.subPos  = (Vector2){ (W - ssz.x) * 0.5f, y };
    L.subSize = subSize;
    y += subH + subBtnGap;

    const float btnX = (W - btnW) * 0.5f;
    for (int i = 0; i < itemCount; i++) {
        L.items[i] = (Rectangle){
            btnX,
            y + i * (btnH + btnGap),
            btnW,
            btnH
        };
    }
    y += btnBlk + 60.0f;

    const char *h1 = "Up/Down or W/S - navigate    Enter/Space - select";
    const char *h2 = "Mouse - click    Esc - back/quit";
    Vector2 h1sz = MeasureTextEx(GetFontDefault(), h1, hintSize, 1);
    Vector2 h2sz = MeasureTextEx(GetFontDefault(), h2, hintSize, 1);
    L.hint1Pos = (Vector2){ (W - h1sz.x) * 0.5f, y };
    L.hint2Pos = (Vector2){ (W - h2sz.x) * 0.5f, y + hintSize + 4 };

    return L;
}

static const MenuLayout*
GetCachedLayout(int itemCount)
{
    if (!s_layoutInitialized || IsWindowResized()) {
        s_layout = ComputeLayout(itemCount);
        s_layoutInitialized = true;
    }
    return &s_layout;
}

static bool
PointInRect(Vector2 p, Rectangle r)
{
    return p.x >= r.x && p.x < r.x + r.width &&
           p.y >= r.y && p.y < r.y + r.height;
}

static void
DrawButton(Rectangle r, const char *label, bool focused)
{
    Color bg = focused ? COL_TILE_HOVER : COL_TILE;
    Color fg = focused ? COL_BG         : COL_TEXT;

    DrawRectangleRec(r, bg);
    DrawRectangleLinesEx(r, 2.0f, focused ? COL_OUTLINE_SEL : COL_OUTLINE);

    int fs = 26;
    Vector2 sz = MeasureTextEx(GetFontDefault(), label, fs, 1);
    DrawText(label,
             (int)(r.x + (r.width  - sz.x) * 0.5f),
             (int)(r.y + (r.height - sz.y) * 0.5f),
             fs, fg);
}

void
MenuReset(void)
{
    s_menuState = MENU_STATE_MAIN;
    g_cursor = MAIN_ITEM_START;
    s_layoutInitialized = false; 
    TraceLog(LOG_INFO, "MenuReset");
}

// Логика главного меню
static GameScreen
UpdateMainMenu(void)
{
    const MenuLayout *L = GetCachedLayout(MAIN_ITEM_COUNT);

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        g_cursor = (g_cursor + 1) % MAIN_ITEM_COUNT;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        g_cursor = (g_cursor + MAIN_ITEM_COUNT - 1) % MAIN_ITEM_COUNT;

    bool activate = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);

    Vector2 m = GetMousePosition();
    bool clickedAny = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    int  clickedIdx = -1;

    for (int i = 0; i < MAIN_ITEM_COUNT; i++) {
        if (PointInRect(m, L->items[i])) {
            g_cursor = i;
            if (clickedAny) clickedIdx = i;
        }
    }

    if (activate || clickedIdx >= 0) {
        int chosen = activate ? g_cursor : clickedIdx;
        switch (chosen) {
            case MAIN_ITEM_START: 
                TraceLog(LOG_INFO, "Menu: Start Game"); 
                return SCREEN_GAME;
            case MAIN_ITEM_NET:   
                TraceLog(LOG_INFO, "Menu: Switch to Network Menu"); 
                s_menuState = MENU_STATE_NET;
                g_cursor = NET_ITEM_CONNECT;
                s_layoutInitialized = false; 
                break;
            case MAIN_ITEM_QUIT:  
                TraceLog(LOG_INFO, "Menu: Quit");  
                return SCREEN_QUIT;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
        return SCREEN_QUIT;

    return SCREEN_MENU;
}

static GameScreen
UpdateNetMenu(void)
{
    const MenuLayout *L = GetCachedLayout(NET_ITEM_COUNT);

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        g_cursor = (g_cursor + 1) % NET_ITEM_COUNT;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        g_cursor = (g_cursor + NET_ITEM_COUNT - 1) % NET_ITEM_COUNT;

    bool activate = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);

    Vector2 m = GetMousePosition();
    bool clickedAny = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    int  clickedIdx = -1;

    for (int i = 0; i < NET_ITEM_COUNT; i++) {
        if (PointInRect(m, L->items[i])) {
            g_cursor = i;
            if (clickedAny) clickedIdx = i;
        }
    }

    if (activate || clickedIdx >= 0) {
        int chosen = activate ? g_cursor : clickedIdx;
        switch (chosen) {
            case NET_ITEM_CONNECT:
                TraceLog(LOG_INFO, "Menu: Connecting to server...");
                ClientNetConnect("::1", 7007); 
                break;
            case NET_ITEM_BACK:
                s_menuState = MENU_STATE_MAIN;
                g_cursor = MAIN_ITEM_NET;
                s_layoutInitialized = false;
                break;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        s_menuState = MENU_STATE_MAIN;
        g_cursor = MAIN_ITEM_NET;
        s_layoutInitialized = false;
    }

    return SCREEN_MENU;
}

GameScreen
UpdateMenu(void)
{
    switch (s_menuState) {
        case MENU_STATE_MAIN: return UpdateMainMenu();
        case MENU_STATE_NET:  return UpdateNetMenu();
    }
    return SCREEN_MENU;
}

static void
DrawMainMenu(void)
{
    const MenuLayout *L = GetCachedLayout(MAIN_ITEM_COUNT);

    DrawText(GAME_TITLE, (int)L->titlePos.x, (int)L->titlePos.y, L->titleSize, COL_TEXT);
    DrawText(GAME_SUBTITLE, (int)L->subPos.x, (int)L->subPos.y, L->subSize, COL_TEXT_DIM);

    DrawButton(L->items[MAIN_ITEM_START], "Start", g_cursor == MAIN_ITEM_START);
    DrawButton(L->items[MAIN_ITEM_NET],   "Network", g_cursor == MAIN_ITEM_NET);
    DrawButton(L->items[MAIN_ITEM_QUIT],  "Quit",  g_cursor == MAIN_ITEM_QUIT);
}

static void
DrawNetMenu(void)
{
    const MenuLayout *L = GetCachedLayout(NET_ITEM_COUNT);

    DrawText(GAME_TITLE, (int)L->titlePos.x, (int)L->titlePos.y, L->titleSize, COL_TEXT);
    DrawText("network lobby", (int)L->subPos.x, (int)L->subPos.y, L->subSize, COL_TEXT_DIM);

    DrawButton(L->items[NET_ITEM_CONNECT], "Connect (::1)", g_cursor == NET_ITEM_CONNECT);
    DrawButton(L->items[NET_ITEM_BACK],    "Back", g_cursor == NET_ITEM_BACK);
}

void
DrawMenu(void)
{
    ClearBackground(COL_BG);

    switch (s_menuState) {
        case MENU_STATE_MAIN: DrawMainMenu(); break;
        case MENU_STATE_NET:  DrawNetMenu();  break;
    }

    const MenuLayout *L = GetCachedLayout(s_menuState == MENU_STATE_MAIN ? MAIN_ITEM_COUNT : NET_ITEM_COUNT);
    DrawText("Up/Down or W/S - navigate    Enter/Space - select",
             (int)L->hint1Pos.x, (int)L->hint1Pos.y, 16, COL_TEXT_DIM);
    DrawText("Mouse - click    Esc - back/quit",
             (int)L->hint2Pos.x, (int)L->hint2Pos.y, 16, COL_TEXT_DIM);
}
