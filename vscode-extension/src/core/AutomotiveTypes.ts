export enum Endianness {
    LittleEndian = 1, // Intel
    BigEndian = 0     // Motorola
}

export enum VhalPropertyType {
    BOOLEAN = 0x00200000,
    INT32   = 0x00400000,
    INT64   = 0x00500000,
    FLOAT   = 0x00600000,
    STRING  = 0x00100000,
    MIXED   = 0x00e00000
}

export function vhalTypeToString(type: VhalPropertyType): string {
    switch (type) {
        case VhalPropertyType.BOOLEAN: return "BOOLEAN";
        case VhalPropertyType.INT32:   return "INT32";
        case VhalPropertyType.INT64:   return "INT64";
        case VhalPropertyType.FLOAT:   return "FLOAT";
        case VhalPropertyType.STRING:  return "STRING";
        case VhalPropertyType.MIXED:   return "MIXED";
        default: return "UNKNOWN";
    }
}

export interface ValueDescription {
    value: number;
    description: string;
}

export interface CanSignal {
    name: string;
    startBit: number;
    bitLength: number;
    endianness: Endianness;
    signedVal: boolean;
    factor: number;
    offset: number;
    minVal: number;
    maxVal: number;
    unit: string;
    receiver: string;
    comment?: string;
    valueDescriptions: ValueDescription[];
}

export interface CanMessage {
    id: number;
    name: string;
    dlc: number;
    sender: string;
    comment?: string;
    signals: CanSignal[];
}

export interface VhalProperty {
    id: number;
    name: string;
    type: VhalPropertyType;
    category: string;
    unit: string;
    description: string;
}

export enum UnitConversion {
    NONE = 0,
    KMH_TO_MPS,       // / 3.6
    MPS_TO_KMH,       // * 3.6
    CELSIUS_TO_KELVIN, // + 273.15
    KELVIN_TO_CELSIUS, // - 273.15
    RPM_TO_RADS,      // * 0.10472
    CUSTOM_EXPR
}

export interface TransformConfig {
    conversion: UnitConversion;
    customScale: number;
    customOffset: number;
    customExpression: string;
    hasClamp: boolean;
    clampMin: number;
    clampMax: number;
    areaId: number;
}

export interface NodeLink {
    id: number;
    canMsgId: number;
    canMsgName: string;
    signalName: string;
    vhalPropertyId: number;
    vhalPropertyName: string;
    fromPinId: number;
    toPinId: number;
    transform: TransformConfig;
}

export enum CanvasNodeType {
    CAN_SIGNAL = 0,
    VHAL_PROPERTY = 1
}

export interface CanvasNode {
    id: number;
    type: CanvasNodeType;
    canMsgId?: number;
    canMsgName?: string;
    signal?: CanSignal;
    property?: VhalProperty;
    outputPinId?: number;
    inputPinId?: number;
    posX: number;
    posY: number;
}

