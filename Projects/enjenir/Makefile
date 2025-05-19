# Project name
PROJECT_NAME = enjenir

# Build directory
BUILD_DIR = build

# Executable names
RELEASE_EXE = $(BUILD_DIR)/$(PROJECT_NAME).exe
DEBUG_EXE = $(BUILD_DIR)/$(PROJECT_NAME)_debug.exe

# Asset directory
ASSETS_DIR = assets
TARGET_ASSETS_DIR = $(BUILD_DIR)/$(ASSETS_DIR)

# Raylib paths (assuming 'lib/raylib' is the cloned repo)
RAYLIB_BASE_DIR = lib/raylib
RAYLIB_INCLUDE_DIR = $(RAYLIB_BASE_DIR)/src
RAYLIB_LIB_FILE = $(RAYLIB_BASE_DIR)/src/libraylib.a

# Compiler and target
ZIG_CC = zig cc
TARGET_FLAGS = -target x86_64-windows-gnu
CC = $(ZIG_CC) $(TARGET_FLAGS)

# Source files
APP_C_FILES = $(wildcard src/*.c)

# Common C flags
COMMON_CFLAGS = -std=c11 \
                -DPLATFORM_DESKTOP \
                -DNOGDI \
                -Isrc \
                -I$(RAYLIB_INCLUDE_DIR)

# Linker flags
SYSTEM_LDFLAGS = -lopengl32 -lgdi32 -lwinmm -lkernel32 -luser32 -lshell32 -ladvapi32 -lole32 \
                 -Wl,/subsystem:windows

# Release specific flags
RELEASE_CFLAGS = $(COMMON_CFLAGS) -O3 -flto -DNDEBUG

# Debug specific flags
DEBUG_CFLAGS = $(COMMON_CFLAGS) -g -DDEBUG

# Default target
all: release debug

# Release build target
release: $(RELEASE_EXE) copy-assets

# Debug build target
debug: $(DEBUG_EXE) copy-assets

# Create build directory if it doesn't exist
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Generate object file names
APP_OBJS_RELEASE = $(patsubst src/%.c,$(BUILD_DIR)/release/%.o,$(APP_C_FILES))
APP_OBJS_DEBUG = $(patsubst src/%.c,$(BUILD_DIR)/debug/%.o,$(APP_C_FILES))

# Create subdirectories for object files
$(BUILD_DIR)/release: | $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/release

$(BUILD_DIR)/debug: | $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/debug

# Rule to compile a C file to an object file (Release)
$(BUILD_DIR)/release/%.o: src/%.c | $(BUILD_DIR)/release
	@echo "Compiling (Release): $< -> $@"
	$(CC) $(RELEASE_CFLAGS) -c $< -o $@

# Rule to compile a C file to an object file (Debug)
$(BUILD_DIR)/debug/%.o: src/%.c | $(BUILD_DIR)/debug
	@echo "Compiling (Debug): $< -> $@"
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

# Rule to link object files into the final executable (Release)
$(RELEASE_EXE): $(APP_OBJS_RELEASE) $(RAYLIB_LIB_FILE)
	@echo "Linking (Release): $@"
	$(CC) $(APP_OBJS_RELEASE) $(RAYLIB_LIB_FILE) $(SYSTEM_LDFLAGS) -o $@

# Rule to link object files into the final executable (Debug)
$(DEBUG_EXE): $(APP_OBJS_DEBUG) $(RAYLIB_LIB_FILE)
	@echo "Linking (Debug): $@"
	$(CC) $(APP_OBJS_DEBUG) $(RAYLIB_LIB_FILE) $(SYSTEM_LDFLAGS) -o $@

# Copy assets - using POSIX-style commands
copy-assets: $(ASSETS_DIR) | $(BUILD_DIR)
	@echo "Copying assets to $(TARGET_ASSETS_DIR)..."
	@mkdir -p $(TARGET_ASSETS_DIR)
	@cp -R $(ASSETS_DIR)/* $(TARGET_ASSETS_DIR)/

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Generate compile_commands.json
compile_commands:
	@echo "Generating compile_commands.json..."
	compiledb -n make debug
	@echo "compile_commands.json generated. You may need to restart your LSP/IDE."

# --- Run targets ---
run-release: release
	@echo "Running Release: $(RELEASE_EXE)"
	@cd $(BUILD_DIR) && ./$(notdir $(RELEASE_EXE))

run-debug: debug
	@echo "Running Debug: $(DEBUG_EXE)"
	@cd $(BUILD_DIR) && ./$(notdir $(DEBUG_EXE))

# Phony targets
.PHONY: all clean release debug run-release run-debug compile_commands copy-assets $(RAYLIB_LIB_FILE)

