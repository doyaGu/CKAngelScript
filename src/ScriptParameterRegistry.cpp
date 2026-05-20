#include "ScriptParameterRegistry.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include "CKAll.h"

#include "ScriptParameterConversion.h"

namespace {

std::string TrimString(const std::string &value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    if (first == value.end()) {
        return {};
    }
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    return std::string(first, last);
}

std::string StripQuotes(const std::string &value) {
    std::string text = TrimString(value);
    if (text.size() >= 2 && ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\''))) {
        return text.substr(1, text.size() - 2);
    }
    return text;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::vector<std::string> SplitNames(const std::string &value) {
    std::string text = StripQuotes(value);
    for (char &c : text) {
        if (c == ';' || c == '|') {
            c = ',';
        }
    }

    std::vector<std::string> result;
    std::size_t offset = 0;
    while (offset <= text.size()) {
        const std::size_t comma = text.find(',', offset);
        const std::size_t end = comma == std::string::npos ? text.size() : comma;
        const std::string token = TrimString(text.substr(offset, end - offset));
        if (!token.empty()) {
            result.push_back(token);
        }
        if (comma == std::string::npos) {
            break;
        }
        offset = comma + 1;
    }
    return result;
}

bool ParseIntegerText(const std::string &value, int &out) {
    const std::string text = StripQuotes(value);
    if (text.empty()) {
        return false;
    }

    char *end = nullptr;
    if (text.front() == '-') {
        const long parsed = std::strtol(text.c_str(), &end, 0);
        if (!end || *end != '\0') {
            return false;
        }
        out = static_cast<int>(parsed);
        return true;
    }

    const unsigned long parsed = std::strtoul(text.c_str(), &end, 0);
    if (!end || *end != '\0') {
        return false;
    }
    out = static_cast<int>(parsed);
    return true;
}

CKParameterManager *ParameterManagerFromContext(CKContext *context) {
    return context ? context->GetParameterManager() : nullptr;
}

CKParameterManager *ParameterManagerFromBehaviorContext(const CKBehaviorContext &context) {
    return context.ParameterManager ? context.ParameterManager : ParameterManagerFromContext(context.Context);
}

CKParameterType ResolveParameterType(CKParameterManager *parameterManager, const std::string &typeName) {
    if (!parameterManager) {
        return -1;
    }

    const std::string text = StripQuotes(typeName);
    if (text.empty()) {
        return -1;
    }

    CKGUID guid;
    if (ParseScriptGuidString(text, guid)) {
        return parameterManager->ParameterGuidToType(guid);
    }

    CKParameterType type = parameterManager->ParameterNameToType(const_cast<CKSTRING>(text.c_str()));
    if (type >= 0) {
        return type;
    }

    const std::string lower = ToLower(text);
    const int typeCount = parameterManager->GetParameterTypesCount();
    for (int i = 0; i < typeCount; ++i) {
        CKParameterTypeDesc *desc = parameterManager->GetParameterTypeDescription(i);
        if (!desc) {
            continue;
        }
        const char *name = desc->TypeName.CStr();
        if (name && ToLower(name) == lower) {
            return i;
        }
    }

    return -1;
}

CKParameterType ResolveParameterType(CKParameterManager *parameterManager, CKGUID guid) {
    return parameterManager ? parameterManager->ParameterGuidToType(guid) : -1;
}

CKParameterTypeDesc *ResolveParameterTypeDesc(CKParameterManager *parameterManager, CKParameterType type) {
    return parameterManager && type >= 0 ? parameterManager->GetParameterTypeDescription(type) : nullptr;
}

bool IsEnumType(CKParameterManager *parameterManager, CKParameterType type) {
    CKParameterTypeDesc *desc = ResolveParameterTypeDesc(parameterManager, type);
    return desc && (desc->dwFlags & CKPARAMETERTYPE_ENUMS) != 0 && parameterManager->GetEnumDescByType(type) != nullptr;
}

bool IsFlagsType(CKParameterManager *parameterManager, CKParameterType type) {
    CKParameterTypeDesc *desc = ResolveParameterTypeDesc(parameterManager, type);
    return desc && (desc->dwFlags & CKPARAMETERTYPE_FLAGS) != 0 && parameterManager->GetFlagsDescByType(type) != nullptr;
}

bool TryResolveEnumValue(CKParameterManager *parameterManager, CKParameterType type, const std::string &name, int &out) {
    if (ParseIntegerText(name, out)) {
        return true;
    }

    CKEnumStruct *desc = parameterManager ? parameterManager->GetEnumDescByType(type) : nullptr;
    if (!desc) {
        return false;
    }

    const std::string wanted = StripQuotes(name);
    const std::string wantedLower = ToLower(wanted);
    for (int i = 0; i < desc->GetNumEnums(); ++i) {
        CKSTRING enumText = desc->GetEnumDescription(i);
        if (!enumText) {
            continue;
        }
        if (wanted == enumText || wantedLower == ToLower(enumText)) {
            out = desc->GetEnumValue(i);
            return true;
        }
    }

    return false;
}

bool TryResolveFlagValue(CKParameterManager *parameterManager, CKParameterType type, const std::string &name, int &out) {
    if (ParseIntegerText(name, out)) {
        return true;
    }

    CKFlagsStruct *desc = parameterManager ? parameterManager->GetFlagsDescByType(type) : nullptr;
    if (!desc) {
        return false;
    }

    const std::string wanted = StripQuotes(name);
    const std::string wantedLower = ToLower(wanted);
    for (int i = 0; i < desc->GetNumFlags(); ++i) {
        CKSTRING flagText = desc->GetFlagDescription(i);
        if (!flagText) {
            continue;
        }
        if (wanted == flagText || wantedLower == ToLower(flagText)) {
            out = desc->GetFlagValue(i);
            return true;
        }
    }

    return false;
}

int ResolveValue(CKParameterManager *parameterManager, CKParameterType type, const std::string &name, int fallback) {
    int value = 0;
    if (TryResolveEnumValue(parameterManager, type, name, value) || TryResolveFlagValue(parameterManager, type, name, value)) {
        return value;
    }
    return fallback;
}

CKDWORD ResolveFlags(CKParameterManager *parameterManager, CKParameterType type, const std::string &names, CKDWORD fallback) {
    const std::vector<std::string> tokens = SplitNames(names);
    if (tokens.empty()) {
        int value = 0;
        return ParseIntegerText(names, value) ? static_cast<CKDWORD>(value) : fallback;
    }

    CKDWORD result = 0;
    for (const std::string &token : tokens) {
        int value = 0;
        if (!TryResolveFlagValue(parameterManager, type, token, value)) {
            return fallback;
        }
        result |= static_cast<CKDWORD>(value);
    }
    return result;
}

std::string EnumValueText(CKParameterManager *parameterManager, CKParameterType type, int value) {
    CKEnumStruct *desc = parameterManager ? parameterManager->GetEnumDescByType(type) : nullptr;
    if (!desc) {
        return {};
    }

    for (int i = 0; i < desc->GetNumEnums(); ++i) {
        if (desc->GetEnumValue(i) == value) {
            CKSTRING text = desc->GetEnumDescription(i);
            return text ? text : "";
        }
    }
    return {};
}

std::string FlagsValueText(CKParameterManager *parameterManager, CKParameterType type, CKDWORD value) {
    CKFlagsStruct *desc = parameterManager ? parameterManager->GetFlagsDescByType(type) : nullptr;
    if (!desc) {
        return {};
    }

    std::string result;
    for (int i = 0; i < desc->GetNumFlags(); ++i) {
        const CKDWORD flag = static_cast<CKDWORD>(desc->GetFlagValue(i));
        if ((value & flag) == 0) {
            continue;
        }
        CKSTRING text = desc->GetFlagDescription(i);
        if (!text || !text[0]) {
            continue;
        }
        if (!result.empty()) {
            result += ",";
        }
        result += text;
    }
    return result;
}

std::string ValueText(CKParameterManager *parameterManager, CKParameterType type, int value) {
    std::string text = EnumValueText(parameterManager, type, value);
    if (!text.empty()) {
        return text;
    }
    text = FlagsValueText(parameterManager, type, static_cast<CKDWORD>(value));
    if (!text.empty()) {
        return text;
    }
    return std::to_string(value);
}

std::string DescribeParameterType(CKParameterManager *parameterManager, CKParameterType type) {
    CKParameterTypeDesc *typeDesc = ResolveParameterTypeDesc(parameterManager, type);
    if (!typeDesc) {
        return {};
    }

    std::ostringstream out;
    out << (typeDesc->TypeName.CStr() ? typeDesc->TypeName.CStr() : "")
        << " " << ScriptGuidToString(typeDesc->Guid);

    if (CKEnumStruct *enumDesc = parameterManager->GetEnumDescByType(type)) {
        out << " [enum]";
        for (int i = 0; i < enumDesc->GetNumEnums(); ++i) {
            out << "\n  " << (enumDesc->GetEnumDescription(i) ? enumDesc->GetEnumDescription(i) : "")
                << " = " << enumDesc->GetEnumValue(i);
        }
        return out.str();
    }

    if (CKFlagsStruct *flagsDesc = parameterManager->GetFlagsDescByType(type)) {
        out << " [flags]";
        for (int i = 0; i < flagsDesc->GetNumFlags(); ++i) {
            out << "\n  " << (flagsDesc->GetFlagDescription(i) ? flagsDesc->GetFlagDescription(i) : "")
                << " = " << flagsDesc->GetFlagValue(i);
        }
        return out.str();
    }

    return out.str();
}

CKGUID ParamGuidByName(CKContext *context, const std::string &typeName) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    const CKParameterType type = ResolveParameterType(parameterManager, typeName);
    return type >= 0 ? parameterManager->ParameterTypeToGuid(type) : CKGUID();
}

CKGUID ParamGuidByNameFromBehaviorContext(const CKBehaviorContext &context, const std::string &typeName) {
    CKParameterManager *parameterManager = ParameterManagerFromBehaviorContext(context);
    const CKParameterType type = ResolveParameterType(parameterManager, typeName);
    return type >= 0 ? parameterManager->ParameterTypeToGuid(type) : CKGUID();
}

CKParameterType ParamTypeByName(CKContext *context, const std::string &typeName) {
    return ResolveParameterType(ParameterManagerFromContext(context), typeName);
}

CKParameterType ParamTypeByNameFromBehaviorContext(const CKBehaviorContext &context, const std::string &typeName) {
    return ResolveParameterType(ParameterManagerFromBehaviorContext(context), typeName);
}

bool ParamIsEnumByName(CKContext *context, const std::string &typeName) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    return IsEnumType(parameterManager, ResolveParameterType(parameterManager, typeName));
}

bool ParamIsEnumByNameFromBehaviorContext(const CKBehaviorContext &context, const std::string &typeName) {
    CKParameterManager *parameterManager = ParameterManagerFromBehaviorContext(context);
    return IsEnumType(parameterManager, ResolveParameterType(parameterManager, typeName));
}

bool ParamIsFlagsByName(CKContext *context, const std::string &typeName) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    return IsFlagsType(parameterManager, ResolveParameterType(parameterManager, typeName));
}

bool ParamIsFlagsByNameFromBehaviorContext(const CKBehaviorContext &context, const std::string &typeName) {
    CKParameterManager *parameterManager = ParameterManagerFromBehaviorContext(context);
    return IsFlagsType(parameterManager, ResolveParameterType(parameterManager, typeName));
}

int ParamValueByName(CKContext *context, const std::string &typeName, const std::string &valueName, int fallback) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    return ResolveValue(parameterManager, ResolveParameterType(parameterManager, typeName), valueName, fallback);
}

int ParamValueByNameFromBehaviorContext(const CKBehaviorContext &context, const std::string &typeName, const std::string &valueName, int fallback) {
    CKParameterManager *parameterManager = ParameterManagerFromBehaviorContext(context);
    return ResolveValue(parameterManager, ResolveParameterType(parameterManager, typeName), valueName, fallback);
}

int ParamValueByGuid(CKContext *context, CKGUID guid, const std::string &valueName, int fallback) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    return ResolveValue(parameterManager, ResolveParameterType(parameterManager, guid), valueName, fallback);
}

int ParamValueByGuidFromBehaviorContext(const CKBehaviorContext &context, CKGUID guid, const std::string &valueName, int fallback) {
    CKParameterManager *parameterManager = ParameterManagerFromBehaviorContext(context);
    return ResolveValue(parameterManager, ResolveParameterType(parameterManager, guid), valueName, fallback);
}

CKDWORD ParamFlagByName(CKContext *context, const std::string &typeName, const std::string &valueName, CKDWORD fallback) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    int value = 0;
    if (TryResolveFlagValue(parameterManager, ResolveParameterType(parameterManager, typeName), valueName, value)) {
        return static_cast<CKDWORD>(value);
    }
    return fallback;
}

CKDWORD ParamFlagByNameFromBehaviorContext(const CKBehaviorContext &context, const std::string &typeName, const std::string &valueName, CKDWORD fallback) {
    CKParameterManager *parameterManager = ParameterManagerFromBehaviorContext(context);
    int value = 0;
    if (TryResolveFlagValue(parameterManager, ResolveParameterType(parameterManager, typeName), valueName, value)) {
        return static_cast<CKDWORD>(value);
    }
    return fallback;
}

CKDWORD ParamFlagsByName(CKContext *context, const std::string &typeName, const std::string &valueNames, CKDWORD fallback) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    return ResolveFlags(parameterManager, ResolveParameterType(parameterManager, typeName), valueNames, fallback);
}

CKDWORD ParamFlagsByNameFromBehaviorContext(const CKBehaviorContext &context, const std::string &typeName, const std::string &valueNames, CKDWORD fallback) {
    CKParameterManager *parameterManager = ParameterManagerFromBehaviorContext(context);
    return ResolveFlags(parameterManager, ResolveParameterType(parameterManager, typeName), valueNames, fallback);
}

std::string ParamTextByName(CKContext *context, const std::string &typeName, int value) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    return ValueText(parameterManager, ResolveParameterType(parameterManager, typeName), value);
}

std::string ParamTextByNameFromBehaviorContext(const CKBehaviorContext &context, const std::string &typeName, int value) {
    CKParameterManager *parameterManager = ParameterManagerFromBehaviorContext(context);
    return ValueText(parameterManager, ResolveParameterType(parameterManager, typeName), value);
}

std::string ParamDescribeByName(CKContext *context, const std::string &typeName) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    return DescribeParameterType(parameterManager, ResolveParameterType(parameterManager, typeName));
}

std::string ParamDescribeByNameFromBehaviorContext(const CKBehaviorContext &context, const std::string &typeName) {
    CKParameterManager *parameterManager = ParameterManagerFromBehaviorContext(context);
    return DescribeParameterType(parameterManager, ResolveParameterType(parameterManager, typeName));
}

} // namespace

void RegisterScriptParameterRegistry(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    const char *previousNamespace = engine->GetDefaultNamespace();
    std::string previous = previousNamespace ? previousNamespace : "";

    r = engine->SetDefaultNamespace("Param"); assert(r >= 0);

    r = engine->RegisterGlobalFunction("CKGUID Guid(CKContext@ context, const string &in typeName)", asFUNCTION(ParamGuidByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKGUID Guid(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ParamGuidByNameFromBehaviorContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKParameterType Type(CKContext@ context, const string &in typeName)", asFUNCTION(ParamTypeByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKParameterType Type(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ParamTypeByNameFromBehaviorContext), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterGlobalFunction("bool IsEnum(CKContext@ context, const string &in typeName)", asFUNCTION(ParamIsEnumByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsEnum(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ParamIsEnumByNameFromBehaviorContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsFlags(CKContext@ context, const string &in typeName)", asFUNCTION(ParamIsFlagsByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsFlags(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ParamIsFlagsByNameFromBehaviorContext), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterGlobalFunction("int Value(CKContext@ context, const string &in typeName, const string &in valueName, int fallback = 0)", asFUNCTION(ParamValueByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Value(const CKBehaviorContext &in ctx, const string &in typeName, const string &in valueName, int fallback = 0)", asFUNCTION(ParamValueByNameFromBehaviorContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Value(CKContext@ context, CKGUID guid, const string &in valueName, int fallback = 0)", asFUNCTION(ParamValueByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Value(const CKBehaviorContext &in ctx, CKGUID guid, const string &in valueName, int fallback = 0)", asFUNCTION(ParamValueByGuidFromBehaviorContext), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterGlobalFunction("uint Flag(CKContext@ context, const string &in typeName, const string &in valueName, uint fallback = 0)", asFUNCTION(ParamFlagByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint Flag(const CKBehaviorContext &in ctx, const string &in typeName, const string &in valueName, uint fallback = 0)", asFUNCTION(ParamFlagByNameFromBehaviorContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint Flags(CKContext@ context, const string &in typeName, const string &in valueNames, uint fallback = 0)", asFUNCTION(ParamFlagsByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint Flags(const CKBehaviorContext &in ctx, const string &in typeName, const string &in valueNames, uint fallback = 0)", asFUNCTION(ParamFlagsByNameFromBehaviorContext), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterGlobalFunction("string Text(CKContext@ context, const string &in typeName, int value)", asFUNCTION(ParamTextByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Text(const CKBehaviorContext &in ctx, const string &in typeName, int value)", asFUNCTION(ParamTextByNameFromBehaviorContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Describe(CKContext@ context, const string &in typeName)", asFUNCTION(ParamDescribeByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Describe(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ParamDescribeByNameFromBehaviorContext), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}

bool RunScriptParameterRegistrySelfTest(CKContext *context, std::string &error) {
    CKParameterManager *parameterManager = ParameterManagerFromContext(context);
    if (!parameterManager) {
        error = "CKParameterManager is not available.";
        return false;
    }

    for (int i = 0; i < parameterManager->GetParameterTypesCount(); ++i) {
        if (CKEnumStruct *enumDesc = parameterManager->GetEnumDescByType(i)) {
            if (enumDesc->GetNumEnums() <= 0 || !enumDesc->GetEnumDescription(0)) {
                continue;
            }

            int value = 0;
            if (!TryResolveEnumValue(parameterManager, i, enumDesc->GetEnumDescription(0), value) ||
                value != enumDesc->GetEnumValue(0)) {
                error = "Enum value lookup failed.";
                return false;
            }

            if (EnumValueText(parameterManager, i, value).empty()) {
                error = "Enum text lookup failed.";
                return false;
            }
            break;
        }
    }

    for (int i = 0; i < parameterManager->GetParameterTypesCount(); ++i) {
        if (CKFlagsStruct *flagsDesc = parameterManager->GetFlagsDescByType(i)) {
            if (flagsDesc->GetNumFlags() <= 0 || !flagsDesc->GetFlagDescription(0)) {
                continue;
            }

            int value = 0;
            if (!TryResolveFlagValue(parameterManager, i, flagsDesc->GetFlagDescription(0), value) ||
                value != flagsDesc->GetFlagValue(0)) {
                error = "Flags value lookup failed.";
                return false;
            }

            if (FlagsValueText(parameterManager, i, static_cast<CKDWORD>(value)).empty()) {
                error = "Flags text lookup failed.";
                return false;
            }
            break;
        }
    }

    error.clear();
    return true;
}
