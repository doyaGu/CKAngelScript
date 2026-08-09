#!/usr/bin/env python3
"""Normalize a live CKAngelScript API export and refresh/check tracked catalog files."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Iterable


REPO_ROOT = Path(__file__).resolve().parents[1]
CATALOG_PATH = REPO_ROOT / "docs" / "script-api.json"
LOCK_PATH = REPO_ROOT / "docs" / "script-api.lock.json"
COMPATIBILITY_ALLOWLIST_PATH = REPO_ROOT / "docs" / "script-api-compatibility-allowlist.json"
PREDEFINED_PATH = REPO_ROOT / "as.predefined"
INVENTORY_PATH = REPO_ROOT / "docs" / "api-inventory.md"
INVENTORY_BEGIN = "<!-- BEGIN GENERATED API CATALOG SUMMARY -->"
INVENTORY_END = "<!-- END GENERATED API CATALOG SUMMARY -->"
REGISTRATION_TOKENS = (
    "RegisterGlobalFunction(",
    "RegisterGlobalProperty(",
    "RegisterObjectType(",
    "RegisterObjectMethod(",
    "RegisterObjectBehaviour(",
    "RegisterObjectProperty(",
    "RegisterFuncdef(",
    "RegisterEnum(",
    "RegisterEnumValue(",
    "RegisterTypedef(",
    "RegisterInterface(",
    "RegisterInterfaceMethod(",
    "SetDefaultNamespace(",
    "RegisterStdAddons(",
    "RegisterVirtools(",
)
# AngelScript assigns these identifiers in registration order. They are useful
# inside one engine instance but are neither stable nor part of the script ABI;
# retaining them turns an additive registration into a repository-wide diff.
VOLATILE_KEYS = frozenset({
    "address",
    "id",
    "typeId",
    "returnTypeId",
    "behaviourId",
    "aliasedTypeId",
    "baseTypeId",
})
CATALOG_PROFILE = {
    "name": "full",
    "cmakeOptions": {
        "CKAS_ENABLE_DYNCALL": "ON",
    },
    "dependencies": {
        "AngelScript": {
            "version": "2.38.0",
            "sha256": "b33b5dbcda10317ef67d628353d83246984ce6fcac102d4dc2aed121eba52e6f",
        },
        "DynCall": {
            "version": "1.4",
            "url": "https://dyncall.org/r1.4/dyncall-1.4.zip",
            "sha256": "877d62fe1beb41e21c9518144065074b35331aeeee303d6452e2482a5cb6500e",
        },
        "VirtoolsSDK": {
            "repository": "doyaGu/Virtools-SDK-2.1",
            "commit": "175bcb12d829c687359b90615be53524cdba0132",
        },
    },
}


def load_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError(f"{path} must contain a JSON object")
    if value.get("schemaVersion") != 1:
        raise ValueError(f"Unsupported script API schemaVersion: {value.get('schemaVersion')!r}")
    return value


def normalize(value: Any) -> Any:
    if isinstance(value, dict):
        return {key: normalize(item) for key, item in value.items() if key not in VOLATILE_KEYS}
    if isinstance(value, list):
        return [normalize(item) for item in value]
    return value


def json_text(value: dict[str, Any]) -> str:
    return json.dumps(value, ensure_ascii=False, indent=2) + "\n"


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def registration_inputs() -> list[Path]:
    inputs: list[Path] = []
    for path in sorted((REPO_ROOT / "src").glob("*.cpp")) + sorted((REPO_ROOT / "src").glob("*.h")):
        text = path.read_text(encoding="utf-8-sig", errors="replace")
        if any(token in text for token in REGISTRATION_TOKENS):
            inputs.append(path)
    # The vendored/downloaded SDK tree is intentionally ignored. Its pinned
    # version and download configuration are tracked in this CMake file.
    inputs.extend([
        REPO_ROOT / "CMakeLists.txt",
        REPO_ROOT / "deps" / "CMakeLists.txt",
        REPO_ROOT / "src" / "CMakeLists.txt",
        REPO_ROOT / "src" / "ScriptInfo.cpp",
        REPO_ROOT / "src" / "ScriptInfo.h",
        REPO_ROOT / "cmake" / "FindDynCall.cmake",
        REPO_ROOT / "cmake" / "FindDynCallback.cmake",
        REPO_ROOT / "cmake" / "FindDynLoad.cmake",
        REPO_ROOT / ".github" / "workflows" / "build.yml",
        REPO_ROOT / "tools" / "Prepare-DynCall.ps1",
    ])
    return sorted(set(inputs), key=lambda path: path.relative_to(REPO_ROOT).as_posix())


def qualified_name(namespace: Any, name: Any) -> str:
    namespace_text = str(namespace or "")
    name_text = str(name or "")
    return f"{namespace_text}::{name_text}" if namespace_text else name_text


def api_surface_entries(catalog: dict[str, Any]) -> set[str]:
    """Return stable, ID-independent keys for the script-visible ABI."""
    entries: set[str] = set()

    for function in catalog.get("globalFunctions", []):
        entries.add(
            f"global-function|{function.get('namespace') or ''}|{function.get('declaration') or ''}")
    for prop in catalog.get("globalProperties", []):
        entries.add(f"global-property|{prop.get('namespace') or ''}|{prop.get('declaration') or ''}")

    for object_type in catalog.get("objectTypes", []):
        object_name = qualified_name(object_type.get("namespace"), object_type.get("name"))
        entries.add(f"object-type|{object_name}|{object_type.get('declaration') or ''}")
        for factory in object_type.get("factories", []):
            entries.add(f"factory|{object_name}|{factory.get('declaration') or ''}")
        for method in object_type.get("methods", []):
            entries.add(f"method|{object_name}|{method.get('declaration') or ''}")
        for prop in object_type.get("properties", []):
            entries.add(f"object-property|{object_name}|{prop.get('declaration') or ''}")
        for behaviour in object_type.get("behaviours", []):
            function = behaviour.get("function") or {}
            entries.add(
                f"behaviour|{object_name}|{behaviour.get('behaviour') or ''}|"
                f"{function.get('declaration') or ''}")

    for enum in catalog.get("enums", []):
        enum_name = qualified_name(enum.get("namespace"), enum.get("name"))
        entries.add(f"enum|{enum_name}")
        for value in enum.get("values", []):
            entries.add(f"enum-value|{enum_name}|{value.get('name') or ''}={value.get('value')}")
    for typedef in catalog.get("typedefs", []):
        name = qualified_name(typedef.get("namespace"), typedef.get("name"))
        entries.add(f"typedef|{name}|{typedef.get('aliasedType') or ''}")
    for funcdef in catalog.get("funcdefs", []):
        name = qualified_name(funcdef.get("namespace"), funcdef.get("name"))
        entries.add(f"funcdef|{name}")
        declaration = str(funcdef.get("declaration") or funcdef.get("typeDeclaration") or "")
        if "(" in declaration:
            entries.add(f"funcdef-signature|{name}|{declaration}")
    return entries


def compatibility_removals(baseline: dict[str, Any], current: dict[str, Any]) -> list[str]:
    return sorted(api_surface_entries(baseline) - api_surface_entries(current))


def catalog_sha256(catalog: dict[str, Any]) -> str:
    return sha256_bytes(json_text(normalize(catalog)).encode("utf-8"))


def allowed_removal_keys(baseline_sha256: str) -> set[str]:
    if not COMPATIBILITY_ALLOWLIST_PATH.is_file():
        return set()
    document = json.loads(COMPATIBILITY_ALLOWLIST_PATH.read_text(encoding="utf-8-sig"))
    if not isinstance(document, dict) or document.get("schemaVersion") != 1:
        raise ValueError(f"Unsupported compatibility allowlist: {COMPATIBILITY_ALLOWLIST_PATH}")
    allowed: set[str] = set()
    for item in document.get("allowedRemovals", []):
        if not isinstance(item, dict) or not item.get("key") or not item.get("reason"):
            raise ValueError("Every compatibility allowlist entry requires key and reason")
        if item.get("baselineCatalogSha256") == baseline_sha256:
            allowed.add(str(item["key"]))
    return allowed


def load_catalog_from_git(ref: str) -> dict[str, Any]:
    result = subprocess.run(
        ["git", "show", f"{ref}:docs/script-api.json"],
        cwd=REPO_ROOT,
        check=False,
        capture_output=True,
    )
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        raise ValueError(f"Could not read docs/script-api.json from {ref}: {detail}")
    value = json.loads(result.stdout.decode("utf-8-sig"))
    if not isinstance(value, dict) or value.get("schemaVersion") != 1:
        raise ValueError(f"Unsupported script API catalog at {ref}")
    return value


def check_compatibility(baseline_ref: str) -> int:
    try:
        baseline = load_catalog_from_git(baseline_ref)
        current = load_json(CATALOG_PATH)
        baseline_sha = catalog_sha256(baseline)
        removals = compatibility_removals(baseline, current)
        allowed = allowed_removal_keys(baseline_sha)
    except (OSError, ValueError) as exc:
        print(f"API compatibility check failed: {exc}", file=sys.stderr)
        return 1
    breaking = [entry for entry in removals if entry not in allowed]
    if breaking:
        print(f"Breaking AngelScript API changes relative to {baseline_ref}:", file=sys.stderr)
        for entry in breaking:
            print(f"  - {entry}", file=sys.stderr)
        print(
            "Restore the declarations or add a scoped, justified entry to "
            "docs/script-api-compatibility-allowlist.json.",
            file=sys.stderr,
        )
        return 1
    print(
        f"API compatibility is intact relative to {baseline_ref}; "
        f"{len(removals)} approved removal(s)."
    )
    return 0


def surface_fingerprint(paths: Iterable[Path]) -> tuple[str, list[str]]:
    digest = hashlib.sha256()
    names: list[str] = []
    for path in paths:
        if not path.is_file():
            raise FileNotFoundError(f"Catalog fingerprint input is missing: {path}")
        name = path.relative_to(REPO_ROOT).as_posix()
        names.append(name)
        digest.update(name.encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest(), names


def generate_predefined(catalog: dict[str, Any]) -> str:
    sys.path.insert(0, str(REPO_ROOT / "tools"))
    try:
        from generate_as_predefined import generate  # type: ignore
    finally:
        sys.path.pop(0)
    return generate(catalog)


def generate_inventory_summary(catalog: dict[str, Any]) -> str:
    object_types = catalog.get("objectTypes", [])
    counts = [
        ("Global functions", len(catalog.get("globalFunctions", []))),
        ("Global properties", len(catalog.get("globalProperties", []))),
        ("Object types", len(object_types)),
        ("Object methods", sum(len(item.get("methods", [])) for item in object_types)),
        ("Object behaviours", sum(len(item.get("behaviours", [])) for item in object_types)),
        ("Object properties", sum(len(item.get("properties", [])) for item in object_types)),
        ("Funcdefs", len(catalog.get("funcdefs", []))),
        ("Enums", len(catalog.get("enums", []))),
        ("Enum values", sum(len(item.get("values", [])) for item in catalog.get("enums", []))),
        ("Typedefs", len(catalog.get("typedefs", []))),
    ]
    namespace_counts: dict[str, int] = {}
    for function in catalog.get("globalFunctions", []):
        namespace = str(function.get("namespace") or "Global")
        namespace_counts[namespace] = namespace_counts.get(namespace, 0) + 1
    responsibilities = {
        "Scene": "Safe scene lookup, typed refs, creation, scene membership, selection, destruction",
        "Param": "Parameter values, enum/flags/type metadata, text/raw conversion, operations",
        "Async": "Frame-based cooperative tasks, aggregates, waits for BB/graph tasks",
        "BB": "Building Block prototype discovery, declaration/config creation, runtime bridge entry points",
        "Message": "Publish/send/request/reply messaging between runtime entries and components",
        "Behavior": "Behavior graph lookup, query construction, graph bridge entry points",
        "Runtime": "Script entry manager inspection and control",
        "Global": "Raw CK/Vx helpers, constructors/operators, allocation helpers, format/info functions",
    }
    ordered_namespaces = ["Scene", "Param", "Async", "BB", "Message", "Behavior", "Runtime", "Global"]
    lines = [
        INVENTORY_BEGIN,
        "## Registration Counts",
        "",
        "Generated from the tracked live-engine export in `docs/script-api.json`:",
        "",
        "| Registration kind | Count |",
        "| --- | ---: |",
    ]
    lines.extend(f"| {name} | {count} |" for name, count in counts)
    lines.append(f"| Total | {sum(count for _, count in counts)} |")
    lines.extend([
        "",
        "Large raw SDK binding files and exported cast/behaviour entries dominate the total. The high-level and integration files are still the workflow surface; raw CK/Vx bindings and SDK constants account for most exported symbols.",
        "",
        "## Namespace Map",
        "",
        "| Namespace | Global functions | Main responsibility |",
        "| --- | ---: | --- |",
    ])
    for namespace in ordered_namespaces:
        lines.append(f"| `{namespace}` | {namespace_counts.get(namespace, 0)} | {responsibilities[namespace]} |")
    lines.extend([
        "",
        "The global bucket includes raw SDK-style helpers and constructors, so new high-level workflow APIs should normally prefer a named namespace.",
        INVENTORY_END,
    ])
    return "\n".join(lines)


def generate_inventory(catalog: dict[str, Any]) -> str:
    current = INVENTORY_PATH.read_text(encoding="utf-8-sig")
    start = current.find(INVENTORY_BEGIN)
    end = current.find(INVENTORY_END)
    if start < 0 or end < start:
        raise ValueError(f"{INVENTORY_PATH} is missing generated catalog summary markers")
    end += len(INVENTORY_END)
    return current[:start] + generate_inventory_summary(catalog) + current[end:]


def lock_document(catalog_bytes: bytes) -> dict[str, Any]:
    fingerprint, inputs = surface_fingerprint(registration_inputs())
    return {
        "schemaVersion": 1,
        "catalog": "docs/script-api.json",
        "catalogSha256": sha256_bytes(catalog_bytes),
        "registrationSurfaceSha256": fingerprint,
        "registrationInputs": inputs,
        "catalogProfile": CATALOG_PROFILE,
        "generated": ["as.predefined", "docs/api-inventory.md"],
    }


def expected_outputs(api_json: Path) -> dict[Path, bytes]:
    catalog = normalize(load_json(api_json))
    catalog_bytes = json_text(catalog).encode("utf-8")
    predefined_bytes = generate_predefined(catalog).encode("utf-8")
    inventory_bytes = generate_inventory(catalog).encode("utf-8")
    lock_bytes = json_text(lock_document(catalog_bytes)).encode("utf-8")
    return {
        CATALOG_PATH: catalog_bytes,
        PREDEFINED_PATH: predefined_bytes,
        INVENTORY_PATH: inventory_bytes,
        LOCK_PATH: lock_bytes,
    }


def check_outputs() -> int:
    if not CATALOG_PATH.is_file():
        print(f"missing tracked live catalog: {CATALOG_PATH.relative_to(REPO_ROOT)}", file=sys.stderr)
        return 1
    expected = expected_outputs(CATALOG_PATH)
    drift: list[str] = []
    for path, content in expected.items():
        if not path.is_file() or path.read_bytes() != content:
            drift.append(path.relative_to(REPO_ROOT).as_posix())
    if drift:
        print("API catalog drift detected:", file=sys.stderr)
        for name in drift:
            print(f"  - {name}", file=sys.stderr)
        print("Run tools/Refresh-ApiCatalog.ps1 with a fresh live-engine export.", file=sys.stderr)
        return 1
    print("API catalog is current; live catalog, registration fingerprint, and generated files agree.")
    return 0


def update_outputs(api_json: Path) -> int:
    expected = expected_outputs(api_json)
    for path, content in expected.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as temporary:
            temporary.write(content)
            temporary_path = Path(temporary.name)
        temporary_path.replace(path)
        print(f"updated {path.relative_to(REPO_ROOT).as_posix()}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-json", type=Path, help="Fresh JSON written by CKAS_EXPORT_SCRIPT_API.")
    parser.add_argument("--check", action="store_true", help="Verify tracked catalog outputs without modifying them.")
    parser.add_argument(
        "--check-compatibility-ref",
        metavar="GIT_REF",
        help="With --check, reject exported declarations removed relative to this Git ref.",
    )
    args = parser.parse_args()
    if args.check:
        if args.api_json:
            parser.error("--api-json cannot be combined with --check")
        result = check_outputs()
        if result == 0 and args.check_compatibility_ref:
            result = check_compatibility(args.check_compatibility_ref)
        return result
    if args.check_compatibility_ref:
        parser.error("--check-compatibility-ref requires --check")
    if not args.api_json:
        parser.error("--api-json is required unless --check is used")
    return update_outputs(args.api_json.resolve())


if __name__ == "__main__":
    raise SystemExit(main())
