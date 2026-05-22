#include "ScriptSelfTests.h"

#include "CKParameterManager.h"
#include "ScriptParameterRegistry.h"

bool RunScriptParameterRegistrySelfTest(CKContext *context, std::string &error) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    if (!registry) {
        error = "ScriptParameterRegistry is not available.";
        return false;
    }

    bool sawEnum = false;
    bool sawFlags = false;
    bool sawStruct = false;
    CKParameterManager *pm = registry->GetParameterManager();
    if (!pm) {
        error = "CKParameterManager is not available.";
        return false;
    }

    for (int i = 0; i < pm->GetParameterTypesCount(); ++i) {
        const ScriptParamTypeRecord *record = registry->GetType(i);
        if (!record) {
            continue;
        }

        if (!sawEnum && record->Has(ScriptParamTypeCaps::EnumLike) && !record->EnumEntries.empty()) {
            int value = 0;
            std::string parseError;
            if (!registry->ParseEnumValue(record->Guid, record->EnumEntries[0].Name, value, parseError) ||
                value != record->EnumEntries[0].Value) {
                error = "Enum registry lookup failed: " + parseError;
                return false;
            }
            if (registry->ParseEnumValue(record->Guid, "__missing_enum__", value, parseError)) {
                error = "Enum registry accepted an unknown token.";
                return false;
            }
            sawEnum = true;
        }

        if (!sawFlags && record->Has(ScriptParamTypeCaps::FlagsLike) && !record->FlagEntries.empty()) {
            CKDWORD value = 0;
            std::string parseError;
            if (!registry->ParseFlagsValue(record->Guid, record->FlagEntries[0].Name, value, parseError) ||
                value != static_cast<CKDWORD>(record->FlagEntries[0].Value)) {
                error = "Flags registry lookup failed: " + parseError;
                return false;
            }
            if (registry->ParseFlagsValue(record->Guid, "__missing_flag__", value, parseError)) {
                error = "Flags registry accepted an unknown token.";
                return false;
            }
            sawFlags = true;
        }

        if (!sawStruct && record->Has(ScriptParamTypeCaps::StructLike) && !record->StructMembers.empty()) {
            if (record->StructMembers[0].Guid == CKGUID() || record->StructMembers[0].Name.empty()) {
                error = "Struct registry member metadata is incomplete.";
                return false;
            }
            sawStruct = true;
        }
    }

    if (!sawEnum || !sawFlags || !sawStruct) {
        error = "Registry did not find enum, flags, and struct parameter metadata.";
        return false;
    }

    const CKGUID enumGuid(0x6a33ce51, 0x22705501);
    CKERROR err = registry->RegisterEnum(enumGuid, "__CKAS_TestEnum", "First=7,Second=9");
    if (err == CK_OK) {
        int value = 0;
        std::string parseError;
        if (!registry->ParseEnumValue(enumGuid, "Second", value, parseError) || value != 9) {
            error = "Registered enum did not refresh registry cache.";
            return false;
        }
    }

    error.clear();
    return true;
}
