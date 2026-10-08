#pragma once

#include "AutomotiveTypes.hpp"
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>

namespace can_vhal {

class VhalCatalog {
public:
    static VhalCatalog& instance();

    const std::vector<VhalProperty>& getAllProperties() const;
    std::vector<std::string> getCategories() const;
    std::vector<VhalProperty> getByCategory(const std::string& category) const;
    std::optional<VhalProperty> findById(uint32_t id) const;
    std::optional<VhalProperty> findByName(const std::string& name) const;
    std::optional<VhalProperty> findClosestMatch(const std::string& fuzzyName) const;
    std::vector<VhalProperty> search(const std::string& query, const std::string& category = "") const;

private:
    VhalCatalog();
    void initStandardCatalog();

    std::vector<VhalProperty> mProperties;
    std::unordered_map<uint32_t, size_t> mIdToIndex;
    std::unordered_map<std::string, size_t> mNameToIndex;
};

} // namespace can_vhal

