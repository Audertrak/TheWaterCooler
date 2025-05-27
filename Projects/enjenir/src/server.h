#ifndef SERVER_H
#define SERVER_H

#include "raylib.h"    // For Vector2, Color, TraceLog etc.
#include <stdbool.h>

// In server.h
#if defined( PLATFORM_WEB )    // For full Raylib web app build
  #include "raylib.h"
// Potentially include raymath.h if server logic uses it directly
// #include "raymath.h"
#elif defined( TOOL_WASM_BUILD )    // For headless server logic WASM
typedef struct Vector2 {
    float x;
    float y;
} Vector2;

  #define TraceLog( logLevel, ... )
  #define LOG_INFO    0
  #define LOG_WARNING 1
// Define other minimal types/stubs if server.c needs them without full Raylib
#else    // For native desktop build
  #include "raylib.h"
  #include "raymath.h"
#endif

#define MAX_ELEMENTS_ON_CANVAS                                               \
    100    ///< Maximum number of elements that can be placed on the canvas.
#define MAX_CARDS_IN_HAND         10    ///< Maximum number of cards a user can hold.
#define MAX_CARDS_IN_DECK         60    ///< Maximum number of cards in a deck.
#define MAX_INPUTS_PER_LOGIC_GATE 5     ///< Max inputs for complex gates like MUX (5 inputs)
#define MAX_OUTPUTS_PER_BUS       4     ///< Max outputs for bus element (quad output)
#define MAX_CONNECTIONS           MAX_ELEMENTS_ON_CANVAS *MAX_INPUTS_PER_LOGIC_GATE    // Theoretical max

// --- Element Definitions ---

/**
 * @brief Defines the types of circuit elements available in the simulator.
 * Elements are categorized as components (simple input/output), nodes (processing), or modules (complex).
 */
typedef enum ElementType {
    ELEMENT_NONE = 0,         ///< No element / empty slot.
    
    // Basic Components (origin or terminal points)
    ELEMENT_SOURCE,           ///< Single output, always outputs signal.
    ELEMENT_BUTTON,           ///< Single output, outputs signal when clicked.
    ELEMENT_SWITCH,           ///< Single output, toggled by user.
    ELEMENT_SENSOR,           ///< Single input, triggered by signal, adds to capability.
    
    // Advanced Components
    ELEMENT_SEQUENCER,        ///< Single input, user-editable sequence, plays back on signal.
    
    // Basic Nodes (input and output processing)
    ELEMENT_NOT,              ///< Single input/output, inverts signal.
    ELEMENT_AND,              ///< Double input, single output, outputs if both inputs active.
    ELEMENT_OR,               ///< Double input, single output, outputs if any input active.
    ELEMENT_BUS,              ///< Single input, quad output, repeats input signal.
    
    // Advanced Nodes
    ELEMENT_FLIP_FLOP,        ///< Single input/output, toggles state on signal.
    ELEMENT_MUX,              ///< 5 inputs, single output, one input is "select".
    ELEMENT_TAPE,             ///< Like sequencer, but single input/output.
    
    ELEMENT_TYPE_COUNT        ///< Total number of defined element types.
} ElementType;

/**
 * @brief Represents a single circuit element placed on the simulator canvas.
 */
typedef struct CircuitElement {
    ElementType type;                    ///< The type of this element.
    Vector2     canvasPosition;          ///< Logical (x, y) position on the canvas.
    bool        outputState;             ///< Current boolean output state of the element.
    bool        defaultOutputState;      ///< Default output state, primarily for switches.
    bool        isActive;                ///< True if this element slot is in use on the canvas.
    int         id;                      ///< Unique identifier for this element instance.
    int         inputElementIDs[MAX_INPUTS_PER_LOGIC_GATE];    ///< IDs of elements
                                                               ///< providing input. -1 if
                                                               ///< not connected.
    bool        actualInputStates[MAX_INPUTS_PER_LOGIC_GATE];  ///< The actual boolean
                                                               ///< state received from
                                                               ///< inputElementIDs.
    int         connectedInputCount;                           ///< Number of connected inputs
    // float timer;             ///< General purpose timer (seconds)
    // bool timerActive;        ///< Whether the timer is currently running
    // float timerDuration;     ///< How long the timer should run (seconds)
} CircuitElement;

/**
 * @brief Represents a connection between two elements.
 */
typedef struct Connection {
    int  fromElementId;      ///< ID of the element outputting the signal.
    int  toElementId;        ///< ID of the element receiving the signal.
    int  toInputSlot;        ///< Which input slot on the toElement (0, 1, etc.).
    bool isActive;           ///< Is this connection slot in use?
} Connection;

/**
 * @brief Defines different types of scenario conditions that can be checked.
 */
typedef enum ScenarioConditionType {
    CONDITION_MIN_ELEMENTS = 0,      ///< Minimum number of specific element types
    CONDITION_MAX_ELEMENTS,          ///< Maximum number of specific element types
    CONDITION_MIN_UNIQUE_STATES,     ///< Minimum number of unique output states
    CONDITION_MAX_UNIQUE_STATES,     ///< Maximum number of unique output states
    CONDITION_SPECIFIC_STATE,        ///< Require a specific output state pattern
    CONDITION_TYPE_COUNT             ///< Total number of condition types
} ScenarioConditionType;

/**
 * @brief Predefined scenario IDs for the progression system.
 */
typedef enum ScenarioId {
    SCENARIO_BASIC_CIRCUIT = 0,    ///< Tutorial: place basic elements
    SCENARIO_SIMPLE_LOGIC,         ///< Build a working AND gate circuit
    SCENARIO_TOGGLE_SWITCH,        ///< Create a toggle using latching switch
    SCENARIO_MULTI_INPUT,          ///< Use multiple inputs with OR gate
    SCENARIO_COMPLEX_LOGIC,        ///< Combine AND and OR gates
    SCENARIO_COUNT                 ///< Total number of scenarios
} ScenarioId;

/**
 * @brief Represents a single condition that must be met to complete a scenario.
 */
typedef struct ScenarioCondition {
    ScenarioConditionType type;                ///< Type of condition to check
    ElementType           elementType;         ///< Element type for element-based conditions
    int                   targetValue;         ///< Target count or value for the condition
    bool                  isMet;               ///< Whether this condition is currently satisfied
    char                  description[128];    ///< Human-readable description of the condition
} ScenarioCondition;

/**
 * @brief Defines a complete scenario with multiple conditions and metadata.
 */
typedef struct Scenario {
    char              name[64];            ///< Display name of the scenario
    char              description[256];    ///< Detailed description of the scenario goals
    ScenarioCondition conditions[8];       ///< Array of conditions that must be met
    int               conditionCount;      ///< Number of active conditions in this scenario
    bool              isCompleted;         ///< Whether all conditions have been met
    int               rewardScore;         ///< Score awarded for completing this scenario
} Scenario;

// --- Card System Definitions ---

/**
 * @brief Defines the types of cards available in the resource deck.
 */
typedef enum CardType {
    CARD_TYPE_ELEMENT = 0,    ///< Persistent element card (components, nodes, modules)
    CARD_TYPE_ACTION,         ///< Consumable action/effect card
    CARD_TYPE_COUNT           ///< Total number of card types
} CardType;

/**
 * @brief Defines the specific action/effect types for action cards.
 */
typedef enum ActionCardType {
    ACTION_REQUISITION = 0,         ///< Draw 3 cards
    ACTION_RECYCLE,                 ///< Discard any number of cards, draw that many
    ACTION_RE_ORG,                  ///< Discard hand, draw to full
    ACTION_JOB_FAIR,                ///< Pick 1 of 3 resources to add to deck
    ACTION_CONTINUOUS_IMPROVEMENT,  ///< Add input/output to element
    ACTION_END_OF_LIFE,             ///< Remove a card from hand permanently
    ACTION_PARTS_BIN,               ///< Duplicate an element in play
    ACTION_BLUEPRINT,               ///< Copy and replay modules/nodes
    ACTION_SCHEMATIC,               ///< Copy and replay modules/nodes
    ACTION_TYPE_COUNT               ///< Total number of action types
} ActionCardType;

/**
 * @brief Represents a single card definition.
 */
typedef struct Card {
    CardType       type;                ///< The general type of this card.
    char           name[64];            ///< Display name of the card.
    char           description[128];    ///< Flavor text or rules text for the card.
    ElementType    elementToPlace;      ///< If CARD_TYPE_ELEMENT, the ElementType it places.
    int            id;                  ///< Unique identifier for this card definition.
    ActionCardType actionType;          ///< If CARD_TYPE_ACTION, the specific action it performs
} Card;

/**
 * @brief Holds the entire state of the simulator logic.
 * This structure is managed by the "server" module.
 */
typedef struct SimulatorState {
    CircuitElement   elementsOnCanvas[MAX_ELEMENTS_ON_CANVAS];    ///< Array of all elements on
                                                                  ///< the canvas.
    int              elementCount;       ///< Number of active elements currently on the canvas.
    int              nextElementId;      ///< Counter for assigning unique IDs to new elements.
    Connection       connections[MAX_CONNECTIONS];     ///< Array of all connections.
    int              connectionCount;                  ///< Number of active connections.
    Card             userHand[MAX_CARDS_IN_HAND];      ///< Cards currently in the user's hand.
    int              handCardCount;                    ///< Number of cards in the user's hand.
    Card             userDeck[MAX_CARDS_IN_DECK];      ///< Cards currently in the user's draw
                                                       ///< pile.
    int              deckCardCount;      ///< Total number of cards currently in the draw pile.
    int              currentDeckIndex;   ///< Index of the next card to be drawn from userDeck.
    Card             userDiscard[MAX_CARDS_IN_DECK];   ///< Cards in the user's discard pile.
    int              discardCardCount;                 ///< Number of cards in the discard pile.
    int              score;              ///< User's current score.
    bool             simulationComplete; ///< Flag indicating if the simulation has ended.
    Scenario         currentScenario;    ///< The scenario the user is currently working on
    int              currentScenarioId;  ///< ID of the currently active scenario
    bool scenarioProgression[SCENARIO_COUNT];    ///< Track which scenarios have been completed
} SimulatorState;

/**
 * @brief Initializes the simulator state to its starting conditions.
 * @param simulatorState Pointer to the SimulatorState struct to be initialized.
 */
void Server_Init( SimulatorState *simulatorState );

/**
 * @brief Updates the simulator state based on elapsed time and internal logic.
 * @param simulatorState Pointer to the SimulatorState struct to be updated.
 * @param deltaTime Time elapsed since the last frame, in seconds.
 */
void Server_Update( SimulatorState *simulatorState, float deltaTime );

/**
 * @brief Processes a card used from the user's hand.
 * Moves the card to the discard pile and updates hand/deck counts.
 * @param simulatorState Pointer to the SimulatorState struct.
 * @param handIndex The index of the card in the user's hand to be used.
 * @return True if the card was successfully used, false otherwise.
 */
bool Server_UseCardFromHand( SimulatorState *simulatorState, int handIndex );

/**
 * @brief Handles user interaction with an element on the canvas.
 * For example, toggling a switch.
 * @param simulatorState Pointer to the SimulatorState struct.
 * @param elementId The unique ID of the element to interact with.
 */
void Server_InteractWithElement( SimulatorState *simulatorState, int elementId );

void Server_ReleaseElementInteraction( SimulatorState *simulatorState, int elementId );

/**
 * @brief Allows the user to attempt to draw a card from their deck.
 * If the deck is empty, it will attempt to reshuffle the discard pile.
 * @param simulatorState Pointer to the SimulatorState struct.
 * @return True if a card was successfully drawn into the hand, false otherwise.
 */
bool Server_UserDrawCard( SimulatorState *simulatorState );

/**
 * @brief Attempts to create a connection between two elements.
 * @param simulatorState Pointer to the SimulatorState.
 * @param fromElementId ID of the source element.
 * @param toElementId ID of the target element.
 * @param toInputSlot The input slot on the target element to connect to.
 * @return True if the connection was successfully made, false otherwise.
 */
bool Server_CreateConnection(
  SimulatorState *simulatorState, int fromElementId, int toElementId, int toInputSlot
);

/**
 * @brief Initializes a scenario with specific conditions.
 * @param scenario Pointer to the scenario to initialize
 * @param name Display name for the scenario
 * @param description Detailed description of the scenario
 */
void Server_InitScenario( Scenario *scenario, const char *name, const char *description );

/**
 * @brief Adds a condition to a scenario.
 * @param scenario Pointer to the scenario
 * @param type Type of condition to add
 * @param elementType Element type for element-based conditions (use ELEMENT_NONE if not
 * applicable)
 * @param targetValue Target value for the condition
 * @param description Human-readable description of the condition
 * @return True if condition was added successfully, false if scenario is full
 */
bool Server_AddScenarioCondition(
  Scenario *scenario, ScenarioConditionType type, ElementType elementType, int targetValue,
  const char *description
);

/**
 * @brief Evaluates all conditions in the current scenario and updates completion status.
 * @param simulatorState Pointer to the simulator state
 */
void Server_EvaluateScenario( SimulatorState *simulatorState );

/**
 * @brief Loads a predefined starter scenario for new users.
 * @param simulatorState Pointer to the simulator state
 */
void Server_LoadStarterScenario( SimulatorState *simulatorState );

/**
 * @brief Loads a specific scenario by ID into the simulator state.
 * @param simulatorState Pointer to the simulator state
 * @param scenarioId The ID of the scenario to load
 */
void Server_LoadScenario( SimulatorState *simulatorState, ScenarioId scenarioId );

/**
 * @brief Advances to the next scenario if the current one is completed.
 * @param simulatorState Pointer to the simulator state
 * @return True if advanced to next scenario, false if no more scenarios or current not completed
 */
bool Server_AdvanceToNextScenario( SimulatorState *simulatorState );

/**
 * @brief Resets the current scenario, clearing all placed elements and restoring hand.
 * @param simulatorState Pointer to the simulator state
 */
void Server_ResetCurrentScenario( SimulatorState *simulatorState );

/**
 * @brief Creates an action card with the specified type and properties.
 * @param id Unique identifier for the card
 * @param name Display name of the card
 * @param actionType The specific action this card performs
 * @return The created action card
 */
Card Server_CreateActionCard( int id, const char *name, ActionCardType actionType );

/**
 * @brief Executes the effect of an action card.
 * @param simulatorState Pointer to the simulator state
 * @param actionType The type of action to execute
 * @return True if the action was executed successfully, false otherwise
 */
bool Server_ExecuteActionCard( SimulatorState *simulatorState, ActionCardType actionType );

#endif    // SERVER_H
