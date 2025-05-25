#include "client.h"
#include "raylib.h"
#include "server.h"

int main(void) {
  GameState gameState;

  Server_Init(&gameState);

  if (!Client_Init()) {
    return 1;
  }

  while (!Client_ShouldClose()) {
    float deltaTime = GetFrameTime();

    if (Client_GetCurrentScreen() == CLIENT_SCREEN_GAMEPLAY) {
      Server_Update(&gameState, deltaTime);
    }

    Client_UpdateAndDraw(&gameState);
  }

  Client_Close();

  return 0;
}
