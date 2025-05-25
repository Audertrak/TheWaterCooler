#include "server.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined( PLATFORM_WEB )
  #include "raylib.h"
  #include "raymath.h"
#elif defined( TOOL_WASM_BUILD )
static inline Vector2 Vector2Add( Vector2 v1, Vector2 v2 ) {
    return (Vector2) { v1.x + v2.x, v1.y + v2.y };
}

static inline bool Vector2Equals( Vector2 v1, Vector2 v2 ) {
    return ( v1.x == v2.x ) && ( v1.y == v2.y );
}
#else    // Native
  #include "raylib.h"
  #include "raymath.h"
#endif

static unsigned int serverUpdateFrameCounter = 0;

static void         PropagateSignals( GameState *gameState );

static Card         CreateObjectCard( int id, const char *name, ComponentType objType ) {
    Card card;
    card.id   = id;
    card.type = CARD_TYPE_OBJECT;
    strncpy( card.name, name, sizeof( card.name ) - 1 );
    card.name[sizeof( card.name ) - 1] = '\0';
    snprintf( card.description, sizeof( card.description ), "Places a %s.", name );
    card.objectToPlace = objType;
    return card;
}

static bool Server_AttemptDrawAndReshuffle( GameState *gameState ) {
    if ( gameState == NULL ) return false;
    if ( gameState->currentDeckIndex >= gameState->deckCardCount ) {
        if ( gameState->discardCardCount > 0 ) {
            TraceLog(
              LOG_INFO, "SERVER: Deck empty. Moving discard pile (%d cards) to deck.",
              gameState->discardCardCount
            );
            for ( int i = 0; i < gameState->discardCardCount; ++i ) {
                gameState->playerDeck[i] = gameState->playerDiscard[i];
            }
            gameState->deckCardCount    = gameState->discardCardCount;
            gameState->discardCardCount = 0;
            gameState->currentDeckIndex = 0;
            if ( gameState->deckCardCount > 1 ) {
                for ( int i = gameState->deckCardCount - 1; i > 0; i-- ) {
                    int  j                   = rand() % ( i + 1 );
                    Card temp                = gameState->playerDeck[i];
                    gameState->playerDeck[i] = gameState->playerDeck[j];
                    gameState->playerDeck[j] = temp;
                }
                TraceLog( LOG_INFO, "SERVER: Deck reshuffled." );
            }
        } else {
            TraceLog( LOG_INFO, "SERVER: Deck and discard pile are empty. Cannot draw." );
            return false;
        }
    }
    if ( gameState->currentDeckIndex >= gameState->deckCardCount ) {
        TraceLog( LOG_INFO, "SERVER: Deck still empty after attempting reshuffle. Cannot draw." );
        return false;
    }
    return true;
}

bool Server_PlayerDrawCard( GameState *gameState ) {
    TraceLog(
      LOG_INFO,
      "SERVER_PLAYER_DRAW_CARD_START: Hand: %d/%d, Deck: %d, Idx: %d, "
      "Discard: %d",
      gameState->handCardCount, MAX_CARDS_IN_HAND, gameState->deckCardCount,
      gameState->currentDeckIndex, gameState->discardCardCount
    );
    if ( gameState == NULL ) return false;
    if ( gameState->handCardCount >= MAX_CARDS_IN_HAND ) {
        TraceLog( LOG_INFO, "SERVER: Hand is full. Cannot draw card." );
        return false;
    }
    if ( !Server_AttemptDrawAndReshuffle( gameState ) ) { return false; }
    gameState->playerHand[gameState->handCardCount] =
      gameState->playerDeck[gameState->currentDeckIndex];
    TraceLog(
      LOG_INFO, "SERVER: Player drew card '%s'. Hand size: %d",
      gameState->playerHand[gameState->handCardCount].name, gameState->handCardCount + 1
    );
    gameState->handCardCount++;
    gameState->currentDeckIndex++;
    return true;
}

bool Server_PlayCardFromHand( GameState *gameState, int handIndex ) {
    if ( gameState == NULL || handIndex < 0 || handIndex >= gameState->handCardCount ) {
        TraceLog( LOG_WARNING, "SERVER: Invalid hand index %d or null gameState.", handIndex );
        return false;
    }
    if ( gameState->discardCardCount >= MAX_CARDS_IN_DECK ) {
        TraceLog( LOG_WARNING, "SERVER: Discard pile is full. Cannot play card." );
        return false;
    }

    Card playedCard = gameState->playerHand[handIndex];
    TraceLog(
      LOG_INFO, "SERVER: Playing card '%s' from hand index %d.", playedCard.name, handIndex
    );

    if ( playedCard.type == CARD_TYPE_ACTION ) {
        if ( Server_ExecuteActionCard( gameState, playedCard.actionType ) ) {
            gameState->playerDiscard[gameState->discardCardCount] = playedCard;
            gameState->discardCardCount++;
            for ( int i = handIndex; i < gameState->handCardCount - 1; ++i ) {
                gameState->playerHand[i] = gameState->playerHand[i + 1];
            }
            gameState->handCardCount--;
            return true;
        }
        return false;
    }

    gameState->playerDiscard[gameState->discardCardCount] = playedCard;
    gameState->discardCardCount++;
    for ( int i = handIndex; i < gameState->handCardCount - 1; ++i ) {
        gameState->playerHand[i] = gameState->playerHand[i + 1];
    }
    gameState->handCardCount--;
    return true;
}

// static void StartComponentTimer(CircuitComponent *comp, float duration) {
//   comp->timer = 0.0f;
//   comp->timerDuration = duration;
//   comp->timerActive = true;
// }
//
// static bool IsComponentTimerExpired(CircuitComponent *comp) {
//   return comp->timerActive && comp->timer >= comp->timerDuration;
// }
//
// static void StopComponentTimer(CircuitComponent *comp) {
//   comp->timerActive = false;
//   comp->timer = 0.0f;
// }

void Server_InteractWithComponent( GameState *gameState, int componentId ) {
    if ( gameState == NULL ) return;

    for ( int i = 0; i < gameState->componentCount; ++i ) {
        if ( gameState->componentsOnGrid[i].isActive &&
             gameState->componentsOnGrid[i].id == componentId ) {
            CircuitComponent *comp = &gameState->componentsOnGrid[i];

            switch ( comp->type ) {
                case COMP_MOMENTARY_SWITCH:
                    comp->outputState = true;    // Set to ON when pressed
                    TraceLog( LOG_INFO, "SERVER: Momentary switch ID %d pressed ON", comp->id );
                    break;

                case COMP_LATCHING_SWITCH:
                    comp->outputState = !comp->outputState;
                    TraceLog(
                      LOG_INFO, "SERVER: Latching switch ID %d toggled to %s", comp->id,
                      comp->outputState ? "ON" : "OFF"
                    );
                    break;

                default:
                    TraceLog(
                      LOG_INFO, "SERVER: Component ID %d (type %d) has no interaction", comp->id,
                      comp->type
                    );
                    break;
            }
            return;
        }
    }
    TraceLog( LOG_WARNING, "SERVER: Component ID %d not found for interaction", componentId );
}

void Server_ReleaseComponentInteraction( GameState *gameState, int componentId ) {
    if ( gameState == NULL ) return;

    for ( int i = 0; i < gameState->componentCount; ++i ) {
        if ( gameState->componentsOnGrid[i].isActive &&
             gameState->componentsOnGrid[i].id == componentId ) {
            CircuitComponent *comp = &gameState->componentsOnGrid[i];

            if ( comp->type == COMP_MOMENTARY_SWITCH ) {
                comp->outputState = false;    // Set to OFF when released
                TraceLog( LOG_INFO, "SERVER: Momentary switch ID %d released OFF", comp->id );
            }
            return;
        }
    }
    TraceLog(
      LOG_WARNING, "SERVER: Component ID %d not found for release interaction", componentId
    );
}

bool Server_CreateConnection(
  GameState *gameState, int fromComponentId, int toComponentId, int toInputSlot
) {
    if ( gameState == NULL || gameState->connectionCount >= MAX_CONNECTIONS ) {
        TraceLog(
          LOG_WARNING, "SERVER: Cannot create connection, max connections "
                       "reached or null gamestate."
        );
        return false;
    }
    if ( fromComponentId == toComponentId ) {
        TraceLog( LOG_WARNING, "SERVER: Cannot connect component to itself." );
        return false;
    }

    CircuitComponent *toComp = NULL;
    for ( int i = 0; i < gameState->componentCount; ++i ) {
        if ( gameState->componentsOnGrid[i].id == toComponentId &&
             gameState->componentsOnGrid[i].isActive ) {
            toComp = &gameState->componentsOnGrid[i];
            break;
        }
    }
    if ( toComp == NULL ) {
        TraceLog(
          LOG_WARNING, "SERVER: Target component for connection not found (ID: %d).", toComponentId
        );
        return false;
    }
    if ( toInputSlot < 0 || toInputSlot >= MAX_INPUTS_PER_LOGIC_GATE ) {
        TraceLog(
          LOG_WARNING, "SERVER: Invalid input slot %d for component ID %d.", toInputSlot,
          toComponentId
        );
        return false;
    }
    if ( toComp->inputComponentIDs[toInputSlot] != -1 ) {
        TraceLog(
          LOG_WARNING, "SERVER: Input slot %d for component ID %d is already connected.",
          toInputSlot, toComponentId
        );
        return false;
    }

    Connection *newConnection      = &gameState->connections[gameState->connectionCount];
    newConnection->fromComponentId = fromComponentId;
    newConnection->toComponentId   = toComponentId;
    newConnection->toInputSlot     = toInputSlot;
    newConnection->isActive        = true;
    gameState->connectionCount++;

    toComp->inputComponentIDs[toInputSlot] = fromComponentId;
    if ( toComp->connectedInputCount < MAX_INPUTS_PER_LOGIC_GATE ) {
        toComp->connectedInputCount++;
    }

    TraceLog(
      LOG_INFO,
      "SERVER: Created connection from %d to component %d (slot %d). "
      "Total connections: %d",
      fromComponentId, toComponentId, toInputSlot, gameState->connectionCount
    );
    return true;
}

void Server_Init( GameState *gameState ) {
    if ( gameState == NULL ) return;

    srand( (unsigned int) time( NULL ) );

    gameState->componentCount  = 0;
    gameState->nextComponentId = 1;
    gameState->connectionCount = 0;

    for ( int i = 0; i < MAX_COMPONENTS_ON_GRID; ++i ) {
        gameState->componentsOnGrid[i].isActive            = false;
        gameState->componentsOnGrid[i].type                = COMP_NONE;
        gameState->componentsOnGrid[i].outputState         = false;
        gameState->componentsOnGrid[i].defaultOutputState  = false;
        gameState->componentsOnGrid[i].connectedInputCount = 0;
        gameState->componentsOnGrid[i].id                  = -1;
        gameState->componentsOnGrid[i].gridPosition        = (Vector2) { 0, 0 };
        for ( int j = 0; j < MAX_INPUTS_PER_LOGIC_GATE; ++j ) {
            gameState->componentsOnGrid[i].inputComponentIDs[j] = -1;
            gameState->componentsOnGrid[i].actualInputStates[j] = false;
        }
    }

    gameState->handCardCount    = 0;
    gameState->deckCardCount    = 0;
    gameState->currentDeckIndex = 0;
    gameState->discardCardCount = 0;

    Card momentarySwitchCard    = CreateObjectCard( 1, "Momentary Switch", COMP_MOMENTARY_SWITCH );
    Card latchingSwitchCard     = CreateObjectCard( 2, "Latching Switch", COMP_LATCHING_SWITCH );
    Card andGateCard            = CreateObjectCard( 3, "AND Gate", COMP_AND_GATE );
    Card orGateCard             = CreateObjectCard( 4, "OR Gate", COMP_OR_GATE );
    Card sourceCard             = CreateObjectCard( 5, "Source", COMP_SOURCE );
    Card sinkCard               = CreateObjectCard( 6, "Sink", COMP_SINK );

    Card requisitionCard        = Server_CreateActionCard( 7, "Requisition", ACTION_REQUISITION );
    Card reOrgCard              = Server_CreateActionCard( 8, "Re-Org", ACTION_RE_ORG );

    int  deckIdx                = 0;

    for ( int type = 0; type < 6; ++type ) {
        int cardCount = ( type < 4 ) ? 4 : 2;
        for ( int i = 0; i < cardCount; ++i ) {
            if ( deckIdx >= MAX_CARDS_IN_DECK ) break;
            switch ( type ) {
                case 0: gameState->playerDeck[deckIdx++] = momentarySwitchCard; break;
                case 1: gameState->playerDeck[deckIdx++] = latchingSwitchCard; break;
                case 2: gameState->playerDeck[deckIdx++] = andGateCard; break;
                case 3: gameState->playerDeck[deckIdx++] = orGateCard; break;
                case 4: gameState->playerDeck[deckIdx++] = sourceCard; break;
                case 5: gameState->playerDeck[deckIdx++] = sinkCard; break;
            }
        }
        if ( deckIdx >= MAX_CARDS_IN_DECK ) break;
    }

    for ( int i = 0; i < 3 && deckIdx < MAX_CARDS_IN_DECK; ++i ) {
        gameState->playerDeck[deckIdx++] = requisitionCard;
    }
    for ( int i = 0; i < 2 && deckIdx < MAX_CARDS_IN_DECK; ++i ) {
        gameState->playerDeck[deckIdx++] = reOrgCard;
    }

    gameState->deckCardCount = deckIdx;
    if ( gameState->deckCardCount > 1 ) {
        for ( int i = gameState->deckCardCount - 1; i > 0; i-- ) {
            int  j                   = rand() % ( i + 1 );
            Card temp                = gameState->playerDeck[i];
            gameState->playerDeck[i] = gameState->playerDeck[j];
            gameState->playerDeck[j] = temp;
        }
        TraceLog( LOG_INFO, "SERVER: Initial deck shuffled." );
    }

    for ( int i = 0; i < 5; ++i ) { Server_PlayerDrawCard( gameState ); }

    gameState->score        = 0;
    gameState->is_game_over = false;
    Server_LoadStarterScenario( gameState );

    TraceLog(
      LOG_INFO,
      "SERVER_INIT_END: Score: %d, DeckCount: %d, CurrentDeckIdx: %d, "
      "HandCount: %d, DiscardCount: %d",
      gameState->score, gameState->deckCardCount, gameState->currentDeckIndex,
      gameState->handCardCount, gameState->discardCardCount
    );
}

static void PropagateSignals( GameState *gameState ) {
    bool stateChanged  = true;
    int  maxIterations = 10;
    int  iteration     = 0;

    while ( stateChanged && iteration < maxIterations ) {
        stateChanged = false;
        iteration++;

        for ( int i = 0; i < gameState->componentCount; ++i ) {
            if ( !gameState->componentsOnGrid[i].isActive ) continue;

            CircuitComponent *comp          = &gameState->componentsOnGrid[i];
            bool              previousState = comp->outputState;

            switch ( comp->type ) {
                case COMP_SOURCE          : comp->outputState = true; break;

                case COMP_SINK            : comp->outputState = false; break;

                case COMP_MOMENTARY_SWITCH:
                case COMP_LATCHING_SWITCH : break;

                case COMP_AND_GATE:
                    {
                        bool hasAllInputs  = true;
                        bool allInputsHigh = true;

                        for ( int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE; ++k ) {
                            if ( comp->inputComponentIDs[k] != -1 ) {
                                CircuitComponent *inputComp = NULL;
                                for ( int j = 0; j < gameState->componentCount; ++j ) {
                                    if ( gameState->componentsOnGrid[j].isActive &&
                                         gameState->componentsOnGrid[j].id ==
                                           comp->inputComponentIDs[k] ) {
                                        inputComp = &gameState->componentsOnGrid[j];
                                        break;
                                    }
                                }
                                if ( inputComp ) {
                                    comp->actualInputStates[k] = inputComp->outputState;
                                    if ( !inputComp->outputState ) { allInputsHigh = false; }
                                } else {
                                    hasAllInputs = false;
                                    break;
                                }
                            } else {
                                hasAllInputs = false;
                                break;
                            }
                        }

                        if ( hasAllInputs &&
                             comp->connectedInputCount == MAX_INPUTS_PER_LOGIC_GATE ) {
                            comp->outputState = allInputsHigh;
                        } else {
                            comp->outputState = false;
                        }
                        break;
                    }

                case COMP_OR_GATE:
                    {
                        bool hasAnyInput  = false;
                        bool anyInputHigh = false;

                        for ( int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE; ++k ) {
                            if ( comp->inputComponentIDs[k] != -1 ) {
                                hasAnyInput                 = true;
                                CircuitComponent *inputComp = NULL;
                                for ( int j = 0; j < gameState->componentCount; ++j ) {
                                    if ( gameState->componentsOnGrid[j].isActive &&
                                         gameState->componentsOnGrid[j].id ==
                                           comp->inputComponentIDs[k] ) {
                                        inputComp = &gameState->componentsOnGrid[j];
                                        break;
                                    }
                                }
                                if ( inputComp ) {
                                    comp->actualInputStates[k] = inputComp->outputState;
                                    if ( inputComp->outputState ) { anyInputHigh = true; }
                                }
                            }
                        }

                        comp->outputState = hasAnyInput && anyInputHigh;
                        break;
                    }

                default: break;
            }

            if ( comp->outputState != previousState ) { stateChanged = true; }
        }
    }

    if ( iteration >= maxIterations && stateChanged ) {
        TODO(
          "Circuit oscillation detected or too complex - implement cycle "
          "detection"
        );
    }
}

void Server_InitScenario( Scenario *scenario, const char *name, const char *description ) {
    if ( scenario == NULL ) return;

    strncpy( scenario->name, name, sizeof( scenario->name ) - 1 );
    scenario->name[sizeof( scenario->name ) - 1] = '\0';
    strncpy( scenario->description, description, sizeof( scenario->description ) - 1 );
    scenario->description[sizeof( scenario->description ) - 1] = '\0';

    scenario->conditionCount                                   = 0;
    scenario->isCompleted                                      = false;
    scenario->rewardScore                                      = 100;

    for ( int i = 0; i < 8; ++i ) {
        scenario->conditions[i].type           = CONDITION_MIN_COMPONENTS;
        scenario->conditions[i].componentType  = COMP_NONE;
        scenario->conditions[i].targetValue    = 0;
        scenario->conditions[i].isMet          = false;
        scenario->conditions[i].description[0] = '\0';
    }
}

bool Server_AddScenarioCondition(
  Scenario *scenario, ScenarioConditionType type, ComponentType componentType, int targetValue,
  const char *description
) {
    if ( scenario == NULL || scenario->conditionCount >= 8 ) return false;

    ScenarioCondition *condition = &scenario->conditions[scenario->conditionCount];
    condition->type              = type;
    condition->componentType     = componentType;
    condition->targetValue       = targetValue;
    condition->isMet             = false;
    strncpy( condition->description, description, sizeof( condition->description ) - 1 );
    condition->description[sizeof( condition->description ) - 1] = '\0';

    scenario->conditionCount++;
    return true;
}

void Server_EvaluateScenario( GameState *gameState ) {
    if ( gameState == NULL ) return;

    Scenario *scenario         = &gameState->currentScenario;
    bool      allConditionsMet = true;

    for ( int i = 0; i < scenario->conditionCount; ++i ) {
        ScenarioCondition *condition = &scenario->conditions[i];
        condition->isMet             = false;

        switch ( condition->type ) {
            case CONDITION_MIN_COMPONENTS:
                {
                    int count = 0;
                    for ( int j = 0; j < gameState->componentCount; ++j ) {
                        if ( gameState->componentsOnGrid[j].isActive &&
                             gameState->componentsOnGrid[j].type == condition->componentType ) {
                            count++;
                        }
                    }
                    condition->isMet = ( count >= condition->targetValue );
                    break;
                }

            case CONDITION_MAX_COMPONENTS:
                {
                    int count = 0;
                    for ( int j = 0; j < gameState->componentCount; ++j ) {
                        if ( gameState->componentsOnGrid[j].isActive &&
                             gameState->componentsOnGrid[j].type == condition->componentType ) {
                            count++;
                        }
                    }
                    condition->isMet = ( count <= condition->targetValue );
                    break;
                }

            case CONDITION_MIN_UNIQUE_STATES:
            case CONDITION_MAX_UNIQUE_STATES:
            case CONDITION_SPECIFIC_STATE:
                TODO( "Implement state-based scenario conditions" );
                condition->isMet = false;
                break;

            default: condition->isMet = false; break;
        }

        if ( !condition->isMet ) { allConditionsMet = false; }
    }

    if ( allConditionsMet && !scenario->isCompleted ) {
        scenario->isCompleted  = true;
        gameState->score      += scenario->rewardScore;
        TraceLog(
          LOG_INFO, "SERVER: Scenario '%s' completed! Score: %d", scenario->name, gameState->score
        );

        if ( Server_AdvanceToNextScenario( gameState ) ) {
            TraceLog( LOG_INFO, "SERVER: Advanced to next scenario" );
        }
    }
}

void Server_LoadScenario( GameState *gameState, ScenarioId scenarioId ) {
    if ( gameState == NULL || scenarioId >= SCENARIO_COUNT ) return;

    gameState->currentScenarioId = scenarioId;

    switch ( scenarioId ) {
        case SCENARIO_BASIC_CIRCUIT:
            Server_InitScenario(
              &gameState->currentScenario, "Basic Circuit",
              "Learn the basics: place a switch and a gate"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_LATCHING_SWITCH, 1,
              "Place at least 1 switch"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_AND_GATE, 1,
              "Place at least 1 AND gate"
            );
            break;

        case SCENARIO_SIMPLE_LOGIC:
            Server_InitScenario(
              &gameState->currentScenario, "Simple Logic",
              "Build a working circuit: connect a source to an AND gate"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_SOURCE, 1,
              "Place at least 1 source"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_AND_GATE, 1,
              "Place at least 1 AND gate"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_LATCHING_SWITCH, 1,
              "Place at least 1 switch"
            );
            break;

        case SCENARIO_TOGGLE_SWITCH:
            Server_InitScenario(
              &gameState->currentScenario, "Toggle Switch",
              "Master switching: use multiple switches with gates"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_LATCHING_SWITCH, 2,
              "Place at least 2 switches"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_OR_GATE, 1,
              "Place at least 1 OR gate"
            );
            break;

        case SCENARIO_MULTI_INPUT:
            Server_InitScenario(
              &gameState->currentScenario, "Multi Input",
              "Advanced logic: combine multiple input types"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_SOURCE, 1,
              "Place at least 1 source"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_MOMENTARY_SWITCH, 1,
              "Place at least 1 momentary switch"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_OR_GATE, 1,
              "Place at least 1 OR gate"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MAX_COMPONENTS, COMP_LATCHING_SWITCH, 0,
              "Use no latching switches"
            );
            break;

        case SCENARIO_COMPLEX_LOGIC:
            Server_InitScenario(
              &gameState->currentScenario, "Complex Logic",
              "Expert challenge: build circuits with both gate types"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_AND_GATE, 1,
              "Place at least 1 AND gate"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_OR_GATE, 1,
              "Place at least 1 OR gate"
            );
            Server_AddScenarioCondition(
              &gameState->currentScenario, CONDITION_MIN_COMPONENTS, COMP_SOURCE, 2,
              "Place at least 2 sources"
            );
            break;

        default:
            Server_InitScenario( &gameState->currentScenario, "Unknown", "Error loading scenario" );
            break;
    }

    TraceLog(
      LOG_INFO, "SERVER: Loaded scenario %d: %s", scenarioId, gameState->currentScenario.name
    );
}

bool Server_AdvanceToNextScenario( GameState *gameState ) {
    if ( gameState == NULL || !gameState->currentScenario.isCompleted ) return false;

    gameState->scenarioProgression[gameState->currentScenarioId] = true;

    int nextScenarioId                                           = gameState->currentScenarioId + 1;
    if ( nextScenarioId >= SCENARIO_COUNT ) {
        TraceLog( LOG_INFO, "SERVER: All scenarios completed!" );
        return false;
    }

    Server_LoadScenario( gameState, nextScenarioId );
    return true;
}

void Server_ResetCurrentScenario( GameState *gameState ) {
    if ( gameState == NULL ) return;

    for ( int i = 0; i < gameState->componentCount; ++i ) {
        gameState->componentsOnGrid[i].isActive = false;
    }
    gameState->componentCount  = 0;
    gameState->connectionCount = 0;

    for ( int i = 0; i < gameState->discardCardCount; ++i ) {
        if ( gameState->handCardCount < MAX_CARDS_IN_HAND ) {
            gameState->playerHand[gameState->handCardCount] = gameState->playerDiscard[i];
            gameState->handCardCount++;
        }
    }
    gameState->discardCardCount = 0;

    Server_LoadScenario( gameState, gameState->currentScenarioId );

    TraceLog( LOG_INFO, "SERVER: Reset scenario %d", gameState->currentScenarioId );
}

void Server_LoadStarterScenario( GameState *gameState ) {
    if ( gameState == NULL ) return;

    for ( int i = 0; i < SCENARIO_COUNT; ++i ) { gameState->scenarioProgression[i] = false; }

    Server_LoadScenario( gameState, SCENARIO_BASIC_CIRCUIT );
}

Card Server_CreateActionCard( int id, const char *name, ActionCardType actionType ) {
    Card card;
    card.id            = id;
    card.type          = CARD_TYPE_ACTION;
    card.actionType    = actionType;
    card.objectToPlace = COMP_NONE;
    strncpy( card.name, name, sizeof( card.name ) - 1 );
    card.name[sizeof( card.name ) - 1] = '\0';

    switch ( actionType ) {
        case ACTION_REQUISITION:
            snprintf( card.description, sizeof( card.description ), "Draw 3 cards from deck." );
            break;
        case ACTION_RECYCLE:
            snprintf(
              card.description, sizeof( card.description ), "Discard any cards, draw that many."
            );
            break;
        case ACTION_RE_ORG:
            snprintf(
              card.description, sizeof( card.description ), "Discard hand, draw to full hand."
            );
            break;
        case ACTION_JOB_FAIR:
            snprintf(
              card.description, sizeof( card.description ), "Pick 1 of 3 cards to add to deck."
            );
            break;
        case ACTION_CONTINUOUS_IMPROVEMENT:
            snprintf(
              card.description, sizeof( card.description ), "Add input/output to element."
            );
            break;
        case ACTION_END_OF_LIFE:
            snprintf( card.description, sizeof( card.description ), "Permanently remove a card." );
            break;
        case ACTION_PARTS_BIN:
            snprintf( card.description, sizeof( card.description ), "Copy an element in play." );
            break;
        default: snprintf( card.description, sizeof( card.description ), "Unknown action." ); break;
    }

    return card;
}

bool Server_ExecuteActionCard( GameState *gameState, ActionCardType actionType ) {
    if ( gameState == NULL ) return false;

    switch ( actionType ) {
        case ACTION_REQUISITION:
            for ( int i = 0; i < 3; ++i ) {
                if ( !Server_PlayerDrawCard( gameState ) ) break;
            }
            TraceLog( LOG_INFO, "SERVER: Requisition executed - drew up to 3 cards" );
            return true;

        case ACTION_RE_ORG:
            while ( gameState->handCardCount > 0 ) {
                if ( gameState->discardCardCount >= MAX_CARDS_IN_DECK ) break;
                gameState->playerDiscard[gameState->discardCardCount] =
                  gameState->playerHand[gameState->handCardCount - 1];
                gameState->discardCardCount++;
                gameState->handCardCount--;
            }
            while ( gameState->handCardCount < MAX_CARDS_IN_HAND ) {
                if ( !Server_PlayerDrawCard( gameState ) ) break;
            }
            TraceLog( LOG_INFO, "SERVER: Re-Org executed - discarded hand and drew full hand" );
            return true;

        case ACTION_RECYCLE:
        case ACTION_JOB_FAIR:
        case ACTION_CONTINUOUS_IMPROVEMENT:
        case ACTION_END_OF_LIFE:
        case ACTION_PARTS_BIN:
            TODO( "Implement interactive action cards that require user input" );
            return false;

        default:
            TraceLog( LOG_WARNING, "SERVER: Unknown action type %d", actionType );
            return false;
    }
}

void Server_Update( GameState *gameState, float deltaTime ) {
    serverUpdateFrameCounter++;

    if ( gameState == NULL || gameState->is_game_over ) {
        TraceLog(
          LOG_INFO, "SERVER_UPDATE_END (Frame: %u): Early exit (null or game over)",
          serverUpdateFrameCounter
        );
        return;
    }

    for ( int i = 0; i < gameState->componentCount; ++i ) {
        if ( !gameState->componentsOnGrid[i].isActive ) continue;

        CircuitComponent *comp    = &gameState->componentsOnGrid[i];
        comp->connectedInputCount = 0;

        for ( int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE; ++k ) {
            if ( comp->inputComponentIDs[k] != -1 ) { comp->connectedInputCount++; }
        }
    }

    PropagateSignals( gameState );
    Server_EvaluateScenario( gameState );
}
