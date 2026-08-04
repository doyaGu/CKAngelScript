#include "ScriptCoreSelfTests.h"

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
        "[script class=\"TestClass\" entry=\"runtime.as\" files=\"a.as;'b;c.as'\" custom=\"first\"]\n"
        "[script description=\"Runtime smoke\" author=\"CKAS\" category=\"Diagnostics\" tags=\"debug;smoke\"]\n"
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
    if (!manifest.HasManifest || !manifest.HasScriptManifest) {
        error = "Runtime metadata parser did not mark the main script manifest.";
        return false;
    }
    if (manifest.EntryPath.filename().string() != "runtime.as" ||
        manifest.Description != "Runtime smoke" ||
        manifest.Author != "CKAS" ||
        manifest.Category != "Diagnostics" ||
        manifest.Tags.size() != 2 ||
        manifest.Tags[0] != "debug" ||
        manifest.Tags[1] != "smoke") {
        error = "Runtime metadata parser did not parse v2 first-class fields.";
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
        MetadataValue(manifest.CustomMetadata, "display") != "Runtime Test" ||
        MetadataValue(manifest.CustomMetadata, "description") != "Runtime smoke" ||
        MetadataValue(manifest.CustomMetadata, "tags") != "debug;smoke") {
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

    const std::string sidecarSource =
        "[script.meta key=\"display\" value=\"Helper\"]\n"
        "[script.depends required=\"core\"]\n";
    ScriptRuntimeManifest sidecar;
    sidecar.RootPath = std::filesystem::path("C:/Scripts");
    sidecar.ManifestPath = sidecar.RootPath / "helper.as";
    sidecar.EntryPath = sidecar.ManifestPath;
    if (!ParseManifestSource(sidecarSource, sidecar.ManifestPath, sidecar, error)) {
        return false;
    }
    if (!sidecar.HasManifest || sidecar.HasScriptManifest) {
        error = "Runtime metadata parser treated sidecar metadata as a script entry manifest.";
        return false;
    }

    ScriptRuntimeManifest runtimeHost;
    runtimeHost.RootPath = std::filesystem::path("C:/Scripts/runtime");
    runtimeHost.ManifestPath = runtimeHost.RootPath / "script.as";
    runtimeHost.EntryPath = runtimeHost.ManifestPath;
    if (!ParseManifestSource("[runtime name=\"Runtime Host\"]\nclass RuntimeHost {}\n",
        runtimeHost.ManifestPath,
        runtimeHost,
        error) ||
        runtimeHost.Host != ScriptEntryHost::Runtime ||
        !runtimeHost.HasScriptManifest) {
        error = error.empty() ? "Runtime metadata parser did not recognize [runtime]." : error;
        return false;
    }

    ScriptRuntimeManifest executorHost;
    executorHost.RootPath = std::filesystem::path("C:/Scripts/executor");
    executorHost.ManifestPath = executorHost.RootPath / "script.as";
    executorHost.EntryPath = executorHost.ManifestPath;
    if (!ParseManifestSource("[executor name=\"Executor Host\" class=\"ExecutorHost\"]\nclass ExecutorHost {}\n",
        executorHost.ManifestPath,
        executorHost,
        error) ||
        executorHost.Host != ScriptEntryHost::Executor ||
        executorHost.ClassName != "ExecutorHost" ||
        !executorHost.HasScriptManifest) {
        error = error.empty() ? "Runtime metadata parser did not recognize [executor]." : error;
        return false;
    }

    const std::string lexicalSource =
        "// [executor name=\"Comment Host\"]\n"
        "/*\n"
        "[executor name=\"Block Comment Host\"]\n"
        "*/\n"
        "[runtime name=\"Lexical Runtime Host\"]\n"
        "class LexicalRuntimeHost {\n"
        "  string label = \"[executor]\";\n"
        "  void Probe() { array<int> values(1); int runtime = 0; values[runtime] = 1; }\n"
        "}\n";
    ScriptRuntimeManifest lexicalHost;
    lexicalHost.RootPath = std::filesystem::path("C:/Scripts/lexical");
    lexicalHost.ManifestPath = lexicalHost.RootPath / "script.as";
    lexicalHost.EntryPath = lexicalHost.ManifestPath;
    if (!ParseManifestSource(lexicalSource, lexicalHost.ManifestPath, lexicalHost, error) ||
        lexicalHost.Host != ScriptEntryHost::Runtime ||
        lexicalHost.Name != "Lexical Runtime Host" ||
        lexicalHost.ClassName != "LexicalRuntimeHost" ||
        !lexicalHost.HasScriptManifest) {
        error = error.empty()
            ? "Runtime metadata parser treated comment, string, or index text as metadata."
            : error;
        return false;
    }

    const std::string fakeOnlySource =
        "// [runtime]\n"
        "string label = \"[executor]\";\n"
        "/* [script] */\n";
    ScriptRuntimeManifest fakeOnly;
    fakeOnly.RootPath = std::filesystem::path("C:/Scripts/fake-only");
    fakeOnly.ManifestPath = fakeOnly.RootPath / "helper.as";
    fakeOnly.EntryPath = fakeOnly.ManifestPath;
    if (!ParseManifestSource(fakeOnlySource, fakeOnly.ManifestPath, fakeOnly, error) ||
        fakeOnly.HasManifest || fakeOnly.HasScriptManifest) {
        error = error.empty()
            ? "Runtime metadata parser discovered metadata inside comments or strings."
            : error;
        return false;
    }

    const std::filesystem::path scriptsRoot = "C:/Game/Scripts";
    if (DeriveIdFromPath(scriptsRoot, scriptsRoot / "base/event/load_menu_level.as") !=
        "base.event.load_menu_level") {
        error = "Runtime metadata path id derivation did not include file parent directories.";
        return false;
    }
    if (DeriveIdFromPath(scriptsRoot, scriptsRoot / "base/loading/register_activate_init/script.as") !=
        "base.loading.register_activate_init") {
        error = "Runtime metadata path id derivation did not include directory manifest parents.";
        return false;
    }
    if (DeriveIdFromPath(scriptsRoot / "isolated", scriptsRoot / "isolated/script.as") !=
        "isolated") {
        error = "Runtime metadata path id derivation did not keep the self manifest fallback.";
        return false;
    }
    return true;
}
} // namespace ScriptRuntimeMetadata
