import * as vscode from "vscode";

export class StudioWebview {
    public static getHtml(
        webview: vscode.Webview,
        extensionUri: vscode.Uri,
        initialDbcMessagesJson: string,
        initialVhalPropertiesJson: string,
        workspaceDbcFiles: string[] = []
    ): string {
        return `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CAN-to-AOSP VHAL Mapping Studio</title>
    <style>
        :root {
            --bg-color: #0f1318;
            --panel-bg: #14181f;
            --panel-border: #222a36;
            --text-color: #e6edf3;
            --text-dim: #7d8590;
            --accent-blue: #388bfd;
            --accent-green: #2ea043;
            --accent-green-hover: #3fb950;
            --accent-cyan: #39c5cf;
            --accent-gold: #d29922;
            --accent-red: #f85149;
            --can-header-bg: #1f3d59;
            --vhal-header-bg: #194d33;
            --node-bg: #161b22;
            --node-border: #30363d;
            --node-selected-border: #58a6ff;
            --font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            --mono-font: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        body {
            background-color: var(--bg-color);
            color: var(--text-color);
            font-family: var(--font-family);
            font-size: 13px;
            overflow: hidden;
            height: 100vh;
            display: flex;
            flex-direction: column;
            user-select: none;
        }

        /* Top Header Navigation */
        .top-navbar {
            background-color: var(--panel-bg);
            border-bottom: 1px solid var(--panel-border);
            padding: 8px 16px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            height: 48px;
            flex-shrink: 0;
        }

        .studio-title {
            display: flex;
            align-items: center;
            gap: 10px;
            font-weight: 600;
            font-size: 14px;
            color: #58a6ff;
        }

        .top-actions {
            display: flex;
            align-items: center;
            gap: 8px;
        }

        /* Studio Main Layout */
        .studio-main {
            display: flex;
            flex: 1;
            overflow: hidden;
            position: relative;
        }

        /* Side Panels */
        .side-panel {
            background-color: var(--panel-bg);
            border-right: 1px solid var(--panel-border);
            width: 360px;
            display: flex;
            flex-direction: column;
            flex-shrink: 0;
            z-index: 10;
        }

        .side-panel.right-panel {
            border-right: none;
            border-left: 1px solid var(--panel-border);
            width: 360px;
        }

        .panel-header {
            padding: 10px 14px;
            border-bottom: 1px solid var(--panel-border);
            display: flex;
            flex-direction: column;
            gap: 8px;
        }

        .panel-title-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .panel-title {
            font-weight: 600;
            font-size: 12px;
            letter-spacing: 0.5px;
            text-transform: uppercase;
        }

        .can-title { color: var(--accent-cyan); }
        .vhal-title { color: #56d364; }

        .search-input {
            width: 100%;
            background-color: #0d1117;
            border: 1px solid var(--node-border);
            color: var(--text-color);
            padding: 6px 10px;
            border-radius: 4px;
            font-size: 12px;
            outline: none;
        }
        .search-input:focus {
            border-color: var(--accent-blue);
        }

        .action-row {
            display: flex;
            gap: 6px;
        }

        /* Buttons */
        .btn {
            background-color: #21262d;
            border: 1px solid var(--node-border);
            color: var(--text-color);
            padding: 5px 10px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 12px;
            display: inline-flex;
            align-items: center;
            gap: 6px;
            white-space: nowrap;
            transition: all 0.15s ease;
        }

        .btn:hover {
            background-color: #30363d;
            border-color: #8b949e;
        }

        .btn-primary {
            background-color: #238636;
            border-color: rgba(240, 246, 252, 0.1);
            color: #ffffff;
            font-weight: 600;
        }
        .btn-primary:hover {
            background-color: #2ea043;
        }

        .btn-blue {
            background-color: #1f6feb;
            border-color: rgba(240, 246, 252, 0.1);
            color: #ffffff;
        }
        .btn-blue:hover {
            background-color: #388bfd;
        }

        .btn-danger {
            background-color: #b62324;
            border-color: rgba(240, 246, 252, 0.1);
            color: #ffffff;
        }
        .btn-danger:hover {
            background-color: #d1242f;
        }

        .btn-sm {
            padding: 2px 7px;
            font-size: 11px;
        }

        .panel-list {
            flex: 1;
            overflow-y: auto;
            padding: 10px;
        }

        /* CAN Tree Items */
        .msg-group {
            background-color: #161b22;
            border: 1px solid var(--node-border);
            border-radius: 6px;
            margin-bottom: 8px;
            overflow: hidden;
        }

        .msg-header {
            padding: 8px 10px;
            background-color: #1c2128;
            display: flex;
            align-items: center;
            justify-content: space-between;
            cursor: pointer;
            font-weight: 500;
            font-size: 12px;
        }
        .msg-header:hover {
            background-color: #242c38;
        }

        .msg-signals {
            padding: 4px 6px;
            display: flex;
            flex-direction: column;
            gap: 4px;
        }

        .sig-item {
            background-color: #0d1117;
            border: 1px solid #21262d;
            border-radius: 4px;
            padding: 6px 8px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            font-size: 12px;
        }
        .sig-item:hover {
            border-color: var(--accent-cyan);
        }

        /* Property Items */
        .prop-item {
            background-color: #161b22;
            border: 1px solid var(--node-border);
            border-radius: 6px;
            padding: 8px 10px;
            margin-bottom: 6px;
            display: flex;
            flex-direction: column;
            gap: 4px;
            cursor: pointer;
        }
        .prop-item:hover {
            border-color: #56d364;
            background-color: #1a222c;
        }

        .prop-top-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .prop-name {
            font-weight: 600;
            color: #7ee787;
        }

        .badge {
            font-size: 10px;
            padding: 2px 6px;
            border-radius: 10px;
            font-family: var(--mono-font);
        }

        .badge-intel { background-color: #1f3d59; color: #58a6ff; }
        .badge-motorola { background-color: #4d2d1f; color: #f0883e; }
        .badge-unit { background-color: #1c3328; color: #3fb950; }
        .badge-type { background-color: #3b2c15; color: #d29922; }

        /* Center Canvas & Toolbar */
        .canvas-container {
            flex: 1;
            position: relative;
            background-color: #0b0e14;
            overflow: hidden;
            display: flex;
            flex-direction: column;
        }

        .canvas-toolbar {
            background-color: rgba(20, 24, 31, 0.85);
            backdrop-filter: blur(8px);
            border-bottom: 1px solid var(--panel-border);
            padding: 8px 14px;
            display: flex;
            gap: 8px;
            align-items: center;
            z-index: 5;
        }

        .blueprint-viewport {
            flex: 1;
            position: relative;
            overflow: hidden;
            background-image: radial-gradient(circle, #1a2333 1px, transparent 1px);
            background-size: 24px 24px;
            cursor: grab;
        }
        .blueprint-viewport:active {
            cursor: grabbing;
        }

        /* SVG Link overlay */
        .links-layer {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none;
            overflow: visible;
        }

        .link-path {
            fill: none;
            stroke: #58a6ff;
            stroke-width: 3;
            pointer-events: stroke;
            cursor: pointer;
            transition: stroke 0.15s ease;
        }
        .link-path:hover {
            stroke: #ffdf5d;
            stroke-width: 4;
        }
        .link-path.transformed {
            stroke: #d29922;
        }

        /* Node Cards on Canvas */
        .node-card {
            position: absolute;
            width: 250px;
            background-color: var(--node-bg);
            border: 1px solid var(--node-border);
            border-radius: 8px;
            box-shadow: 0 8px 24px rgba(0, 0, 0, 0.45);
            user-select: none;
            cursor: move;
            z-index: 2;
            transition: box-shadow 0.15s ease, border-color 0.15s ease;
        }
        .node-card.selected {
            border-color: var(--node-selected-border);
            box-shadow: 0 0 0 2px rgba(88, 166, 255, 0.35), 0 8px 24px rgba(0, 0, 0, 0.5);
        }

        .node-header {
            padding: 8px 12px;
            border-top-left-radius: 7px;
            border-top-right-radius: 7px;
            font-weight: 600;
            font-size: 12px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .node-header.can {
            background-color: var(--can-header-bg);
            color: #79c0ff;
        }

        .node-header.vhal {
            background-color: var(--vhal-header-bg);
            color: #7ee787;
        }

        .node-body {
            padding: 10px 12px;
            display: flex;
            flex-direction: column;
            gap: 6px;
        }

        .pin-row {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-top: 6px;
            padding-top: 6px;
            border-top: 1px solid #21262d;
        }

        .pin-socket {
            width: 14px;
            height: 14px;
            border-radius: 50%;
            background-color: #21262d;
            border: 2px solid #58a6ff;
            cursor: pointer;
            transition: all 0.2s ease;
            position: relative;
        }
        .pin-socket:hover {
            transform: scale(1.3);
            background-color: #58a6ff;
        }
        .pin-socket.connected {
            background-color: #3fb950;
            border-color: #2ea043;
        }
        .pin-socket.input {
            border-color: #56d364;
        }

        /* Bottom Preview Panel */
        .bottom-panel {
            background-color: #0d1117;
            border-top: 1px solid var(--panel-border);
            height: 220px;
            display: flex;
            flex-direction: column;
            flex-shrink: 0;
            z-index: 10;
        }

        .bottom-header {
            padding: 8px 14px;
            background-color: var(--panel-bg);
            border-bottom: 1px solid var(--panel-border);
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .code-preview {
            flex: 1;
            padding: 10px 14px;
            font-family: var(--mono-font);
            font-size: 12px;
            color: #c9d1d9;
            background-color: #090d13;
            overflow: auto;
            white-space: pre;
            line-height: 1.5;
            user-select: text;
        }

        /* Transformation Modal */
        .modal-overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100vw;
            height: 100vh;
            background-color: rgba(0, 0, 0, 0.65);
            display: none;
            justify-content: center;
            align-items: center;
            z-index: 100;
        }
        .modal-overlay.active {
            display: flex;
        }

        .modal-box {
            background-color: var(--panel-bg);
            border: 1px solid var(--node-border);
            border-radius: 8px;
            width: 440px;
            padding: 20px;
            box-shadow: 0 12px 36px rgba(0, 0, 0, 0.7);
            display: flex;
            flex-direction: column;
            gap: 14px;
        }

        .modal-title {
            font-size: 14px;
            font-weight: 600;
            color: #58a6ff;
            border-bottom: 1px solid var(--panel-border);
            padding-bottom: 8px;
        }

        .form-group {
            display: flex;
            flex-direction: column;
            gap: 4px;
        }
        .form-group label {
            font-size: 11px;
            color: var(--text-dim);
            text-transform: uppercase;
        }
        .form-control {
            background-color: #0d1117;
            border: 1px solid var(--node-border);
            color: var(--text-color);
            padding: 6px 10px;
            border-radius: 4px;
            font-size: 12px;
        }

        .modal-actions {
            display: flex;
            justify-content: flex-end;
            gap: 8px;
            margin-top: 10px;
        }
    </style>
</head>
<body>

    <!-- Top Navigation Header -->
    <div class="top-navbar">
        <div class="studio-title">
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="2" y="2" width="20" height="8" rx="2" ry="2"></rect>
                <rect x="2" y="14" width="20" height="8" rx="2" ry="2"></rect>
                <line x1="6" y1="6" x2="6.01" y2="6"></line>
                <line x1="6" y1="18" x2="6.01" y2="18"></line>
            </svg>
            <span>CAN-to-AOSP VHAL Visual Mapping Studio</span>
        </div>
        <div class="top-actions">
            <span id="statusIndicator" style="color: #3fb950; font-size: 12px; margin-right: 10px;">Ready</span>
            <button class="btn btn-blue" id="btnAutoDetect">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>
                Scan Workspace
            </button>
            <button class="btn" id="btnImportCpp">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="16 18 22 12 16 6"></polyline><polyline points="8 6 2 12 8 18"></polyline></svg>
                Reconstruct from Legacy C++
            </button>
        </div>
    </div>

    <!-- Main Studio Body -->
    <div class="studio-main">

        <!-- 1. Left Panel: CAN Signals -->
        <div class="side-panel">
            <div class="panel-header">
                <div class="panel-title-row">
                    <span class="panel-title can-title">CAN DBC Signals</span>
                    <span id="canStats" style="font-size: 11px; color: var(--text-dim);">0 msgs, 0 sigs</span>
                </div>
                <div style="display: flex; gap: 6px;">
                    <select id="dbcSelect" class="search-input" style="flex: 1;">
                        <option value="">-- Choose DBC File --</option>
                        ${workspaceDbcFiles.map(f => `<option value="${f}">${f}</option>`).join("")}
                    </select>
                    <button class="btn btn-sm" id="btnLoadDbcFile">Open...</button>
                </div>
                <input type="text" id="canFilterInput" class="search-input" placeholder="Search signals (e.g. Speed, 0x100)...">
                <div class="action-row">
                    <button class="btn btn-sm" id="btnExpandAll">Expand All</button>
                    <button class="btn btn-sm" id="btnCollapseAll">Collapse All</button>
                    <button class="btn btn-sm btn-primary" id="btnAddAllSignals">+ Add All Msgs</button>
                </div>
            </div>
            <div class="panel-list" id="canSignalList">
                <!-- Dynamically populated -->
            </div>
        </div>

        <!-- 2. Center Panel: Blueprint Node Canvas -->
        <div class="canvas-container">
            <div class="canvas-toolbar">
                <button class="btn btn-primary" id="btnGenerateCode">
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"></polygon></svg>
                    ⚡ Generate C++ Code
                </button>
                <button class="btn btn-blue" id="btnConnectSelected">
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"></path><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"></path></svg>
                    🔗 Connect Selected
                </button>
                <button class="btn" id="btnAutoConnect">⚡ Auto-Connect Matching</button>
                <button class="btn" id="btnAutoLayout">📐 Auto-Layout</button>
                <button class="btn" id="btnZoomFit">🔍 Zoom Fit</button>
                <button class="btn btn-danger" id="btnClearCanvas">🗑 Clear Canvas</button>
                <span id="canvasStats" style="margin-left: auto; font-size: 11px; color: var(--text-dim);">Nodes: 0 | Links: 0</span>
            </div>

            <!-- Blueprint Viewport -->
            <div class="blueprint-viewport" id="viewport">
                <!-- Links SVG Layer -->
                <svg class="links-layer" id="svgLayer"></svg>

                <!-- Draggable Dynamic Nodes Layer -->
                <div id="nodesContainer" style="position: absolute; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none;"></div>
            </div>
        </div>

        <!-- 3. Right Panel: AOSP Vehicle Properties -->
        <div class="side-panel right-panel">
            <div class="panel-header">
                <div class="panel-title-row">
                    <span class="panel-title vhal-title">AOSP Properties</span>
                    <span id="vhalStats" style="font-size: 11px; color: var(--text-dim);">0 Props</span>
                </div>
                <input type="text" id="vhalFilterInput" class="search-input" placeholder="Search property name or category...">
                <div class="action-row" style="flex-wrap: wrap;">
                    <button class="btn btn-sm cat-pill active" data-cat="All">All</button>
                    <button class="btn btn-sm cat-pill" data-cat="Vehicle Dynamics">Dynamics</button>
                    <button class="btn btn-sm cat-pill" data-cat="Powertrain">Powertrain</button>
                    <button class="btn btn-sm cat-pill" data-cat="HVAC">HVAC</button>
                    <button class="btn btn-sm cat-pill" data-cat="Body & Cabin">Body</button>
                </div>
            </div>
            <div class="panel-list" id="vhalPropertyList">
                <!-- Dynamically populated -->
            </div>
        </div>

    </div>

    <!-- 4. Bottom Row: Live Generated C++ Code Preview -->
    <div class="bottom-panel">
        <div class="bottom-header">
            <span style="font-weight: 600; font-size: 12px; color: #79c0ff;">
                LIVE GENERATED AOSP C++ CODE PREVIEW (onCanFrameReceived)
            </span>
            <div style="display: flex; gap: 8px;">
                <button class="btn btn-sm btn-primary" id="btnCopyCode">📋 Copy to Clipboard</button>
                <button class="btn btn-sm btn-blue" id="btnInsertCode">💾 Insert into Active File</button>
                <button class="btn btn-sm" id="btnSaveToFile">💾 Save to New File...</button>
            </div>
        </div>
        <pre class="code-preview" id="codePreview">// Connect CAN signals to AOSP properties on the canvas, then click '⚡ Generate C++ Code'</pre>
    </div>

    <!-- Transformation Modal Dialog -->
    <div class="modal-overlay" id="transformModal">
        <div class="modal-box">
            <div class="modal-title" id="modalTitle">Configure Link Transformation</div>
            <div class="form-group">
                <label>Unit Conversion Preset</label>
                <select id="modalConversion" class="form-control">
                    <option value="0">None (Direct Passthrough)</option>
                    <option value="1">km/h -> m/s (/ 3.6)</option>
                    <option value="2">m/s -> km/h (* 3.6)</option>
                    <option value="3">Celsius -> Kelvin (+ 273.15)</option>
                    <option value="4">Kelvin -> Celsius (- 273.15)</option>
                    <option value="5">RPM -> rad/s (* 0.10472)</option>
                    <option value="6">Custom Math Expression</option>
                </select>
            </div>
            <div class="form-group" id="exprGroup" style="display: none;">
                <label>Custom Math Expression (use 'rawVal')</label>
                <input type="text" id="modalExpr" class="form-control" placeholder="(rawVal * 0.1f) + 5.0f">
            </div>
            <div style="display: flex; gap: 10px;">
                <div class="form-group" style="flex: 1;">
                    <label>Scale Factor</label>
                    <input type="number" step="0.01" id="modalScale" class="form-control" value="1.0">
                </div>
                <div class="form-group" style="flex: 1;">
                    <label>Offset</label>
                    <input type="number" step="0.1" id="modalOffset" class="form-control" value="0.0">
                </div>
            </div>
            <div class="form-group">
                <label><input type="checkbox" id="modalHasClamp"> Enable Clamping</label>
                <div id="clampInputs" style="display: flex; gap: 10px; margin-top: 4px;">
                    <input type="number" id="modalClampMin" class="form-control" placeholder="Min" value="0">
                    <input type="number" id="modalClampMax" class="form-control" placeholder="Max" value="100">
                </div>
            </div>
            <div class="form-group">
                <label>Target Area ID (0 for Global / Vehicle)</label>
                <input type="number" id="modalAreaId" class="form-control" value="0">
            </div>
            <div class="modal-actions">
                <button class="btn" id="btnModalCancel">Cancel</button>
                <button class="btn btn-primary" id="btnModalApply">Apply Transformation</button>
            </div>
        </div>
    </div>

    <!-- Interactive Client Script -->
    <script>
        const vscode = acquireVsCodeApi();

        let canMessages = ${initialDbcMessagesJson};
        let vhalProperties = ${initialVhalPropertiesJson};

        // Canvas State
        let nodes = [];
        let links = [];
        let nextId = 1;
        let selectedNodeId = null;
        let connectingFromPin = null;
        let editingLinkId = null;

        // Viewport pan & zoom
        let panX = 40;
        let panY = 40;
        let zoom = 1.0;
        let isPanning = false;
        let startPanX = 0, startPanY = 0;

        // Elements
        const viewport = document.getElementById("viewport");
        const svgLayer = document.getElementById("svgLayer");
        const nodesContainer = document.getElementById("nodesContainer");
        const codePreview = document.getElementById("codePreview");
        const statusIndicator = document.getElementById("statusIndicator");

        function showStatus(text, duration = 3000) {
            statusIndicator.textContent = text;
            if (duration > 0) {
                setTimeout(() => {
                    statusIndicator.textContent = "Ready";
                }, duration);
            }
        }

        // Render CAN Palette
        function renderCanPalette() {
            const listEl = document.getElementById("canSignalList");
            const filter = document.getElementById("canFilterInput").value.toLowerCase();
            listEl.innerHTML = "";

            let totalSigs = 0;
            canMessages.forEach(msg => {
                totalSigs += msg.signals.length;
                const msgNameMatch = msg.name.toLowerCase().includes(filter);
                const msgIdMatch = ("0x" + msg.id.toString(16)).toLowerCase().includes(filter);
                const matchedSignals = msg.signals.filter(s =>
                    msgNameMatch || msgIdMatch || s.name.toLowerCase().includes(filter)
                );

                if (matchedSignals.length === 0 && filter) return;

                const group = document.createElement("div");
                group.className = "msg-group";

                const header = document.createElement("div");
                header.className = "msg-header";
                header.innerHTML = \`
                    <span>0x\${msg.id.toString(16).toUpperCase()} \${msg.name}</span>
                    <div style="display: flex; align-items: center; gap: 6px;">
                        <span style="font-size: 10px; color: var(--text-dim);">\${msg.signals.length} sigs</span>
                        <button class="btn btn-sm btn-primary" onclick="event.stopPropagation(); addAllMsgSignals(\${msg.id})">+ All</button>
                    </div>
                \`;

                const signalsContainer = document.createElement("div");
                signalsContainer.className = "msg-signals";

                matchedSignals.forEach(sig => {
                    const item = document.createElement("div");
                    item.className = "sig-item";
                    item.innerHTML = \`
                        <div style="display: flex; flex-direction: column; gap: 2px;">
                            <span style="font-weight: 500;">\${sig.name}</span>
                            <div style="display: flex; gap: 4px; align-items: center;">
                                <span class="badge \${sig.endianness === 1 ? 'badge-intel' : 'badge-motorola'}">
                                    \${sig.endianness === 1 ? 'Intel' : 'Motorola'}
                                </span>
                                \${sig.unit ? \`<span class="badge badge-unit">\${sig.unit}</span>\` : ''}
                                <span style="font-size: 10px; color: var(--text-dim);">b\${sig.startBit}:\${sig.bitLength}</span>
                            </div>
                        </div>
                        <button class="btn btn-sm btn-primary" onclick="addCanNode(\${msg.id}, '\${sig.name}')">+ Add</button>
                    \`;
                    signalsContainer.appendChild(item);
                });

                header.onclick = () => {
                    signalsContainer.style.display = signalsContainer.style.display === "none" ? "flex" : "none";
                };

                group.appendChild(header);
                group.appendChild(signalsContainer);
                listEl.appendChild(group);
            });

            document.getElementById("canStats").textContent = \`\${canMessages.length} msgs, \${totalSigs} sigs\`;
        }

        // Render VHAL Palette
        let activeCategory = "All";
        function renderVhalPalette() {
            const listEl = document.getElementById("vhalPropertyList");
            const filter = document.getElementById("vhalFilterInput").value.toLowerCase();
            listEl.innerHTML = "";

            const filtered = vhalProperties.filter(p => {
                if (activeCategory !== "All" && p.category !== activeCategory) return false;
                if (!filter) return true;
                const idHex = "0x" + p.id.toString(16).toLowerCase();
                return p.name.toLowerCase().includes(filter) || p.description.toLowerCase().includes(filter) || idHex.includes(filter);
            });

            filtered.forEach(p => {
                const item = document.createElement("div");
                item.className = "prop-item";
                item.innerHTML = \`
                    <div class="prop-top-row">
                        <span class="prop-name">\${p.name}</span>
                        <button class="btn btn-sm btn-primary" onclick="event.stopPropagation(); addVhalNode(\${p.id})">+ Add</button>
                    </div>
                    <div style="display: flex; gap: 6px; align-items: center;">
                        <span class="badge badge-type">\${p.type === 0x00600000 ? 'FLOAT' : p.type === 0x00400000 ? 'INT32' : 'BOOLEAN'}</span>
                        \${p.unit ? \`<span class="badge badge-unit">\${p.unit}</span>\` : ''}
                        <span style="font-size: 10px; color: var(--text-dim); font-family: var(--mono-font);">0x\${p.id.toString(16).toUpperCase()}</span>
                    </div>
                \`;
                item.ondblclick = () => addVhalNode(p.id);
                listEl.appendChild(item);
            });

            document.getElementById("vhalStats").textContent = \`\${filtered.length} Props\`;
        }

        // Node Add Operations
        window.addCanNode = function(msgId, sigName) {
            const msg = canMessages.find(m => m.id === msgId);
            if (!msg) return;
            const sig = msg.signals.find(s => s.name === sigName);
            if (!sig) return;

            // Check if already on canvas
            const existing = nodes.find(n => n.type === 0 && n.canMsgId === msgId && n.signal.name === sigName);
            if (existing) {
                selectNode(existing.id);
                showStatus("Node already on canvas");
                return;
            }

            const node = {
                id: nextId++,
                type: 0, // CAN
                canMsgId: msgId,
                canMsgName: msg.name,
                signal: sig,
                outputPinId: nextId++,
                posX: 80 + (nodes.length % 5) * 20,
                posY: 80 + (nodes.length % 5) * 30
            };
            nodes.push(node);
            renderCanvas();
            selectNode(node.id);
            showStatus(\`Added CAN signal: \${sig.name}\`);
        };

        window.addAllMsgSignals = function(msgId) {
            const msg = canMessages.find(m => m.id === msgId);
            if (!msg) return;
            msg.signals.forEach(sig => addCanNode(msgId, sig.name));
            autoLayout();
        };

        window.addVhalNode = function(propId) {
            const prop = vhalProperties.find(p => p.id === propId);
            if (!prop) return;

            const existing = nodes.find(n => n.type === 1 && n.property.id === propId);
            if (existing) {
                selectNode(existing.id);
                showStatus("Property already on canvas");
                return;
            }

            const node = {
                id: nextId++,
                type: 1, // VHAL
                property: prop,
                inputPinId: nextId++,
                posX: 460 + (nodes.length % 5) * 20,
                posY: 80 + (nodes.length % 5) * 30
            };
            nodes.push(node);
            renderCanvas();
            selectNode(node.id);
            showStatus(\`Added AOSP property: \${prop.name}\`);
        };

        function selectNode(id) {
            selectedNodeId = id;
            document.querySelectorAll(".node-card").forEach(el => {
                el.classList.toggle("selected", el.dataset.id == id);
            });
        }

        // Render Canvas Nodes and Links
        function renderCanvas() {
            nodesContainer.innerHTML = "";
            svgLayer.innerHTML = "";

            // Render Nodes
            nodes.forEach(node => {
                const card = document.createElement("div");
                card.className = "node-card" + (selectedNodeId === node.id ? " selected" : "");
                card.dataset.id = node.id;
                card.style.left = node.posX + "px";
                card.style.top = node.posY + "px";
                card.style.pointerEvents = "auto";

                if (node.type === 0) {
                    // CAN Signal Node
                    const isConnected = links.some(l => l.fromPinId === node.outputPinId);
                    card.innerHTML = \`
                        <div class="node-header can">
                            <span>CAN: \${node.canMsgName}</span>
                            <span style="font-size: 10px; opacity: 0.8;">0x\${node.canMsgId.toString(16).toUpperCase()}</span>
                        </div>
                        <div class="node-body">
                            <span style="font-weight: 600; color: #fff;">\${node.signal.name}</span>
                            <div style="display: flex; gap: 4px;">
                                <span class="badge \${node.signal.endianness === 1 ? 'badge-intel' : 'badge-motorola'}">
                                    \${node.signal.endianness === 1 ? 'Intel' : 'Motorola'}
                                </span>
                                \${node.signal.unit ? \`<span class="badge badge-unit">\${node.signal.unit}</span>\` : ''}
                            </div>
                            <div class="pin-row">
                                <span style="font-size: 11px; color: var(--accent-cyan);">OUTPUT PIN</span>
                                <div class="pin-socket \${isConnected ? 'connected' : ''}" data-pin="\${node.outputPinId}" title="Drag or click to connect"></div>
                            </div>
                        </div>
                    \`;
                } else {
                    // VHAL Property Node
                    const isConnected = links.some(l => l.toPinId === node.inputPinId);
                    card.innerHTML = \`
                        <div class="node-header vhal">
                            <span>AOSP: \${node.property.name}</span>
                        </div>
                        <div class="node-body">
                            <div style="display: flex; gap: 4px;">
                                <span class="badge badge-type">\${node.property.category}</span>
                                \${node.property.unit ? \`<span class="badge badge-unit">\${node.property.unit}</span>\` : ''}
                            </div>
                            <div class="pin-row">
                                <div class="pin-socket input \${isConnected ? 'connected' : ''}" data-pin="\${node.inputPinId}" title="Target connector pin"></div>
                                <span style="font-size: 11px; color: #7ee787;">INPUT PIN</span>
                            </div>
                        </div>
                    \`;
                }

                // Dragging Node Logic
                card.onmousedown = (e) => {
                    if (e.target.classList.contains("pin-socket")) return;
                    selectNode(node.id);
                    let startX = e.clientX;
                    let startY = e.clientY;
                    let origX = node.posX;
                    let origY = node.posY;

                    function onMouseMove(ev) {
                        node.posX = origX + (ev.clientX - startX);
                        node.posY = origY + (ev.clientY - startY);
                        card.style.left = node.posX + "px";
                        card.style.top = node.posY + "px";
                        renderLinks();
                    }

                    function onMouseUp() {
                        window.removeEventListener("mousemove", onMouseMove);
                        window.removeEventListener("mouseup", onMouseUp);
                    }

                    window.addEventListener("mousemove", onMouseMove);
                    window.addEventListener("mouseup", onMouseUp);
                };

                // Pin Connecting Logic
                const pin = card.querySelector(".pin-socket");
                if (pin) {
                    pin.onclick = (e) => {
                        e.stopPropagation();
                        const pinId = parseInt(pin.dataset.pin, 10);
                        if (!connectingFromPin) {
                            if (node.type === 0) {
                                connectingFromPin = pinId;
                                showStatus("Selected Output. Now click an Input pin on an AOSP node.");
                            }
                        } else {
                            if (node.type === 1) {
                                createLink(connectingFromPin, pinId);
                                connectingFromPin = null;
                            }
                        }
                    };
                }

                nodesContainer.appendChild(card);
            });

            renderLinks();
            document.getElementById("canvasStats").textContent = \`Nodes: \${nodes.length} | Links: \${links.length}\`;
        }

        // Render Bezier Curves between pins
        function renderLinks() {
            svgLayer.innerHTML = "";

            links.forEach(link => {
                const srcNode = nodes.find(n => n.outputPinId === link.fromPinId);
                const dstNode = nodes.find(n => n.inputPinId === link.toPinId);
                if (!srcNode || !dstNode) return;

                // Pin coordinates
                const x1 = srcNode.posX + 242;
                const y1 = srcNode.posY + 85;
                const x2 = dstNode.posX + 18;
                const y2 = dstNode.posY + 70;

                const dx = Math.abs(x2 - x1) * 0.55;
                const pathData = \`M \${x1} \${y1} C \${x1 + dx} \${y1}, \${x2 - dx} \${y2}, \${x2} \${y2}\`;

                const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
                path.setAttribute("d", pathData);
                path.setAttribute("class", "link-path" + (link.transform.conversion !== 0 ? " transformed" : ""));
                path.setAttribute("title", \`\${link.signalName} -> \${link.vhalPropertyName}\`);

                path.ondblclick = () => openTransformModal(link.id);

                svgLayer.appendChild(path);
            });
        }

        // Link Operations
        function createLink(fromPinId, toPinId) {
            const srcNode = nodes.find(n => n.outputPinId === fromPinId);
            const dstNode = nodes.find(n => n.inputPinId === toPinId);
            if (!srcNode || !dstNode) return false;

            // Check if link exists
            if (links.some(l => l.fromPinId === fromPinId && l.toPinId === toPinId)) {
                showStatus("Link already exists");
                return false;
            }

            // Auto-detect unit conversion
            let conv = 0;
            if (srcNode.signal.unit === "km/h" && dstNode.property.unit === "m/s") {
                conv = 1; // KMH_TO_MPS
            } else if (srcNode.signal.unit.includes("celsius") && dstNode.property.unit.includes("kelvin")) {
                conv = 3; // CELSIUS_TO_KELVIN
            }

            const link = {
                id: nextId++,
                canMsgId: srcNode.canMsgId,
                canMsgName: srcNode.canMsgName,
                signalName: srcNode.signal.name,
                vhalPropertyId: dstNode.property.id,
                vhalPropertyName: dstNode.property.name,
                fromPinId: fromPinId,
                toPinId: toPinId,
                transform: {
                    conversion: conv,
                    customScale: 1.0,
                    customOffset: 0.0,
                    customExpression: "",
                    hasClamp: false,
                    clampMin: 0,
                    clampMax: 100,
                    areaId: 0
                }
            };

            links.push(link);
            renderCanvas();
            synthesizeCode();
            showStatus(\`Connected: \${srcNode.signal.name} -> \${dstNode.property.name}!\`);
            return true;
        }

        // Connect Selected Nodes
        document.getElementById("btnConnectSelected").onclick = () => {
            const canNode = nodes.find(n => n.id === selectedNodeId && n.type === 0) || nodes.find(n => n.type === 0);
            const vhalNode = nodes.find(n => n.id === selectedNodeId && n.type === 1) || nodes.find(n => n.type === 1);

            if (canNode && vhalNode) {
                createLink(canNode.outputPinId, vhalNode.inputPinId);
            } else {
                showStatus("Add 1 CAN node and 1 AOSP node to canvas first");
            }
        };

        // Auto-Connect Matching Names
        document.getElementById("btnAutoConnect").onclick = () => {
            let count = 0;
            nodes.filter(n => n.type === 0).forEach(canNode => {
                const sigClean = canNode.signal.name.toLowerCase().replace(/[^a-z0-9]/g, "");
                nodes.filter(n => n.type === 1).forEach(vhalNode => {
                    const propClean = vhalNode.property.name.toLowerCase().replace(/[^a-z0-9]/g, "");
                    if (sigClean.includes(propClean) || propClean.includes(sigClean)) {
                        if (createLink(canNode.outputPinId, vhalNode.inputPinId)) count++;
                    }
                });
            });
            showStatus(\`Auto-connected \${count} matching pairs\`);
        };

        // Auto-Layout
        function autoLayout() {
            let canY = 40;
            let vhalY = 40;

            nodes.forEach(node => {
                if (node.type === 0) {
                    node.posX = 60;
                    node.posY = canY;
                    canY += 140;
                } else {
                    node.posX = 480;
                    node.posY = vhalY;
                    vhalY += 140;
                }
            });

            renderCanvas();
            showStatus("Auto-layout applied");
        }
        document.getElementById("btnAutoLayout").onclick = autoLayout;

        // Clear Canvas
        document.getElementById("btnClearCanvas").onclick = () => {
            nodes = [];
            links = [];
            selectedNodeId = null;
            renderCanvas();
            codePreview.textContent = "// Canvas cleared. Add nodes and connect pins to synthesize code.";
            showStatus("Canvas cleared");
        };

        // Zoom Fit
        document.getElementById("btnZoomFit").onclick = () => {
            panX = 40;
            panY = 40;
            zoom = 1.0;
            renderCanvas();
        };

        // Synthesize Code via Extension Host
        function synthesizeCode() {
            vscode.postMessage({
                command: "synthesize",
                links: links,
                canMessages: canMessages
            });
        }
        document.getElementById("btnGenerateCode").onclick = synthesizeCode;

        // Modal Logic
        function openTransformModal(linkId) {
            const link = links.find(l => l.id === linkId);
            if (!link) return;
            editingLinkId = linkId;

            document.getElementById("modalTitle").textContent = \`Transform: [\${link.signalName}] -> \${link.vhalPropertyName}\`;
            document.getElementById("modalConversion").value = link.transform.conversion;
            document.getElementById("modalScale").value = link.transform.customScale;
            document.getElementById("modalOffset").value = link.transform.customOffset;
            document.getElementById("modalExpr").value = link.transform.customExpression || "";
            document.getElementById("modalHasClamp").checked = link.transform.hasClamp;
            document.getElementById("modalClampMin").value = link.transform.clampMin;
            document.getElementById("modalClampMax").value = link.transform.clampMax;
            document.getElementById("modalAreaId").value = link.transform.areaId;

            document.getElementById("exprGroup").style.display = link.transform.conversion == 6 ? "flex" : "none";
            document.getElementById("transformModal").classList.add("active");
        }

        document.getElementById("modalConversion").onchange = (e) => {
            document.getElementById("exprGroup").style.display = e.target.value == 6 ? "flex" : "none";
        };

        document.getElementById("btnModalCancel").onclick = () => {
            document.getElementById("transformModal").classList.remove("active");
        };

        document.getElementById("btnModalApply").onclick = () => {
            const link = links.find(l => l.id === editingLinkId);
            if (link) {
                link.transform.conversion = parseInt(document.getElementById("modalConversion").value, 10);
                link.transform.customScale = parseFloat(document.getElementById("modalScale").value) || 1.0;
                link.transform.customOffset = parseFloat(document.getElementById("modalOffset").value) || 0.0;
                link.transform.customExpression = document.getElementById("modalExpr").value;
                link.transform.hasClamp = document.getElementById("modalHasClamp").checked;
                link.transform.clampMin = parseFloat(document.getElementById("modalClampMin").value) || 0;
                link.transform.clampMax = parseFloat(document.getElementById("modalClampMax").value) || 100;
                link.transform.areaId = parseInt(document.getElementById("modalAreaId").value, 10) || 0;

                renderLinks();
                synthesizeCode();
                showStatus("Updated transformation parameters");
            }
            document.getElementById("transformModal").classList.remove("active");
        };

        // File buttons & messages from extension
        document.getElementById("btnCopyCode").onclick = () => {
            navigator.clipboard.writeText(codePreview.textContent);
            showStatus("Copied to clipboard!");
        };

        document.getElementById("btnInsertCode").onclick = () => {
            vscode.postMessage({
                command: "insertCode",
                code: codePreview.textContent
            });
        };

        document.getElementById("btnSaveToFile").onclick = () => {
            vscode.postMessage({
                command: "saveCode",
                code: codePreview.textContent
            });
        };

        document.getElementById("btnAutoDetect").onclick = () => {
            vscode.postMessage({ command: "scanWorkspace" });
        };

        document.getElementById("btnImportCpp").onclick = () => {
            vscode.postMessage({ command: "importCpp" });
        };

        document.getElementById("btnLoadDbcFile").onclick = () => {
            vscode.postMessage({ command: "openDbcFile" });
        };

        document.getElementById("dbcSelect").onchange = (e) => {
            if (e.target.value) {
                vscode.postMessage({ command: "loadDbcPath", path: e.target.value });
            }
        };

        // Category filter pills
        document.querySelectorAll(".cat-pill").forEach(pill => {
            pill.onclick = () => {
                document.querySelectorAll(".cat-pill").forEach(p => p.classList.remove("active"));
                pill.classList.add("active");
                activeCategory = pill.dataset.cat;
                renderVhalPalette();
            };
        });

        // Search inputs
        document.getElementById("canFilterInput").oninput = renderCanPalette;
        document.getElementById("vhalFilterInput").oninput = renderVhalPalette;
        document.getElementById("btnExpandAll").onclick = () => {
            document.querySelectorAll(".msg-signals").forEach(el => el.style.display = "flex");
        };
        document.getElementById("btnCollapseAll").onclick = () => {
            document.querySelectorAll(".msg-signals").forEach(el => el.style.display = "none");
        };

        // Handle Messages Received From VS Code Host
        window.addEventListener("message", event => {
            const msg = event.data;
            switch (msg.type) {
                case "setDbcMessages":
                    canMessages = msg.messages;
                    renderCanPalette();
                    showStatus(\`Loaded DBC: \${canMessages.length} messages\`);
                    break;
                case "setCodePreview":
                    codePreview.textContent = msg.code;
                    break;
                case "loadConnections":
                    // Set nodes and links from reverse engineering
                    links = msg.links;
                    nodes = [];
                    // Populate CAN and VHAL nodes for each link
                    links.forEach(l => {
                        const m = canMessages.find(cm => cm.id === l.canMsgId);
                        const s = m ? m.signals.find(sig => sig.name === l.signalName) : { name: l.signalName, startBit: 0, bitLength: 16, endianness: 1, signedVal: false, factor: 1.0, offset: 0, minVal: 0, maxVal: 65535, unit: "", receiver: "", valueDescriptions: [] };
                        const prop = vhalProperties.find(vp => vp.id === l.vhalPropertyId) || { id: l.vhalPropertyId, name: l.vhalPropertyName, type: 0x00600000, category: "Powertrain", unit: "", description: "" };

                        let cNode = nodes.find(n => n.type === 0 && n.canMsgId === l.canMsgId && n.signal.name === l.signalName);
                        if (!cNode) {
                            cNode = { id: nextId++, type: 0, canMsgId: l.canMsgId, canMsgName: l.canMsgName, signal: s, outputPinId: l.fromPinId, posX: 80, posY: 80 };
                            nodes.push(cNode);
                        }
                        let vNode = nodes.find(n => n.type === 1 && n.property.id === l.vhalPropertyId);
                        if (!vNode) {
                            vNode = { id: nextId++, type: 1, property: prop, inputPinId: l.toPinId, posX: 480, posY: 80 };
                            nodes.push(vNode);
                        }
                    });
                    autoLayout();
                    synthesizeCode();
                    showStatus(\`Reconstituted \${links.length} visual connections!\`);
                    break;
                case "setWorkspaceDbcFiles":
                    const sel = document.getElementById("dbcSelect");
                    sel.innerHTML = '<option value="">-- Choose DBC File --</option>' +
                        msg.files.map(f => \`<option value="\${f}">\${f}</option>\`).join("");
                    break;
            }
        });

        // Initialize Palettes
        renderCanPalette();
        renderVhalPalette();

        // Default initial nodes if empty
        if (canMessages.length > 0 && vhalProperties.length > 0) {
            addCanNode(canMessages[0].id, canMessages[0].signals[0].name);
            addVhalNode(vhalProperties[0].id);
            autoLayout();
        }

    </script>
</body>
</html>`;
    }
}

