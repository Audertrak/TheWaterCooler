#include "test_harness.h"
#include <string.h>

void TestHarness_SetupBoard(SimulatorState *state) {
    memset(state, 0, sizeof(SimulatorState));
    Server_Init(state);
    state->elementCount = 0;
}

int TestHarness_PlaceElement(SimulatorState *state, ElementType type, Vector2 pos) {
    if (state->elementCount >= MAX_ELEMENTS_ON_CANVAS) return -1;
    int id = state->nextElementId++;
    CircuitElement *elem = &state->elementsOnCanvas[state->elementCount++];
    elem->type = type;
    elem->canvasPosition = pos;
    elem->isActive = true;
    elem->id = id;
    elem->outputState = false;
    elem->defaultOutputState = false;
    elem->connectedInputCount = 0;
    for (int i = 0; i < MAX_INPUTS_PER_LOGIC_GATE; ++i) {
        elem->inputElementIDs[i] = -1;
        elem->actualInputStates[i] = false;
    }
    return id;
}

void TestHarness_Connect(SimulatorState *state, int fromId, int toId, int slot) {
    Server_CreateConnection(state, fromId, toId, slot);
}

void TestHarness_SetInput(SimulatorState *state, int elementId, bool value) {
    for (int i = 0; i < state->elementCount; ++i) {
        if (state->elementsOnCanvas[i].id == elementId) {
            state->elementsOnCanvas[i].outputState = value;
            break;
        }
    }
}

bool TestHarness_VerifyOutput(SimulatorState *state, int elementId, bool expectedState) {
    for (int i = 0; i < state->elementCount; ++i) {
        if (state->elementsOnCanvas[i].id == elementId) {
            return state->elementsOnCanvas[i].outputState == expectedState;
        }
    }
    return false;
}
