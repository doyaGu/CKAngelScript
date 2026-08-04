#ifndef CK_SCRIPT_CORE_SELF_TESTS_H
#define CK_SCRIPT_CORE_SELF_TESTS_H

#include <string>

bool RunScriptInitialLifecycleSelfTest(std::string &error);

namespace ScriptRuntimeMetadata {
bool RunScriptRuntimeMetadataSelfTest(std::string &error);
}

namespace ScriptRuntimeDependencyResolver {
bool RunScriptRuntimeDependencySelfTest(std::string &error);
}

#endif // CK_SCRIPT_CORE_SELF_TESTS_H
