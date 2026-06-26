#ifndef CK_SCRIPT_ENGINE_HOST_H
#define CK_SCRIPT_ENGINE_HOST_H

#include <vector>

#include <angelscript.h>

#include "CKAngelScript.h"

class ScriptManager;

class ScriptEngineHost {
public:
    asIScriptEngine *Engine() const;
    bool HasEngine() const;
    void SetEngine(asIScriptEngine *engine);
    void ShutdownAndReleaseEngine();

    asIScriptContext *RequestContext(ScriptManager &manager);
    void ReturnContext(ScriptManager &manager, asIScriptContext *context);
    void ReleaseContextPool();

    void SetHostCallFilter(CKAngelScriptHostCallFilterCallback callback, void *userData);
    bool RejectHostCall(const char *apiName, CKDWORD flags) const;

private:
    asIScriptEngine *m_Engine = nullptr;
    std::vector<asIScriptContext *> m_ContextPool;
    CKAngelScriptHostCallFilterCallback m_HostCallFilter = nullptr;
    void *m_HostCallFilterUserData = nullptr;
};

#endif // CK_SCRIPT_ENGINE_HOST_H
