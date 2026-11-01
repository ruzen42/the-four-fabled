/* SPDX-License-Identifier: AGPL-3.0-only 
 * 01/11/2026 ruzen42 */

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <math.h>

#include "game/game.h"
#include "game/colors.h"
#include "core/world.h"

// isometry params
#define TILE_WIDTH  128
#define TILE_HEIGHT 48

// client 
static World    world;          
static Camera2D camera;

static int  selX = -1;
static int  selY = -1;

static int  hoverX = 0;
static int  hoverY = 0;
static bool hoverInMap = false;

static int  prevGridX = -999;
static int  prevGridY = -999;

typedef struct {
    Vector2 v[4];
    Color color;
} VisibleTileData;

#define MAX_VISIBLE_TILES 4000

static VisibleTileData g_visibleTiles[MAX_VISIBLE_TILES];
static int g_visibleCount = 0;

static Vector2
IsoToScreen(int gridX, int gridY)
{
    Vector2 w;
    w.x = (gridX - gridY) * (TILE_WIDTH  / 2.0f);
    w.y = (gridX + gridY) * (TILE_HEIGHT / 2.0f);
    return w;
}

static bool 
IsTileVisible(Camera2D cam, int gx, int gy)
{
    Vector2 pos = IsoToScreen(gx, gy);
    
    float minX = pos.x - TILE_WIDTH / 2.0f;
    float maxX = pos.x + TILE_WIDTH / 2.0f;
    float minY = pos.y; 
    float maxY = pos.y + TILE_HEIGHT; 

    Vector2 screenTopLeft = (Vector2){0, 0};
    Vector2 worldTopLeft = GetScreenToWorld2D(screenTopLeft, cam);
    
    Vector2 screenBottomRight = (Vector2){GetScreenWidth(), GetScreenHeight()};
    Vector2 worldBottomRight = GetScreenToWorld2D(screenBottomRight, cam);
    
    // simpel AABB checking
    if (maxX < worldTopLeft.x || minX > worldBottomRight.x) return false;
    if (maxY < worldTopLeft.y || minY > worldBottomRight.y) return false;

    return true;
}

static Vector2
ScreenToIso(float worldX, float worldY)
{
    Vector2 grid;
    // correct translate 
    grid.x = (worldX / (TILE_WIDTH  / 2.0f)
            + worldY / (TILE_HEIGHT / 2.0f)) / 2.0f;
    grid.y = (worldY / (TILE_HEIGHT / 2.0f)
            - worldX / (TILE_WIDTH  / 2.0f)) / 2.0f;
    return grid;
}

static void
IsoTileVerts(int gridX, int gridY, Vector2 v[4])
{
    Vector2 pos = IsoToScreen(gridX, gridY);

    v[0] = (Vector2){ pos.x,                      pos.y };
    v[1] = (Vector2){ pos.x + TILE_WIDTH  / 2.0f, pos.y + TILE_HEIGHT / 2.0f };
    v[2] = (Vector2){ pos.x,                      pos.y + TILE_HEIGHT };
    v[3] = (Vector2){ pos.x - TILE_WIDTH  / 2.0f, pos.y + TILE_HEIGHT / 2.0f };
}

/* dead code
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
*/

static bool
PointInTileWorld(Vector2 p, int gx, int gy)
{
    Vector2 v[4];
    IsoTileVerts(gx, gy, v);

    float cx = v[0].x;
    float cy = v[0].y + TILE_HEIGHT / 2.0f;
    float dx = fabsf(p.x - cx);
    float dy = fabsf(p.y - cy);

    return (dx / (TILE_WIDTH / 2.0f) + dy / (TILE_HEIGHT / 2.0f)) <= 1.0f;
}

static bool
PickTileWorld(Vector2 p, int *outX, int *outY)
{
    // OFFSET for working
    Vector2 g = ScreenToIso(p.x, p.y + TILE_HEIGHT);
    int bx = (int)floorf(g.x);
    int by = (int)floorf(g.y);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = bx + dx;
            int cy = by + dy;
            if (PointInTileWorld(p, cx, cy)) {
                *outX = cx;
                *outY = cy;
                return true;
            }
        }
    }
    *outX = bx;
    *outY = by;
    return false;
}

static void
SeedTestWorld(void)
{
    for (uint32_t y = 0; y < world.height; y++) {
        for (uint32_t x = 0; x < world.width; x++) {
            Tile *t = WorldGetTile(&world, (Position){ (int)x, (int)y });
            if (!t) continue;

            uint32_t h = x * 374761393u + y * 668265263u;
            h = (h ^ (h >> 13)) * 1274126177u;
            h ^= (h >> 16);

            t->pos.x = (int)x;
            t->pos.y = (int)y;
            t->type  = (TileType)(h % 4);
        }
    }
}

void
GameReset(void)
{
    WorldDestroy(&world);
    if (!WorldCreate(&world, 128, 128)) {
        TraceLog(LOG_ERROR, "WorldCreate failed");
        return;
    }
    SeedTestWorld();

    selX = selY = -1;
    hoverX = hoverY = 0;
    hoverInMap = false;
    prevGridX = prevGridY = -999;

    float cx = ((float)world.width  - 1.0f) * 0.5f;
    float cy = ((float)world.height - 1.0f) * 0.5f;

    Vector2 mapCenter;
    mapCenter.x = (cx - cy) * (TILE_WIDTH  / 2.0f);
    mapCenter.y = (cx + cy) * (TILE_HEIGHT / 2.0f);

    camera.target   = mapCenter;
    camera.offset   = (Vector2){ GetScreenWidth()  / 2.0f,
                                 GetScreenHeight() / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    TraceLog(LOG_INFO, "GameReset: world %ux%u, target=(%.1f,%.1f)",
             world.width, world.height, camera.target.x, camera.target.y);
}

GameScreen
UpdateGame(void)
{
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorld  = GetScreenToWorld2D(mouseScreen, camera);

        camera.offset = Vector2Multiply(mouseScreen, GetWindowScaleDPI());
        camera.target = mouseWorld;

        float scale = 0.2f * wheel;
        camera.zoom = Clamp(expf(logf(camera.zoom) + scale), 0.25f, 4.0f);
    }

    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);

    int gx, gy;
    bool inMap = PickTileWorld(mouseWorld, &gx, &gy);
    inMap = inMap
         && (unsigned)gx < world.width
         && (unsigned)gy < world.height;

    hoverX = gx;
    hoverY = gy;
    hoverInMap = inMap;

    if (gx != prevGridX || gy != prevGridY) {
        if (inMap) TraceLog(LOG_TRACE, "Hover -> (%d,%d)", gx, gy);
        else       TraceLog(LOG_TRACE, "Hover -> outside (%d,%d)", gx, gy);
        prevGridX = gx;
        prevGridY = gy;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (inMap) {
            selX = gx;
            selY = gy;
            TraceLog(LOG_INFO, "Selected tile (%d,%d)", gx, gy);
        } else {
            TraceLog(LOG_DEBUG, "Click outside map: (%d,%d)", gx, gy);
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (selX >= 0) {
            TraceLog(LOG_INFO, "Selection cleared (was %d,%d)", selX, selY);
            selX = selY = -1;
        }
    }

    return SCREEN_GAME;
}

static Color
TileColorFor(TileType t)
{
    switch (t) {
        case SEA:      return (Color){  69, 133, 136, 255 }; // aqua dim
        case GRASS:    return (Color){ 167, 192, 128, 255 }; // green
        case FOREST:   return (Color){  79, 121,  66, 255 }; // deep green
        case MOUNTAIN: return (Color){ 133, 146, 137, 255 }; // grey
        default:       return COL_TILE;
    }
}

static void 
DrawIsoTileOutlineOnly(int gridX, int gridY, Color outLine, float lineThick)
{
    Vector2 v[4];
    IsoTileVerts(gridX, gridY, v);

    DrawLineEx(v[0], v[1], lineThick, outLine);
    DrawLineEx(v[1], v[2], lineThick, outLine);
    DrawLineEx(v[2], v[3], lineThick, outLine);
    DrawLineEx(v[3], v[0], lineThick, outLine);

}

void
DrawGame(void)
{
    ClearBackground(COL_BG);

    BeginMode2D(camera);
    {
        g_visibleCount = 0;

        for (uint32_t y = 0; y < world.height; y++) {
            for (uint32_t x = 0; x < world.width; x++) {
                if (!IsTileVisible(camera, (int)x, (int)y)) continue;

                if (g_visibleCount >= MAX_VISIBLE_TILES) break;

                const Tile *t = 
                    WorldGetTileConst(&world, (Position){(int)x, (int)y});

                Color tileColor = t ? TileColorFor(t->type) : COL_TILE;

                bool isHover = hoverInMap && (int)x == hoverX && (int)y == hoverY;
                bool isSel   = ((int)x == selX && (int)y == selY);

                if (isHover) tileColor = COL_TILE_HOVER;

                if (isSel) {
                    tileColor = COL_TILE_SEL;
                }
                if (isSel && isHover) tileColor = COL_TILE_SEL_HOV;

                IsoTileVerts((int)x, (int)y, g_visibleTiles[g_visibleCount].v);
                g_visibleTiles[g_visibleCount].color = tileColor;
                g_visibleCount++;
            }
        }
        

        // must works with zero
        rlSetTexture(0);
        rlBegin(RL_TRIANGLES);

        for (int i = 0; i < g_visibleCount; i++) {
            Vector2 *v = g_visibleTiles[i].v;
            Color col = g_visibleTiles[i].color;
            
            rlColor4ub(col.r, col.g, col.b, col.a);
            // first triagle for romb (0, 1, 2)
            rlVertex2f(v[0].x, v[0].y);
            rlVertex2f(v[1].x, v[1].y);
            rlVertex2f(v[2].x, v[2].y);

            // second triagle for romb (0, 2, 3)
            rlColor4ub(col.r, col.g, col.b, col.a);
            rlVertex2f(v[0].x, v[0].y);
            rlVertex2f(v[2].x, v[2].y);
            rlVertex2f(v[3].x, v[3].y);
        }
        
        rlEnd();

        if (hoverInMap) {
            DrawIsoTileOutlineOnly(hoverX, hoverY, COL_OUTLINE, 1.0f);
        }
        if (selX >= 0 && selY >= 0) {
            DrawIsoTileOutlineOnly(selX, selY, COL_OUTLINE_SEL, 2.0f);
        }
    }
    EndMode2D();

    DrawText(TextFormat("Grid: (%d,%d)", hoverX, hoverY), 10, 10, 20, COL_TEXT);

    if (selX >= 0 && selY >= 0) {
        const Tile *t = WorldGetTileConst(&world, (Position){ selX, selY });
        DrawText(TextFormat("Selected: (%d,%d) type=%d",
                            selX, selY, t ? (int)t->type : -1),
                 10, 40, 20, COL_TILE_SEL);
    } else {
        DrawText("Click to select tile", 10, 40, 20, COL_TEXT_DIM);
    }

    DrawText("LMB: select   MMB: pan   Wheel: zoom   Esc: clear sel",
             10, 70, 16, COL_TEXT_DIM);
    DrawText(TextFormat("zoom = %.2fx", camera.zoom), 10, 95, 16, COL_TEXT_DIM);
    DrawFPS(10, 120);
}
