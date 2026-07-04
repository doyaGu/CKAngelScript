#!/usr/bin/env python3
"""Generate an angel-lsp as.predefined file from CKAngelScript API JSON."""

from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path
from typing import Any, Iterable


IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
COLLECTIONS = ("globalFunctions", "globalProperties", "objectTypes", "enums", "typedefs", "funcdefs")


def validate_schema(api: dict[str, Any]) -> None:
    if api.get("schemaVersion") != 1:
        raise ValueError(f"Unsupported script API schemaVersion: {api.get('schemaVersion')!r}")


def clean_decl(value: Any) -> str:
    return str(value or "").strip().rstrip(";")


def valid_decl(decl: str) -> bool:
    if not decl:
        return False
    if "?" in decl or "$beh" in decl or "<unrecognized token>" in decl:
        return False
    return True


def item_name(item: dict[str, Any]) -> str:
    return str(item.get("displayName") or item.get("name") or "")


def safe_identifier(value: str, fallback: str = "Value") -> str:
    value = re.sub(r"[^A-Za-z0-9_]+", "_", value.strip()).strip("_")
    if not value:
        value = fallback
    if value[0].isdigit():
        value = f"_{value}"
    return value


def hidden_member(item: dict[str, Any]) -> bool:
    name = str(item.get("name") or "")
    decl = clean_decl(item.get("declaration"))
    role = str(item.get("memberRole") or "")
    return role == "internal" or name.startswith("$") or "$beh" in decl


def namespace_groups(items: Iterable[dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in items:
        grouped[str(item.get("namespace") or "")].append(item)
    return grouped


def emit_namespace(lines: list[str], namespace: str, body: Iterable[str]) -> None:
    body_lines = list(body)
    if not body_lines:
        return
    if namespace:
        lines.append(f"namespace {namespace} {{")
        lines.extend(f"    {line}" if line else "" for line in body_lines)
        lines.append("}")
    else:
        lines.extend(body_lines)
    lines.append("")


def emit_typedefs(lines: list[str], api: dict[str, Any]) -> None:
    for namespace, items in sorted(namespace_groups(api.get("typedefs", [])).items()):
        body: list[str] = []
        for item in sorted(items, key=lambda value: item_name(value).lower()):
            name = str(item.get("name") or "")
            aliased = str(item.get("aliasedType") or "")
            if IDENT_RE.match(name) and aliased and valid_decl(aliased):
                body.append(f"typedef {aliased} {name};")
        emit_namespace(lines, namespace, body)


def emit_funcdefs(lines: list[str], api: dict[str, Any]) -> None:
    for namespace, items in sorted(namespace_groups(api.get("funcdefs", [])).items()):
        body: list[str] = []
        for item in sorted(items, key=lambda value: item_name(value).lower()):
            decl = clean_decl(item.get("declaration") or item.get("declarationWithNamespace"))
            if not decl or decl == item_name(item) or "(" not in decl:
                continue
            if not decl.startswith("funcdef "):
                decl = f"funcdef {decl}"
            if valid_decl(decl):
                body.append(f"{decl};")
        emit_namespace(lines, namespace, body)


def emit_enums(lines: list[str], api: dict[str, Any]) -> None:
    for namespace, items in sorted(namespace_groups(api.get("enums", [])).items()):
        body: list[str] = []
        for enum in sorted(items, key=lambda value: item_name(value).lower()):
            name = item_name(enum)
            if not IDENT_RE.match(name):
                continue
            body.append(f"enum {name} {{")
            values = sorted(enum.get("values", []), key=lambda value: str(value.get("name") or ""))
            for index, value in enumerate(values):
                value_name = safe_identifier(str(value.get("name") or ""), "Value")
                suffix = "," if index + 1 < len(values) else ""
                body.append(f"    {value_name} = {int(value.get('value') or 0)}{suffix}")
            body.append("}")
            body.append("")
        emit_namespace(lines, namespace, body)


def emit_object_types(lines: list[str], api: dict[str, Any]) -> None:
    for namespace, items in sorted(namespace_groups(api.get("objectTypes", [])).items()):
        body: list[str] = []
        for obj in sorted(items, key=lambda value: item_name(value).lower()):
            decl = clean_decl(obj.get("declaration") or f"class {item_name(obj)}")
            if not valid_decl(decl):
                continue
            body.append(f"{decl} {{")
            members: list[str] = []
            for collection in ("factories", "methods", "properties"):
                for member in sorted(obj.get(collection, []), key=lambda value: (str(value.get("name") or ""), str(value.get("declaration") or ""))):
                    if hidden_member(member):
                        continue
                    member_decl = clean_decl(member.get("declaration"))
                    if valid_decl(member_decl):
                        members.append(f"    {member_decl};")
            for funcdef in sorted(obj.get("childFuncdefs", []), key=lambda value: str(value.get("name") or "")):
                funcdef_decl = clean_decl(funcdef.get("declaration") or funcdef.get("declarationWithNamespace"))
                if funcdef_decl and not funcdef_decl.startswith("funcdef "):
                    funcdef_decl = f"funcdef {funcdef_decl}"
                if valid_decl(funcdef_decl):
                    members.append(f"    {funcdef_decl};")
            body.extend(dict.fromkeys(members))
            body.append("}")
            body.append("")
        emit_namespace(lines, namespace, body)


def emit_global_properties(lines: list[str], api: dict[str, Any]) -> None:
    for namespace, items in sorted(namespace_groups(api.get("globalProperties", [])).items()):
        body: list[str] = []
        for prop in sorted(items, key=lambda value: (str(value.get("name") or ""), str(value.get("declaration") or ""))):
            decl = clean_decl(prop.get("declaration") or f"{prop.get('type') or 'auto'} {prop.get('name') or ''}")
            if valid_decl(decl) and IDENT_RE.match(str(prop.get("name") or "")):
                body.append(f"{decl};")
        emit_namespace(lines, namespace, body)


def emit_global_functions(lines: list[str], api: dict[str, Any]) -> None:
    for namespace, items in sorted(namespace_groups(api.get("globalFunctions", [])).items()):
        body: list[str] = []
        for fn in sorted(items, key=lambda value: (str(value.get("name") or ""), str(value.get("declaration") or ""))):
            decl = clean_decl(fn.get("declaration") or fn.get("declarationWithNamespace"))
            if valid_decl(decl):
                body.append(f"{decl};")
        emit_namespace(lines, namespace, body)


def generate(api: dict[str, Any], append_files: Iterable[Path] = ()) -> str:
    validate_schema(api)
    total = sum(len(api.get(collection, [])) for collection in COLLECTIONS)
    lines = [
        "// Generated by tools/generate_as_predefined.py.",
        "// Edit the API export source or regenerate this file instead of editing by hand.",
        f"// AngelScript version: {api.get('angelScriptVersion') or 'unknown'}",
        f"// Exported symbols: {total}",
        "",
    ]
    emit_typedefs(lines, api)
    emit_funcdefs(lines, api)
    emit_enums(lines, api)
    emit_object_types(lines, api)
    emit_global_properties(lines, api)
    emit_global_functions(lines, api)
    for path in append_files:
        text = read_append_file(path)
        if not text:
            continue
        lines.append("")
        lines.append(f"// Appended from {path}.")
        lines.append(text)
    return "\n".join(lines).rstrip() + "\n"


def patch_bml_script_mod_api(text: str) -> str:
    if "class ObjectLoadResult {" in text:
        text = text.replace("  bool Success;\n  int Count;\n", "  bool get_Success() const;\n  int get_Count() const;\n")
    if "class ModContext {" in text and "float GetTimeMs() const;" not in text:
        anchor = "  CKTimeManager@ BorrowTimeManager() const;\n"
        replacement = (
            anchor
            + "  float GetTimeMs() const;\n"
            + "  float GetAbsoluteTimeMs() const;\n"
            + "  float GetDeltaTimeMs() const;\n"
            + "  uint GetFrameCount() const;\n"
        )
        text = text.replace(anchor, replacement, 1)
    return text


def read_append_file(path: Path) -> str:
    text = path.read_text(encoding="utf-8-sig").strip()
    if path.name.lower() == "bml-script-mod-api.as":
        text = patch_bml_script_mod_api(text)
    return text


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-json", type=Path, default=Path("build/docs/script-api.json"))
    parser.add_argument("--output", type=Path, default=Path("as.predefined"))
    parser.add_argument("--append-file", type=Path, action="append", default=[], help="Extra AngelScript stub/predefined file to append verbatim.")
    args = parser.parse_args()

    api = json.loads(args.api_json.read_text(encoding="utf-8-sig"))
    args.output.write_text(generate(api, args.append_file), encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
