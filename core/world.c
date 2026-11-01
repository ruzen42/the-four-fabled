/* SPDX-License-Identifier: AGPL-3.0-only
 * 01/11/2026 ruzen42
 */

#include "core/world.h"

#include <stdlib.h>
#include <stdint.h>

bool
WorldCreate(World *world, uint32_t width, uint32_t height)
{
    if (world == NULL || width == 0 || height == 0)
        return false;

    if ((size_t)width > SIZE_MAX / (size_t)height)
        return false;

    size_t count = (size_t)width * height;

    if (count > SIZE_MAX / sizeof(Tile))
        return false;

    Tile *tiles = calloc(count, sizeof(Tile));

    if (tiles == NULL)
        return false;

    world->width = width;
    world->height = height;
    world->tiles = tiles;

    return true;
}

void
WorldDestroy(World *world)
{
    if (world == NULL)
        return;

    free(world->tiles);

    world->tiles = NULL;
    world->width = 0;
    world->height = 0;
}

Tile *
WorldGetTile(World *world, Position pos)
{
    if (world == NULL || world->tiles == NULL)
        return NULL;

    if (pos.x < 0 || pos.y < 0 ||
        (uint32_t)pos.x >= world->width ||
        (uint32_t)pos.y >= world->height)
        return NULL;

    return &world->tiles[
        (size_t)pos.y * world->width + (size_t)pos.x
    ];
}

const Tile *
WorldGetTileConst(const World *world, Position pos)
{
    return WorldGetTile((World *)world, pos);
}
