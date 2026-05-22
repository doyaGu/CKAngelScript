#include "ScriptParameterRegistry.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <sstream>

#include "CKAll.h"
#include "ScriptManager.h"
#include "ScriptParameterConversion.h"
#include "ScriptXArray.h"

namespace ScriptParameterText {

std::string Trim(const std::string &value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    if (first == value.end()) {
        return {};
    }
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    return std::string(first, last);
}

std::string StripQuotes(const std::string &value) {
    std::string text = Trim(value);
    if (text.size() >= 2 && ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\''))) {
        return text.substr(1, text.size() - 2);
    }
    return text;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::vector<std::string> SplitList(const std::string &value) {
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
        const std::string token = Trim(text.substr(offset, end - offset));
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

bool ParseInteger(const std::string &value, int &out) {
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

}

namespace ScriptParameterRegistryInternal {

CKParameterManager *ParameterManagerFromContext(CKContext *context) {
    return context ? context->GetParameterManager() : nullptr;
}

CKParameterManager *ParameterManagerFromBehaviorContext(const CKBehaviorContext &context) {
    return context.ParameterManager ? context.ParameterManager : ParameterManagerFromContext(context.Context);
}

ScriptParameterRegistry *RegistryFromBehaviorContext(const CKBehaviorContext &context) {
    return ScriptParameterRegistry::FromContext(context.Context);
}

bool GuidIsValid(CKGUID guid) {
    return guid != CKGUID();
}

std::string GuidText(CKGUID guid) {
    return ScriptGuidToString(guid);
}

const ScriptParamTypeRecord *RequireType(ScriptParameterRegistry *registry, const std::string &typeName) {
    return registry ? registry->GetType(typeName) : nullptr;
}

const ScriptParamTypeRecord *RequireType(ScriptParameterRegistry *registry, CKGUID guid) {
    return registry ? registry->GetType(guid) : nullptr;
}

void SetRegistryException(const std::string &message) {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message.c_str());
    }
}

std::string TypeRecordHeader(const ScriptParamTypeRecord &record) {
    std::ostringstream out;
    out << record.Name << " " << GuidText(record.Guid)
        << " type=" << record.Type
        << " size=" << record.DefaultSize
        << " flags=0x" << std::hex << record.CkFlags << std::dec
        << " caps=0x" << std::hex << record.Caps << std::dec
        << " generation=" << record.Generation;
    return out.str();
}

bool TypeRecordMatchesExact(const ScriptParamTypeRecord &record, const std::string &queryLower) {
    return ScriptParameterText::ToLower(record.Name) == queryLower ||
           ScriptParameterText::ToLower(GuidText(record.Guid)) == queryLower ||
           std::to_string(record.Type) == queryLower;
}

bool TypeRecordMatchesSearch(const ScriptParamTypeRecord &record, const std::string &queryLower) {
    if (TypeRecordMatchesExact(record, queryLower)) {
        return true;
    }
    const std::string nameLower = ScriptParameterText::ToLower(record.Name);
    const std::string guidLower = ScriptParameterText::ToLower(GuidText(record.Guid));
    return nameLower.find(queryLower) != std::string::npos ||
           guidLower.find(queryLower) != std::string::npos;
}

ParamTypeInfo *ParamTypeFromName(CKContext *context, const std::string &typeName) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    const ScriptParamTypeRecord *record = RequireType(registry, typeName);
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

int ParamCount(CKContext *context) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return registry ? registry->Count() : 0;
}

int ParamCountCtx(const CKBehaviorContext &ctx) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return registry ? registry->Count() : 0;
}

ParamTypeInfo *ParamAt(CKContext *context, int index) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    const ScriptParamTypeRecord *record = registry ? registry->At(index) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *ParamAtCtx(const CKBehaviorContext &ctx, int index) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    const ScriptParamTypeRecord *record = registry ? registry->At(index) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *ParamFind(CKContext *context, const std::string &query, int occurrence) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    const ScriptParamTypeRecord *record = registry ? registry->Find(query, occurrence) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *ParamFindCtx(const CKBehaviorContext &ctx, const std::string &query, int occurrence) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    const ScriptParamTypeRecord *record = registry ? registry->Find(query, occurrence) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *ParamTypeFromNameCtx(const CKBehaviorContext &ctx, const std::string &typeName) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    const ScriptParamTypeRecord *record = RequireType(registry, typeName);
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *ParamTypeFromGuid(CKContext *context, CKGUID guid) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    const ScriptParamTypeRecord *record = RequireType(registry, guid);
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *ParamTypeFromGuidCtx(const CKBehaviorContext &ctx, CKGUID guid) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    const ScriptParamTypeRecord *record = RequireType(registry, guid);
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

CKGUID ParamGuidFromName(CKContext *context, const std::string &typeName) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    const ScriptParamTypeRecord *record = RequireType(registry, typeName);
    return record ? record->Guid : CKGUID();
}

CKGUID ParamGuidFromNameCtx(const CKBehaviorContext &ctx, const std::string &typeName) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    const ScriptParamTypeRecord *record = RequireType(registry, typeName);
    return record ? record->Guid : CKGUID();
}

bool ParamIsEnum(ScriptParameterRegistry *registry, const ScriptParamTypeRecord *record) {
    return registry && record && record->Has(ScriptParamTypeCaps::EnumLike);
}

bool ParamIsFlags(ScriptParameterRegistry *registry, const ScriptParamTypeRecord *record) {
    return registry && record && record->Has(ScriptParamTypeCaps::FlagsLike);
}

bool ParamIsEnumByName(CKContext *context, const std::string &typeName) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamIsEnum(registry, RequireType(registry, typeName));
}

bool ParamIsEnumByNameCtx(const CKBehaviorContext &ctx, const std::string &typeName) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamIsEnum(registry, RequireType(registry, typeName));
}

bool ParamIsEnumByGuid(CKContext *context, CKGUID guid) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamIsEnum(registry, RequireType(registry, guid));
}

bool ParamIsEnumByGuidCtx(const CKBehaviorContext &ctx, CKGUID guid) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamIsEnum(registry, RequireType(registry, guid));
}

bool ParamIsFlagsByName(CKContext *context, const std::string &typeName) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamIsFlags(registry, RequireType(registry, typeName));
}

bool ParamIsFlagsByNameCtx(const CKBehaviorContext &ctx, const std::string &typeName) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamIsFlags(registry, RequireType(registry, typeName));
}

bool ParamIsFlagsByGuid(CKContext *context, CKGUID guid) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamIsFlags(registry, RequireType(registry, guid));
}

bool ParamIsFlagsByGuidCtx(const CKBehaviorContext &ctx, CKGUID guid) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamIsFlags(registry, RequireType(registry, guid));
}

int ParamValueFromRecord(ScriptParameterRegistry *registry,
                         const ScriptParamTypeRecord *record,
                         const std::string &nameOrValue,
                         int fallback) {
    int value = fallback;
    std::string error;
    return record && registry && registry->ParseEnumValue(record->Guid, nameOrValue, value, error) ? value : fallback;
}

int ParamValueByName(CKContext *context, const std::string &typeName, const std::string &nameOrValue, int fallback) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamValueFromRecord(registry, RequireType(registry, typeName), nameOrValue, fallback);
}

int ParamValueByNameCtx(const CKBehaviorContext &ctx, const std::string &typeName, const std::string &nameOrValue, int fallback) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamValueFromRecord(registry, RequireType(registry, typeName), nameOrValue, fallback);
}

int ParamValueByGuid(CKContext *context, CKGUID guid, const std::string &nameOrValue, int fallback) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamValueFromRecord(registry, RequireType(registry, guid), nameOrValue, fallback);
}

int ParamValueByGuidCtx(const CKBehaviorContext &ctx, CKGUID guid, const std::string &nameOrValue, int fallback) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamValueFromRecord(registry, RequireType(registry, guid), nameOrValue, fallback);
}

CKDWORD ParamFlagsMaskFromRecord(ScriptParameterRegistry *registry,
                                 const ScriptParamTypeRecord *record,
                                 const std::string &namesOrMask,
                                 CKDWORD fallback) {
    CKDWORD value = fallback;
    std::string error;
    return record && registry && registry->ParseFlagsValue(record->Guid, namesOrMask, value, error) ? value : fallback;
}

CKDWORD ParamFlagByName(CKContext *context, const std::string &typeName, const std::string &flagName, CKDWORD fallback) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamFlagsMaskFromRecord(registry, RequireType(registry, typeName), flagName, fallback);
}

CKDWORD ParamFlagByNameCtx(const CKBehaviorContext &ctx, const std::string &typeName, const std::string &flagName, CKDWORD fallback) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamFlagsMaskFromRecord(registry, RequireType(registry, typeName), flagName, fallback);
}

CKDWORD ParamFlagByGuid(CKContext *context, CKGUID guid, const std::string &flagName, CKDWORD fallback) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamFlagsMaskFromRecord(registry, RequireType(registry, guid), flagName, fallback);
}

CKDWORD ParamFlagByGuidCtx(const CKBehaviorContext &ctx, CKGUID guid, const std::string &flagName, CKDWORD fallback) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamFlagsMaskFromRecord(registry, RequireType(registry, guid), flagName, fallback);
}

CKDWORD ParamFlagsMaskByNameValue(CKContext *context, const std::string &typeName, const std::string &namesOrMask, CKDWORD fallback) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamFlagsMaskFromRecord(registry, RequireType(registry, typeName), namesOrMask, fallback);
}

CKDWORD ParamFlagsMaskByNameValueCtx(const CKBehaviorContext &ctx, const std::string &typeName, const std::string &namesOrMask, CKDWORD fallback) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamFlagsMaskFromRecord(registry, RequireType(registry, typeName), namesOrMask, fallback);
}

CKDWORD ParamFlagsMaskByGuidValue(CKContext *context, CKGUID guid, const std::string &namesOrMask, CKDWORD fallback) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamFlagsMaskFromRecord(registry, RequireType(registry, guid), namesOrMask, fallback);
}

CKDWORD ParamFlagsMaskByGuidValueCtx(const CKBehaviorContext &ctx, CKGUID guid, const std::string &namesOrMask, CKDWORD fallback) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamFlagsMaskFromRecord(registry, RequireType(registry, guid), namesOrMask, fallback);
}

std::string ParamNumericText(CKContext *context, const ScriptParamTypeRecord *record, CKDWORD value) {
    if (!record) {
        return std::to_string(value);
    }

    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    if (record->Has(ScriptParamTypeCaps::FlagsLike) && registry) {
        return registry->FlagsText(record->Guid, value);
    }

    if (record->Has(ScriptParamTypeCaps::EnumLike) && registry) {
        std::string text;
        if (registry->EnumNameOf(record->Guid, static_cast<int>(value), text)) {
            return text;
        }
    }

    CKParameterLocal *param = context ? context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_ParamText"), record->Guid, TRUE) : nullptr;
    if (!param) {
        return std::to_string(value);
    }

    std::string text;
    std::string error;
    const CKERROR err = WriteParameterValue(param, MakeScriptParamInt(static_cast<int>(value)), error);
    if (err == CK_OK && ReadParameterText(param, text, nullptr) && !text.empty()) {
        context->DestroyObject(param);
        return text;
    }

    context->DestroyObject(param);
    return std::to_string(value);
}

std::string ParamTextByNameInt(CKContext *context, const std::string &typeName, int value) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamNumericText(context, RequireType(registry, typeName), static_cast<CKDWORD>(value));
}

std::string ParamTextByNameIntCtx(const CKBehaviorContext &ctx, const std::string &typeName, int value) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamNumericText(ctx.Context, RequireType(registry, typeName), static_cast<CKDWORD>(value));
}

std::string ParamTextByGuidInt(CKContext *context, CKGUID guid, int value) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamNumericText(context, RequireType(registry, guid), static_cast<CKDWORD>(value));
}

std::string ParamTextByGuidIntCtx(const CKBehaviorContext &ctx, CKGUID guid, int value) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamNumericText(ctx.Context, RequireType(registry, guid), static_cast<CKDWORD>(value));
}

std::string ParamTextByNameDword(CKContext *context, const std::string &typeName, CKDWORD value) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamNumericText(context, RequireType(registry, typeName), value);
}

std::string ParamTextByNameDwordCtx(const CKBehaviorContext &ctx, const std::string &typeName, CKDWORD value) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamNumericText(ctx.Context, RequireType(registry, typeName), value);
}

std::string ParamTextByGuidDword(CKContext *context, CKGUID guid, CKDWORD value) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamNumericText(context, RequireType(registry, guid), value);
}

std::string ParamTextByGuidDwordCtx(const CKBehaviorContext &ctx, CKGUID guid, CKDWORD value) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamNumericText(ctx.Context, RequireType(registry, guid), value);
}

std::string ParamDescribeRecord(const ScriptParamTypeRecord *record, ScriptParameterRegistry *registry) {
    if (!record || !registry) {
        return std::string();
    }
    ParamTypeInfo info(registry, record->Type);
    return info.Describe();
}

std::string ParamDescribeByName(CKContext *context, const std::string &typeName) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamDescribeRecord(RequireType(registry, typeName), registry);
}

std::string ParamDescribeByNameCtx(const CKBehaviorContext &ctx, const std::string &typeName) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamDescribeRecord(RequireType(registry, typeName), registry);
}

std::string ParamDescribeByGuid(CKContext *context, CKGUID guid) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return ParamDescribeRecord(RequireType(registry, guid), registry);
}

std::string ParamDescribeByGuidCtx(const CKBehaviorContext &ctx, CKGUID guid) {
    ScriptParameterRegistry *registry = RegistryFromBehaviorContext(ctx);
    return ParamDescribeRecord(RequireType(registry, guid), registry);
}

ParamValue *ParamEnumByName(const CKBehaviorContext &ctx, const std::string &typeName, const std::string &nameOrValue);
ParamValue *ParamEnumByGuid(const CKBehaviorContext &ctx, CKGUID guid, const std::string &nameOrValue);
ParamValue *ParamEnumIntByName(const CKBehaviorContext &ctx, const std::string &typeName, int value);
ParamValue *ParamEnumIntByGuid(const CKBehaviorContext &ctx, CKGUID guid, int value);
ParamValue *ParamFlagsByName(const CKBehaviorContext &ctx, const std::string &typeName, const std::string &namesOrMask);
ParamValue *ParamFlagsByGuid(const CKBehaviorContext &ctx, CKGUID guid, const std::string &namesOrMask);
ParamValue *ParamFlagsMaskByName(const CKBehaviorContext &ctx, const std::string &typeName, CKDWORD value);
ParamValue *ParamFlagsMaskByGuid(const CKBehaviorContext &ctx, CKGUID guid, CKDWORD value);

CKERROR RegisterEnumByContext(CKContext *context, CKGUID guid, const std::string &name, const std::string &data) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return registry ? registry->RegisterEnum(guid, name, data) : CKERR_INVALIDPARAMETER;
}

CKERROR RegisterEnumByBehaviorContext(const CKBehaviorContext &ctx, CKGUID guid, const std::string &name, const std::string &data) {
    return RegisterEnumByContext(ctx.Context, guid, name, data);
}

CKERROR RegisterFlagsByContext(CKContext *context, CKGUID guid, const std::string &name, const std::string &data) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    return registry ? registry->RegisterFlags(guid, name, data) : CKERR_INVALIDPARAMETER;
}

CKERROR RegisterFlagsByBehaviorContext(const CKBehaviorContext &ctx, CKGUID guid, const std::string &name, const std::string &data) {
    return RegisterFlagsByContext(ctx.Context, guid, name, data);
}

CKERROR RegisterStructByContext(CKContext *context, CKGUID guid, const std::string &name, const std::string &members, XArray<CKGUID> &memberGuids) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    if (!registry) {
        return CKERR_INVALIDPARAMETER;
    }
    std::vector<CKGUID> guids;
    guids.reserve(memberGuids.Size());
    for (int i = 0; i < memberGuids.Size(); ++i) {
        guids.push_back(memberGuids[i]);
    }
    return registry->RegisterStruct(guid, name, members, guids);
}

CKERROR RegisterStructByBehaviorContext(const CKBehaviorContext &ctx, CKGUID guid, const std::string &name, const std::string &members, XArray<CKGUID> &memberGuids) {
    return RegisterStructByContext(ctx.Context, guid, name, members, memberGuids);
}

}

ScriptParameterRegistry::ScriptParameterRegistry(CKContext *context)
    : m_Context(context) {}

CKParameterManager *ScriptParameterRegistry::GetParameterManager() const {
    return ScriptParameterRegistryInternal::ParameterManagerFromContext(m_Context);
}

void ScriptParameterRegistry::Invalidate() {
    m_TypeCache.clear();
    m_NameToTypeCache.clear();
    m_GuidToTypeCache.clear();
    ++m_Generation;
    if (m_Generation == 0) {
        m_Generation = 1;
    }
}

CKParameterType ScriptParameterRegistry::ResolveType(const std::string &typeName) {
    CKParameterManager *pm = GetParameterManager();
    if (!pm) {
        return -1;
    }

    const std::string text = ScriptParameterText::StripQuotes(typeName);
    if (text.empty()) {
        return -1;
    }

    const std::string lower = ScriptParameterText::ToLower(text);
    auto cached = m_NameToTypeCache.find(lower);
    if (cached != m_NameToTypeCache.end()) {
        return cached->second;
    }

    CKGUID guid;
    if (ParseScriptGuidString(text, guid)) {
        const CKParameterType type = ResolveType(guid);
        m_NameToTypeCache[lower] = type;
        return type;
    }

    CKParameterType type = pm->ParameterNameToType(const_cast<CKSTRING>(text.c_str()));
    if (type >= 0) {
        m_NameToTypeCache[lower] = type;
        return type;
    }

    const int count = pm->GetParameterTypesCount();
    for (int i = 0; i < count; ++i) {
        CKParameterTypeDesc *desc = pm->GetParameterTypeDescription(i);
        const char *name = desc ? desc->TypeName.CStr() : nullptr;
        if (name && ScriptParameterText::ToLower(name) == lower) {
            m_NameToTypeCache[lower] = i;
            return i;
        }
    }

    m_NameToTypeCache[lower] = -1;
    return -1;
}

CKParameterType ScriptParameterRegistry::ResolveType(CKGUID guid) {
    CKParameterManager *pm = GetParameterManager();
    if (!pm || !ScriptParameterRegistryInternal::GuidIsValid(guid)) {
        return -1;
    }

    const std::string key = ScriptParameterRegistryInternal::GuidText(guid);
    auto cached = m_GuidToTypeCache.find(key);
    if (cached != m_GuidToTypeCache.end()) {
        return cached->second;
    }

    const CKParameterType type = pm->ParameterGuidToType(guid);
    m_GuidToTypeCache[key] = type;
    return type;
}

CKGUID ScriptParameterRegistry::ResolveGuid(const std::string &typeName, CKGUID fallbackGuid) {
    const CKParameterType type = ResolveType(typeName);
    CKParameterManager *pm = GetParameterManager();
    if (pm && type >= 0) {
        return pm->ParameterTypeToGuid(type);
    }
    return ScriptParameterRegistryInternal::GuidIsValid(fallbackGuid) ? fallbackGuid : CKGUID();
}

int ScriptParameterRegistry::Count() const {
    CKParameterManager *pm = GetParameterManager();
    return pm ? pm->GetParameterTypesCount() : 0;
}

const ScriptParamTypeRecord *ScriptParameterRegistry::At(int index) {
    return (index >= 0 && index < Count()) ? GetType(index) : nullptr;
}

const ScriptParamTypeRecord *ScriptParameterRegistry::Find(const std::string &query, int occurrence) {
    if (occurrence < 0) {
        return nullptr;
    }

    const std::string text = ScriptParameterText::StripQuotes(query);
    if (text.empty()) {
        return nullptr;
    }

    CKGUID guid;
    if (ParseScriptGuidString(text, guid)) {
        return occurrence == 0 ? GetType(guid) : nullptr;
    }

    const std::string lower = ScriptParameterText::ToLower(text);
    const int count = Count();
    int seen = 0;
    for (int i = 0; i < count; ++i) {
        const ScriptParamTypeRecord *record = At(i);
        if (!record || !ScriptParameterRegistryInternal::TypeRecordMatchesExact(*record, lower)) {
            continue;
        }
        if (seen == occurrence) {
            return record;
        }
        ++seen;
    }

    seen = 0;
    for (int i = 0; i < count; ++i) {
        const ScriptParamTypeRecord *record = At(i);
        if (!record ||
            ScriptParameterRegistryInternal::TypeRecordMatchesExact(*record, lower) ||
            !ScriptParameterRegistryInternal::TypeRecordMatchesSearch(*record, lower)) {
            continue;
        }
        if (seen == occurrence) {
            return record;
        }
        ++seen;
    }

    return nullptr;
}

const ScriptParamTypeRecord *ScriptParameterRegistry::GetType(CKParameterType type) {
    if (type < 0) {
        return nullptr;
    }
    auto it = m_TypeCache.find(type);
    if (it != m_TypeCache.end()) {
        return it->second.Has(ScriptParamTypeCaps::Valid) ? &it->second : nullptr;
    }
    return BuildType(type);
}

const ScriptParamTypeRecord *ScriptParameterRegistry::GetType(CKGUID guid) {
    return GetType(ResolveType(guid));
}

const ScriptParamTypeRecord *ScriptParameterRegistry::GetType(const std::string &typeName) {
    return GetType(ResolveType(typeName));
}

const ScriptParamTypeRecord *ScriptParameterRegistry::GetType(CKParameter *param) {
    return param ? GetType(param->GetType()) : nullptr;
}

bool ScriptParameterRegistry::IsTypeCompatible(CKGUID a, CKGUID b) {
    if (a == b) {
        return true;
    }
    CKParameterManager *pm = GetParameterManager();
    return pm && ScriptParameterRegistryInternal::GuidIsValid(a) && ScriptParameterRegistryInternal::GuidIsValid(b) &&
           (pm->IsTypeCompatible(a, b) != FALSE || pm->IsTypeCompatible(b, a) != FALSE);
}

bool ScriptParameterRegistry::ParseEnumValue(CKGUID typeGuid, const std::string &nameOrValue, int &value, std::string &error) {
    if (ScriptParameterText::ParseInteger(nameOrValue, value)) {
        return true;
    }

    const ScriptParamTypeRecord *record = GetType(typeGuid);
    if (!record || !record->Has(ScriptParamTypeCaps::EnumLike)) {
        error = "Parameter type is not an enum: " + ScriptParameterRegistryInternal::GuidText(typeGuid);
        return false;
    }

    const int index = FindEnumEntry(record->EnumEntries, nameOrValue);
    if (index < 0) {
        error = "Enum token '" + nameOrValue + "' was not found in " + record->Name + ".";
        return false;
    }

    value = record->EnumEntries[index].Value;
    return true;
}

bool ScriptParameterRegistry::EnumNameOf(CKGUID typeGuid, int value, std::string &name) {
    const ScriptParamTypeRecord *record = GetType(typeGuid);
    if (!record || !record->Has(ScriptParamTypeCaps::EnumLike)) {
        return false;
    }
    for (const auto &entry : record->EnumEntries) {
        if (entry.Value == value) {
            name = entry.Name;
            return true;
        }
    }
    return false;
}

bool ScriptParameterRegistry::ParseFlagsValue(CKGUID typeGuid, const std::string &namesOrMask, CKDWORD &value, std::string &error) {
    int parsed = 0;
    if (ScriptParameterText::ParseInteger(namesOrMask, parsed)) {
        value = static_cast<CKDWORD>(parsed);
        return true;
    }

    const ScriptParamTypeRecord *record = GetType(typeGuid);
    if (!record || !record->Has(ScriptParamTypeCaps::FlagsLike)) {
        error = "Parameter type is not flags: " + ScriptParameterRegistryInternal::GuidText(typeGuid);
        return false;
    }

    const std::vector<std::string> tokens = ScriptParameterText::SplitList(namesOrMask);
    if (tokens.empty()) {
        error = "Flags value for " + record->Name + " is empty.";
        return false;
    }

    CKDWORD result = 0;
    for (const std::string &token : tokens) {
        int tokenNumber = 0;
        if (ScriptParameterText::ParseInteger(token, tokenNumber)) {
            result |= static_cast<CKDWORD>(tokenNumber);
            continue;
        }
        const int index = FindEnumEntry(record->FlagEntries, token);
        if (index < 0) {
            error = "Flags token '" + token + "' was not found in " + record->Name + ".";
            return false;
        }
        result |= static_cast<CKDWORD>(record->FlagEntries[index].Value);
    }

    value = result;
    return true;
}

std::string ScriptParameterRegistry::FlagsText(CKGUID typeGuid, CKDWORD value) {
    const ScriptParamTypeRecord *record = GetType(typeGuid);
    if (!record || !record->Has(ScriptParamTypeCaps::FlagsLike)) {
        return std::to_string(value);
    }

    std::string result;
    for (const auto &entry : record->FlagEntries) {
        const CKDWORD flag = static_cast<CKDWORD>(entry.Value);
        if (flag != 0 && (value & flag) == flag) {
            if (!result.empty()) {
                result += ",";
            }
            result += entry.Name;
        }
    }
    return result.empty() ? std::to_string(value) : result;
}

CKERROR ScriptParameterRegistry::RegisterEnum(CKGUID guid, const std::string &name, const std::string &data) {
    CKParameterManager *pm = GetParameterManager();
    CKERROR err = pm ? pm->RegisterNewEnum(guid, const_cast<CKSTRING>(name.c_str()), const_cast<CKSTRING>(data.c_str())) : CKERR_INVALIDPARAMETER;
    Invalidate();
    return err;
}

CKERROR ScriptParameterRegistry::RegisterFlags(CKGUID guid, const std::string &name, const std::string &data) {
    CKParameterManager *pm = GetParameterManager();
    CKERROR err = pm ? pm->RegisterNewFlags(guid, const_cast<CKSTRING>(name.c_str()), const_cast<CKSTRING>(data.c_str())) : CKERR_INVALIDPARAMETER;
    Invalidate();
    return err;
}

CKERROR ScriptParameterRegistry::RegisterStruct(CKGUID guid, const std::string &name, const std::string &memberNames, const std::vector<CKGUID> &memberGuids) {
    CKParameterManager *pm = GetParameterManager();
    if (!pm) {
        return CKERR_INVALIDPARAMETER;
    }

    XArray<CKGUID> guids;
    for (CKGUID memberGuid : memberGuids) {
        guids.PushBack(memberGuid);
    }

    CKERROR err = pm->RegisterNewStructure(guid, const_cast<CKSTRING>(name.c_str()), const_cast<CKSTRING>(memberNames.c_str()), guids);
    Invalidate();
    return err;
}

ScriptParameterRegistry *ScriptParameterRegistry::FromContext(CKContext *context) {
    ScriptManager *manager = context ? ScriptManager::GetManager(context) : nullptr;
    return manager ? manager->GetParameterRegistry() : nullptr;
}

const ScriptParamTypeRecord *ScriptParameterRegistry::BuildType(CKParameterType type) {
    ScriptParamTypeRecord record;
    if (!PopulateFromTypeDesc(record, type)) {
        m_TypeCache[type] = record;
        return nullptr;
    }
    auto inserted = m_TypeCache.emplace(type, record);
    if (!inserted.second) {
        inserted.first->second = record;
    }
    return &inserted.first->second;
}

bool ScriptParameterRegistry::PopulateFromTypeDesc(ScriptParamTypeRecord &record, CKParameterType type) {
    CKParameterManager *pm = GetParameterManager();
    CKParameterTypeDesc *desc = pm && type >= 0 ? pm->GetParameterTypeDescription(type) : nullptr;
    if (!desc) {
        return false;
    }

    record.Type = type;
    record.Guid = desc->Guid;
    record.DerivedFrom = desc->DerivedFrom;
    record.Name = desc->TypeName.CStr() ? desc->TypeName.CStr() : "";
    record.DefaultSize = desc->DefaultSize;
    record.CkFlags = desc->dwFlags;
    record.Generation = m_Generation;
    record.ClassId = static_cast<CK_CLASSID>(desc->Cid);
    m_NameToTypeCache[ScriptParameterText::ToLower(record.Name)] = type;
    m_GuidToTypeCache[ScriptParameterRegistryInternal::GuidText(record.Guid)] = type;
    SetRecordCap(record, ScriptParamTypeCaps::Valid, desc->Valid != 0);
    SetRecordCap(record, ScriptParamTypeCaps::Stringable, desc->StringFunction != nullptr);
    SetRecordCap(record, ScriptParamTypeCaps::VariableSize, (desc->dwFlags & CKPARAMETERTYPE_VARIABLESIZE) != 0);
    SetRecordCap(record, ScriptParamTypeCaps::HasLifecycle, desc->DeleteFunction != nullptr || desc->SaveLoadFunction != nullptr);
    SetRecordCap(record, ScriptParamTypeCaps::FixedSize, desc->DefaultSize > 0 && !record.Has(ScriptParamTypeCaps::VariableSize));
    SetRecordCap(record, ScriptParamTypeCaps::StructLike, (desc->dwFlags & CKPARAMETERTYPE_STRUCT) != 0);
    SetRecordCap(record, ScriptParamTypeCaps::EnumLike, (desc->dwFlags & CKPARAMETERTYPE_ENUMS) != 0);
    SetRecordCap(record, ScriptParamTypeCaps::FlagsLike, (desc->dwFlags & CKPARAMETERTYPE_FLAGS) != 0);
    SetRecordCap(record, ScriptParamTypeCaps::ObjectLike, desc->Cid != 0 || IsTypeCompatible(desc->Guid, CKPGUID_OBJECT));
    SetRecordCap(record, ScriptParamTypeCaps::CollectionLike, IsTypeCompatible(desc->Guid, CKPGUID_OBJECTARRAY));
    SetRecordCap(record, ScriptParamTypeCaps::IntLike, IsTypeCompatible(desc->Guid, CKPGUID_INT) ||
        IsTypeCompatible(desc->Guid, CKPGUID_CLASSID) ||
        IsTypeCompatible(desc->Guid, CKPGUID_PARAMETERTYPE));
    SetRecordCap(record, ScriptParamTypeCaps::FloatLike, IsTypeCompatible(desc->Guid, CKPGUID_FLOAT));
    SetRecordCap(record, ScriptParamTypeCaps::BoolLike, IsTypeCompatible(desc->Guid, CKPGUID_BOOL));
    SetRecordCap(record, ScriptParamTypeCaps::StringLike, IsTypeCompatible(desc->Guid, CKPGUID_STRING));

    if (CKEnumStruct *enumDesc = pm->GetEnumDescByType(type)) {
        SetRecordCap(record, ScriptParamTypeCaps::EnumLike, true);
        for (int i = 0; i < enumDesc->GetNumEnums(); ++i) {
            ScriptParamEnumEntry entry;
            entry.Name = enumDesc->GetEnumDescription(i) ? enumDesc->GetEnumDescription(i) : "";
            entry.Value = enumDesc->GetEnumValue(i);
            record.EnumEntries.push_back(entry);
        }
    }

    if (CKFlagsStruct *flagsDesc = pm->GetFlagsDescByType(type)) {
        SetRecordCap(record, ScriptParamTypeCaps::FlagsLike, true);
        for (int i = 0; i < flagsDesc->GetNumFlags(); ++i) {
            ScriptParamEnumEntry entry;
            entry.Name = flagsDesc->GetFlagDescription(i) ? flagsDesc->GetFlagDescription(i) : "";
            entry.Value = flagsDesc->GetFlagValue(i);
            record.FlagEntries.push_back(entry);
        }
    }

    if (CKStructStruct *structDesc = pm->GetStructDescByType(type)) {
        SetRecordCap(record, ScriptParamTypeCaps::StructLike, true);
        for (int i = 0; i < structDesc->GetNumSubParam(); ++i) {
            ScriptParamStructMember member;
            member.Name = structDesc->GetSubParamDescription(i) ? structDesc->GetSubParamDescription(i) : "";
            member.Guid = structDesc->GetSubParamGuid(i);
            member.Type = ResolveType(member.Guid);
            record.StructMembers.push_back(member);
        }
    }

    UpdateRecordFamily(record);
    return true;
}

void ScriptParameterRegistry::SetRecordCap(ScriptParamTypeRecord &record, ScriptParamTypeCaps cap, bool enabled) const {
    SetScriptParamTypeCap(record.Caps, cap, enabled);
}

void ScriptParameterRegistry::UpdateRecordFamily(ScriptParamTypeRecord &record) const {
    if (record.Has(ScriptParamTypeCaps::StructLike)) {
        record.Family = ScriptParamTypeFamily::Struct;
    } else if (record.Has(ScriptParamTypeCaps::FlagsLike)) {
        record.Family = ScriptParamTypeFamily::Flags;
    } else if (record.Has(ScriptParamTypeCaps::EnumLike)) {
        record.Family = ScriptParamTypeFamily::Enum;
    } else if (record.Has(ScriptParamTypeCaps::CollectionLike)) {
        record.Family = ScriptParamTypeFamily::Collection;
    } else if (record.Has(ScriptParamTypeCaps::ObjectLike)) {
        record.Family = ScriptParamTypeFamily::Object;
    } else if (record.Has(ScriptParamTypeCaps::StringLike)) {
        record.Family = ScriptParamTypeFamily::Text;
    } else if (record.Has(ScriptParamTypeCaps::IntLike) ||
               record.Has(ScriptParamTypeCaps::FloatLike) ||
               record.Has(ScriptParamTypeCaps::BoolLike)) {
        record.Family = ScriptParamTypeFamily::Scalar;
    } else {
        record.Family = ScriptParamTypeFamily::Custom;
    }
}

int ScriptParameterRegistry::FindEnumEntry(const std::vector<ScriptParamEnumEntry> &entries, const std::string &name) const {
    const std::string wanted = ScriptParameterText::StripQuotes(name);
    for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
        if (entries[i].Name == wanted) {
            return i;
        }
    }
    const std::string wantedLower = ScriptParameterText::ToLower(wanted);
    for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
        if (ScriptParameterText::ToLower(entries[i].Name) == wantedLower) {
            return i;
        }
    }
    return -1;
}

ParamTypeInfo::ParamTypeInfo(ScriptParameterRegistry *registry, CKParameterType type)
    : m_Registry(registry), m_Type(type) {}

const ScriptParamTypeRecord *ParamTypeInfo::Record() const {
    return m_Registry ? m_Registry->GetType(m_Type) : nullptr;
}

bool ParamTypeInfo::IsValid() const { return Record() != nullptr; }
CKParameterType ParamTypeInfo::Type() const { const auto *r = Record(); return r ? r->Type : -1; }
CKGUID ParamTypeInfo::Guid() const { const auto *r = Record(); return r ? r->Guid : CKGUID(); }
std::string ParamTypeInfo::Name() const { const auto *r = Record(); return r ? r->Name : std::string(); }
int ParamTypeInfo::Flags() const { const auto *r = Record(); return r ? static_cast<int>(r->CkFlags) : 0; }
int ParamTypeInfo::DefaultSize() const { const auto *r = Record(); return r ? r->DefaultSize : 0; }
int ParamTypeInfo::ClassId() const { const auto *r = Record(); return r ? static_cast<int>(r->ClassId) : 0; }
bool ParamTypeInfo::IsEnum() const { const auto *r = Record(); return r && r->Has(ScriptParamTypeCaps::EnumLike); }
bool ParamTypeInfo::IsFlags() const { const auto *r = Record(); return r && r->Has(ScriptParamTypeCaps::FlagsLike); }
bool ParamTypeInfo::IsStruct() const { const auto *r = Record(); return r && r->Has(ScriptParamTypeCaps::StructLike); }
bool ParamTypeInfo::IsObject() const { const auto *r = Record(); return r && r->Has(ScriptParamTypeCaps::ObjectLike); }
bool ParamTypeInfo::IsCollection() const { const auto *r = Record(); return r && r->Has(ScriptParamTypeCaps::CollectionLike); }
bool ParamTypeInfo::IsStringable() const { const auto *r = Record(); return r && r->Has(ScriptParamTypeCaps::Stringable); }
bool ParamTypeInfo::IsFixedSize() const { const auto *r = Record(); return r && r->Has(ScriptParamTypeCaps::FixedSize); }
ParamEnumInfo *ParamTypeInfo::Enum() const { return IsEnum() ? new ParamEnumInfo(m_Registry, m_Type) : nullptr; }
ParamFlagsInfo *ParamTypeInfo::FlagsInfo() const { return IsFlags() ? new ParamFlagsInfo(m_Registry, m_Type) : nullptr; }
ParamStructInfo *ParamTypeInfo::Struct() const { return IsStruct() ? new ParamStructInfo(m_Registry, m_Type) : nullptr; }
std::string ParamTypeInfo::Describe() const {
    const auto *r = Record();
    if (!r) {
        return "ParamTypeInfo is not valid.";
    }
    std::ostringstream out;
    out << ScriptParameterRegistryInternal::TypeRecordHeader(*r);
    if (r->Has(ScriptParamTypeCaps::EnumLike)) out << " [enum count=" << r->EnumEntries.size() << "]";
    if (r->Has(ScriptParamTypeCaps::FlagsLike)) out << " [flags count=" << r->FlagEntries.size() << "]";
    if (r->Has(ScriptParamTypeCaps::StructLike)) out << " [struct members=" << r->StructMembers.size() << "]";
    return out.str();
}

ParamEnumInfo::ParamEnumInfo(ScriptParameterRegistry *registry, CKParameterType type)
    : m_Registry(registry), m_Type(type) {}

const ScriptParamTypeRecord *ParamEnumInfo::Record() const {
    const auto *record = m_Registry ? m_Registry->GetType(m_Type) : nullptr;
    return record && record->Has(ScriptParamTypeCaps::EnumLike) ? record : nullptr;
}

bool ParamEnumInfo::IsValid() const { return Record() != nullptr; }
int ParamEnumInfo::Count() const { const auto *r = Record(); return r ? static_cast<int>(r->EnumEntries.size()) : 0; }
std::string ParamEnumInfo::Name(int index) const { const auto *r = Record(); return r && index >= 0 && index < Count() ? r->EnumEntries[index].Name : std::string(); }
int ParamEnumInfo::Value(int index) const { const auto *r = Record(); return r && index >= 0 && index < Count() ? r->EnumEntries[index].Value : 0; }
int ParamEnumInfo::Find(const std::string &nameOrValue) const {
    const auto *r = Record();
    int value = 0;
    std::string error;
    if (r && m_Registry->ParseEnumValue(r->Guid, nameOrValue, value, error)) {
        return value;
    }
    ScriptParameterRegistryInternal::SetRegistryException(error.empty() ? "Enum value was not found." : error);
    return 0;
}
std::string ParamEnumInfo::NameOf(int value) const {
    const auto *r = Record();
    std::string text;
    return r && m_Registry->EnumNameOf(r->Guid, value, text) ? text : std::to_string(value);
}
std::string ParamEnumInfo::Describe() const {
    const auto *r = Record();
    if (!r) return "ParamEnumInfo is not valid.";
    std::ostringstream out;
    out << ScriptParameterRegistryInternal::TypeRecordHeader(*r) << " [enum]";
    for (const auto &entry : r->EnumEntries) {
        out << "\n  " << entry.Name << " = " << entry.Value;
    }
    return out.str();
}

ParamFlagsInfo::ParamFlagsInfo(ScriptParameterRegistry *registry, CKParameterType type)
    : m_Registry(registry), m_Type(type) {}

const ScriptParamTypeRecord *ParamFlagsInfo::Record() const {
    const auto *record = m_Registry ? m_Registry->GetType(m_Type) : nullptr;
    return record && record->Has(ScriptParamTypeCaps::FlagsLike) ? record : nullptr;
}

bool ParamFlagsInfo::IsValid() const { return Record() != nullptr; }
int ParamFlagsInfo::Count() const { const auto *r = Record(); return r ? static_cast<int>(r->FlagEntries.size()) : 0; }
std::string ParamFlagsInfo::Name(int index) const { const auto *r = Record(); return r && index >= 0 && index < Count() ? r->FlagEntries[index].Name : std::string(); }
CKDWORD ParamFlagsInfo::Value(int index) const { const auto *r = Record(); return r && index >= 0 && index < Count() ? static_cast<CKDWORD>(r->FlagEntries[index].Value) : 0; }
CKDWORD ParamFlagsInfo::Parse(const std::string &namesOrMask) const {
    const auto *r = Record();
    CKDWORD value = 0;
    std::string error;
    if (r && m_Registry->ParseFlagsValue(r->Guid, namesOrMask, value, error)) {
        return value;
    }
    ScriptParameterRegistryInternal::SetRegistryException(error.empty() ? "Flags value was not found." : error);
    return 0;
}
std::string ParamFlagsInfo::Text(CKDWORD value) const {
    const auto *r = Record();
    return r ? m_Registry->FlagsText(r->Guid, value) : std::to_string(value);
}
bool ParamFlagsInfo::Has(CKDWORD mask, const std::string &flagName) const {
    CKDWORD flag = Parse(flagName);
    return flag != 0 && (mask & flag) == flag;
}
std::string ParamFlagsInfo::Describe() const {
    const auto *r = Record();
    if (!r) return "ParamFlagsInfo is not valid.";
    std::ostringstream out;
    out << ScriptParameterRegistryInternal::TypeRecordHeader(*r) << " [flags]";
    for (const auto &entry : r->FlagEntries) {
        out << "\n  " << entry.Name << " = " << entry.Value;
    }
    return out.str();
}

ParamStructInfo::ParamStructInfo(ScriptParameterRegistry *registry, CKParameterType type)
    : m_Registry(registry), m_Type(type) {}

const ScriptParamTypeRecord *ParamStructInfo::Record() const {
    const auto *record = m_Registry ? m_Registry->GetType(m_Type) : nullptr;
    return record && record->Has(ScriptParamTypeCaps::StructLike) ? record : nullptr;
}

bool ParamStructInfo::IsValid() const { return Record() != nullptr; }
int ParamStructInfo::Count() const { const auto *r = Record(); return r ? static_cast<int>(r->StructMembers.size()) : 0; }
std::string ParamStructInfo::MemberName(int index) const { const auto *r = Record(); return r && index >= 0 && index < Count() ? r->StructMembers[index].Name : std::string(); }
CKGUID ParamStructInfo::MemberGuid(int index) const { const auto *r = Record(); return r && index >= 0 && index < Count() ? r->StructMembers[index].Guid : CKGUID(); }
ParamTypeInfo *ParamStructInfo::MemberType(int index) const {
    const auto *r = Record();
    return r && index >= 0 && index < Count() ? new ParamTypeInfo(m_Registry, r->StructMembers[index].Type) : nullptr;
}
int ParamStructInfo::FindMember(const std::string &name, int occurrence) const {
    const auto *r = Record();
    if (!r || occurrence < 0) {
        return -1;
    }
    int seen = 0;
    for (int i = 0; i < static_cast<int>(r->StructMembers.size()); ++i) {
        if (r->StructMembers[i].Name == name) {
            if (seen == occurrence) {
                return i;
            }
            ++seen;
        }
    }
    return -1;
}
std::string ParamStructInfo::Describe() const {
    const auto *r = Record();
    if (!r) return "ParamStructInfo is not valid.";
    std::ostringstream out;
    out << ScriptParameterRegistryInternal::TypeRecordHeader(*r) << " [struct]";
    for (int i = 0; i < static_cast<int>(r->StructMembers.size()); ++i) {
        const auto &member = r->StructMembers[i];
        const auto *memberRecord = m_Registry ? m_Registry->GetType(member.Guid) : nullptr;
        out << "\n  #" << i << " " << member.Name << ": "
            << (memberRecord ? memberRecord->Name : ScriptParameterRegistryInternal::GuidText(member.Guid));
    }
    return out.str();
}

namespace ScriptParameterRegistryRegistrationInternal {

template <typename T>
void RegisterRefCountedBehaviors(asIScriptEngine *engine, const char *typeName, int &r) {
    r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_ADDREF, "void f()", asMETHOD(T, AddRef), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_RELEASE, "void f()", asMETHOD(T, Release), asCALL_THISCALL); assert(r >= 0);
}

void RegisterParamInfoTypes(asIScriptEngine *engine, int &r) {
    r = engine->RegisterObjectType("ParamTypeInfo", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("ParamEnumInfo", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("ParamFlagsInfo", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("ParamStructInfo", 0, asOBJ_REF); assert(r >= 0);
}

void RegisterParamTypeInfoMethods(asIScriptEngine *engine, int &r) {
    RegisterRefCountedBehaviors<ParamTypeInfo>(engine, "ParamTypeInfo", r);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "bool IsValid() const", asMETHOD(ParamTypeInfo, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "CKParameterType Type() const", asMETHOD(ParamTypeInfo, Type), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "CKGUID Guid() const", asMETHOD(ParamTypeInfo, Guid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "string Name() const", asMETHOD(ParamTypeInfo, Name), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "int TypeFlags() const", asMETHOD(ParamTypeInfo, Flags), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "int DefaultSize() const", asMETHOD(ParamTypeInfo, DefaultSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "int ClassId() const", asMETHOD(ParamTypeInfo, ClassId), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "bool IsEnum() const", asMETHOD(ParamTypeInfo, IsEnum), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "bool IsFlags() const", asMETHOD(ParamTypeInfo, IsFlags), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "bool IsStruct() const", asMETHOD(ParamTypeInfo, IsStruct), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "bool IsObject() const", asMETHOD(ParamTypeInfo, IsObject), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "bool IsCollection() const", asMETHOD(ParamTypeInfo, IsCollection), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "bool IsStringable() const", asMETHOD(ParamTypeInfo, IsStringable), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "bool IsFixedSize() const", asMETHOD(ParamTypeInfo, IsFixedSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "ParamEnumInfo@ Enum() const", asMETHOD(ParamTypeInfo, Enum), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "ParamFlagsInfo@ Flags() const", asMETHOD(ParamTypeInfo, FlagsInfo), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "ParamFlagsInfo@ FlagsDesc() const", asMETHOD(ParamTypeInfo, FlagsInfo), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "ParamStructInfo@ Struct() const", asMETHOD(ParamTypeInfo, Struct), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamTypeInfo", "string Describe() const", asMETHOD(ParamTypeInfo, Describe), asCALL_THISCALL); assert(r >= 0);
}

void RegisterParamEnumInfoMethods(asIScriptEngine *engine, int &r) {
    RegisterRefCountedBehaviors<ParamEnumInfo>(engine, "ParamEnumInfo", r);
    r = engine->RegisterObjectMethod("ParamEnumInfo", "bool IsValid() const", asMETHOD(ParamEnumInfo, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamEnumInfo", "int Count() const", asMETHOD(ParamEnumInfo, Count), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamEnumInfo", "string Name(int index) const", asMETHOD(ParamEnumInfo, Name), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamEnumInfo", "int Value(int index) const", asMETHOD(ParamEnumInfo, Value), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamEnumInfo", "int Find(const string &in nameOrValue) const", asMETHOD(ParamEnumInfo, Find), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamEnumInfo", "string NameOf(int value) const", asMETHOD(ParamEnumInfo, NameOf), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamEnumInfo", "string Describe() const", asMETHOD(ParamEnumInfo, Describe), asCALL_THISCALL); assert(r >= 0);
}

void RegisterParamFlagsInfoMethods(asIScriptEngine *engine, int &r) {
    RegisterRefCountedBehaviors<ParamFlagsInfo>(engine, "ParamFlagsInfo", r);
    r = engine->RegisterObjectMethod("ParamFlagsInfo", "bool IsValid() const", asMETHOD(ParamFlagsInfo, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamFlagsInfo", "int Count() const", asMETHOD(ParamFlagsInfo, Count), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamFlagsInfo", "string Name(int index) const", asMETHOD(ParamFlagsInfo, Name), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamFlagsInfo", "uint Value(int index) const", asMETHOD(ParamFlagsInfo, Value), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamFlagsInfo", "uint Parse(const string &in namesOrMask) const", asMETHOD(ParamFlagsInfo, Parse), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamFlagsInfo", "string Text(uint value) const", asMETHOD(ParamFlagsInfo, Text), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamFlagsInfo", "bool Has(uint mask, const string &in flagName) const", asMETHOD(ParamFlagsInfo, Has), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamFlagsInfo", "string Describe() const", asMETHOD(ParamFlagsInfo, Describe), asCALL_THISCALL); assert(r >= 0);
}

void RegisterParamStructInfoMethods(asIScriptEngine *engine, int &r) {
    RegisterRefCountedBehaviors<ParamStructInfo>(engine, "ParamStructInfo", r);
    r = engine->RegisterObjectMethod("ParamStructInfo", "bool IsValid() const", asMETHOD(ParamStructInfo, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructInfo", "int Count() const", asMETHOD(ParamStructInfo, Count), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructInfo", "string MemberName(int index) const", asMETHOD(ParamStructInfo, MemberName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructInfo", "CKGUID MemberGuid(int index) const", asMETHOD(ParamStructInfo, MemberGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructInfo", "ParamTypeInfo@ MemberType(int index) const", asMETHOD(ParamStructInfo, MemberType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructInfo", "int FindMember(const string &in name, int occurrence = 0) const", asMETHOD(ParamStructInfo, FindMember), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructInfo", "string Describe() const", asMETHOD(ParamStructInfo, Describe), asCALL_THISCALL); assert(r >= 0);
}

void RegisterParamRegistryGlobals(asIScriptEngine *engine, int &r) {
    const char *previousNamespace = engine->GetDefaultNamespace();
    std::string previous = previousNamespace ? previousNamespace : "";
    r = engine->SetDefaultNamespace("Param"); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Count(CKContext@ context)", asFUNCTION(ScriptParameterRegistryInternal::ParamCount), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Count(const CKBehaviorContext &in ctx)", asFUNCTION(ScriptParameterRegistryInternal::ParamCountCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ At(CKContext@ context, int index)", asFUNCTION(ScriptParameterRegistryInternal::ParamAt), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ At(const CKBehaviorContext &in ctx, int index)", asFUNCTION(ScriptParameterRegistryInternal::ParamAtCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Find(CKContext@ context, const string &in query, int occurrence = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFind), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Find(const CKBehaviorContext &in ctx, const string &in query, int occurrence = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFindCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Type(CKContext@ context, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamTypeFromName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Type(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamTypeFromNameCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Type(CKContext@ context, CKGUID guid)", asFUNCTION(ScriptParameterRegistryInternal::ParamTypeFromGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Type(const CKBehaviorContext &in ctx, CKGUID guid)", asFUNCTION(ScriptParameterRegistryInternal::ParamTypeFromGuidCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKGUID Guid(CKContext@ context, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamGuidFromName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKGUID Guid(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamGuidFromNameCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsEnum(CKContext@ context, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamIsEnumByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsEnum(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamIsEnumByNameCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsEnum(CKContext@ context, CKGUID guid)", asFUNCTION(ScriptParameterRegistryInternal::ParamIsEnumByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsEnum(const CKBehaviorContext &in ctx, CKGUID guid)", asFUNCTION(ScriptParameterRegistryInternal::ParamIsEnumByGuidCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsFlags(CKContext@ context, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamIsFlagsByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsFlags(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamIsFlagsByNameCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsFlags(CKContext@ context, CKGUID guid)", asFUNCTION(ScriptParameterRegistryInternal::ParamIsFlagsByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool IsFlags(const CKBehaviorContext &in ctx, CKGUID guid)", asFUNCTION(ScriptParameterRegistryInternal::ParamIsFlagsByGuidCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Value(CKContext@ context, const string &in typeName, const string &in nameOrValue, int fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamValueByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Value(const CKBehaviorContext &in ctx, const string &in typeName, const string &in nameOrValue, int fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamValueByNameCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Value(CKContext@ context, CKGUID guid, const string &in nameOrValue, int fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamValueByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Value(const CKBehaviorContext &in ctx, CKGUID guid, const string &in nameOrValue, int fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamValueByGuidCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint Flag(CKContext@ context, const string &in typeName, const string &in flagName, uint fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFlagByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint Flag(const CKBehaviorContext &in ctx, const string &in typeName, const string &in flagName, uint fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFlagByNameCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint Flag(CKContext@ context, CKGUID guid, const string &in flagName, uint fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFlagByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint Flag(const CKBehaviorContext &in ctx, CKGUID guid, const string &in flagName, uint fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFlagByGuidCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint FlagsMask(CKContext@ context, const string &in typeName, const string &in namesOrMask, uint fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFlagsMaskByNameValue), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint FlagsMask(const CKBehaviorContext &in ctx, const string &in typeName, const string &in namesOrMask, uint fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFlagsMaskByNameValueCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint FlagsMask(CKContext@ context, CKGUID guid, const string &in namesOrMask, uint fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFlagsMaskByGuidValue), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint FlagsMask(const CKBehaviorContext &in ctx, CKGUID guid, const string &in namesOrMask, uint fallback = 0)", asFUNCTION(ScriptParameterRegistryInternal::ParamFlagsMaskByGuidValueCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Text(CKContext@ context, const string &in typeName, int value)", asFUNCTION(ScriptParameterRegistryInternal::ParamTextByNameInt), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Text(const CKBehaviorContext &in ctx, const string &in typeName, int value)", asFUNCTION(ScriptParameterRegistryInternal::ParamTextByNameIntCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Text(CKContext@ context, CKGUID guid, int value)", asFUNCTION(ScriptParameterRegistryInternal::ParamTextByGuidInt), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Text(const CKBehaviorContext &in ctx, CKGUID guid, int value)", asFUNCTION(ScriptParameterRegistryInternal::ParamTextByGuidIntCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Text(CKContext@ context, const string &in typeName, uint value)", asFUNCTION(ScriptParameterRegistryInternal::ParamTextByNameDword), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Text(const CKBehaviorContext &in ctx, const string &in typeName, uint value)", asFUNCTION(ScriptParameterRegistryInternal::ParamTextByNameDwordCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Text(CKContext@ context, CKGUID guid, uint value)", asFUNCTION(ScriptParameterRegistryInternal::ParamTextByGuidDword), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Text(const CKBehaviorContext &in ctx, CKGUID guid, uint value)", asFUNCTION(ScriptParameterRegistryInternal::ParamTextByGuidDwordCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Describe(CKContext@ context, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamDescribeByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Describe(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ScriptParameterRegistryInternal::ParamDescribeByNameCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Describe(CKContext@ context, CKGUID guid)", asFUNCTION(ScriptParameterRegistryInternal::ParamDescribeByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Describe(const CKBehaviorContext &in ctx, CKGUID guid)", asFUNCTION(ScriptParameterRegistryInternal::ParamDescribeByGuidCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKERROR RegisterEnum(CKContext@ context, CKGUID guid, const string &in name, const string &in data)", asFUNCTION(ScriptParameterRegistryInternal::RegisterEnumByContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKERROR RegisterEnum(const CKBehaviorContext &in ctx, CKGUID guid, const string &in name, const string &in data)", asFUNCTION(ScriptParameterRegistryInternal::RegisterEnumByBehaviorContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKERROR RegisterFlags(CKContext@ context, CKGUID guid, const string &in name, const string &in data)", asFUNCTION(ScriptParameterRegistryInternal::RegisterFlagsByContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKERROR RegisterFlags(const CKBehaviorContext &in ctx, CKGUID guid, const string &in name, const string &in data)", asFUNCTION(ScriptParameterRegistryInternal::RegisterFlagsByBehaviorContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKERROR RegisterStruct(CKContext@ context, CKGUID guid, const string &in name, const string &in members, XGUIDArray &in memberGuids)", asFUNCTION(ScriptParameterRegistryInternal::RegisterStructByContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("CKERROR RegisterStruct(const CKBehaviorContext &in ctx, CKGUID guid, const string &in name, const string &in members, XGUIDArray &in memberGuids)", asFUNCTION(ScriptParameterRegistryInternal::RegisterStructByBehaviorContext), asCALL_CDECL); assert(r >= 0);
    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}

} // namespace ScriptParameterRegistryRegistrationInternal

void RegisterScriptParameterRegistry(asIScriptEngine *engine) {
    assert(engine != nullptr);
    int r = 0;

    ScriptParameterRegistryRegistrationInternal::RegisterParamInfoTypes(engine, r);
    ScriptParameterRegistryRegistrationInternal::RegisterParamTypeInfoMethods(engine, r);
    ScriptParameterRegistryRegistrationInternal::RegisterParamEnumInfoMethods(engine, r);
    ScriptParameterRegistryRegistrationInternal::RegisterParamFlagsInfoMethods(engine, r);
    ScriptParameterRegistryRegistrationInternal::RegisterParamStructInfoMethods(engine, r);
    ScriptParameterRegistryRegistrationInternal::RegisterParamRegistryGlobals(engine, r);
}
