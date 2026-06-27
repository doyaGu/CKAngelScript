#ifndef CK_SCRIPTRUNTIME_H
#define CK_SCRIPTRUNTIME_H

#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <angelscript.h>

#include "CKAngelScript.h"
#include "ScriptRuntimeDependency.h"
#include "ScriptRuntimeMetadata.h"

#ifndef CKAS_BUILD_SELF_TESTS
#define CKAS_BUILD_SELF_TESTS 0
#endif

class CKContext;
class ScriptMessage;
class ScriptManager;

class RuntimeDependencyInfo {
public:
    RuntimeDependencyInfo();
    RuntimeDependencyInfo(ScriptRuntimeDependency dependency,
                          bool optional,
                          bool present,
                          bool satisfied,
                          std::string actualVersion);

    std::string Raw() const;
    std::string Id() const;
    std::string Operator() const;
    std::string Version() const;
    std::string ActualVersion() const;
    bool Optional() const;
    bool Present() const;
    bool Satisfied() const;

private:
    ScriptRuntimeDependency m_Dependency;
    bool m_Optional = false;
    bool m_Present = false;
    bool m_Satisfied = false;
    std::string m_ActualVersion;
};

class RuntimeScriptInfo {
public:
    RuntimeScriptInfo();

    bool Exists() const;
    std::string Id() const;
    std::string Name() const;
    std::string Version() const;
    std::string Description() const;
    std::string Author() const;
    std::string Category() const;
    int TagCount() const;
    std::string Tag(int index) const;
    bool Enabled() const;
    bool Loaded() const;
    bool Failed() const;
    std::string State() const;
    std::string Phase() const;
    std::string Error() const;
    std::string Root() const;
    std::string Manifest() const;
    std::string Entry() const;
    int Generation() const;

private:
    friend class ScriptRuntime;

    bool m_Exists = false;
    std::string m_Id;
    std::string m_Name;
    std::string m_Version;
    std::string m_Description;
    std::string m_Author;
    std::string m_Category;
    std::vector<std::string> m_Tags;
    bool m_Enabled = false;
    bool m_Loaded = false;
    bool m_Failed = false;
    std::string m_State;
    std::string m_Phase;
    std::string m_Error;
    std::string m_RootPath;
    std::string m_ManifestPath;
    std::string m_EntryPath;
    int m_Generation = 0;
};

class ScriptContext {
public:
    ScriptContext();
    ScriptContext(CKContext *context,
                  const ScriptRuntimeManifest *metadata,
                  const char *phase,
                  const char *state,
                  int generation,
                  std::uint64_t frameIndex,
                  float deltaTime,
                  float timeSeconds);

    CKContext *Context() const;
    float DeltaTime() const;
    float TimeSeconds() const;
    std::string Id() const;
    std::string Name() const;
    std::string Version() const;
    std::string Root() const;
    std::string Manifest() const;
    std::string Entry() const;
    std::string Target() const;
    std::string Phase() const;
    std::string State() const;
    int Generation() const;
    std::uint64_t FrameIndex() const;
    std::string Metadata(const std::string &key, const std::string &fallback = std::string()) const;
    int MetadataCount() const;
    std::string MetadataKey(int index) const;
    std::string MetadataValue(int index) const;
    void Raise(const std::string &message) const;

    CKBehaviorContext ToBehaviorContext() const;

private:
    CKContext *m_Context = nullptr;
    const ScriptRuntimeManifest *m_Metadata = nullptr;
    const char *m_Phase = "";
    const char *m_State = "";
    int m_Generation = 0;
    std::uint64_t m_FrameIndex = 0;
    float m_DeltaTime = 0.0f;
    float m_TimeSeconds = 0.0f;
};

class ScriptRuntime {
public:
    explicit ScriptRuntime(ScriptManager *manager);
    ~ScriptRuntime();

    void PreProcess();
    void PostProcess();
    void PostLoad();
    void OnReset();
    void OnPause();
    void OnResume();
    void OnEnd();
    void Clear();

    bool ReloadAll(std::string *error = nullptr);
    bool Reload(const std::string &id, std::string *error = nullptr);
    bool Enable(const std::string &id, bool enabled, std::string *error = nullptr);
    std::vector<std::string> List() const;
    std::vector<RuntimeScriptInfo> ListInfo() const;
    RuntimeScriptInfo Info(const std::string &id) const;
    std::string Version(const std::string &id) const;
    std::string Metadata(const std::string &id, const std::string &key, const std::string &fallback = std::string()) const;
    std::vector<std::string> Dependencies(const std::string &id) const;
    std::vector<RuntimeDependencyInfo> RequiredDependencies(const std::string &id) const;
    std::vector<RuntimeDependencyInfo> OptionalDependencies(const std::string &id) const;
    bool DeliverMessage(const std::string &id, const ScriptMessage &message, bool immediate, std::string &error);

#if CKAS_BUILD_SELF_TESTS
    bool RunPauseLifecycleSelfTest(std::string &error);
    bool RunPendingReplacementSelfTest(std::string &error);
#endif

private:
    struct Module;
    enum class InvokeStatus {
        Finished,
        Suspended,
        Failed
    };

    void EnsureScanned();
    ScriptRuntimeLoadPlan Discover(std::string &error) const;
    bool LoadDiscovered(const ScriptRuntimeLoadPlan &plan, bool reconcileModules);
    bool ReplaceWithFailedModule(const ScriptRuntimeManifest &metadata, const std::string &error);
    bool LoadModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> &module, std::string &error);
    bool CacheLifecycleFunctions(Module &module, std::string &error) const;
    void ReleaseCachedFunctions(Module &module) const;
    void SubscribeStaticTopics(Module &module) const;
    void DiscardPendingReplacement(Module &module);
    bool ReplaceModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> module);
    bool RemoveModuleById(const std::string &id);
    void RemoveModulesNotIn(const std::vector<ScriptRuntimeManifest> &scripts);
    bool DestroyModule(Module &module, bool hard = false);
    bool DisableModule(Module &module);
    bool PauseModule(Module &module, const ScriptContext &context);
    bool ResetModule(Module &module, const ScriptContext &context);
    const char *ModuleState(const Module &module) const;
    Module *FindModule(const std::string &id) const;
    RuntimeScriptInfo BuildInfo(const Module &module) const;
    std::vector<RuntimeDependencyInfo> DependencyInfo(const Module &module, bool optional) const;
    void RebuildModuleIndex();
    void FinalizePendingModules();
    void UpdateModule(Module &module, float deltaTime, float timeSeconds);
    InvokeStatus InvokeMessage(Module &module, const ScriptMessage &message, const ScriptContext &context);
    InvokeStatus Invoke(Module &module, const char *name, const ScriptContext &context, bool required = false);
    bool InvokeFinished(Module &module, const char *name, const ScriptContext &context, bool required = false);
    void CancelActiveInvocation(Module &module);
    void SetModuleError(Module &module, const std::string &error);
    void OutputDiagnostic(const std::string &message) const;

    ScriptManager *m_Manager = nullptr;
    bool m_Scanned = false;
    bool m_Paused = false;
    int m_Generation = 0;
    std::uint64_t m_FrameIndex = 0;
    std::vector<std::unique_ptr<Module>> m_Modules;
    std::unordered_map<std::string, Module *> m_ModulesById;
};

void RegisterScriptRuntime(asIScriptEngine *engine);

#endif // CK_SCRIPTRUNTIME_H
