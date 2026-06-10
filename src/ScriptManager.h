#ifndef CK_SCRIPTMANAGER_H
#define CK_SCRIPTMANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <angelscript.h>

#include "CKBaseManager.h"
#include "CKContext.h"
#include "CKAngelScript.h"

#include "ScriptCache.h"

#define SCRIPT_MANAGER_GUID CKGUID(0x70955bd2,0x30684456)

#define SCRIPT_MANAGER_TYPE 3000

class CKBehavior;
class ScriptAsyncScheduler;
class ScriptBehaviorBridge;
class ScriptMessage;
class ScriptMessageBus;
class ScriptParameterRegistry;
class ScriptRuntime;
class ScriptInvoker;

enum class ScriptComponentBindingKind {
    Auto,
    Int,
    Float,
    Bool,
    String,
    Guid,
    Vector,
    Vector2,
    Color,
    Quaternion,
    Matrix,
    ObjectArray,
    Object,
    ParamRef,
    ParamValue,
    ParamTypeInfo,
    BehaviorRef,
    BBPrototype,
    BBDecl,
    BBSlot,
    BBConfig
};

struct ScriptComponentRequiredSlot {
    std::string KindName;
    std::string Name;
    int Occurrence = 0;
};

struct ScriptComponentNamedSlotValue {
    std::string Name;
    std::string Value;
    int Occurrence = 0;
    bool HasValue = false;
};

struct ScriptComponentSourceSlot {
    std::string PinName;
    std::string SourceFieldName;
    std::string SourceSlotName;
    int PinOccurrence = 0;
    int SourceOccurrence = 0;
};

enum class ScriptComponentBBStepPolicy {
    Manual,
    EachUpdate,
    OnChange
};

enum class ScriptComponentBBConfigLifetime {
    Component,
    Manual
};

struct ScriptComponentBinding {
    std::string FieldName;
    std::string ParameterName;
    std::string TypeName;
    std::string DefaultValue;
    bool HasDefault = false;
    bool InjectEveryFrame = true;
    bool HandleInjected = false;
    CK_ID LastObjectId = 0;
    std::string LastTextValue;
    std::string SlotFromFieldName;
    std::string SlotPrototypeName;
    std::string SlotKindName;
    std::string SlotName;
    int SlotOccurrence = 0;
    CKDWORD SlotMetadataFlags = 0;
    std::string SlotValue;
    ScriptComponentBBConfigLifetime BBConfigLifetime = ScriptComponentBBConfigLifetime::Component;
    bool HasBBConfigLifetime = false;
    std::string BindingStartInput;
    std::string BindingStopInput;
    std::vector<ScriptComponentRequiredSlot> RequiredSlots;
    std::vector<ScriptComponentNamedSlotValue> ConfigPinValues;
    std::vector<ScriptComponentNamedSlotValue> ConfigSettingValues;
    std::vector<ScriptComponentSourceSlot> ConfigSources;
    std::string BBConfigOwnerExpression;
    std::string BBConfigTargetExpression;
    ScriptComponentBBStepPolicy BBStepPolicy = ScriptComponentBBStepPolicy::Manual;
    bool AutoStartBBConfig = false;
    bool HasAutoStartBBConfig = false;
    bool HasBBStepPolicy = false;
    bool BBConfigChanged = false;
    std::string MetadataError;

    ScriptComponentBindingKind Kind = ScriptComponentBindingKind::Auto;
    CKGUID ParameterGuid;
    int PropertyIndex = -1;
    int PropertyTypeId = 0;
    int InputParameterIndex = -1;
};

struct ScriptComponentState {
    CK_ID BehaviorId = 0;
    CKBehavior *Behavior = nullptr;
    ScriptInvoker *Invoker = nullptr;
    asIScriptObject *Object = nullptr;

    asIScriptFunction *OnLoad = nullptr;
    asIScriptFunction *Awake = nullptr;
    asIScriptFunction *OnEnable = nullptr;
    asIScriptFunction *Start = nullptr;
    asIScriptFunction *Update = nullptr;
    asIScriptFunction *OnDisable = nullptr;
    asIScriptFunction *OnDestroy = nullptr;
    asIScriptFunction *OnReset = nullptr;
    asIScriptFunction *OnMessage = nullptr;
    asIScriptFunction *ActiveLifecycle = nullptr;
    std::string ActiveLifecycleName;

    std::string ScriptName;
    std::string ClassName;
    std::string Source;
    std::string File;
    std::string Manifest;
    std::string RuntimeModuleName;
    std::string MessageTarget;
    std::vector<ScriptComponentBinding> Bindings;
    std::vector<std::string> MessageTopics;
    std::vector<std::string> ManagedInputParameterNames;

    bool PrivateModule = false;
    bool Loaded = false;
    bool StaticMessageSubscriptionsRegistered = false;
    bool OnLoadCalled = false;
    bool AwakeCalled = false;
    bool StartCalled = false;
    bool DesiredEnabled = false;
    bool InstanceEnabled = false;
    bool ScriptActive = false;
    bool Paused = false;
    bool Failed = false;
    bool PendingDestroy = false;
    bool PendingDisableOutput = false;
    bool PendingResetRuntime = false;
};

struct ScriptEngineExtensionRegistration {
    std::string Name;
    CKAngelScriptEngineExtensionCallback Register = nullptr;
    void *UserData = nullptr;
    CKDWORD Flags = CKAS_ENGINEEXTENSION_DEFAULT;
};

class ScriptManager : public CKBaseManager {
public:
    enum Flag {
        AS_INITED = 0x00000001,
    };

    explicit ScriptManager(CKContext *context);

    ~ScriptManager() override;

    CKStateChunk *SaveData(CKFile *SavedFile) override;
    CKERROR LoadData(CKStateChunk *chunk, CKFile *LoadedFile) override;

    CKERROR PostClearAll() override;
    CKERROR PreProcess() override;
    CKERROR PostProcess() override;

    CKERROR OnCKInit() override;
    CKERROR OnCKEnd() override;

    CKERROR OnCKReset() override;
    CKERROR OnCKPause() override;
    CKERROR OnCKPlay() override;

    CKERROR PostLoad() override;

    CKERROR OnPostCopy(CKDependenciesContext &context) override;

    CKDWORD GetValidFunctionsMask() override {
        return CKMANAGER_FUNC_PostClearAll |
               CKMANAGER_FUNC_PreProcess |
               CKMANAGER_FUNC_PostProcess |
               CKMANAGER_FUNC_OnCKInit |
               CKMANAGER_FUNC_OnCKEnd |
               CKMANAGER_FUNC_OnCKPlay |
               CKMANAGER_FUNC_OnCKReset |
               CKMANAGER_FUNC_OnCKPause |
               CKMANAGER_FUNC_PostLoad |
               CKMANAGER_FUNC_OnPostCopy;
    }

    // Engine
    asIScriptEngine *GetScriptEngine();
    const char *GetVersion();
    const char *GetOptions();
    CKAS_STATUS BorrowEngine(asIScriptEngine **outEngine, CKAngelScriptResult *result = nullptr);

    // Context
    CKContext *GetCKContext() const {
        return m_Context;
    }

    asIScriptContext *GetActiveContext();
    CKAS_STATUS BorrowActiveContext(asIScriptContext **outContext, CKAngelScriptResult *result = nullptr);

    // Internal low-level AngelScript helpers. The public AngelScript C API
    // intentionally exposes only module and execution operations.
    int PrepareMultithread(asIThreadManager *externalMgr = nullptr);
    void UnprepareMultithread();
    asIThreadManager *GetThreadManager();
    void AcquireExclusiveLock();
    void ReleaseExclusiveLock();
    void AcquireSharedLock();
    void ReleaseSharedLock();
    int AtomicInc(int &value);
    int AtomicDec(int &value);
    int ThreadCleanup();
    int SetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc);
    int ResetGlobalMemoryFunctions();
    void *AllocMem(size_t size);
    void FreeMem(void *mem);
    asILockableSharedBool *CreateLockableSharedBool();

    CKAS_STATUS LoadModule(const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result = nullptr);
    CKAS_STATUS CompileModule(const char *moduleName,
                                    const char *scriptCode,
                                    CKDWORD flags = CKAS_COMPILE_DEFAULT,
                                    CKAngelScriptResult *result = nullptr);
    CKAS_STATUS UnloadModule(const char *moduleName, CKAngelScriptResult *result = nullptr);
    bool HasModule(const char *moduleName);
    CKDWORD GetModuleGeneration(const char *moduleName) const;
    asIScriptModule *GetModule(const char *moduleName);
    CKAS_STATUS BorrowModule(const char *moduleName,
                             asIScriptModule **outModule,
                             CKAngelScriptResult *result = nullptr);
    CKAS_STATUS BorrowFunctionByName(const char *moduleName,
                                     const char *functionName,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result = nullptr);
    CKAS_STATUS BorrowFunctionByDecl(const char *moduleName,
                                     const char *functionDecl,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result = nullptr);
    CKAS_STATUS FindFunction(const CKAngelScriptFunctionOptions &options,
                             CKAngelScriptFunction **outFunction,
                             CKAngelScriptResult *result = nullptr);
    CKAS_STATUS ReleaseFunction(CKAngelScriptFunction *function);
    CKAS_STATUS CreateObject(const CKAngelScriptObjectOptions &options,
                             CKAngelScriptObject **outObject,
                             CKAngelScriptResult *result = nullptr);
    CKAS_STATUS ReleaseObject(CKAngelScriptObject *object);
    CKAS_STATUS FindObjectMethod(const CKAngelScriptMethodOptions &options,
                                 CKAngelScriptMethod **outMethod,
                                 CKAngelScriptResult *result = nullptr);
    CKAS_STATUS ReleaseMethod(CKAngelScriptMethod *method);
    CKAS_STATUS CallObjectMethod(const CKAngelScriptObjectMethodExecuteOptions &options,
                                 CKAngelScriptResult *result = nullptr);

    CKAS_STATUS CreateFunctionExecution(const CKAngelScriptFunctionExecutionOptions &options,
                                        CKAngelScriptExecution **outExecution,
                                        CKAngelScriptResult *result = nullptr);
    CKAS_STATUS StartExecution(CKAngelScriptExecution *execution);
    CKAS_STATUS ResumeExecution(CKAngelScriptExecution *execution);
    CKAS_STATUS CancelExecution(CKAngelScriptExecution *execution);
    CKAS_STATUS ReleaseExecution(CKAngelScriptExecution *execution);
    CKAS_STATUS GetExecutionState(const CKAngelScriptExecution *execution,
                                  CKAS_EXECUTIONSTATE *outState);
    CKAS_STATUS BorrowExecutionResult(const CKAngelScriptExecution *execution,
                                      const CKAngelScriptResult **outResult);
    const CKAngelScriptResult *GetLastResult() const;
    CKAS_STATUS StoreApiResult(CKAngelScriptResult *out,
                               CKAS_STATUS status,
                               int angelScriptCode = 0,
                               const char *errorMessage = nullptr,
                               const char *stackTrace = nullptr);
    CKAS_STATUS RegisterEngineExtension(const CKAngelScriptEngineExtension &extension,
                                              CKAngelScriptResult *result = nullptr);
    CKAS_STATUS UnregisterEngineExtension(const char *name,
                                                CKAngelScriptResult *result = nullptr);

    asIScriptModule *GetScript(const char *scriptName);
    std::shared_ptr<CachedScript> GetCachedScript(const char *scriptName);
    std::shared_ptr<CachedScript> NewCachedScript(const char *scriptName);
    bool RestoreCachedScriptFromChunk(const char *scriptName, CKStateChunk *chunk);
    bool SaveCachedScriptToChunk(const char *scriptName, CKStateChunk *chunk);
    bool ClearCachedScriptCode(const char *scriptName);

    CKERROR ResolveScriptFileName(XString &filename);

    void *GetCKObjectData(CK_ID id) const;
    void SetCKObjectData(CK_ID id, void *data);
    void ReleaseCKObjectData(CK_ID id);
    void ClearCKObjectData();
    void TrackCKObjectCallback(CK_ID id, asIScriptFunction *func);
    void UntrackCKObjectCallback(CK_ID id, asIScriptFunction *func);
    void ReleaseCKObjectCallbacks(CK_ID id);

    ScriptComponentState *GetOrCreateComponentState(CKBehavior *behavior);
    ScriptComponentState *GetComponentState(CK_ID id) const;
    void ResetComponentStateRuntime(ScriptComponentState *state, bool unloadPrivateModule = true);
    void ReleaseComponentState(CKBehavior *behavior);
    void ReleaseComponentState(CK_ID id);
    void ClearComponentStates();
    bool DeliverComponentMessage(CK_ID id, const ScriptMessage &message, bool immediate, std::string &error);

    ScriptBehaviorBridge *GetBehaviorBridge();
    ScriptRuntime *GetRuntime() const {
        return m_Runtime.get();
    }
    ScriptAsyncScheduler *GetAsyncScheduler() const {
        return m_AsyncScheduler.get();
    }
    ScriptMessageBus *GetMessageBus() const {
        return m_MessageBus.get();
    }
    ScriptParameterRegistry *GetParameterRegistry() const {
        return m_ParameterRegistry.get();
    }

    bool IsInited() const {
        return (m_Flags & AS_INITED) != 0;
    }

    int Init();
    int Shutdown();

    void MessageCallback(const asSMessageInfo &msg);
    void ExceptionCallback(asIScriptContext *context);
    std::string GetCallStack(asIScriptContext *context);

    asIScriptContext *RequestContextFromPool();
    void ReturnContextToPool(asIScriptContext *ctx);

    static ScriptManager *GetManager(CKContext *context) {
        return (ScriptManager *) context->GetManagerByGuid(SCRIPT_MANAGER_GUID);
    }

    static ScriptManager *GetManager(asIScriptEngine *engine) {
        return (ScriptManager *) engine->GetUserData(SCRIPT_MANAGER_TYPE);
    }

protected:
    void SetupScriptPathCategory();
    int SetupScriptEngine();

    void RegisterStdTypes(asIScriptEngine *engine);
    void RegisterStdAddons(asIScriptEngine *engine);
    void RegisterVirtools(asIScriptEngine *engine);
    int RegisterEngineExtensions(asIScriptEngine *engine);

    bool OwnsExecution(const CKAngelScriptExecution *execution) const;
    bool OwnsFunction(const CKAngelScriptFunction *function) const;
    bool OwnsObject(const CKAngelScriptObject *object) const;
    bool OwnsMethod(const CKAngelScriptMethod *method) const;
    bool HasExecutionForModule(const char *moduleName) const;
    bool HasRuntimeHandleForModule(const char *moduleName) const;
    void BumpModuleGeneration(const char *moduleName);
    struct ModuleReplacementSnapshot {
        std::shared_ptr<CachedScript> Cache;
        std::vector<std::tuple<std::string, std::string>> Sections;
        ScriptMetadata Metadata;
        std::vector<unsigned char> ByteCode;
        bool HasModule = false;
    };
    std::shared_ptr<CachedScript> BuildTransientModule(
        const char *moduleName,
        const std::vector<std::tuple<std::string, std::string>> &sections,
        int &angelScriptCode,
        std::string &diagnostics);
    bool CaptureModuleReplacementSnapshot(const char *moduleName,
                                          ModuleReplacementSnapshot &snapshot,
                                          int &angelScriptCode,
                                          std::string &errorMessage);
    void RemoveModuleForReplacement(const char *moduleName,
                                    ModuleReplacementSnapshot &snapshot);
    bool RestoreModuleReplacementSnapshot(const char *moduleName,
                                          ModuleReplacementSnapshot &snapshot,
                                          int &angelScriptCode,
                                          std::string &errorMessage);
    CKAS_STATUS ReplaceModuleFromSections(
        const char *moduleName,
        const std::vector<std::tuple<std::string, std::string>> &sections,
        CKAngelScriptResult *result = nullptr);
    // Internal low-level shims backing the public module API. Do not call these
    // from behavior blocks or runtime helpers; use LoadModule/CompileModule/
    // UnloadModule and the cache helper methods above.
    int LoadModuleFromDefaultOrFile(const char *moduleName, const char *filename);
    int LoadModuleFromFiles(const char *moduleName, const char **filenames, size_t count);
    int CompileModuleFromMemory(const char *moduleName, const char *scriptCode);
    bool DiscardCachedModule(const char *moduleName);
    CKAngelScriptResult MakeResult(CKAS_STATUS status,
                                 int angelScriptCode = 0,
                                 const std::string &errorMessage = std::string(),
                                 const std::string &stackTrace = std::string());
    CKAS_STATUS StoreResult(CKAngelScriptResult *out,
                                  CKAS_STATUS status,
                                  int angelScriptCode = 0,
                                  const std::string &errorMessage = std::string(),
                                  const std::string &stackTrace = std::string());
    void BeginScriptMessageCapture();
    std::string EndScriptMessageCapture();

    int m_Flags = 0;
    int m_ScriptPathCategoryIndex = -1;
    asIScriptEngine *m_ScriptEngine = nullptr;
    ScriptCache m_ScriptCache;
    std::vector<asIScriptContext *> m_ScriptContexts;
    std::unordered_map<CK_ID, void *> m_CKObjectDataMap;
    std::unordered_map<CK_ID, std::vector<asIScriptFunction *> > m_CKObjectCallbackMap;
    std::unordered_map<CK_ID, std::unique_ptr<ScriptComponentState> > m_ComponentStates;
    std::unique_ptr<ScriptBehaviorBridge> m_BehaviorBridge;
    std::unique_ptr<ScriptParameterRegistry> m_ParameterRegistry;
    std::unique_ptr<ScriptRuntime> m_Runtime;
    std::unique_ptr<ScriptAsyncScheduler> m_AsyncScheduler;
    std::unique_ptr<ScriptMessageBus> m_MessageBus;
    std::unordered_set<CKAngelScriptExecution *> m_Executions;
    std::unordered_set<CKAngelScriptFunction *> m_Functions;
    std::unordered_set<CKAngelScriptObject *> m_Objects;
    std::unordered_set<CKAngelScriptMethod *> m_Methods;
    std::unordered_map<std::string, CKDWORD> m_ModuleGenerations;
    std::vector<ScriptEngineExtensionRegistration> m_EngineExtensions;
    CKAngelScriptResult m_LastResult = {sizeof(CKAngelScriptResult), CKAS_OK, 0, nullptr, nullptr};
    std::string m_LastErrorMessage;
    std::string m_LastStackTrace;
    bool m_CapturingScriptMessages = false;
    std::string m_CapturedScriptMessages;
};

#endif // CK_SCRIPTMANAGER_H
