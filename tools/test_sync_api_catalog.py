#!/usr/bin/env python3

import copy
import unittest

import sync_api_catalog


def catalog() -> dict:
    return {
        "schemaVersion": 1,
        "globalFunctions": [{"namespace": "Scene", "declaration": "int Count()"}],
        "globalProperties": [],
        "objectTypes": [{
            "namespace": "",
            "name": "Probe",
            "declaration": "class Probe",
            "factories": [{"declaration": "Probe@ Probe()"}],
            "methods": [{"declaration": "string Value() const"}],
            "properties": [{"declaration": "int state"}],
            "behaviours": [{
                "behaviour": "addref",
                "function": {"declaration": "void $beh5()"},
            }],
        }],
        "enums": [{
            "namespace": "",
            "name": "ProbeKind",
            "values": [{"name": "ProbeA", "value": 1}],
        }],
        "typedefs": [{"namespace": "", "name": "ProbeId", "aliasedType": "uint"}],
        "funcdefs": [{"namespace": "", "name": "ProbeFunc", "declaration": "int ProbeFunc(int)"}],
    }


class ApiCompatibilityTests(unittest.TestCase):
    def test_additions_are_compatible(self) -> None:
        baseline = catalog()
        current = copy.deepcopy(baseline)
        current["globalFunctions"].append({"namespace": "Scene", "declaration": "int Added()"})
        self.assertEqual([], sync_api_catalog.compatibility_removals(baseline, current))

    def test_removed_method_is_breaking(self) -> None:
        baseline = catalog()
        current = copy.deepcopy(baseline)
        current["objectTypes"][0]["methods"] = []
        self.assertEqual(
            ["method|Probe|string Value() const"],
            sync_api_catalog.compatibility_removals(baseline, current),
        )

    def test_legacy_funcdef_without_signature_accepts_enriched_export(self) -> None:
        baseline = catalog()
        baseline["funcdefs"][0]["declaration"] = "ProbeFunc"
        self.assertEqual([], sync_api_catalog.compatibility_removals(baseline, catalog()))

    def test_signature_and_enum_value_changes_remove_old_abi(self) -> None:
        baseline = catalog()
        current = copy.deepcopy(baseline)
        current["globalFunctions"][0]["declaration"] = "uint Count()"
        current["enums"][0]["values"][0]["value"] = 2
        current["funcdefs"][0]["declaration"] = "uint ProbeFunc(int)"
        self.assertEqual(
            [
                "enum-value|ProbeKind|ProbeA=1",
                "funcdef-signature|ProbeFunc|int ProbeFunc(int)",
                "global-function|Scene|int Count()",
            ],
            sync_api_catalog.compatibility_removals(baseline, current),
        )


if __name__ == "__main__":
    unittest.main()
