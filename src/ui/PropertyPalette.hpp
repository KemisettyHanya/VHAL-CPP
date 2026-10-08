#pragma once

#include "core/AutomotiveTypes.hpp"
#include <vector>
#include <string>
#include <functional>

namespace can_vhal {

class PropertyPalette {
public:
    using OnAddPropertyCallback = std::function<void(const VhalProperty& prop)>;

    PropertyPalette() = default;

    void render(OnAddPropertyCallback onAddProperty);

    void setFilter(const std::string& filter) { mFilterText = filter; }
    const std::string& getFilter() const { return mFilterText; }

private:
    std::string mFilterText;
    std::string mSelectedCategory = "All";
};

} // namespace can_vhal

