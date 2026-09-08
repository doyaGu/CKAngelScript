#ifndef CK_SCRIPTHOSTSCOPE_H
#define CK_SCRIPTHOSTSCOPE_H

#include <string>

#include "CKBehavior.h"

class CKContext;
class CKLevel;
class CKScene;
class ScriptAsyncScheduler;
class ScriptBehaviorBridge;
class ScriptContext;
class ScriptManager;
class ScriptMessageBus;
class ScriptParameterRegistry;

enum class ScriptHostScopeKind {
    Raw,
    Runtime,
    Behavior
};

enum class ScriptHostScopeContextResolution {
    DirectContext,
    BehaviorContextFallback
};

class ScriptHostScope {
public:
    static ScriptHostScope From(CKContext *context);
    static ScriptHostScope From(const ScriptContext &context);
    static ScriptHostScope From(
        const CKBehaviorContext &context,
        ScriptHostScopeContextResolution resolution = ScriptHostScopeContextResolution::BehaviorContextFallback);

    CKBehaviorContext MakeBehaviorContext(CKBehavior *behavior) const;

    ScriptHostScopeKind Kind() const { return m_Kind; }
    CKContext *Context() const { return m_Context; }
    ScriptManager *Manager() const { return m_Manager; }
    ScriptBehaviorBridge *Bridge() const;
    ScriptMessageBus *MessageBus() const;
    ScriptAsyncScheduler *AsyncScheduler() const;
    ScriptParameterRegistry *ParameterRegistry() const;

    CKBehavior *Behavior() const { return m_Behavior; }
    CKBeObject *Owner() const { return m_Owner; }
    CKBeObject *Target() const { return m_Target; }
    CKLevel *CurrentLevel() const { return m_CurrentLevel; }
    CKScene *CurrentScene() const { return m_CurrentScene; }
    CK_ID ComponentId() const { return m_ComponentId; }
    const std::string &MessageSource() const { return m_MessageSource; }

    bool IsValid() const { return m_Context != nullptr; }

private:
    ScriptHostScope() = default;

    ScriptHostScopeKind m_Kind = ScriptHostScopeKind::Raw;
    CKContext *m_Context = nullptr;
    ScriptManager *m_Manager = nullptr;
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehavior *m_Behavior = nullptr;
    CKBeObject *m_Owner = nullptr;
    CKBeObject *m_Target = nullptr;
    CKLevel *m_CurrentLevel = nullptr;
    CKScene *m_CurrentScene = nullptr;
    CK_ID m_ComponentId = 0;
    std::string m_MessageSource;
};

#endif // CK_SCRIPTHOSTSCOPE_H
