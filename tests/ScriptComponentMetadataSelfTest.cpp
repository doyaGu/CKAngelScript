#include "ScriptSelfTests.h"

#include <cstring>
#include <string>
#include <vector>

#include "CKAll.h"
#include "ScriptAsync.h"
#include "ScriptBridgeCommon.h"
#include "ScriptComponentMetadata.h"
#include "ScriptComponentState.h"
#include "ScriptManager.h"
#include "ScriptMessage.h"

namespace {

constexpr int COMPONENT_PARAM_SCRIPT = 0;
constexpr int COMPONENT_PARAM_CLASS = 1;
constexpr int COMPONENT_PARAM_SOURCE = 2;
constexpr int COMPONENT_PARAM_FILE = 3;
constexpr int COMPONENT_PARAM_MANIFEST = 4;

CKBehaviorContext MakeComponentSelfTestContext(CKContext *context, CKBehavior *behavior) {
    CKBehaviorContext behaviorContext = {};
    behaviorContext.Behavior = behavior;
    behaviorContext.DeltaTime = 0.0f;
    behaviorContext.Context = context;
    behaviorContext.ParameterManager = context ? context->GetParameterManager() : nullptr;
    behaviorContext.MessageManager = context ? context->GetMessageManager() : nullptr;
    behaviorContext.AttributeManager = context ? context->GetAttributeManager() : nullptr;
    behaviorContext.TimeManager = context ? context->GetTimeManager() : nullptr;
    return behaviorContext;
}

void DestroyComponentSelfTestObject(CKContext *context, CKObject *object) {
    if (context && object && !object->IsToBeDeleted()) {
        context->DestroyObject(object);
    }
}

bool SetComponentStringInput(CKBehavior *behavior, int index, const char *value, std::string &error) {
    if (!behavior || index < 0 || index >= behavior->GetInputParameterCount()) {
        error = "Component runtime self-test could not resolve string input parameter.";
        return false;
    }

    CKParameterIn *input = behavior->GetInputParameter(index);
    const std::string localName = "__CKAS_ComponentFrameSelfTest_" + std::to_string(index);
    CKParameterLocal *local = behavior->CreateLocalParameter(const_cast<CKSTRING>(localName.c_str()), CKPGUID_STRING);
    if (!input || !local) {
        error = "Component runtime self-test failed to create input source.";
        return false;
    }

    if (local->SetStringValue(const_cast<CKSTRING>(value ? value : "")) != CK_OK) {
        error = "Component runtime self-test failed to write input source.";
        return false;
    }
    behavior->SetInputParameterDefaultValue(input, local);
    if (input->SetDirectSource(local) != CK_OK) {
        error = "Component runtime self-test failed to write input source.";
        return false;
    }
    return true;
}

int ExecuteComponentSelfTestBehavior(CKContext *context, CKBehavior *behavior, const CKBehaviorContext &behaviorContext) {
    if (!behavior) {
        return CKBR_BEHAVIORERROR;
    }

#if CKVERSION == 0x13022002
    CKBehaviorContext savedContext = context->m_BehaviorContext;
    CKBehaviorManager *behaviorManager = context ? context->GetBehaviorManager() : nullptr;
    CKBehavior *savedCurrent = behaviorManager ? behaviorManager->m_CurrentBehavior : nullptr;

    context->m_BehaviorContext = behaviorContext;
    const int rc = behavior->Execute(behaviorContext.DeltaTime);

    context->m_BehaviorContext = savedContext;
    if (behaviorManager) {
        behaviorManager->m_CurrentBehavior = savedCurrent;
    }
    return rc;
#else
    return behavior->Execute(behaviorContext.DeltaTime);
#endif
}

bool ReadIntProperty(asIScriptObject *object, const char *name, int &value, std::string &error) {
    if (!object || !name || name[0] == '\0') {
        error = "Component runtime self-test script object is not available.";
        return false;
    }

    asITypeInfo *type = object->GetObjectType();
    if (!type) {
        error = "Component runtime self-test script object has no type.";
        return false;
    }

    for (asUINT i = 0; i < type->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        int typeId = 0;
        if (type->GetProperty(i, &propertyName, &typeId) < 0 || !propertyName) {
            continue;
        }
        if (std::strcmp(propertyName, name) != 0) {
            continue;
        }
        if (typeId != asTYPEID_INT32) {
            error = std::string("Component runtime self-test property has unexpected type: ") + name;
            return false;
        }
        void *address = object->GetAddressOfProperty(i);
        if (!address) {
            error = std::string("Component runtime self-test property address is null: ") + name;
            return false;
        }
        value = *static_cast<int *>(address);
        return true;
    }

    error = std::string("Component runtime self-test property not found: ") + name;
    return false;
}

bool ExpectIntProperty(asIScriptObject *object,
                       const char *name,
                       int expected,
                       std::string &error) {
    int actual = 0;
    if (!ReadIntProperty(object, name, actual, error)) {
        return false;
    }
    if (actual != expected) {
        error = std::string("Component runtime self-test property '") + name +
                "' expected " + std::to_string(expected) +
                " but got " + std::to_string(actual) + ".";
        return false;
    }
    return true;
}

bool RunComponentBehaviorFrameRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Component runtime self-test requires CKContext and AngelScript engine.";
        return false;
    }

    ScriptManager *manager = ScriptManager::GetManager(context);
    if (!manager) {
        error = "Component runtime self-test could not retrieve ScriptManager.";
        return false;
    }

    const char *source =
        "class ComponentFrameRuntimeProbe {\n"
        "  int startCount = 0;\n"
        "  int updateCount = 0;\n"
        "  int messageCount = 0;\n"
        "  int legacyUpdateCount = 0;\n"
        "  int legacyMessageCount = 0;\n"
        "  int frameError = 0;\n"
        "  void Validate(BehaviorFrame@ frame, int code) {\n"
        "    if (frame is null) { frameError = code; return; }\n"
        "    if (frame.Behavior() is null) { frameError = code + 1; return; }\n"
        "    if (frame.Context() is null) { frameError = code + 2; return; }\n"
        "    if (frame.PinInt(\"Missing\", code + 3) != code + 3) { frameError = code + 3; return; }\n"
        "  }\n"
        "  void Start(BehaviorFrame@ frame) { Validate(frame, 100); startCount++; }\n"
        "  void Update(BehaviorFrame@ frame) { Validate(frame, 200); updateCount++; }\n"
        "  void Update(const CKBehaviorContext &in ctx) { legacyUpdateCount++; }\n"
        "  void OnMessage(const ScriptMessage &in msg, BehaviorFrame@ frame) {\n"
        "    Validate(frame, 300);\n"
        "    if (frameError != 0) return;\n"
        "    if (msg.Topic() != \"component.frame.runtime\") { frameError = 399; return; }\n"
        "    AsyncTask<void>@ delay = Async::Delay(1);\n"
        "    Await(delay);\n"
        "    Validate(frame, 310);\n"
        "    if (frameError != 0) return;\n"
        "    if (msg.Topic() == \"component.frame.runtime\") messageCount++; else frameError = 398;\n"
        "  }\n"
        "  void OnMessage(const ScriptMessage &in msg, const CKBehaviorContext &in ctx) { legacyMessageCount++; }\n"
        "}\n";

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ComponentFrameSelfTestBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    bool deleteCallbackSent = false;
    CKBehaviorContext behaviorContext = {};
    auto cleanup = [&]() {
        if (behavior && deleteCallbackSent) {
            CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE, &behaviorContext);
        }
        DestroyComponentSelfTestObject(context, behavior);
    };

    if (!behavior) {
        cleanup();
        error = "Component runtime self-test failed to create behavior.";
        return false;
    }

    behavior->UseFunction();
    const CKERROR initError = behavior->InitFromGuid(CKGUID(0x5f5d4a84, 0x3dfd4d19));
    if (initError != CK_OK) {
        cleanup();
        error = "Component runtime self-test failed to initialize AngelScript Component behavior.";
        return false;
    }

    if (!SetComponentStringInput(behavior, COMPONENT_PARAM_SCRIPT, "__ckas_component_frame_runtime_selftest", error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_CLASS, "ComponentFrameRuntimeProbe", error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_SOURCE, source, error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_FILE, "", error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_MANIFEST, "", error)) {
        cleanup();
        return false;
    }

    behaviorContext = MakeComponentSelfTestContext(context, behavior);
    const CKERROR createError = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORCREATE, &behaviorContext);
    if (createError != CK_OK) {
        cleanup();
        error = "Component runtime self-test CREATE callback failed.";
        return false;
    }
    deleteCallbackSent = true;

    behavior->ActivateInput(0, TRUE);
    const int returnCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (returnCode != CKBR_ACTIVATENEXTFRAME) {
        cleanup();
        error = "Component runtime self-test returned unexpected code " + std::to_string(returnCode) + ".";
        return false;
    }

    ScriptComponentState *state = manager->GetComponentState(behavior->GetID());
    if (!state || !state->Loaded || !state->InstanceEnabled || !state->Object) {
        cleanup();
        error = "Component runtime self-test did not create an enabled component instance.";
        return false;
    }
    if (state->StartArgument != ScriptBehaviorArgumentKind::BehaviorFrame ||
        state->UpdateArgument != ScriptBehaviorArgumentKind::BehaviorFrame ||
        state->OnMessageArgument != ScriptBehaviorArgumentKind::BehaviorFrame) {
        cleanup();
        error = "Component runtime self-test did not cache BehaviorFrame lifecycle/message methods.";
        return false;
    }

    ScriptMessage message(1,
                          ScriptMessageKind::Direct,
                          "component.frame.runtime",
                          "component-runtime-self-test",
                          ScriptMessageTarget(),
                          0,
                          false,
                          nullptr);
    std::string messageError;
    if (!manager->DeliverComponentMessage(behavior->GetID(), message, true, messageError)) {
        cleanup();
        error = messageError.empty()
            ? "Component runtime self-test message delivery failed."
            : "Component runtime self-test message delivery failed: " + messageError;
        return false;
    }
    if (!state->Invoker->IsContextSuspended() || state->ActiveLifecycleName != "OnMessage") {
        cleanup();
        error = "Component runtime self-test expected OnMessage to suspend explicitly.";
        return false;
    }

    ScriptAsyncScheduler *scheduler = manager->GetAsyncScheduler();
    if (!scheduler) {
        cleanup();
        error = "Component runtime self-test could not retrieve async scheduler.";
        return false;
    }
    scheduler->Tick();

    const int resumeCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (resumeCode != CKBR_ACTIVATENEXTFRAME) {
        cleanup();
        error = "Component runtime self-test returned unexpected message resume code " + std::to_string(resumeCode) + ".";
        return false;
    }
    if (state->Invoker->IsContextSuspended() || state->ActiveLifecycleName == "OnMessage") {
        cleanup();
        error = "Component runtime self-test did not finish suspended OnMessage.";
        return false;
    }

    asIScriptObject *object = state->Object;
    if (!ExpectIntProperty(object, "startCount", 1, error) ||
        !ExpectIntProperty(object, "updateCount", 1, error) ||
        !ExpectIntProperty(object, "messageCount", 1, error) ||
        !ExpectIntProperty(object, "legacyUpdateCount", 0, error) ||
        !ExpectIntProperty(object, "legacyMessageCount", 0, error) ||
        !ExpectIntProperty(object, "frameError", 0, error)) {
        cleanup();
        return false;
    }

    cleanup();
    error.clear();
    return true;
}

} // namespace

bool RunScriptComponentMetadataSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Component metadata self-test requires CKContext and AngelScript engine.";
        return false;
    }

    const int constEntityHandleType = engine->GetTypeIdByDecl("const CK3dEntity@");
    if (constEntityHandleType < 0) {
        error = "Component metadata self-test could not resolve const CK3dEntity@.";
        return false;
    }
    if (ScriptComponentSupport::InferKindFromProperty(engine, constEntityHandleType) != ScriptComponentBindingKind::Object) {
        error = "Component metadata self-test did not infer const CK object handles as Object bindings.";
        return false;
    }
    std::string expected;
    if (!ScriptComponentSupport::IsCompatiblePropertyType(engine, constEntityHandleType, ScriptComponentBindingKind::Object, expected)) {
        error = "Component metadata self-test did not accept const CK object handles as Object fields.";
        return false;
    }
    if (ScriptComponentSupport::ClassIdFromPropertyType(engine, constEntityHandleType) != CKCID_3DENTITY) {
        error = "Component metadata self-test lost const CK object handle class identity.";
        return false;
    }
    if (ScriptComponentSupport::GuidFromPropertyType(context, engine, constEntityHandleType) != CKPGUID_3DENTITY) {
        error = "Component metadata self-test lost const CK object handle parameter GUID.";
        return false;
    }

    std::vector<ScriptComponentBinding> bindings;
    auto addMetadata = [&](const std::string &metadata) -> bool {
        ScriptComponentBinding binding;
        if (!ScriptComponentSupport::ParseBindingMetadata(metadata, "TextConfig", binding)) {
            error = "Component metadata self-test failed to parse '" + metadata + "'.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(bindings, binding);
        return true;
    };

    if (!addMetadata("bbconfig prototype=\"Interface/Text/2D Text\" lifetime=\"component\"") ||
        !addMetadata("bbsetting \"Text Properties\"=\"Screen Proportionnal,WordWrap\"") ||
        !addMetadata("bbpin \"Text\"=\"FPS: ...\"") ||
        !addMetadata("bbsource \"Font\"=\"FontConfig.Font Created\"") ||
        !addMetadata("bboutput \"Out\"") ||
        !addMetadata("bbpout \"Rendered\"")) {
        return false;
    }

    if (bindings.size() != 1) {
        error = "Component metadata self-test did not merge stacked metadata.";
        return false;
    }
    const ScriptComponentBinding &binding = bindings.front();
    if (binding.Kind != ScriptComponentBindingKind::BBConfig ||
        binding.FieldName != "TextConfig" ||
        binding.SlotPrototypeName != "Interface/Text/2D Text" ||
        binding.BBConfigLifetime != ScriptComponentBBConfigLifetime::Component ||
        binding.ConfigPinValues.size() != 1 ||
        binding.ConfigPinValues[0].Name != "Text" ||
        binding.ConfigPinValues[0].Value != "FPS: ..." ||
        binding.ConfigSettingValues.size() != 1 ||
        binding.ConfigSettingValues[0].Name != "Text Properties" ||
        binding.ConfigSettingValues[0].Value != "Screen Proportionnal,WordWrap" ||
        binding.ConfigSources.size() != 1 ||
        binding.ConfigSources[0].PinName != "Font" ||
        binding.ConfigSources[0].SourceFieldName != "FontConfig" ||
        binding.ConfigSources[0].SourceSlotName != "Font Created") {
        error = "Component metadata self-test merged BBConfig fields incorrectly.";
        return false;
    }

    bool sawOutput = false;
    bool sawPout = false;
    bool sawFontPin = false;
    bool sawSetting = false;
    for (const ScriptComponentRequiredSlot &slot : binding.RequiredSlots) {
        sawOutput = sawOutput || (slot.KindName == "output" && slot.Name == "Out");
        sawPout = sawPout || (slot.KindName == "pout" && slot.Name == "Rendered");
        sawFontPin = sawFontPin || (slot.KindName == "pin" && slot.Name == "Font");
        sawSetting = sawSetting || (slot.KindName == "setting" && slot.Name == "Text Properties");
    }
    if (!sawOutput || !sawPout || !sawFontPin || !sawSetting) {
        error = "Component metadata self-test missed required slot fragments.";
        return false;
    }

    std::vector<ScriptComponentBinding> repeatedSlotBindings;
    for (const std::string &fieldName : {std::string("FirstConfig"), std::string("SecondConfig")}) {
        ScriptComponentBinding configBinding;
        if (!ScriptComponentSupport::ParseBindingMetadata(
                "bbconfig prototype=\"Logics/Calculator/Identity\"",
                fieldName,
                configBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot config metadata.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(repeatedSlotBindings, configBinding);

        ScriptComponentBinding slotBinding;
        if (!ScriptComponentSupport::ParseBindingMetadata("bbpout \"pOut 0\"", fieldName, slotBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot fragment metadata.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(repeatedSlotBindings, slotBinding);
    }
    if (repeatedSlotBindings.size() != 2 ||
        repeatedSlotBindings[0].ParameterName != "FirstConfig" ||
        repeatedSlotBindings[1].ParameterName != "SecondConfig") {
        error = "Component metadata self-test let BBConfig fragment slot names override field parameter names.";
        return false;
    }

    ScriptComponentBinding legacyManaged;
    if (!ScriptComponentSupport::ParseBindingMetadata(
            "bbconfig prototype=\"Interface/Text/2D Text\" managed=true",
            "LegacyConfig",
            legacyManaged)) {
        error = "Component metadata self-test failed to parse legacy managed= diagnostic metadata.";
        return false;
    }
    if (legacyManaged.MetadataError.find("lifetime=\"component\"") == std::string::npos ||
        legacyManaged.MetadataError.find("lifetime=\"manual\"") == std::string::npos) {
        error = "Component metadata self-test did not reject managed= with a lifetime replacement diagnostic.";
        return false;
    }

    std::vector<ScriptComponentBinding> aggregateBindings;
    auto addAggregateMetadata = [&](const std::string &metadata) -> bool {
        ScriptComponentBinding aggregateBinding;
        if (!ScriptComponentSupport::ParseBindingMetadata(metadata, "AggregateConfig", aggregateBinding)) {
            error = "Component metadata self-test failed to parse aggregate metadata '" + metadata + "'.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(aggregateBindings, aggregateBinding);
        return true;
    };
    if (!addAggregateMetadata("bbconfig prototype=\"Interface/Text/2D Text\" pins=\"Text='Aggregate'\" settings=\"Text Properties='Screen Proportionnal'\" sources=\"Font<-FontConfig.Font Created\"") ||
        !addAggregateMetadata("bbpin \"Text\"=\"Fragment\"")) {
        return false;
    }
    if (aggregateBindings.size() != 1 ||
        aggregateBindings[0].ConfigPinValues.size() != 2 ||
        aggregateBindings[0].ConfigPinValues.back().Name != "Text" ||
        aggregateBindings[0].ConfigPinValues.back().Value != "Fragment" ||
        aggregateBindings[0].ConfigSettingValues.size() != 1 ||
        aggregateBindings[0].ConfigSources.size() != 1) {
        error = "Component metadata self-test did not preserve aggregate metadata with fragment overwrite order.";
        return false;
    }

    std::vector<ScriptComponentBinding> manifestBindings;
    for (const std::string &line : {
             std::string("bbconfig field=ManifestConfig prototype=\"Interface/Text/2D Text\" lifetime=manual pins=\"Text='Aggregate'\""),
             std::string("bbpin field=ManifestConfig \"Text\"=\"Fragment\""),
             std::string("bbsetting field=ManifestConfig \"Text Properties\"=\"Screen Proportionnal\""),
             std::string("bbsource field=ManifestConfig \"Font\"=\"FontConfig.Font Created\"")}) {
        ScriptComponentBinding manifestBinding;
        if (!ScriptComponentSupport::ParseManifestLine(line, manifestBinding)) {
            error = "Component metadata self-test failed to parse manifest line '" + line + "'.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(manifestBindings, manifestBinding);
    }
    if (manifestBindings.size() != 1 ||
        manifestBindings[0].BBConfigLifetime != ScriptComponentBBConfigLifetime::Manual ||
        manifestBindings[0].ConfigPinValues.size() != 2 ||
        manifestBindings[0].ConfigPinValues.back().Name != "Text" ||
        manifestBindings[0].ConfigPinValues.back().Value != "Fragment" ||
        manifestBindings[0].ConfigSettingValues.size() != 1 ||
        manifestBindings[0].ConfigSources.size() != 1) {
        error = "Component metadata self-test did not merge manifest BBConfig fragments correctly.";
        return false;
    }

    ScriptComponentBinding occurrenceBinding;
    if (!ScriptComponentSupport::ParseBindingMetadata(
            "bbconfig prototype=\"Logics/Calculator/Identity\" pins=\"Value[3]='42'\" sources=\"pIn 0[1]<-SourceConfig.pout:Value[2]\" required=\"pout:Out[4]\"",
            "OccurrenceConfig",
            occurrenceBinding)) {
        error = "Component metadata self-test failed to parse occurrence metadata.";
        return false;
    }
    if (occurrenceBinding.ConfigPinValues.size() != 1 ||
        occurrenceBinding.ConfigPinValues[0].Name != "Value" ||
        occurrenceBinding.ConfigPinValues[0].Occurrence != 3 ||
        occurrenceBinding.ConfigSources.size() != 1 ||
        occurrenceBinding.ConfigSources[0].PinName != "pIn 0" ||
        occurrenceBinding.ConfigSources[0].PinOccurrence != 1 ||
        occurrenceBinding.ConfigSources[0].SourceFieldName != "SourceConfig" ||
        occurrenceBinding.ConfigSources[0].SourceSlotName != "pout:Value" ||
        occurrenceBinding.ConfigSources[0].SourceOccurrence != 2) {
        error = "Component metadata self-test did not preserve BBConfig slot occurrences.";
        return false;
    }
    bool sawPoutOccurrence = false;
    for (const ScriptComponentRequiredSlot &slot : occurrenceBinding.RequiredSlots) {
        sawPoutOccurrence = sawPoutOccurrence || (slot.KindName == "pout" && slot.Name == "Out" && slot.Occurrence == 4);
    }
    if (!sawPoutOccurrence) {
        error = "Component metadata self-test did not preserve required slot occurrence.";
        return false;
    }

    ScriptComponentBinding slotOccurrenceBinding;
    if (!ScriptComponentSupport::ParseBindingMetadata(
            "bbslot from=\"OccurrenceConfig\" pout=\"Out[2]\"",
            "OutSlot",
            slotOccurrenceBinding)) {
        error = "Component metadata self-test failed to parse BBSlot occurrence metadata.";
        return false;
    }
    if (slotOccurrenceBinding.Kind != ScriptComponentBindingKind::BBSlot ||
        slotOccurrenceBinding.SlotName != "Out" ||
        slotOccurrenceBinding.SlotOccurrence != 2) {
        error = "Component metadata self-test did not preserve BBSlot field occurrence.";
        return false;
    }
    const std::string occurrenceCacheText = ScriptComponentSupport::BuildBBConfigBindingCacheText(occurrenceBinding, 0, std::string());
    if (occurrenceCacheText.find("pin:Value[3]=") == std::string::npos ||
        occurrenceCacheText.find("source:pIn 0[1]<-SourceConfig.pout:Value[2]") == std::string::npos) {
        error = "Component metadata self-test did not include slot occurrences in BBConfig cache text.";
        return false;
    }

    if (!RunComponentBehaviorFrameRuntimeSelfTest(context, engine, error)) {
        return false;
    }

    return true;
}
