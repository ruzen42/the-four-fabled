/* SPDX-License-Identifier: AGPL-3.0-only */
#pragma once
#include <stdint.h>
#include <stdbool.h>

// initialize and connect the server
bool ClientNetConnect(const char *addr, uint16_t port);

// poll incoming ENet packets and process events (call every frame)
void ClientNetPoll(void);

// disconnect and clean up network resources
void ClientNetDisconnect(void);

// check if the client as completed the handshake and is connect
bool ClientIsConnected(void);
