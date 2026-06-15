#!/usr/bin/env python3
"""Generate Natural Docs AngelScript stubs from CKAngelScript API JSON."""

from __future__ import annotations

import argparse
import json
import re
import shutil
import tempfile
from pathlib import Path
from typing import Any, Iterable


IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")

CATEGORY_ORDER = ["Scene", "Runtime", "Behavior", "BB", "Param", "Message", "Async", ""]


def sort_key(item: dict[str, Any]) -> tuple[str, str, str]:
    return (
        str(item.get("namespace") or ""),
        str(item.get("name") or ""),
        str(item.get("declarationWithNamespace") or item.get("declaration") or ""),
    )


def title(value: str) -> str:
    return value if value else "<global>"


def safe_identifier(value: str) -> str:
    value = value.strip()
    if IDENT_RE.match(value):
        return value
    value = re.sub(r"[^A-Za-z0-9_]", "_", value)
    if not value or value[0].isdigit():
        value = f"_{value}"
    return value


def clean_decl(decl: str) -> str:
    return decl.strip().rstrip(";")


def is_reasonable_stub_decl(decl: str) -> bool:
    if not decl:
        return False
    if "?" in decl:
        return False
    return True


def category_for(item: dict[str, Any], fallback_name: str = "") -> str:
    exported = str(item.get("apiCategory") or "")
    if exported:
        return exported
    namespace = str(item.get("namespace") or "")
    if not namespace:
        return "global"
    return re.sub(r"[^A-Za-z0-9]+", "_", namespace).strip("_").lower() or "global"


def category_name(item_or_key: dict[str, Any] | str) -> str:
    if isinstance(item_or_key, dict):
        namespace = str(item_or_key.get("namespace") or "")
        return str(item_or_key.get("apiCategoryName") or ("Global namespace" if not namespace else f"Namespace {namespace}"))
    return "Global namespace" if item_or_key == "global" else f"Namespace {item_or_key}"


def category_sort_key(key: str) -> tuple[int, str]:
    namespace = "" if key == "global" else key
    namespace_title = namespace.lower()
    try:
        return (CATEGORY_ORDER.index(namespace), key)
    except ValueError:
        for index, preferred in enumerate(CATEGORY_ORDER):
            if preferred.lower() == namespace_title:
                return (index, key)
        return (len(CATEGORY_ORDER), key)


def category_file_name(index: int, key: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9_]+", "_", key).strip("_").lower() or "misc"
    return f"{index:02d}_{safe}.as"


def metadata_lines(item: dict[str, Any], indent: str = "//") -> list[str]:
    details: list[str] = []
    namespace = str(item.get("namespace") or "")
    if namespace:
        details.append(f"Namespace: {namespace}")
    audience = str(item.get("apiAudience") or "")
    if audience:
        details.append(f"Audience: {audience}")
    source = str(item.get("apiSourceArea") or "")
    if source:
        details.append(f"Source: {source}")
    kind = str(item.get("kind") or "")
    if kind:
        details.append(f"Kind: {kind}")
    flags = item.get("flagNames")
    if isinstance(flags, list) and flags:
        details.append("Flags: " + ", ".join(str(flag) for flag in flags))
    if not details:
        return []
    lines = [indent]
    for detail in details:
        lines.append(f"{indent} {detail}")
    return lines


def emit_topic(lines: list[str], kind: str, name: str, declaration: str = "") -> None:
    lines.append(f"// {kind}: {title(name)}")
    if declaration:
        lines.append("//")
        lines.append("// Declaration:")
        lines.append(f"//   {declaration}")


def emit_item_topic(lines: list[str], kind: str, item: dict[str, Any], declaration: str = "", indent: str = "") -> None:
    comment = f"{indent}//"
    lines.append(f"{comment} {kind}: {title(str(item.get('name') or ''))}")
    if declaration:
        lines.append(comment)
        lines.append(f"{comment} Declaration:")
        lines.append(f"{comment}   {declaration}")
    lines.extend(metadata_lines(item, comment))


def with_namespace(lines: list[str], namespace: str, body: Iterable[str]) -> None:
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


def emit_global_functions(api: dict[str, Any]) -> str:
    lines = [
        "// Title: Global Functions",
        "",
    ]
    by_namespace: dict[str, list[dict[str, Any]]] = {}
    for fn in sorted(api.get("globalFunctions", []), key=sort_key):
        by_namespace.setdefault(str(fn.get("namespace") or ""), []).append(fn)

    for namespace in sorted(by_namespace):
        body: list[str] = []
        if namespace:
            emit_topic(body, "Namespace", namespace)
            body.append("")
        for fn in by_namespace[namespace]:
            decl = clean_decl(str(fn.get("declaration") or fn.get("declarationWithNamespace") or ""))
            emit_topic(body, "Function", str(fn.get("name") or ""), decl)
            if is_reasonable_stub_decl(decl):
                body.append(f"{decl};")
            body.append("")
        with_namespace(lines, namespace, body)
    return "\n".join(lines).rstrip() + "\n"


def emit_global_properties(api: dict[str, Any]) -> str:
    lines = [
        "// Title: Global Properties",
        "",
    ]
    by_namespace: dict[str, list[dict[str, Any]]] = {}
    for prop in sorted(api.get("globalProperties", []), key=sort_key):
        by_namespace.setdefault(str(prop.get("namespace") or ""), []).append(prop)

    for namespace in sorted(by_namespace):
        body: list[str] = []
        if namespace:
            emit_topic(body, "Namespace", namespace)
            body.append("")
        for prop in by_namespace[namespace]:
            prop_type = str(prop.get("type") or "auto")
            name = str(prop.get("name") or "")
            decl = f"{prop_type} {name}".strip()
            emit_topic(body, "Variable", name, decl)
            if is_reasonable_stub_decl(decl) and IDENT_RE.match(name):
                body.append(f"{decl};")
            body.append("")
        with_namespace(lines, namespace, body)
    return "\n".join(lines).rstrip() + "\n"


def emit_enums(api: dict[str, Any]) -> str:
    lines = [
        "// Title: Enums",
        "",
    ]
    by_namespace: dict[str, list[dict[str, Any]]] = {}
    for enum in sorted(api.get("enums", []), key=sort_key):
        by_namespace.setdefault(str(enum.get("namespace") or ""), []).append(enum)

    for namespace in sorted(by_namespace):
        body: list[str] = []
        if namespace:
            emit_topic(body, "Namespace", namespace)
            body.append("")
        for enum in by_namespace[namespace]:
            name = str(enum.get("name") or "")
            emit_topic(body, "Enum", name)
            if IDENT_RE.match(name):
                body.append(f"enum {name} {{")
                values = sorted(enum.get("values", []), key=lambda value: str(value.get("name") or ""))
                for index, value in enumerate(values):
                    suffix = "," if index + 1 < len(values) else ""
                    body.append(f"    {safe_identifier(str(value.get('name') or 'Value'))} = {int(value.get('value') or 0)}{suffix}")
                body.append("}")
            body.append("")
        with_namespace(lines, namespace, body)
    return "\n".join(lines).rstrip() + "\n"


def emit_typedefs(api: dict[str, Any]) -> str:
    lines = [
        "// Title: Typedefs",
        "",
    ]
    by_namespace: dict[str, list[dict[str, Any]]] = {}
    for typedef in sorted(api.get("typedefs", []), key=sort_key):
        by_namespace.setdefault(str(typedef.get("namespace") or ""), []).append(typedef)

    for namespace in sorted(by_namespace):
        body: list[str] = []
        if namespace:
            emit_topic(body, "Namespace", namespace)
            body.append("")
        for typedef in by_namespace[namespace]:
            name = str(typedef.get("name") or "")
            aliased = str(typedef.get("aliasedType") or "")
            decl = f"typedef {aliased} {name}".strip()
            emit_topic(body, "Typedef", name, decl)
            if is_reasonable_stub_decl(decl) and IDENT_RE.match(name):
                body.append(f"{decl};")
            body.append("")
        with_namespace(lines, namespace, body)
    return "\n".join(lines).rstrip() + "\n"


def emit_funcdefs(api: dict[str, Any]) -> str:
    lines = [
        "// Title: Funcdefs",
        "",
    ]
    by_namespace: dict[str, list[dict[str, Any]]] = {}
    for funcdef in sorted(api.get("funcdefs", []), key=sort_key):
        by_namespace.setdefault(str(funcdef.get("namespace") or ""), []).append(funcdef)

    for namespace in sorted(by_namespace):
        body: list[str] = []
        if namespace:
            emit_topic(body, "Namespace", namespace)
            body.append("")
        for funcdef in by_namespace[namespace]:
            name = str(funcdef.get("name") or "")
            decl = clean_decl(str(funcdef.get("declaration") or ""))
            stub_decl = f"funcdef {decl}" if decl else ""
            emit_topic(body, "Function Type", name, stub_decl)
            if is_reasonable_stub_decl(stub_decl):
                body.append(f"{stub_decl};")
            body.append("")
        with_namespace(lines, namespace, body)
    return "\n".join(lines).rstrip() + "\n"


def emit_types(api: dict[str, Any]) -> str:
    lines = [
        "// Title: Classes",
        "",
    ]
    by_namespace: dict[str, list[dict[str, Any]]] = {}
    for obj in sorted(api.get("objectTypes", []), key=sort_key):
        by_namespace.setdefault(str(obj.get("namespace") or ""), []).append(obj)

    for namespace in sorted(by_namespace):
        body: list[str] = []
        if namespace:
            emit_topic(body, "Namespace", namespace)
            body.append("")
        for obj in by_namespace[namespace]:
            name = str(obj.get("name") or "")
            emit_topic(body, "Class", name)
            class_name = safe_identifier(name)
            body.append(f"class {class_name} {{")

            for prop in sorted(obj.get("properties", []), key=sort_key):
                decl = clean_decl(str(prop.get("declaration") or ""))
                prop_name = str(prop.get("name") or "")
                body.append("")
                body.append(f"    // Variable: {title(prop_name)}")
                if decl:
                    body.append("    //")
                    body.append("    // Declaration:")
                    body.append(f"    //   {decl}")
                if is_reasonable_stub_decl(decl):
                    body.append(f"    {decl};")

            for method in sorted(obj.get("methods", []), key=sort_key):
                decl = clean_decl(str(method.get("declaration") or ""))
                method_name = str(method.get("name") or "")
                body.append("")
                body.append(f"    // Function: {title(method_name)}")
                if decl:
                    body.append("    //")
                    body.append("    // Declaration:")
                    body.append(f"    //   {decl}")
                if is_reasonable_stub_decl(decl):
                    body.append(f"    {decl};")

            body.append("}")
            body.append("")
        with_namespace(lines, namespace, body)
    return "\n".join(lines).rstrip() + "\n"


def emit_category(api: dict[str, Any], key: str) -> str:
    members = {
        "functions": [fn for fn in api.get("globalFunctions", []) if category_for(fn) == key],
        "properties": [prop for prop in api.get("globalProperties", []) if category_for(prop) == key],
        "types": [obj for obj in api.get("objectTypes", []) if category_for(obj) == key],
        "enums": [enum for enum in api.get("enums", []) if category_for(enum) == key],
        "typedefs": [typedef for typedef in api.get("typedefs", []) if category_for(typedef) == key],
        "funcdefs": [funcdef for funcdef in api.get("funcdefs", []) if category_for(funcdef) == key],
    }
    representative = next((items[0] for items in members.values() if items), {})
    name = category_name(representative) if representative else category_name(key)

    lines = [
        f"// Title: {name}",
        "",
        f"// Topic: {name} Overview",
        "//",
        f"// Category: {key}",
    ]

    audience = str(representative.get("apiAudience") or "")
    source = str(representative.get("apiSourceArea") or "")
    if audience:
        lines.append(f"// Audience: {audience}")
    if source:
        lines.append(f"// Source: {source}")
    lines.append("")

    for obj in sorted(members["types"], key=sort_key):
        obj_name = str(obj.get("name") or "")
        class_name = safe_identifier(obj_name)
        emit_item_topic(lines, "Class", obj)
        lines.append(f"class {class_name} {{")

        for factory in sorted(obj.get("factories", []), key=sort_key):
            decl = clean_decl(str(factory.get("declaration") or ""))
            lines.append("")
            emit_item_topic(lines, "Function", factory, decl, "    ")
            if is_reasonable_stub_decl(decl):
                lines.append(f"    {decl};")

        for prop in sorted(obj.get("properties", []), key=sort_key):
            decl = clean_decl(str(prop.get("declaration") or ""))
            lines.append("")
            emit_item_topic(lines, "Variable", prop, decl, "    ")
            if is_reasonable_stub_decl(decl):
                lines.append(f"    {decl};")

        for method in sorted(obj.get("methods", []), key=sort_key):
            decl = clean_decl(str(method.get("declaration") or ""))
            lines.append("")
            emit_item_topic(lines, "Function", method, decl, "    ")
            if is_reasonable_stub_decl(decl):
                lines.append(f"    {decl};")

        behaviours = sorted(obj.get("behaviours", []), key=lambda value: (str(value.get("behaviour") or ""), str(value.get("function", {}).get("declaration") or "")))
        for behaviour in behaviours:
            function = dict(behaviour.get("function") or {})
            function["name"] = str(behaviour.get("behaviour") or function.get("name") or "")
            decl = clean_decl(str(function.get("declaration") or ""))
            lines.append("")
            emit_item_topic(lines, "Function", function, decl, "    ")
            lines.append(f"    // Behaviour: {function['name']}")
            if is_reasonable_stub_decl(decl):
                lines.append(f"    {decl};")

        lines.append("}")
        lines.append("")

    by_namespace: dict[str, list[dict[str, Any]]] = {}
    for fn in sorted(members["functions"], key=sort_key):
        by_namespace.setdefault(str(fn.get("namespace") or ""), []).append(fn)
    for namespace in sorted(by_namespace):
        body: list[str] = []
        for fn in by_namespace[namespace]:
            decl = clean_decl(str(fn.get("declaration") or fn.get("declarationWithNamespace") or ""))
            emit_item_topic(body, "Function", fn, decl)
            if is_reasonable_stub_decl(decl):
                body.append(f"{decl};")
            body.append("")
        with_namespace(lines, namespace, body)

    by_namespace = {}
    for prop in sorted(members["properties"], key=sort_key):
        by_namespace.setdefault(str(prop.get("namespace") or ""), []).append(prop)
    for namespace in sorted(by_namespace):
        body = []
        for prop in by_namespace[namespace]:
            prop_type = str(prop.get("type") or "auto")
            prop_name = str(prop.get("name") or "")
            decl = f"{prop_type} {prop_name}".strip()
            emit_item_topic(body, "Variable", prop, decl)
            if is_reasonable_stub_decl(decl) and IDENT_RE.match(prop_name):
                body.append(f"{decl};")
            body.append("")
        with_namespace(lines, namespace, body)

    by_namespace = {}
    for enum in sorted(members["enums"], key=sort_key):
        by_namespace.setdefault(str(enum.get("namespace") or ""), []).append(enum)
    for namespace in sorted(by_namespace):
        body = []
        for enum in by_namespace[namespace]:
            enum_name = str(enum.get("name") or "")
            emit_item_topic(body, "Enum", enum)
            if IDENT_RE.match(enum_name):
                body.append(f"enum {enum_name} {{")
                values = sorted(enum.get("values", []), key=lambda value: str(value.get("name") or ""))
                for index, value in enumerate(values):
                    suffix = "," if index + 1 < len(values) else ""
                    body.append(f"    {safe_identifier(str(value.get('name') or 'Value'))} = {int(value.get('value') or 0)}{suffix}")
                body.append("}")
            body.append("")
        with_namespace(lines, namespace, body)

    by_namespace = {}
    for typedef in sorted(members["typedefs"], key=sort_key):
        by_namespace.setdefault(str(typedef.get("namespace") or ""), []).append(typedef)
    for namespace in sorted(by_namespace):
        body = []
        for typedef in by_namespace[namespace]:
            typedef_name = str(typedef.get("name") or "")
            decl = f"typedef {typedef.get('aliasedType') or ''} {typedef_name}".strip()
            emit_item_topic(body, "Typedef", typedef, decl)
            if is_reasonable_stub_decl(decl) and IDENT_RE.match(typedef_name):
                body.append(f"{decl};")
            body.append("")
        with_namespace(lines, namespace, body)

    by_namespace = {}
    for funcdef in sorted(members["funcdefs"], key=sort_key):
        by_namespace.setdefault(str(funcdef.get("namespace") or ""), []).append(funcdef)
    for namespace in sorted(by_namespace):
        body = []
        for funcdef in by_namespace[namespace]:
            decl = clean_decl(str(funcdef.get("declaration") or funcdef.get("typeDeclaration") or ""))
            stub_decl = f"funcdef {decl}" if decl and not decl.startswith("funcdef ") else decl
            emit_item_topic(body, "Function Type", funcdef, stub_decl)
            if is_reasonable_stub_decl(stub_decl):
                body.append(f"{stub_decl};")
            body.append("")
        with_namespace(lines, namespace, body)

    return "\n".join(lines).rstrip() + "\n"


def validate_schema(api: dict[str, Any]) -> None:
    version = api.get("schemaVersion")
    if version != 1:
        raise ValueError(f"Unsupported script API schemaVersion: {version!r}")


def generate(api_json: Path, out_dir: Path) -> None:
    api = json.loads(api_json.read_text(encoding="utf-8"))
    validate_schema(api)

    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    used_categories = {
        category_for(item)
        for collection in ("globalFunctions", "globalProperties", "objectTypes", "enums", "typedefs", "funcdefs")
        for item in api.get(collection, [])
    }
    files = {
        category_file_name(index, key): emit_category(api, key)
        for index, key in enumerate(sorted(used_categories, key=category_sort_key), start=1)
    }
    for name in sorted(files):
        (out_dir / name).write_text(files[name], encoding="utf-8", newline="\n")


def self_test() -> None:
    sample = {
        "schemaVersion": 1,
        "angelScriptVersion": "2.38.0",
        "engineProperties": {},
        "globalFunctions": [
            {"namespace": "Scene", "name": "FindObject", "declaration": "CKObject@ FindObject(const string &in name)"},
            {"namespace": "", "name": "Print", "declaration": "void Print(const string &in text)"},
        ],
        "globalProperties": [
            {"namespace": "Runtime", "name": "FrameTime", "type": "float"},
        ],
        "objectTypes": [
            {
                "namespace": "",
                "name": "AsyncTask<T>",
                "methods": [
                    {"name": "IsDone", "declaration": "bool IsDone() const"},
                    {"name": "Get", "declaration": "T Get()"},
                ],
                "properties": [
                    {"name": "State", "declaration": "int State"},
                ],
            }
        ],
        "enums": [
            {"namespace": "BB", "name": "PinDirection", "values": [{"name": "In", "value": 0}, {"name": "Out", "value": 1}]}
        ],
        "typedefs": [
            {"namespace": "", "name": "size_t", "aliasedType": "uint"},
        ],
        "funcdefs": [
            {"namespace": "Runtime", "name": "Callback", "declaration": "void Callback(int value)"},
        ],
    }

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        api_json = root / "script-api.json"
        out_dir = root / "out"
        api_json.write_text(json.dumps(sample, sort_keys=True), encoding="utf-8")
        generate(api_json, out_dir)
        first = {path.name: path.read_text(encoding="utf-8") for path in sorted(out_dir.glob("*.as"))}
        generate(api_json, out_dir)
        second = {path.name: path.read_text(encoding="utf-8") for path in sorted(out_dir.glob("*.as"))}
        assert first == second
        assert any("// Function: FindObject" in text for text in first.values())
        assert any("// Enum: PinDirection" in text for text in first.values())
        assert any("// Class: AsyncTask<T>" in text for text in first.values())
        assert any("// Variable: State" in text for text in first.values())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-json", type=Path, help="Path to script-api.json exported from CKAS_EXPORT_SCRIPT_API.")
    parser.add_argument("--out-dir", type=Path, help="Output directory for Natural Docs .as sources.")
    parser.add_argument("--self-test", action="store_true", help="Run generator self-tests.")
    args = parser.parse_args()

    if args.self_test:
        self_test()
        return

    if not args.api_json or not args.out_dir:
        parser.error("--api-json and --out-dir are required unless --self-test is used")
    generate(args.api_json, args.out_dir)


if __name__ == "__main__":
    main()
