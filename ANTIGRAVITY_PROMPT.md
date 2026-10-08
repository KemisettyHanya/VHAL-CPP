# Antigravity Task Directive: Native C++ CAN-to-AOSP VHAL Mapping Studio (Dear ImGui)

Execute the full plan autonomously in full-auto mode. Do not pause to ask for confirmation for terminal commands, compilation steps, or dependency fetching. Continue execution until all acceptance criteria are met.

### Immediate Objectives
1. Read `SPEC_REQUIREMENTS.md` and `IMPLEMENTATION_PLAN.md`.
2. Configure a standalone C++20 desktop application using **CMake**:
   - Windowing & Rendering: **GLFW3 + OpenGL3** (or platform standard).
   - GUI: **Dear ImGui** (docking branch or standard 1.89+).
   - Node Editor Canvas: **`imgui-node-editor`** (by thedmd).
   - File Dialogs: Native file dialog (or lightweight tinyfiledialogs / ImGui file picker).
3. Implement the core engines directly in C++:
   - **DBC Parser Engine:** Reads CAN DBC files into internal data models.
   - **Reverse Engineering Parser:** Reads legacy C++ VHAL code (`onCanFrameReceived`), extracts CAN IDs, bit unpacking shifts/masks, math expressions, and AIDL properties, and automatically reconstitutes visual nodes and links in the node editor canvas.
   - **AOSP Code Synthesizer:** Converts active node links into production-ready C++ code targeting AIDL `IVehicle`.
4. Implement the 3-panel Dear ImGui UI:
   - Left Panel: Searchable CAN Signal Palette (tree view by CAN ID).
   - Center Panel: `imgui-node-editor` canvas with dynamic vertical auto-layout, pin dragging, cubic bezier links, and link inspection popups.
   - Right Panel: Searchable AOSP VehicleProperty Catalog.
   - Bottom Dock / Modal: Syntax-highlighted C++ code preview with "Copy" and "Save to File" buttons.
5. Compile the project via CMake and verify execution with sample DBC files and legacy C++ source code.

Begin execution according to `IMPLEMENTATION_PLAN.md`.