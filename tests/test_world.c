/* SPDX-License-Identifier: AGPL-3.0-only
 * 01/11/2026 ruzen42
 */

#include <assert.h>
#include <stdio.h>

#include "core/world.h"

int
main(void)
{
    World world = {0};

    assert(WorldCreate(&world, 100, 100));

    assert(world.width == 100);
    assert(world.height == 100);

    Position pos = {10, 20};

    Tile *tile = WorldGetTile(&world, pos);

    assert(tile != NULL);

    tile->type = TILE_FOREST;

    assert(WorldGetTile(&world, pos)->type == TILE_FOREST);

    Position invalid = {-1, 0};

    assert(WorldGetTile(&world, invalid) == NULL);

    WorldDestroy(&world);

    assert(world.tiles == NULL);

    puts("World tests passed!");

    return 0;
}
