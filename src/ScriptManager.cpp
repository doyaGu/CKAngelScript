#include "ScriptManager.h"

#include <string>
#include <fmt/format.h>

#ifndef CKAS_BUILD_SELF_TESTS
#define CKAS_BUILD_SELF_TESTS 0
#endif

#ifndef CKAS_ENABLE_DYNCALL
#define CKAS_ENABLE_DYNCALL 0
#endif

#include "CKPathManager.h"
#include "Logger.h"
#include "ScriptApiSupport.h"

#include "ScriptFormat.h"
#include "ScriptNativePointer.h"
#include "ScriptNativeBuffer.h"
#if CKAS_ENABLE_API_EXPORT
#include "ScriptInfo.h"
#endif
#if CKAS_ENABLE_DYNCALL
#include "ScriptDynCall.h"
#endif
#include "ScriptUtils.h"
#include "ScriptVxMath.h"
#include "ScriptCK2.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptComponentState.h"
#include "ScriptParameterRegistry.h"
#include "ScriptScene.h"
#include "ScriptRuntime.h"
#include "ScriptMessage.h"
#include "ScriptAsync.h"
#include "ScriptRegistration.h"

#if CKAS_BUILD_SELF_TESTS
#include "ScriptSelfTests.h"
#endif

// Application modules
#include "add_on/scripthelper/scripthelper.h"
#include "add_on/scriptbuilder/scriptbuilder.h"

// Script extensions
#include "add_on/scriptstdstring/scriptstdstring.h"
#include "add_on/scriptarray/scriptarray.h"
#include "add_on/scriptany/scriptany.h"
#include "add_on/scripthandle/scripthandle.h"
#include "add_on/weakref/weakref.h"
#include "add_on/scriptdictionary/scriptdictionary.h"
#include "add_on/scriptfile/scriptfile.h"
#include "add_on/scriptfile/scriptfilesystem.h"
#include "add_on/scriptmath/scriptmath.h"
#include "add_on/scriptmath/scriptmathcomplex.h"
#include "add_on/scriptgrid/scriptgrid.h"
#include "add_on/datetime/datetime.h"

namespace {

std::string MakeEngineExtensionConfigGroupName(const char *name) {
    return fmt::format("CKAngelScript.Extension.{}", name ? name : "");
}

} // namespace

ScriptManager::ScriptManager(CKContext *context) : CKBaseManager(context, SCRIPT_MANAGER_GUID, (CKSTRING) "AngelScript Manager") {
    int r = Init();
    if (r < 0)
        return;

    context->RegisterNewManager(this);
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



CKERROR ScriptManager::PostClearAll() {
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_Runtime) {
        m_Runtime->Clear();
    }
    if (m_MessageBus) {
        m_MessageBus->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    ClearComponentStates();
    ClearCKObjectData();
    return CK_OK;
}

CKERROR ScriptManager::PreProcess() {
#if CKAS_BUILD_SELF_TESTS
    const CKERROR selfTestResult = RunScriptStartupSelfTests(this);
    if (selfTestResult != CK_OK) {
        return selfTestResult;
    }
#endif
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
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_Runtime) {
        m_Runtime->OnEnd();
    }
    if (m_MessageBus) {
        m_MessageBus->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    ClearComponentStates();
    return CK_OK;
}

CKERROR ScriptManager::OnCKReset() {
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_Runtime) {
        m_Runtime->OnReset();
    }
    if (m_MessageBus) {
        m_MessageBus->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    return CK_OK;
}

CKERROR ScriptManager::OnCKPause() {
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_Runtime) {
        m_Runtime->OnPause();
    }
    return CK_OK;
}

CKERROR ScriptManager::OnCKPlay() {
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

    int r = SetupScriptEngine();
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
    if (!IsInited())
        return -2;

    for (auto *execution : m_Executions) {
        delete execution;
    }
    m_Executions.clear();

    for (auto *function : m_Functions) {
        delete function;
    }
    m_Functions.clear();

    for (auto *method : m_Methods) {
        delete method;
    }
    m_Methods.clear();

    for (auto *object : m_Objects) {
        if (object && object->Object) {
            object->Object->Release();
            object->Object = nullptr;
        }
        delete object;
    }
    m_Objects.clear();

    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_MessageBus) {
        m_MessageBus->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    if (m_Runtime) {
        m_Runtime->Clear();
    }
    ClearComponentStates();

    for (auto *context : m_ScriptContexts) {
        context->Release();
    }
    m_ScriptContexts.clear();

    ClearCKObjectData();
    m_ModuleStates.clear();
    m_ScriptCache.Clear();

    m_BehaviorBridge.reset();
    m_Runtime.reset();
    m_AsyncScheduler.reset();
    m_ParameterRegistry.reset();

    if (m_ScriptEngine) {
        m_ScriptEngine->ShutDownAndRelease();
        m_ScriptEngine = nullptr;
    }
    for (ScriptEngineExtensionRegistration &extension : m_EngineExtensions) {
        extension.ActiveInCurrentEngine = false;
    }

    m_Flags &= ~AS_INITED;
    return 0;
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
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    *outEngine = m_ScriptEngine;
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
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptContext *ctx = GetActiveContext();
    if (!ctx || ctx->GetEngine() != m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "No active context belongs to this CKAngelScript manager.");
    }
    *outContext = ctx;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::SetActiveContextException(const char *message, CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(message)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Exception message is required.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptContext *ctx = GetActiveContext();
    if (!ctx || ctx->GetEngine() != m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "No active context belongs to this CKAngelScript manager.");
    }
    ctx->SetException(message);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::SetHostCallFilter(CKAngelScriptHostCallFilterCallback callback,
                                             void *userData,
                                             CKAngelScriptResult *result) {
    m_HostCallFilter = callback;
    m_HostCallFilterUserData = userData;
    return StoreResult(result, CKAS_OK);
}

bool ScriptManager::RejectHostCall(const char *apiName, CKDWORD flags) {
    if (!m_HostCallFilter) {
        return false;
    }
    return m_HostCallFilter(apiName, flags, m_HostCallFilterUserData) != CKAS_OK;
}

bool ScriptManager::RejectActiveHostCall(const char *apiName, CKDWORD flags) {
    asIScriptContext *ctx = asGetActiveContext();
    if (!ctx) {
        return false;
    }
    asIScriptEngine *engine = ctx->GetEngine();
    ScriptManager *manager = engine ? GetManager(engine) : nullptr;
    return manager ? manager->RejectHostCall(apiName, flags) : false;
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

CKAS_STATUS ScriptManager::RegisterEngineExtension(const CKAngelScriptEngineExtension &extension,
                                                         CKAngelScriptResult *result) {
    if (!ScriptApiSupport::HasCompletePublicStruct(extension)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension size is invalid.");
    }
    const char *name = ScriptApiSupport::PublicField(extension, &CKAngelScriptEngineExtension::Name, static_cast<const char *>(nullptr));
    CKAngelScriptEngineExtensionCallback callback =
        ScriptApiSupport::PublicField(extension, &CKAngelScriptEngineExtension::Register, static_cast<CKAngelScriptEngineExtensionCallback>(nullptr));
    void *userData = ScriptApiSupport::PublicField(extension, &CKAngelScriptEngineExtension::UserData, static_cast<void *>(nullptr));
    const CKDWORD flags = ScriptApiSupport::PublicField(extension,
                                      &CKAngelScriptEngineExtension::Flags,
                                      static_cast<CKDWORD>(CKAS_ENGINEEXTENSION_DEFAULT));

    if (!ScriptApiSupport::IsNonEmpty(name)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension name is required.");
    }
    if (!callback) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension callback is required.");
    }
    if (ScriptApiSupport::HasUnknownPublicFlags(flags, CKAS_ENGINEEXTENSION_DEFERRED)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown engine extension flags.");
    }
    for (const ScriptEngineExtensionRegistration &existing : m_EngineExtensions) {
        if (existing.Name == name) {
            return StoreResult(result,
                               CKAS_ALREADYEXISTS,
                               0,
                               fmt::format("Engine extension '{}' is already registered.", name));
        }
    }

    ScriptEngineExtensionRegistration retained = {};
    retained.Name = name;
    retained.ConfigGroupName = MakeEngineExtensionConfigGroupName(name);
    retained.Register = callback;
    retained.UserData = userData;
    retained.Flags = flags;

    if (m_ScriptEngine && IsInited() && !ScriptApiSupport::HasPublicFlag(flags, CKAS_ENGINEEXTENSION_DEFERRED)) {
        std::string message;
        const int code = RegisterEngineExtensionGroup(m_ScriptEngine, retained, message);
        if (code < 0) {
            const std::string summary = message.empty()
                                            ? fmt::format("Engine extension '{}' failed to register (code {}).", name, code)
                                            : message;
            if (m_Context) {
                m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
            }
            LOG_ERROR("%s", summary.c_str());
            return StoreResult(result, CKAS_EXECUTIONFAILED, code, summary);
        }
    }

    m_EngineExtensions.push_back(retained);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::UnregisterEngineExtension(const char *name,
                                                     CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(name)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension name is required.");
    }
    for (auto it = m_EngineExtensions.begin(); it != m_EngineExtensions.end(); ++it) {
        if (it->Name == name) {
            if (it->ActiveInCurrentEngine && m_ScriptEngine) {
                std::string message;
                const int code = RemoveEngineExtensionGroup(m_ScriptEngine, *it, message);
                if (code < 0) {
                    if (code == asCONFIG_GROUP_IS_IN_USE) {
                        return StoreResult(result,
                                           CKAS_INUSE,
                                           code,
                                           message.empty()
                                               ? fmt::format("Engine extension '{}' is in use.", name)
                                               : message);
                    }
                    return StoreResult(result,
                                       CKAS_EXECUTIONFAILED,
                                       code,
                                       message.empty()
                                           ? fmt::format("Failed to unregister engine extension '{}' (code {}).", name, code)
                                           : message);
                }
            }
            m_EngineExtensions.erase(it);
            return StoreResult(result, CKAS_OK);
        }
    }
    return StoreResult(result,
                       CKAS_NOTFOUND,
                       0,
                       fmt::format("Engine extension '{}' is not registered.", name));
}

void ScriptManager::BeginScriptMessageCapture() {
    m_Diagnostics.BeginScriptMessageCapture();
}

std::string ScriptManager::EndScriptMessageCapture(std::vector<CapturedScriptMessage> *messages) {
    return m_Diagnostics.EndScriptMessageCapture(messages);
}

int ScriptManager::PrepareMultithread(asIThreadManager *externalMgr) {
    return asPrepareMultithread(externalMgr);
}

void ScriptManager::UnprepareMultithread() {
    asUnprepareMultithread();
}

asIThreadManager *ScriptManager::GetThreadManager() {
    return asGetThreadManager();
}

void ScriptManager::AcquireExclusiveLock() {
    asAcquireExclusiveLock();
}

void ScriptManager::ReleaseExclusiveLock() {
    asReleaseExclusiveLock();
}

void ScriptManager::AcquireSharedLock() {
    asAcquireSharedLock();
}

void ScriptManager::ReleaseSharedLock() {
    asReleaseSharedLock();
}

int ScriptManager::AtomicInc(int &value) {
    return asAtomicInc(value);
}

int ScriptManager::AtomicDec(int &value) {
    return asAtomicDec(value);
}

int ScriptManager::ThreadCleanup() {
    return asThreadCleanup();
}

int ScriptManager::SetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc) {
    return asSetGlobalMemoryFunctions(allocFunc, freeFunc);
}

int ScriptManager::ResetGlobalMemoryFunctions() {
    return asResetGlobalMemoryFunctions();
}

void *ScriptManager::AllocMem(size_t size) {
    return asAllocMem(size);
}

void ScriptManager::FreeMem(void *mem) {
    asFreeMem(mem);
}

asILockableSharedBool *ScriptManager::CreateLockableSharedBool() {
    return asCreateLockableSharedBool();
}

CKERROR ScriptManager::ResolveScriptFileName(XString &filename) {
    CKPathManager *pm = m_Context->GetPathManager();
    if (m_ScriptPathCategoryIndex == -1) {
        SetupScriptPathCategory();
    }
    return pm->ResolveFileName(filename, m_ScriptPathCategoryIndex);
}

void * ScriptManager::GetCKObjectData(CK_ID id) const {
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

