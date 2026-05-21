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
#if CKAS_BUILD_SELF_TESTS
bool RunScriptRuntimeDependencySelfTest(std::string &error);
#endif

} // namespace ScriptRuntimeDependencyResolver

#endif // CK_SCRIPTRUNTIMEDEPENDENCY_H
