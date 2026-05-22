#ifndef CK_SCRIPTPARAMETERCONVERSION_H
#define CK_SCRIPTPARAMETERCONVERSION_H

#include <cstring>
#include <string>
#include <vector>

#include <angelscript.h>

#include "CKTypes.h"
#include "ScriptParameterRegistry.h"
#include "VxMath.h"

class CKContext;
class CKObject;
class CKParameter;
class CKParameterLocal;
class XObjectArray;

enum class ScriptParamValueKind : CKBYTE {
    Empty,
    Int,
    Float,
    Bool,
    String,
    Text,
    Guid,
    Vector,
    Vector2,
    Color,
    Quaternion,
    Matrix,
    Object,
    ObjectArray,
    Enum,
    Flags,
    Struct,
    Raw
};

struct ScriptParamTypeTraits {
    CKGUID Guid;
    CKParameterType Type = -1;
    int DefaultSize = 0;
    CKDWORD CkFlags = 0;
    CKDWORD Caps = 0;
    CK_CLASSID ClassId = 0;
    ScriptParamTypeFamily Family = ScriptParamTypeFamily::Unknown;

    bool Has(ScriptParamTypeCaps cap) const { return HasScriptParamTypeCap(Caps, cap); }
};

union ScriptParamValueData {
    int IntValue;
    CKDWORD DwordValue;
    float FloatValue;
    bool BoolValue;
    CKGUID GuidValue;
    VxVector VectorValue;
    Vx2DVector Vector2Value;
    VxColor ColorValue;
    VxQuaternion QuaternionValue;
    VxMatrix MatrixValue;
    CK_ID ObjectId;

    ScriptParamValueData() : ObjectId(0) {}
    ScriptParamValueData(const ScriptParamValueData &other) { std::memcpy(this, &other, sizeof(*this)); }
    ~ScriptParamValueData() {}
    ScriptParamValueData &operator=(const ScriptParamValueData &other) {
        if (this != &other) {
            std::memcpy(this, &other, sizeof(*this));
        }
        return *this;
    }
};

struct ScriptParamValue;

struct ScriptParamStructMemberValue {
    int Index = -1;
    ScriptParamValue *Value = nullptr;

    ScriptParamStructMemberValue() = default;
    ScriptParamStructMemberValue(const ScriptParamStructMemberValue &other);
    ScriptParamStructMemberValue(ScriptParamStructMemberValue &&other) noexcept;
    ~ScriptParamStructMemberValue();
    ScriptParamStructMemberValue &operator=(const ScriptParamStructMemberValue &other);
    ScriptParamStructMemberValue &operator=(ScriptParamStructMemberValue &&other) noexcept;
};

struct ScriptParamValuePayload {
    std::string Text;
    std::string TypeName;
    std::vector<CK_ID> ObjectIds;
    std::vector<char> Raw;
    std::vector<ScriptParamStructMemberValue> StructMembers;
};

struct ScriptParamValue {
    ScriptParamValueKind Kind = ScriptParamValueKind::Empty;
    CKParameterType Type = -1;
    CKGUID TypeGuid;
    ScriptParamValueData Data;
    ScriptParamValuePayload *Payload = nullptr;

    ScriptParamValue() = default;
    ScriptParamValue(const ScriptParamValue &other);
    ScriptParamValue(ScriptParamValue &&other) noexcept;
    ~ScriptParamValue();
    ScriptParamValue &operator=(const ScriptParamValue &other);
    ScriptParamValue &operator=(ScriptParamValue &&other) noexcept;

    void Reset();
    bool HasPayload() const { return Payload != nullptr; }
    ScriptParamValuePayload &EnsurePayload();
    const std::string &Text() const;
    std::string &MutableText();
    const std::string &TypeNameText() const;
    std::string &MutableTypeNameText();
    const std::vector<CK_ID> &ObjectIds() const;
    std::vector<CK_ID> &MutableObjectIds();
    const std::vector<char> &RawBytes() const;
    std::vector<char> &MutableRawBytes();
    const std::vector<ScriptParamStructMemberValue> &StructMembers() const;
    std::vector<ScriptParamStructMemberValue> &MutableStructMembers();
};

static_assert(sizeof(ScriptParamTypeTraits) <= 64, "ScriptParamTypeTraits must stay hot-path compact.");
static_assert(sizeof(ScriptParamValue) <= 128, "ScriptParamValue must stay union-backed and payload-based.");

ScriptParamValue MakeScriptParamInt(int value);
ScriptParamValue MakeScriptParamFloat(float value);
ScriptParamValue MakeScriptParamBool(bool value);
ScriptParamValue MakeScriptParamString(const std::string &value);
ScriptParamValue MakeScriptParamText(const std::string &text, CKGUID typeGuid = CKGUID(), const std::string &typeName = std::string());
ScriptParamValue MakeScriptParamGuid(CKGUID value);
ScriptParamValue MakeScriptParamVector(const VxVector &value);
ScriptParamValue MakeScriptParamVector2(const Vx2DVector &value);
ScriptParamValue MakeScriptParamColor(const VxColor &value);
ScriptParamValue MakeScriptParamQuaternion(const VxQuaternion &value);
ScriptParamValue MakeScriptParamMatrix(const VxMatrix &value);
ScriptParamValue MakeScriptParamObject(CKObject *value);
ScriptParamValue MakeScriptParamObjectArray(const XObjectArray &value);
ScriptParamValue MakeScriptParamEnum(CKGUID typeGuid, const std::string &typeName, CKDWORD value);
ScriptParamValue MakeScriptParamFlags(CKGUID typeGuid, const std::string &typeName, CKDWORD value);
ScriptParamValue MakeScriptParamStruct(CKGUID typeGuid, const std::string &typeName);
ScriptParamValue MakeScriptParamRaw(CKGUID typeGuid, const std::string &typeName, const void *data, int size);

std::string ScriptParamValueKindName(ScriptParamValueKind kind);
std::string ScriptGuidToString(CKGUID guid);

bool ParseScriptGuidString(const std::string &value, CKGUID &guid);
bool ParseScriptFloatList(const std::string &value, std::vector<float> &out);
bool ParseScriptVectorText(const std::string &value, VxVector &out);
bool ParseScriptVector2Text(const std::string &value, Vx2DVector &out);
bool ParseScriptColorText(const std::string &value, VxColor &out);
bool ParseScriptQuaternionText(const std::string &value, VxQuaternion &out);
bool ParseScriptMatrixText(const std::string &value, VxMatrix &out);

ScriptParamValueKind ScriptParamValueKindFromTypeName(const std::string &typeName);
ScriptParamValueKind ScriptParamValueKindFromAngelScriptType(asIScriptEngine *engine, int typeId);
bool IsScriptParamValueKindCompatibleWithAngelScriptType(asIScriptEngine *engine,
                                                         int typeId,
                                                         ScriptParamValueKind kind,
                                                         std::string &expected);
const char *ScriptAngelScriptTypeForParamValueKind(ScriptParamValueKind kind);

CKGUID ScriptParameterGuidForValueKind(ScriptParamValueKind kind);
CKGUID ScriptParameterGuidForValue(const ScriptParamValue &value);
CKGUID ScriptResolveParameterGuid(CKContext *context, const std::string &typeName, CKGUID fallbackGuid = CKGUID());

ScriptParamTypeTraits GetScriptParamTypeTraits(CKContext *context, CKGUID guid);
ScriptParamTypeTraits GetScriptParamTypeTraits(CKParameter *param);
std::string DescribeScriptParamType(CKContext *context, CKGUID guid);
std::string DescribeScriptParamValueKind(const ScriptParamValue &value);
std::string ScriptParamValueToText(const ScriptParamValue &value);

bool ReadParameterText(CKParameter *source, std::string &value, std::string *error = nullptr);
CKERROR WriteParameterText(CKParameter *target, const std::string &value, std::string &error);
CKERROR WriteParameterRaw(CKParameter *target, const void *data, int size, CKGUID sourceGuid, const std::string &sourceTypeName, std::string &error);
CKParameter *GetStructMemberParameter(CKParameter *param, int index);
CKERROR WriteParameterValue(CKParameter *target, const ScriptParamValue &value, std::string &error);
CKERROR CopyParameterValue(CKParameter *target, CKParameter *source, std::string &error);
ScriptParamValue ReadParameterValue(CKParameter *param, std::string *error = nullptr);
bool ReadParameterValueAs(CKParameter *param, ScriptParamValueKind kind, ScriptParamValue &value, std::string &error);
bool SetParameterDefaultText(CKParameterLocal *local, const std::string &defaultValue, std::string &error);
#endif // CK_SCRIPTPARAMETERCONVERSION_H
