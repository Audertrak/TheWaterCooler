#include "server.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

static bool Server_AttemptDrawAndReshuffle(GameState *gameState) {
  if (gameState == NULL)
    return false;
  if (gameState->currentDeckIndex >= gameState->deckCardCount) {
    if (gameState->discardCardCount > 0) {
      TraceLog(LOG_INFO,
               "SERVER: Deck empty. Moving discard pile (%d cards) to deck.",
               gameState->discardCardCount);
      for (int i = 0; i < gameState->discardCardCount; ++i) {
        gameState->playerDeck[i] = gameState->playerDiscard[i];
      }
      gameState->deckCardCount = gameState->discardCardCount;
      gameState->discardCardCount = 0;
      gameState->currentDeckIndex = 0;
      if (gameState->deckCardCount > 1) {
        for (int i = gameState->deckCardCount - 1; i > 0; i--) {
          int j = rand() % (i + 1);
          Card temp = gameState->playerDeck[i];
          gameState->playerDeck[i] = gameState->playerDeck[j];
          gameState->playerDeck[j] = temp;
        }
        TraceLog(LOG_INFO, "SERVER: Deck reshuffled.");
      }
    } else {
      TraceLog(LOG_INFO,
               "SERVER: Deck and discard pile are empty. Cannot draw.");
      return false;
    }
  }
  if (gameState->currentDeckIndex >= gameState->deckCardCount) {
    TraceLog(
        LOG_INFO,
        "SERVER: Deck still empty after attempting reshuffle. Cannot draw.");
    return false;
  }
  return true;
}

bool Server_PlayerDrawCard(GameState *gameState) {
  if (gameState == NULL)
    return false;
  if (gameState->handCardCount >= MAX_CARDS_IN_HAND) {
    TraceLog(LOG_INFO, "SERVER: Hand is full. Cannot draw card.");
    return false;
  }
  if (!Server_AttemptDrawAndReshuffle(gameState)) {
    return false;
  }
  gameState->playerHand[gameState->handCardCount] =
      gameState->playerDeck[gameState->currentDeckIndex];
  TraceLog(LOG_INFO, "SERVER: Player drew card '%s'. Hand size: %d",
           gameState->playerHand[gameState->handCardCount].name,
           gameState->handCardCount + 1);
  gameState->handCardCount++;
  gameState->currentDeckIndex++;
  return true;
}

bool Server_PlayCardFromHand(GameState *gameState, int handIndex) {
  if (gameState == NULL || handIndex < 0 ||
      handIndex >= gameState->handCardCount) {
    TraceLog(LOG_WARNING, "SERVER: Invalid hand index %d or null gameState.",
             handIndex);
    return false;
  }
  if (gameState->discardCardCount >= MAX_CARDS_IN_DECK) {
    TraceLog(LOG_WARNING, "SERVER: Discard pile is full. Cannot play card.");
    return false;
  }
  Card playedCard = gameState->playerHand[handIndex];
  TraceLog(LOG_INFO, "SERVER: Playing card '%s' from hand index %d.",
           playedCard.name, handIndex);
  gameState->playerDiscard[gameState->discardCardCount] = playedCard;
  gameState->discardCardCount++;
  for (int i = handIndex; i < gameState->handCardCount - 1; ++i) {
    gameState->playerHand[i] = gameState->playerHand[i + 1];
  }
  gameState->handCardCount--;
  return true;
}

void Server_InteractWithComponent(GameState *gameState, int componentId) {
  if (gameState == NULL)
    return;
  for (int i = 0; i < gameState->componentCount; ++i) {
    if (gameState->componentsOnGrid[i].isActive &&
        gameState->componentsOnGrid[i].id == componentId) {
      CircuitComponent *comp = &gameState->componentsOnGrid[i];
      switch (comp->type) {
      case COMP_MOMENTARY_SWITCH:
      case COMP_LATCHING_SWITCH:
        comp->outputState = !comp->outputState;
        TraceLog(LOG_INFO, "SERVER: Toggled component ID %d (%s) to state: %s",
                 comp->id,
                 comp->type == COMP_MOMENTARY_SWITCH ? "MomentarySwitch"
                                                     : "LatchingSwitch",
                 comp->outputState ? "ON" : "OFF");
        break;
      default:
        TraceLog(
            LOG_INFO,
            "SERVER: Component ID %d (type %d) has no defined interaction.",
            comp->id, comp->type);
        break;
      }
      return;
    }
  }
  TraceLog(LOG_WARNING, "SERVER: Component ID %d not found for interaction.",
           componentId);
}

bool Server_CreateConnection(GameState *gameState, int fromComponentId,
                             int toComponentId, int toInputSlot) {
  if (gameState == NULL || gameState->connectionCount >= MAX_CONNECTIONS) {
    TraceLog(LOG_WARNING, "SERVER: Cannot create connection, max connections "
                          "reached or null gamestate.");
    return false;
  }
  if (fromComponentId == toComponentId) {
    TraceLog(LOG_WARNING, "SERVER: Cannot connect component to itself.");
    return false;
  }

  CircuitComponent *toComp = NULL;
  for (int i = 0; i < gameState->componentCount; ++i) {
    if (gameState->componentsOnGrid[i].id == toComponentId &&
        gameState->componentsOnGrid[i].isActive) {
      toComp = &gameState->componentsOnGrid[i];
      break;
    }
  }
  if (toComp == NULL) {
    TraceLog(LOG_WARNING,
             "SERVER: Target component for connection not found (ID: %d).",
             toComponentId);
    return false;
  }
  if (toInputSlot < 0 || toInputSlot >= MAX_INPUTS_PER_LOGIC_GATE) {
    TraceLog(LOG_WARNING, "SERVER: Invalid input slot %d for component ID %d.",
             toInputSlot, toComponentId);
    return false;
  }
  if (toComp->inputComponentIDs[toInputSlot] != -1) {
    TraceLog(LOG_WARNING,
             "SERVER: Input slot %d for component ID %d is already connected.",
             toInputSlot, toComponentId);
    return false;
  }

  Connection *newConnection =
      &gameState->connections[gameState->connectionCount];
  newConnection->fromComponentId = fromComponentId;
  newConnection->toComponentId = toComponentId;
  newConnection->toInputSlot = toInputSlot;
  newConnection->isActive = true;
  gameState->connectionCount++;

  toComp->inputComponentIDs[toInputSlot] = fromComponentId;
  if (toComp->connectedInputCount < MAX_INPUTS_PER_LOGIC_GATE) {
    toComp->connectedInputCount++;
  }

  TraceLog(LOG_INFO,
           "SERVER: Created connection from %d to component %d (slot %d). "
           "Total connections: %d",
           fromComponentId, toComponentId, toInputSlot,
           gameState->connectionCount);
  return true;
}

void Server_Init(GameState *gameState) {
  if (gameState == NULL)
    return;

  srand((unsigned int)time(NULL));

  gameState->componentCount = 0;
  gameState->nextComponentId = 1;
  gameState->connectionCount = 0;

  for (int i = 0; i < MAX_COMPONENTS_ON_GRID; ++i) {
    gameState->componentsOnGrid[i].isActive = false;
    gameState->componentsOnGrid[i].type = COMP_NONE;
    gameState->componentsOnGrid[i].outputState = false;
    gameState->componentsOnGrid[i].defaultOutputState = false;
    gameState->componentsOnGrid[i].connectedInputCount = 0;
    for (int j = 0; j < MAX_INPUTS_PER_LOGIC_GATE; ++j) {
      gameState->componentsOnGrid[i].inputComponentIDs[j] = -1;
      gameState->componentsOnGrid[i].actualInputStates[j] = false;
    }
  }
  for (int i = 0; i < MAX_CONNECTIONS; ++i) {
    gameState->connections[i].isActive = false;
  }

  if (gameState->componentCount < MAX_COMPONENTS_ON_GRID) {
    CircuitComponent *sourceComp =
        &gameState->componentsOnGrid[gameState->componentCount];
    sourceComp->isActive = true;
    sourceComp->id = gameState->nextComponentId++;
    sourceComp->type = COMP_SOURCE;
    sourceComp->gridPosition = (Vector2){-2, 0};
    sourceComp->outputState = true;
    sourceComp->defaultOutputState = true;
    gameState->componentCount++;
    TraceLog(
        LOG_INFO, "SERVER: Initialized SOURCE component ID %d at (%.0f, %.0f)",
        sourceComp->id, sourceComp->gridPosition.x, sourceComp->gridPosition.y);
  }
  if (gameState->componentCount < MAX_COMPONENTS_ON_GRID) {
    CircuitComponent *sinkComp =
        &gameState->componentsOnGrid[gameState->componentCount];
    sinkComp->isActive = true;
    sinkComp->id = gameState->nextComponentId++;
    sinkComp->type = COMP_SINK;
    sinkComp->gridPosition = (Vector2){-2, 2};
    sinkComp->outputState = false;
    sinkComp->defaultOutputState = false;
    gameState->componentCount++;
    TraceLog(LOG_INFO,
             "SERVER: Initialized SINK component ID %d at (%.0f, %.0f)",
             sinkComp->id, sinkComp->gridPosition.x, sinkComp->gridPosition.y);
  }

  gameState->handCardCount = 0;
  gameState->deckCardCount = 0;
  gameState->currentDeckIndex = 0;
  gameState->discardCardCount = 0;

  Card momentarySwitchCard =
      CreateObjectCard(1, "Momentary Switch", COMP_MOMENTARY_SWITCH);
  Card latchingSwitchCard =
      CreateObjectCard(2, "Latching Switch", COMP_LATCHING_SWITCH);
  Card andGateCard = CreateObjectCard(3, "AND Gate", COMP_AND_GATE);
  Card orGateCard = CreateObjectCard(4, "OR Gate", COMP_OR_GATE);

  int deckIdx = 0;
  for (int type = 0; type < 4; ++type) {
    for (int i = 0; i < 7; ++i) {
      if (deckIdx >= MAX_CARDS_IN_DECK)
        break;
      if (type == 0)
        gameState->playerDeck[deckIdx++] = momentarySwitchCard;
      else if (type == 1)
        gameState->playerDeck[deckIdx++] = latchingSwitchCard;
      else if (type == 2)
        gameState->playerDeck[deckIdx++] = andGateCard;
      else if (type == 3)
        gameState->playerDeck[deckIdx++] = orGateCard;
    }
    if (deckIdx >= MAX_CARDS_IN_DECK)
      break;
  }
  gameState->deckCardCount = deckIdx;

  if (gameState->deckCardCount > 1) {
    for (int i = gameState->deckCardCount - 1; i > 0; i--) {
      int j = rand() % (i + 1);
      Card temp = gameState->playerDeck[i];
      gameState->playerDeck[i] = gameState->playerDeck[j];
      gameState->playerDeck[j] = temp;
    }
    TraceLog(LOG_INFO, "SERVER: Initial deck shuffled.");
  }

  for (int i = 0; i < 5; ++i) {
    Server_PlayerDrawCard(gameState);
  }

  gameState->score = 0;
  gameState->is_game_over = false;

  TraceLog(LOG_INFO,
           "SERVER: Initialized. Deck: %d cards (after initial draw), Hand: %d "
           "cards.",
           gameState->deckCardCount - gameState->currentDeckIndex,
           gameState->handCardCount);
}

void Server_Update(GameState *gameState, float deltaTime) {
  if (gameState == NULL || gameState->is_game_over)
    return;

  for (int i = 0; i < gameState->componentCount; ++i) {
    if (!gameState->componentsOnGrid[i].isActive)
      continue;

    CircuitComponent *currentComponent = &gameState->componentsOnGrid[i];

    if (currentComponent->type == COMP_SOURCE) {
      currentComponent->outputState = true;
      continue;
    } else if (currentComponent->type == COMP_SINK) {
      currentComponent->outputState = false;
      continue;
    }

    if (currentComponent->type == COMP_AND_GATE ||
        currentComponent->type == COMP_OR_GATE) {
      for (int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE; ++k) {
        currentComponent->actualInputStates[k] = false; // Default to false
        if (currentComponent->inputComponentIDs[k] != -1) {
          for (int j = 0; j < gameState->componentCount; ++j) {
            if (gameState->componentsOnGrid[j].isActive &&
                gameState->componentsOnGrid[j].id ==
                    currentComponent->inputComponentIDs[k]) {
              currentComponent->actualInputStates[k] =
                  gameState->componentsOnGrid[j].outputState;
              break;
            }
          }
        }
      }

      currentComponent->connectedInputCount = 0;
      for (int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE; ++k) {
        if (currentComponent->inputComponentIDs[k] != -1) {
          currentComponent->connectedInputCount++;
        }
      }

      if (currentComponent->type == COMP_AND_GATE) {
        if (currentComponent->connectedInputCount < MAX_INPUTS_PER_LOGIC_GATE) {
          currentComponent->outputState = false;
        } else {
          currentComponent->outputState =
              currentComponent->actualInputStates[0] &&
              currentComponent->actualInputStates[1];
        }
      } else if (currentComponent->type == COMP_OR_GATE) {
        if (currentComponent->connectedInputCount == 0) {
          currentComponent->outputState = false;
        } else {
          currentComponent->outputState = false;
          for (int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE;
               ++k) { // Check all potential input slots
            if (currentComponent->inputComponentIDs[k] != -1 &&
                currentComponent->actualInputStates[k]) {
              currentComponent->outputState = true;
              break;
            }
          }
        }
      }
    } else if (currentComponent->type == COMP_MOMENTARY_SWITCH) {
      STUB("Momentary switch logic (reverting to defaultOutputState) to be "
           "implemented");
    }
  }
}
