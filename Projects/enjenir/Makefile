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

all: release debug web copy-assets-native
release: $(RELEASE_EXE) copy-assets-native
debug: $(DEBUG_EXE) copy-assets-native
web: $(WEB_APP_HTML_FILE) 

$(NATIVE_BUILD_DIR):
	@mkdir -p $(NATIVE_BUILD_DIR)
$(NATIVE_BUILD_DIR)/release: | $(NATIVE_BUILD_DIR)
	@mkdir -p $(NATIVE_BUILD_DIR)/release
$(NATIVE_BUILD_DIR)/debug: | $(NATIVE_BUILD_DIR)
	@mkdir -p $(NATIVE_BUILD_DIR)/debug
$(WASM_LOGIC_BUILD_DIR):
	@mkdir -p $(WASM_LOGIC_BUILD_DIR)
$(WEB_APP_BUILD_DIR):
	@mkdir -p $(WEB_APP_BUILD_DIR)

APP_OBJS_NATIVE_RELEASE = $(patsubst src/%.c,$(NATIVE_BUILD_DIR)/release/%.o,$(APP_C_FILES_NATIVE))
APP_OBJS_NATIVE_DEBUG = $(patsubst src/%.c,$(NATIVE_BUILD_DIR)/debug/%.o,$(APP_C_FILES_NATIVE))

$(NATIVE_BUILD_DIR)/release/%.o: src/%.c | $(NATIVE_BUILD_DIR)/release
	@echo "Compiling Native (Release): $< -> $@"
	$(NATIVE_CC) $(RELEASE_CFLAGS_NATIVE) -c $< -o $@

$(NATIVE_BUILD_DIR)/debug/%.o: src/%.c | $(NATIVE_BUILD_DIR)/debug
	@echo "Compiling Native (Debug): $< -> $@"
	$(NATIVE_CC) $(DEBUG_CFLAGS_NATIVE) -c $< -o $@

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

.PHONY: all clean release debug web wasm_logic run-release run-debug compile_commands copy-assets-native $(RAYLIB_NATIVE_LIB_FILE)

