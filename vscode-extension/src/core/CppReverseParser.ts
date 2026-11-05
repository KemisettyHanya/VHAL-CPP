import { CanMessage, CanSignal, Endianness, NodeLink, UnitConversion } from "./AutomotiveTypes";
import { VhalCatalog } from "./VhalCatalog";

export interface ReverseEngineeringResult {
    success: boolean;
    errorMessage?: string;
    extractedLinks: NodeLink[];
    extractedMessages: CanMessage[];
}

export class CppReverseParser {
    public parseString(cppSource: string, existingDbcMessages: CanMessage[] = []): ReverseEngineeringResult {
        const result: ReverseEngineeringResult = {
            success: true,
            extractedLinks: [],
            extractedMessages: []
        };

        // 1. Check for Studio Reconstitution Metadata (Lossless roundtrip)
        if (cppSource.includes("CAN_VHAL_STUDIO_METADATA_START")) {
            let metaCounter = 5000;
            const lines = cppSource.split(/\r?\n/);
            for (const line of lines) {
                const trimmed = line.trim();
                if (!trimmed.startsWith("// LINK:")) continue;

                const kvPart = trimmed.substring(8).trim();
                const tokens = kvPart.split(/\s+/);
                const kv: Record<string, string> = {};
                for (const tok of tokens) {
                    const eq = tok.indexOf("=");
                    if (eq !== -1) {
                        kv[tok.substring(0, eq)] = tok.substring(eq + 1);
                    }
                }

                if (!kv["CAN_ID"] || !kv["SIGNAL"] || !kv["PROP_ID"]) continue;

                const canId = kv["CAN_ID"].startsWith("0x") || kv["CAN_ID"].startsWith("0X")
                    ? parseInt(kv["CAN_ID"], 16)
                    : parseInt(kv["CAN_ID"], 10);

                const propId = kv["PROP_ID"].startsWith("0x") || kv["PROP_ID"].startsWith("0X")
                    ? parseInt(kv["PROP_ID"], 16)
                    : parseInt(kv["PROP_ID"], 10);

                const link: NodeLink = {
                    id: ++metaCounter,
                    canMsgId: canId,
                    canMsgName: kv["CAN_NAME"] || ("Msg_0x" + canId.toString(16)),
                    signalName: kv["SIGNAL"],
                    vhalPropertyId: propId,
                    vhalPropertyName: kv["PROP_NAME"] || ("Prop_0x" + propId.toString(16)),
                    fromPinId: (canId << 16) + 1,
                    toPinId: (propId << 16) + 2,
                    transform: {
                        conversion: kv["CONV"] ? parseInt(kv["CONV"], 10) : UnitConversion.NONE,
                        customScale: kv["SCALE"] ? parseFloat(kv["SCALE"]) : 1.0,
                        customOffset: kv["OFFSET"] ? parseFloat(kv["OFFSET"]) : 0.0,
                        hasClamp: kv["CLAMP"] === "1",
                        clampMin: kv["MIN"] ? parseFloat(kv["MIN"]) : 0.0,
                        clampMax: kv["MAX"] ? parseFloat(kv["MAX"]) : 100.0,
                        areaId: kv["AREA"] ? parseInt(kv["AREA"], 10) : 0,
                        customExpression: kv["EXPR"] || ""
                    }
                };

                result.extractedLinks.push(link);
            }

            if (result.extractedLinks.length > 0) {
                return result;
            }
        }

        // 2. Generic Legacy C++ Reverse Engineering Parser
        const catalog = VhalCatalog.getInstance();
        const canIdConstants: Map<string, number> = new Map();

        // Extract #define CAN_... or constexpr ... CAN_...
        const defineRegex = /#define\s+([A-Za-z0-9_]+)\s+(0x[0-9a-fA-F]+|\d+)/g;
        let match: RegExpExecArray | null;
        while ((match = defineRegex.exec(cppSource)) !== null) {
            const val = match[2].startsWith("0x") ? parseInt(match[2], 16) : parseInt(match[2], 10);
            canIdConstants.set(match[1], val);
        }

        const constexprRegex = /constexpr\s+(?:uint32_t|int|auto)\s+([A-Za-z0-9_]+)\s*=\s*(0x[0-9a-fA-F]+|\d+)/g;
        while ((match = constexprRegex.exec(cppSource)) !== null) {
            const val = match[2].startsWith("0x") ? parseInt(match[2], 16) : parseInt(match[2], 10);
            canIdConstants.set(match[1], val);
        }

        // Search for switch cases
        const caseRegex = /case\s+([^:]+):([\s\S]*?)(?=case\s+[^:]+:|default\s*:|\})/g;
        let caseMatch: RegExpExecArray | null;
        let linkCounter = 1000;

        while ((caseMatch = caseRegex.exec(cppSource)) !== null) {
            const caseLabel = caseMatch[1].trim();
            const caseBody = caseMatch[2];

            let canId = 0;
            if (caseLabel.startsWith("0x") || caseLabel.startsWith("0X")) {
                canId = parseInt(caseLabel, 16);
            } else if (/^\d+$/.test(caseLabel)) {
                canId = parseInt(caseLabel, 10);
            } else if (canIdConstants.has(caseLabel)) {
                canId = canIdConstants.get(caseLabel)!;
            }

            if (canId === 0) continue;

            // Search for target VehicleProperty in case block
            // Matches VehicleProperty::XYZ or VehicleProperty_XYZ or XYZ
            for (const prop of catalog.getAll()) {
                const propRegex = new RegExp(`(?:VehicleProperty::|VehicleProperty_)?\\b${prop.name}\\b`);
                if (propRegex.test(caseBody)) {
                    // Try to identify signal name from comments or variable names
                    let sigName = "Signal";
                    const sigCommentMatch = caseBody.match(/(?:\/\/|\/\*)\s*(?:Signal|CAN)\s*:\s*([a-zA-Z0-9_]+)/i);
                    if (sigCommentMatch) {
                        sigName = sigCommentMatch[1];
                    } else {
                        // Check if existing DBC has signal for this CAN message
                        const dbcMsg = existingDbcMessages.find(m => m.id === canId);
                        if (dbcMsg && dbcMsg.signals.length > 0) {
                            sigName = dbcMsg.signals[0].name;
                        } else {
                            sigName = prop.name.toLowerCase();
                        }
                    }

                    // Detect conversions
                    let conversion = UnitConversion.NONE;
                    if (caseBody.includes("/ 3.6") || caseBody.includes("/3.6")) {
                        conversion = UnitConversion.KMH_TO_MPS;
                    } else if (caseBody.includes("* 3.6") || caseBody.includes("*3.6")) {
                        conversion = UnitConversion.MPS_TO_KMH;
                    } else if (caseBody.includes("+ 273.15") || caseBody.includes("+273.15")) {
                        conversion = UnitConversion.CELSIUS_TO_KELVIN;
                    } else if (caseBody.includes("* 0.10472")) {
                        conversion = UnitConversion.RPM_TO_RADS;
                    }

                    // Detect clamping
                    let hasClamp = false;
                    let clampMin = 0;
                    let clampMax = 100;
                    const clampMatch = caseBody.match(/std::clamp\s*\([^,]+,\s*([^,]+),\s*([^)]+)\)/);
                    if (clampMatch) {
                        hasClamp = true;
                        clampMin = parseFloat(clampMatch[1]) || 0;
                        clampMax = parseFloat(clampMatch[2]) || 100;
                    }

                    const link: NodeLink = {
                        id: ++linkCounter,
                        canMsgId: canId,
                        canMsgName: "Msg_0x" + canId.toString(16).toUpperCase(),
                        signalName: sigName,
                        vhalPropertyId: prop.id,
                        vhalPropertyName: prop.name,
                        fromPinId: (canId << 16) + 1,
                        toPinId: (prop.id << 16) + 2,
                        transform: {
                            conversion,
                            customScale: 1.0,
                            customOffset: 0.0,
                            customExpression: "",
                            hasClamp,
                            clampMin,
                            clampMax,
                            areaId: 0
                        }
                    };

                    result.extractedLinks.push(link);
                }
            }
        }

        return result;
    }
}

