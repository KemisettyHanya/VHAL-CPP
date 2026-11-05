import { VhalProperty, VhalPropertyType } from "./AutomotiveTypes";

export class VhalCatalog {
    private static instance: VhalCatalog;
    private properties: VhalProperty[] = [];
    private idMap: Map<number, VhalProperty> = new Map();
    private nameMap: Map<string, VhalProperty> = new Map();

    private constructor() {
        this.initCatalog();
    }

    public static getInstance(): VhalCatalog {
        if (!VhalCatalog.instance) {
            VhalCatalog.instance = new VhalCatalog();
        }
        return VhalCatalog.instance;
    }

    private initCatalog(): void {
        this.properties = [
            // Vehicle Dynamics
            {
                id: 0x11600207,
                name: "PERF_VEHICLE_SPEED",
                type: VhalPropertyType.FLOAT,
                category: "Vehicle Dynamics",
                unit: "m/s",
                description: "Vehicle speed in meters per second."
            },
            {
                id: 0x11600204,
                name: "PERF_ODOMETER",
                type: VhalPropertyType.FLOAT,
                category: "Vehicle Dynamics",
                unit: "km",
                description: "Current odometer reading in kilometers."
            },
            {
                id: 0x11400205,
                name: "PERF_STEERING_ANGLE",
                type: VhalPropertyType.FLOAT,
                category: "Vehicle Dynamics",
                unit: "degrees",
                description: "Steering wheel angle in degrees. Positive is clockwise."
            },
            {
                id: 0x11400208,
                name: "LATERAL_ACCELERATION",
                type: VhalPropertyType.FLOAT,
                category: "Vehicle Dynamics",
                unit: "m/s^2",
                description: "Lateral acceleration in m/s^2. Positive is rightward."
            },
            {
                id: 0x11400209,
                name: "LONGITUDINAL_ACCELERATION",
                type: VhalPropertyType.FLOAT,
                category: "Vehicle Dynamics",
                unit: "m/s^2",
                description: "Longitudinal acceleration in m/s^2. Positive is forward."
            },
            {
                id: 0x1140020A,
                name: "YAW_RATE",
                type: VhalPropertyType.FLOAT,
                category: "Vehicle Dynamics",
                unit: "deg/s",
                description: "Yaw rate in degrees per second. Positive is clockwise."
            },
            // Powertrain
            {
                id: 0x11400305,
                name: "ENGINE_RPM",
                type: VhalPropertyType.FLOAT,
                category: "Powertrain",
                unit: "rpm",
                description: "Engine speed in revolutions per minute (RPM)."
            },
            {
                id: 0x11400303,
                name: "ENGINE_OIL_TEMP",
                type: VhalPropertyType.FLOAT,
                category: "Powertrain",
                unit: "celsius",
                description: "Engine oil temperature in Celsius."
            },
            {
                id: 0x11400301,
                name: "ENGINE_COOLANT_TEMP",
                type: VhalPropertyType.FLOAT,
                category: "Powertrain",
                unit: "celsius",
                description: "Engine coolant temperature in Celsius."
            },
            {
                id: 0x11400400,
                name: "GEAR_SELECTION",
                type: VhalPropertyType.INT32,
                category: "Powertrain",
                unit: "enum",
                description: "Current selected transmission gear position (P, R, N, D)."
            },
            {
                id: 0x11400401,
                name: "CURRENT_GEAR",
                type: VhalPropertyType.INT32,
                category: "Powertrain",
                unit: "gear",
                description: "Actual current operational gear ratio of transmission."
            },
            {
                id: 0x11200402,
                name: "PARKING_BRAKE_ON",
                type: VhalPropertyType.BOOLEAN,
                category: "Powertrain",
                unit: "bool",
                description: "Whether parking brake is currently engaged."
            },
            {
                id: 0x11600505,
                name: "FUEL_LEVEL",
                type: VhalPropertyType.FLOAT,
                category: "Powertrain",
                unit: "milliliters",
                description: "Fuel remaining in fuel tank in milliliters."
            },
            {
                id: 0x11200503,
                name: "FUEL_LEVEL_LOW",
                type: VhalPropertyType.BOOLEAN,
                category: "Powertrain",
                unit: "bool",
                description: "Low fuel warning status."
            },
            {
                id: 0x11600507,
                name: "EV_BATTERY_LEVEL",
                type: VhalPropertyType.FLOAT,
                category: "Powertrain",
                unit: "watt_hours",
                description: "EV battery remaining charge level in watt-hours."
            },
            {
                id: 0x1120050B,
                name: "EV_CHARGE_PORT_CONNECTED",
                type: VhalPropertyType.BOOLEAN,
                category: "Powertrain",
                unit: "bool",
                description: "Whether EV charging cable is currently plugged in."
            },
            {
                id: 0x1140050C,
                name: "EV_BATTERY_INSTANTANEOUS_CHARGE_RATE",
                type: VhalPropertyType.FLOAT,
                category: "Powertrain",
                unit: "milliwatts",
                description: "Instantaneous EV battery charge/discharge rate."
            },
            // HVAC
            {
                id: 0x12400502,
                name: "HVAC_TEMPERATURE_CURRENT",
                type: VhalPropertyType.FLOAT,
                category: "HVAC",
                unit: "celsius",
                description: "Current measured cabin temperature in Celsius."
            },
            {
                id: 0x12400503,
                name: "HVAC_TEMPERATURE_SET",
                type: VhalPropertyType.FLOAT,
                category: "HVAC",
                unit: "celsius",
                description: "Target set temperature for HVAC zone in Celsius."
            },
            {
                id: 0x12200504,
                name: "HVAC_DEFROSTER",
                type: VhalPropertyType.BOOLEAN,
                category: "HVAC",
                unit: "bool",
                description: "Defroster on/off state for windshield/windows."
            },
            {
                id: 0x12200505,
                name: "HVAC_AC_ON",
                type: VhalPropertyType.BOOLEAN,
                category: "HVAC",
                unit: "bool",
                description: "A/C compressor activation status."
            },
            {
                id: 0x12400500,
                name: "HVAC_FAN_SPEED",
                type: VhalPropertyType.INT32,
                category: "HVAC",
                unit: "level",
                description: "HVAC blower fan speed setting."
            },
            {
                id: 0x12400501,
                name: "HVAC_FAN_DIRECTION",
                type: VhalPropertyType.INT32,
                category: "HVAC",
                unit: "direction",
                description: "HVAC air vent output direction mask."
            },
            {
                id: 0x1220050A,
                name: "HVAC_AUTO_ON",
                type: VhalPropertyType.BOOLEAN,
                category: "HVAC",
                unit: "bool",
                description: "HVAC automatic climate control mode enabled."
            },
            // Body & Cabin
            {
                id: 0x16200B00,
                name: "DOOR_LOCK",
                type: VhalPropertyType.BOOLEAN,
                category: "Body & Cabin",
                unit: "bool",
                description: "Door lock status (true = locked, false = unlocked)."
            },
            {
                id: 0x16400B01,
                name: "DOOR_POS",
                type: VhalPropertyType.INT32,
                category: "Body & Cabin",
                unit: "mm",
                description: "Door opening position in millimeters or percentage."
            },
            {
                id: 0x16400B0B,
                name: "WINDOW_POS",
                type: VhalPropertyType.INT32,
                category: "Body & Cabin",
                unit: "mm",
                description: "Window position (0 = closed, max = open)."
            },
            // Lighting & Controls
            {
                id: 0x11400F03,
                name: "HAZARD_LIGHTS_SWITCH",
                type: VhalPropertyType.INT32,
                category: "Lighting & Controls",
                unit: "enum",
                description: "Hazard flasher warning switch position."
            },
            {
                id: 0x11400F00,
                name: "HEADLIGHTS_STATE",
                type: VhalPropertyType.INT32,
                category: "Lighting & Controls",
                unit: "enum",
                description: "Current exterior headlight operating mode."
            },
            {
                id: 0x11400F08,
                name: "TURN_SIGNAL_STATE",
                type: VhalPropertyType.INT32,
                category: "Lighting & Controls",
                unit: "enum",
                description: "Turn signal blinker state (NONE, RIGHT, LEFT)."
            },
            {
                id: 0x11400409,
                name: "IGNITION_STATE",
                type: VhalPropertyType.INT32,
                category: "Lighting & Controls",
                unit: "enum",
                description: "Vehicle ignition system state (LOCK, OFF, ACC, ON, START)."
            }
        ];

        for (const prop of this.properties) {
            this.idMap.set(prop.id, prop);
            this.nameMap.set(prop.name, prop);
        }
    }

    public getAll(): VhalProperty[] {
        return this.properties;
    }

    public getCategories(): string[] {
        const cats = new Set<string>();
        for (const p of this.properties) {
            cats.add(p.category);
        }
        return Array.from(cats);
    }

    public findById(id: number): VhalProperty | undefined {
        return this.idMap.get(id);
    }

    public findByName(name: string): VhalProperty | undefined {
        return this.nameMap.get(name);
    }

    public findClosestMatch(query: string): VhalProperty | undefined {
        if (!query) return undefined;
        const clean = query.replace(/[^a-zA-Z0-9]/g, "").toLowerCase();

        for (const p of this.properties) {
            const cleanProp = p.name.replace(/[^a-zA-Z0-9]/g, "").toLowerCase();
            if (cleanProp === clean) return p;
            if (cleanProp.includes(clean) || clean.includes(cleanProp)) return p;
        }

        if (clean.includes("speed")) return this.findByName("PERF_VEHICLE_SPEED");
        if (clean.includes("rpm") || clean.includes("enginespeed")) return this.findByName("ENGINE_RPM");
        if (clean.includes("gear")) return this.findByName("GEAR_SELECTION");
        if (clean.includes("temp") && clean.includes("coolant")) return this.findByName("ENGINE_COOLANT_TEMP");
        if (clean.includes("temp") && clean.includes("oil")) return this.findByName("ENGINE_OIL_TEMP");
        if (clean.includes("fuel")) return this.findByName("FUEL_LEVEL");
        if (clean.includes("steer")) return this.findByName("PERF_STEERING_ANGLE");
        if (clean.includes("yaw")) return this.findByName("YAW_RATE");
        if (clean.includes("door")) return this.findByName("DOOR_LOCK");

        return undefined;
    }

    public search(query: string, category?: string): VhalProperty[] {
        const qLower = query ? query.toLowerCase().trim() : "";
        return this.properties.filter(p => {
            if (category && category !== "All" && p.category !== category) {
                return false;
            }
            if (!qLower) return true;
            const idHex = "0x" + p.id.toString(16).toLowerCase();
            return (
                p.name.toLowerCase().includes(qLower) ||
                p.description.toLowerCase().includes(qLower) ||
                idHex.includes(qLower) ||
                p.unit.toLowerCase().includes(qLower)
            );
        });
    }
}

