import { CanMessage, CanSignal, Endianness, ValueDescription } from "./AutomotiveTypes";

export class DbcParser {
    private messages: CanMessage[] = [];

    public parseString(content: string): boolean {
        this.messages = [];
        const lines = content.split(/\r?\n/);
        let currentMsg: CanMessage | null = null;

        for (let i = 0; i < lines.length; ++i) {
            let line = lines[i].trim();
            if (!line || line.startsWith("//")) continue;

            // 1. BO_ <ID> <Name>: <DLC> <Sender>
            if (line.startsWith("BO_ ")) {
                const match = line.match(/^BO_\s+(\d+)\s+([a-zA-Z0-9_]+)\s*:\s*(\d+)\s+([a-zA-Z0-9_]+)/);
                if (match) {
                    const rawId = parseInt(match[1], 10);
                    // Standard CAN ID is 11-bit or 29-bit (masking top bits if CAN extended flag 0x80000000 is present)
                    const canId = rawId & 0x1FFFFFFF;
                    currentMsg = {
                        id: canId,
                        name: match[2],
                        dlc: parseInt(match[3], 10),
                        sender: match[4],
                        signals: []
                    };
                    this.messages.push(currentMsg);
                }
            }
            // 2. SG_ <Name> [M|m\d+] : <StartBit>|<Length>@<Endian><Sign> (<Factor>,<Offset>) [<Min>|<Max>] "<Unit>" <Receiver>
            else if (line.startsWith("SG_ ") && currentMsg) {
                // Regex handles optional multiplexer indicator (e.g. M or m0)
                const sgMatch = line.match(/^SG_\s+([a-zA-Z0-9_]+)(?:\s+[mM\d]+)?\s*:\s*(\d+)\|(\d+)@([01])([+-])\s*\(([^,]+),([^)]+)\)\s*\[([^|]+)\|([^\]]+)\]\s*"([^"]*)"\s*([a-zA-Z0-9_,\s]+)?/);
                if (sgMatch) {
                    const sig: CanSignal = {
                        name: sgMatch[1],
                        startBit: parseInt(sgMatch[2], 10),
                        bitLength: parseInt(sgMatch[3], 10),
                        endianness: sgMatch[4] === "1" ? Endianness.LittleEndian : Endianness.BigEndian,
                        signedVal: sgMatch[5] === "-",
                        factor: parseFloat(sgMatch[6]),
                        offset: parseFloat(sgMatch[7]),
                        minVal: parseFloat(sgMatch[8]),
                        maxVal: parseFloat(sgMatch[9]),
                        unit: sgMatch[10] || "",
                        receiver: (sgMatch[11] || "Vector__XXX").trim(),
                        valueDescriptions: []
                    };
                    currentMsg.signals.push(sig);
                }
            }
            // 3. CM_ BO_ <ID> "<Comment>"; or CM_ SG_ <ID> <Name> "<Comment>";
            else if (line.startsWith("CM_ ")) {
                const cmBoMatch = line.match(/^CM_\s+BO_\s+(\d+)\s+"([^"]*)";?/);
                if (cmBoMatch) {
                    const id = parseInt(cmBoMatch[1], 10) & 0x1FFFFFFF;
                    const msg = this.findMessage(id);
                    if (msg) msg.comment = cmBoMatch[2];
                    continue;
                }
                const cmSgMatch = line.match(/^CM_\s+SG_\s+(\d+)\s+([a-zA-Z0-9_]+)\s+"([^"]*)";?/);
                if (cmSgMatch) {
                    const id = parseInt(cmSgMatch[1], 10) & 0x1FFFFFFF;
                    const sig = this.findSignal(id, cmSgMatch[2]);
                    if (sig) sig.comment = cmSgMatch[3];
                    continue;
                }
            }
            // 4. VAL_ <ID> <SignalName> <val> "<desc>" ... ;
            else if (line.startsWith("VAL_ ")) {
                const valHeader = line.match(/^VAL_\s+(\d+)\s+([a-zA-Z0-9_]+)\s+(.+);?$/);
                if (valHeader) {
                    const id = parseInt(valHeader[1], 10) & 0x1FFFFFFF;
                    const sig = this.findSignal(id, valHeader[2]);
                    if (sig) {
                        const pairsStr = valHeader[3];
                        const pairRegex = /(\d+)\s+"([^"]*)"/g;
                        let m: RegExpExecArray | null;
                        while ((m = pairRegex.exec(pairsStr)) !== null) {
                            sig.valueDescriptions.push({
                                value: parseInt(m[1], 10),
                                description: m[2]
                            });
                        }
                    }
                }
            }
        }

        return this.messages.length > 0;
    }

    public getMessages(): CanMessage[] {
        return this.messages;
    }

    public totalSignalsCount(): number {
        return this.messages.reduce((acc, m) => acc + m.signals.length, 0);
    }

    public findMessage(id: number): CanMessage | undefined {
        return this.messages.find(m => m.id === id);
    }

    public findSignal(msgId: number, signalName: string): CanSignal | undefined {
        const msg = this.findMessage(msgId);
        if (!msg) return undefined;
        return msg.signals.find(s => s.name === signalName);
    }
}

