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

#include "CKPathManager.h"
#include "Logger.h"
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
asIScriptEngine *ScriptManager::GetScriptEngine() {
    return m_ScriptEngine;
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
    asIScriptContext *ctx = nullptr;
    if (!m_ScriptContexts.empty()) {
        ctx = *m_ScriptContexts.rbegin();
        m_ScriptContexts.pop_back();
    } else
        ctx = m_ScriptEngine->CreateContext();

    const int state = ctx->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        ctx->Abort();
    }
    ctx->Unprepare();
    ctx->SetExceptionCallback(asMETHOD(ScriptManager, ExceptionCallback), this, asCALL_THISCALL);
    return ctx;
}

void ScriptManager::ReturnContextToPool(asIScriptContext *ctx) {
    if (!ctx) {
        return;
    }

    // Unprepare the context to free any objects that might be held
    // as we don't know when the context will be used again.
    const int state = ctx->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        ctx->Abort();
    }
    ctx->Unprepare();
    m_ScriptContexts.push_back(ctx);
}

void ScriptManager::SetupScriptPathCategory() {
    if (m_ScriptPathCategoryIndex == -1) {
        XString category = "Script Paths";
        CKPathManager *pm = m_Context->GetPathManager();
        m_ScriptPathCategoryIndex = pm->GetCategoryIndex(category);
        if (m_ScriptPathCategoryIndex == -1)
            m_ScriptPathCategoryIndex = pm->AddCategory(category);
    }
}

int ScriptManager::SetupScriptEngine() {
    // #if CKVERSION == 0x13022002
    //     asSetGlobalMemoryFunctions(
    //         [](size_t size) { return VxMalloc(size); },
    //         [](void *ptr) { VxFree(ptr); }
    //     );
    // #endif

    m_ScriptEngine = asCreateScriptEngine();
    if (!m_ScriptEngine) {
        m_Context->OutputToConsole(const_cast<char *>("Failed to create script engine."));
        LOG_ERROR("Failed to create script engine.");
        return -1;
    }

    m_ScriptEngine->SetUserData(this, SCRIPT_MANAGER_TYPE);
    m_ScriptEngine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
    m_ScriptEngine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, true);
    m_ScriptEngine->SetEngineProperty(asEP_ALLOW_IMPLICIT_HANDLE_TYPES, true);
    m_ScriptEngine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);
    m_ScriptEngine->SetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE, 1);

    // The script compiler will send any compiler messages to the callback
    int r = m_ScriptEngine->SetMessageCallback(asMETHOD(ScriptManager, MessageCallback), this, asCALL_THISCALL);
    if (r < 0) {
        LOG_ERROR("SetMessageCallback failed with code %d.", r);
        return r;
    }

    // The script handle the pool of script contexts.
    r = m_ScriptEngine->SetContextCallbacks([](asIScriptEngine *, void *param) {
        auto *man = static_cast<ScriptManager *>(param);
        return man->RequestContextFromPool();
    }, [](asIScriptEngine *, asIScriptContext *ctx, void *param) {
        auto *man = static_cast<ScriptManager *>(param);
        man->ReturnContextToPool(ctx);
    }, this);
    if (r < 0) {
        LOG_ERROR("SetContextCallbacks failed with code %d.", r);
        return r;
    }

    m_ScriptEngine->SetEngineUserDataCleanupCallback([](asIScriptEngine *engine) {
        engine->SetUserData(nullptr, SCRIPT_MANAGER_TYPE);
    }, SCRIPT_MANAGER_TYPE);
    m_ScriptEngine->SetFunctionUserDataCleanupCallback([](asIScriptFunction *func) {
        func->SetUserData(nullptr, AS_TEMPORARY_FLAG_TYPE);
    }, AS_TEMPORARY_FLAG_TYPE);
    m_ScriptEngine->SetFunctionUserDataCleanupCallback([](asIScriptFunction *func) {
        func->SetUserData(nullptr, AS_RELEASED_ONCE_FLAG_TYPE);
    }, AS_RELEASED_ONCE_FLAG_TYPE);

    ScriptRegistrationContext registration("AngelScript engine registration");
    {
        ScriptRegistrationScope registrationScope(registration);

        // Register the standard types
        RegisterStdTypes(m_ScriptEngine);

        // Register the standard add-ons
        RegisterStdAddons(m_ScriptEngine);

        // Register the native types
        RegisterNativePointer(m_ScriptEngine);
        RegisterNativeBuffer(m_ScriptEngine);

#if CKAS_ENABLE_DYNCALL
        // Register the DynCall APIs
        RegisterScriptDynCall(m_ScriptEngine);
        RegisterScriptDynCallback(m_ScriptEngine);
        RegisterScriptDynLoad(m_ScriptEngine);
#endif

        // Register the function that we want the scripts to call
        RegisterScriptFormat(m_ScriptEngine);

        // Register the Virtools API
        RegisterVirtools(m_ScriptEngine);
    }

    if (registration.HasFailures()) {
        const std::string summary = registration.GetSummary();
        m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
        LOG_ERROR("%s", summary.c_str());
        m_ScriptEngine->ShutDownAndRelease();
        m_ScriptEngine = nullptr;
        return -1;
    }

    // Register host-provided namespaces/types after CKAngelScript's own public
    // API is available. A failing extension is logged but is non-fatal: it must
    // not take down core scripting or the other extensions.
    RegisterEngineExtensions(m_ScriptEngine);

#if CKAS_ENABLE_API_EXPORT
    std::string apiExportError;
    if (!ExportScriptApiIfRequested(m_ScriptEngine, apiExportError)) {
        const std::string summary = "Script API export failed: " + apiExportError;
        m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
        LOG_ERROR("%s", summary.c_str());
        m_ScriptEngine->ShutDownAndRelease();
        m_ScriptEngine = nullptr;
        return -1;
    }
#endif

    return r;
}

void ScriptManager::RegisterStdTypes(asIScriptEngine *engine) {
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

void ScriptManager::RegisterStdAddons(asIScriptEngine *engine) {
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

void ScriptManager::RegisterVirtools(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterVxMath(m_ScriptEngine);
    RegisterCK2(m_ScriptEngine);
    RegisterScriptParameterRegistry(m_ScriptEngine);
    RegisterScriptBehaviorBridge(m_ScriptEngine);
    RegisterScriptSceneCore(m_ScriptEngine);
    RegisterScriptRuntime(m_ScriptEngine);
    RegisterScriptAsync(m_ScriptEngine);
    RegisterScriptMessage(m_ScriptEngine);
}

int ScriptManager::RegisterEngineExtensionGroup(asIScriptEngine *engine,
                                                ScriptEngineExtensionRegistration &extension,
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
    code = extension.Register(engine, ToPublicHandle(this), extension.UserData, &extensionError);
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

int ScriptManager::RemoveEngineExtensionGroup(asIScriptEngine *engine,
                                              ScriptEngineExtensionRegistration &extension,
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

int ScriptManager::RegisterEngineExtensions(asIScriptEngine *engine) {
    assert(engine != nullptr);

    // A failing host extension must not bring down the whole engine: core
    // CKAngelScript scripting and the remaining extensions stay available. Each
    // failure is reported individually and the first failure code is returned
    // for callers that want to surface it.
    int firstFailure = 0;
    for (ScriptEngineExtensionRegistration &extension : m_EngineExtensions) {
        extension.ActiveInCurrentEngine = false;
        if (!extension.Register) {
            continue;
        }
        std::string message;
        const int code = RegisterEngineExtensionGroup(engine, extension, message);
        if (code < 0) {
            const std::string summary = message.empty()
                                            ? fmt::format("Engine extension '{}' failed to register (code {}).",
                                                          extension.Name.empty() ? "<unnamed extension>" : extension.Name.c_str(),
                                                          code)
                                            : message;
            if (m_Context) {
                m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
            }
            LOG_ERROR("%s", summary.c_str());
            if (firstFailure == 0) {
                firstFailure = code;
            }
        }
    }
    return firstFailure;
}
