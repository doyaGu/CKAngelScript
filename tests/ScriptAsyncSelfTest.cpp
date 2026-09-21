#include "ScriptSelfTests.h"

#include <cstdlib>
#include <fstream>
#include <string>

#include "CKAll.h"
#include "CKAngelScript.h"
#include "ScriptAsync.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeHandles.h"
#include "ScriptManager.h"

#include "add_on/scriptarray/scriptarray.h"

namespace {

void WriteAsyncSelfTestStage(const char *stage) {
    const char *markerPath = std::getenv("CKAS_SELFTEST_MARKER");
    if (!markerPath || !stage) {
        return;
    }
    std::ofstream marker(markerPath, std::ios::trunc);
    if (!marker) {
        return;
    }
    marker << "status=running\n";
    marker << "stage=async:" << stage << "\n";
}

bool CompileAsyncProbe(CKContext *context,
                       asIScriptEngine *engine,
                       const char *moduleName,
                       const char *source,
                       std::string &error) {
    WriteAsyncSelfTestStage(moduleName);
    CKAngelScriptApi api = CKAngelScriptApi::Get(context);
    if (!api.IsValid()) {
        error = "Async self-test could not retrieve CKAngelScript.";
        return false;
    }
    CKAngelScriptResult result = {};
    if (api->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : std::string("Async script API compile probe failed: ") + moduleName;
        return false;
    }
    api->UnloadModule(moduleName, nullptr);
    return true;
}

} // namespace

bool RunScriptAsyncSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    WriteAsyncSelfTestStage("start");
    if (!context || !engine) {
        error = "Async self-test requires a CKContext and AngelScript engine.";
        return false;
    }

    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string restoreNamespace = previousNamespace ? previousNamespace : "";
    if (engine->SetDefaultNamespace("Async") < 0) {
        error = "Async self-test could not enter the Async namespace.";
        return false;
    }
    const char *removedContextDecls[] = {
        "AsyncTask<void>@ Wait(const CKBehaviorContext&in, BBTask@, int)",
        "AsyncTask<void>@ Wait(const CKBehaviorContext&in, GraphTask@)",
        "AsyncTask<void>@ WaitMessage(const CKBehaviorContext&in, const string&in, CKBeObject@)",
        "AsyncTask<void>@ WaitMessage(const CKBehaviorContext&in, CKMessageType, CKBeObject@)"
    };
    for (const char *decl : removedContextDecls) {
        if (engine->GetGlobalFunctionByDecl(decl)) {
            engine->SetDefaultNamespace(restoreNamespace.c_str());
            error = std::string("Async self-test found removed context declaration: ") + decl;
            return false;
        }
    }
    engine->SetDefaultNamespace(restoreNamespace.c_str());

    if (!CompileAsyncProbe(context,
                           engine,
                           "__CKAS_AsyncCompileDelaySelfTest",
                           "void __ckas_async_compile_delay() {\n"
                           "  AsyncTask<void>@ delay = Async::Delay(2);\n"
                           "  bool done = delay.IsDone();\n"
                           "  string err = delay.Error();\n"
                           "  delay.Cancel();\n"
                           "}\n",
                           error)) {
        return false;
    }

    if (!CompileAsyncProbe(context,
                           engine,
                           "__CKAS_AsyncCompileScalarSelfTest",
                           "int __ckas_async_int() { return 42; }\n"
                           "float __ckas_async_float() { return 1.5f; }\n"
                           "string __ckas_async_string() { return \"ok\"; }\n"
                           "CKObject@ __ckas_async_object() { return null; }\n"
                           "void __ckas_async_compile_scalar() {\n"
                           "  AsyncTask<int>@ ti = Async::Spawn(AsyncIntFunc(__ckas_async_int));\n"
                           "  int i = 0;\n"
                           "  Await(ti, i);\n"
                           "  AsyncTask<float>@ tf = Async::Create(AsyncFloatFunc(__ckas_async_float));\n"
                           "  float f = 0.0f;\n"
                           "  Await(tf, f);\n"
                           "  AsyncTask<string>@ ts = Async::Spawn(AsyncStringFunc(__ckas_async_string));\n"
                           "  string s;\n"
                           "  Await(ts, s);\n"
                           "  AsyncTask<CKObject@>@ to = Async::Spawn(AsyncObjectFunc(__ckas_async_object));\n"
                           "  CKObject@ o = null;\n"
                           "  Await(to, o);\n"
                           "}\n",
                           error)) {
        return false;
    }

    if (!CompileAsyncProbe(context,
                           engine,
                           "__CKAS_AsyncCompileIntCreateOutSelfTest",
                           "int __ckas_async_int() { return 42; }\n"
                           "void __ckas_async_compile_int_create_out() {\n"
                           "  AsyncTask<int>@ genericInt;\n"
                           "  Async::Create(AsyncIntFunc(__ckas_async_int), @genericInt);\n"
                           "  int i = 0;\n"
                           "  Await(genericInt, i);\n"
                           "}\n",
                           error)) {
        return false;
    }

    if (!CompileAsyncProbe(context,
                           engine,
                           "__CKAS_AsyncCompileIntAllOutSelfTest",
                           "int __ckas_async_int() { return 42; }\n"
                           "void __ckas_async_compile_int_all_out() {\n"
                           "  AsyncTask<int>@ ti = Async::Create(AsyncIntFunc(__ckas_async_int));\n"
                           "  array<AsyncTask<int>@> ints;\n"
                           "  ints.insertLast(@ti);\n"
                           "  AsyncTask<array<int>@>@ genericAllInts;\n"
                           "  Async::All(ints, @genericAllInts);\n"
                           "  array<int>@ allValues = null;\n"
                           "  Await(@genericAllInts, @allValues);\n"
                           "}\n",
                           error)) {
        return false;
    }

    if (!CompileAsyncProbe(context,
                           engine,
                           "__CKAS_AsyncCompileIntRaceOutSelfTest",
                           "int __ckas_async_int() { return 42; }\n"
                           "void __ckas_async_compile_int_race_out() {\n"
                           "  AsyncTask<int>@ ti = Async::Create(AsyncIntFunc(__ckas_async_int));\n"
                           "  array<AsyncTask<int>@> ints;\n"
                           "  ints.insertLast(@ti);\n"
                           "  AsyncTask<int>@ genericInt;\n"
                           "  Async::Race(ints, @genericInt);\n"
                           "}\n",
                           error)) {
        return false;
    }

    if (!CompileAsyncProbe(context,
                           engine,
                           "__CKAS_AsyncCompileIntAnyOutSelfTest",
                           "int __ckas_async_int() { return 42; }\n"
                           "void __ckas_async_compile_int_any_out() {\n"
                           "  AsyncTask<int>@ ti = Async::Create(AsyncIntFunc(__ckas_async_int));\n"
                           "  array<AsyncTask<int>@> ints;\n"
                           "  ints.insertLast(@ti);\n"
                           "  AsyncTask<int>@ genericInt;\n"
                           "  Async::Any(ints, @genericInt);\n"
                           "}\n",
                           error)) {
        return false;
    }

    if (!CompileAsyncProbe(context,
                           engine,
                           "__CKAS_AsyncCompileObjectAggregateSelfTest",
                           "class __CKAS_AsyncBox { int Value; }\n"
                           "funcdef __CKAS_AsyncBox@ __CKAS_AsyncBoxFunc();\n"
                           "__CKAS_AsyncBox@ __ckas_async_box() { __CKAS_AsyncBox@ b = __CKAS_AsyncBox(); b.Value = 7; return b; }\n"
                           "void __ckas_async_compile_object_aggregate() {\n"
                           "  AsyncTask<__CKAS_AsyncBox@>@ tb;\n"
                           "  Async::Spawn(__CKAS_AsyncBoxFunc(__ckas_async_box), @tb);\n"
                           "  __CKAS_AsyncBox@ b = null;\n"
                           "  Await(@tb, @b);\n"
                           "  array<AsyncTask<__CKAS_AsyncBox@>@> boxes;\n"
                           "  boxes.insertLast(@tb);\n"
                           "  AsyncTask<array<__CKAS_AsyncBox@>@>@ allBoxes;\n"
                           "  Async::All(boxes, @allBoxes);\n"
                           "  AsyncTask<__CKAS_AsyncBox@>@ firstBox;\n"
                           "  Async::Race(boxes, @firstBox);\n"
                           "  Async::Any(boxes, @firstBox);\n"
                           "}\n",
                           error)) {
        return false;
    }

    if (!CompileAsyncProbe(context,
                           engine,
                           "__CKAS_AsyncCompileBridgeSelfTest",
                           "void __ckas_async_compile_bridge(BehaviorFrame@ frame, BBTask@ bbTask, GraphTask@ graphTask, CKBeObject@ object) {\n"
                           "  if (frame is null) return;\n"
                           "  if (bbTask !is null) { Await(frame.Wait(bbTask)); }\n"
                           "  if (graphTask !is null) { Await(frame.Wait(graphTask)); }\n"
                           "  AsyncTask<void>@ task = frame.WaitMessage(\"__CKAS_AsyncCompileMessageWaitSelfTest\", object);\n"
                           "  if (task !is null) { task.Cancel(); }\n"
                           "  CKMessageManager@ manager = frame.Context().GetMessageManager();\n"
                           "  CKMessageType type = manager is null ? -1 : manager.AddMessageType(\"__CKAS_AsyncCompileMessageWaitSelfTest\");\n"
                           "  @task = frame.WaitMessage(type, object);\n"
                           "  if (task !is null) { task.Cancel(); }\n"
                           "}\n",
                           error)) {
        return false;
    }

    WriteAsyncSelfTestStage("gc-flags");
    asITypeInfo *voidTaskType = engine->GetTypeInfoByDecl("AsyncTask<void>");
    asITypeInfo *intTaskType = engine->GetTypeInfoByDecl("AsyncTask<int>");
    if (!voidTaskType || !intTaskType) {
        error = "Async self-test could not resolve instantiated AsyncTask<T> types.";
        return false;
    }
    if ((voidTaskType->GetFlags() & asOBJ_GC) == 0 || (intTaskType->GetFlags() & asOBJ_GC) == 0) {
        error = "AsyncTask<T> template instances are not registered as garbage collected.";
        return false;
    }

    ScriptAsyncScheduler scheduler(ScriptManager::GetManager(context));

    WriteAsyncSelfTestStage("bb-output-retry-completion");
    if (!ScriptAsyncBBTaskWaitComplete(true, CKBR_ACTIVATENEXTFRAME) ||
        ScriptAsyncBBTaskWaitComplete(false, CKBR_ACTIVATENEXTFRAME) ||
        !ScriptAsyncBBTaskWaitComplete(false, CKBR_OK)) {
        error = "Async BBTask output/retry completion decision is invalid.";
        return false;
    }

    WriteAsyncSelfTestStage("gc-cycle");
    CKAngelScriptApi api = CKAngelScriptApi::Get(context);
    if (!api.IsValid()) {
        error = "Async GC self-test could not retrieve CKAngelScript.";
        return false;
    }
    CKAngelScriptResult result = {};
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptAsyncScheduler *managerScheduler = manager ? manager->GetAsyncScheduler() : nullptr;
    if (!managerScheduler) {
        error = "Async completed result self-test could not retrieve the manager scheduler.";
        return false;
    }

    WriteAsyncSelfTestStage("graph-same-tick-completion");
    ScriptBehaviorBridge *behaviorBridge = manager->GetBehaviorBridge();
    CKBehavior *graphBehavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_AsyncGraphSameTickSelfTest"),
        CK_OBJECTCREATION_DYNAMIC));
    GraphTask *graphWatch = nullptr;
    ScriptAsyncTaskBase *graphWait = nullptr;
    auto cleanupGraphWait = [&]() {
        if (graphWait) {
            graphWait->Release();
            graphWait = nullptr;
        }
        scheduler.Clear();
        if (graphWatch) {
            graphWatch->Release();
            graphWatch = nullptr;
        }
        if (graphBehavior && !graphBehavior->IsToBeDeleted()) {
            context->DestroyObject(graphBehavior);
        }
    };
    if (!behaviorBridge || !graphBehavior) {
        cleanupGraphWait();
        error = "Async graph completion self-test could not create its behavior.";
        return false;
    }
    graphBehavior->UseGraph();
    graphBehavior->AddOutput(const_cast<CKSTRING>("Done"));
    if (graphBehavior->GetOutputCount() != 1) {
        cleanupGraphWait();
        error = "Async graph completion self-test could not create its output.";
        return false;
    }
    CKBehaviorContext graphContext = {};
    graphContext.Behavior = graphBehavior;
    graphContext.Context = context;
    graphContext.ParameterManager = context->GetParameterManager();
    graphContext.MessageManager = context->GetMessageManager();
    graphContext.AttributeManager = context->GetAttributeManager();
    graphContext.TimeManager = context->GetTimeManager();
    graphWatch = behaviorBridge->CreateGraphWatch(graphBehavior, 0, 0.0f);
    graphWait = graphWatch ? scheduler.CreateGraphTask(graphWatch, &graphContext) : nullptr;
    if (!graphWatch || !graphWait) {
        cleanupGraphWait();
        error = "Async graph completion self-test could not create its graph wait.";
        return false;
    }
    graphWatch->Release();
    graphWatch = nullptr;
    graphBehavior->ActivateOutput(0, TRUE);
    scheduler.Tick();
    const bool graphCompleted = graphWait->IsCompleted();
    const std::string graphError = graphWait->Error();
    cleanupGraphWait();
    if (!graphCompleted) {
        error = graphError.empty()
            ? "Async graph wait did not complete in the tick that observed its output."
            : graphError;
        return false;
    }

    WriteAsyncSelfTestStage("await-delay-resume-check");
    asIScriptContext *delayWaitContext = engine->RequestContext();
    if (!delayWaitContext) {
        error = "Async delay resume self-test could not request a script context.";
        return false;
    }
    ScriptAsyncTaskBase *delayTask = scheduler.CreateDelay(2);
    std::string waitError;
    auto cleanupDelayWait = [&]() {
        scheduler.ForgetContext(delayWaitContext);
        if (delayTask) {
            delayTask->Release();
            delayTask = nullptr;
        }
        engine->ReturnContext(delayWaitContext);
    };
    if (!delayTask ||
        !scheduler.WaitForTask(delayWaitContext, delayTask, nullptr, asTYPEID_VOID, waitError)) {
        cleanupDelayWait();
        error = waitError.empty() ? "Async delay resume self-test could not register await." : waitError;
        return false;
    }
    if (scheduler.PrepareContextResume(delayWaitContext, waitError) != ScriptAsyncScheduler::ResumeState::Pending) {
        cleanupDelayWait();
        error = "Async delay resume self-test expected initial resume check to stay pending.";
        return false;
    }
    scheduler.Tick();
    if (scheduler.PrepareContextResume(delayWaitContext, waitError) != ScriptAsyncScheduler::ResumeState::Pending) {
        cleanupDelayWait();
        error = "Async delay resume self-test expected Delay(2) to stay pending after one tick.";
        return false;
    }
    scheduler.Tick();
    if (scheduler.PrepareContextResume(delayWaitContext, waitError) != ScriptAsyncScheduler::ResumeState::Ready) {
        cleanupDelayWait();
        error = waitError.empty() ? "Async delay resume self-test expected Delay(2) to become ready after two ticks." : waitError;
        return false;
    }
    cleanupDelayWait();

    WriteAsyncSelfTestStage("pause-freezes-scheduler");
    ScriptAsyncTaskBase *pausedTask = scheduler.CreateDelay(2);
    if (!pausedTask) {
        error = "Async pause self-test could not create delay task.";
        return false;
    }
    scheduler.PauseProcessing();
    scheduler.Tick();
    scheduler.Tick();
    if (!pausedTask->IsPending()) {
        scheduler.ResumeProcessing();
        pausedTask->Release();
        error = "Async pause self-test advanced a task while processing was paused.";
        return false;
    }
    scheduler.ResumeProcessing();
    scheduler.Tick();
    scheduler.Tick();
    if (!pausedTask->IsCompleted()) {
        pausedTask->Release();
        error = "Async pause self-test did not resume task advancement.";
        return false;
    }
    pausedTask->Release();

    WriteAsyncSelfTestStage("await-cancel-resume-check");
    asIScriptContext *cancelledWaitContext = engine->RequestContext();
    if (!cancelledWaitContext) {
        error = "Async cancellation resume self-test could not request a script context.";
        return false;
    }
    ScriptAsyncTaskBase *cancelledTask = scheduler.CreateDelay(2);
    auto cleanupCancelledWait = [&]() {
        scheduler.ForgetContext(cancelledWaitContext);
        if (cancelledTask) {
            cancelledTask->Release();
            cancelledTask = nullptr;
        }
        engine->ReturnContext(cancelledWaitContext);
    };
    waitError.clear();
    if (!cancelledTask ||
        !scheduler.WaitForTask(cancelledWaitContext, cancelledTask, nullptr, asTYPEID_VOID, waitError)) {
        cleanupCancelledWait();
        error = waitError.empty()
            ? "Async cancellation resume self-test could not register await."
            : waitError;
        return false;
    }
    const std::string cancellationReason = "Async wait cancelled by lifecycle transition.";
    scheduler.CancelAll(cancellationReason);
    waitError.clear();
    if (scheduler.PrepareContextResume(cancelledWaitContext, waitError) !=
            ScriptAsyncScheduler::ResumeState::Failed ||
        waitError != cancellationReason) {
        cleanupCancelledWait();
        error = "Async cancellation resume self-test expected the waiter to observe cancellation.";
        return false;
    }
    cleanupCancelledWait();

    WriteAsyncSelfTestStage("shutdown-does-not-succeed-await");
    asIScriptContext *shutdownWaitContext = engine->RequestContext();
    if (!shutdownWaitContext) {
        error = "Async shutdown resume self-test could not request a script context.";
        return false;
    }
    ScriptAsyncTaskBase *shutdownTask = scheduler.CreateDelay(20);
    auto cleanupShutdownWait = [&]() {
        scheduler.ForgetContext(shutdownWaitContext);
        if (shutdownTask) {
            shutdownTask->Release();
            shutdownTask = nullptr;
        }
        engine->ReturnContext(shutdownWaitContext);
    };
    waitError.clear();
    if (!shutdownTask ||
        !scheduler.WaitForTask(shutdownWaitContext, shutdownTask, nullptr, asTYPEID_VOID, waitError)) {
        cleanupShutdownWait();
        error = waitError.empty()
            ? "Async shutdown resume self-test could not register await."
            : waitError;
        return false;
    }
    const std::string shutdownReason = "Async wait cancelled by scheduler shutdown.";
    scheduler.Shutdown(shutdownReason);
    waitError.clear();
    if (scheduler.PrepareContextResume(shutdownWaitContext, waitError) !=
            ScriptAsyncScheduler::ResumeState::Failed ||
        waitError != shutdownReason) {
        cleanupShutdownWait();
        error = "Async shutdown resume self-test expected the waiter to observe cancellation, not success.";
        return false;
    }
    cleanupShutdownWait();

    WriteAsyncSelfTestStage("clear-does-not-succeed-await");
    asIScriptContext *clearedWaitContext = engine->RequestContext();
    if (!clearedWaitContext) {
        error = "Async clear resume self-test could not request a script context.";
        return false;
    }
    ScriptAsyncTaskBase *clearedTask = scheduler.CreateDelay(20);
    auto cleanupClearedWait = [&]() {
        scheduler.ForgetContext(clearedWaitContext);
        if (clearedTask) {
            clearedTask->Release();
            clearedTask = nullptr;
        }
        engine->ReturnContext(clearedWaitContext);
    };
    waitError.clear();
    if (!clearedTask ||
        !scheduler.WaitForTask(clearedWaitContext, clearedTask, nullptr, asTYPEID_VOID, waitError)) {
        cleanupClearedWait();
        error = waitError.empty()
            ? "Async clear resume self-test could not register await."
            : waitError;
        return false;
    }
    scheduler.Clear();
    waitError.clear();
    if (scheduler.PrepareContextResume(clearedWaitContext, waitError) !=
            ScriptAsyncScheduler::ResumeState::Failed) {
        cleanupClearedWait();
        error = "Async clear resume self-test expected a dropped await to fail, not succeed.";
        return false;
    }
    cleanupClearedWait();

    WriteAsyncSelfTestStage("owner-scoped-cancellation");
    constexpr const char *ownerModuleName = "__CKAS_AsyncOwnerCancellationSelfTest";
    if (api->CompileModule(ownerModuleName,
                           "void __ckas_async_owner_wait() { Await(Async::Delay(20)); }\n"
                           "void __ckas_async_new_epoch_wait() { Await(Async::Delay(1)); }\n"
                           "void __ckas_async_other_owner_wait() { Await(Async::Delay(1)); }\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Async owner cancellation self-test compile failed.";
        return false;
    }
    asIScriptModule *ownerModule = engine->GetModule(ownerModuleName, asGM_ONLY_IF_EXISTS);
    asIScriptFunction *ownerFunction = ownerModule
        ? ownerModule->GetFunctionByDecl("void __ckas_async_owner_wait()")
        : nullptr;
    asIScriptContext *ownerContext = ownerFunction ? engine->RequestContext() : nullptr;
    asIScriptContext *otherOwnerContext = nullptr;
    auto cleanupOwner = [&]() {
        if (otherOwnerContext) {
            managerScheduler->ForgetContext(otherOwnerContext);
            otherOwnerContext->Abort();
            otherOwnerContext->Unprepare();
            engine->ReturnContext(otherOwnerContext);
            otherOwnerContext = nullptr;
        }
        if (ownerContext) {
            managerScheduler->ForgetContext(ownerContext);
            ownerContext->Abort();
            ownerContext->Unprepare();
            engine->ReturnContext(ownerContext);
            ownerContext = nullptr;
        }
        api->UnloadModule(ownerModuleName, nullptr);
    };
    if (!ownerContext || ownerContext->Prepare(ownerFunction) < 0) {
        cleanupOwner();
        error = "Async owner cancellation self-test could not prepare its invocation.";
        return false;
    }
    const ScriptAsyncOwnerId ownerId = managerScheduler->CreateOwner();
    managerScheduler->BeginOwnerExecution(ownerContext, ownerId);
    const int ownerExecuteResult = ownerContext->Execute();
    managerScheduler->EndOwnerExecution(ownerContext);
    asIScriptFunction *otherOwnerFunction = ownerModule->GetFunctionByDecl(
        "void __ckas_async_other_owner_wait()");
    otherOwnerContext = otherOwnerFunction ? engine->RequestContext() : nullptr;
    const ScriptAsyncOwnerId otherOwnerId = managerScheduler->CreateOwner();
    if (!otherOwnerContext || otherOwnerContext->Prepare(otherOwnerFunction) < 0) {
        cleanupOwner();
        error = "Async owner cancellation self-test could not prepare independent owner work.";
        return false;
    }
    managerScheduler->BeginOwnerExecution(otherOwnerContext, otherOwnerId);
    const int otherOwnerExecuteResult = otherOwnerContext->Execute();
    managerScheduler->EndOwnerExecution(otherOwnerContext);
    const std::string ownerCancellationReason = "Async work was cancelled because its script host is resetting.";
    waitError.clear();
    if (ownerExecuteResult != asEXECUTION_SUSPENDED ||
        !managerScheduler->CancelOwner(ownerId, ownerCancellationReason) ||
        managerScheduler->PrepareContextResume(ownerContext, waitError) !=
            ScriptAsyncScheduler::ResumeState::Failed ||
        waitError != ownerCancellationReason) {
        cleanupOwner();
        error = "Async owner cancellation self-test did not cancel only the old host invocation work.";
        return false;
    }
    managerScheduler->Tick();
    waitError.clear();
    if (otherOwnerExecuteResult != asEXECUTION_SUSPENDED ||
        managerScheduler->PrepareContextResume(otherOwnerContext, waitError) !=
            ScriptAsyncScheduler::ResumeState::Ready) {
        cleanupOwner();
        error = "Async owner cancellation self-test cancelled an independent script host's work.";
        return false;
    }
    ownerContext->Abort();
    ownerContext->Unprepare();
    asIScriptFunction *newEpochFunction = ownerModule->GetFunctionByDecl(
        "void __ckas_async_new_epoch_wait()");
    if (!newEpochFunction || ownerContext->Prepare(newEpochFunction) < 0) {
        cleanupOwner();
        error = "Async owner cancellation self-test could not prepare new-epoch work.";
        return false;
    }
    managerScheduler->BeginOwnerExecution(ownerContext, ownerId);
    const int newEpochExecuteResult = ownerContext->Execute();
    managerScheduler->EndOwnerExecution(ownerContext);
    managerScheduler->Tick();
    waitError.clear();
    if (newEpochExecuteResult != asEXECUTION_SUSPENDED ||
        managerScheduler->PrepareContextResume(ownerContext, waitError) !=
            ScriptAsyncScheduler::ResumeState::Ready) {
        cleanupOwner();
        error = "Async owner cancellation self-test cancelled work created in the new reset epoch.";
        return false;
    }
    cleanupOwner();

    WriteAsyncSelfTestStage("owner-aggregate-isolation");
    const ScriptAsyncOwnerId aggregateOwner = scheduler.CreateOwner();
    const ScriptAsyncOwnerId childOwner = scheduler.CreateOwner();
    ScriptAsyncTaskBase *foreignChild = scheduler.CreateDelay(5);
    if (!foreignChild) {
        error = "Async owner aggregate self-test could not create foreign child task.";
        return false;
    }
    foreignChild->SetOwnerId(childOwner);
    std::vector<ScriptAsyncTaskBase *> aggregateChildren = {foreignChild};
    ScriptAsyncTaskBase *ownedAggregate = scheduler.CreateAggregate(
        ScriptAsyncTaskKind::All,
        asTYPEID_VOID,
        aggregateChildren);
    if (!ownedAggregate) {
        foreignChild->Release();
        error = "Async owner aggregate self-test could not create aggregate task.";
        return false;
    }
    ownedAggregate->SetOwnerId(aggregateOwner);
    const bool aggregateCancelled = scheduler.CancelOwner(
        aggregateOwner,
        "Aggregate owner was reset.");
    if (!aggregateCancelled || !ownedAggregate->IsCancelled() || !foreignChild->IsPending()) {
        ownedAggregate->Release();
        foreignChild->Release();
        scheduler.Clear();
        error = "Async owner aggregate self-test cancelled a child owned by another script host.";
        return false;
    }
    ownedAggregate->Release();
    foreignChild->Release();
    scheduler.Clear();

    WriteAsyncSelfTestStage("orphan-cleanup-isolation");
    const ScriptAsyncOwnerId liveOwner = scheduler.CreateOwner();
    const ScriptAsyncOwnerId orphanOwner = scheduler.CreateOwner();
    ScriptAsyncTaskBase *unownedTask = scheduler.CreateDelay(5);
    ScriptAsyncTaskBase *liveTask = scheduler.CreateDelay(5);
    ScriptAsyncTaskBase *orphanTask = scheduler.CreateDelay(5);
    if (!unownedTask || !liveTask || !orphanTask) {
        if (unownedTask) unownedTask->Release();
        if (liveTask) liveTask->Release();
        if (orphanTask) orphanTask->Release();
        scheduler.Clear();
        error = "Async orphan cleanup self-test could not create tasks.";
        return false;
    }
    liveTask->SetOwnerId(liveOwner);
    orphanTask->SetOwnerId(orphanOwner);
    const std::string orphanReason = "Async work was orphaned by reset.";
    const std::unordered_set<ScriptAsyncOwnerId> liveOwners = {liveOwner};
    const bool orphanedCancelled = scheduler.CancelOrphaned(liveOwners, orphanReason);
    if (!orphanedCancelled ||
        !unownedTask->IsCancelled() || unownedTask->Error() != orphanReason ||
        !orphanTask->IsCancelled() || orphanTask->Error() != orphanReason ||
        !liveTask->IsPending()) {
        unownedTask->Release();
        liveTask->Release();
        orphanTask->Release();
        scheduler.Clear();
        error = "Async orphan cleanup self-test did not preserve the live Script Host Scope.";
        return false;
    }
    unownedTask->Release();
    liveTask->Release();
    orphanTask->Release();
    scheduler.Clear();

    WriteAsyncSelfTestStage("completed-result-dependencies");
    constexpr const char *primitiveResultModuleName = "__CKAS_AsyncCompletedPrimitiveSelfTest";
    if (api->CompileModule(primitiveResultModuleName,
                           "int __ckas_async_completed_int() { return 3; }\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Async completed primitive result self-test compile failed.";
        return false;
    }
    asIScriptModule *primitiveResultModule = engine->GetModule(primitiveResultModuleName, asGM_ONLY_IF_EXISTS);
    asIScriptFunction *primitiveResultFunction = primitiveResultModule
        ? primitiveResultModule->GetFunctionByDecl("int __ckas_async_completed_int()")
        : nullptr;
    if (!primitiveResultFunction) {
        api->UnloadModule(primitiveResultModuleName, nullptr);
        error = "Async completed primitive result self-test could not resolve function.";
        return false;
    }
    ScriptAsyncTaskBase *primitiveResultTask =
        managerScheduler->CreateScriptTask(primitiveResultFunction, primitiveResultFunction->GetReturnTypeId());
    primitiveResultTask->Advance();
    if (!primitiveResultTask->IsCompleted() ||
        primitiveResultTask->UsesModule(primitiveResultModuleName) ||
        api->UnloadModule(primitiveResultModuleName, &result) != CKAS_OK) {
        primitiveResultTask->Release();
        managerScheduler->Clear();
        api->UnloadModule(primitiveResultModuleName, nullptr);
        error = "Async completed primitive result self-test expected completed primitive tasks not to block unload.";
        return false;
    }
    primitiveResultTask->Release();
    managerScheduler->Clear();

    constexpr const char *objectResultModuleName = "__CKAS_AsyncCompletedObjectSelfTest";
    if (api->CompileModule(objectResultModuleName,
                           "class __CKAS_AsyncCompletedBox { int value; }\n"
                           "__CKAS_AsyncCompletedBox@ __ckas_async_completed_box() {\n"
                           "  __CKAS_AsyncCompletedBox@ box = __CKAS_AsyncCompletedBox();\n"
                           "  box.value = 7;\n"
                           "  return box;\n"
                           "}\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Async completed object result self-test compile failed.";
        return false;
    }
    asIScriptModule *objectResultModule = engine->GetModule(objectResultModuleName, asGM_ONLY_IF_EXISTS);
    asIScriptFunction *objectResultFunction = objectResultModule
        ? objectResultModule->GetFunctionByDecl("__CKAS_AsyncCompletedBox@ __ckas_async_completed_box()")
        : nullptr;
    if (!objectResultFunction) {
        api->UnloadModule(objectResultModuleName, nullptr);
        error = "Async completed object result self-test could not resolve function.";
        return false;
    }
    ScriptAsyncTaskBase *objectResultTask =
        managerScheduler->CreateScriptTask(objectResultFunction, objectResultFunction->GetReturnTypeId());
    objectResultTask->Advance();
    if (!objectResultTask->IsCompleted() ||
        !objectResultTask->UsesModule(objectResultModuleName) ||
        api->UnloadModule(objectResultModuleName, &result) != CKAS_INUSE) {
        objectResultTask->Release();
        managerScheduler->Clear();
        api->UnloadModule(objectResultModuleName, nullptr);
        error = "Async completed object result self-test expected script object results to block unload.";
        return false;
    }
    objectResultTask->Release();
    managerScheduler->Clear();
    if (api->UnloadModule(objectResultModuleName, &result) != CKAS_OK) {
        error = "Async completed object result self-test failed to unload after task release.";
        return false;
    }

    WriteAsyncSelfTestStage("cancel-script-context-release");
    constexpr const char *cancelContextModuleName = "__CKAS_AsyncCancelContextSelfTest";
    if (api->CompileModule(cancelContextModuleName,
                           "void __ckas_async_cancel_wait() {\n"
                           "  AsyncTask<void>@ delay = Async::Delay(100);\n"
                           "  Await(delay);\n"
                           "}\n"
                           "int __ckas_async_after_cancel() { return 8; }\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Async cancel context self-test compile failed.";
        return false;
    }
    asIScriptModule *cancelContextModule = engine->GetModule(cancelContextModuleName, asGM_ONLY_IF_EXISTS);
    asIScriptFunction *cancelWaitFunction = cancelContextModule
        ? cancelContextModule->GetFunctionByDecl("void __ckas_async_cancel_wait()")
        : nullptr;
    asIScriptFunction *afterCancelFunction = cancelContextModule
        ? cancelContextModule->GetFunctionByDecl("int __ckas_async_after_cancel()")
        : nullptr;
    if (!cancelWaitFunction || !afterCancelFunction) {
        managerScheduler->Clear();
        api->UnloadModule(cancelContextModuleName, nullptr);
        error = "Async cancel context self-test could not resolve script functions.";
        return false;
    }
    ScriptAsyncTaskBase *cancelTask = managerScheduler->CreateScriptTask(cancelWaitFunction, asTYPEID_VOID);
    cancelTask->Advance();
    if (!cancelTask->IsRunning() || cancelTask->IsDone()) {
        cancelTask->Release();
        managerScheduler->Clear();
        api->UnloadModule(cancelContextModuleName, nullptr);
        error = "Async cancel context self-test expected script task to suspend.";
        return false;
    }
    cancelTask->Cancel();
    cancelTask->Release();
    managerScheduler->Clear();

    asIScriptContext *afterCancelContext = engine->RequestContext();
    int afterCancelCode = afterCancelContext ? afterCancelContext->Prepare(afterCancelFunction) : asERROR;
    if (afterCancelCode >= 0) {
        afterCancelCode = afterCancelContext->Execute();
    }
    const int afterCancelValue = afterCancelCode == asEXECUTION_FINISHED
        ? static_cast<int>(afterCancelContext->GetReturnDWord())
        : 0;
    if (afterCancelContext) {
        afterCancelContext->Unprepare();
        engine->ReturnContext(afterCancelContext);
    }
    if (afterCancelCode != asEXECUTION_FINISHED ||
        afterCancelValue != 8 ||
        api->UnloadModule(cancelContextModuleName, &result) != CKAS_OK) {
        api->UnloadModule(cancelContextModuleName, nullptr);
        error = "Async cancel context self-test expected cancelled script task contexts to be reusable.";
        return false;
    }

    WriteAsyncSelfTestStage("tick-reentrant-track");
    if (api->CompileModule("__CKAS_AsyncTickMutationSelfTest",
                           "void __ckas_async_tick_mutates() {\n"
                           "  AsyncTask<void>@ delay = Async::Delay(1);\n"
                           "  if (delay !is null) { delay.IsPending(); }\n"
                           "}\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Async tick mutation self-test compile failed.";
        return false;
    }
    asIScriptModule *tickModule = engine->GetModule("__CKAS_AsyncTickMutationSelfTest", asGM_ONLY_IF_EXISTS);
    asIScriptFunction *tickFunction = tickModule
        ? tickModule->GetFunctionByDecl("void __ckas_async_tick_mutates()")
        : nullptr;
    if (!tickFunction) {
        api->UnloadModule("__CKAS_AsyncTickMutationSelfTest", nullptr);
        error = "Async tick mutation self-test could not resolve script function.";
        return false;
    }
    ScriptAsyncTaskBase *tickTask = scheduler.CreateScriptTask(tickFunction, asTYPEID_VOID);
    scheduler.Tick();
    if (!tickTask || !tickTask->IsCompleted()) {
        if (tickTask) {
            tickTask->Release();
        }
        scheduler.Clear();
        api->UnloadModule("__CKAS_AsyncTickMutationSelfTest", nullptr);
        error = "Async scheduler did not complete a script task that tracked another task during Tick().";
        return false;
    }
    tickTask->Release();
    scheduler.Clear();
    api->UnloadModule("__CKAS_AsyncTickMutationSelfTest", nullptr);

    if (api->CompileModule("__CKAS_AsyncGCCycleSelfTest",
                           "class __CKAS_AsyncCycleBox {\n"
                           "  AsyncTask<__CKAS_AsyncCycleBox@>@ task;\n"
                           "}\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Async GC cycle self-test compile failed.";
        return false;
    }

    asIScriptModule *cycleModule = engine->GetModule("__CKAS_AsyncGCCycleSelfTest", asGM_ONLY_IF_EXISTS);
    asITypeInfo *boxType = cycleModule ? cycleModule->GetTypeInfoByDecl("__CKAS_AsyncCycleBox") : nullptr;
    const int boxHandleType = cycleModule ? cycleModule->GetTypeIdByDecl("__CKAS_AsyncCycleBox@") : 0;
    const int boxTaskType = cycleModule ? cycleModule->GetTypeIdByDecl("AsyncTask<__CKAS_AsyncCycleBox@>@") : 0;
    if (!boxType || boxHandleType == 0 || boxTaskType == 0 || boxType->GetPropertyCount() != 1) {
        api->UnloadModule("__CKAS_AsyncGCCycleSelfTest", nullptr);
        error = "Async GC cycle self-test could not resolve cycle test types.";
        return false;
    }
    int boxTaskPropertyType = 0;
    boxType->GetProperty(0, nullptr, &boxTaskPropertyType);
    if (boxTaskPropertyType != boxTaskType) {
        api->UnloadModule("__CKAS_AsyncGCCycleSelfTest", nullptr);
        error = "Async GC cycle self-test resolved an unexpected Box.task type.";
        return false;
    }

    asIScriptObject *box = static_cast<asIScriptObject *>(engine->CreateScriptObject(boxType));
    if (!box) {
        api->UnloadModule("__CKAS_AsyncGCCycleSelfTest", nullptr);
        error = "Async GC cycle self-test could not create the script object.";
        return false;
    }
    ScriptAsyncTaskBase *cycleTask = new ScriptAsyncTaskBase(&scheduler, engine, ScriptAsyncTaskKind::Manual, boxHandleType);
    cycleTask->NotifyGarbageCollector();
    void *boxHandle = box;
    cycleTask->CompleteFromAddress(&boxHandle, boxHandleType);

    ScriptAsyncTaskBase **taskSlot = static_cast<ScriptAsyncTaskBase **>(box->GetAddressOfProperty(0));
    if (!taskSlot) {
        cycleTask->Release();
        box->Release();
        api->UnloadModule("__CKAS_AsyncGCCycleSelfTest", nullptr);
        error = "Async GC cycle self-test could not access Box.task.";
        return false;
    }
    *taskSlot = cycleTask;
    cycleTask->AddRef();

    asUINT destroyedBefore = 0;
    engine->GetGCStatistics(nullptr, &destroyedBefore);
    cycleTask->Release();
    box->Release();
    for (int i = 0; i < 4; ++i) {
        engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    }
    asUINT destroyedAfter = 0;
    engine->GetGCStatistics(nullptr, &destroyedAfter);
    if (destroyedAfter <= destroyedBefore) {
        api->UnloadModule("__CKAS_AsyncGCCycleSelfTest", nullptr);
        error = "Async GC cycle self-test did not destroy the AsyncTask/script-object cycle.";
        return false;
    }
    api->UnloadModule("__CKAS_AsyncGCCycleSelfTest", nullptr);

    WriteAsyncSelfTestStage("delay");
    ScriptAsyncTaskBase *delay = scheduler.CreateDelay(2);
    if (!delay || delay->IsDone()) {
        error = "Async delay self-test created an already completed Delay(2).";
        if (delay) {
            delay->Release();
        }
        return false;
    }
    scheduler.Tick();
    if (delay->IsDone()) {
        error = "Async delay self-test completed after one tick.";
        delay->Release();
        return false;
    }
    scheduler.Tick();
    if (!delay->IsCompleted()) {
        error = "Async delay self-test did not complete after two ticks.";
        delay->Release();
        return false;
    }
    delay->Release();

    WriteAsyncSelfTestStage("message-wait");
    CKMessageManager *messages = context->GetMessageManager();
    CKBeObject *messageTarget = CKBeObject::Cast(context->CreateObject(CKCID_BEOBJECT,
                                                                       const_cast<CKSTRING>("__CKAS_AsyncMessageWaitTarget"),
                                                                       CK_OBJECTCREATION_DYNAMIC));
    if (!messages || !messageTarget) {
        if (messageTarget) {
            context->DestroyObject(messageTarget);
        }
        error = "Async message wait self-test could not create a wait target.";
        return false;
    }
    const char *messageName = "__CKAS_AsyncMessageWaitSelfTest";
    std::string messageWaitError;
    ScriptAsyncTaskBase *messageTask = scheduler.CreateMessageWait(context, messageName, messageTarget, &messageWaitError);
    if (!messageTask) {
        context->DestroyObject(messageTarget);
        error = messageWaitError.empty()
            ? "Async message wait self-test could not create the wait task."
            : messageWaitError;
        return false;
    }
    const CKMessageType messageType = messages->AddMessageType(const_cast<CKSTRING>(messageName));
    messages->SendMessageBroadcast(messageType, CKCID_BEOBJECT, nullptr);
    messages->PostProcess();
    scheduler.Tick();
    if (!messageTask->IsCompleted()) {
        error = messageTask->Error().empty()
            ? "Async message wait self-test did not complete after broadcast."
            : messageTask->Error();
        messageTask->Release();
        scheduler.Clear();
        context->DestroyObject(messageTarget);
        return false;
    }
    messageTask->Release();
    scheduler.Clear();

    std::string staleWaitError;
    ScriptAsyncTaskBase *staleWaitTask = scheduler.CreateMessageWait(context,
                                                                     messageName,
                                                                     messageTarget,
                                                                     &staleWaitError);
    CKBehavior *staleWaitBehavior = CKBehavior::Cast(
        context->GetObjectByName(const_cast<CKSTRING>("__CKAS_MessageWait")));
    if (!staleWaitTask || !staleWaitBehavior) {
        if (staleWaitTask) {
            staleWaitTask->Release();
        }
        scheduler.Clear();
        context->DestroyObject(messageTarget);
        error = staleWaitError.empty()
            ? "Async stale message wait self-test could not create its internal behavior."
            : staleWaitError;
        return false;
    }
    if (!staleWaitBehavior->GetAppData() || staleWaitBehavior->GetAppData() == staleWaitTask) {
        staleWaitTask->Release();
        scheduler.Clear();
        context->DestroyObject(messageTarget);
        error = "Async message wait did not install an independent identity token.";
        return false;
    }
    staleWaitBehavior->SetAppData(nullptr);
    staleWaitBehavior->ActivateOutput(0, TRUE);
    scheduler.Tick();
    const bool staleBehaviorStillAlive =
        context->GetObjectByName(const_cast<CKSTRING>("__CKAS_MessageWait")) == staleWaitBehavior;
    if (!staleWaitTask->IsFailed()) {
        staleWaitTask->Release();
        scheduler.Clear();
        if (staleBehaviorStillAlive) {
            context->DestroyObject(staleWaitBehavior);
        }
        context->DestroyObject(messageTarget);
        error = "Async message wait accepted an internal behavior with a stale identity token.";
        return false;
    }
    staleWaitTask->Release();
    scheduler.Clear();
    if (!staleBehaviorStillAlive ||
        context->GetObjectByName(const_cast<CKSTRING>("__CKAS_MessageWait")) != staleWaitBehavior) {
        context->DestroyObject(messageTarget);
        error = "Async message wait cleanup destroyed a behavior after its identity token changed.";
        return false;
    }
    context->DestroyObject(staleWaitBehavior);
    context->DestroyObject(messageTarget);

    WriteAsyncSelfTestStage("empty-race");
    const int intType = engine->GetTypeIdByDecl("int");
    ScriptAsyncTaskBase *emptyRace = scheduler.CreateAggregate(ScriptAsyncTaskKind::Race, intType, {});
    scheduler.Tick();
    if (!emptyRace || !emptyRace->IsFailed()) {
        error = "Async::Race empty-array self-test expected a failed task.";
        if (emptyRace) {
            emptyRace->Release();
        }
        return false;
    }
    emptyRace->Release();

    WriteAsyncSelfTestStage("empty-any");
    ScriptAsyncTaskBase *emptyAny = scheduler.CreateAggregate(ScriptAsyncTaskKind::Any, intType, {});
    scheduler.Tick();
    if (!emptyAny || !emptyAny->IsFailed()) {
        error = "Async::Any empty-array self-test expected a failed task.";
        if (emptyAny) {
            emptyAny->Release();
        }
        return false;
    }
    emptyAny->Release();

    WriteAsyncSelfTestStage("int-result");
    ScriptAsyncTaskBase *intTask = new ScriptAsyncTaskBase(&scheduler, engine, ScriptAsyncTaskKind::Delay, intType);
    int intIn = 42;
    int intOut = 0;
    intTask->CompleteFromAddress(&intIn, intType);
    if (!intTask->CopyResultTo(&intOut, intType, error) || intOut != 42) {
        if (error.empty()) {
            error = "Async int result storage self-test returned the wrong value.";
        }
        intTask->Release();
        return false;
    }
    intTask->Release();

    WriteAsyncSelfTestStage("float-result");
    const int floatType = engine->GetTypeIdByDecl("float");
    ScriptAsyncTaskBase *floatTask = new ScriptAsyncTaskBase(&scheduler, engine, ScriptAsyncTaskKind::Delay, floatType);
    float floatIn = 1.5f;
    float floatOut = 0.0f;
    floatTask->CompleteFromAddress(&floatIn, floatType);
    if (!floatTask->CopyResultTo(&floatOut, floatType, error) || floatOut != 1.5f) {
        if (error.empty()) {
            error = "Async float result storage self-test returned the wrong value.";
        }
        floatTask->Release();
        return false;
    }
    floatTask->Release();

    WriteAsyncSelfTestStage("string-result");
    const int stringType = engine->GetTypeIdByDecl("string");
    ScriptAsyncTaskBase *stringTask = new ScriptAsyncTaskBase(&scheduler, engine, ScriptAsyncTaskKind::Delay, stringType);
    std::string stringIn = "ok";
    std::string stringOut;
    stringTask->CompleteFromAddress(&stringIn, stringType);
    if (!stringTask->CopyResultTo(&stringOut, stringType, error) || stringOut != "ok") {
        if (error.empty()) {
            error = "Async string result storage self-test returned the wrong value.";
        }
        stringTask->Release();
        return false;
    }
    stringTask->Release();

    WriteAsyncSelfTestStage("array-result");
    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("array<int>");
    const int arrayHandleType = engine->GetTypeIdByDecl("array<int>@");
    if (!arrayType || arrayHandleType == 0) {
        error = "Async array result storage self-test could not resolve array<int>@.";
        return false;
    }
    CScriptArray *arrayIn = CScriptArray::Create(arrayType, 1);
    int arrayValue = 7;
    arrayIn->SetValue(0, &arrayValue);
    ScriptAsyncTaskBase *arrayTask = new ScriptAsyncTaskBase(&scheduler, engine, ScriptAsyncTaskKind::Delay, arrayHandleType);
    arrayTask->CompleteWithArray(arrayHandleType, arrayIn);
    CScriptArray *arrayOut = nullptr;
    if (!arrayTask->CopyResultTo(&arrayOut, arrayHandleType, error) ||
        !arrayOut ||
        arrayOut->GetSize() != 1 ||
        *static_cast<int *>(arrayOut->At(0)) != 7) {
        if (error.empty()) {
            error = "Async array handle result storage self-test returned the wrong value.";
        }
        if (arrayOut) {
            arrayOut->Release();
        }
        arrayTask->Release();
        arrayIn->Release();
        return false;
    }
    arrayOut->Release();
    arrayTask->Release();
    arrayIn->Release();

    WriteAsyncSelfTestStage("const-handle-all-result");
    constexpr const char *constAllModuleName = "__CKAS_AsyncConstAllSelfTest";
    if (api->CompileModule(constAllModuleName,
                           "class __CKAS_AsyncConstAllBox { int Value; }\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Async const-handle All self-test compile failed.";
        return false;
    }
    asIScriptModule *constAllModule = engine->GetModule(constAllModuleName, asGM_ONLY_IF_EXISTS);
    asITypeInfo *constBoxType = constAllModule ? constAllModule->GetTypeInfoByDecl("__CKAS_AsyncConstAllBox") : nullptr;
    const int constBoxHandleType =
        constAllModule ? constAllModule->GetTypeIdByDecl("const __CKAS_AsyncConstAllBox@") : 0;
    const int constBoxArrayHandleType =
        constAllModule ? constAllModule->GetTypeIdByDecl("array<const __CKAS_AsyncConstAllBox@>@") : 0;
    if (!constBoxType || constBoxHandleType == 0 || constBoxArrayHandleType == 0) {
        api->UnloadModule(constAllModuleName, nullptr);
        error = "Async const-handle All self-test could not resolve const handle array types.";
        return false;
    }

    auto *constBox = static_cast<asIScriptObject *>(engine->CreateScriptObject(constBoxType));
    if (!constBox) {
        api->UnloadModule(constAllModuleName, nullptr);
        error = "Async const-handle All self-test could not create script object.";
        return false;
    }
    void *constBoxHandle = constBox;
    ScriptAsyncTaskBase *constChild = scheduler.CreateManualTask(constBoxHandleType);
    constChild->CompleteFromAddress(&constBoxHandle, constBoxHandleType);
    std::vector<ScriptAsyncTaskBase *> constChildren{constChild};
    ScriptAsyncTaskBase *constAllTask =
        scheduler.CreateAggregate(ScriptAsyncTaskKind::All, constBoxArrayHandleType, constChildren);
    scheduler.Tick();
    CScriptArray *constArrayOut = nullptr;
    const bool constAllCopied =
        constAllTask->CopyResultTo(&constArrayOut, constBoxArrayHandleType, error) &&
        constArrayOut &&
        constArrayOut->GetSize() == 1 &&
        *static_cast<void **>(constArrayOut->At(0)) == constBox;
    if (constArrayOut) {
        constArrayOut->Release();
    }
    constAllTask->Release();
    constChild->Release();
    constBox->Release();
    scheduler.Clear();
    if (!constAllCopied) {
        if (error.empty()) {
            error = "Async const-handle All self-test returned the wrong array result.";
        }
        api->UnloadModule(constAllModuleName, nullptr);
        return false;
    }
    if (api->UnloadModule(constAllModuleName, &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Async const-handle All self-test could not unload its module.";
        return false;
    }

    WriteAsyncSelfTestStage("clear");
    scheduler.Clear();
    WriteAsyncSelfTestStage("done");
    return true;
}
