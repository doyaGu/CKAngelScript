#include "ScriptSelfTests.h"

#include <string>

#include <angelscript.h>

#include "ScriptAsync.h"
#include "ScriptInvocation.h"
#include "ScriptManager.h"

bool RunScriptInvocationSelfTest(ScriptManager *manager, std::string &error) {
    asIScriptEngine *engine = manager ? manager->GetScriptEngine() : nullptr;
    ScriptAsyncScheduler *scheduler = manager ? manager->GetAsyncScheduler() : nullptr;
    if (!engine || !scheduler) {
        error = "Script invocation self-test requires an initialized manager.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_InvocationSelfTest";
    constexpr const char *source =
        "void FinishProbe() {}\n"
        "void SuspendProbe() {\n"
        "  AsyncTask<void>@ delay = Async::Delay(1);\n"
        "  Await(delay);\n"
        "}\n"
        "void FailureProbe() {\n"
        "  array<int> values;\n"
        "  int value = values[0];\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module || module->AddScriptSection("invocation-self-test", source) < 0 || module->Build() < 0) {
        engine->DiscardModule(moduleName);
        error = "Script invocation self-test failed to build its probe module.";
        return false;
    }

    ScriptInvocationContext invocation(manager);
    auto fail = [&](const std::string &message) {
        invocation.Cancel();
        engine->DiscardModule(moduleName);
        error = message;
        return false;
    };

    asIScriptFunction *finish = module->GetFunctionByDecl("void FinishProbe()");
    asIScriptFunction *suspend = module->GetFunctionByDecl("void SuspendProbe()");
    asIScriptFunction *failure = module->GetFunctionByDecl("void FailureProbe()");
    if (!finish || !suspend || !failure) {
        return fail("Script invocation self-test could not resolve its probe functions.");
    }

    bool prepared = false;
    bool resultRead = false;
    ScriptInvocationSpec finishSpec;
    finishSpec.Label = "FinishProbe";
    finishSpec.Prepare = [&](asIScriptContext *context) {
        prepared = context && context->GetState() == asEXECUTION_PREPARED;
        return prepared;
    };
    finishSpec.Result = [&](asIScriptContext *context) {
        resultRead = context && context->GetState() == asEXECUTION_FINISHED;
        return resultRead;
    };
    if (invocation.Start(finish, finishSpec) != ScriptInvocationStatus::Finished ||
        !prepared || !resultRead || invocation.IsSuspended()) {
        return fail("Script invocation self-test did not complete a prepared call correctly.");
    }

    ScriptInvocationSpec suspendSpec;
    suspendSpec.Label = "SuspendProbe";
    if (invocation.Start(suspend, suspendSpec) != ScriptInvocationStatus::Suspended ||
        !invocation.IsSuspended()) {
        return fail("Script invocation self-test did not retain a suspended call.");
    }
    if (invocation.Resume(suspendSpec) != ScriptInvocationStatus::Suspended ||
        !invocation.IsSuspended()) {
        return fail("Script invocation self-test resumed a pending task too early.");
    }
    scheduler->Tick();
    if (invocation.Resume(suspendSpec) != ScriptInvocationStatus::Finished || invocation.IsSuspended()) {
        return fail("Script invocation self-test did not finish a ready suspended call.");
    }

    ScriptInvocationSpec failureSpec;
    failureSpec.Label = "FailureProbe";
    if (invocation.Start(failure, failureSpec) != ScriptInvocationStatus::Failed ||
        invocation.GetLastResultCode() != asEXECUTION_EXCEPTION ||
        invocation.GetStackTrace().empty()) {
        return fail("Script invocation self-test did not preserve exception diagnostics.");
    }
    if (invocation.Start(finish, finishSpec) != ScriptInvocationStatus::Finished) {
        return fail("Script invocation self-test could not reuse a context after failure.");
    }

    invocation.Cancel();
    if (invocation.GetContext()) {
        return fail("Script invocation self-test did not return its context on cancellation.");
    }
    engine->DiscardModule(moduleName);
    return true;
}
