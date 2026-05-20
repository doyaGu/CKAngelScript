#include "ScriptBridgeHandles.h"

#include <fmt/format.h>

#include "add_on/scriptarray/scriptarray.h"
#include "ScriptParameterConversion.h"

namespace ScriptBridgeBBInternal {

std::string PrototypeQualifiedName(CKObjectDeclaration *decl) {
    if (!decl) {
        return std::string();
    }
    const std::string name = SafeString(decl->GetName());
    const std::string category = SafeString(decl->GetCategory());
    return category.empty() ? name : category + "/" + name;
}

bool PrototypeMatches(CKObjectDeclaration *decl, const std::string &query) {
    if (!decl) {
        return false;
    }
    return SafeString(decl->GetName()) == query || PrototypeQualifiedName(decl) == query || GuidToString(decl->GetGuid()) == query;
}

asITypeInfo *PrototypeArrayType(ScriptBehaviorBridge *bridge) {
    ScriptManager *manager = bridge ? bridge->GetManager() : nullptr;
    asIScriptEngine *engine = manager ? manager->GetScriptEngine() : nullptr;
    return engine ? engine->GetTypeInfoByDecl("array<BBPrototype@>") : nullptr;
}

CScriptArray *CreatePrototypeArray(ScriptBehaviorBridge *bridge) {
    asITypeInfo *arrayType = PrototypeArrayType(bridge);
    if (!arrayType) {
        SetScriptException("array<BBPrototype@> is not registered.");
        return nullptr;
    }
    return CScriptArray::Create(arrayType, asUINT(0));
}

void AppendPrototype(CScriptArray *array, BBPrototype *prototype) {
    if (!array || !prototype) {
        return;
    }
    const asUINT index = array->GetSize();
    array->Resize(index + 1);
    array->SetValue(index, &prototype);
    prototype->Release();
}

} // namespace ScriptBridgeBBInternal

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

int BBBridge::Count() const {
    return CKGetPrototypeDeclarationCount();
}

BBPrototype *BBBridge::At(int index) const {
    if (!m_Bridge || index < 0 || index >= Count()) {
        return nullptr;
    }

    CKObjectDeclaration *decl = CKGetPrototypeDeclaration(index);
    return decl ? m_Bridge->CreatePrototype(m_Context, decl->GetGuid()) : nullptr;
}

BBPrototype *BBBridge::Find(const std::string &query, int occurrence) const {
    if (!m_Bridge || query.empty() || occurrence < 0) {
        return nullptr;
    }

    CKGUID parsed;
    if (ParseScriptGuidString(query, parsed)) {
        return occurrence == 0 && CKGetPrototypeFromGuid(parsed) ? m_Bridge->CreatePrototype(m_Context, parsed) : nullptr;
    }

    int seen = 0;
    const int count = CKGetPrototypeDeclarationCount();
    for (int i = 0; i < count; ++i) {
        CKObjectDeclaration *decl = CKGetPrototypeDeclaration(i);
        if (!ScriptBridgeBBInternal::PrototypeMatches(decl, query)) {
            continue;
        }
        if (seen == occurrence) {
            return m_Bridge->CreatePrototype(m_Context, decl->GetGuid());
        }
        ++seen;
    }

    return nullptr;
}

CScriptArray *BBBridge::FindAll(const std::string &query) const {
    if (!m_Bridge) {
        return nullptr;
    }

    CScriptArray *results = ScriptBridgeBBInternal::CreatePrototypeArray(m_Bridge);
    if (!results) {
        return nullptr;
    }

    CKGUID parsed;
    if (!query.empty() && ParseScriptGuidString(query, parsed)) {
        if (CKGetPrototypeFromGuid(parsed)) {
            ScriptBridgeBBInternal::AppendPrototype(results, m_Bridge->CreatePrototype(m_Context, parsed));
        }
        return results;
    }

    const int count = CKGetPrototypeDeclarationCount();
    for (int i = 0; i < count; ++i) {
        CKObjectDeclaration *decl = CKGetPrototypeDeclaration(i);
        if (!query.empty() && !ScriptBridgeBBInternal::PrototypeMatches(decl, query)) {
            continue;
        }
        if (decl) {
            ScriptBridgeBBInternal::AppendPrototype(results, m_Bridge->CreatePrototype(m_Context, decl->GetGuid()));
        }
    }

    return results;
}
