#pragma once

#include "AutomotiveTypes.hpp"
#include <string>
#include <vector>

namespace can_vhal {

struct ReverseEngineeringResult {
    std::vector<CanMessage> extractedMessages;
    std::vector<NodeLink> extractedLinks;
    size_t parsedCasesCount = 0;
    size_t parsedPropertiesCount = 0;
    std::vector<std::string> logMessages;
    bool success = false;
    std::string errorMessage;
};

class CppReverseParser {
public:
    CppReverseParser() = default;

    ReverseEngineeringResult parseString(
        const std::string& cppSource,
        const std::vector<CanMessage>& existingDbcMessages = {});

    ReverseEngineeringResult parseFile(
        const std::string& filePath,
        const std::vector<CanMessage>& existingDbcMessages = {});

private:
    void processCaseBlock(
        uint32_t canId,
        const std::string& caseContent,
        const std::vector<CanMessage>& existingDbcMessages,
        ReverseEngineeringResult& result);
};

} // namespace can_vhal

