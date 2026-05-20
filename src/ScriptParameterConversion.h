#ifndef CK_SCRIPTPARAMETERCONVERSION_H
#define CK_SCRIPTPARAMETERCONVERSION_H

#include <string>
#include <vector>

#include <angelscript.h>

#include "CKTypes.h"
#include "VxMath.h"

class CKObject;
class CKParameter;
class CKParameterLocal;
class CKContext;
class XObjectArray;

enum class ScriptBridgeValueKind {
    None,
    Int,
    Float,
    Bool,
    String,
    Guid,
    Vector,
    Vector2,
    Color,
    Quaternion,
    Matrix,
    Object,
    ObjectArray
};

struct ScriptBridgeValue {
    ScriptBridgeValueKind Kind = ScriptBridgeValueKind::None;
    int IntValue = 0;
    float FloatValue = 0.0f;
    bool BoolValue = false;
    std::string StringValue;
    CKGUID GuidValue;
    VxVector VectorValue;
    Vx2DVector Vector2Value;
    VxColor ColorValue;
    VxQuaternion QuaternionValue;
    VxMatrix MatrixValue;
    CK_ID ObjectId = 0;
    std::vector<CK_ID> ObjectIds;
};

ScriptBridgeValue MakeIntValue(int value);
ScriptBridgeValue MakeFloatValue(float value);
ScriptBridgeValue MakeBoolValue(bool value);
ScriptBridgeValue MakeStringValue(const std::string &value);
ScriptBridgeValue MakeGuidValue(CKGUID value);
ScriptBridgeValue MakeVectorValue(const VxVector &value);
ScriptBridgeValue MakeVector2Value(const Vx2DVector &value);
ScriptBridgeValue MakeColorValue(const VxColor &value);
ScriptBridgeValue MakeQuaternionValue(const VxQuaternion &value);
ScriptBridgeValue MakeMatrixValue(const VxMatrix &value);
ScriptBridgeValue MakeObjectValue(CKObject *value);
ScriptBridgeValue MakeObjectArrayValue(const XObjectArray &value);

std::string ScriptBridgeValueKindName(ScriptBridgeValueKind kind);
std::string ScriptGuidToString(CKGUID guid);

bool ParseScriptGuidString(const std::string &value, CKGUID &guid);
bool ParseScriptFloatList(const std::string &value, std::vector<float> &out);
bool ParseScriptVectorText(const std::string &value, VxVector &out);
bool ParseScriptVector2Text(const std::string &value, Vx2DVector &out);
bool ParseScriptColorText(const std::string &value, VxColor &out);
bool ParseScriptQuaternionText(const std::string &value, VxQuaternion &out);
bool ParseScriptMatrixText(const std::string &value, VxMatrix &out);

ScriptBridgeValueKind ScriptValueKindFromTypeName(const std::string &typeName);
ScriptBridgeValueKind ScriptValueKindFromAngelScriptType(asIScriptEngine *engine, int typeId);
bool IsScriptValueKindCompatibleWithAngelScriptType(asIScriptEngine *engine,
                                                    int typeId,
                                                    ScriptBridgeValueKind kind,
                                                    std::string &expected);
const char *ScriptAngelScriptTypeForValueKind(ScriptBridgeValueKind kind);
CKGUID ScriptParameterGuidForValueKind(ScriptBridgeValueKind kind);
CKGUID ScriptResolveParameterGuid(CKContext *context, const std::string &typeName, ScriptBridgeValueKind fallbackKind);

bool ReadParameterString(CKParameter *source, std::string &value);
CKERROR SetParameterValue(CKParameter *param, const ScriptBridgeValue &value);
bool SetParameterDefaultText(CKParameterLocal *local, ScriptBridgeValueKind kind, const std::string &defaultValue);
ScriptBridgeValue ReadParameterValue(CKParameter *param);
bool ReadParameterValueAs(CKParameter *param, ScriptBridgeValueKind kind, ScriptBridgeValue &value, std::string &error);
bool RunScriptParameterConversionSelfTest(std::string &error);

#endif // CK_SCRIPTPARAMETERCONVERSION_H
