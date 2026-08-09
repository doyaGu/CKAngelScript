#ifndef CK_SCRIPTBRIDGESCOPE_H
#define CK_SCRIPTBRIDGESCOPE_H

#include <string>

#include "ScriptHostScope.h"

class BBBridge;
class BBDecl;
class BBPrototype;
class BehaviorBridge;
class BehaviorGraph;
class BehaviorRef;
class CScriptArray;

enum class ScriptBridgeFailureMode {
    Quiet,
    ScriptException
};

namespace ScriptBBAdapter {

BBBridge *From(const ScriptHostScope &scope, ScriptBridgeFailureMode failureMode);
BBDecl *Require(const ScriptHostScope &scope,
                const std::string &query,
                ScriptBridgeFailureMode failureMode);
BBDecl *Require(const ScriptHostScope &scope,
                CKGUID guid,
                ScriptBridgeFailureMode failureMode);
int Count(const ScriptHostScope &scope, ScriptBridgeFailureMode failureMode);
BBPrototype *At(const ScriptHostScope &scope,
                int index,
                ScriptBridgeFailureMode failureMode);
BBPrototype *Find(const ScriptHostScope &scope,
                  const std::string &query,
                  int occurrence,
                  ScriptBridgeFailureMode failureMode);
CScriptArray *FindAll(const ScriptHostScope &scope,
                      const std::string &query,
                      ScriptBridgeFailureMode failureMode);

} // namespace ScriptBBAdapter

namespace ScriptBehaviorAdapter {

BehaviorBridge *From(const ScriptHostScope &scope);
BehaviorGraph *Graph(const ScriptHostScope &scope,
                     const std::string &rootBehaviorName = std::string());
BehaviorRef *Find(const ScriptHostScope &scope, const std::string &name);
BehaviorRef *FindById(const ScriptHostScope &scope, CK_ID id);

} // namespace ScriptBehaviorAdapter

#endif // CK_SCRIPTBRIDGESCOPE_H
