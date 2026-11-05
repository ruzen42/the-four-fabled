/* SPDX-License-Identifier: AGPL-3.0-only
 * 5/11/2026 ruzen42 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <core/net/net.h> /* for NET_MAX_PACKET */ 

#define PROTO_VERSION 4
#define CH_CONTROL 0   
#define CH_SNAPSHOT 1 
#define CH_EVENTS 2

typedef enum MsgType {
    MSG_NONE  = 0,
    MSG_HELLO,
    MSG_HELLO_ACK,
    MSG_BYE,
    MSG_PING,
    MSG_PONG,
    MSG_WORLD_SNAPSHOT,
    MSG_UNIT_COMMAND,
    MSG_UNIT_SNAPSHOT,
    MSG_LOBBY_STATE,
    MSG_LOBBY_START,
    MSG_COUNT
} MsgType;

typedef struct MsgHeader {
    uint16_t type;
    uint16_t size;
} MsgHeader;

#define MSG_MAX_PAYLOAD ((size_t)(NET_MAX_PACKET - sizeof(MsgHeader)))

typedef struct MsgHello {
    MsgHeader header;
    uint32_t  protocolVersion;
    uint32_t  clientVersion;
    char      name[32];
} MsgHello;

typedef struct MsgHelloAck {
    MsgHeader header;         /* type=MSG_HELLO_ACK */
    uint8_t   accepted;
    uint32_t  assignedId;    
} MsgHelloAck;

typedef struct MsgPing { 
    MsgHeader header; 
    uint32_t clientTime; 
} MsgPing;

typedef struct MsgPong { 
    MsgHeader header; 
    uint32_t clientTime; 
} MsgPong;

typedef struct MsgWorldSnapshotHeader {
    MsgHeader header;
    uint32_t width;
    uint32_t height;
} MsgWorldSnapshotHeader;

typedef enum UnitCommandKind {
    UNIT_COMMAND_MOVE = 1,
    UNIT_COMMAND_ATTACK = 2,
    UNIT_COMMAND_END_TURN = 3
} UnitCommandKind;

typedef struct MsgUnitCommand {
    MsgHeader header;
    uint8_t command;
    uint32_t unitId;
    uint32_t targetUnitId;
    int32_t targetX;
    int32_t targetY;
} MsgUnitCommand;

typedef struct MsgUnitSnapshotHeader {
    MsgHeader header;
    uint16_t unitCount;
    uint8_t activeCiv;
} MsgUnitSnapshotHeader;

typedef struct MsgUnitState {
    uint32_t id;
    uint8_t ownerCiv;
    uint8_t type;
    uint8_t level;
    uint16_t health;
    uint16_t mana;
    int32_t x;
    int32_t y;
} MsgUnitState;

typedef struct MsgLobbyState {
    MsgHeader header;
    uint8_t playerMask;
    uint8_t hostId;
    uint8_t requiredPlayers;
    uint8_t started;
} MsgLobbyState;

typedef struct MsgLobbyStart {
    MsgHeader header;
} MsgLobbyStart;
