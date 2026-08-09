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
#include "ScriptHostScope.h"
#include "ScriptInvoker.h"
#include "ScriptManager.h"
#include "ScriptParameterConversion.h"
#include "ScriptRuntime.h"
#include "ScriptExecutorState.h"

#ifndef CKAS_BUILD_SELF_TESTS
#define CKAS_BUILD_SELF_TESTS 0
#endif

namespace {

#if CKAS_BUILD_SELF_TESTS
bool g_FailNextPhysicalInterfaceSyncForSelfTest = false;
#endif

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

bool SyncInputCount(CKBehavior *behavior, const std::vector<ExecutorSlotSpec> &inputs, std::string &error) {
    while (behavior->GetInputCount() > static_cast<int>(inputs.size())) {
        if (behavior->DeleteInput(behavior->GetInputCount() - 1) != CK_OK) {
            error = "Failed to remove an executor input.";
            return false;
        }
    }
    while (behavior->GetInputCount() < static_cast<int>(inputs.size())) {
        const ExecutorSlotSpec &spec = inputs[behavior->GetInputCount()];
        if (behavior->AddInput(const_cast<CKSTRING>(spec.Name.c_str())) < 0) {
            error = "Failed to create executor input '" + spec.Name + "'.";
            return false;
        }
    }
    for (int i = 0; i < static_cast<int>(inputs.size()); ++i) {
        CKBehaviorIO *io = behavior->GetInput(i);
        if (io && ObjectName(io) != inputs[i].Name) {
            io->SetName(const_cast<CKSTRING>(inputs[i].Name.c_str()));
        }
    }
    return true;
}

bool SyncOutputCount(CKBehavior *behavior, const std::vector<ExecutorSlotSpec> &outputs, std::string &error) {
    while (behavior->GetOutputCount() > static_cast<int>(outputs.size())) {
        if (behavior->DeleteOutput(behavior->GetOutputCount() - 1) != CK_OK) {
            error = "Failed to remove an executor output.";
            return false;
        }
    }
    while (behavior->GetOutputCount() < static_cast<int>(outputs.size())) {
        const ExecutorSlotSpec &spec = outputs[behavior->GetOutputCount()];
        if (behavior->AddOutput(const_cast<CKSTRING>(spec.Name.c_str())) < 0) {
            error = "Failed to create executor output '" + spec.Name + "'.";
            return false;
        }
    }
    for (int i = 0; i < static_cast<int>(outputs.size()); ++i) {
        CKBehaviorIO *io = behavior->GetOutput(i);
        if (io && ObjectName(io) != outputs[i].Name) {
            io->SetName(const_cast<CKSTRING>(outputs[i].Name.c_str()));
        }
    }
    return true;
}

bool SyncPinCount(CKBehavior *behavior, const std::vector<ExecutorSlotSpec> &pins, std::string &error) {
    while (behavior->GetInputParameterCount() > static_cast<int>(pins.size())) {
        CKParameterIn *removed = behavior->RemoveInputParameter(behavior->GetInputParameterCount() - 1);
        if (!removed) {
            error = "Failed to remove an executor input parameter.";
            return false;
        }
        CKDestroyObject(removed);
    }
    while (behavior->GetInputParameterCount() < static_cast<int>(pins.size())) {
        const ExecutorSlotSpec &spec = pins[behavior->GetInputParameterCount()];
        if (!behavior->CreateInputParameter(const_cast<CKSTRING>(spec.Name.c_str()), spec.Guid)) {
            error = "Failed to create executor input parameter '" + spec.Name + "'.";
            return false;
        }
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
    return true;
}

bool SyncPoutCount(CKBehavior *behavior, const std::vector<ExecutorSlotSpec> &pouts, std::string &error) {
    while (behavior->GetOutputParameterCount() > static_cast<int>(pouts.size())) {
        CKParameterOut *removed = behavior->RemoveOutputParameter(behavior->GetOutputParameterCount() - 1);
        if (!removed) {
            error = "Failed to remove an executor output parameter.";
            return false;
        }
        CKDestroyObject(removed);
    }
    while (behavior->GetOutputParameterCount() < static_cast<int>(pouts.size())) {
        const ExecutorSlotSpec &spec = pouts[behavior->GetOutputParameterCount()];
        if (!behavior->CreateOutputParameter(const_cast<CKSTRING>(spec.Name.c_str()), spec.Guid)) {
            error = "Failed to create executor output parameter '" + spec.Name + "'.";
            return false;
        }
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
    return true;
}

bool SyncPhysicalInterface(CKBehavior *behavior,
                           const std::vector<ExecutorSlotSpec> &inputs,
                           const std::vector<ExecutorSlotSpec> &outputs,
                           const std::vector<ExecutorSlotSpec> &pins,
                           const std::vector<ExecutorSlotSpec> &pouts,
                           std::string &error) {
#if CKAS_BUILD_SELF_TESTS
    const bool injectFailure = g_FailNextPhysicalInterfaceSyncForSelfTest;
    g_FailNextPhysicalInterfaceSyncForSelfTest = false;
    if (injectFailure) {
        if (!SyncInputCount(behavior, inputs, error)) {
            return false;
        }
        error = "Injected executor physical interface synchronization failure.";
        return false;
    }
#endif
    if (behavior->GetInputCount() <= static_cast<int>(inputs.size()) &&
        !SyncInputCount(behavior, inputs, error)) return false;
    if (behavior->GetOutputCount() <= static_cast<int>(outputs.size()) &&
        !SyncOutputCount(behavior, outputs, error)) return false;
    if (behavior->GetInputParameterCount() <= static_cast<int>(pins.size()) &&
        !SyncPinCount(behavior, pins, error)) return false;
    if (behavior->GetOutputParameterCount() <= static_cast<int>(pouts.size()) &&
        !SyncPoutCount(behavior, pouts, error)) return false;
    if (behavior->GetInputCount() > static_cast<int>(inputs.size()) &&
        !SyncInputCount(behavior, inputs, error)) return false;
    if (behavior->GetOutputCount() > static_cast<int>(outputs.size()) &&
        !SyncOutputCount(behavior, outputs, error)) return false;
    if (behavior->GetInputParameterCount() > static_cast<int>(pins.size()) &&
        !SyncPinCount(behavior, pins, error)) return false;
    if (behavior->GetOutputParameterCount() > static_cast<int>(pouts.size()) &&
        !SyncPoutCount(behavior, pouts, error)) return false;
    return true;
}

bool CapturePhysicalInterface(CKBehavior *behavior,
                              ExecutorInterface &interfaceShape,
                              std::string &error) {
    interfaceShape = ExecutorInterface();
    if (!behavior) {
        error = "Executor interface requires a behavior.";
        return false;
    }

    for (int i = 0; i < behavior->GetInputCount(); ++i) {
        CKBehaviorIO *io = behavior->GetInput(i);
        if (!io) {
            error = "Cannot snapshot missing executor input " + std::to_string(i) + ".";
            return false;
        }
        ExecutorSlotSpec spec;
        spec.Kind = ExecutorSlotKind::Input;
        spec.Name = ObjectName(io);
        interfaceShape.Inputs.push_back(std::move(spec));
    }
    for (int i = 0; i < behavior->GetOutputCount(); ++i) {
        CKBehaviorIO *io = behavior->GetOutput(i);
        if (!io) {
            error = "Cannot snapshot missing executor output " + std::to_string(i) + ".";
            return false;
        }
        ExecutorSlotSpec spec;
        spec.Kind = ExecutorSlotKind::Output;
        spec.Name = ObjectName(io);
        interfaceShape.Outputs.push_back(std::move(spec));
    }
    for (int i = 0; i < behavior->GetInputParameterCount(); ++i) {
        CKParameterIn *param = behavior->GetInputParameter(i);
        if (!param) {
            error = "Cannot snapshot missing executor input parameter " + std::to_string(i) + ".";
            return false;
        }
        ExecutorSlotSpec spec;
        spec.Kind = ExecutorSlotKind::Pin;
        spec.Name = ObjectName(param);
        spec.Guid = param->GetGUID();
        interfaceShape.Pins.push_back(std::move(spec));
    }
    for (int i = 0; i < behavior->GetOutputParameterCount(); ++i) {
        CKParameterOut *param = behavior->GetOutputParameter(i);
        if (!param) {
            error = "Cannot snapshot missing executor output parameter " + std::to_string(i) + ".";
            return false;
        }
        ExecutorSlotSpec spec;
        spec.Kind = ExecutorSlotKind::Pout;
        spec.Name = ObjectName(param);
        spec.Guid = param->GetGUID();
        interfaceShape.Pouts.push_back(std::move(spec));
    }
    return true;
}

bool IsBehaviorIoConnected(CKBehavior *behavior, CKBehaviorIO *io) {
    if (!behavior || !io) {
        return false;
    }
    CKBehavior *graph = behavior->GetParent();
    if (!graph) {
        graph = behavior;
    }
    for (int i = 0; i < graph->GetSubBehaviorLinkCount(); ++i) {
        CKBehaviorLink *link = graph->GetSubBehaviorLink(i);
        if (link && (link->GetInBehaviorIO() == io || link->GetOutBehaviorIO() == io)) {
            return true;
        }
    }
    return false;
}

bool ValidatePhysicalMutationSafety(CKBehavior *behavior,
                                    const ExecutorInterface &target,
                                    std::string &error) {
    for (int i = static_cast<int>(target.Inputs.size()); i < behavior->GetInputCount(); ++i) {
        if (IsBehaviorIoConnected(behavior, behavior->GetInput(i))) {
            error = "Executor interface cannot remove connected input " + std::to_string(i) + ".";
            return false;
        }
    }
    for (int i = static_cast<int>(target.Outputs.size()); i < behavior->GetOutputCount(); ++i) {
        if (IsBehaviorIoConnected(behavior, behavior->GetOutput(i))) {
            error = "Executor interface cannot remove connected output " + std::to_string(i) + ".";
            return false;
        }
    }
    for (int i = 0; i < behavior->GetInputParameterCount(); ++i) {
        CKParameterIn *param = behavior->GetInputParameter(i);
        const bool removed = i >= static_cast<int>(target.Pins.size());
        const bool typeChanged = !removed && param && param->GetGUID() != target.Pins[i].Guid;
        if (param && (removed || typeChanged) && param->GetRealSource()) {
            error = "Executor interface cannot remove or retype connected input parameter " +
                std::to_string(i) + ".";
            return false;
        }
    }
    for (int i = 0; i < behavior->GetOutputParameterCount(); ++i) {
        CKParameterOut *param = behavior->GetOutputParameter(i);
        const bool removed = i >= static_cast<int>(target.Pouts.size());
        const bool typeChanged = !removed && param && param->GetGUID() != target.Pouts[i].Guid;
        if (param && (removed || typeChanged) && param->GetDestinationCount() > 0) {
            error = "Executor interface cannot remove or retype connected output parameter " +
                std::to_string(i) + ".";
            return false;
        }
    }
    return true;
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

bool CommitPhysicalInterface(const CKBehaviorContext &ctx,
                             CKBehavior *behavior,
                             const ExecutorInterface &target,
                             bool strict,
                             std::string &error) {
    ExecutorInterface snapshot;
    if (!CapturePhysicalInterface(behavior, snapshot, error) ||
        !ValidatePhysicalMutationSafety(behavior, target, error)) {
        return false;
    }

    std::string commitError;
    const bool committed =
        SyncPhysicalInterface(behavior,
                              target.Inputs,
                              target.Outputs,
                              target.Pins,
                              target.Pouts,
                              commitError) &&
        (!strict || ValidateInterface(ctx, behavior, target, commitError));
    if (committed) {
        return true;
    }

    std::string restoreError;
    if (!SyncPhysicalInterface(behavior,
                               snapshot.Inputs,
                               snapshot.Outputs,
                               snapshot.Pins,
                               snapshot.Pouts,
                               restoreError)) {
        error = commitError + " Failed to restore previous executor interface: " + restoreError;
    } else {
        error = commitError;
    }
    return false;
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
    return ScriptHostScope::From(
        ctx, ScriptHostScopeContextResolution::DirectContext).Bridge();
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
    if (!bridge) {
        error = "Executor interface requires ScriptBehaviorBridge.";
        return false;
    }

    ExecutorInterface interfaceShape;
    if (interfaceText.empty()) {
        ExecutorSlotSpec defaultInput;
        defaultInput.Kind = ExecutorSlotKind::Input;
        defaultInput.Name = "In";
        interfaceShape.Inputs.push_back(std::move(defaultInput));
        ExecutorSlotSpec defaultOutput;
        defaultOutput.Kind = ExecutorSlotKind::Output;
        defaultOutput.Name = "Out";
        interfaceShape.Outputs.push_back(std::move(defaultOutput));
    } else if (!ParseInterface(ctx.Context, interfaceText, interfaceShape, error)) {
        return false;
    }

    return bridge->ApplyFrameLocalInterfaceTransaction(
        behavior,
        FrameLocalDecls(interfaceShape.Locals),
        USER_LOCAL_START,
        strict,
        [&](std::string &commitError) {
            return CommitPhysicalInterface(ctx, behavior, interfaceShape, strict, commitError);
        },
        error);
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

void ReleaseStateContents(const CKBehaviorContext &ctx, ScriptExecutorState *state) {
    if (!state) {
        return;
    }
    SetState(state->Behavior, nullptr);
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
    state->InitialLifecycle.Clear();
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

void TransitionIdentity(const CKBehaviorContext &ctx,
                        ScriptExecutorState *state,
                        const std::string &scriptId,
                        const std::string &moduleName,
                        const std::string &className,
                        const std::string &interfaceText) {
    if (!IdentityChanged(state, scriptId, moduleName, className, interfaceText)) {
        return;
    }
    InvokeOnDestroy(ctx, state);
    ReleaseRuntimeState(state);
    state->Failed = false;
    state->ScriptId = scriptId;
    state->ModuleName = moduleName;
    state->ClassName = className;
    state->InterfaceText = interfaceText;
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
    ScriptManager *manager = ScriptHostScope::From(
        ctx, ScriptHostScopeContextResolution::DirectContext).Manager();
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
    std::string interfaceError;
    const bool strictInterface = ReadLocalBool(behavior, SETTING_STRICT_INTERFACE, true);
    if (!ApplyInterface(ctx, behavior, interfaceText, strictInterface, interfaceError)) {
        ConsoleError(ctx, interfaceError);
        state->Failed = true;
        return ExecutorReadyStatus::Failed;
    }

    TransitionIdentity(ctx, state, scriptId, moduleName, className, interfaceText);

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
        state->InitialLifecycle.ResetStart();
        return ExecutorReadyStatus::LifecycleOnly;
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

    if (state->InitialLifecycle.Next() == ScriptInitialPhase::OnLoad) {
        const LifecycleInvokeStatus status = InvokeLifecycle(ctx, state, state->OnLoad, state->OnLoadArgument, "OnLoad");
        if (status == LifecycleInvokeStatus::Suspended) {
            return ExecutorReadyStatus::Suspended;
        }
        if (status == LifecycleInvokeStatus::Failed) {
            return ExecutorReadyStatus::Failed;
        }
        state->InitialLifecycle.Complete(ScriptInitialPhase::OnLoad);
    }
    if (state->InitialLifecycle.Next() == ScriptInitialPhase::Awake) {
        const LifecycleInvokeStatus status = InvokeLifecycle(ctx, state, state->Awake, state->AwakeArgument, "Awake");
        if (status == LifecycleInvokeStatus::Suspended) {
            return ExecutorReadyStatus::Suspended;
        }
        if (status == LifecycleInvokeStatus::Failed) {
            return ExecutorReadyStatus::Failed;
        }
        state->InitialLifecycle.Complete(ScriptInitialPhase::Awake);
    }
    if (state->InitialLifecycle.Next() == ScriptInitialPhase::Start) {
        const LifecycleInvokeStatus status = InvokeLifecycle(ctx, state, state->Start, state->StartArgument, "Start");
        if (status == LifecycleInvokeStatus::Suspended) {
            return ExecutorReadyStatus::Suspended;
        }
        if (status == LifecycleInvokeStatus::Failed) {
            return ExecutorReadyStatus::Failed;
        }
        state->InitialLifecycle.Complete(ScriptInitialPhase::Start);
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

CKBehaviorContext ExecutorContext(ScriptManager *manager, ScriptExecutorState *state) {
    CKBehaviorContext ctx = {};
    ctx.Behavior = state ? state->Behavior : nullptr;
    ctx.Context = manager ? manager->GetCKContext() : nullptr;
    ctx.ParameterManager = ctx.Context ? ctx.Context->GetParameterManager() : nullptr;
    ctx.MessageManager = ctx.Context ? ctx.Context->GetMessageManager() : nullptr;
    ctx.AttributeManager = ctx.Context ? ctx.Context->GetAttributeManager() : nullptr;
    ctx.TimeManager = ctx.Context ? ctx.Context->GetTimeManager() : nullptr;
    return ctx;
}

} // namespace

#if CKAS_BUILD_SELF_TESTS
void AngelScriptExecutorFailNextPhysicalInterfaceSyncForSelfTest() {
    g_FailNextPhysicalInterfaceSyncForSelfTest = true;
}
#endif

ScriptExecutorState *ScriptManager::GetOrCreateExecutorState(CKBehavior *behavior) {
    ScriptExecutorState *state = m_ExecutorStates.GetOrCreate(
        behavior,
        this,
        [this](CK_ID, ScriptExecutorState *replaced) {
            ReleaseStateContents(ExecutorContext(this, replaced), replaced);
        });
    SetState(behavior, state);
    return state;
}

ScriptExecutorState *ScriptManager::GetExecutorState(CK_ID id, CKBehavior *expectedBehavior) const {
    return m_ExecutorStates.Get(id, expectedBehavior);
}

void ScriptManager::ReleaseExecutorState(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }
    m_ExecutorStates.Remove(
        behavior->GetID(),
        behavior,
        [this](CK_ID, ScriptExecutorState *state) {
            ReleaseStateContents(ExecutorContext(this, state), state);
        });
}

void ScriptManager::ReleaseExecutorStatesForModule(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }
    m_ExecutorStates.ReleaseModule(
        moduleName,
        [this](CK_ID, ScriptExecutorState *state) {
            ReleaseStateContents(ExecutorContext(this, state), state);
        });
}

void ScriptManager::ClearExecutorStates() {
    m_ExecutorStates.Clear([this](CK_ID, ScriptExecutorState *state) {
        ReleaseStateContents(ExecutorContext(this, state), state);
    });
}

void ScriptManager::PrepareModuleMutation(const char *moduleName) {
    ReleaseExecutorStatesForModule(moduleName);
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
        ScriptManager *manager = ScriptHostScope::From(
            behcontext, ScriptHostScopeContextResolution::DirectContext).Manager();
        state = manager ? manager->GetOrCreateExecutorState(behavior) : nullptr;
        if (!state) {
            ConsoleError(behcontext, "AngelScript manager is not available.");
            return CKBR_OK;
        }
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
            ScriptManager *manager = ScriptHostScope::From(
                behcontext, ScriptHostScopeContextResolution::DirectContext).Manager();
            if (manager) {
                manager->GetOrCreateExecutorState(behavior);
            } else {
                SetState(behavior, nullptr);
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
            const std::string interfaceText = ReadLocalString(behavior, SETTING_INTERFACE);
            std::string error;
            if (!ApplyInterface(behcontext,
                                behavior,
                                interfaceText,
                                ReadLocalBool(behavior, SETTING_STRICT_INTERFACE, true),
                                error)) {
                if (state) {
                    state->Failed = true;
                }
                ConsoleError(behcontext, error);
                break;
            }
            if (state) {
                const std::string scriptId = Trim(ReadLocalString(behavior, SETTING_SCRIPT));
                const std::string className = Trim(ReadLocalString(behavior, SETTING_CLASS));
                ScriptManager *manager = ScriptHostScope::From(
                    behcontext, ScriptHostScopeContextResolution::DirectContext).Manager();
                ScriptRuntime *runtime = manager ? manager->GetRuntime() : nullptr;
                const std::string moduleName = runtime ? runtime->ModuleName(scriptId) : std::string();
                TransitionIdentity(behcontext,
                                   state,
                                   scriptId,
                                   moduleName,
                                   className,
                                   interfaceText);
                state->Failed = false;
            }
            break;
        }
        case CKM_BEHAVIORRESET: {
            ScriptExecutorState *state = GetState(behavior);
            if (state) {
                state->InitialLifecycle.ResetStart();
            }
            if (state && state->Invoker && state->Invoker->IsContextSuspended()) {
                state->Invoker->AbortContext();
                state->SuspendedInputs.clear();
                state->ActiveLifecycleName.clear();
            }
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
            break;
        }
        case CKM_BEHAVIORDETACH:
        case CKM_BEHAVIORDELETE: {
            ScriptExecutorState *state = GetState(behavior);
            if (state) {
                ScriptManager *manager = state->Manager
                    ? state->Manager
                    : ScriptHostScope::From(
                          behcontext, ScriptHostScopeContextResolution::DirectContext).Manager();
                if (manager) {
                    manager->ReleaseExecutorState(behavior);
                }
            }
            break;
        }
        default:
            break;
    }
    return CKBR_OK;
}
