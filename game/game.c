/* SPDX-License-Identifier: AGPL-3.0-only 
 * 31/10/2026 ruzen42 */

#include <raylib.h>
#include <math.h>

#include "game/game.h"
#include "game/colors.h"

// isometry params
#define TILE_WIDTH  64
#define TILE_HEIGHT 32
#define MAP_SIZE    10

// screen offset (map origin)
static const float OFFSET_X = 400.0f;
static const float OFFSET_Y = 100.0f;

// state, in future will be a struct 
static int  g_selX = -1;
static int  g_selY = -1;

static int  g_hoverX = 0;
static int  g_hoverY = 0;
static bool g_hoverInMap = false;

static int  g_prevGridX = -999;
static int  g_prevGridY = -999;

// in-game map -> what player saw
static Vector2
IsoToScreen(int gridX, int gridY)
{
    Vector2 screen;
    screen.x = (gridX - gridY) * (TILE_WIDTH / 2.0f);
    screen.y = (gridX + gridY) * (TILE_HEIGHT / 2.0f);
    return screen;
}

// what player saw -> in-game map 
// reversed IsoToScreen()
static Vector2
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

static void
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

void
GameReset(void)
{
    g_selX = g_selY = -1;
    g_hoverX = g_hoverY = 0;
    g_hoverInMap = false;
    g_prevGridX = g_prevGridY = -999;

    TraceLog(LOG_INFO, "GameReset: map %dx%d, tile %dx%d",
             MAP_SIZE, MAP_SIZE, TILE_WIDTH, TILE_HEIGHT);
}

GameScreen
UpdateGame(void)
{
    if (IsKeyPressed(KEY_ESCAPE))
        return SCREEN_QUIT;

    Vector2 mousePos = GetMousePosition();

    Vector2 isoPos = ScreenToIso(mousePos.x - OFFSET_X,
                                 mousePos.y - OFFSET_Y - TILE_HEIGHT);

    int gridX = (int)floor(isoPos.x);
    int gridY = (int)floor(isoPos.y);

    g_hoverX = gridX;
    g_hoverY = gridY;
    g_hoverInMap = (unsigned)gridX < MAP_SIZE &&
                   (unsigned)gridY < MAP_SIZE;

    if (gridX != g_prevGridX || gridY != g_prevGridY) {
        if (g_hoverInMap) {
            TraceLog(LOG_TRACE, "Hover -> (%d, %d)", gridX, gridY);
        } else {
            TraceLog(LOG_TRACE, "Hover -> outside (%d, %d)", gridX, gridY);
        }
        g_prevGridX = gridX;
        g_prevGridY = gridY;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (g_hoverInMap) {
            g_selX = gridX;
            g_selY = gridY;
            TraceLog(LOG_INFO, "Selected tile (%d, %d)", gridX, gridY);
        } else {
            TraceLog(LOG_DEBUG, "Click outside map: grid (%d, %d)",
                     gridX, gridY);
        }
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) &&
        g_selX >= 0 && g_selY >= 0) {
        TraceLog(LOG_INFO, "Selection cleared (was %d, %d)", g_selX, g_selY);
        g_selX = -1;
        g_selY = -1;
    }

    return SCREEN_GAME;
}

void
DrawGame(void)
{
    ClearBackground(COL_BG);

    for (int y = 0; y < MAP_SIZE; y++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            bool isHover = g_hoverInMap && x == g_hoverX && y == g_hoverY;
            bool isSel   = (x == g_selX && y == g_selY);

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

    DrawText(TextFormat("Grid: (%d, %d)", g_hoverX, g_hoverY),
             10, 10, 20, COL_TEXT);

    if (g_selX >= 0 && g_selY >= 0) {
        DrawText(TextFormat("Selected: (%d, %d)", g_selX, g_selY),
                 10, 40, 20, COL_TILE_SEL);
    } else {
        DrawText("Click to select tile", 10, 40, 20, COL_TEXT_DIM);
    }

    DrawText("RMB: clear selection   Esc: quit", 10, 70, 16, COL_TEXT_DIM);
}
