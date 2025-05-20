#include "server.h"
#include <stddef.h> // For NULL
#include <stdio.h>
#include <stdlib.h> // For rand, srand (optional for shuffle later)
#include <string.h> // For strncpy
#include <time.h>   // For time (for srand)

// Helper to create a card (can be expanded)
static Card CreateObjectCard(int id, const char *name, ComponentType objType) {
  Card card;
  card.id = id;
  card.type = CARD_TYPE_OBJECT;
  strncpy(card.name, name, sizeof(card.name) - 1);
  card.name[sizeof(card.name) - 1] = '\0';
  snprintf(card.description, sizeof(card.description), "Places a %s.", name);
  card.objectToPlace = objType;
  return card;
}

// Basic "draw card" function (simplified)
// Returns true if a card was drawn, false if deck is empty
static bool Server_DrawCard_Internal(GameState *gameState) {
  if (gameState == NULL || gameState->handCardCount >= MAX_CARDS_IN_HAND)
    return false;

  // If deck is empty, try to reshuffle discard pile (basic version)
  if (gameState->currentDeckIndex >= gameState->deckCardCount) {
    if (gameState->discardCardCount > 0) {
      // Simple "reshuffle": move discard to deck
      // A real shuffle would be needed for proper randomization
      for (int i = 0; i < gameState->discardCardCount; ++i) {
        gameState->playerDeck[i] = gameState->playerDiscard[i];
      }
      gameState->deckCardCount = gameState->discardCardCount;
      gameState->discardCardCount = 0;
      gameState->currentDeckIndex = 0;
      // TODO: Implement actual shuffle: Server_ShuffleDeck(gameState);
      TraceLog(LOG_INFO, "Deck empty. Reshuffled discard pile into deck.");
    } else {
      TraceLog(LOG_INFO, "Deck and discard pile are empty. Cannot draw.");
      return false; // No cards left anywhere
    }
  }

  // Check again if deck is drawable after potential reshuffle
  if (gameState->currentDeckIndex >= gameState->deckCardCount) {
    TraceLog(LOG_INFO,
             "Deck still empty after attempting reshuffle. Cannot draw.");
    return false;
  }

  gameState->playerHand[gameState->handCardCount] =
      gameState->playerDeck[gameState->currentDeckIndex];
  gameState->handCardCount++;
  gameState->currentDeckIndex++;
  return true;
}

void Server_Init(GameState *gameState) {
  if (gameState == NULL)
    return;

  // Initialize core game logic state
  gameState->componentCount = 0;
  gameState->nextComponentId = 1;
  for (int i = 0; i < MAX_COMPONENTS_ON_GRID; ++i) {
    gameState->componentsOnGrid[i].isActive = false;
    gameState->componentsOnGrid[i].type = COMP_NONE;
  }

  // Initialize player hand, deck, discard
  gameState->handCardCount = 0;
  gameState->deckCardCount = 0;
  gameState->currentDeckIndex = 0;
  gameState->discardCardCount = 0;

  // Seed random number generator (for future shuffling)
  // srand((unsigned int)time(NULL));

  // Define some master card types (could be loaded from data later)
  Card momentarySwitchCard =
      CreateObjectCard(1, "Momentary Switch", COMP_MOMENTARY_SWITCH);
  Card latchingSwitchCard =
      CreateObjectCard(2, "Latching Switch", COMP_LATCHING_SWITCH);
  Card andGateCard = CreateObjectCard(3, "AND Gate", COMP_AND_GATE);
  Card orGateCard = CreateObjectCard(4, "OR Gate", COMP_OR_GATE);

  // Populate the deck (example: 5 of each for a 20 card deck)
  int deckIdx = 0;
  for (int i = 0; i < 5; ++i) {
    if (deckIdx < MAX_CARDS_IN_DECK)
      gameState->playerDeck[deckIdx++] = momentarySwitchCard;
    if (deckIdx < MAX_CARDS_IN_DECK)
      gameState->playerDeck[deckIdx++] = latchingSwitchCard;
    if (deckIdx < MAX_CARDS_IN_DECK)
      gameState->playerDeck[deckIdx++] = andGateCard;
    if (deckIdx < MAX_CARDS_IN_DECK)
      gameState->playerDeck[deckIdx++] = orGateCard;
  }
  gameState->deckCardCount = deckIdx;

  // TODO: Shuffle deck: Server_ShuffleDeck(gameState);
  // For now, cards are in a fixed order.

  // Deal initial hand (e.g., 5 cards)
  for (int i = 0; i < 5; ++i) {
    Server_DrawCard_Internal(gameState);
  }

  // Initialize scenario/goal (placeholder)
  // strncpy(gameState->currentScenarioDescription, "Scenario: Build a Toggle
  // Switch", sizeof(gameState->currentScenarioDescription) - 1);
  // gameState->currentScenarioDescription[sizeof(gameState->currentScenarioDescription)
  // - 1] = '\0';

  // General State
  gameState->score = 0;
  gameState->is_game_over = false;

  TraceLog(LOG_INFO, "Server Initialized. Deck: %d cards, Hand: %d cards.",
           gameState->deckCardCount, gameState->handCardCount);
}

void Server_Update(GameState *gameState, float deltaTime) {
  if (gameState == NULL || gameState->is_game_over)
    return;

  // Actual game logic updates would go here
  // For example, processing component logic if the game is not paused
  // or if it's a continuous simulation aspect.

  // For now, just a placeholder for score.
  // gameState->score += (int)(10 * deltaTime); // Example: score increases over
  // time
}
