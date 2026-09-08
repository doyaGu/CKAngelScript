#include "ScriptBridgeScope.h"

#include "CKAll.h"
#include "ScriptBridgeCommon.h"
#include "ScriptBridgeHandles.h"

namespace {

void ReportMissingBridge(ScriptBridgeFailureMode failureMode) {
    if (failureMode == ScriptBridgeFailureMode::ScriptException) {
        SetScriptException("AngelScript BB bridge is not available.");
    }
}

void ReportMissingBehaviorBridge(const ScriptHostScope &scope) {
    if (scope.Kind() == ScriptHostScopeKind::Behavior) {
        SetScriptException("AngelScript behavior bridge is not available.");
    }
}

CKBehavior *BehaviorForScope(const ScriptHostScope &scope) {
    return scope.Kind() == ScriptHostScopeKind::Behavior ? scope.Behavior() : nullptr;
}

CKBehavior *FindRuntimeBehaviorByName(const ScriptHostScope &scope,
                                      const std::string &name) {
    if (!scope.Context() || name.empty()) {
        return nullptr;
    }
    return CKBehavior::Cast(
        scope.Context()->GetObjectByName(const_cast<CKSTRING>(name.c_str())));
}

} // namespace

namespace ScriptBBAdapter {

BBBridge *From(const ScriptHostScope &scope, ScriptBridgeFailureMode failureMode) {
    ScriptBehaviorBridge *bridge = scope.Bridge();
    if (!bridge) {
        ReportMissingBridge(failureMode);
        return nullptr;
    }
    return new BBBridge(bridge, scope.MakeBehaviorContext(scope.Behavior()));
}

BBDecl *Require(const ScriptHostScope &scope,
                const std::string &query,
                ScriptBridgeFailureMode failureMode) {
    BBBridge *bridge = From(scope, failureMode);
    if (!bridge) {
        return nullptr;
    }
    BBDecl *decl = bridge->Require(query);
    bridge->Release();
    return decl;
}

BBDecl *Require(const ScriptHostScope &scope,
                CKGUID guid,
                ScriptBridgeFailureMode failureMode) {
    BBBridge *bridge = From(scope, failureMode);
    if (!bridge) {
        return nullptr;
    }
    BBDecl *decl = bridge->RequireGuid(guid);
    bridge->Release();
    return decl;
}

int Count(const ScriptHostScope &scope, ScriptBridgeFailureMode failureMode) {
    BBBridge *bridge = From(scope, failureMode);
    if (!bridge) {
        return 0;
    }
    const int count = bridge->Count();
    bridge->Release();
    return count;
}

BBPrototype *At(const ScriptHostScope &scope,
                int index,
                ScriptBridgeFailureMode failureMode) {
    BBBridge *bridge = From(scope, failureMode);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->At(index);
    bridge->Release();
    return prototype;
}

BBPrototype *Find(const ScriptHostScope &scope,
                  const std::string &query,
                  int occurrence,
                  ScriptBridgeFailureMode failureMode) {
    BBBridge *bridge = From(scope, failureMode);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->Find(query, occurrence);
    bridge->Release();
    return prototype;
}

CScriptArray *FindAll(const ScriptHostScope &scope,
                      const std::string &query,
                      ScriptBridgeFailureMode failureMode) {
    BBBridge *bridge = From(scope, failureMode);
    if (!bridge) {
        return nullptr;
    }
    CScriptArray *prototypes = bridge->FindAll(query);
    bridge->Release();
    return prototypes;
}

} // namespace ScriptBBAdapter

namespace ScriptBehaviorAdapter {

BehaviorBridge *From(const ScriptHostScope &scope) {
    ScriptBehaviorBridge *bridge = scope.Bridge();
    if (!bridge) {
        ReportMissingBehaviorBridge(scope);
        return nullptr;
    }
    return bridge->CreateBehaviorBridge(
        scope.MakeBehaviorContext(BehaviorForScope(scope)));
}

BehaviorGraph *Graph(const ScriptHostScope &scope,
                     const std::string &rootBehaviorName) {
    ScriptBehaviorBridge *bridge = scope.Bridge();
    if (!bridge) {
        ReportMissingBehaviorBridge(scope);
        return nullptr;
    }

    if (scope.Kind() == ScriptHostScopeKind::Behavior) {
        BehaviorBridge *behaviorBridge = From(scope);
        if (!behaviorBridge) {
            return nullptr;
        }
        BehaviorGraph *graph = behaviorBridge->Graph();
        behaviorBridge->Release();
        return graph;
    }

    CKBehavior *root = rootBehaviorName.empty()
        ? nullptr
        : FindRuntimeBehaviorByName(scope, rootBehaviorName);
    return new BehaviorGraph(
        bridge, scope.MakeBehaviorContext(root), root ? root->GetID() : 0);
}

BehaviorRef *Find(const ScriptHostScope &scope, const std::string &name) {
    ScriptBehaviorBridge *bridge = scope.Bridge();
    if (!bridge) {
        ReportMissingBehaviorBridge(scope);
        return nullptr;
    }

    if (scope.Kind() == ScriptHostScopeKind::Behavior) {
        BehaviorBridge *behaviorBridge = From(scope);
        if (!behaviorBridge) {
            return nullptr;
        }
        BehaviorRef *ref = behaviorBridge->Find(name);
        behaviorBridge->Release();
        return ref;
    }

    return bridge->WrapBehavior(FindRuntimeBehaviorByName(scope, name), 0);
}

BehaviorRef *FindById(const ScriptHostScope &scope, CK_ID id) {
    ScriptBehaviorBridge *bridge = scope.Bridge();
    if (!bridge) {
        ReportMissingBehaviorBridge(scope);
        return nullptr;
    }

    if (scope.Kind() == ScriptHostScopeKind::Behavior) {
        BehaviorBridge *behaviorBridge = From(scope);
        if (!behaviorBridge) {
            return nullptr;
        }
        BehaviorRef *ref = behaviorBridge->FindByID(id);
        behaviorBridge->Release();
        return ref;
    }

    CKObject *object = scope.Context() ? CKGetObject(scope.Context(), id) : nullptr;
    return bridge->WrapBehavior(CKBehavior::Cast(object), 0);
}

} // namespace ScriptBehaviorAdapter
