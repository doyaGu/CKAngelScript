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

const std::vector<ScriptBridgeLayoutIoSlot> *IoSlotsForKind(const ScriptBridgeLayoutRecord &layout, ScriptBridgeSlotKind kind) {
    if (kind == ScriptBridgeSlotKind::Input) {
        return &layout.Inputs;
    }
    if (kind == ScriptBridgeSlotKind::Output) {
        return &layout.Outputs;
    }
    return nullptr;
}

const std::vector<ScriptBridgeLayoutParamSlot> *ParamSlotsForKind(const ScriptBridgeLayoutRecord &layout, ScriptBridgeSlotKind kind) {
    if (kind == ScriptBridgeSlotKind::Pin) {
        return &layout.Pins;
    }
    if (kind == ScriptBridgeSlotKind::Pout) {
        return &layout.Pouts;
    }
    if (kind == ScriptBridgeSlotKind::Setting) {
        return &layout.Settings;
    }
    if (kind == ScriptBridgeSlotKind::Local) {
        return &layout.Locals;
    }
    return nullptr;
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
        const std::vector<ScriptBridgeLayoutParamSlot> *slots = ParamSlotsForKind(layout, kind);
        if (slots) {
            for (const ScriptBridgeLayoutParamSlot &slot : *slots) append(slot.Index, slot.Name);
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

const ScriptBridgeLayoutParamSlot *FindParamSlotByIndex(const ScriptBridgeLayoutRecord &layout,
                                                        ScriptBridgeSlotKind kind,
                                                        int index) {
    const std::vector<ScriptBridgeLayoutParamSlot> *slots = ParamSlotsForKind(layout, kind);
    if (!slots || index < 0) {
        return nullptr;
    }
    for (const ScriptBridgeLayoutParamSlot &slot : *slots) {
        if (slot.Index == index) {
            return &slot;
        }
    }
    return nullptr;
}

const ScriptBridgeLayoutParamSlot *FindParamSlot(const ScriptBridgeLayoutRecord &layout,
                                                 ScriptBridgeSlotKind kind,
                                                 const std::string &name,
                                                 int occurrence) {
    const std::vector<ScriptBridgeLayoutParamSlot> *slots = ParamSlotsForKind(layout, kind);
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

std::string StaleSlotError(const char *method,
                           BBSlot *slot,
                           ScriptBridgeSlotKind expected,
                           CKBehavior *behavior,
                           const ScriptBridgeLayoutRecord *layout,
                           const std::string &reason) {
    return fmt::format("{} rejected stale {} slot #{} '{}' for Building Block '{}': {}. Slot generation {}, current runtime generation {}.",
                       method ? method : "BBInstance",
                       SlotKindName(expected),
                       slot ? slot->Index() : -1,
                       slot ? slot->Name() : std::string(),
                       SafeString(behavior ? behavior->GetPrototypeName() : nullptr),
                       reason,
                       slot ? slot->LayoutGeneration() : 0,
                       layout ? layout->LayoutGeneration : 0);
}

bool ResolveRuntimeSlot(ScriptBehaviorBridge *bridge,
                        CKBehavior *behavior,
                        BBSlot *slot,
                        ScriptBridgeSlotKind expected,
                        const char *method,
                        int &index,
                        std::string &error) {
    if (!slot || !slot->ResolveIndex(expected, index, error)) {
        if (error.empty()) {
            error = fmt::format("{} requires a {} BBSlot.", method ? method : "BBInstance", SlotKindName(expected));
        }
        return false;
    }
    if (!bridge || !behavior) {
        error = fmt::format("{} requires a live BBInstance behavior.", method ? method : "BBInstance");
        return false;
    }

    const ScriptBridgeLayoutRecord *layout = bridge->GetBehaviorLayout(behavior->GetID(), CaptureBridgeObjectStamp(behavior));
    if (!layout) {
        error = fmt::format("{} could not read runtime layout for Building Block '{}'.",
                            method ? method : "BBInstance",
                            SafeString(behavior->GetPrototypeName()));
        return false;
    }

    const std::vector<ScriptBridgeLayoutIoSlot> *ioSlots = IoSlotsForKind(*layout, expected);
    if (ioSlots) {
        if (index < 0 || index >= static_cast<int>(ioSlots->size())) {
            error = StaleSlotError(method, slot, expected, behavior, layout, "runtime slot index is no longer present");
            return false;
        }
        const std::string &runtimeName = (*ioSlots)[index].Name;
        if (runtimeName != slot->Name()) {
            error = StaleSlotError(method,
                                   slot,
                                   expected,
                                   behavior,
                                   layout,
                                   fmt::format("runtime name is now '{}'", runtimeName));
            return false;
        }
        return true;
    }

    const ScriptBridgeLayoutParamSlot *runtimeSlot = FindParamSlotByIndex(*layout, expected, index);
    if (!runtimeSlot) {
        error = StaleSlotError(method, slot, expected, behavior, layout, "runtime slot index is no longer present");
        return false;
    }
    if (runtimeSlot->Name != slot->Name()) {
        error = StaleSlotError(method,
                               slot,
                               expected,
                               behavior,
                               layout,
                               fmt::format("runtime name is now '{}'", runtimeSlot->Name));
        return false;
    }
    if (runtimeSlot->TypeGuid != slot->TypeGuid()) {
        error = StaleSlotError(method,
                               slot,
                               expected,
                               behavior,
                               layout,
                               fmt::format("runtime type is now {}", runtimeSlot->TypeName));
        return false;
    }
    return true;
}

bool NameEqualsAny(const std::string &name, std::initializer_list<const char *> candidates) {
    for (const char *candidate : candidates) {
        if (candidate && name == candidate) {
            return true;
        }
    }
    return false;
}

std::string SelectDefaultInputName(const ScriptBridgeLayoutRecord &layout) {
    for (const ScriptBridgeLayoutIoSlot &slot : layout.Inputs) {
        if (NameEqualsAny(slot.Name, {"In", "On", "Start", "Enable"})) {
            return slot.Name;
        }
    }
    return layout.Inputs.size() == 1 ? layout.Inputs.front().Name : std::string();
}

std::string SelectDefaultStopInputName(const ScriptBridgeLayoutRecord &layout) {
    for (const ScriptBridgeLayoutIoSlot &slot : layout.Inputs) {
        if (NameEqualsAny(slot.Name, {"Off", "Stop", "Disable"})) {
            return slot.Name;
        }
    }
    return std::string();
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
        return fmt::format("BBSlot '{}' layout changed; bind the slot again. Slot generation {}, current generation {}.",
                           m_Name,
                           m_LayoutGeneration,
                           layout->LayoutGeneration);
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
bool BBSlot::IsRequired() const { return HasScriptBridgeSlotMetadataFlag(m_MetadataFlags, ScriptBridgeSlotMetadataFlags::Required); }
bool BBSlot::IsStart() const { return HasScriptBridgeSlotMetadataFlag(m_MetadataFlags, ScriptBridgeSlotMetadataFlags::Start); }
bool BBSlot::IsStop() const { return HasScriptBridgeSlotMetadataFlag(m_MetadataFlags, ScriptBridgeSlotMetadataFlags::Stop); }
bool BBSlot::HasDefault() const { return HasScriptBridgeSlotMetadataFlag(m_MetadataFlags, ScriptBridgeSlotMetadataFlags::HasDefault); }
std::string BBSlot::DefaultText() const { return m_DefaultText; }
bool BBSlot::HasValue() const { return HasScriptBridgeSlotMetadataFlag(m_MetadataFlags, ScriptBridgeSlotMetadataFlags::HasValue); }
std::string BBSlot::ValueText() const { return m_ValueText; }

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

void BBSlot::SetMetadata(CKDWORD flags, const std::string &defaultText, const std::string &valueText) {
    m_MetadataFlags = flags;
    m_DefaultText = defaultText;
    m_ValueText = valueText;
}

const ScriptBridgeLayoutRecord *BBSlot::LayoutRecord() const {
    return m_Bridge ? m_Bridge->GetPrototypeLayout(m_Context, m_Request) : nullptr;
}

std::string BBSlot::KindName() const {
    return ScriptBridgeBBInternal::SlotKindName(m_Kind);
}

BBDecl::BBDecl(ScriptBehaviorBridge *bridge,
               const CKBehaviorContext &ctx,
               const ScriptBridgeBBInvocationSpec &request,
               const std::string &error)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request), m_Error(error) {}

bool BBDecl::IsValid() const {
    std::string error;
    return m_Error.empty() && PrototypeObject(error) != nullptr;
}

std::string BBDecl::Error() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    std::string error;
    PrototypeObject(error);
    return error;
}

CKGUID BBDecl::GetGuid() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    return prototype ? prototype->GetGuid() : CKGUID();
}

std::string BBDecl::GetName() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    return prototype ? SafeString(prototype->GetName()) : std::string();
}

std::string BBDecl::GetCategory() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    CKObjectDeclaration *decl = prototype ? prototype->GetSoureObjectDeclaration() : nullptr;
    return decl ? SafeString(decl->GetCategory()) : std::string();
}

std::string BBDecl::GetQualifiedName() const {
    const std::string category = GetCategory();
    const std::string name = GetName();
    return category.empty() ? name : category + "/" + name;
}

CKGUID BBDecl::Guid() const { return GetGuid(); }
std::string BBDecl::Name() const { return GetName(); }
std::string BBDecl::Category() const { return GetCategory(); }
std::string BBDecl::QualifiedName() const { return GetQualifiedName(); }

CKDWORD BBDecl::BehaviorFlags() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    return prototype ? static_cast<CKDWORD>(prototype->GetBehaviorFlags()) : 0;
}

CKDWORD BBDecl::PrototypeFlags() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    return prototype ? static_cast<CKDWORD>(prototype->GetFlags()) : 0;
}

CK_CLASSID BBDecl::CompatibleClassId() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    if (!prototype) {
        return 0;
    }
    CKObjectDeclaration *decl = prototype->GetSoureObjectDeclaration();
    return decl ? decl->GetCompatibleClassId() : prototype->GetApplyToClassID();
}

int BBDecl::NeededManagerCount() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    CKObjectDeclaration *decl = prototype ? prototype->GetSoureObjectDeclaration() : nullptr;
    return decl ? decl->GetManagerNeededCount() : 0;
}

CKGUID BBDecl::NeededManagerGuid(int index) const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    CKObjectDeclaration *decl = prototype ? prototype->GetSoureObjectDeclaration() : nullptr;
    return decl && index >= 0 && index < decl->GetManagerNeededCount() ? decl->GetManagerNeeded(index) : CKGUID();
}

BBPrototype *BBDecl::Prototype() const {
    return IsValid() ? new BBPrototype(m_Bridge, m_Context, m_Request) : nullptr;
}

BehaviorLayout *BBDecl::Layout() const {
    return IsValid() ? new BehaviorLayout(m_Bridge, m_Context, m_Request) : nullptr;
}

BBCallBuilder *BBDecl::Call() {
    return IsValid() ? new BBCallBuilder(m_Bridge, m_Context, m_Request) : nullptr;
}

BBTaskBuilder *BBDecl::Spawn() {
    return IsValid() ? new BBTaskBuilder(m_Bridge, m_Context, m_Request) : nullptr;
}

BBConfig *BBDecl::Configure() {
    return new BBConfig(m_Bridge, m_Context, m_Request, Error());
}

BBSlot *BBDecl::In(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Input, name, occurrence); }
BBSlot *BBDecl::Input(const std::string &name, int occurrence) const { return In(name, occurrence); }
BBSlot *BBDecl::Out(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Output, name, occurrence); }
BBSlot *BBDecl::Output(const std::string &name, int occurrence) const { return Out(name, occurrence); }
BBSlot *BBDecl::Pin(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Pin, name, occurrence); }
BBSlot *BBDecl::Pout(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Pout, name, occurrence); }
BBSlot *BBDecl::Setting(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Setting, name, occurrence); }
BBSlot *BBDecl::Local(const std::string &name, int occurrence) const { return ResolveSlot(ScriptBridgeSlotKind::Local, name, occurrence); }

std::string BBDecl::Describe() const {
    if (!IsValid()) {
        return "Invalid BBDecl: " + Error();
    }
    BehaviorLayout *layout = Layout();
    std::string result = fmt::format("BBDecl '{}' guid={}", GetQualifiedName(), GuidToString(GetGuid()));
    if (layout) {
        result += "\n" + layout->Describe();
        layout->Release();
    }
    return result;
}

CKBehaviorPrototype *BBDecl::PrototypeObject(std::string &error) const {
    return m_Bridge ? m_Bridge->ResolvePrototypeObject(m_Request, error) : nullptr;
}

BBSlot *BBDecl::ResolveSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const {
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

BBConfig::BBConfig(ScriptBehaviorBridge *bridge,
                     const CKBehaviorContext &ctx,
                     const ScriptBridgeBBInvocationSpec &request,
                     const std::string &error)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request), m_Error(error) {}

BBConfig::~BBConfig() {
    Destroy();
    for (BBSlot *slot : m_RegisteredSlots) {
        if (slot) {
            slot->Release();
        }
    }
    m_RegisteredSlots.clear();
    for (CachedSlot &slot : m_Slots) {
        if (slot.Slot) {
            slot.Slot->Release();
            slot.Slot = nullptr;
        }
    }
}

bool BBConfig::IsValid() const {
    if (!m_Error.empty() || !m_Bridge) {
        return false;
    }
    BBDecl spec(m_Bridge, m_Context, m_Request);
    return spec.IsValid();
}

std::string BBConfig::Error() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    if (!m_Bridge) {
        return "BBConfig bridge is not available.";
    }
    BBDecl spec(m_Bridge, m_Context, m_Request);
    return spec.Error();
}

std::string BBConfig::Describe() const {
    BBDecl spec(m_Bridge, m_Context, m_Request, m_Error);
    std::string text = spec.Describe();
    if (!m_DefaultStartInput.empty()) {
        text += fmt::format("\nDefault start input: '{}'", m_DefaultStartInput);
    }
    if (!m_DefaultStopInput.empty()) {
        text += fmt::format("\nDefault stop input: '{}'", m_DefaultStopInput);
    }
    return text;
}

BBDecl *BBConfig::Spec() const {
    return new BBDecl(m_Bridge, m_Context, m_Request, m_Error);
}

BehaviorRef *BBConfig::Behavior() const {
    if (m_Instance && m_Instance->IsValid()) {
        return m_Instance->Behavior();
    }
    return nullptr;
}

bool BBConfig::Raise(const CKBehaviorContext &ctx) const {
    if (IsValid()) {
        return true;
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = Error();
    return RaiseExecutionState(state, ctx);
}

BBSlot *BBConfig::In(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Input, name, occurrence); }
BBSlot *BBConfig::Out(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Output, name, occurrence); }
BBSlot *BBConfig::Pin(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Pin, name, occurrence); }
BBSlot *BBConfig::Pout(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Pout, name, occurrence); }
BBSlot *BBConfig::Setting(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Setting, name, occurrence); }
BBSlot *BBConfig::Local(const std::string &name, int occurrence) { return Slot(ScriptBridgeSlotKind::Local, name, occurrence); }

bool BBConfig::RequireSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) {
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

BBConfig *BBConfig::Owner(CKBeObject *owner) {
    m_Request.OwnerId = owner ? owner->GetID() : 0;
    AddRef();
    return this;
}

BBConfig *BBConfig::Target(CKBeObject *target) {
    m_Request.TargetId = target ? target->GetID() : 0;
    AddRef();
    return this;
}

BBConfig *BBConfig::SetSlot(BBSlot *pin, ParamValue *value) {
    if (!value) {
        SetError("BBConfig.Set requires a valid ParamValue.");
        SetScriptException(m_Error);
        return nullptr;
    }
    return SetValueForPin(pin, value->Value(), "BBConfig.Set") ? (AddRef(), this) : nullptr;
}

BBConfig *BBConfig::SetSlotInt(BBSlot *pin, int value) {
    return SetValueForPin(pin, MakeScriptParamInt(value), "BBConfig.Set") ? (AddRef(), this) : nullptr;
}

BBConfig *BBConfig::SetSlotFloat(BBSlot *pin, float value) {
    return SetValueForPin(pin, MakeScriptParamFloat(value), "BBConfig.Set") ? (AddRef(), this) : nullptr;
}

BBConfig *BBConfig::SetSlotBool(BBSlot *pin, bool value) {
    return SetValueForPin(pin, MakeScriptParamBool(value), "BBConfig.Set") ? (AddRef(), this) : nullptr;
}

BBConfig *BBConfig::SetSlotString(BBSlot *pin, const std::string &value) {
    return SetValueForPin(pin, MakeScriptParamString(value), "BBConfig.Set") ? (AddRef(), this) : nullptr;
}

BBConfig *BBConfig::SetSlotObject(BBSlot *pin, CKObject *value) {
    return SetValueForPin(pin, MakeScriptParamObject(value), "BBConfig.Set") ? (AddRef(), this) : nullptr;
}

BBConfig *BBConfig::SourceSlot(BBSlot *pin, ParamRef *source) {
    return SourceForPin(pin, source, "BBConfig.Source") ? (AddRef(), this) : nullptr;
}

BBConfig *BBConfig::OperationSlot(BBSlot *pin, ParamOp *operation) {
    return OperationForPin(pin, operation, "BBConfig.Operation") ? (AddRef(), this) : nullptr;
}

BBConfig *BBConfig::SetSetting(BBSlot *setting, ParamValue *value) {
    if (!value) {
        SetError("BBConfig.SetSetting requires a valid ParamValue.");
        SetScriptException(m_Error);
        return nullptr;
    }
    return SetValueForSetting(setting, value->Value(), "BBConfig.SetSetting") ? (AddRef(), this) : nullptr;
}

BBConfig *BBConfig::SetSettingString(BBSlot *setting, const std::string &value) {
    return SetValueForSetting(setting, MakeScriptParamString(value), "BBConfig.SetSetting") ? (AddRef(), this) : nullptr;
}

bool BBConfig::Validate(const CKBehaviorContext &ctx) const {
    if (!IsValid()) {
        SetScriptException(Error());
        return false;
    }

    CKContext *context = ctx.Context ? ctx.Context : (m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr);
    if (!context) {
        SetError("BBConfig.Validate requires CKContext.");
        SetScriptException(m_Error);
        return false;
    }

    const ScriptBridgeLayoutRecord *layout = m_Bridge ? m_Bridge->GetPrototypeLayout(ctx, m_Request) : nullptr;
    if (!layout) {
        SetError("BBConfig.Validate could not resolve prototype layout.");
        SetScriptException(m_Error);
        return false;
    }

    for (const CKGUID &managerGuid : layout->NeededManagers) {
        if (!context->GetManagerByGuid(managerGuid)) {
            SetError(fmt::format("Building Block '{}' requires manager {} which is not available.",
                                 layout->QualifiedName,
                                 GuidToString(managerGuid)));
            SetScriptException(m_Error);
            return false;
        }
    }

    CKObject *ownerObject = m_Request.OwnerId ? GetCKObjectById(context, m_Request.OwnerId) : nullptr;
    CKObject *targetObject = m_Request.TargetId ? GetCKObjectById(context, m_Request.TargetId) : nullptr;
    if (targetObject) {
        if (!CKIsChildClassOf(targetObject, layout->CompatibleClassId)) {
            SetError(fmt::format("Target '{}' is not compatible with Building Block '{}' expected class {}.",
                                 SafeString(targetObject->GetName()),
                                 layout->QualifiedName,
                                 layout->CompatibleClassId));
            SetScriptException(m_Error);
            return false;
        }
    } else if (ownerObject) {
        if (!CKIsChildClassOf(ownerObject, layout->CompatibleClassId)) {
            SetError(fmt::format("Owner '{}' is not compatible with Building Block '{}' expected class {}.",
                                 SafeString(ownerObject->GetName()),
                                 layout->QualifiedName,
                                 layout->CompatibleClassId));
            SetScriptException(m_Error);
            return false;
        }
    } else if (layout->CompatibleClassId != CKCID_OBJECT && layout->CompatibleClassId != CKCID_BEOBJECT) {
        SetError(fmt::format("Building Block '{}' requires an owner or target compatible with class {}.",
                             layout->QualifiedName,
                             layout->CompatibleClassId));
        SetScriptException(m_Error);
        return false;
    }

    auto findSlot = [](const std::vector<ScriptBridgeLayoutParamSlot> &slots, int index) -> const ScriptBridgeLayoutParamSlot * {
        for (const ScriptBridgeLayoutParamSlot &slot : slots) {
            if (slot.Index == index) {
                return &slot;
            }
        }
        return nullptr;
    };

    auto validateValue = [&](const char *label,
                             const std::vector<ScriptBridgeLayoutParamSlot> &slots,
                             const ScriptBridgeIndexedValue &entry) -> bool {
        const ScriptBridgeLayoutParamSlot *slot = findSlot(slots, entry.PinIndex);
        if (!slot) {
            SetError(fmt::format("BBConfig.Validate has invalid {} index {} for '{}'.",
                                 label,
                                 entry.PinIndex,
                                 layout->QualifiedName));
            return false;
        }
        CKParameterLocal *probe = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_ValidateParam"), slot->TypeGuid, TRUE);
        if (!probe) {
            SetError(fmt::format("BBConfig.Validate failed to create validation parameter for {} #{} '{}'.",
                                 label,
                                 slot->Index,
                                 slot->Name));
            return false;
        }
        std::string writeError;
        const CKERROR err = WriteParameterValue(probe, entry.Value, writeError);
        context->DestroyObject(probe);
        if (err != CK_OK) {
            SetError(fmt::format("BBConfig.Validate failed for {} #{} '{}' expected {}: {}",
                                 label,
                                 slot->Index,
                                 slot->Name,
                                 slot->TypeName,
                                 writeError.empty() ? fmt::format("CKERROR {}", err) : writeError));
            return false;
        }
        return true;
    };

    for (const ScriptBridgeIndexedValue &entry : m_Request.IndexedSettings) {
        if (!validateValue("setting", layout->Settings, entry)) {
            SetScriptException(m_Error);
            return false;
        }
    }

    for (const ScriptBridgeIndexedValue &entry : m_Request.IndexedParameters) {
        if (!validateValue("pin", layout->Pins, entry)) {
            SetScriptException(m_Error);
            return false;
        }
    }

    CKParameterManager *pm = context->GetParameterManager();
    for (const ScriptBridgeInputSource &source : m_Request.SourceParameters) {
        const ScriptBridgeLayoutParamSlot *slot = findSlot(layout->Pins, source.PinIndex);
        if (!slot) {
            SetError(fmt::format("BBConfig.Validate has invalid source pin index {} for '{}'.",
                                 source.PinIndex,
                                 layout->QualifiedName));
            SetScriptException(m_Error);
            return false;
        }
        CKParameter *sourceParam = ResolveStampedParameterSource(context, source.SourceId, source.SourceStamp, m_Error);
        if (!sourceParam) {
            if (m_Error.empty()) {
                SetError(fmt::format("BBConfig.Validate source for pin #{} '{}' is not valid.", slot->Index, slot->Name));
            }
            SetScriptException(m_Error);
            return false;
        }
        if (pm && pm->IsTypeCompatible(slot->TypeGuid, sourceParam->GetGUID()) == FALSE &&
            pm->IsTypeCompatible(sourceParam->GetGUID(), slot->TypeGuid) == FALSE) {
            SetError(fmt::format("BBConfig.Validate source type mismatch for pin #{} '{}' expected {}, got {}.",
                                 slot->Index,
                                 slot->Name,
                                 slot->TypeName,
                                 ParameterTypeLabel(context, sourceParam)));
            SetScriptException(m_Error);
            return false;
        }
    }

    for (const ScriptBridgeOperationSpec &operation : m_Request.OperationParameters) {
        const ScriptBridgeLayoutParamSlot *slot = findSlot(layout->Pins, operation.TargetPinIndex);
        if (!slot) {
            SetError(fmt::format("BBConfig.Validate has invalid operation target pin index {} for '{}'.",
                                 operation.TargetPinIndex,
                                 layout->QualifiedName));
            SetScriptException(m_Error);
            return false;
        }
        std::string operationError;
        if (!ValidateOperationSpec(context, slot->TypeGuid, operation, operationError)) {
            SetError(fmt::format("BBConfig.Validate operation for pin #{} '{}' failed: {}",
                                 slot->Index,
                                 slot->Name,
                                 operationError));
            SetScriptException(m_Error);
            return false;
        }
    }

    return true;
}

BBDecl *BBConfig::Decl() const {
    return new BBDecl(m_Bridge, m_Context, m_Request, m_Error);
}

BBInstance *BBConfig::SpawnInstance(const CKBehaviorContext &ctx) {
    if (!m_Bridge) {
        SetError("BBConfig.Spawn requires ScriptBehaviorBridge.");
        SetScriptException(m_Error);
        return nullptr;
    }
    if (!Validate(ctx)) {
        return nullptr;
    }
    int generation = 0;
    std::string error;
    const CK_ID instanceId = m_Bridge->CreateInstance(m_Request, ctx, generation, error);
    if (!instanceId) {
        SetError(error.empty() ? "BBConfig.Spawn failed to create runtime behavior." : error);
        SetScriptException(m_Error);
        return nullptr;
    }
    if (m_Instance) {
        m_Instance->Destroy();
        m_Instance->Release();
        m_Instance = nullptr;
    }
    BBInstance *instance = new BBInstance(m_Bridge, ctx, m_Request, instanceId, generation, m_DefaultStartInput, m_DefaultStopInput);
    m_Instance = instance;
    m_Instance->AddRef();
    return instance;
}

BBInstance *BBConfig::SpawnStarted(const CKBehaviorContext &ctx) {
    BBInstance *instance = SpawnInstance(ctx);
    if (!instance) {
        return nullptr;
    }
    if (!instance->Start()) {
        SetError(instance->Error());
        SetScriptException(m_Error);
        instance->Release();
        return nullptr;
    }
    return instance;
}

bool BBConfig::Stop(const CKBehaviorContext &ctx) {
    (void) ctx;
    if (!m_Instance) {
        return true;
    }
    const bool result = m_Instance->Stop();
    if (!result) {
        SetError(m_Instance->Error());
        SetScriptException(m_Error);
    }
    return result;
}

bool BBConfig::Destroy() {
    if (m_Instance) {
        m_Instance->Destroy();
        m_Instance->Release();
        m_Instance = nullptr;
    }
    return true;
}

bool BBConfig::OutputActiveSlot(BBSlot *output) {
    if (m_Instance && m_Instance->IsValid()) {
        return m_Instance->OutputActive(output);
    }
    SetError("BBConfig.OutputActive requires a live BBInstance. Call Spawn() or SpawnStarted() first.");
    SetScriptException(m_Error);
    return false;
}

ParamRef *BBConfig::PinRefSlot(BBSlot *pin) {
    if (m_Instance && m_Instance->IsValid()) {
        return m_Instance->Pin(pin);
    }
    SetError("BBConfig.PinRef requires a live BBInstance. Call Spawn() or SpawnStarted() first.");
    SetScriptException(m_Error);
    return nullptr;
}

ParamRef *BBConfig::PoutRefSlot(BBSlot *pout) {
    if (m_Instance && m_Instance->IsValid()) {
        return m_Instance->Pout(pout);
    }
    SetError("BBConfig.PoutRef requires a live BBInstance. Call Spawn() or SpawnStarted() first.");
    SetScriptException(m_Error);
    return nullptr;
}

void BBConfig::SetDefaultStart(const std::string &inputName) {
    m_DefaultStartInput = inputName;
}

void BBConfig::SetDefaultStop(const std::string &inputName) {
    m_DefaultStopInput = inputName;
}

void BBConfig::SetManaged(bool managed) {
    m_Managed = managed;
}

bool BBConfig::IsManaged() const {
    return m_Managed;
}

bool BBConfig::RegisterSlot(BBSlot *slot) {
    if (!slot || !slot->IsValid()) {
        SetError(slot ? slot->Error() : "BBConfig.RegisterSlot requires a valid BBSlot.");
        return false;
    }
    for (BBSlot *registered : m_RegisteredSlots) {
        if (registered == slot) {
            return true;
        }
    }
    slot->AddRef();
    m_RegisteredSlots.push_back(slot);
    return ApplySlotMetadata(slot);
}

BBSlot *BBConfig::Slot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) {
    if (BBSlot *cached = FindCachedSlot(kind, name, occurrence)) {
        cached->AddRef();
        return cached;
    }

    BBDecl spec(m_Bridge, m_Context, m_Request, m_Error);
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

BBSlot *BBConfig::FindCachedSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const {
    for (const CachedSlot &slot : m_Slots) {
        if (slot.Kind == kind && slot.Name == name && slot.Occurrence == occurrence) {
            return slot.Slot;
        }
    }
    return nullptr;
}

BBSlot *BBConfig::DefaultInputSlot() {
    if (!m_DefaultStartInput.empty()) {
        return In(m_DefaultStartInput);
    }
    const ScriptBridgeLayoutRecord *layout = m_Bridge ? m_Bridge->GetPrototypeLayout(m_Context, m_Request) : nullptr;
    const std::string inferred = layout ? ScriptBridgeBBInternal::SelectDefaultInputName(*layout) : std::string();
    return inferred.empty() ? nullptr : In(inferred);
}

BBSlot *BBConfig::DefaultStopSlot() {
    if (!m_DefaultStopInput.empty()) {
        return In(m_DefaultStopInput);
    }
    const ScriptBridgeLayoutRecord *layout = m_Bridge ? m_Bridge->GetPrototypeLayout(m_Context, m_Request) : nullptr;
    const std::string inferred = layout ? ScriptBridgeBBInternal::SelectDefaultStopInputName(*layout) : std::string();
    return inferred.empty() ? nullptr : In(inferred);
}

bool BBConfig::ResolvePin(BBSlot *slot, int &pinIndex, const char *method) {
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pin, pinIndex, error)) {
        SetError(fmt::format("{} requires a pin BBSlot.{}", method, error.empty() ? "" : " " + error));
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

bool BBConfig::SetValueForPin(BBSlot *slot, const ScriptParamValue &value, const char *method) {
    int pinIndex = -1;
    if (!ResolvePin(slot, pinIndex, method)) {
        return false;
    }
    ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value);
    if (m_Instance && m_Instance->IsValid()) {
        ParamRef *ref = m_Instance->Pin(slot);
        if (!ref) {
            SetError(fmt::format("{} could not resolve live instance pin.", method));
            SetScriptException(m_Error);
            return false;
        }
        ParamValue paramValue(value);
        const bool ok = ref->Set(&paramValue);
        ref->Release();
        if (!ok) {
            SetError(fmt::format("{} failed to write live instance pin '{}'.", method, slot ? slot->Name() : std::string("<null>")));
            SetScriptException(m_Error);
        }
        return ok;
    }
    return true;
}

bool BBConfig::SetValueForSetting(BBSlot *slot, const ScriptParamValue &value, const char *method) {
    int settingIndex = -1;
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Setting, settingIndex, error)) {
        SetError(fmt::format("{} requires a setting BBSlot.{}", method, error.empty() ? "" : " " + error));
        SetScriptException(m_Error);
        return false;
    }
    ScriptBridgeSetIndexedValue(m_Request.IndexedSettings, settingIndex, value);
    if (m_Instance && m_Instance->IsValid()) {
        ParamValue paramValue(value);
        if (!m_Instance->SetSettingValue(slot, &paramValue)) {
            SetError(m_Instance->Error());
            SetScriptException(m_Error);
            return false;
        }
        return true;
    }
    return true;
}

bool BBConfig::ApplySlotMetadata(BBSlot *slot) {
    if (!slot) {
        return true;
    }
    if ((slot->IsStart() || slot->IsStop()) && slot->Kind() != static_cast<int>(ScriptBridgeSlotKind::Input)) {
        SetError(fmt::format("BBSlot '{}' marks start/stop but is not an input slot.", slot->Name()));
        SetScriptException(m_Error);
        return false;
    }
    if (slot->IsStart()) {
        SetDefaultStart(slot->Name());
    }
    if (slot->IsStop()) {
        SetDefaultStop(slot->Name());
    }
    if (slot->HasDefault() && slot->Kind() == static_cast<int>(ScriptBridgeSlotKind::Pin)) {
        return SetValueForPin(slot, MakeScriptParamString(slot->DefaultText()), "BBConfig.RegisterSlot");
    }
    if (slot->HasValue() && slot->Kind() == static_cast<int>(ScriptBridgeSlotKind::Setting)) {
        return SetValueForSetting(slot, MakeScriptParamString(slot->ValueText()), "BBConfig.RegisterSlot");
    }
    if (slot->HasDefault() && slot->Kind() == static_cast<int>(ScriptBridgeSlotKind::Setting)) {
        return SetValueForSetting(slot, MakeScriptParamString(slot->DefaultText()), "BBConfig.RegisterSlot");
    }
    return true;
}

bool BBConfig::SetLiveValue(BBSlot *slot, const ScriptParamValue &value, const char *method) {
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

bool BBConfig::SourceForPin(BBSlot *slot, ParamRef *source, const char *method) {
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
    ReplacePendingSource(request);

    if (!m_Instance || !m_Instance->IsValid()) {
        return true;
    }

    m_Bridge->RemoveInstanceSourceLink(m_Instance->BridgeInstanceId(), m_Instance->BridgeGeneration(), pinIndex);
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
    if (!m_Bridge->StoreInstanceSourceLink(m_Instance->BridgeInstanceId(), m_Instance->BridgeGeneration(), pinIndex, link)) {
        link->Restore();
        link->Release();
        SetError(fmt::format("{} failed to store live source link.", method));
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

bool BBConfig::OperationForPin(BBSlot *slot, ParamOp *operation, const char *method) {
    int pinIndex = -1;
    if (!ResolvePin(slot, pinIndex, method)) {
        return false;
    }
    if (!operation) {
        SetError(fmt::format("{} requires a valid ParamOp.", method));
        SetScriptException(m_Error);
        return false;
    }

    ReplacePendingOperation(operation->RequestForPin(pinIndex));
    if (!m_Instance || !m_Instance->IsValid()) {
        return true;
    }

    m_Bridge->RemoveInstanceOperation(m_Instance->BridgeInstanceId(), m_Instance->BridgeGeneration(), pinIndex);
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
    if (!m_Bridge->StoreInstanceOperation(m_Instance->BridgeInstanceId(), m_Instance->BridgeGeneration(), pinIndex, op)) {
        op->Destroy();
        op->Release();
        SetError(fmt::format("{} failed to store live operation.", method));
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

void BBConfig::ReplacePendingSource(const ScriptBridgeInputSource &source) {
    const auto it = std::lower_bound(m_Request.SourceParameters.begin(),
                                     m_Request.SourceParameters.end(),
                                     source.PinIndex,
                                     [](const ScriptBridgeInputSource &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it != m_Request.SourceParameters.end() && it->PinIndex == source.PinIndex) {
        *it = source;
    } else {
        m_Request.SourceParameters.insert(it, source);
    }
}

void BBConfig::ReplacePendingOperation(const ScriptBridgeOperationSpec &operation) {
    const auto it = std::lower_bound(m_Request.OperationParameters.begin(),
                                     m_Request.OperationParameters.end(),
                                     operation.TargetPinIndex,
                                     [](const ScriptBridgeOperationSpec &entry, int index) {
                                         return entry.TargetPinIndex < index;
                                     });
    if (it != m_Request.OperationParameters.end() && it->TargetPinIndex == operation.TargetPinIndex) {
        *it = operation;
    } else {
        m_Request.OperationParameters.insert(it, operation);
    }
}

void BBConfig::SetError(const std::string &error) const {
    if (!error.empty()) {
        m_Error = error;
    }
}

BBInstance::BBInstance(ScriptBehaviorBridge *bridge,
                       const CKBehaviorContext &ctx,
                       const ScriptBridgeBBInvocationSpec &request,
                       CK_ID instanceId,
                       int generation,
                       const std::string &defaultStartInput,
                       const std::string &defaultStopInput,
                       const std::string &error)
    : m_Bridge(bridge),
      m_Context(ctx),
      m_Request(request),
      m_Error(error),
      m_InstanceId(instanceId),
      m_Generation(generation),
      m_DefaultStartInput(defaultStartInput),
      m_DefaultStopInput(defaultStopInput) {}

BBInstance::~BBInstance() {
    Destroy();
}

bool BBInstance::IsValid() const {
    return m_Error.empty() && m_Bridge && m_InstanceId != 0 && m_Bridge->IsInstanceValid(m_InstanceId, m_Generation);
}

std::string BBInstance::Error() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    if (!m_Bridge) {
        return "BBInstance bridge is not available.";
    }
    ScriptBridgeExecutionState state = m_Bridge->GetInstanceState(m_InstanceId, m_Generation);
    return state.Error;
}

BBDecl *BBInstance::Decl() const {
    return new BBDecl(m_Bridge, m_Context, m_Request, m_Error);
}

BehaviorRef *BBInstance::Behavior() const {
    return m_Bridge ? m_Bridge->WrapBehavior(m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation)) : nullptr;
}

bool BBInstance::Start() {
    if (m_DefaultStartInput.empty()) {
        return Start(nullptr);
    }
    BBDecl decl(m_Bridge, m_Context, m_Request);
    BBSlot *slot = decl.Input(m_DefaultStartInput);
    const bool result = Start(slot);
    if (slot) {
        slot->Release();
    }
    return result;
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
        CKBehavior *behavior = m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation);
        if (!ScriptBridgeBBInternal::ResolveRuntimeSlot(m_Bridge, behavior, input, ScriptBridgeSlotKind::Input, "BBInstance.Start", inputIndex, error)) {
            SetError(error);
            SetScriptException(m_Error);
            return false;
        }
    }
    if (!m_Bridge->StartInstance(m_InstanceId, m_Generation, m_Context, inputIndex)) {
        SetError(m_Bridge->GetInstanceState(m_InstanceId, m_Generation).Error);
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

bool BBInstance::Step(const CKBehaviorContext &ctx) {
    if (!m_Bridge || !m_Bridge->IsInstanceAlive(m_InstanceId, m_Generation)) {
        SetError("BBInstance.Step called before Start or after Destroy.");
        SetScriptException(m_Error);
        return false;
    }
    if (!m_Bridge->StepInstance(m_InstanceId, m_Generation, ctx)) {
        SetError(m_Bridge->GetInstanceState(m_InstanceId, m_Generation).Error);
        return false;
    }
    return true;
}

bool BBInstance::Stop() {
    if (!m_Bridge || !m_InstanceId) {
        return true;
    }

    int inputIndex = -1;
    if (!m_DefaultStopInput.empty()) {
        BBDecl decl(m_Bridge, m_Context, m_Request);
        BBSlot *slot = decl.Input(m_DefaultStopInput);
        if (slot) {
            std::string error;
            CKBehavior *behavior = m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation);
            if (!ScriptBridgeBBInternal::ResolveRuntimeSlot(m_Bridge, behavior, slot, ScriptBridgeSlotKind::Input, "BBInstance.Stop", inputIndex, error)) {
                SetError(error);
                SetScriptException(m_Error);
                slot->Release();
                return false;
            }
            slot->Release();
        }
    }

    if (!m_Bridge->StopInstance(m_InstanceId, m_Generation, m_Context, inputIndex)) {
        SetError(m_Bridge->GetInstanceState(m_InstanceId, m_Generation).Error);
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

bool BBInstance::OutputActive(BBSlot *output) const {
    int outputIndex = -1;
    std::string error;
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    if (!ScriptBridgeBBInternal::ResolveRuntimeSlot(m_Bridge, behavior, output, ScriptBridgeSlotKind::Output, "BBInstance.OutputActive", outputIndex, error)) {
        SetScriptException(error);
        return false;
    }
    return ExecutionOutputActive(m_Bridge ? m_Bridge->GetInstanceState(m_InstanceId, m_Generation) : ScriptBridgeExecutionState(), outputIndex);
}

ParamRef *BBInstance::Pin(BBSlot *pin) const {
    int index = -1;
    std::string error;
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    if (!ScriptBridgeBBInternal::ResolveRuntimeSlot(m_Bridge, behavior, pin, ScriptBridgeSlotKind::Pin, "BBInstance.Pin", index, error)) {
        SetScriptException(error);
        return nullptr;
    }
    CKParameterIn *param = behavior && index >= 0 && index < behavior->GetInputParameterCount() ? behavior->GetInputParameter(index) : nullptr;
    return m_Bridge && param ? new ParamRef(m_Bridge, param->GetID(), ScriptBridgeSlotKind::Pin, index, behavior->GetID()) : nullptr;
}

ParamRef *BBInstance::Pout(BBSlot *pout) const {
    int index = -1;
    std::string error;
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    if (!ScriptBridgeBBInternal::ResolveRuntimeSlot(m_Bridge, behavior, pout, ScriptBridgeSlotKind::Pout, "BBInstance.Pout", index, error)) {
        SetScriptException(error);
        return nullptr;
    }
    CKParameterOut *param = behavior && index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
    return m_Bridge && param ? new ParamRef(m_Bridge, param->GetID(), ScriptBridgeSlotKind::Pout, index, behavior->GetID()) : nullptr;
}

bool BBInstance::SetSettingValue(BBSlot *setting, ParamValue *value) {
    if (!value) {
        SetError("BBInstance.SetSetting requires a valid ParamValue.");
        SetScriptException(m_Error);
        return false;
    }
    int settingIndex = -1;
    std::string error;
    if (!m_Bridge || !m_Bridge->IsInstanceAlive(m_InstanceId, m_Generation)) {
        if (!setting || !setting->ResolveIndex(ScriptBridgeSlotKind::Setting, settingIndex, error)) {
            SetError(error.empty() ? "BBInstance.SetSetting requires a setting BBSlot." : error);
            SetScriptException(m_Error);
            return false;
        }
        ScriptBridgeSetIndexedValue(m_Request.IndexedSettings, settingIndex, value->Value());
        return true;
    }
    CKBehavior *behavior = m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation);
    if (!ScriptBridgeBBInternal::ResolveRuntimeSlot(m_Bridge, behavior, setting, ScriptBridgeSlotKind::Setting, "BBInstance.SetSetting", settingIndex, error)) {
        SetError(error);
        SetScriptException(m_Error);
        return false;
    }
    ScriptBridgeSetIndexedValue(m_Request.IndexedSettings, settingIndex, value->Value());
    if (!m_Bridge->SetInstanceSetting(m_InstanceId, m_Generation, settingIndex, value->Value(), error)) {
        SetError(error);
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

bool BBInstance::SetSetting(BBSlot *setting, const std::string &value) {
    ParamValue paramValue(MakeScriptParamString(value));
    return SetSettingValue(setting, &paramValue);
}

bool BBInstance::Destroy() {
    if (!m_Bridge || !m_InstanceId) {
        return false;
    }
    const bool result = m_Bridge->DestroyInstance(m_InstanceId, m_Generation);
    m_InstanceId = 0;
    m_Generation = 0;
    return result;
}

CK_ID BBInstance::BridgeInstanceId() const { return m_InstanceId; }

int BBInstance::BridgeGeneration() const { return m_Generation; }

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

BBDecl *BBBridge::Require(const std::string &query) const {
    ScriptBridgeBBInvocationSpec request;
    request.ComponentId = ComponentIdFromContext(m_Context);
    request.PrototypeName = query;

    if (!m_Bridge) {
        return new BBDecl(nullptr, m_Context, request, "AngelScript BB bridge is not available.");
    }
    if (query.empty()) {
        return new BBDecl(m_Bridge, m_Context, request, "BB::Require needs a non-empty prototype name, Category/Name, or GUID.");
    }

    CKGUID parsed;
    if (ParseScriptGuidString(query, parsed)) {
        request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
        request.Guid = parsed;
        if (CKGetPrototypeFromGuid(parsed)) {
            return new BBDecl(m_Bridge, m_Context, request);
        }
        return new BBDecl(m_Bridge, m_Context, request, fmt::format("BB prototype GUID {} was not found.", GuidToString(parsed)));
    }

    const std::vector<CKObjectDeclaration *> matches = ScriptBridgeBBInternal::FindPrototypeDeclarations(query);
    if (matches.size() == 1 && matches[0]) {
        request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
        request.Guid = matches[0]->GetGuid();
        return new BBDecl(m_Bridge, m_Context, request);
    }
    if (matches.size() > 1) {
        return new BBDecl(m_Bridge,
                          m_Context,
                          request,
                          fmt::format("BB prototype '{}' is ambiguous. Candidates: {}.",
                                      query,
                                      ScriptBridgeBBInternal::PrototypeCandidateList(matches)));
    }

    return new BBDecl(m_Bridge,
                      m_Context,
                      request,
                      fmt::format("BB prototype '{}' was not found. Use BB::FindAll(query) to inspect candidates.", query));
}

BBDecl *BBBridge::RequireGuid(CKGUID guid) const {
    ScriptBridgeBBInvocationSpec request;
    request.ComponentId = ComponentIdFromContext(m_Context);
    request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
    request.Guid = guid;

    if (!m_Bridge) {
        return new BBDecl(nullptr, m_Context, request, "AngelScript BB bridge is not available.");
    }
    if (!guid.IsValid() || !CKGetPrototypeFromGuid(guid)) {
        return new BBDecl(m_Bridge, m_Context, request, fmt::format("BB prototype GUID {} was not found.", GuidToString(guid)));
    }
    return new BBDecl(m_Bridge, m_Context, request);
}
