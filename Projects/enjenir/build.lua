local M = {}

-- Helper function to execute shell commands
function M.run_command(cmd_str)
    print("Executing: " .. cmd_str)
    -- For Windows, complex commands with && or cd might need `cmd /C "..."`
    -- For POSIX, `sh -c "..."` can be used if needed, but os.execute often handles simple cases.
    -- The current implementation directly passes cmd_str, which is fine for single commands.
    -- For commands involving `cd` then another command, they are wrapped in platform-specific shells later.
    local ok, err, code = os.execute(cmd_str)
    if not ok then
        print("Error executing command: " .. cmd_str)
        if err then print("  Error reason: " .. tostring(err)) end -- 'err' can be a string or number
        if code then print("  Exit code: " .. code) end
        -- os.exit(code or 1) -- Consider uncommenting if build should stop on any error
    end
    return ok, code
end

-- Helper function to create directories if they don't exist (mkdir -p)
function M.mkdir_p(path)
    -- Try Lua's os.execute for simple directory creation, fallback to shell for recursive
    local ok = os.execute('mkdir "' .. path .. '" >nul 2>nul')
    if not ok then
        if package.config:sub(1,1) == "\\" then -- Windows
            M.run_command('mkdir "' .. path:gsub("/", "\\") .. '"')
        else
            M.run_command("mkdir -p '" .. path .. "'")
        end
    end
end

-- Helper function to remove directories/files recursively (rm -rf)
function M.remove_recursive(path)
    -- Try Lua's os.remove for files, then fallback to shell for directories
    local function is_file(p)
        local f = io.open(p, 'r')
        if f then f:close() return true end
        return false
    end
    if is_file(path) then
        os.remove(path)
    else
        if package.config:sub(1,1) == "\\" then -- Windows
            M.run_command('rmdir /s /q "' .. path:gsub("/", "\\") .. '"')
        else
            M.run_command("rm -rf '" .. path .. "'")
        end
    end
end

-- Helper function to copy files/directories recursively (cp -rv)
function M.copy_recursive(src, dest)
    if package.config:sub(1,1) == "\\" then -- Windows
        M.run_command('xcopy "' .. src:gsub("/", "\\") .. '\\*" "' .. dest:gsub("/", "\\") .. '" /S /E /Y /I /D')
    else
        M.run_command("cp -R '" .. src .. "' '" .. dest .. "'")
    end
end

-- Project Configuration
M.config = {
    PROJECT_NAME = "enjenir",

    BUILD_DIR = "build",
    NATIVE_BUILD_DIR = "build/native",
    WASM_LOGIC_BUILD_DIR = "build/wasm_logic",
    WEB_APP_BUILD_DIR = "build/web_app",

    RELEASE_EXE_NAME = "enjenir.exe",
    DEBUG_EXE_NAME = "enjenir_debug.exe",

    WASM_SERVER_LOGIC_OUTPUT_NAME = "server_logic.wasm",
    WEB_APP_HTML_FILE_NAME = "index.html",

    ASSETS_DIR = "assets",

    RAYLIB_WIN64_DIR = "lib/raylib-5.5_win64_mingw-w64",
    RAYLIB_LINUX_DIR = "lib/raylib-5.5_linux_amd64",
    RAYLIB_WEB_DIR = "lib/raylib-5.5_webassembly",

    RAYLIB_INCLUDE_DIR_NATIVE = "lib/raylib-5.5_win64_mingw-w64/include",
    RAYLIB_LIB_FILE_NATIVE = "lib/raylib-5.5_win64_mingw-w64/lib/libraylib.a",
    RAYLIB_INCLUDE_DIR_WEB = "lib/raylib-5.5_webassembly/include",
    -- RAYLIB_LIB_FILE_WEB will be derived

    ZIG_CC = "zig cc",
    EMSDK = os.getenv("EMSDK") or "C:/Users/tcunkle/scoop/apps/emscripten/current", -- Use EMSDK env var if set
    EMCC = "", -- Will be constructed

    NATIVE_TARGET_FLAGS = "-target x86_64-windows-gnu",
    NATIVE_CC = "",

    WASM_LOGIC_TARGET_FLAGS = "-target wasm32-freestanding -nostdlib",
    WASM_LOGIC_CC = "",

    APP_C_FILES_NATIVE = {"src/main.c", "src/client.c", "src/server.c"},
    APP_C_FILES_WASM_LOGIC = {"src/server.c"},
    APP_C_FILES_WEB = {"src/main.c", "src/client.c", "src/server.c"},

    COMMON_CFLAGS_NATIVE_FORMAT = "-std=c11 -DPLATFORM_DESKTOP -DNOGDI -Isrc -I%s",
    COMMON_CFLAGS_WASM_LOGIC = "-std=c11 -DTOOL_WASM_BUILD -Isrc -O2 " ..
                               "--export=Server_Init --export=Server_Update " ..
                               "--export=Server_PlayCardFromHand --export=Server_InteractWithComponent " ..
                               "--export=Server_PlayerDrawCard --export=Server_CreateConnection " ..
                               "--export=malloc --export=free",

    EMCC_CFLAGS_FORMAT = "-std=gnu11 -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 " ..
                         "-Isrc -I%s -I%s/external/glfw/include -Os -Wall " ..
                         "-s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=67108864 " ..
                         "-s ALLOW_MEMORY_GROWTH=1 -nostdinc " ..
                         "-isystem %s/upstream/emscripten/cache/sysroot/include " ..
                         "-isystem %s/upstream/emscripten/system/include",

    SYSTEM_LDFLAGS_NATIVE_RELEASE = "-lopengl32 -lgdi32 -lwinmm -lkernel32 -luser32 -lshell32 -ladvapi32 -lole32 " ..
                                    "-Wl,/subsystem:windows",
    SYSTEM_LDFLAGS_NATIVE_DEBUG = "-lopengl32 -lgdi32 -lwinmm -lkernel32 -luser32 -lshell32 -ladvapi32 -lole32 " ..
                                  "-Wl,/subsystem:console",

    EMCC_LDFLAGS_FORMAT = "--shell-file web/minshell.html --preload-file %s@/ " ..
                          "-s " .. [[EXPORTED_FUNCTIONS="['_main','_malloc','_free']"]] .. " " ..
                          "-s " .. [[EXPORTED_RUNTIME_METHODS="['ccall','cwrap']"]] .. " " ..
                          "-s " .. [[ENVIRONMENT="web,worker"]] .. " -s FORCE_FILESYSTEM=1 " ..
                          "-s DISABLE_EXCEPTION_CATCHING=1",

    RELEASE_CFLAGS_NATIVE_EXTRA = "-O3 -flto -DNDEBUG",
    DEBUG_CFLAGS_NATIVE_EXTRA = "-g -DDEBUG",

    TEST_BUILD_DIR_NATIVE = "build/native/tests",
    TEST_DIR = "testing",
    TEST_HARNESS_SRC = "testing/test_harness.c",
    SERVER_SRC_FOR_TESTS = "src/server.c",
    TEST_TARGETS = {
        test_logic_gates = {src_files = {"testing/test_logic_gates.c"}, run_message = "Running logic gate tests..."},
        test_components = {src_files = {"testing/test_components.c"}, run_message = "Running component tests..."},
        test_nodes = {src_files = {"testing/test_nodes.c"}, run_message = "Running node tests..."},
        test_modules = {src_files = {"testing/test_modules.c"}, run_message = "Running module tests..."},
        test_actions = {src_files = {"testing/test_actions.c"}, run_message = "Running action/effect tests..."},
        test_scenarios = {src_files = {"testing/test_scenarios.c"}, run_message = "Running scenario tests..."},
    },
}

-- Derived paths and commands
M.config.RELEASE_EXE = M.config.NATIVE_BUILD_DIR .. "/" .. M.config.RELEASE_EXE_NAME
M.config.DEBUG_EXE = M.config.NATIVE_BUILD_DIR .. "/" .. M.config.DEBUG_EXE_NAME
M.config.WASM_SERVER_LOGIC_OUTPUT = M.config.WASM_LOGIC_BUILD_DIR .. "/" .. M.config.WASM_SERVER_LOGIC_OUTPUT_NAME
M.config.WEB_APP_HTML_FILE = M.config.WEB_APP_BUILD_DIR .. "/" .. M.config.WEB_APP_HTML_FILE_NAME
M.config.TARGET_ASSETS_NATIVE_DIR = M.config.NATIVE_BUILD_DIR .. "/" .. M.config.ASSETS_DIR
M.config.RAYLIB_LIB_FILE_WEB = M.config.RAYLIB_WEB_DIR .. "/lib/libraylib.a"

M.config.EMCC = M.config.EMSDK .. "/upstream/emscripten/emcc"
M.config.NATIVE_CC = M.config.ZIG_CC .. " " .. M.config.NATIVE_TARGET_FLAGS
M.config.WASM_LOGIC_CC = M.config.ZIG_CC .. " " .. M.config.WASM_LOGIC_TARGET_FLAGS

M.config.COMMON_CFLAGS_NATIVE = string.format(M.config.COMMON_CFLAGS_NATIVE_FORMAT, M.config.RAYLIB_INCLUDE_DIR_NATIVE)
M.config.RELEASE_CFLAGS_NATIVE = M.config.COMMON_CFLAGS_NATIVE .. " " .. M.config.RELEASE_CFLAGS_NATIVE_EXTRA
M.config.DEBUG_CFLAGS_NATIVE = M.config.COMMON_CFLAGS_NATIVE .. " " .. M.config.DEBUG_CFLAGS_NATIVE_EXTRA

M.config.EMCC_CFLAGS = string.format(M.config.EMCC_CFLAGS_FORMAT,
    M.config.RAYLIB_INCLUDE_DIR_WEB, M.config.RAYLIB_WEB_DIR, M.config.EMSDK, M.config.EMSDK)
M.config.EMCC_LDFLAGS = string.format(M.config.EMCC_LDFLAGS_FORMAT, M.config.ASSETS_DIR)

-- Function to get object file path from source file path
function M.get_obj_path(src_file, base_build_dir, sub_dir)
    local file_name = src_file:match("([^/]+)$"):match("([^%.]+)")
    local path_sep = package.config:sub(1,1) == "\\" and "\\" or "/"
    local actual_sub_dir = sub_dir and (sub_dir .. path_sep) or ""
    if actual_sub_dir == "" then
        return base_build_dir .. path_sep .. file_name .. ".o"
    else
        return base_build_dir .. path_sep .. actual_sub_dir .. file_name .. ".o"
    end
end

M.config.SERVER_OBJ_DEBUG_PATH = M.get_obj_path(M.config.SERVER_SRC_FOR_TESTS, M.config.NATIVE_BUILD_DIR, "debug")

-- Function to get test object file path
function M.get_test_obj_path(src_file)
    local file_name = src_file:match("([^/]+)$"):match("([^%.]+)")
    local path_sep = package.config:sub(1,1) == "\\" and "\\" or "/"
    return M.config.TEST_BUILD_DIR_NATIVE .. path_sep .. file_name .. ".o"
end

-- Function to get test executable path
function M.get_test_exe_path(test_name)
    local path_sep = package.config:sub(1,1) == "\\" and "\\" or "/"
    return M.config.TEST_BUILD_DIR_NATIVE .. path_sep .. test_name .. ".exe"
end

-- Targets
function M.clean()
    print("Cleaning build artifacts...")
    M.remove_recursive(M.config.BUILD_DIR)
    print("Clean complete.")
end

function M.ensure_build_dirs()
    M.mkdir_p(M.config.BUILD_DIR)
    M.mkdir_p(M.config.NATIVE_BUILD_DIR)
    M.mkdir_p(M.config.NATIVE_BUILD_DIR .. "/release")
    M.mkdir_p(M.config.NATIVE_BUILD_DIR .. "/debug")
    M.mkdir_p(M.config.WASM_LOGIC_BUILD_DIR)
    M.mkdir_p(M.config.WEB_APP_BUILD_DIR)
    M.mkdir_p(M.config.TEST_BUILD_DIR_NATIVE)
end

function M.compile_native_obj(src_file, obj_file, build_type_flags)
    print("Compiling Native: " .. src_file .. " -> " .. obj_file)
    local cmd = string.format("%s %s -c %s -o %s", M.config.NATIVE_CC, build_type_flags, src_file, obj_file)
    M.run_command(cmd)
end

function M.build_native(build_type)
    M.ensure_build_dirs()
    local build_type_upper = build_type:upper()
    print("Building Native (" .. build_type_upper .. ")...")
    local cflags = (build_type == "release") and M.config.RELEASE_CFLAGS_NATIVE or M.config.DEBUG_CFLAGS_NATIVE
    local ldflags = (build_type == "release") and M.config.SYSTEM_LDFLAGS_NATIVE_RELEASE or M.config.SYSTEM_LDFLAGS_NATIVE_DEBUG
    local exe_path = (build_type == "release") and M.config.RELEASE_EXE or M.config.DEBUG_EXE
    local obj_subdir = build_type

    local app_obj_files = {}
    for _, src_file in ipairs(M.config.APP_C_FILES_NATIVE) do
        local obj_file = M.get_obj_path(src_file, M.config.NATIVE_BUILD_DIR, obj_subdir)
        table.insert(app_obj_files, obj_file)
        M.compile_native_obj(src_file, obj_file, cflags)
    end

    print("Linking Native (" .. build_type_upper .. "): " .. exe_path)
    local link_cmd = string.format("%s %s %s %s -o %s", M.config.NATIVE_CC, table.concat(app_obj_files, " "), M.config.RAYLIB_LIB_FILE_NATIVE, ldflags, exe_path)
    M.run_command(link_cmd)
end

function M.release() M.build_native("release") end
function M.debug() M.build_native("debug") end

function M.copy_assets_native()
    M.ensure_build_dirs()
    print("Copying assets to " .. M.config.TARGET_ASSETS_NATIVE_DIR .. "...")
    M.remove_recursive(M.config.TARGET_ASSETS_NATIVE_DIR)
    M.mkdir_p(M.config.TARGET_ASSETS_NATIVE_DIR)
    M.copy_recursive(M.config.ASSETS_DIR, M.config.TARGET_ASSETS_NATIVE_DIR)
end

function M.wasm_logic()
    M.ensure_build_dirs()
    print("Compiling Server Logic to WASM: " .. table.concat(M.config.APP_C_FILES_WASM_LOGIC, " ") .. " -> " .. M.config.WASM_SERVER_LOGIC_OUTPUT)
    local cmd = string.format("%s %s %s -o %s", M.config.WASM_LOGIC_CC, M.config.COMMON_CFLAGS_WASM_LOGIC, table.concat(M.config.APP_C_FILES_WASM_LOGIC, " "), M.config.WASM_SERVER_LOGIC_OUTPUT)
    M.run_command(cmd)
end

function M.web()
    M.ensure_build_dirs()
    print("Compiling Full Application for Web to " .. M.config.WEB_APP_HTML_FILE .. "...")
    local cmd = string.format("%s %s %s %s %s -o %s", M.config.EMCC, M.config.EMCC_CFLAGS, table.concat(M.config.APP_C_FILES_WEB, " "), M.config.RAYLIB_LIB_FILE_WEB, M.config.EMCC_LDFLAGS, M.config.WEB_APP_HTML_FILE)
    M.run_command(cmd)
end

function M.compile_server_obj_for_tests()
    M.ensure_build_dirs()
    local src_file = M.config.SERVER_SRC_FOR_TESTS
    local obj_file = M.config.SERVER_OBJ_DEBUG_PATH
    print("Compiling Server Object for Tests (Debug): " .. src_file .. " -> " .. obj_file)
    M.compile_native_obj(src_file, obj_file, M.config.DEBUG_CFLAGS_NATIVE)
end

function M.compile_test_obj(src_file)
    M.ensure_build_dirs()
    local obj_file = M.get_test_obj_path(src_file)
    print("Compiling Test Object: " .. src_file .. " -> " .. obj_file)
    local cmd = string.format("%s %s -c %s -o %s", M.config.NATIVE_CC, M.config.DEBUG_CFLAGS_NATIVE, src_file, obj_file)
    M.run_command(cmd)
end

function M.link_test_executable(test_name, specific_test_obj_files)
    M.ensure_build_dirs()
    local exe_path = M.get_test_exe_path(test_name)
    local harness_obj = M.get_test_obj_path(M.config.TEST_HARNESS_SRC)
    local server_obj_debug = M.config.SERVER_OBJ_DEBUG_PATH

    local all_obj_files = {harness_obj}
    for _, obj in ipairs(specific_test_obj_files) do table.insert(all_obj_files, obj) end
    table.insert(all_obj_files, server_obj_debug)

    -- Ensure all object paths use the correct separator for the command line
    for i, f_path in ipairs(all_obj_files) do
        all_obj_files[i] = f_path:gsub("/", package.config:sub(1,1))
    end
    local raylib_lib_file_native_plat = M.config.RAYLIB_LIB_FILE_NATIVE:gsub("/", package.config:sub(1,1))
    local exe_path_plat = exe_path:gsub("/", package.config:sub(1,1))

    print("Linking Test: " .. exe_path)
    local link_cmd_str = string.format("%s %s %s %s %s -o %s",
        M.config.NATIVE_CC,
        M.config.DEBUG_CFLAGS_NATIVE,
        table.concat(all_obj_files, " "),
        raylib_lib_file_native_plat,
        M.config.SYSTEM_LDFLAGS_NATIVE_DEBUG,
        exe_path_plat
    )
    M.run_command(link_cmd_str)
    return exe_path
end

function M.test()
    M.ensure_build_dirs()
    print("Building tests...")
    M.compile_server_obj_for_tests()
    M.compile_test_obj(M.config.TEST_HARNESS_SRC)

    local tests_to_run = {}
    local test_order = {
        "test_logic_gates", "test_components", "test_nodes",
        "test_modules", "test_actions", "test_scenarios"
    }

    for _, test_name_key in ipairs(test_order) do
        local test_info = M.config.TEST_TARGETS[test_name_key]
        if test_info then
            local test_name_str = tostring(test_name_key)
            local specific_test_objs = {}
            for _, src in ipairs(test_info.src_files) do
                M.compile_test_obj(src)
                table.insert(specific_test_objs, M.get_test_obj_path(src))
            end
            local exe_path = M.link_test_executable(test_name_str, specific_test_objs)
            table.insert(tests_to_run, {message = test_info.run_message, exe = exe_path})
        else
            print("Warning: Test information for " .. test_name_key .. " not found in M.config.TEST_TARGETS.")
        end
    end

    print("\nRunning tests...")
    for _, run_info in ipairs(tests_to_run) do
        print(run_info.message)
        local exe_to_run_cmd = run_info.exe:gsub("/", package.config:sub(1,1))
        if package.config:sub(1,1) == "\\" then
            exe_to_run_cmd = '"' .. exe_to_run_cmd .. '"'
        end
        M.run_command(exe_to_run_cmd)
    end
    print("All tests finished.")
end

function M.all()
    print("Building all targets...")
    M.test()
    M.release()
    M.debug()
    M.web()
    M.copy_assets_native()
    print("All targets built successfully.")
end

function M.run_target(build_func, exe_path_str, exe_name_str, relative_dir_str, run_message_format)
    if build_func then build_func() end
    print(string.format(run_message_format, exe_path_str))
    local cmd
    local dir_to_cd = relative_dir_str:gsub('/', package.config:sub(1,1))
    local exe_to_run = (package.config:sub(1,1) == '\\' and ".\\" or "./") .. exe_name_str

    if package.config:sub(1,1) == '\\' then -- Windows
        cmd = string.format('cmd /C "cd /D \"%s\" && \"%s\""', dir_to_cd, exe_to_run)
    else -- POSIX
        cmd = string.format('sh -c "cd \'%s\' && \'%s\'"', dir_to_cd, exe_to_run)
    end
    M.run_command(cmd)
end

function M.run_release()
    M.run_target(M.release, M.config.RELEASE_EXE, M.config.RELEASE_EXE_NAME, M.config.NATIVE_BUILD_DIR, "Running Release: %s")
end

function M.run_debug()
    M.run_target(M.debug, M.config.DEBUG_EXE, M.config.DEBUG_EXE_NAME, M.config.NATIVE_BUILD_DIR, "Running Debug: %s")
end

function M.serve_web()
    M.web()
    print("Serving web build from " .. M.config.WEB_APP_BUILD_DIR .. " on http://localhost:8000")
    local web_dir = M.config.WEB_APP_BUILD_DIR:gsub('/', package.config:sub(1,1))
    
    local py_executable = os.getenv("PYTHON") 
    if not py_executable or py_executable == "" then
        py_executable = (package.config:sub(1,1) == '\\' and "python" or "python3")
    end
    local py_command_args = " -m http.server 8000"

    local cmd
    if package.config:sub(1,1) == '\\' then -- Windows
        cmd = string.format('cmd /C "cd /D \"%s\" && \"%s\"%s"', web_dir, py_executable, py_command_args)
    else -- POSIX
        cmd = string.format('sh -c "cd \'%s\' && \'%s\'%s"', web_dir, py_executable, py_command_args)
    end
    print("Note: This will block. Press Ctrl+C to stop the server.")
    M.run_command(cmd)
end

-- Helper to split string by delimiter, for CFLAGS
local function split_string(inputstr, sep)
    if sep == nil then
        sep = "%s"
    end
    local t={}
    for str in string.gmatch(inputstr, "([^"..sep.."]+)") do
        table.insert(t, str)
    end
    return t
end

function M.compile_commands()
    print("Generating compile_commands.json...")
    M.ensure_build_dirs() -- Ensure debug directory exists for paths

    local commands = {}
    local cwd = os.getenv("PWD") or os.getenv("CD") or "." -- Get current working directory
    -- Normalize CWD to use forward slashes for consistency in compile_commands.json
    cwd = cwd:gsub("\\\\", "/")

    local native_cc_parts = split_string(M.config.NATIVE_CC, " ")
    local debug_cflags_parts = split_string(M.config.DEBUG_CFLAGS_NATIVE, " ")

    -- Function to add a compile command entry
    local function add_command(src_file, obj_file, cc_parts, cflags_parts)
        local full_src_path = src_file -- Assuming src_file is relative to CWD or already absolute
        -- If src_file is not absolute, make it so relative to CWD
        if not (full_src_path:match("^/") or full_src_path:match("^[A-Za-z]:")) then
            full_src_path = cwd .. "/" .. src_file
        end
        full_src_path = full_src_path:gsub("\\\\", "/")
        
        local arguments = {}
        for _, p in ipairs(cc_parts) do table.insert(arguments, p) end
        for _, p in ipairs(cflags_parts) do table.insert(arguments, p) end
        table.insert(arguments, "-c")
        table.insert(arguments, full_src_path) -- Use full path for file
        table.insert(arguments, "-o")
        table.insert(arguments, obj_file:gsub("\\\\", "/"))

        table.insert(commands, {
            directory = cwd,
            file = full_src_path,
            output = obj_file:gsub("\\\\", "/"),
            arguments = arguments
        })
    end

    -- 1. Native application object files (debug)
    for _, src_file in ipairs(M.config.APP_C_FILES_NATIVE) do
        local obj_file = M.get_obj_path(src_file, M.config.NATIVE_BUILD_DIR, "debug")
        add_command(src_file, obj_file, native_cc_parts, debug_cflags_parts)
    end

    -- 2. Server object file for tests (debug)
    -- Check if server.c is already in APP_C_FILES_NATIVE, if so, it's already added.
    local server_in_app = false
    for _, app_src in ipairs(M.config.APP_C_FILES_NATIVE) do
        if app_src == M.config.SERVER_SRC_FOR_TESTS then
            server_in_app = true
            break
        end
    end
    if not server_in_app then
         add_command(M.config.SERVER_SRC_FOR_TESTS, M.config.SERVER_OBJ_DEBUG_PATH, native_cc_parts, debug_cflags_parts)
    end

    -- 3. Test harness object file
    add_command(M.config.TEST_HARNESS_SRC, M.get_test_obj_path(M.config.TEST_HARNESS_SRC), native_cc_parts, debug_cflags_parts)

    -- 4. Individual test source files
    for _, test_info in pairs(M.config.TEST_TARGETS) do
        for _, src_file in ipairs(test_info.src_files) do
            add_command(src_file, M.get_test_obj_path(src_file), native_cc_parts, debug_cflags_parts)
        end
    end

    -- Serialize to JSON (basic manual JSON generation for simplicity)
    local json_parts = {}
    table.insert(json_parts, "[")
    for i, cmd_entry in ipairs(commands) do
        table.insert(json_parts, "  {")
        table.insert(json_parts, string.format("    \"directory\": \"%s\",", cmd_entry.directory:gsub("\\\\", "\\\\\\")))
        table.insert(json_parts, string.format("    \"file\": \"%s\",", cmd_entry.file:gsub("\\\\", "\\\\\\")))
        table.insert(json_parts, string.format("    \"output\": \"%s\",", cmd_entry.output:gsub("\\\\", "\\\\\\")))
        table.insert(json_parts, "    \"arguments\": [")
        local arg_json_parts = {}
        for _, arg_val in ipairs(cmd_entry.arguments) do
            table.insert(arg_json_parts, string.format("      \"%s\"", arg_val:gsub("\"", "\\\""):gsub("\\\\", "\\\\\\")))
        end
        table.insert(json_parts, table.concat(arg_json_parts, ",\n"))
        table.insert(json_parts, "    ]")
        table.insert(json_parts, "  }")
        if i < #commands then
            table.insert(json_parts, ",")
        end
    end
    table.insert(json_parts, "]")

    local file, err = io.open("compile_commands.json", "w")
    if file then
        file:write(table.concat(json_parts, "\n"))
        file:close()
        print("compile_commands.json generated successfully.")
    else
        print("Error writing compile_commands.json: " .. (err or "unknown error"))
    end
end


-- Main execution logic
local args = {...} -- Capture command line arguments passed to the script
if #args > 0 then
    local target_name = args[1]
    local target_func = M[target_name]
    if target_func and type(target_func) == "function" then
        target_func()
    else
        print("Unknown target: " .. target_name)
        io.write("Available targets: all, clean, release, debug, web, wasm_logic, copy_assets_native, test,\n")
        io.write("                 run-release, run-debug, serve-web, compile_commands\n")
    end
else
    print("No target specified. Building default target: all")
    M.all()
end

return M
