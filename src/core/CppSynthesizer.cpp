#include "CppSynthesizer.hpp"
#include "VhalCatalog.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <set>

namespace can_vhal {

namespace {

std::string indent(int level) {
    return std::string(level * 4, ' ');
}

std::string formatFloat(double val) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4) << val << "f";
    std::string s = ss.str();
    // Trim trailing zeros after decimal if clean
    size_t dot = s.find('.');
    if (dot != std::string::npos) {
        size_t last = s.find_last_not_of("0f");
        if (last != std::string::npos && last >= dot) {
            if (s[last] == '.') {
                return s.substr(0, last) + ".0f";
            }
            return s.substr(0, last + 1) + "f";
        }
    }
    return s;
}

} // namespace

std::string CppSynthesizer::generateBitExtraction(
    const CanSignal& sig,
    const std::string& payloadVar,
    int indentLevel) {

    std::ostringstream ss;
    const std::string ind = indent(indentLevel);

    ss << ind << "// Signal: " << sig.name
       << " [StartBit: " << sig.startBit << ", Length: " << sig.bitLength
       << ", " << (sig.endianness == Endianness::LittleEndian ? "Intel" : "Motorola")
       << ", " << (sig.isSigned() ? "Signed" : "Unsigned") << "]\n";

    // Optimized standard byte-aligned cases
    if (sig.endianness == Endianness::LittleEndian) {
        if (sig.startBit % 8 == 0 && sig.bitLength == 8) {
            uint32_t byteIdx = sig.startBit / 8;
            ss << ind << "const uint8_t raw_" << sig.name << " = "
               << payloadVar << "[" << byteIdx << "];\n";
        } else if (sig.startBit % 8 == 0 && sig.bitLength == 16) {
            uint32_t b0 = sig.startBit / 8;
            uint32_t b1 = b0 + 1;
            ss << ind << "const uint16_t raw_" << sig.name << " = "
               << "static_cast<uint16_t>(" << payloadVar << "[" << b0 << "]) | "
               << "(static_cast<uint16_t>(" << payloadVar << "[" << b1 << "]) << 8);\n";
        } else if (sig.startBit % 8 == 0 && sig.bitLength == 32) {
            uint32_t b0 = sig.startBit / 8;
            ss << ind << "const uint32_t raw_" << sig.name << " = "
               << "static_cast<uint32_t>(" << payloadVar << "[" << b0 << "]) | "
               << "(static_cast<uint32_t>(" << payloadVar << "[" << b0 + 1 << "]) << 8) | "
               << "(static_cast<uint32_t>(" << payloadVar << "[" << b0 + 2 << "]) << 16) | "
               << "(static_cast<uint32_t>(" << payloadVar << "[" << b0 + 3 << "]) << 24);\n";
        } else {
            // General Little-Endian bit extraction
            uint32_t startByte = sig.startByte();
            uint32_t endByte = sig.endByte();
            uint32_t shift = sig.bitShiftInByte();
            uint64_t mask = sig.mask();

            if (startByte == endByte) {
                ss << ind << "const uint8_t raw_" << sig.name << " = ("
                   << payloadVar << "[" << startByte << "] >> " << shift << ") & 0x"
                   << std::hex << mask << std::dec << ";\n";
            } else {
                ss << ind << "uint64_t raw_" << sig.name << " = 0ULL;\n";
                ss << ind << "for (uint32_t i = 0; i < " << sig.bitLength << "; ++i) {\n";
                ss << ind << "    const uint32_t absBit = " << sig.startBit << " + i;\n";
                ss << ind << "    raw_" << sig.name << " |= (static_cast<uint64_t>(("
                   << payloadVar << "[absBit / 8] >> (absBit % 8)) & 1U) << i);\n";
                ss << ind << "}\n";
            }
        }
    } else {
        // Motorola Big-Endian
        if (sig.bitLength == 8 && sig.startBit % 8 == 7) {
            uint32_t byteIdx = sig.startBit / 8;
            ss << ind << "const uint8_t raw_" << sig.name << " = "
               << payloadVar << "[" << byteIdx << "];\n";
        } else if (sig.bitLength == 16 && sig.startBit % 8 == 7) {
            uint32_t b0 = sig.startBit / 8;
            uint32_t b1 = b0 + 1;
            ss << ind << "const uint16_t raw_" << sig.name << " = "
               << "(static_cast<uint16_t>(" << payloadVar << "[" << b0 << "]) << 8) | "
               << "static_cast<uint16_t>(" << payloadVar << "[" << b1 << "]);\n";
        } else {
            // General Motorola bit traversal
            ss << ind << "uint64_t raw_" << sig.name << " = 0ULL;\n";
            ss << ind << "{\n";
            ss << ind << "    uint32_t pos = " << sig.startBit << ";\n";
            ss << ind << "    for (uint32_t i = 0; i < " << sig.bitLength << "; ++i) {\n";
            ss << ind << "        const uint8_t bit = (" << payloadVar << "[pos / 8] >> (pos % 8)) & 1U;\n";
            ss << ind << "        raw_" << sig.name << " = (raw_" << sig.name << " << 1) | bit;\n";
            ss << ind << "        pos = (pos % 8 == 0) ? (pos + 15) : (pos - 1);\n";
            ss << ind << "    }\n";
            ss << ind << "}\n";
        }
    }

    // Signed conversion if required
    if (sig.isSigned()) {
        ss << ind << "int64_t signed_" << sig.name << " = static_cast<int64_t>(raw_" << sig.name << ");\n";
        ss << ind << "if (raw_" << sig.name << " & (1ULL << (" << (sig.bitLength - 1) << "))) {\n";
        ss << ind << "    signed_" << sig.name << " |= (~0ULL << " << sig.bitLength << ");\n";
        ss << ind << "}\n";
    }

    return ss.str();
}

std::string CppSynthesizer::synthesize(
    const std::vector<NodeLink>& links,
    const std::vector<CanMessage>& messages,
    const SynthesizerOptions& options) {

    std::ostringstream ss;

    // Header banner
    ss << "// " << std::string(76, '=') << "\n";
    ss << "// AUTO-GENERATED NATIVE CAN-TO-AOSP AIDL VHAL TRANSLATION LAYER\n";
    ss << "// Generated by: Native C++ CAN-to-AOSP VHAL Mapping Studio\n";
    ss << "// Standard: C++20 | Target: aidl::android::hardware::automotive::vehicle\n";
    if (options.includeMisraComments) {
        ss << "// Compliance: MISRA C++:2008 & Android Treble AIDL Guidelines\n";
    }
    ss << "// " << std::string(76, '=') << "\n\n";

    // Studio Reconstitution Metadata (Allows reopening this generated file to restore all visual nodes & connections)
    ss << "// CAN_VHAL_STUDIO_METADATA_START\n";
    for (const auto& l : links) {
        ss << "// LINK: CAN_ID=0x" << std::hex << l.canMsgId << std::dec
           << " CAN_NAME=" << l.canMsgName
           << " SIGNAL=" << l.signalName
           << " PROP_ID=0x" << std::hex << l.vhalPropertyId << std::dec
           << " PROP_NAME=" << l.vhalPropertyName
           << " CONV=" << static_cast<int>(l.transform.conversion)
           << " SCALE=" << l.transform.customScale
           << " OFFSET=" << l.transform.customOffset
           << " CLAMP=" << (l.transform.hasClamp ? 1 : 0)
           << " MIN=" << l.transform.clampMin
           << " MAX=" << l.transform.clampMax
           << " AREA=" << l.transform.areaId;
        if (!l.transform.customExpression.empty()) {
            ss << " EXPR=" << l.transform.customExpression;
        }
        ss << "\n";
    }
    ss << "// CAN_VHAL_STUDIO_METADATA_END\n\n";

    // Standard includes
    ss << "#include <cstdint>\n";
    ss << "#include <cstring>\n";
    ss << "#include <algorithm>\n";
    ss << "#include <vector>\n";
    ss << "#include <functional>\n";
    if (options.emitTimestamp) {
        ss << "#include <chrono>\n";
    }
    ss << "\n#if defined(__linux__)\n";
    ss << "#include <linux/can.h>\n";
    ss << "#else\n";
    ss << "// Portable CAN frame definition for non-Linux / Windows platforms\n";
    ss << "struct can_frame {\n";
    ss << "    uint32_t can_id;  // 32-bit CAN_ID + EFF/RTR/ERR flags\n";
    ss << "    uint8_t  can_dlc; // frame payload length in bytes (0 .. 8)\n";
    ss << "    uint8_t  __pad;\n";
    ss << "    uint8_t  __res0;\n";
    ss << "    uint8_t  len8_dlc;\n";
    ss << "    alignas(8) uint8_t data[8];\n";
    ss << "};\n";
    ss << "#endif\n\n";

    // Android Automotive AIDL includes
    ss << "#include <aidl/android/hardware/automotive/vehicle/VehiclePropValue.h>\n";
    ss << "#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>\n";
    ss << "#include <aidl/android/hardware/automotive/vehicle/VehicleAreaWheel.h>\n";
    ss << "#include <aidl/android/hardware/automotive/vehicle/IVehicle.h>\n\n";

    // Namespace
    ss << "namespace " << options.namespaceName << " {\n\n";

    // Class wrapper
    if (options.generateClassWrapper) {
        ss << "class " << options.className << " {\n";
        ss << "public:\n";
        ss << "    using PropertyCallback = std::function<void(const std::vector<VehiclePropValue>&)>;\n\n";
        ss << "    explicit " << options.className << "(PropertyCallback callback = nullptr)\n";
        ss << "        : mCallback(std::move(callback)) {}\n\n";
        ss << "    void setCallback(PropertyCallback callback) {\n";
        ss << "        mCallback = std::move(callback);\n";
        ss << "    }\n\n";
        ss << "    void onCanFrameReceived(const struct can_frame& frame) {\n";
    } else {
        ss << "void onCanFrameReceived(const struct can_frame& frame, "
           << "std::function<void(const std::vector<VehiclePropValue>&)> mCallback) {\n";
    }

    int baseIndent = options.generateClassWrapper ? 2 : 1;

    if (options.emitTimestamp) {
        ss << indent(baseIndent) << "const int64_t timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(\n";
        ss << indent(baseIndent) << "    std::chrono::system_clock::now().time_since_epoch()).count();\n\n";
    }

    // Group links by CAN Message ID
    std::map<uint32_t, std::vector<NodeLink>> linksByMsgId;
    for (const auto& l : links) {
        linksByMsgId[l.canMsgId].push_back(l);
    }

    ss << indent(baseIndent) << "switch (frame.can_id) {\n";

    for (const auto& [canId, msgLinks] : linksByMsgId) {
        std::string msgName = msgLinks.front().canMsgName;
        ss << indent(baseIndent + 1) << "case 0x" << std::hex << std::uppercase << canId << std::dec << ": { // "
           << (msgName.empty() ? "CAN Message" : msgName) << "\n";

        // Collect all dispatched properties in this frame
        ss << indent(baseIndent + 2) << "std::vector<VehiclePropValue> dispatchedProps;\n";
        ss << indent(baseIndent + 2) << "dispatchedProps.reserve(" << msgLinks.size() << ");\n\n";

        // Find CAN Message in catalog
        const CanMessage* matchedMsg = nullptr;
        for (const auto& m : messages) {
            if (m.id == canId) {
                matchedMsg = &m;
                break;
            }
        }

        for (const auto& link : msgLinks) {
            // Find the signal
            const CanSignal* sig = nullptr;
            if (matchedMsg) {
                sig = matchedMsg->findSignal(link.signalName);
            }

            // Fallback signal if not found in DBC
            CanSignal fallbackSig;
            if (!sig) {
                fallbackSig.name = link.signalName;
                fallbackSig.startBit = 0;
                fallbackSig.bitLength = 16;
                fallbackSig.endianness = Endianness::LittleEndian;
                fallbackSig.factor = 1.0;
                fallbackSig.offset = 0.0;
                sig = &fallbackSig;
            }

            // 1. Bit extraction
            ss << generateBitExtraction(*sig, "frame.data", baseIndent + 2);

            // 2. Physical scaling
            std::string rawRef = sig->isSigned() ? ("signed_" + sig->name) : ("raw_" + sig->name);
            ss << indent(baseIndent + 2) << "float val_" << sig->name << " = static_cast<float>("
               << rawRef << ") * " << formatFloat(sig->factor)
               << " + " << formatFloat(sig->offset) << ";\n";

            // 3. Transformation / Unit conversion
            if (link.transform.conversion == UnitConversion::KMH_TO_MPS) {
                ss << indent(baseIndent + 2) << "// Unit Conversion: km/h -> m/s (/ 3.6)\n";
                ss << indent(baseIndent + 2) << "val_" << sig->name << " = val_" << sig->name << " / 3.6f;\n";
            } else if (link.transform.conversion == UnitConversion::MPS_TO_KMH) {
                ss << indent(baseIndent + 2) << "// Unit Conversion: m/s -> km/h (* 3.6)\n";
                ss << indent(baseIndent + 2) << "val_" << sig->name << " = val_" << sig->name << " * 3.6f;\n";
            } else if (link.transform.conversion == UnitConversion::CELSIUS_TO_KELVIN) {
                ss << indent(baseIndent + 2) << "// Unit Conversion: Celsius -> Kelvin (+ 273.15)\n";
                ss << indent(baseIndent + 2) << "val_" << sig->name << " = val_" << sig->name << " + 273.15f;\n";
            } else if (link.transform.conversion == UnitConversion::KELVIN_TO_CELSIUS) {
                ss << indent(baseIndent + 2) << "// Unit Conversion: Kelvin -> Celsius (- 273.15)\n";
                ss << indent(baseIndent + 2) << "val_" << sig->name << " = val_" << sig->name << " - 273.15f;\n";
            } else if (link.transform.conversion == UnitConversion::RPM_TO_RADS) {
                ss << indent(baseIndent + 2) << "// Unit Conversion: RPM -> rad/s\n";
                ss << indent(baseIndent + 2) << "val_" << sig->name << " = val_" << sig->name << " * 0.104719755f;\n";
            } else if (link.transform.conversion == UnitConversion::CUSTOM_EXPR && !link.transform.customExpression.empty()) {
                ss << indent(baseIndent + 2) << "// Custom Transformation: " << link.transform.customExpression << "\n";
                ss << indent(baseIndent + 2) << "val_" << sig->name << " = static_cast<float>("
                   << link.transform.customExpression << ");\n";
            } else if (link.transform.customScale != 1.0 || link.transform.customOffset != 0.0) {
                ss << indent(baseIndent + 2) << "val_" << sig->name << " = val_" << sig->name << " * "
                   << formatFloat(link.transform.customScale) << " + "
                   << formatFloat(link.transform.customOffset) << ";\n";
            }

            // 4. Clamping
            if (link.transform.hasClamp) {
                ss << indent(baseIndent + 2) << "val_" << sig->name << " = std::clamp(val_" << sig->name << ", "
                   << formatFloat(link.transform.clampMin) << ", "
                   << formatFloat(link.transform.clampMax) << ");\n";
            }

            // 5. Pack into VehiclePropValue
            auto propOpt = VhalCatalog::instance().findByName(link.vhalPropertyName);
            VhalType pType = propOpt ? propOpt->type : VhalType::FLOAT;

            std::string propVar = "prop_" + link.vhalPropertyName;
            ss << "\n" << indent(baseIndent + 2) << "VehiclePropValue " << propVar << "{};\n";
            ss << indent(baseIndent + 2) << propVar << ".prop = static_cast<int32_t>(VehicleProperty::"
               << link.vhalPropertyName << ");\n";
            ss << indent(baseIndent + 2) << propVar << ".areaId = " << link.transform.areaId << ";\n";
            if (options.emitTimestamp) {
                ss << indent(baseIndent + 2) << propVar << ".timestamp = timestampNs;\n";
            }

            if (pType == VhalType::FLOAT) {
                ss << indent(baseIndent + 2) << propVar << ".value.floatValues = { val_" << sig->name << " };\n";
            } else if (pType == VhalType::INT32) {
                ss << indent(baseIndent + 2) << propVar << ".value.int32Values = { static_cast<int32_t>(val_" << sig->name << ") };\n";
            } else if (pType == VhalType::BOOLEAN) {
                ss << indent(baseIndent + 2) << propVar << ".value.int32Values = { val_" << sig->name << " > 0.5f ? 1 : 0 };\n";
            } else if (pType == VhalType::INT64 || pType == VhalType::INT64_VEC) {
                ss << indent(baseIndent + 2) << propVar << ".value.int64Values = { static_cast<int64_t>(val_" << sig->name << ") };\n";
            } else {
                ss << indent(baseIndent + 2) << propVar << ".value.floatValues = { val_" << sig->name << " };\n";
            }

            ss << indent(baseIndent + 2) << "dispatchedProps.push_back(std::move(" << propVar << "));\n\n";
        }

        // Callback dispatch
        if (options.emitCallbackDispatch) {
            ss << indent(baseIndent + 2) << "if (mCallback && !dispatchedProps.empty()) {\n";
            ss << indent(baseIndent + 3) << "mCallback(dispatchedProps);\n";
            ss << indent(baseIndent + 2) << "}\n";
        }

        ss << indent(baseIndent + 2) << "break;\n";
        ss << indent(baseIndent + 1) << "}\n";
    }

    // Default case
    ss << indent(baseIndent + 1) << "default:\n";
    ss << indent(baseIndent + 2) << "// Unmapped CAN frame\n";
    ss << indent(baseIndent + 2) << "break;\n";
    ss << indent(baseIndent) << "}\n";

    if (options.generateClassWrapper) {
        ss << "    }\n\n";
        ss << "private:\n";
        ss << "    PropertyCallback mCallback;\n";
        ss << "};\n\n";
    } else {
        ss << "}\n\n";
    }

    ss << "} // namespace " << options.namespaceName << "\n";

    return ss.str();
}

bool CppSynthesizer::saveToFile(
    const std::string& filePath,
    const std::vector<NodeLink>& links,
    const std::vector<CanMessage>& messages,
    const SynthesizerOptions& options) {

    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    std::string code = synthesize(links, messages, options);
    file << code;
    return true;
}

} // namespace can_vhal

