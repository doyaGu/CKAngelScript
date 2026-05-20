#include "ScriptBridgeHandles.h"

#include <fstream>

static std::string EscapeAngelScriptString(const std::string &value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

static void WriteBehaviorBridgeSelfTestMarker(const std::string &status,
                                              const std::string &operationName,
                                              const std::string &typeName,
                                              const std::string &message) {
    const char *path = std::getenv("CKAS_SELFTEST_MARKER");
    if (!path || path[0] == '\0') {
        return;
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        return;
    }

    out << "status=" << status << "\n";
    out << "operation=" << operationName << "\n";
    out << "type=" << typeName << "\n";
    if (!message.empty()) {
        out << "message=" << message << "\n";
    }
}

static bool RunBehaviorBridgeScriptSelfTest(CKContext *context,
                                            CKParameterManager *parameterManager,
                                            asIScriptEngine *engine,
                                            const std::string &operationName,
                                            const std::string &typeName,
                                            std::string &error) {
    constexpr const char *moduleName = "__CKAS_BehaviorBridgeSelfTest";

    if (!engine) {
        error = "AngelScript engine is not available.";
        return false;
    }

    std::string source;
    source += "void ProbeGraphTaskApi(const CKBehaviorContext &in ctx, BehaviorRef@ ref, GraphTask@ task) {\n";
    source += "    if (ref !is null) { BehaviorLayout@ l = ref.Layout(); int p = l.FindPin(\"Value\"); ParamRef@ pin = p >= 0 ? ref.Pin(p) : null; GraphTask@ s = ref.Start(0); GraphTask@ w = ref.Watch(); }\n";
    source += "    GraphTask@ ns = Behavior::Start(ctx, \"__missing__\", 0);\n";
    source += "    GraphTask@ nw = Behavior::Watch(ctx, \"__missing__\");\n";
    source += "    if (task is null) return;\n";
    source += "    task.IsValid(); task.IsAlive(); task.IsPaused(); task.TimedOut(); task.Error(); task.Elapsed();\n";
    source += "    task.Timeout(1.0f); task.Step(ctx); task.Done(); task.Done(0); task.OutputActive(); task.OutputActive(0); task.Cancel(); task.Reset();\n";
    source += "    BehaviorRef@ b = task.Behavior(); task.Raise(ctx);\n";
    source += "    ParamRef@ outp = task.Pout(0); if (outp !is null) { outp.GetText(); ParamValue@ v = outp.Get(); NativeBuffer@ raw = outp.GetRaw(); }\n";
    source += "}\n";
    source += "int Run(const CKBehaviorContext &in ctx) {\n";
    source += "    const string typeName = \"" + EscapeAngelScriptString(typeName) + "\";\n";
    source += "    const string operationName = \"" + EscapeAngelScriptString(operationName) + "\";\n";
    source += "    XObjectArray ids;\n";
    source += "    CK_ID first = 0;\n";
    source += "    CK_ID second = 42;\n";
    source += "    ids.PushBack(first);\n";
    source += "    ids.PushBack(second);\n";
    source += "    if (ids.Size() != 2 || ids[0] != first || ids[1] != second) return 10;\n";
    source += "    if (Param::Describe(ctx, typeName) == \"\") return 20;\n";
    source += "    CKGUID typeGuid = Param::Guid(ctx, typeName);\n";
    source += "    if (!typeGuid.IsValid()) return 30;\n";
    source += "    ParamOp@ byName = Param::Operation(ctx, operationName);\n";
    source += "    if (byName is null) return 40;\n";
    source += "    ParamValue@ objectArrayValue = Param::ObjectArray(ids);\n";
    source += "    @byName = byName.Result(typeName).In(0, objectArrayValue).In(1, objectArrayValue);\n";
    source += "    if (byName.Describe() == \"\") return 50;\n";
    source += "    ParamValue@ textValue = Param::Text(ctx, typeName, \"0\");\n";
    source += "    if (textValue is null || !textValue.IsValid()) return 55;\n";
    source += "    ParamOp@ byNameCtxLast = Param::Operation(ctx, operationName);\n";
    source += "    if (byNameCtxLast is null) return 60;\n";
    source += "    return 0;\n";
    source += "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "Failed to create AngelScript self-test module.";
        return false;
    }

    int r = module->AddScriptSection("behavior_bridge_selftest.as", source.c_str(), static_cast<unsigned int>(source.size()));
    if (r < 0) {
        error = fmt::format("Failed to add AngelScript self-test section ({}).", r);
        engine->DiscardModule(moduleName);
        return false;
    }

    r = module->Build();
    if (r < 0) {
        error = fmt::format("Failed to build AngelScript self-test module ({}).", r);
        engine->DiscardModule(moduleName);
        return false;
    }

    asIScriptFunction *function = module->GetFunctionByDecl("int Run(const CKBehaviorContext &in ctx)");
    if (!function) {
        error = "Failed to find AngelScript self-test Run() function.";
        engine->DiscardModule(moduleName);
        return false;
    }

    CKBehaviorContext behaviorContext;
    behaviorContext.Behavior = nullptr;
    behaviorContext.DeltaTime = 0.0f;
    behaviorContext.Context = context;
    behaviorContext.CurrentLevel = nullptr;
    behaviorContext.CurrentScene = nullptr;
    behaviorContext.PreviousScene = nullptr;
    behaviorContext.CurrentRenderContext = nullptr;
    behaviorContext.ParameterManager = parameterManager;
    behaviorContext.MessageManager = nullptr;
    behaviorContext.AttributeManager = nullptr;
    behaviorContext.TimeManager = nullptr;
    behaviorContext.CallbackMessage = 0;
    behaviorContext.CallbackArg = nullptr;

    asIScriptContext *scriptContext = engine->CreateContext();
    if (!scriptContext) {
        error = "Failed to create AngelScript execution context.";
        engine->DiscardModule(moduleName);
        return false;
    }

    r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, &behaviorContext);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        if (returnCode == 0) {
            ok = true;
        } else {
            error = fmt::format("AngelScript self-test returned {}.", returnCode);
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = fmt::format("AngelScript self-test exception: {}.",
                            exception && exception[0] ? exception : "<empty>");
    } else {
        error = fmt::format("AngelScript self-test execution failed ({}).", r);
    }

    scriptContext->Release();
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunScriptBehaviorBridgeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context) {
        error = "CKContext is not available.";
        return false;
    }

    CKParameterManager *parameterManager = context->GetParameterManager();
    if (!parameterManager) {
        error = "CKParameterManager is not available.";
        return false;
    }

    std::string operationName;
    for (int i = 0; i < parameterManager->GetParameterOperationCount(); ++i) {
        const CKGUID guid = parameterManager->OperationCodeToGuid(i);
        if (!guid.IsValid()) {
            continue;
        }

        CKSTRING name = parameterManager->OperationGuidToName(guid);
        if (!name || name[0] == '\0') {
            continue;
        }

        std::string resolveError;
        const CKGUID byName = ResolveOperationGuid(context, CKGUID(), name, resolveError);
        if (byName != guid) {
            error = fmt::format("Parameter operation lookup failed for '{}'.", name);
            return false;
        }
        operationName = name;
        break;
    }
    if (operationName.empty()) {
        error = "No registered parameter operation was found.";
        return false;
    }

    XObjectArray objects;
    objects.PushBack(0);
    objects.PushBack(42);
    const ScriptBridgeValue value = MakeObjectArrayValue(objects);
    if (value.Kind != ScriptBridgeValueKind::ObjectArray ||
        value.ObjectIds.size() != 2 ||
        value.ObjectIds[0] != 0 ||
        value.ObjectIds[1] != 42 ||
        ScriptParameterGuidForValueKind(ScriptBridgeValueKind::ObjectArray) != CKPGUID_OBJECTARRAY) {
        error = "XObjectArray conversion self-test failed.";
        return false;
    }

    std::string typeName;
    for (int i = 0; i < parameterManager->GetParameterTypesCount(); ++i) {
        CKSTRING name = parameterManager->ParameterTypeToName(i);
        if (name && name[0] != '\0') {
            typeName = name;
            break;
        }
    }
    if (typeName.empty()) {
        error = "No registered parameter type was found.";
        return false;
    }

    if (!RunBehaviorBridgeScriptSelfTest(context, parameterManager, engine, operationName, typeName, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, typeName, error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("ok", operationName, typeName, std::string());

    error.clear();
    return true;
}
