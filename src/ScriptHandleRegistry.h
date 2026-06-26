#ifndef CK_SCRIPT_HANDLE_REGISTRY_H
#define CK_SCRIPT_HANDLE_REGISTRY_H

#include <unordered_set>

#include "ScriptApiHandles.h"

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

    void RemoveExecution(CKAngelScriptExecution *execution);
    void RemoveFunction(CKAngelScriptFunction *function);
    void RemoveObject(CKAngelScriptObject *object);
    void RemoveMethod(CKAngelScriptMethod *method);

    bool HasExecutionForModule(const char *moduleName) const;
    bool HasRuntimeHandleForModule(const char *moduleName) const;

private:
    std::unordered_set<CKAngelScriptExecution *> m_Executions;
    std::unordered_set<CKAngelScriptFunction *> m_Functions;
    std::unordered_set<CKAngelScriptObject *> m_Objects;
    std::unordered_set<CKAngelScriptMethod *> m_Methods;
};

#endif // CK_SCRIPT_HANDLE_REGISTRY_H
