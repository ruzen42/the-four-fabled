/* SPDX-License-Identifier: AGPL-3.0-only 
 * 4/11/2026 ruzen42 */

#include <raylib.h>
#include "log.h"

#include "game/game.h"
#include "menu/menu.h"
#include "net.h"
#include "core/net/net.h"

int
main(void)
{
    SetTraceLogLevel(LOG_NONE);
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_FULLSCREEN_MODE);

    InitWindow(0, 0, "The Four Fabled");
    SetExitKey(0);
    #ifdef DEBUG
        SetTargetFPS(0);
    #else
        SetTargetFPS(165);
    #endif

    if (!NetInit()) {
        ClientLog("NetInit() failed");
        CloseWindow();
        return 1;
    }
    ClientLog("NetInit()");

    GameScreen screen = SCREEN_MENU;
    GameReset();
    MenuReset();

    while (!WindowShouldClose()) {
        ClientNetPoll();

        if (ClientIsConnected() && GameHasWorld())
            screen = SCREEN_GAME;

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
    ClientLog("Game closed");
    CloseWindow();
    return 0;
}
