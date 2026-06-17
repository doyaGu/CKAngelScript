#include "ScriptAsync.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>

#include <fmt/format.h>

#include "CKAll.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeCommon.h"
#include "ScriptBridgeHandles.h"
#include "ScriptManager.h"
#include "add_on/scriptarray/scriptarray.h"
#include "ScriptRegistration.h"

namespace ScriptAsyncInternal {

std::vector<ScriptAsyncTaskBase *> ReadTaskArray(CScriptArray *tasks);

ScriptAsyncScheduler *SchedulerFromEngine(asIScriptEngine *engine) {
    ScriptManager *manager = engine ? ScriptManager::GetManager(engine) : nullptr;
    return manager ? manager->GetAsyncScheduler() : nullptr;
}

ScriptAsyncScheduler *SchedulerFromActiveContext() {
    asIScriptContext *ctx = asGetActiveContext();
    return ctx ? SchedulerFromEngine(ctx->GetEngine()) : nullptr;
}

asIScriptEngine *EngineFromActiveContext(const char *apiName) {
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx ? ctx->GetEngine() : nullptr;
    if (!engine) {
        SetScriptException(fmt::format("{} requires an active AngelScript context.", apiName ? apiName : "Async API"));
    }
    return engine;
}

std::string TypeName(asIScriptEngine *engine, int typeId) {
    if (!engine) {
        return "<no engine>";
    }
    const char *decl = engine->GetTypeDeclaration(typeId, true);
    return decl ? decl : fmt::format("#{}", typeId);
}

asITypeInfo *TypeInfoById(asIScriptEngine *engine, int typeId) {
    if (!engine || typeId == asTYPEID_VOID) {
        return nullptr;
    }
    asITypeInfo *type = engine->GetTypeInfoById(typeId);
    if (!type && (typeId & asTYPEID_OBJHANDLE)) {
        type = engine->GetTypeInfoById(typeId & ~asTYPEID_OBJHANDLE);
    }
    return type;
}

bool IsAsyncTaskType(asIScriptEngine *engine, int typeId, int &subtypeId, std::string &error) {
    if (!(typeId & asTYPEID_OBJHANDLE)) {
        error = fmt::format("Expected AsyncTask<T>@ handle, got {}.", TypeName(engine, typeId));
        return false;
    }
    asITypeInfo *type = TypeInfoById(engine, typeId);
    if (!type || !type->GetName() || std::string(type->GetName()) != "AsyncTask") {
        error = fmt::format("Expected AsyncTask<T>@ handle, got {}.", TypeName(engine, typeId));
        return false;
    }
    subtypeId = type->GetSubTypeId();
    return true;
}

ScriptAsyncTaskBase *TaskFromGenericArg(asIScriptGeneric *gen, asUINT arg, int *subtypeId, std::string &error) {
    asIScriptEngine *engine = gen ? gen->GetEngine() : nullptr;
    int subtype = asTYPEID_VOID;
    if (!gen || !IsAsyncTaskType(engine, gen->GetArgTypeId(arg), subtype, error)) {
        return nullptr;
    }
    void *address = gen->GetArgAddress(arg);
    ScriptAsyncTaskBase *task = address ? *static_cast<ScriptAsyncTaskBase **>(address) : nullptr;
    if (!task) {
        error = "AsyncTask argument is null.";
        return nullptr;
    }
    if (subtypeId) {
        *subtypeId = subtype;
    }
    return task;
}

bool TaskOutFromGenericArg(asIScriptGeneric *gen,
                           asUINT arg,
                           int &subtypeId,
                           ScriptAsyncTaskBase ***slot,
                           std::string &error) {
    asIScriptEngine *engine = gen ? gen->GetEngine() : nullptr;
    if (!gen || !IsAsyncTaskType(engine, gen->GetArgTypeId(arg), subtypeId, error)) {
        return false;
    }
    void *address = gen->GetArgAddress(arg);
    if (!address) {
        error = "AsyncTask output argument address is null.";
        return false;
    }
    *slot = static_cast<ScriptAsyncTaskBase **>(address);
    return true;
}

bool AssignTaskOut(asIScriptGeneric *gen, asUINT arg, ScriptAsyncTaskBase *task, std::string &error) {
    int outputSubtype = asTYPEID_VOID;
    ScriptAsyncTaskBase **slot = nullptr;
    if (!TaskOutFromGenericArg(gen, arg, outputSubtype, &slot, error)) {
        return false;
    }
    if (!task) {
        error = "Async API produced a null task.";
        return false;
    }
    if (outputSubtype != task->SubTypeId()) {
        error = fmt::format("AsyncTask output type mismatch: task is AsyncTask<{}>, output is AsyncTask<{}>.",
                            TypeName(gen->GetEngine(), task->SubTypeId()),
                            TypeName(gen->GetEngine(), outputSubtype));
        return false;
    }
    *slot = task;
    return true;
}

asIScriptFunction *FunctionFromGenericArg(asIScriptGeneric *gen, asUINT arg, std::string &error) {
    asIScriptEngine *engine = gen ? gen->GetEngine() : nullptr;
    if (!gen || !engine) {
        error = "Async generic API requires an AngelScript engine.";
        return nullptr;
    }
    const int typeId = gen->GetArgTypeId(arg);
    asITypeInfo *type = TypeInfoById(engine, typeId);
    if (!type || (type->GetFlags() & asOBJ_FUNCDEF) == 0) {
        error = fmt::format("Async::Spawn requires a function handle, got {}.", TypeName(engine, typeId));
        return nullptr;
    }
    void *address = gen->GetArgAddress(arg);
    asIScriptFunction *function = address ? *static_cast<asIScriptFunction **>(address) : nullptr;
    if (!function) {
        error = "Async::Spawn requires a non-null function handle.";
        return nullptr;
    }
    return function;
}

bool ValidateFunctionReturn(asIScriptEngine *engine, asIScriptFunction *function, int subtypeId, std::string &error) {
    const int returnTypeId = function ? function->GetReturnTypeId() : asTYPEID_VOID;
    if (returnTypeId == subtypeId) {
        return true;
    }
    error = fmt::format("Async::Spawn return type mismatch: function returns {}, task expects {}.",
                        TypeName(engine, returnTypeId),
                        TypeName(engine, subtypeId));
    return false;
}

int ArrayHandleTypeForElement(asIScriptEngine *engine, int elementTypeId) {
    if (!engine || elementTypeId == asTYPEID_VOID) {
        return asTYPEID_VOID;
    }
    const char *elementDecl = engine->GetTypeDeclaration(elementTypeId, true);
    if (!elementDecl) {
        return asTYPEID_VOID;
    }
    const std::string decl = fmt::format("array<{}>@", elementDecl);
    return engine->GetTypeIdByDecl(decl.c_str());
}

bool ReadGenericTaskArray(asIScriptGeneric *gen,
                          asUINT arg,
                          std::vector<ScriptAsyncTaskBase *> &children,
                          int &elementSubtypeId,
                          std::string &error) {
    asIScriptEngine *engine = gen ? gen->GetEngine() : nullptr;
    const int tasksTypeId = gen ? gen->GetArgTypeId(arg) : asTYPEID_VOID;
    asITypeInfo *arrayType = TypeInfoById(engine, tasksTypeId);
    if (!arrayType || std::string(arrayType->GetName() ? arrayType->GetName() : "") != "array") {
        error = fmt::format("Async aggregate requires array<AsyncTask<T>@>@, got {}.", TypeName(engine, tasksTypeId));
        return false;
    }
    CScriptArray *tasks = nullptr;
    void *address = gen->GetArgAddress(arg);
    if (tasksTypeId & asTYPEID_OBJHANDLE) {
        tasks = address ? *static_cast<CScriptArray **>(address) : nullptr;
    } else {
        tasks = static_cast<CScriptArray *>(address);
    }
    if (!tasks) {
        error = "Async aggregate task array is null.";
        return false;
    }
    const int taskElementTypeId = tasks->GetElementTypeId();
    if (!IsAsyncTaskType(engine, taskElementTypeId, elementSubtypeId, error)) {
        return false;
    }
    children = ReadTaskArray(tasks);
    return true;
}

std::string ContextExceptionMessage(asIScriptContext *ctx, const char *label) {
    if (!ctx) {
        return "AngelScript context is not available.";
    }

    const char *section = nullptr;
    int col = 0;
    const int row = ctx->GetExceptionLineNumber(&col, &section);
    asIScriptFunction *exFunc = ctx->GetExceptionFunction();
    return fmt::format("{} threw in {} at {}({},{}): {}",
                       label ? label : "Async task",
                       exFunc ? exFunc->GetDeclaration() : "<unknown>",
                       section ? section : "<unknown>",
                       row,
                       col,
                       ctx->GetExceptionString() ? ctx->GetExceptionString() : "");
}

bool ScriptAsyncTemplateCallback(asITypeInfo *type, bool &dontGarbageCollect) {
    dontGarbageCollect = true;
    return type != nullptr;
}

ScriptAsyncTaskBase *TaskFromArray(CScriptArray *tasks, asUINT index) {
    if (!tasks || index >= tasks->GetSize()) {
        return nullptr;
    }
    void *slot = tasks->At(index);
    return slot ? *static_cast<ScriptAsyncTaskBase **>(slot) : nullptr;
}

std::vector<ScriptAsyncTaskBase *> ReadTaskArray(CScriptArray *tasks) {
    std::vector<ScriptAsyncTaskBase *> result;
    if (!tasks) {
        return result;
    }
    result.reserve(tasks->GetSize());
    for (asUINT i = 0; i < tasks->GetSize(); ++i) {
        result.push_back(TaskFromArray(tasks, i));
    }
    return result;
}

int ArrayHandleType(asIScriptEngine *engine, const char *elementDecl) {
    if (!engine || !elementDecl) {
        return asTYPEID_VOID;
    }
    const std::string decl = fmt::format("array<{}>@", elementDecl);
    return engine->GetTypeIdByDecl(decl.c_str());
}

ScriptAsyncTaskBase *CreateAggregateFromArray(CScriptArray *tasks,
                                              ScriptAsyncTaskKind kind,
                                              int subtypeId) {
    ScriptAsyncScheduler *scheduler = SchedulerFromActiveContext();
    if (!scheduler) {
        SetScriptException("Async scheduler is not available.");
        return nullptr;
    }
    return scheduler->CreateAggregate(kind, subtypeId, ReadTaskArray(tasks));
}

ScriptAsyncTaskBase *AsyncDelay(int frames) {
    ScriptAsyncScheduler *scheduler = SchedulerFromActiveContext();
    if (!scheduler) {
        SetScriptException("Async scheduler is not available.");
        return nullptr;
    }
    return scheduler->CreateDelay(frames);
}

ScriptAsyncTaskBase *AsyncSpawn(asIScriptFunction *function, const char *typeDecl) {
    ScriptAsyncScheduler *scheduler = SchedulerFromActiveContext();
    asIScriptEngine *engine = scheduler ? scheduler->GetEngine() : nullptr;
    if (!scheduler || !engine) {
        SetScriptException("Async scheduler is not available.");
        return nullptr;
    }
    if (!function) {
        SetScriptException("Async::Spawn requires a function.");
        return nullptr;
    }

    const int typeId = typeDecl && typeDecl[0] != '\0' ? engine->GetTypeIdByDecl(typeDecl) : asTYPEID_VOID;
    if (typeId == 0) {
        SetScriptException(fmt::format("Async::Spawn result type '{}' is not registered.", typeDecl ? typeDecl : "void"));
        return nullptr;
    }
    return scheduler->CreateScriptTask(function, typeId);
}

ScriptAsyncTaskBase *AsyncSpawnVoid(asIScriptFunction *function) { return AsyncSpawn(function, "void"); }
ScriptAsyncTaskBase *AsyncSpawnInt(asIScriptFunction *function) { return AsyncSpawn(function, "int"); }
ScriptAsyncTaskBase *AsyncSpawnFloat(asIScriptFunction *function) { return AsyncSpawn(function, "float"); }
ScriptAsyncTaskBase *AsyncSpawnString(asIScriptFunction *function) { return AsyncSpawn(function, "string"); }
ScriptAsyncTaskBase *AsyncSpawnObject(asIScriptFunction *function) { return AsyncSpawn(function, "CKObject@"); }

void AwaitTask(ScriptAsyncTaskBase *task, void *outAddress, int outTypeId) {
    asIScriptContext *ctx = asGetActiveContext();
    ScriptAsyncScheduler *scheduler = ctx ? SchedulerFromEngine(ctx->GetEngine()) : nullptr;
    if (!ctx || !scheduler) {
        SetScriptException("Await requires an active AngelScript context and async scheduler.");
        return;
    }
    if (!task) {
        ctx->SetException("Await requires a valid AsyncTask.");
        return;
    }

    if (task->IsCompleted()) {
        if (outTypeId != asTYPEID_VOID) {
            std::string error;
            if (!task->CopyResultTo(outAddress, outTypeId, error)) {
                ctx->SetException(error.c_str());
            }
        }
        return;
    }
    if (task->IsFailed() || task->IsCancelled()) {
        ctx->SetException(task->Error().empty() ? "Async task failed." : task->Error().c_str());
        return;
    }

    std::string error;
    if (!scheduler->WaitForTask(ctx, task, outAddress, outTypeId, error)) {
        ctx->SetException(error.c_str());
        return;
    }
    ctx->Suspend();
}

void AwaitVoid(ScriptAsyncTaskBase *task) {
    AwaitTask(task, nullptr, asTYPEID_VOID);
}

void AwaitInt(ScriptAsyncTaskBase *task, int &out) {
    asIScriptContext *ctx = asGetActiveContext();
    int typeId = ctx && ctx->GetEngine() ? ctx->GetEngine()->GetTypeIdByDecl("int") : asTYPEID_INT32;
    AwaitTask(task, &out, typeId);
}

void AwaitFloat(ScriptAsyncTaskBase *task, float &out) {
    asIScriptContext *ctx = asGetActiveContext();
    int typeId = ctx && ctx->GetEngine() ? ctx->GetEngine()->GetTypeIdByDecl("float") : asTYPEID_FLOAT;
    AwaitTask(task, &out, typeId);
}

void AwaitString(ScriptAsyncTaskBase *task, std::string &out) {
    asIScriptContext *ctx = asGetActiveContext();
    int typeId = ctx && ctx->GetEngine() ? ctx->GetEngine()->GetTypeIdByDecl("string") : 0;
    AwaitTask(task, &out, typeId);
}

void AwaitObject(ScriptAsyncTaskBase *task, CKObject *&out) {
    asIScriptContext *ctx = asGetActiveContext();
    int typeId = ctx && ctx->GetEngine() ? ctx->GetEngine()->GetTypeIdByDecl("CKObject@") : 0;
    AwaitTask(task, &out, typeId);
}

void AwaitGeneric(asIScriptGeneric *gen) {
    std::string error;
    int declaredSubtype = asTYPEID_VOID;
    ScriptAsyncTaskBase *task = TaskFromGenericArg(gen, 0, &declaredSubtype, error);
    if (!task) {
        SetScriptException(error);
        return;
    }
    if (declaredSubtype != task->SubTypeId()) {
        SetScriptException(fmt::format("Await task type mismatch: handle is AsyncTask<{}>, task stores AsyncTask<{}>.",
                                       TypeName(gen->GetEngine(), declaredSubtype),
                                       TypeName(gen->GetEngine(), task->SubTypeId())));
        return;
    }
    AwaitTask(task, gen->GetArgAddress(1), gen->GetArgTypeId(1));
}

ScriptAsyncTaskBase *AsyncAllVoid(CScriptArray *tasks) {
    return CreateAggregateFromArray(tasks, ScriptAsyncTaskKind::All, asTYPEID_VOID);
}

ScriptAsyncTaskBase *AsyncRaceVoid(CScriptArray *tasks) {
    return CreateAggregateFromArray(tasks, ScriptAsyncTaskKind::Race, asTYPEID_VOID);
}

ScriptAsyncTaskBase *AsyncAnyVoid(CScriptArray *tasks) {
    return CreateAggregateFromArray(tasks, ScriptAsyncTaskKind::Any, asTYPEID_VOID);
}

ScriptAsyncTaskBase *AsyncWaitBBTask(BBTask *task) {
    ScriptAsyncScheduler *scheduler = SchedulerFromActiveContext();
    if (!scheduler) {
        SetScriptException("Async scheduler is not available.");
        return nullptr;
    }
    return scheduler->CreateBBTask(task);
}

ScriptAsyncTaskBase *AsyncWaitGraphTask(GraphTask *task) {
    ScriptAsyncScheduler *scheduler = SchedulerFromActiveContext();
    if (!scheduler) {
        SetScriptException("Async scheduler is not available.");
        return nullptr;
    }
    return scheduler->CreateGraphTask(task);
}

ScriptAsyncTaskBase *AsyncWaitBBTaskWithContext(const CKBehaviorContext &ctx, BBTask *task, int inputIndex) {
    ScriptAsyncScheduler *scheduler = SchedulerFromActiveContext();
    if (!scheduler) {
        SetScriptException("Async scheduler is not available.");
        return nullptr;
    }
    return scheduler->CreateBBTask(task, &ctx, inputIndex);
}

ScriptAsyncTaskBase *AsyncWaitGraphTaskWithContext(const CKBehaviorContext &ctx, GraphTask *task) {
    ScriptAsyncScheduler *scheduler = SchedulerFromActiveContext();
    if (!scheduler) {
        SetScriptException("Async scheduler is not available.");
        return nullptr;
    }
    return scheduler->CreateGraphTask(task, &ctx);
}

void AsyncSpawnGeneric(asIScriptGeneric *gen) {
    std::string error;
    asIScriptFunction *function = FunctionFromGenericArg(gen, 0, error);
    int subtypeId = asTYPEID_VOID;
    ScriptAsyncTaskBase **slot = nullptr;
    if (!function ||
        !TaskOutFromGenericArg(gen, 1, subtypeId, &slot, error) ||
        !ValidateFunctionReturn(gen->GetEngine(), function, subtypeId, error)) {
        SetScriptException(error);
        return;
    }

    ScriptAsyncScheduler *scheduler = SchedulerFromEngine(gen->GetEngine());
    if (!scheduler) {
        SetScriptException("Async scheduler is not available.");
        return;
    }
    ScriptAsyncTaskBase *task = scheduler->CreateScriptTask(function, subtypeId);
    if (!AssignTaskOut(gen, 1, task, error)) {
        task->Release();
        SetScriptException(error);
    }
}

void AsyncAggregateGeneric(asIScriptGeneric *gen, ScriptAsyncTaskKind kind, const char *name) {
    std::string error;
    std::vector<ScriptAsyncTaskBase *> children;
    int childSubtypeId = asTYPEID_VOID;
    int outputSubtypeId = asTYPEID_VOID;
    ScriptAsyncTaskBase **slot = nullptr;
    asIScriptEngine *engine = gen ? gen->GetEngine() : nullptr;

    if (!ReadGenericTaskArray(gen, 0, children, childSubtypeId, error) ||
        !TaskOutFromGenericArg(gen, 1, outputSubtypeId, &slot, error)) {
        SetScriptException(error);
        return;
    }

    int expectedOutputSubtype = childSubtypeId;
    if (kind == ScriptAsyncTaskKind::All) {
        expectedOutputSubtype = childSubtypeId == asTYPEID_VOID
            ? asTYPEID_VOID
            : ArrayHandleTypeForElement(engine, childSubtypeId);
    }
    if (expectedOutputSubtype == asTYPEID_VOID && childSubtypeId != asTYPEID_VOID && kind == ScriptAsyncTaskKind::All) {
        SetScriptException(fmt::format("Async::{} could not resolve array result type for {}.",
                                       name,
                                       TypeName(engine, childSubtypeId)));
        return;
    }
    if (outputSubtypeId != expectedOutputSubtype) {
        SetScriptException(fmt::format("Async::{} output type mismatch: expected AsyncTask<{}>, got AsyncTask<{}>.",
                                       name,
                                       TypeName(engine, expectedOutputSubtype),
                                       TypeName(engine, outputSubtypeId)));
        return;
    }

    ScriptAsyncScheduler *scheduler = SchedulerFromEngine(engine);
    if (!scheduler) {
        SetScriptException("Async scheduler is not available.");
        return;
    }
    ScriptAsyncTaskBase *task = scheduler->CreateAggregate(kind, outputSubtypeId, children);
    if (!AssignTaskOut(gen, 1, task, error)) {
        task->Release();
        SetScriptException(error);
    }
}

void AsyncAllGeneric(asIScriptGeneric *gen) { AsyncAggregateGeneric(gen, ScriptAsyncTaskKind::All, "All"); }
void AsyncRaceGeneric(asIScriptGeneric *gen) { AsyncAggregateGeneric(gen, ScriptAsyncTaskKind::Race, "Race"); }
void AsyncAnyGeneric(asIScriptGeneric *gen) { AsyncAggregateGeneric(gen, ScriptAsyncTaskKind::Any, "Any"); }

} // namespace ScriptAsyncInternal

ScriptAsyncStoredValue::~ScriptAsyncStoredValue() {
    Clear();
}

void ScriptAsyncStoredValue::Clear() {
    if (m_HasValue && m_Object && m_Engine && (m_TypeId & asTYPEID_MASK_OBJECT)) {
        m_Engine->ReleaseScriptObject(m_Object, m_Engine->GetTypeInfoById(m_TypeId));
    }
    m_Engine = nullptr;
    m_TypeId = asTYPEID_VOID;
    m_HasValue = false;
    m_Primitive.clear();
    m_Object = nullptr;
}

void ScriptAsyncStoredValue::Store(asIScriptEngine *engine, void *address, int typeId) {
    Clear();
    m_Engine = engine;
    m_TypeId = typeId;
    m_HasValue = true;

    if (!m_Engine || typeId == asTYPEID_VOID) {
        return;
    }

    if (typeId & asTYPEID_OBJHANDLE) {
        m_Object = address ? *static_cast<void **>(address) : nullptr;
        if (m_Object) {
            m_Engine->AddRefScriptObject(m_Object, m_Engine->GetTypeInfoById(typeId));
        }
        return;
    }

    if (typeId & asTYPEID_MASK_OBJECT) {
        m_Object = address ? m_Engine->CreateScriptObjectCopy(address, m_Engine->GetTypeInfoById(typeId)) : nullptr;
        return;
    }

    const int size = m_Engine->GetSizeOfPrimitiveType(typeId);
    if (size > 0 && address) {
        m_Primitive.resize(static_cast<size_t>(size));
        std::memcpy(m_Primitive.data(), address, m_Primitive.size());
    }
}

bool ScriptAsyncStoredValue::CopyTo(void *address, int typeId, std::string &error) const {
    if (typeId == asTYPEID_VOID) {
        return true;
    }
    if (!address) {
        error = "Async task result output address is null.";
        return false;
    }
    if (!m_HasValue) {
        error = "Async task has no result value.";
        return false;
    }
    if (!m_Engine) {
        error = "Async task result storage has no AngelScript engine.";
        return false;
    }

    if (typeId & asTYPEID_OBJHANDLE) {
        void **out = static_cast<void **>(address);
        *out = nullptr;
        if (!m_Object) {
            return true;
        }

        asITypeInfo *fromType = m_Engine->GetTypeInfoById(m_TypeId);
        asITypeInfo *toType = m_Engine->GetTypeInfoById(typeId);
        if (!fromType || !toType) {
            error = fmt::format("Async task result type mismatch: stored {}, requested {}.",
                                ScriptAsyncInternal::TypeName(m_Engine, m_TypeId),
                                ScriptAsyncInternal::TypeName(m_Engine, typeId));
            return false;
        }

        if (fromType == toType || m_TypeId == typeId) {
            *out = m_Object;
            m_Engine->AddRefScriptObject(*out, toType);
            return true;
        }

        const int r = m_Engine->RefCastObject(m_Object, fromType, toType, out);
        if (r < 0 || !*out) {
            error = fmt::format("Async task result type mismatch: stored {}, requested {}.",
                                ScriptAsyncInternal::TypeName(m_Engine, m_TypeId),
                                ScriptAsyncInternal::TypeName(m_Engine, typeId));
            return false;
        }
        return true;
    }

    if (typeId & asTYPEID_MASK_OBJECT) {
        if (!(m_TypeId & asTYPEID_MASK_OBJECT) || (m_TypeId & asTYPEID_OBJHANDLE) || m_TypeId != typeId || !m_Object) {
            error = fmt::format("Async task result type mismatch: stored {}, requested {}.",
                                ScriptAsyncInternal::TypeName(m_Engine, m_TypeId),
                                ScriptAsyncInternal::TypeName(m_Engine, typeId));
            return false;
        }
        if (m_Engine->AssignScriptObject(address, m_Object, m_Engine->GetTypeInfoById(typeId)) < 0) {
            error = fmt::format("Async task failed to assign result of type {}.", ScriptAsyncInternal::TypeName(m_Engine, typeId));
            return false;
        }
        return true;
    }

    const int size = m_Engine->GetSizeOfPrimitiveType(typeId);
    if (m_TypeId != typeId || size <= 0 || m_Primitive.size() != static_cast<size_t>(size)) {
        error = fmt::format("Async task result type mismatch: stored {}, requested {}.",
                            ScriptAsyncInternal::TypeName(m_Engine, m_TypeId),
                            ScriptAsyncInternal::TypeName(m_Engine, typeId));
        return false;
    }
    std::memcpy(address, m_Primitive.data(), m_Primitive.size());
    return true;
}

bool ScriptAsyncStoredValue::CopyFrom(const ScriptAsyncStoredValue &other, std::string &error) {
    Clear();
    if (!other.m_HasValue) {
        return true;
    }

    m_Engine = other.m_Engine;
    m_TypeId = other.m_TypeId;
    m_HasValue = true;
    m_Primitive = other.m_Primitive;

    if (!m_Engine || !(m_TypeId & asTYPEID_MASK_OBJECT) || !other.m_Object) {
        return true;
    }

    asITypeInfo *type = m_Engine->GetTypeInfoById(m_TypeId);
    if (!type) {
        error = fmt::format("Async task result type '{}' is not registered.", ScriptAsyncInternal::TypeName(m_Engine, m_TypeId));
        Clear();
        return false;
    }

    if (m_TypeId & asTYPEID_OBJHANDLE) {
        m_Object = other.m_Object;
        m_Engine->AddRefScriptObject(m_Object, type);
    } else {
        m_Object = m_Engine->CreateScriptObjectCopy(other.m_Object, type);
        if (!m_Object) {
            error = fmt::format("Async task failed to copy result of type {}.", ScriptAsyncInternal::TypeName(m_Engine, m_TypeId));
            Clear();
            return false;
        }
    }
    return true;
}

ScriptAsyncTaskBase::ScriptAsyncTaskBase(ScriptAsyncScheduler *scheduler,
                                         asIScriptEngine *engine,
                                         ScriptAsyncTaskKind kind,
                                         int subtypeId)
    : m_Scheduler(scheduler), m_Engine(engine), m_Kind(kind), m_SubTypeId(subtypeId) {}

ScriptAsyncTaskBase::~ScriptAsyncTaskBase() {
    ReleaseContext();
    if (m_Function) {
        m_Function->Release();
        m_Function = nullptr;
    }
    ReleaseChildren();
    ReleaseBridgeTasks();
    ReleaseResult();
}

bool ScriptAsyncTaskBase::IsPending() const { return m_State == ScriptAsyncTaskState::Pending; }
bool ScriptAsyncTaskBase::IsRunning() const { return m_State == ScriptAsyncTaskState::Running; }
bool ScriptAsyncTaskBase::IsCompleted() const { return m_State == ScriptAsyncTaskState::Completed; }
bool ScriptAsyncTaskBase::IsFailed() const { return m_State == ScriptAsyncTaskState::Failed; }
bool ScriptAsyncTaskBase::IsCancelled() const { return m_State == ScriptAsyncTaskState::Cancelled; }
bool ScriptAsyncTaskBase::IsDone() const { return IsCompleted() || IsFailed() || IsCancelled(); }

std::string ScriptAsyncTaskBase::Error() const {
    if (m_State == ScriptAsyncTaskState::Cancelled && m_Error.empty()) {
        return "Async task was cancelled.";
    }
    return m_Error;
}

bool ScriptAsyncTaskBase::Cancel() {
    if (IsDone()) {
        return false;
    }
    if (m_Context) {
        m_Context->Abort();
    }
    for (ScriptAsyncTaskBase *child : m_Children) {
        if (child) {
            child->Cancel();
        }
    }
    if (m_BBTask) {
        m_BBTask->Destroy();
    }
    if (m_GraphTask) {
        m_GraphTask->Cancel();
    }
    m_Error = "Async task was cancelled.";
    SetState(ScriptAsyncTaskState::Cancelled);
    return true;
}

void ScriptAsyncTaskBase::SetDelayFrames(int frames) {
    m_DelayFrames = std::max(0, frames);
}

void ScriptAsyncTaskBase::SetFunction(asIScriptFunction *function) {
    if (m_Function == function) {
        return;
    }
    if (m_Function) {
        m_Function->Release();
    }
    m_Function = function;
    if (m_Function) {
        m_Function->AddRef();
    }
}

void ScriptAsyncTaskBase::SetChildren(const std::vector<ScriptAsyncTaskBase *> &children) {
    ReleaseChildren();
    m_Children = children;
    for (ScriptAsyncTaskBase *child : m_Children) {
        if (child) {
            child->AddRef();
        }
    }
}

void ScriptAsyncTaskBase::SetBBTask(BBTask *task, const CKBehaviorContext *context, int inputIndex) {
    if (m_BBTask == task) {
        m_DriveBridgeTask = context != nullptr;
        m_BridgeTaskContext = context ? *context : CKBehaviorContext();
        m_BBTaskInputIndex = inputIndex;
        return;
    }
    if (m_BBTask) {
        m_BBTask->Release();
    }
    m_BBTask = task;
    if (m_BBTask) {
        m_BBTask->AddRef();
    }
    m_DriveBridgeTask = context != nullptr;
    m_BridgeTaskContext = context ? *context : CKBehaviorContext();
    m_BBTaskInputIndex = inputIndex;
}

void ScriptAsyncTaskBase::SetGraphTask(GraphTask *task, const CKBehaviorContext *context) {
    if (m_GraphTask == task) {
        m_DriveBridgeTask = context != nullptr;
        m_BridgeTaskContext = context ? *context : CKBehaviorContext();
        return;
    }
    if (m_GraphTask) {
        m_GraphTask->Release();
    }
    m_GraphTask = task;
    if (m_GraphTask) {
        m_GraphTask->AddRef();
    }
    m_DriveBridgeTask = context != nullptr;
    m_BridgeTaskContext = context ? *context : CKBehaviorContext();
}

void ScriptAsyncTaskBase::Advance() {
    if (IsDone()) {
        return;
    }

    switch (m_Kind) {
        case ScriptAsyncTaskKind::Manual:
            break;
        case ScriptAsyncTaskKind::Delay:
            AdvanceDelay();
            break;
        case ScriptAsyncTaskKind::Script:
            AdvanceScript();
            break;
        case ScriptAsyncTaskKind::All:
            AdvanceAll();
            break;
        case ScriptAsyncTaskKind::Race:
            AdvanceRace();
            break;
        case ScriptAsyncTaskKind::Any:
            AdvanceAny();
            break;
        case ScriptAsyncTaskKind::BBTask:
            AdvanceBBTask();
            break;
        case ScriptAsyncTaskKind::GraphTask:
            AdvanceGraphTask();
            break;
    }
}

bool ScriptAsyncTaskBase::CopyResultTo(void *address, int typeId, std::string &error) const {
    if (typeId == asTYPEID_VOID) {
        return true;
    }
    if (!IsCompleted()) {
        error = TaskErrorForAwait();
        return false;
    }
    return m_Result.CopyTo(address, typeId, error);
}

bool ScriptAsyncTaskBase::CopyResultFrom(const ScriptAsyncTaskBase *other, std::string &error) {
    if (!other || !other->IsCompleted()) {
        error = "Async task result source is not completed.";
        return false;
    }
    ReleaseResult();
    if (!m_Result.CopyFrom(other->m_Result, error)) {
        return false;
    }
    SetState(ScriptAsyncTaskState::Completed);
    return true;
}

void ScriptAsyncTaskBase::CompleteVoid() {
    ReleaseResult();
    m_Result.Store(m_Engine, nullptr, asTYPEID_VOID);
    SetState(ScriptAsyncTaskState::Completed);
}

void ScriptAsyncTaskBase::CompleteFromAddress(void *address, int typeId) {
    ReleaseResult();
    m_Result.Store(m_Engine, address, typeId);
    SetState(ScriptAsyncTaskState::Completed);
}

void ScriptAsyncTaskBase::CompleteWithArray(int arrayTypeId, void *arrayHandle) {
    void *handle = arrayHandle;
    CompleteFromAddress(&handle, arrayTypeId);
}

void ScriptAsyncTaskBase::Fail(const std::string &error) {
    if (IsDone()) {
        return;
    }
    m_Error = error.empty() ? "Async task failed." : error;
    SetState(ScriptAsyncTaskState::Failed);
}

void ScriptAsyncTaskBase::SetState(ScriptAsyncTaskState state) {
    m_State = state;
}

void ScriptAsyncTaskBase::ReleaseResult() {
    m_Result.Clear();
}

void ScriptAsyncTaskBase::ReleaseContext() {
    if (!m_Context) {
        return;
    }
    asIScriptContext *ctx = m_Context;
    m_Context = nullptr;
    if (m_Scheduler) {
        m_Scheduler->ForgetContext(ctx);
    }
    if (m_Engine) {
        m_Engine->ReturnContext(ctx);
    } else {
        ctx->Release();
    }
}

void ScriptAsyncTaskBase::ReleaseChildren() {
    for (ScriptAsyncTaskBase *child : m_Children) {
        if (child) {
            child->Release();
        }
    }
    m_Children.clear();
}

void ScriptAsyncTaskBase::ReleaseBridgeTasks() {
    if (m_BBTask) {
        m_BBTask->Release();
        m_BBTask = nullptr;
    }
    if (m_GraphTask) {
        m_GraphTask->Release();
        m_GraphTask = nullptr;
    }
}

void ScriptAsyncTaskBase::AdvanceDelay() {
    SetState(ScriptAsyncTaskState::Running);
    if (m_DelayFrames > 0) {
        --m_DelayFrames;
    }
    if (m_DelayFrames <= 0) {
        CompleteVoid();
    }
}

void ScriptAsyncTaskBase::AdvanceScript() {
    if (!m_Function || !m_Engine) {
        Fail("Async script task has no function.");
        return;
    }

    if (!m_Context) {
        m_Context = m_Engine->RequestContext();
        if (!m_Context) {
            Fail("Async script task failed to request AngelScript context.");
            return;
        }
    }

    int r = 0;
    if (!m_Started) {
        if (m_Function->GetFuncType() == asFUNC_DELEGATE) {
            asIScriptFunction *delegate = m_Function->GetDelegateFunction();
            void *delegateObject = m_Function->GetDelegateObject();
            r = m_Context->Prepare(delegate);
            if (r >= 0) {
                r = m_Context->SetObject(delegateObject);
            }
        } else {
            r = m_Context->Prepare(m_Function);
        }
        if (r < 0) {
            Fail("Async script task failed to prepare function.");
            ReleaseContext();
            return;
        }
        m_Started = true;
    } else if (m_Context->GetState() == asEXECUTION_SUSPENDED && m_Scheduler) {
        std::string waitError;
        const ScriptAsyncScheduler::ResumeState resume = m_Scheduler->PrepareContextResume(m_Context, waitError);
        if (resume == ScriptAsyncScheduler::ResumeState::Pending) {
            return;
        }
        if (resume == ScriptAsyncScheduler::ResumeState::Failed) {
            Fail(waitError);
            ReleaseContext();
            return;
        }
    }

    SetState(ScriptAsyncTaskState::Running);
    r = m_Context->Execute();
    if (r == asEXECUTION_FINISHED) {
        const int returnTypeId = m_Function->GetReturnTypeId();
        if (returnTypeId == asTYPEID_VOID) {
            CompleteVoid();
        } else {
            CompleteFromAddress(m_Context->GetAddressOfReturnValue(), returnTypeId);
        }
        ReleaseContext();
    } else if (r == asEXECUTION_SUSPENDED) {
        SetState(ScriptAsyncTaskState::Running);
    } else {
        Fail(r == asEXECUTION_EXCEPTION
             ? ScriptAsyncInternal::ContextExceptionMessage(m_Context, "Async::Spawn")
             : fmt::format("Async::Spawn failed with AngelScript result {}.", r));
        ReleaseContext();
    }
}

void ScriptAsyncTaskBase::AdvanceAll() {
    if (m_Children.empty()) {
        if (m_SubTypeId == asTYPEID_VOID) {
            CompleteVoid();
        } else {
            std::string error;
            if (!CompleteAllArray(error)) {
                Fail(error);
            }
        }
        return;
    }

    bool allComplete = true;
    for (ScriptAsyncTaskBase *child : m_Children) {
        if (!child) {
            Fail("Async::All received a null task.");
            return;
        }
        if (child->IsFailed() || child->IsCancelled()) {
            Fail(child->Error());
            return;
        }
        if (!child->IsCompleted()) {
            allComplete = false;
        }
    }
    if (!allComplete) {
        SetState(ScriptAsyncTaskState::Running);
        return;
    }
    if (m_SubTypeId == asTYPEID_VOID) {
        CompleteVoid();
        return;
    }
    std::string error;
    if (!CompleteAllArray(error)) {
        Fail(error);
    }
}

void ScriptAsyncTaskBase::AdvanceRace() {
    if (m_Children.empty()) {
        Fail("Async::Race requires at least one task.");
        return;
    }
    for (ScriptAsyncTaskBase *child : m_Children) {
        if (!child || !child->IsDone()) {
            continue;
        }
        for (ScriptAsyncTaskBase *other : m_Children) {
            if (other && other != child && !other->IsDone()) {
                other->Cancel();
            }
        }
        if (child->IsCompleted()) {
            if (m_SubTypeId == asTYPEID_VOID) {
                CompleteVoid();
            } else {
                std::string error;
                if (!CopyResultFrom(child, error)) {
                    Fail(error);
                }
            }
        } else {
            Fail(child->Error());
        }
        return;
    }
    SetState(ScriptAsyncTaskState::Running);
}

void ScriptAsyncTaskBase::AdvanceAny() {
    if (m_Children.empty()) {
        Fail("Async::Any requires at least one task.");
        return;
    }
    int failures = 0;
    std::ostringstream errors;
    for (ScriptAsyncTaskBase *child : m_Children) {
        if (!child) {
            ++failures;
            errors << "Null async task. ";
            continue;
        }
        if (child->IsCompleted()) {
            for (ScriptAsyncTaskBase *other : m_Children) {
                if (other && other != child && !other->IsDone()) {
                    other->Cancel();
                }
            }
            if (m_SubTypeId == asTYPEID_VOID) {
                CompleteVoid();
            } else {
                std::string error;
                if (!CopyResultFrom(child, error)) {
                    Fail(error);
                }
            }
            return;
        }
        if (child->IsFailed() || child->IsCancelled()) {
            ++failures;
            errors << child->Error() << " ";
        }
    }
    if (failures == static_cast<int>(m_Children.size())) {
        Fail(errors.str().empty() ? "Async::Any failed because all tasks failed." : errors.str());
        return;
    }
    SetState(ScriptAsyncTaskState::Running);
}

void ScriptAsyncTaskBase::AdvanceBBTask() {
    if (!m_BBTask || !m_BBTask->IsValid()) {
        Fail("BBTask is not valid.");
        return;
    }
    if (m_DriveBridgeTask && m_BBTask->IsAlive() && !m_BBTask->Step(m_BridgeTaskContext, m_BBTaskInputIndex)) {
        Fail(m_BBTask->Error().empty() ? "BBTask.Step failed while awaiting task." : m_BBTask->Error());
        return;
    }
    const std::string taskError = m_BBTask->Error();
    const int returnCode = m_BBTask->ReturnCode();
    if (!taskError.empty()) {
        Fail(taskError);
        return;
    }
    if ((returnCode & CKBR_ACTIVATENEXTFRAME) != 0) {
        SetState(ScriptAsyncTaskState::Running);
        return;
    }
    CompleteVoid();
}

void ScriptAsyncTaskBase::AdvanceGraphTask() {
    if (!m_GraphTask || !m_GraphTask->IsValid()) {
        Fail("GraphTask is not valid.");
        return;
    }
    if (m_GraphTask->TimedOut()) {
        Fail(m_GraphTask->Error());
        return;
    }
    if (!m_GraphTask->IsAlive() && !m_GraphTask->Done(-1)) {
        Fail(m_GraphTask->Error().empty() ? "GraphTask stopped before completion." : m_GraphTask->Error());
        return;
    }
    if (m_GraphTask->Done(-1)) {
        CompleteVoid();
        return;
    }
    if (m_DriveBridgeTask && !m_GraphTask->Step(m_BridgeTaskContext)) {
        Fail(m_GraphTask->Error().empty() ? "GraphTask.Step failed while awaiting task." : m_GraphTask->Error());
        return;
    }
    SetState(ScriptAsyncTaskState::Running);
}

bool ScriptAsyncTaskBase::CompleteAllArray(std::string &error) {
    if (!m_Engine || m_SubTypeId == asTYPEID_VOID) {
        error = "Async::All array result type is not available.";
        return false;
    }

    asITypeInfo *arrayType = m_Engine->GetTypeInfoById(m_SubTypeId & ~asTYPEID_OBJHANDLE);
    if (!arrayType) {
        error = fmt::format("Async::All result type '{}' is not registered.", ScriptAsyncInternal::TypeName(m_Engine, m_SubTypeId));
        return false;
    }
    CScriptArray *array = CScriptArray::Create(arrayType, static_cast<asUINT>(m_Children.size()));
    if (!array) {
        error = "Async::All failed to create result array.";
        return false;
    }

    const int elementTypeId = array->GetElementTypeId();
    for (asUINT i = 0; i < static_cast<asUINT>(m_Children.size()); ++i) {
        ScriptAsyncTaskBase *child = m_Children[i];
        if (!child || !child->CopyResultTo(array->At(i), elementTypeId, error)) {
            array->Release();
            return false;
        }
    }

    CompleteWithArray(m_SubTypeId, array);
    array->Release();
    return true;
}

std::string ScriptAsyncTaskBase::TaskErrorForAwait() const {
    if (IsCancelled()) {
        return Error();
    }
    if (IsFailed()) {
        return Error();
    }
    return "Async task is not completed.";
}

ScriptAsyncScheduler::ScriptAsyncScheduler(ScriptManager *manager) : m_Manager(manager) {}

ScriptAsyncScheduler::~ScriptAsyncScheduler() {
    Clear();
}

asIScriptEngine *ScriptAsyncScheduler::GetEngine() const {
    return m_Manager ? m_Manager->GetScriptEngine() : nullptr;
}

ScriptAsyncTaskBase *ScriptAsyncScheduler::CreateDelay(int frames) {
    ScriptAsyncTaskBase *task = new ScriptAsyncTaskBase(this, GetEngine(), ScriptAsyncTaskKind::Delay, asTYPEID_VOID);
    task->SetDelayFrames(frames);
    Track(task);
    return task;
}

ScriptAsyncTaskBase *ScriptAsyncScheduler::CreateManualTask(int subtypeId) {
    ScriptAsyncTaskBase *task = new ScriptAsyncTaskBase(this, GetEngine(), ScriptAsyncTaskKind::Manual, subtypeId);
    Track(task);
    return task;
}

ScriptAsyncTaskBase *ScriptAsyncScheduler::CreateScriptTask(asIScriptFunction *function, int subtypeId) {
    ScriptAsyncTaskBase *task = new ScriptAsyncTaskBase(this, GetEngine(), ScriptAsyncTaskKind::Script, subtypeId);
    task->SetFunction(function);
    Track(task);
    return task;
}

ScriptAsyncTaskBase *ScriptAsyncScheduler::CreateAggregate(ScriptAsyncTaskKind kind,
                                                           int subtypeId,
                                                           const std::vector<ScriptAsyncTaskBase *> &children) {
    ScriptAsyncTaskBase *task = new ScriptAsyncTaskBase(this, GetEngine(), kind, subtypeId);
    task->SetChildren(children);
    Track(task);
    return task;
}

ScriptAsyncTaskBase *ScriptAsyncScheduler::CreateBBTask(BBTask *taskHandle, const CKBehaviorContext *context, int inputIndex) {
    ScriptAsyncTaskBase *task = new ScriptAsyncTaskBase(this, GetEngine(), ScriptAsyncTaskKind::BBTask, asTYPEID_VOID);
    task->SetBBTask(taskHandle, context, inputIndex);
    Track(task);
    return task;
}

ScriptAsyncTaskBase *ScriptAsyncScheduler::CreateGraphTask(GraphTask *taskHandle, const CKBehaviorContext *context) {
    ScriptAsyncTaskBase *task = new ScriptAsyncTaskBase(this, GetEngine(), ScriptAsyncTaskKind::GraphTask, asTYPEID_VOID);
    task->SetGraphTask(taskHandle, context);
    Track(task);
    return task;
}

void ScriptAsyncScheduler::Track(ScriptAsyncTaskBase *task) {
    if (!task) {
        return;
    }
    task->AddRef();
    m_Tasks.push_back(task);
}

void ScriptAsyncScheduler::Tick() {
    for (ScriptAsyncTaskBase *task : m_Tasks) {
        if (task) {
            task->Advance();
        }
    }
    RemoveFinishedTrackedTasks();
}

bool ScriptAsyncScheduler::WaitForTask(asIScriptContext *context,
                                       ScriptAsyncTaskBase *task,
                                       void *outAddress,
                                       int outTypeId,
                                       std::string &error) {
    if (!context || !task) {
        error = "Await requires a valid context and task.";
        return false;
    }
    ForgetContext(context);
    WaitRecord record;
    record.Task = task;
    record.OutAddress = outAddress;
    record.OutTypeId = outTypeId;
    record.Task->AddRef();
    m_Waits[context] = record;
    return true;
}

ScriptAsyncScheduler::ResumeState ScriptAsyncScheduler::PrepareContextResume(asIScriptContext *context, std::string &error) {
    auto it = m_Waits.find(context);
    if (it == m_Waits.end()) {
        return ResumeState::Ready;
    }

    WaitRecord &record = it->second;
    ScriptAsyncTaskBase *task = record.Task;
    if (!task) {
        error = "Awaited async task is no longer available.";
        m_Waits.erase(it);
        return ResumeState::Failed;
    }
    task->Advance();
    if (!task->IsDone()) {
        return ResumeState::Pending;
    }
    if (task->IsFailed() || task->IsCancelled()) {
        error = task->Error();
        ReleaseWaitRecord(record);
        m_Waits.erase(it);
        return ResumeState::Failed;
    }
    if (record.OutTypeId != asTYPEID_VOID &&
        !task->CopyResultTo(record.OutAddress, record.OutTypeId, error)) {
        ReleaseWaitRecord(record);
        m_Waits.erase(it);
        return ResumeState::Failed;
    }

    ReleaseWaitRecord(record);
    m_Waits.erase(it);
    return ResumeState::Ready;
}

void ScriptAsyncScheduler::ForgetContext(asIScriptContext *context) {
    auto it = m_Waits.find(context);
    if (it == m_Waits.end()) {
        return;
    }
    ReleaseWaitRecord(it->second);
    m_Waits.erase(it);
}

void ScriptAsyncScheduler::Clear() {
    for (auto &entry : m_Waits) {
        ReleaseWaitRecord(entry.second);
    }
    m_Waits.clear();

    for (ScriptAsyncTaskBase *task : m_Tasks) {
        if (task) {
            task->Cancel();
            task->Release();
        }
    }
    m_Tasks.clear();
}

void ScriptAsyncScheduler::ReleaseWaitRecord(WaitRecord &record) {
    if (record.Task) {
        record.Task->Release();
        record.Task = nullptr;
    }
    record.OutAddress = nullptr;
    record.OutTypeId = asTYPEID_VOID;
}

void ScriptAsyncScheduler::RemoveFinishedTrackedTasks() {
    auto it = m_Tasks.begin();
    while (it != m_Tasks.end()) {
        ScriptAsyncTaskBase *task = *it;
        if (task && task->IsDone()) {
            task->Release();
            it = m_Tasks.erase(it);
        } else {
            ++it;
        }
    }
}

void RegisterScriptAsync(asIScriptEngine *engine) {
    assert(engine != nullptr);
    int r = 0;

    r = engine->RegisterObjectType("AsyncTask<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("AsyncTask<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(ScriptAsyncInternal::ScriptAsyncTemplateCallback), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("AsyncTask<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(ScriptAsyncTaskBase, AddRef), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("AsyncTask<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(ScriptAsyncTaskBase, Release), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("AsyncTask<T>", "bool IsPending() const", asMETHOD(ScriptAsyncTaskBase, IsPending), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("AsyncTask<T>", "bool IsRunning() const", asMETHOD(ScriptAsyncTaskBase, IsRunning), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("AsyncTask<T>", "bool IsCompleted() const", asMETHOD(ScriptAsyncTaskBase, IsCompleted), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("AsyncTask<T>", "bool IsFailed() const", asMETHOD(ScriptAsyncTaskBase, IsFailed), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("AsyncTask<T>", "bool IsCancelled() const", asMETHOD(ScriptAsyncTaskBase, IsCancelled), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("AsyncTask<T>", "bool IsDone() const", asMETHOD(ScriptAsyncTaskBase, IsDone), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("AsyncTask<T>", "string Error() const", asMETHOD(ScriptAsyncTaskBase, Error), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("AsyncTask<T>", "bool Cancel()", asMETHOD(ScriptAsyncTaskBase, Cancel), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterFuncdef("void AsyncVoidFunc()"); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterFuncdef("int AsyncIntFunc()"); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterFuncdef("float AsyncFloatFunc()"); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterFuncdef("string AsyncStringFunc()"); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterFuncdef("CKObject@ AsyncObjectFunc()"); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("void Await(AsyncTask<void>@+ task)", asFUNCTION(ScriptAsyncInternal::AwaitVoid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void Await(AsyncTask<int>@+ task, int &out result)", asFUNCTION(ScriptAsyncInternal::AwaitInt), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void Await(AsyncTask<float>@+ task, float &out result)", asFUNCTION(ScriptAsyncInternal::AwaitFloat), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void Await(AsyncTask<string>@+ task, string &out result)", asFUNCTION(ScriptAsyncInternal::AwaitString), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void Await(AsyncTask<CKObject@>@+ task, CKObject@ &out result)", asFUNCTION(ScriptAsyncInternal::AwaitObject), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void Await(?&in task, ?&out result)", asFUNCTION(ScriptAsyncInternal::AwaitGeneric), asCALL_GENERIC); CKAS_CHECK_REGISTER(r);

    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string previous = previousNamespace ? previousNamespace : "";
    r = engine->SetDefaultNamespace("Async"); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("AsyncTask<void>@ Delay(int frames)", asFUNCTION(ScriptAsyncInternal::AsyncDelay), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<void>@ Spawn(AsyncVoidFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnVoid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<int>@ Spawn(AsyncIntFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnInt), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<float>@ Spawn(AsyncFloatFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnFloat), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<string>@ Spawn(AsyncStringFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnString), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<CKObject@>@ Spawn(AsyncObjectFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnObject), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void Spawn(?&in fn, ?&out task)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnGeneric), asCALL_GENERIC); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<void>@ Create(AsyncVoidFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnVoid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<int>@ Create(AsyncIntFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnInt), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<float>@ Create(AsyncFloatFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnFloat), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<string>@ Create(AsyncStringFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnString), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<CKObject@>@ Create(AsyncObjectFunc@ fn)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnObject), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void Create(?&in fn, ?&out task)", asFUNCTION(ScriptAsyncInternal::AsyncSpawnGeneric), asCALL_GENERIC); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("AsyncTask<void>@ All(array<AsyncTask<void>@>@ tasks)", asFUNCTION(ScriptAsyncInternal::AsyncAllVoid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    // Use the out-parameter aggregate overloads for typed results. The direct typed-return
    // overloads can make AngelScript's overload resolver hang in the Virtools Player host.
    r = engine->RegisterGlobalFunction("void All(?&in tasks, ?&out task)", asFUNCTION(ScriptAsyncInternal::AsyncAllGeneric), asCALL_GENERIC); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<void>@ Race(array<AsyncTask<void>@>@ tasks)", asFUNCTION(ScriptAsyncInternal::AsyncRaceVoid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void Race(?&in tasks, ?&out task)", asFUNCTION(ScriptAsyncInternal::AsyncRaceGeneric), asCALL_GENERIC); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<void>@ Any(array<AsyncTask<void>@>@ tasks)", asFUNCTION(ScriptAsyncInternal::AsyncAnyVoid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void Any(?&in tasks, ?&out task)", asFUNCTION(ScriptAsyncInternal::AsyncAnyGeneric), asCALL_GENERIC); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<void>@ Wait(BBTask@+ task)", asFUNCTION(ScriptAsyncInternal::AsyncWaitBBTask), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<void>@ Wait(GraphTask@+ task)", asFUNCTION(ScriptAsyncInternal::AsyncWaitGraphTask), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<void>@ Wait(const CKBehaviorContext &in ctx, BBTask@+ task, int inputIndex = -1)", asFUNCTION(ScriptAsyncInternal::AsyncWaitBBTaskWithContext), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("AsyncTask<void>@ Wait(const CKBehaviorContext &in ctx, GraphTask@+ task)", asFUNCTION(ScriptAsyncInternal::AsyncWaitGraphTaskWithContext), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->SetDefaultNamespace(previous.c_str()); CKAS_CHECK_REGISTER(r);
}
