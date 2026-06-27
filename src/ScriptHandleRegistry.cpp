#include "ScriptHandleRegistry.h"

#include "ScriptApiSupport.h"

void ScriptHandleRegistry::Clear() {
    while (!m_Executions.empty()) {
        ReleaseExecution(*m_Executions.begin());
    }

    while (!m_Functions.empty()) {
        ReleaseFunction(*m_Functions.begin());
    }

    while (!m_Methods.empty()) {
        ReleaseMethod(*m_Methods.begin());
    }

    while (!m_Objects.empty()) {
        ReleaseObject(*m_Objects.begin());
    }
}

bool ScriptHandleRegistry::OwnsExecution(const CKAngelScriptExecution *execution) const {
    return execution && m_Executions.find(const_cast<CKAngelScriptExecution *>(execution)) != m_Executions.end();
}

bool ScriptHandleRegistry::OwnsFunction(const CKAngelScriptFunction *function) const {
    return function && m_Functions.find(const_cast<CKAngelScriptFunction *>(function)) != m_Functions.end();
}

bool ScriptHandleRegistry::OwnsObject(const CKAngelScriptObject *object) const {
    return object && m_Objects.find(const_cast<CKAngelScriptObject *>(object)) != m_Objects.end();
}

bool ScriptHandleRegistry::OwnsMethod(const CKAngelScriptMethod *method) const {
    return method && m_Methods.find(const_cast<CKAngelScriptMethod *>(method)) != m_Methods.end();
}

void ScriptHandleRegistry::AddExecution(CKAngelScriptExecution *execution) {
    if (execution) {
        m_Executions.insert(execution);
    }
}

void ScriptHandleRegistry::AddFunction(CKAngelScriptFunction *function) {
    if (function) {
        m_Functions.insert(function);
    }
}

void ScriptHandleRegistry::AddObject(CKAngelScriptObject *object) {
    if (object) {
        m_Objects.insert(object);
    }
}

void ScriptHandleRegistry::AddMethod(CKAngelScriptMethod *method) {
    if (method) {
        m_Methods.insert(method);
    }
}

CKAS_STATUS ScriptHandleRegistry::ValidateFunction(const CKAngelScriptFunction *function,
                                                   const ScriptManager *owner,
                                                   const char **errorMessage) const {
    if (errorMessage) {
        *errorMessage = nullptr;
    }
    if (!function) {
        if (errorMessage) {
            *errorMessage = "Function handle is invalid.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    if (function->Manager && function->Manager != owner) {
        if (errorMessage) {
            *errorMessage = "Function handle belongs to another CKAngelScript manager.";
        }
        return CKAS_FOREIGNHANDLE;
    }
    if (!OwnsFunction(function)) {
        if (errorMessage) {
            *errorMessage = "Function handle is invalid.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptHandleRegistry::ValidateObject(const CKAngelScriptObject *object,
                                                 const ScriptManager *owner,
                                                 const char **errorMessage) const {
    if (errorMessage) {
        *errorMessage = nullptr;
    }
    if (!object) {
        if (errorMessage) {
            *errorMessage = "Object handle is invalid.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    if (object->Manager && object->Manager != owner) {
        if (errorMessage) {
            *errorMessage = "Object handle belongs to another CKAngelScript manager.";
        }
        return CKAS_FOREIGNHANDLE;
    }
    if (!OwnsObject(object)) {
        if (errorMessage) {
            *errorMessage = "Object handle is invalid.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptHandleRegistry::ValidateMethod(const CKAngelScriptMethod *method,
                                                 const ScriptManager *owner,
                                                 const char **errorMessage) const {
    if (errorMessage) {
        *errorMessage = nullptr;
    }
    if (!method) {
        if (errorMessage) {
            *errorMessage = "Method handle is invalid.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    if (method->Manager && method->Manager != owner) {
        if (errorMessage) {
            *errorMessage = "Method handle belongs to another CKAngelScript manager.";
        }
        return CKAS_FOREIGNHANDLE;
    }
    if (!OwnsMethod(method)) {
        if (errorMessage) {
            *errorMessage = "Method handle is invalid.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptHandleRegistry::ValidateObjectMethod(const CKAngelScriptObject *object,
                                                       const CKAngelScriptMethod *method,
                                                       const ScriptManager *owner,
                                                       const char **errorMessage) const {
    if (errorMessage) {
        *errorMessage = nullptr;
    }
    if (!object || !method) {
        if (errorMessage) {
            *errorMessage = "Object and method handles are required.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    if ((object->Manager && object->Manager != owner) || (method->Manager && method->Manager != owner)) {
        if (errorMessage) {
            *errorMessage = "Object or method handle belongs to another CKAngelScript manager.";
        }
        return CKAS_FOREIGNHANDLE;
    }
    if (!OwnsObject(object) || !OwnsMethod(method) || !object->Object) {
        if (errorMessage) {
            *errorMessage = "Object or method handle is invalid.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptHandleRegistry::ValidateExecution(const CKAngelScriptExecution *execution,
                                                    const ScriptManager *owner,
                                                    const char **errorMessage) const {
    if (errorMessage) {
        *errorMessage = nullptr;
    }
    if (!execution) {
        if (errorMessage) {
            *errorMessage = "Execution handle is invalid.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    if (execution->Manager && execution->Manager != owner) {
        if (errorMessage) {
            *errorMessage = "Execution handle belongs to another CKAngelScript manager.";
        }
        return CKAS_FOREIGNHANDLE;
    }
    if (!OwnsExecution(execution)) {
        if (errorMessage) {
            *errorMessage = "Execution handle is invalid.";
        }
        return CKAS_INVALIDARGUMENT;
    }
    return CKAS_OK;
}

void ScriptHandleRegistry::ReleaseFunction(CKAngelScriptFunction *function) {
    m_Functions.erase(function);
    delete function;
}

void ScriptHandleRegistry::ReleaseObject(CKAngelScriptObject *object) {
    m_Objects.erase(object);
    if (object && object->Object) {
        object->Object->Release();
        object->Object = nullptr;
    }
    delete object;
}

void ScriptHandleRegistry::ReleaseMethod(CKAngelScriptMethod *method) {
    m_Methods.erase(method);
    delete method;
}

void ScriptHandleRegistry::ReleaseExecution(CKAngelScriptExecution *execution) {
    m_Executions.erase(execution);
    if (execution && execution->Invoker.IsContextSuspended()) {
        execution->Invoker.AbortContext();
    }
    delete execution;
}

bool ScriptHandleRegistry::HasExecutionForModule(const char *moduleName) const {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return false;
    }
    for (const CKAngelScriptExecution *execution : m_Executions) {
        if (execution && execution->ModuleName == moduleName) {
            return true;
        }
    }
    return false;
}

bool ScriptHandleRegistry::HasRuntimeHandleForModule(const char *moduleName) const {
    if (HasExecutionForModule(moduleName)) {
        return true;
    }
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return false;
    }
    for (const CKAngelScriptObject *object : m_Objects) {
        if (object && object->ModuleName == moduleName) {
            return true;
        }
    }
    return false;
}
