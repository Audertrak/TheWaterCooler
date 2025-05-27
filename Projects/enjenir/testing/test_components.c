#include "test_harness.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    SimulatorState state;
    TestHarness_SetupBoard(&state);

    int source = TestHarness_PlaceElement(&state, ELEMENT_SOURCE, (Vector2){0,0});
    int sensor = TestHarness_PlaceElement(&state, ELEMENT_SENSOR, (Vector2){1,0});
    TestHarness_Connect(&state, source, sensor, 0);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, source, true));
    // Sensor output is always false (terminal)
    assert(TestHarness_VerifyOutput(&state, sensor, false));

    int button = TestHarness_PlaceElement(&state, ELEMENT_BUTTON, (Vector2){2,0});
    TestHarness_SetInput(&state, button, true);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, button, true));
    TestHarness_SetInput(&state, button, false);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, button, false));

    int sw = TestHarness_PlaceElement(&state, ELEMENT_SWITCH, (Vector2){3,0});
    TestHarness_SetInput(&state, sw, true);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, sw, true));
    TestHarness_SetInput(&state, sw, false);
    Server_Update(&state, 0.0f);
    assert(TestHarness_VerifyOutput(&state, sw, false));

    printf("Component tests passed.\n");
    return 0;
}
