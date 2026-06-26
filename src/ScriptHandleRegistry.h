#ifndef CK_SCRIPT_HANDLE_REGISTRY_H
#define CK_SCRIPT_HANDLE_REGISTRY_H

#include <unordered_set>

#include "ScriptApiHandles.h"

class ScriptManager;

class ScriptHandleRegistry {
public:
    void Clear();

    bool OwnsExecution(const CKAngelScriptExecution *execution) const;
    bool OwnsFunction(const CKAngelScriptFunction *function) const;
    bool OwnsObject(const CKAngelScriptObject *object) const;
    bool OwnsMethod(const CKAngelScriptMethod *method) const;

    void AddExecution(CKAngelScriptExecution *execution);
    void AddFunction(CKAngelScriptFunction *function);
    void AddObject(CKAngelScriptObject *object);
    void AddMethod(CKAngelScriptMethod *method);

    CKAS_STATUS ValidateExecution(const CKAngelScriptExecution *execution,
                                  const ScriptManager *owner,
                                  const char **errorMessage) const;
    CKAS_STATUS ValidateFunction(const CKAngelScriptFunction *function,
                                 const ScriptManager *owner,
                                 const char **errorMessage) const;
    CKAS_STATUS ValidateObject(const CKAngelScriptObject *object,
                               const ScriptManager *owner,
                               const char **errorMessage) const;
    CKAS_STATUS ValidateMethod(const CKAngelScriptMethod *method,
                               const ScriptManager *owner,
                               const char **errorMessage) const;
    CKAS_STATUS ValidateObjectMethod(const CKAngelScriptObject *object,
                                     const CKAngelScriptMethod *method,
                                     const ScriptManager *owner,
                                     const char **errorMessage) const;

    void ReleaseExecution(CKAngelScriptExecution *execution);
    void ReleaseFunction(CKAngelScriptFunction *function);
    void ReleaseObject(CKAngelScriptObject *object);
    void ReleaseMethod(CKAngelScriptMethod *method);

    bool HasExecutionForModule(const char *moduleName) const;
    bool HasRuntimeHandleForModule(const char *moduleName) const;

private:
    std::unordered_set<CKAngelScriptExecution *> m_Executions;
    std::unordered_set<CKAngelScriptFunction *> m_Functions;
    std::unordered_set<CKAngelScriptObject *> m_Objects;
    std::unordered_set<CKAngelScriptMethod *> m_Methods;
};

#endif // CK_SCRIPT_HANDLE_REGISTRY_H
