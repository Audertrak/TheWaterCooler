// #include "raylib.h"

#ifndef CONFIG_H
#define CONFIG_H

// Screen dimensions
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// Game title
#define WINDOW_TITLE "return to enjenir"

// Font
#define FONT_PATH "assets/fonts/CourierPrime-Regular.ttf"
#define FONT_RASTER_SIZE 96

// Colors
#define COLOR_BACKGROUND RAYWHITE
#define COLOR_TEXT_PRIMARY (Color){51, 51, 51, 255}
#define COLOR_TEXT_SECONDARY GRAY
#define COLOR_ACCENT_PRIMARY ORANGE
#define COLOR_ACCENT_SECONDARY LIGHTGRAY
#define COLOR_GRID_LINES (Color){220, 220, 220, 255}
#define COLOR_UI_AREA_BORDER DARKGRAY
#define COLOR_UI_AREA_BG_HEADER Fade(LIGHTGRAY, 0.1f)
#define COLOR_UI_AREA_BG_DECK Fade(LIGHTGRAY, 0.1f)
#define COLOR_CARD_BG Fade(WHITE, 0.85f)
#define COLOR_CARD_BORDER DARKGRAY

// Gameplay Grid
#define GRID_CELL_SIZE 50
#define GRID_LINE_THICKNESS 1

// UI Area Layout
#define UI_HEADER_HEIGHT 60
#define UI_DECK_AREA_HEIGHT 150
#define UI_PADDING 10

// Card Display Properties
#define CARD_WIDTH 140
#define CARD_HEIGHT 100
#define CARD_PADDING 5
#define CARD_SPACING 10
#define CARD_TEXT_SIZE 16

// Debugging Macros
#ifdef DEBUG
// __FILE__, __LINE__, __func__ are standard predefined macros
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
// AT_FILE_LINE will be a string literal like "src/file.c:123"
#define AT_FILE_LINE __FILE__ ":" TOSTRING(__LINE__)

// Pass __func__ as a separate argument to TraceLog
#define TODO(message)                                                          \
  TraceLog(LOG_WARNING, "TODO: %s [%s in %s]", message, AT_FILE_LINE, __func__)
#define STUB(message)                                                          \
  TraceLog(LOG_DEBUG, "STUB: %s [%s in %s]", message, AT_FILE_LINE, __func__)
#else
#define TODO(message)                                                          \
  ((void)0) // Cast to void to prevent unused variable warnings if message is
            // complex
#define STUB(message) ((void)0)
#endif

#endif // CONFIG_H
