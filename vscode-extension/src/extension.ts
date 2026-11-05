import * as vscode from "vscode";
import * as fs from "fs";
import * as path from "path";
import { DbcParser } from "./core/DbcParser";
import { VhalCatalog } from "./core/VhalCatalog";
import { CppSynthesizer } from "./core/CppSynthesizer";
import { CppReverseParser } from "./core/CppReverseParser";
import { StudioWebview } from "./webview/StudioWebview";

export function activate(context: vscode.ExtensionContext) {
    let currentPanel: vscode.WebviewPanel | undefined = undefined;
    const dbcParser = new DbcParser();
    const vhalCatalog = VhalCatalog.getInstance();
    const synthesizer = new CppSynthesizer();
    const reverseParser = new CppReverseParser();

    // Default sample DBC data embedded for instant first-run experience
    const defaultSampleDbc = `
BO_ 256 VehicleDynamics: 8 Vector__XXX
 SG_ VehicleSpeed : 0|16@1+ (0.01,0) [0|300] "km/h" Vector__XXX
 SG_ YawRate : 16|16@1- (0.05,0) [-100|100] "deg/s" Vector__XXX
 SG_ SteeringAngle : 32|16@1- (0.1,0) [-720|720] "deg" Vector__XXX
 SG_ LateralAccel : 48|16@1- (0.01,0) [-20|20] "m/s2" Vector__XXX

BO_ 512 EngineStatus: 8 Vector__XXX
 SG_ EngineRPM : 0|16@1+ (0.25,0) [0|8000] "rpm" Vector__XXX
 SG_ EngineOilTemp : 16|8@1+ (1,-40) [-40|215] "deg C" Vector__XXX
 SG_ EngineCoolantTemp : 24|8@1+ (1,-40) [-40|215] "deg C" Vector__XXX
 SG_ FuelLevel : 32|8@1+ (0.5,0) [0|100] "%" Vector__XXX

BO_ 768 TransmissionStatus: 8 Vector__XXX
 SG_ GearSelection : 0|4@1+ (1,0) [0|7] "" Vector__XXX
 SG_ CurrentGear : 4|4@1+ (1,0) [0|7] "" Vector__XXX
 SG_ ParkingBrake : 8|1@1+ (1,0) [0|1] "" Vector__XXX
`;
    dbcParser.parseString(defaultSampleDbc);

    // 1. Status Bar Item
    const statusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 100);
    statusBarItem.command = "canVhalStudio.openStudio";
    statusBarItem.text = "$(circuit-board) CAN-VHAL Studio";
    statusBarItem.tooltip = "Click to open CAN-to-AOSP VHAL Visual Mapping Studio (Ctrl+Alt+V)";
    statusBarItem.show();
    context.subscriptions.push(statusBarItem);

    // Helper: Scan workspace for DBC files
    async function scanWorkspaceDbcFiles(): Promise<string[]> {
        const uris = await vscode.workspace.findFiles("**/*.dbc", "**/node_modules/**", 20);
        return uris.map(u => u.fsPath);
    }

    // 2. Open Studio Command
    const openStudioCommand = vscode.commands.registerCommand("canVhalStudio.openStudio", async (uri?: vscode.Uri) => {
        // If already open, reveal
        if (currentPanel) {
            currentPanel.reveal(vscode.ViewColumn.One);
            if (uri && uri.fsPath.endsWith(".dbc")) {
                loadDbcFromFile(uri.fsPath);
            }
            return;
        }

        const workspaceDbcs = await scanWorkspaceDbcFiles();

        // If command was triggered by clicking a DBC file in the explorer
        if (uri && uri.fsPath.endsWith(".dbc")) {
            try {
                const content = fs.readFileSync(uri.fsPath, "utf8");
                dbcParser.parseString(content);
            } catch (err: any) {
                vscode.window.showErrorMessage(`Failed to open DBC: ${err.message}`);
            }
        }

        currentPanel = vscode.window.createWebviewPanel(
            "canVhalStudio",
            "CAN-to-AOSP VHAL Studio",
            vscode.ViewColumn.One,
            {
                enableScripts: true,
                retainContextWhenHidden: true,
                localResourceRoots: [context.extensionUri]
            }
        );

        currentPanel.webview.html = StudioWebview.getHtml(
            currentPanel.webview,
            context.extensionUri,
            JSON.stringify(dbcParser.getMessages()),
            JSON.stringify(vhalCatalog.getAll()),
            workspaceDbcs
        );

        // Handle Messages from Webview
        currentPanel.webview.onDidReceiveMessage(async (message) => {
            switch (message.command) {
                case "synthesize": {
                    const code = synthesizer.synthesize(message.links || [], dbcParser.getMessages());
                    currentPanel?.webview.postMessage({
                        type: "setCodePreview",
                        code: code
                    });
                    break;
                }
                case "insertCode": {
                    const editor = vscode.window.activeTextEditor;
                    if (editor) {
                        editor.edit(editBuilder => {
                            editBuilder.insert(editor.selection.active, message.code);
                        });
                        vscode.window.showInformationMessage("Inserted generated VHAL C++ code into active file!");
                    } else {
                        vscode.window.showWarningMessage("No active editor found to insert code. Open a C++ file first.");
                    }
                    break;
                }
                case "saveCode": {
                    const saveUri = await vscode.window.showSaveDialog({
                        filters: { "C++ Source": ["cpp", "cc", "cxx"] },
                        defaultUri: vscode.Uri.file("generated_vhal_mapping.cpp")
                    });
                    if (saveUri) {
                        fs.writeFileSync(saveUri.fsPath, message.code, "utf8");
                        vscode.window.showInformationMessage(`Saved VHAL C++ code to ${path.basename(saveUri.fsPath)}!`);
                        const doc = await vscode.workspace.openTextDocument(saveUri);
                        await vscode.window.showTextDocument(doc, vscode.ViewColumn.Beside);
                    }
                    break;
                }
                case "openDbcFile": {
                    const openUris = await vscode.window.showOpenDialog({
                        canSelectFiles: true,
                        canSelectFolders: false,
                        canSelectMany: false,
                        filters: { "CAN Database": ["dbc"] }
                    });
                    if (openUris && openUris[0]) {
                        loadDbcFromFile(openUris[0].fsPath);
                    }
                    break;
                }
                case "loadDbcPath": {
                    loadDbcFromFile(message.path);
                    break;
                }
                case "scanWorkspace": {
                    const files = await scanWorkspaceDbcFiles();
                    currentPanel?.webview.postMessage({
                        type: "setWorkspaceDbcFiles",
                        files: files
                    });
                    vscode.window.showInformationMessage(`Found ${files.length} DBC files in workspace.`);
                    break;
                }
                case "importCpp": {
                    const cppUris = await vscode.window.showOpenDialog({
                        canSelectFiles: true,
                        canSelectFolders: false,
                        canSelectMany: false,
                        filters: { "C++ Files": ["cpp", "hpp", "cc", "h"] }
                    });
                    if (cppUris && cppUris[0]) {
                        try {
                            const content = fs.readFileSync(cppUris[0].fsPath, "utf8");
                            const res = reverseParser.parseString(content, dbcParser.getMessages());
                            if (res.extractedLinks.length > 0) {
                                currentPanel?.webview.postMessage({
                                    type: "loadConnections",
                                    links: res.extractedLinks
                                });
                                vscode.window.showInformationMessage(
                                    `Reconstituted ${res.extractedLinks.length} visual node connections from ${path.basename(cppUris[0].fsPath)}!`
                                );
                            } else {
                                vscode.window.showWarningMessage("No CAN-to-AOSP mappings detected in the selected C++ file.");
                            }
                        } catch (err: any) {
                            vscode.window.showErrorMessage(`Error reading C++ file: ${err.message}`);
                        }
                    }
                    break;
                }
            }
        });

        currentPanel.onDidDispose(() => {
            currentPanel = undefined;
        });
    });

    function loadDbcFromFile(filePath: string) {
        try {
            const content = fs.readFileSync(filePath, "utf8");
            if (dbcParser.parseString(content)) {
                currentPanel?.webview.postMessage({
                    type: "setDbcMessages",
                    messages: dbcParser.getMessages()
                });
                vscode.window.showInformationMessage(
                    `Loaded DBC: ${path.basename(filePath)} (${dbcParser.getMessages().length} messages, ${dbcParser.totalSignalsCount()} signals)`
                );
            } else {
                vscode.window.showErrorMessage("Failed to parse DBC file. Ensure valid Vector DBC format.");
            }
        } catch (err: any) {
            vscode.window.showErrorMessage(`Error reading DBC file: ${err.message}`);
        }
    }

    // 3. Import DBC File Command
    const importDbcCommand = vscode.commands.registerCommand("canVhalStudio.importDbc", async () => {
        const uris = await vscode.window.showOpenDialog({
            canSelectFiles: true,
            canSelectFolders: false,
            canSelectMany: false,
            filters: { "CAN Database": ["dbc"] }
        });
        if (uris && uris[0]) {
            vscode.commands.executeCommand("canVhalStudio.openStudio", uris[0]);
        }
    });

    // 4. Reconstruct from C++ Command
    const reconstructCommand = vscode.commands.registerCommand("canVhalStudio.reconstructFromCpp", async () => {
        vscode.commands.executeCommand("canVhalStudio.openStudio");
        setTimeout(() => {
            currentPanel?.webview.postMessage({ command: "triggerImportCpp" });
        }, 500);
    });

    // 5. Synthesize into Active File Command
    const synthesizeCommand = vscode.commands.registerCommand("canVhalStudio.synthesizeCpp", async () => {
        if (!currentPanel) {
            vscode.window.showWarningMessage("Open CAN-to-VHAL Studio first to configure active node mappings.");
            return;
        }
        currentPanel.webview.postMessage({ command: "triggerSynthesize" });
    });

    context.subscriptions.push(
        openStudioCommand,
        importDbcCommand,
        reconstructCommand,
        synthesizeCommand
    );
}

export function deactivate() {}

