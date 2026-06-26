#ifndef CK_SCRIPT_ENGINE_HOST_H
#define CK_SCRIPT_ENGINE_HOST_H

#include <string>
#include <vector>

#include <angelscript.h>

#include "CKAngelScript.h"

class CKContext;
class ScriptManager;

class ScriptEngineHost {
public:
    asIScriptEngine *Engine() const;
    bool HasEngine() const;
    int Setup(ScriptManager &manager, CKContext *context);
    void ShutdownAndReleaseEngine();

    asIScriptContext *RequestContext(ScriptManager &manager);
    void ReturnContext(ScriptManager &manager, asIScriptContext *context);
    void ReleaseContextPool();

    void SetHostCallFilter(CKAngelScriptHostCallFilterCallback callback, void *userData);
    bool RejectHostCall(const char *apiName, CKDWORD flags) const;

    CKAS_STATUS RegisterExtension(ScriptManager &manager,
                                  const CKAngelScriptEngineExtension &extension,
                                  CKAngelScriptResult *result);
    CKAS_STATUS UnregisterExtension(ScriptManager &manager,
                                    const char *name,
                                    CKAngelScriptResult *result);
    void MarkExtensionsInactive();

private:
    struct EngineExtensionRegistration {
        std::string Name;
        std::string ConfigGroupName;
        CKAngelScriptEngineExtensionCallback Register = nullptr;
        void *UserData = nullptr;
        CKDWORD Flags = CKAS_ENGINEEXTENSION_DEFAULT;
        bool ActiveInCurrentEngine = false;
    };

    int RegisterExtensionGroup(ScriptManager &manager,
                               asIScriptEngine *engine,
                               EngineExtensionRegistration &extension,
                               std::string &message);
    int RemoveExtensionGroup(asIScriptEngine *engine,
                             EngineExtensionRegistration &extension,
                             std::string &message);
    int RegisterExtensions(ScriptManager &manager, asIScriptEngine *engine);

    void RegisterStdTypes(asIScriptEngine *engine);
    void RegisterStdAddons(asIScriptEngine *engine);
    void RegisterVirtools(asIScriptEngine *engine);

    asIScriptEngine *m_Engine = nullptr;
    std::vector<asIScriptContext *> m_ContextPool;
    std::vector<EngineExtensionRegistration> m_EngineExtensions;
    CKAngelScriptHostCallFilterCallback m_HostCallFilter = nullptr;
    void *m_HostCallFilterUserData = nullptr;
};

#endif // CK_SCRIPT_ENGINE_HOST_H
