#ifndef CK_SCRIPT_COMPONENT_INJECTION_H
#define CK_SCRIPT_COMPONENT_INJECTION_H

#include "CKAll.h"

#include <string>

#include "ScriptComponentState.h"

class BBConfig;

namespace ScriptComponentSupport {

BBConfig *GetBBConfigField(ScriptComponentState *state, const ScriptComponentBinding &binding);
bool ApplyBBConfigSourceBindings(const CKBehaviorContext &behcontext,
                                 ScriptComponentState *state,
                                 const ScriptComponentBinding &binding,
                                 BBConfig *bbinding,
                                 bool allowPending,
                                 std::string &error);
bool InjectComponentParameters(const CKBehaviorContext &behcontext,
                               ScriptComponentState *state,
                               bool forceHandles,
                               std::string &error);

} // namespace ScriptComponentSupport

#endif
