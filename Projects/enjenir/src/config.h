#ifndef CONFIG_H
#define CONFIG_H

/**
 * @file config.h
 * @brief Centralized configuration file for application settings.
 *
 * This file contains macros and definitions for various aspects of the
 * application, including screen dimensions, UI layout, colors, fonts,
 * gameplay parameters, and debugging utilities.
 *
 * Modifying values in this file will affect the overall appearance and
 * behavior of the application. Ensure that changes are consistent with
 * the intended design and functionality.
 */

// --- Screen and Window Configuration ---

/** @brief Default width of the application window in pixels. */
#define SCREEN_WIDTH              1280
/** @brief Default height of the application window in pixels. */
#define SCREEN_HEIGHT             720
/** @brief Title displayed on the application window. */
#define WINDOW_TITLE              "return to enjenir"

// --- Font Configuration ---

/**
 * @brief Path to the primary font file.
 * This path is relative to the application's working directory
 * (typically where the executable is run from, or where assets are bundled).
 */
#define FONT_PATH                 "assets/fonts/CourierPrime-Regular.ttf"
/**
 * @brief Base size at which the font should be rasterized (loaded).
 * Larger sizes can provide better quality when scaling down, but consume
 * more memory. This is the size used by LoadFontEx.
 */
#define FONT_RASTER_SIZE          96

// --- Color Palette ---
// Colors are defined using Raylib's Color struct or predefined color macros.
// The Fade() function can be used for transparency.

/** @brief Background color for most screens and areas. */
#define COLOR_BACKGROUND          RAYWHITE
/** @brief Primary color for text elements. */
#define COLOR_TEXT_PRIMARY        (Color){51, 51, 51, 255}
/** @brief Secondary color for less prominent text or disabled elements. */
#define COLOR_TEXT_SECONDARY      GRAY
/** @brief Primary accent color used for highlights, selections, etc. */
#define COLOR_ACCENT_PRIMARY      ORANGE
/** @brief Secondary accent color, often a lighter shade for hover states or backgrounds. */
#define COLOR_ACCENT_SECONDARY    LIGHTGRAY
/** @brief Color for the lines of the gameplay grid. */
#define COLOR_GRID_LINES          (Color){220, 220, 220, 255}
/** @brief Border color for UI panel areas like header and deck. */
#define COLOR_UI_AREA_BORDER      DARKGRAY
/** @brief Background color for the header UI panel. */
#define COLOR_UI_AREA_BG_HEADER   Fade(LIGHTGRAY, 0.1f)
/** @brief Background color for the deck/hand UI panel. */
#define COLOR_UI_AREA_BG_DECK     Fade(LIGHTGRAY, 0.1f)
/** @brief Background color for individual cards. */
#define COLOR_CARD_BG             Fade(WHITE, 0.85f)
/** @brief Border color for individual cards. */
#define COLOR_CARD_BORDER         DARKGRAY

// --- Gameplay Grid Configuration ---

/** @brief Size of each cell in the gameplay grid in pixels. */
#define GRID_CELL_SIZE            50
/** @brief Thickness of the grid lines in pixels. */
#define GRID_LINE_THICKNESS       1

// --- UI Area Layout ---
// These define the dimensions and spacing for major UI panels.

/** @brief Height of the header area at the top of the screen in pixels. */
#define UI_HEADER_HEIGHT          60
/** @brief Height of the deck/hand area at the bottom of the screen in pixels. */
#define UI_DECK_AREA_HEIGHT       150
/** @brief General padding used around UI elements in pixels. */
#define UI_PADDING                10
/**
 * @brief Maximum number of cards visible in the hand area without scrolling.
 * If handCardCount exceeds this, scrolling arrows will appear.
 */
#define MAX_VISIBLE_CARDS_IN_HAND 8
/**
 * @brief Speed factor for scrolling the hand area using the mouse wheel.
 * Higher values result in faster scrolling. Adjusted by frame time for
 * consistent speed.
 */
#define HAND_SCROLL_SPEED         600.0f

// --- Card Display Properties ---

/** @brief Width of a single card in the hand display in pixels. */
#define CARD_WIDTH                140
/** @brief Height of a single card in the hand display in pixels. */
#define CARD_HEIGHT               100
/** @brief Padding inside a card, between the border and content, in pixels. */
#define CARD_PADDING              5
/** @brief Horizontal spacing between cards in the hand display in pixels. */
#define CARD_SPACING              10
/** @brief Default font size for text displayed on cards. */
#define CARD_TEXT_SIZE            16

// --- Debugging Macros ---
// These macros provide utility functions for debugging, primarily for logging
// TODO items and STUBbed functions. They are active only when the DEBUG
// preprocessor directive is defined (e.g., via a compiler flag like -DDEBUG).

#ifdef DEBUG
  /**
   * @brief Stringifies a preprocessor token.
   * Helper macro for AT_FILE_LINE.
   */
  #define STRINGIFY(x) #x
  /**
   * @brief Converts a preprocessor token to a string literal after expansion.
   * Helper macro for AT_FILE_LINE.
   */
  #define TOSTRING(x)  STRINGIFY(x)
  /**
   * @brief Creates a string literal representing the current file and line number.
   * Example: "src/client.c:123"
   */
  #define AT_FILE_LINE __FILE__ ":" TOSTRING(__LINE__)

  /**
   * @brief Logs a message once per session from a specific call site.
   *
   * This macro uses a static boolean flag to ensure that a given log message
   * (identified by its source file, line, and function) is only printed to
   * the console once during the application's runtime. This helps reduce
   * log spam for repeated calls within loops or frequent updates.
   *
   * @param logLevel The Raylib log level (e.g., LOG_INFO, LOG_WARNING).
   * @param tag A short string tag for the log type (e.g., "TODO", "STUB").
   * @param message The main message string to log.
   * @param location_str A string representing the file and line (e.g., from AT_FILE_LINE).
   * @param func_str The name of the function where the log occurs (from __func__).
   *
   * @note This macro relies on static variables within a `do-while(0)` block,
   * a common pattern that leverages GNU C extensions for macro hygiene and
   * static state. It should be compatible with most modern C compilers (like GCC, Clang,
   * and Zig's C compiler). If compiler compatibility issues arise with the static
   * variable, this mechanism might need to be re-evaluated.
   */
  #define LOG_ONCE(logLevel, tag, message, location_str, func_str)                     \
    do {                                                                               \
      static bool already_logged_this_session = false;                                 \
      if (!already_logged_this_session) {                                              \
        TraceLog(logLevel, "%s: %s [%s in %s]", tag, message, location_str, func_str); \
        already_logged_this_session = true;                                            \
      }                                                                                \
    } while (0)

  /**
   * @brief Logs a "TODO" message at LOG_WARNING level, once per call site.
   * Used to mark areas of code that require future implementation or attention.
   * The message includes the file, line, and function where TODO is called.
   * @param message A string describing the pending task.
   */
  #define TODO(message) LOG_ONCE(LOG_WARNING, "TODO", message, AT_FILE_LINE, __func__)
  /**
   * @brief Logs a "STUB" message at LOG_DEBUG level, once per call site.
   * Used to mark stubbed-out functions or features that are placeholders
   * or have minimal/no actual implementation yet. Useful for suppressing
   * compiler warnings for unused parameters or empty function bodies during
   * early development.
   * @param message A string describing the stubbed feature.
   */
  #define STUB(message) LOG_ONCE(LOG_DEBUG, "STUB", message, AT_FILE_LINE, __func__)
#else
  // If DEBUG is not defined, TODO and STUB macros compile to nothing,
  // effectively removing them from release builds.
  #define TODO(message) ((void)0)
  #define STUB(message) ((void)0)
#endif // DEBUG

#endif // CONFIG_H
