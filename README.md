# DjusovEngine

[FluffyGit](http://83.143.112.6:3000/TinyTosha/DjusovEngine) | [GitHub](https://github.com/amethyst-bin/DjusovEngine) | [Codeberg](https://codeberg.org/TinyTosha/DjusovEngine)

[![Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Graphics](https://img.shields.io/badge/Graphics-OpenGL%203.3%2F4.5%20Core-green.svg)](https://www.opengl.org/)
[![Scripting](https://img.shields.io/badge/Scripting-Luau%200.6-blueviolet.svg)](https://luau.org/)
[![Audio](https://img.shields.io/badge/Audio-miniaudio%20Spatial%203D-orange.svg)](https://miniaud.io/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20(MinGW)-lightgrey.svg)](https://github.com/amethyst-bin/DjusovEngine)
[![License](https://img.shields.io/badge/License-MIT-brightgreen.svg)](LICENSE)

DjusovEngine is a specialized 3D game engine and studio suite written in modern C++20. Designed for constructing detailed urban districts, hosting 1v1 tactical engagements, and building narrative experiences with Luau scripting, real-time PBR graphics, and multiplayer networking.

---

## Core Systems & Architecture

### 1. PBR Rendering Pipeline
- **Cook-Torrance BRDF:** GGX microfacet distribution, Smith geometric attenuation, and Fresnel-Schlick approximation.
- **Dynamic Shadows:** PCF (Percentage-Closer Filtering) 5x5 directional sun shadow mapping.
- **HDR & ACES Tonemapping:** FP16 HDR rendering with multi-pass Gaussian bloom and Academy Color Encoding System (ACES) film curve tonemapping.
- **Real-Time Planar Reflections:** Exact mathematical planar mirror reflections with layer culling (renders full 3D humanoid player avatars in mirrors while omitting first-person viewmodel arms).
- **Realistic Glass Material:** Fresnel reflections, refraction (IOR 1.52), concentrated specular highlights, and edge absorption.
- **Peripheral Scope Blur:** Radial scope blur during ADS (Aim Down Sights).

### 2. First-Person ViewModel & Skeletal Skinning
- **Rigged Hands Loader:** Custom FBX parser based on `ufbx` loading rigged first-person arms (`assets/models/fps-hands.fbx`, 50 bones) with hardware GPU vertex skinning (`skinned.vert`).
- **Procedural ViewModel Animation:** Spring-damper mouse sway, velocity-based bobbing, ADS aperture alignment, and elastic kickback recoil.
- **Combat Mechanics:** Raycast ballistics, bullet spread, muzzle flash illumination, projectile tracers, and hitmarker feedback.

### 3. Audio System & Source Engine Library
- Powered by `miniaudio` with 3D spatial positioning and zero external runtime dependencies.
- Built-in library of Source-style sound effects:
  - `weapon_shoot`: Punchy rifle gunfire
  - `weapon_reload`: Magazine eject, slide, and lock
  - `weapon_empty`: Dry fire hammer click
  - `hitmarker`: High-frequency audio confirmation
  - `footstep`: Concrete surface footsteps
  - `lamp_hum`: Industrial electrical hum
  - `switch_click`: Mechanical toggle click

### 4. Luau Scripting Architecture
- Embedded Luau VM with sandboxed modular service imports via `require("@de/...")`:
  - `local World = require("@de/world")`
  - `local Players = require("@de/players")`
  - `local SaveManager = require("@de/savemanager")`
  - `local Lighting = require("@de/lighting")`
- Sound playback API support:
  ```luau
  local World = require("@de/world")
  World.SoundObject:Play()
  World.playSound("weapon_shoot")
  ```
- Support for `Script` (server authoritative), `LocalScript` (client predicted), and `ModuleScript` (shared libraries).

### 5. Level Studio & Editor Suite
- **Amoled Theme:** High-contrast deep black palette with auto-detected system accent color (GTK / Qt / DWM) and manual customization.
- **Pre-Docked Layout:** Godot / Roblox Studio layout configuration:
  - Top: Main Menu Bar with embedded Play/Stop, Grid Snap, and Build controls.
  - Left Top: Scene Hierarchy with entity tree.
  - Left Bottom: 1-Click Object Palette (Cube, Sphere, Cylinder, Plane, Ramp, Street Lamp, Mirror, Glass, Light, Sound).
  - Center: Interactive 3D Viewport with real-time preview, FreeCam, and click-to-select raycasting.
  - Right: Inspector with iconified component headers (`[Transform]`, `[Material]`, `[Collider]`, `[Light]`, `[Sound]`, `[Script]`).
  - Bottom: Integrated Luau Script Editor with syntax highlighting and line numbers.

### 6. Networking & Dedicated Server
- Non-blocking TCP binary packet protocol (`NetPacket.hpp`).
- Headless dedicated server mode (`--server <port>`).
- Client state interpolation, weapon fire replication, and in-game text chat.

---

## Directory Structure

```
DjusovEngine/
├── assets/
│   ├── fonts/          # JetBrains Mono UI font
│   ├── models/         # Rigged first-person hands FBX
│   ├── scripts/        # Default Luau scripts
│   ├── shaders/        # GLSL PBR, shadow, sky, post-process shaders
│   └── sounds/         # Source-style WAV sound effects
├── dist/               # Exported standalone distribution
│   ├── bin/            # Game runner binaries
│   ├── de/             # Maps, configs, scripts, saves
│   ├── Game            # Linux bootstrap launcher
│   └── Game.exe        # Windows MinGW launcher
├── src/
│   ├── audio/          # miniaudio wrapper and audio engine
│   ├── build/          # Standalone game packager
│   ├── core/           # Math, Time, Theme management
│   ├── network/        # Client and Server sockets
│   ├── player/         # Controller, ViewModel, Weapon, Health
│   ├── render/         # PBR pipeline, meshes, materials, shaders
│   ├── script/         # Luau runtime integration
│   ├── ui/             # EditorUI, GameHUD, ScriptEditorDock
│   ├── vendor/         # ImGui, TextEditor, ufbx, Luau, miniaudio
│   └── world/          # Entities, World graph, SaveManager
├── CMakeLists.txt
└── README.md
```

---

## Prerequisites & Dependencies

### Linux (Arch Linux / Fedora / Ubuntu / Debian)
Required packages:
- GCC 12+ or Clang 15+ (C++20 support)
- CMake 3.20+ and Ninja
- GLFW 3
- GLEW
- GLM
- nlohmann-json
- MinGW-w64 (optional, for compiling Windows launcher from Linux)

Install on Arch Linux:
```bash
sudo pacman -S base-devel cmake ninja glfw glew glm nlohmann-json mingw-w64-gcc
```

Install on Ubuntu / Debian:
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libglfw3-dev libglew-dev libglm-dev nlohmann-json3-dev mingw-w64
```

---

## Building from Source

```bash
# Clone the repository
git clone https://github.com/amethyst-bin/DjusovEngine.git
cd DjusovEngine

# Configure with CMake and Ninja
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Compile all targets
cmake --build build
```

The build produces three primary executables in `build/`:
1. `DjusovEngine`: Level Studio & Editor Suite
2. `djusov_runner`: Core Game Runner & Headless Dedicated Server
3. `Game`: Native Bootstrap Launcher

---

## Running the Application

### 1. Launch the Level Studio (Editor Mode)
```bash
./build/DjusovEngine
```
- Hold **Right Mouse Button + WASD** to fly the viewport camera.
- Hold **Left Shift** while flying to boost speed.
- **Left Click** on any object in the Viewport to select it in the Hierarchy and Inspector.
- Press **F** to center and focus the camera on the selected entity.
- Press **Delete** to remove the selected entity.
- Press **F5** or click **Play** in the top bar to test the map in Play Mode.

### 2. Launch the Standalone Game Launcher
```bash
./build/Game
# or from the release distribution:
./dist/Game
```
- Tactical native main menu screen.
- Configurable Mouse Sensitivity, Field of View (FOV), and Master Volume.
- Singleplayer campaign / sandbox level launcher with save slot management.
- Multiplayer matchmaking client and local dedicated server host.
- Direct shortcut to launch DjusovEngine Studio.

### 3. Run a Headless Dedicated Multiplayer Server
```bash
./build/djusov_runner --server 7777
```

### 4. Connect to a Server Directly
```bash
./build/djusov_runner --connect 127.0.0.1:7777 --name "PlayerName" --fov 85 --sens 1.2
```

---

## Controls Reference

### Editor Mode
| Input | Action |
| --- | --- |
| **RMB (Hold) + WASD** | Fly Viewport Camera |
| **Q / E** | Move Camera Down / Up |
| **Left Shift (Hold)** | Boost Camera Speed |
| **LMB** | Select Entity via Raycast |
| **F** | Focus Viewport Camera on Selection |
| **Delete** | Delete Selected Entity |
| **Ctrl + S** | Save Level to Disk |
| **F5** | Toggle Play / Edit Mode |

### Play Mode (FPS Combat)
| Input | Action |
| --- | --- |
| **WASD** | Walk / Move |
| **Left Shift (Hold)** | Sprint (Consumes Stamina) |
| **Space** | Jump |
| **LMB** | Fire Weapon |
| **RMB (Hold)** | Aim Down Sights (ADS + Scope Blur) |
| **R** | Reload Magazine |
| **T** | Open Multiplayer Text Chat |
| **Escape** | Pause Menu (Adjust Sensitivity, Volume, Resume, Quit) |

---

## Luau Scripting Examples

### Ambient Sound Trigger
```luau
local World = require("@de/world")

-- Play a global Source-style sound effect
World.playSound("weapon_shoot")

-- Access the global SoundObject API
World.SoundObject:Play()

-- Play a 3D positioned sound from an entity
local speaker = World.getObject("SoundSource")
if speaker then
    speaker:Play()
end
```

### Dynamic Day/Night Lighting Cycle
```luau
local Lighting = require("@de/lighting")

local time = 0
while true do
    time = time + 0.01
    local intensity = math.max(0.2, math.sin(time) * 3.0)
    Lighting.setSunIntensity(intensity)
    Lighting.setSunColor(1.0, 0.9, 0.7)
    task.wait(0.05)
end
```

---

## License

This project is licensed under the MIT License.
