#include "ScriptComponentInjection.h"

#include <cstdlib>
#include <sstream>

#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeCommon.h"
#include "ScriptBridgeHandles.h"
#include "ScriptComponentMetadata.h"
#include "ScriptParameterConversion.h"
#include "ScriptInvoker.h"

namespace AngelScriptComponentInternal {

bool ReadStringValue(CKParameter *source, std::string &value) {
    return ReadParameterText(source, value);
}

bool IsObjectValueParameter(CKParameter *source) {
    if (!source) {
        return false;
    }

    const CK_CLASSID classId = source->GetParameterClassID();
    return classId != 0 && CKIsChildClassOf(classId, CKCID_OBJECT);
}

CKObject *ReadObjectValue(CKParameter *source, CKContext *context) {
    if (!source) {
        return nullptr;
    }

    const bool objectParameter = IsObjectValueParameter(source);
    if (objectParameter) {
        if (CKObject *object = source->GetValueObject(TRUE)) {
            return object;
        }
    }

    std::string text;
    if (ReadStringValue(source, text)) {
        text = TrimString(text);
        if (!text.empty()) {
            char *end = nullptr;
            const unsigned long parsed = std::strtoul(text.c_str(), &end, 0);
            if (end && *end == '\0') {
                return context ? CKGetObject(context, static_cast<CK_ID>(parsed)) : nullptr;
            }
            return context ? context->GetObjectByName(const_cast<CKSTRING>(text.c_str())) : nullptr;
        }
    }

    if (objectParameter) {
        CK_ID id = 0;
        if (source->GetValue(&id) == CK_OK && id != 0) {
            return context ? CKGetObject(context, id) : nullptr;
        }
    }

    return nullptr;
}

CKBehavior *ReadBehaviorValue(CKParameter *source, CKContext *context) {
    if (!source) {
        return nullptr;
    }

    if (CKBehavior *behavior = CKBehavior::Cast(ReadObjectValue(source, context))) {
        return behavior;
    }

    std::string text;
    if (ReadStringValue(source, text)) {
        text = StripQuotes(text);
        if (!text.empty()) {
            char *end = nullptr;
            const unsigned long parsed = std::strtoul(text.c_str(), &end, 0);
            if (end && *end == '\0') {
                return context ? CKBehavior::Cast(CKGetObject(context, static_cast<CK_ID>(parsed))) : nullptr;
            }
            return FindBehaviorByNameInContext(context, text);
        }
    }

    return nullptr;
}

bool AssignObjectHandle(asIScriptObject *object, int propertyIndex, void *value) {
    if (!object || propertyIndex < 0) {
        return false;
    }

    void *address = object->GetAddressOfProperty(static_cast<asUINT>(propertyIndex));
    if (!address) {
        return false;
    }

    *static_cast<void **>(address) = value;
    return true;
}

void **GetHandleSlot(asIScriptObject *object, int propertyIndex) {
    if (!object || propertyIndex < 0) {
        return nullptr;
    }

    void *address = object->GetAddressOfProperty(static_cast<asUINT>(propertyIndex));
    return address ? static_cast<void **>(address) : nullptr;
}

bool AssignBehaviorRefHandle(ScriptBehaviorBridge *bridge, asIScriptObject *object, int propertyIndex, BehaviorRef *value) {
    void **slot = GetHandleSlot(object, propertyIndex);
    if (!slot) {
        if (bridge && value) {
            bridge->ReleaseBehaviorRef(value);
        }
        return false;
    }

    if (*slot == value) {
        return true;
    }

    if (bridge && *slot) {
        bridge->ReleaseBehaviorRef(static_cast<BehaviorRef *>(*slot));
    }
    *slot = value;
    return true;
}

bool AssignBBPrototypeHandle(ScriptBehaviorBridge *bridge, asIScriptObject *object, int propertyIndex, BBPrototype *value) {
    void **slot = GetHandleSlot(object, propertyIndex);
    if (!slot) {
        if (bridge && value) {
            bridge->ReleasePrototype(value);
        }
        return false;
    }

    if (*slot == value) {
        return true;
    }

    if (bridge && *slot) {
        bridge->ReleasePrototype(static_cast<BBPrototype *>(*slot));
    }
    *slot = value;
    return true;
}

template <typename T>
bool AssignRefCountedHandle(asIScriptObject *object, int propertyIndex, T *value) {
    void **slot = GetHandleSlot(object, propertyIndex);
    if (!slot) {
        if (value) {
            value->Release();
        }
        return false;
    }

    if (*slot == value) {
        return true;
    }

    if (*slot) {
        static_cast<T *>(*slot)->Release();
    }
    *slot = value;
    return true;
}

BBConfig *GetBBConfigField(ScriptComponentState *state, const ScriptComponentBinding &binding) {
    if (!state || !state->Object || binding.PropertyIndex < 0 || binding.Kind != ScriptComponentBindingKind::BBConfig) {
        return nullptr;
    }
    void **slot = GetHandleSlot(state->Object, binding.PropertyIndex);
    return slot ? static_cast<BBConfig *>(*slot) : nullptr;
}

BBConfig *GetBBConfigFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (binding.Kind == ScriptComponentBindingKind::BBConfig && binding.FieldName == fieldName) {
            return GetBBConfigField(state, binding);
        }
    }
    return nullptr;
}

ParamRef *GetParamRefFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (binding.Kind == ScriptComponentBindingKind::ParamRef && binding.FieldName == fieldName) {
            void **slot = GetHandleSlot(state->Object, binding.PropertyIndex);
            ParamRef *ref = slot ? static_cast<ParamRef *>(*slot) : nullptr;
            if (ref) {
                ref->AddRef();
            }
            return ref;
        }
    }
    return nullptr;
}

BehaviorRef *GetBehaviorRefFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (binding.Kind == ScriptComponentBindingKind::BehaviorRef && binding.FieldName == fieldName) {
            void **slot = GetHandleSlot(state->Object, binding.PropertyIndex);
            BehaviorRef *ref = slot ? static_cast<BehaviorRef *>(*slot) : nullptr;
            if (ref) {
                ref->AddRef();
            }
            return ref;
        }
    }
    return nullptr;
}

BBInstance *GetBBInstanceFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    asITypeInfo *type = state->Object->GetObjectType();
    asIScriptEngine *engine = type ? type->GetEngine() : nullptr;
    for (asUINT i = 0; type && i < type->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        int propertyTypeId = 0;
        bool isPrivate = false;
        bool isProtected = false;
        bool isConst = false;
        type->GetProperty(i, &propertyName, &propertyTypeId, &isPrivate, &isProtected, nullptr, nullptr, nullptr, nullptr, nullptr, &isConst);
        if (!propertyName || fieldName != propertyName || isPrivate || isProtected || isConst) {
            continue;
        }
        const char *decl = engine ? engine->GetTypeDeclaration(propertyTypeId, true) : nullptr;
        if (!decl || std::string(decl) != "BBInstance@") {
            return nullptr;
        }
        void **slot = GetHandleSlot(state->Object, static_cast<int>(i));
        BBInstance *instance = slot ? static_cast<BBInstance *>(*slot) : nullptr;
        if (instance) {
            instance->AddRef();
        }
        return instance;
    }
    return nullptr;
}

CKBeObject *GetBeObjectFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    asITypeInfo *type = state->Object->GetObjectType();
    for (asUINT i = 0; type && i < type->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        int propertyTypeId = 0;
        bool isPrivate = false;
        bool isProtected = false;
        bool isConst = false;
        type->GetProperty(i, &propertyName, &propertyTypeId, &isPrivate, &isProtected, nullptr, nullptr, nullptr, nullptr, nullptr, &isConst);
        if (!propertyName || fieldName != propertyName || isPrivate || isProtected || isConst) {
            continue;
        }
        if (InferKindFromProperty(type->GetEngine(), propertyTypeId) != ScriptComponentBindingKind::Object) {
            return nullptr;
        }
        void **slot = GetHandleSlot(state->Object, static_cast<int>(i));
        return slot ? CKBeObject::Cast(static_cast<CKObject *>(*slot)) : nullptr;
    }
    return nullptr;
}

CKBeObject *ResolveBBConfigObjectExpression(ScriptComponentState *state,
                                            const CKBehaviorContext &behcontext,
                                            const std::string &expression) {
    const std::string text = StripQuotes(expression);
    if (text.empty()) {
        return nullptr;
    }
    if (text == "$owner") {
        return behcontext.Behavior ? behcontext.Behavior->GetOwner() : nullptr;
    }
    if (text == "$target") {
        return behcontext.Behavior ? behcontext.Behavior->GetTarget() : nullptr;
    }
    if (text == "$level") {
        return behcontext.CurrentLevel ? behcontext.CurrentLevel : (behcontext.Context ? behcontext.Context->GetCurrentLevel() : nullptr);
    }
    return GetBeObjectFieldByName(state, text);
}

bool ValidateObjectFieldValue(asIScriptEngine *engine, const ScriptComponentBinding &binding, CKObject *value, std::string &error) {
    if (!value) {
        return true;
    }

    const CK_CLASSID expected = ClassIdFromPropertyType(engine, binding.PropertyTypeId);
    if (expected == CKCID_OBJECT || CKIsChildClassOf(value, expected)) {
        return true;
    }

    error = "Component parameter '" + binding.ParameterName + "' value '" +
            (value->GetName() ? value->GetName() : "<unnamed>") +
            "' is not compatible with field '" + binding.FieldName + "' (" +
            ClassNameFromPropertyType(engine, binding.PropertyTypeId) + ").";
    return false;
}

BBPrototype *CreatePrototypeFromParameter(ScriptBehaviorBridge *bridge,
                                          const CKBehaviorContext &behcontext,
                                          CKParameter *source,
                                          std::string &error) {
    if (!source) {
        error = "BBPrototype Component parameter source is not available.";
        return nullptr;
    }

    if (CKBehavior *behavior = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context))) {
        const CKGUID guid = behavior->GetPrototypeGuid();
        if (guid != CKGUID()) {
            return bridge ? bridge->CreatePrototype(behcontext, guid) : nullptr;
        }
        error = "BBPrototype Component parameter '" + SafeString(source->GetName()) +
                "' references behavior '" + SafeString(behavior->GetName()) +
                "' without a prototype GUID.";
        return nullptr;
    }

    std::string prototypeName;
    if (!ReadStringValue(source, prototypeName)) {
        error = "Failed to read BBPrototype Component parameter '" + SafeString(source->GetName()) +
                "' as text. CK type=" + ParameterTypeLabel(behcontext.Context, source) + ".";
        return nullptr;
    }

    prototypeName = TrimString(prototypeName);
    if (prototypeName.empty() || !bridge) {
        if (prototypeName.empty()) {
            error = "BBPrototype Component parameter '" + SafeString(source->GetName()) +
                    "' is empty. Expected BB name, Category/Name, GUID, or CKBehavior object.";
        } else {
            error = "BBPrototype Component injection requires ScriptBehaviorBridge.";
        }
        return nullptr;
    }

    CKGUID guid;
    BBPrototype *prototype = ParseScriptGuidString(prototypeName, guid)
        ? bridge->CreatePrototype(behcontext, guid)
        : bridge->CreatePrototype(behcontext, prototypeName);
    if (!prototype) {
        error = "BBPrototype Component parameter '" + SafeString(source->GetName()) +
                "' did not resolve to a BB prototype: '" + prototypeName + "'.";
    }
    return prototype;
}

ScriptBridgeSlotKind SlotKindFromText(const std::string &value) {
    const std::string text = ToLower(StripQuotes(value));
    if (text == "input" || text == "in") {
        return ScriptBridgeSlotKind::Input;
    }
    if (text == "output" || text == "out") {
        return ScriptBridgeSlotKind::Output;
    }
    if (text == "pin" || text == "inputparam" || text == "inputparameter") {
        return ScriptBridgeSlotKind::Pin;
    }
    if (text == "pout" || text == "outputparam" || text == "outputparameter") {
        return ScriptBridgeSlotKind::Pout;
    }
    if (text == "setting" || text == "settings") {
        return ScriptBridgeSlotKind::Setting;
    }
    if (text == "local" || text == "plocal" || text == "localparam" || text == "localparameter") {
        return ScriptBridgeSlotKind::Local;
    }
    return ScriptBridgeSlotKind::Standalone;
}

struct ScriptComponentSourceSelector {
    ScriptBridgeSlotKind Kind = ScriptBridgeSlotKind::Pout;
    std::string Name;
    std::string Prefix;
};

ScriptComponentSourceSelector ParseSourceSelector(const std::string &value) {
    ScriptComponentSourceSelector selector;
    selector.Name = StripQuotes(value);
    const std::size_t separator = selector.Name.find(':');
    if (separator != std::string::npos) {
        selector.Prefix = StripQuotes(selector.Name.substr(0, separator));
        selector.Kind = SlotKindFromText(selector.Prefix);
        selector.Name = StripQuotes(selector.Name.substr(separator + 1));
    }
    return selector;
}

std::string SourceSelectorKindName(const ScriptComponentSourceSelector &selector) {
    if (!selector.Prefix.empty()) {
        return selector.Prefix;
    }
    switch (selector.Kind) {
        case ScriptBridgeSlotKind::Pin: return "pin";
        case ScriptBridgeSlotKind::Pout: return "pout";
        case ScriptBridgeSlotKind::Local: return "local";
        default: return "source";
    }
}

BBDecl *CreateSpecFromParameter(ScriptBehaviorBridge *bridge,
                                const CKBehaviorContext &behcontext,
                                CKParameter *source,
                                std::string &error) {
    if (!bridge) {
        error = "BBDecl Component injection requires ScriptBehaviorBridge.";
        return nullptr;
    }
    if (!source) {
        error = "BBDecl Component parameter source is not available.";
        return nullptr;
    }

    BBBridge bb(bridge, behcontext);
    if (CKBehavior *behavior = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context))) {
        CKGUID guid = behavior->GetPrototypeGuid();
        if (CKGuidIsValid(guid)) {
            return bb.RequireGuid(guid);
        }
        error = "BBDecl Component parameter '" + SafeString(source->GetName()) +
                "' references behavior '" + SafeString(behavior->GetName()) +
                "' without a prototype GUID.";
        return nullptr;
    }

    std::string query;
    if (!ReadStringValue(source, query)) {
        error = "Failed to read BBDecl Component parameter '" + SafeString(source->GetName()) +
                "' as text. CK type=" + ParameterTypeLabel(behcontext.Context, source) + ".";
        return nullptr;
    }

    query = TrimString(query);
    if (query.empty()) {
        error = "BBDecl Component parameter '" + SafeString(source->GetName()) +
                "' is empty. Expected BB name, Category/Name, GUID, or CKBehavior object.";
        return nullptr;
    }

    BBDecl *spec = bb.Require(query);
    if (!spec || !spec->IsValid()) {
        error = spec ? spec->Error() : "Failed to create BBDecl.";
        if (spec) {
            spec->Release();
        }
        return nullptr;
    }
    return spec;
}

BBSlot *CreateSlotFromBinding(ScriptBehaviorBridge *bridge,
                              const CKBehaviorContext &behcontext,
                              CKParameter *source,
                              const ScriptComponentBinding &binding,
                              std::string &error) {
    if (!bridge) {
        error = "BBSlot Component injection requires ScriptBehaviorBridge.";
        return nullptr;
    }

    const ScriptBridgeSlotKind kind = SlotKindFromText(binding.SlotKindName);
    if (kind == ScriptBridgeSlotKind::Standalone) {
        error = "BBSlot Component binding has invalid or missing slot kind (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }
    if (binding.SlotName.empty()) {
        error = "BBSlot Component binding has no slotName (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }

    BBBridge bb(bridge, behcontext);
    BBDecl *spec = nullptr;
    if (!binding.SlotPrototypeName.empty()) {
        spec = bb.Require(binding.SlotPrototypeName);
    } else {
        spec = CreateSpecFromParameter(bridge, behcontext, source, error);
    }
    if (!spec) {
        return nullptr;
    }
    if (!spec->IsValid()) {
        error = spec->Error() + " (" + BindingSummary(binding, behcontext.Context) + ")";
        spec->Release();
        return nullptr;
    }

    BBSlot *slot = nullptr;
    switch (kind) {
        case ScriptBridgeSlotKind::Input: slot = spec->In(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Output: slot = spec->Out(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Pin: slot = spec->Pin(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Pout: slot = spec->Pout(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Setting: slot = spec->Setting(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Local: slot = spec->Local(binding.SlotName, binding.SlotOccurrence); break;
        default: break;
    }
    if (!slot || !slot->IsValid()) {
        error = (slot ? slot->Error() : "Failed to create BBSlot.") + " (" + BindingSummary(binding, behcontext.Context) + ")";
        if (slot) {
            slot->Release();
        }
        spec->Release();
        return nullptr;
    }

    spec->Release();
    return slot;
}

BBSlot *CreateSlotFromConfig(BBConfig *config,
                             const ScriptComponentBinding &binding,
                             const CKBehaviorContext &behcontext,
                             std::string &error) {
    if (!config || !config->IsValid()) {
        error = "BBSlot Component binding references unavailable BBConfig field '" +
                binding.SlotFromFieldName + "' (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }

    const ScriptBridgeSlotKind kind = SlotKindFromText(binding.SlotKindName);
    if (kind == ScriptBridgeSlotKind::Standalone) {
        error = "BBSlot Component binding has invalid or missing slot kind (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }
    if (binding.SlotName.empty()) {
        error = "BBSlot Component binding has no slotName (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }

    BBSlot *slot = nullptr;
    switch (kind) {
        case ScriptBridgeSlotKind::Input: slot = config->In(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Output: slot = config->Out(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Pin: slot = config->Pin(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Pout: slot = config->Pout(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Local: slot = config->Local(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Setting: {
            BBDecl *decl = config->Decl();
            slot = decl ? decl->Setting(binding.SlotName, binding.SlotOccurrence) : nullptr;
            if (decl) {
                decl->Release();
            }
            break;
        }
        default: break;
    }
    if (!slot || !slot->IsValid()) {
        error = (slot ? slot->Error() : "Failed to create BBSlot.") + " (" + BindingSummary(binding, behcontext.Context) + ")";
        if (slot) {
            slot->Release();
        }
        return nullptr;
    }
    return slot;
}

void ApplySlotBindingMetadata(BBSlot *slot, const ScriptComponentBinding &binding) {
    if (!slot) {
        return;
    }
    slot->SetMetadata(binding.SlotMetadataFlags, binding.DefaultValue, binding.SlotValue);
}

bool ApplyBBConfigSlotValues(BBConfig *bbinding,
                             const ScriptComponentBinding &binding,
                             std::string &error) {
    if (!bbinding) {
        error = "BBConfig metadata application requires a config.";
        return false;
    }
    for (const ScriptComponentNamedSlotValue &entry : binding.ConfigPinValues) {
        BBSlot *slot = bbinding->Pin(entry.Name, entry.Occurrence);
        if (!slot || !slot->IsValid()) {
            error = "BBConfig pin metadata failed for '" + entry.Name + "': " +
                    (slot ? slot->Error() : std::string("slot was not created")) + " (" + BindingSummary(binding, nullptr) + ")";
            if (slot) {
                slot->Release();
            }
            return false;
        }
        if (entry.HasValue && !bbinding->SetSlotString(slot, entry.Value)) {
            error = "BBConfig pin value failed for '" + entry.Name + "': " + bbinding->Error() + " (" + BindingSummary(binding, nullptr) + ")";
            slot->Release();
            return false;
        }
        slot->Release();
    }
    for (const ScriptComponentNamedSlotValue &entry : binding.ConfigSettingValues) {
        BBSlot *slot = bbinding->Setting(entry.Name, entry.Occurrence);
        if (!slot || !slot->IsValid()) {
            error = "BBConfig setting metadata failed for '" + entry.Name + "': " +
                    (slot ? slot->Error() : std::string("slot was not created")) + " (" + BindingSummary(binding, nullptr) + ")";
            if (slot) {
                slot->Release();
            }
            return false;
        }
        if (entry.HasValue && !bbinding->SetSettingString(slot, entry.Value)) {
            error = "BBConfig setting value failed for '" + entry.Name + "': " + bbinding->Error() + " (" + BindingSummary(binding, nullptr) + ")";
            slot->Release();
            return false;
        }
        slot->Release();
    }
    return true;
}

ParamRef *ResolveBBInstanceSourceRef(BBInstance *instance,
                                     const ScriptComponentSourceSelector &selector,
                                     const std::string &fieldName,
                                     int occurrence,
                                     std::string &error) {
    if (!instance) {
        error = "BBConfig source instance '" + fieldName + "' is not available.";
        return nullptr;
    }
    if (selector.Kind != ScriptBridgeSlotKind::Pin &&
        selector.Kind != ScriptBridgeSlotKind::Pout &&
        selector.Kind != ScriptBridgeSlotKind::Local) {
        error = "BBConfig source instance '" + fieldName +
                "' source selector '" + SourceSelectorKindName(selector) +
                "' must use pin:, pout:, or local:.";
        return nullptr;
    }

    BBSlot *slot = nullptr;
    ParamRef *ref = nullptr;
    switch (selector.Kind) {
        case ScriptBridgeSlotKind::Pin:
            slot = instance->PinSlot(selector.Name, occurrence);
            ref = slot ? instance->Pin(slot) : nullptr;
            break;
        case ScriptBridgeSlotKind::Pout:
            slot = instance->PoutSlot(selector.Name, occurrence);
            ref = slot ? instance->Pout(slot) : nullptr;
            break;
        case ScriptBridgeSlotKind::Local: {
            slot = instance->Local(selector.Name, occurrence);
            int index = -1;
            std::string slotError;
            if (slot && slot->ResolveIndex(ScriptBridgeSlotKind::Local, index, slotError)) {
                BehaviorRef *behavior = instance->Behavior();
                ref = behavior ? behavior->Local(index) : nullptr;
                if (behavior) {
                    behavior->Release();
                }
            } else if (!slotError.empty()) {
                error = slotError;
            }
            break;
        }
        default:
            break;
    }

    if (!ref && error.empty()) {
        error = "BBConfig source instance '" + fieldName + "' has no " +
                SourceSelectorKindName(selector) + " '" + selector.Name + "'.";
    }
    if (slot) {
        slot->Release();
    }
    return ref;
}

ParamRef *ResolveBBConfigSourceRef(ScriptComponentState *state,
                                   const CKBehaviorContext &behcontext,
                                   const ScriptComponentSourceSlot &source,
                                   std::string &error) {
    if (source.SourceSlotName.empty()) {
        ParamRef *ref = GetParamRefFieldByName(state, source.SourceFieldName);
        if (!ref) {
            error = "BBConfig source '" + source.SourceFieldName + "' is not a ParamRef@ field.";
        }
        return ref;
    }

    if (BehaviorRef *behavior = GetBehaviorRefFieldByName(state, source.SourceFieldName)) {
        const ScriptComponentSourceSelector selector = ParseSourceSelector(source.SourceSlotName);
        ParamRef *ref = nullptr;
        BehaviorLayout *layout = behavior->Layout();
        int index = -1;
        if (!layout) {
            error = "BBConfig source behavior '" + source.SourceFieldName + "' has no layout.";
        } else if (selector.Kind == ScriptBridgeSlotKind::Pin) {
            index = layout->FindPin(selector.Name, source.SourceOccurrence);
            ref = index >= 0 ? behavior->Pin(index) : nullptr;
        } else if (selector.Kind == ScriptBridgeSlotKind::Pout) {
            index = layout->FindPout(selector.Name, source.SourceOccurrence);
            ref = index >= 0 ? behavior->Pout(index) : nullptr;
        } else if (selector.Kind == ScriptBridgeSlotKind::Local) {
            index = layout->FindLocal(selector.Name, source.SourceOccurrence);
            ref = index >= 0 ? behavior->Local(index) : nullptr;
        } else {
            error = "BBConfig source behavior '" + source.SourceFieldName +
                    "' source selector '" + source.SourceSlotName +
                    "' must use pin:, pout:, or local:.";
        }

        if (!ref && error.empty()) {
            error = "BBConfig source behavior '" + source.SourceFieldName +
                    "' has no " + SourceSelectorKindName(selector) +
                    " '" + selector.Name + "'.";
        }
        if (layout) {
            layout->Release();
        }
        behavior->Release();
        return ref;
    }

    if (BBConfig *config = GetBBConfigFieldByName(state, source.SourceFieldName)) {
        BBInstance *instance = config->Instance();
        if (!instance) {
            const std::string configError = config->Error();
            error = "BBConfig source config '" + source.SourceFieldName +
                    "' has no live instance. Ensure the source config autostarts or call EnsureStarted() before binding sources.";
            if (!configError.empty()) {
                error += " Current config error: " + configError;
            }
            return nullptr;
        }
        const ScriptComponentSourceSelector selector = ParseSourceSelector(source.SourceSlotName);
        ParamRef *ref = ResolveBBInstanceSourceRef(instance, selector, source.SourceFieldName, source.SourceOccurrence, error);
        instance->Release();
        return ref;
    }

    BBInstance *instance = GetBBInstanceFieldByName(state, source.SourceFieldName);
    if (!instance) {
        error = "BBConfig source '" + source.SourceFieldName + "' is not a BehaviorRef@, BBConfig@, or BBInstance@ field.";
        return nullptr;
    }
    const ScriptComponentSourceSelector selector = ParseSourceSelector(source.SourceSlotName);
    ParamRef *ref = ResolveBBInstanceSourceRef(instance, selector, source.SourceFieldName, source.SourceOccurrence, error);
    instance->Release();
    return ref;
}

bool ApplyBBConfigSourceBindings(const CKBehaviorContext &behcontext,
                                 ScriptComponentState *state,
                                 const ScriptComponentBinding &binding,
                                 BBConfig *bbinding,
                                 bool requireResolved,
                                 std::string &error) {
    if (!bbinding) {
        error = "BBConfig source binding requires a config.";
        return false;
    }
    for (const ScriptComponentSourceSlot &source : binding.ConfigSources) {
        std::string sourceError;
        ParamRef *ref = ResolveBBConfigSourceRef(state, behcontext, source, sourceError);
        if (!ref) {
            if (requireResolved) {
                error = "BBConfig source '" + source.PinName + "<-" + source.SourceFieldName +
                        (source.SourceSlotName.empty() ? std::string() : "." + source.SourceSlotName) +
                        "' failed: " + sourceError + " (" + BindingSummary(binding, behcontext.Context) + ")";
                return false;
            }
            continue;
        }

        BBSlot *pin = bbinding->Pin(source.PinName, source.PinOccurrence);
        if (!pin || !pin->IsValid()) {
            error = "BBConfig source pin '" + source.PinName + "' failed: " +
                    (pin ? pin->Error() : std::string("slot was not created")) + " (" + BindingSummary(binding, behcontext.Context) + ")";
            if (pin) {
                pin->Release();
            }
            ref->Release();
            return false;
        }
        const bool ok = bbinding->SourceSlot(pin, ref) != nullptr;
        pin->Release();
        ref->Release();
        if (!ok) {
            error = "BBConfig source binding failed for pin '" + source.PinName + "': " + bbinding->Error();
            return false;
        }
    }
    return true;
}

BBConfig *CreateBBConfigFromBinding(ScriptBehaviorBridge *bridge,
                                      ScriptComponentState *state,
                                      const CKBehaviorContext &behcontext,
                                      CKParameter *source,
                                      const ScriptComponentBinding &binding,
                                      std::string &error) {
    if (!bridge) {
        error = "BBConfig Component injection requires ScriptBehaviorBridge.";
        return nullptr;
    }

    BBBridge bb(bridge, behcontext);
    BBDecl *spec = nullptr;
    if (!binding.SlotPrototypeName.empty()) {
        spec = bb.Require(binding.SlotPrototypeName);
    } else {
        spec = CreateSpecFromParameter(bridge, behcontext, source, error);
    }
    if (!spec) {
        return nullptr;
    }
    if (!spec->IsValid()) {
        error = spec->Error() + " (" + BindingSummary(binding, behcontext.Context) + ")";
        spec->Release();
        return nullptr;
    }

    BBConfig *bbinding = spec->Configure();
    spec->Release();
    if (!bbinding || !bbinding->IsValid()) {
        error = bbinding ? bbinding->Error() : "Failed to create BBConfig.";
        if (bbinding) {
            bbinding->Release();
        }
        return nullptr;
    }

    bbinding->SetComponentLifetime(UsesComponentLifetime(binding));
    bbinding->SetDefaultStart(binding.BindingStartInput);
    bbinding->SetDefaultStop(binding.BindingStopInput);
    if (!binding.BBConfigOwnerExpression.empty()) {
        bbinding->Owner(ResolveBBConfigObjectExpression(state, behcontext, binding.BBConfigOwnerExpression));
    }
    if (!binding.BBConfigTargetExpression.empty()) {
        bbinding->Target(ResolveBBConfigObjectExpression(state, behcontext, binding.BBConfigTargetExpression));
    }

    if (!ApplyBBConfigSlotValues(bbinding, binding, error)) {
        bbinding->Release();
        return nullptr;
    }

    for (const ScriptComponentRequiredSlot &required : binding.RequiredSlots) {
        const ScriptBridgeSlotKind kind = SlotKindFromText(required.KindName);
        if (kind == ScriptBridgeSlotKind::Standalone) {
            error = "BBConfig required slot has invalid kind '" + required.KindName + "' (" + BindingSummary(binding, behcontext.Context) + ").";
            bbinding->Release();
            return nullptr;
        }
        if (!bbinding->RequireSlot(kind, required.Name, required.Occurrence)) {
            error = "BBConfig required slot failed: " + bbinding->Error() + " (" + BindingSummary(binding, behcontext.Context) + ")";
            bbinding->Release();
            return nullptr;
        }
    }

    if (!ApplyBBConfigSourceBindings(behcontext, state, binding, bbinding, false, error)) {
        bbinding->Release();
        return nullptr;
    }

    return bbinding;
}

bool AssignComponentValueField(const ScriptParamValue &value,
                               void *propertyAddress,
                               const ScriptComponentBinding &binding,
                               std::string &error) {
    if (!propertyAddress) {
        error = "Component field address is not available: " + binding.FieldName;
        return false;
    }

    switch (value.Kind) {
        case ScriptParamValueKind::Int:
            if (binding.PropertyTypeId == asTYPEID_UINT32) {
                *static_cast<asUINT *>(propertyAddress) = static_cast<asUINT>(value.Data.IntValue);
            } else {
                *static_cast<int *>(propertyAddress) = value.Data.IntValue;
            }
            return true;
        case ScriptParamValueKind::Float:
            *static_cast<float *>(propertyAddress) = value.Data.FloatValue;
            return true;
        case ScriptParamValueKind::Bool:
            *static_cast<bool *>(propertyAddress) = value.Data.BoolValue;
            return true;
        case ScriptParamValueKind::String:
            *static_cast<std::string *>(propertyAddress) = value.Text();
            return true;
        case ScriptParamValueKind::Guid:
            *static_cast<CKGUID *>(propertyAddress) = value.Data.GuidValue;
            return true;
        case ScriptParamValueKind::Vector:
            *static_cast<VxVector *>(propertyAddress) = value.Data.VectorValue;
            return true;
        case ScriptParamValueKind::Vector2:
            *static_cast<Vx2DVector *>(propertyAddress) = value.Data.Vector2Value;
            return true;
        case ScriptParamValueKind::Color:
            *static_cast<VxColor *>(propertyAddress) = value.Data.ColorValue;
            return true;
        case ScriptParamValueKind::Quaternion:
            *static_cast<VxQuaternion *>(propertyAddress) = value.Data.QuaternionValue;
            return true;
        case ScriptParamValueKind::Matrix:
            *static_cast<VxMatrix *>(propertyAddress) = value.Data.MatrixValue;
            return true;
        case ScriptParamValueKind::ObjectArray: {
            XObjectArray *array = static_cast<XObjectArray *>(propertyAddress);
            array->Clear();
            for (CK_ID id : value.ObjectIds()) {
                array->PushBack(id);
            }
            return true;
        }
        default:
            error = "Unsupported Component binding kind for field: " + binding.FieldName;
            return false;
    }
}

bool InjectComponentParameters(const CKBehaviorContext &behcontext,
                               ScriptComponentState *state,
                               bool initial,
                               std::string &error) {
    CKBehavior *beh = behcontext.Behavior;
    if (!beh || !state || !state->Object) {
        error = "Component object is not ready for parameter injection.";
        return false;
    }

    ScriptBehaviorBridge *bridge = nullptr;
    ScriptManager *man = ScriptManager::GetManager(behcontext.Context);
    if (man) {
        bridge = man->GetBehaviorBridge();
    }
    asIScriptEngine *engine = state->Invoker && state->Invoker->GetModule() ? state->Invoker->GetModule()->GetEngine() : nullptr;

    for (ScriptComponentBinding &binding : state->Bindings) {
        if (!initial && !binding.InjectEveryFrame) {
            continue;
        }

        CKParameterIn *input = binding.InputParameterIndex >= 0 && binding.InputParameterIndex < beh->GetInputParameterCount()
            ? beh->GetInputParameter(binding.InputParameterIndex)
            : nullptr;
        if (!input) {
            error = "Component input parameter is not available: " + binding.ParameterName;
            return false;
        }

        CKParameter *source = input->GetRealSource();
        if (!source) {
            error = "Component input parameter has no source: " + binding.ParameterName;
            return false;
        }

        void *propertyAddress = state->Object->GetAddressOfProperty(static_cast<asUINT>(binding.PropertyIndex));
        if (!propertyAddress) {
            error = "Component field address is not available: " + binding.FieldName;
            return false;
        }

        const ScriptParamValueKind valueKind = ValueKindFromComponentKind(binding.Kind);
        if (valueKind != ScriptParamValueKind::Empty) {
            ScriptParamValue value;
            std::string readError;
            if (!ReadParameterValueAs(source, valueKind, value, readError)) {
                error = readError + " (" + binding.ParameterName + ")";
                return false;
            }
            if (!AssignComponentValueField(value, propertyAddress, binding, error)) {
                return false;
            }
            continue;
        }

        switch (binding.Kind) {
            case ScriptComponentBindingKind::Int: {
                int value = 0;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read int Component parameter: " + binding.ParameterName;
                    return false;
                }
                if (binding.PropertyTypeId == asTYPEID_UINT32) {
                    *static_cast<asUINT *>(propertyAddress) = static_cast<asUINT>(value);
                } else {
                    *static_cast<int *>(propertyAddress) = value;
                }
                break;
            }
            case ScriptComponentBindingKind::Float: {
                float value = 0.0f;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read float Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<float *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Bool: {
                CKBOOL value = FALSE;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read bool Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<bool *>(propertyAddress) = value != FALSE;
                break;
            }
            case ScriptComponentBindingKind::String: {
                std::string value;
                if (!ReadStringValue(source, value)) {
                    error = "Failed to read string Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<std::string *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Guid: {
                std::string text;
                if (!ReadStringValue(source, text)) {
                    error = "Failed to read CKGUID Component parameter: " + binding.ParameterName;
                    return false;
                }
                CKGUID value;
                if (!TrimString(text).empty() && !ParseScriptGuidString(text, value)) {
                    error = "Failed to parse CKGUID Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<CKGUID *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Vector: {
                VxVector value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read VxVector Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<VxVector *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Vector2: {
                Vx2DVector value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read Vx2DVector Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<Vx2DVector *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Color: {
                VxColor value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read VxColor Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<VxColor *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Quaternion: {
                VxQuaternion value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read VxQuaternion Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<VxQuaternion *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Matrix: {
                VxMatrix value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read VxMatrix Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<VxMatrix *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Object: {
                CKObject *objectValue = ReadObjectValue(source, behcontext.Context);
                if (!ValidateObjectFieldValue(engine, binding, objectValue, error)) {
                    return false;
                }
                if (!AssignObjectHandle(state->Object, binding.PropertyIndex, objectValue)) {
                    error = "Failed to assign object Component field: " + binding.FieldName;
                    return false;
                }
                break;
            }
            case ScriptComponentBindingKind::ParamRef: {
                if (!bridge) {
                    error = "ParamRef Component injection requires ScriptBehaviorBridge.";
                    return false;
                }
                const CK_ID inputId = input->GetID();
                if (!initial && binding.HandleInjected && binding.LastObjectId == inputId) {
                    break;
                }
                ParamRef *ref = new ParamRef(bridge, inputId, ScriptBridgeSlotKind::Pin, binding.InputParameterIndex, state->BehaviorId);
                if (!AssignRefCountedHandle<ParamRef>(state->Object, binding.PropertyIndex, ref)) {
                    error = "Failed to assign ParamRef Component field: " + binding.FieldName;
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastObjectId = inputId;
                break;
            }
            case ScriptComponentBindingKind::ParamValue: {
                std::string readError;
                ScriptParamValue readValue = ReadParameterValue(source, &readError);
                if (readValue.Kind == ScriptParamValueKind::Empty && !readError.empty()) {
                    error = readError + " (" + binding.ParameterName + ")";
                    return false;
                }
                readValue.TypeGuid = source->GetGUID();
                readValue.Type = source->GetType();
                ParamValue *value = new ParamValue(readValue);
                if (!AssignRefCountedHandle<ParamValue>(state->Object, binding.PropertyIndex, value)) {
                    error = "Failed to assign ParamValue Component field: " + binding.FieldName;
                    return false;
                }
                binding.HandleInjected = true;
                break;
            }
            case ScriptComponentBindingKind::ParamTypeInfo: {
                const CKGUID sourceGuid = source->GetGUID();
                const std::string sourceGuidText = GuidToString(sourceGuid);
                if (!initial && binding.HandleInjected && binding.LastTextValue == sourceGuidText) {
                    break;
                }

                ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(behcontext.Context);
                const ScriptParamTypeRecord *record = registry ? registry->GetType(sourceGuid) : nullptr;
                if (!registry || !record) {
                    error = "Failed to resolve ParamTypeInfo Component field (" + BindingSummary(binding, behcontext.Context) +
                            "). Source parameter='" + SafeString(source->GetName()) +
                            "', source CK type=" + ParameterTypeLabel(behcontext.Context, source) +
                            " " + sourceGuidText + ".";
                    return false;
                }

                ParamTypeInfo *info = new ParamTypeInfo(registry, record->Type);
                if (!AssignRefCountedHandle<ParamTypeInfo>(state->Object, binding.PropertyIndex, info)) {
                    error = "Failed to assign ParamTypeInfo Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastTextValue = sourceGuidText;
                break;
            }
            case ScriptComponentBindingKind::BehaviorRef: {
                CKBehavior *target = ReadBehaviorValue(source, behcontext.Context);
                const CK_ID targetId = target ? target->GetID() : 0;
                if (!initial && binding.HandleInjected && binding.LastObjectId == targetId) {
                    break;
                }
                if (!bridge && target) {
                    error = "BehaviorRef Component injection requires ScriptBehaviorBridge.";
                    return false;
                }
                BehaviorRef *ref = bridge && target ? bridge->WrapBehavior(target, state->BehaviorId) : nullptr;
                if (!AssignBehaviorRefHandle(bridge, state->Object, binding.PropertyIndex, ref)) {
                    error = "Failed to assign BehaviorRef Component field: " + binding.FieldName;
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastObjectId = targetId;
                break;
            }
            case ScriptComponentBindingKind::BBPrototype: {
                CKBehavior *behaviorSource = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context));
                std::string sourceText;
                if (!behaviorSource) {
                    ReadStringValue(source, sourceText);
                    sourceText = TrimString(sourceText);
                }
                const CK_ID sourceId = behaviorSource ? behaviorSource->GetID() : 0;
                if (!initial && binding.HandleInjected && binding.LastObjectId == sourceId && binding.LastTextValue == sourceText) {
                    break;
                }

                if (!bridge && (behaviorSource || !sourceText.empty())) {
                    error = "BBPrototype Component injection requires ScriptBehaviorBridge (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                std::string prototypeError;
                BBPrototype *prototype = CreatePrototypeFromParameter(bridge, behcontext, source, prototypeError);
                if (!prototypeError.empty()) {
                    error = prototypeError + " (" + BindingSummary(binding, behcontext.Context) + ")";
                    return false;
                }
                if (!AssignBBPrototypeHandle(bridge, state->Object, binding.PropertyIndex, prototype)) {
                    error = "Failed to assign BBPrototype Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastObjectId = sourceId;
                binding.LastTextValue = sourceText;
                break;
            }
            case ScriptComponentBindingKind::BBDecl: {
                CKBehavior *behaviorSource = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context));
                std::string sourceText;
                if (!behaviorSource) {
                    ReadStringValue(source, sourceText);
                    sourceText = TrimString(sourceText);
                }
                const CK_ID sourceId = behaviorSource ? behaviorSource->GetID() : 0;
                if (!initial && binding.HandleInjected && binding.LastObjectId == sourceId && binding.LastTextValue == sourceText) {
                    break;
                }

                std::string specError;
                BBDecl *spec = CreateSpecFromParameter(bridge, behcontext, source, specError);
                if (!spec) {
                    error = specError + " (" + BindingSummary(binding, behcontext.Context) + ")";
                    return false;
                }
                if (!AssignRefCountedHandle<BBDecl>(state->Object, binding.PropertyIndex, spec)) {
                    error = "Failed to assign BBDecl Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastObjectId = sourceId;
                binding.LastTextValue = sourceText;
                break;
            }
            case ScriptComponentBindingKind::BBSlot: {
                std::string sourceText;
                ReadStringValue(source, sourceText);
                sourceText = TrimString(sourceText);
                const std::string cacheText = sourceText + "|" + binding.SlotPrototypeName + "|" +
                    binding.SlotFromFieldName + "|" + binding.SlotKindName + "|" + binding.SlotName + "|" +
                    std::to_string(binding.SlotOccurrence) + "|" + std::to_string(binding.SlotMetadataFlags) + "|" +
                    binding.DefaultValue + "|" + binding.SlotValue;
                if (!initial && binding.HandleInjected && binding.LastTextValue == cacheText) {
                    break;
                }

                std::string slotError;
                BBConfig *ownerConfig = nullptr;
                BBSlot *slot = nullptr;
                if (!binding.SlotFromFieldName.empty()) {
                    ownerConfig = GetBBConfigFieldByName(state, binding.SlotFromFieldName);
                    slot = CreateSlotFromConfig(ownerConfig, binding, behcontext, slotError);
                } else {
                    slot = CreateSlotFromBinding(bridge, behcontext, source, binding, slotError);
                }
                if (!slot) {
                    error = slotError;
                    return false;
                }
                ApplySlotBindingMetadata(slot, binding);
                if (ownerConfig && !ownerConfig->RegisterSlot(slot)) {
                    error = "Failed to register BBSlot with BBConfig '" + binding.SlotFromFieldName + "': " +
                            ownerConfig->Error() + " (" + BindingSummary(binding, behcontext.Context) + ")";
                    slot->Release();
                    return false;
                }
                if (!AssignRefCountedHandle<BBSlot>(state->Object, binding.PropertyIndex, slot)) {
                    error = "Failed to assign BBSlot Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastTextValue = cacheText;
                break;
            }
            case ScriptComponentBindingKind::BBConfig: {
                CKBehavior *behaviorSource = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context));
                std::string sourceText;
                if (!behaviorSource) {
                    ReadStringValue(source, sourceText);
                    sourceText = TrimString(sourceText);
                }
                const CK_ID sourceId = behaviorSource ? behaviorSource->GetID() : 0;
                const std::string cacheText = BuildBBConfigBindingCacheText(binding, sourceId, sourceText);
                if (!initial && binding.HandleInjected && binding.LastTextValue == cacheText) {
                    break;
                }

                std::string bindingError;
                BBConfig *bbinding = CreateBBConfigFromBinding(bridge, state, behcontext, source, binding, bindingError);
                if (!bbinding) {
                    error = bindingError;
                    return false;
                }
                if (!AssignRefCountedHandle<BBConfig>(state->Object, binding.PropertyIndex, bbinding)) {
                    error = "Failed to assign BBConfig Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.BBConfigChanged = true;
                binding.LastObjectId = sourceId;
                binding.LastTextValue = cacheText;
                break;
            }
            default:
                error = "Unsupported Component binding kind for field: " + binding.FieldName;
                return false;
        }
    }

    return true;
}


} // namespace AngelScriptComponentInternal
