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

std::vector<CKObjectDeclaration *> FindPrototypeDeclarations(const std::string &query) {
    std::vector<CKObjectDeclaration *> matches;
    CKGUID parsed;
    if (!query.empty() && ParseScriptGuidString(query, parsed)) {
        if (CKBehaviorPrototype *prototype = CKGetPrototypeFromGuid(parsed)) {
            if (CKObjectDeclaration *decl = prototype->GetSoureObjectDeclaration()) {
                matches.push_back(decl);
            }
        }
        return matches;
    }

    const int count = CKGetPrototypeDeclarationCount();
    for (int i = 0; i < count; ++i) {
        CKObjectDeclaration *decl = CKGetPrototypeDeclaration(i);
        if (query.empty() || PrototypeMatches(decl, query)) {
            if (decl) {
                matches.push_back(decl);
            }
        }
    }
    return matches;
}

std::string PrototypeCandidateList(const std::vector<CKObjectDeclaration *> &matches) {
    std::string text;
    for (CKObjectDeclaration *decl : matches) {
        if (!decl) {
            continue;
        }
        if (!text.empty()) {
            text += "; ";
        }
        text += fmt::format("{} ({})", PrototypeQualifiedName(decl), GuidToString(decl->GetGuid()));
    }
    return text.empty() ? "<none>" : text;
}

const char *SlotKindName(ScriptBridgeSlotKind kind) {
    switch (kind) {
        case ScriptBridgeSlotKind::Input: return "input";
        case ScriptBridgeSlotKind::Output: return "output";
        case ScriptBridgeSlotKind::Pin: return "pin";
        case ScriptBridgeSlotKind::Pout: return "pout";
        case ScriptBridgeSlotKind::Setting: return "setting";
        case ScriptBridgeSlotKind::Local: return "local";
        default: return "slot";
    }
}

std::string CandidateList(const ScriptBridgeLayoutRecord &layout, ScriptBridgeSlotKind kind) {
    std::string text;
    auto append = [&](int index, const std::string &name) {
        if (!text.empty()) {
            text += ", ";
        }
        text += fmt::format("#{} '{}'", index, name);
    };

    if (kind == ScriptBridgeSlotKind::Input) {
        for (int i = 0; i < static_cast<int>(layout.Inputs.size()); ++i) append(i, layout.Inputs[i].Name);
    } else if (kind == ScriptBridgeSlotKind::Output) {
        for (int i = 0; i < static_cast<int>(layout.Outputs.size()); ++i) append(i, layout.Outputs[i].Name);
    } else {
        const std::vector<ScriptBridgeLayoutParamSlot> *slots = nullptr;
        if (kind == ScriptBridgeSlotKind::Pin) slots = &layout.Pins;
        if (kind == ScriptBridgeSlotKind::Pout) slots = &layout.Pouts;
        if (kind == ScriptBridgeSlotKind::Setting) slots = &layout.Settings;
        if (kind == ScriptBridgeSlotKind::Local) slots = &layout.Locals;
        if (slots) {
            for (int i = 0; i < static_cast<int>(slots->size()); ++i) append(i, (*slots)[i].Name);
        }
    }

    return text.empty() ? "<none>" : text;
}

bool FindIoSlot(const ScriptBridgeLayoutRecord &layout,
                ScriptBridgeSlotKind kind,
                const std::string &name,
                int occurrence,
                int &index) {
    const std::vector<ScriptBridgeLayoutIoSlot> *slots = nullptr;
    if (kind == ScriptBridgeSlotKind::Input) slots = &layout.Inputs;
    if (kind == ScriptBridgeSlotKind::Output) slots = &layout.Outputs;
    if (!slots || occurrence < 0) {
        return false;
    }

    int seen = 0;
    for (int i = 0; i < static_cast<int>(slots->size()); ++i) {
        if ((*slots)[i].Name == name) {
            if (seen == occurrence) {
                index = i;
                return true;
            }
            ++seen;
        }
    }
    return false;
}

const ScriptBridgeLayoutParamSlot *FindParamSlot(const ScriptBridgeLayoutRecord &layout,
                                                 ScriptBridgeSlotKind kind,
                                                 const std::string &name,
                                                 int occurrence) {
    const std::vector<ScriptBridgeLayoutParamSlot> *slots = nullptr;
    if (kind == ScriptBridgeSlotKind::Pin) slots = &layout.Pins;
    if (kind == ScriptBridgeSlotKind::Pout) slots = &layout.Pouts;
    if (kind == ScriptBridgeSlotKind::Setting) slots = &layout.Settings;
    if (kind == ScriptBridgeSlotKind::Local) slots = &layout.Locals;
    if (!slots || occurrence < 0) {
        return nullptr;
    }

    int seen = 0;
    for (const ScriptBridgeLayoutParamSlot &slot : *slots) {
        if (slot.Name == name) {
            if (seen == occurrence) {
                return &slot;
            }
            ++seen;
        }
    }
    return nullptr;
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

BBSlot::BBSlot(ScriptBehaviorBridge *bridge,
               const CKBehaviorContext &ctx,
               const ScriptBridgeBBInvocationSpec &request,
               ScriptBridgeSlotKind kind,
               int index,
               const std::string &name,
               CKGUID typeGuid,
               const std::string &typeName,
               int dataSize,
               CKDWORD caps,
               int layoutGeneration,
               const std::string &layoutSignature,
               const std::string &error)
    : m_Bridge(bridge),
      m_Context(ctx),
      m_Request(request),
      m_Kind(kind),
      m_Index(index),
      m_Name(name),
      m_TypeGuid(typeGuid),
      m_TypeName(typeName),
      m_DataSize(dataSize),
      m_Caps(caps),
      m_LayoutGeneration(layoutGeneration),
      m_LayoutSignature(layoutSignature),
      m_Error(error) {}

bool BBSlot::IsValid() const {
    if (!m_Error.empty() || m_Index < 0 || !m_Bridge) {
        return false;
    }
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    return layout && layout->Signature == m_LayoutSignature;
}

std::string BBSlot::Error() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    if (m_Index < 0) {
        return "BBSlot is not bound.";
    }
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    if (!layout) {
        return "BBSlot layout is not available.";
    }
    if (layout->Signature != m_LayoutSignature) {
        return fmt::format("BBSlot '{}' layout changed; bind the slot again.", m_Name);
    }
    return std::string();
}

int BBSlot::Kind() const { return static_cast<int>(m_Kind); }
int BBSlot::Index() const { return m_Index; }
std::string BBSlot::Name() const { return m_Name; }
CKGUID BBSlot::TypeGuid() const { return m_TypeGuid; }
std::string BBSlot::TypeName() const { return m_TypeName; }
int BBSlot::DataSize() const { return m_DataSize; }
CKDWORD BBSlot::Caps() const { return m_Caps; }
int BBSlot::LayoutGeneration() const { return m_LayoutGeneration; }
bool BBSlot::IsSetting() const { return HasScriptBridgeSlotCap(m_Caps, ScriptBridgeSlotCaps::Setting) || m_Kind == ScriptBridgeSlotKind::Setting; }

std::string BBSlot::Describe() const {
    if (!IsValid()) {
        return "Invalid BBSlot: " + Error();
    }
    if (m_Kind == ScriptBridgeSlotKind::Input || m_Kind == ScriptBridgeSlotKind::Output) {
        return fmt::format("{} #{} '{}'", KindName(), m_Index, m_Name);
    }
    return fmt::format("{} #{} '{}' type={} size={}",
                       KindName(),
                       m_Index,
                       m_Name,
                       m_TypeName,
                       m_DataSize);
}

bool BBSlot::ResolveIndex(ScriptBridgeSlotKind expected, int &index, std::string &error) const {
    if (m_Kind != expected) {
        error = fmt::format("Expected {} slot, got {} slot '{}'.",
                            ScriptBridgeBBInternal::SlotKindName(expected),
                            KindName(),
                            m_Name);
        return false;
    }
    if (!IsValid()) {
        error = Error();
        return false;
    }
    index = m_Index;
    return true;
}

const ScriptBridgeLayoutRecord *BBSlot::LayoutRecord() const {
    return m_Bridge ? m_Bridge->GetPrototypeLayout(m_Context, m_Request) : nullptr;
}

std::string BBSlot::KindName() const {
    return ScriptBridgeBBInternal::SlotKindName(m_Kind);
}

BBSpec::BBSpec(ScriptBehaviorBridge *bridge,
               const CKBehaviorContext &ctx,
               const ScriptBridgeBBInvocationSpec &request,
               const std::string &error)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request), m_Error(error) {}

bool BBSpec::IsValid() const {
    std::string error;
    return m_Error.empty() && PrototypeObject(error) != nullptr;
}

std::string BBSpec::Error() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    std::string error;
    PrototypeObject(error);
    return error;
}

CKGUID BBSpec::GetGuid() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    return prototype ? prototype->GetGuid() : CKGUID();
}

std::string BBSpec::GetName() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    return prototype ? SafeString(prototype->GetName()) : std::string();
}

std::string BBSpec::GetCategory() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    CKObjectDeclaration *decl = prototype ? prototype->GetSoureObjectDeclaration() : nullptr;
    return decl ? SafeString(decl->GetCategory()) : std::string();
}

std::string BBSpec::GetQualifiedName() const {
    const std::string category = GetCategory();
    const std::string name = GetName();
    return category.empty() ? name : category + "/" + name;
}

CKGUID BBSpec::Guid() const { return GetGuid(); }
std::string BBSpec::Name() const { return GetName(); }
std::string BBSpec::Category() const { return GetCategory(); }
std::string BBSpec::QualifiedName() const { return GetQualifiedName(); }

CKDWORD BBSpec::BehaviorFlags() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    return prototype ? static_cast<CKDWORD>(prototype->GetBehaviorFlags()) : 0;
}

CKDWORD BBSpec::PrototypeFlags() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    return prototype ? static_cast<CKDWORD>(prototype->GetFlags()) : 0;
}

CK_CLASSID BBSpec::CompatibleClassId() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    if (!prototype) {
        return 0;
    }
    CKObjectDeclaration *decl = prototype->GetSoureObjectDeclaration();
    return decl ? decl->GetCompatibleClassId() : prototype->GetApplyToClassID();
}

int BBSpec::NeededManagerCount() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    CKObjectDeclaration *decl = prototype ? prototype->GetSoureObjectDeclaration() : nullptr;
    return decl ? decl->GetManagerNeededCount() : 0;
}

CKGUID BBSpec::NeededManagerGuid(int index) const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    CKObjectDeclaration *decl = prototype ? prototype->GetSoureObjectDeclaration() : nullptr;
    return decl && index >= 0 && index < decl->GetManagerNeededCount() ? decl->GetManagerNeeded(index) : CKGUID();
}

BBPrototype *BBSpec::Prototype() const {
    return IsValid() ? new BBPrototype(m_Bridge, m_Context, m_Request) : nullptr;
}

BehaviorLayout *BBSpec::Layout() const {
    return IsValid() ? new BehaviorLayout(m_Bridge, m_Context, m_Request) : nullptr;
}

BBCallBuilder *BBSpec::Call() {
    return IsValid() ? new BBCallBuilder(m_Bridge, m_Context, m_Request) : nullptr;
}

BBTaskBuilder *BBSpec::Spawn() {
    return IsValid() ? new BBTaskBuilder(m_Bridge, m_Context, m_Request) : nullptr;
}

BBBinding *BBSpec::Bind() {
    return new BBBinding(m_Bridge, m_Context, m_Request, Error());
}

BBBinding *BBSpec::Configure() {
    return Bind();
}

BBSlot *BBSpec::In(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Input, name, occurrence); }
BBSlot *BBSpec::Input(const std::string &name, int occurrence) const { return In(name, occurrence); }
BBSlot *BBSpec::Out(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Output, name, occurrence); }
BBSlot *BBSpec::Output(const std::string &name, int occurrence) const { return Out(name, occurrence); }
BBSlot *BBSpec::Pin(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Pin, name, occurrence); }
BBSlot *BBSpec::Pout(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Pout, name, occurrence); }
BBSlot *BBSpec::Setting(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Setting, name, occurrence); }
BBSlot *BBSpec::Local(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Local, name, occurrence); }

std::string BBSpec::Describe() const {
    if (!IsValid()) {
        return "Invalid BBSpec: " + Error();
    }
    BehaviorLayout *layout = Layout();
    std::string result = fmt::format("BBSpec '{}' guid={}", GetQualifiedName(), GuidToString(GetGuid()));
    if (layout) {
        result += "\n" + layout->Describe();
        layout->Release();
    }
    return result;
}

CKBehaviorPrototype *BBSpec::PrototypeObject(std::string &error) const {
    return m_Bridge ? m_Bridge->ResolvePrototypeObject(m_Request, error) : nullptr;
}

BBSlot *BBSpec::ResolveSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const {
    if (!m_Bridge) {
        return new BBSlot(nullptr, m_Context, m_Request, kind, -1, name, CKGUID(), std::string(), 0, 0, 0, std::string(), "BB bridge is not available.");
    }
    if (!m_Error.empty()) {
        return new BBSlot(m_Bridge, m_Context, m_Request, kind, -1, name, CKGUID(), std::string(), 0, 0, 0, std::string(), m_Error);
    }
    const ScriptBridgeLayoutRecord *layout = m_Bridge->GetPrototypeLayout(m_Context, m_Request);
    if (!layout) {
        return new BBSlot(m_Bridge, m_Context, m_Request, kind, -1, name, CKGUID(), std::string(), 0, 0, 0, std::string(), "BB layout is not available.");
    }

    if (kind == ScriptBridgeSlotKind::Input || kind == ScriptBridgeSlotKind::Output) {
        int index = -1;
        if (ScriptBridgeBBInternal::FindIoSlot(*layout, kind, name, occurrence, index)) {
            return new BBSlot(m_Bridge, m_Context, m_Request, kind, index, name, CKGUID(), std::string(), 0, 0, layout->LayoutGeneration, layout->Signature);
        }
    } else if (const ScriptBridgeLayoutParamSlot *slot = ScriptBridgeBBInternal::FindParamSlot(*layout, kind, name, occurrence)) {
        return new BBSlot(m_Bridge, m_Context, m_Request, kind, slot->Index, slot->Name, slot->TypeGuid, slot->TypeName, slot->DataSize, slot->Caps, layout->LayoutGeneration, layout->Signature);
    }

    return new BBSlot(m_Bridge,
                      m_Context,
                      m_Request,
                      kind,
                      -1,
                      name,
                      CKGUID(),
                      std::string(),
                      0,
                      0,
                      layout->LayoutGeneration,
                      layout->Signature,
                      fmt::format("BB '{}' has no {} named '{}' (occurrence {}). Candidates: {}.",
                                  GetQualifiedName(),
                                  ScriptBridgeBBInternal::SlotKindName(kind),
                                  name,
                                  occurrence,
                                  ScriptBridgeBBInternal::CandidateList(*layout, kind)));
}

BBBinding::BBBinding(ScriptBehaviorBridge *bridge,
                     const CKBehaviorContext &ctx,
                     const ScriptBridgeBBInvocationSpec &request,
                     const std::string &error)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request), m_Error(error) {}

BBBinding::~BBBinding() {
    Destroy();
    for (CachedSlot &slot : m_Slots) {
        if (slot.Slot) {
            slot.Slot->Release();
            slot.Slot = nullptr;
        }
    }
}

bool BBBinding::IsValid() const {
    if (!m_Error.empty() || !m_Bridge) {
        return false;
    }
    BBSpec spec(m_Bridge, m_Context, m_Request);
    return spec.IsValid();
}

std::string BBBinding::Error() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    if (!m_Bridge) {
        return "BBBinding bridge is not available.";
    }
    BBSpec spec(m_Bridge, m_Context, m_Request);
    return spec.Error();
}

std::string BBBinding::Describe() const {
    BBSpec spec(m_Bridge, m_Context, m_Request, m_Error);
    std::string text = spec.Describe();
    if (!m_DefaultStartInput.empty()) {
        text += fmt::format("\nDefault start input: '{}'", m_DefaultStartInput);
    }
    if (!m_DefaultStopInput.empty()) {
        text += fmt::format("\nDefault stop input: '{}'", m_DefaultStopInput);
    }
    return text;
}

BBSpec *BBBinding::Spec() const {
    return new BBSpec(m_Bridge, m_Context, m_Request, m_Error);
}

BBTask *BBBinding::Task() const {
    return ReturnTask();
}

BehaviorRef *BBBinding::Behavior() const {
    return m_Task ? m_Task->Behavior() : nullptr;
}

bool BBBinding::Raise(const CKBehaviorContext &ctx) const {
    if (IsValid()) {
        return true;
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = Error();
    return RaiseExecutionState(state, ctx);
}

BBSlot *BBBinding::In(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Input, name, occurrence); }
BBSlot *BBBinding::Out(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Output, name, occurrence); }
BBSlot *BBBinding::Pin(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Pin, name, occurrence); }
BBSlot *BBBinding::Pout(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Pout, name, occurrence); }
BBSlot *BBBinding::Local(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Local, name, occurrence); }

bool BBBinding::RequireSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) {
    BBSlot *slot = Slot(kind, name, occurrence);
    const bool ok = slot && slot->IsValid();
    if (!ok) {
        SetError(slot ? slot->Error() : fmt::format("Failed to bind {} '{}'.", ScriptBridgeBBInternal::SlotKindName(kind), name));
    }
    if (slot) {
        slot->Release();
    }
    return ok;
}

BBBinding *BBBinding::Owner(CKBeObject *owner) {
    m_Request.OwnerId = owner ? owner->GetID() : 0;
    AddRef();
    return this;
}

BBBinding *BBBinding::Target(CKBeObject *target) {
    m_Request.TargetId = target ? target->GetID() : 0;
    AddRef();
    return this;
}

BBBinding *BBBinding::Set(const std::string &pinName, ParamValue *value) {
    BBSlot *slot = Pin(pinName);
    BBBinding *result = SetSlot(slot, value);
    if (slot) {
        slot->Release();
    }
    return result;
}

BBBinding *BBBinding::SetInt(const std::string &pinName, int value) {
    BBSlot *slot = Pin(pinName);
    BBBinding *result = SetSlotInt(slot, value);
    if (slot) {
        slot->Release();
    }
    return result;
}

BBBinding *BBBinding::SetFloat(const std::string &pinName, float value) {
    BBSlot *slot = Pin(pinName);
    BBBinding *result = SetSlotFloat(slot, value);
    if (slot) {
        slot->Release();
    }
    return result;
}

BBBinding *BBBinding::SetBool(const std::string &pinName, bool value) {
    BBSlot *slot = Pin(pinName);
    BBBinding *result = SetSlotBool(slot, value);
    if (slot) {
        slot->Release();
    }
    return result;
}

BBBinding *BBBinding::SetString(const std::string &pinName, const std::string &value) {
    BBSlot *slot = Pin(pinName);
    BBBinding *result = SetSlotString(slot, value);
    if (slot) {
        slot->Release();
    }
    return result;
}

BBBinding *BBBinding::SetObject(const std::string &pinName, CKObject *value) {
    BBSlot *slot = Pin(pinName);
    BBBinding *result = SetSlotObject(slot, value);
    if (slot) {
        slot->Release();
    }
    return result;
}

BBBinding *BBBinding::SetSlot(BBSlot *pin, ParamValue *value) {
    if (!value) {
        SetError("BBBinding.Set requires a valid ParamValue.");
        SetScriptException(m_Error);
        return nullptr;
    }
    return SetValueForPin(pin, value->Value(), "BBBinding.Set") ? (AddRef(), this) : nullptr;
}

BBBinding *BBBinding::SetSlotInt(BBSlot *pin, int value) {
    return SetValueForPin(pin, MakeScriptParamInt(value), "BBBinding.Set") ? (AddRef(), this) : nullptr;
}

BBBinding *BBBinding::SetSlotFloat(BBSlot *pin, float value) {
    return SetValueForPin(pin, MakeScriptParamFloat(value), "BBBinding.Set") ? (AddRef(), this) : nullptr;
}

BBBinding *BBBinding::SetSlotBool(BBSlot *pin, bool value) {
    return SetValueForPin(pin, MakeScriptParamBool(value), "BBBinding.Set") ? (AddRef(), this) : nullptr;
}

BBBinding *BBBinding::SetSlotString(BBSlot *pin, const std::string &value) {
    return SetValueForPin(pin, MakeScriptParamString(value), "BBBinding.Set") ? (AddRef(), this) : nullptr;
}

BBBinding *BBBinding::SetSlotObject(BBSlot *pin, CKObject *value) {
    return SetValueForPin(pin, MakeScriptParamObject(value), "BBBinding.Set") ? (AddRef(), this) : nullptr;
}

BBBinding *BBBinding::Source(const std::string &pinName, ParamRef *source) {
    BBSlot *slot = Pin(pinName);
    BBBinding *result = SourceSlot(slot, source);
    if (slot) {
        slot->Release();
    }
    return result;
}

BBBinding *BBBinding::SourceSlot(BBSlot *pin, ParamRef *source) {
    return SourceForPin(pin, source, "BBBinding.Source") ? (AddRef(), this) : nullptr;
}

BBBinding *BBBinding::Operation(const std::string &pinName, ParamOp *operation) {
    BBSlot *slot = Pin(pinName);
    BBBinding *result = OperationSlot(slot, operation);
    if (slot) {
        slot->Release();
    }
    return result;
}

BBBinding *BBBinding::OperationSlot(BBSlot *pin, ParamOp *operation) {
    return OperationForPin(pin, operation, "BBBinding.Operation") ? (AddRef(), this) : nullptr;
}

BBBinding *BBBinding::SetSetting(BBSlot *setting, ParamValue *value) {
    if (!value) {
        SetError("BBConfig.SetSetting requires a valid ParamValue.");
        SetScriptException(m_Error);
        return nullptr;
    }
    return SetValueForSetting(setting, value->Value(), "BBConfig.SetSetting") ? (AddRef(), this) : nullptr;
}

BBBinding *BBBinding::SetSettingString(BBSlot *setting, const std::string &value) {
    return SetValueForSetting(setting, MakeScriptParamString(value), "BBConfig.SetSetting") ? (AddRef(), this) : nullptr;
}

bool BBBinding::Validate(const CKBehaviorContext &ctx) const {
    (void) ctx;
    if (!IsValid()) {
        SetScriptException(Error());
        return false;
    }
    return true;
}

BBSpec *BBBinding::Decl() const {
    return new BBSpec(m_Bridge, m_Context, m_Request, m_Error);
}

BBInstance *BBBinding::SpawnInstance(const CKBehaviorContext &ctx) {
    return new BBInstance(m_Bridge, ctx, m_Request, Error());
}

BBTask *BBBinding::Start(const CKBehaviorContext &ctx) {
    BBSlot *input = DefaultInputSlot();
    BBTask *task = StartSlot(ctx, input);
    if (input) {
        input->Release();
    }
    return task;
}

BBTask *BBBinding::StartName(const CKBehaviorContext &ctx, const std::string &inputName) {
    BBSlot *input = inputName.empty() ? nullptr : In(inputName);
    BBTask *task = input ? StartSlot(ctx, input) : Start(ctx);
    if (input) {
        input->Release();
    }
    return task;
}

BBTask *BBBinding::StartSlot(const CKBehaviorContext &ctx, BBSlot *input) {
    if (!m_Bridge) {
        SetError("BBBinding.Start requires ScriptBehaviorBridge.");
        SetScriptException(m_Error);
        return nullptr;
    }
    if (!IsValid()) {
        SetScriptException(Error());
        return nullptr;
    }

    int inputIndex = 0;
    if (input) {
        std::string error;
        if (!input->ResolveIndex(ScriptBridgeSlotKind::Input, inputIndex, error)) {
            SetError(error.empty() ? "BBBinding.Start requires an input BBSlot." : error);
            SetScriptException(m_Error);
            return nullptr;
        }
    }

    Destroy();
    m_Context = ctx;
    m_Task = m_Bridge->StartTask(m_Request, ctx, inputIndex);
    if (!m_Task) {
        SetError("BBBinding.Start failed to create task.");
        SetScriptException(m_Error);
        return nullptr;
    }
    if (!m_Task->IsAlive()) {
        SetError(m_Task->Error());
    }
    return ReturnTask();
}

bool BBBinding::Step(const CKBehaviorContext &ctx) {
    BBSlot *input = DefaultInputSlot();
    const bool result = input ? StepSlot(ctx, input) : (m_Task && m_Task->Step(ctx, -1));
    if (!input && !m_Task) {
        SetError("BBBinding.Step called before Start.");
        SetScriptException(m_Error);
    }
    if (input) {
        input->Release();
    }
    return result;
}

bool BBBinding::StepName(const CKBehaviorContext &ctx, const std::string &inputName) {
    BBSlot *input = inputName.empty() ? nullptr : In(inputName);
    const bool result = input ? StepSlot(ctx, input) : Step(ctx);
    if (input) {
        input->Release();
    }
    return result;
}

bool BBBinding::StepSlot(const CKBehaviorContext &ctx, BBSlot *input) {
    if (!m_Task || !m_Task->IsAlive()) {
        SetError("BBBinding.Step called before Start or after Destroy.");
        SetScriptException(m_Error);
        return false;
    }
    if (!input) {
        return m_Task->Step(ctx, -1);
    }
    if (!m_Task->StepSlot(ctx, input)) {
        SetError(m_Task->Error());
        return false;
    }
    return true;
}

bool BBBinding::Stop(const CKBehaviorContext &ctx) {
    BBSlot *stop = DefaultStopSlot();
    const bool result = stop ? StopSlot(ctx, stop) : Destroy();
    if (stop) {
        stop->Release();
    }
    return result;
}

bool BBBinding::StopName(const CKBehaviorContext &ctx, const std::string &inputName) {
    BBSlot *input = inputName.empty() ? nullptr : In(inputName);
    const bool result = input ? StopSlot(ctx, input) : Stop(ctx);
    if (input) {
        input->Release();
    }
    return result;
}

bool BBBinding::StopSlot(const CKBehaviorContext &ctx, BBSlot *input) {
    if (m_Task && m_Task->IsAlive() && input) {
        m_Task->StepSlot(ctx, input);
    }
    return Destroy();
}

BBTask *BBBinding::Restart(const CKBehaviorContext &ctx) {
    Stop(ctx);
    return Start(ctx);
}

bool BBBinding::Destroy() {
    ClearOwnedGraphLinks();
    if (!m_Task) {
        return true;
    }
    const bool result = m_Task->Destroy();
    m_Task->Release();
    m_Task = nullptr;
    return result;
}

bool BBBinding::OutputActive(const std::string &outputName) {
    BBSlot *output = Out(outputName);
    const bool result = OutputActiveSlot(output);
    if (output) {
        output->Release();
    }
    return result;
}

bool BBBinding::OutputActiveSlot(BBSlot *output) {
    return m_Task && m_Task->OutputActiveSlot(output);
}

ParamRef *BBBinding::PinRef(const std::string &pinName) {
    BBSlot *slot = Pin(pinName);
    ParamRef *ref = PinRefSlot(slot);
    if (slot) {
        slot->Release();
    }
    return ref;
}

ParamRef *BBBinding::PinRefSlot(BBSlot *pin) {
    BehaviorRef *behavior = Behavior();
    ParamRef *ref = behavior ? behavior->PinSlot(pin) : nullptr;
    if (behavior) {
        behavior->Release();
    }
    return ref;
}

ParamRef *BBBinding::PoutRef(const std::string &poutName) {
    BBSlot *slot = Pout(poutName);
    ParamRef *ref = PoutRefSlot(slot);
    if (slot) {
        slot->Release();
    }
    return ref;
}

ParamRef *BBBinding::PoutRefSlot(BBSlot *pout) {
    return m_Task ? m_Task->PoutSlot(pout) : nullptr;
}

void BBBinding::SetDefaultStart(const std::string &inputName) {
    m_DefaultStartInput = inputName;
}

void BBBinding::SetDefaultStop(const std::string &inputName) {
    m_DefaultStopInput = inputName;
}

void BBBinding::SetManaged(bool managed) {
    m_Managed = managed;
}

bool BBBinding::IsManaged() const {
    return m_Managed;
}

BBSlot *BBBinding::Slot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) {
    if (BBSlot *cached = FindCachedSlot(kind, name, occurrence)) {
        cached->AddRef();
        return cached;
    }

    BBSpec spec(m_Bridge, m_Context, m_Request, m_Error);
    BBSlot *slot = nullptr;
    switch (kind) {
        case ScriptBridgeSlotKind::Input: slot = spec.In(name, occurrence); break;
        case ScriptBridgeSlotKind::Output: slot = spec.Out(name, occurrence); break;
        case ScriptBridgeSlotKind::Pin: slot = spec.Pin(name, occurrence); break;
        case ScriptBridgeSlotKind::Pout: slot = spec.Pout(name, occurrence); break;
        case ScriptBridgeSlotKind::Setting: slot = spec.Setting(name, occurrence); break;
        case ScriptBridgeSlotKind::Local: slot = spec.Local(name, occurrence); break;
        default: break;
    }
    if (!slot) {
        SetError(fmt::format("Failed to bind {} '{}'.", ScriptBridgeBBInternal::SlotKindName(kind), name));
        return nullptr;
    }
    if (!slot->IsValid()) {
        SetError(slot->Error());
    }

    CachedSlot cached;
    cached.Kind = kind;
    cached.Occurrence = occurrence;
    cached.Name = name;
    cached.Slot = slot;
    m_Slots.push_back(cached);
    slot->AddRef();
    return slot;
}

BBSlot *BBBinding::FindCachedSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const {
    for (const CachedSlot &slot : m_Slots) {
        if (slot.Kind == kind && slot.Name == name && slot.Occurrence == occurrence) {
            return slot.Slot;
        }
    }
    return nullptr;
}

BBSlot *BBBinding::DefaultInputSlot() {
    return m_DefaultStartInput.empty() ? nullptr : In(m_DefaultStartInput);
}

BBSlot *BBBinding::DefaultStopSlot() {
    return m_DefaultStopInput.empty() ? nullptr : In(m_DefaultStopInput);
}

bool BBBinding::ResolvePin(BBSlot *slot, int &pinIndex, const char *method) {
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pin, pinIndex, error)) {
        SetError(fmt::format("{} requires a pin BBSlot.{}", method, error.empty() ? "" : " " + error));
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

bool BBBinding::SetValueForPin(BBSlot *slot, const ScriptParamValue &value, const char *method) {
    int pinIndex = -1;
    if (!ResolvePin(slot, pinIndex, method)) {
        return false;
    }
    ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value);
    if (m_Task && m_Task->IsAlive()) {
        return SetLiveValue(slot, value, method);
    }
    return true;
}

bool BBBinding::SetValueForSetting(BBSlot *slot, const ScriptParamValue &value, const char *method) {
    int settingIndex = -1;
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Setting, settingIndex, error)) {
        SetError(fmt::format("{} requires a setting BBSlot.{}", method, error.empty() ? "" : " " + error));
        SetScriptException(m_Error);
        return false;
    }
    ScriptBridgeSetIndexedValue(m_Request.IndexedSettings, settingIndex, value);
    if (!m_Task || !m_Task->IsAlive()) {
        return true;
    }
    if (!m_Bridge->SetTaskSetting(m_Task->BridgeTaskId(), m_Task->BridgeGeneration(), settingIndex, value, error)) {
        SetError(error);
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

bool BBBinding::SetLiveValue(BBSlot *slot, const ScriptParamValue &value, const char *method) {
    ParamRef *ref = PinRefSlot(slot);
    if (!ref) {
        SetError(fmt::format("{} could not resolve live pin.", method));
        SetScriptException(m_Error);
        return false;
    }
    ParamValue paramValue(value);
    const bool ok = ref->Set(&paramValue);
    if (!ok) {
        SetError(fmt::format("{} failed to write live pin '{}'.", method, slot ? slot->Name() : std::string("<null>")));
    }
    ref->Release();
    return ok;
}

bool BBBinding::SourceForPin(BBSlot *slot, ParamRef *source, const char *method) {
    int pinIndex = -1;
    if (!ResolvePin(slot, pinIndex, method)) {
        return false;
    }
    if (!source || !source->IsValid()) {
        SetError(fmt::format("{} requires a valid ParamRef source.", method));
        SetScriptException(m_Error);
        return false;
    }

    ScriptBridgeInputSource request;
    request.PinIndex = pinIndex;
    request.SourceId = source->GetID();
    request.SourceStamp = source->Stamp();
    m_Request.SourceParameters.push_back(request);

    if (!m_Task || !m_Task->IsAlive()) {
        return true;
    }

    ParamRef *target = PinRefSlot(slot);
    ParamSourceLinkRef *link = target ? target->SetSourceScoped(source) : nullptr;
    if (target) {
        target->Release();
    }
    if (!link || !link->IsValid()) {
        SetError(fmt::format("{} failed to connect live source.", method));
        if (link) {
            link->Release();
        }
        SetScriptException(m_Error);
        return false;
    }
    m_SourceLinks.push_back(link);
    return true;
}

bool BBBinding::OperationForPin(BBSlot *slot, ParamOp *operation, const char *method) {
    int pinIndex = -1;
    if (!ResolvePin(slot, pinIndex, method)) {
        return false;
    }
    if (!operation) {
        SetError(fmt::format("{} requires a valid ParamOp.", method));
        SetScriptException(m_Error);
        return false;
    }

    m_Request.OperationParameters.push_back(operation->RequestForPin(pinIndex));
    if (!m_Task || !m_Task->IsAlive()) {
        return true;
    }

    BehaviorRef *behavior = Behavior();
    ParamOperationRef *op = behavior ? behavior->ConnectOperationSlot(slot, operation) : nullptr;
    if (behavior) {
        behavior->Release();
    }
    if (!op || !op->IsValid()) {
        SetError(fmt::format("{} failed to connect live operation.", method));
        if (op) {
            op->Release();
        }
        SetScriptException(m_Error);
        return false;
    }
    m_Operations.push_back(op);
    return true;
}

void BBBinding::ClearOwnedGraphLinks() {
    for (ParamSourceLinkRef *link : m_SourceLinks) {
        if (link) {
            link->Restore();
            link->Release();
        }
    }
    m_SourceLinks.clear();

    for (ParamOperationRef *operation : m_Operations) {
        if (operation) {
            operation->Destroy();
            operation->Release();
        }
    }
    m_Operations.clear();
}

void BBBinding::SetError(const std::string &error) const {
    if (!error.empty()) {
        m_Error = error;
    }
}

BBTask *BBBinding::ReturnTask() const {
    if (m_Task) {
        m_Task->AddRef();
    }
    return m_Task;
}

BBInstance::BBInstance(ScriptBehaviorBridge *bridge,
                       const CKBehaviorContext &ctx,
                       const ScriptBridgeBBInvocationSpec &request,
                       const std::string &error)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request), m_Error(error) {}

BBInstance::~BBInstance() {
    Destroy();
}

bool BBInstance::IsValid() const {
    return m_Error.empty() && m_Bridge != nullptr;
}

std::string BBInstance::Error() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    if (!m_Bridge) {
        return "BBInstance bridge is not available.";
    }
    return m_Task ? m_Task->Error() : std::string();
}

BBSpec *BBInstance::Decl() const {
    return new BBSpec(m_Bridge, m_Context, m_Request, m_Error);
}

BehaviorRef *BBInstance::Behavior() const {
    return m_Task ? m_Task->Behavior() : nullptr;
}

bool BBInstance::Start(BBSlot *input) {
    if (!m_Bridge) {
        SetError("BBInstance.Start requires ScriptBehaviorBridge.");
        SetScriptException(m_Error);
        return false;
    }
    int inputIndex = 0;
    if (input) {
        std::string error;
        if (!input->ResolveIndex(ScriptBridgeSlotKind::Input, inputIndex, error)) {
            SetError(error.empty() ? "BBInstance.Start requires an input BBSlot." : error);
            SetScriptException(m_Error);
            return false;
        }
    }
    Destroy();
    m_Task = m_Bridge->StartTask(m_Request, m_Context, inputIndex);
    if (!m_Task) {
        SetError("BBInstance.Start failed to create runtime behavior.");
        SetScriptException(m_Error);
        return false;
    }
    return m_Task->IsAlive();
}

bool BBInstance::Step(const CKBehaviorContext &ctx) {
    if (!m_Task || !m_Task->IsAlive()) {
        SetError("BBInstance.Step called before Start or after Destroy.");
        SetScriptException(m_Error);
        return false;
    }
    return m_Task->Step(ctx, -1);
}

bool BBInstance::Stop() {
    return Destroy();
}

bool BBInstance::OutputActive(BBSlot *output) const {
    return m_Task && m_Task->OutputActiveSlot(output);
}

ParamRef *BBInstance::Pin(BBSlot *pin) const {
    BehaviorRef *behavior = Behavior();
    ParamRef *ref = behavior ? behavior->PinSlot(pin) : nullptr;
    if (behavior) {
        behavior->Release();
    }
    return ref;
}

ParamRef *BBInstance::Pout(BBSlot *pout) const {
    return m_Task ? m_Task->PoutSlot(pout) : nullptr;
}

bool BBInstance::SetSetting(BBSlot *setting, const std::string &value) {
    int settingIndex = -1;
    std::string error;
    if (!setting || !setting->ResolveIndex(ScriptBridgeSlotKind::Setting, settingIndex, error)) {
        SetError(error.empty() ? "BBInstance.SetSetting requires a setting BBSlot." : error);
        SetScriptException(m_Error);
        return false;
    }
    if (!m_Task || !m_Task->IsAlive()) {
        ScriptBridgeSetIndexedValue(m_Request.IndexedSettings, settingIndex, MakeScriptParamString(value));
        return true;
    }
    if (!m_Bridge->SetTaskSetting(m_Task->BridgeTaskId(), m_Task->BridgeGeneration(), settingIndex, MakeScriptParamString(value), error)) {
        SetError(error);
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

bool BBInstance::Destroy() {
    if (!m_Task) {
        return false;
    }
    const bool result = m_Task->Destroy();
    m_Task->Release();
    m_Task = nullptr;
    return result;
}

bool BBInstance::Raise(const CKBehaviorContext &ctx) const {
    if (IsValid()) {
        return true;
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = Error();
    return RaiseExecutionState(state, ctx);
}

void BBInstance::SetError(const std::string &error) const {
    if (!error.empty()) {
        m_Error = error;
    }
}

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
    if (value) {
        ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value->Value());
    }
    AddRef();
    return this;
}

BBCallBuilder *BBCallBuilder::SetSlot(BBSlot *slot, ParamValue *value) {
    int pinIndex = -1;
    if (!ResolvePinSlot(slot, pinIndex, "BBCallBuilder.Set") || !value) {
        if (!value) SetScriptException("BBCallBuilder.Set requires a valid ParamValue.");
        return nullptr;
    }
    return SetValueForPin(pinIndex, value->Value());
}

BBCallBuilder *BBCallBuilder::SetSlotInt(BBSlot *slot, int value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBCallBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamInt(value)) : nullptr;
}

BBCallBuilder *BBCallBuilder::SetSlotFloat(BBSlot *slot, float value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBCallBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamFloat(value)) : nullptr;
}

BBCallBuilder *BBCallBuilder::SetSlotBool(BBSlot *slot, bool value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBCallBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamBool(value)) : nullptr;
}

BBCallBuilder *BBCallBuilder::SetSlotString(BBSlot *slot, const std::string &value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBCallBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamString(value)) : nullptr;
}

BBCallBuilder *BBCallBuilder::SetSlotObject(BBSlot *slot, CKObject *value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBCallBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamObject(value)) : nullptr;
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

BBCallBuilder *BBCallBuilder::Source(BBSlot *slot, ParamRef *source) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBCallBuilder.Source") ? SetSource(pinIndex, source) : nullptr;
}

BBCallBuilder *BBCallBuilder::SetOperation(int pinIndex, ParamOp *operation) {
    if (operation) m_Request.OperationParameters.push_back(operation->RequestForPin(pinIndex));
    AddRef();
    return this;
}

BBCallBuilder *BBCallBuilder::Operation(BBSlot *slot, ParamOp *operation) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBCallBuilder.Operation") ? SetOperation(pinIndex, operation) : nullptr;
}

BBResult *BBCallBuilder::Run(int inputIndex) {
    return m_Bridge ? m_Bridge->RunCall(m_Request, m_Context, inputIndex) : nullptr;
}

BBResult *BBCallBuilder::RunSlot(BBSlot *input) {
    int inputIndex = -1;
    std::string error;
    if (!input || !input->ResolveIndex(ScriptBridgeSlotKind::Input, inputIndex, error)) {
        SetScriptException(error.empty() ? "BBCallBuilder.Run requires an input BBSlot." : error);
        return nullptr;
    }
    return Run(inputIndex);
}

BBCallBuilder *BBCallBuilder::SetValueForPin(int pinIndex, const ScriptParamValue &value) {
    ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value);
    AddRef();
    return this;
}

bool BBCallBuilder::ResolvePinSlot(BBSlot *slot, int &pinIndex, const char *method) {
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pin, pinIndex, error)) {
        SetScriptException(fmt::format("{} requires a pin BBSlot.{}", method, error.empty() ? "" : " " + error));
        return false;
    }
    return true;
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
    if (value) {
        ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value->Value());
    }
    AddRef();
    return this;
}

BBTaskBuilder *BBTaskBuilder::SetSlot(BBSlot *slot, ParamValue *value) {
    int pinIndex = -1;
    if (!ResolvePinSlot(slot, pinIndex, "BBTaskBuilder.Set") || !value) {
        if (!value) SetScriptException("BBTaskBuilder.Set requires a valid ParamValue.");
        return nullptr;
    }
    return SetValueForPin(pinIndex, value->Value());
}

BBTaskBuilder *BBTaskBuilder::SetSlotInt(BBSlot *slot, int value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBTaskBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamInt(value)) : nullptr;
}

BBTaskBuilder *BBTaskBuilder::SetSlotFloat(BBSlot *slot, float value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBTaskBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamFloat(value)) : nullptr;
}

BBTaskBuilder *BBTaskBuilder::SetSlotBool(BBSlot *slot, bool value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBTaskBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamBool(value)) : nullptr;
}

BBTaskBuilder *BBTaskBuilder::SetSlotString(BBSlot *slot, const std::string &value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBTaskBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamString(value)) : nullptr;
}

BBTaskBuilder *BBTaskBuilder::SetSlotObject(BBSlot *slot, CKObject *value) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBTaskBuilder.Set") ? SetValueForPin(pinIndex, MakeScriptParamObject(value)) : nullptr;
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

BBTaskBuilder *BBTaskBuilder::Source(BBSlot *slot, ParamRef *source) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBTaskBuilder.Source") ? SetSource(pinIndex, source) : nullptr;
}

BBTaskBuilder *BBTaskBuilder::SetOperation(int pinIndex, ParamOp *operation) {
    if (operation) m_Request.OperationParameters.push_back(operation->RequestForPin(pinIndex));
    AddRef();
    return this;
}

BBTaskBuilder *BBTaskBuilder::Operation(BBSlot *slot, ParamOp *operation) {
    int pinIndex = -1;
    return ResolvePinSlot(slot, pinIndex, "BBTaskBuilder.Operation") ? SetOperation(pinIndex, operation) : nullptr;
}

BBTask *BBTaskBuilder::Start(int inputIndex) {
    return m_Bridge ? m_Bridge->StartTask(m_Request, m_Context, inputIndex) : nullptr;
}

BBTask *BBTaskBuilder::StartSlot(BBSlot *input) {
    int inputIndex = -1;
    std::string error;
    if (!input || !input->ResolveIndex(ScriptBridgeSlotKind::Input, inputIndex, error)) {
        SetScriptException(error.empty() ? "BBTaskBuilder.Start requires an input BBSlot." : error);
        return nullptr;
    }
    return Start(inputIndex);
}

BBTaskBuilder *BBTaskBuilder::SetValueForPin(int pinIndex, const ScriptParamValue &value) {
    ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value);
    AddRef();
    return this;
}

bool BBTaskBuilder::ResolvePinSlot(BBSlot *slot, int &pinIndex, const char *method) {
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pin, pinIndex, error)) {
        SetScriptException(fmt::format("{} requires a pin BBSlot.{}", method, error.empty() ? "" : " " + error));
        return false;
    }
    return true;
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

    for (CKObjectDeclaration *decl : ScriptBridgeBBInternal::FindPrototypeDeclarations(query)) {
        ScriptBridgeBBInternal::AppendPrototype(results, m_Bridge->CreatePrototype(m_Context, decl->GetGuid()));
    }

    return results;
}

BBSpec *BBBridge::Require(const std::string &query) const {
    ScriptBridgeBBInvocationSpec request;
    request.ComponentId = ComponentIdFromContext(m_Context);
    request.PrototypeName = query;

    if (!m_Bridge) {
        return new BBSpec(nullptr, m_Context, request, "AngelScript BB bridge is not available.");
    }
    if (query.empty()) {
        return new BBSpec(m_Bridge, m_Context, request, "BB::Require needs a non-empty prototype name, Category/Name, or GUID.");
    }

    CKGUID parsed;
    if (ParseScriptGuidString(query, parsed)) {
        request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
        request.Guid = parsed;
        if (CKGetPrototypeFromGuid(parsed)) {
            return new BBSpec(m_Bridge, m_Context, request);
        }
        return new BBSpec(m_Bridge, m_Context, request, fmt::format("BB prototype GUID {} was not found.", GuidToString(parsed)));
    }

    const std::vector<CKObjectDeclaration *> matches = ScriptBridgeBBInternal::FindPrototypeDeclarations(query);
    if (matches.size() == 1 && matches[0]) {
        request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
        request.Guid = matches[0]->GetGuid();
        return new BBSpec(m_Bridge, m_Context, request);
    }
    if (matches.size() > 1) {
        return new BBSpec(m_Bridge,
                          m_Context,
                          request,
                          fmt::format("BB prototype '{}' is ambiguous. Candidates: {}.",
                                      query,
                                      ScriptBridgeBBInternal::PrototypeCandidateList(matches)));
    }

    return new BBSpec(m_Bridge,
                      m_Context,
                      request,
                      fmt::format("BB prototype '{}' was not found. Use BB::FindAll(query) to inspect candidates.", query));
}

BBSpec *BBBridge::RequireGuid(CKGUID guid) const {
    ScriptBridgeBBInvocationSpec request;
    request.ComponentId = ComponentIdFromContext(m_Context);
    request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
    request.Guid = guid;

    if (!m_Bridge) {
        return new BBSpec(nullptr, m_Context, request, "AngelScript BB bridge is not available.");
    }
    if (!guid.IsValid() || !CKGetPrototypeFromGuid(guid)) {
        return new BBSpec(m_Bridge, m_Context, request, fmt::format("BB prototype GUID {} was not found.", GuidToString(guid)));
    }
    return new BBSpec(m_Bridge, m_Context, request);
}

BBBinding *BBBridge::Bind(const std::string &query) const {
    BBSpec *spec = Require(query);
    if (!spec) {
        return new BBBinding(m_Bridge, m_Context, MakeDefaultRequest(m_Context), "Failed to create BBSpec.");
    }
    BBBinding *binding = spec->Bind();
    spec->Release();
    return binding;
}

BBBinding *BBBridge::BindGuid(CKGUID guid) const {
    BBSpec *spec = RequireGuid(guid);
    if (!spec) {
        return new BBBinding(m_Bridge, m_Context, MakeDefaultRequest(m_Context), "Failed to create BBSpec.");
    }
    BBBinding *binding = spec->Bind();
    spec->Release();
    return binding;
}
