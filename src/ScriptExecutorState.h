#ifndef CK_SCRIPT_EXECUTOR_STATE_H
#define CK_SCRIPT_EXECUTOR_STATE_H

#include <string>
#include <vector>

#include <angelscript.h>

#include "CKAll.h"
#include "ScriptInvoker.h"
#include "ScriptInitialLifecycle.h"

class ScriptManager;

struct ScriptExecutorState {
    CK_ID BehaviorId = 0;
    CKBehavior *Behavior = nullptr;
    ScriptManager *Manager = nullptr;
    ScriptInvoker *Invoker = nullptr;
    asIScriptObject *Object = nullptr;
    asIScriptFunction *Execute = nullptr;
    asIScriptFunction *OnLoad = nullptr;
    asIScriptFunction *Awake = nullptr;
    asIScriptFunction *Start = nullptr;
    asIScriptFunction *OnDestroy = nullptr;
    asIScriptFunction *OnReset = nullptr;
    ScriptBehaviorArgumentKind ExecuteArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind OnLoadArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind AwakeArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind StartArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind OnDestroyArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind OnResetArgument = ScriptBehaviorArgumentKind::BehaviorContext;

    std::string ScriptId;
    std::string ModuleName;
    std::string ClassName;
    std::string InterfaceText;
    bool Loaded = false;
    bool Failed = false;
    ScriptInitialLifecycle InitialLifecycle;
    std::vector<int> SuspendedInputs;
    std::string ActiveLifecycleName;

    static void ReleaseFunction(asIScriptFunction *&function) {
        if (function) {
            function->Release();
            function = nullptr;
        }
    }

    void ReleaseCachedMethods() {
        ReleaseFunction(Execute);
        ReleaseFunction(OnLoad);
        ReleaseFunction(Awake);
        ReleaseFunction(Start);
        ReleaseFunction(OnDestroy);
        ReleaseFunction(OnReset);
        ExecuteArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        OnLoadArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        AwakeArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        StartArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        OnDestroyArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        OnResetArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    }

    void ReleaseObject() {
        if (Object) {
            Object->Release();
            Object = nullptr;
        }
    }
};

#endif // CK_SCRIPT_EXECUTOR_STATE_H
