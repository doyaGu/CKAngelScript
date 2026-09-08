#ifndef CK_SCRIPTBEHAVIORFRAME_H
#define CK_SCRIPTBEHAVIORFRAME_H

#include <cstdint>
#include <string>

#include "CKBehavior.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptRefCounted.h"

class asIScriptGeneric;
class BBBridge;
class BBCallBuilder;
class BBTask;
class BehaviorGraph;
class BehaviorLayout;
class BehaviorRef;
class GraphTask;
class ParamRef;
class ScriptAsyncTaskBase;
class NativeBuffer;
struct ScriptParamValue;

class BehaviorFrame final : public RefCounted {
public:
    explicit BehaviorFrame(const CKBehaviorContext &ctx);
    ~BehaviorFrame() override = default;

    void Refresh(const CKBehaviorContext &ctx);

    CKBehavior *Behavior() const;
    CKContext *Context() const;

    float DeltaTime() const;
    CKLevel *CurrentLevel() const;
    CKScene *CurrentScene() const;
    CKScene *PreviousScene() const;
    CKRenderContext *CurrentRenderContext() const;
    CKDWORD CallbackMessage() const;

    bool IsValid() const;
    CK_ID Id() const;
    std::string Name() const;
    BehaviorRef *Self() const;
    BehaviorRef *Parent() const;
    BehaviorRef *OwnerScript() const;
    BehaviorFrame *FrameFor(CKBehavior *behavior) const;
    BehaviorFrame *FrameForRef(BehaviorRef *behavior) const;
    BehaviorFrame *SubFrame(int index) const;
    BehaviorLayout *Layout() const;
    BehaviorGraph *Graph() const;
    BehaviorGraph *AsGraph() const;

    int InputCount() const;
    int OutputCount() const;
    int PinCount() const;
    int PoutCount() const;
    int LocalCount() const;
    int BehaviorLocalCount() const;
    int SettingCount() const;
    int SubBehaviorCount() const;
    std::string InputName(int index) const;
    std::string OutputName(int index) const;
    int FindInput(const std::string &name, int occurrence = 0) const;
    int FindOutput(const std::string &name, int occurrence = 0) const;
    int FindPin(const std::string &name, int occurrence = 0) const;
    int FindPout(const std::string &name, int occurrence = 0) const;
    int FindLocal(const std::string &name, int occurrence = 0) const;
    int FindBehaviorLocal(const std::string &name, int occurrence = 0) const;
    int FindSetting(const std::string &name, int occurrence = 0) const;

    bool InputActive(const std::string &name = "In", int occurrence = 0) const;
    bool InputActiveIndex(int index) const;
    bool Take(const std::string &name = "In", int occurrence = 0);
    bool TakeIndex(int index);
    void Emit(const std::string &name = "Out", int occurrence = 0);
    void EmitIndex(int index);
    bool OutputActive(const std::string &name = "Out", int occurrence = 0) const;
    bool OutputActiveIndex(int index) const;
    void ActivateInput(const std::string &name = "In", bool active = true, int occurrence = 0);
    void ActivateInputIndex(int index, bool active = true);
    void ActivateOutput(const std::string &name = "Out", bool active = true, int occurrence = 0);
    void ActivateOutputIndex(int index, bool active = true);

    CKParameterLocal *LocalParameter(int index) const;
    BehaviorRef *SubBehavior(int index) const;

    ParamRef *Pin(const std::string &name, int occurrence = 0) const;
    ParamRef *PinIndex(int index) const;
    ParamRef *Pout(const std::string &name, int occurrence = 0) const;
    ParamRef *PoutIndex(int index) const;
    ParamRef *Local(const std::string &name, int occurrence = 0) const;
    ParamRef *LocalIndex(int index) const;
    ParamRef *BehaviorLocal(const std::string &name, int occurrence = 0) const;
    ParamRef *BehaviorLocalIndex(int index) const;
    ParamRef *Setting(const std::string &name, int occurrence = 0) const;
    ParamRef *SettingIndex(int index) const;
    std::string PinString(const std::string &name, const std::string &defaultValue = "", int occurrence = 0) const;
    std::string PinStringIndex(int index, const std::string &defaultValue = "") const;
    int PinInt(const std::string &name, int defaultValue = 0, int occurrence = 0) const;
    int PinIntIndex(int index, int defaultValue = 0) const;
    bool PinBool(const std::string &name, bool defaultValue = false, int occurrence = 0) const;
    bool PinBoolIndex(int index, bool defaultValue = false) const;
    float PinFloat(const std::string &name, float defaultValue = 0.0f, int occurrence = 0) const;
    float PinFloatIndex(int index, float defaultValue = 0.0f) const;
    CKObject *PinObject(const std::string &name, int occurrence = 0) const;
    CKObject *PinObjectIndex(int index) const;
    std::string LocalString(const std::string &name, const std::string &defaultValue = "", int occurrence = 0) const;
    std::string LocalStringIndex(int index, const std::string &defaultValue = "") const;
    int LocalInt(const std::string &name, int defaultValue = 0, int occurrence = 0) const;
    int LocalIntIndex(int index, int defaultValue = 0) const;
    bool LocalBool(const std::string &name, bool defaultValue = false, int occurrence = 0) const;
    bool LocalBoolIndex(int index, bool defaultValue = false) const;
    float LocalFloat(const std::string &name, float defaultValue = 0.0f, int occurrence = 0) const;
    float LocalFloatIndex(int index, float defaultValue = 0.0f) const;
    CKObject *LocalObject(const std::string &name, int occurrence = 0) const;
    CKObject *BehaviorLocalObject(const std::string &name, int occurrence = 0) const;
    CKObject *BehaviorLocalObjectIndex(int index) const;
    bool PinEnabled(const std::string &name, int occurrence = 0) const;
    bool PinEnabledIndex(int index) const;
    void EnablePin(const std::string &name, bool enable, int occurrence = 0);
    void EnablePinIndex(int index, bool enable);
    NativeBuffer *PinReadData(int index) const;
    bool PoutEnabled(const std::string &name, int occurrence = 0) const;
    bool PoutEnabledIndex(int index) const;
    void EnablePout(const std::string &name, bool enable, int occurrence = 0);
    void EnablePoutIndex(int index, bool enable);
    NativeBuffer *PoutWriteData(int index) const;
    CKObject *PoutObject(int index) const;
    CKERROR SetOutputParameterObject(int index, CKObject *object);

    bool SetPoutInt(const std::string &name, int value, int occurrence = 0);
    bool SetPoutIntIndex(int index, int value);
    bool SetPoutFloat(const std::string &name, float value, int occurrence = 0);
    bool SetPoutFloatIndex(int index, float value);
    bool SetPoutBool(const std::string &name, bool value, int occurrence = 0);
    bool SetPoutBoolIndex(int index, bool value);
    bool SetPoutString(const std::string &name, const std::string &value, int occurrence = 0);
    bool SetPoutStringIndex(int index, const std::string &value);
    bool SetPoutObject(const std::string &name, CKObject *value, int occurrence = 0);
    bool SetPoutObjectIndex(int index, CKObject *value);

    bool SetLocalInt(const std::string &name, int value, int occurrence = 0);
    bool SetLocalIntIndex(int index, int value);
    bool SetLocalFloat(const std::string &name, float value, int occurrence = 0);
    bool SetLocalFloatIndex(int index, float value);
    bool SetLocalBool(const std::string &name, bool value, int occurrence = 0);
    bool SetLocalBoolIndex(int index, bool value);
    bool SetLocalString(const std::string &name, const std::string &value, int occurrence = 0);
    bool SetLocalStringIndex(int index, const std::string &value);
    bool SetLocalObject(const std::string &name, CKObject *value, int occurrence = 0);
    bool SetLocalObjectIndex(int index, CKObject *value);
    NativeBuffer *LocalWriteData(int index) const;
    NativeBuffer *LocalReadData(int index) const;
    CKObject *LocalObject(int index) const;
    NativeBuffer *BehaviorLocalWriteData(int index) const;
    NativeBuffer *BehaviorLocalReadData(int index) const;
    CKERROR SetLocalObject(int index, CKObject *object);

    bool SetSettingInt(const std::string &name, int value, int occurrence = 0);
    bool SetSettingIntIndex(int index, int value);
    bool SetSettingFloat(const std::string &name, float value, int occurrence = 0);
    bool SetSettingFloatIndex(int index, float value);
    bool SetSettingBool(const std::string &name, bool value, int occurrence = 0);
    bool SetSettingBoolIndex(int index, bool value);
    bool SetSettingString(const std::string &name, const std::string &value, int occurrence = 0);
    bool SetSettingStringIndex(int index, const std::string &value);
    bool SetSettingObject(const std::string &name, CKObject *value, int occurrence = 0);
    bool SetSettingObjectIndex(int index, CKObject *value);
    bool IsSetting(int localIndex) const;
    BBCallBuilder *CallName(const std::string &prototype) const;
    BBCallBuilder *CallGuid(CKGUID guid) const;
    BBBridge *BB() const;
    ScriptAsyncTaskBase *LinkDelay(int frames) const;
    ScriptAsyncTaskBase *WaitBBTask(BBTask *task, int inputIndex = -1) const;
    ScriptAsyncTaskBase *WaitGraphTask(GraphTask *task) const;
    ScriptAsyncTaskBase *WaitMessageName(const std::string &message, CKBeObject *object = nullptr) const;
    ScriptAsyncTaskBase *WaitMessageType(CKMessageType messageType, CKBeObject *object = nullptr) const;

    void Trace(const std::string &message) const;

    CKBehaviorContext MakeContext() const;

private:
    ScriptBehaviorBridge *Bridge() const;
    BehaviorRef *WrapBehavior(CKBehavior *behavior) const;
    ParamRef *WrapParameter(CKObject *parameter, ScriptBridgeSlotKind kind, int index) const;
    bool SetSettingValue(int localIndex, const ScriptParamValue &value, const char *method);
    int ResolveInput(const std::string &name, int occurrence, const char *method) const;
    int ResolveOutput(const std::string &name, int occurrence, const char *method) const;
    int ResolvePin(const std::string &name, int occurrence, const char *method) const;
    int ResolvePout(const std::string &name, int occurrence, const char *method) const;
    int ResolveLocal(const std::string &name, int occurrence, const char *method) const;
    int ResolveBehaviorLocal(const std::string &name, int occurrence, const char *method) const;
    int ResolveSetting(const std::string &name, int occurrence, const char *method) const;
    int ResolveSettingIndex(int index, const char *method) const;
    CKParameterIn *FindPinParameter(const std::string &name, int occurrence) const;
    CKParameterIn *FindPinParameterIndex(int index) const;
    CKParameter *FindLocalSource(const std::string &name, int occurrence) const;
    CKParameter *FindLocalSourceIndex(int index) const;
    CKParameterLocal *FindLocalParameter(const std::string &name, int occurrence) const;
    CKParameterLocal *FindLocalParameterIndex(int index) const;
    ScriptParamValue ReadPinValueOrEmpty(const std::string &name, int occurrence) const;
    ScriptParamValue ReadPinValueOrEmpty(int index) const;
    ScriptParamValue ReadLocalValueOrEmpty(const std::string &name, int occurrence) const;
    ScriptParamValue ReadLocalValueOrEmpty(int index) const;

    CKBehaviorContext m_ContextSnapshot;
    CKContext *m_Context = nullptr;
    CK_ID m_BehaviorId = 0;
    ScriptBridgeObjectStamp m_BehaviorStamp;
    ScriptBridgeObjectResolvePolicy m_BehaviorResolvePolicy =
        ScriptBridgeObjectResolvePolicy::LiveOnly;
};

void BehaviorFrameGetInputParameterValueGeneric(asIScriptGeneric *generic);
void BehaviorFrameGetOutputParameterValueGeneric(asIScriptGeneric *generic);
void BehaviorFrameSetOutputParameterValueGeneric(asIScriptGeneric *generic);
void BehaviorFrameGetLocalParameterValueGeneric(asIScriptGeneric *generic);
void BehaviorFrameSetLocalParameterValueGeneric(asIScriptGeneric *generic);

#endif // CK_SCRIPTBEHAVIORFRAME_H
