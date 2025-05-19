#include "client.h"
#include "config.h" // For screen dimensions, title, font path, and colors
#include "raylib.h" // Raylib API
#include <stddef.h> // For NULL

// Client-specific state (e.g., UI elements, current screen)
typedef enum ClientScreen {
  CLIENT_SCREEN_LOADING,
  CLIENT_SCREEN_TITLE,
  CLIENT_SCREEN_GAMEPLAY
  // Add more as needed
} ClientScreen;

static ClientScreen currentClientScreen = CLIENT_SCREEN_LOADING;
static Font clientFont;
static bool customFontLoaded = false; // Flag to track if our custom font loaded
static int framesCounter = 0;

bool Client_Init(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
  if (!IsWindowReady()) {
    return false; // Failed to initialize window
  }

  clientFont = LoadFont(FONT_PATH);
  // A basic check: if texture.id is 0 after LoadFont, it likely failed.
  // Or, if it's the same as default font's texture id AND we didn't intend to
  // load default.
  if (clientFont.texture.id == 0 ||
      (clientFont.texture.id == GetFontDefault().texture.id &&
       TextIsEqual(FONT_PATH, "") == false)) {
    TraceLog(LOG_WARNING,
             "Failed to load custom font at '%s', using default Raylib font.",
             FONT_PATH);
    clientFont = GetFontDefault();
    customFontLoaded = false;
  } else {
    customFontLoaded = true;
    TraceLog(LOG_INFO, "Custom font loaded successfully: %s", FONT_PATH);
  }

  SetTargetFPS(60);
  currentClientScreen = CLIENT_SCREEN_LOADING;
  framesCounter = 0;
  return true;
}

void Client_Close(void) {
  if (customFontLoaded) { // Only unload if we successfully loaded a custom font
    UnloadFont(clientFont);
    TraceLog(LOG_INFO, "Custom font unloaded.");
  }
  CloseWindow();
}

bool Client_ShouldClose(void) { return WindowShouldClose(); }

static void DrawLoadingScreen(void) {
  const char *loadingText = "LOADING...";
  // Use clientFont which is now either custom or default
  Vector2 textSize = MeasureTextEx(clientFont, loadingText, 40, 2);
  DrawTextEx(clientFont, loadingText,
             (Vector2){SCREEN_WIDTH / 2.0f - textSize.x / 2.0f,
                       SCREEN_HEIGHT / 2.0f - textSize.y / 2.0f},
             40, 2, COLOR_TEXT_SECONDARY);
}

static void DrawTitleScreen(void) {
  const char *titleText = "ENGINEERING CARD GAME";
  const char *subtitleText = "Press [ENTER] to Start";
  Vector2 titleSize = MeasureTextEx(clientFont, titleText, 60, 3);
  Vector2 subtitleSize = MeasureTextEx(clientFont, subtitleText, 30, 2);

  DrawTextEx(
      clientFont, titleText,
      (Vector2){SCREEN_WIDTH / 2.0f - titleSize.x / 2.0f, SCREEN_HEIGHT / 4.0f},
      60, 3, COLOR_TEXT_PRIMARY);
  DrawTextEx(clientFont, subtitleText,
             (Vector2){SCREEN_WIDTH / 2.0f - subtitleSize.x / 2.0f,
                       SCREEN_HEIGHT / 1.8f},
             30, 2, COLOR_TEXT_SECONDARY);
}

static void DrawGameplayScreen(const GameState *gameState) {
  DrawTextEx(clientFont, TextFormat("Score: %d", gameState->score),
             (Vector2){20, 20}, 30, 2, COLOR_TEXT_PRIMARY);
  DrawTextEx(clientFont, "GAMEPLAY SCREEN - Placeholder", (Vector2){20, 60}, 30,
             2, COLOR_TEXT_SECONDARY);
}

void Client_UpdateAndDraw(GameState *gameState) {
  // --- Client-side Update Logic (UI state, input handling) ---
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
  case CLIENT_SCREEN_GAMEPLAY:
    if (IsKeyPressed(KEY_ESCAPE)) {
      currentClientScreen = CLIENT_SCREEN_TITLE;
    }
    break;
  default:
    break;
  }

  // --- Drawing ---
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

  DrawFPS(SCREEN_WIDTH - 100, 10);
  EndDrawing();
}
