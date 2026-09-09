#include "ScriptManager.h"

#include <string>
#include <utility>
#include <vector>

#include "CKBehavior.h"

#include <fmt/format.h>

#ifndef CKAS_BUILD_SELF_TESTS
#define CKAS_BUILD_SELF_TESTS 0
#endif

#include "ScriptApiSupport.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptParameterRegistry.h"
#include "ScriptRuntime.h"
#include "ScriptMessage.h"
#include "ScriptObjectIdentity.h"
#include "ScriptAsync.h"

#if CKAS_BUILD_SELF_TESTS
#include "ScriptSelfTests.h"
#endif

namespace {

void ClearRuntimeQueues(ScriptMessageBus *messageBus, ScriptAsyncScheduler *asyncScheduler) {
    if (messageBus) {
        messageBus->Clear();
    }
    if (asyncScheduler) {
        asyncScheduler->Clear();
    }
}

} // namespace

ScriptManager::ScriptManager(CKContext *context) : CKBaseManager(context, SCRIPT_MANAGER_GUID, (CKSTRING) "AngelScript Manager") {
    const int r = Init();
    if (r < 0) {
        return;
    }

    m_InitializationError = context
                                ? context->RegisterNewManager(this)
                                : CKERR_INVALIDPARAMETER;
}

ScriptManager::~ScriptManager() {
    Shutdown();
}

CKStateChunk *ScriptManager::SaveData(CKFile *SavedFile) {
    return nullptr;
}

CKERROR ScriptManager::LoadData(CKStateChunk *chunk, CKFile *LoadedFile) {
    return CK_OK;
}
void ScriptManager::ClearObjectBoundRuntime() {
    m_ResetOrphanCleanupPending = false;
    m_HandleRegistry.CancelActiveExecutions("Execution was cancelled because CK objects are being cleared.");
    if (m_MessageBus) {
        m_MessageBus->CancelPendingWork("Message work was cancelled because CK objects are being cleared.");
    }
    if (m_AsyncScheduler) {
        m_AsyncScheduler->CancelAll("Async task was cancelled because CK objects are being cleared.");
    }

    // CK2 calls PreClearAll while CKBehavior objects are still resolvable. Keep
    // the behavior bridge alive until all adapters have delivered OnDestroy.
    ClearExecutorStates();
    ClearComponentStates();
    if (m_Runtime) {
        m_Runtime->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    ClearRuntimeQueues(m_MessageBus.get(), m_AsyncScheduler.get());
    ClearCKObjectData();
}

CKERROR ScriptManager::PreClearAll() {
#if CKAS_BUILD_SELF_TESTS
    RecordScriptManagerLifecycleSelfTestEvent("PreClearAll");
#endif
    ClearObjectBoundRuntime();
    return CK_OK;
}

CKERROR ScriptManager::PostClearAll() {
#if CKAS_BUILD_SELF_TESTS
    RecordScriptManagerLifecycleSelfTestEvent("PostClearAll");
#endif
    // Idempotent fallback for hosts/tools that do not dispatch PreClearAll.
    ClearObjectBoundRuntime();
    return CK_OK;
}

CKERROR ScriptManager::PreProcess() {
#if CKAS_BUILD_SELF_TESTS
    const CKERROR selfTestResult = RunScriptStartupSelfTests(this);
    if (selfTestResult != CK_OK) {
        return selfTestResult;
    }
#endif
    // OnCKPostReset is the normal Virtools seam. Keep this fallback for hosts
    // that omit that callback and for direct lifecycle calls in tests/tools.
    CancelResetOrphanedAsyncWork();
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Tick();
    }
    if (m_MessageBus) {
        m_MessageBus->Tick();
    }
    if (m_Runtime) {
        m_Runtime->PreProcess();
    }
    return m_BehaviorBridge ? m_BehaviorBridge->PreProcess() : CK_OK;
}

CKERROR ScriptManager::PostProcess() {
    if (m_Runtime) {
        m_Runtime->PostProcess();
    }
    return m_BehaviorBridge ? m_BehaviorBridge->PostProcess() : CK_OK;
}

CKERROR ScriptManager::OnCKInit() {
    return CK_OK;
}

CKERROR ScriptManager::OnCKEnd() {
#if CKAS_BUILD_SELF_TESTS
    RecordScriptManagerLifecycleSelfTestEvent("OnCKEnd.begin");
    PrepareScriptManagerLifecycleEndSelfTest(this);
#endif
    m_ResetOrphanCleanupPending = false;
    m_HandleRegistry.CancelActiveExecutions("Execution was cancelled because CK is ending.");
    if (m_MessageBus) {
        m_MessageBus->CancelPendingWork("Message work was cancelled because CK is ending.");
    }
    if (m_AsyncScheduler) {
        m_AsyncScheduler->CancelAll("Async task was cancelled because CK is ending.");
    }
    ClearExecutorStates();
    ClearComponentStates();
    if (m_Runtime) {
        m_Runtime->OnEnd();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    ClearRuntimeQueues(m_MessageBus.get(), m_AsyncScheduler.get());
#if CKAS_BUILD_SELF_TESTS
    return CompleteScriptManagerLifecycleSelfTest(this);
#else
    return CK_OK;
#endif
}

CKERROR ScriptManager::OnCKReset() {
#if CKAS_BUILD_SELF_TESTS
    RecordScriptManagerLifecycleSelfTestEvent("OnCKReset");
#endif
    m_HandleRegistry.CancelActiveExecutions("Execution was cancelled because CK is resetting.");
    if (m_MessageBus) {
        m_MessageBus->CancelPendingWork("Message work was cancelled because CK is resetting.");
    }
    if (m_Runtime) {
        m_Runtime->OnReset();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear(false);
    }
    m_ResetOrphanCleanupPending = true;
    return CK_OK;
}

CKERROR ScriptManager::OnCKPostReset() {
#if CKAS_BUILD_SELF_TESTS
    RecordScriptManagerLifecycleSelfTestEvent("OnCKPostReset");
#endif
    CancelResetOrphanedAsyncWork();
    return CK_OK;
}

CKERROR ScriptManager::SequenceToBeDeleted(CK_ID *objids, int count) {
    if (!m_Context || !objids || count <= 0) {
        return CK_OK;
    }

    for (int i = 0; i < count; ++i) {
        AdvanceBridgeObjectIdentityGeneration(m_Context, objids[i]);
        CKBehavior *behavior = CKBehavior::Cast(m_Context->GetObject(objids[i]));
        if (!behavior) {
            continue;
        }

        // Original CK2 sends this manager callback while the behavior is still
        // live and does not emit CKM_BEHAVIORDELETE from ordinary deletion.
        // The Ballanced source implementation mirrors that contract.
        // Both releases are idempotent and only the matching store can own the
        // behavior, so this is also safe on hosts that do emit the BB callback.
        ReleaseExecutorState(behavior);
        ReleaseComponentState(behavior);
    }
    return CK_OK;
}

void ScriptManager::CancelResetOrphanedAsyncWork() {
    if (!m_ResetOrphanCleanupPending) {
        return;
    }
    m_ResetOrphanCleanupPending = false;
    if (!m_AsyncScheduler) {
        return;
    }

    std::unordered_set<ScriptAsyncOwnerId> liveOwnerIds;
    if (m_Runtime) {
        m_Runtime->CollectAsyncOwnerIds(liveOwnerIds);
    }
    m_ComponentStates.CollectAsyncOwnerIds(liveOwnerIds);
    m_ExecutorStates.CollectAsyncOwnerIds(liveOwnerIds);
    m_AsyncScheduler->CancelOrphaned(
        liveOwnerIds,
        "Async task was cancelled because it was orphaned by CK reset.");
}

CKERROR ScriptManager::OnCKPause() {
#if CKAS_BUILD_SELF_TESTS
    RecordScriptManagerLifecycleSelfTestEvent("OnCKPause");
#endif
    if (m_Runtime) {
        m_Runtime->OnPause();
    }
    if (m_AsyncScheduler) {
        m_AsyncScheduler->PauseProcessing();
    }
    if (m_MessageBus) {
        m_MessageBus->PauseProcessing();
    }
    return CK_OK;
}

CKERROR ScriptManager::OnCKPlay() {
#if CKAS_BUILD_SELF_TESTS
    RecordScriptManagerLifecycleSelfTestEvent("OnCKPlay");
#endif
    // A failed/partial host Reset can omit OnCKPostReset. Sweep before either
    // scheduler resumes so no orphan can advance on the first played frame.
    CancelResetOrphanedAsyncWork();
    if (m_AsyncScheduler) {
        m_AsyncScheduler->ResumeProcessing();
    }
    if (m_MessageBus) {
        m_MessageBus->ResumeProcessing();
    }
    if (m_Runtime) {
        m_Runtime->OnResume();
    }
    return CK_OK;
}

CKERROR ScriptManager::PostLoad() {
    if (m_Runtime) {
        m_Runtime->PostLoad();
    }
    return CK_OK;
}

CKERROR ScriptManager::OnPostCopy(CKDependenciesContext &context) {
    return CK_OK;
}

int ScriptManager::Init() {
    if (IsInited())
        return -2;

    if (!m_ParameterRegistry) {
        m_ParameterRegistry = std::make_unique<ScriptParameterRegistry>(m_Context);
    }

    int r = m_EngineHost.Setup(*this, m_Context);
    if (r < 0)
        return r;

    if (!m_Runtime) {
        m_Runtime = std::make_unique<ScriptRuntime>(this);
    }
    if (!m_AsyncScheduler) {
        m_AsyncScheduler = std::make_unique<ScriptAsyncScheduler>(this);
    }
    if (!m_MessageBus) {
        m_MessageBus = std::make_unique<ScriptMessageBus>(this);
    }

    m_Flags |= AS_INITED;
    return r;
}

int ScriptManager::Shutdown() {
    if (!IsInited()) {
        ClearBridgeObjectIdentityGenerations(m_Context);
        return -2;
    }

    m_ResetOrphanCleanupPending = false;
    m_HandleRegistry.CancelActiveExecutions("Execution was cancelled because the script manager is shutting down.");
    m_HandleRegistry.Clear();

    ClearExecutorStates();
    ClearComponentStates();
    if (m_Runtime) {
        m_Runtime->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    ClearRuntimeQueues(m_MessageBus.get(), m_AsyncScheduler.get());

    m_EngineHost.ReleaseContextPool();

    ClearCKObjectData();
    m_ModuleStateStore.Clear();
    m_ModuleRegistry.Clear();

    m_BehaviorBridge.reset();
    m_Runtime.reset();
    m_MessageBus.reset();
    m_AsyncScheduler.reset();
    m_ParameterRegistry.reset();

    m_EngineHost.ShutdownAndReleaseEngine();
    m_EngineHost.MarkExtensionsInactive();
    ClearBridgeObjectIdentityGenerations(m_Context);

    m_Flags &= ~AS_INITED;
    return 0;
}

asIScriptEngine *ScriptManager::GetScriptEngine() {
    return m_EngineHost.Engine();
}

const char *ScriptManager::GetVersion() {
    return asGetLibraryVersion();
}

const char *ScriptManager::GetOptions() {
    return asGetLibraryOptions();
}

CKAS_STATUS ScriptManager::BorrowEngine(asIScriptEngine **outEngine, CKAngelScriptResult *result) {
    if (outEngine) {
        *outEngine = nullptr;
    }
    if (!outEngine) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine out pointer is required.");
    }
    asIScriptEngine *engine = GetScriptEngine();
    if (!engine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    *outEngine = engine;
    return StoreResult(result, CKAS_OK);
}

asIScriptContext *ScriptManager::GetActiveContext() {
    return asGetActiveContext();
}

CKAS_STATUS ScriptManager::BorrowActiveContext(asIScriptContext **outContext, CKAngelScriptResult *result) {
    if (outContext) {
        *outContext = nullptr;
    }
    if (!outContext) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Active context out pointer is required.");
    }
    asIScriptEngine *engine = GetScriptEngine();
    if (!engine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptContext *ctx = GetActiveContext();
    if (!ctx || ctx->GetEngine() != engine) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "No active context belongs to this CKAngelScript manager.");
    }
    *outContext = ctx;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::SetActiveContextException(const char *message, CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(message)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Exception message is required.");
    }
    asIScriptEngine *engine = GetScriptEngine();
    if (!engine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptContext *ctx = GetActiveContext();
    if (!ctx || ctx->GetEngine() != engine) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "No active context belongs to this CKAngelScript manager.");
    }
    ctx->SetException(message);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::RegisterEngineExtension(const CKAngelScriptEngineExtension &extension,
                                                   CKAngelScriptResult *result) {
    return m_EngineHost.RegisterExtension(*this, extension, result);
}

CKAS_STATUS ScriptManager::UnregisterEngineExtension(const char *name,
                                                     CKAngelScriptResult *result) {
    return m_EngineHost.UnregisterExtension(*this, name, result);
}

void ScriptManager::MessageCallback(const asSMessageInfo &msg) {
    const char *type = "NULL";
    CKAS_MESSAGETYPE publicType = CKAS_MESSAGE_INFORMATION;
    switch (msg.type) {
        case asMSGTYPE_ERROR:
            type = "ERROR";
            publicType = CKAS_MESSAGE_ERROR;
            break;
        case asMSGTYPE_WARNING:
            type = "WARN";
            publicType = CKAS_MESSAGE_WARNING;
            break;
        case asMSGTYPE_INFORMATION:
            type = "INFO";
            publicType = CKAS_MESSAGE_INFORMATION;
            break;
    }
    const std::string formatted = fmt::format("{}({},{}): {}: {}",
                                              msg.section ? msg.section : "<unknown section>",
                                              msg.row,
                                              msg.col,
                                              type,
                                              msg.message ? msg.message : "");
    if (m_Diagnostics.IsCapturingScriptMessages()) {
        CapturedScriptMessage captured;
        captured.Section = msg.section ? msg.section : "";
        captured.Row = msg.row;
        captured.Column = msg.col;
        captured.Type = publicType;
        captured.Message = msg.message ? msg.message : "";
        m_Diagnostics.CaptureScriptMessage(formatted, std::move(captured));
    }
    if (m_MessageOutputSuppressionDepth == 0) {
        m_Context->OutputToConsoleEx(const_cast<char *>("%s"), formatted.c_str());
    }
}

void ScriptManager::PushMessageOutputSuppression() {
    ++m_MessageOutputSuppressionDepth;
}

void ScriptManager::PopMessageOutputSuppression() {
    if (m_MessageOutputSuppressionDepth > 0) {
        --m_MessageOutputSuppressionDepth;
    }
}

void ScriptManager::ExceptionCallback(asIScriptContext *context) {
    std::string callStack = GetCallStack(context);
    asIScriptFunction *func = context ? context->GetExceptionFunction() : nullptr;
    const char *funcDecl = func ? func->GetDeclaration() : nullptr;
    const char *exception = context ? context->GetExceptionString() : nullptr;
    std::string message = fmt::format("Exception in '{}': '{}'\n{}",
                                      funcDecl ? funcDecl : "<unknown function>",
                                      exception ? exception : "",
                                      callStack);

    asSMessageInfo info = {};
    if (context) {
        info.row = context->GetExceptionLineNumber(&info.col, &info.section);
    }
    info.type = asMSGTYPE_ERROR;
    info.message = message.c_str();
    MessageCallback(info);
}

std::string ScriptManager::GetCallStack(asIScriptContext *context) {
    std::string str("Callstack:\n");
    if (!context) {
        return str;
    }

    for (asUINT i = 0; i < context->GetCallstackSize(); i++) {
        asIScriptFunction *func = context->GetFunction(i);
        int column;
        const char *section;
        int line = context->GetLineNumber(i, &column, &section);
        const char *funcDecl = func ? func->GetDeclaration() : nullptr;
        str.append(fmt::format("\t{} at {}({},{})\n",
                               funcDecl ? funcDecl : "<unknown function>",
                               section ? section : "<unknown section>",
                               line,
                               column));
    }
    return std::move(str);
}

asIScriptContext *ScriptManager::RequestContextFromPool() {
    return m_EngineHost.RequestContext(*this);
}

void ScriptManager::ReturnContextToPool(asIScriptContext *ctx) {
    m_EngineHost.ReturnContext(*this, ctx);
}

CKAngelScriptResult ScriptManager::MakeResult(CKAS_STATUS status,
                                              int angelScriptCode,
                                              const std::string &errorMessage,
                                              const std::string &stackTrace,
                                              const std::vector<CapturedScriptMessage> *compilerMessages) {
    return m_Diagnostics.MakeResult(status, angelScriptCode, errorMessage, stackTrace, compilerMessages);
}

CKAS_STATUS ScriptManager::StoreResult(CKAngelScriptResult *out,
                                       CKAS_STATUS status,
                                       int angelScriptCode,
                                       const std::string &errorMessage,
                                       const std::string &stackTrace,
                                       const std::vector<CapturedScriptMessage> *compilerMessages) {
    return m_Diagnostics.StoreResult(out, status, angelScriptCode, errorMessage, stackTrace, compilerMessages);
}

const CKAngelScriptResult *ScriptManager::GetLastResult() const {
    return m_Diagnostics.GetLastResult();
}

CKAS_STATUS ScriptManager::StoreApiResult(CKAngelScriptResult *out,
                                          CKAS_STATUS status,
                                          int angelScriptCode,
                                          const char *errorMessage,
                                          const char *stackTrace) {
    return StoreResult(out,
                       status,
                       angelScriptCode,
                       std::string(errorMessage ? errorMessage : ""),
                       std::string(stackTrace ? stackTrace : ""));
}

void ScriptManager::BeginScriptMessageCapture() {
    m_Diagnostics.BeginScriptMessageCapture();
}

std::string ScriptManager::EndScriptMessageCapture(std::vector<CapturedScriptMessage> *messages) {
    return m_Diagnostics.EndScriptMessageCapture(messages);
}

CKERROR ScriptManager::ResolveScriptFileName(XString &filename) {
    return m_PathResolver.ResolveScriptFileName(m_Context, filename);
}

void *ScriptManager::GetCKObjectData(CK_ID id) const {
    return m_CKObjectRetainer.GetData(id);
}

void ScriptManager::SetCKObjectData(CK_ID id, void *data) {
    m_CKObjectRetainer.SetData(id, data);
}

void ScriptManager::ReleaseCKObjectData(CK_ID id) {
    m_CKObjectRetainer.ReleaseData(id);
}

void ScriptManager::ClearCKObjectData() {
    m_CKObjectRetainer.Clear();
}

void ScriptManager::TrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    m_CKObjectRetainer.TrackCallback(id, func);
}

bool ScriptManager::UntrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    return m_CKObjectRetainer.UntrackCallback(id, func);
}

void ScriptManager::ReleaseCKObjectCallbacks(CK_ID id) {
    m_CKObjectRetainer.ReleaseCallbacks(id);
}

ScriptBehaviorBridge *ScriptManager::GetBehaviorBridge() {
    if (!m_BehaviorBridge) {
        m_BehaviorBridge = std::make_unique<ScriptBehaviorBridge>(this);
    }
    return m_BehaviorBridge.get();
}

