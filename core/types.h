/* SPDX-License-Identifier: AGPL-3.0-only
 * 01/11/2026 ruzen42 */
#pragma once
#include <stdint.h>

typedef struct Position {
    int x;
    int y;
} Position;

typedef enum TileType {
    SEA = 0,
    GRASS,
    FOREST,
    MOUNTAIN,
} TileType;

typedef struct Tile {
    Position pos;
    TileType type;
} Tile;
