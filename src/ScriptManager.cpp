#include "ScriptManager.h"

#include <cstdlib>
#include <fstream>

#include <fmt/format.h>

#include "CKPathManager.h"
#include "ScriptRunner.h"

#include "ScriptInfo.h"
#include "ScriptFormat.h"
#include "ScriptNativePointer.h"
#include "ScriptNativeBuffer.h"
#include "ScriptDynCall.h"
#include "ScriptUtils.h"
#include "ScriptVxMath.h"
#include "ScriptCK2.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptParameterConversion.h"
#include "ScriptParameterRegistry.h"
#include "ScriptRuntime.h"

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

#ifndef CKAS_BUILD_SELF_TESTS
#define CKAS_BUILD_SELF_TESTS 0
#endif

namespace ScriptManagerInternal {

bool IsTruthyEnvironmentValue(const char *name) {
    const char *value = std::getenv(name);
    return value && value[0] != '\0' && !(value[0] == '0' && value[1] == '\0');
}

bool ShouldRunStartupSelfTests() {
    return IsTruthyEnvironmentValue("CKAS_RUN_SELFTESTS") || IsTruthyEnvironmentValue("CKAS_SELFTEST_MARKER");
}

void WriteStartupSelfTestMarker(const char *status, const char *stage, const std::string &message) {
    const char *path = std::getenv("CKAS_SELFTEST_MARKER");
    if (!path || path[0] == '\0') {
        return;
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        return;
    }

    out << "status=" << (status ? status : "unknown") << "\n";
    out << "stage=" << (stage ? stage : "unknown") << "\n";
    if (!message.empty()) {
        out << "message=" << message << "\n";
    }
}

} // namespace ScriptManagerInternal

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
    if (m_Runtime) {
        m_Runtime->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    ClearComponentStates();
    ClearCKObjectData();
    return CK_OK;
}

CKERROR ScriptManager::PreProcess() {
    const CKERROR selfTestResult = RunStartupSelfTests();
    if (selfTestResult != CK_OK) {
        return selfTestResult;
    }
    if (m_Runtime) {
        m_Runtime->PreProcess();
    }
    return m_BehaviorBridge ? m_BehaviorBridge->PreProcess() : CK_OK;
}

CKERROR ScriptManager::PostProcess() {
    return m_BehaviorBridge ? m_BehaviorBridge->PostProcess() : CK_OK;
}

CKERROR ScriptManager::OnCKInit() {
    return CK_OK;
}

CKERROR ScriptManager::RunStartupSelfTests() {
    if (m_StartupSelfTestsAttempted) {
        return CK_OK;
    }
    if (!ScriptManagerInternal::ShouldRunStartupSelfTests()) {
        return CK_OK;
    }
    m_StartupSelfTestsAttempted = true;

#if !CKAS_BUILD_SELF_TESTS
    constexpr const char *message = "Startup self-tests were not compiled. Configure with -DCKAS_BUILD_SELF_TESTS=ON to enable them.";
    if (m_Context) {
        m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), message);
    }
    ScriptManagerInternal::WriteStartupSelfTestMarker("failed", "disabled", message);
    return CKERR_INVALIDOPERATION;
#else
    ScriptManagerInternal::WriteStartupSelfTestMarker("running", "start", std::string());

    std::string conversionError;
    ScriptManagerInternal::WriteStartupSelfTestMarker("running", "parameter-conversion", std::string());
    if (!RunScriptParameterConversionSelfTest(conversionError)) {
        if (m_Context) {
            m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] Parameter conversion self-test failed: %s"),
                                         conversionError.c_str());
        }
        ScriptManagerInternal::WriteStartupSelfTestMarker("failed", "parameter-conversion", conversionError);
        return CKERR_INVALIDOPERATION;
    }
    ScriptManagerInternal::WriteStartupSelfTestMarker("running", "parameter-registry", std::string());
    if (!RunScriptParameterRegistrySelfTest(m_Context, conversionError)) {
        if (m_Context) {
            m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] Parameter registry self-test failed: %s"),
                                         conversionError.c_str());
        }
        ScriptManagerInternal::WriteStartupSelfTestMarker("failed", "parameter-registry", conversionError);
        return CKERR_INVALIDOPERATION;
    }
    ScriptManagerInternal::WriteStartupSelfTestMarker("running", "component-metadata", std::string());
    if (!RunScriptComponentMetadataSelfTest(conversionError)) {
        if (m_Context) {
            m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] Component metadata self-test failed: %s"),
                                         conversionError.c_str());
        }
        ScriptManagerInternal::WriteStartupSelfTestMarker("failed", "component-metadata", conversionError);
        return CKERR_INVALIDOPERATION;
    }
    ScriptManagerInternal::WriteStartupSelfTestMarker("running", "behavior-bridge", std::string());
    if (!RunScriptBehaviorBridgeSelfTest(m_Context, m_ScriptEngine, conversionError)) {
        if (m_Context) {
            m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] Behavior bridge self-test failed: %s"),
                                         conversionError.c_str());
        }
        ScriptManagerInternal::WriteStartupSelfTestMarker("failed", "behavior-bridge", conversionError);
        return CKERR_INVALIDOPERATION;
    }
    ScriptManagerInternal::WriteStartupSelfTestMarker("running", "runtime", std::string());
    if (!RunScriptRuntimeSelfTest(m_Context, m_ScriptEngine, conversionError)) {
        if (m_Context) {
            m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] Runtime self-test failed: %s"),
                                         conversionError.c_str());
        }
        ScriptManagerInternal::WriteStartupSelfTestMarker("failed", "runtime", conversionError);
        return CKERR_INVALIDOPERATION;
    }
    ScriptManagerInternal::WriteStartupSelfTestMarker("ok", "complete", std::string());
    return CK_OK;
#endif
}

CKERROR ScriptManager::OnCKEnd() {
    if (m_Runtime) {
        m_Runtime->OnEnd();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    ClearComponentStates();
    return CK_OK;
}

CKERROR ScriptManager::OnCKReset() {
    if (m_Runtime) {
        m_Runtime->OnReset();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    return CK_OK;
}

CKERROR ScriptManager::OnCKPause() {
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

    m_Flags |= AS_INITED;
    return r;
}

int ScriptManager::Shutdown() {
    if (!IsInited())
        return -2;

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
    m_ScriptCache.Clear();

    m_BehaviorBridge.reset();
    m_Runtime.reset();
    m_ParameterRegistry.reset();

    if (m_ScriptEngine) {
        m_ScriptEngine->ShutDownAndRelease();
        m_ScriptEngine = nullptr;
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

asIScriptContext *ScriptManager::GetActiveContext() {
    return asGetActiveContext();
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

asIScriptEngine *ScriptManager::GetScriptEngine() {
    return m_ScriptEngine;
}

int ScriptManager::LoadScript(const char *scriptName, const char *filename) {
    if (!scriptName || scriptName[0] == '\0')
        return -1;

    if (!m_ScriptEngine)
        return -2;

    XString scriptFilename;
    if (filename) {
        scriptFilename = filename;
    } else {
        scriptFilename = scriptName;
        scriptFilename += ".as";
    }

    auto cache = m_ScriptCache.LoadScript(m_ScriptEngine, scriptName, scriptFilename.CStr());
    if (!cache)
        return -3;
    return 0;
}

int ScriptManager::LoadScripts(const char *scriptName, const char **filenames, size_t count) {
    if (!scriptName || scriptName[0] == '\0')
        return -1;

    if (!m_ScriptEngine)
        return -2;

    std::vector<std::string> files;
    for (size_t i = 0; i < count; i++) {
        XString scriptFilename = filenames[i];
        if (scriptFilename.Find(".as") == XString::NOTFOUND)
            scriptFilename += ".as";
        ResolveScriptFileName(scriptFilename);
        files.emplace_back(scriptFilename.CStr());
    }

    auto cache = m_ScriptCache.LoadScript(m_ScriptEngine, scriptName, files);
    if (!cache)
        return -3;
    return 0;
}

int ScriptManager::CompileScript(const char *scriptName, const char *scriptCode) {
    if (!scriptName || scriptName[0] == '\0')
        return -1;

    if (!scriptCode)
        return -1;

    if (!m_ScriptEngine)
        return -2;

    auto cache = m_ScriptCache.CompileScript(m_ScriptEngine, scriptName, scriptCode);
    if (!cache)
        return -3;
    return 0;
}

bool ScriptManager::UnloadScript(const char *scriptName) {
    if (!scriptName || scriptName[0] == '\0')
        return false;
    return m_ScriptCache.UnloadScript(scriptName);
}

asIScriptModule *ScriptManager::GetScript(const char *scriptName) {
    if (!m_ScriptEngine)
        return nullptr;
    return m_ScriptEngine->GetModule(scriptName, asGM_ONLY_IF_EXISTS);
}

CKERROR ScriptManager::ResolveScriptFileName(XString &filename) {
    CKPathManager *pm = m_Context->GetPathManager();
    if (m_ScriptPathCategoryIndex == -1) {
        SetupScriptPathCategory();
    }
    return pm->ResolveFileName(filename, m_ScriptPathCategoryIndex);
}

void * ScriptManager::GetCKObjectData(CK_ID id) const {
    const auto it = m_CKObjectDataMap.find(id);
    if (it == m_CKObjectDataMap.end()) {
        return nullptr;
    }
    return it->second;
}

void ScriptManager::SetCKObjectData(CK_ID id, void *data) {
    if (data) {
        m_CKObjectDataMap[id] = data;
    } else {
        m_CKObjectDataMap.erase(id);
    }
}

void ScriptManager::ReleaseCKObjectData(CK_ID id) {
    const auto it = m_CKObjectDataMap.find(id);
    if (it == m_CKObjectDataMap.end()) {
        return;
    }

    auto *func = static_cast<asIScriptFunction *>(it->second);
    m_CKObjectDataMap.erase(it);
    if (func) {
        func->Release();
    }
}

void ScriptManager::ClearCKObjectData() {
    for (const auto &entry : m_CKObjectDataMap) {
        auto *func = static_cast<asIScriptFunction *>(entry.second);
        if (func) {
            func->Release();
        }
    }
    m_CKObjectDataMap.clear();

    for (const auto &entry : m_CKObjectCallbackMap) {
        for (auto *func : entry.second) {
            if (!func) {
                continue;
            }
            if (IsMarkedAsReleasedOnce(func)) {
                ClearReleasedOnceMark(func);
            } else {
                if (IsMarkedAsTemporary(func)) {
                    ClearTemporaryMark(func);
                }
                func->Release();
            }
        }
    }
    m_CKObjectCallbackMap.clear();
}

void ScriptManager::TrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    if (func) {
        m_CKObjectCallbackMap[id].push_back(func);
    }
}

void ScriptManager::UntrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    auto it = m_CKObjectCallbackMap.find(id);
    if (it == m_CKObjectCallbackMap.end()) {
        return;
    }

    auto &callbacks = it->second;
    for (auto cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        if (*cb == func) {
            callbacks.erase(cb);
            break;
        }
    }

    if (callbacks.empty()) {
        m_CKObjectCallbackMap.erase(it);
    }
}

void ScriptManager::ReleaseCKObjectCallbacks(CK_ID id) {
    auto it = m_CKObjectCallbackMap.find(id);
    if (it == m_CKObjectCallbackMap.end()) {
        return;
    }

    auto callbacks = std::move(it->second);
    m_CKObjectCallbackMap.erase(it);
    for (auto *func : callbacks) {
        if (!func) {
            continue;
        }
        if (IsMarkedAsReleasedOnce(func)) {
            ClearReleasedOnceMark(func);
        } else {
            if (IsMarkedAsTemporary(func)) {
                ClearTemporaryMark(func);
            }
            func->Release();
        }
    }
}

static void ReleaseScriptFunction(asIScriptFunction *&func) {
    if (!func) {
        return;
    }

    func->Release();
    func = nullptr;
}

ScriptComponentState *ScriptManager::GetOrCreateComponentState(CKBehavior *behavior) {
    if (!behavior) {
        return nullptr;
    }

    const CK_ID id = behavior->GetID();
    auto it = m_ComponentStates.find(id);
    if (it != m_ComponentStates.end()) {
        it->second->Behavior = behavior;
        return it->second.get();
    }

    auto state = std::make_unique<ScriptComponentState>();
    state->BehaviorId = id;
    state->Behavior = behavior;

    ScriptComponentState *raw = state.get();
    m_ComponentStates[id] = std::move(state);
    return raw;
}

ScriptComponentState *ScriptManager::GetComponentState(CK_ID id) const {
    auto it = m_ComponentStates.find(id);
    if (it == m_ComponentStates.end()) {
        return nullptr;
    }

    return it->second.get();
}

void ScriptManager::ResetComponentStateRuntime(ScriptComponentState *state, bool unloadPrivateModule) {
    if (!state) {
        return;
    }

    if (m_BehaviorBridge && state->BehaviorId) {
        m_BehaviorBridge->DestroyComponentTasks(state->BehaviorId);
    }

    ReleaseScriptFunction(state->OnLoad);
    ReleaseScriptFunction(state->Awake);
    ReleaseScriptFunction(state->OnEnable);
    ReleaseScriptFunction(state->Start);
    ReleaseScriptFunction(state->Update);
    ReleaseScriptFunction(state->OnDisable);
    ReleaseScriptFunction(state->OnDestroy);
    ReleaseScriptFunction(state->OnReset);

    if (state->Object) {
        state->Object->Release();
        state->Object = nullptr;
    }

    if (state->Runner) {
        state->Runner->Reset();
        delete state->Runner;
        state->Runner = nullptr;
    }

    if (unloadPrivateModule && state->PrivateModule && !state->RuntimeModuleName.empty()) {
        UnloadScript(state->RuntimeModuleName.c_str());
    }

    state->RuntimeModuleName.clear();
    state->Bindings.clear();
    state->PrivateModule = false;
    state->Loaded = false;
    state->OnLoadCalled = false;
    state->AwakeCalled = false;
    state->StartCalled = false;
    state->InstanceEnabled = false;
    state->Failed = false;
}

void ScriptManager::ReleaseComponentState(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }

    ScriptComponentState *state = GetComponentState(behavior->GetID());
    if (state && state->Behavior) {
        ScriptComponentState *nullState = nullptr;
        state->Behavior->SetLocalParameterValue(0, &nullState);
    }

    ReleaseComponentState(behavior->GetID());
}

void ScriptManager::ReleaseComponentState(CK_ID id) {
    auto it = m_ComponentStates.find(id);
    if (it == m_ComponentStates.end()) {
        return;
    }

    if (m_BehaviorBridge) {
        m_BehaviorBridge->DestroyComponentTasks(id);
    }
    ResetComponentStateRuntime(it->second.get(), true);
    m_ComponentStates.erase(it);
}

void ScriptManager::ClearComponentStates() {
    for (auto &entry : m_ComponentStates) {
        if (m_BehaviorBridge) {
            m_BehaviorBridge->DestroyComponentTasks(entry.first);
        }
        ResetComponentStateRuntime(entry.second.get(), true);
    }
    m_ComponentStates.clear();
}

ScriptBehaviorBridge *ScriptManager::GetBehaviorBridge() {
    if (!m_BehaviorBridge) {
        m_BehaviorBridge = std::make_unique<ScriptBehaviorBridge>(this);
    }
    return m_BehaviorBridge.get();
}

void ScriptManager::MessageCallback(const asSMessageInfo &msg) {
    const char *type = "NULL";
    switch (msg.type) {
        case asMSGTYPE_ERROR:
            type = "ERROR";
            break;
        case asMSGTYPE_WARNING:
            type = "WARN";
            break;
        case asMSGTYPE_INFORMATION:
            type = "INFO";
            break;
    }
    m_Context->OutputToConsoleEx(const_cast<char *>("%s(%d,%d): %s: %s"),
        msg.section ? msg.section : "<unknown section>",
        msg.row,
        msg.col,
        type,
        msg.message ? msg.message : "");
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
    asIScriptContext *ctx = nullptr;
    if (!m_ScriptContexts.empty()) {
        ctx = *m_ScriptContexts.rbegin();
        m_ScriptContexts.pop_back();
    } else
        ctx = m_ScriptEngine->CreateContext();

    ctx->SetExceptionCallback(asMETHOD(ScriptManager, ExceptionCallback), this, asCALL_THISCALL);
    return ctx;
}

void ScriptManager::ReturnContextToPool(asIScriptContext *ctx) {
    if (!ctx) {
        return;
    }

    // Unprepare the context to free any objects that might be held
    // as we don't know when the context will be used again.
    ctx->Unprepare();
    m_ScriptContexts.push_back(ctx);
}

void ScriptManager::SetupScriptPathCategory() {
    if (m_ScriptPathCategoryIndex == -1) {
        XString category = "Script Paths";
        CKPathManager *pm = m_Context->GetPathManager();
        m_ScriptPathCategoryIndex = pm->GetCategoryIndex(category);
        if (m_ScriptPathCategoryIndex == -1)
            m_ScriptPathCategoryIndex = pm->AddCategory(category);
    }
}

int ScriptManager::SetupScriptEngine() {
    // #if CKVERSION == 0x13022002
    //     asSetGlobalMemoryFunctions(
    //         [](size_t size) { return VxMalloc(size); },
    //         [](void *ptr) { VxFree(ptr); }
    //     );
    // #endif

    m_ScriptEngine = asCreateScriptEngine();
    if (!m_ScriptEngine) {
        m_Context->OutputToConsole(const_cast<char *>("Failed to create script engine."));
        return -1;
    }

    m_ScriptEngine->SetUserData(this, SCRIPT_MANAGER_TYPE);
    m_ScriptEngine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
    m_ScriptEngine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, true);
    m_ScriptEngine->SetEngineProperty(asEP_ALLOW_IMPLICIT_HANDLE_TYPES, true);
    m_ScriptEngine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);
    m_ScriptEngine->SetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE, 1);

    // The script compiler will send any compiler messages to the callback
    int r = m_ScriptEngine->SetMessageCallback(asMETHOD(ScriptManager, MessageCallback), this, asCALL_THISCALL);
    if (r < 0)
        return r;

    // The script handle the pool of script contexts.
    r = m_ScriptEngine->SetContextCallbacks([](asIScriptEngine *, void *param) {
        auto *man = static_cast<ScriptManager *>(param);
        return man->RequestContextFromPool();
    }, [](asIScriptEngine *, asIScriptContext *ctx, void *param) {
        auto *man = static_cast<ScriptManager *>(param);
        man->ReturnContextToPool(ctx);
    }, this);
    if (r < 0)
        return r;

    m_ScriptEngine->SetEngineUserDataCleanupCallback([](asIScriptEngine *engine) {
        engine->SetUserData(nullptr, SCRIPT_MANAGER_TYPE);
    }, SCRIPT_MANAGER_TYPE);
    m_ScriptEngine->SetFunctionUserDataCleanupCallback([](asIScriptFunction *func) {
        func->SetUserData(nullptr, AS_TEMPORARY_FLAG_TYPE);
    }, AS_TEMPORARY_FLAG_TYPE);
    m_ScriptEngine->SetFunctionUserDataCleanupCallback([](asIScriptFunction *func) {
        func->SetUserData(nullptr, AS_RELEASED_ONCE_FLAG_TYPE);
    }, AS_RELEASED_ONCE_FLAG_TYPE);

    // Register the standard types
    RegisterStdTypes(m_ScriptEngine);

    // Register the standard add-ons
    RegisterStdAddons(m_ScriptEngine);

    // Register the native types
    RegisterNativePointer(m_ScriptEngine);
    RegisterNativeBuffer(m_ScriptEngine);

    // Register the DynCall APIs
    RegisterScriptDynCall(m_ScriptEngine);
    RegisterScriptDynCallback(m_ScriptEngine);
    RegisterScriptDynLoad(m_ScriptEngine);

    // Register the function that we want the scripts to call
    RegisterScriptFormat(m_ScriptEngine);

    RegisterScriptInfo(m_ScriptEngine);

    // Register the Virtools API
    RegisterVirtools(m_ScriptEngine);

    return r;
}

void ScriptManager::RegisterStdTypes(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    if constexpr (sizeof(void *) == 4) {
        r = engine->RegisterTypedef("size_t", "uint"); assert(r >= 0);
        r = engine->RegisterTypedef("ptrdiff_t", "int"); assert(r >= 0);
        r = engine->RegisterTypedef("intptr_t", "int"); assert(r >= 0);
        r = engine->RegisterTypedef("uintptr_t", "uint"); assert(r >= 0);
    } else {
        r = engine->RegisterTypedef("size_t", "uint64"); assert(r >= 0);
        r = engine->RegisterTypedef("ptrdiff_t", "int64"); assert(r >= 0);
        r = engine->RegisterTypedef("intptr_t", "int64"); assert(r >= 0);
        r = engine->RegisterTypedef("uintptr_t", "uint64"); assert(r >= 0);
    }
}

void ScriptManager::RegisterStdAddons(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterStdString(engine);
    RegisterScriptArray(engine, true);
    RegisterStdStringUtils(engine);
    RegisterScriptAny(engine);
    RegisterScriptHandle(engine);
    RegisterScriptWeakRef(engine);
    RegisterScriptDictionary(engine);
    RegisterScriptDateTime(engine);
    RegisterScriptFile(engine);
    RegisterScriptFileSystem(engine);
    RegisterScriptMath(engine);
    RegisterScriptMathComplex(engine);
    RegisterScriptGrid(engine);
    RegisterExceptionRoutines(engine);
}

void ScriptManager::RegisterVirtools(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterVxMath(m_ScriptEngine);
    RegisterCK2(m_ScriptEngine);
    RegisterScriptParameterRegistry(m_ScriptEngine);
    RegisterScriptBehaviorBridge(m_ScriptEngine);
    RegisterScriptRuntime(m_ScriptEngine);
}
