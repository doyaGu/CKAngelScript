#ifndef CK_SCRIPT_COMPONENT_AUTOMATION_H
#define CK_SCRIPT_COMPONENT_AUTOMATION_H

#include "CKAll.h"

#include <string>

#include "ScriptManager.h"

namespace AngelScriptComponentInternal {

void StopComponentLifetimeBBConfigs(const CKBehaviorContext &behcontext, ScriptComponentState *state);
void DestroyComponentLifetimeBBConfigs(ScriptComponentState *state);
bool EnsureAutoStartedBBConfigs(const CKBehaviorContext &behcontext, ScriptComponentState *state, std::string &error);
bool StepAutomatedBBConfigs(const CKBehaviorContext &behcontext, ScriptComponentState *state, std::string &error);

} // namespace AngelScriptComponentInternal

#endif
