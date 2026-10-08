#include "CppReverseParser.hpp"
#include "VhalCatalog.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>
#include <unordered_map>

namespace can_vhal {

namespace {

uint64_t generatePinId(uint32_t msgId, uint32_t propOrSigIdx, bool isOutput) {
    uint64_t pin = (static_cast<uint64_t>(msgId) << 32) | static_cast<uint64_t>(propOrSigIdx);
    if (isOutput) {
        pin |= (1ULL << 63);
    }
    return pin;
}

std::string toLower(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) {
        res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return res;
}

} // namespace

ReverseEngineeringResult CppReverseParser::parseFile(
    const std::string& filePath,
    const std::vector<CanMessage>& existingDbcMessages) {

    ReverseEngineeringResult result;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        result.success = false;
        result.errorMessage = "Failed to open legacy C++ file: " + filePath;
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str(), existingDbcMessages);
}

ReverseEngineeringResult CppReverseParser::parseString(
    const std::string& cppSource,
    const std::vector<CanMessage>& existingDbcMessages) {

    ReverseEngineeringResult result;
    result.success = true;

    // 0. Studio Reconstitution Metadata Check (Exact fidelity for studio-generated files)
    if (cppSource.find("CAN_VHAL_STUDIO_METADATA_START") != std::string::npos) {
        static uint64_t sMetaLinkCounter = 5000;
        std::istringstream stream(cppSource);
        std::string line;

        while (std::getline(stream, line)) {
            // Trim leading whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            std::string trimmed = line.substr(start);

            if (trimmed.rfind("// LINK:", 0) != 0) continue;

            std::istringstream lineStream(trimmed.substr(8)); // skip "// LINK:"
            std::string token;
            std::unordered_map<std::string, std::string> kv;
            while (lineStream >> token) {
                auto eq = token.find('=');
                if (eq != std::string::npos) {
                    kv[token.substr(0, eq)] = token.substr(eq + 1);
                }
            }

            if (!kv.count("CAN_ID") || !kv.count("SIGNAL") || !kv.count("PROP_ID")) {
                continue;
            }

            NodeLink link;
            link.id = ++sMetaLinkCounter;

            std::string idStr = kv["CAN_ID"];
            link.canMsgId = (idStr.rfind("0x", 0) == 0 || idStr.rfind("0X", 0) == 0)
                ? static_cast<uint32_t>(std::stoul(idStr, nullptr, 16))
                : static_cast<uint32_t>(std::stoul(idStr));
            link.canMsgName = kv.count("CAN_NAME") ? kv["CAN_NAME"] : ("Msg_0x" + idStr);
            link.signalName = kv["SIGNAL"];

            std::string propIdStr = kv["PROP_ID"];
            link.vhalPropertyId = (propIdStr.rfind("0x", 0) == 0 || propIdStr.rfind("0X", 0) == 0)
                ? static_cast<uint32_t>(std::stoul(propIdStr, nullptr, 16))
                : static_cast<uint32_t>(std::stoul(propIdStr));
            link.vhalPropertyName = kv.count("PROP_NAME") ? kv["PROP_NAME"] : ("Prop_0x" + propIdStr);

            if (kv.count("CONV")) link.transform.conversion = static_cast<UnitConversion>(std::stoi(kv["CONV"]));
            if (kv.count("SCALE")) link.transform.customScale = std::stod(kv["SCALE"]);
            if (kv.count("OFFSET")) link.transform.customOffset = std::stod(kv["OFFSET"]);
            if (kv.count("CLAMP")) link.transform.hasClamp = (std::stoi(kv["CLAMP"]) != 0);
            if (kv.count("MIN")) link.transform.clampMin = std::stof(kv["MIN"]);
            if (kv.count("MAX")) link.transform.clampMax = std::stof(kv["MAX"]);
            if (kv.count("AREA")) link.transform.areaId = std::stoi(kv["AREA"]);
            if (kv.count("EXPR")) link.transform.customExpression = kv["EXPR"];

            link.fromPinId = generatePinId(link.canMsgId, 1, true);
            link.toPinId = generatePinId(link.vhalPropertyId, 1, false);

            result.extractedLinks.push_back(link);

            // Reconstruct or match message from DBC
            const CanMessage* matched = nullptr;
            for (const auto& msg : existingDbcMessages) {
                if (msg.id == link.canMsgId) {
                    matched = &msg;
                    break;
                }
            }
            if (matched) {
                bool alreadyAdded = false;
                for (const auto& em : result.extractedMessages) {
                    if (em.id == matched->id) { alreadyAdded = true; break; }
                }
                if (!alreadyAdded) {
                    result.extractedMessages.push_back(*matched);
                }
            } else {
                CanMessage syntheticMsg;
                syntheticMsg.id = link.canMsgId;
                syntheticMsg.name = link.canMsgName;
                syntheticMsg.dlc = 8;
                CanSignal syntheticSig;
                syntheticSig.name = link.signalName;
                syntheticSig.startBit = 0;
                syntheticSig.bitLength = 16;
                syntheticSig.factor = 1.0;
                syntheticMsg.signals.push_back(syntheticSig);
                result.extractedMessages.push_back(syntheticMsg);
            }
        }

        if (!result.extractedLinks.empty()) {
            result.success = true;
            result.parsedCasesCount = static_cast<uint32_t>(result.extractedLinks.size());
            return result;
        }
    }

    // 1. Scan for constants (#define CAN_ID_... or constexpr)
    std::unordered_map<std::string, double> constantValues;
    std::unordered_map<std::string, uint32_t> canIdConstants;

    static const std::regex constFloatRegex(
        R"((?:const|constexpr)?\s+(?:float|double)\s+([A-Za-z0-9_]+)\s*=\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)f?\s*;)");
    for (std::sregex_iterator it(cppSource.begin(), cppSource.end(), constFloatRegex), end; it != end; ++it) {
        constantValues[(*it)[1].str()] = std::stod((*it)[2].str());
    }

    static const std::regex constIntRegex(
        R"((?:#define|(?:const|constexpr)?\s+(?:uint32_t|int|uint16_t))\s+([A-Za-z0-9_]+)\s*=?\s*(0x[0-9a-fA-F]+|\d+)\s*;?)");
    for (std::sregex_iterator it(cppSource.begin(), cppSource.end(), constIntRegex), end; it != end; ++it) {
        std::string name = (*it)[1].str();
        std::string valStr = (*it)[2].str();
        uint32_t val = (valStr.rfind("0x", 0) == 0 || valStr.rfind("0X", 0) == 0)
            ? static_cast<uint32_t>(std::stoul(valStr, nullptr, 16))
            : static_cast<uint32_t>(std::stoul(valStr));
        if (name.find("CAN_ID") != std::string::npos || name.find("ID") != std::string::npos || name.find("MSG") != std::string::npos) {
            canIdConstants[name] = val;
        }
    }

    // 2. Scan switch/case blocks
    static const std::regex caseRegex(
        R"(case\s+(0x[0-9a-fA-F]+|\d+|[A-Za-z0-9_]+)\s*:\s*([\s\S]*?)(?=(?:case\s+[0-9A-Za-z_]+|default\s*:|\}\s*\}|\z)))");

    bool foundCases = false;
    for (std::sregex_iterator it(cppSource.begin(), cppSource.end(), caseRegex), end; it != end; ++it) {
        foundCases = true;
        std::string idStr = (*it)[1].str();
        std::string blockContent = (*it)[2].str();

        uint32_t canId = 0;
        if (idStr.rfind("0x", 0) == 0 || idStr.rfind("0X", 0) == 0) {
            canId = static_cast<uint32_t>(std::stoul(idStr, nullptr, 16));
        } else if (canIdConstants.count(idStr)) {
            canId = canIdConstants[idStr];
        } else if (std::all_of(idStr.begin(), idStr.end(), ::isdigit)) {
            canId = static_cast<uint32_t>(std::stoul(idStr));
        }

        if (canId != 0) {
            processCaseBlock(canId, blockContent, existingDbcMessages, result);
            result.parsedCasesCount++;
        }
    }

    // 3. Scan adapter classes / decode functions
    if (!foundCases) {
        static const std::regex adapterRegex(
            R"(([A-Za-z0-9_]+Adapter)::decodeCanPayload\s*\([^)]*\)\s*\{([\s\S]*?)\n\})");
        std::smatch adapterMatch;
        if (std::regex_search(cppSource, adapterMatch, adapterRegex)) {
            std::string body = adapterMatch[2].str();
            uint32_t canId = 0x100;
            if (!canIdConstants.empty()) {
                canId = canIdConstants.begin()->second;
            }

            processCaseBlock(canId, body + "\n" + cppSource, existingDbcMessages, result);
            result.parsedCasesCount++;
        }
    }

    if (result.extractedLinks.empty() && result.extractedMessages.empty()) {
        result.success = false;
        result.errorMessage = "Could not identify any CAN dispatch or payload decode routines in file.";
    }

    return result;
}

void CppReverseParser::processCaseBlock(
    uint32_t canId,
    const std::string& caseContent,
    const std::vector<CanMessage>& existingDbcMessages,
    ReverseEngineeringResult& result) {

    // 1. Extract all target properties mentioned in this block
    std::vector<std::string> propertyNames;
    static const std::regex propEnumRegex(R"(VehicleProperty::([A-Za-z0-9_]+))");
    for (std::sregex_iterator it(caseContent.begin(), caseContent.end(), propEnumRegex), end; it != end; ++it) {
        propertyNames.push_back((*it)[1].str());
    }

    // Check hex ID pattern
    if (propertyNames.empty()) {
        static const std::regex propHexRegex(R"(prop\s*=\s*(0x[0-9a-fA-F]+))");
        for (std::sregex_iterator it(caseContent.begin(), caseContent.end(), propHexRegex), end; it != end; ++it) {
            uint32_t propId = static_cast<uint32_t>(std::stoul((*it)[1].str(), nullptr, 16));
            auto propOpt = VhalCatalog::instance().findById(propId);
            if (propOpt) {
                propertyNames.push_back(propOpt->name);
            }
        }
    }

    // Fallback keyword detection
    if (propertyNames.empty()) {
        if (caseContent.find("speed") != std::string::npos || caseContent.find("Speed") != std::string::npos) {
            propertyNames.push_back("PERF_VEHICLE_SPEED");
        } else if (caseContent.find("rpm") != std::string::npos || caseContent.find("RPM") != std::string::npos) {
            propertyNames.push_back("ENGINE_RPM");
        } else if (caseContent.find("coolant") != std::string::npos) {
            propertyNames.push_back("ENGINE_COOLANT_TEMP");
        } else if (caseContent.find("gear") != std::string::npos) {
            propertyNames.push_back("GEAR_SELECTION");
        } else if (caseContent.find("fuel") != std::string::npos) {
            propertyNames.push_back("FUEL_LEVEL");
        }
    }

    // Match DBC Message
    const CanMessage* matchedDbcMsg = nullptr;
    for (const auto& msg : existingDbcMessages) {
        if (msg.id == canId) {
            matchedDbcMsg = &msg;
            break;
        }
    }

    static uint64_t sLinkCounter = 1000;

    for (const auto& rawPropName : propertyNames) {
        auto vhalPropOpt = VhalCatalog::instance().findByName(rawPropName);
        if (!vhalPropOpt) {
            vhalPropOpt = VhalCatalog::instance().findClosestMatch(rawPropName);
        }
        if (!vhalPropOpt) continue;

        CanMessage currentMsg;
        CanSignal currentSig;

        if (matchedDbcMsg && !matchedDbcMsg->signals.empty()) {
            currentMsg = *matchedDbcMsg;

            // Smart signal matching by keyword
            std::string propLower = toLower(vhalPropOpt->name);
            const CanSignal* bestSig = nullptr;
            for (const auto& sig : matchedDbcMsg->signals) {
                std::string sigLower = toLower(sig.name);
                if (propLower.find("speed") != std::string::npos && sigLower.find("speed") != std::string::npos) {
                    bestSig = &sig; break;
                }
                if (propLower.find("rpm") != std::string::npos && (sigLower.find("rpm") != std::string::npos || sigLower.find("speed") != std::string::npos)) {
                    bestSig = &sig; break;
                }
                if (propLower.find("coolant") != std::string::npos && sigLower.find("coolant") != std::string::npos) {
                    bestSig = &sig; break;
                }
                if (propLower.find("temp") != std::string::npos && sigLower.find("temp") != std::string::npos) {
                    bestSig = &sig; break;
                }
                if (propLower.find("gear") != std::string::npos && sigLower.find("gear") != std::string::npos) {
                    bestSig = &sig; break;
                }
            }
            currentSig = bestSig ? *bestSig : matchedDbcMsg->signals.front();
        } else {
            // Reconstruct CAN message & signal
            currentMsg.id = canId;
            std::ostringstream ss;
            ss << "Msg_0x" << std::hex << std::uppercase << canId;
            currentMsg.name = ss.str();
            currentMsg.dlc = 8;
            currentMsg.sender = "ECU";

            currentSig.name = vhalPropOpt->name + "_Raw";
            currentSig.startBit = 0;
            currentSig.bitLength = 16;
            currentSig.endianness = Endianness::LittleEndian;
            currentSig.factor = 1.0;
            currentSig.offset = 0.0;
            currentSig.unit = vhalPropOpt->unit;

            currentMsg.signals.push_back(currentSig);
            result.extractedMessages.push_back(currentMsg);
        }

        // Transformations
        TransformConfig transform;

        // Detect conversion
        if (caseContent.find("/ 3.6") != std::string::npos ||
            caseContent.find("convertKmhToMps") != std::string::npos ||
            (currentSig.unit == "km/h" && vhalPropOpt->unit == "m/s")) {
            transform.conversion = UnitConversion::KMH_TO_MPS;
        } else if (caseContent.find("* 3.6") != std::string::npos) {
            transform.conversion = UnitConversion::MPS_TO_KMH;
        } else if (caseContent.find("+ 273.15") != std::string::npos) {
            transform.conversion = UnitConversion::CELSIUS_TO_KELVIN;
        } else if (caseContent.find("- 273.15") != std::string::npos) {
            transform.conversion = UnitConversion::KELVIN_TO_CELSIUS;
        }

        // Detect clamp
        static const std::regex clampRegex(
            R"(std::clamp\s*\([^,]+,\s*([-+]?[0-9]*\.?[0-9]+)f?,\s*([-+]?[0-9]*\.?[0-9]+)f?\))");
        std::smatch clampMatch;
        if (std::regex_search(caseContent, clampMatch, clampRegex)) {
            transform.hasClamp = true;
            transform.clampMin = std::stof(clampMatch[1].str());
            transform.clampMax = std::stof(clampMatch[2].str());
        }

        // Detect areaId
        static const std::regex areaRegex(R"(areaId\s*=\s*(0x[0-9a-fA-F]+|\d+))");
        std::smatch areaMatch;
        if (std::regex_search(caseContent, areaMatch, areaRegex)) {
            std::string aStr = areaMatch[1].str();
            transform.areaId = (aStr.rfind("0x", 0) == 0)
                ? static_cast<int32_t>(std::stol(aStr, nullptr, 16))
                : std::stoi(aStr);
        }

        // Create NodeLink
        NodeLink link;
        link.id = ++sLinkCounter;
        link.canMsgId = currentMsg.id;
        link.canMsgName = currentMsg.name;
        link.signalName = currentSig.name;
        link.vhalPropertyId = vhalPropOpt->id;
        link.vhalPropertyName = vhalPropOpt->name;
        link.fromPinId = generatePinId(currentMsg.id, 1, true);
        link.toPinId = generatePinId(vhalPropOpt->id, 1, false);
        link.transform = transform;

        result.extractedLinks.push_back(link);
        result.parsedPropertiesCount++;

        std::ostringstream log;
        log << "Parsed CAN ID 0x" << std::hex << canId
            << " -> Linked Signal '" << currentSig.name
            << "' to AOSP Property '" << vhalPropOpt->name
            << "' (" << unitConversionToString(transform.conversion) << ")";
        result.logMessages.push_back(log.str());
    }
}

} // namespace can_vhal
