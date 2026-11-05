/* SPDX-License-Identifier: AGPL-3.0-only
 * 5/11/2026 ruzen42 */
#include "game.h"

#include <stdlib.h>
#include <string.h>

static const UnitStats g_unitStats[] = {
    [UNIT_WORKER]   = { 50,  0,  4, 1, 1, true  },
    [UNIT_WIZARD]   = { 65, 80, 14, 2, 1, false },
    [UNIT_WARRIOR]  = {100,  0, 12, 5, 1, false },
    [UNIT_ARCHER]   = { 70,  0, 10, 2, 1, false },
    [UNIT_MAGICIAN] = { 75, 60, 16, 2, 1, false }
};

const UnitStats *
CoreUnitGetStats(UnitType type)
{
    if ((unsigned)type >= sizeof g_unitStats / sizeof g_unitStats[0])
        return NULL;
    return &g_unitStats[type];
}

bool
CoreGameInit(Game *game, uint8_t civilizationCount)
{
    if (!game || civilizationCount == 0) return false;
    memset(game, 0, sizeof *game);
    game->civs = calloc(civilizationCount, sizeof *game->civs);
    if (!game->civs) return false;
    game->civCount = civilizationCount;
    game->nextUnitId = 1;
    game->context.state = STATE_CLIENT_WAIT;
    game->context.activeCiv = 0;
    return true;
}

void
CoreGameDestroy(Game *game)
{
    if (!game) return;
    for (uint8_t i = 0; i < game->civCount; i++)
        free(game->civs[i].units);
    free(game->civs);
    memset(game, 0, sizeof *game);
}

Unit *
CoreGameFindUnit(Game *game, uint32_t unitId, uint8_t *ownerCiv)
{
    if (!game || !game->civs || unitId == 0) return NULL;
    for (uint8_t c = 0; c < game->civCount; c++) {
        Civilization *civ = &game->civs[c];
        for (size_t i = 0; i < civ->unitCount; i++) {
            if (civ->units[i].id == unitId) {
                if (ownerCiv) *ownerCiv = c;
                return &civ->units[i];
            }
        }
    }
    return NULL;
}

const Unit *
CoreGameFindUnitConst(const Game *game, uint32_t unitId, uint8_t *ownerCiv)
{
    if (!game || !game->civs || unitId == 0) return NULL;
    for (uint8_t c = 0; c < game->civCount; c++) {
        const Civilization *civ = &game->civs[c];
        for (size_t i = 0; i < civ->unitCount; i++) {
            if (civ->units[i].id == unitId) {
                if (ownerCiv) *ownerCiv = c;
                return &civ->units[i];
            }
        }
    }
    return NULL;
}

static bool
IsPassable(const World *world, Position pos)
{
    const Tile *tile = WorldGetTileConst(world, pos);
    return tile && tile->type != SEA && tile->type != MOUNTAIN;
}

static Unit *
FindUnitAt(Game *game, Position pos)
{
    for (uint8_t c = 0; c < game->civCount; c++) {
        Civilization *civ = &game->civs[c];
        for (size_t i = 0; i < civ->unitCount; i++)
            if (civ->units[i].pos.x == pos.x && civ->units[i].pos.y == pos.y)
                return &civ->units[i];
    }
    return NULL;
}

bool
CoreGameSpawnUnit(Game *game, const World *world, uint8_t civIndex,
                  UnitType type, Position pos, uint32_t *outUnitId)
{
    if (!game || !game->civs || civIndex >= game->civCount ||
        !CoreUnitGetStats(type) || !IsPassable(world, pos) ||
        FindUnitAt(game, pos) || game->nextUnitId == 0)
        return false;

    Civilization *civ = &game->civs[civIndex];
    if (civ->unitCount == civ->unitCapacity) {
        size_t capacity = civ->unitCapacity ? civ->unitCapacity * 2 : 8;
        if (capacity < civ->unitCapacity || capacity > SIZE_MAX / sizeof(Unit))
            return false;
        Unit *units = realloc(civ->units, capacity * sizeof *units);
        if (!units) return false;
        civ->units = units;
        civ->unitCapacity = capacity;
    }

    const UnitStats *stats = CoreUnitGetStats(type);
    uint32_t id = game->nextUnitId++;
    civ->units[civ->unitCount++] = (Unit){
        .id = id,
        .level = 1,
        .actionsLeft = 1,
        .health = stats->maxHealth,
        .mana = stats->maxMana,
        .type = type,
        .pos = pos
    };
    if (outUnitId) *outUnitId = id;
    return true;
}

bool
CoreGameMoveUnit(Game *game, const World *world, uint8_t civIndex,
                 uint32_t unitId, Position destination)
{
    if (!game || !game->civs || civIndex >= game->civCount ||
        game->context.activeCiv != civIndex || !IsPassable(world, destination) ||
        FindUnitAt(game, destination))
        return false;

    Unit *unit = CoreGameFindUnit(game, unitId, NULL);
    if (!unit || unit->health == 0 || unit->actionsLeft == 0) return false;
    uint8_t owner = 0;
    (void)CoreGameFindUnit(game, unitId, &owner);
    if (owner != civIndex) return false;

    int dx = destination.x - unit->pos.x;
    int dy = destination.y - unit->pos.y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    const UnitStats *stats = CoreUnitGetStats(unit->type);
    if (!stats || dx + dy == 0 || dx + dy > stats->movement) return false;
    unit->pos = destination;
    unit->actionsLeft--;
    return true;
}

bool
CoreGameEndTurn(Game *game, uint8_t civ)
{
    if (!game || !game->civs || civ >= game->civCount ||
        game->context.activeCiv != civ)
        return false;
    game->context.activeCiv = (uint8_t)((civ + 1) % game->civCount);
    Civilization *next = &game->civs[game->context.activeCiv];
    for (size_t i = 0; i < next->unitCount; i++)
        next->units[i].actionsLeft = 1;
    return true;
}

static void
RemoveUnit(Civilization *civ, size_t index)
{
    if (index + 1 < civ->unitCount)
        memmove(&civ->units[index], &civ->units[index + 1],
                (civ->unitCount - index - 1) * sizeof *civ->units);
    civ->unitCount--;
}

bool
CoreGameAttack(Game *game, uint8_t civIndex, uint32_t attackerId,
               uint32_t defenderId, BattleResult *result)
{
    if (!game || !game->civs || civIndex >= game->civCount ||
        attackerId == defenderId)
        return false;

    uint8_t attackerOwner = 0, defenderOwner = 0;
    Unit *attacker = CoreGameFindUnit(game, attackerId, &attackerOwner);
    Unit *defender = CoreGameFindUnit(game, defenderId, &defenderOwner);
    if (game->context.activeCiv != civIndex || !attacker || !defender ||
        attackerOwner != civIndex || attacker->actionsLeft == 0 ||
        defenderOwner == civIndex || attacker->health == 0 || defender->health == 0)
        return false;
    int dx = attacker->pos.x - defender->pos.x;
    int dy = attacker->pos.y - defender->pos.y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx + dy != 1) return false;

    const UnitStats *attackStats = CoreUnitGetStats(attacker->type);
    const UnitStats *defendStats = CoreUnitGetStats(defender->type);
    if (!attackStats || !defendStats) return false;

    uint16_t damage = attackStats->attack + (attacker->level - 1) * 2;
    damage = damage > defendStats->defense ? damage - defendStats->defense : 1;
    BattleResult outcome = {
        .attackerId = attackerId,
        .defenderId = defenderId,
        .damageToDefender = damage
    };
    attacker->actionsLeft--;
    defender->health = damage >= defender->health ? 0 : defender->health - damage;
    outcome.defenderKilled = defender->health == 0;

    if (outcome.defenderKilled) {
        Civilization *defenderCiv = &game->civs[defenderOwner];
        size_t index = (size_t)(defender - defenderCiv->units);
        RemoveUnit(defenderCiv, index);
    } else {
        uint16_t counter = defendStats->attack > attackStats->defense ?
                           defendStats->attack - attackStats->defense : 1;
        outcome.damageToAttacker = counter;
        attacker->health = counter >= attacker->health ? 0 : attacker->health - counter;
        outcome.attackerKilled = attacker->health == 0;
        if (outcome.attackerKilled) {
            Civilization *attackerCiv = &game->civs[attackerOwner];
            size_t index = (size_t)(attacker - attackerCiv->units);
            RemoveUnit(attackerCiv, index);
        }
    }

    if (result) *result = outcome;
    return true;
}
