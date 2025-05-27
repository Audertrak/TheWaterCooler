import os
import shutil
import json
import sys
import subprocess
import glob

# List of required executables
executables = [
    "aarch64-linux-android28-clang",
    "llvm-ar",
    "aapt2",
    "aapt",
    "zipalign",
    "apksigner",
    "keytool",
    "cp",
    "mkdir",
    "rm"
]

# List of required libraries and output artifacts
libraries = [
    "lib/raylib/lib/android/arm64-v8a/libraylib.a",
    "build/android/lib/arm64-v8a/libenjenir.so"
]

# List of required source files
source_files = [
    "src/main.c",
    "src/client.c",
    "src/server.c",
    "lib/raylib/src/rcore.c",
    "lib/raylib/src/rshapes.c",
    "lib/raylib/src/rtextures.c",
    "lib/raylib/src/rtext.c",
    "lib/raylib/src/rmodels.c",
    "lib/raylib/src/utils.c",
    "lib/raylib/src/raudio.c",
    "lib/raylib/projects/VS2019-Android/raylib_android/raylib_android.NativeActivity/android_native_app_glue.h",
    "lib/raylib/projects/VS2019-Android/raylib_android/raylib_android.NativeActivity/android_native_app_glue.c"
]

# List of required environment variables
env_vars = [
    "ANDROID_HOME",
    "ANDROID_NDK_VERSION",
    "ANDROID_NDK",
    "JAVA_HOME",
    "ANDROID_SDK_BUILD_TOOLS_VERSION",
    "PATH"
]

# Key environment info (static for now, could be loaded from Makefile)
environment_info = {
    "TARGET_ABI": "arm64-v8a",
    "API_LEVEL": 28,
    "ANDROID_SYSROOT": "C:/Users/Tim/scoop/apps/android-clt/current/ndk/25.2.9519653/toolchains/llvm/prebuilt/windows-x86_64/sysroot",
    "ANDROID_BUILD_DIR": "build/android",
    "ANDROID_NATIVE_LIB_DIR": "build/android/lib/arm64-v8a",
    "ANDROID_EXE_NAME": "libenjenir.so",
    "ANDROID_EXE": "build/android/lib/arm64-v8a/libenjenir.so",
    "APK_NAME": "build/enjenir.apk",
    "ANDROID_ASSETS_DIR": "build/android/assets",
    "ANDROID_MANIFEST": "AndroidManifest.xml",
    "ANDROID_RES_DIR": "res"
}

# Helper to check for executables, searching extra Android SDK/NDK locations if needed

def find_android_tool(tool_name):
    # 1. Try PATH first
    exe_path = shutil.which(tool_name)
    if exe_path:
        return exe_path
    env = os.environ
    # 2. Search build-tools/*
    android_home = env.get("ANDROID_HOME")
    if android_home:
        build_tools_dir = os.path.join(android_home, "build-tools")
        if os.path.isdir(build_tools_dir):
            for root, dirs, files in os.walk(build_tools_dir):
                if tool_name in files:
                    return os.path.join(root, tool_name)
    # 3. Search NDK/toolchains/*/prebuilt/*/bin
    ndk = env.get("ANDROID_NDK")
    if ndk and os.path.isdir(ndk):
        toolchains_dir = os.path.join(ndk, "toolchains")
        if os.path.isdir(toolchains_dir):
            for tc in os.listdir(toolchains_dir):
                prebuilt_dir = os.path.join(toolchains_dir, tc, "prebuilt")
                if os.path.isdir(prebuilt_dir):
                    for pre in os.listdir(prebuilt_dir):
                        bin_dir = os.path.join(prebuilt_dir, pre, "bin")
                        if os.path.isdir(bin_dir):
                            candidate = os.path.join(bin_dir, tool_name)
                            if os.path.isfile(candidate):
                                return candidate
    # 4. Search NDK root bin
    if ndk:
        ndk_bin = os.path.join(ndk, "bin")
        if os.path.isdir(ndk_bin):
            candidate = os.path.join(ndk_bin, tool_name)
            if os.path.isfile(candidate):
                return candidate
    return None

# Use enhanced search for all required executables
found_executables = {}
for exe in executables:
    found_executables[exe] = find_android_tool(exe)

# Helper to check for files
found_libraries = {}
for lib in libraries:
    found_libraries[lib] = os.path.exists(lib)

found_sources = {}
for src in source_files:
    found_sources[src] = os.path.exists(src)

# Helper to check for environment variables
found_env = {}
for var in env_vars:
    found_env[var] = os.environ.get(var)

# Healthcheck summary
missing = {
    "executables": [exe for exe, path in found_executables.items() if not path],
    "libraries": [lib for lib, ok in found_libraries.items() if not ok],
    "source_files": [src for src, ok in found_sources.items() if not ok],
    "env_vars": [var for var, val in found_env.items() if not val]
}

health = "PASS" if not any(missing.values()) else "FAIL"

report = {
    "executables": found_executables,
    "libraries": found_libraries,
    "source_files": found_sources,
    "env_vars": found_env,
    "environment_info": environment_info,
    "missing": missing,
    "health": health
}

with open("android_build_deps_report.json", "w") as f:
    json.dump(report, f, indent=2)

print("Android Build Dependency Healthcheck: {}".format(health))
if health == "FAIL":
    print("Missing dependencies:")
    for k, v in missing.items():
        if v:
            print(f"  {k}: {v}")
else:
    print("All dependencies found.")

# Mapping from executable to scoop package (where possible)
scoop_map = {
    "llvm-ar": "llvm",
    "keytool": "openjdk",
    "cp": "coreutils",
    "mkdir": "coreutils",
    "rm": "coreutils"
}

# Android SDK/NDK tools (manual or SDK manager)
android_tools = [
    "aarch64-linux-android28-clang",
    "aapt2",
    "aapt",
    "zipalign",
    "apksigner"
]

# Try to install missing scoop dependencies
def try_scoop_install(pkg):
    print(f"Attempting: scoop install {pkg}")
    try:
        subprocess.run(["scoop", "install", pkg], check=True)
    except Exception as e:
        print(f"  [!] Failed to install {pkg} via scoop: {e}")

# Install missing scoop dependencies
def install_missing_scoop_deps(missing_execs):
    for exe in missing_execs:
        if exe in scoop_map:
            try_scoop_install(scoop_map[exe])

# Print manual instructions for Android SDK/NDK tools
def print_android_manuals(missing_execs):
    android_sdk_cmd = (
        'scoop install android-clt\n'
        '& "$env:ANDROID_HOME\\cmdline-tools\\latest\\bin\\sdkmanager.bat" "platform-tools"\n'
        '& "$env:ANDROID_HOME\\cmdline-tools\\latest\\bin\\sdkmanager.bat" "build-tools;28.0.3"\n'
        '& "$env:ANDROID_HOME\\cmdline-tools\\latest\\bin\\sdkmanager.bat" "platforms;android-28"\n'
        '& "$env:ANDROID_HOME\\cmdline-tools\\latest\\bin\\sdkmanager.bat" "ndk;25.2.9519653"\n'
    )
    # If any Android tool is missing, print the instructions
    if any(tool in missing_execs for tool in android_tools):
        print("[MANUAL] One or more Android SDK/NDK tools are missing.")
        print("  To install all required Android SDK/NDK tools, run the following commands in PowerShell:")
        print(android_sdk_cmd)
        print("  (Adjust the NDK version if needed for your project.)")

# --- Android SDK/NDK environment variable auto-detection and PATH update ---
def detect_and_set_android_env():
    updated = False
    env = os.environ
    # Detect ANDROID_HOME
    android_home = env.get("ANDROID_HOME")
    if android_home and os.path.isdir(android_home):
        # Detect ANDROID_SDK_BUILD_TOOLS_VERSION
        build_tools_dir = os.path.join(android_home, "build-tools")
        if os.path.isdir(build_tools_dir):
            versions = [d for d in os.listdir(build_tools_dir) if os.path.isdir(os.path.join(build_tools_dir, d))]
            if versions:
                versions.sort(reverse=True)  # Prefer latest
                build_tools_version = versions[0]
                if not env.get("ANDROID_SDK_BUILD_TOOLS_VERSION"):
                    os.environ["ANDROID_SDK_BUILD_TOOLS_VERSION"] = build_tools_version
                    print(f"[AUTO] Set ANDROID_SDK_BUILD_TOOLS_VERSION={build_tools_version}")
                    updated = True
                # Add all build-tools/* dirs to PATH for tool discovery
                for v in versions:
                    tool_path = os.path.join(build_tools_dir, v)
                    if tool_path not in os.environ["PATH"]:
                        os.environ["PATH"] = tool_path + os.pathsep + os.environ["PATH"]
                        print(f"[AUTO] Added {tool_path} to PATH")
                        updated = True
        # Detect NDK
        ndk_dir = os.path.join(android_home, "ndk")
        if os.path.isdir(ndk_dir):
            ndk_versions = [d for d in os.listdir(ndk_dir) if os.path.isdir(os.path.join(ndk_dir, d))]
            if ndk_versions:
                ndk_versions.sort(reverse=True)
                ndk_version = ndk_versions[0]
                ndk_path = os.path.join(ndk_dir, ndk_version)
                if not env.get("ANDROID_NDK_VERSION"):
                    os.environ["ANDROID_NDK_VERSION"] = ndk_version
                    print(f"[AUTO] Set ANDROID_NDK_VERSION={ndk_version}")
                    updated = True
                if not env.get("ANDROID_NDK"):
                    os.environ["ANDROID_NDK"] = ndk_path
                    print(f"[AUTO] Set ANDROID_NDK={ndk_path}")
                    updated = True
    return updated

if __name__ == "__main__":
    print("\n=== Android Environment Auto-Detection ===")
    env_changed = detect_and_set_android_env()
    if env_changed:
        print("[INFO] Environment variables and PATH updated for this process.")
        print("[INFO] To persist these changes, set them in your shell profile or system environment.")
    print("\n=== Dependency Auto-Install ===")
    install_missing_scoop_deps(missing["executables"])
    print_android_manuals(missing["executables"])
    print("\nRe-run this script to re-check after installing dependencies.")
