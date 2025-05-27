# Project name
PROJECT_NAME = enjenir

# Build directory
BUILD_DIR = build
NATIVE_BUILD_DIR = $(BUILD_DIR)/native
WASM_LOGIC_BUILD_DIR = $(BUILD_DIR)/wasm_logic
WEB_APP_BUILD_DIR = $(BUILD_DIR)/web_app

# Executable names
RELEASE_EXE = $(NATIVE_BUILD_DIR)/$(PROJECT_NAME).exe
DEBUG_EXE = $(NATIVE_BUILD_DIR)/$(PROJECT_NAME)_debug.exe

# WASM output name (headless server logic)
WASM_SERVER_LOGIC_OUTPUT = $(WASM_LOGIC_BUILD_DIR)/server_logic.wasm

# Web App output name (HTML file)
WEB_APP_HTML_FILE = $(WEB_APP_BUILD_DIR)/index.html

# Asset directory
ASSETS_DIR = assets
TARGET_ASSETS_NATIVE_DIR = $(NATIVE_BUILD_DIR)/$(ASSETS_DIR)

# Raylib paths (platform-specific)
RAYLIB_WIN64_DIR = lib/raylib-5.5_win64_mingw-w64
RAYLIB_LINUX_DIR = lib/raylib-5.5_linux_amd64
RAYLIB_WEB_DIR = lib/raylib-5.5_webassembly

# Platform-specific Raylib include and lib
RAYLIB_INCLUDE_DIR_NATIVE = $(RAYLIB_WIN64_DIR)/include
RAYLIB_LIB_FILE_NATIVE = $(RAYLIB_WIN64_DIR)/lib/libraylib.a

RAYLIB_INCLUDE_DIR_LINUX = $(RAYLIB_LINUX_DIR)/include
RAYLIB_LIB_FILE_LINUX = $(RAYLIB_LINUX_DIR)/lib/libraylib.a

RAYLIB_INCLUDE_DIR_WEB = $(RAYLIB_WEB_DIR)/include
RAYLIB_LIB_FILE_WEB = $(RAYLIB_WEB_DIR)/lib/libraylib.a

# Compilers
ZIG_CC = zig cc
EMSDK ?= C:/Users/tcunkle/scoop/apps/emscripten/current 
EMCC = $(EMSDK)/upstream/emscripten/emcc 

# Native Target
NATIVE_TARGET_FLAGS = -target x86_64-windows-gnu
NATIVE_CC = $(ZIG_CC) $(NATIVE_TARGET_FLAGS)

# WASM Logic Target (Headless)
WASM_LOGIC_TARGET_FLAGS = -target wasm32-freestanding -nostdlib 
WASM_LOGIC_CC = $(ZIG_CC) $(WASM_LOGIC_TARGET_FLAGS)

# Source files
APP_C_FILES_NATIVE = src/main.c src/client.c src/server.c
APP_C_FILES_WASM_LOGIC = src/server.c 
APP_C_FILES_WEB = src/main.c src/client.c src/server.c

RAYLIB_WEB_SRCS = \
    $(RAYLIB_SRC_DIR)/rcore.c \
    $(RAYLIB_SRC_DIR)/rshapes.c \
    $(RAYLIB_SRC_DIR)/rtextures.c \
    $(RAYLIB_SRC_DIR)/rtext.c \
    $(RAYLIB_SRC_DIR)/rmodels.c \
    $(RAYLIB_SRC_DIR)/utils.c \
    $(RAYLIB_SRC_DIR)/raudio.c 

# CFLAGS
COMMON_CFLAGS_NATIVE = -std=c11 -DPLATFORM_DESKTOP -DNOGDI -Isrc -I$(RAYLIB_INCLUDE_DIR_NATIVE)
COMMON_CFLAGS_WASM_LOGIC = -std=c11 -DTOOL_WASM_BUILD -Isrc -O2 \
                           --export=Server_Init --export=Server_Update \
                           --export=Server_PlayCardFromHand --export=Server_InteractWithComponent \
                           --export=Server_PlayerDrawCard --export=Server_CreateConnection \
                           --export=malloc --export=free

EMCC_CFLAGS = -std=gnu11 -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 \
              -Isrc \
              -I$(RAYLIB_INCLUDE_DIR_WEB) \
              -I$(RAYLIB_WEB_DIR)/external/glfw/include \
              -Os -Wall \
              -s USE_GLFW=3 \
              -s ASYNCIFY \
              -s TOTAL_MEMORY=67108864 \
              -s ALLOW_MEMORY_GROWTH=1 \
              -nostdinc \
              -isystem $(EMSDK)/upstream/emscripten/cache/sysroot/include \
              -isystem $(EMSDK)/upstream/emscripten/system/include

# Linker flags for Native
SYSTEM_LDFLAGS_NATIVE_RELEASE = -lopengl32 -lgdi32 -lwinmm -lkernel32 -luser32 -lshell32 -ladvapi32 -lole32 \
                                -Wl,/subsystem:windows
SYSTEM_LDFLAGS_NATIVE_DEBUG = -lopengl32 -lgdi32 -lwinmm -lkernel32 -luser32 -lshell32 -ladvapi32 -lole32 \
                              -Wl,/subsystem:console

# Emscripten Linker flags
EMCC_LDFLAGS = --shell-file web/minshell.html \
               --preload-file $(ASSETS_DIR)@/ \
               -s EXPORTED_FUNCTIONS="['_main','_malloc','_free']" \
               -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
               -s ENVIRONMENT="web,worker" \
               -s FORCE_FILESYSTEM=1 \
               -s DISABLE_EXCEPTION_CATCHING=1

RELEASE_CFLAGS_NATIVE = $(COMMON_CFLAGS_NATIVE) -O3 -flto -DNDEBUG
DEBUG_CFLAGS_NATIVE = $(COMMON_CFLAGS_NATIVE) -g -DDEBUG

# Test build directory
TEST_BUILD_DIR = $(NATIVE_BUILD_DIR)/tests

# Test sources
TEST_DIR = testing
TEST_SRCS = $(TEST_DIR)/test_harness.c \
			$(TEST_DIR)/test_logic_gates.c \
			$(TEST_DIR)/test_components.c \
			$(TEST_DIR)/test_nodes.c \
			$(TEST_DIR)/test_modules.c \
			$(TEST_DIR)/test_actions.c \
			$(TEST_DIR)/test_scenarios.c

# Test object files are placed in TEST_BUILD_DIR
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(TEST_BUILD_DIR)/%.o,$(TEST_SRCS))
TEST_HARNESS_OBJ = $(TEST_BUILD_DIR)/test_harness.o
TEST_LOGIC_GATES_OBJ = $(TEST_BUILD_DIR)/test_logic_gates.o
TEST_COMPONENTS_OBJ = $(TEST_BUILD_DIR)/test_components.o
TEST_NODES_OBJ = $(TEST_BUILD_DIR)/test_nodes.o
TEST_MODULES_OBJ = $(TEST_BUILD_DIR)/test_modules.o
TEST_ACTIONS_OBJ = $(TEST_BUILD_DIR)/test_actions.o
TEST_SCENARIOS_OBJ = $(TEST_BUILD_DIR)/test_scenarios.o

# Test executables
TEST_BINS = $(TEST_BUILD_DIR)/test_logic_gates.exe \
            $(TEST_BUILD_DIR)/test_components.exe \
            $(TEST_BUILD_DIR)/test_nodes.exe \
            $(TEST_BUILD_DIR)/test_modules.exe \
            $(TEST_BUILD_DIR)/test_actions.exe \
            $(TEST_BUILD_DIR)/test_scenarios.exe

# Server object file (debug version) used by tests
SERVER_OBJ_DEBUG = $(NATIVE_BUILD_DIR)/debug/server.o

# Main application object files
APP_OBJS_NATIVE_RELEASE = $(patsubst src/%.c,$(NATIVE_BUILD_DIR)/release/%.o,$(APP_C_FILES_NATIVE))
APP_OBJS_NATIVE_DEBUG = $(patsubst src/%.c,$(NATIVE_BUILD_DIR)/debug/%.o,$(APP_C_FILES_NATIVE))

# Default target
all:
	$(MAKE) test && \
	$(MAKE) release && \
	$(MAKE) debug && \
	$(MAKE) web && \
	$(MAKE) copy-assets-native

# Build all test object files before linking test executables
$(TEST_OBJS): | $(TEST_BUILD_DIR) $(NATIVE_BUILD_DIR)/debug

# Rule for compiling test source files from $(TEST_DIR)/ into $(TEST_BUILD_DIR)/
# Example: build/native/tests/test_harness.o from testing/test_harness.c
$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_BUILD_DIR) $(NATIVE_BUILD_DIR)/debug
	@echo "Compiling Test Object: $< -> $@"
	$(NATIVE_CC) $(DEBUG_CFLAGS_NATIVE) -c $< -o $@

# Rules for linking test executables
$(TEST_BUILD_DIR)/test_logic_gates.exe: $(TEST_HARNESS_OBJ) $(TEST_LOGIC_GATES_OBJ) $(SERVER_OBJ_DEBUG)
	@echo "Linking Test: $@"
	$(NATIVE_CC) $(DEBUG_CFLAGS_NATIVE) $^ $(RAYLIB_LIB_FILE_NATIVE) $(SYSTEM_LDFLAGS_NATIVE_DEBUG) -o $@

$(TEST_BUILD_DIR)/test_components.exe: $(TEST_HARNESS_OBJ) $(TEST_COMPONENTS_OBJ) $(SERVER_OBJ_DEBUG)
	@echo "Linking Test: $@"
	$(NATIVE_CC) $(DEBUG_CFLAGS_NATIVE) $^ $(RAYLIB_LIB_FILE_NATIVE) $(SYSTEM_LDFLAGS_NATIVE_DEBUG) -o $@

$(TEST_BUILD_DIR)/test_nodes.exe: $(TEST_HARNESS_OBJ) $(TEST_NODES_OBJ) $(SERVER_OBJ_DEBUG)
	@echo "Linking Test: $@"
	$(NATIVE_CC) $(DEBUG_CFLAGS_NATIVE) $^ $(RAYLIB_LIB_FILE_NATIVE) $(SYSTEM_LDFLAGS_NATIVE_DEBUG) -o $@

$(TEST_BUILD_DIR)/test_modules.exe: $(TEST_HARNESS_OBJ) $(TEST_MODULES_OBJ) $(SERVER_OBJ_DEBUG)
	@echo "Linking Test: $@"
	$(NATIVE_CC) $(DEBUG_CFLAGS_NATIVE) $^ $(RAYLIB_LIB_FILE_NATIVE) $(SYSTEM_LDFLAGS_NATIVE_DEBUG) -o $@

$(TEST_BUILD_DIR)/test_actions.exe: $(TEST_HARNESS_OBJ) $(TEST_ACTIONS_OBJ) $(SERVER_OBJ_DEBUG)
	@echo "Linking Test: $@"
	$(NATIVE_CC) $(DEBUG_CFLAGS_NATIVE) $^ $(RAYLIB_LIB_FILE_NATIVE) $(SYSTEM_LDFLAGS_NATIVE_DEBUG) -o $@

$(TEST_BUILD_DIR)/test_scenarios.exe: $(TEST_HARNESS_OBJ) $(TEST_SCENARIOS_OBJ) $(SERVER_OBJ_DEBUG)
	@echo "Linking Test: $@"
	$(NATIVE_CC) $(DEBUG_CFLAGS_NATIVE) $^ $(RAYLIB_LIB_FILE_NATIVE) $(SYSTEM_LDFLAGS_NATIVE_DEBUG) -o $@

test: $(TEST_BINS)
	@echo "Running logic gate tests..."
	@$(TEST_BUILD_DIR)/test_logic_gates.exe
	@echo "Running component tests..."
	@$(TEST_BUILD_DIR)/test_components.exe
	@echo "Running node tests..."
	@$(TEST_BUILD_DIR)/test_nodes.exe
	@echo "Running module tests..."
	@$(TEST_BUILD_DIR)/test_modules.exe
	@echo "Running action/effect tests..."
	@$(TEST_BUILD_DIR)/test_actions.exe
	@echo "Running scenario tests..."
	@$(TEST_BUILD_DIR)/test_scenarios.exe

# Ensure test build directory exists
$(TEST_BUILD_DIR): | $(NATIVE_BUILD_DIR)
	@mkdir -p $(TEST_BUILD_DIR)

# Rule for compiling native source files from src/ into NATIVE_BUILD_DIR
# Example: build/native/release/main.o from src/main.c
$(NATIVE_BUILD_DIR)/release/%.o: src/%.c | $(NATIVE_BUILD_DIR)/release
	@echo "Compiling Native (Release): $< -> $@"
	$(NATIVE_CC) $(RELEASE_CFLAGS_NATIVE) -c $< -o $@

$(NATIVE_BUILD_DIR)/debug/%.o: src/%.c | $(NATIVE_BUILD_DIR)/debug
	@echo "Compiling Native (Debug): $< -> $@"
	$(NATIVE_CC) $(DEBUG_CFLAGS_NATIVE) -c $< -o $@

# Ensure release and debug directories exist
$(NATIVE_BUILD_DIR)/release:
	@mkdir -p $(NATIVE_BUILD_DIR)/release

$(NATIVE_BUILD_DIR)/debug:
	@mkdir -p $(NATIVE_BUILD_DIR)/debug

# Build targets for main application
release: $(APP_OBJS_NATIVE_RELEASE) $(RELEASE_EXE)

debug: $(APP_OBJS_NATIVE_DEBUG) $(DEBUG_EXE)

# Rules for application object files
$(APP_OBJS_NATIVE_RELEASE): | $(NATIVE_BUILD_DIR)/release
$(APP_OBJS_NATIVE_DEBUG): | $(NATIVE_BUILD_DIR)/debug

# Main application executables
$(RELEASE_EXE): $(APP_OBJS_NATIVE_RELEASE) $(RAYLIB_LIB_FILE_NATIVE)
	@echo "Linking Native (Release): $@"
	$(NATIVE_CC) $(APP_OBJS_NATIVE_RELEASE) $(RAYLIB_LIB_FILE_NATIVE) $(SYSTEM_LDFLAGS_NATIVE_RELEASE) -o $@

$(DEBUG_EXE): $(APP_OBJS_NATIVE_DEBUG) $(RAYLIB_LIB_FILE_NATIVE)
	@echo "Linking Native (Debug): $@"
	$(NATIVE_CC) $(APP_OBJS_NATIVE_DEBUG) $(RAYLIB_LIB_FILE_NATIVE) $(SYSTEM_LDFLAGS_NATIVE_DEBUG) -o $@

wasm_logic: $(WASM_SERVER_LOGIC_OUTPUT)
$(WASM_SERVER_LOGIC_OUTPUT): $(APP_C_FILES_WASM_LOGIC) | $(WASM_LOGIC_BUILD_DIR)
	@echo "Compiling Server Logic to WASM: $(APP_C_FILES_WASM_LOGIC) -> $@"
	$(WASM_LOGIC_CC) $(COMMON_CFLAGS_WASM_LOGIC) $(APP_C_FILES_WASM_LOGIC) -o $@

$(WEB_APP_HTML_FILE): $(APP_C_FILES_WEB) | $(WEB_APP_BUILD_DIR)
	@echo "Compiling Full Application for Web to $(WEB_APP_HTML_FILE)..."
	$(EMCC) $(EMCC_CFLAGS) $(APP_C_FILES_WEB) $(RAYLIB_LIB_FILE_WEB) $(EMCC_LDFLAGS) -o $(WEB_APP_HTML_FILE)

copy-assets-native: $(ASSETS_DIR) | $(NATIVE_BUILD_DIR)
	@echo "Copying assets to $(TARGET_ASSETS_NATIVE_DIR)..."
	rm -rf $(TARGET_ASSETS_NATIVE_DIR)
	mkdir -p $(TARGET_ASSETS_NATIVE_DIR)
	cp -rv $(ASSETS_DIR)/. $(TARGET_ASSETS_NATIVE_DIR)/

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) 
	@echo "Clean complete."

compile_commands:
	@echo "Generating compile_commands.json for native debug build..."
	compiledb -n make debug
	@echo "compile_commands.json generated."

run-release: release
	@echo "Running Release: $(RELEASE_EXE)"
	@cd $(NATIVE_BUILD_DIR) && ./$(notdir $(RELEASE_EXE))
run-debug: debug
	@echo "Running Debug: $(DEBUG_EXE)"
	@cd $(NATIVE_BUILD_DIR) && ./$(notdir $(DEBUG_EXE))

serve-web: web
	@echo "Serving web build from $(WEB_APP_BUILD_DIR) on http://localhost:8000"
	@cd $(WEB_APP_BUILD_DIR) && python3 -m http.server 8000

$(NATIVE_BUILD_DIR):
	@mkdir -p $(NATIVE_BUILD_DIR)

.PHONY: all clean release debug web wasm_logic run-release run-debug compile_commands copy-assets-native test

