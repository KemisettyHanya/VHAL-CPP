# CAN-to-AOSP VHAL Visual Mapping Studio (VS Code Extension)

A visual Blueprint-style CAN-to-Android Automotive OS (AOSP) Vehicle Hardware Abstraction Layer (VHAL) mapping studio integrated directly into Visual Studio Code.

---

## Features

- **Integrated Visual Canvas in VS Code:**
  - Launch with `Ctrl+Alt+V` (or `Cmd+Alt+V` on macOS), from the Command Palette (`CAN-to-VHAL: Open Visual Mapping Studio`), or via the `$(circuit-board) CAN-VHAL Studio` status bar button.
- **CAN DBC Signal Palette (Left Panel):**
  - Searchable tree view of all messages and signals in your CAN Database (`.dbc`).
  - Bit layout display (`b0:16`), Little Endian (Intel) vs Big Endian (Motorola) tags, units (`km/h`, `rpm`, `celsius`), and physical scaling factors.
  - One-click `+ Add` to canvas or `+ All` for entire messages.
- **Blueprint Interactive Node Editor (Center Canvas):**
  - High-performance visual node canvas with dynamic cubic bezier curve connectors.
  - Interactive pins: drag & drop from CAN signal output sockets to AOSP input sockets.
  - Transformation Modal: Configure unit conversions (`km/h -> m/s`, `Celsius -> Kelvin`, `RPM -> rad/s`), custom math formulas, clamping, and `areaId`.
  - One-click `🔗 Connect Selected` and `⚡ Auto-Connect Matching` tools.
  - `📐 Auto-Layout` and `🔍 Zoom Fit`.
- **AOSP VehicleProperty Catalog (Right Panel):**
  - Searchable AIDL vehicle properties (`PERF_VEHICLE_SPEED`, `ENGINE_RPM`, `GEAR_SELECTION`, `FUEL_LEVEL`, etc.).
  - Categorized into Vehicle Dynamics, Powertrain, HVAC, Body & Cabin, and Lighting & Controls.
- **Live AIDL C++ Synthesizer (Bottom Panel):**
  - Live preview of modern, MISRA-compliant `onCanFrameReceived(const struct can_frame& frame)`.
  - Generates byte-aligned masks, shifts, endianness conversions, formulas, and `VehiclePropValue` packaging.
  - **Insert Directly into Active Editor**: Place generated C++ code directly into your currently open C++ VHAL source file with one click!
  - **Save to File**: Save synthesized code into your project workspace.
- **Legacy C++ Reconstitution:**
  - Import legacy C++ files to automatically reconstruct canvas nodes and connections!

---

## Getting Started

1. Open any Android Automotive OS (AOSP) or VHAL project folder in VS Code.
2. Click **`CAN-VHAL Studio`** in the status bar at the bottom, or press **`Ctrl+Alt+V`**.
3. Select your `.dbc` file from the dropdown or click **`Open...`**.
4. Add CAN signals from the left panel and target properties from the right panel.
5. Connect them on the canvas and click **`⚡ Generate C++ Code`**!
6. Click **`💾 Insert into Active File`** to paste the generated code directly into your VHAL implementation.

---

## Extension Commands

| Command | Title |
| :--- | :--- |
| `canVhalStudio.openStudio` | Open Visual Mapping Studio (`Ctrl+Alt+V`) |
| `canVhalStudio.importDbc` | Import CAN DBC File... |
| `canVhalStudio.reconstructFromCpp` | Reconstruct Visual Connections from Legacy C++... |
| `canVhalStudio.synthesizeCpp` | Synthesize AIDL C++ Code into Active File |

