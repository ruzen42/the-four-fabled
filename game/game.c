/* SPDX-License-Identifier: AGPL-3.0-only 
 * 31/10/2026 ruzen42 */

#include <raylib.h>
#include <math.h>

// isometry params
#define TILE_WIDTH  64
#define TILE_HEIGHT 32
#define MAP_SIZE    10

// screen offset (map origin)
static const float OFFSET_X = 400.0f;
static const float OFFSET_Y = 100.0f;

// dark theme palette
static const Color COL_BG        = { 30, 30, 40, 255 };    // #1E1E28
static const Color COL_TILE      = { 58, 58, 74, 255 };    // #3A3A4A
static const Color COL_TILE_HOVER= { 224, 176, 64, 255 };  // #E0B040
static const Color COL_TILE_SEL  = { 224, 90, 80, 255 };   // #E05A50
static const Color COL_OUTLINE   = { 90, 90, 110, 160 };   // semiclear 
static const Color COL_TEXT      = { 232, 232, 240, 255 };// #E8E8F0
static const Color COL_TEXT_DIM  = { 144, 144, 160, 255 };// #9090A0

// in-game map -> what player saw
Vector2
IsoToScreen(int gridX, int gridY)
{
    Vector2 screen;
    screen.x = (gridX - gridY) * (TILE_WIDTH / 2.0f);
    screen.y = (gridX + gridY) * (TILE_HEIGHT / 2.0f);
    return screen;
}

// what player saw -> in-game map 
// reversed IsoToScreen()
Vector2
ScreenToIso(float screenX, float screenY)
{
    Vector2 grid;
    grid.x = (screenX / (TILE_WIDTH / 2.0f)
            + screenY / (TILE_HEIGHT / 2.0f)) / 2.0f;

    grid.y = (screenY / (TILE_HEIGHT / 2.0f)
            - screenX / (TILE_WIDTH / 2.0f)) / 2.0f;
    return grid;
}

void
DrawIsoTile(int gridX, int gridY, Color color, Color outLine)
{
    Vector2 pos = IsoToScreen(gridX, gridY);

    pos.x += OFFSET_X;
    pos.y += OFFSET_Y;

    Vector2 top    = { pos.x, pos.y };
    Vector2 right  = { pos.x + TILE_WIDTH / 2.0f,  pos.y + TILE_HEIGHT / 2.0f };
    Vector2 bottom = { pos.x,                      pos.y + TILE_HEIGHT };
    Vector2 left   = { pos.x - TILE_WIDTH / 2.0f,  pos.y + TILE_HEIGHT / 2.0f };

    // заливка ромба одним веером
    Vector2 fan[4] = { top, right, bottom, left };
    DrawTriangleFan(fan, 4, color);

    // обводка одной полосой
    Vector2 outline[5] = { top, right, bottom, left, top };
    DrawLineStrip(outline, 5, outLine);
}

int
RunGame()
{
    const int screenWidth  = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "The Four Fabled");

    SetTargetFPS(60);

    int selectedX = -1;
    int selectedY = -1;

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();

        Vector2 isoPos = ScreenToIso(mousePos.x - OFFSET_X,
                                     mousePos.y - OFFSET_Y);

        int gridX = (int)floor(isoPos.x);
        int gridY = (int)floor(isoPos.y);

        bool inMap = (unsigned)gridX < MAP_SIZE &&
                     (unsigned)gridY < MAP_SIZE;

        if (inMap && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            selectedX = gridX;
            selectedY = gridY;
        }

        BeginDrawing();
        {
            ClearBackground(COL_BG);

            for (int y = 0; y < MAP_SIZE; y++) {
                for (int x = 0; x < MAP_SIZE; x++) {
                    Color tileColor = COL_TILE;

                    // hover
                    if (inMap && x == gridX && y == gridY) {
                        tileColor = COL_TILE_HOVER;
                    }

                    // selected
                    if (x == selectedX && y == selectedY) {
                        tileColor = COL_TILE_SEL;
                    }

                    DrawIsoTile(x, y, tileColor, COL_OUTLINE);
                }
            }

            DrawText(TextFormat("Grid: (%d, %d)", gridX, gridY),
                     10, 10, 20, COL_TEXT);
            DrawText("Click to select tile",
                     10, 40, 20, COL_TEXT_DIM);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

