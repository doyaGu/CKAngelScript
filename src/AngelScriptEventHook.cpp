#include "AngelScriptEventHook.h"

#include <string>

#include "CKAll.h"
#include "Logger.h"
#include "ScriptManager.h"
#include "ScriptMessage.h"
#include "add_on/scriptdictionary/scriptdictionary.h"

namespace AngelScriptEventHookInternal {

constexpr int OUTPUT_OUT = 0;
constexpr int OUTPUT_ERROR = 1;
constexpr int PARAM_TOPIC = 0;
constexpr int PARAM_TARGET = 1;
constexpr int PARAM_PAYLOAD = 2;
constexpr int POUT_ERROR_MESSAGE = 0;

std::string ReadStringParameter(CKBehavior *behavior, int index) {
    if (!behavior || index < 0 || index >= behavior->GetInputParameterCount()) {
        return std::string();
    }
    CKSTRING value = static_cast<CKSTRING>(behavior->GetInputParameterReadDataPtr(index));
    return value ? std::string(value) : std::string();
}

void RaiseError(CKBehavior *behavior, const std::string &message) {
    if (!behavior) {
        return;
    }
    if (behavior->GetOutputParameterCount() > POUT_ERROR_MESSAGE) {
        behavior->SetOutputParameterValue(POUT_ERROR_MESSAGE, message.c_str());
    }
    if (behavior->GetOutputCount() > OUTPUT_ERROR) {
        behavior->ActivateOutput(OUTPUT_ERROR);
    }
}

CScriptDictionary *CreatePayloadDictionary(asIScriptEngine *engine, const std::string &payloadText, CK_ID behaviorId) {
    if (!engine || payloadText.empty()) {
        return nullptr;
    }
    CScriptDictionary *payload = CScriptDictionary::Create(engine);
    if (!payload) {
        return nullptr;
    }

    const int stringType = engine->GetTypeIdByDecl("string");
    if (stringType >= 0) {
        std::string text = payloadText;
        payload->Set("text", &text, stringType);
    }
    payload->Set("behavior_id", static_cast<asINT64>(behaviorId));
    return payload;
}

} // namespace AngelScriptEventHookInternal

CKObjectDeclaration *FillBehaviorAngelScriptEventHookDecl() {
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Event Hook");
    od->SetDescription("Publish an AngelScript Message topic from a Virtools behavior graph");
    od->SetCategory("AngelScript");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x12e4f2a0, 0x4b7d4e21));
    od->SetAuthorGuid(CKGUID(0x3a086b4d, 0x2f4a4f01));
    od->SetAuthorName("Kakuty");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAngelScriptEventHookProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateAngelScriptEventHookProto(CKBehaviorPrototype **pproto) {
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Event Hook");
    if (!proto) {
        return CKERR_OUTOFMEMORY;
    }

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");
    proto->DeclareOutput("Error");

    proto->DeclareInParameter("Topic", CKPGUID_STRING);
    proto->DeclareInParameter("Target", CKPGUID_STRING);
    proto->DeclareInParameter("Payload", CKPGUID_STRING);
    proto->DeclareOutParameter("Error Message", CKPGUID_STRING);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AngelScriptEventHook);
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS) (CKBEHAVIOR_MESSAGESENDER |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS));

    *pproto = proto;
    return CK_OK;
}

int AngelScriptEventHook(const CKBehaviorContext &behcontext) {
    CKBehavior *behavior = behcontext.Behavior;
    if (!behavior) {
        return CKBR_PARAMETERERROR;
    }

    ScriptManager *manager = behcontext.Context ? ScriptManager::GetManager(behcontext.Context) : nullptr;
    if (!manager || !manager->GetMessageBus()) {
        const std::string error = "AngelScript manager or message bus is not available.";
        AngelScriptEventHookInternal::RaiseError(behavior, error);
        LOG_ERROR("[AngelScript Event Hook] %s", error.c_str());
        return CKBR_OK;
    }

    const std::string topic = AngelScriptEventHookInternal::ReadStringParameter(behavior, AngelScriptEventHookInternal::PARAM_TOPIC);
    if (topic.empty()) {
        const std::string error = "Event Hook topic is empty.";
        AngelScriptEventHookInternal::RaiseError(behavior, error);
        return CKBR_OK;
    }

    const std::string target = AngelScriptEventHookInternal::ReadStringParameter(behavior, AngelScriptEventHookInternal::PARAM_TARGET);
    const std::string payloadText = AngelScriptEventHookInternal::ReadStringParameter(behavior, AngelScriptEventHookInternal::PARAM_PAYLOAD);
    CScriptDictionary *payload = AngelScriptEventHookInternal::CreatePayloadDictionary(manager->GetScriptEngine(),
                                                                                       payloadText,
                                                                                       behavior->GetID());

    std::string error;
    const std::string source = "behavior:event_hook:" + std::to_string(behavior->GetID());
    const bool published = manager->GetMessageBus()->Publish(source, topic, payload, target, error);
    if (payload) {
        payload->Release();
    }

    if (!published) {
        const std::string message = error.empty() ? "Event Hook publish failed." : error;
        AngelScriptEventHookInternal::RaiseError(behavior, message);
        if (behcontext.Context) {
            behcontext.Context->OutputToConsoleEx(const_cast<char *>("[AngelScript Event Hook] %s"), message.c_str());
        }
        LOG_ERROR("[AngelScript Event Hook] %s", message.c_str());
        return CKBR_OK;
    }

    if (behavior->GetOutputCount() > AngelScriptEventHookInternal::OUTPUT_OUT) {
        behavior->ActivateOutput(AngelScriptEventHookInternal::OUTPUT_OUT);
    }
    return CKBR_OK;
}
