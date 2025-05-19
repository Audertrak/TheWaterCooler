#include "client.h" // Includes server.h transitively for GameState
#include "raylib.h" // For GetFrameTime()
#include "server.h"

int main(void) {
  GameState gameState;

  // Initialize Server (game logic state)
  Server_Init(&gameState);

  // Initialize Client (window, UI, rendering)
  if (!Client_Init()) {
    // Handle client initialization failure (e.g., log error)
    return 1;
  }

  // Main game loop
  while (!Client_ShouldClose()) {
    // Get delta time for frame-independent updates
    float deltaTime = GetFrameTime();

    // Update server state (game logic)
    Server_Update(&gameState, deltaTime);

    // Update client (input, UI logic) and draw based on server state
    Client_UpdateAndDraw(&gameState);
  }

  // Clean up client resources
  Client_Close();

  return 0;
}
