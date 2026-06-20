#include "ScriptSelfTests.h"

#include "ScriptParameterConversion.h"

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
