#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>

// Represents the overall game state managed by the server
typedef struct GameState {
  int score; // Example field
  bool is_game_over;
  // Add more game-specific state fields here
} GameState;

void Server_Init(GameState *gameState);
void Server_Update(GameState *gameState, float deltaTime);
// void Server_ProcessInput(GameState *gameState, ClientInput input); // Future
// const char* Server_GetUIData(const GameState *gameState); // Future: for
// serialization

#endif // SERVER_H
