#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace can_vhal {

enum class Endianness {
    LittleEndian = 1, // Intel (@1+)
    BigEndian = 0     // Motorola (@0+)
};

enum class SignType {
    Unsigned = 0, // '+'
    Signed = 1    // '-'
};

enum class VhalType {
    INT32,
    INT64,
    BOOLEAN,
    FLOAT,
    FLOAT_VEC,
    INT32_VEC,
    INT64_VEC,
    STRING,
    BYTES
};

inline const char* vhalTypeToString(VhalType t) {
    switch (t) {
        case VhalType::INT32: return "INT32";
        case VhalType::INT64: return "INT64";
        case VhalType::BOOLEAN: return "BOOLEAN";
        case VhalType::FLOAT: return "FLOAT";
        case VhalType::FLOAT_VEC: return "FLOAT_VEC";
        case VhalType::INT32_VEC: return "INT32_VEC";
        case VhalType::INT64_VEC: return "INT64_VEC";
        case VhalType::STRING: return "STRING";
        case VhalType::BYTES: return "BYTES";
        default: return "UNKNOWN";
    }
}

enum class ChangeMode {
    STATIC,
    ON_CHANGE,
    CONTINUOUS
};

inline const char* changeModeToString(ChangeMode m) {
    switch (m) {
        case ChangeMode::STATIC: return "STATIC";
        case ChangeMode::ON_CHANGE: return "ON_CHANGE";
        case ChangeMode::CONTINUOUS: return "CONTINUOUS";
        default: return "UNKNOWN";
    }
}

enum class AccessMode {
    READ,
    WRITE,
    READ_WRITE
};

enum class UnitConversion {
    NONE,
    KMH_TO_MPS,       // value / 3.6f
    MPS_TO_KMH,       // value * 3.6f
    CELSIUS_TO_KELVIN,// value + 273.15f
    KELVIN_TO_CELSIUS,// value - 273.15f
    RPM_TO_RADS,      // value * 0.104719755f
    CUSTOM_EXPR       // custom arithmetic expression
};

inline const char* unitConversionToString(UnitConversion uc) {
    switch (uc) {
        case UnitConversion::NONE: return "None (1:1)";
        case UnitConversion::KMH_TO_MPS: return "km/h -> m/s (/ 3.6)";
        case UnitConversion::MPS_TO_KMH: return "m/s -> km/h (* 3.6)";
        case UnitConversion::CELSIUS_TO_KELVIN: return "Celsius -> Kelvin (+ 273.15)";
        case UnitConversion::KELVIN_TO_CELSIUS: return "Kelvin -> Celsius (- 273.15)";
        case UnitConversion::RPM_TO_RADS: return "RPM -> rad/s (* 0.1047)";
        case UnitConversion::CUSTOM_EXPR: return "Custom Formula";
        default: return "Unknown";
    }
}

struct ValueDescription {
    int64_t value = 0;
    std::string description;
};

struct CanSignal {
    std::string name;
    uint32_t startBit = 0;
    uint32_t bitLength = 0;
    Endianness endianness = Endianness::LittleEndian;
    SignType signType = SignType::Unsigned;
    double factor = 1.0;
    double offset = 0.0;
    double minVal = 0.0;
    double maxVal = 0.0;
    std::string unit;
    std::string receiver;
    std::string comment;
    std::vector<ValueDescription> valueDescriptions;

    [[nodiscard]] bool isSigned() const {
        return signType == SignType::Signed;
    }

    [[nodiscard]] uint32_t startByte() const {
        return startBit / 8;
    }

    [[nodiscard]] uint32_t endByte() const {
        if (endianness == Endianness::LittleEndian) {
            return (startBit + bitLength - 1) / 8;
        } else {
            // Motorola: startBit is MSB.
            // Move bitLength - 1 steps through CAN matrix
            uint32_t pos = startBit;
            for (uint32_t i = 1; i < bitLength; ++i) {
                uint32_t bitInByte = pos % 8;
                if (bitInByte == 0) pos += 15; else pos -= 1;
            }
            return pos / 8;
        }
    }

    [[nodiscard]] uint32_t bitShiftInByte() const {
        return startBit % 8;
    }

    [[nodiscard]] uint64_t mask() const {
        if (bitLength >= 64) return ~0ULL;
        return (1ULL << bitLength) - 1ULL;
    }

    [[nodiscard]] uint64_t decodeRaw(const uint8_t* payload, size_t len) const {
        if (!payload || len == 0 || bitLength == 0) return 0;
        uint64_t raw = 0;

        if (endianness == Endianness::LittleEndian) {
            for (uint32_t i = 0; i < bitLength; ++i) {
                uint32_t absBit = startBit + i;
                uint32_t byteIdx = absBit / 8;
                uint32_t bitIdx = absBit % 8;
                if (byteIdx < len) {
                    uint64_t bit = (payload[byteIdx] >> bitIdx) & 1ULL;
                    raw |= (bit << i);
                }
            }
        } else {
            // BigEndian (Motorola): startBit is MSB
            uint32_t pos = startBit;
            for (uint32_t i = 0; i < bitLength; ++i) {
                uint32_t byteIdx = pos / 8;
                uint32_t bitIdx = pos % 8;
                if (byteIdx < len) {
                    uint64_t bit = (payload[byteIdx] >> bitIdx) & 1ULL;
                    raw = (raw << 1) | bit;
                }
                if (bitIdx == 0) pos += 15; else pos -= 1;
            }
        }
        return raw;
    }

    [[nodiscard]] double decodePhysical(const uint8_t* payload, size_t len) const {
        uint64_t raw = decodeRaw(payload, len);
        if (isSigned() && bitLength > 0 && bitLength < 64) {
            if (raw & (1ULL << (bitLength - 1))) {
                int64_t signedVal = static_cast<int64_t>(raw | (~0ULL << bitLength));
                return static_cast<double>(signedVal) * factor + offset;
            }
        }
        return static_cast<double>(raw) * factor + offset;
    }
};

struct CanMessage {
    uint32_t id = 0;
    std::string name;
    uint32_t dlc = 8;
    std::string sender;
    std::vector<CanSignal> signals;
    std::string comment;
    bool isExtended = false;

    [[nodiscard]] const CanSignal* findSignal(const std::string& sigName) const {
        for (const auto& sig : signals) {
            if (sig.name == sigName) return &sig;
        }
        return nullptr;
    }

    [[nodiscard]] CanSignal* findSignal(const std::string& sigName) {
        for (auto& sig : signals) {
            if (sig.name == sigName) return &sig;
        }
        return nullptr;
    }

    [[nodiscard]] std::string idHexStr() const {
        std::ostringstream ss;
        ss << "0x" << std::uppercase << std::hex << id;
        return ss.str();
    }
};

struct VhalProperty {
    uint32_t id = 0;
    std::string name;
    VhalType type = VhalType::FLOAT;
    ChangeMode changeMode = ChangeMode::CONTINUOUS;
    AccessMode access = AccessMode::READ;
    std::string category;
    std::string unit;
    std::string description;
    int32_t defaultAreaId = 0;

    [[nodiscard]] std::string idHexStr() const {
        std::ostringstream ss;
        ss << "0x" << std::uppercase << std::hex << id;
        return ss.str();
    }
};

struct TransformConfig {
    UnitConversion conversion = UnitConversion::NONE;
    std::string customExpression;
    double customScale = 1.0;
    double customOffset = 0.0;
    int32_t areaId = 0;
    float clampMin = 0.0f;
    float clampMax = 0.0f;
    bool hasClamp = false;

    [[nodiscard]] float apply(float rawPhysical) const {
        float val = rawPhysical;
        switch (conversion) {
            case UnitConversion::NONE:
                val = static_cast<float>(val * customScale + customOffset);
                break;
            case UnitConversion::KMH_TO_MPS:
                val = val / 3.6f;
                break;
            case UnitConversion::MPS_TO_KMH:
                val = val * 3.6f;
                break;
            case UnitConversion::CELSIUS_TO_KELVIN:
                val = val + 273.15f;
                break;
            case UnitConversion::KELVIN_TO_CELSIUS:
                val = val - 273.15f;
                break;
            case UnitConversion::RPM_TO_RADS:
                val = val * 0.104719755f;
                break;
            case UnitConversion::CUSTOM_EXPR:
                val = static_cast<float>(val * customScale + customOffset);
                break;
        }
        if (hasClamp) {
            val = std::clamp(val, clampMin, clampMax);
        }
        return val;
    }
};

struct NodeLink {
    uint64_t id = 0;
    uint32_t canMsgId = 0;
    std::string canMsgName;
    std::string signalName;
    uint32_t vhalPropertyId = 0;
    std::string vhalPropertyName;
    uint64_t fromPinId = 0; // CAN output pin
    uint64_t toPinId = 0;   // VHAL input pin
    TransformConfig transform;
};

enum class CanvasNodeType {
    CAN_SIGNAL,
    VHAL_PROPERTY
};

struct CanvasNode {
    uint64_t id = 0;
    CanvasNodeType type = CanvasNodeType::CAN_SIGNAL;
    
    // CAN payload if type == CAN_SIGNAL
    uint32_t canMsgId = 0;
    std::string canMsgName;
    CanSignal signal;
    uint64_t outputPinId = 0;

    // VHAL payload if type == VHAL_PROPERTY
    VhalProperty property;
    uint64_t inputPinId = 0;

    // Positioning
    float posX = 0.0f;
    float posY = 0.0f;
};

} // namespace can_vhal
