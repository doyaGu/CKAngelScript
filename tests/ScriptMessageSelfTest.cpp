#include "ScriptSelfTests.h"

#include <cstdint>
#include <string>

#include "CKAll.h"
#include "CKAngelScript.h"
#include "ScriptComponentState.h"
#include "ScriptComponentStateStore.h"
#include "ScriptHostScope.h"
#include "ScriptMessage.h"
#include "ScriptManager.h"
#include "ScriptRuntime.h"
#include "add_on/scriptdictionary/scriptdictionary.h"

namespace ScriptMessageSelfTest {

bool RunScriptMessageSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Message self-test requires a CKContext and AngelScript engine.";
        return false;
    }

    ScriptMessage message;
    if (message.Id() != 0 || message.RequiresReply()) {
        error = "Default ScriptMessage has invalid state.";
        return false;
    }

    ScriptMessageBus bus(nullptr);
    std::string busError;
    bus.ResetPerfStats();
    if (bus.Send("runtime:source", "runtime:missing", "topic.direct", nullptr, busError)) {
        error = "Message performance self-test expected direct send without manager to fail.";
        return false;
    }
    ScriptMessageBusPerfStats stats = bus.PerfStats();
    if (stats.TargetParses != 1 || stats.PendingRequestFullScans != 0) {
        error = "Message performance self-test detected unexpected direct target parsing or pending scans.";
        return false;
    }
    busError.clear();
    if (!bus.Subscribe("runtime:subscriber", "topic.broadcast", true, busError)) {
        error = "Message performance self-test failed to subscribe broadcast target: " + busError;
        return false;
    }
    bus.ResetPerfStats();
    if (!bus.Publish("runtime:source", "topic.broadcast", nullptr, "", busError)) {
        error = "Message performance self-test failed to publish broadcast: " + busError;
        return false;
    }
    bus.Tick();
    stats = bus.PerfStats();
    if (stats.BroadcastSnapshotBuilds != 1 || stats.PendingRequestFullScans != 0) {
        error = "Message performance self-test expected one broadcast snapshot build and no pending scans.";
        return false;
    }
    bus.ResetPerfStats();
    busError.clear();
    if (!bus.Publish("runtime:source", "topic.broadcast", nullptr, "", busError)) {
        error = "Message performance self-test failed to publish cached broadcast: " + busError;
        return false;
    }
    bus.Tick();
    stats = bus.PerfStats();
    if (stats.BroadcastSnapshotBuilds != 0 || stats.PendingRequestFullScans != 0) {
        error = "Message performance self-test rebuilt an unchanged broadcast snapshot or scanned pending requests.";
        return false;
    }

    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string restoreNamespace = previousNamespace ? previousNamespace : "";
    if (engine->SetDefaultNamespace("Message") < 0) {
        error = "Message self-test could not enter Message namespace.";
        return false;
    }

    const char *decls[] = {
        "ScriptMessageTarget Runtime(const string&in)",
        "ScriptMessageTarget Component(const ScriptContext&in, CK_ID)",
        "ScriptMessageTarget Component(const CKBehaviorContext&in, CK_ID)",
        "bool Publish(const ScriptContext&in, const string&in, dictionary@)",
        "bool Publish(const CKBehaviorContext&in, const string&in, dictionary@)",
        "bool Publish(const ScriptContext&in, const ScriptMessageTarget&in, const string&in, dictionary@)",
        "bool Publish(const CKBehaviorContext&in, const ScriptMessageTarget&in, const string&in, dictionary@)",
        "bool Publish(const ScriptContext&in, const string&in, dictionary@, const string&in)",
        "bool Publish(const CKBehaviorContext&in, const string&in, dictionary@, const string&in)",
        "bool Send(const ScriptContext&in, const ScriptMessageTarget&in, const string&in, dictionary@)",
        "bool Send(const CKBehaviorContext&in, const ScriptMessageTarget&in, const string&in, dictionary@)",
        "bool Send(const ScriptContext&in, const string&in, const string&in, dictionary@)",
        "bool Send(const CKBehaviorContext&in, const string&in, const string&in, dictionary@)",
        "AsyncTask<dictionary@>@ Request(const ScriptContext&in, const ScriptMessageTarget&in, const string&in, dictionary@, int)",
        "AsyncTask<dictionary@>@ Request(const CKBehaviorContext&in, const ScriptMessageTarget&in, const string&in, dictionary@, int)",
        "AsyncTask<dictionary@>@ Request(const ScriptContext&in, const string&in, const string&in, dictionary@, int)",
        "AsyncTask<dictionary@>@ Request(const CKBehaviorContext&in, const string&in, const string&in, dictionary@, int)",
        "bool Subscribe(const ScriptContext&in, const string&in)",
        "bool Subscribe(const CKBehaviorContext&in, const string&in)",
        "bool Unsubscribe(const ScriptContext&in, const string&in)",
        "bool Unsubscribe(const CKBehaviorContext&in, const string&in)"
    };
    for (const char *decl : decls) {
        if (!engine->GetGlobalFunctionByDecl(decl)) {
            engine->SetDefaultNamespace(restoreNamespace.c_str());
            error = std::string("Message self-test could not resolve declaration: ") + decl;
            return false;
        }
    }
    engine->SetDefaultNamespace(restoreNamespace.c_str());

    asITypeInfo *messageType = engine->GetTypeInfoByDecl("ScriptMessage");
    if (!messageType ||
        !messageType->GetMethodByDecl("ScriptMessageTarget Target() const") ||
        messageType->GetMethodByDecl("string Target() const") ||
        messageType->GetMethodByDecl("ScriptMessageTarget TypedTarget() const")) {
        error = "Message self-test detected an unexpected ScriptMessage.Target surface.";
        return false;
    }

    ScriptManager *manager = ScriptManager::GetManager(context);
    if (!manager) {
        error = "Message self-test could not retrieve ScriptManager.";
        return false;
    }

    const char *moduleName = "__CKAS_MessageSelfTest";
    const char *source =
        "int __ckas_message_probe(const ScriptMessage &in msg) {\n"
        "  if (msg.Id() != 123) return 1;\n"
        "  if (msg.Kind() != \"direct\") return 2;\n"
        "  if (msg.Topic() != \"topic.exec\" || msg.Source() != \"runtime:source\") return 3;\n"
        "  ScriptMessageTarget target = msg.Target();\n"
        "  if (!target.IsValid() || target.Kind() != \"runtime\" || target.ToString() != \"runtime:target\" || msg.FrameIndex() != 456 || msg.RequiresReply()) return 4;\n"
        "  dictionary@ payload = msg.Payload();\n"
        "  if (payload is null) return 5;\n"
        "  string text;\n"
        "  if (!payload.get(\"text\", text) || text != \"hello\") return 6;\n"
        "  ScriptMessage copied(msg);\n"
        "  ScriptMessage assigned;\n"
        "  assigned = copied;\n"
        "  dictionary@ copiedPayload = copied.Payload();\n"
        "  dictionary@ assignedPayload = assigned.Payload();\n"
        "  if (copiedPayload is null || assignedPayload is null) return 7;\n"
        "  string copiedText;\n"
        "  string assignedText;\n"
        "  if (!copiedPayload.get(\"text\", copiedText) || copiedText != \"hello\") return 8;\n"
        "  if (!assignedPayload.get(\"text\", assignedText) || assignedText != \"hello\") return 9;\n"
        "  return 0;\n"
        "}\n"
        "class __CKAS_MessageReceiver {\n"
        "  int DirectCount = 0;\n"
        "  void OnMessage(const ScriptMessage &in msg, const CKBehaviorContext &in ctx) {\n"
        "    if (msg.Kind() == \"direct\") { ++DirectCount; return; }\n"
        "    if (msg.Topic() == \"compat.reject\") Message::Reject(ctx, msg, \"rejected\");\n"
        "    else Message::Reply(ctx, msg, null);\n"
        "  }\n"
        "}\n"
        "int __ckas_message_string_runtime(const ScriptContext &in ctx, const string &in target) {\n"
        "  if (!Message::Send(ctx, target, \"compat.direct\", null)) return 31;\n"
        "  AsyncTask<dictionary@>@ task = Message::Request(ctx, target, \"compat.reply\", null, 1);\n"
        "  if (task is null || !task.IsCompleted()) return 32;\n"
        "  @task = Message::Request(ctx, target, \"compat.reject\", null, 1);\n"
        "  if (task is null || !task.IsFailed() || task.Error() != \"rejected\") return 35;\n"
        "  return 0;\n"
        "}\n"
        "int __ckas_message_string_behavior(const CKBehaviorContext &in ctx, const string &in target) {\n"
        "  if (!Message::Send(ctx, target, \"compat.direct\", null)) return 33;\n"
        "  AsyncTask<dictionary@>@ task = Message::Request(ctx, target, \"compat.reply\", null, 1);\n"
        "  if (task is null || !task.IsCompleted()) return 34;\n"
        "  return 0;\n"
        "}\n"
        "int __ckas_message_null_runtime(const ScriptContext &in ctx, const string &in target) {\n"
        "  if (Message::Publish(ctx, \"compat.null.runtime.publish\", null, target)) return 41;\n"
        "  if (Message::Send(ctx, target, \"compat.null.runtime.send\", null)) return 42;\n"
        "  AsyncTask<dictionary@>@ task = Message::Request(ctx, target, \"compat.null.runtime.request\", null, 1);\n"
        "  if (task !is null) return 43;\n"
        "  if (Message::Subscribe(ctx, \"compat.null.runtime.subscription\")) return 44;\n"
        "  if (Message::Unsubscribe(ctx, \"compat.null.runtime.subscription\")) return 45;\n"
        "  return 0;\n"
        "}\n"
        "int __ckas_message_null_behavior(const CKBehaviorContext &in ctx, const string &in target) {\n"
        "  if (Message::Publish(ctx, \"compat.null.behavior.publish\", null, target)) return 46;\n"
        "  if (Message::Send(ctx, target, \"compat.null.behavior.send\", null)) return 47;\n"
        "  AsyncTask<dictionary@>@ task = Message::Request(ctx, target, \"compat.null.behavior.request\", null, 1);\n"
        "  if (task !is null) return 48;\n"
        "  if (Message::Subscribe(ctx, \"compat.null.behavior.subscription\")) return 49;\n"
        "  if (Message::Unsubscribe(ctx, \"compat.null.behavior.subscription\")) return 50;\n"
        "  return 0;\n"
        "}\n";

    CKAngelScriptResult compileResult = {};
    if (manager->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &compileResult) != CKAS_OK) {
        error = compileResult.ErrorMessage && compileResult.ErrorMessage[0] != '\0'
            ? compileResult.ErrorMessage
            : "Message self-test script compile failed.";
        return false;
    }
    asIScriptModule *module = manager->GetModule(moduleName);
    asIScriptFunction *function = module ? module->GetFunctionByDecl("int __ckas_message_probe(const ScriptMessage &in msg)") : nullptr;
    if (!function) {
        manager->UnloadModule(moduleName, nullptr);
        error = "Message self-test script function was not found.";
        return false;
    }

    CScriptDictionary *payload = CScriptDictionary::Create(engine);
    if (!payload) {
        manager->UnloadModule(moduleName, nullptr);
        error = "Message self-test could not create payload dictionary.";
        return false;
    }
    const int stringType = engine->GetTypeIdByDecl("string");
    std::string text = "hello";
    payload->Set("text", &text, stringType);

    ScriptMessageTarget target;
    target.Kind = ScriptMessageTargetKind::Runtime;
    target.Text = "runtime:target";
    target.RuntimeId = "target";
    ScriptMessage executableMessage(123,
                                    ScriptMessageKind::Direct,
                                    "topic.exec",
                                    "runtime:source",
                                    target,
                                    456,
                                    false,
                                    payload);
    payload->Release();

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        manager->UnloadModule(moduleName, nullptr);
        error = "Message self-test could not create an execution context.";
        return false;
    }
    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, &executableMessage);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }
    bool executeOk = false;
    if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        if (returnCode == 0) {
            executeOk = true;
        } else {
            error = "Message self-test script returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string("Message self-test script exception: ") +
                (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = "Message self-test script execution failed with code " + std::to_string(r) + ".";
    }
    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    if (!executeOk) {
        manager->UnloadModule(moduleName, nullptr);
        return false;
    }

    auto executeCompatibilityProbe = [&](const char *declaration,
                                         void *argument,
                                         const std::string &targetText,
                                         const char *label) {
        asIScriptFunction *compatibilityFunction = module ? module->GetFunctionByDecl(declaration) : nullptr;
        if (!compatibilityFunction) {
            error = std::string("Message self-test script function was not found: ") + declaration;
            return false;
        }
        asIScriptContext *compatibilityContext = engine->RequestContext();
        if (!compatibilityContext) {
            error = std::string("Message self-test could not create the ") + label + " execution context.";
            return false;
        }
        int compatibilityResult = compatibilityContext->Prepare(compatibilityFunction);
        if (compatibilityResult >= 0) {
            compatibilityResult = compatibilityContext->SetArgObject(0, argument);
        }
        if (compatibilityResult >= 0) {
            compatibilityResult = compatibilityContext->SetArgObject(1, const_cast<std::string *>(&targetText));
        }
        if (compatibilityResult >= 0) {
            compatibilityResult = compatibilityContext->Execute();
        }
        bool compatibilityOk = false;
        if (compatibilityResult == asEXECUTION_FINISHED) {
            const int returnCode = static_cast<int>(compatibilityContext->GetReturnDWord());
            compatibilityOk = returnCode == 0;
            if (!compatibilityOk) {
                error = std::string("Message ") + label + " compatibility probe returned " + std::to_string(returnCode) + ".";
            }
        } else if (compatibilityResult == asEXECUTION_EXCEPTION) {
            const char *exception = compatibilityContext->GetExceptionString();
            error = std::string("Message ") + label + " compatibility probe exception: " +
                    (exception && exception[0] ? exception : "<empty>") + ".";
        } else {
            error = std::string("Message ") + label + " compatibility probe failed with code " +
                    std::to_string(compatibilityResult) + ".";
        }
        compatibilityContext->Unprepare();
        engine->ReturnContext(compatibilityContext);
        return compatibilityOk;
    };

    CKBehavior *compatibilityReceiver = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_MessageCompatibilityReceiver"),
        CK_OBJECTCREATION_DYNAMIC));
    ScriptComponentState *receiverState = compatibilityReceiver
        ? manager->GetOrCreateComponentState(compatibilityReceiver)
        : nullptr;
    asITypeInfo *receiverType = module ? module->GetTypeInfoByName("__CKAS_MessageReceiver") : nullptr;
    if (!receiverState || !receiverType) {
        if (receiverState) manager->ReleaseComponentState(compatibilityReceiver);
        if (compatibilityReceiver) context->DestroyObject(compatibilityReceiver);
        manager->UnloadModule(moduleName, nullptr);
        error = "Message compatibility self-test could not create a Component receiver.";
        return false;
    }
    receiverState->Invoker = new ScriptInvoker(manager);
    receiverState->Object = receiverState->Invoker->SetScript(moduleName)
        ? receiverState->Invoker->CreateScriptObject(receiverType)
        : nullptr;
    receiverState->OnMessage = receiverType->GetMethodByDecl(
        "void OnMessage(const ScriptMessage &in msg, const CKBehaviorContext &in ctx)");
    if (receiverState->OnMessage) receiverState->OnMessage->AddRef();
    receiverState->Loaded = receiverState->Object && receiverState->OnMessage;
    receiverState->InstanceEnabled = receiverState->Loaded;
    const std::string compatibilityTarget = ScriptMessageBus::ComponentTarget(compatibilityReceiver->GetID());

    ScriptContext runtimeContext(context, nullptr, "SelfTest", "Running", 1, 0, 0.0f, 0.0f);
    CKBehaviorContext compatibilityBehaviorContext = {};
    compatibilityBehaviorContext.Context = context;
    compatibilityBehaviorContext.Behavior = compatibilityReceiver;
    CKBehaviorContext behaviorOnlyContext = compatibilityBehaviorContext;
    behaviorOnlyContext.Context = nullptr;
    ScriptContext nullRuntimeContext(nullptr, nullptr, "SelfTest", "Running", 1, 0, 0.0f, 0.0f);

    const ScriptHostScope directBehaviorScope = ScriptHostScope::From(
        compatibilityBehaviorContext,
        ScriptHostScopeContextResolution::DirectContext);
    const ScriptHostScope fallbackBehaviorScope = ScriptHostScope::From(behaviorOnlyContext);
    const ScriptHostScope directBehaviorOnlyScope = ScriptHostScope::From(
        behaviorOnlyContext,
        ScriptHostScopeContextResolution::DirectContext);
    if (directBehaviorScope.Context() != context ||
        directBehaviorScope.Manager() != manager ||
        directBehaviorScope.MessageBus() != manager->GetMessageBus() ||
        directBehaviorScope.MessageSource() != compatibilityTarget ||
        fallbackBehaviorScope.Context() != context ||
        fallbackBehaviorScope.Manager() != manager ||
        directBehaviorOnlyScope.Context() != nullptr ||
        directBehaviorOnlyScope.Manager() != nullptr ||
        directBehaviorOnlyScope.MessageBus() != nullptr ||
        directBehaviorOnlyScope.MessageSource() != compatibilityTarget) {
        manager->ReleaseComponentState(compatibilityReceiver);
        context->DestroyObject(compatibilityReceiver);
        manager->UnloadModule(moduleName, nullptr);
        error = "Message Script Host Scope compatibility policy changed context or source resolution.";
        return false;
    }

    if (!receiverState->Loaded ||
        !executeCompatibilityProbe("int __ckas_message_string_runtime(const ScriptContext &in ctx, const string &in target)",
                                   &runtimeContext,
                                   compatibilityTarget,
                                   "runtime string target") ||
        !executeCompatibilityProbe("int __ckas_message_string_behavior(const CKBehaviorContext &in ctx, const string &in target)",
                                   &compatibilityBehaviorContext,
                                   compatibilityTarget,
                                   "behavior string target") ||
        !executeCompatibilityProbe("int __ckas_message_null_runtime(const ScriptContext &in ctx, const string &in target)",
                                   &nullRuntimeContext,
                                   compatibilityTarget,
                                   "null runtime context") ||
        !executeCompatibilityProbe("int __ckas_message_null_behavior(const CKBehaviorContext &in ctx, const string &in target)",
                                   &behaviorOnlyContext,
                                   compatibilityTarget,
                                   "behavior-only context")) {
        manager->ReleaseComponentState(compatibilityReceiver);
        context->DestroyObject(compatibilityReceiver);
        manager->UnloadModule(moduleName, nullptr);
        return false;
    }
    manager->ReleaseComponentState(compatibilityReceiver);
    context->DestroyObject(compatibilityReceiver);
    manager->UnloadModule(moduleName, nullptr);

    CKBeObject *waitTarget = CKBeObject::Cast(context->CreateObject(
        CKCID_BEOBJECT,
        const_cast<CKSTRING>("__CKAS_MessageWaitSelfTestTarget"),
        CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *waitBehavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_MessageWaitSelfTestBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    if (!waitTarget || !waitBehavior || !waitBehavior->CreateOutput(const_cast<CKSTRING>("Out")) ||
        waitBehavior->SetOwner(waitTarget, FALSE) != CK_OK) {
        if (waitBehavior && !waitBehavior->IsToBeDeleted()) {
            context->DestroyObject(waitBehavior);
        }
        if (waitTarget && !waitTarget->IsToBeDeleted()) {
            context->DestroyObject(waitTarget);
        }
        error = "Message self-test could not create wait behavior output.";
        return false;
    }

    ScriptComponentStateStore identityStore;
    int identityRemovals = 0;
    CKBehavior *foreignBehavior = reinterpret_cast<CKBehavior *>(static_cast<std::uintptr_t>(1));
    ScriptComponentState *identityState = identityStore.GetOrCreate(
        waitBehavior,
        "component:identity-self-test",
        {});
    if (!identityState ||
        identityStore.Get(waitBehavior->GetID(), foreignBehavior) != nullptr ||
        identityStore.Remove(waitBehavior->GetID(), foreignBehavior, [&identityRemovals](CK_ID, ScriptComponentState *) {
            ++identityRemovals;
        }) ||
        identityStore.Get(waitBehavior->GetID(), waitBehavior) != identityState ||
        identityRemovals != 0 ||
        !identityStore.Remove(waitBehavior->GetID(), waitBehavior, [&identityRemovals](CK_ID, ScriptComponentState *) {
            ++identityRemovals;
        }) ||
        identityRemovals != 1 ||
        identityStore.Get(waitBehavior->GetID()) != nullptr) {
        context->DestroyObject(waitBehavior);
        context->DestroyObject(waitTarget);
        error = "Component state store did not reject a mismatched behavior identity.";
        return false;
    }

    ScriptComponentState *firstComponentState = manager->GetOrCreateComponentState(waitBehavior);
    if (!firstComponentState || firstComponentState->MessageGeneration == 0) {
        context->DestroyObject(waitBehavior);
        context->DestroyObject(waitTarget);
        error = "Message self-test could not create a generated Component target.";
        return false;
    }
    const std::uint64_t staleGeneration = firstComponentState->MessageGeneration;
    manager->ReleaseComponentState(waitBehavior);
    ScriptComponentState *replacementComponentState = manager->GetOrCreateComponentState(waitBehavior);
    if (!replacementComponentState || replacementComponentState->MessageGeneration == staleGeneration) {
        manager->ReleaseComponentState(waitBehavior);
        context->DestroyObject(waitBehavior);
        context->DestroyObject(waitTarget);
        error = "Message self-test reused a Component message generation.";
        return false;
    }
    ScriptMessage staleComponentMessage;
    std::string staleTargetError;
    if (manager->DeliverComponentMessage(waitBehavior->GetID(),
                                         staleGeneration,
                                         staleComponentMessage,
                                         true,
                                         staleTargetError) ||
        staleTargetError != "Component message target is stale.") {
        manager->ReleaseComponentState(waitBehavior);
        context->DestroyObject(waitBehavior);
        context->DestroyObject(waitTarget);
        error = "Message self-test did not reject a stale Component target.";
        return false;
    }
    manager->ReleaseComponentState(waitBehavior);

    const char *waitModuleName = "__CKAS_MessageWaitSelfTest";
    const char *waitSource =
        "int __ckas_message_wait_probe(const CKBehaviorContext &in ctx) {\n"
        "  if (ctx.MessageManager is null || ctx.Behavior is null) return 21;\n"
        "  CKMessageType type = ctx.MessageManager.AddMessageType(\"__CKAS_MessageWaitSelfTest\");\n"
        "  if (type < 0) return 22;\n"
        "  CKBeObject@ target = ctx.Behavior.GetTarget();\n"
        "  if (target is null) return 23;\n"
        "  CKERROR rc = ctx.MessageManager.RegisterWait(type, ctx.Behavior, 0, target);\n"
        "  if (rc != CK_OK) return 24;\n"
        "  rc = ctx.MessageManager.UnRegisterWait(type, ctx.Behavior, 0);\n"
        "  if (rc != CK_OK) return 25;\n"
        "  rc = ctx.MessageManager.RegisterWait(\"__CKAS_MessageWaitSelfTest\", ctx.Behavior, 0, target);\n"
        "  if (rc != CK_OK) return 26;\n"
        "  rc = ctx.MessageManager.UnRegisterWait(\"__CKAS_MessageWaitSelfTest\", ctx.Behavior, 0);\n"
        "  if (rc != CK_OK) return 27;\n"
        "  return 0;\n"
        "}\n";

    CKAngelScriptResult waitCompileResult = {};
    if (manager->CompileModule(waitModuleName, waitSource, CKAS_COMPILE_REPLACEEXISTING, &waitCompileResult) != CKAS_OK) {
        error = waitCompileResult.ErrorMessage && waitCompileResult.ErrorMessage[0] != '\0'
            ? waitCompileResult.ErrorMessage
            : "Message wait self-test script compile failed.";
        context->DestroyObject(waitBehavior);
        context->DestroyObject(waitTarget);
        return false;
    }
    asIScriptModule *waitModule = manager->GetModule(waitModuleName);
    asIScriptFunction *waitFunction = waitModule
        ? waitModule->GetFunctionByDecl("int __ckas_message_wait_probe(const CKBehaviorContext &in ctx)")
        : nullptr;
    if (!waitFunction) {
        manager->UnloadModule(waitModuleName, nullptr);
        context->DestroyObject(waitBehavior);
        context->DestroyObject(waitTarget);
        error = "Message wait self-test script function was not found.";
        return false;
    }

    CKBehaviorContext behaviorContext = {};
    behaviorContext.Behavior = waitBehavior;
    behaviorContext.DeltaTime = 0.0f;
    behaviorContext.Context = context;
    behaviorContext.ParameterManager = context->GetParameterManager();
    behaviorContext.MessageManager = context->GetMessageManager();
    behaviorContext.AttributeManager = context->GetAttributeManager();
    behaviorContext.TimeManager = context->GetTimeManager();

    asIScriptContext *waitScriptContext = engine->RequestContext();
    if (!waitScriptContext) {
        manager->UnloadModule(waitModuleName, nullptr);
        context->DestroyObject(waitBehavior);
        context->DestroyObject(waitTarget);
        error = "Message wait self-test could not create an execution context.";
        return false;
    }
    r = waitScriptContext->Prepare(waitFunction);
    if (r >= 0) {
        r = waitScriptContext->SetArgObject(0, &behaviorContext);
    }
    if (r >= 0) {
        r = waitScriptContext->Execute();
    }
    executeOk = false;
    if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(waitScriptContext->GetReturnDWord());
        if (returnCode == 0) {
            executeOk = true;
        } else {
            error = "Message wait self-test script returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = waitScriptContext->GetExceptionString();
        error = std::string("Message wait self-test script exception: ") +
                (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = "Message wait self-test script execution failed with code " + std::to_string(r) + ".";
    }
    waitScriptContext->Unprepare();
    engine->ReturnContext(waitScriptContext);
    manager->UnloadModule(waitModuleName, nullptr);
    context->DestroyObject(waitBehavior);
    context->DestroyObject(waitTarget);
    if (!executeOk) {
        return false;
    }
    return true;
}

} // namespace ScriptMessageSelfTest
