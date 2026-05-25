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
        "void __ckas_runtime_compile_probe(const ScriptRuntimeContext &in ctx) {\n"
        "  CKContext@ c = ctx.Context();\n"
        "  float dt = ctx.DeltaTime();\n"
        "  string id = ctx.ScriptId();\n"
        "  string name = ctx.ScriptName();\n"
        "  string version = ctx.ScriptVersion();\n"
        "  string phase = ctx.Phase();\n"
        "  string state = ctx.State();\n"
        "  int generation = ctx.Generation();\n"
        "  uint64 frame = ctx.FrameIndex();\n"
        "  string manifestPath = ctx.ManifestPath();\n"
        "  string entryPath = ctx.EntryPath();\n"
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
        "  void OnLoad(const ScriptRuntimeContext &in ctx) {}\n"
        "  void Awake(const ScriptRuntimeContext &in ctx) {}\n"
        "  void OnEnable(const ScriptRuntimeContext &in ctx) {}\n"
        "  void Start(const ScriptRuntimeContext &in ctx) {}\n"
        "  void Update(const ScriptRuntimeContext &in ctx) {}\n"
        "  void OnPostLoad(const ScriptRuntimeContext &in ctx) {}\n"
        "  void OnPostProcess(const ScriptRuntimeContext &in ctx) {}\n"
        "  void OnDisable(const ScriptRuntimeContext &in ctx) {}\n"
        "  void OnDestroy(const ScriptRuntimeContext &in ctx) {}\n"
        "  void OnReset(const ScriptRuntimeContext &in ctx) {}\n"
        "  void OnPause(const ScriptRuntimeContext &in ctx) {}\n"
        "  void OnResume(const ScriptRuntimeContext &in ctx) {}\n"
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
    return true;
}
