#include "VhalCatalog.hpp"
#include <algorithm>
#include <cctype>

namespace can_vhal {

namespace {

std::string toLower(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) {
        res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return res;
}

std::string cleanName(std::string_view s) {
    std::string res;
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return res;
}

} // namespace

VhalCatalog& VhalCatalog::instance() {
    static VhalCatalog sInstance;
    return sInstance;
}

VhalCatalog::VhalCatalog() {
    initStandardCatalog();
}

void VhalCatalog::initStandardCatalog() {
    mProperties = {
        // Vehicle Dynamics
        {
            0x11600207,
            "PERF_VEHICLE_SPEED",
            VhalType::FLOAT,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Vehicle Dynamics",
            "m/s",
            "Vehicle speed in meters per second.",
            0
        },
        {
            0x11600204,
            "PERF_ODOMETER",
            VhalType::FLOAT,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Vehicle Dynamics",
            "km",
            "Current odometer reading in kilometers.",
            0
        },
        {
            0x11400205,
            "PERF_STEERING_ANGLE",
            VhalType::FLOAT,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Vehicle Dynamics",
            "degrees",
            "Steering wheel angle in degrees. Positive is clockwise.",
            0
        },
        {
            0x11400208,
            "LATERAL_ACCELERATION",
            VhalType::FLOAT,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Vehicle Dynamics",
            "m/s^2",
            "Lateral acceleration in m/s^2. Positive is rightward.",
            0
        },
        {
            0x11400209,
            "LONGITUDINAL_ACCELERATION",
            VhalType::FLOAT,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Vehicle Dynamics",
            "m/s^2",
            "Longitudinal acceleration in m/s^2. Positive is forward.",
            0
        },
        {
            0x1140020A,
            "YAW_RATE",
            VhalType::FLOAT,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Vehicle Dynamics",
            "deg/s",
            "Yaw rate in degrees per second. Positive is clockwise.",
            0
        },
        {
            0x11510306,
            "WHEEL_TICK",
            VhalType::INT64_VEC,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Vehicle Dynamics",
            "ticks",
            "Wheel tick count for each wheel (FL, FR, RL, RR).",
            0
        },

        // Powertrain
        {
            0x11400305,
            "ENGINE_RPM",
            VhalType::FLOAT,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Powertrain",
            "rpm",
            "Engine speed in revolutions per minute (RPM).",
            0
        },
        {
            0x11400303,
            "ENGINE_OIL_TEMP",
            VhalType::FLOAT,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Powertrain",
            "celsius",
            "Engine oil temperature in Celsius.",
            0
        },
        {
            0x11400301,
            "ENGINE_COOLANT_TEMP",
            VhalType::FLOAT,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Powertrain",
            "celsius",
            "Engine coolant temperature in Celsius.",
            0
        },
        {
            0x11400400,
            "GEAR_SELECTION",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Powertrain",
            "enum",
            "Current selected transmission gear position (P, R, N, D).",
            0
        },
        {
            0x11400401,
            "CURRENT_GEAR",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Powertrain",
            "gear",
            "Actual current operational gear ratio of transmission.",
            0
        },
        {
            0x11200402,
            "PARKING_BRAKE_ON",
            VhalType::BOOLEAN,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Powertrain",
            "bool",
            "Whether parking brake is currently engaged.",
            0
        },
        {
            0x11600505,
            "FUEL_LEVEL",
            VhalType::FLOAT,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Powertrain",
            "milliliters",
            "Fuel remaining in fuel tank in milliliters.",
            0
        },
        {
            0x11200503,
            "FUEL_LEVEL_LOW",
            VhalType::BOOLEAN,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Powertrain",
            "bool",
            "Low fuel warning status.",
            0
        },
        {
            0x11600507,
            "EV_BATTERY_LEVEL",
            VhalType::FLOAT,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Powertrain",
            "watt_hours",
            "EV battery remaining charge level in watt-hours.",
            0
        },
        {
            0x1120050B,
            "EV_CHARGE_PORT_CONNECTED",
            VhalType::BOOLEAN,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Powertrain",
            "bool",
            "Whether EV charging cable is currently plugged in.",
            0
        },
        {
            0x1140050C,
            "EV_BATTERY_INSTANTANEOUS_CHARGE_RATE",
            VhalType::FLOAT,
            ChangeMode::CONTINUOUS,
            AccessMode::READ,
            "Powertrain",
            "milliwatts",
            "Instantaneous EV battery charge/discharge rate.",
            0
        },

        // HVAC
        {
            0x12400502,
            "HVAC_TEMPERATURE_CURRENT",
            VhalType::FLOAT,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "HVAC",
            "celsius",
            "Current measured cabin temperature in Celsius.",
            0
        },
        {
            0x12400503,
            "HVAC_TEMPERATURE_SET",
            VhalType::FLOAT,
            ChangeMode::ON_CHANGE,
            AccessMode::READ_WRITE,
            "HVAC",
            "celsius",
            "Target set temperature for HVAC zone in Celsius.",
            0
        },
        {
            0x12200504,
            "HVAC_DEFROSTER",
            VhalType::BOOLEAN,
            ChangeMode::ON_CHANGE,
            AccessMode::READ_WRITE,
            "HVAC",
            "bool",
            "Defroster on/off state for windshield/windows.",
            0
        },
        {
            0x12200505,
            "HVAC_AC_ON",
            VhalType::BOOLEAN,
            ChangeMode::ON_CHANGE,
            AccessMode::READ_WRITE,
            "HVAC",
            "bool",
            "A/C compressor activation status.",
            0
        },
        {
            0x12400500,
            "HVAC_FAN_SPEED",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ_WRITE,
            "HVAC",
            "level",
            "HVAC blower fan speed setting.",
            0
        },
        {
            0x12400501,
            "HVAC_FAN_DIRECTION",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ_WRITE,
            "HVAC",
            "direction",
            "HVAC air vent output direction mask.",
            0
        },
        {
            0x1220050A,
            "HVAC_AUTO_ON",
            VhalType::BOOLEAN,
            ChangeMode::ON_CHANGE,
            AccessMode::READ_WRITE,
            "HVAC",
            "bool",
            "HVAC automatic climate control mode enabled.",
            0
        },

        // Body & Cabin
        {
            0x16200B00,
            "DOOR_LOCK",
            VhalType::BOOLEAN,
            ChangeMode::ON_CHANGE,
            AccessMode::READ_WRITE,
            "Body & Cabin",
            "bool",
            "Door lock status (true = locked, false = unlocked).",
            0
        },
        {
            0x16400B01,
            "DOOR_POS",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Body & Cabin",
            "mm",
            "Door opening position in millimeters or percentage.",
            0
        },
        {
            0x16400B0B,
            "WINDOW_POS",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ_WRITE,
            "Body & Cabin",
            "mm",
            "Window position (0 = closed, max = open).",
            0
        },

        // Lighting & Controls
        {
            0x11400F03,
            "HAZARD_LIGHTS_SWITCH",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ_WRITE,
            "Lighting & Controls",
            "enum",
            "Hazard flasher warning switch position.",
            0
        },
        {
            0x11400F00,
            "HEADLIGHTS_STATE",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Lighting & Controls",
            "enum",
            "Current exterior headlight operating mode.",
            0
        },
        {
            0x11400F08,
            "TURN_SIGNAL_STATE",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Lighting & Controls",
            "enum",
            "Turn signal blinker state (NONE, RIGHT, LEFT).",
            0
        },
        {
            0x11400409,
            "IGNITION_STATE",
            VhalType::INT32,
            ChangeMode::ON_CHANGE,
            AccessMode::READ,
            "Lighting & Controls",
            "enum",
            "Vehicle ignition system state (LOCK, OFF, ACC, ON, START).",
            0
        }
    };

    mIdToIndex.clear();
    mNameToIndex.clear();
    for (size_t i = 0; i < mProperties.size(); ++i) {
        mIdToIndex[mProperties[i].id] = i;
        mNameToIndex[mProperties[i].name] = i;
    }
}

const std::vector<VhalProperty>& VhalCatalog::getAllProperties() const {
    return mProperties;
}

std::vector<std::string> VhalCatalog::getCategories() const {
    std::vector<std::string> cats;
    for (const auto& p : mProperties) {
        if (std::find(cats.begin(), cats.end(), p.category) == cats.end()) {
            cats.push_back(p.category);
        }
    }
    return cats;
}

std::vector<VhalProperty> VhalCatalog::getByCategory(const std::string& category) const {
    if (category.empty() || category == "All") return mProperties;
    std::vector<VhalProperty> res;
    for (const auto& p : mProperties) {
        if (p.category == category) {
            res.push_back(p);
        }
    }
    return res;
}

std::optional<VhalProperty> VhalCatalog::findById(uint32_t id) const {
    auto it = mIdToIndex.find(id);
    if (it != mIdToIndex.end()) {
        return mProperties[it->second];
    }
    return std::nullopt;
}

std::optional<VhalProperty> VhalCatalog::findByName(const std::string& name) const {
    auto it = mNameToIndex.find(name);
    if (it != mNameToIndex.end()) {
        return mProperties[it->second];
    }
    return std::nullopt;
}

std::optional<VhalProperty> VhalCatalog::findClosestMatch(const std::string& fuzzyName) const {
    if (fuzzyName.empty()) return std::nullopt;

    // Direct match
    auto direct = findByName(fuzzyName);
    if (direct) return direct;

    std::string cleanQuery = cleanName(fuzzyName);

    // Substring or cleaned comparison
    for (const auto& p : mProperties) {
        std::string cleanProp = cleanName(p.name);
        if (cleanProp == cleanQuery) return p;
        if (cleanProp.find(cleanQuery) != std::string::npos || cleanQuery.find(cleanProp) != std::string::npos) {
            return p;
        }
    }

    // Try common aliases
    if (cleanQuery.find("speed") != std::string::npos) return findByName("PERF_VEHICLE_SPEED");
    if (cleanQuery.find("rpm") != std::string::npos || cleanQuery.find("enginespeed") != std::string::npos) return findByName("ENGINE_RPM");
    if (cleanQuery.find("gear") != std::string::npos) return findByName("GEAR_SELECTION");
    if (cleanQuery.find("temp") != std::string::npos && cleanQuery.find("coolant") != std::string::npos) return findByName("ENGINE_COOLANT_TEMP");
    if (cleanQuery.find("temp") != std::string::npos && cleanQuery.find("oil") != std::string::npos) return findByName("ENGINE_OIL_TEMP");
    if (cleanQuery.find("fuel") != std::string::npos) return findByName("FUEL_LEVEL");
    if (cleanQuery.find("steer") != std::string::npos) return findByName("PERF_STEERING_ANGLE");
    if (cleanQuery.find("yaw") != std::string::npos) return findByName("YAW_RATE");
    if (cleanQuery.find("door") != std::string::npos) return findByName("DOOR_LOCK");

    return std::nullopt;
}

std::vector<VhalProperty> VhalCatalog::search(const std::string& query, const std::string& category) const {
    std::string qLower = toLower(query);
    std::vector<VhalProperty> res;

    for (const auto& p : mProperties) {
        if (!category.empty() && category != "All" && p.category != category) {
            continue;
        }
        if (qLower.empty()) {
            res.push_back(p);
            continue;
        }

        std::string nameLower = toLower(p.name);
        std::string descLower = toLower(p.description);
        std::string idStr = toLower(p.idHexStr());

        if (nameLower.find(qLower) != std::string::npos ||
            descLower.find(qLower) != std::string::npos ||
            idStr.find(qLower) != std::string::npos) {
            res.push_back(p);
        }
    }
    return res;
}

} // namespace can_vhal
