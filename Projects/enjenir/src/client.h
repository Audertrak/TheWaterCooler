#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include <stdbool.h>

bool Client_Init(void);
void Client_UpdateAndDraw(GameState *gameState);
void Client_Close(void);
bool Client_ShouldClose(void);

typedef enum ClientScreen {
  CLIENT_SCREEN_LOADING,
  CLIENT_SCREEN_TITLE,
  CLIENT_SCREEN_GAMEPLAY
} ClientScreen;

ClientScreen Client_GetCurrentScreen(void);

#endif // CLIENT_H
