#include "client.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

typedef enum ClientInteractionMode {
  INTERACTION_MODE_NORMAL,
  INTERACTION_MODE_WIRING_SELECT_OUTPUT,
  INTERACTION_MODE_WIRING_SELECT_INPUT
} ClientInteractionMode;

typedef enum ClientScreen {
  CLIENT_SCREEN_LOADING,
  CLIENT_SCREEN_TITLE,
  CLIENT_SCREEN_GAMEPLAY
} ClientScreen;

static ClientScreen currentClientScreen = CLIENT_SCREEN_LOADING;
static Font clientFont;
static bool customFontLoaded = false;
static int framesCounter = 0;
static Camera2D gameCamera;
static int selectedCardIndex = -1;
static ClientInteractionMode interactionMode = INTERACTION_MODE_NORMAL;
static int wiringFromComponentId = -1;

static void DrawGameplayGrid(void);
static void DrawComponentsOnGrid(const GameState *gameState);
static void DrawConnections(const GameState *gameState);
static void DrawWiringPreview(void);

bool Client_Init(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
  if (!IsWindowReady()) {
    return false;
  }

  clientFont = LoadFontEx(FONT_PATH, FONT_RASTER_SIZE, NULL, 0);

  if (clientFont.texture.id == 0 ||
      (clientFont.texture.id == GetFontDefault().texture.id &&
       TextIsEqual(FONT_PATH, "") == false)) {
    TraceLog(LOG_WARNING,
             "Failed to load custom font at '%s' with size %d, using default "
             "Raylib font.",
             FONT_PATH, FONT_RASTER_SIZE);
    clientFont = GetFontDefault();
    customFontLoaded = false;
  } else {
    customFontLoaded = true;
    TraceLog(LOG_INFO, "Custom font loaded successfully: %s at size %d",
             FONT_PATH, FONT_RASTER_SIZE);
  }

  GuiSetFont(clientFont);
  GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
  GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
  GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(COLOR_TEXT_PRIMARY));
  GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

  gameCamera.target = (Vector2){0.0f, 0.0f};
  gameCamera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
  gameCamera.rotation = 0.0f;
  gameCamera.zoom = 1.0f;

  SetTargetFPS(60);
  currentClientScreen = CLIENT_SCREEN_LOADING;
  framesCounter = 0;
  selectedCardIndex = -1;
  return true;
}

void Client_Close(void) {
  if (customFontLoaded) {
    UnloadFont(clientFont);
    TraceLog(LOG_INFO, "Custom font unloaded.");
  }
  CloseWindow();
}

bool Client_ShouldClose(void) { return WindowShouldClose(); }

static void DrawLoadingScreen(void) {
  const char *loadingText = "LOADING...";
  float fontSize = 40;
  float spacing = 2;
  if (clientFont.baseSize > 0)
    spacing = fontSize / (float)clientFont.baseSize *
              GetFontDefault().recs[0].height / 10.0f;

  Vector2 textSize = MeasureTextEx(clientFont, loadingText, fontSize, spacing);
  DrawTextEx(clientFont, loadingText,
             (Vector2){SCREEN_WIDTH / 2.0f - textSize.x / 2.0f,
                       SCREEN_HEIGHT / 2.0f - textSize.y / 2.0f},
             fontSize, spacing, COLOR_TEXT_SECONDARY);
}

static void DrawTitleScreen(void) {
  const char *titleText = "ENGINEERING CARD GAME";
  const char *subtitleText = "Press [ENTER] to Start";

  float titleFontSize = 60;
  float titleSpacing = 3;
  if (clientFont.baseSize > 0)
    titleSpacing = titleFontSize / (float)clientFont.baseSize *
                   GetFontDefault().recs[0].height / 10.0f;

  float subtitleFontSize = 30;
  float subtitleSpacing = 2;
  if (clientFont.baseSize > 0)
    subtitleSpacing = subtitleFontSize / (float)clientFont.baseSize *
                      GetFontDefault().recs[0].height / 10.0f;

  Vector2 titleSize =
      MeasureTextEx(clientFont, titleText, titleFontSize, titleSpacing);
  Vector2 subtitleSize = MeasureTextEx(clientFont, subtitleText,
                                       subtitleFontSize, subtitleSpacing);

  DrawTextEx(
      clientFont, titleText,
      (Vector2){SCREEN_WIDTH / 2.0f - titleSize.x / 2.0f, SCREEN_HEIGHT / 4.0f},
      titleFontSize, titleSpacing, COLOR_TEXT_PRIMARY);
  DrawTextEx(clientFont, subtitleText,
             (Vector2){SCREEN_WIDTH / 2.0f - subtitleSize.x / 2.0f,
                       SCREEN_HEIGHT / 1.8f},
             subtitleFontSize, subtitleSpacing, COLOR_TEXT_SECONDARY);
}

static void DrawGameplayGrid(void) {
  int worldViewWidth = SCREEN_WIDTH * 4;
  int worldViewHeight = SCREEN_HEIGHT * 4;

  int startX = (int)(gameCamera.target.x -
                     gameCamera.offset.x / gameCamera.zoom - GRID_CELL_SIZE);
  startX = (startX / GRID_CELL_SIZE) * GRID_CELL_SIZE - GRID_CELL_SIZE;

  int startY = (int)(gameCamera.target.y -
                     gameCamera.offset.y / gameCamera.zoom - GRID_CELL_SIZE);
  startY = (startY / GRID_CELL_SIZE) * GRID_CELL_SIZE - GRID_CELL_SIZE;

  int endX = startX + worldViewWidth + GRID_CELL_SIZE * 2;
  int endY = startY + worldViewHeight + GRID_CELL_SIZE * 2;

  for (int x = startX; x < endX; x += GRID_CELL_SIZE) {
    DrawLine(x, startY, x, endY, COLOR_GRID_LINES);
  }
  for (int y = startY; y < endY; y += GRID_CELL_SIZE) {
    DrawLine(startX, y, endX, y, COLOR_GRID_LINES);
  }
}

static void DrawComponentsOnGrid(const GameState *gameState) {
  if (gameState == NULL)
    return;

  for (int i = 0; i < gameState->componentCount; ++i) {
    if (gameState->componentsOnGrid[i].isActive) {
      CircuitComponent comp = gameState->componentsOnGrid[i];
      Vector2 worldPos = {
          comp.gridPosition.x * GRID_CELL_SIZE + GRID_CELL_SIZE / 2.0f,
          comp.gridPosition.y * GRID_CELL_SIZE + GRID_CELL_SIZE / 2.0f};

      Rectangle compRec = {worldPos.x - GRID_CELL_SIZE / 3.0f,
                           worldPos.y - GRID_CELL_SIZE / 3.0f,
                           GRID_CELL_SIZE * 2.0f / 3.0f,
                           GRID_CELL_SIZE * 2.0f / 3.0f};

      Color compColor = COLOR_ACCENT_SECONDARY;
      const char *compText = "";

      switch (comp.type) {
      case COMP_MOMENTARY_SWITCH:
      case COMP_LATCHING_SWITCH:
        compColor = comp.outputState ? GREEN : RED;
        compText = comp.outputState ? "ON" : "OFF";
        break;
      case COMP_AND_GATE:
        compColor = comp.outputState ? SKYBLUE : DARKBLUE;
        compText = "AND";
        break;
      case COMP_OR_GATE:
        compColor = comp.outputState ? PINK : PURPLE;
        compText = "OR";
        break;
      case COMP_SOURCE:
        compColor = GOLD; // Always ON
        compText = "SRC";
        break;
      case COMP_SINK:
        compColor = DARKBROWN; // Always OFF
        compText = "SNK";
        break;
      default:
        compText = "???";
        break;
      }
      DrawRectangleRec(compRec, compColor);
      DrawRectangleLinesEx(compRec, 2, DARKGRAY);

      if (clientFont.texture.id > 0) { // Ensure font is loaded
        float compFontSize = 10;
        float compSpacing = 1;
        Vector2 textSize =
            MeasureTextEx(clientFont, compText, compFontSize, compSpacing);
        DrawTextEx(clientFont, compText,
                   (Vector2){compRec.x + (compRec.width - textSize.x) / 2,
                             compRec.y + (compRec.height - textSize.y) / 2},
                   compFontSize, compSpacing, BLACK);
      }
    }
  }
}

static void DrawGameplayScreen(const GameState *gameState) {
  Rectangle headerArea = {0, 0, SCREEN_WIDTH, UI_HEADER_HEIGHT};
  Rectangle deckArea = {0, SCREEN_HEIGHT - UI_DECK_AREA_HEIGHT, SCREEN_WIDTH,
                        UI_DECK_AREA_HEIGHT};
  Rectangle playArea = {0, UI_HEADER_HEIGHT, SCREEN_WIDTH,
                        SCREEN_HEIGHT - UI_HEADER_HEIGHT - UI_DECK_AREA_HEIGHT};

  gameCamera.offset = (Vector2){playArea.x + playArea.width / 2.0f,
                                playArea.y + playArea.height / 2.0f};

  BeginScissorMode((int)playArea.x, (int)playArea.y, (int)playArea.width,
                   (int)playArea.height);
  BeginMode2D(gameCamera);
  DrawGameplayGrid();
  DrawComponentsOnGrid(gameState);
  DrawConnections(gameState); // Draw established connections

  if (interactionMode == INTERACTION_MODE_WIRING_SELECT_INPUT &&
      wiringFromComponentId != -1) {
    CircuitComponent *fromComp = NULL;
    for (int i = 0; i < gameState->componentCount; ++i) {
      if (gameState->componentsOnGrid[i].id == wiringFromComponentId) {
        fromComp = &gameState->componentsOnGrid[i];
        break;
      }
    }
    if (fromComp) {
      Vector2 startPos = GetWorldPositionForGrid(fromComp->gridPosition);
      Vector2 mouseWorldPos =
          GetScreenToWorld2D(GetMousePosition(), gameCamera);
      DrawLineEx(startPos, mouseWorldPos, 2.0f,
                 Fade(COLOR_ACCENT_PRIMARY, 0.7f));
    }
  }
  EndMode2D();
  EndScissorMode();

  DrawRectangleRec(headerArea, COLOR_UI_AREA_BG_HEADER);
  DrawRectangleLinesEx(headerArea, GRID_LINE_THICKNESS, COLOR_UI_AREA_BORDER);
  DrawTextEx(clientFont, "Scenario: Build a Toggle Switch",
             (Vector2){headerArea.x + UI_PADDING,
                       headerArea.y + (UI_HEADER_HEIGHT - 30) / 2.0f},
             30, 2, COLOR_TEXT_PRIMARY);

  DrawRectangleRec(deckArea, COLOR_UI_AREA_BG_DECK);
  DrawRectangleLinesEx(deckArea, GRID_LINE_THICKNESS, COLOR_UI_AREA_BORDER);

  float currentCardX = deckArea.x + UI_PADDING;
  float handLabelY = deckArea.y + UI_PADDING;
  DrawTextEx(clientFont, "Hand:", (Vector2){currentCardX, handLabelY}, 20, 1,
             COLOR_TEXT_PRIMARY);
  if (interactionMode == INTERACTION_MODE_WIRING_SELECT_OUTPUT) {
    DrawTextEx(clientFont, "WIRING: Select Output",
               (Vector2){currentCardX + 100, handLabelY}, 20, 1,
               COLOR_ACCENT_PRIMARY);
  } else if (interactionMode == INTERACTION_MODE_WIRING_SELECT_INPUT) {
    DrawTextEx(
        clientFont,
        TextFormat("WIRING: From %d, Select Input", wiringFromComponentId),
        (Vector2){currentCardX + 100, handLabelY}, 20, 1, COLOR_ACCENT_PRIMARY);
  }

  float cardAreaY = handLabelY + 20 + UI_PADDING;
  int previousLabelTextSize = GuiGetStyle(LABEL, TEXT_SIZE);
  int previousLabelAlignment = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
  GuiSetStyle(LABEL, TEXT_SIZE, CARD_TEXT_SIZE);
  GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
  for (int i = 0; i < gameState->handCardCount; ++i) { /* ... */
  } // Card drawing loop remains same
  GuiSetStyle(LABEL, TEXT_SIZE, previousLabelTextSize);
  GuiSetStyle(LABEL, TEXT_ALIGNMENT, previousLabelAlignment);
  DrawTextEx(clientFont,
             TextFormat("Deck: %d | Discard: %d",
                        gameState->deckCardCount - gameState->currentDeckIndex,
                        gameState->discardCardCount),
             (Vector2){deckArea.x + UI_PADDING,
                       deckArea.y + UI_DECK_AREA_HEIGHT - 20 - UI_PADDING},
             20, 1, COLOR_TEXT_SECONDARY);
  DrawTextEx(clientFont, TextFormat("Score: %d", gameState->score),
             (Vector2){SCREEN_WIDTH - 200, UI_HEADER_HEIGHT + UI_PADDING}, 20,
             1, COLOR_TEXT_PRIMARY);
  DrawTextEx(
      clientFont, TextFormat("Zoom: %.2fx", gameCamera.zoom),
      (Vector2){SCREEN_WIDTH - 200, UI_HEADER_HEIGHT + UI_PADDING + 20 + 5}, 20,
      1, COLOR_TEXT_SECONDARY);
  DrawTextEx(clientFont,
             TextFormat("Target: (%.0f, %.0f)", gameCamera.target.x,
                        gameCamera.target.y),
             (Vector2){SCREEN_WIDTH - 200,
                       UI_HEADER_HEIGHT + UI_PADDING + (20 + 5) * 2},
             20, 1, COLOR_TEXT_SECONDARY);
}

static Vector2 GetWorldPositionForGrid(Vector2 gridPos) {
  return (Vector2){gridPos.x * GRID_CELL_SIZE + GRID_CELL_SIZE / 2.0f,
                   gridPos.y * GRID_CELL_SIZE + GRID_CELL_SIZE / 2.0f};
}

static void DrawConnections(const GameState *gameState) {
  if (gameState == NULL)
    return;
  for (int i = 0; i < gameState->connectionCount; ++i) {
    if (gameState->connections[i].isActive) {
      Connection conn = gameState->connections[i];
      CircuitComponent *fromComp = NULL;
      CircuitComponent *toComp = NULL;

      for (int j = 0; j < gameState->componentCount; ++j) {
        if (gameState->componentsOnGrid[j].isActive) {
          if (gameState->componentsOnGrid[j].id == conn.fromComponentId) {
            fromComp = &gameState->componentsOnGrid[j];
          }
          if (gameState->componentsOnGrid[j].id == conn.toComponentId) {
            toComp = &gameState->componentsOnGrid[j];
          }
        }
      }

      if (fromComp && toComp) {
        Vector2 startPos = GetWorldPositionForGrid(fromComp->gridPosition);
        Vector2 endPos = GetWorldPositionForGrid(toComp->gridPosition);
        STUB("For components with multiple inputs/outputs, adjust start/end "
             "points For now, connect centers");
        DrawLineEx(startPos, endPos, 2.0f, COLOR_TEXT_PRIMARY);
      }
    }
  }
}

static void DrawWiringPreview(void) {
  if (interactionMode == INTERACTION_MODE_WIRING_SELECT_INPUT &&
      wiringFromComponentId != -1) {
    Vector2 mouseScreenPos = GetMousePosition();
    DrawLineEx((Vector2){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}, mouseScreenPos,
               2.0f, Fade(COLOR_ACCENT_PRIMARY, 0.7f));
  }
}

void Client_UpdateAndDraw(GameState *gameState) {
  switch (currentClientScreen) {
  case CLIENT_SCREEN_LOADING:
    framesCounter++;
    if (framesCounter > 120) {
      currentClientScreen = CLIENT_SCREEN_TITLE;
    }
    break;
  case CLIENT_SCREEN_TITLE:
    if (IsKeyPressed(KEY_ENTER)) {
      currentClientScreen = CLIENT_SCREEN_GAMEPLAY;
    }
    break;
  case CLIENT_SCREEN_GAMEPLAY: {
    if (IsKeyPressed(KEY_ESCAPE)) {
      currentClientScreen = CLIENT_SCREEN_TITLE;
      interactionMode = INTERACTION_MODE_NORMAL;
      wiringFromComponentId = -1;
      selectedCardIndex = -1;
    }
    if (IsKeyPressed(KEY_D)) { /* ... */
    }
    if (IsKeyPressed(KEY_W)) { // 'W' to enter/exit wiring mode
      if (interactionMode == INTERACTION_MODE_NORMAL) {
        interactionMode = INTERACTION_MODE_WIRING_SELECT_OUTPUT;
        selectedCardIndex = -1; // Deselect card if entering wiring mode
        TraceLog(LOG_INFO, "CLIENT: Entered Wiring Mode - Select Output.");
      } else {
        interactionMode = INTERACTION_MODE_NORMAL;
        wiringFromComponentId = -1;
        TraceLog(LOG_INFO, "CLIENT: Exited Wiring Mode.");
      }
    }

    Rectangle playArea = {0, UI_HEADER_HEIGHT, SCREEN_WIDTH,
                          SCREEN_HEIGHT - UI_HEADER_HEIGHT -
                              UI_DECK_AREA_HEIGHT};
    Rectangle deckArea = {0, SCREEN_HEIGHT - UI_DECK_AREA_HEIGHT, SCREEN_WIDTH,
                          UI_DECK_AREA_HEIGHT};
    Vector2 mousePosition = GetMousePosition();

    if (CheckCollisionPointRec(mousePosition, playArea)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) { /* ... camera pan ... */
      }
      float wheel = GetMouseWheelMove();
      if (wheel != 0) { /* ... camera zoom ... */
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (interactionMode == INTERACTION_MODE_NORMAL) {
        if (CheckCollisionPointRec(mousePosition,
                                   deckArea)) { /* ... card selection ... */
        } else if (CheckCollisionPointRec(mousePosition, playArea)) {
          Vector2 worldMousePos = GetScreenToWorld2D(mousePosition, gameCamera);
          Vector2 gridPos = {floorf(worldMousePos.x / GRID_CELL_SIZE),
                             floorf(worldMousePos.y / GRID_CELL_SIZE)};
          if (selectedCardIndex != -1) { /* ... component placement ... */
          } else {                       /* ... component interaction ... */
          }
        }
      } else if (interactionMode == INTERACTION_MODE_WIRING_SELECT_OUTPUT) {
        if (CheckCollisionPointRec(mousePosition, playArea)) {
          Vector2 worldMousePos = GetScreenToWorld2D(mousePosition, gameCamera);
          Vector2 gridPos = {floorf(worldMousePos.x / GRID_CELL_SIZE),
                             floorf(worldMousePos.y / GRID_CELL_SIZE)};
          int clickedCompId = -1;
          for (int i = 0; i < gameState->componentCount; ++i) {
            if (gameState->componentsOnGrid[i].isActive &&
                (int)gameState->componentsOnGrid[i].gridPosition.x ==
                    (int)gridPos.x &&
                (int)gameState->componentsOnGrid[i].gridPosition.y ==
                    (int)gridPos.y) {
              clickedCompId = gameState->componentsOnGrid[i].id;
              break;
            }
          }
          if (clickedCompId != -1) {
            wiringFromComponentId = clickedCompId;
            interactionMode = INTERACTION_MODE_WIRING_SELECT_INPUT;
            TraceLog(LOG_INFO,
                     "CLIENT: Wiring - Output selected from component ID %d. "
                     "Select Target Input.",
                     wiringFromComponentId);
          }
        }
      } else if (interactionMode == INTERACTION_MODE_WIRING_SELECT_INPUT) {
        if (CheckCollisionPointRec(mousePosition, playArea)) {
          Vector2 worldMousePos = GetScreenToWorld2D(mousePosition, gameCamera);
          Vector2 gridPos = {floorf(worldMousePos.x / GRID_CELL_SIZE),
                             floorf(worldMousePos.y / GRID_CELL_SIZE)};
          int clickedCompId = -1;
          CircuitComponent *targetComp = NULL;
          for (int i = 0; i < gameState->componentCount; ++i) {
            if (gameState->componentsOnGrid[i].isActive &&
                (int)gameState->componentsOnGrid[i].gridPosition.x ==
                    (int)gridPos.x &&
                (int)gameState->componentsOnGrid[i].gridPosition.y ==
                    (int)gridPos.y) {
              clickedCompId = gameState->componentsOnGrid[i].id;
              targetComp = &gameState->componentsOnGrid[i];
              break;
            }
          }
          if (clickedCompId != -1 && clickedCompId != wiringFromComponentId) {
            int targetInputSlot = -1;
            if (targetComp->type == COMP_AND_GATE ||
                targetComp->type == COMP_OR_GATE) {
              // Simple: try to connect to the first available input slot
              for (int s = 0; s < MAX_INPUTS_PER_LOGIC_GATE; ++s) {
                if (targetComp->inputComponentIDs[s] == -1) {
                  targetInputSlot = s;
                  break;
                }
              }
            }
            if (targetInputSlot != -1) {
              if (Server_CreateConnection(gameState, wiringFromComponentId,
                                          clickedCompId, targetInputSlot)) {
                TraceLog(LOG_INFO,
                         "CLIENT: Connection attempt sent to server.");
              } else {
                TraceLog(LOG_WARNING,
                         "CLIENT: Server_CreateConnection failed.");
              }
            } else {
              TraceLog(LOG_INFO, "CLIENT: Target component has no available "
                                 "input slots or is not a gate.");
            }
          } else if (clickedCompId == wiringFromComponentId) {
            TraceLog(LOG_INFO, "CLIENT: Cannot connect component to itself.");
          }
          interactionMode =
              INTERACTION_MODE_NORMAL; // Exit wiring mode after attempt
          wiringFromComponentId = -1;
        } else { // Clicked outside play area while wiring
          interactionMode = INTERACTION_MODE_NORMAL;
          wiringFromComponentId = -1;
          TraceLog(LOG_INFO,
                   "CLIENT: Wiring cancelled (clicked outside play area).");
        }
      }
    }
  } break;
  default:
    break;
  }

  BeginDrawing();
  ClearBackground(COLOR_BACKGROUND);

  switch (currentClientScreen) {
  case CLIENT_SCREEN_LOADING:
    DrawLoadingScreen();
    break;
  case CLIENT_SCREEN_TITLE:
    DrawTitleScreen();
    break;
  case CLIENT_SCREEN_GAMEPLAY:
    if (gameState != NULL) {
      DrawGameplayScreen(gameState);
    } else {
      DrawTextEx(clientFont, "Error: GameState is NULL", (Vector2){20, 20}, 20,
                 1, RED);
    }
    break;
  default:
    DrawTextEx(clientFont, "UNKNOWN CLIENT SCREEN", (Vector2){20, 20}, 20, 1,
               RED);
    break;
  }

  DrawFPS(SCREEN_WIDTH - 100, UI_PADDING);
  EndDrawing();
}
