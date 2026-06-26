#include "ScriptManager.h"

#include <cassert>
#include <string>
#include <utility>

#include <fmt/format.h>

#ifndef CKAS_ENABLE_DYNCALL
#define CKAS_ENABLE_DYNCALL 0
#endif

#ifndef CKAS_ENABLE_API_EXPORT
#define CKAS_ENABLE_API_EXPORT 0
#endif

#include "Logger.h"
#include "ScriptEngineHost.h"
#include "ScriptAsync.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptCK2.h"
#if CKAS_ENABLE_DYNCALL
#include "ScriptDynCall.h"
#endif
#include "ScriptFormat.h"
#if CKAS_ENABLE_API_EXPORT
#include "ScriptInfo.h"
#endif
#include "ScriptMessage.h"
#include "ScriptNativeBuffer.h"
#include "ScriptNativePointer.h"
#include "ScriptParameterRegistry.h"
#include "ScriptApiSupport.h"
#include "ScriptPublicOptions.h"
#include "ScriptRegistration.h"
#include "ScriptRuntime.h"
#include "ScriptScene.h"
#include "ScriptUtils.h"
#include "ScriptVxMath.h"

#include "add_on/scriptany/scriptany.h"
#include "add_on/scriptarray/scriptarray.h"
#include "add_on/datetime/datetime.h"
#include "add_on/scriptdictionary/scriptdictionary.h"
#include "add_on/scriptfile/scriptfile.h"
#include "add_on/scriptfile/scriptfilesystem.h"
#include "add_on/scriptgrid/scriptgrid.h"
#include "add_on/scripthandle/scripthandle.h"
#include "add_on/scriptmath/scriptmath.h"
#include "add_on/scriptmath/scriptmathcomplex.h"
#include "add_on/scriptstdstring/scriptstdstring.h"
#include "add_on/weakref/weakref.h"
#include "add_on/scripthelper/scripthelper.h"

namespace {

std::string MakeEngineExtensionConfigGroupName(const char *name) {
    return fmt::format("CKAngelScript.Extension.{}", name ? name : "");
}

} // namespace

asIScriptEngine *ScriptEngineHost::Engine() const {
    return m_Engine;
}

bool ScriptEngineHost::HasEngine() const {
    return m_Engine != nullptr;
}

void ScriptEngineHost::ShutdownAndReleaseEngine() {
    if (m_Engine) {
        m_Engine->ShutDownAndRelease();
        m_Engine = nullptr;
    }
}

asIScriptContext *ScriptEngineHost::RequestContext(ScriptManager &manager) {
    asIScriptContext *ctx = nullptr;
    if (!m_ContextPool.empty()) {
        ctx = *m_ContextPool.rbegin();
        m_ContextPool.pop_back();
    } else if (m_Engine) {
        ctx = m_Engine->CreateContext();
    }
    if (!ctx) {
        return nullptr;
    }

    const int state = ctx->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        ctx->Abort();
    }
    ctx->Unprepare();
    ctx->SetExceptionCallback(asMETHOD(ScriptManager, ExceptionCallback), &manager, asCALL_THISCALL);
    return ctx;
}

void ScriptEngineHost::ReturnContext(ScriptManager &, asIScriptContext *context) {
    if (!context) {
        return;
    }

    const int state = context->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        context->Abort();
    }
    context->Unprepare();
    m_ContextPool.push_back(context);
}

void ScriptEngineHost::ReleaseContextPool() {
    for (auto *context : m_ContextPool) {
        context->Release();
    }
    m_ContextPool.clear();
}

void ScriptEngineHost::SetHostCallFilter(CKAngelScriptHostCallFilterCallback callback,
                                         void *userData) {
    m_HostCallFilter = callback;
    m_HostCallFilterUserData = userData;
}

bool ScriptEngineHost::RejectHostCall(const char *apiName, CKDWORD flags) const {
    if (!m_HostCallFilter) {
        return false;
    }
    return m_HostCallFilter(apiName, flags, m_HostCallFilterUserData) != CKAS_OK;
}

CKAS_STATUS ScriptEngineHost::RegisterExtension(ScriptManager &manager,
                                                const CKAngelScriptEngineExtension &extension,
                                                CKAngelScriptResult *result) {
    ScriptPublicOptions::EngineExtensionRequest request;
    std::string errorMessage;
    const CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeEngineExtension(extension,
                                                                                request,
                                                                                errorMessage);
    if (optionStatus != CKAS_OK) {
        return manager.StoreApiResult(result, optionStatus, 0, errorMessage.c_str());
    }
    for (const EngineExtensionRegistration &existing : m_EngineExtensions) {
        if (existing.Name == request.Name) {
            const std::string message = fmt::format("Engine extension '{}' is already registered.", request.Name);
            return manager.StoreApiResult(result, CKAS_ALREADYEXISTS, 0, message.c_str());
        }
    }

    EngineExtensionRegistration retained = {};
    retained.Name = request.Name;
    retained.ConfigGroupName = MakeEngineExtensionConfigGroupName(request.Name);
    retained.Register = request.Register;
    retained.UserData = request.UserData;
    retained.Flags = request.Flags;

    if (m_Engine &&
        manager.IsInited() &&
        !ScriptApiSupport::HasPublicFlag(request.Flags, CKAS_ENGINEEXTENSION_DEFERRED)) {
        std::string message;
        const int code = RegisterExtensionGroup(manager, m_Engine, retained, message);
        if (code < 0) {
            const std::string summary = message.empty()
                                            ? fmt::format("Engine extension '{}' failed to register (code {}).",
                                                          request.Name,
                                                          code)
                                            : message;
            CKContext *context = manager.GetCKContext();
            if (context) {
                context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
            }
            LOG_ERROR("%s", summary.c_str());
            return manager.StoreApiResult(result, CKAS_EXECUTIONFAILED, code, summary.c_str());
        }
    }

    m_EngineExtensions.push_back(retained);
    return manager.StoreApiResult(result, CKAS_OK, 0, nullptr);
}

CKAS_STATUS ScriptEngineHost::UnregisterExtension(ScriptManager &manager,
                                                  const char *name,
                                                  CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(name)) {
        return manager.StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension name is required.");
    }
    for (auto it = m_EngineExtensions.begin(); it != m_EngineExtensions.end(); ++it) {
        if (it->Name == name) {
            if (it->ActiveInCurrentEngine && m_Engine) {
                std::string message;
                const int code = RemoveExtensionGroup(m_Engine, *it, message);
                if (code < 0) {
                    if (code == asCONFIG_GROUP_IS_IN_USE) {
                        const std::string summary = message.empty()
                                                        ? fmt::format("Engine extension '{}' is in use.", name)
                                                        : message;
                        return manager.StoreApiResult(result, CKAS_INUSE, code, summary.c_str());
                    }
                    const std::string summary = message.empty()
                                                    ? fmt::format("Failed to unregister engine extension '{}' (code {}).", name, code)
                                                    : message;
                    return manager.StoreApiResult(result, CKAS_EXECUTIONFAILED, code, summary.c_str());
                }
            }
            m_EngineExtensions.erase(it);
            return manager.StoreApiResult(result, CKAS_OK, 0, nullptr);
        }
    }
    const std::string message = fmt::format("Engine extension '{}' is not registered.", name);
    return manager.StoreApiResult(result, CKAS_NOTFOUND, 0, message.c_str());
}

void ScriptEngineHost::MarkExtensionsInactive() {
    for (EngineExtensionRegistration &extension : m_EngineExtensions) {
        extension.ActiveInCurrentEngine = false;
    }
}

asIScriptEngine *ScriptManager::GetScriptEngine() {
    return m_EngineHost.Engine();
}

void ScriptManager::MessageCallback(const asSMessageInfo &msg) {
    const char *type = "NULL";
    CKAS_MESSAGETYPE publicType = CKAS_MESSAGE_INFORMATION;
    switch (msg.type) {
        case asMSGTYPE_ERROR:
            type = "ERROR";
            publicType = CKAS_MESSAGE_ERROR;
            break;
        case asMSGTYPE_WARNING:
            type = "WARN";
            publicType = CKAS_MESSAGE_WARNING;
            break;
        case asMSGTYPE_INFORMATION:
            type = "INFO";
            publicType = CKAS_MESSAGE_INFORMATION;
            break;
    }
    const std::string formatted = fmt::format("{}({},{}): {}: {}",
        msg.section ? msg.section : "<unknown section>",
        msg.row,
        msg.col,
        type,
        msg.message ? msg.message : "");
    if (m_Diagnostics.IsCapturingScriptMessages()) {
        CapturedScriptMessage captured;
        captured.Section = msg.section ? msg.section : "";
        captured.Row = msg.row;
        captured.Column = msg.col;
        captured.Type = publicType;
        captured.Message = msg.message ? msg.message : "";
        m_Diagnostics.CaptureScriptMessage(formatted, std::move(captured));
    }
    m_Context->OutputToConsoleEx(const_cast<char *>("%s"), formatted.c_str());
}

void ScriptManager::ExceptionCallback(asIScriptContext *context) {
    std::string callStack = GetCallStack(context);
    asIScriptFunction *func = context ? context->GetExceptionFunction() : nullptr;
    const char *funcDecl = func ? func->GetDeclaration() : nullptr;
    const char *exception = context ? context->GetExceptionString() : nullptr;
    std::string message = fmt::format("Exception in '{}': '{}'\n{}",
        funcDecl ? funcDecl : "<unknown function>",
        exception ? exception : "",
        callStack);

    asSMessageInfo info = {};
    if (context) {
        info.row = context->GetExceptionLineNumber(&info.col, &info.section);
    }
    info.type = asMSGTYPE_ERROR;
    info.message = message.c_str();
    MessageCallback(info);
}

std::string ScriptManager::GetCallStack(asIScriptContext *context) {
    std::string str("Callstack:\n");
    if (!context) {
        return str;
    }

    for (asUINT i = 0; i < context->GetCallstackSize(); i++) {
        asIScriptFunction *func = context->GetFunction(i);
        int column;
        const char *section;
        int line = context->GetLineNumber(i, &column, &section);
        const char *funcDecl = func ? func->GetDeclaration() : nullptr;
        str.append(fmt::format("\t{} at {}({},{})\n",
            funcDecl ? funcDecl : "<unknown function>",
            section ? section : "<unknown section>",
            line,
            column));
    }
    return std::move(str);
}

asIScriptContext *ScriptManager::RequestContextFromPool() {
    return m_EngineHost.RequestContext(*this);
}

void ScriptManager::ReturnContextToPool(asIScriptContext *ctx) {
    m_EngineHost.ReturnContext(*this, ctx);
}

int ScriptEngineHost::Setup(ScriptManager &manager, CKContext *context) {
    // #if CKVERSION == 0x13022002
    //     asSetGlobalMemoryFunctions(
    //         [](size_t size) { return VxMalloc(size); },
    //         [](void *ptr) { VxFree(ptr); }
    //     );
    // #endif

    m_Engine = asCreateScriptEngine();
    asIScriptEngine *engine = Engine();
    if (!engine) {
        if (context) {
            context->OutputToConsole(const_cast<char *>("Failed to create script engine."));
        }
        LOG_ERROR("Failed to create script engine.");
        return -1;
    }

    engine->SetUserData(&manager, SCRIPT_MANAGER_TYPE);
    engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
    engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, true);
    engine->SetEngineProperty(asEP_ALLOW_IMPLICIT_HANDLE_TYPES, true);
    engine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);
    engine->SetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE, 1);

    // The script compiler will send any compiler messages to the callback
    int r = engine->SetMessageCallback(asMETHOD(ScriptManager, MessageCallback), &manager, asCALL_THISCALL);
    if (r < 0) {
        LOG_ERROR("SetMessageCallback failed with code %d.", r);
        return r;
    }

    // The script handle the pool of script contexts.
    r = engine->SetContextCallbacks([](asIScriptEngine *, void *param) {
        auto *man = static_cast<ScriptManager *>(param);
        return man->RequestContextFromPool();
    }, [](asIScriptEngine *, asIScriptContext *ctx, void *param) {
        auto *man = static_cast<ScriptManager *>(param);
        man->ReturnContextToPool(ctx);
    }, &manager);
    if (r < 0) {
        LOG_ERROR("SetContextCallbacks failed with code %d.", r);
        return r;
    }

    engine->SetEngineUserDataCleanupCallback([](asIScriptEngine *engine) {
        engine->SetUserData(nullptr, SCRIPT_MANAGER_TYPE);
    }, SCRIPT_MANAGER_TYPE);
    engine->SetFunctionUserDataCleanupCallback([](asIScriptFunction *func) {
        func->SetUserData(nullptr, AS_TEMPORARY_FLAG_TYPE);
    }, AS_TEMPORARY_FLAG_TYPE);
    engine->SetFunctionUserDataCleanupCallback([](asIScriptFunction *func) {
        func->SetUserData(nullptr, AS_RELEASED_ONCE_FLAG_TYPE);
    }, AS_RELEASED_ONCE_FLAG_TYPE);

    ScriptRegistrationContext registration("AngelScript engine registration");
    {
        ScriptRegistrationScope registrationScope(registration);

        // Register the standard types
        RegisterStdTypes(engine);

        // Register the standard add-ons
        RegisterStdAddons(engine);

        // Register the native types
        RegisterNativePointer(engine);
        RegisterNativeBuffer(engine);

#if CKAS_ENABLE_DYNCALL
        // Register the DynCall APIs
        RegisterScriptDynCall(engine);
        RegisterScriptDynCallback(engine);
        RegisterScriptDynLoad(engine);
#endif

        // Register the function that we want the scripts to call
        RegisterScriptFormat(engine);

        // Register the Virtools API
        RegisterVirtools(engine);
    }

    if (registration.HasFailures()) {
        const std::string summary = registration.GetSummary();
        if (context) {
            context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
        }
        LOG_ERROR("%s", summary.c_str());
        ShutdownAndReleaseEngine();
        return -1;
    }

    // Register host-provided namespaces/types after CKAngelScript's own public
    // API is available. A failing extension is logged but is non-fatal: it must
    // not take down core scripting or the other extensions.
    RegisterExtensions(manager, engine);

#if CKAS_ENABLE_API_EXPORT
    std::string apiExportError;
    if (!ExportScriptApiIfRequested(engine, apiExportError)) {
        const std::string summary = "Script API export failed: " + apiExportError;
        if (context) {
            context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
        }
        LOG_ERROR("%s", summary.c_str());
        ShutdownAndReleaseEngine();
        return -1;
    }
#endif

    return r;
}

void ScriptEngineHost::RegisterStdTypes(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    if constexpr (sizeof(void *) == 4) {
        r = engine->RegisterTypedef("size_t", "uint"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("ptrdiff_t", "int"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("intptr_t", "int"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("uintptr_t", "uint"); CKAS_CHECK_REGISTER(r);
    } else {
        r = engine->RegisterTypedef("size_t", "uint64"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("ptrdiff_t", "int64"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("intptr_t", "int64"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("uintptr_t", "uint64"); CKAS_CHECK_REGISTER(r);
    }
}

void ScriptEngineHost::RegisterStdAddons(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterStdString(engine);
    RegisterScriptArray(engine, true);
    RegisterStdStringUtils(engine);
    RegisterScriptAny(engine);
    RegisterScriptHandle(engine);
    RegisterScriptWeakRef(engine);
    RegisterScriptDictionary(engine);
    RegisterScriptDateTime(engine);
    RegisterScriptFile(engine);
    RegisterScriptFileSystem(engine);
    RegisterScriptMath(engine);
    RegisterScriptMathComplex(engine);
    RegisterScriptGrid(engine);
    RegisterExceptionRoutines(engine);
}

void ScriptEngineHost::RegisterVirtools(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterVxMath(engine);
    RegisterCK2(engine);
    RegisterScriptParameterRegistry(engine);
    RegisterScriptBehaviorBridge(engine);
    RegisterScriptSceneCore(engine);
    RegisterScriptRuntime(engine);
    RegisterScriptAsync(engine);
    RegisterScriptMessage(engine);
}

int ScriptEngineHost::RegisterExtensionGroup(ScriptManager &manager,
                                             asIScriptEngine *engine,
                                             EngineExtensionRegistration &extension,
                                             std::string &message) {
    message.clear();
    extension.ActiveInCurrentEngine = false;
    if (!engine || !extension.Register || extension.ConfigGroupName.empty()) {
        message = "Engine extension registration arguments are invalid.";
        return asERROR;
    }

    const char *currentNamespace = engine->GetDefaultNamespace();
    const std::string previousNamespace = currentNamespace ? currentNamespace : "";
    int code = engine->BeginConfigGroup(extension.ConfigGroupName.c_str());
    if (code < 0) {
        message = fmt::format("Engine extension '{}' failed to begin config group '{}' (code {}).",
                              extension.Name,
                              extension.ConfigGroupName,
                              code);
        return code;
    }

    const char *extensionError = nullptr;
    code = extension.Register(engine, ToPublicHandle(&manager), extension.UserData, &extensionError);
    const int namespaceCode = engine->SetDefaultNamespace(previousNamespace.c_str());
    const int endCode = engine->EndConfigGroup();

    int failureCode = 0;
    if (code < 0) {
        failureCode = code;
    } else if (namespaceCode < 0) {
        failureCode = namespaceCode;
    } else if (endCode < 0) {
        failureCode = endCode;
    }

    if (failureCode < 0) {
        const int removeCode = engine->RemoveConfigGroup(extension.ConfigGroupName.c_str());
        const std::string detail =
            extensionError && extensionError[0] != '\0'
                ? fmt::format(": {}", extensionError)
                : std::string();
        message = fmt::format("Engine extension '{}' failed to register (code {}){}.",
                              extension.Name,
                              failureCode,
                              detail);
        if (removeCode < 0) {
            message += fmt::format(" Rollback of config group '{}' also failed (code {}).",
                                   extension.ConfigGroupName,
                                   removeCode);
        }
        return failureCode;
    }

    extension.ActiveInCurrentEngine = true;
    return 0;
}

int ScriptEngineHost::RemoveExtensionGroup(asIScriptEngine *engine,
                                           EngineExtensionRegistration &extension,
                                           std::string &message) {
    message.clear();
    if (!extension.ActiveInCurrentEngine) {
        return 0;
    }
    if (!engine || extension.ConfigGroupName.empty()) {
        message = "Engine extension config group is invalid.";
        return asERROR;
    }

    const int code = engine->RemoveConfigGroup(extension.ConfigGroupName.c_str());
    if (code < 0) {
        message = code == asCONFIG_GROUP_IS_IN_USE
                      ? fmt::format("Engine extension '{}' is still in use by the current AngelScript engine.",
                                    extension.Name)
                      : fmt::format("Failed to remove engine extension '{}' config group '{}' (code {}).",
                                    extension.Name,
                                    extension.ConfigGroupName,
                                    code);
        return code;
    }

    extension.ActiveInCurrentEngine = false;
    return 0;
}

int ScriptEngineHost::RegisterExtensions(ScriptManager &manager, asIScriptEngine *engine) {
    assert(engine != nullptr);

    // A failing host extension must not bring down the whole engine: core
    // CKAngelScript scripting and the remaining extensions stay available. Each
    // failure is reported individually and the first failure code is returned
    // for callers that want to surface it.
    int firstFailure = 0;
    for (EngineExtensionRegistration &extension : m_EngineExtensions) {
        extension.ActiveInCurrentEngine = false;
        if (!extension.Register) {
            continue;
        }
        std::string message;
        const int code = RegisterExtensionGroup(manager, engine, extension, message);
        if (code < 0) {
            const std::string summary = message.empty()
                                            ? fmt::format("Engine extension '{}' failed to register (code {}).",
                                                          extension.Name.empty() ? "<unnamed extension>" : extension.Name.c_str(),
                                                          code)
                                            : message;
            CKContext *context = manager.GetCKContext();
            if (context) {
                context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
            }
            LOG_ERROR("%s", summary.c_str());
            if (firstFailure == 0) {
                firstFailure = code;
            }
        }
    }
    return firstFailure;
}
