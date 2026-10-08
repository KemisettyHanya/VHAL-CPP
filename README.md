# Native C++ CAN-to-AOSP VHAL Visual Mapping Studio (Dear ImGui)

A standalone, hardware-accelerated desktop mapping studio written in modern C++ (C++20). The application has zero external server or browser dependencies and operates directly in-memory using **GLFW3 + OpenGL3**, **Dear ImGui** (docking), and **`imgui-node-editor`** (by thedmd).

---

## Architecture Overview

```
+-----------------------------------------------------------------------------------------------+
| Main Menu Bar: [File: Open DBC, Import C++, Save] [Tools: Auto-Layout, Benchmark] [Help]       |
+----------------------+------------------------------------------------+-----------------------+
| LEFT PANEL           | CENTER PANEL                                   | RIGHT PANEL           |
| (Source CAN Signals) | (imgui-node-editor Blueprint Canvas)           | (Target AOSP Props)   |
|                      |                                                |                       |
| Filter: [veh       ] |  +------------------+     +------------------+ | Filter: [speed     ]  |
|                      |  | 0x1A4 VehDynamics|     | PERF_VEH_SPEED   | |                       |
| v 0x1A4 VehDynamics  |  |  VehicleSpeed (O)----->(I) floatValues    | | > PERF_VEHICLE_SPEED  |
|   * VehicleSpeed     |  +------------------+     +------------------+ | > ENGINE_RPM          |
|   * YawRate          |                                                | > GEAR_SELECTION      |
|                      |  +------------------+     +------------------+ | > CURRENT_GEAR        |
| v 0x2B0 EngineStatus |  | 0x2B0 EngStatus  |     | ENGINE_RPM       | | > FUEL_LEVEL          |
|   * EngineRPM        |  |  EngineRPM    (O)----->(I) floatValues    | |                       |
|                      |  +------------------+     +------------------+ |                       |
|                      |  ... handles 150+ node pairs with 60 FPS ...   |                       |
+----------------------+------------------------------------------------+-----------------------+
| BOTTOM PANEL: Live Generated AOSP C++ Code Preview (onCanFrameReceived)                     |
+-----------------------------------------------------------------------------------------------+
```

---

## Features & Engines

### 1. Native CAN DBC Parser (`DbcParser.hpp / .cpp`)
- Parses Vector DBC format records:
  - `BO_ <ID> <MessageName>: <DLC> <Sender>`
  - `SG_ <Name> : <StartBit>|<Length>@<Endian><Sign> (<Scale>,<Offset>) [<Min>|<Max>] "<Unit>" <Receiver>`
  - Comments: `CM_ BO_` and `CM_ SG_`
  - Value description enums: `VAL_ <ID> <SignalName> ... ;`
- Handles bit extraction attributes:
  - Start byte, end byte, bit-shift within byte.
  - Endianness handling: Little Endian (`@1+` / Intel) and Big Endian (`@0+` / Motorola).
  - Data signedness: Two's-complement signed (`-`) vs unsigned (`+`).
  - Bit unpacking verification: `sig.decodeRaw(payload, len)` and `sig.decodePhysical(payload, len)`.

### 2. Reverse Engineering Engine (`CppReverseParser.hpp / .cpp`)
- Ingests legacy C++ VHAL files implementing `onCanFrameReceived(const struct can_frame& frame)` or adapter classes.
- Extracts `switch (frame.can_id)` blocks, `#define` / `constexpr` CAN ID constants, and conditional checks.
- Detects bit-shifting patterns (e.g. `(frame.data[0] | (frame.data[1] << 8))`, `(frame.data[0] >> 4) & 0x0F`).
- Extracts scale factors, offsets, unit conversions (`/ 3.6f`, `convertKmhToMps`), and clamping (`std::clamp`).
- Identifies matching target Android properties (`VehicleProperty::PERF_VEHICLE_SPEED`, `ENGINE_RPM`, etc.).
- Automatically reconstitutes canvas graph nodes and cubic bezier links.

### 3. AOSP C++ Code Synthesizer (`CppSynthesizer.hpp / .cpp`)
- Produces clean, MISRA-compliant modern C++ code targeting AIDL `IVehicle`:
  - Encloses frame handlers in `switch (frame.can_id)`.
  - Generates byte masks, shifts, and sign extensions matching signal endianness and bit length.
  - Generates physical scaling (`raw * scale + offset`) and formula conversions (`/ 3.6f`, etc.).
  - Packs values into `aidl::android::hardware::automotive::vehicle::VehiclePropValue`.
  - Emits callback dispatch: `mCallback(dispatchedProps);`.

### 4. Interactive Blueprint Node Editor (`CanvasView.hpp / .cpp`)
- Uses `imgui-node-editor` (by thedmd) with blueprint styling.
- CAN source nodes with slate-blue headers and output pins.
- VHAL target nodes with emerald-green headers and input pins.
- Pin dragging with cubic bezier links and validation.
- Double-clicking a link opens a modal transformation editor (unit presets, custom formula, `areaId`, clamping).
- **Auto-Layout Vertical Stack:**
  - CAN nodes aligned at $X = 100$.
  - VHAL nodes aligned at $X = 650$.
  - Node pairs stacked at $Y_n = Y_0 + n \times 140\text{ px}$.
- Benchmarked to sustain **300+ nodes (150 CAN + 150 VHAL + 150 links) at 60 FPS**.

---

## Directory Layout

```
can_vhal_studio/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── AutomotiveTypes.hpp
│   │   ├── VhalCatalog.hpp / VhalCatalog.cpp
│   │   ├── DbcParser.hpp / DbcParser.cpp
│   │   ├── CppReverseParser.hpp / CppReverseParser.cpp
│   │   └── CppSynthesizer.hpp / CppSynthesizer.cpp
│   └── ui/
│       ├── AppWindow.hpp / AppWindow.cpp
│       ├── CanvasView.hpp / CanvasView.cpp
│       ├── SignalPalette.hpp / SignalPalette.cpp
│       └── PropertyPalette.hpp / PropertyPalette.cpp
├── tests/
│   └── unit_tests.cpp
└── sample_data/
    ├── sample_vehicle_speed.dbc
    ├── complete_vehicle_powertrain.dbc
    ├── legacy_vhal_speed.cpp
    └── legacy_vehicle_vhal.cpp
```

---

## Building and Running

### Prerequisites
- Modern C++20 compiler (`g++` 13+ or `clang++` 16+)
- CMake 3.20+
- OpenGL & X11 / Wayland development libraries

### Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Windows Build (Visual Studio 2022 / MSVC / Ninja)
On Windows, open PowerShell or Developer Command Prompt for VS:
```powershell
# Configure with Visual Studio 2022 (x64)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build Release executable
cmake --build build --config Release

# Run GUI Studio
.\build\Release\can_vhal_studio.exe

# Run test suite
.\build\Release\can_vhal_tests.exe
```

Or using Ninja with MSVC:
```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\can_vhal_studio.exe
```

### Linux Build & Run
```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run GUI Studio
./build/can_vhal_studio
```

### CLI / Headless Execution & Verification Options
```bash
# Run unit and integration tests
./build/can_vhal_tests

# Run CTest suite
ctest --test-dir build --output-on-failure

# Verify DBC parsing, reverse engineering, and code synthesis
./build/can_vhal_studio --verify

# Run 300+ node benchmark stress test
./build/can_vhal_studio --benchmark

# Render N GUI frames (for headless automated testing)
./build/can_vhal_studio --frames 60
```

