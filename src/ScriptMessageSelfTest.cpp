#include "ScriptSelfTests.h"

#include <string>

#include "CKAngelScript.h"
#include "ScriptMessage.h"
#include "ScriptManager.h"

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
    return true;
}

} // namespace ScriptMessageSelfTest
