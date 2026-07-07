#ifndef CK_SCRIPTRUNTIMEMETADATA_H
#define CK_SCRIPTRUNTIMEMETADATA_H

#include <filesystem>
#include <string>
#include <vector>

struct ScriptRuntimeMetadataEntry {
    std::string Key;
    std::string Value;
};

struct ScriptRuntimeVersion {
    int Major = 0;
    int Minor = 0;
    int Patch = 0;
    std::string Text = "0.0.0";
};

enum class ScriptRuntimeVersionOp {
    Any,
    Equal,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
};

struct ScriptRuntimeDependency {
    std::string Raw;
    std::string Id;
    ScriptRuntimeVersionOp Op = ScriptRuntimeVersionOp::Any;
    ScriptRuntimeVersion Version;
};

enum class ScriptEntryHost {
    Runtime,
    Executor,
};

struct ScriptRuntimeManifest {
    std::string Id;
    std::string Name;
    std::string VersionText = "0.0.0";
    ScriptRuntimeVersion Version;
    std::string ClassName;
    ScriptEntryHost Host = ScriptEntryHost::Runtime;
    std::filesystem::path RootPath;
    std::filesystem::path ManifestPath;
    std::filesystem::path EntryPath;
    std::vector<std::filesystem::path> Files;
    std::vector<std::string> FileSpecs;
    std::string Description;
    std::string Author;
    std::string Category;
    std::vector<std::string> Tags;
    std::vector<ScriptRuntimeDependency> RequiredDependencies;
    std::vector<ScriptRuntimeDependency> OptionalDependencies;
    std::vector<std::string> Before;
    std::vector<std::string> After;
    std::vector<std::string> MessageTopics;
    std::vector<ScriptRuntimeMetadataEntry> CustomMetadata;
    bool Enabled = true;
    int Order = 1000;
    bool Reloadable = true;
    bool HasManifest = false;
    bool HasScriptManifest = false;
    std::string Error;
};

namespace ScriptRuntimeMetadata {

std::string Trim(const std::string &text);
std::string ToLower(std::string value);
std::string StripQuotes(const std::string &value);
std::vector<std::string> SplitList(const std::string &value);
bool ParseBool(const std::string &value, bool fallback);
int ParseInt(const std::string &value, int fallback);
std::string SanitizeId(const std::string &id);
bool ParseHost(const std::string &text, ScriptEntryHost &host);
std::string HostText(ScriptEntryHost host);
std::string DeriveIdFromPath(const std::filesystem::path &scriptsRoot,
                             const std::filesystem::path &manifestPath);
std::string PathString(const std::filesystem::path &path);
std::string ReadTextFile(const std::filesystem::path &path);
bool IsSkippedMainFile(const std::filesystem::path &path);
bool IsSkippedDirectory(const std::filesystem::path &path);

ScriptRuntimeVersion ParseVersion(const std::string &text);
int CompareVersion(const ScriptRuntimeVersion &lhs, const ScriptRuntimeVersion &rhs);
bool SatisfiesVersion(const ScriptRuntimeVersion &actual, const ScriptRuntimeDependency &dependency);
std::string VersionRequirementText(const ScriptRuntimeDependency &dependency);
bool ParseDependencySpec(const std::string &text, ScriptRuntimeDependency &dependency, std::string &error);

bool ParseManifestSource(const std::string &source,
                         const std::filesystem::path &mainPath,
                         ScriptRuntimeManifest &manifest,
                         std::string &error);
std::string MetadataValue(const std::vector<ScriptRuntimeMetadataEntry> &metadata,
                          const std::string &key,
                          const std::string &fallback = std::string());
std::vector<std::string> MetadataKeys(const std::vector<ScriptRuntimeMetadataEntry> &metadata);

} // namespace ScriptRuntimeMetadata

#endif // CK_SCRIPTRUNTIMEMETADATA_H
