#include "ScriptSelfTests.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "CKAll.h"
#include "CKAngelScript.h"
#include "AngelScriptExecutor.h"
#include "ScriptAsync.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeCommon.h"
#include "ScriptBridgeHandles.h"
#include "ScriptManager.h"
#include "ScriptRuntime.h"
#include "ScriptExecutorState.h"
#include "ScriptExecutorStateStore.h"

namespace {

constexpr int EXECUTOR_SETTING_SCRIPT = 1;
constexpr int EXECUTOR_SETTING_CLASS = 2;
constexpr int EXECUTOR_SETTING_INTERFACE = 3;
constexpr int EXECUTOR_SETTING_STRICT_INTERFACE = 4;
constexpr int EXECUTOR_SETTING_TRACE = 5;
constexpr int EXECUTOR_USER_LOCAL_START = EXECUTOR_SETTING_TRACE + 1;

CKBehaviorContext MakeExecutorSelfTestContext(CKContext *context, CKBehavior *behavior) {
    CKBehaviorContext behaviorContext = {};
    behaviorContext.Behavior = behavior;
    behaviorContext.DeltaTime = 0.0f;
    behaviorContext.Context = context;
    behaviorContext.ParameterManager = context ? context->GetParameterManager() : nullptr;
    behaviorContext.MessageManager = context ? context->GetMessageManager() : nullptr;
    behaviorContext.AttributeManager = context ? context->GetAttributeManager() : nullptr;
    behaviorContext.TimeManager = context ? context->GetTimeManager() : nullptr;
    return behaviorContext;
}

void DestroyExecutorSelfTestObject(CKContext *context, CKObject *object) {
    if (context && object && !object->IsToBeDeleted()) {
        context->DestroyObject(object);
    }
}

class ScopedEnvVar {
public:
    ScopedEnvVar(const char *name, const std::string &value) : m_Name(name ? name : "") {
        if (m_Name.empty()) {
            return;
        }
        char *oldValue = nullptr;
        size_t oldSize = 0;
        if (_dupenv_s(&oldValue, &oldSize, m_Name.c_str()) == 0 && oldValue) {
            m_HadValue = true;
            m_OldValue = oldValue;
            std::free(oldValue);
        }
        _putenv_s(m_Name.c_str(), value.c_str());
    }

    ~ScopedEnvVar() {
        if (m_Name.empty()) {
            return;
        }
        _putenv_s(m_Name.c_str(), m_HadValue ? m_OldValue.c_str() : "");
    }

private:
    std::string m_Name;
    std::string m_OldValue;
    bool m_HadValue = false;
};

bool WriteExecutorSelfTestFile(const std::filesystem::path &path, const char *text, std::string &error) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        error = "Executor self-test failed to create script entry file: " + path.string();
        return false;
    }
    output << (text ? text : "");
    if (!output) {
        error = "Executor self-test failed to write script entry file: " + path.string();
        return false;
    }
    return true;
}

bool SetExecutorStringSetting(CKBehavior *behavior, int index, const char *value, std::string &error) {
    if (!behavior || index < 0 || index >= behavior->GetLocalParameterCount()) {
        error = "Executor self-test could not resolve string setting.";
        return false;
    }
    CKParameterLocal *setting = behavior->GetLocalParameter(index);
    if (!setting || setting->SetStringValue(const_cast<CKSTRING>(value ? value : "")) != CK_OK) {
        error = "Executor self-test failed to write string setting.";
        return false;
    }
    return true;
}

bool SetExecutorBoolSetting(CKBehavior *behavior, int index, bool value, std::string &error) {
    if (!behavior || index < 0 || index >= behavior->GetLocalParameterCount()) {
        error = "Executor self-test could not resolve bool setting.";
        return false;
    }
    CKBOOL ckValue = value ? TRUE : FALSE;
    if (behavior->SetLocalParameterValue(index, &ckValue) != CK_OK) {
        error = "Executor self-test failed to write bool setting.";
        return false;
    }
    return true;
}

bool TextContains(const std::string &text, const char *needle) {
    return needle && text.find(needle) != std::string::npos;
}

ScriptBridgeFrameLocalDecl FrameLocalDecl(const char *name,
                                          CKGUID guid,
                                          const char *defaultText = nullptr) {
    ScriptBridgeFrameLocalDecl decl;
    decl.Name = name ? name : "";
    decl.TypeGuid = guid;
    if (defaultText) {
        decl.HasDefault = true;
        decl.DefaultText = defaultText;
    }
    return decl;
}

bool ExpectFrameLocalApplyFailure(ScriptBehaviorBridge *bridge,
                                  CKBehavior *behavior,
                                  const std::vector<ScriptBridgeFrameLocalDecl> &decls,
                                  const char *expectedText,
                                  std::string &error) {
    std::string applyError;
    if (!bridge || !behavior) {
        error = "Executor frame-local negative self-test is missing a bridge or behavior.";
        return false;
    }
    if (bridge->ApplyFrameLocalInterface(behavior, decls, 0, true, applyError)) {
        error = "Executor frame-local negative self-test expected interface application to fail.";
        return false;
    }
    if (expectedText && !TextContains(applyError, expectedText)) {
        error = "Executor frame-local negative self-test returned the wrong error. Expected text '" +
                std::string(expectedText) + "', got '" + applyError + "'.";
        return false;
    }
    return true;
}

bool RunFrameLocalBindingNativeSelfTest(CKContext *context,
                                        ScriptBehaviorBridge *bridge,
                                        std::string &error) {
    if (!context || !bridge) {
        error = "Executor frame-local native self-test requires CKContext and ScriptBehaviorBridge.";
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_FrameLocalBindingSelfTestBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    CKParameterLocal *intSource = nullptr;
    CKParameterLocal *otherIntSource = nullptr;
    CKParameterLocal *floatSource = nullptr;
    CKParameterLocal *staleSource = nullptr;

    auto cleanup = [&]() {
        if (behavior) {
            bridge->ClearFrameLocalInterface(behavior->GetID(), true);
        }
        DestroyExecutorSelfTestObject(context, staleSource);
        DestroyExecutorSelfTestObject(context, floatSource);
        DestroyExecutorSelfTestObject(context, otherIntSource);
        DestroyExecutorSelfTestObject(context, intSource);
        DestroyExecutorSelfTestObject(context, behavior);
    };

    if (!behavior) {
        cleanup();
        error = "Executor frame-local native self-test failed to create behavior.";
        return false;
    }

    intSource = context->CreateCKParameterLocal(
        const_cast<CKSTRING>("__CKAS_FrameLocalBindingInt"),
        CKPGUID_INT,
        TRUE);
    otherIntSource = context->CreateCKParameterLocal(
        const_cast<CKSTRING>("__CKAS_FrameLocalBindingOtherInt"),
        CKPGUID_INT,
        TRUE);
    floatSource = context->CreateCKParameterLocal(
        const_cast<CKSTRING>("__CKAS_FrameLocalBindingFloat"),
        CKPGUID_FLOAT,
        TRUE);
    staleSource = context->CreateCKParameterLocal(
        const_cast<CKSTRING>("__CKAS_FrameLocalBindingStale"),
        CKPGUID_INT,
        TRUE);
    if (!intSource || !otherIntSource || !floatSource || !staleSource) {
        cleanup();
        error = "Executor frame-local native self-test failed to create source parameters.";
        return false;
    }

    int intValue = 11;
    int otherIntValue = 12;
    float floatValue = 3.5f;
    int staleValue = 13;
    if (intSource->SetValue(&intValue, sizeof(intValue)) != CK_OK ||
        otherIntSource->SetValue(&otherIntValue, sizeof(otherIntValue)) != CK_OK ||
        floatSource->SetValue(&floatValue, sizeof(floatValue)) != CK_OK ||
        staleSource->SetValue(&staleValue, sizeof(staleValue)) != CK_OK) {
        cleanup();
        error = "Executor frame-local native self-test failed to initialize source values.";
        return false;
    }

    ParamRef *intRef = bridge->WrapParameter(intSource);
    ParamRef *otherIntRef = bridge->WrapParameter(otherIntSource);
    ParamRef *floatRef = bridge->WrapParameter(floatSource);
    ParamRef *staleRef = bridge->WrapParameter(staleSource);
    CKParameterIn *unconnectedPin = behavior->CreateInputParameter(
        const_cast<CKSTRING>("__CKAS_FrameLocalBindingPin"),
        CKPGUID_INT);
    ParamRef *pinRef = bridge->WrapParameter(unconnectedPin);
    auto releaseRefs = [&]() {
        if (pinRef) pinRef->Release();
        if (staleRef) staleRef->Release();
        if (floatRef) floatRef->Release();
        if (otherIntRef) otherIntRef->Release();
        if (intRef) intRef->Release();
    };
    if (!intRef || !otherIntRef || !floatRef || !staleRef || !pinRef) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test failed to wrap source parameters.";
        return false;
    }

    std::string bindError;
    if (bridge->BindFrameLocal(behavior, "Missing", CKPGUID_INT, nullptr, "missing", bindError) ||
        !TextContains(bindError, "writable real source")) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test did not reject a null source binding.";
        return false;
    }

    bindError.clear();
    if (bridge->BindFrameLocal(behavior, "MissingPinSource", CKPGUID_INT, pinRef, "unconnected pin", bindError) ||
        !TextContains(bindError, "writable real source")) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test did not reject an unconnected CKParameterIn binding.";
        return false;
    }

    bindError.clear();
    if (bridge->BindFrameLocal(behavior, "Mapped", CKPGUID_INT, floatRef, "wrong type", bindError) ||
        !TextContains(bindError, "type mismatch")) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test did not reject a source type mismatch.";
        return false;
    }

    bindError.clear();
    if (!bridge->BindFrameLocal(behavior, "Mapped", CKPGUID_INT, intRef, "int source", bindError)) {
        releaseRefs();
        cleanup();
        error = bindError.empty()
            ? "Executor frame-local native self-test failed to bind a valid mapped local."
            : bindError;
        return false;
    }

    bindError.clear();
    if (!bridge->BindFrameLocal(behavior, "Mapped", CKPGUID_INT, intRef, "int source update", bindError)) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test rejected an idempotent binding update: " + bindError;
        return false;
    }

    bindError.clear();
    if (bridge->BindFrameLocal(behavior, "Mapped", CKPGUID_INT, otherIntRef, "conflicting int source", bindError) ||
        !TextContains(bindError, "conflicts")) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test did not reject a conflicting duplicate binding.";
        return false;
    }

    if (!bridge->ApplyFrameLocalInterface(
            behavior,
            {FrameLocalDecl("Owned", CKPGUID_INT, "4"), FrameLocalDecl("Mapped", CKPGUID_INT)},
            0,
            true,
            error)) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test failed to apply a valid interface: " + error;
        return false;
    }
    if (bridge->FrameLocalCount(behavior->GetID()) != 2 ||
        bridge->FindFrameLocal(behavior->GetID(), "Owned", 0) != 0 ||
        bridge->FindFrameLocal(behavior->GetID(), "Mapped", 0) != 1 ||
        behavior->GetLocalParameterCount() != 1) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test installed the wrong logical/physical local shape.";
        return false;
    }
    std::string resolveError;
    CKParameter *mapped = bridge->ResolveFrameLocal(context, behavior->GetID(), 1, resolveError);
    if (mapped != intSource) {
        releaseRefs();
        cleanup();
        error = resolveError.empty()
            ? "Executor frame-local native self-test mapped local did not resolve to the original parameter."
            : resolveError;
        return false;
    }

    bridge->ClearFrameLocalInterface(behavior->GetID(), true);
    const std::vector<ScriptBridgeFrameLocalDecl> lateDecls = {
        FrameLocalDecl("LateMapped", CKPGUID_INT, "9"),
        FrameLocalDecl("LateOwned", CKPGUID_INT, "21"),
    };
    if (!bridge->ApplyFrameLocalInterface(
            behavior,
            lateDecls,
            0,
            true,
            error)) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test failed to apply a late-binding interface: " + error;
        return false;
    }
    if (behavior->GetLocalParameterCount() != 2) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test did not create owned locals before late binding.";
        return false;
    }
    bindError.clear();
    if (!bridge->BindFrameLocal(behavior, "LateMapped", CKPGUID_INT, intRef, "late mapped source", bindError)) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test rejected a late local binding: " + bindError;
        return false;
    }
    if (behavior->GetLocalParameterCount() != 1) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test did not compact owned locals after late binding.";
        return false;
    }
    resolveError.clear();
    CKParameter *lateMapped = bridge->ResolveFrameLocal(context, behavior->GetID(), 0, resolveError);
    if (lateMapped != intSource) {
        releaseRefs();
        cleanup();
        error = resolveError.empty()
            ? "Executor frame-local native self-test late binding did not resolve to the original parameter."
            : resolveError;
        return false;
    }
    resolveError.clear();
    CKParameter *lateOwned = bridge->ResolveFrameLocal(context, behavior->GetID(), 1, resolveError);
    int lateOwnedValue = 0;
    if (!lateOwned ||
        lateOwned->GetValue(&lateOwnedValue) != CK_OK ||
        lateOwnedValue != 21 ||
        behavior->GetLocalParameter(0) != lateOwned) {
        releaseRefs();
        cleanup();
        error = resolveError.empty()
            ? "Executor frame-local native self-test late-owned local kept the wrong physical/default state."
            : resolveError;
        return false;
    }

    bridge->ClearFrameLocalInterface(behavior->GetID(), false);
    if (bridge->HasFrameLocalInterface(behavior->GetID())) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test reset-style clear kept stale interface layout.";
        return false;
    }
    if (!bridge->ApplyFrameLocalInterface(behavior, lateDecls, 0, true, error)) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test failed to reapply locals after reset-style clear: " + error;
        return false;
    }
    resolveError.clear();
    lateMapped = bridge->ResolveFrameLocal(context, behavior->GetID(), 0, resolveError);
    resolveError.clear();
    lateOwned = bridge->ResolveFrameLocal(context, behavior->GetID(), 1, resolveError);
    if (lateMapped != intSource || !lateOwned || behavior->GetLocalParameterCount() != 1 || behavior->GetLocalParameter(0) != lateOwned) {
        releaseRefs();
        cleanup();
        error = resolveError.empty()
            ? "Executor frame-local native self-test lost mapped locals across reset-style clear."
            : resolveError;
        return false;
    }

    std::string rollbackError;
    ScriptBehaviorBridgeFailNextOwnedLocalSyncForSelfTest();
    if (bridge->ApplyFrameLocalInterface(behavior, lateDecls, -1, true, rollbackError) ||
        !bridge->ApplyFrameLocalInterface(behavior, lateDecls, 0, true, rollbackError)) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local self-test leaked an injected failure past an invalid snapshot: " + rollbackError;
        return false;
    }
    CKParameterLocal *rollbackLocal = behavior->GetLocalParameter(0);
    ScriptBehaviorBridgeFailNextOwnedLocalSyncForSelfTest();
    if (bridge->ApplyFrameLocalInterface(
            behavior,
            {FrameLocalDecl("Changed", CKPGUID_INT, "31")},
            0,
            false,
            rollbackError)) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test expected invalid default application to fail.";
        return false;
    }
    lateOwned = behavior->GetLocalParameter(0);
    lateOwnedValue = 0;
    if (behavior->GetLocalParameterCount() != 1 ||
        !lateOwned ||
        lateOwned != rollbackLocal ||
        std::string(lateOwned->GetName() ? lateOwned->GetName() : "") != "LateOwned" ||
        lateOwned->GetGUID() != CKPGUID_INT ||
        lateOwned->GetValue(&lateOwnedValue) != CK_OK ||
        lateOwnedValue != 21) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test did not roll back a failed local mutation: " + rollbackError;
        return false;
    }

    bridge->ClearFrameLocalInterface(behavior->GetID(), true);
    if (!ExpectFrameLocalApplyFailure(
            bridge,
            behavior,
            {FrameLocalDecl("Dup", CKPGUID_INT), FrameLocalDecl("Dup", CKPGUID_INT)},
            "duplicate local",
            error)) {
        releaseRefs();
        cleanup();
        return false;
    }

    bindError.clear();
    if (!bridge->BindFrameLocal(behavior, "Undeclared", CKPGUID_INT, intRef, "undeclared", bindError)) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test failed to install undeclared binding: " + bindError;
        return false;
    }
    if (!ExpectFrameLocalApplyFailure(
            bridge,
            behavior,
            {FrameLocalDecl("Declared", CKPGUID_INT)},
            "no matching interface local declaration",
            error)) {
        releaseRefs();
        cleanup();
        return false;
    }

    bridge->ClearFrameLocalInterface(behavior->GetID(), true);
    bindError.clear();
    if (!bridge->BindFrameLocal(behavior, "Gone", CKPGUID_INT, staleRef, "stale", bindError)) {
        releaseRefs();
        cleanup();
        error = "Executor frame-local native self-test failed to install stale-source binding: " + bindError;
        return false;
    }
    DestroyExecutorSelfTestObject(context, staleSource);
    staleSource = nullptr;
    if (!ExpectFrameLocalApplyFailure(
            bridge,
            behavior,
            {FrameLocalDecl("Gone", CKPGUID_INT)},
            "source is invalid",
            error)) {
        releaseRefs();
        cleanup();
        return false;
    }

    releaseRefs();
    cleanup();
    error.clear();
    return true;
}

int ExecuteExecutorSelfTestBehavior(CKContext *context, CKBehavior *behavior, const CKBehaviorContext &behaviorContext) {
    if (!behavior) {
        return CKBR_BEHAVIORERROR;
    }

#if CKVERSION == 0x13022002
    CKBehaviorContext savedContext = context->m_BehaviorContext;
    CKBehaviorManager *behaviorManager = context ? context->GetBehaviorManager() : nullptr;
    CKBehavior *savedCurrent = behaviorManager ? behaviorManager->m_CurrentBehavior : nullptr;

    context->m_BehaviorContext = behaviorContext;
    const int rc = behavior->Execute(behaviorContext.DeltaTime);

    context->m_BehaviorContext = savedContext;
    if (behaviorManager) {
        behaviorManager->m_CurrentBehavior = savedCurrent;
    }
    return rc;
#else
    return behavior->Execute(behaviorContext.DeltaTime);
#endif
}

bool RunExecutorStateStoreIsolationSelfTest(CKContext *context,
                                            ScriptManager *manager,
                                            std::string &error) {
    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorStateStoreIsolation"),
        CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        error = "Executor state store self-test could not create a behavior.";
        return false;
    }

    ScriptExecutorStateStore first;
    ScriptExecutorStateStore second;
    ScriptExecutorState *firstState = first.GetOrCreate(behavior, manager, {});
    ScriptExecutorState *secondState = second.GetOrCreate(behavior, manager, {});
    bool ok = firstState && secondState && firstState != secondState &&
              first.Get(behavior->GetID(), behavior) == firstState &&
              second.Get(behavior->GetID(), behavior) == secondState;
    int removals = 0;
    CKBehavior *foreignBehavior = reinterpret_cast<CKBehavior *>(static_cast<std::uintptr_t>(1));
    ok = ok && !first.Remove(behavior->GetID(), foreignBehavior, [&removals](CK_ID, ScriptExecutorState *) {
        ++removals;
    });
    ok = ok && first.Get(behavior->GetID(), behavior) == firstState && removals == 0;
    ok = ok && first.Remove(behavior->GetID(), behavior, [&removals](CK_ID, ScriptExecutorState *) {
        ++removals;
    });
    ok = ok && removals == 1 && first.Get(behavior->GetID()) == nullptr &&
         second.Get(behavior->GetID(), behavior) == secondState;
    secondState->ModuleName = "__CKAS_ExecutorStateStoreIsolation";
    int moduleReleases = 0;
    second.ReleaseModule("__CKAS_OtherModule", [&moduleReleases](CK_ID, ScriptExecutorState *) {
        ++moduleReleases;
    });
    ok = ok && moduleReleases == 0 && second.Get(behavior->GetID(), behavior) == secondState;
    second.ReleaseModule("__CKAS_ExecutorStateStoreIsolation", [&moduleReleases](CK_ID, ScriptExecutorState *) {
        ++moduleReleases;
    });
    ok = ok && moduleReleases == 1 && second.Get(behavior->GetID()) == nullptr;
    secondState = second.GetOrCreate(behavior, manager, {});
    int clearReleases = 0;
    second.Clear([&clearReleases](CK_ID, ScriptExecutorState *) {
        ++clearReleases;
    });
    ok = ok && secondState && clearReleases == 1 && second.Get(behavior->GetID()) == nullptr;
    DestroyExecutorSelfTestObject(context, behavior);
    if (!ok) {
        error = "Executor state stores did not isolate ownership.";
    }
    return ok;
}

bool RunExecutorCreateStateRebindSelfTest(CKContext *context,
                                          ScriptManager *manager,
                                          std::string &error) {
    CKBehavior *source = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorStateRebindSource"),
        CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *target = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorStateRebindTarget"),
        CK_OBJECTCREATION_DYNAMIC));
    auto cleanup = [&]() {
        ScriptExecutorState *nullState = nullptr;
        if (target && target->GetLocalParameterCount() > 0) {
            target->SetLocalParameterValue(0, &nullState);
            manager->ReleaseExecutorState(target);
        }
        if (source && source->GetLocalParameterCount() > 0) {
            source->SetLocalParameterValue(0, &nullState);
            manager->ReleaseExecutorState(source);
        }
        DestroyExecutorSelfTestObject(context, target);
        DestroyExecutorSelfTestObject(context, source);
    };

    if (!source || !target) {
        cleanup();
        error = "Executor state rebind self-test could not create its behaviors.";
        return false;
    }
    source->UseFunction();
    target->UseFunction();
    const CKGUID executorGuid(0x15a41762, 0x0c7c4a91);
    if (source->InitFromGuid(executorGuid) != CK_OK || target->InitFromGuid(executorGuid) != CK_OK) {
        cleanup();
        error = "Executor state rebind self-test could not initialize Executor behaviors.";
        return false;
    }

    ScriptExecutorState *sourceState = manager->GetOrCreateExecutorState(source);
    if (!sourceState || target->SetLocalParameterValue(0, &sourceState) != CK_OK) {
        cleanup();
        error = "Executor state rebind self-test could not install its foreign state pointer.";
        return false;
    }

    CKBehaviorContext targetContext = MakeExecutorSelfTestContext(context, target);
    if (CallBridgeBehaviorCallback(target, CKM_BEHAVIORCREATE, &targetContext) != CK_OK) {
        cleanup();
        error = "Executor state rebind self-test CREATE callback failed.";
        return false;
    }

    ScriptExecutorState *boundState = nullptr;
    target->GetLocalParameterValue(0, &boundState);
    ScriptExecutorState *targetState = manager->GetExecutorState(target->GetID(), target);
    const bool rebound = targetState && targetState != sourceState && boundState == targetState;
    cleanup();
    if (!rebound) {
        error = "Executor CREATE callback retained a foreign behavior state pointer.";
        return false;
    }
    return true;
}

} // namespace

bool RunScriptExecutorSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Executor self-test requires a CKContext and AngelScript engine.";
        return false;
    }

    ScriptManager *manager = ScriptManager::GetManager(context);
    if (!manager) {
        error = "Executor self-test could not retrieve ScriptManager.";
        return false;
    }
    asITypeInfo *frameType = engine->GetTypeInfoByDecl("BehaviorFrame");
    const char *requiredSizedDataMethods[] = {
        "NativeBuffer@ PinReadData(int index) const",
        "NativeBuffer@ PoutWriteData(int index) const",
        "NativeBuffer@ LocalReadData(int index) const",
        "NativeBuffer@ LocalWriteData(int index) const",
        "NativeBuffer@ BehaviorLocalReadData(int index) const",
        "NativeBuffer@ BehaviorLocalWriteData(int index) const",
        "NativeBuffer@ GetInputParameterReadData(int index) const",
        "NativeBuffer@ GetOutputParameterWriteData(int index) const",
        "NativeBuffer@ GetLocalParameterReadData(int index) const",
        "NativeBuffer@ GetLocalParameterWriteData(int index) const",
    };
    if (!frameType) {
        error = "Executor self-test could not resolve BehaviorFrame.";
        return false;
    }
    for (const char *decl : requiredSizedDataMethods) {
        if (!frameType->GetMethodByDecl(decl)) {
            error = std::string("Executor self-test could not resolve sized BehaviorFrame method: ") + decl;
            return false;
        }
    }
    const char *forbiddenRawDataMethods[] = {
        "NativePointer PinReadDataPtr(int index) const",
        "NativePointer PoutWriteDataPtr(int index) const",
        "NativePointer LocalReadDataPtr(int index) const",
        "NativePointer LocalWriteDataPtr(int index) const",
        "NativePointer BehaviorLocalReadDataPtr(int index) const",
        "NativePointer BehaviorLocalWriteDataPtr(int index) const",
        "NativePointer GetInputParameterReadDataPtr(int index) const",
        "NativePointer GetOutputParameterWriteDataPtr(int index) const",
        "NativePointer GetLocalParameterReadDataPtr(int index) const",
        "NativePointer GetLocalParameterWriteDataPtr(int index) const",
    };
    for (const char *decl : forbiddenRawDataMethods) {
        if (frameType->GetMethodByDecl(decl)) {
            error = std::string("Executor self-test found removed raw BehaviorFrame method: ") + decl;
            return false;
        }
    }
    if (!RunExecutorStateStoreIsolationSelfTest(context, manager, error)) {
        return false;
    }
    if (!RunExecutorCreateStateRebindSelfTest(context, manager, error)) {
        return false;
    }
    ScriptBehaviorBridge *bridge = manager->GetBehaviorBridge();
    if (!RunFrameLocalBindingNativeSelfTest(context, bridge, error)) {
        return false;
    }

    constexpr const char *scriptId = "ckas.executor.selftest";
    constexpr const char *className = "ExecutorProbe";
    const char *source =
        "class ExecutorProbe {\n"
        "  int Execute(BehaviorFrame@ frame) {\n"
        "    if (frame is null) return 101;\n"
        "    if (frame.Behavior() is null) return 102;\n"
        "    if (!frame.valid || frame.id == 0 || frame.name.length() == 0) return 115;\n"
        "    CKBehaviorContext frameCtx = frame.ToBehaviorContext();\n"
        "    if (frameCtx.Behavior is null || frameCtx.Context is null) return 116;\n"
        "    if (frame.Context() is null || frame.ParameterManager() is null || frame.MessageManager() is null || frame.AttributeManager() is null || frame.TimeManager() is null) return 117;\n"
        "    if (frame.InputCount() != 1 || frame.OutputCount() != 1 || frame.PinCount() != 1 || frame.PoutCount() != 1) return 118;\n"
        "    if (frame.InputName(0) != \"In\" || frame.OutputName(0) != \"Out\") return 119;\n"
        "    if (frame.FindInput(\"In\") != 0 || frame.FindOutput(\"Out\") != 0 || frame.FindPin(\"Value\") != 0 || frame.FindPout(\"Handled\") != 0) return 120;\n"
        "    if (frame.Input(0) is null || frame.Output(0) is null) return 121;\n"
        "    if (frame.Pin(0) is null || frame.Pin(\"Value\") is null || frame.Pout(0) is null || frame.Pout(\"Handled\") is null) return 122;\n"
        "    if (!frame.PinEnabled(0) || !frame.PinEnabled(\"Value\") || !frame.PoutEnabled(0) || !frame.PoutEnabled(\"Handled\")) return 123;\n"
        "    frame.EnablePin(0, false); if (frame.PinEnabled(0)) return 124; frame.EnablePin(\"Value\", true); if (!frame.PinEnabled(0)) return 125;\n"
        "    frame.EnablePout(0, false); if (frame.PoutEnabled(0)) return 126; frame.EnablePout(\"Handled\", true); if (!frame.PoutEnabled(0)) return 127;\n"
        "    BehaviorRef@ self = frame.Self(); if (self is null || !self.valid || self.id != frame.id) return 128;\n"
        "    if (frame.Layout() is null || self.Layout() is null || frame.Graph() is null || frame.AsGraph() is null) return 129;\n"
        "    CKBehavior@ raw = frame.Behavior();\n"
        "    if (raw is null) return 143;\n"
        "    CKObject@ rawObject = frame.CKGetObject(frame.id); if (rawObject is null || rawObject.GetID() != frame.id) return 166;\n"
        "    if (frame.GetParent() !is raw.GetParent() || frame.GetOwnerScript() !is raw.GetOwnerScript()) return 159;\n"
        "    if (frame.GetSubBehaviorCount() > 0 && frame.GetSubBehavior(0) !is raw.GetSubBehavior(0)) return 160;\n"
        "    frame.ClassId(); frame.ObjectFlags(); frame.IsDynamic(); frame.IsToBeDeleted(); frame.IsNotToBeSaved(); frame.IsPrivate(); frame.IsInterfaceObj(); frame.IsUpToDate(); frame.MemoryOccupation(); frame.AppData();\n"
        "    frame.SceneInCount(); frame.SceneIn(0); frame.IsInScene(frame.CurrentScene()); frame.IsActiveInCurrentScene(); frame.IsActiveInScene(frame.CurrentScene()); frame.IsVisible(); frame.CanBeHide(); frame.IsHiddenByParent(); frame.IsHierarchicallyHide(); frame.IsObjectUsed(raw, raw.GetClassID());\n"
        "    if (frame.GetCKContext() is null || frame.GetID() != frame.id || frame.GetName() != frame.name || frame.GetClassID() != frame.ClassId()) return 154;\n"
        "    if (frame.GetInputCount() != frame.InputCount() || frame.GetOutputCount() != frame.OutputCount() || frame.GetInputParameterCount() != frame.PinCount() || frame.GetOutputParameterCount() != frame.PoutCount()) return 155;\n"
        "    if (frame.GetLocalParameterCount() != frame.BehaviorLocalCount() || frame.LocalCount() != 2) return 175;\n"
        "    if (!frame.IsInputActive(0) || frame.IsOutputActive(0) || !frame.IsInputParameterEnabled(0) || !frame.IsOutputParameterEnabled(0)) return 156;\n"
        "    if (frame.GetInput(0) is null || frame.GetOutput(0) is null) return 157;\n"
        "    if (frame.InputPosition(raw.GetInput(0)) != 0 || frame.OutputPosition(raw.GetOutput(0)) != 0) return 144;\n"
        "    if (frame.InputParameter(0) is null || frame.OutputParameter(0) is null) return 151;\n"
        "    if (frame.GetInputParameter(0) is null || frame.GetOutputParameter(0) is null) return 152;\n"
        "    if (frame.GetInputParameterPosition(frame.GetInputParameter(0)) != 0 || frame.GetOutputParameterPosition(frame.GetOutputParameter(0)) != 0) return 158;\n"
        "    if (frame.PinPosition(frame.InputParameter(0)) != 0 || frame.PoutPosition(frame.OutputParameter(0)) != 0) return 145;\n"
        "    NativeBuffer@ pinData = frame.PinReadData(0); NativeBuffer@ sdkPinData = frame.GetInputParameterReadData(0);\n"
        "    int bufferedPinValue = 0; if (pinData is null || sdkPinData is null || pinData.Size() != 4 || sdkPinData.Size() != 4 || pinData.Read(bufferedPinValue) != 4 || bufferedPinValue != 41) return 182;\n"
        "    NativeBuffer@ poutData = frame.PoutWriteData(0); NativeBuffer@ sdkPoutData = frame.GetOutputParameterWriteData(0); if (poutData is null || sdkPoutData is null || poutData.Size() != 4 || sdkPoutData.Size() != 4) return 183;\n"
        "    int sdkPinValue = 0;\n"
        "    if (frame.GetInputParameterValue(0, sdkPinValue) != CK_OK || sdkPinValue != 41) return 161;\n"
        "    frame.Function(); frame.InterfaceChunk();\n"
        "    frame.Type(); frame.Flags(); frame.PrototypeGuid(); frame.PrototypeName(); frame.Version(); frame.Priority(); frame.LastExecutionTime(); frame.IsActive(); frame.IsUsingFunction(); frame.IsTargetable(); frame.IsUsingTarget(); frame.CompatibleClassId();\n"
        "    frame.SubBehaviorCount(); frame.SubBehaviorLinkCount(); frame.ParameterOperationCount(); frame.CurrentLevel(); frame.CurrentScene(); frame.PreviousScene(); frame.CurrentRenderContext(); frame.CallbackMessage();\n"
        "    if (!frame.InputActive(0) || !frame.InputActive(\"In\")) return 130;\n"
        "    if (frame.OutputActive(0) || frame.OutputActive(\"Out\")) return 131;\n"
        "    int scratchLocal = frame.FindLocal(\"__CKAS_FrameScratch\");\n"
        "    if (scratchLocal < 0 || frame.Local(scratchLocal) is null || frame.Local(\"__CKAS_FrameScratch\") is null) return 132;\n"
        "    int scratchPhysical = frame.FindBehaviorLocal(\"__CKAS_FrameScratch\");\n"
        "    if (scratchLocal != 0 || scratchPhysical != 6 || frame.LocalInt(\"__CKAS_FrameScratch\", -1) != 7) return 174;\n"
        "    int mappedLocal = frame.FindLocal(\"__CKAS_MappedLocal\");\n"
        "    if (mappedLocal != 1 || frame.Local(mappedLocal) is null || frame.Local(\"__CKAS_MappedLocal\") is null) return 176;\n"
        "    if (frame.FindBehaviorLocal(\"__CKAS_MappedLocal\") >= 0) return 177;\n"
        "    if (frame.LocalInt(\"__CKAS_MappedLocal\", -1) != 301) return 178;\n"
        "    if (!frame.SetLocal(\"__CKAS_MappedLocal\", 302) || !frame.SetLocal(mappedLocal, 303)) return 179;\n"
        "    if (frame.LocalInt(\"__CKAS_MappedLocal\", -1) != 303 || frame.LocalInt(mappedLocal, -1) != 303) return 180;\n"
        "    int sdkLocalValue = 0;\n"
        "    if (frame.SetLocalParameterValue(scratchPhysical, 222) != CK_OK) return 162;\n"
        "    if (frame.GetLocalParameterValue(scratchPhysical, sdkLocalValue) != CK_OK || sdkLocalValue != 222) return 163;\n"
        "    if (!frame.SetLocal(scratchLocal, 123) || !frame.SetLocal(\"__CKAS_FrameScratch\", 124)) return 133;\n"
        "    if (frame.LocalInt(\"__CKAS_FrameScratch\", -1) != 124 || frame.LocalInt(scratchLocal, -1) != 124) return 168;\n"
        "    if (frame.LocalInt(\"__CKAS_MissingLocal\", 77) != 77) return 169;\n"
        "    if (frame.LocalString(\"__CKAS_FrameScratch\", \"fallback\") != \"fallback\") return 170;\n"
        "    if (!frame.LocalBool(\"__CKAS_FrameScratch\", true)) return 171;\n"
        "    if (frame.LocalFloat(\"__CKAS_FrameScratch\", -1.0f) != 124.0f || frame.LocalFloat(scratchLocal, -1.0f) != 124.0f) return 172;\n"
        "    if (frame.LocalObject(\"__CKAS_MissingLocal\") !is null) return 173;\n"
        "    if (frame.BehaviorLocalParameter(scratchPhysical) is null || frame.GetLocalParameter(scratchPhysical) is null) return 153;\n"
        "    if (frame.LocalPosition(frame.BehaviorLocalParameter(scratchPhysical)) != scratchPhysical || frame.GetLocalParameterPosition(frame.GetLocalParameter(scratchPhysical)) != scratchPhysical) return 147;\n"
        "    NativeBuffer@ localRead = frame.LocalReadData(scratchLocal); NativeBuffer@ localWrite = frame.LocalWriteData(scratchLocal); if (localRead is null || localWrite is null || localRead.Size() != 4 || localWrite.Size() != 4) return 184;\n"
        "    NativeBuffer@ physicalRead = frame.BehaviorLocalReadData(scratchPhysical); NativeBuffer@ physicalWrite = frame.BehaviorLocalWriteData(scratchPhysical); NativeBuffer@ sdkLocalRead = frame.GetLocalParameterReadData(scratchPhysical); NativeBuffer@ sdkLocalWrite = frame.GetLocalParameterWriteData(scratchPhysical);\n"
        "    if (physicalRead is null || physicalWrite is null || sdkLocalRead is null || sdkLocalWrite is null || physicalRead.Size() != 4 || physicalWrite.Size() != 4 || sdkLocalRead.Size() != 4 || sdkLocalWrite.Size() != 4) return 185;\n"
        "    frame.LocalObject(scratchLocal);\n"
        "    frame.NotifyEdition(); frame.NotifySettingsEdition();\n"
        "    BehaviorLayout@ runtimeLayout = frame.Layout();\n"
        "    int traceSetting = frame.FindSetting(\"Trace\");\n"
        "    if (frame.SettingCount() <= 0 || runtimeLayout is null || traceSetting < 0 || traceSetting != runtimeLayout.FindSetting(\"Trace\")) return 134;\n"
        "    ParamRef@ traceRef = frame.Setting(traceSetting);\n"
        "    if (traceRef is null || traceRef.index != traceSetting || !frame.IsSetting(traceSetting)) return 140;\n"
        "    if (!frame.SetSetting(\"Trace\", true) || !traceRef.Get().AsBool()) return 141;\n"
        "    if (!frame.SetSetting(traceSetting, false) || traceRef.Get().AsBool()) return 142;\n"
        "    if (!frame.Take(0)) return 103;\n"
        "    if (frame.InputActive(\"In\")) return 135;\n"
        "    int value = frame.PinInt(\"Value\", -1);\n"
        "    if (value != 41) return 104;\n"
        "    if (frame.PinInt(0, -1) != 41) return 136;\n"
        "    if (frame.PinInt(\"Missing\", 42) != 42) return 106;\n"
        "    if (frame.PinString(\"MissingText\", \"fallback\") != \"fallback\") return 107;\n"
        "    if (!frame.PinBool(\"MissingBool\", true)) return 108;\n"
        "    if (frame.PinFloat(\"MissingFloat\", 3.5f) != 3.5f) return 109;\n"
        "    if (frame.PinString(0, \"int-fallback\") != \"int-fallback\" || frame.PinBool(0, true) != true || frame.PinFloat(0, -1.0f) != 41.0f) return 137;\n"
        "    if (frame.PinBool(\"Value\", false)) return 110;\n"
        "    if (frame.PinString(\"Value\", \"int-fallback\") != \"int-fallback\") return 114;\n"
        "    AsyncTask<void>@ delay = frame.LinkDelay(1);\n"
        "    if (delay is null) return 167;\n"
        "    Await(delay);\n"
        "    if (frame.Context() is null) return 111;\n"
        "    if (frame.Behavior() is null) return 112;\n"
        "    if (frame.Self() is null || frame.Layout() is null) return 138;\n"
        "    if (frame.PinInt(\"MissingAfterResume\", 43) != 43) return 113;\n"
        "    bool sdkPoutValue = false;\n"
        "    if (frame.SetOutputParameterValue(0, true) != CK_OK) return 164;\n"
        "    if (frame.GetOutputParameterValue(0, sdkPoutValue) != CK_OK || !sdkPoutValue) return 165;\n"
        "    if (!frame.SetPout(\"Handled\", false) || !frame.SetPout(0, true)) return 105;\n"
        "    frame.Emit(0);\n"
        "    if (!frame.OutputActive(\"Out\")) return 139;\n"
        "    return 0;\n"
        "  }\n"
        "  void OnDestroy(BehaviorFrame@ frame) {\n"
        "    if (frame is null || frame.LocalCount() != 2) return;\n"
        "    frame.SetLocal(\"__CKAS_FrameScratch\", 901);\n"
        "    frame.SetLocal(\"__CKAS_MappedLocal\", 902);\n"
        "  }\n"
        "  int Execute(const CKBehaviorContext &in ctx) {\n"
        "    return 1901;\n"
        "  }\n"
        "}\n"
        "class ExecutorRetriggerProbe {\n"
        "  int starts = 0;\n"
        "  int resets = 0;\n"
        "  void Start(BehaviorFrame@ frame) {\n"
        "    if (frame !is null) frame.SetLocal(\"__CKAS_FrameScratch\", 779);\n"
        "  }\n"
        "  int Execute(BehaviorFrame@ frame) {\n"
        "    ++starts;\n"
        "    if (frame is null) return 201;\n"
        "    if (!frame.InputActive(\"In\")) return 202;\n"
        "    if (!frame.Take(\"In\")) return 203;\n"
        "    if (starts == 1) {\n"
        "      AsyncTask<void>@ delay = frame.LinkDelay(10);\n"
        "      if (delay is null) return 204;\n"
        "      Await(delay);\n"
        "      if (!frame.SetPout(\"Handled\", false)) return 205;\n"
        "      frame.Emit(\"Out\");\n"
        "      return 206;\n"
        "    }\n"
        "    if (starts == 2) {\n"
        "      if (!frame.SetPout(\"Handled\", true)) return 207;\n"
        "      frame.Emit(\"Out\");\n"
        "      return 0;\n"
        "    }\n"
        "    return 208;\n"
        "  }\n"
        "  void OnReset(BehaviorFrame@ frame) {\n"
        "    ++resets;\n"
        "    if (resets == 1) { if (frame !is null) frame.SetLocal(\"__CKAS_FrameScratch\", 777); return; }\n"
        "    array<int> values; values[1] = 1;\n"
        "  }\n"
        "  void OnDestroy(BehaviorFrame@ frame) {\n"
        "    if (frame !is null) frame.SetLocal(\"__CKAS_FrameScratch\", 778);\n"
        "  }\n"
        "}\n"
        "class ExecutorSuspendingSourceProbe {\n"
        "  int Execute(BehaviorFrame@ frame) {\n"
        "    if (frame is null) return 301;\n"
        "    if (!frame.Take(\"In\")) return 302;\n"
        "    AsyncTask<void>@ delay = frame.LinkDelay(1);\n"
        "    if (delay is null) return 303;\n"
        "    Await(delay);\n"
        "    frame.Emit(\"Out\");\n"
        "    return 0;\n"
        "  }\n"
        "}\n"
        "class ExecutorSuspendingDownstreamProbe {\n"
        "  int Execute(BehaviorFrame@ frame) {\n"
        "    if (frame is null) return 401;\n"
        "    if (!frame.Take(\"In\")) return 402;\n"
        "    AsyncTask<void>@ delay = frame.LinkDelay(1);\n"
        "    if (delay is null) return 403;\n"
        "    Await(delay);\n"
        "    int count = frame.LocalInt(\"__CKAS_FrameScratch\", 0);\n"
        "    if (!frame.SetLocal(\"__CKAS_FrameScratch\", count + 1)) return 404;\n"
        "    frame.Emit(\"Out\");\n"
        "    return 0;\n"
        "  }\n"
        "}\n";

    const std::filesystem::path scriptRoot =
        std::filesystem::temp_directory_path() / "__ckas_executor_runtime_selftest";
    const std::filesystem::path packageDir = scriptRoot / "Scripts" / scriptId;
    constexpr const char *reloadScriptId = "ckas.executor.reload.selftest";
    const std::filesystem::path reloadPackageDir = scriptRoot / "Scripts" / reloadScriptId;
    const char *reloadSourceV1 =
        "class ExecutorReloadProbe {\n"
        "  int Execute(BehaviorFrame@ frame) {\n"
        "    if (frame is null || !frame.Take(\"In\")) return 501;\n"
        "    if (!frame.SetLocal(\"Version\", 1)) return 502;\n"
        "    frame.Emit(\"Out\");\n"
        "    return 0;\n"
        "  }\n"
        "}\n";
    const char *reloadSourceV2 =
        "class ExecutorReloadProbe {\n"
        "  int Execute(BehaviorFrame@ frame) {\n"
        "    if (frame is null || !frame.Take(\"In\")) return 511;\n"
        "    if (!frame.SetLocal(\"Version\", 2)) return 512;\n"
        "    frame.Emit(\"Out\");\n"
        "    return 0;\n"
        "  }\n"
        "}\n";
    std::error_code fsError;
    std::filesystem::remove_all(scriptRoot, fsError);
    fsError.clear();
    if (!std::filesystem::create_directories(packageDir, fsError) ||
        !std::filesystem::create_directories(reloadPackageDir, fsError)) {
        error = "Executor self-test failed to create script entry directory: " + packageDir.string();
        return false;
    }
    if (!WriteExecutorSelfTestFile(packageDir / "script.as",
                                   "[executor id=\"ckas.executor.selftest\" name=\"Executor Self Test\" version=\"1.0.0\" entry=\"runtime.as\"]\n",
                                   error) ||
        !WriteExecutorSelfTestFile(packageDir / "runtime.as", source, error)) {
        std::filesystem::remove_all(scriptRoot, fsError);
        return false;
    }
    if (!WriteExecutorSelfTestFile(
            reloadPackageDir / "script.as",
            "[executor id=\"ckas.executor.reload.selftest\" name=\"Executor Reload Self Test\" version=\"1.0.0\" entry=\"runtime.as\"]\n",
            error) ||
        !WriteExecutorSelfTestFile(reloadPackageDir / "runtime.as", reloadSourceV1, error)) {
        std::filesystem::remove_all(scriptRoot, fsError);
        return false;
    }

    ScopedEnvVar scriptRoots("CKAS_SCRIPT_ROOTS", scriptRoot.string());
    if (!manager->GetRuntime()) {
        std::filesystem::remove_all(scriptRoot, fsError);
        error = "Executor self-test could not retrieve ScriptRuntime.";
        return false;
    }
    std::string reloadError;
    if (!manager->GetRuntime()->ReloadAll(&reloadError)) {
        manager->GetRuntime()->Clear();
        std::filesystem::remove_all(scriptRoot, fsError);
        error = reloadError.empty() ? "Executor self-test script entry package failed to load." : reloadError;
        return false;
    }

    auto runReloadProbe = [&]() -> bool {
        CKBehavior *reloadBehavior = CKBehavior::Cast(context->CreateObject(
            CKCID_BEHAVIOR,
            const_cast<CKSTRING>("__CKAS_ExecutorReloadSelfTestBehavior"),
            CK_OBJECTCREATION_DYNAMIC));
        bool deleteCallbackSent = false;
        auto cleanupReloadBehavior = [&]() {
            if (reloadBehavior && deleteCallbackSent) {
                CallBridgeBehaviorCallback(reloadBehavior, CKM_BEHAVIORDELETE);
            }
            DestroyExecutorSelfTestObject(context, reloadBehavior);
        };
        if (!reloadBehavior) {
            error = "Executor reload self-test failed to create behavior.";
            return false;
        }
        reloadBehavior->UseFunction();
        if (reloadBehavior->InitFromGuid(CKGUID(0x15a41762, 0x0c7c4a91)) != CK_OK) {
            cleanupReloadBehavior();
            error = "Executor reload self-test failed to initialize behavior.";
            return false;
        }
        const char *reloadInterface =
            "input 0 name=\"In\"\n"
            "output 0 name=\"Out\"\n"
            "local 0 name=\"Version\" type=\"Integer\" default=\"0\"\n";
        if (!SetExecutorStringSetting(reloadBehavior, EXECUTOR_SETTING_SCRIPT, reloadScriptId, error) ||
            !SetExecutorStringSetting(reloadBehavior, EXECUTOR_SETTING_CLASS, "ExecutorReloadProbe", error) ||
            !SetExecutorStringSetting(reloadBehavior, EXECUTOR_SETTING_INTERFACE, reloadInterface, error)) {
            cleanupReloadBehavior();
            return false;
        }
        CKBehaviorContext reloadContext = MakeExecutorSelfTestContext(context, reloadBehavior);
        if (CallBridgeBehaviorCallback(reloadBehavior, CKM_BEHAVIORCREATE, &reloadContext) != CK_OK) {
            cleanupReloadBehavior();
            error = "Executor reload self-test CREATE callback failed.";
            return false;
        }
        deleteCallbackSent = true;
        if (CallBridgeBehaviorCallback(reloadBehavior, CKM_BEHAVIORSETTINGSEDITED, &reloadContext) != CK_OK) {
            cleanupReloadBehavior();
            error = "Executor reload self-test SETTINGSEDITED callback failed.";
            return false;
        }
        auto executeAndReadVersion = [&]() {
            reloadBehavior->ActivateInput(0, TRUE);
            const int rc = ExecuteExecutorSelfTestBehavior(context, reloadBehavior, reloadContext);
            CKParameterLocal *version = reloadBehavior->GetLocalParameter(EXECUTOR_USER_LOCAL_START);
            int value = 0;
            return rc == CKBR_OK && version && version->GetValue(&value) == CK_OK ? value : -1;
        };
        if (executeAndReadVersion() != 1) {
            cleanupReloadBehavior();
            error = "Executor reload self-test did not execute generation 1.";
            return false;
        }
        if (!WriteExecutorSelfTestFile(reloadPackageDir / "runtime.as",
                                       "class ExecutorReloadProbe { this is not valid AngelScript; }\n",
                                       error)) {
            cleanupReloadBehavior();
            return false;
        }
        reloadError.clear();
        if (manager->GetRuntime()->Reload(reloadScriptId, &reloadError) || reloadError.empty()) {
            cleanupReloadBehavior();
            error = "Executor reload self-test expected invalid generation 2 to fail.";
            return false;
        }
        if (manager->GetExecutorState(reloadBehavior->GetID(), reloadBehavior) == nullptr ||
            executeAndReadVersion() != 1) {
            cleanupReloadBehavior();
            error = "Executor reload self-test discarded generation 1 after a failed replacement.";
            return false;
        }
        if (!WriteExecutorSelfTestFile(reloadPackageDir / "runtime.as", reloadSourceV2, error) ||
            !manager->GetRuntime()->Reload(reloadScriptId, &reloadError)) {
            cleanupReloadBehavior();
            if (error.empty()) {
                error = reloadError.empty() ? "Executor reload self-test could not reload generation 2." : reloadError;
            }
            return false;
        }
        if (executeAndReadVersion() != 2) {
            cleanupReloadBehavior();
            error = "Executor reload self-test kept executing the stale module generation.";
            return false;
        }
        std::string enableError;
        if (!manager->GetRuntime()->Enable(reloadScriptId, false, &enableError) ||
            !manager->GetRuntime()->ModuleName(reloadScriptId).empty() ||
            manager->GetExecutorState(reloadBehavior->GetID(), reloadBehavior) != nullptr) {
            cleanupReloadBehavior();
            error = enableError.empty()
                ? "Executor reload self-test did not invalidate state when its entry was disabled."
                : enableError;
            return false;
        }
        CKParameterLocal *disabledVersion = reloadBehavior->GetLocalParameter(EXECUTOR_USER_LOCAL_START);
        int resetVersion = 0;
        if (!disabledVersion || disabledVersion->SetValue(&resetVersion) != CK_OK ||
            executeAndReadVersion() != 0) {
            cleanupReloadBehavior();
            error = "Executor reload self-test executed a disabled entry.";
            return false;
        }
        ScriptExecutorState *disabledState = manager->GetExecutorState(reloadBehavior->GetID(), reloadBehavior);
        if (!disabledState || !disabledState->Failed || disabledState->Loaded ||
            !disabledState->ModuleName.empty() || disabledState->Object) {
            cleanupReloadBehavior();
            error = "Executor reload self-test retained a live module while disabled.";
            return false;
        }
        enableError.clear();
        if (!manager->GetRuntime()->Enable(reloadScriptId, true, &enableError) ||
            manager->GetRuntime()->ModuleName(reloadScriptId).empty() ||
            executeAndReadVersion() != 2) {
            cleanupReloadBehavior();
            error = enableError.empty()
                ? "Executor reload self-test did not recover after re-enabling its entry."
                : enableError;
            return false;
        }
        cleanupReloadBehavior();
        return true;
    };
    if (!runReloadProbe()) {
        manager->GetRuntime()->Clear();
        std::filesystem::remove_all(scriptRoot, fsError);
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorSelfTestBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *retriggerBehavior = nullptr;
    CKBehavior *chainRoot = nullptr;
    CKBehavior *chainSource = nullptr;
    CKBehavior *chainTarget = nullptr;
    CKBehaviorLink *chainRootToSource = nullptr;
    CKBehaviorLink *chainSourceToTarget = nullptr;
    CKBehaviorLink *chainTargetToRoot = nullptr;
    CKParameterLocal *sourceParam = nullptr;
    CKParameterLocal *mappedParam = nullptr;
    CKParameterLocal *scratchParam = nullptr;
    bool deleteCallbackSent = false;
    bool retriggerDeleteCallbackSent = false;
    bool chainSourceDeleteCallbackSent = false;
    bool chainTargetDeleteCallbackSent = false;
    auto cleanup = [&]() {
        if (chainTarget && chainTargetDeleteCallbackSent) {
            CallBridgeBehaviorCallback(chainTarget, CKM_BEHAVIORDELETE);
        }
        if (chainSource && chainSourceDeleteCallbackSent) {
            CallBridgeBehaviorCallback(chainSource, CKM_BEHAVIORDELETE);
        }
        if (retriggerBehavior && retriggerDeleteCallbackSent) {
            CallBridgeBehaviorCallback(retriggerBehavior, CKM_BEHAVIORDELETE);
        }
        if (behavior && deleteCallbackSent) {
            CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE);
        }
        DestroyExecutorSelfTestObject(context, chainTargetToRoot);
        DestroyExecutorSelfTestObject(context, chainSourceToTarget);
        DestroyExecutorSelfTestObject(context, chainRootToSource);
        DestroyExecutorSelfTestObject(context, chainTarget);
        DestroyExecutorSelfTestObject(context, chainSource);
        DestroyExecutorSelfTestObject(context, chainRoot);
        DestroyExecutorSelfTestObject(context, mappedParam);
        DestroyExecutorSelfTestObject(context, sourceParam);
        DestroyExecutorSelfTestObject(context, retriggerBehavior);
        DestroyExecutorSelfTestObject(context, behavior);
        if (manager->GetRuntime()) {
            manager->GetRuntime()->Clear();
        }
        std::filesystem::remove_all(scriptRoot, fsError);
    };

    if (!behavior) {
        cleanup();
        error = "Executor self-test failed to create behavior.";
        return false;
    }

    behavior->UseFunction();
    const CKERROR initError = behavior->InitFromGuid(CKGUID(0x15a41762, 0x0c7c4a91));
    if (initError != CK_OK) {
        cleanup();
        error = "Executor self-test failed to initialize AngelScript Executor behavior.";
        return false;
    }

    const char *interfaceText =
        "input 0 name=\"In\"\n"
        "output 0 name=\"Out\"\n"
        "pin 0 name=\"Value\" type=\"Integer\"\n"
        "pout 0 name=\"Handled\" type=\"Boolean\"\n"
        "local 0 name=\"__CKAS_FrameScratch\" type=\"Integer\" default=\"7\"\n"
        "local 1 name=\"__CKAS_MappedLocal\" type=\"Integer\"\n";
    if (!SetExecutorStringSetting(behavior, EXECUTOR_SETTING_SCRIPT, scriptId, error) ||
        !SetExecutorStringSetting(behavior, EXECUTOR_SETTING_CLASS, className, error) ||
        !SetExecutorStringSetting(behavior, EXECUTOR_SETTING_INTERFACE, interfaceText, error) ||
        !SetExecutorBoolSetting(behavior, EXECUTOR_SETTING_STRICT_INTERFACE, true, error) ||
        !SetExecutorBoolSetting(behavior, EXECUTOR_SETTING_TRACE, false, error)) {
        cleanup();
        return false;
    }

    mappedParam = context->CreateCKParameterLocal(
        const_cast<CKSTRING>("__CKAS_ExecutorSelfTestMapped"),
        CKPGUID_INT,
        TRUE);
    int mappedValue = 301;
    ParamRef *mappedRef = bridge && mappedParam ? bridge->WrapParameter(mappedParam) : nullptr;
    std::string bindError;
    const bool mappedBound = mappedParam &&
                             mappedParam->SetValue(&mappedValue, sizeof(mappedValue)) == CK_OK &&
                             mappedRef &&
                             bridge->BindFrameLocal(behavior,
                                                    "__CKAS_MappedLocal",
                                                    CKPGUID_INT,
                                                    mappedRef,
                                                    "executor self-test mapped local",
                                                    bindError);
    if (mappedRef) {
        mappedRef->Release();
    }
    if (!mappedBound) {
        cleanup();
        error = bindError.empty() ? "Executor self-test failed to bind mapped interface local." : bindError;
        return false;
    }

    CKBehaviorContext behaviorContext = MakeExecutorSelfTestContext(context, behavior);
    const CKERROR createError = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORCREATE, &behaviorContext);
    if (createError != CK_OK) {
        cleanup();
        error = "Executor self-test CREATE callback failed.";
        return false;
    }
    deleteCallbackSent = true;

    const CKERROR editedError = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORSETTINGSEDITED, &behaviorContext);
    if (editedError != CK_OK) {
        cleanup();
        error = "Executor self-test SETTINGSEDITED callback failed.";
        return false;
    }

    if (behavior->GetInputCount() != 1 ||
        behavior->GetOutputCount() != 1 ||
        behavior->GetInputParameterCount() != 1 ||
        behavior->GetOutputParameterCount() != 1 ||
        behavior->GetLocalParameterCount() != EXECUTOR_USER_LOCAL_START + 1) {
        cleanup();
        error = "Executor self-test dynamic interface shape did not match.";
        return false;
    }
    if (!bridge ||
        !bridge->HasFrameLocalInterface(behavior->GetID()) ||
        bridge->FrameLocalCount(behavior->GetID()) != 2) {
        cleanup();
        error = "Executor self-test did not install logical frame locals.";
        return false;
    }

    const char *failedInterfaceText =
        "input 0 name=\"ChangedIn\"\n"
        "output 0 name=\"Out\"\n"
        "pin 0 name=\"Value\" type=\"Integer\"\n"
        "pout 0 name=\"Handled\" type=\"Boolean\"\n"
        "local 0 name=\"__CKAS_ChangedScratch\" type=\"Integer\" default=\"99\"\n"
        "local 1 name=\"__CKAS_MappedLocal\" type=\"Integer\"\n";
    AngelScriptExecutorFailNextPhysicalInterfaceSyncForSelfTest();
    if (!SetExecutorStringSetting(behavior,
                                  EXECUTOR_SETTING_INTERFACE,
                                  failedInterfaceText,
                                  error) ||
        CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORSETTINGSEDITED, &behaviorContext) != CK_OK) {
        cleanup();
        if (error.empty()) {
            error = "Executor interface transaction self-test could not inject a failed update.";
        }
        return false;
    }
    ScriptExecutorState *transactionState = manager->GetExecutorState(behavior->GetID(), behavior);
    CKParameterLocal *transactionLocal = behavior->GetLocalParameter(EXECUTOR_USER_LOCAL_START);
    int transactionLocalValue = 0;
    if (!transactionState || !transactionState->Failed || transactionState->InterfaceText != interfaceText ||
        behavior->GetInputCount() != 1 ||
        std::string(behavior->GetInput(0)->GetName() ? behavior->GetInput(0)->GetName() : "") != "In" ||
        bridge->FindFrameLocal(behavior->GetID(), "__CKAS_FrameScratch", 0) != 0 ||
        bridge->FindFrameLocal(behavior->GetID(), "__CKAS_ChangedScratch", 0) >= 0 ||
        !transactionLocal ||
        transactionLocal->GetValue(&transactionLocalValue) != CK_OK ||
        transactionLocalValue != 7) {
        cleanup();
        error = "Executor interface transaction self-test did not restore the previous layout and identity.";
        return false;
    }
    if (!SetExecutorStringSetting(behavior, EXECUTOR_SETTING_INTERFACE, interfaceText, error) ||
        CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORSETTINGSEDITED, &behaviorContext) != CK_OK ||
        transactionState->Failed) {
        cleanup();
        if (error.empty()) {
            error = "Executor interface transaction self-test did not recover after rollback.";
        }
        return false;
    }

    sourceParam = context->CreateCKParameterLocal(
        const_cast<CKSTRING>("__CKAS_ExecutorSelfTestValue"),
        CKPGUID_INT,
        TRUE);
    CKParameterIn *pin = behavior->GetInputParameter(0);
    int inputValue = 41;
    if (!sourceParam ||
        sourceParam->SetValue(&inputValue, sizeof(inputValue)) != CK_OK ||
        !pin ||
        pin->SetDirectSource(sourceParam) != CK_OK) {
        cleanup();
        error = "Executor self-test failed to install input parameter source.";
        return false;
    }
    const char *removeConnectedPinInterface =
        "input 0 name=\"In\"\n"
        "output 0 name=\"Out\"\n"
        "pout 0 name=\"Handled\" type=\"Boolean\"\n"
        "local 0 name=\"__CKAS_FrameScratch\" type=\"Integer\" default=\"7\"\n"
        "local 1 name=\"__CKAS_MappedLocal\" type=\"Integer\"\n";
    if (!SetExecutorStringSetting(behavior,
                                  EXECUTOR_SETTING_INTERFACE,
                                  removeConnectedPinInterface,
                                  error) ||
        CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORSETTINGSEDITED, &behaviorContext) != CK_OK ||
        behavior->GetInputParameterCount() != 1 ||
        behavior->GetInputParameter(0) != pin ||
        pin->GetRealSource() != sourceParam ||
        transactionState->InterfaceText != interfaceText ||
        !transactionState->Failed) {
        cleanup();
        if (error.empty()) {
            error = "Executor interface transaction self-test removed a connected input parameter.";
        }
        return false;
    }
    if (!SetExecutorStringSetting(behavior, EXECUTOR_SETTING_INTERFACE, interfaceText, error) ||
        CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORSETTINGSEDITED, &behaviorContext) != CK_OK ||
        transactionState->Failed) {
        cleanup();
        if (error.empty()) {
            error = "Executor interface transaction self-test did not recover after rejecting a connected mutation.";
        }
        return false;
    }
    scratchParam = behavior->GetLocalParameter(EXECUTOR_USER_LOCAL_START);
    int scratchValue = 0;
    if (!scratchParam || scratchParam->GetValue(&scratchValue) != CK_OK || scratchValue != 7) {
        cleanup();
        error = "Executor self-test failed to initialize interface local parameter.";
        return false;
    }

    behavior->ActivateOutput(0, TRUE);
    behavior->ActivateInput(0, TRUE);
    const int suspendedCode = ExecuteExecutorSelfTestBehavior(context, behavior, behaviorContext);
    if (suspendedCode != CKBR_ACTIVATENEXTFRAME) {
        cleanup();
        error = "Executor self-test expected Execute to suspend, got code " + std::to_string(suspendedCode) + ".";
        return false;
    }
    if (behavior->IsInputActive(0)) {
        cleanup();
        error = "Executor self-test did not clear its active input.";
        return false;
    }
    if (behavior->IsOutputActive(0)) {
        cleanup();
        error = "Executor self-test did not clear stale output before a suspending Execute.";
        return false;
    }

    ScriptAsyncScheduler *scheduler = manager->GetAsyncScheduler();
    if (!scheduler) {
        cleanup();
        error = "Executor self-test could not retrieve async scheduler.";
        return false;
    }
    scheduler->Tick();

    const int returnCode = ExecuteExecutorSelfTestBehavior(context, behavior, behaviorContext);
    if (returnCode != CKBR_OK) {
        cleanup();
        error = "Executor self-test returned unexpected code " + std::to_string(returnCode) + ".";
        return false;
    }
    if (!behavior->IsOutputActive(0)) {
        cleanup();
        error = "Executor self-test did not activate output 0.";
        return false;
    }

    CKBOOL handled = FALSE;
    if (behavior->GetOutputParameterValue(0, &handled) != CK_OK || handled == FALSE) {
        cleanup();
        error = "Executor self-test did not write output parameter 0.";
        return false;
    }
    scratchValue = 0;
    if (scratchParam->GetValue(&scratchValue) != CK_OK || scratchValue != 124) {
        cleanup();
        error = "Executor self-test did not write scratch local parameter through BehaviorFrame.";
        return false;
    }
    mappedValue = 0;
    if (mappedParam->GetValue(&mappedValue) != CK_OK || mappedValue != 303) {
        cleanup();
        error = "Executor self-test did not write mapped local source through BehaviorFrame.";
        return false;
    }
    for (int i = EXECUTOR_USER_LOCAL_START; i < behavior->GetLocalParameterCount(); ++i) {
        CKParameterLocal *local = behavior->GetLocalParameter(i);
        if (local && std::string(local->GetName() ? local->GetName() : "").find("__CKAS_BridgeInput_") != std::string::npos) {
            cleanup();
            error = "Executor self-test generated a bridge input source while writing a mapped local.";
            return false;
        }
    }

    retriggerBehavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorSelfTestRetriggerBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    if (!retriggerBehavior) {
        cleanup();
        error = "Executor self-test failed to create retrigger behavior.";
        return false;
    }
    retriggerBehavior->UseFunction();
    const CKERROR retriggerInitError = retriggerBehavior->InitFromGuid(CKGUID(0x15a41762, 0x0c7c4a91));
    if (retriggerInitError != CK_OK) {
        cleanup();
        error = "Executor self-test failed to initialize retrigger Executor behavior.";
        return false;
    }
    if (!SetExecutorStringSetting(retriggerBehavior, EXECUTOR_SETTING_SCRIPT, scriptId, error) ||
        !SetExecutorStringSetting(retriggerBehavior, EXECUTOR_SETTING_CLASS, "ExecutorRetriggerProbe", error) ||
        !SetExecutorStringSetting(retriggerBehavior, EXECUTOR_SETTING_INTERFACE, interfaceText, error) ||
        !SetExecutorBoolSetting(retriggerBehavior, EXECUTOR_SETTING_STRICT_INTERFACE, true, error) ||
        !SetExecutorBoolSetting(retriggerBehavior, EXECUTOR_SETTING_TRACE, false, error)) {
        cleanup();
        return false;
    }

    CKBehaviorContext retriggerContext = MakeExecutorSelfTestContext(context, retriggerBehavior);
    const CKERROR retriggerCreateError = CallBridgeBehaviorCallback(retriggerBehavior, CKM_BEHAVIORCREATE, &retriggerContext);
    if (retriggerCreateError != CK_OK) {
        cleanup();
        error = "Executor self-test retrigger CREATE callback failed.";
        return false;
    }
    retriggerDeleteCallbackSent = true;

    const CKERROR retriggerEditedError = CallBridgeBehaviorCallback(retriggerBehavior, CKM_BEHAVIORSETTINGSEDITED, &retriggerContext);
    if (retriggerEditedError != CK_OK) {
        cleanup();
        error = "Executor self-test retrigger SETTINGSEDITED callback failed.";
        return false;
    }

    retriggerBehavior->ActivateOutput(0, TRUE);
    retriggerBehavior->ActivateInput(0, TRUE);
    const int firstRetriggerCode = ExecuteExecutorSelfTestBehavior(context, retriggerBehavior, retriggerContext);
    if (firstRetriggerCode != CKBR_ACTIVATENEXTFRAME) {
        cleanup();
        error = "Executor self-test expected retrigger probe to suspend, got code " +
            std::to_string(firstRetriggerCode) + ".";
        return false;
    }
    if (retriggerBehavior->IsInputActive(0) || retriggerBehavior->IsOutputActive(0)) {
        cleanup();
        error = "Executor self-test retrigger probe did not clear input and stale output on first activation.";
        return false;
    }

    retriggerBehavior->ActivateOutput(0, TRUE);
    retriggerBehavior->ActivateInput(0, TRUE);
    const int secondRetriggerCode = ExecuteExecutorSelfTestBehavior(context, retriggerBehavior, retriggerContext);
    if (secondRetriggerCode != CKBR_ACTIVATENEXTFRAME) {
        cleanup();
        error = "Executor self-test did not ignore a duplicate suspended input; got code " +
            std::to_string(secondRetriggerCode) + ".";
        return false;
    }
    if (retriggerBehavior->IsInputActive(0) || retriggerBehavior->IsOutputActive(0)) {
        cleanup();
        error = "Executor self-test retrigger probe did not consume duplicate suspended input.";
        return false;
    }
    if (CallBridgeBehaviorCallback(retriggerBehavior, CKM_BEHAVIORRESET, &retriggerContext) != CK_OK) {
        cleanup();
        error = "Executor self-test RESET callback failed while Execute was suspended.";
        return false;
    }
    CKParameterLocal *resetScratch = retriggerBehavior->GetLocalParameter(EXECUTOR_USER_LOCAL_START);
    int resetScratchValue = 0;
    if (!resetScratch ||
        resetScratch->GetValue(&resetScratchValue) != CK_OK ||
        resetScratchValue != 777) {
        cleanup();
        error = "Executor self-test resumed suspended Execute instead of invoking OnReset.";
        return false;
    }
    if (CallBridgeBehaviorCallback(retriggerBehavior, CKM_BEHAVIORRESET, &retriggerContext) != CK_OK ||
        !SetExecutorBoolSetting(retriggerBehavior, EXECUTOR_SETTING_TRACE, true, error) ||
        CallBridgeBehaviorCallback(retriggerBehavior, CKM_BEHAVIORSETTINGSEDITED, &retriggerContext) != CK_OK) {
        cleanup();
        if (error.empty()) {
            error = "Executor self-test could not recover after a failed OnReset.";
        }
        return false;
    }
    retriggerBehavior->ActivateInput(0, TRUE);
    if (ExecuteExecutorSelfTestBehavior(context, retriggerBehavior, retriggerContext) != CKBR_OK) {
        cleanup();
        error = "Executor self-test failed to execute after recovering from OnReset failure.";
        return false;
    }
    resetScratchValue = 0;
    if (resetScratch->GetValue(&resetScratchValue) != CK_OK || resetScratchValue != 779) {
        cleanup();
        error = "Executor self-test did not rerun Start after OnReset failure recovery.";
        return false;
    }
    const std::string editedRetriggerInterface = std::string(interfaceText) + "\n";
    if (!SetExecutorStringSetting(retriggerBehavior,
                                  EXECUTOR_SETTING_INTERFACE,
                                  editedRetriggerInterface.c_str(),
                                  error) ||
        CallBridgeBehaviorCallback(retriggerBehavior, CKM_BEHAVIORSETTINGSEDITED, &retriggerContext) != CK_OK) {
        cleanup();
        if (error.empty()) {
            error = "Executor self-test failed to edit an active executor interface.";
        }
        return false;
    }
    resetScratchValue = 0;
    if (resetScratch->GetValue(&resetScratchValue) != CK_OK || resetScratchValue != 778) {
        cleanup();
        error = "Executor self-test skipped OnDestroy while changing executor identity.";
        return false;
    }
    if (!SetExecutorStringSetting(retriggerBehavior, EXECUTOR_SETTING_INTERFACE, "", error) ||
        CallBridgeBehaviorCallback(retriggerBehavior, CKM_BEHAVIORSETTINGSEDITED, &retriggerContext) != CK_OK) {
        cleanup();
        if (error.empty()) {
            error = "Executor self-test failed to clear the dynamic interface.";
        }
        return false;
    }
    if (retriggerBehavior->GetInputCount() != 1 ||
        retriggerBehavior->GetOutputCount() != 1 ||
        retriggerBehavior->GetInputParameterCount() != 0 ||
        retriggerBehavior->GetOutputParameterCount() != 0 ||
        retriggerBehavior->GetLocalParameterCount() != EXECUTOR_USER_LOCAL_START ||
        bridge->HasFrameLocalInterface(retriggerBehavior->GetID())) {
        cleanup();
        error = "Executor self-test retained stale generated slots after clearing Interface.";
        return false;
    }
    const char *chainInterfaceText =
        "input 0 name=\"In\"\n"
        "output 0 name=\"Out\"\n"
        "local 0 name=\"__CKAS_FrameScratch\" type=\"Integer\" default=\"0\"\n";

    auto initializeChainExecutor = [&](CKBehavior *executor,
                                       const char *executorClassName,
                                       bool &deleteCallbackFlag,
                                       const char *label) -> bool {
        if (!executor) {
            error = std::string("Executor self-test failed to create ") + label + ".";
            return false;
        }
        executor->UseFunction();
        const CKERROR init = executor->InitFromGuid(CKGUID(0x15a41762, 0x0c7c4a91));
        if (init != CK_OK) {
            error = std::string("Executor self-test failed to initialize ") + label + ".";
            return false;
        }
        if (!SetExecutorStringSetting(executor, EXECUTOR_SETTING_SCRIPT, scriptId, error) ||
            !SetExecutorStringSetting(executor, EXECUTOR_SETTING_CLASS, executorClassName, error) ||
            !SetExecutorStringSetting(executor, EXECUTOR_SETTING_INTERFACE, chainInterfaceText, error) ||
            !SetExecutorBoolSetting(executor, EXECUTOR_SETTING_STRICT_INTERFACE, true, error) ||
            !SetExecutorBoolSetting(executor, EXECUTOR_SETTING_TRACE, false, error)) {
            return false;
        }
        CKBehaviorContext executorContext = MakeExecutorSelfTestContext(context, executor);
        if (CallBridgeBehaviorCallback(executor, CKM_BEHAVIORCREATE, &executorContext) != CK_OK) {
            error = std::string("Executor self-test ") + label + " CREATE callback failed.";
            return false;
        }
        deleteCallbackFlag = true;
        if (CallBridgeBehaviorCallback(executor, CKM_BEHAVIORSETTINGSEDITED, &executorContext) != CK_OK) {
            error = std::string("Executor self-test ") + label + " SETTINGSEDITED callback failed.";
            return false;
        }
        if (executor->GetInputCount() != 1 ||
            executor->GetOutputCount() != 1 ||
            executor->GetLocalParameterCount() != EXECUTOR_USER_LOCAL_START + 1) {
            error = std::string("Executor self-test ") + label + " interface shape did not match.";
            return false;
        }
        return true;
    };

    auto createChainLink = [&](const char *name,
                               CKBehaviorIO *sourceIo,
                               CKBehaviorIO *targetIo,
                               CKBehaviorLink *&slot) -> bool {
        slot = CKBehaviorLink::Cast(context->CreateObject(CKCID_BEHAVIORLINK,
                                                          const_cast<CKSTRING>(name),
                                                          CK_OBJECTCREATION_DYNAMIC));
        if (!slot || !sourceIo || !targetIo) {
            error = "Executor self-test failed to create a behavior graph link.";
            return false;
        }
        CKERROR linkError = slot->SetInBehaviorIO(sourceIo);
        if (linkError == CK_OK) {
            linkError = slot->SetOutBehaviorIO(targetIo);
        }
        if (linkError == CK_OK) {
            slot->SetInitialActivationDelay(0);
            slot->SetActivationDelay(0);
            linkError = chainRoot->AddSubBehaviorLink(slot);
        }
        if (linkError != CK_OK) {
            error = "Executor self-test failed to attach a behavior graph link.";
            return false;
        }
        return true;
    };

    chainRoot = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorLinkRoot"),
        CK_OBJECTCREATION_DYNAMIC));
    chainSource = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorLinkSource"),
        CK_OBJECTCREATION_DYNAMIC));
    chainTarget = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorLinkTarget"),
        CK_OBJECTCREATION_DYNAMIC));
    if (!chainRoot || !chainSource || !chainTarget) {
        cleanup();
        error = "Executor self-test failed to create suspended link graph behaviors.";
        return false;
    }
    chainRoot->UseGraph();
    chainRoot->AddInput(const_cast<CKSTRING>("In"));
    chainRoot->AddOutput(const_cast<CKSTRING>("Out"));
    if (!initializeChainExecutor(chainSource,
                                 "ExecutorSuspendingSourceProbe",
                                 chainSourceDeleteCallbackSent,
                                 "suspending source") ||
        !initializeChainExecutor(chainTarget,
                                 "ExecutorSuspendingDownstreamProbe",
                                 chainTargetDeleteCallbackSent,
                                 "downstream target")) {
        cleanup();
        return false;
    }
    CKERROR graphError = chainRoot->AddSubBehavior(chainSource);
    if (graphError == CK_OK) {
        graphError = chainRoot->AddSubBehavior(chainTarget);
    }
    if (graphError != CK_OK) {
        cleanup();
        error = "Executor self-test failed to attach suspended link graph sub-behaviors.";
        return false;
    }
    if (!createChainLink("__CKAS_ExecutorLinkRootToSource",
                         chainRoot->GetInput(0),
                         chainSource->GetInput(0),
                         chainRootToSource) ||
        !createChainLink("__CKAS_ExecutorLinkSourceToTarget",
                         chainSource->GetOutput(0),
                         chainTarget->GetInput(0),
                         chainSourceToTarget) ||
        !createChainLink("__CKAS_ExecutorLinkTargetToRoot",
                         chainTarget->GetOutput(0),
                         chainRoot->GetOutput(0),
                         chainTargetToRoot)) {
        cleanup();
        return false;
    }

    CKBehaviorContext chainContext = MakeExecutorSelfTestContext(context, chainRoot);
    chainRoot->ActivateInput(0, TRUE);
    chainRoot->Activate(TRUE, FALSE);
    const int chainFirstCode = ExecuteExecutorSelfTestBehavior(context, chainRoot, chainContext);
    if ((chainFirstCode & CKBR_ACTIVATENEXTFRAME) == 0 && !chainSource->IsActive()) {
        cleanup();
        error = "Executor self-test suspended source did not keep the graph active.";
        return false;
    }
    scheduler->Tick();
    const int chainSecondCode = ExecuteExecutorSelfTestBehavior(context, chainRoot, chainContext);
    if (chainSecondCode >= CKBR_GENERICERROR) {
        cleanup();
        error = "Executor self-test suspended link graph returned error code " +
            std::to_string(chainSecondCode) + ".";
        return false;
    }
    CKParameterLocal *targetCount = chainTarget->GetLocalParameter(EXECUTOR_USER_LOCAL_START);
    int downstreamCount = 0;
    if (!targetCount || targetCount->GetValue(&downstreamCount) != CK_OK || downstreamCount != 0) {
        cleanup();
        error = "Executor self-test expected downstream behavior to be suspended before incrementing.";
        return false;
    }
    if (!chainTarget->IsActive()) {
        cleanup();
        error = "Executor self-test did not keep the downstream behavior active while suspended.";
        return false;
    }
    chainSource->ActivateOutput(0, TRUE);
    chainTarget->ActivateInput(0, TRUE);
    CKBehaviorContext chainTargetContext = MakeExecutorSelfTestContext(context, chainTarget);
    const int duplicateTargetCode = ExecuteExecutorSelfTestBehavior(context, chainTarget, chainTargetContext);
    if (duplicateTargetCode != CKBR_ACTIVATENEXTFRAME || chainTarget->IsInputActive(0)) {
        cleanup();
        error = "Executor self-test did not consume a duplicate suspended target input.";
        return false;
    }
    if (!chainSource->IsOutputActive(0)) {
        cleanup();
        error = "Executor self-test cleared a shared upstream output while consuming a duplicate input.";
        return false;
    }
    bool downstreamCompleted = false;
    int chainFourthCode = CKBR_ACTIVATENEXTFRAME;
    for (int attempt = 0; attempt < 5; ++attempt) {
        scheduler->Tick();
        chainFourthCode = ExecuteExecutorSelfTestBehavior(context, chainRoot, chainContext);
        if (chainFourthCode >= CKBR_GENERICERROR) {
            cleanup();
            error = "Executor self-test suspended downstream follow-up returned error code " +
                std::to_string(chainFourthCode) + ".";
            return false;
        }
        downstreamCount = 0;
        if (!targetCount || targetCount->GetValue(&downstreamCount) != CK_OK) {
            cleanup();
            error = "Executor self-test could not read downstream execution count.";
            return false;
        }
        if (downstreamCount > 1) {
            cleanup();
            error = "Executor self-test propagated a stale output pulse to downstream behavior more than once.";
            return false;
        }
        if (downstreamCount == 1) {
            downstreamCompleted = true;
        }
    }
    if (!downstreamCompleted) {
        cleanup();
        error = "Executor self-test did not resume suspended downstream behavior after clearing duplicate input.";
        return false;
    }
    if (!chainRoot->IsOutputActive(0)) {
        cleanup();
        error = "Executor self-test did not propagate resumed downstream output to the root output.";
        return false;
    }

    if (CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDETACH, &behaviorContext) != CK_OK) {
        cleanup();
        error = "Executor self-test DETACH callback failed.";
        return false;
    }
    scratchValue = 0;
    mappedValue = 0;
    if (!scratchParam || scratchParam->GetValue(&scratchValue) != CK_OK || scratchValue != 901 ||
        !mappedParam || mappedParam->GetValue(&mappedValue) != CK_OK || mappedValue != 902) {
        cleanup();
        error = "Executor self-test DETACH OnDestroy lost its logical local interface.";
        return false;
    }
    if (bridge->HasFrameLocalInterface(behavior->GetID()) ||
        manager->GetExecutorState(behavior->GetID(), behavior)) {
        cleanup();
        error = "Executor self-test did not release its state after DETACH.";
        return false;
    }
    if (CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE, &behaviorContext) != CK_OK) {
        cleanup();
        error = "Executor self-test DELETE after DETACH was not idempotent.";
        return false;
    }
    deleteCallbackSent = false;

    cleanup();
    error.clear();
    return true;
}
