#include "ScriptSelfTests.h"

#include <string>

#include "AngelScriptManager.h"
#include "ScriptMessage.h"

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

    const char *source =
        "void __ckas_message_runtime_probe(const ScriptRuntimeContext &in ctx) {\n"
        "  dictionary payload;\n"
        "  payload.set(\"value\", int64(7));\n"
        "  bool published = Message::Publish(ctx, \"topic.runtime\", payload);\n"
        "  bool sent = Message::Send(ctx, \"runtime:missing\", \"topic.runtime\", payload);\n"
        "  AsyncTask<dictionary@>@ task = Message::Request(ctx, \"runtime:missing\", \"topic.runtime\", payload, 1);\n"
        "  Message::Subscribe(ctx, \"topic.runtime\");\n"
        "  Message::Unsubscribe(ctx, \"topic.runtime\");\n"
        "}\n"
        "void __ckas_message_component_probe(const CKBehaviorContext &in ctx) {\n"
        "  dictionary payload;\n"
        "  bool published = Message::Publish(ctx, \"topic.component\", payload);\n"
        "  bool sent = Message::Send(ctx, \"component:1\", \"topic.component\", payload);\n"
        "  AsyncTask<dictionary@>@ task = Message::Request(ctx, \"component:1\", \"topic.component\", payload, 1);\n"
        "  Message::Subscribe(ctx, \"topic.component\");\n"
        "  Message::Unsubscribe(ctx, \"topic.component\");\n"
        "}\n"
        "class __CKAS_MessageRuntimeReceiver {\n"
        "  void OnMessage(const ScriptMessage &in msg, const ScriptRuntimeContext &in ctx) {\n"
        "    Await(Async::Delay(1));\n"
        "    uint64 id = msg.Id();\n"
        "    string kind = msg.Kind();\n"
        "    string topic = msg.Topic();\n"
        "    string source = msg.Source();\n"
        "    string target = msg.Target();\n"
        "    uint64 frame = msg.FrameIndex();\n"
        "    dictionary@ payload = msg.Payload();\n"
        "    if (msg.RequiresReply()) Message::Reply(ctx, msg, payload);\n"
        "  }\n"
        "}\n"
        "class __CKAS_MessageComponentReceiver {\n"
        "  void OnMessage(const ScriptMessage &in msg, const CKBehaviorContext &in ctx) {\n"
        "    Await(Async::Delay(1));\n"
        "    dictionary@ payload = msg.Payload();\n"
        "    if (msg.RequiresReply()) Message::Reject(ctx, msg, \"rejected\");\n"
        "  }\n"
        "}\n";

    AngelScriptManager *manager = AngelScriptManager::GetManager(context);
    if (!manager) {
        error = "Message self-test could not retrieve AngelScriptManager.";
        return false;
    }
    AngelScriptResult result = {};
    constexpr const char *moduleName = "__CKAS_MessageCompileSelfTest";
    if (manager->CompileModule(moduleName, source, true, &result) != ANGELSCRIPT_STATUS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Message script API compile probe failed.";
        return false;
    }
    manager->UnloadModule(moduleName, nullptr);
    return true;
}

} // namespace ScriptMessageSelfTest
