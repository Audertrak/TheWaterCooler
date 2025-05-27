#include "../src/server.h"
#include <stdio.h>
#include <string.h>

// Stub TraceLog to avoid dependencies
void TraceLog(int logLevel, const char *text, ...) {
    // No-op for testing
}

bool GetElementState(SimulatorState *state, int elementId) {
    for (int i = 0; i < state->elementCount; ++i) {
        if (state->elementsOnCanvas[i].id == elementId) {
            return state->elementsOnCanvas[i].outputState;
        }
    }
    return false;
}

void test_button_interaction() {
    printf("Testing button interaction behavior...\n");
    
    SimulatorState state;
    memset(&state, 0, sizeof(SimulatorState));
    Server_Init(&state);
    
    // Manually create a button element
    state.elementCount = 1;
    state.elementsOnCanvas[0].type = ELEMENT_BUTTON;
    state.elementsOnCanvas[0].canvasPosition = (Vector2){0, 0};
    state.elementsOnCanvas[0].isActive = true;
    state.elementsOnCanvas[0].id = 1;
    state.elementsOnCanvas[0].outputState = false;
    state.elementsOnCanvas[0].defaultOutputState = false;
    state.elementsOnCanvas[0].connectedInputCount = 0;
    
    int buttonId = 1;
    printf("Created button with ID %d\n", buttonId);
    
    printf("Initial button state: %s\n", 
           GetElementState(&state, buttonId) ? "ON" : "OFF");
    
    printf("\n=== Testing button press sequence ===\n");
    
    printf("1. Calling Server_InteractWithElement (simulating click)...\n");
    Server_InteractWithElement(&state, buttonId);
    printf("   Button state after interact: %s\n", 
           GetElementState(&state, buttonId) ? "ON" : "OFF");
    
    printf("2. Calling Server_Update (simulating frame update)...\n");
    Server_Update(&state, 0.016f);
    printf("   Button state after Server_Update: %s\n", 
           GetElementState(&state, buttonId) ? "ON" : "OFF (BUG!)");
    
    printf("3. Calling Server_InteractWithElement again (simulating continuous hold)...\n");
    Server_InteractWithElement(&state, buttonId);
    printf("   Button state after second interact: %s\n", 
           GetElementState(&state, buttonId) ? "ON" : "OFF");
    
    printf("4. Calling Server_Update again...\n");
    Server_Update(&state, 0.016f);
    printf("   Button state after second Server_Update: %s\n", 
           GetElementState(&state, buttonId) ? "ON" : "OFF (BUG!)");
    
    printf("5. Calling Server_ReleaseElementInteraction (simulating release)...\n");
    Server_ReleaseElementInteraction(&state, buttonId);
    printf("   Button state after release: %s\n", 
           GetElementState(&state, buttonId) ? "ON" : "OFF");
    
    printf("\nButton interaction test completed.\n");
}

int main() {
    test_button_interaction();
    return 0;
}
