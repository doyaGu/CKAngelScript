#ifndef CK_SCRIPTRUNTIME_H
#define CK_SCRIPTRUNTIME_H

#include <memory>
#include <string>
#include <vector>

#include <angelscript.h>

#include "CKTypes.h"
#include "ScriptRuntimeMetadata.h"

class CKContext;
class ScriptManager;

class ScriptRuntimeContext {
public:
    ScriptRuntimeContext();
    ScriptRuntimeContext(CKContext *context,
                         std::string scriptId,
                         std::string scriptName,
                         std::string scriptVersion,
                         std::string rootPath,
                         std::vector<ScriptRuntimeMetadataEntry> metadata,
                         float deltaTime,
                         float timeSeconds);

    CKContext *Context() const;
    float DeltaTime() const;
    float TimeSeconds() const;
    std::string ScriptId() const;
    std::string ScriptName() const;
    std::string ScriptVersion() const;
    std::string RootPath() const;
    std::string Metadata(const std::string &key, const std::string &fallback = std::string()) const;
    int MetadataCount() const;
    std::string MetadataKey(int index) const;
    std::string MetadataValue(int index) const;
    void Raise(const std::string &message) const;

    CKBehaviorContext ToBehaviorContext() const;

private:
    CKContext *m_Context = nullptr;
    std::string m_ScriptId;
    std::string m_ScriptName;
    std::string m_ScriptVersion;
    std::string m_RootPath;
    std::vector<ScriptRuntimeMetadataEntry> m_Metadata;
    float m_DeltaTime = 0.0f;
    float m_TimeSeconds = 0.0f;
};

class ScriptRuntime {
public:
    explicit ScriptRuntime(ScriptManager *manager);
    ~ScriptRuntime();

    void PreProcess();
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
    std::string Version(const std::string &id) const;
    std::string Metadata(const std::string &id, const std::string &key, const std::string &fallback = std::string()) const;
    std::vector<std::string> Dependencies(const std::string &id) const;

private:
    struct Module;
    enum class InvokeStatus {
        Finished,
        Suspended,
        Failed
    };

#if CKAS_BUILD_SELF_TESTS
    friend bool RunScriptRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
#endif

    void EnsureScanned();
    std::vector<ScriptRuntimeManifest> Discover(std::string &error) const;
    bool LoadDiscovered(const std::vector<ScriptRuntimeManifest> &scripts, bool reconcileModules);
    bool LoadModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> &module, std::string &error);
    bool ReplaceModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> module);
    bool RemoveModuleById(const std::string &id);
    void RemoveModulesNotIn(const std::vector<ScriptRuntimeManifest> &scripts);
    bool DestroyModule(Module &module, bool hard = false);
    bool DisableModule(Module &module);
    bool PauseModule(Module &module, const ScriptRuntimeContext &context);
    bool ResetModule(Module &module, const ScriptRuntimeContext &context);
    void FinalizePendingModules();
    void UpdateModule(Module &module, float deltaTime, float timeSeconds);
    InvokeStatus Invoke(Module &module, const char *name, const ScriptRuntimeContext &context, bool required = false);
    bool InvokeFinished(Module &module, const char *name, const ScriptRuntimeContext &context, bool required = false);
    void CancelActiveInvocation(Module &module);
    void SetModuleError(Module &module, const std::string &error);
    void OutputDiagnostic(const std::string &message) const;

    ScriptManager *m_Manager = nullptr;
    bool m_Scanned = false;
    bool m_Paused = false;
    int m_Generation = 0;
    std::vector<std::unique_ptr<Module>> m_Modules;
};

void RegisterScriptRuntime(asIScriptEngine *engine);
#if CKAS_BUILD_SELF_TESTS
bool RunScriptRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
#endif

#endif // CK_SCRIPTRUNTIME_H
