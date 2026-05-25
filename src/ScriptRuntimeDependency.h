#ifndef CK_SCRIPTRUNTIMEDEPENDENCY_H
#define CK_SCRIPTRUNTIMEDEPENDENCY_H

#include <string>
#include <vector>

#include "ScriptRuntimeMetadata.h"

struct ScriptRuntimeSkippedScript {
    ScriptRuntimeManifest Manifest;
    std::string Error;
};

struct ScriptRuntimeLoadPlan {
    std::vector<ScriptRuntimeManifest> Scripts;
    std::vector<ScriptRuntimeSkippedScript> SkippedScripts;
    std::string Diagnostics;
};

namespace ScriptRuntimeDependencyResolver {

ScriptRuntimeLoadPlan Resolve(const std::vector<ScriptRuntimeManifest> &scripts);
bool HasDependencyFailure(const ScriptRuntimeManifest &script,
                          const std::vector<std::string> &failedIds,
                          std::string &error);

} // namespace ScriptRuntimeDependencyResolver

#endif // CK_SCRIPTRUNTIMEDEPENDENCY_H
