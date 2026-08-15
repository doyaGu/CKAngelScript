#include "ScriptModuleGeneration.h"

bool IsAttachedModuleGenerationCurrent(std::uint32_t attachedGeneration,
                                       std::uint32_t currentGeneration) {
    return attachedGeneration != 0 && attachedGeneration == currentGeneration;
}
