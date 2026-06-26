#include "ScriptBridgeHandles.h"

#include <cstdlib>

#include <fmt/format.h>

#include "add_on/scriptarray/scriptarray.h"
#include "ScriptParameterConversion.h"

namespace {

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
            if (CKObjectDeclaration *decl = ResolvePrototypeDeclaration(prototype)) {
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

bool WriteLivePinValue(CKBehavior *behavior,
                       int pinIndex,
                       const ScriptParamValue &value,
                       const char *method,
                       const std::string &pinName,
                       std::string &error) {
    if (!behavior || pinIndex < 0 || pinIndex >= behavior->GetInputParameterCount()) {
        error = fmt::format("{} could not resolve live pin '{}'.", method, pinName);
        return false;
    }

    CKParameterIn *input = behavior->GetInputParameter(pinIndex);
    CKParameter *previousSource = input ? input->GetDirectSource() : nullptr;
    const std::string bridgeSourceName = fmt::format("__CKAS_BridgeInput_{}", pinIndex);
    CKParameterLocal *existingBridgeSource = FindLocalParameterByName(behavior, bridgeSourceName);
    const CK_ID existingBridgeSourceId = existingBridgeSource ? existingBridgeSource->GetID() : 0;

    std::string writeError;
    if (SetInputParameterValueByIndex(behavior, pinIndex, value, writeError, nullptr)) {
        return true;
    }

    CKParameter *currentSource = input ? input->GetDirectSource() : nullptr;
    if (input && currentSource != previousSource) {
        const CKERROR restoreError = input->SetDirectSource(previousSource);
        if (restoreError != CK_OK) {
            writeError = fmt::format("{} Failed to restore previous source (CKERROR {}).",
                                     writeError.empty() ? std::string("Conversion failed.") : writeError,
                                     restoreError);
        }
    }

    CKParameterLocal *createdBridgeSource = FindLocalParameterByName(behavior, bridgeSourceName);
    if (createdBridgeSource && createdBridgeSource->GetID() != existingBridgeSourceId) {
        const int position = behavior->GetLocalParameterPosition(createdBridgeSource);
        if (position >= 0) {
            behavior->RemoveLocalParameter(position);
        }
        if (CKContext *context = behavior->GetCKContext()) {
            if (!createdBridgeSource->IsToBeDeleted()) {
                context->DestroyObject(createdBridgeSource);
            }
        }
    }

    error = fmt::format("{} failed to write live pin '{}'.{}{}",
                        method,
                        pinName,
                        writeError.empty() ? "" : " ",
                        writeError);
    return false;
}

class RuntimePinMutation {
public:
    RuntimePinMutation(ScriptBehaviorBridge *bridge, CK_ID instanceId, int generation, int pinIndex, const char *method)
        : m_Bridge(bridge),
          m_InstanceId(instanceId),
          m_Generation(generation),
          m_PinIndex(pinIndex),
          m_Method(method ? method : "BB runtime pin mutation") {
        if (m_Bridge) {
            m_OldSource = m_Bridge->TakeInstanceSourceLink(m_InstanceId, m_Generation, m_PinIndex);
            m_OldOperation = m_Bridge->TakeInstanceOperation(m_InstanceId, m_Generation, m_PinIndex);
        }
    }

    ~RuntimePinMutation() {
        if (!m_Finished) {
            CleanupDetached(m_OldSource);
            CleanupDetached(m_OldOperation);
        }
    }

    void ClearExisting() {
        CleanupDetached(m_OldSource);
        CleanupDetached(m_OldOperation);
        m_Finished = true;
    }

    bool StoreSource(ParamSourceLinkRef *link, std::string &error) {
        if (!m_Bridge || !link || !m_Bridge->StoreInstanceSourceLink(m_InstanceId, m_Generation, m_PinIndex, link)) {
            RollbackNew(link);
            RestorePrevious(error);
            error = WithRestoreError("failed to store live source link", error);
            m_Finished = true;
            return false;
        }
        CleanupDetached(m_OldSource);
        CleanupDetached(m_OldOperation);
        m_Finished = true;
        return true;
    }

    bool StoreOperation(ParamOperationRef *operation, std::string &error) {
        if (!m_Bridge || !operation || !m_Bridge->StoreInstanceOperation(m_InstanceId, m_Generation, m_PinIndex, operation)) {
            RollbackNew(operation);
            RestorePrevious(error);
            error = WithRestoreError("failed to store live operation", error);
            m_Finished = true;
            return false;
        }
        CleanupDetached(m_OldSource);
        CleanupDetached(m_OldOperation);
        m_Finished = true;
        return true;
    }

private:
    template <typename T>
    static void CleanupDetached(T *&handle) {
        if (!handle) {
            return;
        }
        handle->DestroyDetached();
        handle->Release();
        handle = nullptr;
    }

    static void RollbackNew(ParamSourceLinkRef *link) {
        if (!link) {
            return;
        }
        link->Restore();
        link->Release();
    }

    static void RollbackNew(ParamOperationRef *operation) {
        if (!operation) {
            return;
        }
        operation->Destroy();
        operation->Release();
    }

    bool RestorePrevious(std::string &error) {
        bool ok = true;
        if (m_OldSource) {
            if (m_Bridge && m_Bridge->StoreInstanceSourceLink(m_InstanceId, m_Generation, m_PinIndex, m_OldSource)) {
                m_OldSource = nullptr;
            } else {
                CleanupDetached(m_OldSource);
                AppendRestoreError(error, "lost previous live source ownership");
                ok = false;
            }
        }
        if (m_OldOperation) {
            if (m_Bridge && m_Bridge->StoreInstanceOperation(m_InstanceId, m_Generation, m_PinIndex, m_OldOperation)) {
                m_OldOperation = nullptr;
            } else {
                CleanupDetached(m_OldOperation);
                AppendRestoreError(error, "lost previous live operation ownership");
                ok = false;
            }
        }
        return ok;
    }

    static void AppendRestoreError(std::string &error, const char *message) {
        if (!error.empty()) {
            error += " ";
        }
        error += message;
        error += ".";
    }

    std::string WithRestoreError(const char *action, const std::string &restoreError) const {
        return fmt::format("{} {} for pin #{}.{}{}",
                           m_Method,
                           action,
                           m_PinIndex,
                           restoreError.empty() ? "" : " ",
                           restoreError);
    }

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_InstanceId = 0;
    int m_Generation = 0;
    int m_PinIndex = -1;
    const char *m_Method = "";
    ParamSourceLinkRef *m_OldSource = nullptr;
    ParamOperationRef *m_OldOperation = nullptr;
    bool m_Finished = false;
};

bool IsInputSourceTypeCompatible(CKContext *context, CKGUID targetGuid, CKParameter *source) {
    if (!context && source) {
        context = source->GetCKContext();
    }
    if (!context || !source) {
        return false;
    }
    const CKGUID sourceGuid = source->GetGUID();
    if (targetGuid == sourceGuid) {
        return true;
    }
    if (ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context)) {
        return registry->IsTypeCompatible(targetGuid, sourceGuid);
    }
    CKParameterManager *pm = context->GetParameterManager();
    return pm && (pm->IsTypeCompatible(targetGuid, sourceGuid) != FALSE ||
                  pm->IsTypeCompatible(sourceGuid, targetGuid) != FALSE);
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

struct SlotNameOccurrence {
    std::string Name;
    int Occurrence = 0;
};

SlotNameOccurrence ParseSlotNameOccurrenceText(const std::string &text) {
    SlotNameOccurrence result;
    result.Name = text;

    const std::size_t open = text.rfind('[');
    if (open != std::string::npos && !text.empty() && text.back() == ']') {
        char *end = nullptr;
        const std::string number = text.substr(open + 1, text.size() - open - 2);
        const long occurrence = std::strtol(number.c_str(), &end, 10);
        if (end && *end == '\0' && occurrence >= 0) {
            result.Name = text.substr(0, open);
            result.Occurrence = static_cast<int>(occurrence);
            return result;
        }
    }

    const std::size_t hash = text.rfind('#');
    if (hash != std::string::npos && hash + 1 < text.size()) {
        char *end = nullptr;
        const long occurrence = std::strtol(text.c_str() + hash + 1, &end, 10);
        if (end && *end == '\0' && occurrence >= 0) {
            result.Name = text.substr(0, hash);
            result.Occurrence = static_cast<int>(occurrence);
        }
    }

    return result;
}

std::string FormatSlotNameOccurrenceText(const std::string &name, int occurrence) {
    return occurrence > 0 && !name.empty()
        ? fmt::format("{}[{}]", name, occurrence)
        : name;
}

} // namespace

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
               const std::string &error,
               CK_ID runtimeBehaviorId,
               const ScriptBridgeObjectStamp &runtimeBehaviorStamp)
    : m_Bridge(bridge),
      m_Context(ctx),
      m_Request(request),
      m_RuntimeBehaviorId(runtimeBehaviorId),
      m_RuntimeBehaviorStamp(runtimeBehaviorStamp),
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
int BBSlot::Occurrence() const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    if (!layout || m_Index < 0) {
        return 0;
    }

    int occurrence = 0;
    auto countIoOccurrence = [&](const std::vector<ScriptBridgeLayoutIoSlot> &slots) {
        for (int i = 0; i < static_cast<int>(slots.size()); ++i) {
            if (slots[i].Name != m_Name) {
                continue;
            }
            if (i == m_Index) {
                return occurrence;
            }
            ++occurrence;
        }
        return 0;
    };
    auto countParamOccurrence = [&](const std::vector<ScriptBridgeLayoutParamSlot> &slots) {
        for (const ScriptBridgeLayoutParamSlot &slot : slots) {
            if (slot.Name != m_Name) {
                continue;
            }
            if (slot.Index == m_Index) {
                return occurrence;
            }
            ++occurrence;
        }
        return 0;
    };

    switch (m_Kind) {
        case ScriptBridgeSlotKind::Input: return countIoOccurrence(layout->Inputs);
        case ScriptBridgeSlotKind::Output: return countIoOccurrence(layout->Outputs);
        case ScriptBridgeSlotKind::Pin: return countParamOccurrence(layout->Pins);
        case ScriptBridgeSlotKind::Pout: return countParamOccurrence(layout->Pouts);
        case ScriptBridgeSlotKind::Setting: return countParamOccurrence(layout->Settings);
        case ScriptBridgeSlotKind::Local: return countParamOccurrence(layout->Locals);
        default: return 0;
    }
}
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
                            SlotKindName(expected),
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
    if (!m_Bridge) {
        return nullptr;
    }
    return m_RuntimeBehaviorId
        ? m_Bridge->GetBehaviorLayout(m_RuntimeBehaviorId, m_RuntimeBehaviorStamp)
        : m_Bridge->GetPrototypeLayout(m_Context, m_Request);
}

std::string BBSlot::KindName() const {
    return SlotKindName(m_Kind);
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
    CKObjectDeclaration *decl = ResolvePrototypeDeclaration(prototype);
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

const ScriptBridgeBBInvocationSpec &BBDecl::Request() const {
    return m_Request;
}

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
    CKObjectDeclaration *decl = ResolvePrototypeDeclaration(prototype);
    return decl ? decl->GetCompatibleClassId() : prototype->GetApplyToClassID();
}

int BBDecl::NeededManagerCount() const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    CKObjectDeclaration *decl = ResolvePrototypeDeclaration(prototype, true);
    return decl ? decl->GetManagerNeededCount() : 0;
}

CKGUID BBDecl::NeededManagerGuid(int index) const {
    std::string error;
    CKBehaviorPrototype *prototype = PrototypeObject(error);
    CKObjectDeclaration *decl = ResolvePrototypeDeclaration(prototype, true);
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
        if (FindIoSlot(*layout, kind, name, occurrence, index)) {
            return new BBSlot(m_Bridge, m_Context, m_Request, kind, index, name, CKGUID(), std::string(), 0, 0, layout->LayoutGeneration, layout->Signature);
        }
    } else if (const ScriptBridgeLayoutParamSlot *slot = FindParamSlot(*layout, kind, name, occurrence)) {
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
                                  SlotKindName(kind),
                                  name,
                                  occurrence,
                                  CandidateList(*layout, kind)));
}

BBConfig::BBConfig(ScriptBehaviorBridge *bridge,
                     const CKBehaviorContext &ctx,
                     const ScriptBridgeBBInvocationSpec &request,
                     const std::string &error)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request), m_Error(error) {
    if (ctx.Behavior) {
        CKBeObject *owner = ctx.Behavior->GetOwner();
        if (owner && m_Request.OwnerId == owner->GetID() && m_Request.OwnerStamp.Flags == 0) {
            m_Request.OwnerStamp = CaptureBridgeObjectStamp(owner);
        }
        CKBeObject *target = ctx.Behavior->GetTarget();
        if (target && m_Request.TargetId == target->GetID() && m_Request.TargetStamp.Flags == 0) {
            m_Request.TargetStamp = CaptureBridgeObjectStamp(target);
        }
    }
}

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
        text += fmt::format("\nDefault start input: '{}'",
                            FormatSlotNameOccurrenceText(m_DefaultStartInput, m_DefaultStartOccurrence));
    }
    if (!m_DefaultStopInput.empty()) {
        text += fmt::format("\nDefault stop input: '{}'",
                            FormatSlotNameOccurrenceText(m_DefaultStopInput, m_DefaultStopOccurrence));
    }
    return text;
}

std::string BBConfig::Explain() const {
    std::string text = Describe();
    text += fmt::format("\nValid: {}", IsValid() ? "true" : "false");
    text += fmt::format("\nLifetime: {}", m_ComponentLifetime ? "component" : "manual");
    text += fmt::format("\nPending pins: {}", m_Request.IndexedParameters.size());
    text += fmt::format("\nPending settings: {}", m_Request.IndexedSettings.size());
    text += fmt::format("\nPending sources: {}", m_Request.SourceParameters.size());
    text += fmt::format("\nPending operations: {}", m_Request.OperationParameters.size());
    text += fmt::format("\nCached slots: {}", m_Slots.size());
    text += fmt::format("\nCurrent instance: {}", (m_Instance && m_Instance->IsValid()) ? "alive" : "none");
    const std::string error = Error();
    if (!error.empty()) {
        text += "\nError: " + error;
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
        SetError(slot ? slot->Error() : fmt::format("Failed to bind {} '{}'.", SlotKindName(kind), name));
    }
    if (slot) {
        slot->Release();
    }
    return ok;
}

BBConfig *BBConfig::Owner(CKBeObject *owner) {
    m_Request.OwnerId = owner ? owner->GetID() : 0;
    m_Request.OwnerStamp = owner ? CaptureBridgeObjectStamp(owner) : ScriptBridgeObjectStamp();
    AddRef();
    return this;
}

BBConfig *BBConfig::Target(CKBeObject *target) {
    m_Request.TargetId = target ? target->GetID() : 0;
    m_Request.TargetStamp = target ? CaptureBridgeObjectStamp(target) : ScriptBridgeObjectStamp();
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
    m_Error.clear();
    if (!m_Bridge) {
        SetError("BBConfig bridge is not available.");
        SetScriptException(m_Error);
        return false;
    }

    BBDecl spec(m_Bridge, m_Context, m_Request);
    if (!spec.IsValid()) {
        SetError(spec.Error());
        SetScriptException(m_Error);
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

    CKObject *ownerObject = m_Request.OwnerId
        ? (m_Request.OwnerStamp.Flags != 0
            ? GetStampedCKObjectById(context, m_Request.OwnerId, m_Request.OwnerStamp)
            : GetCKObjectById(context, m_Request.OwnerId))
        : nullptr;
    CKObject *targetObject = m_Request.TargetId
        ? (m_Request.TargetStamp.Flags != 0
            ? GetStampedCKObjectById(context, m_Request.TargetId, m_Request.TargetStamp)
            : GetCKObjectById(context, m_Request.TargetId))
        : nullptr;
    if (m_Request.OwnerId && !ownerObject) {
        SetError(fmt::format("Owner object {} is no longer valid.", m_Request.OwnerId));
        SetScriptException(m_Error);
        return false;
    }
    if (m_Request.TargetId && !targetObject) {
        SetError(fmt::format("Target object {} is no longer valid.", m_Request.TargetId));
        SetScriptException(m_Error);
        return false;
    }
    if (targetObject) {
        if ((layout->BehaviorFlags & CKBEHAVIOR_TARGETABLE) == 0) {
            SetError(fmt::format("Building Block '{}' is not targetable.",
                                 layout->QualifiedName));
            SetScriptException(m_Error);
            return false;
        }
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
        if (!IsInputSourceTypeCompatible(context, slot->TypeGuid, sourceParam)) {
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

BBInstance *BBConfig::Instance() const {
    if (m_Instance && m_Instance->IsValid()) {
        m_Instance->AddRef();
        return m_Instance;
    }
    return nullptr;
}

BBInstance *BBConfig::EnsureSpawned(const CKBehaviorContext &ctx) {
    if (m_Instance && m_Instance->IsValid()) {
        m_Instance->AddRef();
        return m_Instance;
    }
    return SpawnInstance(ctx);
}

BBInstance *BBConfig::EnsureStarted(const CKBehaviorContext &ctx) {
    BBInstance *instance = EnsureSpawned(ctx);
    if (!instance) {
        return nullptr;
    }
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(instance->BridgeInstanceId(), instance->BridgeGeneration()) : nullptr;
    if (!behavior || !behavior->IsActive()) {
        if (!instance->StartWithContext(ctx)) {
            SetError(instance->Error());
            SetScriptException(m_Error);
            instance->Release();
            return nullptr;
        }
    }
    return instance;
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
    BBInstance *instance = new BBInstance(m_Bridge,
                                          ctx,
                                          m_Request,
                                          instanceId,
                                          generation,
                                          m_DefaultStartInput,
                                          m_DefaultStartOccurrence,
                                          m_DefaultStopInput,
                                          m_DefaultStopOccurrence);
    m_Instance = instance;
    m_Instance->AddRef();
    return instance;
}

BBInstance *BBConfig::SpawnStarted(const CKBehaviorContext &ctx) {
    BBInstance *instance = SpawnInstance(ctx);
    if (!instance) {
        return nullptr;
    }
    if (!instance->StartWithContext(ctx)) {
        SetError(instance->Error());
        SetScriptException(m_Error);
        instance->Destroy();
        if (m_Instance == instance) {
            m_Instance->Release();
            m_Instance = nullptr;
        }
        instance->Release();
        return nullptr;
    }
    return instance;
}

bool BBConfig::Stop(const CKBehaviorContext &ctx) {
    if (!m_Instance) {
        return true;
    }
    const bool result = m_Instance->StopWithContext(ctx);
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
    SetScriptException("BBConfig.OutputActive requires a live BBInstance. Call SpawnStarted(), EnsureSpawned(), or EnsureStarted() first.");
    return false;
}

ParamRef *BBConfig::PinRefSlot(BBSlot *pin) {
    if (m_Instance && m_Instance->IsValid()) {
        return m_Instance->Pin(pin);
    }
    SetScriptException("BBConfig.PinRef requires a live BBInstance. Call SpawnStarted(), EnsureSpawned(), or EnsureStarted() first.");
    return nullptr;
}

ParamRef *BBConfig::PoutRefSlot(BBSlot *pout) {
    if (m_Instance && m_Instance->IsValid()) {
        return m_Instance->Pout(pout);
    }
    SetScriptException("BBConfig.PoutRef requires a live BBInstance. Call SpawnStarted(), EnsureSpawned(), or EnsureStarted() first.");
    return nullptr;
}

void BBConfig::SetDefaultStart(const std::string &inputName) {
    const SlotNameOccurrence slot = ParseSlotNameOccurrenceText(inputName);
    m_DefaultStartInput = slot.Name;
    m_DefaultStartOccurrence = slot.Occurrence;
}

void BBConfig::SetDefaultStop(const std::string &inputName) {
    const SlotNameOccurrence slot = ParseSlotNameOccurrenceText(inputName);
    m_DefaultStopInput = slot.Name;
    m_DefaultStopOccurrence = slot.Occurrence;
}

void BBConfig::SetComponentLifetime(bool componentLifetime) {
    m_ComponentLifetime = componentLifetime;
}

bool BBConfig::UsesComponentLifetime() const {
    return m_ComponentLifetime;
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

const ScriptBridgeBBInvocationSpec &BBConfig::Request() const {
    return m_Request;
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
        SetError(fmt::format("Failed to bind {} '{}'.", SlotKindName(kind), name));
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
        return In(m_DefaultStartInput, m_DefaultStartOccurrence);
    }
    const ScriptBridgeLayoutRecord *layout = m_Bridge ? m_Bridge->GetPrototypeLayout(m_Context, m_Request) : nullptr;
    const std::string inferred = layout ? SelectDefaultInputName(*layout) : std::string();
    return inferred.empty() ? nullptr : In(inferred);
}

BBSlot *BBConfig::DefaultStopSlot() {
    if (!m_DefaultStopInput.empty()) {
        return In(m_DefaultStopInput, m_DefaultStopOccurrence);
    }
    const ScriptBridgeLayoutRecord *layout = m_Bridge ? m_Bridge->GetPrototypeLayout(m_Context, m_Request) : nullptr;
    const std::string inferred = layout ? SelectDefaultStopInputName(*layout) : std::string();
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
    if (m_Instance && m_Instance->IsValid()) {
        CKBehavior *behavior = m_Bridge->GetInstanceBehavior(m_Instance->BridgeInstanceId(), m_Instance->BridgeGeneration());
        std::string writeError;
        if (!WriteLivePinValue(behavior,
                                                       pinIndex,
                                                       value,
                                                       method,
                                                       slot ? slot->Name() : std::string("<null>"),
                                                       writeError)) {
            SetError(writeError);
            SetScriptException(m_Error);
            return false;
        }

        RuntimePinMutation(m_Bridge, m_Instance->BridgeInstanceId(), m_Instance->BridgeGeneration(), pinIndex, method).ClearExisting();
    }
    RemovePendingSource(pinIndex);
    RemovePendingOperation(pinIndex);
    ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value);
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
    if (m_Instance && m_Instance->IsValid()) {
        ParamValue paramValue(value);
        if (!m_Instance->SetSettingValue(slot, &paramValue)) {
            SetError(m_Instance->Error());
            SetScriptException(m_Error);
            return false;
        }
    }
    ScriptBridgeSetIndexedValue(m_Request.IndexedSettings, settingIndex, value);
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
        m_DefaultStartInput = slot->Name();
        m_DefaultStartOccurrence = slot->Occurrence();
    }
    if (slot->IsStop()) {
        m_DefaultStopInput = slot->Name();
        m_DefaultStopOccurrence = slot->Occurrence();
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
    CKParameter *sourceParam = source->Source();
    CKContext *context = m_Context.Context ? m_Context.Context : (sourceParam ? sourceParam->GetCKContext() : nullptr);
    if (!sourceParam) {
        SetError(fmt::format("{} source for pin #{} '{}' is not valid.",
                             method,
                             pinIndex,
                             slot ? slot->Name() : std::string("<null>")));
        SetScriptException(m_Error);
        return false;
    }
    if (!IsInputSourceTypeCompatible(context, slot->TypeGuid(), sourceParam)) {
        SetError(fmt::format("{} source type mismatch for pin #{} '{}' expected {}, got {}.",
                             method,
                             pinIndex,
                             slot ? slot->Name() : std::string("<null>"),
                             slot ? slot->TypeName() : std::string("<unknown>"),
                             ParameterTypeLabel(context, sourceParam)));
        SetScriptException(m_Error);
        return false;
    }

    ScriptBridgeInputSource request;
    request.PinIndex = pinIndex;
    request.SourceId = source->GetID();
    request.SourceStamp = source->Stamp();
    if (!m_Instance || !m_Instance->IsValid()) {
        RemovePendingValue(pinIndex);
        RemovePendingOperation(pinIndex);
        ReplacePendingSource(request);
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
    RuntimePinMutation mutation(m_Bridge, m_Instance->BridgeInstanceId(), m_Instance->BridgeGeneration(), pinIndex, method);
    std::string ownershipError;
    if (!mutation.StoreSource(link, ownershipError)) {
        SetError(ownershipError);
        SetScriptException(m_Error);
        return false;
    }
    RemovePendingValue(pinIndex);
    RemovePendingOperation(pinIndex);
    ReplacePendingSource(request);
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

    const ScriptBridgeOperationSpec operationRequest = operation->RequestForPin(pinIndex);
    if (!m_Instance || !m_Instance->IsValid()) {
        RemovePendingValue(pinIndex);
        RemovePendingSource(pinIndex);
        ReplacePendingOperation(operationRequest);
        return true;
    }

    CKBehavior *behavior = m_Bridge->GetInstanceBehavior(m_Instance->BridgeInstanceId(), m_Instance->BridgeGeneration());
    std::string operationError;
    ParamOperationRef *op = behavior
        ? ConnectOperationToInput(m_Bridge, behavior, pinIndex, operationRequest, operationError, true, nullptr)
        : nullptr;
    if (!op || !op->IsValid()) {
        SetError(fmt::format("{} failed to connect live operation.{}",
                             method,
                             operationError.empty() ? "" : " " + operationError));
        if (op) {
            op->Release();
        }
        SetScriptException(m_Error);
        return false;
    }
    RuntimePinMutation mutation(m_Bridge, m_Instance->BridgeInstanceId(), m_Instance->BridgeGeneration(), pinIndex, method);
    std::string ownershipError;
    if (!mutation.StoreOperation(op, ownershipError)) {
        SetError(ownershipError);
        SetScriptException(m_Error);
        return false;
    }
    RemovePendingValue(pinIndex);
    RemovePendingSource(pinIndex);
    ReplacePendingOperation(operationRequest);
    return true;
}

void BBConfig::RemovePendingValue(int pinIndex) {
    const auto it = std::lower_bound(m_Request.IndexedParameters.begin(),
                                     m_Request.IndexedParameters.end(),
                                     pinIndex,
                                     [](const ScriptBridgeIndexedValue &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it != m_Request.IndexedParameters.end() && it->PinIndex == pinIndex) {
        m_Request.IndexedParameters.erase(it);
    }
}

void BBConfig::RemovePendingSource(int pinIndex) {
    const auto it = std::lower_bound(m_Request.SourceParameters.begin(),
                                     m_Request.SourceParameters.end(),
                                     pinIndex,
                                     [](const ScriptBridgeInputSource &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it != m_Request.SourceParameters.end() && it->PinIndex == pinIndex) {
        m_Request.SourceParameters.erase(it);
    }
}

void BBConfig::RemovePendingOperation(int pinIndex) {
    const auto it = std::lower_bound(m_Request.OperationParameters.begin(),
                                     m_Request.OperationParameters.end(),
                                     pinIndex,
                                     [](const ScriptBridgeOperationSpec &entry, int index) {
                                         return entry.TargetPinIndex < index;
                                     });
    if (it != m_Request.OperationParameters.end() && it->TargetPinIndex == pinIndex) {
        m_Request.OperationParameters.erase(it);
    }
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
                       int defaultStartOccurrence,
                       const std::string &defaultStopInput,
                       int defaultStopOccurrence,
                       const std::string &error)
    : m_Bridge(bridge),
      m_Context(ctx),
      m_Request(request),
      m_Error(error),
      m_InstanceId(instanceId),
      m_Generation(generation),
      m_DefaultStartInput(defaultStartInput),
      m_DefaultStartOccurrence(defaultStartOccurrence),
      m_DefaultStopInput(defaultStopInput),
      m_DefaultStopOccurrence(defaultStopOccurrence) {}

BBInstance::~BBInstance() {
    Destroy();
}

bool BBInstance::IsValid() const {
    return m_Error.empty() && m_Bridge && m_InstanceId != 0 && m_Bridge->IsInstanceValid(m_InstanceId, m_Generation);
}

bool BBInstance::IsAlive() const {
    return m_Error.empty() && m_Bridge && m_InstanceId != 0 && m_Bridge->IsInstanceAlive(m_InstanceId, m_Generation);
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

std::string BBInstance::Explain() const {
    std::string text = fmt::format("BBInstance id={} generation={}", m_InstanceId, m_Generation);
    text += fmt::format("\nValid: {}", IsValid() ? "true" : "false");
    text += fmt::format("\nAlive: {}", IsAlive() ? "true" : "false");
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    text += fmt::format("\nBehavior: {}", behavior ? SafeString(behavior->GetName()) : std::string("<none>"));
    if (!m_DefaultStartInput.empty()) {
        text += fmt::format("\nDefault start input: '{}'",
                            FormatSlotNameOccurrenceText(m_DefaultStartInput, m_DefaultStartOccurrence));
    }
    if (!m_DefaultStopInput.empty()) {
        text += fmt::format("\nDefault stop input: '{}'",
                            FormatSlotNameOccurrenceText(m_DefaultStopInput, m_DefaultStopOccurrence));
    }
    const ScriptBridgeExecutionState state = m_Bridge ? m_Bridge->GetInstanceState(m_InstanceId, m_Generation) : ScriptBridgeExecutionState();
    text += fmt::format("\nLast return: {}", state.ReturnCode);
    const std::string error = Error();
    if (!error.empty()) {
        text += "\nError: " + error;
    }
    return text;
}

BBDecl *BBInstance::Decl() const {
    return new BBDecl(m_Bridge, m_Context, m_Request, m_Error);
}

BehaviorRef *BBInstance::Behavior() const {
    return m_Bridge ? m_Bridge->WrapBehavior(m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation)) : nullptr;
}

BehaviorLayout *BBInstance::Layout() const {
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    return behavior ? new BehaviorLayout(m_Bridge, behavior->GetID()) : nullptr;
}

BBSlot *BBInstance::Input(const std::string &name, int occurrence) const {
    return RuntimeSlot(ScriptBridgeSlotKind::Input, name, occurrence);
}

BBSlot *BBInstance::Output(const std::string &name, int occurrence) const {
    return RuntimeSlot(ScriptBridgeSlotKind::Output, name, occurrence);
}

BBSlot *BBInstance::PinSlot(const std::string &name, int occurrence) const {
    return RuntimeSlot(ScriptBridgeSlotKind::Pin, name, occurrence);
}

BBSlot *BBInstance::PoutSlot(const std::string &name, int occurrence) const {
    return RuntimeSlot(ScriptBridgeSlotKind::Pout, name, occurrence);
}

BBSlot *BBInstance::Setting(const std::string &name, int occurrence) const {
    return RuntimeSlot(ScriptBridgeSlotKind::Setting, name, occurrence);
}

BBSlot *BBInstance::Local(const std::string &name, int occurrence) const {
    return RuntimeSlot(ScriptBridgeSlotKind::Local, name, occurrence);
}

bool BBInstance::Start() {
    if (m_DefaultStartInput.empty()) {
        return Start(nullptr);
    }
    BBSlot *slot = Input(m_DefaultStartInput, m_DefaultStartOccurrence);
    const bool result = Start(slot);
    if (slot) {
        slot->Release();
    }
    return result;
}

bool BBInstance::StartWithContext(const CKBehaviorContext &ctx) {
    m_Context = ctx;
    return Start();
}

bool BBInstance::StartSlotWithContext(const CKBehaviorContext &ctx, BBSlot *input) {
    m_Context = ctx;
    return Start(input);
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
        if (!ResolveRuntimeSlot(m_Bridge, behavior, input, ScriptBridgeSlotKind::Input, "BBInstance.Start", inputIndex, error)) {
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
        BBSlot *slot = Input(m_DefaultStopInput, m_DefaultStopOccurrence);
        if (slot) {
            std::string error;
            CKBehavior *behavior = m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation);
            if (!ResolveRuntimeSlot(m_Bridge, behavior, slot, ScriptBridgeSlotKind::Input, "BBInstance.Stop", inputIndex, error)) {
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

bool BBInstance::StopWithContext(const CKBehaviorContext &ctx) {
    m_Context = ctx;
    return Stop();
}

bool BBInstance::OutputActive(BBSlot *output) const {
    int outputIndex = -1;
    std::string error;
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    if (!ResolveRuntimeSlot(m_Bridge, behavior, output, ScriptBridgeSlotKind::Output, "BBInstance.OutputActive", outputIndex, error)) {
        SetScriptException(error);
        return false;
    }
    return ExecutionOutputActive(m_Bridge ? m_Bridge->GetInstanceState(m_InstanceId, m_Generation) : ScriptBridgeExecutionState(), outputIndex);
}

ParamRef *BBInstance::Pin(BBSlot *pin) const {
    int index = -1;
    std::string error;
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    if (!ResolveRuntimeSlot(m_Bridge, behavior, pin, ScriptBridgeSlotKind::Pin, "BBInstance.Pin", index, error)) {
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
    if (!ResolveRuntimeSlot(m_Bridge, behavior, pout, ScriptBridgeSlotKind::Pout, "BBInstance.Pout", index, error)) {
        SetScriptException(error);
        return nullptr;
    }
    CKParameterOut *param = behavior && index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
    return m_Bridge && param ? new ParamRef(m_Bridge, param->GetID(), ScriptBridgeSlotKind::Pout, index, behavior->GetID()) : nullptr;
}

bool BBInstance::SetSlot(BBSlot *pin, ParamValue *value) {
    if (!value) {
        SetError("BBInstance.Set requires a valid ParamValue.");
        SetScriptException(m_Error);
        return false;
    }
    return SetValueForPin(pin, value->Value(), "BBInstance.Set");
}

bool BBInstance::SetSlotInt(BBSlot *pin, int value) {
    return SetValueForPin(pin, MakeScriptParamInt(value), "BBInstance.Set");
}

bool BBInstance::SetSlotFloat(BBSlot *pin, float value) {
    return SetValueForPin(pin, MakeScriptParamFloat(value), "BBInstance.Set");
}

bool BBInstance::SetSlotBool(BBSlot *pin, bool value) {
    return SetValueForPin(pin, MakeScriptParamBool(value), "BBInstance.Set");
}

bool BBInstance::SetSlotString(BBSlot *pin, const std::string &value) {
    return SetValueForPin(pin, MakeScriptParamString(value), "BBInstance.Set");
}

bool BBInstance::SetSlotObject(BBSlot *pin, CKObject *value) {
    return SetValueForPin(pin, MakeScriptParamObject(value), "BBInstance.Set");
}

bool BBInstance::SourceSlot(BBSlot *pin, ParamRef *source) {
    return SourceForPin(pin, source, "BBInstance.Source");
}

bool BBInstance::OperationSlot(BBSlot *pin, ParamOp *operation) {
    return OperationForPin(pin, operation, "BBInstance.Operation");
}

bool BBInstance::StepSetSlot(const CKBehaviorContext &ctx, BBSlot *pin, ParamValue *value) {
    return SetSlot(pin, value) && Step(ctx);
}

bool BBInstance::StepSetSlotInt(const CKBehaviorContext &ctx, BBSlot *pin, int value) {
    return SetSlotInt(pin, value) && Step(ctx);
}

bool BBInstance::StepSetSlotFloat(const CKBehaviorContext &ctx, BBSlot *pin, float value) {
    return SetSlotFloat(pin, value) && Step(ctx);
}

bool BBInstance::StepSetSlotBool(const CKBehaviorContext &ctx, BBSlot *pin, bool value) {
    return SetSlotBool(pin, value) && Step(ctx);
}

bool BBInstance::StepSetSlotString(const CKBehaviorContext &ctx, BBSlot *pin, const std::string &value) {
    return SetSlotString(pin, value) && Step(ctx);
}

bool BBInstance::StepSetSlotObject(const CKBehaviorContext &ctx, BBSlot *pin, CKObject *value) {
    return SetSlotObject(pin, value) && Step(ctx);
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
    if (!ResolveRuntimeSlot(m_Bridge, behavior, setting, ScriptBridgeSlotKind::Setting, "BBInstance.SetSetting", settingIndex, error)) {
        SetError(error);
        SetScriptException(m_Error);
        return false;
    }
    if (!m_Bridge->SetInstanceSetting(m_InstanceId, m_Generation, settingIndex, value->Value(), error)) {
        SetError(error);
        SetScriptException(m_Error);
        return false;
    }
    ScriptBridgeSetIndexedValue(m_Request.IndexedSettings, settingIndex, value->Value());
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

BBSlot *BBInstance::RuntimeSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const {
    if (!m_Bridge) {
        return new BBSlot(nullptr, m_Context, m_Request, kind, -1, name, CKGUID(), std::string(), 0, 0, 0, std::string(), "BBInstance bridge is not available.");
    }

    CKBehavior *behavior = m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation);
    if (!behavior) {
        return new BBSlot(m_Bridge, m_Context, m_Request, kind, -1, name, CKGUID(), std::string(), 0, 0, 0, std::string(), "BBInstance behavior is not available.");
    }

    const ScriptBridgeObjectStamp stamp = CaptureBridgeObjectStamp(behavior);
    const ScriptBridgeLayoutRecord *layout = m_Bridge->GetBehaviorLayout(behavior->GetID(), stamp);
    if (!layout) {
        return new BBSlot(m_Bridge, m_Context, m_Request, kind, -1, name, CKGUID(), std::string(), 0, 0, 0, std::string(), "BBInstance runtime layout is not available.", behavior->GetID(), stamp);
    }

    if (kind == ScriptBridgeSlotKind::Input || kind == ScriptBridgeSlotKind::Output) {
        int index = -1;
        if (FindIoSlot(*layout, kind, name, occurrence, index)) {
            return new BBSlot(m_Bridge,
                              m_Context,
                              m_Request,
                              kind,
                              index,
                              name,
                              CKGUID(),
                              std::string(),
                              0,
                              0,
                              layout->LayoutGeneration,
                              layout->Signature,
                              std::string(),
                              behavior->GetID(),
                              stamp);
        }
    } else if (const ScriptBridgeLayoutParamSlot *slot = FindParamSlot(*layout, kind, name, occurrence)) {
        return new BBSlot(m_Bridge,
                          m_Context,
                          m_Request,
                          kind,
                          slot->Index,
                          slot->Name,
                          slot->TypeGuid,
                          slot->TypeName,
                          slot->DataSize,
                          slot->Caps,
                          layout->LayoutGeneration,
                          layout->Signature,
                          std::string(),
                          behavior->GetID(),
                          stamp);
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
                      fmt::format("Runtime Building Block '{}' has no {} named '{}' (occurrence {}). Candidates: {}.",
                                  SafeString(behavior->GetPrototypeName()),
                                  SlotKindName(kind),
                                  name,
                                  occurrence,
                                  CandidateList(*layout, kind)),
                      behavior->GetID(),
                      stamp);
}

bool BBInstance::SetValueForPin(BBSlot *pin, const ScriptParamValue &value, const char *method) {
    int pinIndex = -1;
    std::string error;
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    if (!ResolveRuntimeSlot(m_Bridge, behavior, pin, ScriptBridgeSlotKind::Pin, method, pinIndex, error)) {
        SetError(error);
        SetScriptException(m_Error);
        return false;
    }

    std::string writeError;
    if (!WriteLivePinValue(behavior,
                                                   pinIndex,
                                                   value,
                                                   method,
                                                   pin ? pin->Name() : std::string("<null>"),
                                                   writeError)) {
        SetError(writeError);
        SetScriptException(m_Error);
        return false;
    }

    RuntimePinMutation(m_Bridge, m_InstanceId, m_Generation, pinIndex, method).ClearExisting();
    ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value);
    return true;
}

bool BBInstance::SourceForPin(BBSlot *pin, ParamRef *source, const char *method) {
    int pinIndex = -1;
    std::string error;
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    if (!ResolveRuntimeSlot(m_Bridge, behavior, pin, ScriptBridgeSlotKind::Pin, method, pinIndex, error)) {
        SetError(error);
        SetScriptException(m_Error);
        return false;
    }
    if (!source || !source->IsValid()) {
        SetError(fmt::format("{} requires a valid ParamRef source.", method));
        SetScriptException(m_Error);
        return false;
    }
    CKParameterIn *targetPin = behavior && pinIndex >= 0 && pinIndex < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(pinIndex)
        : nullptr;
    CKParameter *sourceParam = source->Source();
    if (!targetPin || !sourceParam ||
        !IsInputSourceTypeCompatible(behavior ? behavior->GetCKContext() : nullptr,
                                                             targetPin->GetGUID(),
                                                             sourceParam)) {
        SetError(fmt::format("{} source type mismatch for pin #{} '{}' expected {}, got {}.",
                             method,
                             pinIndex,
                             pin ? pin->Name() : std::string("<null>"),
                             ParameterTypeLabel(behavior ? behavior->GetCKContext() : nullptr, targetPin),
                             ParameterTypeLabel(behavior ? behavior->GetCKContext() : nullptr, sourceParam)));
        SetScriptException(m_Error);
        return false;
    }

    ParamRef *target = Pin(pin);
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
    RuntimePinMutation mutation(m_Bridge, m_InstanceId, m_Generation, pinIndex, method);
    std::string ownershipError;
    if (!mutation.StoreSource(link, ownershipError)) {
        SetError(ownershipError);
        SetScriptException(m_Error);
        return false;
    }
    return true;
}

bool BBInstance::OperationForPin(BBSlot *pin, ParamOp *operation, const char *method) {
    int pinIndex = -1;
    std::string error;
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetInstanceBehavior(m_InstanceId, m_Generation) : nullptr;
    if (!ResolveRuntimeSlot(m_Bridge, behavior, pin, ScriptBridgeSlotKind::Pin, method, pinIndex, error)) {
        SetError(error);
        SetScriptException(m_Error);
        return false;
    }
    if (!operation) {
        SetError(fmt::format("{} requires a valid ParamOp.", method));
        SetScriptException(m_Error);
        return false;
    }

    std::string operationError;
    ParamOperationRef *op = behavior
        ? ConnectOperationToInput(m_Bridge, behavior, pinIndex, operation->RequestForPin(pinIndex), operationError, true, nullptr)
        : nullptr;
    if (!op || !op->IsValid()) {
        SetError(fmt::format("{} failed to connect live operation.{}",
                             method,
                             operationError.empty() ? "" : " " + operationError));
        if (op) {
            op->Release();
        }
        SetScriptException(m_Error);
        return false;
    }
    RuntimePinMutation mutation(m_Bridge, m_InstanceId, m_Generation, pinIndex, method);
    std::string ownershipError;
    if (!mutation.StoreOperation(op, ownershipError)) {
        SetError(ownershipError);
        SetScriptException(m_Error);
        return false;
    }
    return true;
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
    m_Request.OwnerStamp = owner ? CaptureBridgeObjectStamp(owner) : ScriptBridgeObjectStamp();
    AddRef();
    return this;
}

BBCallBuilder *BBCallBuilder::Target(CKBeObject *target) {
    m_Request.TargetId = target ? target->GetID() : 0;
    m_Request.TargetStamp = target ? CaptureBridgeObjectStamp(target) : ScriptBridgeObjectStamp();
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
    : m_Context(ctx.Context ? ctx.Context : (ctx.Behavior ? ctx.Behavior->GetCKContext() : nullptr)),
      m_ComponentId(ComponentIdFromContext(ctx)),
      m_ComponentStamp(CaptureBridgeObjectStamp(ctx.Behavior)),
      m_Request(request) {
    (void)bridge;
    if (!m_Request.ComponentId) {
        m_Request.ComponentId = m_ComponentId;
    }
    if (ctx.Behavior) {
        CKBeObject *owner = ctx.Behavior->GetOwner();
        if (owner && m_Request.OwnerId == owner->GetID() && m_Request.OwnerStamp.Flags == 0) {
            m_Request.OwnerStamp = CaptureBridgeObjectStamp(owner);
        }
        CKBeObject *target = ctx.Behavior->GetTarget();
        if (target && m_Request.TargetId == target->GetID() && m_Request.TargetStamp.Flags == 0) {
            m_Request.TargetStamp = CaptureBridgeObjectStamp(target);
        }
    }
}

BBTaskBuilder *BBTaskBuilder::Owner(CKBeObject *owner) {
    m_Request.OwnerId = owner ? owner->GetID() : 0;
    m_Request.OwnerStamp = owner ? CaptureBridgeObjectStamp(owner) : ScriptBridgeObjectStamp();
    AddRef();
    return this;
}

BBTaskBuilder *BBTaskBuilder::Target(CKBeObject *target) {
    m_Request.TargetId = target ? target->GetID() : 0;
    m_Request.TargetStamp = target ? CaptureBridgeObjectStamp(target) : ScriptBridgeObjectStamp();
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
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BBTaskBuilder.Start requires a live AngelScript behavior bridge.");
        return nullptr;
    }
    CKBehaviorContext ctx = MakeContext();
    if (m_ComponentId && !ctx.Behavior) {
        SetScriptException("BBTaskBuilder.Start owning behavior is no longer valid.");
        return nullptr;
    }
    return bridge->StartTask(m_Request, ctx, inputIndex);
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

ScriptBehaviorBridge *BBTaskBuilder::Bridge() const {
    ScriptManager *manager = m_Context ? ScriptManager::GetManager(m_Context) : nullptr;
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

CKBehavior *BBTaskBuilder::Component() const {
    return CKBehavior::Cast(GetStampedCKObjectById(m_Context, m_ComponentId, m_ComponentStamp));
}

CKBehaviorContext BBTaskBuilder::MakeContext() const {
    CKBehaviorContext ctx = {};
    ctx.Context = m_Context;
    ctx.Behavior = Component();
    ctx.ParameterManager = m_Context ? m_Context->GetParameterManager() : nullptr;
    ctx.MessageManager = m_Context ? m_Context->GetMessageManager() : nullptr;
    ctx.AttributeManager = m_Context ? m_Context->GetAttributeManager() : nullptr;
    ctx.TimeManager = m_Context ? m_Context->GetTimeManager() : nullptr;
    ctx.CurrentRenderContext = m_Context ? m_Context->GetPlayerRenderContext() : nullptr;
    return ctx;
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
    CKObjectDeclaration *decl = ResolvePrototypeDeclaration(prototype);
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

BBBridge::BBBridge(ScriptBehaviorBridge *, const CKBehaviorContext &ctx)
    : m_Context(ctx.Context),
      m_ComponentId(ComponentIdFromContext(ctx)),
      m_ComponentStamp(CaptureBridgeObjectStamp(ctx.Behavior)) {}

ScriptBehaviorBridge *BBBridge::Bridge() const {
    ScriptManager *manager = m_Context ? ScriptManager::GetManager(m_Context) : nullptr;
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

CKBehavior *BBBridge::Component() const {
    return CKBehavior::Cast(GetStampedCKObjectById(m_Context, m_ComponentId, m_ComponentStamp));
}

CKBehaviorContext BBBridge::MakeContext() const {
    CKBehaviorContext ctx;
    ctx.Context = m_Context;
    ctx.Behavior = Component();
    ctx.ParameterManager = m_Context ? m_Context->GetParameterManager() : nullptr;
    ctx.MessageManager = m_Context ? m_Context->GetMessageManager() : nullptr;
    ctx.AttributeManager = m_Context ? m_Context->GetAttributeManager() : nullptr;
    ctx.TimeManager = m_Context ? m_Context->GetTimeManager() : nullptr;
    ctx.CurrentRenderContext = m_Context ? m_Context->GetPlayerRenderContext() : nullptr;
    return ctx;
}

BBPrototype *BBBridge::PrototypeByName(const std::string &name) const {
    ScriptBehaviorBridge *bridge = Bridge();
    CKBehaviorContext ctx = MakeContext();
    return bridge ? bridge->CreatePrototype(ctx, name) : nullptr;
}

BBPrototype *BBBridge::PrototypeByGuid(CKGUID guid) const {
    ScriptBehaviorBridge *bridge = Bridge();
    CKBehaviorContext ctx = MakeContext();
    return bridge ? bridge->CreatePrototype(ctx, guid) : nullptr;
}

int BBBridge::Count() const {
    return CKGetPrototypeDeclarationCount();
}

BBPrototype *BBBridge::At(int index) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge || index < 0 || index >= Count()) {
        return nullptr;
    }

    CKObjectDeclaration *decl = CKGetPrototypeDeclaration(index);
    CKBehaviorContext ctx = MakeContext();
    return decl ? bridge->CreatePrototype(ctx, decl->GetGuid()) : nullptr;
}

BBPrototype *BBBridge::Find(const std::string &query, int occurrence) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge || query.empty() || occurrence < 0) {
        return nullptr;
    }

    CKBehaviorContext ctx = MakeContext();
    CKGUID parsed;
    if (ParseScriptGuidString(query, parsed)) {
        return occurrence == 0 && CKGetPrototypeFromGuid(parsed) ? bridge->CreatePrototype(ctx, parsed) : nullptr;
    }

    int seen = 0;
    const int count = CKGetPrototypeDeclarationCount();
    for (int i = 0; i < count; ++i) {
        CKObjectDeclaration *decl = CKGetPrototypeDeclaration(i);
        if (!PrototypeMatches(decl, query)) {
            continue;
        }
        if (seen == occurrence) {
            return bridge->CreatePrototype(ctx, decl->GetGuid());
        }
        ++seen;
    }

    return nullptr;
}

CScriptArray *BBBridge::FindAll(const std::string &query) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        return nullptr;
    }

    CScriptArray *results = CreatePrototypeArray(bridge);
    if (!results) {
        return nullptr;
    }

    CKBehaviorContext ctx = MakeContext();
    CKGUID parsed;
    if (!query.empty() && ParseScriptGuidString(query, parsed)) {
        if (CKGetPrototypeFromGuid(parsed)) {
            AppendPrototype(results, bridge->CreatePrototype(ctx, parsed));
        }
        return results;
    }

    for (CKObjectDeclaration *decl : FindPrototypeDeclarations(query)) {
        AppendPrototype(results, bridge->CreatePrototype(ctx, decl->GetGuid()));
    }

    return results;
}

BBDecl *BBBridge::Require(const std::string &query) const {
    ScriptBehaviorBridge *bridge = Bridge();
    CKBehaviorContext ctx = MakeContext();
    ScriptBridgeBBInvocationSpec request;
    request.ComponentId = ComponentIdFromContext(ctx);
    request.PrototypeName = query;

    if (!bridge) {
        return new BBDecl(nullptr, ctx, request, "AngelScript BB bridge is not available.");
    }
    if (query.empty()) {
        return new BBDecl(bridge, ctx, request, "BB::Require needs a non-empty prototype name, Category/Name, or GUID.");
    }

    CKGUID parsed;
    if (ParseScriptGuidString(query, parsed)) {
        request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
        request.Guid = parsed;
        if (CKGetPrototypeFromGuid(parsed)) {
            return new BBDecl(bridge, ctx, request);
        }
        return new BBDecl(bridge, ctx, request, fmt::format("BB prototype GUID {} was not found.", GuidToString(parsed)));
    }

    const std::vector<CKObjectDeclaration *> matches = FindPrototypeDeclarations(query);
    if (matches.size() == 1 && matches[0]) {
        request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
        request.Guid = matches[0]->GetGuid();
        return new BBDecl(bridge, ctx, request);
    }
    if (matches.size() > 1) {
        return new BBDecl(bridge,
                          ctx,
                          request,
                          fmt::format("BB prototype '{}' is ambiguous. Candidates: {}.",
                                      query,
                                      PrototypeCandidateList(matches)));
    }

    return new BBDecl(bridge,
                      ctx,
                      request,
                      fmt::format("BB prototype '{}' was not found. Use BB::FindAll(query) to inspect candidates.", query));
}

BBDecl *BBBridge::RequireGuid(CKGUID guid) const {
    ScriptBehaviorBridge *bridge = Bridge();
    CKBehaviorContext ctx = MakeContext();
    ScriptBridgeBBInvocationSpec request;
    request.ComponentId = ComponentIdFromContext(ctx);
    request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
    request.Guid = guid;

    if (!bridge) {
        return new BBDecl(nullptr, ctx, request, "AngelScript BB bridge is not available.");
    }
    if (!CKGuidIsValid(guid) || !CKGetPrototypeFromGuid(guid)) {
        return new BBDecl(bridge, ctx, request, fmt::format("BB prototype GUID {} was not found.", GuidToString(guid)));
    }
    return new BBDecl(bridge, ctx, request);
}
