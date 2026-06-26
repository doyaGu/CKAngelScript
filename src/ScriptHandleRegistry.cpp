#include "ScriptManager.h"

#include <cstring>
#include <string>

#include "ScriptApiSupport.h"

bool ScriptManager::OwnsExecution(const CKAngelScriptExecution *execution) const {
    return execution && m_Executions.find(const_cast<CKAngelScriptExecution *>(execution)) != m_Executions.end();
}

bool ScriptManager::OwnsFunction(const CKAngelScriptFunction *function) const {
    return function && m_Functions.find(const_cast<CKAngelScriptFunction *>(function)) != m_Functions.end();
}

bool ScriptManager::OwnsObject(const CKAngelScriptObject *object) const {
    return object && m_Objects.find(const_cast<CKAngelScriptObject *>(object)) != m_Objects.end();
}

bool ScriptManager::OwnsObjectHandle(const CKAngelScriptObject *object) const {
    return OwnsObject(object);
}

bool ScriptManager::OwnsMethod(const CKAngelScriptMethod *method) const {
    return method && m_Methods.find(const_cast<CKAngelScriptMethod *>(method)) != m_Methods.end();
}

CKAS_STATUS ScriptManager::ValidateFunctionHandle(const CKAngelScriptFunction *function, CKAngelScriptResult *result) {
    if (!function) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function handle is invalid.");
    }
    if (function->Manager && function->Manager != this) {
        return StoreResult(result, CKAS_FOREIGNHANDLE, 0, "Function handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsFunction(function)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function handle is invalid.");
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptManager::ValidateObjectHandle(const CKAngelScriptObject *object, CKAngelScriptResult *result) {
    if (!object) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object handle is invalid.");
    }
    if (object->Manager && object->Manager != this) {
        return StoreResult(result, CKAS_FOREIGNHANDLE, 0, "Object handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsObject(object)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object handle is invalid.");
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptManager::ValidateMethodHandle(const CKAngelScriptMethod *method, CKAngelScriptResult *result) {
    if (!method) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Method handle is invalid.");
    }
    if (method->Manager && method->Manager != this) {
        return StoreResult(result, CKAS_FOREIGNHANDLE, 0, "Method handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsMethod(method)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Method handle is invalid.");
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptManager::ValidateObjectMethodHandles(const CKAngelScriptObject *object,
                                                       const CKAngelScriptMethod *method,
                                                       CKAngelScriptResult *result) {
    if (!object || !method) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object and method handles are required.");
    }
    if ((object->Manager && object->Manager != this) || (method->Manager && method->Manager != this)) {
        return StoreResult(result, CKAS_FOREIGNHANDLE, 0, "Object or method handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsObject(object) || !OwnsMethod(method) || !object->Object) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object or method handle is invalid.");
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptManager::ValidateExecutionHandle(const CKAngelScriptExecution *execution, CKAngelScriptResult *result) {
    if (!execution) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->Manager && execution->Manager != this) {
        return StoreResult(result, CKAS_FOREIGNHANDLE, 0, "Execution handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsExecution(execution)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    return CKAS_OK;
}

bool ScriptManager::HasExecutionForModule(const char *moduleName) const {
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

bool ScriptManager::HasRuntimeHandleForModule(const char *moduleName) const {
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

CKAS_STATUS ScriptManager::FindFunction(const CKAngelScriptFunctionOptions &options,
                                        CKAngelScriptFunction **outFunction,
                                        CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function handle out pointer is required.");
    }
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "FindFunction options size is invalid.");
    }
    const char *moduleName = ScriptApiSupport::PublicField(options, &CKAngelScriptFunctionOptions::ModuleName, static_cast<const char *>(nullptr));
    const char *functionName = ScriptApiSupport::PublicField(options, &CKAngelScriptFunctionOptions::FunctionName, static_cast<const char *>(nullptr));
    const char *functionDecl = ScriptApiSupport::PublicField(options, &CKAngelScriptFunctionOptions::FunctionDecl, static_cast<const char *>(nullptr));
    const CKDWORD flags = ScriptApiSupport::PublicField(options, &CKAngelScriptFunctionOptions::Flags, static_cast<CKDWORD>(0));
    const bool hasFunctionName = ScriptApiSupport::IsNonEmpty(functionName);
    const bool hasFunctionDecl = ScriptApiSupport::IsNonEmpty(functionDecl);
    if (ScriptApiSupport::HasUnknownPublicFlags(flags, 0)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown FindFunction flags.");
    }
    if (hasFunctionName == hasFunctionDecl) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Exactly one of FunctionName or FunctionDecl is required.");
    }
    asIScriptFunction *scriptFunction = nullptr;
    const CKAS_STATUS status = hasFunctionDecl
                                   ? BorrowFunctionByDecl(moduleName, functionDecl, &scriptFunction, result)
                                   : BorrowFunctionByName(moduleName, functionName, &scriptFunction, result);
    if (status != CKAS_OK) {
        return status;
    }

    auto *function = new CKAngelScriptFunction();
    function->Manager = this;
    function->ModuleName = moduleName ? moduleName : "";
    function->FunctionName = scriptFunction->GetName() ? scriptFunction->GetName() : "";
    function->FunctionDecl = scriptFunction->GetDeclaration() ? scriptFunction->GetDeclaration() : "";
    function->ModuleGeneration = GetModuleGeneration(moduleName);
    m_Functions.insert(function);
    *outFunction = function;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseFunction(CKAngelScriptFunction *function, CKAngelScriptResult *result) {
    if (!function) {
        return StoreResult(result, CKAS_OK);
    }
    const CKAS_STATUS handleStatus = ValidateFunctionHandle(function, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
    }
    m_Functions.erase(function);
    delete function;
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
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "CreateObject options size is invalid.");
    }
    const char *moduleName = ScriptApiSupport::PublicField(options, &CKAngelScriptObjectOptions::ModuleName, static_cast<const char *>(nullptr));
    const char *className = ScriptApiSupport::PublicField(options, &CKAngelScriptObjectOptions::ClassName, static_cast<const char *>(nullptr));
    const char *classNamespace = ScriptApiSupport::PublicField(options, &CKAngelScriptObjectOptions::ClassNamespace, static_cast<const char *>(nullptr));
    const char *typeDecl = ScriptApiSupport::PublicField(options, &CKAngelScriptObjectOptions::TypeDecl, static_cast<const char *>(nullptr));
    const bool hasClassName = ScriptApiSupport::IsNonEmpty(className);
    const bool hasTypeDecl = ScriptApiSupport::IsNonEmpty(typeDecl);
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (hasClassName == hasTypeDecl) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Exactly one of ClassName or TypeDecl is required.");
    }
    if (hasTypeDecl && ScriptApiSupport::IsNonEmpty(classNamespace)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "ClassNamespace cannot be used with TypeDecl.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
    }
    asITypeInfo *type = hasTypeDecl
                            ? module->GetTypeInfoByDecl(typeDecl)
                            : ScriptApiSupport::FindTypeByNameAndNamespace(module, className, classNamespace);
    if (!type) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Script class was not found.");
    }
    auto *scriptObject = static_cast<asIScriptObject *>(m_ScriptEngine->CreateScriptObject(type));
    if (!scriptObject) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Failed to create script object.");
    }

    auto *object = new CKAngelScriptObject();
    object->Manager = this;
    object->Object = scriptObject;
    object->ModuleName = moduleName;
    object->ClassName = type->GetName() ? type->GetName() : "";
    object->ClassNamespace = type->GetNamespace() ? type->GetNamespace() : "";
    object->ModuleGeneration = GetModuleGeneration(moduleName);
    m_Objects.insert(object);
    *outObject = object;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseObject(CKAngelScriptObject *object, CKAngelScriptResult *result) {
    if (!object) {
        return StoreResult(result, CKAS_OK);
    }
    const CKAS_STATUS handleStatus = ValidateObjectHandle(object, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
    }
    m_Objects.erase(object);
    if (object->Object) {
        object->Object->Release();
        object->Object = nullptr;
    }
    delete object;
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
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "FindObjectMethod options size is invalid.");
    }
    CKAngelScriptObject *object = ScriptApiSupport::PublicField(options, &CKAngelScriptMethodOptions::Object, static_cast<CKAngelScriptObject *>(nullptr));
    const char *methodName = ScriptApiSupport::PublicField(options, &CKAngelScriptMethodOptions::MethodName, static_cast<const char *>(nullptr));
    const char *methodDecl = ScriptApiSupport::PublicField(options, &CKAngelScriptMethodOptions::MethodDecl, static_cast<const char *>(nullptr));
    const bool hasMethodName = ScriptApiSupport::IsNonEmpty(methodName);
    const bool hasMethodDecl = ScriptApiSupport::IsNonEmpty(methodDecl);
    if (!object) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object handle is required.");
    }
    const CKAS_STATUS objectStatus = ValidateObjectHandle(object, result);
    if (objectStatus != CKAS_OK) {
        return objectStatus;
    }
    if (!object->Object) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object handle is invalid.");
    }
    if (hasMethodName == hasMethodDecl) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Exactly one of MethodName or MethodDecl is required.");
    }
    if (!HasModule(object->ModuleName.c_str()) || GetModuleGeneration(object->ModuleName.c_str()) != object->ModuleGeneration) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Object handle is stale.");
    }
    asITypeInfo *type = object->Object->GetObjectType();
    if (!type) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Script object has no type information.");
    }

    asIScriptFunction *function = nullptr;
    if (hasMethodDecl) {
        function = type->GetMethodByDecl(methodDecl);
    } else {
        asUINT matchCount = 0;
        const asUINT count = type->GetMethodCount();
        for (asUINT i = 0; i < count; ++i) {
            asIScriptFunction *candidate = type->GetMethodByIndex(i);
            const char *candidateName = candidate ? candidate->GetName() : nullptr;
            if (candidateName && std::strcmp(candidateName, methodName) == 0) {
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
    method->ModuleName = object->ModuleName;
    method->ClassName = object->ClassName;
    method->ClassNamespace = object->ClassNamespace;
    method->MethodName = function->GetName() ? function->GetName() : "";
    method->MethodDecl = function->GetDeclaration(false) ? function->GetDeclaration(false) : "";
    method->ModuleGeneration = object->ModuleGeneration;
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
    m_Methods.insert(method);
    *outMethod = method;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseMethod(CKAngelScriptMethod *method, CKAngelScriptResult *result) {
    if (!method) {
        return StoreResult(result, CKAS_OK);
    }
    const CKAS_STATUS handleStatus = ValidateMethodHandle(method, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
    }
    m_Methods.erase(method);
    delete method;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::CallObjectMethod(const CKAngelScriptObjectMethodExecuteOptions &options,
                                            CKAngelScriptResult *result) {
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "CallObjectMethod options size is invalid.");
    }
    CKAngelScriptObject *object = ScriptApiSupport::PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Object, static_cast<CKAngelScriptObject *>(nullptr));
    CKAngelScriptMethod *method = ScriptApiSupport::PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Method, static_cast<CKAngelScriptMethod *>(nullptr));
    const CKDWORD flags = ScriptApiSupport::PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Flags, static_cast<CKDWORD>(CKAS_CALL_DEFAULT));
    if (ScriptApiSupport::HasUnknownPublicFlags(flags, CKAS_CALL_NO_SUSPEND)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown object method call flags.");
    }
    const CKAS_STATUS handleStatus = ValidateObjectMethodHandles(object, method, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
    }
    if (object->ModuleName != method->ModuleName ||
        object->ClassName != method->ClassName ||
        object->ClassNamespace != method->ClassNamespace) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Method handle does not belong to the object type.");
    }
    if (!HasModule(object->ModuleName.c_str()) ||
        GetModuleGeneration(object->ModuleName.c_str()) != object->ModuleGeneration ||
        method->ModuleGeneration != object->ModuleGeneration) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Object or method handle is stale.");
    }
    asITypeInfo *type = object->Object->GetObjectType();
    if (!type) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Script object has no type information.");
    }
    asIScriptFunction *function = type->GetMethodByDecl(method->MethodDecl.c_str());
    if (!function) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Object method handle is stale.");
    }
    CKAngelScriptWriteArgsCallback writeArgs =
        ScriptApiSupport::PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::WriteArgs, static_cast<CKAngelScriptWriteArgsCallback>(nullptr));
    CKAngelScriptReadResultCallback readResult =
        ScriptApiSupport::PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::ReadResult, static_cast<CKAngelScriptReadResultCallback>(nullptr));
    CKAngelScriptContextCallback configureContext =
        ScriptApiSupport::PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::ConfigureContext, static_cast<CKAngelScriptContextCallback>(nullptr));
    CKAngelScriptContextCallback readContextResult =
        ScriptApiSupport::PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::ReadContextResult, static_cast<CKAngelScriptContextCallback>(nullptr));
    void *userData = ScriptApiSupport::PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::UserData, static_cast<void *>(nullptr));

    const ScriptApiSupport::ObjectCallOutcome outcome = ScriptApiSupport::ExecutePreparedObjectMethod(this,
                                                                  m_PublicCallbackDepth,
                                                                  object->Object,
                                                                  function,
                                                                  method,
                                                                  writeArgs,
                                                                  readResult,
                                                                  configureContext,
                                                                  readContextResult,
                                                                  userData,
                                                                  flags);
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
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "CreateFunctionExecution options size is invalid.");
    }
    CKAngelScriptFunction *functionHandle =
        ScriptApiSupport::PublicField(options, &CKAngelScriptFunctionExecutionOptions::Function, static_cast<CKAngelScriptFunction *>(nullptr));
    const CKBehaviorContext *behaviorContext =
        ScriptApiSupport::PublicField(options, &CKAngelScriptFunctionExecutionOptions::BehaviorContext, static_cast<const CKBehaviorContext *>(nullptr));
    const CKDWORD flags = ScriptApiSupport::PublicField(options, &CKAngelScriptFunctionExecutionOptions::Flags, static_cast<CKDWORD>(CKAS_CALL_DEFAULT));
    if (ScriptApiSupport::HasUnknownPublicFlags(flags, CKAS_CALL_NO_SUSPEND)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown function execution flags.");
    }
    if (!functionHandle) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function handle is required.");
    }
    const CKAS_STATUS functionStatus = ValidateFunctionHandle(functionHandle, result);
    if (functionStatus != CKAS_OK) {
        return functionStatus;
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    if (!HasModule(functionHandle->ModuleName.c_str()) ||
        GetModuleGeneration(functionHandle->ModuleName.c_str()) != functionHandle->ModuleGeneration) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Function handle is stale.");
    }
    asIScriptModule *module = GetModule(functionHandle->ModuleName.c_str());
    if (!module) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Function module is no longer loaded.");
    }

    asIScriptFunction *function = functionHandle->FunctionDecl.empty()
                                      ? nullptr
                                      : module->GetFunctionByDecl(functionHandle->FunctionDecl.c_str());
    if (!function) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Function handle is stale.");
    }

    auto *execution = new CKAngelScriptExecution(this);
    execution->ModuleName = functionHandle->ModuleName;
    execution->FunctionName = functionHandle->FunctionName;
    execution->FunctionDecl = functionHandle->FunctionDecl;
    execution->ModuleGeneration = functionHandle->ModuleGeneration;
    execution->Flags = flags;
    if (behaviorContext) {
        execution->BehaviorContextStorage = *behaviorContext;
        execution->HasBehaviorContext = true;
    }
    function->AddRef();
    execution->Function = function;
    if (!execution->Invoker.SetScript(functionHandle->ModuleName.c_str())) {
        const std::string error = execution->Invoker.GetErrorMessage();
        delete execution;
        return StoreResult(result, CKAS_NOTFOUND, 0, error.empty() ? "Module cache was not found." : error);
    }

    ScriptApiSupport::MakeExecutionResult(execution, CKAS_OK);
    m_Executions.insert(execution);
    *outExecution = execution;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::StartExecution(CKAngelScriptExecution *execution,
                                          const CKAngelScriptExecutionStepOptions *options,
                                          CKAngelScriptResult *result) {
    if (options && !ScriptApiSupport::HasCompletePublicStruct(*options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "StartExecution step options size is invalid.");
    }
    const CKAS_STATUS handleStatus = ValidateExecutionHandle(execution, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
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
    if (options && !ScriptApiSupport::HasCompletePublicStruct(*options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "ResumeExecution step options size is invalid.");
    }
    const CKAS_STATUS handleStatus = ValidateExecutionHandle(execution, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
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
    const CKAS_STATUS handleStatus = ValidateExecutionHandle(execution, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
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
    const CKAS_STATUS handleStatus = ValidateExecutionHandle(execution, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
    }
    if (execution->Invoker.IsContextSuspended()) {
        execution->Invoker.AbortContext();
    }
    m_Executions.erase(execution);
    delete execution;
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
    const CKAS_STATUS handleStatus = ValidateExecutionHandle(execution, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
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
    const CKAS_STATUS handleStatus = ValidateExecutionHandle(execution, result);
    if (handleStatus != CKAS_OK) {
        return handleStatus;
    }
    *outResult = &execution->Result;
    return StoreResult(result, CKAS_OK);
}

