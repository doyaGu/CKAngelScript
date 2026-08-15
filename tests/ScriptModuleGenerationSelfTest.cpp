#include "ScriptCoreSelfTests.h"

#include "ScriptModuleGeneration.h"

bool RunScriptModuleGenerationSelfTest(std::string &error) {
    if (IsAttachedModuleGenerationCurrent(0, 0) ||
        IsAttachedModuleGenerationCurrent(0, 1) ||
        IsAttachedModuleGenerationCurrent(1, 0)) {
        error = "An unknown module generation was accepted as current.";
        return false;
    }
    if (!IsAttachedModuleGenerationCurrent(1, 1) ||
        !IsAttachedModuleGenerationCurrent(0xffffffffu, 0xffffffffu)) {
        error = "An equal tracked module generation was rejected.";
        return false;
    }
    if (IsAttachedModuleGenerationCurrent(1, 2) ||
        IsAttachedModuleGenerationCurrent(2, 1)) {
        error = "A stale module generation was accepted as current.";
        return false;
    }
    return true;
}
