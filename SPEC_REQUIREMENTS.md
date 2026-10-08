# Software Requirements Specification (SRS)
## Native C++ CAN-to-AOSP VHAL Visual Mapping Studio

### 1. Architecture Overview
A unified, hardware-accelerated desktop application written in modern C++ (C++20). The application has zero external server or browser dependencies and operates directly in-memory.

+-----------------------------------------------------------------------------------------------+
| Main Menu Bar: [File: Open DBC, Import C++, Save Workspace] [Tools: Auto-Layout] [Generate]   |
+----------------------+------------------------------------------------+-----------------------+
| LEFT PANEL           | CENTER PANEL                                   | RIGHT PANEL           |
| (Source CAN Signals) | (imgui-node-editor Blueprint Canvas)           | (Target AOSP Props)   |
|                      |                                                |                       |
| Filter: [veh       ] |  +------------------+     +------------------+ | Filter: [speed     ]  |
|                      |  | 0x1A4 VehDynamics|     | PERF_VEH_SPEED   | |                       |
| v 0x1A4 VehDynamics  |  |  VehicleSpeed (O)>(I) floatValues    | | > PERF_VEHICLE_SPEED  |
|   * VehicleSpeed     |  +------------------+     +------------------+ | > ENGINE_RPM          |
|   * YawRate          |                                                | > GEAR_SELECTION      |
|                      |  +------------------+     +------------------+ | > CURRENT_GEAR        |
| v 0x2B0 EngineStatus |  | 0x2B0 EngStatus  |     | ENGINE_RPM       | | > FUEL_LEVEL          |
|   * EngineRPM        |  |  EngineRPM    (O)>(I) floatValues    | |                       |
|                      |  +------------------+     +------------------+ |                       |
|                      |  ... handles 150+ node pairs with 60 FPS ...   |                       |
+----------------------+------------------------------------------------+-----------------------+
| BOTTOM PANEL: Live Generated AOSP C++ Code Preview (onCanFrameReceived)                     |
+-----------------------------------------------------------------------------------------------+

---

### 2. Functional Requirements

#### 2.1 Native CAN DBC Parser (`DbcParser.hpp/.cpp`)
* **FR-1.1:** Parse Vector DBC text records:
  - `BO_ <ID> <MessageName>: <DLC> <Sender>`
  - `SG_ <Name> : <StartBit>|<Length>@<Endian><Sign> (<Scale>,<Offset>) [<Min>|<Max>] "<Unit>" <Receiver>`
* **FR-1.2:** Calculate bit extraction attributes:
  - Start byte, end byte, bit-shift within byte.
  - Endianness handling: Little Endian (`@1+` / Intel) and Big Endian (`@0+` / Motorola).
  - Data signedness: Two's-complement signed (`-`) vs unsigned (`+`).

#### 2.2 Reverse Engineering Engine (`CppReverseParser.hpp/.cpp`)
* **FR-2.1 Source Code Ingestion:**
  - Ingest an existing AOSP C++ source file implementing `onCanFrameReceived(const struct can_frame& frame)`.
* **FR-2.2 Syntax & AST Reconstruction:**
  - Parse `switch (frame.can_id)` and `case 0x...:` blocks or conditional `if (frame.can_id == ...)` statements.
  - Detect bit-shifting patterns (e.g., `(frame.data[2] << 8) | frame.data[3]`).
  - Extract scale, offset, and arithmetic conversions (e.g., `* 0.01f`, `/ 3.6f`).
  - Identify matching target Android properties (e.g., `VehicleProperty::PERF_VEHICLE_SPEED`).
* **FR-2.3 Canvas Graph Auto-Population:**
  - Create source nodes for extracted CAN messages and signals.
  - Create target nodes for identified AOSP Vehicle Properties.
  - Instantiate links connecting output pins to input pins.
  - Retain extracted custom arithmetic expressions in link metadata.

#### 2.3 Visual Node Graph Canvas (`imgui-node-editor`)
* **FR-3.1 Canvas Execution:**
  - Utilize `imgui-node-editor` (by thedmd) inside an ImGui child window.
  - Nodes use blueprint styling: Distinct headers, clear labels, and visible input/output pins.
* **FR-3.2 Vertical Layout & Dynamic Growth:**
  - Support drag-and-drop or double-click to place nodes onto the canvas.
  - Implement an **Auto-Layout Vertical Stack** algorithm:
    - CAN nodes align along X = 100.
    - VHAL nodes align along X = 600.
    - Node pairs increment along Y ($Y_n = Y_0 + n \times \Delta Y$).
  - Canvas must sustain **at least 150 CAN nodes + 150 VHAL nodes + 150 cubic bezier links (300+ total nodes)** at 60 FPS without rendering stalls.
* **FR-3.3 Interactive Wiring:**
  - Drag from a CAN output pin to a VHAL input pin to create a link.
  - Validate pin compatibility (source must be CAN output; target must be VHAL input).
  - Right-click a link or node to delete it.
  - Double-click a link to open a modal editor for unit transformations (e.g., `km/h -> m/s`, custom scaling factor, target `areaId`).

#### 2.4 AOSP C++ Code Generator (`CppSynthesizer.hpp/.cpp`)
* **FR-4.1 Synthesis Output:**
  - Produce clean, MISRA-compliant C++ code matching AOSP AIDL VHAL conventions:
    - Enclose frame handlers in `switch (frame.can_id)`.
    - Apply byte masks, shifts, and sign extensions matching the signal's endianness and bit length.
    - Apply physical scaling (`raw * scale + offset`) and formula conversions.
    - Pack values into `aidl::android::hardware::automotive::vehicle::VehiclePropValue`.
    - Emit callback dispatch: `mCallback->onPropertyEvent({ propValue });`.

---

### 3. Non-Functional Requirements
* **NFR-1 Portability:** Compiles with modern CMake on Linux (GCC/Clang) and Windows (MSVC).
* **NFR-2 Zero Lag:** Native ImGui immediate-mode rendering must maintain 60 FPS during zooming, panning, and rapid vertical scrolling across 300+ elements.
* **NFR-3 Self-Contained:** Uses CMake `FetchContent` to retrieve `imgui`, `imgui-node-editor`, and `glfw` automatically with no manual library installation required.