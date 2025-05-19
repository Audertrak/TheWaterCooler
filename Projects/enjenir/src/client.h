#ifndef CLIENT_H
#define CLIENT_H

#include "server.h" // Client needs to know about GameState
#include <stdbool.h>

bool Client_Init(void);
void Client_UpdateAndDraw(
    GameState *gameState); // Combines update and draw for simplicity
void Client_Close(void);
bool Client_ShouldClose(void);

#endif // CLIENT_H
