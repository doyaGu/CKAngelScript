#include "ScriptParameterConversion.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "CKAll.h"
#include "XObjectArray.h"

namespace {

std::string TrimString(const std::string &value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    if (first == value.end()) {
        return {};
    }

    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    return std::string(first, last);
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string StripQuotes(const std::string &value) {
    std::string text = TrimString(value);
    if (text.size() >= 2 && ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\''))) {
        text = text.substr(1, text.size() - 2);
    }
    return text;
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
    const int base = hasHexLetter ? 16 : 0;
    const unsigned long parsed = std::strtoul(text.c_str(), &end, base);
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
    ScriptBridgeValueKind Kind;
};

const TypeAlias kTypeAliases[] = {
    {"int", ScriptBridgeValueKind::Int},
    {"integer", ScriptBridgeValueKind::Int},
    {"uint", ScriptBridgeValueKind::Int},
    {"ck_id", ScriptBridgeValueKind::Int},
    {"float", ScriptBridgeValueKind::Float},
    {"number", ScriptBridgeValueKind::Float},
    {"angle", ScriptBridgeValueKind::Float},
    {"percentage", ScriptBridgeValueKind::Float},
    {"time", ScriptBridgeValueKind::Float},
    {"bool", ScriptBridgeValueKind::Bool},
    {"boolean", ScriptBridgeValueKind::Bool},
    {"string", ScriptBridgeValueKind::String},
    {"text", ScriptBridgeValueKind::String},
    {"guid", ScriptBridgeValueKind::Guid},
    {"ckguid", ScriptBridgeValueKind::Guid},
    {"vector", ScriptBridgeValueKind::Vector},
    {"vxvector", ScriptBridgeValueKind::Vector},
    {"vector2", ScriptBridgeValueKind::Vector2},
    {"2dvector", ScriptBridgeValueKind::Vector2},
    {"vx2dvector", ScriptBridgeValueKind::Vector2},
    {"color", ScriptBridgeValueKind::Color},
    {"vxcolor", ScriptBridgeValueKind::Color},
    {"quat", ScriptBridgeValueKind::Quaternion},
    {"quaternion", ScriptBridgeValueKind::Quaternion},
    {"vxquaternion", ScriptBridgeValueKind::Quaternion},
    {"matrix", ScriptBridgeValueKind::Matrix},
    {"vxmatrix", ScriptBridgeValueKind::Matrix},
    {"objectarray", ScriptBridgeValueKind::ObjectArray},
    {"object array", ScriptBridgeValueKind::ObjectArray},
    {"xobjectarray", ScriptBridgeValueKind::ObjectArray},
};

bool TryParseIntegerText(const std::string &value, int &out) {
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

bool IsRegisteredParameterGuid(CKParameterManager *parameterManager, CKGUID guid) {
    return parameterManager && guid != CKGUID() && parameterManager->GetParameterTypeDescription(guid) != nullptr;
}

CKGUID ResolveRegisteredParameterGuid(CKContext *context, const std::string &typeName) {
    CKParameterManager *parameterManager = context ? context->GetParameterManager() : nullptr;
    if (!parameterManager) {
        return CKGUID();
    }

    const std::string text = StripQuotes(typeName);
    if (text.empty()) {
        return CKGUID();
    }

    CKGUID parsedGuid;
    if (ParseScriptGuidString(text, parsedGuid) && IsRegisteredParameterGuid(parameterManager, parsedGuid)) {
        return parsedGuid;
    }

    CKGUID exact = parameterManager->ParameterNameToGuid(const_cast<CKSTRING>(text.c_str()));
    if (IsRegisteredParameterGuid(parameterManager, exact)) {
        return exact;
    }

    const std::string lower = ToLower(text);
    const int typeCount = parameterManager->GetParameterTypesCount();
    for (int i = 0; i < typeCount; ++i) {
        CKParameterTypeDesc *desc = parameterManager->GetParameterTypeDescription(i);
        if (!desc) {
            continue;
        }

        const char *registeredName = desc->TypeName.CStr();
        if (registeredName && ToLower(registeredName) == lower) {
            return desc->Guid;
        }
    }

    return CKGUID();
}

bool IsEnumOrFlagsParameter(CKParameter *param) {
    CKParameterTypeDesc *desc = param ? param->GetParameterType() : nullptr;
    if (!desc || desc->DefaultSize != static_cast<int>(sizeof(CKDWORD))) {
        return false;
    }
    return (desc->dwFlags & (CKPARAMETERTYPE_ENUMS | CKPARAMETERTYPE_FLAGS)) != 0;
}

bool IsSdkDwordStringParameter(CKParameter *param) {
    CKParameterTypeDesc *desc = param ? param->GetParameterType() : nullptr;
    if (!desc || desc->DefaultSize != static_cast<int>(sizeof(CKDWORD))) {
        return false;
    }
    return desc->StringFunction != nullptr ||
           (desc->dwFlags & (CKPARAMETERTYPE_ENUMS | CKPARAMETERTYPE_FLAGS)) != 0;
}

CKERROR SetIntLikeParameterValue(CKParameter *param, int value) {
    if (IsSdkDwordStringParameter(param)) {
        CKDWORD v = static_cast<CKDWORD>(value);
        return param->SetValue(&v, sizeof(v));
    }

    int v = value;
    return param->SetValue(&v, sizeof(v));
}

bool ReadIntLikeParameterValue(CKParameter *param, int &out) {
    if (!param) {
        return false;
    }

    if (IsSdkDwordStringParameter(param)) {
        CKDWORD v = 0;
        if (param->GetValue(&v) != CK_OK) {
            return false;
        }
        out = static_cast<int>(v);
        return true;
    }

    int v = 0;
    if (param->GetValue(&v) != CK_OK) {
        return false;
    }
    out = v;
    return true;
}

CKERROR SetObjectArrayParameterValue(CKParameter *param, const std::vector<CK_ID> &ids) {
    if (!param || param->GetGUID() != CKPGUID_OBJECTARRAY) {
        return CKERR_INVALIDPARAMETERTYPE;
    }

    XObjectArray *array = nullptr;
    if (param->GetValue(&array, FALSE) != CK_OK || !array) {
        array = new XObjectArray();
        if (!array) {
            return CKERR_OUTOFMEMORY;
        }

        CKERROR err = param->SetValue(&array, 0);
        if (err != CK_OK) {
            delete array;
            return err;
        }
    }

    array->Clear();
    for (CK_ID id : ids) {
        array->PushBack(id);
    }
    return CK_OK;
}

bool ReadObjectArrayParameterValue(CKParameter *param, ScriptBridgeValue &value) {
    if (!param || param->GetGUID() != CKPGUID_OBJECTARRAY) {
        return false;
    }

    XObjectArray *array = nullptr;
    if (param->GetValue(&array, TRUE) != CK_OK || !array) {
        return false;
    }

    value.Kind = ScriptBridgeValueKind::ObjectArray;
    value.ObjectIds.clear();
    value.ObjectIds.reserve(array->Size());
    for (int i = 0; i < array->Size(); ++i) {
        value.ObjectIds.push_back((*array)[i]);
    }
    return true;
}

bool ParseObjectArrayDefaultText(const std::string &defaultValue, ScriptBridgeValue &value) {
    std::string text = StripQuotes(defaultValue);
    for (char &c : text) {
        if (c == ';' || c == '|') {
            c = ',';
        }
    }

    value.Kind = ScriptBridgeValueKind::ObjectArray;
    value.ObjectIds.clear();
    std::size_t offset = 0;
    while (offset <= text.size()) {
        const std::size_t comma = text.find(',', offset);
        const std::size_t end = comma == std::string::npos ? text.size() : comma;
        const std::string token = TrimString(text.substr(offset, end - offset));
        if (!token.empty()) {
            int id = 0;
            if (!TryParseIntegerText(token, id)) {
                return false;
            }
            value.ObjectIds.push_back(static_cast<CK_ID>(id));
        }
        if (comma == std::string::npos) {
            break;
        }
        offset = comma + 1;
    }
    return true;
}

bool SetIntDefaultText(CKParameterLocal *local, const std::string &defaultValue) {
    int parsed = 0;
    if (TryParseIntegerText(defaultValue, parsed)) {
        return SetIntLikeParameterValue(local, parsed) == CK_OK;
    }

    if (IsSdkDwordStringParameter(local)) {
        const std::string text = StripQuotes(defaultValue);
        return local->SetStringValue(const_cast<CKSTRING>(text.c_str())) == CK_OK;
    }

    return false;
}

} // namespace

ScriptBridgeValue MakeIntValue(int value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Int;
    result.IntValue = value;
    return result;
}

ScriptBridgeValue MakeFloatValue(float value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Float;
    result.FloatValue = value;
    return result;
}

ScriptBridgeValue MakeBoolValue(bool value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Bool;
    result.BoolValue = value;
    return result;
}

ScriptBridgeValue MakeStringValue(const std::string &value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::String;
    result.StringValue = value;
    return result;
}

ScriptBridgeValue MakeGuidValue(CKGUID value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Guid;
    result.GuidValue = value;
    result.StringValue = ScriptGuidToString(value);
    return result;
}

ScriptBridgeValue MakeVectorValue(const VxVector &value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Vector;
    result.VectorValue = value;
    return result;
}

ScriptBridgeValue MakeVector2Value(const Vx2DVector &value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Vector2;
    result.Vector2Value = value;
    return result;
}

ScriptBridgeValue MakeColorValue(const VxColor &value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Color;
    result.ColorValue = value;
    return result;
}

ScriptBridgeValue MakeQuaternionValue(const VxQuaternion &value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Quaternion;
    result.QuaternionValue = value;
    return result;
}

ScriptBridgeValue MakeMatrixValue(const VxMatrix &value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Matrix;
    result.MatrixValue = value;
    return result;
}

ScriptBridgeValue MakeObjectValue(CKObject *value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::Object;
    result.ObjectId = value ? value->GetID() : 0;
    return result;
}

ScriptBridgeValue MakeObjectArrayValue(const XObjectArray &value) {
    ScriptBridgeValue result;
    result.Kind = ScriptBridgeValueKind::ObjectArray;
    result.ObjectIds.reserve(value.Size());
    for (int i = 0; i < value.Size(); ++i) {
        result.ObjectIds.push_back(value[i]);
    }
    return result;
}

std::string ScriptBridgeValueKindName(ScriptBridgeValueKind kind) {
    switch (kind) {
        case ScriptBridgeValueKind::Int: return "int";
        case ScriptBridgeValueKind::Float: return "float";
        case ScriptBridgeValueKind::Bool: return "bool";
        case ScriptBridgeValueKind::String: return "string";
        case ScriptBridgeValueKind::Guid: return "CKGUID";
        case ScriptBridgeValueKind::Vector: return "VxVector";
        case ScriptBridgeValueKind::Vector2: return "Vx2DVector";
        case ScriptBridgeValueKind::Color: return "VxColor";
        case ScriptBridgeValueKind::Quaternion: return "VxQuaternion";
        case ScriptBridgeValueKind::Matrix: return "VxMatrix";
        case ScriptBridgeValueKind::Object: return "CKObject@";
        case ScriptBridgeValueKind::ObjectArray: return "XObjectArray";
        default: return "none";
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
        text = TrimString(text.substr(5));
    } else if (lower.rfind("ckguid", 0) == 0) {
        const std::size_t open = text.find('(');
        const std::size_t close = text.rfind(')');
        if (open == std::string::npos || close == std::string::npos || close <= open) {
            return false;
        }
        text = text.substr(open + 1, close - open - 1);
    } else if (!text.empty() && text.front() == '{' && text.back() == '}') {
        text = text.substr(1, text.size() - 2);
    } else if (text.find(',') == std::string::npos) {
        return false;
    }

    for (char &c : text) {
        if (c == ';' || c == ':' || c == '|') {
            c = ',';
        }
    }

    const std::size_t comma = text.find(',');
    if (comma == std::string::npos) {
        return false;
    }

    CKDWORD d1 = 0;
    CKDWORD d2 = 0;
    if (!ParseGuidToken(text.substr(0, comma), d1) || !ParseGuidToken(text.substr(comma + 1), d2)) {
        return false;
    }

    guid = CKGUID(d1, d2);
    return true;
}

bool ParseScriptFloatList(const std::string &value, std::vector<float> &out) {
    std::string text = StripQuotes(value);
    const std::size_t open = text.find('(');
    const std::size_t close = text.rfind(')');
    if (open != std::string::npos && close != std::string::npos && close > open) {
        text = text.substr(open + 1, close - open - 1);
    } else if (text.size() >= 2 &&
               ((text.front() == '{' && text.back() == '}') ||
                (text.front() == '[' && text.back() == ']'))) {
        text = text.substr(1, text.size() - 2);
    }

    for (char &c : text) {
        if (c == ',' || c == ';' || c == '|') {
            c = ' ';
        }
    }

    std::istringstream stream(text);
    float token = 0.0f;
    while (stream >> token) {
        out.push_back(token);
    }
    if (!stream.eof()) {
        stream.clear();
        char extra = '\0';
        if (stream >> extra) {
            return false;
        }
    }

    return !out.empty();
}

bool ParseScriptVectorText(const std::string &value, VxVector &out) {
    std::vector<float> values;
    if (!ParseScriptFloatList(value, values) || values.size() < 3) {
        return false;
    }
    out = VxVector(values[0], values[1], values[2]);
    return true;
}

bool ParseScriptVector2Text(const std::string &value, Vx2DVector &out) {
    std::vector<float> values;
    if (!ParseScriptFloatList(value, values) || values.size() < 2) {
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

    const float alpha = values.size() == 4 ? values[3] : 1.0f;
    const bool byteStyle = values[0] > 1.0f || values[1] > 1.0f || values[2] > 1.0f || alpha > 1.0f;
    if (byteStyle) {
        out = VxColor(values[0] / 255.0f, values[1] / 255.0f, values[2] / 255.0f, alpha / 255.0f);
    } else {
        out = VxColor(values[0], values[1], values[2], alpha);
    }
    out.Check();
    return true;
}

bool ParseScriptQuaternionText(const std::string &value, VxQuaternion &out) {
    std::vector<float> values;
    if (!ParseScriptFloatList(value, values) || values.size() < 4) {
        return false;
    }
    out = VxQuaternion(values[0], values[1], values[2], values[3]);
    return true;
}

bool ParseScriptMatrixText(const std::string &value, VxMatrix &out) {
    const std::string text = ToLower(StripQuotes(value));
    if (text.empty() || text == "identity") {
        out.SetIdentity();
        return true;
    }

    std::vector<float> values;
    if (!ParseScriptFloatList(value, values) || values.size() < 16) {
        return false;
    }

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[row][col] = values[static_cast<std::size_t>(row * 4 + col)];
        }
    }
    return true;
}

ScriptBridgeValueKind ScriptValueKindFromTypeName(const std::string &typeName) {
    const std::string type = ToLower(StripQuotes(typeName));
    if (type.empty() || type == "auto") {
        return ScriptBridgeValueKind::None;
    }
    for (const TypeAlias &alias : kTypeAliases) {
        if (type == alias.Name) {
            return alias.Kind;
        }
    }
    return ScriptBridgeValueKind::None;
}

ScriptBridgeValueKind ScriptValueKindFromAngelScriptType(asIScriptEngine *engine, int typeId) {
    if (!engine) {
        return ScriptBridgeValueKind::None;
    }

    if (typeId == asTYPEID_INT32 || typeId == asTYPEID_UINT32) {
        return ScriptBridgeValueKind::Int;
    }
    if (typeId == asTYPEID_FLOAT) {
        return ScriptBridgeValueKind::Float;
    }
    if (typeId == asTYPEID_BOOL) {
        return ScriptBridgeValueKind::Bool;
    }
    if (typeId == engine->GetTypeIdByDecl("string")) {
        return ScriptBridgeValueKind::String;
    }

    const ScriptBridgeValueKind valueKinds[] = {
        ScriptBridgeValueKind::Guid,
        ScriptBridgeValueKind::Vector,
        ScriptBridgeValueKind::Vector2,
        ScriptBridgeValueKind::Color,
        ScriptBridgeValueKind::Quaternion,
        ScriptBridgeValueKind::Matrix,
        ScriptBridgeValueKind::ObjectArray,
    };
    for (ScriptBridgeValueKind kind : valueKinds) {
        const char *decl = ScriptAngelScriptTypeForValueKind(kind);
        if (decl && typeId == engine->GetTypeIdByDecl(decl)) {
            return kind;
        }
    }

    return ScriptBridgeValueKind::None;
}

bool IsScriptValueKindCompatibleWithAngelScriptType(asIScriptEngine *engine,
                                                   int typeId,
                                                   ScriptBridgeValueKind kind,
                                                   std::string &expected) {
    if (!engine) {
        expected = "AngelScript engine";
        return false;
    }

    switch (kind) {
        case ScriptBridgeValueKind::Int:
            expected = "int or uint";
            return typeId == asTYPEID_INT32 || typeId == asTYPEID_UINT32;
        case ScriptBridgeValueKind::Float:
            expected = "float";
            return typeId == asTYPEID_FLOAT;
        case ScriptBridgeValueKind::Bool:
            expected = "bool";
            return typeId == asTYPEID_BOOL;
        case ScriptBridgeValueKind::String:
            expected = "string";
            return typeId == engine->GetTypeIdByDecl("string");
        case ScriptBridgeValueKind::Guid:
        case ScriptBridgeValueKind::Vector:
        case ScriptBridgeValueKind::Vector2:
        case ScriptBridgeValueKind::Color:
        case ScriptBridgeValueKind::Quaternion:
        case ScriptBridgeValueKind::Matrix:
        case ScriptBridgeValueKind::ObjectArray: {
            const char *decl = ScriptAngelScriptTypeForValueKind(kind);
            expected = decl ? decl : "registered value type";
            return decl && typeId == engine->GetTypeIdByDecl(decl);
        }
        default:
            expected = "supported value type";
            return false;
    }
}

const char *ScriptAngelScriptTypeForValueKind(ScriptBridgeValueKind kind) {
    switch (kind) {
        case ScriptBridgeValueKind::Int: return "int";
        case ScriptBridgeValueKind::Float: return "float";
        case ScriptBridgeValueKind::Bool: return "bool";
        case ScriptBridgeValueKind::String: return "string";
        case ScriptBridgeValueKind::Guid: return "CKGUID";
        case ScriptBridgeValueKind::Vector: return "VxVector";
        case ScriptBridgeValueKind::Vector2: return "Vx2DVector";
        case ScriptBridgeValueKind::Color: return "VxColor";
        case ScriptBridgeValueKind::Quaternion: return "VxQuaternion";
        case ScriptBridgeValueKind::Matrix: return "VxMatrix";
        case ScriptBridgeValueKind::ObjectArray: return "XObjectArray";
        default: return nullptr;
    }
}

CKGUID ScriptParameterGuidForValueKind(ScriptBridgeValueKind kind) {
    switch (kind) {
        case ScriptBridgeValueKind::Int: return CKPGUID_INT;
        case ScriptBridgeValueKind::Float: return CKPGUID_FLOAT;
        case ScriptBridgeValueKind::Bool: return CKPGUID_BOOL;
        case ScriptBridgeValueKind::String: return CKPGUID_STRING;
        case ScriptBridgeValueKind::Guid: return CKPGUID_STRING;
        case ScriptBridgeValueKind::Vector: return CKPGUID_VECTOR;
        case ScriptBridgeValueKind::Vector2: return CKPGUID_2DVECTOR;
        case ScriptBridgeValueKind::Color: return CKPGUID_COLOR;
        case ScriptBridgeValueKind::Quaternion: return CKPGUID_QUATERNION;
        case ScriptBridgeValueKind::Matrix: return CKPGUID_MATRIX;
        case ScriptBridgeValueKind::Object: return CKPGUID_OBJECT;
        case ScriptBridgeValueKind::ObjectArray: return CKPGUID_OBJECTARRAY;
        default: return CKGUID();
    }
}

CKGUID ScriptResolveParameterGuid(CKContext *context, const std::string &typeName, ScriptBridgeValueKind fallbackKind) {
    CKGUID registered = ResolveRegisteredParameterGuid(context, typeName);
    if (registered != CKGUID()) {
        return registered;
    }
    return ScriptParameterGuidForValueKind(fallbackKind);
}

bool ReadParameterString(CKParameter *source, std::string &value) {
    if (!source) {
        value.clear();
        return false;
    }

    char buffer[4096] = {0};
    const int result = source->GetStringValue(buffer, TRUE);
    if (result >= 0) {
        value = buffer;
        return true;
    }

    CKSTRING raw = static_cast<CKSTRING>(source->GetReadDataPtr(TRUE));
    value = raw ? raw : "";
    return raw != nullptr;
}

CKERROR SetParameterValue(CKParameter *param, const ScriptBridgeValue &value) {
    if (!param) {
        return CKERR_INVALIDPARAMETER;
    }

    switch (value.Kind) {
        case ScriptBridgeValueKind::Int: {
            return SetIntLikeParameterValue(param, value.IntValue);
        }
        case ScriptBridgeValueKind::Float: {
            float v = value.FloatValue;
            return param->SetValue(&v, sizeof(v));
        }
        case ScriptBridgeValueKind::Bool: {
            CKBOOL v = value.BoolValue ? TRUE : FALSE;
            return param->SetValue(&v, sizeof(v));
        }
        case ScriptBridgeValueKind::String:
            return param->SetStringValue(const_cast<CKSTRING>(value.StringValue.c_str()));
        case ScriptBridgeValueKind::Guid: {
            const std::string text = value.StringValue.empty() ? ScriptGuidToString(value.GuidValue) : value.StringValue;
            return param->SetStringValue(const_cast<CKSTRING>(text.c_str()));
        }
        case ScriptBridgeValueKind::Vector: {
            VxVector v = value.VectorValue;
            return param->SetValue(&v, sizeof(v));
        }
        case ScriptBridgeValueKind::Vector2: {
            Vx2DVector v = value.Vector2Value;
            return param->SetValue(&v, sizeof(v));
        }
        case ScriptBridgeValueKind::Color: {
            VxColor v = value.ColorValue;
            return param->SetValue(&v, sizeof(v));
        }
        case ScriptBridgeValueKind::Quaternion: {
            VxQuaternion v = value.QuaternionValue;
            return param->SetValue(&v, sizeof(v));
        }
        case ScriptBridgeValueKind::Matrix: {
            VxMatrix v = value.MatrixValue;
            return param->SetValue(&v, sizeof(v));
        }
        case ScriptBridgeValueKind::Object: {
            CK_ID id = value.ObjectId;
            return param->SetValue(&id, sizeof(id));
        }
        case ScriptBridgeValueKind::ObjectArray:
            return SetObjectArrayParameterValue(param, value.ObjectIds);
        default:
            return CKERR_INVALIDPARAMETER;
    }
}

bool SetParameterDefaultText(CKParameterLocal *local, ScriptBridgeValueKind kind, const std::string &defaultValue) {
    if (!local) {
        return false;
    }

    switch (kind) {
        case ScriptBridgeValueKind::Int:
            return SetIntDefaultText(local, defaultValue);
        case ScriptBridgeValueKind::Float:
            return SetParameterValue(local, MakeFloatValue(static_cast<float>(std::atof(defaultValue.c_str())))) == CK_OK;
        case ScriptBridgeValueKind::Bool:
            return SetParameterValue(local, MakeBoolValue(ParseBoolText(defaultValue))) == CK_OK;
        case ScriptBridgeValueKind::String:
            return SetParameterValue(local, MakeStringValue(defaultValue)) == CK_OK;
        case ScriptBridgeValueKind::Guid: {
            CKGUID value;
            if (!ParseScriptGuidString(defaultValue, value)) {
                return false;
            }
            ScriptBridgeValue bridgeValue = MakeGuidValue(value);
            bridgeValue.StringValue = defaultValue;
            return SetParameterValue(local, bridgeValue) == CK_OK;
        }
        case ScriptBridgeValueKind::Vector: {
            VxVector value;
            return ParseScriptVectorText(defaultValue, value) && SetParameterValue(local, MakeVectorValue(value)) == CK_OK;
        }
        case ScriptBridgeValueKind::Vector2: {
            Vx2DVector value;
            return ParseScriptVector2Text(defaultValue, value) && SetParameterValue(local, MakeVector2Value(value)) == CK_OK;
        }
        case ScriptBridgeValueKind::Color: {
            VxColor value;
            return ParseScriptColorText(defaultValue, value) && SetParameterValue(local, MakeColorValue(value)) == CK_OK;
        }
        case ScriptBridgeValueKind::Quaternion: {
            VxQuaternion value;
            return ParseScriptQuaternionText(defaultValue, value) && SetParameterValue(local, MakeQuaternionValue(value)) == CK_OK;
        }
        case ScriptBridgeValueKind::Matrix: {
            VxMatrix value;
            return ParseScriptMatrixText(defaultValue, value) && SetParameterValue(local, MakeMatrixValue(value)) == CK_OK;
        }
        case ScriptBridgeValueKind::ObjectArray: {
            ScriptBridgeValue value;
            return ParseObjectArrayDefaultText(defaultValue, value) && SetParameterValue(local, value) == CK_OK;
        }
        default:
            return false;
    }
}

ScriptBridgeValue ReadParameterValue(CKParameter *param) {
    ScriptBridgeValue value;
    if (!param) {
        return value;
    }

    const CKGUID guid = param->GetGUID();
    if (IsSdkDwordStringParameter(param)) {
        int v = 0;
        if (ReadIntLikeParameterValue(param, v)) {
            ScriptBridgeValue result = MakeIntValue(v);
            ReadParameterString(param, result.StringValue);
            return result;
        }
    }

    if (guid == CKPGUID_INT) {
        int v = 0;
        if (param->GetValue(&v) == CK_OK) {
            return MakeIntValue(v);
        }
    }

    if (guid == CKPGUID_FLOAT || guid == CKPGUID_ANGLE || guid == CKPGUID_PERCENTAGE || guid == CKPGUID_TIME) {
        float v = 0.0f;
        if (param->GetValue(&v) == CK_OK) {
            return MakeFloatValue(v);
        }
    }

    if (guid == CKPGUID_BOOL) {
        CKBOOL v = FALSE;
        if (param->GetValue(&v) == CK_OK) {
            return MakeBoolValue(v != FALSE);
        }
    }

    if (guid == CKPGUID_STRING) {
        std::string text;
        if (ReadParameterString(param, text)) {
            return MakeStringValue(text);
        }
    }

    if (guid == CKPGUID_VECTOR) {
        VxVector v;
        if (param->GetValue(&v) == CK_OK) {
            return MakeVectorValue(v);
        }
    }

    if (guid == CKPGUID_2DVECTOR) {
        Vx2DVector v;
        if (param->GetValue(&v) == CK_OK) {
            return MakeVector2Value(v);
        }
    }

    if (guid == CKPGUID_COLOR) {
        VxColor v;
        if (param->GetValue(&v) == CK_OK) {
            return MakeColorValue(v);
        }
    }

    if (guid == CKPGUID_QUATERNION) {
        VxQuaternion v;
        if (param->GetValue(&v) == CK_OK) {
            return MakeQuaternionValue(v);
        }
    }

    if (guid == CKPGUID_MATRIX) {
        VxMatrix v;
        if (param->GetValue(&v) == CK_OK) {
            return MakeMatrixValue(v);
        }
    }

    if (guid == CKPGUID_OBJECTARRAY) {
        ScriptBridgeValue result;
        if (ReadObjectArrayParameterValue(param, result)) {
            return result;
        }
    }

    if (CKObject *obj = param->GetValueObject(TRUE)) {
        return MakeObjectValue(obj);
    }

    return value;
}

bool ReadParameterValueAs(CKParameter *param, ScriptBridgeValueKind kind, ScriptBridgeValue &value, std::string &error) {
    if (!param) {
        error = "Parameter is null.";
        return false;
    }

    switch (kind) {
        case ScriptBridgeValueKind::Int: {
            int v = 0;
            if (!ReadIntLikeParameterValue(param, v)) {
                error = "Failed to read int parameter.";
                return false;
            }
            value = MakeIntValue(v);
            return true;
        }
        case ScriptBridgeValueKind::Float: {
            float v = 0.0f;
            if (param->GetValue(&v) != CK_OK) {
                error = "Failed to read float parameter.";
                return false;
            }
            value = MakeFloatValue(v);
            return true;
        }
        case ScriptBridgeValueKind::Bool: {
            CKBOOL v = FALSE;
            if (param->GetValue(&v) != CK_OK) {
                error = "Failed to read bool parameter.";
                return false;
            }
            value = MakeBoolValue(v != FALSE);
            return true;
        }
        case ScriptBridgeValueKind::String: {
            std::string text;
            if (!ReadParameterString(param, text)) {
                error = "Failed to read string parameter.";
                return false;
            }
            value = MakeStringValue(text);
            return true;
        }
        case ScriptBridgeValueKind::Guid: {
            std::string text;
            if (!ReadParameterString(param, text)) {
                error = "Failed to read CKGUID parameter.";
                return false;
            }
            CKGUID guid;
            if (!TrimString(text).empty() && !ParseScriptGuidString(text, guid)) {
                error = "Failed to parse CKGUID parameter.";
                return false;
            }
            value = MakeGuidValue(guid);
            value.StringValue = text;
            return true;
        }
        case ScriptBridgeValueKind::Vector: {
            VxVector v;
            if (param->GetValue(&v) != CK_OK) {
                error = "Failed to read VxVector parameter.";
                return false;
            }
            value = MakeVectorValue(v);
            return true;
        }
        case ScriptBridgeValueKind::Vector2: {
            Vx2DVector v;
            if (param->GetValue(&v) != CK_OK) {
                error = "Failed to read Vx2DVector parameter.";
                return false;
            }
            value = MakeVector2Value(v);
            return true;
        }
        case ScriptBridgeValueKind::Color: {
            VxColor v;
            if (param->GetValue(&v) != CK_OK) {
                error = "Failed to read VxColor parameter.";
                return false;
            }
            value = MakeColorValue(v);
            return true;
        }
        case ScriptBridgeValueKind::Quaternion: {
            VxQuaternion v;
            if (param->GetValue(&v) != CK_OK) {
                error = "Failed to read VxQuaternion parameter.";
                return false;
            }
            value = MakeQuaternionValue(v);
            return true;
        }
        case ScriptBridgeValueKind::Matrix: {
            VxMatrix v;
            if (param->GetValue(&v) != CK_OK) {
                error = "Failed to read VxMatrix parameter.";
                return false;
            }
            value = MakeMatrixValue(v);
            return true;
        }
        case ScriptBridgeValueKind::Object: {
            value = MakeObjectValue(param->GetValueObject(TRUE));
            return true;
        }
        case ScriptBridgeValueKind::ObjectArray: {
            if (!ReadObjectArrayParameterValue(param, value)) {
                error = "Failed to read XObjectArray parameter.";
                return false;
            }
            return true;
        }
        default:
            error = "Unsupported parameter value kind.";
            return false;
    }
}

bool RunScriptParameterConversionSelfTest(std::string &error) {
    CKGUID guid;
    if (!ParseScriptGuidString("guid:0x12345678,0x90abcdef", guid) ||
        guid != CKGUID(0x12345678, 0x90abcdef)) {
        error = "CKGUID text parsing failed.";
        return false;
    }

    std::vector<float> values;
    if (!ParseScriptFloatList("(1, 2; 3|4)", values) || values.size() != 4 ||
        values[0] != 1.0f || values[1] != 2.0f || values[2] != 3.0f || values[3] != 4.0f) {
        error = "Float list parsing failed.";
        return false;
    }

    VxVector vector;
    if (!ParseScriptVectorText("1,2,3", vector) || vector.x != 1.0f || vector.y != 2.0f || vector.z != 3.0f) {
        error = "VxVector text parsing failed.";
        return false;
    }

    Vx2DVector vector2;
    if (!ParseScriptVector2Text("4,5", vector2) || vector2.x != 4.0f || vector2.y != 5.0f) {
        error = "Vx2DVector text parsing failed.";
        return false;
    }

    VxColor color;
    if (!ParseScriptColorText("#ff8040cc", color) ||
        std::fabs(color.r - 1.0f) > 0.001f ||
        std::fabs(color.g - (128.0f / 255.0f)) > 0.001f ||
        std::fabs(color.b - (64.0f / 255.0f)) > 0.001f ||
        std::fabs(color.a - (204.0f / 255.0f)) > 0.001f) {
        error = "VxColor text parsing failed.";
        return false;
    }

    VxQuaternion quaternion;
    if (!ParseScriptQuaternionText("0,0,0,1", quaternion) ||
        quaternion.x != 0.0f || quaternion.y != 0.0f || quaternion.z != 0.0f || quaternion.w != 1.0f) {
        error = "VxQuaternion text parsing failed.";
        return false;
    }

    VxMatrix matrix;
    if (!ParseScriptMatrixText("identity", matrix) ||
        matrix[0][0] != 1.0f || matrix[1][1] != 1.0f || matrix[2][2] != 1.0f || matrix[3][3] != 1.0f) {
        error = "VxMatrix text parsing failed.";
        return false;
    }

    if (ScriptValueKindFromTypeName("ckguid") != ScriptBridgeValueKind::Guid ||
        ScriptValueKindFromTypeName("vxquaternion") != ScriptBridgeValueKind::Quaternion) {
        error = "Type alias resolution failed.";
        return false;
    }

    error.clear();
    return true;
}
