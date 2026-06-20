#include "ScriptSelfTests.h"

#include <string>
#include <vector>

#include "ScriptComponentMetadata.h"

bool RunScriptComponentMetadataSelfTest(std::string &error) {
    std::vector<ScriptComponentBinding> bindings;
    auto addMetadata = [&](const std::string &metadata) -> bool {
        ScriptComponentBinding binding;
        if (!AngelScriptComponentInternal::ParseBindingMetadata(metadata, "TextConfig", binding)) {
            error = "Component metadata self-test failed to parse '" + metadata + "'.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(bindings, binding);
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
        if (!AngelScriptComponentInternal::ParseBindingMetadata(
                "bbconfig prototype=\"Logics/Calculator/Identity\"",
                fieldName,
                configBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot config metadata.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(repeatedSlotBindings, configBinding);

        ScriptComponentBinding slotBinding;
        if (!AngelScriptComponentInternal::ParseBindingMetadata("bbpout \"pOut 0\"", fieldName, slotBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot fragment metadata.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(repeatedSlotBindings, slotBinding);
    }
    if (repeatedSlotBindings.size() != 2 ||
        repeatedSlotBindings[0].ParameterName != "FirstConfig" ||
        repeatedSlotBindings[1].ParameterName != "SecondConfig") {
        error = "Component metadata self-test let BBConfig fragment slot names override field parameter names.";
        return false;
    }

    ScriptComponentBinding legacyManaged;
    if (!AngelScriptComponentInternal::ParseBindingMetadata(
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
        if (!AngelScriptComponentInternal::ParseBindingMetadata(metadata, "AggregateConfig", aggregateBinding)) {
            error = "Component metadata self-test failed to parse aggregate metadata '" + metadata + "'.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(aggregateBindings, aggregateBinding);
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
        if (!AngelScriptComponentInternal::ParseManifestLine(line, manifestBinding)) {
            error = "Component metadata self-test failed to parse manifest line '" + line + "'.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(manifestBindings, manifestBinding);
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
    if (!AngelScriptComponentInternal::ParseBindingMetadata(
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
    if (!AngelScriptComponentInternal::ParseBindingMetadata(
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
    const std::string occurrenceCacheText = AngelScriptComponentInternal::BuildBBConfigBindingCacheText(occurrenceBinding, 0, std::string());
    if (occurrenceCacheText.find("pin:Value[3]=") == std::string::npos ||
        occurrenceCacheText.find("source:pIn 0[1]<-SourceConfig.pout:Value[2]") == std::string::npos) {
        error = "Component metadata self-test did not include slot occurrences in BBConfig cache text.";
        return false;
    }

    return true;
}
