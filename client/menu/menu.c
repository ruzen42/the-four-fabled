/* SPDX-License-Identifier: AGPL-3.0-only 
 * 4/11/2026 ruzen42 */

#include <raylib.h>
#include "log.h"
#include "menu/menu.h"
#include "game/colors.h"
#include "net.h"

#define GAME_TITLE     "THE FOUR FABLED"
#define GAME_SUBTITLE  "pc edition v0.1.0"

static MenuState s_menuState = MENU_STATE_MAIN;
static int       g_cursor    = MAIN_ITEM_START;
static char      s_serverAddress[64] = "::1";
static char      s_serverPort[6] = "7007";
static int       s_netInputFocus = 0;
static const char *s_netStatus = "Enter server IPv6 address and port";

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
    const float fieldsH = (s_menuState == MENU_STATE_NET) ? 150.0f : 0.0f;

    const float totalH = titleH + titleGap + subH + subBtnGap
                       + fieldsH + btnBlk + 60.0f + hintsH;

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

    if (s_menuState == MENU_STATE_NET) {
        const float fieldW = 440.0f;
        const float fieldX = (W - fieldW) * 0.5f;
        L.addressField = (Rectangle){ fieldX, y + 20.0f, fieldW, 38.0f };
        L.portField = (Rectangle){ fieldX, y + 84.0f, fieldW, 38.0f };
        y += fieldsH;
    }

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
    ClientLog("MenuReset");
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
                ClientLog("Menu: Start Game");
                return SCREEN_GAME;
            case MAIN_ITEM_NET:   
                ClientLog("Menu: Switch to Network Menu");
                s_menuState = MENU_STATE_NET;
                g_cursor = NET_ITEM_CONNECT;
                s_netInputFocus = 0;
                s_netStatus = "Enter server IPv6 address and port";
                s_layoutInitialized = false; 
                break;
            case MAIN_ITEM_QUIT:  
                ClientLog("Menu: Quit");
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

    if (IsKeyPressed(KEY_TAB)) {
        s_netInputFocus = (s_netInputFocus + 1) % 3;
        if (s_netInputFocus == 2) s_netInputFocus = -1;
    }

    if (s_netInputFocus < 0 && (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)))
        g_cursor = (g_cursor + 1) % NET_ITEM_COUNT;
    if (s_netInputFocus < 0 && (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)))
        g_cursor = (g_cursor + NET_ITEM_COUNT - 1) % NET_ITEM_COUNT;

    if (s_netInputFocus >= 0) {
        char *field = s_netInputFocus == 0 ? s_serverAddress : s_serverPort;
        size_t capacity = s_netInputFocus == 0 ? sizeof s_serverAddress : sizeof s_serverPort;
        size_t length = strlen(field);

        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
            IsKeyPressed(KEY_A)) {
            field[0] = '\0';
            length = 0;
        }

        if (IsKeyPressed(KEY_BACKSPACE) && length > 0)
            field[length - 1] = '\0';

        int codepoint;
        while ((codepoint = GetCharPressed()) != 0) {
            bool allowed = false;
            if (s_netInputFocus == 0) {
                allowed = (codepoint >= '0' && codepoint <= '9') ||
                          (codepoint >= 'a' && codepoint <= 'f') ||
                          (codepoint >= 'A' && codepoint <= 'F') ||
                          codepoint == ':' || codepoint == '.';
            } else {
                allowed = codepoint >= '0' && codepoint <= '9';
            }
            if (allowed && length + 1 < capacity) {
                field[length++] = (char)codepoint;
                field[length] = '\0';
            }
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
            s_netInputFocus = s_netInputFocus == 0 ? 1 : -1;
    }

    bool activate = s_netInputFocus < 0 &&
                    (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE));

    Vector2 m = GetMousePosition();
    bool clickedAny = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    int  clickedIdx = -1;

    if (clickedAny) {
        if (PointInRect(m, L->addressField)) s_netInputFocus = 0;
        else if (PointInRect(m, L->portField)) s_netInputFocus = 1;
        else if (PointInRect(m, L->items[NET_ITEM_CONNECT]) ||
                 PointInRect(m, L->items[NET_ITEM_BACK])) s_netInputFocus = -1;
    }

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
            {
                unsigned port = 0;
                if (s_serverPort[0] == '\0') {
                    s_netStatus = "Port is required";
                    break;
                }
                for (size_t i = 0; s_serverPort[i]; i++)
                    port = port * 10u + (unsigned)(s_serverPort[i] - '0');
                if (port == 0 || port > 65535u) {
                    s_netStatus = "Port must be between 1 and 65535";
                    break;
                }
                if (s_serverAddress[0] == '\0') {
                    s_netStatus = "IPv6 address is required";
                    break;
                }
                ClientLog("Menu: Connecting to server");
                s_netStatus = ClientNetConnect(s_serverAddress, (uint16_t)port)
                            ? "Connecting..." : "Could not start connection";
                break;
            }
            case NET_ITEM_BACK:
                s_menuState = MENU_STATE_MAIN;
                g_cursor = MAIN_ITEM_NET;
                s_netInputFocus = -1;
                s_layoutInitialized = false;
                break;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        s_menuState = MENU_STATE_MAIN;
        g_cursor = MAIN_ITEM_NET;
        s_netInputFocus = -1;
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

    DrawText("IPv6 address", (int)L->addressField.x, (int)L->addressField.y - 20, 16, COL_TEXT_DIM);
    DrawRectangleRec(L->addressField, COL_TILE);
    DrawRectangleLinesEx(L->addressField, 2.0f,
                         s_netInputFocus == 0 ? COL_OUTLINE_SEL : COL_OUTLINE);
    DrawText(s_serverAddress, (int)L->addressField.x + 10,
             (int)L->addressField.y + 9, 20, COL_TEXT);

    DrawText("Port", (int)L->portField.x, (int)L->portField.y - 20, 16, COL_TEXT_DIM);
    DrawRectangleRec(L->portField, COL_TILE);
    DrawRectangleLinesEx(L->portField, 2.0f,
                         s_netInputFocus == 1 ? COL_OUTLINE_SEL : COL_OUTLINE);
    DrawText(s_serverPort, (int)L->portField.x + 10,
             (int)L->portField.y + 9, 20, COL_TEXT);

    DrawButton(L->items[NET_ITEM_CONNECT], "Connect", g_cursor == NET_ITEM_CONNECT);
    DrawButton(L->items[NET_ITEM_BACK],    "Back", g_cursor == NET_ITEM_BACK);
    DrawText(s_netStatus, (int)L->portField.x,
             (int)L->portField.y + 44, 16, COL_TEXT_DIM);
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
    const char *hint1 = s_menuState == MENU_STATE_NET
                      ? "Click a field to edit    Tab - switch focus    Ctrl+A - clear field"
                      : "Up/Down or W/S - navigate    Enter/Space - select";
    const char *hint2 = s_menuState == MENU_STATE_NET
                      ? "Tab to Connect/Back    Enter - select    Esc - back"
                      : "Mouse - click    Esc - back/quit";
    DrawText(hint1,
             (int)L->hint1Pos.x, (int)L->hint1Pos.y, 16, COL_TEXT_DIM);
    DrawText(hint2,
             (int)L->hint2Pos.x, (int)L->hint2Pos.y, 16, COL_TEXT_DIM);
}
