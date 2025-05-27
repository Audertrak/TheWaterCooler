import os
import shutil
import subprocess
from pathlib import Path

# --- CONFIG ---
PROJECT_NAME = "enjenir"
BUILD_DIR = Path("build")
ANDROID_BUILD_DIR = BUILD_DIR / "android"
ANDROID_NATIVE_LIB_DIR = ANDROID_BUILD_DIR / "lib" / "arm64-v8a"
ANDROID_EXE_NAME = f"lib{PROJECT_NAME}.so"
ANDROID_EXE = ANDROID_NATIVE_LIB_DIR / ANDROID_EXE_NAME
APK_NAME = BUILD_DIR / f"{PROJECT_NAME}.apk"
ANDROID_ASSETS_DIR = ANDROID_BUILD_DIR / "assets"
ANDROID_MANIFEST = Path("AndroidManifest.xml")
ANDROID_RES_DIR = Path("res")
ANDROID_SDK_BUILD_TOOLS_VERSION = "34.0.0"
ANDROID_HOME = os.environ.get("ANDROID_HOME") or str(Path.home() / "scoop/apps/android-clt/current")
JAVA_HOME = os.environ.get("JAVA_HOME") or str(Path.home() / "scoop/apps/openjdk/current")
API_LEVEL = 28

# --- TOOLS ---
def tool_path(tool):
    return str(Path(ANDROID_HOME) / f"build-tools/{ANDROID_SDK_BUILD_TOOLS_VERSION}" / tool)

def run(cmd, check=True, env=None):
    print(f"[RUN] {cmd}")
    result = subprocess.run(cmd, shell=True, check=check, env=env)
    return result

# --- BUILD STEPS ---
def ensure_dirs():
    ANDROID_BUILD_DIR.mkdir(parents=True, exist_ok=True)
    (ANDROID_BUILD_DIR / "obj").mkdir(parents=True, exist_ok=True)
    ANDROID_NATIVE_LIB_DIR.mkdir(parents=True, exist_ok=True)
    ANDROID_ASSETS_DIR.mkdir(parents=True, exist_ok=True)

def copy_assets():
    if ANDROID_ASSETS_DIR.exists():
        shutil.rmtree(ANDROID_ASSETS_DIR)
    shutil.copytree("assets", ANDROID_ASSETS_DIR)
    print(f"[INFO] Copied assets to {ANDROID_ASSETS_DIR}")

def build_android_so():
    # This assumes the .so is already built by your toolchain (see Makefile for details)
    if not ANDROID_EXE.exists():
        print(f"[ERROR] {ANDROID_EXE} not found. Please build the .so first.")
        exit(1)
    print(f"[INFO] Found {ANDROID_EXE}")

def package_apk():
    aapt = tool_path("aapt.exe")
    zipalign = tool_path("zipalign.exe")
    apksigner = tool_path("apksigner.bat")
    android_jar = str(Path(ANDROID_HOME) / f"platforms/android-{API_LEVEL}/android.jar")
    # Staging
    staging_lib_dir = ANDROID_BUILD_DIR / "apk_staging/lib/arm64-v8a"
    staging_lib_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ANDROID_EXE, staging_lib_dir / ANDROID_EXE_NAME)
    res_staging = ANDROID_BUILD_DIR / "apk_staging/res"
    if ANDROID_RES_DIR.exists():
        shutil.copytree(ANDROID_RES_DIR, res_staging, dirs_exist_ok=True)
    # Remove res_staging if it exists but is empty or only contains mipmap with PNGs
    if res_staging.exists():
        # Remove all files in res_staging/mipmap
        mipmap_dir = res_staging / "mipmap"
        if mipmap_dir.exists():
            for f in mipmap_dir.glob("*"):
                f.unlink()
            mipmap_dir.rmdir()
        # Remove res_staging if now empty
        if not any(res_staging.iterdir()):
            res_staging.rmdir()
    # Ensure valid PNGs are present in staging
    from shutil import copy2
    for fname in ["ic_launcher.png", "ic_launcher_round.png"]:
        src = Path("res/mipmap") / fname
        dst = res_staging / "mipmap" / fname
        dst.parent.mkdir(parents=True, exist_ok=True)
        if src.exists():
            copy2(src, dst)
    shutil.copy2(ANDROID_MANIFEST, ANDROID_BUILD_DIR / "apk_staging/AndroidManifest.xml")
    if ANDROID_ASSETS_DIR.exists():
        assets_staging = ANDROID_BUILD_DIR / "apk_staging/assets"
        assets_staging.mkdir(parents=True, exist_ok=True)
        for item in ANDROID_ASSETS_DIR.iterdir():
            if item.is_dir():
                shutil.copytree(item, assets_staging / item.name, dirs_exist_ok=True)
            else:
                shutil.copy2(item, assets_staging / item.name)
    # Build aapt command
    aapt_cmd = f'"{aapt}" package -f -M "{ANDROID_BUILD_DIR}/apk_staging/AndroidManifest.xml"'
    if res_staging.exists() and any(res_staging.iterdir()):
        aapt_cmd += f' -S "{res_staging}"'
    aapt_cmd += f' -A "{ANDROID_BUILD_DIR}/apk_staging/assets" -I "{android_jar}" -F "{BUILD_DIR}/{PROJECT_NAME}.unaligned.apk" "{ANDROID_BUILD_DIR}/apk_staging/lib"'
    run(aapt_cmd)
    # Align APK
    run(f'"{zipalign}" -f -p 4 "{BUILD_DIR}/{PROJECT_NAME}.unaligned.apk" "{APK_NAME}"')
    # Sign APK
    run(f'"{apksigner}" sign --ks debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --key-pass pass:android --out "{APK_NAME}" "{APK_NAME}"')
    print(f"[SUCCESS] APK built at {APK_NAME}")

def ensure_minimal_res():
    # Create minimal res/values/strings.xml and res/mipmap/ic_launcher.png
    res_dir = Path("res")
    values_dir = res_dir / "values"
    mipmap_dir = res_dir / "mipmap"
    values_dir.mkdir(parents=True, exist_ok=True)
    mipmap_dir.mkdir(parents=True, exist_ok=True)
    # Minimal strings.xml
    strings_xml = values_dir / "strings.xml"
    if not strings_xml.exists():
        strings_xml.write_text('<resources>\n    <string name="app_name">enjenir</string>\n</resources>\n')
    # Minimal ic_launcher.png (empty file as placeholder)
    ic_launcher = mipmap_dir / "ic_launcher.png"
    ic_launcher_round = mipmap_dir / "ic_launcher_round.png"
    # Minimal valid 1x1 transparent PNG
    minimal_png = (
        b'\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4\x89'
        b'\x00\x00\x00\x0cIDAT\x08\x99c``\x00\x00\x00\x02\x00\x01\xe2!\xbc3\x00\x00\x00\x00IEND\xaeB`\x82'
    )
    if not ic_launcher.exists():
        ic_launcher.write_bytes(minimal_png)
    if not ic_launcher_round.exists():
        ic_launcher_round.write_bytes(minimal_png)

def patch_manifest_for_minimal():
    # Replace icon/label/roundIcon with defaults in the manifest
    manifest_path = ANDROID_MANIFEST
    if not manifest_path.exists():
        return
    text = manifest_path.read_text(encoding="utf-8")
    import re
    # Replace icon
    text = re.sub(r'android:icon="[^"]+"', 'android:icon="@android:drawable/sym_def_app_icon"', text)
    # Remove roundIcon
    text = re.sub(r'android:roundIcon="[^"]+"', '', text)
    # Replace label
    text = re.sub(r'android:label="[^"]+"', 'android:label="enjenir"', text)
    manifest_path.write_text(text, encoding="utf-8")

def main():
    ensure_dirs()
    patch_manifest_for_minimal()
    copy_assets()
    build_android_so()
    package_apk()

if __name__ == "__main__":
    main()
