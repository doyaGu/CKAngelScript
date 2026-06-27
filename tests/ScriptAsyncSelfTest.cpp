#include "ScriptSelfTests.h"

#include <cstdlib>
#include <fstream>
#include <string>

#include "CKAngelScript.h"
#include "ScriptAsync.h"
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
                           "void __ckas_async_compile_bridge(const CKBehaviorContext &in ctx, BBTask@ bbTask, GraphTask@ graphTask) {\n"
                           "  if (bbTask !is null) { Await(Async::Wait(ctx, bbTask)); }\n"
                           "  if (graphTask !is null) { Await(Async::Wait(ctx, graphTask)); }\n"
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

    WriteAsyncSelfTestStage("gc-cycle");
    CKAngelScriptApi api = CKAngelScriptApi::Get(context);
    if (!api.IsValid()) {
        error = "Async GC self-test could not retrieve CKAngelScript.";
        return false;
    }
    CKAngelScriptResult result = {};

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

    WriteAsyncSelfTestStage("clear");
    scheduler.Clear();
    WriteAsyncSelfTestStage("done");
    return true;
}
