#!/usr/bin/env python3
"""Generate AngelScript catalog hints from exported Virtools metadata JSON."""

from __future__ import annotations

import argparse
import json
import keyword
import re
from pathlib import Path
from typing import Any, Iterable


def read_json_array(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    text = path.read_text(encoding="utf-8-sig")
    if not text.strip():
        return []
    data = json.loads(text)
    if data is None:
        return []
    if isinstance(data, list):
        return [item for item in data if isinstance(item, dict)]
    return [data] if isinstance(data, dict) else []


def u32(value: Any) -> int:
    return int(value) & 0xFFFFFFFF


def hex_u32(value: Any) -> str:
    return f"0x{u32(value):08x}"


def guid_parts(item: dict[str, Any]) -> tuple[str, str]:
    guid = item.get("guid")
    if isinstance(guid, list) and len(guid) >= 2:
        return hex_u32(guid[0]), hex_u32(guid[1])

    guid_hex = item.get("guid_hex")
    if isinstance(guid_hex, str):
        match = re.fullmatch(r"0x([0-9a-fA-F]+)-0x([0-9a-fA-F]+)", guid_hex)
        if match:
            return f"0x{match.group(1)}", f"0x{match.group(2)}"

    return "0x00000000", "0x00000000"


def guid_expr(item: dict[str, Any]) -> str:
    first, second = guid_parts(item)
    return f"CKGUID({first}, {second})"


def as_string(value: Any) -> str:
    text = str(value or "")
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"').replace("\r", "\\r").replace("\n", "\\n") + '"'


AS_RESERVED = {
    "and",
    "auto",
    "bool",
    "break",
    "case",
    "cast",
    "class",
    "const",
    "continue",
    "default",
    "do",
    "else",
    "enum",
    "false",
    "float",
    "for",
    "funcdef",
    "if",
    "import",
    "in",
    "inout",
    "int",
    "interface",
    "is",
    "mixin",
    "namespace",
    "null",
    "or",
    "out",
    "private",
    "return",
    "shared",
    "super",
    "switch",
    "this",
    "true",
    "uint",
    "void",
    "while",
    "xor",
}


def identifier(name: Any, used: set[str]) -> str:
    text = re.sub(r"[^A-Za-z0-9_]", "_", str(name or "")).strip("_")
    if not text:
        text = "Item"
    if text[0].isdigit():
        text = f"_{text}"
    if text in AS_RESERVED or keyword.iskeyword(text):
        text = f"_{text}"

    base = text
    suffix = 2
    while text in used:
        text = f"{base}_{suffix}"
        suffix += 1
    used.add(text)
    return text


def add_guid_function(lines: list[str], name: str, item: dict[str, Any]) -> None:
    lines.append(f"    CKGUID {name}() {{ return {guid_expr(item)}; }}")


def bb_qualified_name(item: dict[str, Any]) -> str:
    category = str(item.get("category") or "")
    name = str(item.get("name") or "")
    return name if not category else f"{category}/{name}"


def bb_slot_items(item: dict[str, Any], key: str) -> list[str]:
    values = item.get(key)
    if isinstance(values, list):
        result: list[str] = []
        for value in values:
            if isinstance(value, str):
                result.append(value)
            elif isinstance(value, dict):
                result.append(str(value.get("name") or ""))
        return result
    return []

def guid_text(item: dict[str, Any]) -> str:
    first, second = guid_parts(item)
    return f"{first.lower()}-{second.lower()}"


def bb_matches_query(item: dict[str, Any], query: str) -> bool:
    text = query.strip()
    if not text:
        return False
    lowered = text.lower()
    return lowered in {
        str(item.get("name") or "").lower(),
        bb_qualified_name(item).lower(),
        guid_text(item),
        f"guid:{guid_text(item)}",
    }


def selected_bbs(bbs: list[dict[str, Any]], queries: Iterable[str]) -> list[dict[str, Any]]:
    selected: list[dict[str, Any]] = []
    seen: set[str] = set()
    for raw in queries:
        for query in str(raw).split(","):
            query = query.strip()
            if not query:
                continue
            for bb in bbs:
                if bb_matches_query(bb, query):
                    key = guid_text(bb)
                    if key not in seen:
                        selected.append(bb)
                        seen.add(key)
                    break
    return selected


def add_bb_slot_helpers(lines: list[str], bb: dict[str, Any]) -> None:
    slot_specs = [
        ("In", "input", "inputs"),
        ("Out", "output", "outputs"),
        ("Pin", "pin", "input_params"),
        ("Pout", "pout", "output_params"),
        ("Setting", "setting", "settings"),
        ("Local", "local", "local_params"),
    ]
    for method, metadata_key, key in slot_specs:
        used_names: set[str] = set()
        for name in bb_slot_items(bb, key):
            function_name = identifier(name or f"{method}Slot", used_names)
            lines.append(f"    BBSlot@ {method}_{function_name}(const CKBehaviorContext &in ctx) {{ BBDecl@ decl = Decl(ctx); return decl is null ? null : decl.{method}({as_string(name)}); }}")
            lines.append(f"    string {method}_{function_name}_Metadata(const string &in fromField) {{ return \"[bbslot from=\\\"\" + fromField + \"\\\" {metadata_key}=\\\"\" + {as_string(name)} + \"\\\"]\"; }}")


def add_selected_bb_wrapper(lines: list[str], bb: dict[str, Any], wrapper_name: str) -> None:
    hint_name = wrapper_name
    lines.append(f"class {wrapper_name} {{")
    lines.append("    BBConfig@ config;")
    lines.append("    BBInstance@ instance;")
    used_slot_fields: set[str] = set()
    for name in bb_slot_items(bb, "input_params"):
        field = identifier(name or "Pin", used_slot_fields)
        lines.append(f"    BBSlot@ {field};")
    lines.append("")
    lines.append("    bool Bind(const CKBehaviorContext &in ctx) {")
    lines.append(f"        @config = CKASCatalog::BBHints::{hint_name}::Config(ctx);")
    lines.append("        if (config is null || !config.IsValid()) return false;")
    used_slot_fields.clear()
    for name in bb_slot_items(bb, "input_params"):
        field = identifier(name or "Pin", used_slot_fields)
        lines.append(f"        @{field} = config.Pin({as_string(name)});")
    lines.append("        return true;")
    lines.append("    }")
    lines.append("")
    lines.append("    BBInstance@ EnsureStarted(const CKBehaviorContext &in ctx) {")
    lines.append("        if (config is null && !Bind(ctx)) return null;")
    lines.append("        @instance = config.EnsureStarted(ctx);")
    lines.append("        return instance;")
    lines.append("    }")
    used_methods: set[str] = set()
    used_slot_fields.clear()
    for name in bb_slot_items(bb, "input_params"):
        field = identifier(name or "Pin", used_slot_fields)
        method = identifier(f"Set_{name}", used_methods)
        lines.append("")
        lines.append(f"    bool {method}(const CKBehaviorContext &in ctx, const string &in value) {{")
        lines.append("        BBInstance@ live = EnsureStarted(ctx);")
        lines.append(f"        return live !is null && live.StepSet(ctx, {field}, value);")
        lines.append("    }")
    lines.append("}")
    lines.append("")


def value_items(params: Iterable[dict[str, Any]], category: str) -> Iterable[dict[str, Any]]:
    for param in params:
        if param.get("category") == category and isinstance(param.get("values"), list):
            yield param


def generate(params: list[dict[str, Any]],
             ops: list[dict[str, Any]],
             bbs: list[dict[str, Any]],
             selected: list[dict[str, Any]] | None = None) -> str:
    lines: list[str] = [
        "// Generated by tools/generate_angelscript_catalog.py.",
        "// Edit the source JSON or regenerate this file instead of editing by hand.",
        "",
        "namespace CKASCatalog {",
    ]

    lines.append("namespace ParamTypes {")
    used_param_names: set[str] = set()
    for param in params:
        add_guid_function(lines, identifier(param.get("name"), used_param_names), param)
    lines.append("}")
    lines.append("")

    lines.append("namespace Enums {")
    used_enum_type_names: set[str] = set()
    for param in value_items(params, "enum"):
        type_name = identifier(param.get("name"), used_enum_type_names)
        used_value_names: set[str] = set()
        lines.append(f"namespace {type_name} {{")
        lines.append(f"    CKGUID Type() {{ return {guid_expr(param)}; }}")
        lines.append("    ParamTypeInfo@ Info(const CKBehaviorContext &in ctx) { return Param::Type(ctx, Type()); }")
        lines.append("    string Text(const CKBehaviorContext &in ctx, int value) { return Param::Text(ctx, Type(), value); }")
        for value in param["values"]:
            lines.append(f"    const int {identifier(value.get('name'), used_value_names)} = {int(value.get('value', 0))};")
        lines.append("}")
    lines.append("}")
    lines.append("")

    lines.append("namespace Flags {")
    used_flags_type_names: set[str] = set()
    for param in value_items(params, "flags"):
        type_name = identifier(param.get("name"), used_flags_type_names)
        used_value_names: set[str] = set()
        lines.append(f"namespace {type_name} {{")
        lines.append(f"    CKGUID Type() {{ return {guid_expr(param)}; }}")
        lines.append("    ParamTypeInfo@ Info(const CKBehaviorContext &in ctx) { return Param::Type(ctx, Type()); }")
        lines.append("    string Text(const CKBehaviorContext &in ctx, uint value) { return Param::Text(ctx, Type(), value); }")
        lines.append("    uint Mask(const CKBehaviorContext &in ctx, const string &in namesOrMask) { return Param::FlagsMask(ctx, Type(), namesOrMask); }")
        for value in param["values"]:
            lines.append(f"    const uint {identifier(value.get('name'), used_value_names)} = {hex_u32(value.get('value', 0))};")
        lines.append("}")
    lines.append("}")
    lines.append("")

    lines.append("namespace Operations {")
    used_op_names: set[str] = set()
    for op in ops:
        add_guid_function(lines, identifier(op.get("name"), used_op_names), op)
    lines.append("}")
    lines.append("")

    lines.append("namespace OperationHints {")
    used_op_hint_names: set[str] = set()
    for op in ops:
        op_name = identifier(op.get("name"), used_op_hint_names)
        lines.append(f"namespace {op_name} {{")
        lines.append(f"    CKGUID Guid() {{ return {guid_expr(op)}; }}")
        lines.append("    ParamOp@ Create(const CKBehaviorContext &in ctx) { return Param::Operation(ctx, Guid()); }")
        lines.append("}")
    lines.append("}")
    lines.append("")

    lines.append("namespace BBs {")
    used_bb_names: set[str] = set()
    for bb in bbs:
        category = str(bb.get("category") or "")
        name = str(bb.get("name") or "")
        qualified = name if not category else f"{category}_{name}"
        add_guid_function(lines, identifier(qualified, used_bb_names), bb)
    lines.append("}")
    lines.append("")

    lines.append("namespace BBHints {")
    used_bb_hint_names: set[str] = set()
    for bb in bbs:
        category = str(bb.get("category") or "")
        name = str(bb.get("name") or "")
        qualified = bb_qualified_name(bb)
        hint_name = identifier(qualified.replace("/", "_"), used_bb_hint_names)
        lines.append(f"namespace {hint_name} {{")
        lines.append(f"    CKGUID Guid() {{ return {guid_expr(bb)}; }}")
        lines.append(f"    const string Name = {as_string(name)};")
        lines.append(f"    const string Category = {as_string(category)};")
        lines.append(f"    const string QualifiedName = {as_string(qualified)};")
        lines.append("    BBPrototype@ Find(const CKBehaviorContext &in ctx) { return BB::Prototype(ctx, Guid()); }")
        lines.append("    BBDecl@ Decl(const CKBehaviorContext &in ctx) { return BB::Require(ctx, Guid()); }")
        lines.append("    BBConfig@ Config(const CKBehaviorContext &in ctx) { BBDecl@ decl = Decl(ctx); return decl is null ? null : decl.Configure(); }")
        add_bb_slot_helpers(lines, bb)
        lines.append("}")
    lines.append("}")
    lines.append("}")
    if selected:
        lines.append("")
        lines.append("namespace SelectedBBWrappers {")
        used_wrapper_names: set[str] = set()
        used_hint_names: set[str] = set()
        hint_names_by_guid: dict[str, str] = {}
        for bb in bbs:
            hint_names_by_guid[guid_text(bb)] = identifier(bb_qualified_name(bb).replace("/", "_"), used_hint_names)
        for bb in selected:
            wrapper_name = identifier(bb_qualified_name(bb).replace("/", "_"), used_wrapper_names)
            add_selected_bb_wrapper(lines, bb, hint_names_by_guid.get(guid_text(bb), wrapper_name))
        lines.append("}")
    lines.append("")

    return "\n".join(lines)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    default_validation_dir = root / "build" / "validation" / "ballance"

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validation-dir", type=Path, default=default_validation_dir)
    parser.add_argument("--params-json", type=Path)
    parser.add_argument("--ops-json", type=Path)
    parser.add_argument("--bbs-json", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--selected-bb", action="append", default=[], help="Name, Category/Name, GUID, or comma-separated list of BBs to wrap.")
    parser.add_argument("--selected-bbs-file", type=Path, help="Text file with one selected BB query per line.")
    args = parser.parse_args()

    params_json = args.params_json or args.validation_dir / "params.json"
    ops_json = args.ops_json or args.validation_dir / "ops.json"
    bbs_json = args.bbs_json or args.validation_dir / "bbs.json"
    output = args.output or args.validation_dir / "CKAngelScriptCatalog.as"

    params = read_json_array(params_json)
    ops = read_json_array(ops_json)
    bbs = read_json_array(bbs_json)
    selected_queries = list(args.selected_bb)
    if args.selected_bbs_file and args.selected_bbs_file.exists():
        selected_queries.extend(line.strip() for line in args.selected_bbs_file.read_text(encoding="utf-8-sig").splitlines() if line.strip() and not line.strip().startswith("#"))
    selected = selected_bbs(bbs, selected_queries)

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generate(params, ops, bbs, selected), encoding="utf-8")

    print(json.dumps({
        "params": len(params),
        "operations": len(ops),
        "building_blocks": len(bbs),
        "selected_wrappers": len(selected),
        "output": str(output.resolve()),
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
