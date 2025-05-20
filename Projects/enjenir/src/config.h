// #include "raylib.h"

#ifndef CONFIG_H
#define CONFIG_H

// Screen dimensions
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// Game title
#define WINDOW_TITLE "return to enjenir"

// Font
#define FONT_PATH "assets/fonts/cour.ttf"
#define FONT_RASTER_SIZE 96

// Colors
#define COLOR_BACKGROUND RAYWHITE
#define COLOR_TEXT_PRIMARY (Color){51, 51, 51, 255}
#define COLOR_TEXT_SECONDARY GRAY
#define COLOR_ACCENT_PRIMARY ORANGE
#define COLOR_ACCENT_SECONDARY LIGHTGRAY
#define COLOR_GRID_LINES (Color){220, 220, 220, 255}
#define COLOR_UI_AREA_BORDER DARKGRAY
#define COLOR_UI_AREA_BG Fade(LIGHTGRAY, 0.5f)

// Gameplay Grid
#define GRID_CELL_SIZE 50
#define GRID_LINE_THICKNESS 1

// UI Area Layout (simple example, can be refined)
#define UI_HEADER_HEIGHT 60
#define UI_DECK_AREA_HEIGHT 150
#define UI_PADDING 10

// RayGUI
#define RAYGUI_SUPPORT_ICONS // If you plan to use icons later

// Gameplay Grid
#define GRID_CELL_SIZE 50
#define GRID_LINE_THICKNESS 1

// UI Area Layout
#define UI_HEADER_HEIGHT 60
#define UI_DECK_AREA_HEIGHT 150 // Keep this for the overall area
#define UI_PADDING 10

// Card Display Properties
#define CARD_WIDTH 140
#define CARD_HEIGHT 100
#define CARD_PADDING 5  // Padding inside the card
#define CARD_SPACING 10 // Spacing between cards

#endif // CONFIG_H
