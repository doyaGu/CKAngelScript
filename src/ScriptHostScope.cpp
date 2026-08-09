#include "ScriptHostScope.h"

#include "CKAll.h"
#include "ScriptComponentState.h"
#include "ScriptManager.h"
#include "ScriptMessage.h"
#include "ScriptRuntime.h"

namespace {

CKContext *ResolveBehaviorContext(const CKBehaviorContext &context,
                                  ScriptHostScopeContextResolution resolution) {
    if (context.Context || resolution == ScriptHostScopeContextResolution::DirectContext) {
        return context.Context;
    }
    return context.Behavior ? context.Behavior->GetCKContext() : nullptr;
}

} // namespace

ScriptHostScope ScriptHostScope::From(CKContext *context) {
    ScriptHostScope scope;
    scope.m_Context = context;
    scope.m_Manager = context ? ScriptManager::GetManager(context) : nullptr;
    scope.m_Bridge = scope.m_Manager ? scope.m_Manager->GetBehaviorBridge() : nullptr;
    scope.m_CurrentLevel = context ? context->GetCurrentLevel() : nullptr;
    scope.m_CurrentScene = context ? context->GetCurrentScene() : nullptr;
    return scope;
}

ScriptHostScope ScriptHostScope::From(const ScriptContext &context) {
    ScriptHostScope scope = From(context.Context());
    scope.m_Kind = ScriptHostScopeKind::Runtime;
    scope.m_MessageSource = context.Target();

    const CKBehaviorContext behaviorContext = context.ToBehaviorContext();
    scope.m_Behavior = behaviorContext.Behavior;
    scope.m_ComponentId = scope.m_Behavior ? scope.m_Behavior->GetID() : 0;
    scope.m_Owner = scope.m_Behavior ? scope.m_Behavior->GetOwner() : nullptr;
    scope.m_Target = scope.m_Behavior ? scope.m_Behavior->GetTarget() : nullptr;
    return scope;
}

ScriptHostScope ScriptHostScope::From(const CKBehaviorContext &context,
                                      ScriptHostScopeContextResolution resolution) {
    ScriptHostScope scope = From(ResolveBehaviorContext(context, resolution));
    scope.m_Kind = ScriptHostScopeKind::Behavior;
    scope.m_Behavior = context.Behavior;
    scope.m_ComponentId = context.Behavior ? context.Behavior->GetID() : 0;
    scope.m_Owner = context.Behavior ? context.Behavior->GetOwner() : nullptr;
    scope.m_Target = context.Behavior ? context.Behavior->GetTarget() : nullptr;
    if (context.CurrentLevel) {
        scope.m_CurrentLevel = context.CurrentLevel;
    }
    if (context.CurrentScene) {
        scope.m_CurrentScene = context.CurrentScene;
    }

    ScriptComponentState *state = scope.m_Manager && context.Behavior
        ? scope.m_Manager->GetComponentState(context.Behavior->GetID(), context.Behavior)
        : nullptr;
    scope.m_MessageSource = state ? state->MessageTarget
                                  : (context.Behavior ? ScriptMessageBus::ComponentTarget(context.Behavior->GetID())
                                                      : std::string());
    return scope;
}

ScriptBehaviorBridge *ScriptHostScope::Bridge() const {
    return m_Bridge;
}

ScriptMessageBus *ScriptHostScope::MessageBus() const {
    return m_Manager ? m_Manager->GetMessageBus() : nullptr;
}

ScriptAsyncScheduler *ScriptHostScope::AsyncScheduler() const {
    return m_Manager ? m_Manager->GetAsyncScheduler() : nullptr;
}

ScriptParameterRegistry *ScriptHostScope::ParameterRegistry() const {
    return m_Manager ? m_Manager->GetParameterRegistry() : nullptr;
}
