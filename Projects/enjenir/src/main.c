#include "client.h"
#include "raylib.h"
#include "server.h"
#include "client.h"

int main(void) {
  SimulatorState simulatorState;

  Server_Init(&simulatorState);

  if (!Client_Init()) {
    return 1;
  }

  while (!Client_ShouldClose()) {
    float deltaTime = GetFrameTime();

    if (Client_GetCurrentScreen() == CLIENT_SCREEN_SIMULATION) {
      Server_Update(&simulatorState, deltaTime);
    }

    Client_UpdateAndDraw(&simulatorState);
  }

  Client_Close();

  return 0;
}
