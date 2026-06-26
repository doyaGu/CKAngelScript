#ifndef CK_SCRIPTMANAGER_H
#define CK_SCRIPTMANAGER_H

#include <memory>
#include <string>
#include <vector>

#include <angelscript.h>

#include "CKBaseManager.h"
#include "CKContext.h"
#include "CKAngelScript.h"

#include "ScriptApiDiagnostics.h"
#include "ScriptHandleRegistry.h"
#include "ScriptCache.h"
#include "ScriptCKObjectRetainer.h"
#include "ScriptComponentStateStore.h"
#include "ScriptEngineHost.h"
#include "ScriptImportBinder.h"
#include "ScriptModuleBytecodeStore.h"
#include "ScriptModuleReplacer.h"
#include "ScriptModuleStateStore.h"
#include "ScriptPathResolver.h"

#define SCRIPT_MANAGER_GUID CKGUID(0x70955bd2,0x30684456)

#define SCRIPT_MANAGER_TYPE 3000

class CKBehavior;
class ScriptAsyncScheduler;
class ScriptBehaviorBridge;
struct ScriptComponentState;
class ScriptMessage;
class ScriptMessageBus;
class ScriptParameterRegistry;
class ScriptRuntime;
class ScriptInvoker;

class ScriptManager : public CKBaseManager {
    friend class ScriptImportBinder;
    friend class ScriptModuleBytecodeStore;
    friend class ScriptModuleReplacer;

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
    CKAS_STATUS SetActiveContextException(const char *message, CKAngelScriptResult *result = nullptr);
    CKAS_STATUS SetHostCallFilter(CKAngelScriptHostCallFilterCallback callback,
                                  void *userData,
                                  CKAngelScriptResult *result = nullptr);
    bool RejectHostCall(const char *apiName, CKDWORD flags);

    // Low-level AngelScript helpers. The public AngelScript C API
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
    CKAS_STATUS EnumerateMetadata(const char *moduleName,
                                  CKAngelScriptMetadataCallback callback,
                                  void *userData,
                                  CKAngelScriptResult *result = nullptr);
    CKAS_STATUS GetImportedFunctionCount(const char *moduleName,
                                         CKDWORD *outCount,
                                         CKAngelScriptResult *result = nullptr);
    CKAS_STATUS EnumerateImportedFunctions(const char *moduleName,
                                           CKAngelScriptImportCallback callback,
                                           void *userData,
                                           CKAngelScriptResult *result = nullptr);
    CKAS_STATUS BindImportedFunction(const CKAngelScriptImportBindOptions &options,
                                     CKAngelScriptResult *result = nullptr);
    CKAS_STATUS BindAllImportedFunctions(const char *moduleName,
                                         CKAngelScriptResult *result = nullptr);
    CKAS_STATUS UnbindImportedFunction(const char *moduleName,
                                       CKDWORD importIndex,
                                       CKAngelScriptResult *result = nullptr);
    CKAS_STATUS UnbindAllImportedFunctions(const char *moduleName,
                                           CKAngelScriptResult *result = nullptr);
    CKAS_STATUS SaveModuleBytecode(const CKAngelScriptBytecodeSaveOptions &options,
                                   CKAngelScriptResult *result = nullptr);
    CKAS_STATUS LoadModuleBytecode(const CKAngelScriptBytecodeLoadOptions &options,
                                   CKAngelScriptResult *result = nullptr);
    CKAS_STATUS EnumerateBoundImportEdges(const char *moduleName,
                                          CKAngelScriptBoundImportEdgeCallback callback,
                                          void *userData = nullptr,
                                          CKAngelScriptResult *result = nullptr);
    CKAS_STATUS EnumerateModuleIncludeEdges(const char *moduleName,
                                            CKAngelScriptIncludeEdgeCallback callback,
                                            void *userData = nullptr,
                                            CKAngelScriptResult *result = nullptr);
    CKAS_STATUS GetModuleFingerprint(const char *moduleName,
                                     CKAngelScriptModuleFingerprint *outFingerprint,
                                     CKAngelScriptResult *result = nullptr);
    CKAS_STATUS FindFunction(const CKAngelScriptFunctionOptions &options,
                             CKAngelScriptFunction **outFunction,
                             CKAngelScriptResult *result = nullptr);
    CKAS_STATUS ReleaseFunction(CKAngelScriptFunction *function, CKAngelScriptResult *result = nullptr);
    CKAS_STATUS CreateObject(const CKAngelScriptObjectOptions &options,
                             CKAngelScriptObject **outObject,
                             CKAngelScriptResult *result = nullptr);
    CKAS_STATUS ReleaseObject(CKAngelScriptObject *object, CKAngelScriptResult *result = nullptr);
    bool OwnsObjectHandle(const CKAngelScriptObject *object) const;
    CKAS_STATUS FindObjectMethod(const CKAngelScriptMethodOptions &options,
                                 CKAngelScriptMethod **outMethod,
                                 CKAngelScriptResult *result = nullptr);
    CKAS_STATUS ReleaseMethod(CKAngelScriptMethod *method, CKAngelScriptResult *result = nullptr);
    CKAS_STATUS CallObjectMethod(const CKAngelScriptObjectMethodExecuteOptions &options,
                                 CKAngelScriptResult *result = nullptr);

    CKAS_STATUS CreateFunctionExecution(const CKAngelScriptFunctionExecutionOptions &options,
                                        CKAngelScriptExecution **outExecution,
                                        CKAngelScriptResult *result = nullptr);
    CKAS_STATUS StartExecution(CKAngelScriptExecution *execution,
                               const CKAngelScriptExecutionStepOptions *options = nullptr,
                               CKAngelScriptResult *result = nullptr);
    CKAS_STATUS ResumeExecution(CKAngelScriptExecution *execution,
                                const CKAngelScriptExecutionStepOptions *options = nullptr,
                                CKAngelScriptResult *result = nullptr);
    CKAS_STATUS CancelExecution(CKAngelScriptExecution *execution,
                                CKAngelScriptResult *result = nullptr);
    CKAS_STATUS ReleaseExecution(CKAngelScriptExecution *execution,
                                 CKAngelScriptResult *result = nullptr);
    CKAS_STATUS GetExecutionState(const CKAngelScriptExecution *execution,
                                  CKAS_EXECUTIONSTATE *outState,
                                  CKAngelScriptResult *result = nullptr);
    CKAS_STATUS BorrowExecutionResult(const CKAngelScriptExecution *execution,
                                      const CKAngelScriptResult **outResult,
                                      CKAngelScriptResult *result = nullptr);
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
    bool UntrackCKObjectCallback(CK_ID id, asIScriptFunction *func);
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

    static bool RejectActiveHostCall(const char *apiName, CKDWORD flags);

protected:
    bool HasExecutionForModule(const char *moduleName) const;
    bool HasRuntimeHandleForModule(const char *moduleName) const;
    bool HasBoundImportConsumersForModule(const char *moduleName, std::string *consumerModule = nullptr) const;
    CKAS_STATUS CheckModuleRuntimeHandlesReleased(const char *moduleName, CKAngelScriptResult *result);
    CKAS_STATUS CheckModuleHasNoBoundImportConsumers(const char *moduleName, CKAngelScriptResult *result);
    CKAS_STATUS CheckModuleReplaceOrUnloadAllowed(const char *moduleName, CKAngelScriptResult *result);
    bool IsModuleMutationBlockedByCallback() const;
    CKAS_STATUS RejectModuleMutationDuringCallback(const char *apiName, CKAngelScriptResult *result);
    void BumpModuleGeneration(const char *moduleName);
    void SetModuleKind(const char *moduleName, ScriptModuleKind kind);
    void SetModuleIncludeEdges(const char *moduleName, const std::vector<ScriptIncludeEdge> &includeEdges);
    void RefreshModuleIncludeEdgesFromCache(const char *moduleName);
    void ClearModuleIncludeEdges(const char *moduleName);
    void MarkModuleStateDirty(const char *moduleName);
    unsigned long long BuildModuleSourceHash(const char *moduleName);
    unsigned long long BuildDeclaredImportHash(const char *moduleName);
    CKAS_STATUS ReplaceModuleFromSections(
        const char *moduleName,
        const std::vector<std::tuple<std::string, std::string>> &sections,
        bool sourceSnapshotSections,
        CKAngelScriptResult *result);
    // Low-level shims backing the public module API. Do not call these
    // from behavior blocks or runtime helpers; use LoadModule/CompileModule/
    // UnloadModule and the cache helper methods above.
    int LoadModuleFromDefaultOrFile(const char *moduleName, const char *filename);
    int LoadModuleFromFiles(const char *moduleName, const char **filenames, size_t count);
    int CompileModuleFromMemory(const char *moduleName, const char *scriptCode);
    bool DiscardModule(const char *moduleName);
    bool DiscardCachedModule(const char *moduleName);
    CKAngelScriptResult MakeResult(CKAS_STATUS status,
                                 int angelScriptCode = 0,
                                 const std::string &errorMessage = std::string(),
                                 const std::string &stackTrace = std::string(),
                                 const std::vector<CapturedScriptMessage> *compilerMessages = nullptr);
    CKAS_STATUS StoreResult(CKAngelScriptResult *out,
                                  CKAS_STATUS status,
                                  int angelScriptCode = 0,
                                  const std::string &errorMessage = std::string(),
                                  const std::string &stackTrace = std::string(),
                                  const std::vector<CapturedScriptMessage> *compilerMessages = nullptr);
    void BeginScriptMessageCapture();
    std::string EndScriptMessageCapture(std::vector<CapturedScriptMessage> *messages = nullptr);

    int m_Flags = 0;
    ScriptPathResolver m_PathResolver;
    ScriptEngineHost m_EngineHost;
    ScriptCache m_ScriptCache;
    ScriptCKObjectRetainer m_CKObjectRetainer;
    ScriptComponentStateStore m_ComponentStates;
    std::unique_ptr<ScriptBehaviorBridge> m_BehaviorBridge;
    std::unique_ptr<ScriptParameterRegistry> m_ParameterRegistry;
    std::unique_ptr<ScriptRuntime> m_Runtime;
    std::unique_ptr<ScriptAsyncScheduler> m_AsyncScheduler;
    std::unique_ptr<ScriptMessageBus> m_MessageBus;
    ScriptHandleRegistry m_HandleRegistry;
    ScriptModuleStateStore m_ModuleStateStore;
    ScriptImportBinder m_ImportBinder;
    ScriptModuleBytecodeStore m_ModuleBytecodeStore;
    ScriptModuleReplacer m_ModuleReplacer;
    ScriptApiDiagnostics m_Diagnostics;
    int m_PublicCallbackDepth = 0;
    int m_BytecodeCallbackDepth = 0;
};

#endif // CK_SCRIPTMANAGER_H
