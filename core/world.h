/* SPDX-License-Identifier: AGPL-3.0-only
 * 01/11/2026 ruzen42 
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/types.h"

typedef struct World {
    uint32_t width;
    uint32_t height;
    Tile *tiles;
} World;

bool WorldCreate(World *world, uint32_t width, uint32_t height);

void WorldDestroy(World *world);

Tile *WorldGetTile(World *world, Position pos);

const Tile *WorldGetTileConst(
    const World *world,
    Position pos
);
