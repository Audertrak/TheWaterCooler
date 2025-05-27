/**
 * @file client.h
 * @brief Client-side UI and rendering interface for Enjenir.
 *
 * Handles initialization, main update/draw loop, and shutdown for the client (UI) portion.
 * All rendering and user input is managed here. Uses Raylib/Raygui for all graphics and input.
 *
 * The client interacts with the core logic via the SimulatorState structure defined in server.h.
 *
 * @see server.h
 */
#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include <stdbool.h>

/**
 * @brief Initialize the client UI and resources.
 * @return true on success, false on failure.
 */
bool Client_Init(void);

/**
 * @brief Update the client state and render the UI.
 * @param simulatorState Pointer to the current SimulatorState structure.
 */
void Client_UpdateAndDraw(SimulatorState *simulatorState);

/**
 * @brief Release all client resources and close the UI.
 */
void Client_Close(void);

/**
 * @brief Check if the client window should close.
 * @return true if the window should close, false otherwise.
 */
bool Client_ShouldClose(void);

/**
 * @brief Enumeration of all client UI screens.
 */
typedef enum ClientScreen {
  CLIENT_SCREEN_LOADING,           /**< Loading screen */
  CLIENT_SCREEN_TITLE,             /**< Title screen */
  CLIENT_SCREEN_SCENARIO_DETAILS,  /**< Scenario details screen */
  CLIENT_SCREEN_SIMULATION         /**< Main system construction screen */
} ClientScreen;

/**
 * @brief Get the current client UI screen.
 * @return The current ClientScreen value.
 */
ClientScreen Client_GetCurrentScreen(void);

#endif // CLIENT_H
