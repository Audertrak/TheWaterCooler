#ifndef SERVER_H
#define SERVER_H

#include "raylib.h" // For Vector2, Color, TraceLog etc.
#include <stdbool.h>

// In server.h
#if defined(PLATFORM_WEB) // For full Raylib web app build
#include "raylib.h"
// Potentially include raymath.h if server logic uses it directly
// #include "raymath.h"
#elif defined(TOOL_WASM_BUILD) // For headless server logic WASM
typedef struct Vector2 {
  float x;
  float y;
} Vector2;
#define TraceLog(logLevel, ...)
#define LOG_INFO 0
#define LOG_WARNING 1
// Define other minimal types/stubs if server.c needs them without full Raylib
#else // For native desktop build
#include "raylib.h"
#include "raymath.h"
#endif

#define MAX_COMPONENTS_ON_GRID                                                 \
  100 ///< Maximum number of components that can be placed on the grid.
#define MAX_CARDS_IN_HAND 10 ///< Maximum number of cards a player can hold.
#define MAX_CARDS_IN_DECK 60 ///< Maximum number of cards in a deck.
#define MAX_INPUTS_PER_LOGIC_GATE 2 ///< Max inputs for simple gates like AND/OR
#define MAX_OUTPUTS_PER_COMPONENT 1 // Most components have one output for now
#define MAX_CONNECTIONS                                                        \
  MAX_COMPONENTS_ON_GRID *MAX_INPUTS_PER_LOGIC_GATE // Theoretical max

// --- Component Definitions ---

/**
 * @brief Defines the types of circuit components available in the game.
 */
typedef enum ComponentType {
  COMP_NONE = 0,         ///< No component / empty slot.
  COMP_MOMENTARY_SWITCH, ///< A switch that is active only while interacted
                         ///< with.
  COMP_LATCHING_SWITCH,  ///< A switch that toggles its state on interaction.
  COMP_AND_GATE,         ///< A logical AND gate.
  COMP_OR_GATE,          ///< A logical OR gate.
  COMP_SOURCE,           ///< Global source, always outputs true.
  COMP_SINK,             ///< Global sink, always outputs false.
  COMP_TYPE_COUNT        ///< Total number of defined component types.
} ComponentType;

/**
 * @brief Represents a single circuit component placed on the game grid.
 */
typedef struct CircuitComponent {
  ComponentType type;      ///< The type of this component.
  Vector2 gridPosition;    ///< Logical (column, row) position on the grid.
  bool outputState;        ///< Current boolean output state of the component.
  bool defaultOutputState; ///< Default output state, primarily for switches.
  bool isActive; ///< True if this component slot is in use on the grid.
  int id;        ///< Unique identifier for this component instance.
  int inputComponentIDs[MAX_INPUTS_PER_LOGIC_GATE];  ///< IDs of components
                                                     ///< providing input. -1 if
                                                     ///< not connected.
  bool actualInputStates[MAX_INPUTS_PER_LOGIC_GATE]; ///< The actual boolean
                                                     ///< state received from
                                                     ///< inputComponentIDs.
  int connectedInputCount;
} CircuitComponent;

/**
 * @brief Represents a connection between two components.
 */
typedef struct Connection {
  int fromComponentId; ///< ID of the component outputting the signal.
  int toComponentId;   ///< ID of the component receiving the signal.
  int toInputSlot;     ///< Which input slot on the toComponent (0, 1, etc.).
  bool isActive;       ///< Is this connection slot in use?
} Connection;

// --- Card Definitions ---

/**
 * @brief Defines the general types of cards in the game.
 */
typedef enum CardType {
  CARD_TYPE_OBJECT = 0,     ///< A card that places a CircuitComponent.
  CARD_TYPE_ACTION,         ///< A card that performs an immediate action.
  CARD_TYPE_EFFECT,         ///< A card that applies a lasting effect.
  CARD_TYPE_DECK_MANAGEMENT ///< A card that manipulates the deck or hand.
} CardType;

/**
 * @brief Represents a single card definition.
 */
typedef struct Card {
  CardType type;         ///< The general type of this card.
  char name[64];         ///< Display name of the card.
  char description[128]; ///< Flavor text or rules text for the card.
  ComponentType
      objectToPlace; ///< If CARD_TYPE_OBJECT, the ComponentType it places.
  int id;            ///< Unique identifier for this card definition.
} Card;

/**
 * @brief Holds the entire state of the game logic.
 * This structure is managed by the "server" module.
 */
typedef struct GameState {
  CircuitComponent
      componentsOnGrid[MAX_COMPONENTS_ON_GRID]; ///< Array of all components on
                                                ///< the grid.
  int componentCount;  ///< Number of active components currently on the grid.
  int nextComponentId; ///< Counter for assigning unique IDs to new components.

  Connection connections[MAX_CONNECTIONS]; ///< Array of all connections.
  int connectionCount;                     ///< Number of active connections.

  Card playerHand[MAX_CARDS_IN_HAND]; ///< Cards currently in the player's hand.
  int handCardCount;                  ///< Number of cards in the player's hand.

  Card playerDeck[MAX_CARDS_IN_DECK]; ///< Cards currently in the player's draw
                                      ///< pile.
  int deckCardCount;    ///< Total number of cards currently in the draw pile.
  int currentDeckIndex; ///< Index of the next card to be drawn from playerDeck.

  Card
      playerDiscard[MAX_CARDS_IN_DECK]; ///< Cards in the player's discard pile.
  int discardCardCount; ///< Number of cards in the discard pile.

  int score;         ///< Player's current score (example field).
  bool is_game_over; ///< Flag indicating if the game has ended.
} GameState;

/**
 * @brief Initializes the game state to its starting conditions.
 * @param gameState Pointer to the GameState struct to be initialized.
 */
void Server_Init(GameState *gameState);

/**
 * @brief Updates the game state based on elapsed time and internal logic.
 * @param gameState Pointer to the GameState struct to be updated.
 * @param deltaTime Time elapsed since the last frame, in seconds.
 */
void Server_Update(GameState *gameState, float deltaTime);

/**
 * @brief Processes a card played from the player's hand.
 * Moves the card to the discard pile and updates hand/deck counts.
 * @param gameState Pointer to the GameState struct.
 * @param handIndex The index of the card in the player's hand to be played.
 * @return True if the card was successfully played, false otherwise.
 */
bool Server_PlayCardFromHand(GameState *gameState, int handIndex);

/**
 * @brief Handles player interaction with a component on the grid.
 * For example, toggling a switch.
 * @param gameState Pointer to the GameState struct.
 * @param componentId The unique ID of the component to interact with.
 */
void Server_InteractWithComponent(GameState *gameState, int componentId);

/**
 * @brief Allows the player to attempt to draw a card from their deck.
 * If the deck is empty, it will attempt to reshuffle the discard pile.
 * @param gameState Pointer to the GameState struct.
 * @return True if a card was successfully drawn into the hand, false otherwise.
 */
bool Server_PlayerDrawCard(GameState *gameState);

/**
 * @brief Attempts to create a connection between two components.
 * @param gameState Pointer to the GameState.
 * @param fromComponentId ID of the source component.
 * @param toComponentId ID of the target component.
 * @param toInputSlot The input slot on the target component to connect to.
 * @return True if the connection was successfully made, false otherwise.
 */
bool Server_CreateConnection(GameState *gameState, int fromComponentId,
                             int toComponentId, int toInputSlot);

#endif // SERVER_H
