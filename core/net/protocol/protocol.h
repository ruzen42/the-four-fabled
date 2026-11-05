/* SPDX-License-Identifier: AGPL-3.0-only
 * 4/11/2026 ruzen42 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <core/net/net.h> /* for NET_MAX_PACKET */ 

#define PROTO_VERSION 2
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
