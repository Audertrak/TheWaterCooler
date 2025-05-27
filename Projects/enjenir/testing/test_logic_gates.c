#include "test_harness.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    SimulatorState state;
    TestHarness_SetupBoard(&state);

    int switchA = TestHarness_PlaceElement(&state, ELEMENT_SWITCH, (Vector2){0,0});
    int switchB = TestHarness_PlaceElement(&state, ELEMENT_SWITCH, (Vector2){0,1});
    int andGate = TestHarness_PlaceElement(&state, ELEMENT_AND, (Vector2){1,0});
    TestHarness_Connect(&state, switchA, andGate, 0);
    TestHarness_Connect(&state, switchB, andGate, 1);

    // Test AND gate truth table
    TestHarness_SetInput(&state, switchA, false);
    TestHarness_SetInput(&state, switchB, false);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, andGate, false));

    TestHarness_SetInput(&state, switchA, true);
    TestHarness_SetInput(&state, switchB, false);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, andGate, false));

    TestHarness_SetInput(&state, switchA, false);
    TestHarness_SetInput(&state, switchB, true);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, andGate, false));

    TestHarness_SetInput(&state, switchA, true);
    TestHarness_SetInput(&state, switchB, true);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, andGate, true));

    printf("AND gate tests passed.\n");
    return 0;
}
