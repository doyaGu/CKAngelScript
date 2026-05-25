#include "ScriptSelfTests.h"

#include <string>

#include "AngelScriptManager.h"
#include "ScriptManager.h"
#include "ScriptRuntime.h"
#include "ScriptRuntimeDependency.h"
#include "ScriptRuntimeMetadata.h"

bool RunScriptRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Runtime self-test requires a CKContext and AngelScript engine.";
        return false;
    }
    if (!ScriptRuntimeMetadata::RunScriptRuntimeMetadataSelfTest(error)) {
        return false;
    }
    if (!ScriptRuntimeDependencyResolver::RunScriptRuntimeDependencySelfTest(error)) {
        return false;
    }
    if (!ScriptMessageSelfTest::RunScriptMessageSelfTest(context, engine, error)) {
        return false;
    }
    const char *source =
        "void __ckas_runtime_compile_probe(const ScriptContext &in ctx) {\n"
        "  CKContext@ c = ctx.Context();\n"
        "  float dt = ctx.DeltaTime();\n"
        "  string id = ctx.Id();\n"
        "  string name = ctx.Name();\n"
        "  string version = ctx.Version();\n"
        "  string target = ctx.Target();\n"
        "  string phase = ctx.Phase();\n"
        "  string state = ctx.State();\n"
        "  int generation = ctx.Generation();\n"
        "  uint64 frame = ctx.FrameIndex();\n"
        "  string manifestPath = ctx.Manifest();\n"
        "  string entryPath = ctx.Entry();\n"
        "  string custom = ctx.Metadata(\"custom\", \"fallback\");\n"
        "  int metadataCount = ctx.MetadataCount();\n"
        "  string metadataKey = ctx.MetadataKey(0);\n"
        "  string metadataValue = ctx.MetadataValue(0);\n"
        "  CKBehaviorContext behaviorContext = ctx.ToBehaviorContext();\n"
        "  BehaviorGraph@ graph = Behavior::Graph(ctx, \"__missing__\");\n"
        "  BehaviorRef@ behavior = Behavior::Find(ctx, \"__missing__\");\n"
        "  BBDecl@ decl = BB::Require(ctx, \"__missing__\");\n"
        "  int bbCount = BB::Count(ctx);\n"
        "  ParamTypeInfo@ param = Param::Find(ctx, \"int\");\n"
        "  array<string>@ scripts = Runtime::List(ctx);\n"
        "  array<RuntimeScriptInfo>@ infos = Runtime::ListInfo(ctx);\n"
        "  RuntimeScriptInfo info = Runtime::Info(ctx, \"ckas.runtime.smoke\");\n"
        "  string root = info.Root();\n"
        "  string manifest = info.Manifest();\n"
        "  string entry = info.Entry();\n"
        "  bool exists = info.Exists();\n"
        "  string infoId = info.Id();\n"
        "  string infoState = info.State();\n"
        "  int tagCount = info.TagCount();\n"
        "  string firstTag = info.Tag(0);\n"
        "  string runtimeVersion = Runtime::Version(ctx, \"ckas.runtime.smoke\");\n"
        "  string runtimeMetadata = Runtime::Metadata(ctx, \"ckas.runtime.smoke\", \"custom\", \"fallback\");\n"
        "  array<string>@ deps = Runtime::Dependencies(ctx, \"ckas.runtime.smoke\");\n"
        "  array<RuntimeDependencyInfo>@ requiredDeps = Runtime::RequiredDependencies(ctx, \"ckas.runtime.smoke\");\n"
        "  array<RuntimeDependencyInfo>@ optionalDeps = Runtime::OptionalDependencies(ctx, \"ckas.runtime.smoke\");\n"
        "}\n"
        "class __CKAS_RuntimeLifecycleProbe {\n"
        "  void OnLoad(const ScriptContext &in ctx) {}\n"
        "  void Awake(const ScriptContext &in ctx) {}\n"
        "  void OnEnable(const ScriptContext &in ctx) {}\n"
        "  void Start(const ScriptContext &in ctx) {}\n"
        "  void Update(const ScriptContext &in ctx) {}\n"
        "  void OnPostLoad(const ScriptContext &in ctx) {}\n"
        "  void OnPostProcess(const ScriptContext &in ctx) {}\n"
        "  void OnDisable(const ScriptContext &in ctx) {}\n"
        "  void OnDestroy(const ScriptContext &in ctx) {}\n"
        "  void OnReset(const ScriptContext &in ctx) {}\n"
        "  void OnPause(const ScriptContext &in ctx) {}\n"
        "  void OnResume(const ScriptContext &in ctx) {}\n"
        "}\n";
    AngelScriptManager *manager = AngelScriptManager::GetManager(context);
    if (!manager) {
        error = "Runtime self-test could not retrieve AngelScriptManager.";
        return false;
    }
    const char *moduleName = "__CKAS_RuntimeCompileSelfTest";
    AngelScriptResult result = {};
    if (manager->CompileModule(moduleName, source, true, &result) != ANGELSCRIPT_STATUS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Runtime script API compile probe failed.";
        return false;
    }
    manager->UnloadModule(moduleName, nullptr);
    const char *oldApiSource =
        "void __ckas_runtime_old_api_probe(const ScriptContext &in ctx) {\n"
        "  string id = ctx.ScriptId();\n"
        "}\n";
    const char *oldApiModuleName = "__CKAS_RuntimeOldApiNegativeSelfTest";
    result = {};
    if (manager->CompileModule(oldApiModuleName, oldApiSource, true, &result) == ANGELSCRIPT_STATUS_OK) {
        manager->UnloadModule(oldApiModuleName, nullptr);
        error = "Runtime script old ScriptContext API unexpectedly compiled.";
        return false;
    }
    manager->UnloadModule(oldApiModuleName, nullptr);
    return true;
}
