# Bloody Roar Extreme - XBE Recomp

An unofficial native PC port of the original Xbox version of **Bloody Roar Extreme** (*Bloody Roar: Primal Fury*), with enhanced graphics options, Vulkan backend, high-resolution rendering, and controller support.

This port was created using the [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp) toolchain.

> [!NOTE]
> This project does not contain any copyrighted game assets or proprietary code. You must provide the files or disc image from your own legally acquired copy of the game to install or build.

---

## Game & ISO Requirements

To install or compile this port, you must provide your own copy from a legally owned Xbox disc:

- **Game**: *Bloody Roar Extreme* (*Bloody Roar: Primal Fury*)
- **Region**: **USA (NTSC-U)**
- **Title ID**: `48550001`
- **Serial**: `HU-001`
- **Supported Formats**: 1:1 Redump disc image (`.iso`), trimmed (`.xiso`), or extracted game directory.
- **`default.xbe` SHA-1**: `e58541178170e32ba71babce2c9ccf905bdb4db4`

> [!IMPORTANT]
> This port was created specifically from the **USA (NTSC-U)** release. Other regions (PAL / NTSC-J) have different code layouts and will not boot. Verify that your executable's SHA-1 matches `e58541178170e32ba71babce2c9ccf905bdb4db4`.

---

## Installation

1. Download the latest release from the [Releases](https://github.com/bromurok/BloodyRoarExtremeRecomp/releases) page.
2. Extract the archive into a folder of your choice.
3. Place your USA game files in the `assets` folder. You can use:
   - The game's disc image (`.iso`, supporting both XISO and 1:1 Redump disc images).
   - Or the extracted game files containing `default.xbe`.
4. Run `BloodyRoarExtreme.exe`.
5. Press **F1** during gameplay to open the in-game Configuration Menu (Display mode, Resolution, Render Scale).

---

## Features

### Display & Graphics
- **Native Vulkan Backend**: High-performance Vulkan renderer with triple buffering for smooth frame pacing.
- **Resolution**: Supports all monitor resolutions, Fullscreen, Borderless windowed, and Windowed modes.
- **Render Scale**: Internal resolution multiplier (Native x1, x2, x4, x6, x8) for crisp 3D rendering.
- **Locked 60 FPS & V-Sync**: Fluid 60 FPS gameplay with vertical synchronization.
- **Widescreen & 4:3**: Automatically adapts to widescreen displays while preserving aspect ratio.

### Audio
- **Enhanced Xbox ADPCM Streaming**: Low-latency BGM streaming with optimized multi-slot block caching.
- **Hermite Cubic Resampling**: 4-point 3rd-order Hermite interpolation for clear high-frequency audio.
- **Dynamic AGC Peak Limiter**: Smooth transparent peak limiter preventing distortion and audio saturation during intense special moves and Beast Drives.

### Input & Multiplayer
- **2-Player Local Versus Mode**: Full support for 2 simultaneous players. Play head-to-head in local Versus mode (Player 1 on Keyboard or Controller 1, and Player 2 on a connected Controller).
- **Native Controller Support**: Full plug-and-play support for Xbox, DualShock/DualSense (PlayStation), and Nintendo Switch controllers via SDL3 with vibration / rumble support.
- **Keyboard Controls**: Fully playable on keyboard with two built-in layouts (Arcade/WASD style and Classic/Arrow style):

| Xbox Button | In-Game Action | Layout 1 (Arcade) | Layout 2 (Classic) |
| :--- | :--- | :--- | :--- |
| **D-Pad / Stick** | Movement | `W` / `A` / `S` / `D` | `Arrow Keys` (Up/Down/Left/Right) |
| **A** | Punch / Confirm | `J` | `Z` |
| **B** | Kick / Cancel | `K` | `X` |
| **X** | Beast Transform | `U` | `C` |
| **Y** | Special Attack | `I` | `V` |
| **Start** | Start / Pause | `Enter` | `Spacebar` |
| **Back** | Back | `Escape` | `Backspace` |
| **Black (RB)** | Guard / Side step | `E` | `2` |
| **White (LB)** | Action | `Q` | `1` |
| **LT** | Action | `Left Shift` | `Left Shift` |
| **RT** | Action | `Ctrl` | `Right Shift` |

> [!TIP]
> **Customizing Controls:**
> - **In-game button actions** (Punch, Kick, Beast, Guard, etc.) can be reassigned natively inside the game's menu under **Options → Controller**.
> - **Physical keyboard keys** can be modified directly in the source code inside [`src/common/game_info.cpp`](src/common/game_info.cpp#L111-L134).

---

## System Requirements

### Minimum
- **OS**: Windows 10 / 11 64-bit
- **CPU**: x86-64 CPU with SSE4.2 and POPCNT (Intel Nehalem / AMD Bulldozer or newer)
- **GPU**: Graphics card with Vulkan 1.2+ support (AMD GCN+, NVIDIA Kepler+, Intel Skylake+)
- **RAM**: 4 GB
- **Storage**: ~2 GB for game files and saves

---

## Building from Source

### 1. Clone the Repository
Clone this repository with Git:
```cmd
git clone https://github.com/bromurok/BloodyRoarExtremeRecomp.git
cd BloodyRoarExtremeRecomp
```

### 2. Add Required Game Files
Place your legal USA release `default.xbe` (SHA-1: `e58541178170e32ba71babce2c9ccf905bdb4db4`) inside the `assets/` folder.

### 3. Install Dependencies
Install [Visual Studio Community 2022](https://visualstudio.microsoft.com/downloads/) with:
- **Desktop development with C++**
- **C++ Clang tools for Windows**
- **C++ CMake tools for Windows**

### 4. Build
Run `build.bat` in the root folder:
```cmd
build.bat
```
The compiled executable will be placed in `out\build\release\BloodyRoarExtreme.exe`.

---

## Credits & Third Party
- [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp) by TGP482 (MIT License).
- [Dear ImGui](https://github.com/ocornut/imgui) by Omar Cornut (MIT License).
- [SDL](https://github.com/libsdl-org/SDL) by Sam Lantinga (zlib License).
- [toml++](https://github.com/marzer/tomlplusplus) by Mark Gillard (MIT License).
- [Zydis](https://github.com/zyantific/zydis) (MIT License).
- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) by hedge-dev.
