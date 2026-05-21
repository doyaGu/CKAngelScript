#ifndef CK_SCRIPTMANAGER_H
#define CK_SCRIPTMANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <angelscript.h>

#include "CKBaseManager.h"
#include "CKContext.h"

#include "ScriptCache.h"

#define SCRIPT_MANAGER_GUID CKGUID(0x70955bd2,0x30684456)

#define SCRIPT_MANAGER_TYPE 3000

class CKBehavior;
class ScriptBehaviorBridge;
class ScriptParameterRegistry;
class ScriptRuntime;
class ScriptRunner;

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
    ScriptRunner *Runner = nullptr;
    asIScriptObject *Object = nullptr;

    asIScriptFunction *OnLoad = nullptr;
    asIScriptFunction *Awake = nullptr;
    asIScriptFunction *OnEnable = nullptr;
    asIScriptFunction *Start = nullptr;
    asIScriptFunction *Update = nullptr;
    asIScriptFunction *OnDisable = nullptr;
    asIScriptFunction *OnDestroy = nullptr;
    asIScriptFunction *OnReset = nullptr;

    std::string ScriptName;
    std::string ClassName;
    std::string Source;
    std::string File;
    std::string Manifest;
    std::string RuntimeModuleName;
    std::vector<ScriptComponentBinding> Bindings;
    std::vector<std::string> ManagedInputParameterNames;

    bool PrivateModule = false;
    bool Loaded = false;
    bool OnLoadCalled = false;
    bool AwakeCalled = false;
    bool StartCalled = false;
    bool DesiredEnabled = false;
    bool InstanceEnabled = false;
    bool ScriptActive = false;
    bool Paused = false;
    bool Failed = false;
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
    virtual asIScriptEngine *GetScriptEngine();
    virtual const char *GetVersion();
    virtual const char *GetOptions();

    // Context
    CKContext *GetCKContext() const {
        return m_Context;
    }

    virtual asIScriptContext *GetActiveContext();

    // Thread support
    virtual int PrepareMultithread(asIThreadManager *externalMgr = nullptr);
    virtual void UnprepareMultithread();
    virtual asIThreadManager *GetThreadManager();
    virtual void AcquireExclusiveLock();
    virtual void ReleaseExclusiveLock();
    virtual void AcquireSharedLock();
    virtual void ReleaseSharedLock();
    virtual int AtomicInc(int &value);
    virtual int AtomicDec(int &value);
    virtual int ThreadCleanup();

    // Memory management
    virtual int SetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc);
    virtual int ResetGlobalMemoryFunctions();
    virtual void *AllocMem(size_t size);
    virtual void FreeMem(void *mem);

    // Auxiliary
    virtual asILockableSharedBool *CreateLockableSharedBool();

    // Script
    virtual int LoadScript(const char *scriptName, const char *filename);
    virtual int LoadScripts(const char *scriptName, const char **filenames, size_t count);
    virtual int CompileScript(const char *scriptName, const char *scriptCode);
    virtual bool UnloadScript(const char *scriptName);

    asIScriptModule *GetScript(const char *scriptName);

    ScriptCache &GetScriptCache() {
        return m_ScriptCache;
    }

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

    ScriptBehaviorBridge *GetBehaviorBridge();
    ScriptRuntime *GetRuntime() const {
        return m_Runtime.get();
    }
    ScriptParameterRegistry *GetParameterRegistry() const {
        return m_ParameterRegistry.get();
    }

    bool IsInited() const {
        return (m_Flags & AS_INITED) != 0;
    }

#if CKAS_BUILD_SELF_TESTS
    CKERROR RunStartupSelfTests();
#endif
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
#if CKAS_BUILD_SELF_TESTS
    bool m_StartupSelfTestsAttempted = false;
#endif
};

#if CKAS_BUILD_SELF_TESTS
bool RunScriptComponentMetadataSelfTest(std::string &error);
#endif

#endif // CK_SCRIPTMANAGER_H
