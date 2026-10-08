#pragma once

#include "core/AutomotiveTypes.hpp"
#include <vector>
#include <string>
#include <functional>

namespace can_vhal {

class SignalPalette {
public:
    using OnAddSignalCallback = std::function<void(const CanMessage& msg, const CanSignal& sig)>;
    using OnAddMessageCallback = std::function<void(const CanMessage& msg)>;

    SignalPalette() = default;

    void render(
        const std::vector<CanMessage>& messages,
        OnAddSignalCallback onAddSignal,
        OnAddMessageCallback onAddMessage);

    void setFilter(const std::string& filter) { mFilterText = filter; }
    const std::string& getFilter() const { return mFilterText; }

    void triggerExpandAll() { mExpandCollapseState = 1; }
    void triggerCollapseAll() { mExpandCollapseState = -1; }

private:
    std::string mFilterText;
    int mExpandCollapseState = 0; // 0 = idle, 1 = expand all, -1 = collapse all
};

} // namespace can_vhal
