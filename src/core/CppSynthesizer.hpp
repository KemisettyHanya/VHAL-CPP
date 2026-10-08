#pragma once

#include "AutomotiveTypes.hpp"
#include <string>
#include <vector>

namespace can_vhal {

struct SynthesizerOptions {
    std::string namespaceName = "aidl::android::hardware::automotive::vehicle";
    std::string className = "CanToVhalTranslator";
    bool generateClassWrapper = true;
    bool emitTimestamp = true;
    bool emitCallbackDispatch = true;
    bool includeMisraComments = true;
};

class CppSynthesizer {
public:
    CppSynthesizer() = default;

    std::string synthesize(
        const std::vector<NodeLink>& links,
        const std::vector<CanMessage>& messages,
        const SynthesizerOptions& options = {});

    bool saveToFile(
        const std::string& filePath,
        const std::vector<NodeLink>& links,
        const std::vector<CanMessage>& messages,
        const SynthesizerOptions& options = {});

private:
    std::string generateBitExtraction(const CanSignal& sig, const std::string& payloadVar, int indentLevel);
};

} // namespace can_vhal

