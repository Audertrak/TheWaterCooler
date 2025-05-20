#ifndef SERVER_H
#define SERVER_H

#include "raylib.h" // For Vector2 (used in CircuitComponent)
#include <stdbool.h>

#define MAX_COMPONENTS_ON_GRID 100
#define MAX_CARDS_IN_HAND 10
#define MAX_CARDS_IN_DECK 60 // Example size

// --- Component Definitions ---
typedef enum ComponentType {
  COMP_NONE = 0,
  COMP_MOMENTARY_SWITCH,
  COMP_LATCHING_SWITCH,
  COMP_AND_GATE,
  COMP_OR_GATE,
  // COMP_WIRE, // Wires might be handled differently (as connections)
  // Add more types as per your GDD
  COMP_TYPE_COUNT
} ComponentType;

typedef struct CircuitComponent {
  ComponentType type;
  Vector2 gridPosition; // Position on the logical grid (e.g., {col, row})
  bool outputState;
  bool defaultOutputState; // For switches
  // Input states/connections will be more complex, TBD
  // int inputCount;
  // bool inputStates[MAX_INPUTS_PER_COMPONENT];
  // int connectedInputs[MAX_INPUTS_PER_COMPONENT]; // IDs of components
  // connected to inputs
  bool isActive; // Is this component slot in use?
  int id;        // Unique ID for this component instance
} CircuitComponent;

// --- Card Definitions ---
typedef enum CardType {
  CARD_TYPE_OBJECT = 0, // Represents a placeable CircuitComponent
  CARD_TYPE_ACTION,
  CARD_TYPE_EFFECT,
  CARD_TYPE_DECK_MANAGEMENT
} CardType;

typedef struct Card {
  CardType type;
  char name[64];
  char description[128];
  ComponentType objectToPlace; // If CARD_TYPE_OBJECT
  // Add other card-specific data, e.g., action_id, effect_id
  int id; // Unique card definition ID
} Card;

// Represents the overall game state managed by the server
typedef struct GameState {
  // Core Game Logic
  CircuitComponent componentsOnGrid[MAX_COMPONENTS_ON_GRID];
  int componentCount; // Number of active components
  int nextComponentId;

  // Player Hand & Deck
  Card playerHand[MAX_CARDS_IN_HAND];
  int handCardCount;

  Card playerDeck[MAX_CARDS_IN_DECK];
  int deckCardCount;
  int currentDeckIndex; // Points to the next card to draw

  Card playerDiscard[MAX_CARDS_IN_DECK];
  int discardCardCount;

  // Scenario / Goal
  // Example: char currentScenarioDescription[256];
  // Example: bool targetStates[MAX_TARGET_OUTPUTS];

  // General State
  int score;
  bool is_game_over;
  // Add more game-specific state fields here
} GameState;

void Server_Init(GameState *gameState);
void Server_Update(GameState *gameState, float deltaTime);
// void Server_ProcessInput(GameState *gameState, ClientInput input);
// const char* Server_GetUIData(const GameState *gameState);

// Card/Deck Management (Example functions, to be implemented in server.c)
// void Server_ShuffleDeck(GameState *gameState);
// bool Server_DrawCard(GameState *gameState);
// void Server_PlayCard(GameState *gameState, int handIndex, Vector2
// gridTarget);

#endif // SERVER_H
