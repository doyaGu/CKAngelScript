#include "ScriptSelfTests.h"

#include <string>

#include "ScriptRuntimeDependency.h"
#include "ScriptRuntimeMetadata.h"

namespace ScriptRuntimeDependencyResolver {

bool RunScriptRuntimeDependencySelfTest(std::string &error) {
    ScriptRuntimeManifest core;
    core.Id = "core";
    core.Name = "core";
    core.VersionText = "1.2.0";
    core.Version = ScriptRuntimeMetadata::ParseVersion(core.VersionText);
    core.Order = 10;

    ScriptRuntimeManifest smoke;
    smoke.Id = "smoke";
    smoke.Name = "smoke";
    smoke.VersionText = "1.0.0";
    smoke.Version = ScriptRuntimeMetadata::ParseVersion(smoke.VersionText);
    smoke.Order = 0;
    ScriptRuntimeDependency required;
    std::string parseError;
    if (!ScriptRuntimeMetadata::ParseDependencySpec("core>=1.0.0", required, parseError)) {
        error = parseError;
        return false;
    }
    smoke.RequiredDependencies.push_back(required);

    ScriptRuntimeLoadPlan plan = Resolve({smoke, core});
    if (plan.Scripts.size() != 2 || plan.Scripts[0].Id != "core" || plan.Scripts[1].Id != "smoke") {
        error = "Runtime dependency resolver did not sort required dependencies before dependents.";
        return false;
    }

    ScriptRuntimeManifest missing = smoke;
    missing.Id = "missing";
    missing.RequiredDependencies[0].Id = "not.present";
    plan = Resolve({missing});
    if (!plan.Scripts.empty() || plan.Diagnostics.find("required dependency") == std::string::npos) {
        error = "Runtime dependency resolver did not reject missing required dependencies.";
        return false;
    }
    if (plan.SkippedScripts.size() != 1 || plan.SkippedScripts[0].Manifest.Id != "missing" ||
        plan.SkippedScripts[0].Error.find("required dependency") == std::string::npos) {
        error = "Runtime dependency resolver did not preserve skipped script diagnostics.";
        return false;
    }

    ScriptRuntimeManifest a;
    a.Id = "a";
    a.Name = "a";
    a.Version = ScriptRuntimeMetadata::ParseVersion("1.0.0");
    ScriptRuntimeManifest b = a;
    b.Id = "b";
    ScriptRuntimeDependency depA;
    ScriptRuntimeDependency depB;
    if (!ScriptRuntimeMetadata::ParseDependencySpec("b", depA, parseError) ||
        !ScriptRuntimeMetadata::ParseDependencySpec("a", depB, parseError)) {
        error = parseError;
        return false;
    }
    a.RequiredDependencies.push_back(depA);
    b.RequiredDependencies.push_back(depB);
    plan = Resolve({a, b});
    if (!plan.Scripts.empty() || plan.Diagnostics.find("cycle") == std::string::npos) {
        error = "Runtime dependency resolver did not reject dependency cycles.";
        return false;
    }
    if (plan.SkippedScripts.size() != 2) {
        error = "Runtime dependency resolver did not preserve cycle skipped scripts.";
        return false;
    }
    return true;
}
} // namespace ScriptRuntimeDependencyResolver
