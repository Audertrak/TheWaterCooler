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

static void         PropagateSignals( SimulatorState *simulatorState );

static Card         CreateElementCard( int id, const char *name, ElementType elemType ) {
    Card card;
    card.id   = id;
    card.type = CARD_TYPE_ELEMENT;
    strncpy( card.name, name, sizeof( card.name ) - 1 );
    card.name[sizeof( card.name ) - 1] = '\0';
    snprintf( card.description, sizeof( card.description ), "Places a %s.", name );
    card.elementToPlace = elemType;
    return card;
}

static bool Server_AttemptDrawAndReshuffle( SimulatorState *simulatorState ) {
    if ( simulatorState == NULL ) return false;
    if ( simulatorState->currentDeckIndex >= simulatorState->deckCardCount ) {
        if ( simulatorState->discardCardCount > 0 ) {
            TraceLog(
              LOG_INFO, "SERVER: Deck empty. Moving discard pile (%d cards) to deck.",
              simulatorState->discardCardCount
            );
            for ( int i = 0; i < simulatorState->discardCardCount; ++i ) {
                simulatorState->userDeck[i] = simulatorState->userDiscard[i];
            }            simulatorState->deckCardCount    = simulatorState->discardCardCount;
            simulatorState->discardCardCount = 0;
            simulatorState->currentDeckIndex = 0;
            if ( simulatorState->deckCardCount > 1 ) {
                for ( int i = simulatorState->deckCardCount - 1; i > 0; i-- ) {
                    int  j                        = rand() % ( i + 1 );
                    Card temp                     = simulatorState->userDeck[i];
                    simulatorState->userDeck[i]   = simulatorState->userDeck[j];
                    simulatorState->userDeck[j]   = temp;
                }
                TraceLog( LOG_INFO, "SERVER: Deck reshuffled." );
            }
        } else {
            TraceLog( LOG_INFO, "SERVER: Deck and discard pile are empty. Cannot draw." );
            return false;
        }
    }
    if ( simulatorState->currentDeckIndex >= simulatorState->deckCardCount ) {
        TraceLog( LOG_INFO, "SERVER: Deck still empty after attempting reshuffle. Cannot draw." );
        return false;
    }
    return true;
}

bool Server_UserDrawCard( SimulatorState *simulatorState ) {
    TraceLog(
      LOG_INFO,
      "SERVER_USER_DRAW_CARD_START: Hand: %d/%d, Deck: %d, Idx: %d, "
      "Discard: %d",
      simulatorState->handCardCount, MAX_CARDS_IN_HAND, simulatorState->deckCardCount,
      simulatorState->currentDeckIndex, simulatorState->discardCardCount
    );
    if ( simulatorState == NULL ) return false;
    if ( simulatorState->handCardCount >= MAX_CARDS_IN_HAND ) {
        TraceLog( LOG_INFO, "SERVER: Hand is full. Cannot draw card." );
        return false;
    }
    if ( !Server_AttemptDrawAndReshuffle( simulatorState ) ) { return false; }
    simulatorState->userHand[simulatorState->handCardCount] =
      simulatorState->userDeck[simulatorState->currentDeckIndex];
    TraceLog(
      LOG_INFO, "SERVER: User drew card '%s'. Hand size: %d",
      simulatorState->userHand[simulatorState->handCardCount].name, simulatorState->handCardCount + 1
    );
    simulatorState->handCardCount++;
    simulatorState->currentDeckIndex++;
    return true;
}

bool Server_UseCardFromHand( SimulatorState *simulatorState, int handIndex ) {
    if ( simulatorState == NULL || handIndex < 0 || handIndex >= simulatorState->handCardCount ) {
        TraceLog( LOG_WARNING, "SERVER: Invalid hand index %d or null simulatorState.", handIndex );
        return false;
    }
    if ( simulatorState->discardCardCount >= MAX_CARDS_IN_DECK ) {
        TraceLog( LOG_WARNING, "SERVER: Discard pile is full. Cannot use card." );
        return false;
    }

    Card usedCard = simulatorState->userHand[handIndex];
    TraceLog(
      LOG_INFO, "SERVER: Using card '%s' from hand index %d.", usedCard.name, handIndex
    );

    if ( usedCard.type == CARD_TYPE_ACTION ) {
        if ( Server_ExecuteActionCard( simulatorState, usedCard.actionType ) ) {
            simulatorState->userDiscard[simulatorState->discardCardCount] = usedCard;
            simulatorState->discardCardCount++;
            for ( int i = handIndex; i < simulatorState->handCardCount - 1; ++i ) {
                simulatorState->userHand[i] = simulatorState->userHand[i + 1];
            }
            simulatorState->handCardCount--;
            return true;
        }
        return false;
    }

    simulatorState->userDiscard[simulatorState->discardCardCount] = usedCard;
    simulatorState->discardCardCount++;
    for ( int i = handIndex; i < simulatorState->handCardCount - 1; ++i ) {
        simulatorState->userHand[i] = simulatorState->userHand[i + 1];
    }
    simulatorState->handCardCount--;
    return true;
}

void Server_InteractWithElement( SimulatorState *simulatorState, int elementId ) {
    if ( simulatorState == NULL ) return;

    for ( int i = 0; i < simulatorState->elementCount; ++i ) {
        if ( simulatorState->elementsOnCanvas[i].isActive &&
             simulatorState->elementsOnCanvas[i].id == elementId ) {
            CircuitElement *elem = &simulatorState->elementsOnCanvas[i];

            switch ( elem->type ) {
                case ELEMENT_BUTTON:
                    elem->outputState = true;
                    break;

                case ELEMENT_SWITCH:
                    elem->outputState = !elem->outputState;
                    TraceLog(
                      LOG_INFO, "SERVER: Switch ID %d toggled to %s", elem->id,
                      elem->outputState ? "ON" : "OFF"
                    );
                    break;

                default:
                    TraceLog(
                      LOG_INFO, "SERVER: Element ID %d (type %d) has no interaction", elem->id,
                      elem->type
                    );
                    break;
            }
            return;
        }
    }
    TraceLog( LOG_WARNING, "SERVER: Element ID %d not found for interaction", elementId );
}

void Server_ReleaseElementInteraction( SimulatorState *simulatorState, int elementId ) {
    if ( simulatorState == NULL ) return;

    for ( int i = 0; i < simulatorState->elementCount; ++i ) {
        if ( simulatorState->elementsOnCanvas[i].isActive &&
             simulatorState->elementsOnCanvas[i].id == elementId ) {
            CircuitElement *elem = &simulatorState->elementsOnCanvas[i];

            if ( elem->type == ELEMENT_BUTTON ) {
                elem->outputState = false;
                TraceLog( LOG_INFO, "SERVER: Button ID %d released OFF", elem->id );
            }
            return;
        }
    }
    TraceLog(
      LOG_WARNING, "SERVER: Element ID %d not found for release interaction", elementId
    );
}

bool Server_CreateConnection(
  SimulatorState *simulatorState, int fromElementId, int toElementId, int toInputSlot
) {
    if ( simulatorState == NULL || simulatorState->connectionCount >= MAX_CONNECTIONS ) {
        TraceLog(
          LOG_WARNING, "SERVER: Cannot create connection, max connections "
                       "reached or null simulatorState."
        );
        return false;
    }
    if ( fromElementId == toElementId ) {
        TraceLog( LOG_WARNING, "SERVER: Cannot connect element to itself." );
        return false;
    }

    CircuitElement *toElem = NULL;
    for ( int i = 0; i < simulatorState->elementCount; ++i ) {
        if ( simulatorState->elementsOnCanvas[i].id == toElementId &&
             simulatorState->elementsOnCanvas[i].isActive ) {
            toElem = &simulatorState->elementsOnCanvas[i];
            break;
        }
    }
    if ( toElem == NULL ) {
        TraceLog(
          LOG_WARNING, "SERVER: Target element for connection not found (ID: %d).", toElementId
        );
        return false;
    }
    if ( toInputSlot < 0 || toInputSlot >= MAX_INPUTS_PER_LOGIC_GATE ) {
        TraceLog(
          LOG_WARNING, "SERVER: Invalid input slot %d for element ID %d.", toInputSlot,
          toElementId
        );
        return false;
    }
    if ( toElem->inputElementIDs[toInputSlot] != -1 ) {
        TraceLog(
          LOG_WARNING, "SERVER: Input slot %d for element ID %d is already connected.",
          toInputSlot, toElementId
        );
        return false;
    }

    Connection *newConnection      = &simulatorState->connections[simulatorState->connectionCount];
    newConnection->fromElementId = fromElementId;
    newConnection->toElementId   = toElementId;
    newConnection->toInputSlot     = toInputSlot;
    newConnection->isActive        = true;
    simulatorState->connectionCount++;

    toElem->inputElementIDs[toInputSlot] = fromElementId;
    if ( toElem->connectedInputCount < MAX_INPUTS_PER_LOGIC_GATE ) {
        toElem->connectedInputCount++;
    }

    TraceLog(
      LOG_INFO,
      "SERVER: Created connection from %d to element %d (slot %d). "
      "Total connections: %d",
      fromElementId, toElementId, toInputSlot, simulatorState->connectionCount
    );
    return true;
}

void Server_Init( SimulatorState *simulatorState ) {
    if ( simulatorState == NULL ) return;

    srand( (unsigned int) time( NULL ) );

    simulatorState->elementCount  = 0;
    simulatorState->nextElementId = 1;
    simulatorState->connectionCount = 0;

    for ( int i = 0; i < MAX_ELEMENTS_ON_CANVAS; ++i ) {
        simulatorState->elementsOnCanvas[i].isActive            = false;
        simulatorState->elementsOnCanvas[i].type                = ELEMENT_NONE;
        simulatorState->elementsOnCanvas[i].outputState         = false;
        simulatorState->elementsOnCanvas[i].defaultOutputState  = false;
        simulatorState->elementsOnCanvas[i].connectedInputCount = 0;
        simulatorState->elementsOnCanvas[i].id                  = -1;
        simulatorState->elementsOnCanvas[i].canvasPosition      = (Vector2) { 0, 0 };
        for ( int j = 0; j < MAX_INPUTS_PER_LOGIC_GATE; ++j ) {
            simulatorState->elementsOnCanvas[i].inputElementIDs[j] = -1;
            simulatorState->elementsOnCanvas[i].actualInputStates[j] = false;
        }
    }

    simulatorState->handCardCount    = 0;
    simulatorState->deckCardCount    = 0;
    simulatorState->currentDeckIndex = 0;
    simulatorState->discardCardCount = 0;

    Card buttonCard         = CreateElementCard( 1, "Button", ELEMENT_BUTTON );
    Card switchCard         = CreateElementCard( 2, "Switch", ELEMENT_SWITCH );
    Card andGateCard        = CreateElementCard( 3, "AND Gate", ELEMENT_AND );
    Card orGateCard         = CreateElementCard( 4, "OR Gate", ELEMENT_OR );
    Card sourceCard         = CreateElementCard( 5, "Source", ELEMENT_SOURCE );
    Card sensorCard         = CreateElementCard( 6, "Sensor", ELEMENT_SENSOR );

    Card requisitionCard    = Server_CreateActionCard( 7, "Requisition", ACTION_REQUISITION );
    Card reOrgCard          = Server_CreateActionCard( 8, "Re-Org", ACTION_RE_ORG );

    int  deckIdx            = 0;

    for ( int type = 0; type < 6; ++type ) {
        int cardCount = ( type < 4 ) ? 4 : 2;
        for ( int i = 0; i < cardCount; ++i ) {
            if ( deckIdx >= MAX_CARDS_IN_DECK ) break;
            switch ( type ) {
                case 0: simulatorState->userDeck[deckIdx++] = buttonCard; break;
                case 1: simulatorState->userDeck[deckIdx++] = switchCard; break;
                case 2: simulatorState->userDeck[deckIdx++] = andGateCard; break;
                case 3: simulatorState->userDeck[deckIdx++] = orGateCard; break;
                case 4: simulatorState->userDeck[deckIdx++] = sourceCard; break;
                case 5: simulatorState->userDeck[deckIdx++] = sensorCard; break;
            }
        }
        if ( deckIdx >= MAX_CARDS_IN_DECK ) break;
    }

    for ( int i = 0; i < 3 && deckIdx < MAX_CARDS_IN_DECK; ++i ) {
        simulatorState->userDeck[deckIdx++] = requisitionCard;
    }
    for ( int i = 0; i < 2 && deckIdx < MAX_CARDS_IN_DECK; ++i ) {
        simulatorState->userDeck[deckIdx++] = reOrgCard;
    }

    simulatorState->deckCardCount = deckIdx;
    if ( simulatorState->deckCardCount > 1 ) {
        for ( int i = simulatorState->deckCardCount - 1; i > 0; i-- ) {
            int  j                         = rand() % ( i + 1 );
            Card temp                      = simulatorState->userDeck[i];
            simulatorState->userDeck[i]    = simulatorState->userDeck[j];
            simulatorState->userDeck[j]    = temp;
        }
        TraceLog( LOG_INFO, "SERVER: Initial deck shuffled." );
    }    for ( int i = 0; i < 5; ++i ) { Server_UserDrawCard( simulatorState ); }

    simulatorState->score               = 0;
    simulatorState->simulationComplete  = false;
    Server_LoadStarterScenario( simulatorState );

    TraceLog(
      LOG_INFO,
      "SERVER_INIT_END: Score: %d, DeckCount: %d, CurrentDeckIdx: %d, "
      "HandCount: %d, DiscardCount: %d",
      simulatorState->score, simulatorState->deckCardCount, simulatorState->currentDeckIndex,
      simulatorState->handCardCount, simulatorState->discardCardCount
    );
}

static void PropagateSignals( SimulatorState *simulatorState ) {
    bool stateChanged  = true;
    int  maxIterations = 10;
    int  iteration     = 0;

    while ( stateChanged && iteration < maxIterations ) {
        stateChanged = false;
        iteration++;

        for ( int i = 0; i < simulatorState->elementCount; ++i ) {
            if ( !simulatorState->elementsOnCanvas[i].isActive ) continue;

            CircuitElement *elem          = &simulatorState->elementsOnCanvas[i];
            bool            previousState = elem->outputState;

            switch ( elem->type ) {
                case ELEMENT_SOURCE : elem->outputState = true; break;

                case ELEMENT_SENSOR : elem->outputState = false; break;

                case ELEMENT_BUTTON:
                case ELEMENT_SWITCH : break;                case ELEMENT_AND:
                    {
                        bool hasAllInputs  = false;
                        bool allInputsHigh = true;

                        // Count valid inputs and check if they're high
                        for (int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE; ++k) {
                            if (elem->inputElementIDs[k] != -1) {
                                CircuitElement *inputElem = NULL;
                                for (int j = 0; j < simulatorState->elementCount; ++j) {
                                    if (simulatorState->elementsOnCanvas[j].isActive &&
                                        simulatorState->elementsOnCanvas[j].id == elem->inputElementIDs[k]) {
                                        inputElem = &simulatorState->elementsOnCanvas[j];
                                        break;
                                    }
                                }
                                if (inputElem) {
                                    elem->actualInputStates[k] = inputElem->outputState;
                                    if (!inputElem->outputState) {
                                        allInputsHigh = false;
                                    }
                                    hasAllInputs = true;
                                }
                            }
                        }

                        // AND gate needs at least 2 inputs and all must be high
                        elem->outputState = (hasAllInputs && elem->connectedInputCount >= 2 && allInputsHigh);
                        break;
                    }

                case ELEMENT_OR:
                    {
                        bool hasAnyInput  = false;
                        bool anyInputHigh = false;

                        for ( int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE; ++k ) {
                            if ( elem->inputElementIDs[k] != -1 ) {
                                hasAnyInput                 = true;
                                CircuitElement *inputElem = NULL;
                                for ( int j = 0; j < simulatorState->elementCount; ++j ) {
                                    if ( simulatorState->elementsOnCanvas[j].isActive &&
                                         simulatorState->elementsOnCanvas[j].id ==
                                           elem->inputElementIDs[k] ) {
                                        inputElem = &simulatorState->elementsOnCanvas[j];
                                        break;
                                    }
                                }
                                if ( inputElem ) {
                                    elem->actualInputStates[k] = inputElem->outputState;
                                    if ( inputElem->outputState ) { anyInputHigh = true; }
                                }
                            }
                        }

                        elem->outputState = hasAnyInput && anyInputHigh;
                        break;
                    }

                default: break;
            }

            if ( elem->outputState != previousState ) { stateChanged = true; }
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
    scenario->rewardScore                                      = 100;    for ( int i = 0; i < 8; ++i ) {
        scenario->conditions[i].type           = CONDITION_MIN_ELEMENTS;
        scenario->conditions[i].elementType    = ELEMENT_NONE;
        scenario->conditions[i].targetValue    = 0;
        scenario->conditions[i].isMet          = false;
        scenario->conditions[i].description[0] = '\0';
    }
}

bool Server_AddScenarioCondition(
  Scenario *scenario, ScenarioConditionType type, ElementType elementType, int targetValue,
  const char *description
) {
    if ( scenario == NULL || scenario->conditionCount >= 8 ) return false;

    ScenarioCondition *condition = &scenario->conditions[scenario->conditionCount];
    condition->type              = type;
    condition->elementType       = elementType;
    condition->targetValue       = targetValue;
    condition->isMet             = false;
    strncpy( condition->description, description, sizeof( condition->description ) - 1 );
    condition->description[sizeof( condition->description ) - 1] = '\0';

    scenario->conditionCount++;
    return true;
}

void Server_EvaluateScenario( SimulatorState *simulatorState ) {
    if ( simulatorState == NULL ) return;

    Scenario *scenario         = &simulatorState->currentScenario;
    bool      allConditionsMet = true;

    for ( int i = 0; i < scenario->conditionCount; ++i ) {
        ScenarioCondition *condition = &scenario->conditions[i];
        condition->isMet             = false;

        switch ( condition->type ) {
            case CONDITION_MIN_ELEMENTS:
                {
                    int count = 0;
                    for ( int j = 0; j < simulatorState->elementCount; ++j ) {
                        if ( simulatorState->elementsOnCanvas[j].isActive &&
                             simulatorState->elementsOnCanvas[j].type == condition->elementType ) {
                            count++;
                        }
                    }
                    condition->isMet = ( count >= condition->targetValue );
                    break;
                }

            case CONDITION_MAX_ELEMENTS:
                {
                    int count = 0;
                    for ( int j = 0; j < simulatorState->elementCount; ++j ) {
                        if ( simulatorState->elementsOnCanvas[j].isActive &&
                             simulatorState->elementsOnCanvas[j].type == condition->elementType ) {
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
        scenario->isCompleted     = true;
        simulatorState->score    += scenario->rewardScore;
        TraceLog(
          LOG_INFO, "SERVER: Scenario '%s' completed! Score: %d", scenario->name, simulatorState->score
        );

        if ( Server_AdvanceToNextScenario( simulatorState ) ) {
            TraceLog( LOG_INFO, "SERVER: Advanced to next scenario" );
        }
    }
}

void Server_LoadScenario( SimulatorState *simulatorState, ScenarioId scenarioId ) {
    if ( simulatorState == NULL || scenarioId >= SCENARIO_COUNT ) return;

    simulatorState->currentScenarioId = scenarioId;

    switch ( scenarioId ) {
        case SCENARIO_BASIC_CIRCUIT:
            Server_InitScenario(
              &simulatorState->currentScenario, "Basic Circuit",
              "Learn the basics: place a switch and a gate"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_SWITCH, 1,
              "Place at least 1 switch"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_AND, 1,
              "Place at least 1 AND gate"
            );
            break;

        case SCENARIO_SIMPLE_LOGIC:
            Server_InitScenario(
              &simulatorState->currentScenario, "Simple Logic",
              "Build a working circuit: connect a source to an AND gate"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_SOURCE, 1,
              "Place at least 1 source"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_AND, 1,
              "Place at least 1 AND gate"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_SWITCH, 1,
              "Place at least 1 switch"
            );            break;

        case SCENARIO_TOGGLE_SWITCH:
            Server_InitScenario(
              &simulatorState->currentScenario, "Toggle Switch",
              "Master switching: use multiple switches with gates"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_SWITCH, 2,
              "Place at least 2 switches"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_OR, 1,
              "Place at least 1 OR gate"
            );
            break;

        case SCENARIO_MULTI_INPUT:
            Server_InitScenario(
              &simulatorState->currentScenario, "Multi Input",
              "Advanced logic: combine multiple input types"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_SOURCE, 1,
              "Place at least 1 source"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_BUTTON, 1,
              "Place at least 1 button"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_OR, 1,
              "Place at least 1 OR gate"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MAX_ELEMENTS, ELEMENT_SWITCH, 0,
              "Use no switches"
            );
            break;

        case SCENARIO_COMPLEX_LOGIC:
            Server_InitScenario(
              &simulatorState->currentScenario, "Complex Logic",
              "Expert challenge: build circuits with both gate types"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_AND, 1,
              "Place at least 1 AND gate"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_OR, 1,
              "Place at least 1 OR gate"
            );
            Server_AddScenarioCondition(
              &simulatorState->currentScenario, CONDITION_MIN_ELEMENTS, ELEMENT_SOURCE, 2,
              "Place at least 2 sources"
            );
            break;

        default:
            Server_InitScenario( &simulatorState->currentScenario, "Unknown", "Error loading scenario" );
            break;
    }

    TraceLog(
      LOG_INFO, "SERVER: Loaded scenario %d: %s", scenarioId, simulatorState->currentScenario.name
    );
}

bool Server_AdvanceToNextScenario( SimulatorState *simulatorState ) {
    if ( simulatorState == NULL || !simulatorState->currentScenario.isCompleted ) return false;

    simulatorState->scenarioProgression[simulatorState->currentScenarioId] = true;

    int nextScenarioId                                           = simulatorState->currentScenarioId + 1;
    if ( nextScenarioId >= SCENARIO_COUNT ) {
        TraceLog( LOG_INFO, "SERVER: All scenarios completed!" );
        return false;
    }

    Server_LoadScenario( simulatorState, nextScenarioId );
    return true;
}

void Server_ResetCurrentScenario( SimulatorState *simulatorState ) {
    if ( simulatorState == NULL ) return;

    for ( int i = 0; i < simulatorState->elementCount; ++i ) {
        simulatorState->elementsOnCanvas[i].isActive = false;
    }
    simulatorState->elementCount  = 0;
    simulatorState->connectionCount = 0;

    for ( int i = 0; i < simulatorState->discardCardCount; ++i ) {
        if ( simulatorState->handCardCount < MAX_CARDS_IN_HAND ) {
            simulatorState->userHand[simulatorState->handCardCount] = simulatorState->userDiscard[i];
            simulatorState->handCardCount++;
        }
    }
    simulatorState->discardCardCount = 0;

    Server_LoadScenario( simulatorState, simulatorState->currentScenarioId );

    TraceLog( LOG_INFO, "SERVER: Reset scenario %d", simulatorState->currentScenarioId );
}

void Server_LoadStarterScenario( SimulatorState *simulatorState ) {
    if ( simulatorState == NULL ) return;

    for ( int i = 0; i < SCENARIO_COUNT; ++i ) { simulatorState->scenarioProgression[i] = false; }

    Server_LoadScenario( simulatorState, SCENARIO_BASIC_CIRCUIT );
}

Card Server_CreateActionCard( int id, const char *name, ActionCardType actionType ) {
    Card card;
    card.id            = id;
    card.type          = CARD_TYPE_ACTION;
    card.actionType    = actionType;
    card.elementToPlace = ELEMENT_NONE;
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

bool Server_ExecuteActionCard( SimulatorState *simulatorState, ActionCardType actionType ) {
    if ( simulatorState == NULL ) return false;

    switch ( actionType ) {
        case ACTION_REQUISITION:
            for ( int i = 0; i < 3; ++i ) {
                if ( !Server_UserDrawCard( simulatorState ) ) break;
            }
            TraceLog( LOG_INFO, "SERVER: Requisition executed - drew up to 3 cards" );
            return true;

        case ACTION_RE_ORG:
            while ( simulatorState->handCardCount > 0 ) {
                if ( simulatorState->discardCardCount >= MAX_CARDS_IN_DECK ) break;
                simulatorState->userDiscard[simulatorState->discardCardCount] =
                  simulatorState->userHand[simulatorState->handCardCount - 1];
                simulatorState->discardCardCount++;
                simulatorState->handCardCount--;
            }
            while ( simulatorState->handCardCount < MAX_CARDS_IN_HAND ) {
                if ( !Server_UserDrawCard( simulatorState ) ) break;
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

void Server_Update( SimulatorState *simulatorState, float deltaTime ) {
    serverUpdateFrameCounter++;

    if ( simulatorState == NULL || simulatorState->simulationComplete ) {
        TraceLog(
          LOG_INFO, "SERVER_UPDATE_END (Frame: %u): Early exit (null or simulation complete)",
          serverUpdateFrameCounter
        );
        return;
    }

    for ( int i = 0; i < simulatorState->elementCount; ++i ) {
        if ( !simulatorState->elementsOnCanvas[i].isActive ) continue;

        CircuitElement *elem    = &simulatorState->elementsOnCanvas[i];
        elem->connectedInputCount = 0;

        for ( int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE; ++k ) {
            if ( elem->inputElementIDs[k] != -1 ) { elem->connectedInputCount++; }
        }
    }

    PropagateSignals( simulatorState );
    Server_EvaluateScenario( simulatorState );
}
