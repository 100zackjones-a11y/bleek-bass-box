# BLEEK BASS BOX — Build & Ableton Live 12 Installation Guide

## Prerequisites
- **CMake 3.22+**
- **C++20 Compiler**: Xcode 15+ (macOS) or Visual Studio 2022 (Windows)
- **Internet connection** on first configure (CMake automatically fetches JUCE 8.0.4 via `FetchContent`), or place JUCE 7/8 in `./JUCE`.

---

## macOS Build Instructions (Universal Apple Silicon + Intel -> VST3 & AU)

1. Open Terminal in your `BleekBassBox` project root directory:
```bash
cmake -B build -G "Xcode" \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
```

2. Build the Release targets (JUCE automatically copies `.vst3` and `.component` into your user plug-in folders via `COPY_PLUGIN_AFTER_BUILD TRUE`):
```bash
cmake --build build --config Release
```

3. Verify the installed plugin bundles:
```bash
ls -la ~/Library/Audio/Plug-Ins/VST3/BLEEK\ BASS\ BOX.vst3
ls -la ~/Library/Audio/Plug-Ins/Components/BLEEK\ BASS\ BOX.component
```

4. Open **Ableton Live 12**, go to **Settings -> Plug-Ins**, and click **Rescan** (if needed). **BLEEK BASS BOX** will appear under both **VST3** and **Audio Units -> BLEEK STATE AUDIO**.

---

## Windows Build Instructions (Visual Studio 2022 -> VST3)

1. Open **x64 Native Tools Command Prompt for VS 2022** or PowerShell in your `BleekBassBox` project root:
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
```

2. Build the Release target:
```powershell
cmake --build build --config Release
```

3. If your user account lacks write permissions to `C:\Program Files\Common Files\VST3`, run the copy step from an elevated Administrator terminal:
```powershell
Copy-Item -Recurse -Force "build\BleekBassBox_artefacts\Release\VST3\BLEEK BASS BOX.vst3" "C:\Program Files\Common Files\VST3\"
```

4. Launch **Ableton Live 12** -> **Plug-Ins -> BLEEK STATE AUDIO -> BLEEK BASS BOX**.
