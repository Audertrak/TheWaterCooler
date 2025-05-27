/**
 * @file test_harness.h
 * @brief Minimal test harness for Enjenir core logic (server.c).
 *
 * Provides helpers for setting up test boards, placing elements, connecting, and verifying outputs.
 */
#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include "../src/server.h"
#include <stdbool.h>

void TestHarness_SetupBoard(SimulatorState *state);
int TestHarness_PlaceElement(SimulatorState *state, ElementType type, Vector2 pos);
void TestHarness_Connect(SimulatorState *state, int fromId, int toId, int slot);
void TestHarness_SetInput(SimulatorState *state, int elementId, bool value);
bool TestHarness_VerifyOutput(SimulatorState *state, int elementId, bool expectedState);

#endif // TEST_HARNESS_H
