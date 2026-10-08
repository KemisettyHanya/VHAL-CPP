#include "DbcParser.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

namespace can_vhal {

namespace {

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

} // namespace

void DbcParser::clear() {
    mMessages.clear();
}

const CanMessage* DbcParser::findMessage(uint32_t id) const {
    for (const auto& msg : mMessages) {
        if (msg.id == id) return &msg;
    }
    return nullptr;
}

const CanMessage* DbcParser::findMessageByName(const std::string& name) const {
    for (const auto& msg : mMessages) {
        if (msg.name == name) return &msg;
    }
    return nullptr;
}

size_t DbcParser::totalSignalsCount() const {
    size_t count = 0;
    for (const auto& msg : mMessages) {
        count += msg.signals.size();
    }
    return count;
}

bool DbcParser::parseFile(const std::string& filePath, std::string& outError) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        outError = "Failed to open DBC file: " + filePath;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str(), outError);
}

bool DbcParser::parseString(const std::string& content, std::string& outError) {
    clear();

    std::istringstream stream(content);
    std::string line;
    CanMessage* currentMsg = nullptr;
    int lineNum = 0;

    while (std::getline(stream, line)) {
        ++lineNum;
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        if (trimmed.rfind("BO_ ", 0) == 0) {
            CanMessage msg;
            std::string err;
            if (parseBoLine(trimmed, msg, err)) {
                mMessages.push_back(msg);
                currentMsg = &mMessages.back();
            } else {
                std::cerr << "Warning (Line " << lineNum << "): " << err << std::endl;
            }
        } else if (trimmed.rfind("SG_ ", 0) == 0) {
            if (currentMsg) {
                CanSignal sig;
                std::string err;
                if (parseSgLine(trimmed, sig, err)) {
                    currentMsg->signals.push_back(sig);
                } else {
                    std::cerr << "Warning (Line " << lineNum << "): " << err << std::endl;
                }
            }
        } else if (trimmed.rfind("CM_ BO_ ", 0) == 0) {
            parseCmBoLine(trimmed);
        } else if (trimmed.rfind("CM_ SG_ ", 0) == 0) {
            parseCmSgLine(trimmed);
        } else if (trimmed.rfind("VAL_ ", 0) == 0) {
            parseValLine(trimmed);
        }
    }

    if (mMessages.empty()) {
        outError = "No CAN messages (BO_ records) found in DBC content.";
        return false;
    }

    return true;
}

bool DbcParser::parseBoLine(const std::string& line, CanMessage& outMsg, std::string& err) {
    // Format: BO_ <ID> <Name>: <DLC> <Sender>
    // e.g. BO_ 256 VehicleSpeedMsg: 8 VehicleECU
    static const std::regex boRegex(R"(^BO_\s+(\d+)\s+([A-Za-z0-9_]+)\s*:\s*(\d+)\s+([A-Za-z0-9_]+))");
    std::smatch match;

    if (std::regex_search(line, match, boRegex)) {
        uint32_t rawId = static_cast<uint32_t>(std::stoul(match[1].str()));
        // Check extended CAN bit in Vector DBC (bit 31 set)
        if (rawId & 0x80000000U) {
            outMsg.id = rawId & ~0x80000000U;
            outMsg.isExtended = true;
        } else {
            outMsg.id = rawId;
            outMsg.isExtended = false;
        }
        outMsg.name = match[2].str();
        outMsg.dlc = static_cast<uint32_t>(std::stoul(match[3].str()));
        outMsg.sender = match[4].str();
        return true;
    }

    err = "Invalid BO_ format: " + line;
    return false;
}

bool DbcParser::parseSgLine(const std::string& line, CanSignal& outSig, std::string& err) {
    // Format: SG_ <Name> [M|m\d+]? : <StartBit>|<Length>@<Endian><Sign> (<Scale>,<Offset>) [<Min>|<Max>] "<Unit>" <Receiver>
    // e.g. SG_ VehicleSpeed : 0|16@1+ (0.01,0) [0|300] "km/h" AndroidVHAL
    static const std::regex sgRegex(
        R"DBC(^SG_\s+([A-Za-z0-9_]+)(?:\s+[Mm\d]+)?\s*:\s*(\d+)\|(\d+)@([01])([+-])\s*\(\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*,\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*\)\s*\[\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*\|\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*\]\s*"([^"]*)"\s*([A-Za-z0-9_]+)?)DBC");

    std::smatch match;
    if (std::regex_search(line, match, sgRegex)) {
        outSig.name = match[1].str();
        outSig.startBit = static_cast<uint32_t>(std::stoul(match[2].str()));
        outSig.bitLength = static_cast<uint32_t>(std::stoul(match[3].str()));
        outSig.endianness = (match[4].str() == "1") ? Endianness::LittleEndian : Endianness::BigEndian;
        outSig.signType = (match[5].str() == "-") ? SignType::Signed : SignType::Unsigned;
        outSig.factor = std::stod(match[6].str());
        outSig.offset = std::stod(match[7].str());
        outSig.minVal = std::stod(match[8].str());
        outSig.maxVal = std::stod(match[9].str());
        outSig.unit = match[10].str();
        if (match.size() > 11) {
            outSig.receiver = match[11].str();
        }
        return true;
    }

    err = "Invalid SG_ format: " + line;
    return false;
}

bool DbcParser::parseCmBoLine(const std::string& line) {
    // Format: CM_ BO_ <ID> "Comment";
    static const std::regex cmBoRegex(R"DBC(^CM_\s+BO_\s+(\d+)\s+"([^"]*)"\s*;?)DBC");
    std::smatch match;
    if (std::regex_search(line, match, cmBoRegex)) {
        uint32_t rawId = static_cast<uint32_t>(std::stoul(match[1].str())) & ~0x80000000U;
        std::string comment = match[2].str();
        for (auto& msg : mMessages) {
            if (msg.id == rawId) {
                msg.comment = comment;
                return true;
            }
        }
    }
    return false;
}

bool DbcParser::parseCmSgLine(const std::string& line) {
    // Format: CM_ SG_ <ID> <SignalName> "Comment";
    static const std::regex cmSgRegex(R"DBC(^CM_\s+SG_\s+(\d+)\s+([A-Za-z0-9_]+)\s+"([^"]*)"\s*;?)DBC");
    std::smatch match;
    if (std::regex_search(line, match, cmSgRegex)) {
        uint32_t rawId = static_cast<uint32_t>(std::stoul(match[1].str())) & ~0x80000000U;
        std::string sigName = match[2].str();
        std::string comment = match[3].str();
        for (auto& msg : mMessages) {
            if (msg.id == rawId) {
                for (auto& sig : msg.signals) {
                    if (sig.name == sigName) {
                        sig.comment = comment;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool DbcParser::parseValLine(const std::string& line) {
    // Format: VAL_ <ID> <SignalName> <val1> "desc1" <val2> "desc2" ... ;
    static const std::regex valHeaderRegex(R"DBC(^VAL_\s+(\d+)\s+([A-Za-z0-9_]+)\s+(.+);?)DBC");
    std::smatch match;
    if (std::regex_search(line, match, valHeaderRegex)) {
        uint32_t rawId = static_cast<uint32_t>(std::stoul(match[1].str())) & ~0x80000000U;
        std::string sigName = match[2].str();
        std::string rest = match[3].str();

        static const std::regex pairRegex(R"DBC((-?\d+)\s+"([^"]*)")DBC");
        auto begin = std::sregex_iterator(rest.begin(), rest.end(), pairRegex);
        auto end = std::sregex_iterator();

        std::vector<ValueDescription> descs;
        for (std::sregex_iterator it = begin; it != end; ++it) {
            std::smatch pairMatch = *it;
            ValueDescription vd;
            vd.value = std::stoll(pairMatch[1].str());
            vd.description = pairMatch[2].str();
            descs.push_back(vd);
        }

        if (!descs.empty()) {
            for (auto& msg : mMessages) {
                if (msg.id == rawId) {
                    for (auto& sig : msg.signals) {
                        if (sig.name == sigName) {
                            sig.valueDescriptions = descs;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

} // namespace can_vhal
