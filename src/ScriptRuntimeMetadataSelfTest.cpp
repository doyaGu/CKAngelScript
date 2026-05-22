#include "ScriptSelfTests.h"

#include <filesystem>
#include <string>

#include "ScriptRuntimeMetadata.h"

namespace ScriptRuntimeMetadata {

bool RunScriptRuntimeMetadataSelfTest(std::string &error) {
    const std::string source =
        "[script\n"
        "  id=\"ckas.test\"\n"
        "  name=\"Test Script\"\n"
        "  version=\"1.2\"\n"
        "]\n"
        "[script class=\"TestClass\" files=\"a.as;'b;c.as'\" custom=\"first\"]\n"
        "[script.meta custom=\"second\" key=\"display\" value=\"Runtime Test\"]\n"
        "[script.depends required=\"core>=1.0.0\" optional=\"debug\" before=\"late\" after=\"boot\"]\n"
        "class TestClass {}\n";

    ScriptRuntimeManifest manifest;
    manifest.Id = "fallback";
    manifest.Name = "fallback";
    manifest.RootPath = std::filesystem::path("C:/Scripts/ckas_test");
    manifest.ManifestPath = manifest.RootPath / "main.as";
    manifest.EntryPath = manifest.ManifestPath;
    if (!ParseManifestSource(source, manifest.ManifestPath, manifest, error)) {
        return false;
    }
    if (manifest.Id != "ckas.test" || manifest.Name != "Test Script" || manifest.ClassName != "TestClass") {
        error = "Runtime metadata parser did not merge scalar fragments.";
        return false;
    }
    if (manifest.Version.Major != 1 || manifest.Version.Minor != 2 || manifest.Version.Patch != 0) {
        error = "Runtime metadata parser did not parse version fields.";
        return false;
    }
    if (manifest.FileSpecs.size() != 2 || manifest.FileSpecs[1] != "b;c.as") {
        error = "Runtime metadata parser did not preserve quoted list values.";
        return false;
    }
    if (MetadataValue(manifest.CustomMetadata, "custom") != "second" ||
        MetadataValue(manifest.CustomMetadata, "display") != "Runtime Test") {
        error = "Runtime metadata parser did not merge custom metadata.";
        return false;
    }
    if (manifest.RequiredDependencies.size() != 1 || manifest.RequiredDependencies[0].Id != "core" ||
        manifest.RequiredDependencies[0].Op != ScriptRuntimeVersionOp::GreaterEqual) {
        error = "Runtime metadata parser did not parse required dependency versions.";
        return false;
    }
    if (manifest.OptionalDependencies.size() != 1 || manifest.Before.size() != 1 || manifest.After.size() != 1) {
        error = "Runtime metadata parser did not parse optional or ordering dependencies.";
        return false;
    }
    return true;
}
} // namespace ScriptRuntimeMetadata
