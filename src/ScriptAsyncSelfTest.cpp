#include "ScriptSelfTests.h"

#include <memory>
#include <string>

#include "ScriptAsync.h"
#include "ScriptCache.h"
#include "ScriptManager.h"

#include "add_on/scriptarray/scriptarray.h"

bool RunScriptAsyncSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Async self-test requires a CKContext and AngelScript engine.";
        return false;
    }

    const char *source =
        "class __CKAS_AsyncBox { int Value; }\n"
        "funcdef __CKAS_AsyncBox@ __CKAS_AsyncBoxFunc();\n"
        "void __ckas_async_void() {}\n"
        "int __ckas_async_int() { return 42; }\n"
        "float __ckas_async_float() { return 1.5f; }\n"
        "string __ckas_async_string() { return \"ok\"; }\n"
        "CKObject@ __ckas_async_object() { return null; }\n"
        "__CKAS_AsyncBox@ __ckas_async_box() { __CKAS_AsyncBox@ b = __CKAS_AsyncBox(); b.Value = 7; return b; }\n"
        "void __ckas_async_compile_probe(const CKBehaviorContext &in ctx, BBTask@ bbTask, GraphTask@ graphTask) {\n"
        "  AsyncTask<void>@ delay = Async::Delay(2);\n"
        "  bool done = delay.IsDone();\n"
        "  string err = delay.Error();\n"
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
        "  AsyncTask<__CKAS_AsyncBox@>@ tb;\n"
        "  Async::Spawn(__CKAS_AsyncBoxFunc(__ckas_async_box), @tb);\n"
        "  __CKAS_AsyncBox@ b = null;\n"
        "  Await(@tb, @b);\n"
        "  array<AsyncTask<int>@> ints;\n"
        "  ints.insertLast(@ti);\n"
        "  AsyncTask<array<int>@>@ allInts = Async::All(ints);\n"
        "  AsyncTask<int>@ raceInt = Async::Race(ints);\n"
        "  AsyncTask<int>@ anyInt = Async::Any(ints);\n"
        "  AsyncTask<int>@ genericInt;\n"
        "  Async::Create(AsyncIntFunc(__ckas_async_int), @genericInt);\n"
        "  Await(genericInt, i);\n"
        "  AsyncTask<array<int>@>@ genericAllInts;\n"
        "  Async::All(ints, @genericAllInts);\n"
        "  array<int>@ allValues = null;\n"
        "  Await(@genericAllInts, @allValues);\n"
        "  Async::Race(ints, @genericInt);\n"
        "  Async::Any(ints, @genericInt);\n"
        "  array<AsyncTask<__CKAS_AsyncBox@>@> boxes;\n"
        "  boxes.insertLast(@tb);\n"
        "  AsyncTask<array<__CKAS_AsyncBox@>@>@ allBoxes;\n"
        "  Async::All(boxes, @allBoxes);\n"
        "  AsyncTask<__CKAS_AsyncBox@>@ firstBox;\n"
        "  Async::Race(boxes, @firstBox);\n"
        "  Async::Any(boxes, @firstBox);\n"
        "  if (bbTask !is null) { Await(Async::Wait(ctx, bbTask)); }\n"
        "  if (graphTask !is null) { Await(Async::Wait(ctx, graphTask)); }\n"
        "  delay.Cancel();\n"
        "}\n";

    const std::string moduleName = "__CKAS_AsyncCompileSelfTest";
    std::shared_ptr<CachedScript> script = ScriptManager::GetManager(context)->GetScriptCache().CompileScript(engine, moduleName, source);
    if (!script || !script->module) {
        error = "Async script API compile probe failed.";
        return false;
    }
    ScriptManager::GetManager(context)->GetScriptCache().UnloadScript(moduleName);

    ScriptAsyncScheduler scheduler(ScriptManager::GetManager(context));
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

    scheduler.Clear();
    return true;
}
