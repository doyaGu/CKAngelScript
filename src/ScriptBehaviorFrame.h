#ifndef CK_SCRIPTBEHAVIORFRAME_H
#define CK_SCRIPTBEHAVIORFRAME_H

#include <string>

#include "CKBehavior.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptRefCounted.h"

class BBCallBuilder;
class ParamRef;

class BehaviorFrame final : public RefCounted {
public:
    explicit BehaviorFrame(const CKBehaviorContext &ctx);
    ~BehaviorFrame() override = default;

    CKBehavior *Behavior() const;
    CKContext *Context() const;
    CKBeObject *Owner() const;
    CKBeObject *Target() const;

    bool InputActive(const std::string &name = "In") const;
    bool Take(const std::string &name = "In");
    void Emit(const std::string &name = "Out");

    ParamRef *Pin(const std::string &name, int occurrence = 0) const;
    ParamRef *Pout(const std::string &name, int occurrence = 0) const;
    std::string PinString(const std::string &name, const std::string &defaultValue = "", int occurrence = 0) const;
    int PinInt(const std::string &name, int defaultValue = 0, int occurrence = 0) const;
    bool PinBool(const std::string &name, bool defaultValue = false, int occurrence = 0) const;
    float PinFloat(const std::string &name, float defaultValue = 0.0f, int occurrence = 0) const;
    CKObject *PinObject(const std::string &name, int occurrence = 0) const;

    bool SetPoutInt(const std::string &name, int value, int occurrence = 0);
    bool SetPoutFloat(const std::string &name, float value, int occurrence = 0);
    bool SetPoutBool(const std::string &name, bool value, int occurrence = 0);
    bool SetPoutString(const std::string &name, const std::string &value, int occurrence = 0);
    bool SetPoutObject(const std::string &name, CKObject *value, int occurrence = 0);

    BBCallBuilder *CallName(const std::string &prototype) const;
    BBCallBuilder *CallGuid(CKGUID guid) const;

    void Trace(const std::string &message) const;

    CKBehaviorContext MakeContext() const;

private:
    ScriptBehaviorBridge *Bridge() const;
    int ResolveInput(const std::string &name, const char *method) const;
    int ResolveOutput(const std::string &name, const char *method) const;
    int ResolvePin(const std::string &name, int occurrence, const char *method) const;
    int ResolvePout(const std::string &name, int occurrence, const char *method) const;

    CKBehaviorContext m_ContextSnapshot;
    CKContext *m_Context = nullptr;
    CK_ID m_BehaviorId = 0;
    ScriptBridgeObjectStamp m_BehaviorStamp;
};

#endif // CK_SCRIPTBEHAVIORFRAME_H
