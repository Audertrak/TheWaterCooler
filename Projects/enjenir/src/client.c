#include "client.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include "server.h"
#include <stddef.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

typedef enum ClientInteractionMode {
  INTERACTION_MODE_NORMAL,
  INTERACTION_MODE_WIRING_SELECT_OUTPUT,
  INTERACTION_MODE_WIRING_SELECT_INPUT
} ClientInteractionMode;

static bool                  gameplayHasLoggedEntry = false;
static ClientScreen          currentClientScreen    = CLIENT_SCREEN_LOADING;
static Font                  clientFont;
static bool                  customFontLoaded = false;
static int                   framesCounter    = 0;
static Camera2D              gameCamera;
static int                   selectedCardIndex     = -1;
static ClientInteractionMode interactionMode       = INTERACTION_MODE_NORMAL;
static int                   wiringFromElementId = -1;
static int                   heldButtonId = -1;
static float                 handScrollOffset      = 0.0f;
static bool                  turnInProgress        = true;
static int                   actionsThisTurn       = 0;
static const int             maxActionsPerTurn     = 3;

static Rectangle          GetUIButtonBarRect(float screenWidth, float screenHeight);
static bool               DrawUIButton(Rectangle rect, const char* label, Color bg, Color fg);
static void               DrawTouchUIAndHandle(SimulatorState* simulatorState);
static void               DrawGameplayGrid(void);
static void               DrawComponentsOnGrid(const SimulatorState *simulatorState);
static void               DrawConnections(const SimulatorState *simulatorState);
static void               DrawScenarioDetailsScreen(const SimulatorState *simulatorState);
static void               HandleGameplayInput(SimulatorState *simulatorState);

static Vector2               GetWorldPositionForGrid(Vector2 gridPos) {
  return (Vector2){gridPos.x * GRID_CELL_SIZE + GRID_CELL_SIZE / 2.0f,
                                 gridPos.y * GRID_CELL_SIZE + GRID_CELL_SIZE / 2.0f};
}

ClientScreen Client_GetCurrentScreen(void) { return currentClientScreen; }

bool         Client_Init(void) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
  if (!IsWindowReady()) { return false; }

  clientFont = LoadFontEx(FONT_PATH, FONT_RASTER_SIZE, NULL, 0);

  if (clientFont.texture.id == 0 ||
      (clientFont.texture.id == GetFontDefault().texture.id && TextIsEqual(FONT_PATH, "") == false)) {
    TraceLog(
      LOG_WARNING,
      "Failed to load custom font at '%s' with size %d, using default "
              "Raylib font.",
      FONT_PATH, FONT_RASTER_SIZE
    );
    clientFont       = GetFontDefault();
    customFontLoaded = false;
  } else {
    customFontLoaded = true;
    TraceLog(LOG_INFO, "Custom font loaded successfully: %s at size %d", FONT_PATH, FONT_RASTER_SIZE);
  }

  GuiSetFont(clientFont);
  GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
  GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
  GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(COLOR_TEXT_PRIMARY));
  GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

  gameCamera.target   = (Vector2){0.0f, 0.0f};
  gameCamera.offset   = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
  gameCamera.rotation = 0.0f;
  gameCamera.zoom     = 1.0f;

  SetTargetFPS(60);
  currentClientScreen   = CLIENT_SCREEN_LOADING;
  framesCounter         = 0;
  selectedCardIndex     = -1;
  interactionMode       = INTERACTION_MODE_NORMAL;
  wiringFromElementId = -1;
  return true;
}

void Client_Close(void) {
  if (customFontLoaded) {
    UnloadFont(clientFont);
    TraceLog(LOG_INFO, "Custom font unloaded.");
  }
  CloseWindow();
}

bool        Client_ShouldClose(void) { return WindowShouldClose(); }

static void DrawLoadingScreen(void) {
  const char *loadingText = "LOADING...";
  float       fontSize    = 40;
  float       spacing     = 2;
  if (clientFont.baseSize > 0)
    spacing = fontSize / (float)clientFont.baseSize * GetFontDefault().recs[0].height / 10.0f;

  Vector2 textSize = MeasureTextEx(clientFont, loadingText, fontSize, spacing);
  DrawTextEx(
    clientFont, loadingText,
    (Vector2){SCREEN_WIDTH / 2.0f - textSize.x / 2.0f, SCREEN_HEIGHT / 2.0f - textSize.y / 2.0f}, fontSize, spacing,
    COLOR_TEXT_SECONDARY
  );
}

static void DrawTitleScreen(void) {
  const char *titleText     = "ENGINEERING CARD GAME";
  const char *subtitleText  = "Press [ENTER] to Start";

  float       titleFontSize = 60;
  float       titleSpacing  = 3;
  if (clientFont.baseSize > 0)
    titleSpacing = titleFontSize / (float)clientFont.baseSize * GetFontDefault().recs[0].height / 10.0f;

  float subtitleFontSize = 30;
  float subtitleSpacing  = 2;
  if (clientFont.baseSize > 0)
    subtitleSpacing = subtitleFontSize / (float)clientFont.baseSize * GetFontDefault().recs[0].height / 10.0f;

  Vector2 titleSize    = MeasureTextEx(clientFont, titleText, titleFontSize, titleSpacing);
  Vector2 subtitleSize = MeasureTextEx(clientFont, subtitleText, subtitleFontSize, subtitleSpacing);

  DrawTextEx(
    clientFont, titleText, (Vector2){SCREEN_WIDTH / 2.0f - titleSize.x / 2.0f, SCREEN_HEIGHT / 4.0f}, titleFontSize,
    titleSpacing, COLOR_TEXT_PRIMARY
  );
  DrawTextEx(
    clientFont, subtitleText, (Vector2){SCREEN_WIDTH / 2.0f - subtitleSize.x / 2.0f, SCREEN_HEIGHT / 1.8f},
    subtitleFontSize, subtitleSpacing, COLOR_TEXT_SECONDARY
  );
}

static void DrawGameplayGrid(void) {
  int worldViewWidth  = SCREEN_WIDTH * 4;
  int worldViewHeight = SCREEN_HEIGHT * 4;

  int startX          = (int)(gameCamera.target.x - gameCamera.offset.x / gameCamera.zoom - GRID_CELL_SIZE);
  startX              = (startX / GRID_CELL_SIZE) * GRID_CELL_SIZE - GRID_CELL_SIZE;

  int startY          = (int)(gameCamera.target.y - gameCamera.offset.y / gameCamera.zoom - GRID_CELL_SIZE);
  startY              = (startY / GRID_CELL_SIZE) * GRID_CELL_SIZE - GRID_CELL_SIZE;

  int endX            = startX + worldViewWidth + GRID_CELL_SIZE * 2;
  int endY            = startY + worldViewHeight + GRID_CELL_SIZE * 2;

  for (int x = startX; x < endX; x += GRID_CELL_SIZE) { DrawLine(x, startY, x, endY, COLOR_GRID_LINES); }
  for (int y = startY; y < endY; y += GRID_CELL_SIZE) { DrawLine(startX, y, endX, y, COLOR_GRID_LINES); }
}

static void DrawComponentsOnGrid(const SimulatorState *simulatorState) {
  if (simulatorState == NULL) return;

  for (int i = 0; i < simulatorState->elementCount; ++i) {
    if (simulatorState->elementsOnCanvas[i].isActive) {
      CircuitElement element = simulatorState->elementsOnCanvas[i];
      Vector2 worldPos = GetWorldPositionForGrid(element.canvasPosition);

      Rectangle compRec = {
        worldPos.x - GRID_CELL_SIZE / 3.0f, worldPos.y - GRID_CELL_SIZE / 3.0f, GRID_CELL_SIZE * 2.0f / 3.0f,
        GRID_CELL_SIZE * 2.0f / 3.0f
      };

      Color compColor = COLOR_ACCENT_SECONDARY;
      const char *compText = "";      switch (element.type) {
        case ELEMENT_BUTTON:
          compColor = element.outputState ? LIME : MAROON;
          compText = element.outputState ? "MOM" : "mom";
          break;
        case ELEMENT_SWITCH:
          compColor = element.outputState ? GREEN : RED;
          compText = element.outputState ? "ON" : "OFF";
          break;
        case ELEMENT_AND:
          compColor = element.outputState ? SKYBLUE : DARKBLUE;
          compText = "AND";
          break;
        case ELEMENT_OR:
          compColor = element.outputState ? PINK : PURPLE;
          compText = "OR";
          break;
        case ELEMENT_SOURCE:
          compColor = GOLD;
          compText = "SRC";
          break;
        case ELEMENT_SENSOR:
          compColor = DARKBROWN;
          compText = "SNK";
          break;
        default: compText = "???"; break;
      }
      DrawRectangleRec(compRec, compColor);
      DrawRectangleLinesEx(compRec, 2, DARKGRAY);

      if (clientFont.texture.id > 0) {
        float compFontSize = 10;
        float compSpacing = 1;
        Vector2 textSize = MeasureTextEx(clientFont, compText, compFontSize, compSpacing);
        DrawTextEx(
          clientFont, compText,
          (Vector2){compRec.x + (compRec.width - textSize.x) / 2, compRec.y + (compRec.height - textSize.y) / 2},
          compFontSize, compSpacing, BLACK
        );
      }
    }
  }
}

static void DrawConnections(const SimulatorState *simulatorState) {
  if (simulatorState == NULL) return;
  for (int i = 0; i < simulatorState->connectionCount; ++i) {
    if (simulatorState->connections[i].isActive) {
      Connection conn = simulatorState->connections[i];
      const CircuitElement *fromElement = NULL;
      const CircuitElement *toElement = NULL;

      for (int j = 0; j < simulatorState->elementCount; ++j) {
        if (simulatorState->elementsOnCanvas[j].isActive) {
          if (simulatorState->elementsOnCanvas[j].id == conn.fromElementId) { 
            fromElement = &simulatorState->elementsOnCanvas[j]; 
          }
          if (simulatorState->elementsOnCanvas[j].id == conn.toElementId) { 
            toElement = &simulatorState->elementsOnCanvas[j]; 
          }
        }
      }

      if (fromElement && toElement) {
        Vector2 startPos = GetWorldPositionForGrid(fromElement->canvasPosition);
        Vector2 endPos = GetWorldPositionForGrid(toElement->canvasPosition);
        STUB(
          "For elements with multiple inputs/outputs, adjust start/end "
          "points. For now, connect centers."
        );
        DrawLineEx(startPos, endPos, 2.0f, COLOR_TEXT_PRIMARY);
      }
    }
  }
}

static void DrawScenarioDetailsScreen(const SimulatorState *simulatorState) {
  ClearBackground(COLOR_BACKGROUND);
  float       currentScreenWidth  = (float)GetScreenWidth();
  float       currentScreenHeight = (float)GetScreenHeight();

  float closeBtnW = 100, closeBtnH = 36;
  Rectangle closeBtn = {currentScreenWidth - closeBtnW - UI_PADDING, UI_PADDING, closeBtnW, closeBtnH};
  if (DrawUIButton(closeBtn, "Return", LIGHTGRAY, COLOR_TEXT_PRIMARY)) {
    currentClientScreen = CLIENT_SCREEN_SIMULATION;
    TraceLog(LOG_INFO, "CLIENT: Closing Scenario Details view, returning to Simulation");
  }

  const char *title               = TextFormat("Details for Scenario: %s", simulatorState->currentScenario.name);
  Vector2     titleSize           = MeasureTextEx(clientFont, title, 30, 2);
  DrawTextEx(
    clientFont, title, (Vector2){(currentScreenWidth - titleSize.x) / 2, UI_PADDING * 2}, 30, 2, COLOR_TEXT_PRIMARY
  );
  const char *instructions     = "Press [Q] to return to Gameplay";
  Vector2     instructionsSize = MeasureTextEx(clientFont, instructions, 20, 1);
  DrawTextEx(
    clientFont, instructions,
    (Vector2){(currentScreenWidth - instructionsSize.x) / 2, currentScreenHeight - UI_PADDING * 2 - instructionsSize.y},
    20, 1, COLOR_TEXT_SECONDARY
  );

  float     sectionPadding = 20;
  float     sectionWidth   = (currentScreenWidth - 4 * sectionPadding) / 3;
  float     sectionHeight  = currentScreenHeight - UI_PADDING * 8 - titleSize.y - instructionsSize.y;
  float     sectionY       = UI_PADDING * 4 + titleSize.y;

  Rectangle fsmRect        = {sectionPadding, sectionY, sectionWidth, sectionHeight};
  DrawRectangleLinesEx(fsmRect, 2, DARKGRAY);
  DrawTextEx(
    clientFont, "System States (Operational Flow)", (Vector2){fsmRect.x + 10, fsmRect.y + 10}, 18, 1, COLOR_TEXT_PRIMARY
  );
  TODO("Draw placeholder FSM diagram: circles, arrows, state labels");
  DrawCircle((int)(fsmRect.x + fsmRect.width / 2 - 50), (int)(fsmRect.y + fsmRect.height / 2), 30, LIGHTGRAY);
  DrawCircle((int)(fsmRect.x + fsmRect.width / 2 + 50), (int)(fsmRect.y + fsmRect.height / 2 - 60), 30, LIGHTGRAY);
  DrawLineEx(
    (Vector2){fsmRect.x + fsmRect.width / 2 - 20, fsmRect.y + fsmRect.height / 2},
    (Vector2){fsmRect.x + fsmRect.width / 2 + 20, fsmRect.y + fsmRect.height / 2 - 50}, 2, DARKGRAY
  );

  Rectangle truthTableRect = {sectionPadding * 2 + sectionWidth, sectionY, sectionWidth, sectionHeight};
  DrawRectangleLinesEx(truthTableRect, 2, DARKGRAY);
  DrawTextEx(
    clientFont, "Signal Logic (Behavior Matrix)", (Vector2){truthTableRect.x + 10, truthTableRect.y + 10}, 18, 1,
    COLOR_TEXT_PRIMARY
  );
  TODO("Draw placeholder Truth Table: grid, headers (Input A, Input B, Output), values (0/1)");
  DrawLine(
    (int)truthTableRect.x + 10, (int)truthTableRect.y + 80, (int)truthTableRect.x + (int)truthTableRect.width - 10,
    (int)truthTableRect.y + 80, DARKGRAY
  );
  DrawLine(
    (int)truthTableRect.x + (int)truthTableRect.width / 2, (int)truthTableRect.y + 40,
    (int)truthTableRect.x + (int)truthTableRect.width / 2, (int)truthTableRect.y + (int)truthTableRect.height - 10,
    DARKGRAY
  );
  DrawTextEx(
    clientFont, "In1 | Out", (Vector2){truthTableRect.x + 20, truthTableRect.y + 50}, 16, 1, COLOR_TEXT_SECONDARY
  );
  DrawTextEx(
    clientFont, " 0  |  1 ", (Vector2){truthTableRect.x + 20, truthTableRect.y + 90}, 16, 1, COLOR_TEXT_SECONDARY
  );

  Rectangle circuitRect = {sectionPadding * 3 + sectionWidth * 2, sectionY, sectionWidth, sectionHeight};
  DrawRectangleLinesEx(circuitRect, 2, DARKGRAY);
  DrawTextEx(
    clientFont, "Element Configuration (Layout)", (Vector2){circuitRect.x + 10, circuitRect.y + 10}, 18, 1,
    COLOR_TEXT_PRIMARY
  );
  TODO("Draw placeholder Circuit Diagram: component-like boxes, connection lines");
  DrawRectangle(
    (int)(circuitRect.x + circuitRect.width / 2 - 60), (int)(circuitRect.y + circuitRect.height / 2 - 20), 40, 40,
    LIGHTGRAY
  );
  DrawRectangle(
    (int)(circuitRect.x + circuitRect.width / 2 + 20), (int)(circuitRect.y + circuitRect.height / 2 - 20), 40, 40,
    LIGHTGRAY
  );
  DrawLineEx(
    (Vector2){circuitRect.x + circuitRect.width / 2 - 20, circuitRect.y + circuitRect.height / 2},
    (Vector2){circuitRect.x + circuitRect.width / 2 + 20, circuitRect.y + circuitRect.height / 2}, 2, DARKGRAY
  );
}

static void DrawGameplayScreen(const SimulatorState *simulatorState) {
  float     currentScreenWidth  = (float)GetScreenWidth();
  float     currentScreenHeight = (float)GetScreenHeight();

  Rectangle headerArea          = {0, 0, currentScreenWidth, UI_HEADER_HEIGHT};
  Rectangle deckArea = {0, currentScreenHeight - UI_DECK_AREA_HEIGHT, currentScreenWidth, UI_DECK_AREA_HEIGHT};
  Rectangle playArea = {
    0, UI_HEADER_HEIGHT, currentScreenWidth, currentScreenHeight - UI_HEADER_HEIGHT - UI_DECK_AREA_HEIGHT
  };

  gameCamera.offset = (Vector2){playArea.x + playArea.width / 2.0f, playArea.y + playArea.height / 2.0f};
  BeginScissorMode((int)playArea.x, (int)playArea.y, (int)playArea.width, (int)playArea.height);
  BeginMode2D(gameCamera);
  DrawGameplayGrid();
  DrawComponentsOnGrid(simulatorState);
  DrawConnections(simulatorState);
  if (interactionMode == INTERACTION_MODE_WIRING_SELECT_INPUT && wiringFromElementId != -1) {
    const CircuitElement *fromElement = NULL;
    for (int i = 0; i < simulatorState->elementCount; ++i) {
      if (simulatorState->elementsOnCanvas[i].id == wiringFromElementId) {
        fromElement = &simulatorState->elementsOnCanvas[i];
        break;
      }
    }
    if (fromElement) {
      Vector2 startPos = GetWorldPositionForGrid(fromElement->canvasPosition);
      Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), gameCamera);
      DrawLineEx(startPos, mouseWorldPos, 2.0f, Fade(COLOR_ACCENT_PRIMARY, 0.7f));
    }
  }
  EndMode2D();
  EndScissorMode();

  DrawRectangleRec(headerArea, COLOR_UI_AREA_BG_HEADER);
  DrawRectangleLinesEx(headerArea, GRID_LINE_THICKNESS, COLOR_UI_AREA_BORDER);
  float headerTextY       = headerArea.y + UI_PADDING;
  float scenarioNameSize  = 20;
  float conditionSize     = 14;
  float statusTextSizeVal = 18;  DrawTextEx(
    clientFont, TextFormat("Scenario: %s", simulatorState->currentScenario.name),
    (Vector2){headerArea.x + UI_PADDING, headerTextY}, scenarioNameSize, 2, COLOR_TEXT_PRIMARY
  );

  if (simulatorState->currentScenario.isCompleted) {
    DrawTextEx(clientFont, "COMPLETED!", (Vector2){headerArea.x + 400, headerTextY}, scenarioNameSize, 2, GREEN);
  }

  const char *statusText = TextFormat(
    "Deck: %d | Discard: %d | Turn: %s | Actions: %d/%d", simulatorState->deckCardCount - simulatorState->currentDeckIndex,
    simulatorState->discardCardCount, turnInProgress ? "Active" : "Ended", actionsThisTurn, maxActionsPerTurn
  );
  Vector2 statusTextDimensions = MeasureTextEx(clientFont, statusText, statusTextSizeVal, 1);
  DrawTextEx(
    clientFont, statusText,
    (Vector2){currentScreenWidth - statusTextDimensions.x - UI_PADDING,
              headerArea.y + (UI_HEADER_HEIGHT - statusTextDimensions.y) / 2.0f},
    statusTextSizeVal, 1, COLOR_TEXT_SECONDARY
  );
  float conditionsStartX = headerArea.x + UI_PADDING;
  float conditionsStartY =
    headerTextY + scenarioNameSize + (simulatorState->currentScenario.isCompleted ? scenarioNameSize + 4 : 5);
  for (int i = 0; i < simulatorState->currentScenario.conditionCount; ++i) {
    if (conditionsStartY + (i * (conditionSize + 2)) + conditionSize < headerArea.y + UI_HEADER_HEIGHT - UI_PADDING) {
      ScenarioCondition condition      = simulatorState->currentScenario.conditions[i];
      Color             conditionColor = condition.isMet ? GREEN : COLOR_TEXT_SECONDARY;
      const char       *statusIcon     = condition.isMet ? "[X]" : "[ ]";

      DrawTextEx(
        clientFont, TextFormat("%s %s", statusIcon, condition.description),
        (Vector2){conditionsStartX, conditionsStartY + (i * (conditionSize + 2))}, conditionSize, 1, conditionColor
      );
    }
  }

  float detailsButtonWidth  = 120;
  float detailsButtonHeight = 25;
  float detailsButtonX =
    headerArea.x + UI_PADDING +
    MeasureTextEx(clientFont, TextFormat("Scenario: %s", simulatorState->currentScenario.name), scenarioNameSize, 2).x + 20;

  if (detailsButtonX + detailsButtonWidth >
      currentScreenWidth - MeasureTextEx(clientFont, statusText, statusTextSizeVal, 1).x - UI_PADDING - 10) {    detailsButtonX =
      headerArea.x + UI_PADDING +
      MeasureTextEx(clientFont, TextFormat("Scenario: %s", simulatorState->currentScenario.name), scenarioNameSize, 2).x +
      20;
    if (detailsButtonX + detailsButtonWidth > currentScreenWidth / 1.5f) { detailsButtonX = currentScreenWidth / 2.0f; }
  }

  Rectangle detailsButtonRect = {detailsButtonX, headerTextY, detailsButtonWidth, detailsButtonHeight};

  if (CheckCollisionPointRec(GetMousePosition(), detailsButtonRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    currentClientScreen = CLIENT_SCREEN_SCENARIO_DETAILS;
    TraceLog(LOG_INFO, "CLIENT: Opening Scenario Details view.");
  }

  DrawRectangleRec(
    detailsButtonRect,
    CheckCollisionPointRec(GetMousePosition(), detailsButtonRect) ? COLOR_ACCENT_SECONDARY : LIGHTGRAY
  );
  DrawRectangleLinesEx(detailsButtonRect, 1, DARKGRAY);
  const char *detailsButtonText     = "[View Details]";
  Vector2     detailsButtonTextSize = MeasureTextEx(clientFont, detailsButtonText, 18, 1);
  DrawTextEx(
    clientFont, detailsButtonText,
    (Vector2){detailsButtonRect.x + (detailsButtonRect.width - detailsButtonTextSize.x) / 2,
              detailsButtonRect.y + (detailsButtonRect.height - detailsButtonTextSize.y) / 2},
    18, 1, COLOR_TEXT_PRIMARY
  );

  DrawRectangleRec(deckArea, COLOR_UI_AREA_BG_DECK);
  DrawRectangleLinesEx(deckArea, GRID_LINE_THICKNESS, COLOR_UI_AREA_BORDER);

  float currentCardX = deckArea.x + UI_PADDING - handScrollOffset;
  float handLabelY   = deckArea.y + UI_PADDING;
  DrawTextEx(
    clientFont, TextFormat("Hand (%d/%d):", simulatorState->handCardCount, MAX_CARDS_IN_HAND),
    (Vector2){deckArea.x + UI_PADDING, handLabelY}, 20, 1, COLOR_TEXT_PRIMARY
  );

  const char *handLabelText      = TextFormat("Hand (%d/%d):", simulatorState->handCardCount, MAX_CARDS_IN_HAND);
  Vector2     handLabelSize      = MeasureTextEx(clientFont, handLabelText, 20, 1);
  float       handLabelTextWidth = handLabelSize.x;

  DrawTextEx(clientFont, handLabelText, (Vector2){deckArea.x + UI_PADDING, handLabelY}, 20, 1, COLOR_TEXT_PRIMARY);

  if (interactionMode == INTERACTION_MODE_WIRING_SELECT_OUTPUT) {
    DrawTextEx(
      clientFont, "WIRING: Select Output", (Vector2){deckArea.x + UI_PADDING + handLabelTextWidth + 10, handLabelY}, 20,
      1, COLOR_ACCENT_PRIMARY
    );  } else if (interactionMode == INTERACTION_MODE_WIRING_SELECT_INPUT) {
    DrawTextEx(
      clientFont, TextFormat("WIRING: From %d, Select Input", wiringFromElementId),
      (Vector2){deckArea.x + UI_PADDING + handLabelTextWidth + 10, handLabelY}, 20, 1, COLOR_ACCENT_PRIMARY
    );
  }

  float cardAreaY              = handLabelY + 20 + UI_PADDING;
  int   previousLabelTextSize  = GuiGetStyle(LABEL, TEXT_SIZE);
  int   previousLabelAlignment = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
  GuiSetStyle(LABEL, TEXT_SIZE, CARD_TEXT_SIZE);
  GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

  BeginScissorMode(
    (int)deckArea.x, (int)cardAreaY, (int)deckArea.width, (int)(deckArea.height - (cardAreaY - deckArea.y))
  );
  for (int i = 0; i < simulatorState->handCardCount; ++i) {
    Rectangle cardRect = {currentCardX, cardAreaY, CARD_WIDTH, CARD_HEIGHT};

    if (cardRect.x + cardRect.width > deckArea.x && cardRect.x < deckArea.x + deckArea.width) {
      Color cardBorderColor = COLOR_CARD_BORDER;
      Color cardBgColor     = COLOR_CARD_BG;

      if (simulatorState->userHand[i].type == CARD_TYPE_ACTION) {
        cardBgColor     = Fade(YELLOW, 0.3f);
        cardBorderColor = ORANGE;
      } else if (i == selectedCardIndex) {
        cardBorderColor = COLOR_ACCENT_PRIMARY;
      }

      DrawRectangleRec(cardRect, cardBgColor);
      DrawRectangleLinesEx(cardRect, (i == selectedCardIndex) ? 3 : 1, cardBorderColor);

      Rectangle cardTextRect = {
        cardRect.x + CARD_PADDING, cardRect.y + CARD_PADDING, cardRect.width - 2 * CARD_PADDING,
        cardRect.height - 2 * CARD_PADDING
      };
      GuiLabel(cardTextRect, simulatorState->userHand[i].name);

      if (simulatorState->userHand[i].type == CARD_TYPE_ACTION) {
        Rectangle actionLabelRect = {
          cardRect.x + CARD_PADDING, cardRect.y + cardRect.height - 20, cardRect.width - 2 * CARD_PADDING, 15
        };
        int prevSize = GuiGetStyle(LABEL, TEXT_SIZE);
        GuiSetStyle(LABEL, TEXT_SIZE, 12);
        GuiLabel(actionLabelRect, "[ACTION]");
        GuiSetStyle(LABEL, TEXT_SIZE, prevSize);
      }
    }

    currentCardX += CARD_WIDTH + CARD_SPACING;
  }

  EndScissorMode();
  GuiSetStyle(LABEL, TEXT_SIZE, previousLabelTextSize);
  GuiSetStyle(LABEL, TEXT_ALIGNMENT, previousLabelAlignment);
  if (simulatorState->handCardCount > MAX_VISIBLE_CARDS_IN_HAND) {
    float maxScroll = (simulatorState->handCardCount - MAX_VISIBLE_CARDS_IN_HAND) * (CARD_WIDTH + CARD_SPACING) +
                      CARD_SPACING; // Added some padding
    DrawTextEx(
      clientFont, "<", (Vector2){deckArea.x + UI_PADDING, cardAreaY - 20 - UI_PADDING}, 20, 1,
      handScrollOffset > 0 ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY
    );
    DrawTextEx(
      clientFont, ">",
      (Vector2){deckArea.x + deckArea.width - UI_PADDING - MeasureTextEx(clientFont, ">", 20, 1).x,
                cardAreaY - 20 - UI_PADDING},
      20, 1, handScrollOffset < maxScroll ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY
    );
  }

  float scoreZoomTargetX = currentScreenWidth - 200;
  if (scoreZoomTargetX <
      playArea.x + playArea.width - 200) { // Ensure it doesn't go too far left if play area is narrow
    scoreZoomTargetX = playArea.x + playArea.width - 200;
  }
  if (scoreZoomTargetX < UI_PADDING) scoreZoomTargetX = UI_PADDING;
  DrawTextEx(
    clientFont, TextFormat("Score: %d", simulatorState->score), (Vector2){scoreZoomTargetX, UI_HEADER_HEIGHT + UI_PADDING},
    20, 1, COLOR_TEXT_PRIMARY
  );
  DrawTextEx(
    clientFont, TextFormat("Zoom: %.2fx", gameCamera.zoom),
    (Vector2){scoreZoomTargetX, UI_HEADER_HEIGHT + UI_PADDING + 20 + 5}, 20, 1, COLOR_TEXT_SECONDARY
  );
  DrawTextEx(
    clientFont, TextFormat("Target: (%.0f, %.0f)", gameCamera.target.x, gameCamera.target.y),
    (Vector2){scoreZoomTargetX, UI_HEADER_HEIGHT + UI_PADDING + (20 + 5) * 2}, 20, 1, COLOR_TEXT_SECONDARY
  );
}

static void HandleGameplayInput(SimulatorState *simulatorState) {
  if (IsKeyPressed(KEY_ESCAPE)) {
    currentClientScreen = CLIENT_SCREEN_TITLE;
    interactionMode = INTERACTION_MODE_NORMAL;
    wiringFromElementId = -1;
    selectedCardIndex = -1;
    heldButtonId = -1;
    gameplayHasLoggedEntry = false;
    TraceLog(LOG_INFO, "CLIENT: Returning to Title Screen from Simulation.");
    return;
  }

  if (!gameplayHasLoggedEntry && simulatorState != NULL) {
    TraceLog(
      LOG_INFO,
      "CLIENT_SIMULATION_START: Score: %d, DeckCount: %d, "
      "CurrentDeckIdx: %d, HandCount: %d, DiscardCount: %d",
      simulatorState->score, simulatorState->deckCardCount, simulatorState->currentDeckIndex, simulatorState->handCardCount,
      simulatorState->discardCardCount
    );
    gameplayHasLoggedEntry = true;
  }

  if (IsKeyPressed(KEY_D)) {
    if (Server_UserDrawCard(simulatorState)) {
      TraceLog(LOG_INFO, "CLIENT: User attempted to draw a card. Hand size now: %d", simulatorState->handCardCount);
    } else {
      TraceLog(LOG_INFO, "CLIENT: User tried to draw, but couldn't (hand full or no cards left).");
    }
  }
  if (IsKeyPressed(KEY_W)) {
    if (interactionMode == INTERACTION_MODE_NORMAL) {
      interactionMode       = INTERACTION_MODE_WIRING_SELECT_OUTPUT;
      selectedCardIndex     = -1;
      heldButtonId = -1;
      TraceLog(LOG_INFO, "CLIENT: Entered Wiring Mode - Select Output.");
    } else {
      interactionMode       = INTERACTION_MODE_NORMAL;
      wiringFromElementId = -1;
      TraceLog(LOG_INFO, "CLIENT: Exited Wiring Mode.");
    }
  }

  Rectangle playArea = {
    0, UI_HEADER_HEIGHT, (float)GetScreenWidth(), (float)GetScreenHeight() - UI_HEADER_HEIGHT - UI_DECK_AREA_HEIGHT
  };
  Rectangle deckArea = {
    0, (float)GetScreenHeight() - UI_DECK_AREA_HEIGHT, (float)GetScreenWidth(), UI_DECK_AREA_HEIGHT
  };  Vector2 mousePosition = GetMousePosition();

  if (CheckCollisionPointRec(mousePosition, playArea)) {
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
      Vector2 delta     = GetMouseDelta();
      delta             = Vector2Scale(delta, -1.0f / gameCamera.zoom);
      gameCamera.target = Vector2Add(gameCamera.target, delta);
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      Vector2 mouseWorldPosBeforeZoom  = GetScreenToWorld2D(mousePosition, gameCamera);
      gameCamera.zoom                 += (wheel * 0.125f);
      if (gameCamera.zoom < 0.25f) gameCamera.zoom = 0.25f;
      if (gameCamera.zoom > 4.0f) gameCamera.zoom = 4.0f;
      Vector2 mouseWorldPosAfterZoom = GetScreenToWorld2D(mousePosition, gameCamera);
      gameCamera.target =
        Vector2Add(gameCamera.target, Vector2Subtract(mouseWorldPosBeforeZoom, mouseWorldPosAfterZoom));
    }
  } 
  
  if (CheckCollisionPointRec(mousePosition, deckArea) && simulatorState->handCardCount > MAX_VISIBLE_CARDS_IN_HAND) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      float maxScroll   = (simulatorState->handCardCount - MAX_VISIBLE_CARDS_IN_HAND) * (CARD_WIDTH + CARD_SPACING);
      handScrollOffset -= wheel * HAND_SCROLL_SPEED * GetFrameTime();
      if (handScrollOffset < 0) handScrollOffset = 0;
      if (handScrollOffset > maxScroll) handScrollOffset = maxScroll;
    }
  }

  static Vector2 lastTouchPos = {0};
  static bool touchScrolling = false;
  
  if (GetTouchPointCount() > 0) {
    Vector2 currentTouchPos = GetTouchPosition(0);
    if (CheckCollisionPointRec(currentTouchPos, deckArea) && simulatorState->handCardCount > MAX_VISIBLE_CARDS_IN_HAND) {
      if (!touchScrolling) {
        lastTouchPos = currentTouchPos;
        touchScrolling = true;
      } else {
        float deltaX = currentTouchPos.x - lastTouchPos.x;
        float maxScroll = (simulatorState->handCardCount - MAX_VISIBLE_CARDS_IN_HAND) * (CARD_WIDTH + CARD_SPACING);
        handScrollOffset -= deltaX;
        if (handScrollOffset < 0) handScrollOffset = 0;
        if (handScrollOffset > maxScroll) handScrollOffset = maxScroll;
        lastTouchPos = currentTouchPos;
      }
    }
  } else {
    touchScrolling = false;
  }

  if (IsKeyPressed(KEY_SPACE)) {
    if (!turnInProgress) {
      turnInProgress  = true;
      actionsThisTurn = 0;
      TraceLog(LOG_INFO, "CLIENT: Started new turn");
    } else {
      turnInProgress   = false;
      handScrollOffset = 0.0f;
      TraceLog(LOG_INFO, "CLIENT: Ended turn");
    }
  }  bool leftInputPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || 
                         (GetTouchPointCount() > 0 && GetTouchPointCount() == 1);
  bool leftInputReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || 
                          (GetTouchPointCount() == 0 && heldButtonId != -1 && !IsMouseButtonDown(MOUSE_BUTTON_LEFT));
  
  Vector2 inputPosition = GetTouchPointCount() > 0 ? GetTouchPosition(0) : GetMousePosition();
  
  if (leftInputPressed) {
    Rectangle playArea = {
      0, UI_HEADER_HEIGHT, (float)GetScreenWidth(),
      (float)GetScreenHeight() - UI_HEADER_HEIGHT - UI_DECK_AREA_HEIGHT
    };
    Rectangle deckArea = {
      0, (float)GetScreenHeight() - UI_DECK_AREA_HEIGHT,
      (float)GetScreenWidth(), UI_DECK_AREA_HEIGHT
    };
    if (CheckCollisionPointRec(inputPosition, deckArea) && interactionMode == INTERACTION_MODE_NORMAL) {
      float currentCardX = deckArea.x + UI_PADDING;
      float cardAreaY    = deckArea.y + UI_PADDING + 20 + UI_PADDING;      for (int i = 0; i < simulatorState->handCardCount; ++i) {
        Rectangle cardRect = {currentCardX - handScrollOffset, cardAreaY, CARD_WIDTH, CARD_HEIGHT};
        if (CheckCollisionPointRec(inputPosition, cardRect)) {
          Card selectedCardFromHand = simulatorState->userHand[i];
          if (selectedCardFromHand.type == CARD_TYPE_ACTION) {
            if (!turnInProgress) {
              TraceLog(LOG_INFO, "CLIENT: Cannot play action cards outside of turn");
            } else if (actionsThisTurn >= maxActionsPerTurn) {
              TraceLog(LOG_INFO, "CLIENT: Maximum actions per turn reached");
            } else if (Server_UseCardFromHand(simulatorState, i)) {
              actionsThisTurn++;
              TraceLog(
                LOG_INFO, "CLIENT: Played action card '%s' (%d/%d actions)", selectedCardFromHand.name,
                actionsThisTurn, maxActionsPerTurn
              );
            }
          } else {
            if (selectedCardIndex == i) selectedCardIndex = -1;
            else selectedCardIndex = i;
            TraceLog(LOG_INFO, "CLIENT: Card %d selected/deselected.", i);
          }
          break;
        }
        currentCardX += CARD_WIDTH + CARD_SPACING;
      }    } 

    else if (CheckCollisionPointRec(inputPosition, playArea)) {      Vector2 worldInputPos = GetScreenToWorld2D(inputPosition, gameCamera);
      Vector2 gridPos = {
        floorf(worldInputPos.x / GRID_CELL_SIZE),
        floorf(worldInputPos.y / GRID_CELL_SIZE)
      }; 
      if (interactionMode == INTERACTION_MODE_WIRING_SELECT_OUTPUT) {
        for (int i = 0; i < simulatorState->elementCount; ++i) {
          if (simulatorState->elementsOnCanvas[i].isActive &&
              (int)simulatorState->elementsOnCanvas[i].canvasPosition.x == (int)gridPos.x &&
              (int)simulatorState->elementsOnCanvas[i].canvasPosition.y == (int)gridPos.y) {
            wiringFromElementId = simulatorState->elementsOnCanvas[i].id;
            interactionMode = INTERACTION_MODE_WIRING_SELECT_INPUT;
            TraceLog(LOG_INFO, "CLIENT: Wiring - Output selected from element ID %d",
                    wiringFromElementId);
            break;
          }
        }
      } 
      else if (interactionMode == INTERACTION_MODE_WIRING_SELECT_INPUT) {
        CircuitElement *targetElement = NULL;
        int clickedElementId = -1;

        for (int i = 0; i < simulatorState->elementCount; ++i) {
          if (simulatorState->elementsOnCanvas[i].isActive &&
              (int)simulatorState->elementsOnCanvas[i].canvasPosition.x == (int)gridPos.x &&
              (int)simulatorState->elementsOnCanvas[i].canvasPosition.y == (int)gridPos.y) {
            clickedElementId = simulatorState->elementsOnCanvas[i].id;
            targetElement = &simulatorState->elementsOnCanvas[i];
            break;
          }
        }

        if (clickedElementId != -1 && clickedElementId != wiringFromElementId) {
          int targetInputSlot = -1;
          
          if (targetElement->type == ELEMENT_AND || targetElement->type == ELEMENT_OR) {
            for (int s = 0; s < MAX_INPUTS_PER_LOGIC_GATE; ++s) {
              if (targetElement->inputElementIDs[s] == -1) {
                targetInputSlot = s;
                break;
              }
            }
          }

          if (targetInputSlot != -1) {
            if (Server_CreateConnection(simulatorState, wiringFromElementId, clickedElementId,
                                     targetInputSlot)) {
              TraceLog(LOG_INFO, "CLIENT: Connection created");
            }
          } else {
            TraceLog(LOG_INFO, "CLIENT: Target element has no available inputs");
          }
        }
        
        interactionMode = INTERACTION_MODE_NORMAL;
        wiringFromElementId = -1;
      }
      else {
        if (selectedCardIndex != -1) {
          if (simulatorState->userHand[selectedCardIndex].type == CARD_TYPE_ELEMENT) {
            if (!turnInProgress) {
              TraceLog(LOG_INFO, "CLIENT: Cannot place elements outside of turn");
              selectedCardIndex = -1;
            } else if (actionsThisTurn >= maxActionsPerTurn) {
              TraceLog(LOG_INFO, "CLIENT: Maximum actions per turn reached for placing element");
              selectedCardIndex = -1;
            } else {
              // Check if cell is occupied
              bool cellOccupied = false;
              for (int k = 0; k < simulatorState->elementCount; ++k) {
                if (simulatorState->elementsOnCanvas[k].isActive &&
                    (int)simulatorState->elementsOnCanvas[k].canvasPosition.x == (int)gridPos.x &&
                    (int)simulatorState->elementsOnCanvas[k].canvasPosition.y == (int)gridPos.y) {
                  cellOccupied = true;
                  TraceLog(LOG_WARNING, "CLIENT: Canvas cell (%.0f, %.0f) is already occupied.", gridPos.x, gridPos.y);
                  break;
                }
              }
              
              if (!cellOccupied && simulatorState->elementCount < MAX_ELEMENTS_ON_CANVAS) {
                Card cardToPlace = simulatorState->userHand[selectedCardIndex];
                CircuitElement *newElement = &simulatorState->elementsOnCanvas[simulatorState->elementCount];
                newElement->isActive = true;
                newElement->id = simulatorState->nextElementId++;
                newElement->type = cardToPlace.elementToPlace;
                newElement->canvasPosition = gridPos;
                newElement->outputState = false;
                newElement->defaultOutputState = false;
                newElement->connectedInputCount = 0;
                
                for (int k = 0; k < MAX_INPUTS_PER_LOGIC_GATE; ++k) {
                  newElement->inputElementIDs[k] = -1;
                  newElement->actualInputStates[k] = false;
                }
                
                TraceLog(LOG_INFO, "CLIENT: Placed %s (ID: %d) at canvas (%.0f, %.0f)", 
                        cardToPlace.name, newElement->id, gridPos.x, gridPos.y);
                simulatorState->elementCount++;
                
                if (Server_UseCardFromHand(simulatorState, selectedCardIndex)) {
                  actionsThisTurn++;
                  const char *elementName = "Unknown Element";
                  switch (newElement->type) {
                    case ELEMENT_BUTTON: elementName = "Button"; break;
                    case ELEMENT_SWITCH: elementName = "Switch"; break;
                    case ELEMENT_AND: elementName = "AND Gate"; break;
                    case ELEMENT_OR: elementName = "OR Gate"; break;
                    case ELEMENT_SOURCE: elementName = "Source"; break;
                    case ELEMENT_SENSOR: elementName = "Sensor"; break;
                    default: break;
                  }
                  TraceLog(LOG_INFO, "CLIENT: Placed element '%s' (%d/%d actions)", 
                          elementName, actionsThisTurn, maxActionsPerTurn);
                }
                selectedCardIndex = -1;
              } else if (cellOccupied) {
                selectedCardIndex = -1;
              } else {
                TraceLog(LOG_WARNING, "CLIENT: Max elements reached on canvas.");
                selectedCardIndex = -1;
              }
            }          } else {
            TraceLog(LOG_INFO, "CLIENT: Selected card is not an object card. Deselecting.");
            selectedCardIndex = -1;
          }
        } else {
          int clickedElementId = -1;
          ElementType clickedElementType = ELEMENT_NONE;
          for (int k = 0; k < simulatorState->elementCount; ++k) {
            if (simulatorState->elementsOnCanvas[k].isActive) {
              CircuitElement element = simulatorState->elementsOnCanvas[k];
              if ((int)element.canvasPosition.x == (int)gridPos.x && 
                  (int)element.canvasPosition.y == (int)gridPos.y) {
                clickedElementId = element.id;
                clickedElementType = element.type;
                break;
              }
            }
          }
          
          if (clickedElementId != -1) {
            Server_InteractWithElement(simulatorState, clickedElementId);
            if (clickedElementType == ELEMENT_BUTTON) {
              heldButtonId = clickedElementId;
              TraceLog(LOG_INFO, "CLIENT: Holding button ID %d", heldButtonId);
            }
          }
        }
      }
    }  }
    
  if (leftInputReleased && heldButtonId != -1) {
    Server_ReleaseElementInteraction(simulatorState, heldButtonId);
    heldButtonId = -1;
  }

  if (heldButtonId != -1 && (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0)) {
    Server_InteractWithElement(simulatorState, heldButtonId);
  }
}

static Rectangle GetUIButtonBarRect(float screenWidth, float screenHeight) {
    float barHeight = 48;
    Rectangle deckArea = {0, screenHeight - UI_DECK_AREA_HEIGHT, screenWidth, UI_DECK_AREA_HEIGHT};
    return (Rectangle){0, deckArea.y - barHeight, screenWidth, barHeight};
}

static bool DrawUIButton(Rectangle rect, const char* label, Color bg, Color fg) {
    Vector2 inputPos = GetTouchPointCount() > 0 ? GetTouchPosition(0) : GetMousePosition();
    bool isHovered = CheckCollisionPointRec(inputPos, rect);
    
    // Draw button background with hover effect
    Color bgColor = bg;
    if (isHovered) {
        bgColor = Fade(COLOR_ACCENT_PRIMARY, 0.3f);
    }
    DrawRectangleRec(rect, bgColor);
    
    // Draw border with increased thickness for touch targets
    float borderThickness = 2.0f;
    DrawRectangleLinesEx(rect, borderThickness, isHovered ? COLOR_ACCENT_PRIMARY : DARKGRAY);
    
    // Draw button text with proper centering
    Vector2 textSize = MeasureTextEx(clientFont, label, 20, 1);
    Vector2 textPos = {
        rect.x + (rect.width - textSize.x) / 2,
        rect.y + (rect.height - textSize.y) / 2
    };
    DrawTextEx(clientFont, label, textPos, 20, 1, fg);
    
    // Handle both mouse and touch input
    bool pressed = false;
    bool inputPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || 
                       (GetTouchPointCount() > 0);
    
    if (isHovered && inputPressed) {
        pressed = true;
        DrawRectangleRec(rect, Fade(COLOR_ACCENT_PRIMARY, 0.5f));  // Visual feedback for press
    }
    
    return pressed;
}

static void DrawTouchUIAndHandle(SimulatorState* simulatorState) {
    float w = (float)GetScreenWidth();
    float h = (float)GetScreenHeight();
    Rectangle bar = GetUIButtonBarRect(w, h);
    
    // Increase button sizes for better touch targets
    float btnW = 130, btnH = bar.height - 8;
    float spacing = 16;
    float x = bar.x + spacing, y = bar.y + 4;

    // Wiring mode toggle button
    Rectangle wiringBtn = {x, y, btnW, btnH};
    if (DrawUIButton(wiringBtn, interactionMode == INTERACTION_MODE_WIRING_SELECT_OUTPUT ? "Exit Wiring" : "Wiring", 
        interactionMode != INTERACTION_MODE_NORMAL ? COLOR_ACCENT_PRIMARY : LIGHTGRAY, COLOR_TEXT_PRIMARY)) {
        if (interactionMode == INTERACTION_MODE_WIRING_SELECT_OUTPUT || interactionMode == INTERACTION_MODE_WIRING_SELECT_INPUT) {
            interactionMode = INTERACTION_MODE_NORMAL;
            wiringFromElementId = -1;
        } else {
            interactionMode = INTERACTION_MODE_WIRING_SELECT_OUTPUT;
            selectedCardIndex = -1;
            heldButtonId = -1;
        }
    }

    // Draw card button
    x += btnW + spacing;
    Rectangle drawBtn = {x, y, btnW, btnH};
    if (DrawUIButton(drawBtn, "Draw Card", LIGHTGRAY, COLOR_TEXT_PRIMARY)) {
        Server_UserDrawCard(simulatorState);
    }

    // Turn control button
    x += btnW + spacing;
    Rectangle turnBtn = {x, y, btnW, btnH};
    if (DrawUIButton(turnBtn, turnInProgress ? "End Turn" : "Start Turn", 
        turnInProgress ? COLOR_ACCENT_SECONDARY : LIGHTGRAY, COLOR_TEXT_PRIMARY)) {
        if (!turnInProgress) {
            turnInProgress = true;
            actionsThisTurn = 0;
        } else {
            turnInProgress = false;
            handScrollOffset = 0.0f;
        }
    }
    
    x += btnW + spacing;
    if (simulatorState->handCardCount > MAX_VISIBLE_CARDS_IN_HAND) {
        Rectangle leftBtn = {w - btnW*2 - spacing*2, y, btnW, btnH};
        Rectangle rightBtn = {w - btnW - spacing, y, btnW, btnH};
        if (DrawUIButton(leftBtn, "<", LIGHTGRAY, COLOR_TEXT_PRIMARY)) {
            handScrollOffset -= (CARD_WIDTH + CARD_SPACING);
            if (handScrollOffset < 0) handScrollOffset = 0;
        }
        if (DrawUIButton(rightBtn, ">", LIGHTGRAY, COLOR_TEXT_PRIMARY)) {
            float maxScroll = (simulatorState->handCardCount - MAX_VISIBLE_CARDS_IN_HAND) * (CARD_WIDTH + CARD_SPACING);
            handScrollOffset += (CARD_WIDTH + CARD_SPACING);
            if (handScrollOffset > maxScroll) handScrollOffset = maxScroll;
        }
    }
}

void Client_UpdateAndDraw(SimulatorState *simulatorState) {
  if (currentClientScreen == CLIENT_SCREEN_LOADING) {
    framesCounter++;
    if (framesCounter > 120) { currentClientScreen = CLIENT_SCREEN_TITLE; }
  } else if (currentClientScreen == CLIENT_SCREEN_TITLE) {
    if (IsKeyPressed(KEY_ENTER)) { currentClientScreen = CLIENT_SCREEN_SIMULATION; }
  } else if (currentClientScreen == CLIENT_SCREEN_SCENARIO_DETAILS) {
    if (IsKeyPressed(KEY_Q)) {
      currentClientScreen = CLIENT_SCREEN_SIMULATION;
      TraceLog(LOG_INFO, "CLIENT: Closing Scenario Details view, returning to Simulation (Q key).");
    }
  } else if (currentClientScreen == CLIENT_SCREEN_SIMULATION) {
    HandleGameplayInput(simulatorState);
  }
  BeginDrawing();
  ClearBackground(COLOR_BACKGROUND);
  if (currentClientScreen == CLIENT_SCREEN_LOADING) {
    DrawLoadingScreen();
  } else if (currentClientScreen == CLIENT_SCREEN_TITLE) {
    DrawTitleScreen();
  } else if (currentClientScreen == CLIENT_SCREEN_SCENARIO_DETAILS) {
    if (simulatorState != NULL) {
      DrawScenarioDetailsScreen(simulatorState);
    } else {
      DrawTextEx(clientFont, "Error: SimulatorState is NULL for Details", (Vector2){20, 20}, 20, 1, RED);
    }
  } else if (currentClientScreen == CLIENT_SCREEN_SIMULATION) {
    if (simulatorState != NULL) {
      DrawGameplayScreen(simulatorState);
      DrawTouchUIAndHandle(simulatorState);
    } else {
      DrawTextEx(clientFont, "Error: SimulatorState is NULL", (Vector2){20, 20}, 20, 1, RED);
    }
  } else {
    DrawTextEx(clientFont, "UNKNOWN CLIENT SCREEN", (Vector2){20, 20}, 20, 1, RED);
  }
  DrawFPS(GetScreenWidth() - 100, UI_PADDING);
  EndDrawing();
}
