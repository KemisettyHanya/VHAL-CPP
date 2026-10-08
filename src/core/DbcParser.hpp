#pragma once

#include "AutomotiveTypes.hpp"
#include <string>
#include <vector>
#include <optional>

namespace can_vhal {

class DbcParser {
public:
    DbcParser() = default;

    bool parseString(const std::string& content, std::string& outError);
    bool parseFile(const std::string& filePath, std::string& outError);

    const std::vector<CanMessage>& getMessages() const { return mMessages; }
    std::vector<CanMessage>& getMessages() { return mMessages; }

    const CanMessage* findMessage(uint32_t id) const;
    const CanMessage* findMessageByName(const std::string& name) const;
    size_t totalSignalsCount() const;

    void clear();

private:
    std::vector<CanMessage> mMessages;

    bool parseBoLine(const std::string& line, CanMessage& outMsg, std::string& err);
    bool parseSgLine(const std::string& line, CanSignal& outSig, std::string& err);
    bool parseCmBoLine(const std::string& line);
    bool parseCmSgLine(const std::string& line);
    bool parseValLine(const std::string& line);
};

} // namespace can_vhal

