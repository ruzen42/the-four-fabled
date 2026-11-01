/* SPDX-License-Identifier: AGPL-3.0-only 
 * 01/11/2026 ruzen42 */

#include <raylib.h>

#include "client/game/game.h"
#include "client/menu/menu.h"

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

    GameScreen screen = SCREEN_MENU;

    GameReset();
    MenuReset();

    while (!WindowShouldClose()) {
        switch (screen) {
            case SCREEN_GAME:
                screen = UpdateGame();
                break;
            case SCREEN_MENU:
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
    TraceLog(LOG_INFO, "Game closed");
    CloseWindow();
    return 0;
}
