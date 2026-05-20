#include "ScriptBridgeHandles.h"

#include <fmt/format.h>

BBCallBuilder::BBCallBuilder(ScriptBehaviorBridge *bridge,
                             const CKBehaviorContext &ctx,
                             const ScriptBridgeBBInvocationSpec &request)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request) {}

BBCallBuilder *BBCallBuilder::Owner(CKBeObject *owner) {
    m_Request.OwnerId = owner ? owner->GetID() : 0;
    AddRef();
    return this;
}

BBCallBuilder *BBCallBuilder::Target(CKBeObject *target) {
    m_Request.TargetId = target ? target->GetID() : 0;
    AddRef();
    return this;
}

BBCallBuilder *BBCallBuilder::Set(int pinIndex, ParamValue *value) {
    if (value) ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value->Value());
    AddRef();
    return this;
}

BBCallBuilder *BBCallBuilder::SetSource(int pinIndex, ParamRef *source) {
    if (!source || !source->IsValid()) {
        SetScriptException("BBCallBuilder.SetSource requires a valid ParamRef source.");
        return nullptr;
    }
    ScriptBridgeInputSource request;
    request.PinIndex = pinIndex;
    request.SourceId = source->GetID();
    request.SourceStamp = source->Stamp();
    m_Request.SourceParameters.push_back(request);
    AddRef();
    return this;
}

BBCallBuilder *BBCallBuilder::SetOperation(int pinIndex, ParamOp *operation) {
    if (operation) m_Request.OperationParameters.push_back(operation->RequestForPin(pinIndex));
    AddRef();
    return this;
}

BBResult *BBCallBuilder::Run(int inputIndex) {
    return m_Bridge ? m_Bridge->RunCall(m_Request, m_Context, inputIndex) : nullptr;
}

BBTaskBuilder::BBTaskBuilder(ScriptBehaviorBridge *bridge,
                             const CKBehaviorContext &ctx,
                             const ScriptBridgeBBInvocationSpec &request)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request) {}

BBTaskBuilder *BBTaskBuilder::Owner(CKBeObject *owner) {
    m_Request.OwnerId = owner ? owner->GetID() : 0;
    AddRef();
    return this;
}

BBTaskBuilder *BBTaskBuilder::Target(CKBeObject *target) {
    m_Request.TargetId = target ? target->GetID() : 0;
    AddRef();
    return this;
}

BBTaskBuilder *BBTaskBuilder::Set(int pinIndex, ParamValue *value) {
    if (value) ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value->Value());
    AddRef();
    return this;
}

BBTaskBuilder *BBTaskBuilder::SetSource(int pinIndex, ParamRef *source) {
    if (!source || !source->IsValid()) {
        SetScriptException("BBTaskBuilder.SetSource requires a valid ParamRef source.");
        return nullptr;
    }
    ScriptBridgeInputSource request;
    request.PinIndex = pinIndex;
    request.SourceId = source->GetID();
    request.SourceStamp = source->Stamp();
    m_Request.SourceParameters.push_back(request);
    AddRef();
    return this;
}

BBTaskBuilder *BBTaskBuilder::SetOperation(int pinIndex, ParamOp *operation) {
    if (operation) m_Request.OperationParameters.push_back(operation->RequestForPin(pinIndex));
    AddRef();
    return this;
}

BBTask *BBTaskBuilder::Start(int inputIndex) {
    return m_Bridge ? m_Bridge->StartTask(m_Request, m_Context, inputIndex) : nullptr;
}

BBPrototype::BBPrototype(ScriptBehaviorBridge *bridge,
                         const CKBehaviorContext &ctx,
                         const ScriptBridgeBBInvocationSpec &request)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request) {}

BBCallBuilder *BBPrototype::Call() {
    return m_Bridge ? new BBCallBuilder(m_Bridge, m_Context, m_Request) : nullptr;
}

BBTaskBuilder *BBPrototype::Spawn() {
    return m_Bridge ? new BBTaskBuilder(m_Bridge, m_Context, m_Request) : nullptr;
}

BehaviorLayout *BBPrototype::Layout() const {
    return m_Bridge ? new BehaviorLayout(m_Bridge, m_Context, m_Request) : nullptr;
}

bool BBPrototype::IsValid() const {
    std::string error;
    return Prototype(error) != nullptr;
}

CKGUID BBPrototype::GetGuid() const {
    std::string error;
    CKBehaviorPrototype *prototype = Prototype(error);
    return prototype ? prototype->GetGuid() : CKGUID();
}

std::string BBPrototype::GetName() const {
    std::string error;
    CKBehaviorPrototype *prototype = Prototype(error);
    return prototype ? SafeString(prototype->GetName()) : std::string();
}

std::string BBPrototype::GetCategory() const {
    std::string error;
    CKBehaviorPrototype *prototype = Prototype(error);
    CKObjectDeclaration *decl = prototype ? prototype->GetSoureObjectDeclaration() : nullptr;
    return decl ? SafeString(decl->GetCategory()) : std::string();
}

std::string BBPrototype::GetQualifiedName() const {
    const std::string category = GetCategory();
    const std::string name = GetName();
    return category.empty() ? name : category + "/" + name;
}

std::string BBPrototype::Describe() const {
    BehaviorLayout *layout = Layout();
    std::string result = fmt::format("Building Block '{}' guid={}", GetQualifiedName(), GuidToString(GetGuid()));
    if (layout) {
        result += "\n" + layout->Describe();
        layout->Release();
    }
    return result;
}

CKBehaviorPrototype *BBPrototype::Prototype(std::string &error) const {
    return m_Bridge ? m_Bridge->ResolvePrototypeObject(m_Request, error) : nullptr;
}

BBBridge::BBBridge(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx)
    : m_Bridge(bridge), m_Context(ctx) {}

BBPrototype *BBBridge::PrototypeByName(const std::string &name) const {
    return m_Bridge ? m_Bridge->CreatePrototype(m_Context, name) : nullptr;
}

BBPrototype *BBBridge::PrototypeByGuid(CKGUID guid) const {
    return m_Bridge ? m_Bridge->CreatePrototype(m_Context, guid) : nullptr;
}
