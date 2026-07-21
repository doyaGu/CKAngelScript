#include "ScriptSelfTests.h"

#include <algorithm>
#include <cstdint>

#include "CKAll.h"
#include "ScriptParameterConversion.h"

namespace {

template <std::size_t Size>
bool GuardIs(const unsigned char (&guard)[Size], unsigned char expected) {
    return std::all_of(std::begin(guard), std::end(guard), [expected](unsigned char value) {
        return value == expected;
    });
}

bool RunBehaviorParameterGenericSelfTest(CKContext *context,
                                         asIScriptEngine *engine,
                                         std::string &error) {
    if (!context || !engine) {
        error = "Behavior parameter conversion self-test requires CKContext and AngelScript engine.";
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ParameterConversionSelfTest"),
        CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        error = "Behavior parameter conversion self-test could not create its behavior.";
        return false;
    }
    auto cleanup = [&]() {
        if (!behavior->IsToBeDeleted()) {
            context->DestroyObject(behavior);
        }
    };

    CKParameterOut *boolOutput = behavior->CreateOutputParameter(
        const_cast<CKSTRING>("BoolOutput"), CKPGUID_BOOL);
    CKParameterOut *vectorOutput = behavior->CreateOutputParameter(
        const_cast<CKSTRING>("VectorOutput"), CKPGUID_VECTOR);
    CKParameterOut *stringOutput = behavior->CreateOutputParameter(
        const_cast<CKSTRING>("StringOutput"), CKPGUID_STRING);
    CKParameterIn *boolInput = behavior->CreateInputParameter(
        const_cast<CKSTRING>("BoolInput"), CKPGUID_BOOL);
    if (!boolOutput || !vectorOutput || !stringOutput || !boolInput ||
        boolInput->SetDirectSource(boolOutput) != CK_OK) {
        cleanup();
        error = "Behavior parameter conversion self-test could not create its parameters.";
        return false;
    }

    CKBOOL ckTrue = TRUE;
    VxVector vectorValue;
    vectorValue.x = 1.0f;
    vectorValue.y = 2.0f;
    vectorValue.z = 3.0f;
    if (boolOutput->SetValue(&ckTrue, sizeof(ckTrue)) != CK_OK ||
        vectorOutput->SetValue(&vectorValue, sizeof(vectorValue)) != CK_OK) {
        cleanup();
        error = "Behavior parameter conversion self-test could not initialize parameter values.";
        return false;
    }

    const int stringTypeId = engine->GetTypeIdByDecl("string");
    std::string stringWrite = "guarded string";
    std::string stringRead;
    if (stringTypeId <= 0 ||
        WriteBehaviorParameterValue(behavior,
                                    ScriptBehaviorParameterSlot::Output,
                                    2,
                                    stringTypeId,
                                    &stringWrite,
                                    engine,
                                    nullptr) != CK_OK ||
        ReadBehaviorParameterValue(behavior,
                                   ScriptBehaviorParameterSlot::Output,
                                   2,
                                   stringTypeId,
                                   &stringRead,
                                   engine,
                                   nullptr) != CK_OK ||
        stringRead != stringWrite) {
        cleanup();
        error = "Behavior parameter conversion self-test failed its safe string path.";
        return false;
    }

    struct GuardedBool {
        bool Value = false;
        unsigned char Guard[7] = {};
    } outputRead, inputRead, boolWrite;
    std::fill(std::begin(outputRead.Guard), std::end(outputRead.Guard), 0xa5u);
    std::fill(std::begin(inputRead.Guard), std::end(inputRead.Guard), 0x5au);
    std::fill(std::begin(boolWrite.Guard), std::end(boolWrite.Guard), 0xc3u);

    if (ReadBehaviorParameterValue(behavior,
                                   ScriptBehaviorParameterSlot::Output,
                                   0,
                                   asTYPEID_BOOL,
                                   &outputRead.Value,
                                   engine,
                                   nullptr) != CK_OK ||
        !outputRead.Value ||
        !GuardIs(outputRead.Guard, 0xa5u) ||
        ReadBehaviorParameterValue(behavior,
                                   ScriptBehaviorParameterSlot::Input,
                                   0,
                                   asTYPEID_BOOL,
                                   &inputRead.Value,
                                   engine,
                                   nullptr) != CK_OK ||
        !inputRead.Value ||
        !GuardIs(inputRead.Guard, 0x5au)) {
        cleanup();
        error = "Behavior parameter conversion self-test detected a CKBOOL read buffer overrun.";
        return false;
    }

    boolWrite.Value = false;
    CKBOOL storedBool = TRUE;
    if (WriteBehaviorParameterValue(behavior,
                                    ScriptBehaviorParameterSlot::Output,
                                    0,
                                    asTYPEID_BOOL,
                                    &boolWrite.Value,
                                    engine,
                                    nullptr) != CK_OK ||
        boolOutput->GetValue(&storedBool) != CK_OK ||
        storedBool != FALSE ||
        !GuardIs(boolWrite.Guard, 0xc3u)) {
        cleanup();
        error = "Behavior parameter conversion self-test did not translate AngelScript bool to CKBOOL.";
        return false;
    }

    struct GuardedInt {
        int Value = 0x13572468;
        unsigned char Guard[16] = {};
    } mismatchedRead, mismatchedWrite;
    std::fill(std::begin(mismatchedRead.Guard), std::end(mismatchedRead.Guard), 0x6du);
    std::fill(std::begin(mismatchedWrite.Guard), std::end(mismatchedWrite.Guard), 0x96u);
    if (ReadBehaviorParameterValue(behavior,
                                   ScriptBehaviorParameterSlot::Output,
                                   1,
                                   asTYPEID_INT32,
                                   &mismatchedRead.Value,
                                   engine,
                                   nullptr) != CKERR_INVALIDPARAMETERTYPE ||
        mismatchedRead.Value != 0x13572468 ||
        !GuardIs(mismatchedRead.Guard, 0x6du) ||
        WriteBehaviorParameterValue(behavior,
                                    ScriptBehaviorParameterSlot::Output,
                                    1,
                                    asTYPEID_INT32,
                                    &mismatchedWrite.Value,
                                    engine,
                                    nullptr) != CKERR_INVALIDPARAMETERTYPE ||
        !GuardIs(mismatchedWrite.Guard, 0x96u)) {
        cleanup();
        error = "Behavior parameter conversion self-test accepted a mismatched generic buffer size.";
        return false;
    }

    VxVector storedVector;
    if (vectorOutput->GetValue(&storedVector) != CK_OK ||
        storedVector.x != vectorValue.x ||
        storedVector.y != vectorValue.y ||
        storedVector.z != vectorValue.z) {
        cleanup();
        error = "Behavior parameter conversion self-test modified a parameter after rejecting its buffer size.";
        return false;
    }

    cleanup();
    return true;
}

} // namespace

bool RunScriptParameterConversionSelfTest(CKContext *context,
                                          asIScriptEngine *engine,
                                          std::string &error) {
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

    if (!RunBehaviorParameterGenericSelfTest(context, engine, error)) {
        return false;
    }

    error.clear();
    return true;
}
