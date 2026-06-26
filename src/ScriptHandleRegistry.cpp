#include "ScriptManager.h"

#include <cstring>
#include <string>

#include "ScriptApiSupport.h"
#include "ScriptHandleRegistry.h"
#include "ScriptPublicOptions.h"

void ScriptHandleRegistry::Clear() {
    for (auto *execution : m_Executions) {
        delete execution;
    }
    m_Executions.clear();

    for (auto *function : m_Functions) {
        delete function;
    }
    m_Functions.clear();

    for (auto *method : m_Methods) {
        delete method;
    }
    m_Methods.clear();

    for (auto *object : m_Objects) {
        if (object && object->Object) {
            object->Object->Release();
            object->Object = nullptr;
        }
        delete object;
    }
    m_Objects.clear();
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

bool ScriptManager::OwnsObjectHandle(const CKAngelScriptObject *object) const {
    return m_HandleRegistry.OwnsObject(object);
}

bool ScriptManager::HasExecutionForModule(const char *moduleName) const {
    return m_HandleRegistry.HasExecutionForModule(moduleName);
}

bool ScriptManager::HasRuntimeHandleForModule(const char *moduleName) const {
    return m_HandleRegistry.HasRuntimeHandleForModule(moduleName);
}

CKAS_STATUS ScriptManager::FindFunction(const CKAngelScriptFunctionOptions &options,
                                        CKAngelScriptFunction **outFunction,
                                        CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function handle out pointer is required.");
    }
    ScriptPublicOptions::FunctionRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeFunctionOptions(options, request, errorMessage);
    if (optionStatus != CKAS_OK) {
        return StoreResult(result, optionStatus, 0, errorMessage);
    }
    asIScriptFunction *scriptFunction = nullptr;
    const CKAS_STATUS status = request.HasFunctionDecl
                                   ? BorrowFunctionByDecl(request.ModuleName, request.FunctionDecl, &scriptFunction, result)
                                   : BorrowFunctionByName(request.ModuleName, request.FunctionName, &scriptFunction, result);
    if (status != CKAS_OK) {
        return status;
    }

    auto *function = new CKAngelScriptFunction();
    function->Manager = this;
    function->ModuleName = request.ModuleName ? request.ModuleName : "";
    function->FunctionName = scriptFunction->GetName() ? scriptFunction->GetName() : "";
    function->FunctionDecl = scriptFunction->GetDeclaration() ? scriptFunction->GetDeclaration() : "";
    function->ModuleGeneration = GetModuleGeneration(request.ModuleName);
    m_HandleRegistry.AddFunction(function);
    *outFunction = function;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseFunction(CKAngelScriptFunction *function, CKAngelScriptResult *result) {
    if (!function) {
        return StoreResult(result, CKAS_OK);
    }
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus = m_HandleRegistry.ValidateFunction(function, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    m_HandleRegistry.ReleaseFunction(function);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::CreateObject(const CKAngelScriptObjectOptions &options,
                                        CKAngelScriptObject **outObject,
                                        CKAngelScriptResult *result) {
    if (outObject) {
        *outObject = nullptr;
    }
    if (!outObject) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object out pointer is required.");
    }
    ScriptPublicOptions::ObjectRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeObjectOptions(options, request, errorMessage);
    if (optionStatus != CKAS_OK) {
        return StoreResult(result, optionStatus, 0, errorMessage);
    }
    if (!GetScriptEngine()) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptModule *module = GetModule(request.ModuleName);
    if (!module) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
    }
    asITypeInfo *type = request.HasTypeDecl
                            ? module->GetTypeInfoByDecl(request.TypeDecl)
                            : ScriptApiSupport::FindTypeByNameAndNamespace(module,
                                                                           request.ClassName,
                                                                           request.ClassNamespace);
    if (!type) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Script class was not found.");
    }
    auto *scriptObject = static_cast<asIScriptObject *>(GetScriptEngine()->CreateScriptObject(type));
    if (!scriptObject) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Failed to create script object.");
    }

    auto *object = new CKAngelScriptObject();
    object->Manager = this;
    object->Object = scriptObject;
    object->ModuleName = request.ModuleName;
    object->ClassName = type->GetName() ? type->GetName() : "";
    object->ClassNamespace = type->GetNamespace() ? type->GetNamespace() : "";
    object->ModuleGeneration = GetModuleGeneration(request.ModuleName);
    m_HandleRegistry.AddObject(object);
    *outObject = object;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseObject(CKAngelScriptObject *object, CKAngelScriptResult *result) {
    if (!object) {
        return StoreResult(result, CKAS_OK);
    }
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus = m_HandleRegistry.ValidateObject(object, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    m_HandleRegistry.ReleaseObject(object);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::FindObjectMethod(const CKAngelScriptMethodOptions &options,
                                            CKAngelScriptMethod **outMethod,
                                            CKAngelScriptResult *result) {
    if (outMethod) {
        *outMethod = nullptr;
    }
    if (!outMethod) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Method handle out pointer is required.");
    }
    ScriptPublicOptions::MethodRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeMethodOptions(options, request, errorMessage);
    if (optionStatus != CKAS_OK) {
        return StoreResult(result, optionStatus, 0, errorMessage);
    }
    const char *objectError = nullptr;
    const CKAS_STATUS objectStatus = m_HandleRegistry.ValidateObject(request.Object, this, &objectError);
    if (objectStatus != CKAS_OK) {
        return StoreResult(result, objectStatus, 0, objectError ? objectError : "");
    }
    if (!request.Object->Object) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object handle is invalid.");
    }
    if (!HasModule(request.Object->ModuleName.c_str()) ||
        GetModuleGeneration(request.Object->ModuleName.c_str()) != request.Object->ModuleGeneration) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Object handle is stale.");
    }
    asITypeInfo *type = request.Object->Object->GetObjectType();
    if (!type) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Script object has no type information.");
    }

    asIScriptFunction *function = nullptr;
    if (request.HasMethodDecl) {
        function = type->GetMethodByDecl(request.MethodDecl);
    } else {
        asUINT matchCount = 0;
        const asUINT count = type->GetMethodCount();
        for (asUINT i = 0; i < count; ++i) {
            asIScriptFunction *candidate = type->GetMethodByIndex(i);
            const char *candidateName = candidate ? candidate->GetName() : nullptr;
            if (candidateName && std::strcmp(candidateName, request.MethodName) == 0) {
                function = candidate;
                ++matchCount;
            }
        }
        if (matchCount > 1) {
            return StoreResult(result, CKAS_AMBIGUOUS, 0, "Method name matched multiple overloads.");
        }
    }
    if (!function) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Script object method was not found.");
    }

    auto *method = new CKAngelScriptMethod();
    method->Manager = this;
    method->ModuleName = request.Object->ModuleName;
    method->ClassName = request.Object->ClassName;
    method->ClassNamespace = request.Object->ClassNamespace;
    method->MethodName = function->GetName() ? function->GetName() : "";
    method->MethodDecl = function->GetDeclaration(false) ? function->GetDeclaration(false) : "";
    method->ModuleGeneration = request.Object->ModuleGeneration;
    const asUINT paramCount = function->GetParamCount();
    method->ParamTypes.resize(paramCount);
    method->ParamFlags.resize(paramCount);
    for (asUINT i = 0; i < paramCount; ++i) {
        int typeId = 0;
        asDWORD flags = 0;
        function->GetParam(i, &typeId, &flags);
        method->ParamTypes[i] = typeId;
        method->ParamFlags[i] = flags;
    }
    method->ReturnType = function->GetReturnTypeId(&method->ReturnFlags);
    m_HandleRegistry.AddMethod(method);
    *outMethod = method;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseMethod(CKAngelScriptMethod *method, CKAngelScriptResult *result) {
    if (!method) {
        return StoreResult(result, CKAS_OK);
    }
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus = m_HandleRegistry.ValidateMethod(method, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    m_HandleRegistry.ReleaseMethod(method);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::CallObjectMethod(const CKAngelScriptObjectMethodExecuteOptions &options,
                                            CKAngelScriptResult *result) {
    ScriptPublicOptions::ObjectMethodExecuteRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus =
        ScriptPublicOptions::DecodeObjectMethodExecuteOptions(options, request, errorMessage);
    if (optionStatus != CKAS_OK) {
        return StoreResult(result, optionStatus, 0, errorMessage);
    }
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus =
        m_HandleRegistry.ValidateObjectMethod(request.Object, request.Method, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    if (request.Object->ModuleName != request.Method->ModuleName ||
        request.Object->ClassName != request.Method->ClassName ||
        request.Object->ClassNamespace != request.Method->ClassNamespace) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Method handle does not belong to the object type.");
    }
    if (!HasModule(request.Object->ModuleName.c_str()) ||
        GetModuleGeneration(request.Object->ModuleName.c_str()) != request.Object->ModuleGeneration ||
        request.Method->ModuleGeneration != request.Object->ModuleGeneration) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Object or method handle is stale.");
    }
    asITypeInfo *type = request.Object->Object->GetObjectType();
    if (!type) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Script object has no type information.");
    }
    asIScriptFunction *function = type->GetMethodByDecl(request.Method->MethodDecl.c_str());
    if (!function) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Object method handle is stale.");
    }

    const ScriptApiSupport::ObjectCallOutcome outcome = ScriptApiSupport::ExecutePreparedObjectMethod(this,
                                                                  m_PublicCallbackDepth,
                                                                  request.Object->Object,
                                                                  function,
                                                                  request.Method,
                                                                  request.WriteArgs,
                                                                  request.ReadResult,
                                                                  request.ConfigureContext,
                                                                  request.ReadContextResult,
                                                                  request.UserData,
                                                                  request.Flags);
    return StoreResult(result, outcome.Status, outcome.AngelScriptCode, outcome.ErrorMessage, outcome.StackTrace);
}

CKAS_STATUS ScriptManager::CreateFunctionExecution(const CKAngelScriptFunctionExecutionOptions &options,
                                                   CKAngelScriptExecution **outExecution,
                                                   CKAngelScriptResult *result) {
    if (outExecution) {
        *outExecution = nullptr;
    }
    if (!outExecution) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Execution out pointer is required.");
    }
    ScriptPublicOptions::FunctionExecutionRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeFunctionExecutionOptions(options,
                                                                                   request,
                                                                                   errorMessage);
    if (optionStatus != CKAS_OK) {
        return StoreResult(result, optionStatus, 0, errorMessage);
    }
    const char *functionError = nullptr;
    const CKAS_STATUS functionStatus = m_HandleRegistry.ValidateFunction(request.Function, this, &functionError);
    if (functionStatus != CKAS_OK) {
        return StoreResult(result, functionStatus, 0, functionError ? functionError : "");
    }
    if (!GetScriptEngine()) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    if (!HasModule(request.Function->ModuleName.c_str()) ||
        GetModuleGeneration(request.Function->ModuleName.c_str()) != request.Function->ModuleGeneration) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Function handle is stale.");
    }
    asIScriptModule *module = GetModule(request.Function->ModuleName.c_str());
    if (!module) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Function module is no longer loaded.");
    }

    asIScriptFunction *function = request.Function->FunctionDecl.empty()
                                      ? nullptr
                                      : module->GetFunctionByDecl(request.Function->FunctionDecl.c_str());
    if (!function) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Function handle is stale.");
    }

    auto *execution = new CKAngelScriptExecution(this);
    execution->ModuleName = request.Function->ModuleName;
    execution->FunctionName = request.Function->FunctionName;
    execution->FunctionDecl = request.Function->FunctionDecl;
    execution->ModuleGeneration = request.Function->ModuleGeneration;
    execution->Flags = request.Flags;
    if (request.BehaviorContext) {
        execution->BehaviorContextStorage = *request.BehaviorContext;
        execution->HasBehaviorContext = true;
    }
    function->AddRef();
    execution->Function = function;
    if (!execution->Invoker.SetScript(request.Function->ModuleName.c_str())) {
        const std::string error = execution->Invoker.GetErrorMessage();
        delete execution;
        return StoreResult(result, CKAS_NOTFOUND, 0, error.empty() ? "Module cache was not found." : error);
    }

    ScriptApiSupport::MakeExecutionResult(execution, CKAS_OK);
    m_HandleRegistry.AddExecution(execution);
    *outExecution = execution;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::StartExecution(CKAngelScriptExecution *execution,
                                          const CKAngelScriptExecutionStepOptions *options,
                                          CKAngelScriptResult *result) {
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::ValidateExecutionStepOptions(options,
                                                                                 "StartExecution",
                                                                                 errorMessage);
    if (optionStatus != CKAS_OK) {
        return StoreResult(result, optionStatus, 0, errorMessage);
    }
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus = m_HandleRegistry.ValidateExecution(execution, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    if (execution->State != CKAS_EXECUTION_READY) {
        ScriptApiSupport::MakeExecutionResult(execution, CKAS_INVALIDSTATE, 0, "Execution is not ready to start.");
        return StoreResult(result, CKAS_INVALIDSTATE, 0, "Execution is not ready to start.");
    }
    const CKAS_STATUS status = ScriptApiSupport::RunExecution(execution, options, m_PublicCallbackDepth);
    StoreResult(result,
                status,
                execution->Result.AngelScriptCode,
                execution->ErrorMessage,
                execution->StackTrace);
    return status;
}

CKAS_STATUS ScriptManager::ResumeExecution(CKAngelScriptExecution *execution,
                                           const CKAngelScriptExecutionStepOptions *options,
                                           CKAngelScriptResult *result) {
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::ValidateExecutionStepOptions(options,
                                                                                 "ResumeExecution",
                                                                                 errorMessage);
    if (optionStatus != CKAS_OK) {
        return StoreResult(result, optionStatus, 0, errorMessage);
    }
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus = m_HandleRegistry.ValidateExecution(execution, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    if (execution->State != CKAS_EXECUTION_SUSPENDED) {
        ScriptApiSupport::MakeExecutionResult(execution, CKAS_INVALIDSTATE, 0, "Execution is not suspended.");
        return StoreResult(result, CKAS_INVALIDSTATE, 0, "Execution is not suspended.");
    }
    const CKAS_STATUS status = ScriptApiSupport::RunExecution(execution, options, m_PublicCallbackDepth);
    StoreResult(result,
                status,
                execution->Result.AngelScriptCode,
                execution->ErrorMessage,
                execution->StackTrace);
    return status;
}

CKAS_STATUS ScriptManager::CancelExecution(CKAngelScriptExecution *execution, CKAngelScriptResult *result) {
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus = m_HandleRegistry.ValidateExecution(execution, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    execution->Invoker.AbortContext();
    execution->State = CKAS_EXECUTION_CANCELLED;
    ScriptApiSupport::MakeExecutionResult(execution, CKAS_CANCELLED);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseExecution(CKAngelScriptExecution *execution, CKAngelScriptResult *result) {
    if (!execution) {
        return StoreResult(result, CKAS_OK);
    }
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus = m_HandleRegistry.ValidateExecution(execution, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    m_HandleRegistry.ReleaseExecution(execution);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::GetExecutionState(const CKAngelScriptExecution *execution,
                                             CKAS_EXECUTIONSTATE *outState,
                                             CKAngelScriptResult *result) {
    if (outState) {
        *outState = CKAS_EXECUTION_FAILED;
    }
    if (!outState) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Execution state out pointer is required.");
    }
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus = m_HandleRegistry.ValidateExecution(execution, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    *outState = execution->State;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::BorrowExecutionResult(const CKAngelScriptExecution *execution,
                                                 const CKAngelScriptResult **outResult,
                                                 CKAngelScriptResult *result) {
    if (outResult) {
        *outResult = nullptr;
    }
    if (!outResult) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Execution result out pointer is required.");
    }
    const char *handleError = nullptr;
    const CKAS_STATUS handleStatus = m_HandleRegistry.ValidateExecution(execution, this, &handleError);
    if (handleStatus != CKAS_OK) {
        return StoreResult(result, handleStatus, 0, handleError ? handleError : "");
    }
    *outResult = &execution->Result;
    return StoreResult(result, CKAS_OK);
}

