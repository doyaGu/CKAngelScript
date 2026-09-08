#ifndef CK_SCRIPT_EXECUTOR_STATE_H
#define CK_SCRIPT_EXECUTOR_STATE_H

#include <cstdint>
#include <string>
#include <vector>

#include <angelscript.h>

#include "CKAll.h"
#include "ScriptInvoker.h"
#include "ScriptInitialLifecycle.h"

class ScriptManager;

struct ScriptExecutorState {
    CK_ID BehaviorId = 0;
    std::uint64_t AsyncOwnerId = 0;
    CKDWORD AttachedModuleGeneration = 0;
    CKBehavior *Behavior = nullptr;
    ScriptManager *Manager = nullptr;
    ScriptInvoker *Invoker = nullptr;
    asIScriptObject *Object = nullptr;
    asIScriptFunction *Execute = nullptr;
    asIScriptFunction *OnLoad = nullptr;
    asIScriptFunction *Awake = nullptr;
    asIScriptFunction *Start = nullptr;
    asIScriptFunction *OnPause = nullptr;
    asIScriptFunction *OnResume = nullptr;
    asIScriptFunction *OnDestroy = nullptr;
    asIScriptFunction *OnReset = nullptr;

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
        ReleaseFunction(OnPause);
        ReleaseFunction(OnResume);
        ReleaseFunction(OnDestroy);
        ReleaseFunction(OnReset);
    }

    void ReleaseObject() {
        if (Object) {
            Object->Release();
            Object = nullptr;
        }
    }
};

#endif // CK_SCRIPT_EXECUTOR_STATE_H
