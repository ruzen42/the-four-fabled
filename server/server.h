#pragma once

#include <enet6/enet.h>
#include "core/types.h" 

typedef struct ServerClient {
    uint8_t civ_number;      
    ENetPeer *peer;
} ServerClient;

typedef struct GameServer {
    ENetHost *host;
    GameContext context;  
    
    Civilization *civs;  
    ServerClient *clients;   
} GameServer;
