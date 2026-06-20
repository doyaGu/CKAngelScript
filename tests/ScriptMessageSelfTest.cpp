#include "ScriptSelfTests.h"

#include <string>

#include "CKAngelScript.h"
#include "ScriptMessage.h"
#include "ScriptManager.h"
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
        "bool Publish(const ScriptContext&in, const string&in, dictionary@, const string&in)",
        "bool Publish(const CKBehaviorContext&in, const string&in, dictionary@, const string&in)",
        "bool Send(const ScriptContext&in, const string&in, const string&in, dictionary@)",
        "bool Send(const CKBehaviorContext&in, const string&in, const string&in, dictionary@)",
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
        "  if (msg.Target() != \"runtime:target\" || msg.FrameIndex() != 456 || msg.RequiresReply()) return 4;\n"
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
    manager->UnloadModule(moduleName, nullptr);
    if (!executeOk) {
        return false;
    }
    return true;
}

} // namespace ScriptMessageSelfTest
