#ifndef CK_SCRIPTCKPROTOTYPEHOOK_H
#define CK_SCRIPTCKPROTOTYPEHOOK_H

#include <string>

bool ParseCKASEnvironmentFlag(const char *value);
bool InstallCKBehaviorPrototypeRuntimeHook(std::string *error = nullptr);
void UninstallCKBehaviorPrototypeRuntimeHook();
bool IsCKBehaviorPrototypeRuntimeHookInstalled();

#endif // CK_SCRIPTCKPROTOTYPEHOOK_H
