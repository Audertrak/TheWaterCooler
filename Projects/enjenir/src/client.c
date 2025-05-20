// ... (includes and static globals up to gameCamera remain the same) ...
#include "client.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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

static void DrawGameplayGrid(void);

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
  SetTextLineSpacing(FONT_RASTER_SIZE + FONT_RASTER_SIZE / 4);
  GuiSetFont(clientFont);

  // Corrected RayGui style enums
  GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT); // Corrected
  GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(COLOR_TEXT_PRIMARY));
  GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER); // Corrected

  gameCamera.target = (Vector2){0.0f, 0.0f};
  gameCamera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
  gameCamera.rotation = 0.0f;
  gameCamera.zoom = 1.0f;

  SetTargetFPS(60);
  currentClientScreen = CLIENT_SCREEN_LOADING;
  framesCounter = 0;
  return true;
}

// ... (Client_Close, Client_ShouldClose, DrawLoadingScreen, DrawTitleScreen,
// DrawGameplayGrid, DrawGameplayScreen, Client_UpdateAndDraw remain the same as
// in Goal 16) ...

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
  EndMode2D();
  EndScissorMode();

  DrawRectangleRec(headerArea, COLOR_UI_AREA_BG);
  DrawRectangleLinesEx(headerArea, GRID_LINE_THICKNESS, COLOR_UI_AREA_BORDER);
  DrawTextEx(clientFont, "Scenario: Build a Toggle Switch",
             (Vector2){headerArea.x + UI_PADDING,
                       headerArea.y + (UI_HEADER_HEIGHT - 30) / 2.0f},
             30, 2, COLOR_TEXT_PRIMARY);

  DrawRectangleRec(deckArea, COLOR_UI_AREA_BG);
  DrawRectangleLinesEx(deckArea, GRID_LINE_THICKNESS, COLOR_UI_AREA_BORDER);

  float currentCardX = deckArea.x + UI_PADDING;
  float handLabelY = deckArea.y + UI_PADDING;
  DrawTextEx(clientFont, "Hand:", (Vector2){currentCardX, handLabelY}, 20, 1,
             COLOR_TEXT_PRIMARY);

  float cardAreaY = handLabelY + 20 + UI_PADDING;

  for (int i = 0; i < gameState->handCardCount; ++i) {
    Rectangle cardRect = {currentCardX, cardAreaY, CARD_WIDTH, CARD_HEIGHT};

    DrawRectangleRec(cardRect, Fade(COLOR_ACCENT_SECONDARY, 0.7f));
    DrawRectangleLinesEx(cardRect, 1, COLOR_TEXT_SECONDARY);

    Rectangle cardTextRect = {
        cardRect.x + CARD_PADDING, cardRect.y + CARD_PADDING,
        cardRect.width - 2 * CARD_PADDING, cardRect.height - 2 * CARD_PADDING};
    // Ensure RayGui uses the correct font size for card text
    // int previousTextSize = GuiGetStyle(LABEL, TEXT_SIZE);
    // GuiSetStyle(LABEL, TEXT_SIZE, 18); // Example card text size
    GuiLabel(cardTextRect, gameState->playerHand[i].name);
    // GuiSetStyle(LABEL, TEXT_SIZE, previousTextSize); // Reset if changed

    currentCardX += CARD_WIDTH + CARD_SPACING;
  }
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
    }

    Rectangle playArea = {0, UI_HEADER_HEIGHT, SCREEN_WIDTH,
                          SCREEN_HEIGHT - UI_HEADER_HEIGHT -
                              UI_DECK_AREA_HEIGHT};
    Vector2 mousePosition = GetMousePosition();

    if (CheckCollisionPointRec(mousePosition, playArea)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / gameCamera.zoom);
        gameCamera.target = Vector2Add(gameCamera.target, delta);
      }

      float wheel = GetMouseWheelMove();
      if (wheel != 0) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(mousePosition, gameCamera);
        gameCamera.offset = mousePosition;
        gameCamera.target = mouseWorldPos;

        float zoomIncrement = 0.125f;
        gameCamera.zoom += (wheel * zoomIncrement);
        if (gameCamera.zoom < 0.25f)
          gameCamera.zoom = 0.25f;
        if (gameCamera.zoom > 4.0f)
          gameCamera.zoom = 4.0f;
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
