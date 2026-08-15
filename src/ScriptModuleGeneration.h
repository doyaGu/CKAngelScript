#ifndef CK_SCRIPT_MODULE_GENERATION_H
#define CK_SCRIPT_MODULE_GENERATION_H

#include <cstdint>

bool IsAttachedModuleGenerationCurrent(std::uint32_t attachedGeneration,
                                       std::uint32_t currentGeneration);

#endif // CK_SCRIPT_MODULE_GENERATION_H
