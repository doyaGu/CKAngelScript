#include "AngelScriptExecutor.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "CKAll.h"
#include "Logger.h"
#include "ScriptCache.h"
#include "ScriptBridgeCommon.h"
#include "ScriptInvoker.h"
#include "ScriptManager.h"
#include "ScriptParameterConversion.h"
#include "ScriptRuntime.h"

namespace {

constexpr int LOCAL_STATE = 0;
constexpr int SETTING_SCRIPT = 1;
constexpr int SETTING_CLASS = 2;
constexpr int SETTING_INTERFACE = 3;
constexpr int SETTING_STRICT_INTERFACE = 4;
constexpr int SETTING_TRACE = 5;
constexpr int USER_LOCAL_START = SETTING_TRACE + 1;

enum class ExecutorSlotKind {
    Input,
    Output,
    Pin,
    Pout,
    Local,
};

enum class ExecutorReadyStatus {
    Ready,
    LifecycleOnly,
    Suspended,
    Failed,
};

enum class LifecycleInvokeStatus {
    Finished,
    Suspended,
    Failed,
};

struct ExecutorSlotSpec {
    ExecutorSlotKind Kind = ExecutorSlotKind::Input;
    std::string Name;
    CKGUID Guid;
    std::string TypeName;
    bool HasDefault = false;
    std::string DefaultText;
};

struct ExecutorInterface {
    std::vector<ExecutorSlotSpec> Inputs;
    std::vector<ExecutorSlotSpec> Outputs;
    std::vector<ExecutorSlotSpec> Pins;
    std::vector<ExecutorSlotSpec> Pouts;
    std::vector<ExecutorSlotSpec> Locals;
};

struct ScriptExecutorState {
    CK_ID BehaviorId = 0;
    CKBehavior *Behavior = nullptr;
    ScriptManager *Manager = nullptr;
    ScriptInvoker *Invoker = nullptr;
    asIScriptObject *Object = nullptr;
    asIScriptFunction *Execute = nullptr;
    asIScriptFunction *OnLoad = nullptr;
    asIScriptFunction *Awake = nullptr;
    asIScriptFunction *Start = nullptr;
    asIScriptFunction *OnDestroy = nullptr;
    asIScriptFunction *OnReset = nullptr;
    ScriptBehaviorArgumentKind ExecuteArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind OnLoadArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind AwakeArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind StartArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind OnDestroyArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    ScriptBehaviorArgumentKind OnResetArgument = ScriptBehaviorArgumentKind::BehaviorContext;

    std::string ScriptId;
    std::string ModuleName;
    std::string ClassName;
    std::string InterfaceText;
    bool Loaded = false;
    bool Failed = false;
    bool OnLoadCalled = false;
    bool AwakeCalled = false;
    bool StartCalled = false;
    std::vector<int> SuspendedInputs;
    std::string ActiveLifecycleName;

    static void ReleaseFunction(asIScriptFunction *&function) {
        if (function) {
            function->Release();
            function = nullptr;
        }
    }

    void ReleaseCachedMethods() {
        ReleaseFunction(Execute);
        ReleaseFunction(OnLoad);
        ReleaseFunction(Awake);
        ReleaseFunction(Start);
        ReleaseFunction(OnDestroy);
        ReleaseFunction(OnReset);
        ExecuteArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        OnLoadArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        AwakeArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        StartArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        OnDestroyArgument = ScriptBehaviorArgumentKind::BehaviorContext;
        OnResetArgument = ScriptBehaviorArgumentKind::BehaviorContext;
    }

    void ReleaseObject() {
        if (Object) {
            Object->Release();
            Object = nullptr;
        }
    }
};

std::vector<ScriptExecutorState *> &ExecutorStates() {
    static std::vector<ScriptExecutorState *> states;
    return states;
}

void RegisterExecutorState(ScriptExecutorState *state) {
    if (!state) {
        return;
    }
    std::vector<ScriptExecutorState *> &states = ExecutorStates();
    if (std::find(states.begin(), states.end(), state) == states.end()) {
        states.push_back(state);
    }
}

void UnregisterExecutorState(ScriptExecutorState *state) {
    std::vector<ScriptExecutorState *> &states = ExecutorStates();
    states.erase(std::remove(states.begin(), states.end(), state), states.end());
}

std::string Trim(const std::string &value) {
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }
    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return value.substr(first, last - first);
}

std::string Unquote(std::string value) {
    value = Trim(value);
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                              (value.front() == '\'' && value.back() == '\''))) {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::string ReadLocalString(CKBehavior *behavior, int index) {
    if (!behavior || index < 0 || index >= behavior->GetLocalParameterCount()) {
        return {};
    }
    CKSTRING value = static_cast<CKSTRING>(behavior->GetLocalParameterReadDataPtr(index));
    return value ? std::string(value) : std::string();
}

bool ReadLocalBool(CKBehavior *behavior, int index, bool defaultValue) {
    if (!behavior || index < 0 || index >= behavior->GetLocalParameterCount()) {
        return defaultValue;
    }
    CKBOOL value = defaultValue ? TRUE : FALSE;
    if (behavior->GetLocalParameterValue(index, &value) != CK_OK) {
        return defaultValue;
    }
    return value != FALSE;
}

void ConsoleError(const CKBehaviorContext &ctx, const std::string &message) {
    if (ctx.Context) {
        ctx.Context->OutputToConsoleEx(const_cast<char *>("[AngelScript Executor] %s"), message.c_str());
    }
    LOG_ERROR("[AngelScript Executor] %s", message.c_str());
}

std::vector<std::string> SplitInterfaceLine(const std::string &line) {
    std::vector<std::string> tokens;
    std::string current;
    bool quoted = false;
    char quote = '\0';
    for (char ch : line) {
        if (quoted) {
            current.push_back(ch);
            if (ch == quote) {
                quoted = false;
            }
            continue;
        }
        if (ch == '"' || ch == '\'') {
            quoted = true;
            quote = ch;
            current.push_back(ch);
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::string TokenValue(const std::vector<std::string> &tokens, const char *key) {
    const std::string prefix = std::string(key) + "=";
    for (const std::string &token : tokens) {
        if (token.rfind(prefix, 0) == 0) {
            return Unquote(token.substr(prefix.size()));
        }
    }
    return {};
}

bool HasToken(const std::vector<std::string> &tokens, const char *key) {
    const std::string prefix = std::string(key) + "=";
    for (const std::string &token : tokens) {
        if (token.rfind(prefix, 0) == 0) {
            return true;
        }
    }
    return false;
}

bool IsIntegerToken(const std::string &token) {
    if (token.empty()) {
        return false;
    }
    return std::all_of(token.begin(), token.end(), [](char ch) {
        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
    });
}

int ParsePositiveIndexToken(const std::string &token) {
    int value = 0;
    for (char ch : token) {
        value = value * 10 + (ch - '0');
    }
    return value;
}

CKGUID ResolveSlotGuid(CKContext *context, const std::string &guidText, const std::string &typeName) {
    CKGUID guid;
    if (!guidText.empty() && ParseScriptGuidString(guidText, guid)) {
        return guid;
    }
    if (!typeName.empty()) {
        return ScriptResolveParameterGuid(context, typeName, CKGUID());
    }
    return CKGUID();
}

bool ParseInterface(CKContext *context, const std::string &text, ExecutorInterface &interfaceShape, std::string &error) {
    interfaceShape = ExecutorInterface();
    std::istringstream input(text);
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        std::vector<std::string> tokens = SplitInterfaceLine(trimmed);
        if (tokens.empty()) {
            continue;
        }

        ExecutorSlotKind kind;
        std::vector<ExecutorSlotSpec> *target = nullptr;
        if (tokens[0] == "input") {
            kind = ExecutorSlotKind::Input;
            target = &interfaceShape.Inputs;
        } else if (tokens[0] == "output") {
            kind = ExecutorSlotKind::Output;
            target = &interfaceShape.Outputs;
        } else if (tokens[0] == "pin") {
            kind = ExecutorSlotKind::Pin;
            target = &interfaceShape.Pins;
        } else if (tokens[0] == "pout") {
            kind = ExecutorSlotKind::Pout;
            target = &interfaceShape.Pouts;
        } else if (tokens[0] == "local") {
            kind = ExecutorSlotKind::Local;
            target = &interfaceShape.Locals;
        } else if (tokens[0] == "root") {
            continue;
        } else {
            error = "Unknown interface entry at line " + std::to_string(lineNumber) + ": " + tokens[0];
            return false;
        }

        int cursor = 1;
        if (cursor < static_cast<int>(tokens.size()) && IsIntegerToken(tokens[cursor])) {
            const int declaredIndex = ParsePositiveIndexToken(tokens[cursor]);
            const int expectedIndex = static_cast<int>(target->size());
            if (declaredIndex != expectedIndex) {
                error = "Interface entry at line " + std::to_string(lineNumber) +
                    " declares index " + std::to_string(declaredIndex) +
                    ", expected " + std::to_string(expectedIndex) + ".";
                return false;
            }
            ++cursor;
        }

        std::string name = TokenValue(tokens, "name");
        if (name.empty() && cursor < static_cast<int>(tokens.size()) && tokens[cursor].find('=') == std::string::npos) {
            name = Unquote(tokens[cursor]);
        }
        if (name.empty()) {
            error = "Interface entry at line " + std::to_string(lineNumber) + " has no name.";
            return false;
        }

        ExecutorSlotSpec spec;
        spec.Kind = kind;
        spec.Name = name;
        spec.TypeName = TokenValue(tokens, "type");
        spec.Guid = ResolveSlotGuid(context, TokenValue(tokens, "guid"), spec.TypeName);
        spec.HasDefault = HasToken(tokens, "default") || HasToken(tokens, "value");
        spec.DefaultText = HasToken(tokens, "default") ? TokenValue(tokens, "default") : TokenValue(tokens, "value");

        if ((kind == ExecutorSlotKind::Pin ||
             kind == ExecutorSlotKind::Pout ||
             kind == ExecutorSlotKind::Local) &&
            spec.Guid == CKGUID()) {
            error = "Interface parameter at line " + std::to_string(lineNumber) + " has no resolvable guid/type.";
            return false;
        }

        target->push_back(std::move(spec));
    }
    return true;
}

std::string ObjectName(CKObject *object) {
    CKSTRING name = object ? object->GetName() : nullptr;
    return name ? std::string(name) : std::string();
}

void SyncInputCount(CKBehavior *behavior, const std::vector<ExecutorSlotSpec> &inputs) {
    while (behavior->GetInputCount() > static_cast<int>(inputs.size())) {
        behavior->DeleteInput(behavior->GetInputCount() - 1);
    }
    while (behavior->GetInputCount() < static_cast<int>(inputs.size())) {
        const ExecutorSlotSpec &spec = inputs[behavior->GetInputCount()];
        behavior->AddInput(const_cast<CKSTRING>(spec.Name.c_str()));
    }
    for (int i = 0; i < static_cast<int>(inputs.size()); ++i) {
        CKBehaviorIO *io = behavior->GetInput(i);
        if (io && ObjectName(io) != inputs[i].Name) {
            io->SetName(const_cast<CKSTRING>(inputs[i].Name.c_str()));
        }
    }
}

void SyncOutputCount(CKBehavior *behavior, const std::vector<ExecutorSlotSpec> &outputs) {
    while (behavior->GetOutputCount() > static_cast<int>(outputs.size())) {
        behavior->DeleteOutput(behavior->GetOutputCount() - 1);
    }
    while (behavior->GetOutputCount() < static_cast<int>(outputs.size())) {
        const ExecutorSlotSpec &spec = outputs[behavior->GetOutputCount()];
        behavior->AddOutput(const_cast<CKSTRING>(spec.Name.c_str()));
    }
    for (int i = 0; i < static_cast<int>(outputs.size()); ++i) {
        CKBehaviorIO *io = behavior->GetOutput(i);
        if (io && ObjectName(io) != outputs[i].Name) {
            io->SetName(const_cast<CKSTRING>(outputs[i].Name.c_str()));
        }
    }
}

void SyncPinCount(CKBehavior *behavior, const std::vector<ExecutorSlotSpec> &pins) {
    while (behavior->GetInputParameterCount() > static_cast<int>(pins.size())) {
        CKDestroyObject(behavior->RemoveInputParameter(behavior->GetInputParameterCount() - 1));
    }
    while (behavior->GetInputParameterCount() < static_cast<int>(pins.size())) {
        const ExecutorSlotSpec &spec = pins[behavior->GetInputParameterCount()];
        behavior->CreateInputParameter(const_cast<CKSTRING>(spec.Name.c_str()), spec.Guid);
    }
    for (int i = 0; i < static_cast<int>(pins.size()); ++i) {
        CKParameterIn *param = behavior->GetInputParameter(i);
        if (!param) {
            continue;
        }
        if (param->GetGUID() != pins[i].Guid) {
            param->SetGUID(pins[i].Guid, TRUE, const_cast<CKSTRING>(pins[i].Name.c_str()));
        } else if (ObjectName(param) != pins[i].Name) {
            param->SetName(const_cast<CKSTRING>(pins[i].Name.c_str()));
        }
    }
}

void SyncPoutCount(CKBehavior *behavior, const std::vector<ExecutorSlotSpec> &pouts) {
    while (behavior->GetOutputParameterCount() > static_cast<int>(pouts.size())) {
        CKDestroyObject(behavior->RemoveOutputParameter(behavior->GetOutputParameterCount() - 1));
    }
    while (behavior->GetOutputParameterCount() < static_cast<int>(pouts.size())) {
        const ExecutorSlotSpec &spec = pouts[behavior->GetOutputParameterCount()];
        behavior->CreateOutputParameter(const_cast<CKSTRING>(spec.Name.c_str()), spec.Guid);
    }
    for (int i = 0; i < static_cast<int>(pouts.size()); ++i) {
        CKParameterOut *param = behavior->GetOutputParameter(i);
        if (!param) {
            continue;
        }
        if (param->GetGUID() != pouts[i].Guid) {
            param->SetGUID(pouts[i].Guid);
        }
        if (ObjectName(param) != pouts[i].Name) {
            param->SetName(const_cast<CKSTRING>(pouts[i].Name.c_str()));
        }
    }
}

bool ValidateIoNames(CKBehavior *behavior,
                     const std::vector<ExecutorSlotSpec> &specs,
                     bool input,
                     std::string &error) {
    for (int i = 0; i < static_cast<int>(specs.size()); ++i) {
        CKBehaviorIO *io = input ? behavior->GetInput(i) : behavior->GetOutput(i);
        if (!io) {
            error = std::string(input ? "Input" : "Output") + " slot " + std::to_string(i) + " is missing.";
            return false;
        }
        const std::string actual = ObjectName(io);
        if (actual != specs[i].Name) {
            error = std::string(input ? "Input" : "Output") + " slot " + std::to_string(i) +
                " name mismatch. Expected '" + specs[i].Name + "', got '" + actual + "'.";
            return false;
        }
    }
    return true;
}

bool ValidateInputParameterGuids(const CKBehaviorContext &ctx,
                                 CKBehavior *behavior,
                                 const std::vector<ExecutorSlotSpec> &specs,
                                 std::string &error) {
    for (int i = 0; i < static_cast<int>(specs.size()); ++i) {
        CKParameterIn *param = behavior->GetInputParameter(i);
        if (!param) {
            error = "Input parameter slot " + std::to_string(i) + " is missing.";
            return false;
        }
        const std::string actualName = ObjectName(param);
        if (actualName != specs[i].Name) {
            error = "Input parameter slot " + std::to_string(i) +
                " name mismatch. Expected '" + specs[i].Name +
                "', got '" + actualName + "'.";
            return false;
        }
        if (param->GetGUID() != specs[i].Guid) {
            error = "Input parameter slot " + std::to_string(i) +
                " type mismatch. Expected " +
                DescribeScriptParamType(ctx.Context, specs[i].Guid) + ", got " +
                DescribeScriptParamType(ctx.Context, param->GetGUID()) + ".";
            return false;
        }
    }
    return true;
}

bool ValidateOutputParameterGuids(const CKBehaviorContext &ctx,
                                  CKBehavior *behavior,
                                  const std::vector<ExecutorSlotSpec> &specs,
                                  std::string &error) {
    for (int i = 0; i < static_cast<int>(specs.size()); ++i) {
        CKParameterOut *param = behavior->GetOutputParameter(i);
        if (!param) {
            error = "Output parameter slot " + std::to_string(i) + " is missing.";
            return false;
        }
        const std::string actualName = ObjectName(param);
        if (actualName != specs[i].Name) {
            error = "Output parameter slot " + std::to_string(i) +
                " name mismatch. Expected '" + specs[i].Name +
                "', got '" + actualName + "'.";
            return false;
        }
        if (param->GetGUID() != specs[i].Guid) {
            error = "Output parameter slot " + std::to_string(i) +
                " type mismatch. Expected " +
                DescribeScriptParamType(ctx.Context, specs[i].Guid) + ", got " +
                DescribeScriptParamType(ctx.Context, param->GetGUID()) + ".";
            return false;
        }
    }
    return true;
}

bool ValidateInterface(const CKBehaviorContext &ctx,
                      CKBehavior *behavior,
                      const ExecutorInterface &interfaceShape,
                      std::string &error) {
    return ValidateIoNames(behavior, interfaceShape.Inputs, true, error) &&
           ValidateIoNames(behavior, interfaceShape.Outputs, false, error) &&
           ValidateInputParameterGuids(ctx, behavior, interfaceShape.Pins, error) &&
           ValidateOutputParameterGuids(ctx, behavior, interfaceShape.Pouts, error);
}

std::vector<ScriptBridgeFrameLocalDecl> FrameLocalDecls(const std::vector<ExecutorSlotSpec> &locals) {
    std::vector<ScriptBridgeFrameLocalDecl> decls;
    decls.reserve(locals.size());
    for (const ExecutorSlotSpec &spec : locals) {
        ScriptBridgeFrameLocalDecl decl;
        decl.Name = spec.Name;
        decl.TypeGuid = spec.Guid;
        decl.HasDefault = spec.HasDefault;
        decl.DefaultText = spec.DefaultText;
        decls.push_back(std::move(decl));
    }
    return decls;
}

ScriptBehaviorBridge *BehaviorBridgeFromContext(const CKBehaviorContext &ctx) {
    ScriptManager *manager = ctx.Context ? ScriptManager::GetManager(ctx.Context) : nullptr;
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

bool ApplyInterface(const CKBehaviorContext &ctx,
                   CKBehavior *behavior,
                   const std::string &interfaceText,
                   bool strict,
                   std::string &error) {
    if (!behavior) {
        return true;
    }
    ScriptBehaviorBridge *bridge = BehaviorBridgeFromContext(ctx);
    if (interfaceText.empty()) {
        if (bridge) {
            bridge->ClearFrameLocalInterface(behavior->GetID(), false);
        }
        return true;
    }
    ExecutorInterface interfaceShape;
    if (!ParseInterface(ctx.Context, interfaceText, interfaceShape, error)) {
        return false;
    }
    SyncInputCount(behavior, interfaceShape.Inputs);
    SyncOutputCount(behavior, interfaceShape.Outputs);
    SyncPinCount(behavior, interfaceShape.Pins);
    SyncPoutCount(behavior, interfaceShape.Pouts);

    if (!bridge) {
        error = "Executor interface locals require ScriptBehaviorBridge.";
        return false;
    }
    if (!bridge->ApplyFrameLocalInterface(behavior,
                                          FrameLocalDecls(interfaceShape.Locals),
                                          USER_LOCAL_START,
                                          strict,
                                          error)) {
        return false;
    }
    if (strict && !ValidateInterface(ctx, behavior, interfaceShape, error)) {
        return false;
    }
    return true;
}

ScriptExecutorState *GetState(CKBehavior *behavior) {
    if (!behavior || behavior->GetLocalParameterCount() <= LOCAL_STATE) {
        return nullptr;
    }
    ScriptExecutorState *state = nullptr;
    behavior->GetLocalParameterValue(LOCAL_STATE, &state);
    return state;
}

void SetState(CKBehavior *behavior, ScriptExecutorState *state) {
    if (behavior && behavior->GetLocalParameterCount() > LOCAL_STATE) {
        behavior->SetLocalParameterValue(LOCAL_STATE, &state);
    }
    RegisterExecutorState(state);
}

bool IsExecutorMethod(asIScriptFunction *method, bool execute, ScriptBehaviorArgumentKind &argumentKind) {
    if (!method || method->GetParamCount() != 1) {
        return false;
    }
    const bool returnOk = execute
        ? (method->GetReturnTypeId() == asTYPEID_VOID || method->GetReturnTypeId() == asTYPEID_INT32)
        : method->GetReturnTypeId() == asTYPEID_VOID;
    if (!returnOk) {
        return false;
    }
    int typeId = 0;
    asDWORD flags = 0;
    if (method->GetParam(0, &typeId, &flags) < 0) {
        return false;
    }
    asIScriptEngine *engine = method->GetEngine();
    const int expectedContext = engine ? engine->GetTypeIdByDecl("CKBehaviorContext") : -1;
    const int expectedFrame = engine ? engine->GetTypeIdByDecl("BehaviorFrame@") : -1;
    const asDWORD refFlags = flags & asTM_INOUTREF;
    if (typeId == expectedFrame && refFlags == 0) {
        argumentKind = ScriptBehaviorArgumentKind::BehaviorFrame;
        return true;
    }
    if (typeId == expectedContext && refFlags == asTM_INREF && (flags & asTM_CONST) != 0) {
        argumentKind = ScriptBehaviorArgumentKind::BehaviorContext;
        return true;
    }
    return false;
}

bool CacheLifecycle(asITypeInfo *type,
                    const char *name,
                    bool required,
                    asIScriptFunction *&out,
                    ScriptBehaviorArgumentKind &argumentKind,
                    std::string &error) {
    out = nullptr;
    argumentKind = ScriptBehaviorArgumentKind::BehaviorContext;
    if (!type) {
        return false;
    }
    const bool execute = std::strcmp(name, "Execute") == 0;
    bool sawName = false;
    std::string candidateDecls;
    asIScriptFunction *contextMethod = nullptr;
    asIScriptFunction *frameMethod = nullptr;
    for (asUINT i = 0; i < type->GetMethodCount(); ++i) {
        asIScriptFunction *method = type->GetMethodByIndex(i);
        if (!method || std::strcmp(method->GetName(), name) != 0) {
            continue;
        }
        sawName = true;
        if (!candidateDecls.empty()) {
            candidateDecls += "; ";
        }
        candidateDecls += method->GetDeclaration(false, false, true);
        ScriptBehaviorArgumentKind matchedKind = ScriptBehaviorArgumentKind::BehaviorContext;
        if (IsExecutorMethod(method, execute, matchedKind)) {
            if (matchedKind == ScriptBehaviorArgumentKind::BehaviorFrame && !frameMethod) {
                frameMethod = method;
            } else if (matchedKind == ScriptBehaviorArgumentKind::BehaviorContext && !contextMethod) {
                contextMethod = method;
            }
        }
    }

    if (frameMethod || contextMethod) {
        out = frameMethod ? frameMethod : contextMethod;
        argumentKind = frameMethod ? ScriptBehaviorArgumentKind::BehaviorFrame : ScriptBehaviorArgumentKind::BehaviorContext;
        out->AddRef();
        return true;
    }

    if (sawName || required) {
        error = execute
            ? "Invalid or missing executor method. Expected: void/int Execute(BehaviorFrame@ frame) or void/int Execute(const CKBehaviorContext &in ctx)"
            : std::string("Invalid lifecycle method. Expected: void ") + name + "(BehaviorFrame@ frame) or void " + name + "(const CKBehaviorContext &in ctx)";
        if (!candidateDecls.empty()) {
            error += ". Found candidates: ";
            error += candidateDecls;
        }
        return false;
    }
    return true;
}

LifecycleInvokeStatus InvokeLifecycle(const CKBehaviorContext &ctx,
                                      ScriptExecutorState *state,
                                      asIScriptFunction *method,
                                      ScriptBehaviorArgumentKind argumentKind,
                                      const char *name) {
    if (!state || !method) {
        return LifecycleInvokeStatus::Finished;
    }
    if (state->Invoker && !state->Invoker->IsContextSuspended()) {
        state->ActiveLifecycleName = name ? name : "";
    }
    const ScriptInvocationStatus status = state->Invoker->ExecuteObjectMethodStatus(state->Object, method, ctx, argumentKind);
    if (status == ScriptInvocationStatus::Suspended) {
        return LifecycleInvokeStatus::Suspended;
    }
    state->ActiveLifecycleName.clear();
    if (status == ScriptInvocationStatus::Failed) {
        ConsoleError(ctx, std::string(name) + " failed: " + state->Invoker->GetErrorMessage());
        state->Failed = true;
        return LifecycleInvokeStatus::Failed;
    }
    return LifecycleInvokeStatus::Finished;
}

void InvokeOnDestroy(const CKBehaviorContext &ctx, ScriptExecutorState *state) {
    if (state->Invoker && state->Object && state->OnDestroy) {
        if (state->Invoker->IsContextSuspended()) {
            state->Invoker->AbortContext();
            state->SuspendedInputs.clear();
            state->ActiveLifecycleName.clear();
        }
        const ScriptInvocationStatus status = state->Invoker->ExecuteObjectMethodStatus(
            state->Object,
            state->OnDestroy,
            ctx,
            state->OnDestroyArgument);
        if (status == ScriptInvocationStatus::Suspended) {
            state->Invoker->AbortContext();
        }
    }
}

void DestroyState(const CKBehaviorContext &ctx, ScriptExecutorState *state) {
    if (!state) {
        return;
    }
    UnregisterExecutorState(state);
    InvokeOnDestroy(ctx, state);
    if (state->Invoker) {
        state->Invoker->AbortContext();
    }
    if (ScriptBehaviorBridge *bridge = BehaviorBridgeFromContext(ctx)) {
        bridge->ClearFrameLocalInterface(state->BehaviorId, true);
    }
    state->ReleaseCachedMethods();
    state->ReleaseObject();
    delete state->Invoker;
    state->Invoker = nullptr;
    delete state;
}

void ReleaseRuntimeState(ScriptExecutorState *state) {
    if (!state) {
        return;
    }
    if (state->Invoker) {
        state->Invoker->AbortContext();
    }
    state->ReleaseCachedMethods();
    state->ReleaseObject();
    delete state->Invoker;
    state->Invoker = nullptr;
    state->Loaded = false;
    state->OnLoadCalled = false;
    state->AwakeCalled = false;
    state->StartCalled = false;
    state->SuspendedInputs.clear();
    state->ActiveLifecycleName.clear();
}

bool FailSetup(const CKBehaviorContext &ctx, ScriptExecutorState *state, const std::string &message) {
    ConsoleError(ctx, message);
    ReleaseRuntimeState(state);
    if (state) {
        state->Failed = true;
    }
    return false;
}

bool IdentityChanged(ScriptExecutorState *state,
                     const std::string &scriptId,
                     const std::string &moduleName,
                     const std::string &className,
                     const std::string &interfaceText) {
    return !state ||
           state->ScriptId != scriptId ||
           state->ModuleName != moduleName ||
           state->ClassName != className ||
           state->InterfaceText != interfaceText;
}

bool AttachScriptEntryModule(ScriptManager *manager,
                             ScriptInvoker *invoker,
                             const std::string &scriptId,
                             const std::string &scriptModule,
                             std::string &error) {
    if (!manager || !invoker) {
        error = "AngelScript manager is not available.";
        return false;
    }

    ScriptRuntime *runtime = manager->GetRuntime();
    if (!runtime) {
        error = "Script module registry is not available.";
        return false;
    }
    auto describeModule = [&](const std::string &moduleName) {
        std::ostringstream out;
        std::shared_ptr<CachedScript> cached = manager->GetCachedScript(moduleName.c_str());
        out << "module='" << moduleName << "'"
            << " cached=" << (cached ? "yes" : "no")
            << " cachedModule=" << (cached && cached->module ? "yes" : "no")
            << " engineModule=" << (manager->GetModule(moduleName.c_str()) ? "yes" : "no");
        return out.str();
    };

    std::ostringstream details;
    details << "scriptId='" << scriptId << "'";
    const RuntimeScriptInfo info = runtime->Info(scriptId);
    details << "; module.exists=" << (info.Exists() ? "yes" : "no")
            << " host='" << info.Host() << "'"
            << " loaded=" << (info.Loaded() ? "yes" : "no")
            << " failed=" << (info.Failed() ? "yes" : "no")
            << " state='" << info.State() << "'"
            << " error='" << info.Error() << "'";

    if (scriptModule.empty()) {
        details << "; module=<none>";
        error = details.str();
        return false;
    }

    if (invoker->SetScript(scriptModule.c_str())) {
        return true;
    }
    const std::string scriptError = invoker->GetErrorMessage();

    if (!scriptModule.empty()) {
        details << "; module " << describeModule(scriptModule);
        if (!scriptError.empty()) {
            details << " scriptAttachError='" << scriptError << "'";
        }
    }
    error = details.str();
    return false;
}

ExecutorReadyStatus EnsureReady(const CKBehaviorContext &ctx, ScriptExecutorState *state) {
    CKBehavior *behavior = ctx.Behavior;
    if (!behavior || !state) {
        return ExecutorReadyStatus::Failed;
    }
    ScriptManager *manager = ctx.Context ? ScriptManager::GetManager(ctx.Context) : nullptr;
    if (!manager) {
        ConsoleError(ctx, "AngelScript manager is not available.");
        return ExecutorReadyStatus::Failed;
    }
    state->Manager = manager;

    const std::string scriptId = Trim(ReadLocalString(behavior, SETTING_SCRIPT));
    const std::string className = Trim(ReadLocalString(behavior, SETTING_CLASS));
    const std::string interfaceText = ReadLocalString(behavior, SETTING_INTERFACE);
    if (scriptId.empty() || className.empty()) {
        ConsoleError(ctx, "Script module id and Class settings are required.");
        return ExecutorReadyStatus::Failed;
    }

    ScriptRuntime *runtime = manager->GetRuntime();
    const std::string moduleName = runtime ? runtime->ModuleName(scriptId) : std::string();
    if (IdentityChanged(state, scriptId, moduleName, className, interfaceText)) {
        ReleaseRuntimeState(state);
        state->Failed = false;
        state->ScriptId = scriptId;
        state->ModuleName = moduleName;
        state->ClassName = className;
        state->InterfaceText = interfaceText;
    }

    if (state->Failed) {
        return ExecutorReadyStatus::Failed;
    }

    if (state->Invoker &&
        state->Invoker->IsContextSuspended() &&
        state->ActiveLifecycleName == "OnReset") {
        const LifecycleInvokeStatus status = InvokeLifecycle(ctx, state, state->OnReset, state->OnResetArgument, "OnReset");
        if (status == LifecycleInvokeStatus::Suspended) {
            return ExecutorReadyStatus::Suspended;
        }
        if (status == LifecycleInvokeStatus::Failed) {
            return ExecutorReadyStatus::Failed;
        }
        state->StartCalled = false;
        return ExecutorReadyStatus::LifecycleOnly;
    }

    std::string interfaceError;
    const bool strictInterface = ReadLocalBool(behavior, SETTING_STRICT_INTERFACE, true);
    if (!ApplyInterface(ctx, behavior, interfaceText, strictInterface, interfaceError)) {
        ConsoleError(ctx, interfaceError);
        state->Failed = true;
        return ExecutorReadyStatus::Failed;
    }

    if (!state->Loaded) {
        state->Invoker = new ScriptInvoker(manager);
        std::string attachError;
        if (!AttachScriptEntryModule(manager, state->Invoker, scriptId, moduleName, attachError)) {
            FailSetup(ctx, state, "Failed to attach script module: " + attachError);
            return ExecutorReadyStatus::Failed;
        }

        asITypeInfo *type = state->Invoker->GetTypeInfoByName(className.c_str());
        if (!type) {
            FailSetup(ctx, state, "Executor class not found: " + className);
            return ExecutorReadyStatus::Failed;
        }

        state->Object = state->Invoker->CreateScriptObject(type);
        if (!state->Object) {
            FailSetup(ctx, state, "Failed to instantiate executor class: " + state->Invoker->GetErrorMessage());
            return ExecutorReadyStatus::Failed;
        }

        std::string cacheError;
        if (!CacheLifecycle(type, "Execute", true, state->Execute, state->ExecuteArgument, cacheError) ||
            !CacheLifecycle(type, "OnLoad", false, state->OnLoad, state->OnLoadArgument, cacheError) ||
            !CacheLifecycle(type, "Awake", false, state->Awake, state->AwakeArgument, cacheError) ||
            !CacheLifecycle(type, "Start", false, state->Start, state->StartArgument, cacheError) ||
            !CacheLifecycle(type, "OnDestroy", false, state->OnDestroy, state->OnDestroyArgument, cacheError) ||
            !CacheLifecycle(type, "OnReset", false, state->OnReset, state->OnResetArgument, cacheError)) {
            FailSetup(ctx, state, cacheError);
            return ExecutorReadyStatus::Failed;
        }
        state->Loaded = true;
    }

    if (!state->OnLoadCalled) {
        const LifecycleInvokeStatus status = InvokeLifecycle(ctx, state, state->OnLoad, state->OnLoadArgument, "OnLoad");
        if (status == LifecycleInvokeStatus::Suspended) {
            return ExecutorReadyStatus::Suspended;
        }
        if (status == LifecycleInvokeStatus::Failed) {
            return ExecutorReadyStatus::Failed;
        }
        state->OnLoadCalled = true;
    }
    if (!state->AwakeCalled) {
        const LifecycleInvokeStatus status = InvokeLifecycle(ctx, state, state->Awake, state->AwakeArgument, "Awake");
        if (status == LifecycleInvokeStatus::Suspended) {
            return ExecutorReadyStatus::Suspended;
        }
        if (status == LifecycleInvokeStatus::Failed) {
            return ExecutorReadyStatus::Failed;
        }
        state->AwakeCalled = true;
    }
    if (!state->StartCalled) {
        const LifecycleInvokeStatus status = InvokeLifecycle(ctx, state, state->Start, state->StartArgument, "Start");
        if (status == LifecycleInvokeStatus::Suspended) {
            return ExecutorReadyStatus::Suspended;
        }
        if (status == LifecycleInvokeStatus::Failed) {
            return ExecutorReadyStatus::Failed;
        }
        state->StartCalled = true;
    }

    return ExecutorReadyStatus::Ready;
}

std::vector<int> ActiveInputs(CKBehavior *behavior) {
    std::vector<int> active;
    if (!behavior) {
        return active;
    }
    for (int i = 0; i < behavior->GetInputCount(); ++i) {
        if (behavior->IsInputActive(i)) {
            active.push_back(i);
        }
    }
    return active;
}

void ClearExecutorInputs(CKBehavior *behavior, const std::vector<int> &inputs) {
    if (!behavior) {
        return;
    }
    for (int input : inputs) {
        if (input >= 0 && input < behavior->GetInputCount()) {
            behavior->ActivateInput(input, FALSE);
        }
    }
}

void ClearExecutorOutputs(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }
    for (int output = 0; output < behavior->GetOutputCount(); ++output) {
        behavior->ActivateOutput(output, FALSE);
    }
}

bool HasActiveOutputs(CKBehavior *behavior) {
    if (!behavior) {
        return false;
    }
    for (int output = 0; output < behavior->GetOutputCount(); ++output) {
        if (behavior->IsOutputActive(output)) {
            return true;
        }
    }
    return false;
}

bool SameInputSet(const std::vector<int> &left, const std::vector<int> &right) {
    return left == right;
}

void KeepExecutorScheduled(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }
    behavior->Activate(TRUE, FALSE);
    ActivateParentChain(behavior, false);
    ActivateOwnerScriptInCurrentScene(behavior, false);
}

} // namespace

void ReleaseAngelScriptExecutorStatesForManager(ScriptManager *manager) {
    std::vector<ScriptExecutorState *> states = ExecutorStates();
    for (ScriptExecutorState *state : states) {
        if (!state || (manager && state->Manager != manager)) {
            continue;
        }

        CKBehaviorContext ctx = {};
        ctx.Behavior = state->Behavior;
        ctx.Context = manager ? manager->GetCKContext() : nullptr;
        ctx.ParameterManager = ctx.Context ? ctx.Context->GetParameterManager() : nullptr;
        ctx.MessageManager = ctx.Context ? ctx.Context->GetMessageManager() : nullptr;
        ctx.AttributeManager = ctx.Context ? ctx.Context->GetAttributeManager() : nullptr;
        ctx.TimeManager = ctx.Context ? ctx.Context->GetTimeManager() : nullptr;

        InvokeOnDestroy(ctx, state);
        ReleaseRuntimeState(state);
    }
}

void ReleaseAngelScriptExecutorStatesForModule(ScriptManager *manager, const char *moduleName) {
    if (!manager || !moduleName || moduleName[0] == '\0') {
        return;
    }
    std::vector<ScriptExecutorState *> states = ExecutorStates();
    for (ScriptExecutorState *state : states) {
        if (!state || state->Manager != manager || state->ModuleName != moduleName) {
            continue;
        }

        CKBehaviorContext ctx = {};
        ctx.Behavior = state->Behavior;
        ctx.Context = manager->GetCKContext();
        ctx.ParameterManager = ctx.Context ? ctx.Context->GetParameterManager() : nullptr;
        ctx.MessageManager = ctx.Context ? ctx.Context->GetMessageManager() : nullptr;
        ctx.AttributeManager = ctx.Context ? ctx.Context->GetAttributeManager() : nullptr;
        ctx.TimeManager = ctx.Context ? ctx.Context->GetTimeManager() : nullptr;

        InvokeOnDestroy(ctx, state);
        ReleaseRuntimeState(state);
    }
}

CKObjectDeclaration *FillBehaviorAngelScriptExecutorDecl() {
    CKObjectDeclaration *od = CreateCKObjectDeclaration("AngelScript Executor");
    od->SetDescription("Execute an AngelScript class from a Virtools behavior graph with dynamic IO");
    od->SetCategory("AngelScript");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x15a41762, 0x0c7c4a91));
    od->SetAuthorGuid(CKGUID(0x3a086b4d, 0x2f4a4f01));
    od->SetAuthorName("Kakuty");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAngelScriptExecutorProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateAngelScriptExecutorProto(CKBehaviorPrototype **pproto) {
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("AngelScript Executor");
    if (!proto) {
        return CKERR_OUTOFMEMORY;
    }

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareLocalParameter(nullptr, CKPGUID_POINTER);
    proto->DeclareSetting("Script", CKPGUID_STRING);
    proto->DeclareSetting("Class", CKPGUID_STRING);
    proto->DeclareSetting("Interface", CKPGUID_STRING);
    proto->DeclareSetting("Strict Interface", CKPGUID_BOOL, "TRUE");
    proto->DeclareSetting("Trace", CKPGUID_BOOL, "FALSE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AngelScriptExecutor);
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_MESSAGESENDER |
                                                 CKBEHAVIOR_MESSAGERECEIVER |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDLOCALPARAMS));
    proto->SetBehaviorCallbackFct(AngelScriptExecutorCallback);

    *pproto = proto;
    return CK_OK;
}

int AngelScriptExecutor(const CKBehaviorContext &behcontext) {
    CKBehavior *behavior = behcontext.Behavior;
    if (!behavior) {
        return CKBR_PARAMETERERROR;
    }

    ScriptExecutorState *state = GetState(behavior);
    if (!state) {
        state = new ScriptExecutorState();
        state->BehaviorId = behavior->GetID();
        state->Behavior = behavior;
        state->Manager = behcontext.Context ? ScriptManager::GetManager(behcontext.Context) : nullptr;
        SetState(behavior, state);
    }

    const ExecutorReadyStatus ready = EnsureReady(behcontext, state);
    if (ready == ExecutorReadyStatus::Suspended) {
        return CKBR_ACTIVATENEXTFRAME;
    }
    if (ready == ExecutorReadyStatus::LifecycleOnly) {
        return CKBR_OK;
    }
    if (ready != ExecutorReadyStatus::Ready) {
        return CKBR_OK;
    }

    const bool trace = ReadLocalBool(behavior, SETTING_TRACE, false);
    const std::vector<int> activeInputs = ActiveInputs(behavior);
    if (trace && behcontext.Context) {
        const std::string className = ReadLocalString(behavior, SETTING_CLASS);
        behcontext.Context->OutputToConsoleEx(const_cast<char *>("[AngelScript Executor] #%d %s class=%s active inputs=%d"),
                                              behavior->GetID(),
                                              behavior->GetName(),
                                              className.c_str(),
                                              static_cast<int>(activeInputs.size()));
    }

    int returnCode = CKBR_OK;
    const bool hasActiveInputs = !activeInputs.empty();
    if (hasActiveInputs &&
        state->Invoker &&
        state->Invoker->IsContextSuspended() &&
        state->ActiveLifecycleName.empty()) {
        if (SameInputSet(activeInputs, state->SuspendedInputs)) {
            if (trace && behcontext.Context) {
                behcontext.Context->OutputToConsoleEx(
                    const_cast<char *>("[AngelScript Executor] #%d duplicate suspended input consumed while resuming"),
                    behavior->GetID());
            }
            ClearExecutorOutputs(behavior);
            ClearExecutorInputs(behavior, activeInputs);
        } else {
            if (trace && behcontext.Context) {
                behcontext.Context->OutputToConsoleEx(
                    const_cast<char *>("[AngelScript Executor] #%d aborting suspended Execute on new input"),
                    behavior->GetID());
            }
            state->Invoker->AbortContext();
            state->SuspendedInputs.clear();
        }
    }
    const bool resumingSuspendedExecute = state->Invoker && state->Invoker->IsContextSuspended();
    if (!resumingSuspendedExecute && hasActiveInputs) {
        ClearExecutorOutputs(behavior);
    }
    const ScriptInvocationStatus status = state->Invoker->ExecuteObjectMethodStatus(
        state->Object,
        state->Execute,
        behcontext,
        state->ExecuteArgument,
        [&](asIScriptContext *ctx) {
            if (!ctx) {
                return false;
            }
            if (state->Execute->GetReturnTypeId() == asTYPEID_INT32) {
                returnCode = static_cast<int>(ctx->GetReturnDWord());
            }
            return true;
        });

    if (status == ScriptInvocationStatus::Suspended) {
        if (!activeInputs.empty()) {
            state->SuspendedInputs = activeInputs;
        }
        KeepExecutorScheduled(behavior);
        ClearExecutorInputs(behavior, activeInputs);
        return CKBR_ACTIVATENEXTFRAME;
    }
    ClearExecutorInputs(behavior, activeInputs);
    if (status == ScriptInvocationStatus::Failed) {
        ConsoleError(behcontext, state->Invoker->GetErrorMessage());
        state->Failed = true;
        state->SuspendedInputs.clear();
        return CKBR_OK;
    }
    state->SuspendedInputs.clear();
    if (resumingSuspendedExecute && HasActiveOutputs(behavior)) {
        KeepExecutorScheduled(behavior);
    }
    return returnCode;
}

CKERROR AngelScriptExecutorCallback(const CKBehaviorContext &behcontext) {
    CKBehavior *behavior = behcontext.Behavior;
    if (!behavior) {
        return CKBR_PARAMETERERROR;
    }

    switch (behcontext.CallbackMessage) {
        case CKM_BEHAVIORCREATE:
        case CKM_BEHAVIORLOAD: {
            if (!GetState(behavior)) {
                ScriptExecutorState *state = new ScriptExecutorState();
                state->BehaviorId = behavior->GetID();
                state->Behavior = behavior;
                state->Manager = behcontext.Context ? ScriptManager::GetManager(behcontext.Context) : nullptr;
                SetState(behavior, state);
            }
            std::string error;
            if (!ApplyInterface(behcontext,
                               behavior,
                               ReadLocalString(behavior, SETTING_INTERFACE),
                               ReadLocalBool(behavior, SETTING_STRICT_INTERFACE, true),
                               error)) {
                ConsoleError(behcontext, error);
            }
            break;
        }
        case CKM_BEHAVIORSETTINGSEDITED: {
            ScriptExecutorState *state = GetState(behavior);
            if (state) {
                state->Failed = false;
            }
            std::string error;
            if (!ApplyInterface(behcontext,
                               behavior,
                               ReadLocalString(behavior, SETTING_INTERFACE),
                               ReadLocalBool(behavior, SETTING_STRICT_INTERFACE, true),
                               error)) {
                ConsoleError(behcontext, error);
            }
            break;
        }
        case CKM_BEHAVIORRESET: {
            ScriptExecutorState *state = GetState(behavior);
            if (state && state->Invoker && state->Object && state->OnReset) {
                const LifecycleInvokeStatus status = InvokeLifecycle(behcontext, state, state->OnReset, state->OnResetArgument, "OnReset");
                if (status == LifecycleInvokeStatus::Suspended) {
                    behavior->Activate(TRUE);
                    return CKBR_OK;
                }
                if (status == LifecycleInvokeStatus::Failed) {
                    return CKBR_OK;
                }
            }
            if (state) {
                state->StartCalled = false;
            }
            break;
        }
        case CKM_BEHAVIORDELETE: {
            ScriptExecutorState *state = GetState(behavior);
            if (state) {
                DestroyState(behcontext, state);
                SetState(behavior, nullptr);
            }
            break;
        }
        default:
            break;
    }
    return CKBR_OK;
}
