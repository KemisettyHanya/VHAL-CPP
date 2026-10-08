#pragma once

#include <string>
#include <filesystem>

namespace can_vhal {

inline std::string resolvePath(const std::string& relPath) {
    namespace fs = std::filesystem;
    std::error_code ec;

    // 1. Check direct relative path (e.g. running from repo root)
    if (fs::exists(relPath, ec)) {
        return relPath;
    }

    // 2. Check parent directory (e.g. running from build/ on Linux / single-config)
    fs::path p1 = fs::path("..") / relPath;
    if (fs::exists(p1, ec)) {
        return p1.string();
    }

    // 3. Check grandparent directory (e.g. running from build/Release or build/Debug on Windows MSVC)
    fs::path p2 = fs::path("..") / ".." / relPath;
    if (fs::exists(p2, ec)) {
        return p2.string();
    }

    // 4. Fallback to original
    return relPath;
}

} // namespace can_vhal

