/* SPDX-License-Identifier: AGPL-3.0-only 
 * 4/11/2026 ruzen42 */

#include <raylib.h>

#include "game/game.h"
#include "menu/menu.h"
#include "net.h"
#include "core/net/net.h"

int
main(void)
{
    SetTraceLogLevel(LOG_DEBUG);
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_FULLSCREEN_MODE);

    InitWindow(0, 0, "The Four Fabled");
    SetExitKey(0);
    #ifdef DEBUG
        SetTargetFPS(0);
    #else
        SetTargetFPS(165);
    #endif

    if (!NetInit()) {
        TraceLog(LOG_ERROR, "NetInit() failed");
        CloseWindow();
        return 1;
    }
    TraceLog(LOG_INFO, "NetInit()");

    GameScreen screen = SCREEN_MENU;
    GameReset();
    MenuReset();

    while (!WindowShouldClose()) {
        ClientNetPoll();

        switch (screen) {
            case SCREEN_GAME:
                screen = UpdateGame();
                break;
            case SCREEN_MENU:
                /* TODO: button trigger for ClientNetConnect("::1", 7007); */
                screen = UpdateMenu();
                break;
            case SCREEN_QUIT:
                goto done;
        }

        // draw 
        BeginDrawing();
        {
            switch (screen) {
                case SCREEN_GAME: DrawGame(); break;
                case SCREEN_MENU: DrawMenu(); break;
                case SCREEN_QUIT: break;
            }
        }
        EndDrawing();
    }

done:
    ClientNetDisconnect();
    TraceLog(LOG_INFO, "Game closed");
    CloseWindow();
    return 0;
}
