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
    int gx, gy;
} VisibleTileData;

#define MAX_VISIBLE_TILES 2304 

static VisibleTileData g_visibleTiles[MAX_VISIBLE_TILES];
static int g_visibleCount = 0;

static Vector2
IsoToScreen(float worldX, float worldY)
{
    Vector2 w;
    w.x = (worldX - worldY) * (TILE_WIDTH  / 2.0f);
    w.y = (worldX + worldY) * (TILE_HEIGHT / 2.0f);
    return w;
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

static bool
PointInTileWorld(Vector2 mouseScreen, int gx, int gy, Camera2D cam)
{
    Vector2 pos = IsoToScreen(gx, gy);

    Vector2 centerWorld = {
        pos.x,
        pos.y + TILE_HEIGHT * 0.5f
    };

    Vector2 centerScreen = GetWorldToScreen2D(centerWorld, cam);

    float halfW = TILE_WIDTH * cam.zoom * 0.5f;
    float halfH = TILE_HEIGHT * cam.zoom * 0.5f;

    float dx = fabsf(mouseScreen.x - centerScreen.x);
    float dy = fabsf(mouseScreen.y - centerScreen.y);
    
    return (dx / halfW + dy / halfH) <= 1.0f;
}

static bool
PickTileWorld(Vector2 mouseScreen, Camera2D cam, int *outX, int *outY)
{
    Vector2 w = GetScreenToWorld2D(mouseScreen, cam);
    float wy = w.y - TILE_HEIGHT * 0.5f;

    float fx = (w.x / (TILE_WIDTH  * 0.5f) + wy / (TILE_HEIGHT * 0.5f)) * 0.5f;
    float fy = (wy  / (TILE_HEIGHT * 0.5f) - w.x / (TILE_WIDTH  * 0.5f)) * 0.5f;

    int bx = (int)floorf(fx + 0.5f);
    int by = (int)floorf(fy + 0.5f);

    // check candidate and closer heigrou O(1) 
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = bx + dx;
            int cy = by + dy;
            if (PointInTileWorld(mouseScreen, cx, cy, cam)) {
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
    if (!WorldCreate(&world, 48, 48)) {
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

    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    TraceLog(LOG_INFO, "GameReset: world %ux%u, target=(%.1f,%.1f)",
             world.width, world.height, camera.target.x, camera.target.y);
}

GameScreen
UpdateGame(void)
{
    camera.offset = 
    (Vector2)
    { 
        GetScreenWidth()  * 0.5f,
        GetScreenHeight() * 0.5f
    };

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorldBefore = GetScreenToWorld2D(mouseScreen, camera);

        camera.zoom += wheel * 0.1f; 
        camera.zoom = Clamp(camera.zoom, 0.5f, 2.0f);

        Vector2 mouseWorldAfter = GetScreenToWorld2D(mouseScreen, camera);

        camera.target.x += (mouseWorldBefore.x - mouseWorldAfter.x);
        camera.target.y += (mouseWorldBefore.y - mouseWorldAfter.y);

    }

    int gx = -1;
    int gy = -1;
    //ScreenToIsoGrid(GetMousePosition(), camera, &gx, &gy);
    bool inMap = PickTileWorld(GetMousePosition(), camera, &gx, &gy);
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

    Vector2 ms = GetMousePosition();
    Vector2 mw = GetScreenToWorld2D(ms, camera);

    Vector2 worldTopLeft = GetScreenToWorld2D((Vector2){0, 0}, camera);
    Vector2 worldBottomRight = 
        GetScreenToWorld2D((Vector2){GetScreenWidth(), GetScreenHeight()}, camera);

    BeginMode2D(camera);
    {
        g_visibleCount = 0;

        for (uint32_t y = 0; y < world.height; y++) {
            for (uint32_t x = 0; x < world.width; x++) {
                Vector2 pos = IsoToScreen((int)x, (int)y);
                float minX = pos.x - TILE_WIDTH * 0.5f;
                float maxX = pos.x + TILE_WIDTH * 0.5f;
                float minY = pos.y; 
                float maxY = pos.y + TILE_HEIGHT; 

                if (maxX < worldTopLeft.x || minX > worldBottomRight.x) continue;
                if (maxY < worldTopLeft.y || minY > worldBottomRight.y) continue;

                if (g_visibleCount >= MAX_VISIBLE_TILES) break;

                const Tile *t = 
                    WorldGetTileConst(&world, (Position){(int)x, (int)y});

                Color tileColor = t ? TileColorFor(t->type) : COL_TILE;

                bool isHover = hoverInMap && (int)x == hoverX && (int)y == hoverY;
                bool isSel   = ((int)x == selX && (int)y == selY);

                if (isHover) tileColor = COL_TILE_HOVER;
                if (isSel)   tileColor = COL_TILE_SEL;
                if (isSel && isHover) tileColor = COL_TILE_SEL_HOV;

                IsoTileVerts((int)x, (int)y, g_visibleTiles[g_visibleCount].v);
                g_visibleTiles[g_visibleCount].color = tileColor;
                g_visibleTiles[g_visibleCount].gx = (int)x;
                g_visibleTiles[g_visibleCount].gy = (int)y;
                g_visibleCount++;
            }
        }

        rlSetTexture(0);
        rlBegin(RL_TRIANGLES);

        for (int i = 0; i < g_visibleCount; i++) {
            Vector2 *v = g_visibleTiles[i].v;
            Color col = g_visibleTiles[i].color;
            
            rlColor4ub(col.r, col.g, col.b, col.a);
            rlVertex2f(v[0].x, v[0].y);
            rlVertex2f(v[1].x, v[1].y);
            rlVertex2f(v[2].x, v[2].y);

            rlColor4ub(col.r, col.g, col.b, col.a);
            rlVertex2f(v[0].x, v[0].y);
            rlVertex2f(v[2].x, v[2].y);
            rlVertex2f(v[3].x, v[3].y);
        }
        
        rlEnd();

        for (int i = 0; i < g_visibleCount; i++) {
            DrawIsoTileOutlineOnly(
                g_visibleTiles[i].gx,
                g_visibleTiles[i].gy,
                COL_OUTLINE, 1.0f);
        }

        if (hoverInMap) {
            DrawIsoTileOutlineOnly(hoverX, hoverY, COL_TILE_HOVER, 1.0f);
        }
        if (selX >= 0 && selY >= 0) {
            DrawIsoTileOutlineOnly(selX, selY, COL_OUTLINE_SEL, 2.0f);
        }
    }
    EndMode2D();    

    DrawCircleV(GetMousePosition(), 4.0f, GREEN);
    Vector2 dpi = GetWindowScaleDPI();
    DrawText(TextFormat("mouse scr (%.0f,%.0f)  world (%.1f,%.1f)",
         ms.x, ms.y, mw.x, mw.y), 10, 150, 16, YELLOW);
    DrawText(TextFormat("screen %dx%d  render %dx%d  dpi %.2f/%.2f",
         GetScreenWidth(), GetScreenHeight(),
         GetRenderWidth(), GetRenderHeight(), dpi.x, dpi.y), 10, 170, 16, YELLOW);
    DrawText(TextFormat("offset (%.0f,%.0f)", camera.offset.x, camera.offset.y),
         10, 190, 16, YELLOW);

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
