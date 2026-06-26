#include "ScriptComponentAutomation.h"

#include <vector>

#include "ScriptBridgeHandles.h"
#include "ScriptComponentInjection.h"
#include "ScriptComponentMetadata.h"

namespace ScriptComponentSupport {

void StopComponentLifetimeBBConfigs(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    if (!state || !state->Object) {
        return;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (!UsesComponentLifetime(binding)) {
            continue;
        }
        BBConfig *bbinding = GetBBConfigField(state, binding);
        if (bbinding) {
            bbinding->Stop(behcontext);
        }
    }
}

void DestroyComponentLifetimeBBConfigs(ScriptComponentState *state) {
    if (!state || !state->Object) {
        return;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (!UsesComponentLifetime(binding)) {
            continue;
        }
        BBConfig *bbinding = GetBBConfigField(state, binding);
        if (bbinding) {
            bbinding->Destroy();
        }
    }
}

bool EnsureAutoStartedBBConfigs(const CKBehaviorContext &behcontext, ScriptComponentState *state, std::string &error) {
    if (!state || !state->Object) {
        return true;
    }

    std::vector<std::size_t> pending;
    for (std::size_t i = 0; i < state->Bindings.size(); ++i) {
        const ScriptComponentBinding &binding = state->Bindings[i];
        if (binding.Kind == ScriptComponentBindingKind::BBConfig && binding.AutoStartBBConfig) {
            pending.push_back(i);
        }
    }

    std::string lastError;
    while (!pending.empty()) {
        bool madeProgress = false;
        for (std::size_t i = 0; i < pending.size();) {
            ScriptComponentBinding &binding = state->Bindings[pending[i]];
            BBConfig *bbinding = GetBBConfigField(state, binding);
            if (!bbinding) {
                error = "Autostart BBConfig field is not available (" + BindingSummary(binding, behcontext.Context) + ").";
                return false;
            }

            std::string attemptError;
            if (!ApplyBBConfigSourceBindings(behcontext, state, binding, bbinding, true, attemptError)) {
                lastError = attemptError;
                ++i;
                continue;
            }
            BBInstance *instance = bbinding->EnsureStarted(behcontext);
            if (!instance) {
                lastError = "Autostart BBConfig failed: " + bbinding->Error() + " (" + BindingSummary(binding, behcontext.Context) + ").";
                ++i;
                continue;
            }
            instance->Release();
            pending.erase(pending.begin() + static_cast<std::vector<std::size_t>::difference_type>(i));
            madeProgress = true;
        }

        if (!madeProgress) {
            error = lastError.empty()
                ? "Autostart BBConfig dependencies could not be resolved."
                : lastError;
            return false;
        }
    }
    return true;
}

bool StepAutomatedBBConfigs(const CKBehaviorContext &behcontext, ScriptComponentState *state, std::string &error) {
    if (!state || !state->Object) {
        return true;
    }
    for (ScriptComponentBinding &binding : state->Bindings) {
        if (binding.Kind != ScriptComponentBindingKind::BBConfig || binding.BBStepPolicy == ScriptComponentBBStepPolicy::Manual) {
            continue;
        }
        BBConfig *bbinding = GetBBConfigField(state, binding);
        if (!bbinding) {
            error = "Automated BBConfig field is not available (" + BindingSummary(binding, behcontext.Context) + ").";
            return false;
        }
        BBInstance *instance = bbinding->Instance();
        if (!instance) {
            continue;
        }
        if (!ApplyBBConfigSourceBindings(behcontext, state, binding, bbinding, true, error)) {
            instance->Release();
            return false;
        }
        const bool shouldStep = binding.BBStepPolicy == ScriptComponentBBStepPolicy::EachUpdate ||
                                (binding.BBStepPolicy == ScriptComponentBBStepPolicy::OnChange && binding.BBConfigChanged);
        if (shouldStep && !instance->Step(behcontext)) {
            error = "Automated BBConfig step failed: " + instance->Error() + " (" + BindingSummary(binding, behcontext.Context) + ").";
            instance->Release();
            return false;
        }
        binding.BBConfigChanged = false;
        instance->Release();
    }
    return true;
}

} // namespace ScriptComponentSupport
