#ifndef CK_SCRIPT_COMPONENT_STATE_H
#define CK_SCRIPT_COMPONENT_STATE_H

#include <string>
#include <vector>

#include <angelscript.h>

#include "CKAll.h"

class CKBehavior;
class ScriptInvoker;

enum class ScriptComponentBindingKind {
    Auto,
    Int,
    Float,
    Bool,
    String,
    Guid,
    Vector,
    Vector2,
    Color,
    Quaternion,
    Matrix,
    ObjectArray,
    Object,
    ParamRef,
    ParamValue,
    ParamTypeInfo,
    BehaviorRef,
    BBPrototype,
    BBDecl,
    BBSlot,
    BBConfig
};

struct ScriptComponentRequiredSlot {
    std::string KindName;
    std::string Name;
    int Occurrence = 0;
};

struct ScriptComponentNamedSlotValue {
    std::string Name;
    std::string Value;
    int Occurrence = 0;
    bool HasValue = false;
};

struct ScriptComponentSourceSlot {
    std::string PinName;
    std::string SourceFieldName;
    std::string SourceSlotName;
    int PinOccurrence = 0;
    int SourceOccurrence = 0;
};

enum class ScriptComponentBBStepPolicy {
    Manual,
    EachUpdate,
    OnChange
};

enum class ScriptComponentBBConfigLifetime {
    Component,
    Manual
};

struct ScriptComponentBinding {
    std::string FieldName;
    std::string ParameterName;
    std::string TypeName;
    std::string DefaultValue;
    bool HasDefault = false;
    bool InjectEveryFrame = true;
    bool HandleInjected = false;
    CK_ID LastObjectId = 0;
    std::string LastTextValue;
    std::string SlotFromFieldName;
    std::string SlotPrototypeName;
    std::string SlotKindName;
    std::string SlotName;
    int SlotOccurrence = 0;
    CKDWORD SlotMetadataFlags = 0;
    std::string SlotValue;
    ScriptComponentBBConfigLifetime BBConfigLifetime = ScriptComponentBBConfigLifetime::Component;
    bool HasBBConfigLifetime = false;
    std::string BindingStartInput;
    std::string BindingStopInput;
    std::vector<ScriptComponentRequiredSlot> RequiredSlots;
    std::vector<ScriptComponentNamedSlotValue> ConfigPinValues;
    std::vector<ScriptComponentNamedSlotValue> ConfigSettingValues;
    std::vector<ScriptComponentSourceSlot> ConfigSources;
    std::string BBConfigOwnerExpression;
    std::string BBConfigTargetExpression;
    ScriptComponentBBStepPolicy BBStepPolicy = ScriptComponentBBStepPolicy::Manual;
    bool AutoStartBBConfig = false;
    bool HasAutoStartBBConfig = false;
    bool HasBBStepPolicy = false;
    bool BBConfigChanged = false;
    std::string MetadataError;

    ScriptComponentBindingKind Kind = ScriptComponentBindingKind::Auto;
    CKGUID ParameterGuid;
    int PropertyIndex = -1;
    int PropertyTypeId = 0;
    int InputParameterIndex = -1;
};

struct ScriptComponentState {
    CK_ID BehaviorId = 0;
    CKBehavior *Behavior = nullptr;
    ScriptInvoker *Invoker = nullptr;
    asIScriptObject *Object = nullptr;

    asIScriptFunction *OnLoad = nullptr;
    asIScriptFunction *Awake = nullptr;
    asIScriptFunction *OnEnable = nullptr;
    asIScriptFunction *Start = nullptr;
    asIScriptFunction *Update = nullptr;
    asIScriptFunction *OnDisable = nullptr;
    asIScriptFunction *OnDestroy = nullptr;
    asIScriptFunction *OnReset = nullptr;
    asIScriptFunction *OnMessage = nullptr;
    asIScriptFunction *ActiveLifecycle = nullptr;
    std::string ActiveLifecycleName;

    std::string ScriptName;
    std::string ClassName;
    std::string Source;
    std::string File;
    std::string Manifest;
    std::string RuntimeModuleName;
    std::string MessageTarget;
    std::vector<ScriptComponentBinding> Bindings;
    std::vector<std::string> MessageTopics;
    std::vector<std::string> ManagedInputParameterNames;

    bool PrivateModule = false;
    bool Loaded = false;
    bool StaticMessageSubscriptionsRegistered = false;
    bool OnLoadCalled = false;
    bool AwakeCalled = false;
    bool StartCalled = false;
    bool DesiredEnabled = false;
    bool InstanceEnabled = false;
    bool ScriptActive = false;
    bool Paused = false;
    bool Failed = false;
    bool PendingDestroy = false;
    bool PendingDisableOutput = false;
    bool PendingResetRuntime = false;
};

#endif // CK_SCRIPT_COMPONENT_STATE_H
