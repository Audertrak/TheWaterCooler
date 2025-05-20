#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include <stdbool.h>

bool Client_Init(void);
void Client_UpdateAndDraw(GameState *gameState);
void Client_Close(void);
bool Client_ShouldClose(void);

// Add a function to get the currently selected card index (or -1 if none)
// This is more of a client-side UI state.
// int Client_GetSelectedCardIndex(void);

#endif // CLIENT_H
