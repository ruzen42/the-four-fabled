/* SPDX-License-Identifier: AGPL-3.0-only 
 * 31/10/2026 ruzen42 */

#include <raylib.h>
#include <stdio.h>
#include <math.h>

// isometry params
#define TILE_WIDTH  64
#define TILE_HEIGHT 32
#define MAP_SIZE    10

// screen offset (map origin)
static const float OFFSET_X = 400.0f;
static const float OFFSET_Y = 100.0f;

static const Color COL_BG           = { 26,  28,  36, 255 };  // #1A1C24
static const Color COL_TILE         = { 46,  52,  64, 255 };  // #2E3440
static const Color COL_TILE_HOVER   = { 224, 176, 64, 255 };  // #E0B040 
static const Color COL_TILE_SEL     = { 64,  200, 224, 255 }; // #40C8E0 
static const Color COL_TILE_SEL_HOV = { 128, 240, 255, 255 }; // #80F0FF 

static const Color COL_OUTLINE      = { 74,  74,  90, 120 };  
static const Color COL_OUTLINE_SEL  = { 64,  200, 224, 255 }; 

static const Color COL_TEXT         = { 232, 232, 240, 255 };// #E8E8F0
static const Color COL_TEXT_DIM     = { 144, 144, 160, 255 };// #9090A0


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

static void
IsoTileVerts(int gridX, int gridY, Vector2 v[4])
{
    Vector2 pos = IsoToScreen(gridX, gridY);
    pos.x += OFFSET_X;
    pos.y += OFFSET_Y;

    v[0] = (Vector2){ pos.x,                      pos.y };                      // top
    v[1] = (Vector2){ pos.x + TILE_WIDTH / 2.0f,  pos.y + TILE_HEIGHT / 2.0f };// right
    v[2] = (Vector2){ pos.x,                      pos.y + TILE_HEIGHT };       // bottom
    v[3] = (Vector2){ pos.x - TILE_WIDTH / 2.0f,  pos.y + TILE_HEIGHT / 2.0f };// left
}

void
DrawIsoTile(int gridX, int gridY, Color color, Color outLine, float lineThick)
{
    Vector2 v[4];
    IsoTileVerts(gridX, gridY, v);

    DrawTriangleFan(v, 4, color);   

    DrawLineEx(v[0], v[1], lineThick, outLine);
    DrawLineEx(v[1], v[2], lineThick, outLine);
    DrawLineEx(v[2], v[3], lineThick, outLine);
    DrawLineEx(v[3], v[0], lineThick, outLine);
}

int
RunGame()
{
    const int screenWidth  = 800;
    const int screenHeight = 450;

    SetTraceLogLevel(LOG_DEBUG);

    InitWindow(screenWidth, screenHeight, "The Four Fabled");
    SetTargetFPS(60);

    TraceLog(LOG_INFO, "Game started: map %dx%d, tile %dx%d",
             MAP_SIZE, MAP_SIZE, TILE_WIDTH, TILE_HEIGHT);

    int selectedX = -1;
    int selectedY = -1;

    int prevGridX = -999;
    int prevGridY = -999;

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();

        Vector2 isoPos = ScreenToIso(mousePos.x - OFFSET_X,
                                     mousePos.y - OFFSET_Y - TILE_HEIGHT);

        int gridX = (int)floor(isoPos.x);
        int gridY = (int)floor(isoPos.y);

        bool inMap = (unsigned)gridX < MAP_SIZE &&
                     (unsigned)gridY < MAP_SIZE;

        if (gridX != prevGridX || gridY != prevGridY) {
            if (inMap) {
                TraceLog(LOG_TRACE, "Hover -> (%d, %d)", gridX, gridY);
            } else {
                TraceLog(LOG_TRACE, "Hover -> outside (%d, %d)", gridX, gridY);
            }
            prevGridX = gridX;
            prevGridY = gridY;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (inMap) {
                selectedX = gridX;
                selectedY = gridY;
                TraceLog(LOG_INFO, "Selected tile (%d, %d)", gridX, gridY);
            } else {
                TraceLog(LOG_DEBUG, "Click outside map: grid (%d, %d)",
                         gridX, gridY);
            }
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) &&
            selectedX >= 0 && selectedY >= 0) {
            TraceLog(LOG_INFO, "Selection cleared (was %d, %d)",
                     selectedX, selectedY);
            selectedX = -1;
            selectedY = -1;
        }

        BeginDrawing();
        {
            ClearBackground(COL_BG);

            for (int y = 0; y < MAP_SIZE; y++) {
                for (int x = 0; x < MAP_SIZE; x++) {
                    bool isHover = inMap && x == gridX && y == gridY;
                    bool isSel   = (x == selectedX && y == selectedY);

                    Color tileColor = COL_TILE;
                    Color outline   = COL_OUTLINE;
                    float thick     = 1.0f;

                    if (isHover) tileColor = COL_TILE_HOVER;

                    if (isSel) {
                        tileColor = COL_TILE_SEL;
                        outline   = COL_OUTLINE_SEL;
                        thick     = 2.0f;
                    }

                    if (isSel && isHover) tileColor = COL_TILE_SEL_HOV;

                    DrawIsoTile(x, y, tileColor, outline, thick);
                }
            }

            DrawText(TextFormat("Grid: (%d, %d)", gridX, gridY),
                     10, 10, 20, COL_TEXT);

            if (selectedX >= 0 && selectedY >= 0) {
                DrawText(TextFormat("Selected: (%d, %d)",
                                    selectedX, selectedY),
                         10, 40, 20, COL_TILE_SEL);
            } else {
                DrawText("Click to select tile", 10, 40, 20, COL_TEXT_DIM);
            }

            DrawText("RMB: clear selection", 10, 70, 16, COL_TEXT_DIM);
        }
        EndDrawing();
    }

    TraceLog(LOG_INFO, "Game closed");
    CloseWindow();
    return 0;
}
