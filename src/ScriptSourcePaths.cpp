#include "ScriptSourcePaths.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace ScriptSourcePaths {

std::string NormalizeSectionName(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');

    std::string prefix;
    size_t start = 0;
    if (path.size() >= 2 && path[1] == ':') {
        prefix = path.substr(0, 2);
        start = 2;
        if (start < path.size() && path[start] == '/') {
            ++start;
        }
    } else if (!path.empty() && path[0] == '/') {
        prefix = "/";
        start = 1;
    }

    std::vector<std::string> parts;
    while (start <= path.size()) {
        const size_t end = path.find('/', start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (part.empty() || part == ".") {
            // skip
        } else if (part == "..") {
            if (!parts.empty()) {
                parts.pop_back();
            }
        } else {
            parts.push_back(part);
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    std::string normalized = prefix;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (!normalized.empty() && normalized.back() != '/') {
            normalized.push_back('/');
        }
        normalized += parts[i];
    }
    if (normalized.empty()) {
        normalized = ".";
    }

#ifdef _WIN32
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
#endif
    return normalized;
}

std::string ResolveSnapshotIncludeName(const char *include, const char *from) {
    std::string includeName = include ? include : "";
    std::replace(includeName.begin(), includeName.end(), '\\', '/');
    const bool includeAbsolute = !includeName.empty() &&
                                 (includeName[0] == '/' ||
                                  (includeName.size() >= 2 && includeName[1] == ':'));
    if (includeAbsolute) {
        return NormalizeSectionName(includeName);
    }

    std::string base = from ? from : "";
    std::replace(base.begin(), base.end(), '\\', '/');
    const size_t slash = base.find_last_of('/');
    if (slash != std::string::npos) {
        includeName = base.substr(0, slash + 1) + includeName;
    }
    return NormalizeSectionName(includeName);
}

} // namespace ScriptSourcePaths
