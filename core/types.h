/* SPDX-License-Identifier: AGPL-3.0-only
 * 5/11/2026 ruzen42 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>

// in game grid position
typedef struct Position {
    int x;
    int y;
} Position;

// simple enum, in future replaced by something else
typedef enum TileType {
    SEA = 0,
    GRASS,
    FOREST,
    MOUNTAIN,
} TileType;

typedef enum Resource {
    RESOURCE_COPPER = 0,
    RESOURCE_COAL,
    RESOURCE_MEAT,
    RESOURCE_TIN,
    RESOURCE_GOLD,
    RESOURCE_IRON,
    RESOURCE_WOOD,
    RESOURCE_TITAN,
    RESOURCE_MITHRIL,
    RESOURCE_COUNT 
} Resource;

typedef enum UnitType {
    UNIT_WORKER,
    UNIT_WIZARD,
    UNIT_WARRIOR,
    UNIT_ARCHER,
    UNIT_MAGICIAN,
} UnitType;

typedef enum UnitEffect {
    EFFECT_HEAL,
    EFFECT_POISON,
} UnitEffect;

typedef struct Unit {
    uint32_t id;
    uint8_t level;
    uint8_t actionsLeft;
    uint16_t health;
    uint16_t mana;
    UnitType type;
    Position pos;
} Unit;

// wrapper for town 
typedef struct TileResource {
    Resource type;
    uint16_t amount;
} TileResource;

// basic tile type
typedef struct Tile {
    TileType type;
    TileResource *resources; 
    Position pos;
} Tile;

typedef struct Town {
    uint8_t level;
    Position center;
    // Dynamic arrays via stb_ds.h    
    Tile *tiles; 
} Town;

typedef struct Civilization {
    /* 0 - ehh
     * 1
     * 2
     * 3 */
    uint8_t number; 
    
    Town *towns;             // Dynamic Array also
    Unit *units;             // Dynamic Array also
    size_t unitCount;
    size_t unitCapacity;
} Civilization;

typedef enum GameState {
    STATE_PAUSE,
    STATE_CLIENT_WAIT,
    STATE_SERVER_WAIT,
    STATE_END,
} GameState;

typedef struct GameContext {
    GameState state;
    uint8_t activeCiv; 
} GameContext;

typedef struct Game {
    Civilization *civs;
    uint8_t civCount;
    uint32_t nextUnitId;
    GameContext context;
} Game;

