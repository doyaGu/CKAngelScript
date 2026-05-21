#ifndef CK_SCRIPTRUNTIMEDEPENDENCY_H
#define CK_SCRIPTRUNTIMEDEPENDENCY_H

#include <string>
#include <vector>

#include "ScriptRuntimeMetadata.h"

struct ScriptRuntimeLoadPlan {
    std::vector<ScriptRuntimeManifest> Scripts;
    std::string Diagnostics;
};

namespace ScriptRuntimeDependencyResolver {

ScriptRuntimeLoadPlan Resolve(const std::vector<ScriptRuntimeManifest> &scripts);
bool HasDependencyFailure(const ScriptRuntimeManifest &script,
                          const std::vector<std::string> &failedIds,
                          std::string &error);
bool RunScriptRuntimeDependencySelfTest(std::string &error);

} // namespace ScriptRuntimeDependencyResolver

#endif // CK_SCRIPTRUNTIMEDEPENDENCY_H
