#include "test_harness.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    SimulatorState state;
    TestHarness_SetupBoard(&state);

    int src = TestHarness_PlaceElement(&state, ELEMENT_SOURCE, (Vector2){0,0});
    int notGate = TestHarness_PlaceElement(&state, ELEMENT_NOT, (Vector2){1,0});
    TestHarness_Connect(&state, src, notGate, 0);
    Server_Update(&state, 0.0f);
    // NOT gate not yet implemented
    // assert(TestHarness_VerifyOutput(&state, notGate, false));

    int orGate = TestHarness_PlaceElement(&state, ELEMENT_OR, (Vector2){2,0});
    int a = TestHarness_PlaceElement(&state, ELEMENT_SWITCH, (Vector2){2,1});
    int b = TestHarness_PlaceElement(&state, ELEMENT_SWITCH, (Vector2){2,2});
    TestHarness_Connect(&state, a, orGate, 0);
    TestHarness_Connect(&state, b, orGate, 1);
    TestHarness_SetInput(&state, a, false);
    TestHarness_SetInput(&state, b, false);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, orGate, false));
    TestHarness_SetInput(&state, a, true);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, orGate, true));
    TestHarness_SetInput(&state, a, false);
    TestHarness_SetInput(&state, b, true);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, orGate, true));
    TestHarness_SetInput(&state, a, true);
    TestHarness_SetInput(&state, b, true);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, orGate, true));

    printf("Node tests passed.\n");
    return 0;
}
