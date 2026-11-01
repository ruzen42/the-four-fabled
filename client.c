/* SPDX-License-Identifier: AGPL-3.0-only 
 * 31/10/2026 ruzen42 */

#include <raylib.h>

#include "game/game.h"

int
main(void)
{
    const int screenWidth  = 1600;
    const int screenHeight = 900;

    SetTraceLogLevel(LOG_DEBUG);

    InitWindow(screenWidth, screenHeight, "The Four Fabled");
    SetTargetFPS(0);

    GameScreen screen = SCREEN_GAME;
    GameReset();

    while (!WindowShouldClose()) {
        switch (screen) {
            case SCREEN_GAME:
                screen = UpdateGame();
                break;
            case SCREEN_MENU:
                // TODO: screen = UpdateMenu();
                screen = SCREEN_QUIT;
                break;
            case SCREEN_QUIT:
                goto done;
        }

        // draw 
        BeginDrawing();
        {
            switch (screen) {
                case SCREEN_GAME: DrawGame(); break;
                case SCREEN_MENU: /* TODO: DrawMenu(); */ break;
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
