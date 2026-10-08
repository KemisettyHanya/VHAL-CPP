# Implementation Plan: Native C++ CAN-to-AOSP VHAL Mapping Studio

## Phase 1: CMake Setup & Dependency Management
- [x] Create repository layout:
```
can_vhal_studio/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp
    ├── core/
    │   ├── AutomotiveTypes.hpp
    │   ├── DbcParser.hpp / DbcParser.cpp
    │   ├── CppReverseParser.hpp / CppReverseParser.cpp
    │   ├── CppSynthesizer.hpp / CppSynthesizer.cpp
    │   └── VhalCatalog.hpp / VhalCatalog.cpp
    └── ui/
        ├── AppWindow.hpp / AppWindow.cpp
        ├── CanvasView.hpp / CanvasView.cpp
        ├── SignalPalette.hpp / SignalPalette.cpp
        └── PropertyPalette.hpp / PropertyPalette.cpp
```
- [x] Configure `CMakeLists.txt` using `FetchContent`:
  - `glfw` (Windowing)
  - `imgui` (v1.89+ docking or master) + backends (`imgui_impl_glfw.cpp`, `imgui_impl_opengl3.cpp`)
  - `imgui-node-editor` (https://github.com/thedmd/imgui-node-editor)
- [x] Verify clean build producing a basic running ImGui + Node Editor window.

## Phase 2: Automotive Core Data Structures & Parsers
- [x] Implement `src/core/AutomotiveTypes.hpp`:
  - Structures: `CanSignal`, `CanMessage`, `VhalProperty`, `NodeLink`, `TransformConfig`.
- [x] Implement `src/core/VhalCatalog.cpp`:
  - Hardcoded catalog of standard Android 13/14+ AIDL properties (`PERF_VEHICLE_SPEED`, `ENGINE_RPM`, `GEAR_SELECTION`, `FUEL_LEVEL`, etc.).
- [x] Implement `src/core/DbcParser.cpp`:
  - State-machine and regex parser to extract `BO_` and `SG_` lines.
  - Endianness calculation (Intel vs Motorola), bit offsets, min/max, scale, offset, and units.
  - Value descriptions (`VAL_`) and comments (`CM_`).
- [x] Implement `src/core/CppReverseParser.cpp`:
  - Parse existing C++ text files.
  - Regex matcher for `switch (frame.can_id)` blocks, `frame.data[...]` bit manipulations, arithmetic factors, and `VehicleProperty::...` bindings.
  - Reconstitute data into `NodeLink` vectors to feed directly into the canvas.

## Phase 3: Visual Canvas with `imgui-node-editor`
- [x] Implement `src/ui/CanvasView.cpp`:
  - Initialize `ax::NodeEditor::EditorContext`.
  - Draw custom CAN nodes (Left side, showing CAN ID, signal name, unit, and output pin).
  - Draw custom VHAL nodes (Right side, showing Property ID, data type, and input pin).
  - Handle link creation: Query `ax::NodeEditor::BeginCreate()`, `QueryNewLink()`, and `AcceptNewItem()`.
  - Handle link deletion: Query `ax::NodeEditor::BeginDelete()`.
- [x] Implement Auto-Layout & Vertical Dynamic Growth:
  - Provide an "Auto-Arrange Vertically" function placing connected pairs sequentially downward ($Y_n = Y_0 + n \times 140\text{ px}$).
  - Ensure viewport zoom and pan handle vertical extents for 150+ node pairs.
- [x] Add Link Configuration Popup:
  - Double-clicking a link opens a modal to adjust unit conversions (e.g., `/ 3.6f` for speed, presets, custom expressions) and `areaId`.

## Phase 4: Left & Right Palettes and Synthesis UI
- [x] Implement `src/ui/SignalPalette.cpp`:
  - Searchable tree view of parsed DBC signals. Double-clicking or dragging an item spawns a node on the canvas.
- [x] Implement `src/ui/PropertyPalette.cpp`:
  - Searchable list of AOSP properties with category filters (Vehicle Dynamics, Powertrain, HVAC, Body).
- [x] Implement `src/core/CppSynthesizer.cpp`:
  - Iterate over all active `NodeLink` connections grouped by `can_id`.
  - Generate standard AOSP C++ AIDL code targeting `onCanFrameReceived`.
- [x] Implement Bottom Code Preview Dock:
  - Render generated C++ code inside a scrollable `ImGui::InputTextMultiline` with "Copy to Clipboard" and "Save to File" buttons.

## Phase 5: Verification & End-to-End Testing
- [x] Load sample DBC file containing 10+ signals.
- [x] Auto-generate links and click "Synthesize C++" to confirm output matches AOSP AIDL specifications.
- [x] Test the reverse engineering engine: Load a legacy C++ file containing `onCanFrameReceived` and verify that nodes and links automatically populate on the canvas.
- [x] Benchmark performance with 150+ CAN signals and 150+ VHAL properties to confirm steady 60 FPS viewport rendering.