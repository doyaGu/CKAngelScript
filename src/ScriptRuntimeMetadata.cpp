#include "ScriptRuntimeMetadata.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include <fmt/format.h>

namespace {

struct KeyValue {
    std::string Key;
    std::string Value;
};

struct MetadataBlock {
    std::string Kind;
    std::vector<KeyValue> Values;
    std::string AttachedClass;
};

bool IsScalarKey(const std::string &key) {
    return key == "id" || key == "name" || key == "version" || key == "class" ||
           key == "entry" || key == "enabled" || key == "order" || key == "reload" ||
           key == "description" || key == "author" || key == "category" || key == "host";
}

bool IsListKey(const std::string &key) {
    return key == "files" || key == "depends" || key == "required" || key == "optional" ||
           key == "before" || key == "after" || key == "tags";
}

void SetMetadata(std::vector<ScriptRuntimeMetadataEntry> &metadata,
                 const std::string &key,
                 const std::string &value) {
    const std::string lowered = ScriptRuntimeMetadata::ToLower(ScriptRuntimeMetadata::Trim(key));
    if (lowered.empty()) {
        return;
    }
    for (ScriptRuntimeMetadataEntry &entry : metadata) {
        if (entry.Key == lowered) {
            entry.Value = value;
            return;
        }
    }
    metadata.push_back({lowered, value});
}

void AppendUnique(std::vector<std::string> &values, const std::string &value) {
    std::string item = ScriptRuntimeMetadata::Trim(ScriptRuntimeMetadata::StripQuotes(value));
    if (item.empty()) {
        return;
    }
    if (std::find(values.begin(), values.end(), item) == values.end()) {
        values.push_back(std::move(item));
    }
}

void AppendList(std::vector<std::string> &values, const std::string &text) {
    for (const std::string &item : ScriptRuntimeMetadata::SplitList(text)) {
        AppendUnique(values, item);
    }
}

void AppendDependencies(std::vector<ScriptRuntimeDependency> &dependencies,
                        const std::string &text,
                        std::string &error) {
    for (const std::string &item : ScriptRuntimeMetadata::SplitList(text)) {
        ScriptRuntimeDependency dependency;
        std::string parseError;
        if (!ScriptRuntimeMetadata::ParseDependencySpec(item, dependency, parseError)) {
            error += parseError + "\n";
            continue;
        }
        const auto it = std::find_if(dependencies.begin(), dependencies.end(), [&](const ScriptRuntimeDependency &existing) {
            return existing.Id == dependency.Id && existing.Op == dependency.Op &&
                   ScriptRuntimeMetadata::CompareVersion(existing.Version, dependency.Version) == 0;
        });
        if (it == dependencies.end()) {
            dependencies.push_back(std::move(dependency));
        }
    }
}

std::vector<KeyValue> ParseKeyValues(const std::string &metadataBody) {
    std::vector<KeyValue> result;
    std::size_t pos = 0;
    while (pos < metadataBody.size()) {
        while (pos < metadataBody.size() && std::isspace(static_cast<unsigned char>(metadataBody[pos]))) {
            ++pos;
        }
        if (pos >= metadataBody.size()) {
            break;
        }

        const std::size_t keyStart = pos;
        while (pos < metadataBody.size()) {
            const char c = metadataBody[pos];
            if (std::isspace(static_cast<unsigned char>(c)) || c == '=') {
                break;
            }
            ++pos;
        }
        std::string key = metadataBody.substr(keyStart, pos - keyStart);
        while (pos < metadataBody.size() && std::isspace(static_cast<unsigned char>(metadataBody[pos]))) {
            ++pos;
        }
        std::string value = "true";
        if (pos < metadataBody.size() && metadataBody[pos] == '=') {
            ++pos;
            while (pos < metadataBody.size() && std::isspace(static_cast<unsigned char>(metadataBody[pos]))) {
                ++pos;
            }
            if (pos < metadataBody.size() && (metadataBody[pos] == '"' || metadataBody[pos] == '\'')) {
                const char quote = metadataBody[pos++];
                std::string parsed;
                while (pos < metadataBody.size()) {
                    const char c = metadataBody[pos++];
                    if (c == quote) {
                        break;
                    }
                    if (c == '\\' && pos < metadataBody.size()) {
                        parsed.push_back(metadataBody[pos++]);
                    } else {
                        parsed.push_back(c);
                    }
                }
                value = parsed;
            } else {
                const std::size_t valueStart = pos;
                while (pos < metadataBody.size() && !std::isspace(static_cast<unsigned char>(metadataBody[pos]))) {
                    ++pos;
                }
                value = metadataBody.substr(valueStart, pos - valueStart);
            }
        }
        key = ScriptRuntimeMetadata::ToLower(key);
        if (!key.empty()) {
            result.push_back({std::move(key), std::move(value)});
        }
    }
    return result;
}

bool StartsWithWord(const std::string &text, std::size_t pos, const char *word) {
    const std::size_t length = std::strlen(word);
    if (text.compare(pos, length, word) != 0) {
        return false;
    }
    const std::size_t end = pos + length;
    return end >= text.size() || !std::isalnum(static_cast<unsigned char>(text[end]));
}

std::string MetadataBaseKind(const std::string &kind) {
    const std::size_t dot = kind.find('.');
    return dot == std::string::npos ? kind : kind.substr(0, dot);
}

std::string MetadataSuffixKind(const std::string &kind) {
    const std::size_t dot = kind.find('.');
    return dot == std::string::npos ? std::string() : kind.substr(dot + 1);
}

bool IsMetadataBaseKind(const std::string &base) {
    return base == "script" || base == "runtime" || base == "executor";
}

bool IsMainMetadataKind(const std::string &kind) {
    return IsMetadataBaseKind(MetadataBaseKind(kind)) && MetadataSuffixKind(kind).empty();
}

bool IsMetadataKind(const std::string &kind) {
    if (IsMainMetadataKind(kind)) {
        return true;
    }
    const std::string base = MetadataBaseKind(kind);
    const std::string suffix = MetadataSuffixKind(kind);
    return IsMetadataBaseKind(base) &&
           (suffix == "meta" || suffix == "depends" || suffix == "messages");
}

std::string HostFromMetadataKind(const std::string &kind) {
    const std::string base = MetadataBaseKind(kind);
    if (base == "runtime") {
        return "runtime";
    }
    if (base == "executor") {
        return "executor";
    }
    return std::string();
}

std::string ParseClassAfterMetadata(const std::string &source, std::size_t pos) {
    while (pos < source.size()) {
        while (pos < source.size() && std::isspace(static_cast<unsigned char>(source[pos]))) {
            ++pos;
        }
        if (source.compare(pos, 2, "//") == 0) {
            pos = source.find('\n', pos);
            if (pos == std::string::npos) {
                return std::string();
            }
            continue;
        }
        if (source.compare(pos, 2, "/*") == 0) {
            pos = source.find("*/", pos + 2);
            if (pos == std::string::npos) {
                return std::string();
            }
            pos += 2;
            continue;
        }
        break;
    }
    if (!StartsWithWord(source, pos, "class")) {
        return std::string();
    }
    pos += 5;
    while (pos < source.size() && std::isspace(static_cast<unsigned char>(source[pos]))) {
        ++pos;
    }
    const std::size_t start = pos;
    while (pos < source.size()) {
        const unsigned char c = static_cast<unsigned char>(source[pos]);
        if (!std::isalnum(c) && source[pos] != '_') {
            break;
        }
        ++pos;
    }
    return source.substr(start, pos - start);
}

bool ScanMetadataBlocks(const std::string &source, std::vector<MetadataBlock> &blocks, std::string &error) {
    std::size_t pos = 0;
    while ((pos = source.find('[', pos)) != std::string::npos) {
        std::size_t tagEnd = pos + 1;
        while (tagEnd < source.size()) {
            const char c = source[tagEnd];
            if (std::isspace(static_cast<unsigned char>(c)) || c == ']') {
                break;
            }
            ++tagEnd;
        }
        std::string kind = ScriptRuntimeMetadata::ToLower(source.substr(pos + 1, tagEnd - (pos + 1)));
        if (!IsMetadataKind(kind)) {
            pos = tagEnd;
            continue;
        }

        bool quoted = false;
        char quote = '\0';
        std::size_t end = tagEnd;
        for (; end < source.size(); ++end) {
            const char c = source[end];
            if ((c == '"' || c == '\'') && (!quoted || quote == c)) {
                quoted = !quoted;
                quote = quoted ? c : '\0';
            } else if (c == ']' && !quoted) {
                break;
            }
        }
        if (end >= source.size()) {
            error = fmt::format("unterminated [{}] metadata", kind);
            return false;
        }

        MetadataBlock block;
        block.Kind = std::move(kind);
        block.Values = ParseKeyValues(source.substr(tagEnd, end - tagEnd));
        block.AttachedClass = ParseClassAfterMetadata(source, end + 1);
        blocks.push_back(std::move(block));
        pos = end + 1;
    }
    return true;
}

std::string DependencyOperatorText(ScriptRuntimeVersionOp op) {
    switch (op) {
    case ScriptRuntimeVersionOp::Equal:
        return "==";
    case ScriptRuntimeVersionOp::Greater:
        return ">";
    case ScriptRuntimeVersionOp::GreaterEqual:
        return ">=";
    case ScriptRuntimeVersionOp::Less:
        return "<";
    case ScriptRuntimeVersionOp::LessEqual:
        return "<=";
    case ScriptRuntimeVersionOp::Any:
    default:
        return "";
    }
}

} // namespace

namespace ScriptRuntimeMetadata {

std::string Trim(const std::string &text) {
    std::size_t first = 0;
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first]))) {
        ++first;
    }
    std::size_t last = text.size();
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) {
        --last;
    }
    return text.substr(first, last - first);
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string StripQuotes(const std::string &value) {
    if (value.size() >= 2) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.substr(1, value.size() - 2);
        }
    }
    return value;
}

std::vector<std::string> SplitList(const std::string &value) {
    std::vector<std::string> result;
    std::string current;
    bool quoted = false;
    char quote = '\0';
    for (char c : value) {
        if ((c == '"' || c == '\'') && (!quoted || quote == c)) {
            quoted = !quoted;
            quote = quoted ? c : '\0';
            current.push_back(c);
            continue;
        }
        if ((c == ';' || c == ',') && !quoted) {
            std::string item = Trim(StripQuotes(Trim(current)));
            if (!item.empty()) {
                result.push_back(std::move(item));
            }
            current.clear();
            continue;
        }
        current.push_back(c);
    }
    std::string item = Trim(StripQuotes(Trim(current)));
    if (!item.empty()) {
        result.push_back(std::move(item));
    }
    return result;
}

bool ParseBool(const std::string &value, bool fallback) {
    const std::string lowered = ToLower(Trim(StripQuotes(value)));
    if (lowered == "true" || lowered == "1" || lowered == "yes" || lowered == "on") {
        return true;
    }
    if (lowered == "false" || lowered == "0" || lowered == "no" || lowered == "off") {
        return false;
    }
    return fallback;
}

int ParseInt(const std::string &value, int fallback) {
    char *end = nullptr;
    const std::string text = Trim(StripQuotes(value));
    const long parsed = std::strtol(text.c_str(), &end, 0);
    if (end && *end == '\0') {
        return static_cast<int>(parsed);
    }
    return fallback;
}

std::string SanitizeId(const std::string &id) {
    std::string result;
    result.reserve(id.size());
    for (char c : id) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) || c == '_' || c == '-' || c == '.') {
            result.push_back(c);
        } else {
            result.push_back('_');
        }
    }
    return result.empty() ? "script" : result;
}

bool ParseHost(const std::string &text, ScriptEntryHost &host) {
    const std::string value = ToLower(Trim(StripQuotes(text)));
    if (value.empty() || value == "runtime") {
        host = ScriptEntryHost::Runtime;
        return true;
    }
    if (value == "executor") {
        host = ScriptEntryHost::Executor;
        return true;
    }
    return false;
}

std::string HostText(ScriptEntryHost host) {
    switch (host) {
    case ScriptEntryHost::Runtime:
        return "runtime";
    case ScriptEntryHost::Executor:
        return "executor";
    }
    return "runtime";
}

std::string DeriveIdFromPath(const std::filesystem::path &scriptsRoot,
                             const std::filesystem::path &manifestPath) {
    namespace fs = std::filesystem;

    const bool isDirectoryManifest = ToLower(manifestPath.filename().string()) == "script.as";
    const fs::path subject = isDirectoryManifest ? manifestPath.parent_path() : manifestPath;
    fs::path relative = subject.lexically_relative(scriptsRoot);

    bool useFallback = relative.empty() || relative == ".";
    if (!useFallback) {
        for (const fs::path &part : relative) {
            if (part == "..") {
                useFallback = true;
                break;
            }
        }
    }

    if (useFallback) {
        relative = isDirectoryManifest ? subject.filename() : manifestPath.stem();
    }

    std::vector<fs::path> parts;
    for (const fs::path &part : relative) {
        if (part.empty() || part == "." || part == "..") {
            continue;
        }
        parts.push_back(part);
    }

    std::string id;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        std::string part = (!isDirectoryManifest && i + 1 == parts.size())
                               ? parts[i].stem().string()
                               : parts[i].string();
        part = Trim(part);
        if (part.empty()) {
            continue;
        }
        if (!id.empty()) {
            id += ".";
        }
        id += part;
    }

    if (id.empty()) {
        id = isDirectoryManifest ? subject.filename().string() : manifestPath.stem().string();
    }
    return SanitizeId(id);
}

std::string PathString(const std::filesystem::path &path) {
    return path.string();
}

std::string ReadTextFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::string();
    }
    std::ostringstream stream;
    stream << input.rdbuf();
    return stream.str();
}

bool IsSkippedMainFile(const std::filesystem::path &path) {
    const std::string filename = ToLower(path.filename().string());
    return filename.ends_with(".inc.as") || filename.ends_with(".include.as");
}

bool IsSkippedDirectory(const std::filesystem::path &path) {
    const std::string name = path.filename().string();
    return name.empty() || name.front() == '_' || name.front() == '.';
}

ScriptRuntimeVersion ParseVersion(const std::string &text) {
    ScriptRuntimeVersion version;
    version.Text = Trim(StripQuotes(text));
    if (version.Text.empty()) {
        version.Text = "0.0.0";
    }

    int parts[3] = {0, 0, 0};
    int partIndex = 0;
    int value = 0;
    bool reading = false;
    for (char ch : version.Text) {
        if (ch >= '0' && ch <= '9') {
            value = value * 10 + (ch - '0');
            reading = true;
        } else {
            if (reading && partIndex < 3) {
                parts[partIndex++] = value;
            }
            value = 0;
            reading = false;
            if (partIndex >= 3) {
                break;
            }
        }
    }
    if (reading && partIndex < 3) {
        parts[partIndex] = value;
    }
    version.Major = parts[0];
    version.Minor = parts[1];
    version.Patch = parts[2];
    return version;
}

int CompareVersion(const ScriptRuntimeVersion &lhs, const ScriptRuntimeVersion &rhs) {
    if (lhs.Major != rhs.Major) {
        return lhs.Major < rhs.Major ? -1 : 1;
    }
    if (lhs.Minor != rhs.Minor) {
        return lhs.Minor < rhs.Minor ? -1 : 1;
    }
    if (lhs.Patch != rhs.Patch) {
        return lhs.Patch < rhs.Patch ? -1 : 1;
    }
    return 0;
}

bool SatisfiesVersion(const ScriptRuntimeVersion &actual, const ScriptRuntimeDependency &dependency) {
    const int cmp = CompareVersion(actual, dependency.Version);
    switch (dependency.Op) {
    case ScriptRuntimeVersionOp::Any:
        return true;
    case ScriptRuntimeVersionOp::Equal:
        return cmp == 0;
    case ScriptRuntimeVersionOp::Greater:
        return cmp > 0;
    case ScriptRuntimeVersionOp::GreaterEqual:
        return cmp >= 0;
    case ScriptRuntimeVersionOp::Less:
        return cmp < 0;
    case ScriptRuntimeVersionOp::LessEqual:
        return cmp <= 0;
    }
    return false;
}

std::string VersionRequirementText(const ScriptRuntimeDependency &dependency) {
    return dependency.Id + DependencyOperatorText(dependency.Op) +
           (dependency.Op == ScriptRuntimeVersionOp::Any ? "" : dependency.Version.Text);
}

bool ParseDependencySpec(const std::string &text, ScriptRuntimeDependency &dependency, std::string &error) {
    std::string value = Trim(StripQuotes(text));
    if (value.empty()) {
        error = "empty script entry dependency";
        return false;
    }
    dependency.Raw = value;

    const std::pair<const char *, ScriptRuntimeVersionOp> ops[] = {
        {">=", ScriptRuntimeVersionOp::GreaterEqual},
        {"<=", ScriptRuntimeVersionOp::LessEqual},
        {"==", ScriptRuntimeVersionOp::Equal},
        {">", ScriptRuntimeVersionOp::Greater},
        {"<", ScriptRuntimeVersionOp::Less},
        {"=", ScriptRuntimeVersionOp::Equal},
    };
    for (const auto &op : ops) {
        const std::size_t pos = value.find(op.first);
        if (pos != std::string::npos) {
            dependency.Id = SanitizeId(Trim(value.substr(0, pos)));
            dependency.Op = op.second;
            dependency.Version = ParseVersion(value.substr(pos + std::strlen(op.first)));
            break;
        }
    }
    if (dependency.Id.empty()) {
        dependency.Id = SanitizeId(value);
        dependency.Op = ScriptRuntimeVersionOp::Any;
        dependency.Version = ParseVersion("0.0.0");
    }
    if (dependency.Id.empty()) {
        error = fmt::format("invalid script entry dependency '{}'", value);
        return false;
    }
    return true;
}

bool ParseManifestSource(const std::string &source,
                         const std::filesystem::path &mainPath,
                         ScriptRuntimeManifest &manifest,
                         std::string &error) {
    std::vector<MetadataBlock> blocks;
    if (!ScanMetadataBlocks(source, blocks, error)) {
        return false;
    }

    std::map<std::string, std::string> scalars;
    std::vector<std::string> files;
    std::vector<std::string> tags;
    std::vector<std::string> before;
    std::vector<std::string> after;
    std::vector<std::string> messageTopics;
    std::vector<ScriptRuntimeDependency> required;
    std::vector<ScriptRuntimeDependency> optional;
    std::vector<ScriptRuntimeMetadataEntry> custom;
    std::string attachedClass;
    std::string dependencyErrors;
    bool hasScriptManifest = false;
    std::string declaredHost;

    for (const MetadataBlock &block : blocks) {
        const std::string suffix = MetadataSuffixKind(block.Kind);
        if (IsMainMetadataKind(block.Kind)) {
            hasScriptManifest = true;
            const std::string blockHost = HostFromMetadataKind(block.Kind);
            if (!blockHost.empty()) {
                if (!declaredHost.empty() && declaredHost != blockHost) {
                    error = fmt::format("conflicting script host declarations '{}' and '{}'",
                                        declaredHost,
                                        blockHost);
                    return false;
                }
                declaredHost = blockHost;
            }
        }
        if (!block.AttachedClass.empty()) {
            attachedClass = block.AttachedClass;
        }
        if (suffix == "meta") {
            std::string explicitKey;
            std::string explicitValue;
            for (const KeyValue &pair : block.Values) {
                if (pair.Key == "key") {
                    explicitKey = pair.Value;
                } else if (pair.Key == "value") {
                    explicitValue = pair.Value;
                } else {
                    SetMetadata(custom, pair.Key, pair.Value);
                }
            }
            if (!explicitKey.empty()) {
                SetMetadata(custom, explicitKey, explicitValue);
            }
            continue;
        }

        for (const KeyValue &pair : block.Values) {
            if (suffix == "depends") {
                if (pair.Key == "depends" || pair.Key == "required") {
                    AppendDependencies(required, pair.Value, dependencyErrors);
                } else if (pair.Key == "optional") {
                    AppendDependencies(optional, pair.Value, dependencyErrors);
                } else if (pair.Key == "before") {
                    AppendList(before, pair.Value);
                } else if (pair.Key == "after") {
                    AppendList(after, pair.Value);
                } else {
                    SetMetadata(custom, pair.Key, pair.Value);
                }
                continue;
            }
            if (suffix == "messages") {
                if (pair.Key == "topics") {
                    AppendList(messageTopics, pair.Value);
                } else {
                    SetMetadata(custom, pair.Key, pair.Value);
                }
                continue;
            }

            if (IsScalarKey(pair.Key)) {
                scalars[pair.Key] = pair.Value;
            } else if (pair.Key == "files") {
                AppendList(files, pair.Value);
            } else if (pair.Key == "depends" || pair.Key == "required") {
                AppendDependencies(required, pair.Value, dependencyErrors);
            } else if (pair.Key == "optional") {
                AppendDependencies(optional, pair.Value, dependencyErrors);
            } else if (pair.Key == "tags") {
                AppendList(tags, pair.Value);
            } else if (pair.Key == "before") {
                AppendList(before, pair.Value);
            } else if (pair.Key == "after") {
                AppendList(after, pair.Value);
            } else if (!IsListKey(pair.Key)) {
                SetMetadata(custom, pair.Key, pair.Value);
            }
        }
    }

    if (!dependencyErrors.empty()) {
        error = ScriptRuntimeMetadata::Trim(dependencyErrors);
        return false;
    }

    manifest.HasManifest = !blocks.empty();
    manifest.HasScriptManifest = hasScriptManifest;
    if (const auto it = scalars.find("id"); it != scalars.end()) {
        manifest.Id = it->second;
    }
    if (const auto it = scalars.find("host"); it != scalars.end()) {
        ScriptEntryHost parsedHost = ScriptEntryHost::Runtime;
        if (!ParseHost(it->second, parsedHost)) {
            error = fmt::format("unsupported script host '{}'",
                                Trim(StripQuotes(it->second)));
            return false;
        }
        const std::string explicitHost = HostText(parsedHost);
        if (!declaredHost.empty() && explicitHost != declaredHost) {
            error = fmt::format("conflicting script host declarations '{}' and '{}'",
                                declaredHost,
                                explicitHost);
            return false;
        }
        manifest.Host = parsedHost;
    } else if (!declaredHost.empty()) {
        ScriptEntryHost parsedHost = ScriptEntryHost::Runtime;
        if (!ParseHost(declaredHost, parsedHost)) {
            error = fmt::format("unsupported script host '{}'", declaredHost);
            return false;
        }
        manifest.Host = parsedHost;
    } else {
        manifest.Host = ScriptEntryHost::Runtime;
    }
    if (const auto it = scalars.find("name"); it != scalars.end()) {
        manifest.Name = it->second;
    }
    if (manifest.Name.empty()) {
        manifest.Name = manifest.Id;
    }
    if (const auto it = scalars.find("version"); it != scalars.end()) {
        manifest.VersionText = it->second;
    }
    manifest.Version = ParseVersion(manifest.VersionText);
    manifest.VersionText = manifest.Version.Text;
    if (const auto it = scalars.find("class"); it != scalars.end()) {
        manifest.ClassName = it->second;
    } else if (!attachedClass.empty()) {
        manifest.ClassName = attachedClass;
    }
    if (const auto it = scalars.find("enabled"); it != scalars.end()) {
        manifest.Enabled = ParseBool(it->second, true);
    }
    if (const auto it = scalars.find("order"); it != scalars.end()) {
        manifest.Order = ParseInt(it->second, 1000);
    }
    if (const auto it = scalars.find("reload"); it != scalars.end()) {
        manifest.Reloadable = ParseBool(it->second, true);
    }
    if (const auto it = scalars.find("description"); it != scalars.end()) {
        manifest.Description = it->second;
    }
    if (const auto it = scalars.find("author"); it != scalars.end()) {
        manifest.Author = it->second;
    }
    if (const auto it = scalars.find("category"); it != scalars.end()) {
        manifest.Category = it->second;
    }
    if (const auto it = scalars.find("entry"); it != scalars.end() && !Trim(it->second).empty()) {
        manifest.EntryPath = manifest.RootPath / StripQuotes(it->second);
    } else {
        manifest.EntryPath = mainPath;
    }

    manifest.FileSpecs = std::move(files);
    manifest.Tags = std::move(tags);
    if (!manifest.Description.empty()) {
        SetMetadata(custom, "description", manifest.Description);
    }
    if (!manifest.Author.empty()) {
        SetMetadata(custom, "author", manifest.Author);
    }
    if (!manifest.Category.empty()) {
        SetMetadata(custom, "category", manifest.Category);
    }
    if (!manifest.Tags.empty()) {
        std::string tagText;
        for (const std::string &tag : manifest.Tags) {
            if (!tagText.empty()) {
                tagText += ";";
            }
            tagText += tag;
        }
        SetMetadata(custom, "tags", tagText);
    }
    manifest.RequiredDependencies = std::move(required);
    manifest.OptionalDependencies = std::move(optional);
    for (std::string &value : before) {
        value = SanitizeId(value);
    }
    for (std::string &value : after) {
        value = SanitizeId(value);
    }
    manifest.Before = std::move(before);
    manifest.After = std::move(after);
    manifest.MessageTopics = std::move(messageTopics);
    manifest.CustomMetadata = std::move(custom);
    manifest.Id = SanitizeId(manifest.Id);
    if (manifest.Name.empty()) {
        manifest.Name = manifest.Id;
    }
    return true;
}

std::string MetadataValue(const std::vector<ScriptRuntimeMetadataEntry> &metadata,
                          const std::string &key,
                          const std::string &fallback) {
    const std::string lowered = ToLower(Trim(key));
    for (const ScriptRuntimeMetadataEntry &entry : metadata) {
        if (entry.Key == lowered) {
            return entry.Value;
        }
    }
    return fallback;
}

std::vector<std::string> MetadataKeys(const std::vector<ScriptRuntimeMetadataEntry> &metadata) {
    std::vector<std::string> keys;
    keys.reserve(metadata.size());
    for (const ScriptRuntimeMetadataEntry &entry : metadata) {
        keys.push_back(entry.Key);
    }
    return keys;
}

} // namespace ScriptRuntimeMetadata
