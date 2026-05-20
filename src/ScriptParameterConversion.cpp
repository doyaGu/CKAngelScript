#include "ScriptParameterConversion.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "CKAll.h"
#include "ScriptParameterRegistry.h"
#include "XObjectArray.h"

ScriptParamStructMemberValue::ScriptParamStructMemberValue(const ScriptParamStructMemberValue &other)
    : Index(other.Index),
      Value(other.Value ? new ScriptParamValue(*other.Value) : nullptr) {}

ScriptParamStructMemberValue::ScriptParamStructMemberValue(ScriptParamStructMemberValue &&other) noexcept
    : Index(other.Index),
      Value(other.Value) {
    other.Index = -1;
    other.Value = nullptr;
}

ScriptParamStructMemberValue::~ScriptParamStructMemberValue() {
    delete Value;
}

ScriptParamStructMemberValue &ScriptParamStructMemberValue::operator=(const ScriptParamStructMemberValue &other) {
    if (this != &other) {
        ScriptParamStructMemberValue copy(other);
        *this = std::move(copy);
    }
    return *this;
}

ScriptParamStructMemberValue &ScriptParamStructMemberValue::operator=(ScriptParamStructMemberValue &&other) noexcept {
    if (this != &other) {
        delete Value;
        Index = other.Index;
        Value = other.Value;
        other.Index = -1;
        other.Value = nullptr;
    }
    return *this;
}

ScriptParamValue::ScriptParamValue(const ScriptParamValue &other)
    : Kind(other.Kind),
      Type(other.Type),
      TypeGuid(other.TypeGuid),
      Data(other.Data),
      Payload(other.Payload ? new ScriptParamValuePayload(*other.Payload) : nullptr) {}

ScriptParamValue::ScriptParamValue(ScriptParamValue &&other) noexcept
    : Kind(other.Kind),
      Type(other.Type),
      TypeGuid(other.TypeGuid),
      Data(other.Data),
      Payload(other.Payload) {
    other.Kind = ScriptParamValueKind::Empty;
    other.Type = -1;
    other.TypeGuid = CKGUID();
    other.Data = ScriptParamValueData();
    other.Payload = nullptr;
}

ScriptParamValue::~ScriptParamValue() {
    Reset();
}

ScriptParamValue &ScriptParamValue::operator=(const ScriptParamValue &other) {
    if (this != &other) {
        ScriptParamValue copy(other);
        *this = std::move(copy);
    }
    return *this;
}

ScriptParamValue &ScriptParamValue::operator=(ScriptParamValue &&other) noexcept {
    if (this != &other) {
        Reset();
        Kind = other.Kind;
        Type = other.Type;
        TypeGuid = other.TypeGuid;
        Data = other.Data;
        Payload = other.Payload;
        other.Kind = ScriptParamValueKind::Empty;
        other.Type = -1;
        other.TypeGuid = CKGUID();
        other.Data = ScriptParamValueData();
        other.Payload = nullptr;
    }
    return *this;
}

void ScriptParamValue::Reset() {
    delete Payload;
    Payload = nullptr;
    Kind = ScriptParamValueKind::Empty;
    Type = -1;
    TypeGuid = CKGUID();
    Data = ScriptParamValueData();
}

ScriptParamValuePayload &ScriptParamValue::EnsurePayload() {
    if (!Payload) {
        Payload = new ScriptParamValuePayload();
    }
    return *Payload;
}

const std::string &ScriptParamValue::Text() const {
    static const std::string empty;
    return Payload ? Payload->Text : empty;
}

std::string &ScriptParamValue::MutableText() {
    return EnsurePayload().Text;
}

const std::vector<CK_ID> &ScriptParamValue::ObjectIds() const {
    static const std::vector<CK_ID> empty;
    return Payload ? Payload->ObjectIds : empty;
}

std::vector<CK_ID> &ScriptParamValue::MutableObjectIds() {
    return EnsurePayload().ObjectIds;
}

const std::vector<char> &ScriptParamValue::RawBytes() const {
    static const std::vector<char> empty;
    return Payload ? Payload->Raw : empty;
}

std::vector<char> &ScriptParamValue::MutableRawBytes() {
    return EnsurePayload().Raw;
}

const std::vector<ScriptParamStructMemberValue> &ScriptParamValue::StructMembers() const {
    static const std::vector<ScriptParamStructMemberValue> empty;
    return Payload ? Payload->StructMembers : empty;
}

std::vector<ScriptParamStructMemberValue> &ScriptParamValue::MutableStructMembers() {
    return EnsurePayload().StructMembers;
}

namespace ScriptParamCodecInternal {

std::string TrimString(const std::string &value) {
    return ScriptParameterText::Trim(value);
}

std::string ToLower(std::string value) {
    return ScriptParameterText::ToLower(value);
}

std::string StripQuotes(const std::string &value) {
    return ScriptParameterText::StripQuotes(value);
}

bool ParseBoolText(const std::string &value, bool fallback = false) {
    const std::string text = ToLower(StripQuotes(value));
    if (text == "true" || text == "yes" || text == "on" || text == "1") {
        return true;
    }
    if (text == "false" || text == "no" || text == "off" || text == "0") {
        return false;
    }
    return fallback;
}

bool ParseIntegerText(const std::string &value, int &out) {
    return ScriptParameterText::ParseInteger(value, out);
}

bool ParseGuidToken(const std::string &token, CKDWORD &value) {
    const std::string text = TrimString(token);
    if (text.empty()) {
        return false;
    }

    bool hasHexLetter = false;
    for (char c : text) {
        if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            hasHexLetter = true;
            break;
        }
    }

    char *end = nullptr;
    const unsigned long parsed = std::strtoul(text.c_str(), &end, hasHexLetter ? 16 : 0);
    if (!end || *end != '\0') {
        return false;
    }

    value = static_cast<CKDWORD>(parsed);
    return true;
}

bool ParseHexColorText(const std::string &value, VxColor &out) {
    std::string text = StripQuotes(value);
    if (text.empty()) {
        return false;
    }
    if (text.front() == '#') {
        text = text.substr(1);
    } else if (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0) {
        text = text.substr(2);
    } else {
        return false;
    }

    if (text.size() != 6 && text.size() != 8) {
        return false;
    }

    char *end = nullptr;
    const unsigned long parsed = std::strtoul(text.c_str(), &end, 16);
    if (!end || *end != '\0') {
        return false;
    }

    const unsigned int r = text.size() == 8 ? static_cast<unsigned int>((parsed >> 24) & 0xff) : static_cast<unsigned int>((parsed >> 16) & 0xff);
    const unsigned int g = text.size() == 8 ? static_cast<unsigned int>((parsed >> 16) & 0xff) : static_cast<unsigned int>((parsed >> 8) & 0xff);
    const unsigned int b = text.size() == 8 ? static_cast<unsigned int>((parsed >> 8) & 0xff) : static_cast<unsigned int>(parsed & 0xff);
    const unsigned int a = text.size() == 8 ? static_cast<unsigned int>(parsed & 0xff) : 255u;
    out = VxColor(static_cast<float>(r) / 255.0f,
                  static_cast<float>(g) / 255.0f,
                  static_cast<float>(b) / 255.0f,
                  static_cast<float>(a) / 255.0f);
    return true;
}

struct TypeAlias {
    const char *Name;
    ScriptParamValueKind Kind;
};

const TypeAlias kTypeAliases[] = {
    {"int", ScriptParamValueKind::Int},
    {"integer", ScriptParamValueKind::Int},
    {"uint", ScriptParamValueKind::Int},
    {"ck_id", ScriptParamValueKind::Int},
    {"float", ScriptParamValueKind::Float},
    {"number", ScriptParamValueKind::Float},
    {"angle", ScriptParamValueKind::Float},
    {"percentage", ScriptParamValueKind::Float},
    {"time", ScriptParamValueKind::Float},
    {"bool", ScriptParamValueKind::Bool},
    {"boolean", ScriptParamValueKind::Bool},
    {"string", ScriptParamValueKind::String},
    {"text", ScriptParamValueKind::Text},
    {"guid", ScriptParamValueKind::Guid},
    {"ckguid", ScriptParamValueKind::Guid},
    {"vector", ScriptParamValueKind::Vector},
    {"vxvector", ScriptParamValueKind::Vector},
    {"vector2", ScriptParamValueKind::Vector2},
    {"2dvector", ScriptParamValueKind::Vector2},
    {"vx2dvector", ScriptParamValueKind::Vector2},
    {"color", ScriptParamValueKind::Color},
    {"vxcolor", ScriptParamValueKind::Color},
    {"quat", ScriptParamValueKind::Quaternion},
    {"quaternion", ScriptParamValueKind::Quaternion},
    {"vxquaternion", ScriptParamValueKind::Quaternion},
    {"matrix", ScriptParamValueKind::Matrix},
    {"vxmatrix", ScriptParamValueKind::Matrix},
    {"objectarray", ScriptParamValueKind::ObjectArray},
    {"object array", ScriptParamValueKind::ObjectArray},
    {"xobjectarray", ScriptParamValueKind::ObjectArray},
    {"raw", ScriptParamValueKind::Raw},
};

CKParameterManager *ParameterManagerFromContext(CKContext *context) {
    return context ? context->GetParameterManager() : nullptr;
}

CKParameterManager *ParameterManagerFromParameter(CKParameter *param) {
    CKContext *context = param ? param->GetCKContext() : nullptr;
    return ParameterManagerFromContext(context);
}

bool GuidIsValid(CKGUID guid) {
    return guid != CKGUID();
}

bool IsRegisteredGuid(CKParameterManager *pm, CKGUID guid) {
    return pm && GuidIsValid(guid) && pm->GetParameterTypeDescription(guid) != nullptr;
}

bool IsTypeCompatible(CKContext *context, CKGUID a, CKGUID b) {
    if (a == b) {
        return true;
    }
    if (ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context)) {
        return registry->IsTypeCompatible(a, b);
    }
    CKParameterManager *pm = ParameterManagerFromContext(context);
    if (!pm || !GuidIsValid(a) || !GuidIsValid(b)) {
        return false;
    }
    return pm->IsTypeCompatible(a, b) != FALSE || pm->IsTypeCompatible(b, a) != FALSE;
}

bool IsTypeCompatible(CKParameter *param, CKGUID guid) {
    return param && IsTypeCompatible(param->GetCKContext(), param->GetGUID(), guid);
}

bool IsObjectCompatible(CKContext *context, CK_CLASSID expected, CK_ID objectId) {
    if (objectId == 0 || expected == 0) {
        return true;
    }
    CKObject *object = context ? CKGetObject(context, objectId) : nullptr;
    return object && CKIsChildClassOf(object->GetClassID(), expected);
}

template <typename T>
bool ReadFixedValue(CKParameter *param, T &out) {
    if (!param || param->GetDataSize() != static_cast<int>(sizeof(T))) {
        return false;
    }
    void *data = param->GetReadDataPtr(TRUE);
    if (!data) {
        return false;
    }
    std::memcpy(&out, data, sizeof(T));
    return true;
}

template <typename T>
CKERROR WriteFixedValue(CKParameter *param, const T &value) {
    return param ? param->SetValue(&value, sizeof(T)) : CKERR_INVALIDPARAMETER;
}

bool CanRawAccess(const ScriptParamTypeTraits &traits) {
    return traits.Has(ScriptParamTypeCaps::Valid) &&
           traits.Has(ScriptParamTypeCaps::FixedSize) &&
           !traits.Has(ScriptParamTypeCaps::VariableSize) &&
           !traits.Has(ScriptParamTypeCaps::HasLifecycle) &&
           !traits.Has(ScriptParamTypeCaps::ObjectLike) &&
           !traits.Has(ScriptParamTypeCaps::CollectionLike) &&
           !traits.Has(ScriptParamTypeCaps::StructLike) &&
           traits.DefaultSize > 0;
}

std::string ParameterName(CKParameter *param) {
    return param && param->GetName() ? std::string(param->GetName()) : std::string("<unnamed>");
}

std::string ConversionError(CKParameter *target, const ScriptParamValue &value, const std::string &detail) {
    std::ostringstream out;
    out << "Cannot write parameter '" << ParameterName(target) << "'"
        << " expected " << DescribeScriptParamType(target ? target->GetCKContext() : nullptr, target ? target->GetGUID() : CKGUID())
        << ", got " << DescribeScriptParamValueKind(value);
    if (!detail.empty()) {
        out << ": " << detail;
    }
    return out.str();
}

bool WriteStringFallback(CKParameter *target, const ScriptParamValue &value, std::string &error) {
    const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(target);
    if (!traits.Has(ScriptParamTypeCaps::Stringable)) {
        error = ConversionError(target, value, "target has no SDK string conversion");
        return false;
    }
    return WriteParameterText(target, ScriptParamValueToText(value), error) == CK_OK;
}

bool ReadObjectArrayParameterValue(CKParameter *param, ScriptParamValue &value) {
    if (!param || !IsTypeCompatible(param, CKPGUID_OBJECTARRAY)) {
        return false;
    }

    XObjectArray *array = nullptr;
    if (param->GetValue(&array, TRUE) != CK_OK || !array) {
        return false;
    }

    value = ScriptParamValue();
    value.Kind = ScriptParamValueKind::ObjectArray;
    value.MutableObjectIds().reserve(array->Size());
    for (int i = 0; i < array->Size(); ++i) {
        value.MutableObjectIds().push_back((*array)[i]);
    }
    value.TypeGuid = param->GetGUID();
    value.Type = param->GetType();
    return true;
}

CKERROR WriteObjectArrayParameterValue(CKParameter *param, const ScriptParamValue &value, std::string &error) {
    if (!param || !IsTypeCompatible(param, CKPGUID_OBJECTARRAY)) {
        error = ConversionError(param, value, "target is not object-array compatible");
        return CKERR_INVALIDPARAMETERTYPE;
    }

    XObjectArray *array = nullptr;
    if (param->GetValue(&array, FALSE) != CK_OK || !array) {
        array = new XObjectArray();
        if (!array) {
            error = "Failed to allocate XObjectArray.";
            return CKERR_OUTOFMEMORY;
        }

        CKERROR err = param->SetValue(&array, 0);
        if (err != CK_OK) {
            delete array;
            error = "Failed to attach XObjectArray storage.";
            return err;
        }
    }

    array->Clear();
    for (CK_ID id : value.ObjectIds()) {
        array->PushBack(id);
    }
    return CK_OK;
}

bool ParseObjectArrayText(const std::string &text, ScriptParamValue &value) {
    std::string normalized = StripQuotes(text);
    for (char &c : normalized) {
        if (c == ';' || c == '|') {
            c = ',';
        }
    }

    value = ScriptParamValue();
    value.Kind = ScriptParamValueKind::ObjectArray;
    std::size_t offset = 0;
    while (offset <= normalized.size()) {
        const std::size_t comma = normalized.find(',', offset);
        const std::size_t end = comma == std::string::npos ? normalized.size() : comma;
        const std::string token = TrimString(normalized.substr(offset, end - offset));
        if (!token.empty()) {
            int id = 0;
            if (!ParseIntegerText(token, id)) {
                return false;
            }
            value.MutableObjectIds().push_back(static_cast<CK_ID>(id));
        }
        if (comma == std::string::npos) {
            break;
        }
        offset = comma + 1;
    }
    return true;
}

CKERROR WriteTextOrReport(CKParameter *target, const std::string &text, ScriptParamValueKind kind, std::string &error);

bool ReadStructParameterValue(CKParameter *param, ScriptParamValue &value, std::string *error) {
    ScriptParameterRegistry *registry = param ? ScriptParameterRegistry::FromContext(param->GetCKContext()) : nullptr;
    const ScriptParamTypeRecord *record = registry ? registry->GetType(param) : nullptr;
    if (!param || !record || !record->Has(ScriptParamTypeCaps::StructLike)) {
        if (error) {
            *error = "Parameter is not a CK struct parameter.";
        }
        return false;
    }

    value = MakeScriptParamStruct(record->Guid, record->Name);
    for (int i = 0; i < static_cast<int>(record->StructMembers.size()); ++i) {
        CKParameter *member = GetStructMemberParameter(param, i);
        if (!member) {
            continue;
        }
        std::string memberError;
        ScriptParamValue memberValue = ReadParameterValue(member, &memberError);
        if (memberValue.Kind == ScriptParamValueKind::Empty) {
            if (error) {
                *error = "Failed to read struct member '" + record->StructMembers[i].Name + "': " + memberError;
            }
            return false;
        }
        ScriptParamStructMemberValue item;
        item.Index = i;
        item.Value = new ScriptParamValue(memberValue);
        value.MutableStructMembers().push_back(std::move(item));
    }
    return true;
}

CKERROR WriteStructParameterValue(CKParameter *param, const ScriptParamValue &value, std::string &error) {
    ScriptParameterRegistry *registry = param ? ScriptParameterRegistry::FromContext(param->GetCKContext()) : nullptr;
    const ScriptParamTypeRecord *record = registry ? registry->GetType(param) : nullptr;
    if (!param || !record || !record->Has(ScriptParamTypeCaps::StructLike)) {
        error = ConversionError(param, value, "target is not a CK struct parameter");
        return CKERR_INVALIDPARAMETERTYPE;
    }
    if (GuidIsValid(value.TypeGuid) && !registry->IsTypeCompatible(record->Guid, value.TypeGuid)) {
        error = ConversionError(param, value, "struct type is not compatible with target parameter");
        return CKERR_INVALIDPARAMETERTYPE;
    }

    for (const ScriptParamStructMemberValue &memberValue : value.StructMembers()) {
        if (memberValue.Index < 0 || memberValue.Index >= static_cast<int>(record->StructMembers.size())) {
            error = ConversionError(param, value, "struct member index is out of range");
            return CKERR_INVALIDPARAMETER;
        }
        if (!memberValue.Value) {
            continue;
        }
        CKParameter *member = GetStructMemberParameter(param, memberValue.Index);
        if (!member) {
            error = ConversionError(param, value, "struct member parameter is not available");
            return CKERR_INVALIDPARAMETER;
        }
        CKERROR err = WriteParameterValue(member, *memberValue.Value, error);
        if (err != CK_OK) {
            error = "Failed to set struct member '" + record->StructMembers[memberValue.Index].Name + "': " + error;
            return err;
        }
    }
    return CK_OK;
}

CKERROR WriteTypedTextValue(CKParameter *target, const std::string &text, ScriptParamValueKind kind, std::string &error) {
    const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(target);
    ScriptParameterRegistry *registry = target ? ScriptParameterRegistry::FromContext(target->GetCKContext()) : nullptr;
    if (traits.Has(ScriptParamTypeCaps::EnumLike)) {
        int value = 0;
        if (!registry || !registry->ParseEnumValue(target->GetGUID(), text, value, error)) {
            ScriptParamValue source;
            source.Kind = kind;
            source.MutableText() = text;
            error = ConversionError(target, source, error);
            return CKERR_INVALIDPARAMETERTYPE;
        }
        const CKDWORD dwordValue = static_cast<CKDWORD>(value);
        return WriteFixedValue(target, dwordValue);
    }
    if (traits.Has(ScriptParamTypeCaps::FlagsLike)) {
        CKDWORD value = 0;
        if (!registry || !registry->ParseFlagsValue(target->GetGUID(), text, value, error)) {
            ScriptParamValue source;
            source.Kind = kind;
            source.MutableText() = text;
            error = ConversionError(target, source, error);
            return CKERR_INVALIDPARAMETERTYPE;
        }
        return WriteFixedValue(target, value);
    }
    return WriteTextOrReport(target, text, kind, error);
}

CKERROR WriteTextOrReport(CKParameter *target, const std::string &text, ScriptParamValueKind kind, std::string &error) {
    const CKERROR err = WriteParameterText(target, text, error);
    if (err != CK_OK && error.empty()) {
        ScriptParamValue value;
        value.Kind = kind;
        error = ConversionError(target, value, "SDK text conversion failed");
    }
    return err;
}

} // namespace ScriptParamCodecInternal

using namespace ScriptParamCodecInternal;

ScriptParamValue MakeScriptParamInt(int value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Int;
    result.Data.IntValue = value;
    return result;
}

ScriptParamValue MakeScriptParamFloat(float value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Float;
    result.Data.FloatValue = value;
    return result;
}

ScriptParamValue MakeScriptParamBool(bool value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Bool;
    result.Data.BoolValue = value;
    return result;
}

ScriptParamValue MakeScriptParamString(const std::string &value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::String;
    result.MutableText() = value;
    return result;
}

ScriptParamValue MakeScriptParamText(const std::string &text, CKGUID typeGuid, const std::string &typeName) {
    (void) typeName;
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Text;
    result.TypeGuid = typeGuid;
    result.MutableText() = text;
    return result;
}

ScriptParamValue MakeScriptParamGuid(CKGUID value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Guid;
    result.Data.GuidValue = value;
    return result;
}

ScriptParamValue MakeScriptParamVector(const VxVector &value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Vector;
    result.Data.VectorValue = value;
    return result;
}

ScriptParamValue MakeScriptParamVector2(const Vx2DVector &value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Vector2;
    result.Data.Vector2Value = value;
    return result;
}

ScriptParamValue MakeScriptParamColor(const VxColor &value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Color;
    result.Data.ColorValue = value;
    return result;
}

ScriptParamValue MakeScriptParamQuaternion(const VxQuaternion &value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Quaternion;
    result.Data.QuaternionValue = value;
    return result;
}

ScriptParamValue MakeScriptParamMatrix(const VxMatrix &value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Matrix;
    result.Data.MatrixValue = value;
    return result;
}

ScriptParamValue MakeScriptParamObject(CKObject *value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Object;
    result.Data.ObjectId = value ? value->GetID() : 0;
    return result;
}

ScriptParamValue MakeScriptParamObjectArray(const XObjectArray &value) {
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::ObjectArray;
    result.MutableObjectIds().reserve(value.Size());
    for (int i = 0; i < value.Size(); ++i) {
        result.MutableObjectIds().push_back(value[i]);
    }
    return result;
}

ScriptParamValue MakeScriptParamEnum(CKGUID typeGuid, const std::string &typeName, CKDWORD value) {
    (void) typeName;
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Enum;
    result.Data.DwordValue = value;
    result.TypeGuid = typeGuid;
    return result;
}

ScriptParamValue MakeScriptParamFlags(CKGUID typeGuid, const std::string &typeName, CKDWORD value) {
    (void) typeName;
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Flags;
    result.Data.DwordValue = value;
    result.TypeGuid = typeGuid;
    return result;
}

ScriptParamValue MakeScriptParamStruct(CKGUID typeGuid, const std::string &typeName) {
    (void) typeName;
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Struct;
    result.TypeGuid = typeGuid;
    return result;
}

ScriptParamValue MakeScriptParamRaw(CKGUID typeGuid, const std::string &typeName, const void *data, int size) {
    (void) typeName;
    ScriptParamValue result;
    result.Kind = ScriptParamValueKind::Raw;
    result.TypeGuid = typeGuid;
    if (data && size > 0) {
        const auto *bytes = static_cast<const char *>(data);
        result.MutableRawBytes().assign(bytes, bytes + size);
    }
    return result;
}

std::string ScriptParamValueKindName(ScriptParamValueKind kind) {
    switch (kind) {
        case ScriptParamValueKind::Int: return "int";
        case ScriptParamValueKind::Float: return "float";
        case ScriptParamValueKind::Bool: return "bool";
        case ScriptParamValueKind::String: return "string";
        case ScriptParamValueKind::Text: return "text";
        case ScriptParamValueKind::Guid: return "CKGUID";
        case ScriptParamValueKind::Vector: return "VxVector";
        case ScriptParamValueKind::Vector2: return "Vx2DVector";
        case ScriptParamValueKind::Color: return "VxColor";
        case ScriptParamValueKind::Quaternion: return "VxQuaternion";
        case ScriptParamValueKind::Matrix: return "VxMatrix";
        case ScriptParamValueKind::Object: return "CKObject@";
        case ScriptParamValueKind::ObjectArray: return "XObjectArray";
        case ScriptParamValueKind::Enum: return "enum";
        case ScriptParamValueKind::Flags: return "flags";
        case ScriptParamValueKind::Struct: return "struct";
        case ScriptParamValueKind::Raw: return "raw";
        default: return "empty";
    }
}

std::string ScriptGuidToString(CKGUID guid) {
    char buffer[64] = {0};
    std::snprintf(buffer, sizeof(buffer), "guid:0x%08x,0x%08x", static_cast<unsigned int>(guid.d[0]), static_cast<unsigned int>(guid.d[1]));
    return buffer;
}

bool ParseScriptGuidString(const std::string &value, CKGUID &guid) {
    std::string text = TrimString(value);
    if (text.empty()) {
        return false;
    }

    const std::string lower = ToLower(text);
    if (lower.rfind("guid:", 0) == 0) {
        text = text.substr(5);
    }

    for (char &c : text) {
        if (c == '{' || c == '}' || c == '(' || c == ')' || c == ';') {
            c = ' ';
        }
    }

    std::vector<std::string> tokens;
    std::size_t offset = 0;
    while (offset <= text.size()) {
        const std::size_t comma = text.find(',', offset);
        const std::size_t space = text.find_first_of(" \t\r\n", offset);
        std::size_t end = std::min(comma == std::string::npos ? text.size() : comma,
                                   space == std::string::npos ? text.size() : space);
        const std::string token = TrimString(text.substr(offset, end - offset));
        if (!token.empty()) {
            tokens.push_back(token);
        }
        if (end >= text.size()) {
            break;
        }
        offset = end + 1;
    }

    if (tokens.size() != 2) {
        return false;
    }

    CKDWORD a = 0;
    CKDWORD b = 0;
    if (!ParseGuidToken(tokens[0], a) || !ParseGuidToken(tokens[1], b)) {
        return false;
    }

    guid = CKGUID(a, b);
    return true;
}

bool ParseScriptFloatList(const std::string &value, std::vector<float> &out) {
    out.clear();
    std::string text = StripQuotes(value);
    for (char &c : text) {
        if (c == '(' || c == ')' || c == '[' || c == ']' || c == ';' || c == '|') {
            c = ',';
        }
    }

    std::size_t offset = 0;
    while (offset <= text.size()) {
        const std::size_t comma = text.find(',', offset);
        const std::size_t end = comma == std::string::npos ? text.size() : comma;
        const std::string token = TrimString(text.substr(offset, end - offset));
        if (!token.empty()) {
            char *parseEnd = nullptr;
            const double parsed = std::strtod(token.c_str(), &parseEnd);
            if (!parseEnd || *parseEnd != '\0' || !std::isfinite(parsed)) {
                out.clear();
                return false;
            }
            out.push_back(static_cast<float>(parsed));
        }
        if (comma == std::string::npos) {
            break;
        }
        offset = comma + 1;
    }
    return !out.empty();
}

bool ParseScriptVectorText(const std::string &value, VxVector &out) {
    std::vector<float> values;
    if (!ParseScriptFloatList(value, values) || values.size() != 3) {
        return false;
    }
    out = VxVector(values[0], values[1], values[2]);
    return true;
}

bool ParseScriptVector2Text(const std::string &value, Vx2DVector &out) {
    std::vector<float> values;
    if (!ParseScriptFloatList(value, values) || values.size() != 2) {
        return false;
    }
    out = Vx2DVector(values[0], values[1]);
    return true;
}

bool ParseScriptColorText(const std::string &value, VxColor &out) {
    if (ParseHexColorText(value, out)) {
        return true;
    }
    std::vector<float> values;
    if (!ParseScriptFloatList(value, values) || (values.size() != 3 && values.size() != 4)) {
        return false;
    }
    out = VxColor(values[0], values[1], values[2], values.size() == 4 ? values[3] : 1.0f);
    return true;
}

bool ParseScriptQuaternionText(const std::string &value, VxQuaternion &out) {
    std::vector<float> values;
    if (!ParseScriptFloatList(value, values) || values.size() != 4) {
        return false;
    }
    out = VxQuaternion(values[0], values[1], values[2], values[3]);
    return true;
}

bool ParseScriptMatrixText(const std::string &value, VxMatrix &out) {
    std::vector<float> values;
    if (!ParseScriptFloatList(value, values) || values.size() != 16) {
        return false;
    }
    std::memcpy(&out, values.data(), sizeof(float) * 16);
    return true;
}

ScriptParamValueKind ScriptParamValueKindFromTypeName(const std::string &typeName) {
    const std::string type = ToLower(StripQuotes(typeName));
    if (type.empty()) {
        return ScriptParamValueKind::Empty;
    }
    for (const TypeAlias &alias : kTypeAliases) {
        if (type == alias.Name) {
            return alias.Kind;
        }
    }
    return ScriptParamValueKind::Empty;
}

ScriptParamValueKind ScriptParamValueKindFromAngelScriptType(asIScriptEngine *engine, int typeId) {
    if (!engine) {
        return ScriptParamValueKind::Empty;
    }
    if (typeId == asTYPEID_INT32 || typeId == asTYPEID_UINT32) {
        return ScriptParamValueKind::Int;
    }
    if (typeId == asTYPEID_FLOAT) {
        return ScriptParamValueKind::Float;
    }
    if (typeId == asTYPEID_BOOL) {
        return ScriptParamValueKind::Bool;
    }
    if (typeId == engine->GetTypeIdByDecl("string")) {
        return ScriptParamValueKind::String;
    }

    const ScriptParamValueKind valueKinds[] = {
        ScriptParamValueKind::Guid,
        ScriptParamValueKind::Vector,
        ScriptParamValueKind::Vector2,
        ScriptParamValueKind::Color,
        ScriptParamValueKind::Quaternion,
        ScriptParamValueKind::Matrix,
        ScriptParamValueKind::ObjectArray,
    };
    for (ScriptParamValueKind kind : valueKinds) {
        const char *decl = ScriptAngelScriptTypeForParamValueKind(kind);
        if (decl && typeId == engine->GetTypeIdByDecl(decl)) {
            return kind;
        }
    }
    return ScriptParamValueKind::Empty;
}

bool IsScriptParamValueKindCompatibleWithAngelScriptType(asIScriptEngine *engine,
                                                         int typeId,
                                                         ScriptParamValueKind kind,
                                                         std::string &expected) {
    expected = ScriptAngelScriptTypeForParamValueKind(kind);
    if (!engine) {
        return false;
    }

    switch (kind) {
        case ScriptParamValueKind::Int:
            expected = "int or uint";
            return typeId == asTYPEID_INT32 || typeId == asTYPEID_UINT32;
        case ScriptParamValueKind::Float:
            expected = "float";
            return typeId == asTYPEID_FLOAT;
        case ScriptParamValueKind::Bool:
            expected = "bool";
            return typeId == asTYPEID_BOOL;
        case ScriptParamValueKind::String:
        case ScriptParamValueKind::Text:
            expected = "string";
            return typeId == engine->GetTypeIdByDecl("string");
        case ScriptParamValueKind::Guid:
        case ScriptParamValueKind::Vector:
        case ScriptParamValueKind::Vector2:
        case ScriptParamValueKind::Color:
        case ScriptParamValueKind::Quaternion:
        case ScriptParamValueKind::Matrix:
        case ScriptParamValueKind::ObjectArray: {
            const char *decl = ScriptAngelScriptTypeForParamValueKind(kind);
            expected = decl ? decl : "supported value type";
            return decl && typeId == engine->GetTypeIdByDecl(decl);
        }
        default:
            expected = "supported value type";
            return false;
    }
}

const char *ScriptAngelScriptTypeForParamValueKind(ScriptParamValueKind kind) {
    switch (kind) {
        case ScriptParamValueKind::Int: return "int";
        case ScriptParamValueKind::Float: return "float";
        case ScriptParamValueKind::Bool: return "bool";
        case ScriptParamValueKind::String:
        case ScriptParamValueKind::Text: return "string";
        case ScriptParamValueKind::Guid: return "CKGUID";
        case ScriptParamValueKind::Vector: return "VxVector";
        case ScriptParamValueKind::Vector2: return "Vx2DVector";
        case ScriptParamValueKind::Color: return "VxColor";
        case ScriptParamValueKind::Quaternion: return "VxQuaternion";
        case ScriptParamValueKind::Matrix: return "VxMatrix";
        case ScriptParamValueKind::ObjectArray: return "XObjectArray";
        default: return nullptr;
    }
}

CKGUID ScriptParameterGuidForValueKind(ScriptParamValueKind kind) {
    switch (kind) {
        case ScriptParamValueKind::Int: return CKPGUID_INT;
        case ScriptParamValueKind::Float: return CKPGUID_FLOAT;
        case ScriptParamValueKind::Bool: return CKPGUID_BOOL;
        case ScriptParamValueKind::String:
        case ScriptParamValueKind::Text:
        case ScriptParamValueKind::Guid: return CKPGUID_STRING;
        case ScriptParamValueKind::Vector: return CKPGUID_VECTOR;
        case ScriptParamValueKind::Vector2: return CKPGUID_2DVECTOR;
        case ScriptParamValueKind::Color: return CKPGUID_COLOR;
        case ScriptParamValueKind::Quaternion: return CKPGUID_QUATERNION;
        case ScriptParamValueKind::Matrix: return CKPGUID_MATRIX;
        case ScriptParamValueKind::Object: return CKPGUID_OBJECT;
        case ScriptParamValueKind::ObjectArray: return CKPGUID_OBJECTARRAY;
        case ScriptParamValueKind::Enum: return CKPGUID_INT;
        case ScriptParamValueKind::Flags: return CKPGUID_FLAGS;
        case ScriptParamValueKind::Struct: return CKPGUID_STRUCTS;
        default: return CKGUID();
    }
}

CKGUID ScriptParameterGuidForValue(const ScriptParamValue &value) {
    if (GuidIsValid(value.TypeGuid)) {
        return value.TypeGuid;
    }
    return ScriptParameterGuidForValueKind(value.Kind);
}

CKGUID ScriptResolveParameterGuid(CKContext *context, const std::string &typeName, CKGUID fallbackGuid) {
    CKParameterManager *pm = ParameterManagerFromContext(context);
    const std::string text = StripQuotes(typeName);
    if (pm && !text.empty()) {
        CKGUID parsed;
        if (ParseScriptGuidString(text, parsed) && IsRegisteredGuid(pm, parsed)) {
            return parsed;
        }

        const CKGUID byName = pm->ParameterNameToGuid(const_cast<CKSTRING>(text.c_str()));
        if (IsRegisteredGuid(pm, byName)) {
            return byName;
        }

        const std::string lower = ToLower(text);
        const int typeCount = pm->GetParameterTypesCount();
        for (int i = 0; i < typeCount; ++i) {
            CKParameterTypeDesc *desc = pm->GetParameterTypeDescription(i);
            if (!desc) {
                continue;
            }
            const char *name = desc->TypeName.CStr();
            if (name && ToLower(name) == lower) {
                return desc->Guid;
            }
        }
    }

    if (GuidIsValid(fallbackGuid)) {
        return fallbackGuid;
    }
    const ScriptParamValueKind fallbackKind = ScriptParamValueKindFromTypeName(text);
    return ScriptParameterGuidForValueKind(fallbackKind);
}

ScriptParamTypeTraits GetScriptParamTypeTraits(CKContext *context, CKGUID guid) {
    ScriptParamTypeTraits traits;
    if (ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context)) {
        if (const ScriptParamTypeRecord *record = registry->GetType(guid)) {
            traits.Guid = record->Guid;
            traits.Type = record->Type;
            traits.DefaultSize = record->DefaultSize;
            traits.CkFlags = record->CkFlags;
            traits.Caps = record->Caps;
            traits.ClassId = record->ClassId;
            traits.Family = record->Family;
            return traits;
        }
    }

    CKParameterManager *pm = ParameterManagerFromContext(context);
    CKParameterTypeDesc *desc = pm && GuidIsValid(guid) ? pm->GetParameterTypeDescription(guid) : nullptr;
    if (!desc) {
        return traits;
    }

    traits.Guid = desc->Guid;
    traits.Type = desc->Index;
    traits.DefaultSize = desc->DefaultSize;
    traits.CkFlags = desc->dwFlags;
    traits.ClassId = static_cast<CK_CLASSID>(desc->Cid);
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::Valid, desc->Valid != 0);
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::Stringable, desc->StringFunction != nullptr);
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::VariableSize, (desc->dwFlags & CKPARAMETERTYPE_VARIABLESIZE) != 0);
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::HasLifecycle, desc->DeleteFunction != nullptr || desc->SaveLoadFunction != nullptr);
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::FixedSize, desc->DefaultSize > 0 && !traits.Has(ScriptParamTypeCaps::VariableSize));
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::ObjectLike, desc->Cid != 0 || IsTypeCompatible(context, desc->Guid, CKPGUID_OBJECT));
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::CollectionLike, IsTypeCompatible(context, desc->Guid, CKPGUID_OBJECTARRAY));
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::StructLike, (desc->dwFlags & CKPARAMETERTYPE_STRUCT) != 0);
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::EnumLike, (desc->dwFlags & CKPARAMETERTYPE_ENUMS) != 0);
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::FlagsLike, (desc->dwFlags & CKPARAMETERTYPE_FLAGS) != 0);
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::IntLike, IsTypeCompatible(context, desc->Guid, CKPGUID_INT) ||
        IsTypeCompatible(context, desc->Guid, CKPGUID_CLASSID) ||
        IsTypeCompatible(context, desc->Guid, CKPGUID_PARAMETERTYPE));
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::FloatLike, IsTypeCompatible(context, desc->Guid, CKPGUID_FLOAT));
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::BoolLike, IsTypeCompatible(context, desc->Guid, CKPGUID_BOOL));
    SetScriptParamTypeCap(traits.Caps, ScriptParamTypeCaps::StringLike, IsTypeCompatible(context, desc->Guid, CKPGUID_STRING));
    if (traits.Has(ScriptParamTypeCaps::StructLike)) {
        traits.Family = ScriptParamTypeFamily::Struct;
    } else if (traits.Has(ScriptParamTypeCaps::FlagsLike)) {
        traits.Family = ScriptParamTypeFamily::Flags;
    } else if (traits.Has(ScriptParamTypeCaps::EnumLike)) {
        traits.Family = ScriptParamTypeFamily::Enum;
    } else if (traits.Has(ScriptParamTypeCaps::CollectionLike)) {
        traits.Family = ScriptParamTypeFamily::Collection;
    } else if (traits.Has(ScriptParamTypeCaps::ObjectLike)) {
        traits.Family = ScriptParamTypeFamily::Object;
    } else if (traits.Has(ScriptParamTypeCaps::StringLike)) {
        traits.Family = ScriptParamTypeFamily::Text;
    } else if (traits.Has(ScriptParamTypeCaps::IntLike) ||
               traits.Has(ScriptParamTypeCaps::FloatLike) ||
               traits.Has(ScriptParamTypeCaps::BoolLike)) {
        traits.Family = ScriptParamTypeFamily::Scalar;
    } else {
        traits.Family = ScriptParamTypeFamily::Custom;
    }
    return traits;
}

ScriptParamTypeTraits GetScriptParamTypeTraits(CKParameter *param) {
    return param ? GetScriptParamTypeTraits(param->GetCKContext(), param->GetGUID()) : ScriptParamTypeTraits();
}

std::string DescribeScriptParamType(CKContext *context, CKGUID guid) {
    CKParameterManager *pm = ParameterManagerFromContext(context);
    CKSTRING name = pm && GuidIsValid(guid) ? pm->ParameterGuidToName(guid) : nullptr;
    std::ostringstream out;
    if (name && name[0] != '\0') {
        out << name << " ";
    }
    out << ScriptGuidToString(guid);
    return out.str();
}

std::string DescribeScriptParamValueKind(const ScriptParamValue &value) {
    std::ostringstream out;
    out << ScriptParamValueKindName(value.Kind);
    if (GuidIsValid(value.TypeGuid)) {
        out << " " << ScriptGuidToString(value.TypeGuid);
    }
    return out.str();
}

std::string ScriptParamValueToText(const ScriptParamValue &value) {
    char buffer[1024] = {0};
    switch (value.Kind) {
        case ScriptParamValueKind::Int:
            return std::to_string(value.Data.IntValue);
        case ScriptParamValueKind::Float:
            return std::to_string(value.Data.FloatValue);
        case ScriptParamValueKind::Bool:
            return value.Data.BoolValue ? "true" : "false";
        case ScriptParamValueKind::String:
        case ScriptParamValueKind::Text:
            return value.Text();
        case ScriptParamValueKind::Guid:
            return ScriptGuidToString(value.Data.GuidValue);
        case ScriptParamValueKind::Vector:
            std::snprintf(buffer, sizeof(buffer), "%g,%g,%g", value.Data.VectorValue.x, value.Data.VectorValue.y, value.Data.VectorValue.z);
            return buffer;
        case ScriptParamValueKind::Vector2:
            std::snprintf(buffer, sizeof(buffer), "%g,%g", value.Data.Vector2Value.x, value.Data.Vector2Value.y);
            return buffer;
        case ScriptParamValueKind::Color:
            std::snprintf(buffer, sizeof(buffer), "%g,%g,%g,%g", value.Data.ColorValue.r, value.Data.ColorValue.g, value.Data.ColorValue.b, value.Data.ColorValue.a);
            return buffer;
        case ScriptParamValueKind::Quaternion:
            std::snprintf(buffer, sizeof(buffer), "%g,%g,%g,%g", value.Data.QuaternionValue.x, value.Data.QuaternionValue.y, value.Data.QuaternionValue.z, value.Data.QuaternionValue.w);
            return buffer;
        case ScriptParamValueKind::Matrix: {
            const float *m = reinterpret_cast<const float *>(&value.Data.MatrixValue);
            std::ostringstream out;
            for (int i = 0; i < 16; ++i) {
                if (i > 0) {
                    out << ",";
                }
                out << m[i];
            }
            return out.str();
        }
        case ScriptParamValueKind::Object:
            return std::to_string(value.Data.ObjectId);
        case ScriptParamValueKind::ObjectArray: {
            std::ostringstream out;
            const std::vector<CK_ID> &ids = value.ObjectIds();
            for (std::size_t i = 0; i < ids.size(); ++i) {
                if (i > 0) {
                    out << ",";
                }
                out << ids[i];
            }
            return out.str();
        }
        case ScriptParamValueKind::Enum:
        case ScriptParamValueKind::Flags:
            return std::to_string(value.Data.DwordValue);
        case ScriptParamValueKind::Struct: {
            std::ostringstream out;
            const std::vector<ScriptParamStructMemberValue> &members = value.StructMembers();
            for (std::size_t i = 0; i < members.size(); ++i) {
                if (i > 0) {
                    out << ";";
                }
                if (members[i].Value) {
                    out << ScriptParamValueToText(*members[i].Value);
                }
            }
            return out.str();
        }
        default:
            return {};
    }
}

bool ReadParameterText(CKParameter *source, std::string &value, std::string *error) {
    value.clear();
    if (!source) {
        if (error) {
            *error = "Parameter is not valid.";
        }
        return false;
    }

    const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(source);
    if (!traits.Has(ScriptParamTypeCaps::Stringable)) {
        if (error) {
            *error = "Parameter '" + ParameterName(source) + "' has no SDK string conversion.";
        }
        return false;
    }

    const int size = source->GetStringValue(nullptr, TRUE);
    if (size <= 0) {
        value.clear();
        return true;
    }

    std::vector<char> buffer(static_cast<std::size_t>(size) + 1u, '\0');
    const int written = source->GetStringValue(buffer.data(), TRUE);
    if (written < 0) {
        if (error) {
            *error = "Failed to read parameter text from '" + ParameterName(source) + "'.";
        }
        return false;
    }

    value.assign(buffer.data());
    return true;
}

CKERROR WriteParameterText(CKParameter *target, const std::string &value, std::string &error) {
    if (!target) {
        error = "Parameter is not valid.";
        return CKERR_INVALIDPARAMETER;
    }

    const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(target);
    if (!traits.Has(ScriptParamTypeCaps::Stringable)) {
        error = "Parameter '" + ParameterName(target) + "' has no SDK string conversion.";
        return CKERR_INVALIDOPERATION;
    }

    CKERROR err = target->SetStringValue(const_cast<CKSTRING>(value.c_str()));
    if (err != CK_OK) {
        std::ostringstream out;
        out << "Failed to set text for parameter '" << ParameterName(target) << "'"
            << " expected " << DescribeScriptParamType(target->GetCKContext(), target->GetGUID())
            << ", CKERROR " << err << ".";
        error = out.str();
    }
    return err;
}

CKERROR WriteParameterRaw(CKParameter *target, const void *data, int size, CKGUID sourceGuid, const std::string &sourceTypeName, std::string &error) {
    if (!target) {
        error = "Parameter is not valid.";
        return CKERR_INVALIDPARAMETER;
    }

    const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(target);
    if (!CanRawAccess(traits)) {
        error = "Raw write rejected for parameter '" + ParameterName(target) + "' expected " +
                DescribeScriptParamType(target->GetCKContext(), target->GetGUID()) +
                " because the CK type is not fixed-size POD storage.";
        return CKERR_INVALIDPARAMETERTYPE;
    }

    if (GuidIsValid(sourceGuid) && !IsTypeCompatible(target->GetCKContext(), target->GetGUID(), sourceGuid)) {
        error = "Raw write type mismatch for parameter '" + ParameterName(target) + "' expected " +
                DescribeScriptParamType(target->GetCKContext(), target->GetGUID()) +
                ", got " + (!sourceTypeName.empty() ? sourceTypeName : DescribeScriptParamType(target->GetCKContext(), sourceGuid)) + ".";
        return CKERR_INVALIDPARAMETERTYPE;
    }

    const int expectedSize = target->GetDataSize() > 0 ? target->GetDataSize() : traits.DefaultSize;
    if (size != expectedSize) {
        std::ostringstream out;
        out << "Raw size mismatch for parameter '" << ParameterName(target) << "' expected "
            << expectedSize << " bytes, got " << size << ".";
        error = out.str();
        return CKERR_INVALIDPARAMETER;
    }

    const CKERROR err = target->SetValue(data, size);
    if (err != CK_OK) {
        std::ostringstream out;
        out << "Raw write failed for parameter '" << ParameterName(target) << "' CKERROR " << err << ".";
        error = out.str();
    }
    return err;
}

CKParameter *GetStructMemberParameter(CKParameter *param, int index) {
    if (!param || index < 0) {
        return nullptr;
    }
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(param->GetCKContext());
    const ScriptParamTypeRecord *record = registry ? registry->GetType(param) : nullptr;
    if (!record || !record->Has(ScriptParamTypeCaps::StructLike) || index >= static_cast<int>(record->StructMembers.size())) {
        return nullptr;
    }

    CK_ID *memberIds = static_cast<CK_ID *>(param->GetReadDataPtr(TRUE));
    if (!memberIds) {
        return nullptr;
    }

    return CKParameter::Cast(CKGetObject(param->GetCKContext(), memberIds[index]));
}

CKERROR WriteParameterValue(CKParameter *target, const ScriptParamValue &value, std::string &error) {
    if (!target) {
        error = "Parameter is not valid.";
        return CKERR_INVALIDPARAMETER;
    }

    const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(target);
    if (!traits.Has(ScriptParamTypeCaps::Valid)) {
        error = ConversionError(target, value, "target CK parameter type is not registered");
        return CKERR_INVALIDPARAMETERTYPE;
    }

    switch (value.Kind) {
        case ScriptParamValueKind::Text:
            return WriteTypedTextValue(target, value.Text(), value.Kind, error);
        case ScriptParamValueKind::Raw:
            return WriteParameterRaw(target,
                                     value.RawBytes().empty() ? nullptr : value.RawBytes().data(),
                                     static_cast<int>(value.RawBytes().size()),
                                     value.TypeGuid,
                                     std::string(),
                                     error);
        case ScriptParamValueKind::String:
            return WriteTypedTextValue(target, value.Text(), value.Kind, error);
        case ScriptParamValueKind::Enum: {
            if (!traits.Has(ScriptParamTypeCaps::EnumLike) && !traits.Has(ScriptParamTypeCaps::IntLike)) {
                error = ConversionError(target, value, "target is not enum-compatible");
                return CKERR_INVALIDPARAMETERTYPE;
            }
            if (GuidIsValid(value.TypeGuid) && !IsTypeCompatible(target->GetCKContext(), target->GetGUID(), value.TypeGuid)) {
                error = ConversionError(target, value, "enum type is not compatible with target parameter");
                return CKERR_INVALIDPARAMETERTYPE;
            }
            const CKDWORD dwordValue = value.Data.DwordValue;
            return WriteFixedValue(target, dwordValue);
        }
        case ScriptParamValueKind::Flags: {
            if (!traits.Has(ScriptParamTypeCaps::FlagsLike) && !traits.Has(ScriptParamTypeCaps::IntLike)) {
                error = ConversionError(target, value, "target is not flags-compatible");
                return CKERR_INVALIDPARAMETERTYPE;
            }
            if (GuidIsValid(value.TypeGuid) && !IsTypeCompatible(target->GetCKContext(), target->GetGUID(), value.TypeGuid)) {
                error = ConversionError(target, value, "flags type is not compatible with target parameter");
                return CKERR_INVALIDPARAMETERTYPE;
            }
            const CKDWORD dwordValue = value.Data.DwordValue;
            return WriteFixedValue(target, dwordValue);
        }
        case ScriptParamValueKind::Struct:
            return WriteStructParameterValue(target, value, error);
        case ScriptParamValueKind::Guid: {
            if (target->GetGUID() == CKPGUID_PARAMETERTYPE) {
                CKParameterManager *pm = ParameterManagerFromParameter(target);
                const CKParameterType type = pm ? pm->ParameterGuidToType(value.Data.GuidValue) : -1;
                if (type >= 0) {
                    return WriteFixedValue(target, type);
                }
            }
            return WriteTextOrReport(target, ScriptGuidToString(value.Data.GuidValue), value.Kind, error);
        }
        case ScriptParamValueKind::Object: {
            if (!traits.Has(ScriptParamTypeCaps::ObjectLike)) {
                error = ConversionError(target, value, "target is not an object parameter");
                return CKERR_INVALIDPARAMETERTYPE;
            }
            if (!IsObjectCompatible(target->GetCKContext(), traits.ClassId, value.Data.ObjectId)) {
                error = ConversionError(target, value, "object class is not compatible with target parameter class");
                return CKERR_INVALIDPARAMETERTYPE;
            }
            const CK_ID id = value.Data.ObjectId;
            return WriteFixedValue(target, id);
        }
        case ScriptParamValueKind::ObjectArray:
            return WriteObjectArrayParameterValue(target, value, error);
        case ScriptParamValueKind::Int: {
            if (traits.Has(ScriptParamTypeCaps::BoolLike)) {
                const CKBOOL boolValue = value.Data.IntValue != 0 ? TRUE : FALSE;
                return WriteFixedValue(target, boolValue);
            }
            if ((traits.Has(ScriptParamTypeCaps::IntLike) ||
                 traits.Has(ScriptParamTypeCaps::EnumLike) ||
                 traits.Has(ScriptParamTypeCaps::FlagsLike) ||
                 traits.Has(ScriptParamTypeCaps::Stringable)) &&
                traits.Has(ScriptParamTypeCaps::FixedSize) &&
                target->GetDataSize() == static_cast<int>(sizeof(CKDWORD)) &&
                !traits.Has(ScriptParamTypeCaps::ObjectLike) &&
                !traits.Has(ScriptParamTypeCaps::CollectionLike)) {
                const CKDWORD dwordValue = static_cast<CKDWORD>(value.Data.IntValue);
                return WriteFixedValue(target, dwordValue);
            }
            if (WriteStringFallback(target, value, error)) {
                return CK_OK;
            }
            return CKERR_INVALIDPARAMETERTYPE;
        }
        case ScriptParamValueKind::Float: {
            if (traits.Has(ScriptParamTypeCaps::FloatLike) && target->GetDataSize() == static_cast<int>(sizeof(float))) {
                return WriteFixedValue(target, value.Data.FloatValue);
            }
            if (WriteStringFallback(target, value, error)) {
                return CK_OK;
            }
            return CKERR_INVALIDPARAMETERTYPE;
        }
        case ScriptParamValueKind::Bool: {
            if (traits.Has(ScriptParamTypeCaps::BoolLike) ||
                (traits.Has(ScriptParamTypeCaps::IntLike) && target->GetDataSize() == static_cast<int>(sizeof(CKBOOL)))) {
                const CKBOOL boolValue = value.Data.BoolValue ? TRUE : FALSE;
                return WriteFixedValue(target, boolValue);
            }
            if (WriteStringFallback(target, value, error)) {
                return CK_OK;
            }
            return CKERR_INVALIDPARAMETERTYPE;
        }
        case ScriptParamValueKind::Vector:
            if (IsTypeCompatible(target, CKPGUID_VECTOR) && target->GetDataSize() == static_cast<int>(sizeof(VxVector))) {
                return WriteFixedValue(target, value.Data.VectorValue);
            }
            break;
        case ScriptParamValueKind::Vector2:
            if (IsTypeCompatible(target, CKPGUID_2DVECTOR) && target->GetDataSize() == static_cast<int>(sizeof(Vx2DVector))) {
                return WriteFixedValue(target, value.Data.Vector2Value);
            }
            break;
        case ScriptParamValueKind::Color:
            if (IsTypeCompatible(target, CKPGUID_COLOR) && target->GetDataSize() == static_cast<int>(sizeof(VxColor))) {
                return WriteFixedValue(target, value.Data.ColorValue);
            }
            break;
        case ScriptParamValueKind::Quaternion:
            if (IsTypeCompatible(target, CKPGUID_QUATERNION) && target->GetDataSize() == static_cast<int>(sizeof(VxQuaternion))) {
                return WriteFixedValue(target, value.Data.QuaternionValue);
            }
            break;
        case ScriptParamValueKind::Matrix:
            if (IsTypeCompatible(target, CKPGUID_MATRIX) && target->GetDataSize() == static_cast<int>(sizeof(VxMatrix))) {
                return WriteFixedValue(target, value.Data.MatrixValue);
            }
            break;
        default:
            error = ConversionError(target, value, "value is empty");
            return CKERR_INVALIDPARAMETER;
    }

    if (WriteStringFallback(target, value, error)) {
        return CK_OK;
    }
    return CKERR_INVALIDPARAMETERTYPE;
}

CKERROR CopyParameterValue(CKParameter *target, CKParameter *source, std::string &error) {
    if (!target || !source) {
        error = "CopyParameterValue requires valid target and source parameters.";
        return CKERR_INVALIDPARAMETER;
    }

    CKERROR err = target->CopyValue(source);
    if (err != CK_OK) {
        std::ostringstream out;
        out << "Failed to copy parameter value to '" << ParameterName(target) << "' expected "
            << DescribeScriptParamType(target->GetCKContext(), target->GetGUID())
            << ", got " << DescribeScriptParamType(source->GetCKContext(), source->GetGUID())
            << ", CKERROR " << err << ".";
        error = out.str();
    }
    return err;
}

ScriptParamValue ReadParameterValue(CKParameter *param, std::string *error) {
    ScriptParamValue value;
    if (!param) {
        if (error) {
            *error = "Parameter is not valid.";
        }
        return value;
    }

    const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(param);
    value.TypeGuid = param->GetGUID();
    value.Type = param->GetType();

    if (traits.Has(ScriptParamTypeCaps::StructLike)) {
        if (ReadStructParameterValue(param, value, error)) {
            return value;
        }
        return ScriptParamValue();
    }

    if (traits.Has(ScriptParamTypeCaps::ObjectLike)) {
        value.Kind = ScriptParamValueKind::Object;
        CKObject *object = param->GetValueObject(TRUE);
        value.Data.ObjectId = object ? object->GetID() : 0;
        return value;
    }

    if (traits.Has(ScriptParamTypeCaps::CollectionLike)) {
        if (ReadObjectArrayParameterValue(param, value)) {
            return value;
        }
        if (error) {
            *error = "Failed to read object array parameter '" + ParameterName(param) + "'.";
        }
        return ScriptParamValue();
    }

    if (traits.Has(ScriptParamTypeCaps::BoolLike)) {
        CKBOOL v = FALSE;
        if (ReadFixedValue(param, v)) {
            value.Kind = ScriptParamValueKind::Bool;
            value.Data.BoolValue = v != FALSE;
            return value;
        }
    }

    if (traits.Has(ScriptParamTypeCaps::FloatLike)) {
        float v = 0.0f;
        if (ReadFixedValue(param, v)) {
            value.Kind = ScriptParamValueKind::Float;
            value.Data.FloatValue = v;
            return value;
        }
    }

    if (traits.Has(ScriptParamTypeCaps::IntLike) ||
        traits.Has(ScriptParamTypeCaps::EnumLike) ||
        traits.Has(ScriptParamTypeCaps::FlagsLike)) {
        CKDWORD v = 0;
        if (ReadFixedValue(param, v)) {
            if (traits.Has(ScriptParamTypeCaps::EnumLike)) {
                value = MakeScriptParamEnum(param->GetGUID(), DescribeScriptParamType(param->GetCKContext(), param->GetGUID()), v);
            } else if (traits.Has(ScriptParamTypeCaps::FlagsLike)) {
                value = MakeScriptParamFlags(param->GetGUID(), DescribeScriptParamType(param->GetCKContext(), param->GetGUID()), v);
            } else {
                value.Kind = ScriptParamValueKind::Int;
                value.Data.IntValue = static_cast<int>(v);
            }
            return value;
        }
    }

    if (IsTypeCompatible(param, CKPGUID_VECTOR)) {
        VxVector v;
        if (ReadFixedValue(param, v)) {
            value.Kind = ScriptParamValueKind::Vector;
            value.Data.VectorValue = v;
            return value;
        }
    }

    if (IsTypeCompatible(param, CKPGUID_2DVECTOR)) {
        Vx2DVector v;
        if (ReadFixedValue(param, v)) {
            value.Kind = ScriptParamValueKind::Vector2;
            value.Data.Vector2Value = v;
            return value;
        }
    }

    if (IsTypeCompatible(param, CKPGUID_COLOR)) {
        VxColor v;
        if (ReadFixedValue(param, v)) {
            value.Kind = ScriptParamValueKind::Color;
            value.Data.ColorValue = v;
            return value;
        }
    }

    if (IsTypeCompatible(param, CKPGUID_QUATERNION)) {
        VxQuaternion v;
        if (ReadFixedValue(param, v)) {
            value.Kind = ScriptParamValueKind::Quaternion;
            value.Data.QuaternionValue = v;
            return value;
        }
    }

    if (IsTypeCompatible(param, CKPGUID_MATRIX)) {
        VxMatrix v;
        if (ReadFixedValue(param, v)) {
            value.Kind = ScriptParamValueKind::Matrix;
            value.Data.MatrixValue = v;
            return value;
        }
    }

    std::string text;
    if (ReadParameterText(param, text, nullptr)) {
        value.Kind = traits.Has(ScriptParamTypeCaps::StringLike) ? ScriptParamValueKind::String : ScriptParamValueKind::Text;
        value.MutableText() = text;
        return value;
    }

    if (CanRawAccess(traits)) {
        void *data = param->GetReadDataPtr(TRUE);
        if (data && param->GetDataSize() > 0) {
            value.Kind = ScriptParamValueKind::Raw;
            const auto *bytes = static_cast<const char *>(data);
            value.MutableRawBytes().assign(bytes, bytes + param->GetDataSize());
            return value;
        }
    }

    if (error) {
        *error = "Cannot read parameter '" + ParameterName(param) + "' as a supported value, text, or fixed-size raw buffer.";
    }
    return ScriptParamValue();
}

bool ReadParameterValueAs(CKParameter *param, ScriptParamValueKind kind, ScriptParamValue &value, std::string &error) {
    value = ScriptParamValue();
    if (!param) {
        error = "Parameter is not valid.";
        return false;
    }

    value.TypeGuid = param->GetGUID();
    value.Type = param->GetType();

    switch (kind) {
        case ScriptParamValueKind::Int: {
            CKDWORD v = 0;
            if (ReadFixedValue(param, v)) {
                value.Kind = ScriptParamValueKind::Int;
                value.Data.IntValue = static_cast<int>(v);
                return true;
            }
            std::string text;
            int parsed = 0;
            if (ReadParameterText(param, text, nullptr) && ParseIntegerText(text, parsed)) {
                value = MakeScriptParamInt(parsed);
                return true;
            }
            break;
        }
        case ScriptParamValueKind::Enum: {
            const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(param);
            if (!traits.Has(ScriptParamTypeCaps::EnumLike)) {
                break;
            }
            CKDWORD v = 0;
            if (ReadFixedValue(param, v)) {
                value = MakeScriptParamEnum(param->GetGUID(), DescribeScriptParamType(param->GetCKContext(), param->GetGUID()), v);
                return true;
            }
            break;
        }
        case ScriptParamValueKind::Flags: {
            const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(param);
            if (!traits.Has(ScriptParamTypeCaps::FlagsLike)) {
                break;
            }
            CKDWORD v = 0;
            if (ReadFixedValue(param, v)) {
                value = MakeScriptParamFlags(param->GetGUID(), DescribeScriptParamType(param->GetCKContext(), param->GetGUID()), v);
                return true;
            }
            break;
        }
        case ScriptParamValueKind::Float: {
            float v = 0.0f;
            if (ReadFixedValue(param, v)) {
                value.Kind = ScriptParamValueKind::Float;
                value.Data.FloatValue = v;
                return true;
            }
            std::string text;
            if (ReadParameterText(param, text, nullptr)) {
                char *end = nullptr;
                const double parsed = std::strtod(StripQuotes(text).c_str(), &end);
                if (end && *end == '\0') {
                    value = MakeScriptParamFloat(static_cast<float>(parsed));
                    return true;
                }
            }
            break;
        }
        case ScriptParamValueKind::Bool: {
            CKBOOL v = FALSE;
            if (ReadFixedValue(param, v)) {
                value.Kind = ScriptParamValueKind::Bool;
                value.Data.BoolValue = v != FALSE;
                return true;
            }
            std::string text;
            if (ReadParameterText(param, text, nullptr)) {
                value = MakeScriptParamBool(ParseBoolText(text));
                return true;
            }
            break;
        }
        case ScriptParamValueKind::String:
        case ScriptParamValueKind::Text: {
            std::string text;
            if (ReadParameterText(param, text, &error)) {
                value = kind == ScriptParamValueKind::String
                    ? MakeScriptParamString(text)
                    : MakeScriptParamText(text, param->GetGUID(), DescribeScriptParamType(param->GetCKContext(), param->GetGUID()));
                return true;
            }
            return false;
        }
        case ScriptParamValueKind::Guid: {
            std::string text;
            CKGUID guid;
            if (ReadParameterText(param, text, nullptr) && ParseScriptGuidString(text, guid)) {
                value = MakeScriptParamGuid(guid);
                return true;
            }
            if (param->GetGUID() == CKPGUID_PARAMETERTYPE) {
                CKParameterType type = -1;
                if (ReadFixedValue(param, type)) {
                    CKParameterManager *pm = ParameterManagerFromParameter(param);
                    value = MakeScriptParamGuid(pm ? pm->ParameterTypeToGuid(type) : CKGUID());
                    return true;
                }
            }
            break;
        }
        case ScriptParamValueKind::Vector: {
            VxVector v;
            if (ReadFixedValue(param, v)) {
                value = MakeScriptParamVector(v);
                return true;
            }
            std::string text;
            if (ReadParameterText(param, text, nullptr) && ParseScriptVectorText(text, v)) {
                value = MakeScriptParamVector(v);
                return true;
            }
            break;
        }
        case ScriptParamValueKind::Vector2: {
            Vx2DVector v;
            if (ReadFixedValue(param, v)) {
                value = MakeScriptParamVector2(v);
                return true;
            }
            std::string text;
            if (ReadParameterText(param, text, nullptr) && ParseScriptVector2Text(text, v)) {
                value = MakeScriptParamVector2(v);
                return true;
            }
            break;
        }
        case ScriptParamValueKind::Color: {
            VxColor v;
            if (ReadFixedValue(param, v)) {
                value = MakeScriptParamColor(v);
                return true;
            }
            std::string text;
            if (ReadParameterText(param, text, nullptr) && ParseScriptColorText(text, v)) {
                value = MakeScriptParamColor(v);
                return true;
            }
            break;
        }
        case ScriptParamValueKind::Quaternion: {
            VxQuaternion v;
            if (ReadFixedValue(param, v)) {
                value = MakeScriptParamQuaternion(v);
                return true;
            }
            std::string text;
            if (ReadParameterText(param, text, nullptr) && ParseScriptQuaternionText(text, v)) {
                value = MakeScriptParamQuaternion(v);
                return true;
            }
            break;
        }
        case ScriptParamValueKind::Matrix: {
            VxMatrix v;
            if (ReadFixedValue(param, v)) {
                value = MakeScriptParamMatrix(v);
                return true;
            }
            std::string text;
            if (ReadParameterText(param, text, nullptr) && ParseScriptMatrixText(text, v)) {
                value = MakeScriptParamMatrix(v);
                return true;
            }
            break;
        }
        case ScriptParamValueKind::Object:
            value = MakeScriptParamObject(param->GetValueObject(TRUE));
            return true;
        case ScriptParamValueKind::ObjectArray: {
            if (ReadObjectArrayParameterValue(param, value)) {
                return true;
            }
            std::string text;
            if (ReadParameterText(param, text, nullptr) && ParseObjectArrayText(text, value)) {
                return true;
            }
            break;
        }
        case ScriptParamValueKind::Struct: {
            if (ReadStructParameterValue(param, value, &error)) {
                return true;
            }
            return false;
        }
        case ScriptParamValueKind::Raw: {
            const ScriptParamTypeTraits traits = GetScriptParamTypeTraits(param);
            if (CanRawAccess(traits)) {
                void *data = param->GetReadDataPtr(TRUE);
                if (data && param->GetDataSize() > 0) {
                    value = MakeScriptParamRaw(param->GetGUID(), DescribeScriptParamType(param->GetCKContext(), param->GetGUID()), data, param->GetDataSize());
                    return true;
                }
            }
            break;
        }
        default:
            break;
    }

    error = "Failed to read parameter '" + ParameterName(param) + "' expected " +
            ScriptParamValueKindName(kind) + ", actual " +
            DescribeScriptParamType(param->GetCKContext(), param->GetGUID()) + ".";
    return false;
}

bool SetParameterDefaultText(CKParameterLocal *local, const std::string &defaultValue, std::string &error) {
    if (!local) {
        error = "Parameter is not valid.";
        return false;
    }

    ScriptParamValue objectArray;
    if (IsTypeCompatible(local, CKPGUID_OBJECTARRAY) && ParseObjectArrayText(defaultValue, objectArray)) {
        return WriteParameterValue(local, objectArray, error) == CK_OK;
    }

    return WriteParameterValue(local, MakeScriptParamString(StripQuotes(defaultValue)), error) == CK_OK;
}

bool RunScriptParameterConversionSelfTest(std::string &error) {
    if (ScriptParamValueKindFromTypeName("ckguid") != ScriptParamValueKind::Guid ||
        ScriptParamValueKindFromTypeName("vxquaternion") != ScriptParamValueKind::Quaternion ||
        ScriptParameterGuidForValueKind(ScriptParamValueKind::ObjectArray) != CKPGUID_OBJECTARRAY) {
        error = "Script parameter type alias lookup failed.";
        return false;
    }

    CKGUID guid;
    if (!ParseScriptGuidString("guid:0x12345678,0x9abcdef0", guid) ||
        guid.d[0] != 0x12345678 ||
        guid.d[1] != 0x9abcdef0) {
        error = "Script GUID parser failed.";
        return false;
    }

    VxVector vector;
    if (!ParseScriptVectorText("1, 2, 3", vector) ||
        vector.x != 1.0f ||
        vector.y != 2.0f ||
        vector.z != 3.0f) {
        error = "Script vector parser failed.";
        return false;
    }

    ScriptParamValue raw = MakeScriptParamRaw(CKPGUID_VECTOR, "Vector", &vector, sizeof(vector));
    if (raw.Kind != ScriptParamValueKind::Raw || raw.RawBytes().size() != sizeof(vector)) {
        error = "Script raw value creation failed.";
        return false;
    }

    error.clear();
    return true;
}
