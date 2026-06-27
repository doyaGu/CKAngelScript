#include "ScriptSourcePaths.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>
#include <vector>

namespace ScriptSourcePaths {

namespace {

bool IsBuilderAbsolutePath(const std::string &path) {
    return !path.empty() &&
           (path[0] == '/' || path[0] == '\\' || path.find(':') != std::string::npos);
}

std::string MakeBuilderAbsolutePath(std::string path) {
    if (!IsBuilderAbsolutePath(path)) {
        std::error_code ec;
        std::filesystem::path currentPath = std::filesystem::current_path(ec);
        if (!ec) {
            path = (currentPath / path).generic_string();
        }
    }

    std::replace(path.begin(), path.end(), '\\', '/');

    size_t pos = 0;
    while ((pos = path.find("/./", pos)) != std::string::npos) {
        path.erase(pos + 1, 2);
    }

    while ((pos = path.find("/../")) != std::string::npos) {
        const size_t parent = path.rfind('/', pos - 1);
        if (parent == std::string::npos) {
            break;
        }
        path.erase(parent, pos + 3 - parent);
    }

    return path;
}

} // namespace

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

std::string ResolveFileIncludeName(const char *include, const char *from) {
    std::string includeName = include ? include : "";
    std::replace(includeName.begin(), includeName.end(), '\\', '/');

    if (!IsBuilderAbsolutePath(includeName)) {
        std::string base = from ? from : "";
        std::replace(base.begin(), base.end(), '\\', '/');
        const size_t slash = base.find_last_of('/');
        if (slash != std::string::npos) {
            includeName = base.substr(0, slash + 1) + includeName;
        }
    }

    return MakeBuilderAbsolutePath(includeName);
}

} // namespace ScriptSourcePaths
