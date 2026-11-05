/* SPDX-License-Identifier: AGPL-3.0-only
 * 5/11/2026 ruzen42 */
#pragma once

#include <stdbool.h>
#include "types.h"
#include "world.h"

typedef struct UnitStats {
    uint16_t maxHealth;
    uint16_t maxMana;
    uint8_t attack;
    uint8_t defense;
    uint8_t movement;
    bool canCapture;
} UnitStats;

typedef struct BattleResult {
    uint32_t attackerId;
    uint32_t defenderId;
    uint16_t damageToDefender;
    uint16_t damageToAttacker;
    bool defenderKilled;
    bool attackerKilled;
} BattleResult;

bool CoreGameInit(Game *game, uint8_t civilizationCount);
void CoreGameDestroy(Game *game);

const UnitStats *CoreUnitGetStats(UnitType type);
Unit *CoreGameFindUnit(Game *game, uint32_t unitId, uint8_t *ownerCiv);
const Unit *CoreGameFindUnitConst(
    const Game *game, uint32_t unitId, uint8_t *ownerCiv);

bool CoreGameSpawnUnit(
    Game *game, const World *world, uint8_t civ, UnitType type,
    Position pos, uint32_t *outUnitId);
bool CoreGameMoveUnit(
    Game *game, const World *world, uint8_t civ, uint32_t unitId,
    Position destination);
bool CoreGameEndTurn(Game *game, uint8_t civ);
bool CoreGameAttack(
    Game *game, uint8_t civ, uint32_t attackerId, uint32_t defenderId,
    BattleResult *result);
