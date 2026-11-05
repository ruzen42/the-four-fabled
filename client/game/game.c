/* SPDX-License-Identifier: AGPL-3.0-only 
 * 5/11/2026 ruzen42 */

#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdlib.h>
#include <rlgl.h>

#include "log.h"
#include "game/game.h"
#include "game/colors.h"
#include "core/world.h"
#include "net.h"

// isometry params
#define TILE_WIDTH  128
#define TILE_HEIGHT 64

// client 
static World    world;          
static Camera2D camera;
static MsgUnitState *g_units;
static size_t g_unitCount;
static uint32_t selectedUnitId;
static uint8_t g_activeCiv;
static Texture2D g_tilesTex;

static int  selX = -1;
static int  selY = -1;

static int  hoverX = 0;
static int  hoverY = 0;
static bool hoverInMap = false;

static int  prevGridX = -999;
static int  prevGridY = -999;

typedef struct {
    Vector2 v[4];
    int gx, gy;
} VisibleTileData;

#define MAX_VISIBLE_TILES 4096 

static VisibleTileData g_visibleTiles[MAX_VISIBLE_TILES];
static int g_visibleCount = 0;

static const MsgUnitState *
FindUnitAt(int x, int y)
{
    for (size_t i = 0; i < g_unitCount; i++)
        if (g_units[i].x == x && g_units[i].y == y) return &g_units[i];
    return NULL;
}

static Rectangle
TileAtlasSrc(TileType type)
{
    int col = 0, row = 0;
    switch (type) {
        case GRASS:    col = 0; row = 0; break;
        case FOREST:   col = 0; row = 1; break; 
        case SEA:      col = 0; row = 2; break;
        case MOUNTAIN: col = 0; row = 3; break;  
        default:       col = 0; row = 0; break;
    }
    return (Rectangle){
        (float)(col * TILE_WIDTH),
        (float)(row * TILE_HEIGHT),
        (float)TILE_WIDTH,
        (float)TILE_HEIGHT
    };
}

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

void
GameReset(void)
{
    WorldDestroy(&world);
    free(g_units);
    g_units = NULL;
    g_unitCount = 0;
    selectedUnitId = 0;
    g_activeCiv = 0;

    selX = selY = -1;
    hoverX = hoverY = 0;
    hoverInMap = false;
    prevGridX = prevGridY = -999;

    camera.target   = (Vector2){ 0.0f, 0.0f };

    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    if (g_tilesTex.id) UnloadTexture(g_tilesTex);
    g_tilesTex = LoadTexture("content/sprites/tiles.png");
    SetTextureFilter(g_tilesTex, TEXTURE_FILTER_POINT);
}

bool
GameSetWorldSnapshot(uint32_t width, uint32_t height, const uint8_t *tileTypes)
{
    if (!tileTypes || width == 0 || height == 0 ||
        (size_t)width > SIZE_MAX / (size_t)height)
        return false;

    size_t count = (size_t)width * height;
    World received = { 0 };
    if (!WorldCreate(&received, width, height)) return false;

    for (size_t i = 0; i < count; i++) {
        if (tileTypes[i] > MOUNTAIN) {
            WorldDestroy(&received);
            return false;
        }
        received.tiles[i].pos = (Position){ (int)(i % width), (int)(i / width) };
        received.tiles[i].type = (TileType)tileTypes[i];
    }

    WorldDestroy(&world);
    world = received;
    selX = selY = -1;
    hoverInMap = false;
    float cx = ((float)width - 1.0f) * 0.5f;
    float cy = ((float)height - 1.0f) * 0.5f;
    camera.target = (Vector2){
        (cx - cy) * (TILE_WIDTH / 2.0f),
        (cx + cy) * (TILE_HEIGHT / 2.0f)
    };
    return true;
}

bool
GameHasWorld(void)
{
    return world.tiles != NULL;
}

bool
GameSetUnitSnapshot(const MsgUnitState *units, size_t unitCount,
                    uint8_t activeCiv)
{
    if (activeCiv >= 4 || (unitCount && !units) ||
        unitCount > NET_MAX_PACKET / sizeof(MsgUnitState))
        return false;
    MsgUnitState *copy = unitCount ? malloc(unitCount * sizeof *copy) : NULL;
    if (unitCount && !copy) return false;
    for (size_t i = 0; i < unitCount; i++) {
        if (!units[i].id || units[i].ownerCiv >= 4 || units[i].type > UNIT_MAGICIAN ||
            units[i].health == 0 || units[i].x < 0 || units[i].y < 0 ||
            (uint32_t)units[i].x >= world.width ||
            (uint32_t)units[i].y >= world.height) {
            free(copy);
            return false;
        }
        copy[i] = units[i];
    }
    free(g_units);
    g_units = copy;
    g_unitCount = unitCount;
    g_activeCiv = activeCiv;
    bool selectedFound = false;
    for (size_t i = 0; i < g_unitCount; i++)
        if (g_units[i].id == selectedUnitId) selectedFound = true;
    if (!selectedFound) selectedUnitId = 0;
    return true;
}

void
GameShutdown(void)
{
    WorldDestroy(&world);
    free(g_units);
    g_units = NULL;
    g_unitCount = 0;
    UnloadTexture(g_tilesTex);
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
        camera.zoom = Clamp(camera.zoom, 0.7f, 2.0f);

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
        prevGridX = gx;
        prevGridY = gy;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (inMap) {
            selX = gx;
            selY = gy;
            const MsgUnitState *unit = FindUnitAt(gx, gy);
            uint32_t clientId = ClientNetAssignedId();
            if (unit && clientId && unit->ownerCiv == clientId - 1) {
                selectedUnitId = unit->id;
            } else if (selectedUnitId) {
                ClientNetMoveUnit(selectedUnitId, (Position){ gx, gy });
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && inMap && selectedUnitId) {
        const MsgUnitState *target = FindUnitAt(gx, gy);
        uint32_t clientId = ClientNetAssignedId();
        if (target && clientId && target->ownerCiv != clientId - 1)
            ClientNetAttackUnit(selectedUnitId, target->id);
    }

    uint32_t clientId = ClientNetAssignedId();
    if (IsKeyPressed(KEY_ENTER) && clientId && clientId - 1 == g_activeCiv)
        ClientNetEndTurn();

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (selX >= 0 || selectedUnitId) {
            selX = selY = -1;
            selectedUnitId = 0;
        }
    }

    return SCREEN_GAME;
}

static void
DrawIsoTileTextured(Texture2D atlas, Rectangle src, const Vector2 v[4])
{
    if (atlas.id == 0 || src.width <= 0 || src.height <= 0) return;

    const float side = TILE_WIDTH / 1.41421356f;
    Rectangle dst = { 0.0f, 0.0f, side, side };

    rlPushMatrix();
    rlTranslatef(v[0].x, v[0].y, 0.0f);
    rlScalef(1.0f, 0.5f, 1.0f);
    DrawTexturePro(atlas, src, dst, (Vector2){ 0.0f, 0.0f }, 45.0f, WHITE);
    rlPopMatrix();
}

static Color
TileColorFor(TileType type)
{
    switch (type) {
        case SEA:      return (Color){  69, 133, 136, 255 };
        case GRASS:    return (Color){ 167, 192, 128, 255 };
        case FOREST:   return (Color){  79, 121,  66, 255 };
        case MOUNTAIN: return (Color){ 133, 146, 137, 255 };
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

    if (!world.tiles) {
        DrawText("Waiting for server world", 24, 24, 24, COL_TEXT);
        return;
    }

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

                IsoTileVerts((int)x, (int)y, g_visibleTiles[g_visibleCount].v);
                g_visibleTiles[g_visibleCount].gx = (int)x;
                g_visibleTiles[g_visibleCount].gy = (int)y;
                g_visibleCount++;
            }
        }

        for (int i = 0; i < g_visibleCount; i++) {
            Vector2 *v = g_visibleTiles[i].v;
            int gx = g_visibleTiles[i].gx;
            int gy = g_visibleTiles[i].gy;
            const Tile *tile = WorldGetTileConst(&world, (Position){ gx, gy });
            /* Fill the 2:1 diamond directly so every terrain tile is visible */
            TileType type = tile ? tile->type : SEA;

            /* follback */
            Color base = TileColorFor(type);
            DrawTriangle(v[0], v[3], v[2], base);
            DrawTriangle(v[0], v[2], v[1], base);
            DrawIsoTileTextured(g_tilesTex, TileAtlasSrc(type), v);

            /* Hover and selection remain color overlays above the terrain art */
            bool isHover = hoverInMap && gx == hoverX && gy == hoverY;
            bool isSel = gx == selX && gy == selY;
            if (isHover || isSel) {
                Color overlay = isHover && isSel ? COL_TILE_SEL_HOV :
                                isHover ? COL_TILE_HOVER : COL_TILE_SEL;
                overlay.a = 112;
                DrawTriangle(v[0], v[3], v[2], overlay);
                DrawTriangle(v[0], v[2], v[1], overlay);
            }
        }

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

        for (size_t i = 0; i < g_unitCount; i++) {
            const MsgUnitState *unit = &g_units[i];
            Vector2 pos = IsoToScreen((float)unit->x, (float)unit->y);
            pos.y += TILE_HEIGHT * 0.5f;
            static const Color civColors[] = { ORANGE, SKYBLUE, LIME, VIOLET };
            Color color = civColors[unit->ownerCiv];
            DrawCircleV(pos, 11.0f, color);
            DrawCircleLines((int)pos.x, (int)pos.y, 11.0f,
                unit->id == selectedUnitId ? WHITE : COL_BG);
            DrawText(TextFormat("%u", (unsigned)unit->health), (int)pos.x - 10,
                (int)pos.y - 5, 10, BLACK);
        }
    }
    EndMode2D();    

    DrawCircleV(GetMousePosition(), 4.0f, GREEN);
    DrawText("Grid position", 10, 10, 20, COL_TEXT);

    if (selX >= 0 && selY >= 0) {
        DrawText("Tile selected", 10, 40, 20, COL_TILE_SEL);
    } else {
        DrawText("Click to select tile", 10, 40, 20, COL_TEXT_DIM);
    }

    DrawText("LMB: select/move   RMB: attack   Enter: end turn   MMB: pan",
             10, 70, 14, COL_TEXT_DIM);
    DrawText(TextFormat("Active civilization: %u", (unsigned)g_activeCiv + 1),
             10, 90, 16, COL_TEXT);
    DrawFPS(10, 115);
}
