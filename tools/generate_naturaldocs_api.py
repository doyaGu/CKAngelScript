#!/usr/bin/env python3
"""Generate Natural Docs AngelScript stubs from CKAngelScript API JSON."""

from __future__ import annotations

import argparse
import fnmatch
import json
import re
import shutil
import tempfile
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable


IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
COLLECTIONS = ("globalFunctions", "globalProperties", "objectTypes", "enums", "typedefs", "funcdefs")


def load_structured(path: Path) -> dict[str, Any]:
    text = path.read_text(encoding="utf-8-sig")
    try:
        import yaml  # type: ignore

        data = yaml.safe_load(text)
        if isinstance(data, dict):
            return data
    except ModuleNotFoundError:
        pass
    data = json.loads(text)
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a mapping")
    return data


def safe_name(value: str) -> str:
    value = re.sub(r"[^A-Za-z0-9_]+", "_", value).strip("_").lower()
    return value or "misc"


def title(value: str) -> str:
    return value if value else "<global>"


def item_name(item: dict[str, Any]) -> str:
    return str(item.get("displayName") or item.get("name") or "")


def clean_decl(decl: str) -> str:
    return decl.strip().rstrip(";")


def valid_decl(decl: str) -> bool:
    if not decl:
        return False
    if "?" in decl or "$beh" in decl:
        return False
    return True


def glob_match(pattern: str, value: str) -> bool:
    return fnmatch.fnmatchcase(value, pattern)


def sort_key(item: dict[str, Any]) -> tuple[str, str, str]:
    return (
        str(item.get("namespace") or ""),
        item_name(item),
        str(item.get("declarationWithNamespace") or item.get("declaration") or ""),
    )


def member_sort_key(item: dict[str, Any]) -> tuple[str, str]:
    return (str(item.get("name") or ""), str(item.get("declaration") or ""))


def safe_identifier(value: str) -> str:
    value = value.strip()
    if IDENT_RE.match(value):
        return value
    value = re.sub(r"[^A-Za-z0-9_<>,]+", "_", value)
    if not value or value[0].isdigit():
        value = f"_{value}"
    return value


def doc_id(kind: str, item: dict[str, Any]) -> str:
    existing = str(item.get("docId") or "")
    if existing:
        return existing
    namespace = str(item.get("namespace") or "")
    declaration = str(item.get("declarationWithNamespace") or item.get("declaration") or item_name(item))
    return "::".join(part.lower().strip() for part in (kind, namespace, declaration))


@dataclass(frozen=True)
class Group:
    id: str
    title: str
    overview: str
    order: int


@dataclass(frozen=True)
class Family:
    id: str
    group: str
    title: str
    overview: str
    order: int


class Metadata:
    def __init__(self, raw: dict[str, Any]) -> None:
        if raw.get("schemaVersion") != 1:
            raise ValueError(f"Unsupported metadata schemaVersion: {raw.get('schemaVersion')!r}")
        self.title = str(raw.get("title") or "CKAngelScript API Reference")
        self.subtitle = str(raw.get("subtitle") or "Generated from the live AngelScript engine.")
        self.fallback_group = str(raw.get("fallbackGroup") or "misc")
        self.namespace_groups = {str(k): str(v) for k, v in dict(raw.get("namespaceGroups") or {}).items()}
        self.namespace_families = {str(k): str(v) for k, v in dict(raw.get("namespaceFamilies") or {}).items()}
        self.type_groups = dict(raw.get("typeGroups") or {})
        self.symbols = dict(raw.get("symbols") or {})
        self.type_patterns = list(raw.get("typePatterns") or [])
        self.symbol_patterns = list(raw.get("symbolPatterns") or [])
        self.funcdefs = dict(raw.get("funcdefs") or {})
        self.quality = dict(raw.get("quality") or {})
        self.links = dict(raw.get("links") or {})
        self.groups: list[Group] = []
        for index, entry in enumerate(raw.get("groups") or []):
            group_id = str(entry["id"])
            self.groups.append(Group(
                id=group_id,
                title=str(entry.get("title") or group_id),
                overview=str(entry.get("overview") or ""),
                order=int(entry.get("order") or index + 1),
            ))
        self.group_by_id = {group.id: group for group in self.groups}
        if self.fallback_group not in self.group_by_id:
            raise ValueError(f"fallbackGroup {self.fallback_group!r} is not declared in groups")
        self.families: list[Family] = []
        for entry in raw.get("families") or []:
            family = Family(
                id=str(entry["id"]),
                group=str(entry["group"]),
                title=str(entry.get("title") or entry["id"]),
                overview=str(entry.get("overview") or ""),
                order=int(entry.get("order") or 1),
            )
            if family.group not in self.group_by_id:
                raise ValueError(f"family {family.id!r} references unknown group {family.group!r}")
            self.families.append(family)
        self.family_by_id = {family.id: family for family in self.families}
        self.families_by_group: dict[str, list[Family]] = defaultdict(list)
        for family in sorted(self.families, key=lambda value: value.order):
            self.families_by_group[family.group].append(family)

    def group(self, group_id: str) -> Group:
        return self.group_by_id.get(group_id, self.group_by_id[self.fallback_group])

    def symbol_override(self, kind: str, item: dict[str, Any]) -> dict[str, Any]:
        keys = [
            doc_id(kind, item),
            f"{kind}:{str(item.get('namespace') or '')}:{item_name(item)}",
            f"{kind}:{item_name(item)}",
        ]
        for key in keys:
            value = self.symbols.get(key)
            if isinstance(value, dict):
                return value
        return {}

    def type_override(self, item: dict[str, Any]) -> dict[str, Any]:
        names = [item_name(item), str(item.get("name") or "")]
        for name in names:
            value = self.type_groups.get(name)
            if isinstance(value, str):
                return {"group": value}
            if isinstance(value, dict):
                return value
        return {}

    def pattern_group(self, kind: str, item: dict[str, Any]) -> str:
        values = [item_name(item), str(item.get("name") or ""), str(item.get("declaration") or "")]
        if kind in {"type", "enum", "typedef", "funcdef"}:
            for rule in self.type_patterns:
                pattern = str(rule.get("pattern") or "")
                if any(glob_match(pattern, value) for value in values):
                    return str(rule.get("group") or self.fallback_group)
        for rule in self.symbol_patterns:
            rule_kind = str(rule.get("kind") or "*")
            pattern = str(rule.get("pattern") or "")
            if rule_kind not in {"*", kind}:
                continue
            if any(glob_match(pattern, value) for value in values):
                return str(rule.get("group") or self.fallback_group)
        return ""

    def group_for(self, kind: str, item: dict[str, Any]) -> str:
        override = self.symbol_override(kind, item)
        if isinstance(override.get("group"), str):
            return str(override["group"])
        if kind in {"type", "enum", "typedef", "funcdef"}:
            type_override = self.type_override(item)
            if isinstance(type_override.get("group"), str):
                return str(type_override["group"])
        namespace = str(item.get("namespace") or "")
        if namespace in self.namespace_groups:
            return self.namespace_groups[namespace]
        pattern = self.pattern_group(kind, item)
        if pattern:
            return pattern
        return self.fallback_group

    def pattern_family(self, kind: str, item: dict[str, Any], group_id: str) -> str:
        values = [item_name(item), str(item.get("name") or ""), str(item.get("declaration") or "")]
        rules = self.type_patterns if kind in {"type", "enum", "typedef", "funcdef"} else []
        rules = [*rules, *self.symbol_patterns]
        for rule in rules:
            rule_kind = str(rule.get("kind") or "*")
            if rule_kind not in {"*", kind}:
                continue
            family = str(rule.get("family") or "")
            if not family:
                continue
            if family not in self.family_by_id or self.family_by_id[family].group != group_id:
                continue
            pattern = str(rule.get("pattern") or "")
            if any(glob_match(pattern, value) for value in values):
                return family
        return ""

    def default_family_for_group(self, group_id: str) -> str:
        families = self.families_by_group.get(group_id) or []
        if families:
            return families[0].id
        return group_id

    def family_for(self, kind: str, item: dict[str, Any], group_id: str) -> str:
        override = self.symbol_override(kind, item)
        if isinstance(override.get("family"), str):
            return str(override["family"])
        if kind in {"type", "enum", "typedef", "funcdef"}:
            type_override = self.type_override(item)
            if isinstance(type_override.get("family"), str):
                return str(type_override["family"])
        namespace = str(item.get("namespace") or "")
        if namespace in self.namespace_families:
            return self.namespace_families[namespace]
        family = self.pattern_family(kind, item, group_id)
        if family:
            return family
        return self.default_family_for_group(group_id)

    def summary_for(self, kind: str, item: dict[str, Any]) -> str:
        override = self.symbol_override(kind, item)
        if isinstance(override.get("summary"), str):
            return str(override["summary"])
        if kind in {"type", "enum", "typedef", "funcdef"}:
            type_override = self.type_override(item)
            if isinstance(type_override.get("summary"), str):
                return str(type_override["summary"])
        return ""

    def declaration_for(self, kind: str, item: dict[str, Any]) -> str:
        override = self.symbol_override(kind, item)
        if isinstance(override.get("declaration"), str):
            return str(override["declaration"])
        if kind == "funcdef":
            funcdef = self.funcdefs.get(item_name(item)) or self.funcdefs.get(str(item.get("name") or ""))
            if isinstance(funcdef, dict) and isinstance(funcdef.get("declaration"), str):
                return str(funcdef["declaration"])
        return str(item.get("declaration") or item.get("declarationWithNamespace") or "")

    def hidden(self, kind: str, item: dict[str, Any]) -> bool:
        override = self.symbol_override(kind, item)
        if override.get("hide") is True:
            return True
        if str(item.get("memberRole") or "") == "internal":
            return True
        declaration = str(item.get("declaration") or "")
        name = str(item.get("name") or "")
        return "$beh" in declaration or name.startswith("$")

    def orphan_symbol_keys(self, known_doc_ids: set[str]) -> list[str]:
        orphans: list[str] = []
        for key in self.symbols:
            if "::" in key and key not in known_doc_ids:
                orphans.append(key)
        return sorted(orphans)


class Quality:
    def __init__(self) -> None:
        self.group_counts: dict[str, int] = defaultdict(int)
        self.family_counts: dict[str, int] = defaultdict(int)
        self.bad_declarations: list[dict[str, str]] = []
        self.hidden_internals: list[dict[str, str]] = []
        self.uncovered: list[dict[str, str]] = []
        self.public_files_with_internal_names: list[str] = []
        self.public_files_with_unknown_types: list[str] = []
        self.missing_required_primary_types: list[str] = []
        self.metadata_orphans: list[str] = []

    def to_dict(self) -> dict[str, Any]:
        return {
            "groupCounts": dict(sorted(self.group_counts.items())),
            "familyCounts": dict(sorted(self.family_counts.items())),
            "badDeclarations": self.bad_declarations,
            "hiddenInternals": self.hidden_internals,
            "uncoveredSymbols": self.uncovered,
            "metadataOrphans": self.metadata_orphans,
            "publicFilesWithInternalNames": self.public_files_with_internal_names,
            "publicFilesWithUnknownTypes": self.public_files_with_unknown_types,
            "missingRequiredPrimaryTypes": self.missing_required_primary_types,
        }

    def assert_pass(self) -> None:
        errors: list[str] = []
        if self.metadata_orphans:
            errors.append(f"metadata references missing docIds: {len(self.metadata_orphans)}")
        if self.public_files_with_internal_names:
            errors.append(f"public docs contain $beh internals: {self.public_files_with_internal_names}")
        if self.public_files_with_unknown_types:
            errors.append(f"public docs contain '?' declarations: {self.public_files_with_unknown_types}")
        if self.missing_required_primary_types:
            errors.append(f"missing required primary API types: {', '.join(self.missing_required_primary_types)}")
        if errors:
            raise RuntimeError("; ".join(errors))


def validate_schema(api: dict[str, Any]) -> None:
    version = api.get("schemaVersion")
    if version != 1:
        raise ValueError(f"Unsupported script API schemaVersion: {version!r}")


def emit_comment(lines: list[str], text: str = "", indent: str = "") -> None:
    if not text:
        lines.append(f"{indent}//")
        return
    for part in text.splitlines():
        lines.append(f"{indent}// {part}".rstrip())


def emit_topic(lines: list[str], kind: str, name: str, summary: str = "", declaration: str = "", indent: str = "") -> None:
    lines.append(f"{indent}// {kind}: {title(name)}")
    if summary:
        emit_comment(lines, "", indent)
        emit_comment(lines, summary, indent)
    if declaration:
        emit_comment(lines, "", indent)
        emit_comment(lines, "Declaration:", indent)
        emit_comment(lines, f"  {declaration}", indent)


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


def record_bad_decl(quality: Quality, kind: str, item: dict[str, Any], declaration: str) -> None:
    if declaration and not valid_decl(declaration):
        quality.bad_declarations.append({
            "kind": kind,
            "name": item_name(item),
            "docId": doc_id(kind, item),
            "declaration": declaration,
        })


def group_overloads(items: list[dict[str, Any]]) -> dict[tuple[str, str], list[dict[str, Any]]]:
    grouped: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    for item in sorted(items, key=sort_key):
        grouped[(str(item.get("namespace") or ""), str(item.get("name") or ""))].append(item)
    return grouped


def emit_overload_function_group(
    body: list[str],
    metadata: Metadata,
    quality: Quality,
    kind: str,
    name: str,
    overloads: list[dict[str, Any]],
) -> None:
    first = overloads[0]
    declarations = [clean_decl(str(fn.get("declaration") or fn.get("declarationWithNamespace") or "")) for fn in overloads]
    summary = metadata.summary_for(kind, first)
    emit_topic(body, "Function", name, summary)
    emit_comment(body)
    emit_comment(body, "Overloads:")
    for declaration in declarations:
        record_bad_decl(quality, kind, first, declaration)
        if valid_decl(declaration):
            emit_comment(body, f"  {declaration}")
    for declaration in declarations:
        if valid_decl(declaration):
            body.append(f"{declaration};")
    body.append("")


def emit_functions(lines: list[str], items: list[dict[str, Any]], metadata: Metadata, quality: Quality) -> None:
    by_namespace: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in items:
        by_namespace[str(item.get("namespace") or "")].append(item)
    for namespace in sorted(by_namespace):
        body: list[str] = []
        for (_, name), overloads in sorted(group_overloads(by_namespace[namespace]).items()):
            emit_overload_function_group(body, metadata, quality, "function", name, overloads)
        with_namespace(lines, namespace, body)


def emit_properties(lines: list[str], items: list[dict[str, Any]], metadata: Metadata, quality: Quality) -> None:
    by_namespace: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in sorted(items, key=sort_key):
        by_namespace[str(item.get("namespace") or "")].append(item)
    for namespace in sorted(by_namespace):
        body: list[str] = []
        for prop in by_namespace[namespace]:
            declaration = clean_decl(str(prop.get("declaration") or f"{prop.get('type') or 'auto'} {prop.get('name') or ''}"))
            record_bad_decl(quality, "global-property", prop, declaration)
            emit_topic(body, "Variable", str(prop.get("name") or ""), metadata.summary_for("global-property", prop), declaration)
            if valid_decl(declaration) and IDENT_RE.match(str(prop.get("name") or "")):
                body.append(f"{declaration};")
            body.append("")
        with_namespace(lines, namespace, body)


def emit_class(lines: list[str], obj: dict[str, Any], metadata: Metadata, quality: Quality) -> None:
    display_name = item_name(obj)
    class_decl = clean_decl(str(obj.get("declaration") or f"class {display_name}"))
    emit_topic(lines, "Class", display_name, metadata.summary_for("type", obj), class_decl)
    class_name = safe_identifier(display_name)
    lines.append(f"class {class_name} {{")

    factories = [f for f in obj.get("factories", []) if not metadata.hidden("factory", f)]
    properties = [p for p in obj.get("properties", []) if not metadata.hidden("property", p)]
    methods = [m for m in obj.get("methods", []) if not metadata.hidden("method", m)]
    behaviours = []
    for behaviour in obj.get("behaviours", []):
        function = dict(behaviour.get("function") or {})
        function["name"] = str(behaviour.get("behaviour") or function.get("name") or "")
        function["memberRole"] = str(behaviour.get("memberRole") or function.get("memberRole") or "")
        if metadata.hidden("behaviour", function):
            quality.hidden_internals.append({
                "kind": "behaviour",
                "name": f"{display_name}.{function['name']}",
                "docId": doc_id("behaviour", function),
            })
            continue
        behaviours.append(function)

    for collection, label, kind in (
        (factories, "Factory", "factory"),
        (properties, "Variable", "property"),
        (methods, "Function", "method"),
        (behaviours, "Function", "behaviour"),
    ):
        grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
        for member in sorted(collection, key=member_sort_key):
            grouped[str(member.get("name") or label)].append(member)
        for name, overloads in sorted(grouped.items()):
            lines.append("")
            declarations = [clean_decl(str(metadata.declaration_for(kind, member) or member.get("declaration") or "")) for member in overloads]
            emit_topic(lines, label, name, metadata.summary_for(kind, overloads[0]), indent="    ")
            if len(declarations) > 1:
                emit_comment(lines, "", "    ")
                emit_comment(lines, "Overloads:", "    ")
            for declaration in declarations:
                record_bad_decl(quality, kind, overloads[0], declaration)
                if len(declarations) > 1 and valid_decl(declaration):
                    emit_comment(lines, f"  {declaration}", "    ")
            for declaration in declarations:
                if valid_decl(declaration):
                    lines.append(f"    {declaration};")

    lines.append("}")
    lines.append("")


def emit_types(lines: list[str], items: list[dict[str, Any]], metadata: Metadata, quality: Quality) -> None:
    for obj in sorted(items, key=sort_key):
        emit_class(lines, obj, metadata, quality)


def emit_enums(lines: list[str], items: list[dict[str, Any]], metadata: Metadata, quality: Quality) -> None:
    by_namespace: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for enum in sorted(items, key=sort_key):
        by_namespace[str(enum.get("namespace") or "")].append(enum)
    for namespace in sorted(by_namespace):
        body: list[str] = []
        for enum in by_namespace[namespace]:
            name = item_name(enum)
            emit_topic(body, "Enum", name, metadata.summary_for("enum", enum))
            if IDENT_RE.match(name):
                body.append(f"enum {name} {{")
                values = sorted(enum.get("values", []), key=lambda value: str(value.get("name") or ""))
                for index, value in enumerate(values):
                    suffix = "," if index + 1 < len(values) else ""
                    body.append(f"    {safe_identifier(str(value.get('name') or 'Value'))} = {int(value.get('value') or 0)}{suffix}")
                body.append("}")
            body.append("")
        with_namespace(lines, namespace, body)


def emit_typedefs(lines: list[str], items: list[dict[str, Any]], metadata: Metadata, quality: Quality) -> None:
    by_namespace: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for typedef in sorted(items, key=sort_key):
        by_namespace[str(typedef.get("namespace") or "")].append(typedef)
    for namespace in sorted(by_namespace):
        body: list[str] = []
        for typedef in by_namespace[namespace]:
            name = item_name(typedef)
            declaration = clean_decl(str(metadata.declaration_for("typedef", typedef) or f"typedef {typedef.get('aliasedType') or ''} {name}"))
            record_bad_decl(quality, "typedef", typedef, declaration)
            emit_topic(body, "Typedef", name, metadata.summary_for("typedef", typedef), declaration)
            if valid_decl(declaration) and IDENT_RE.match(str(typedef.get("name") or "")):
                body.append(f"{declaration};")
            body.append("")
        with_namespace(lines, namespace, body)


def emit_funcdefs(lines: list[str], items: list[dict[str, Any]], metadata: Metadata, quality: Quality) -> None:
    by_namespace: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for funcdef in sorted(items, key=sort_key):
        by_namespace[str(funcdef.get("namespace") or "")].append(funcdef)
    for namespace in sorted(by_namespace):
        body: list[str] = []
        for funcdef in by_namespace[namespace]:
            name = item_name(funcdef)
            declaration = clean_decl(metadata.declaration_for("funcdef", funcdef))
            if declaration == name:
                quality.uncovered.append({
                    "kind": "funcdef",
                    "name": name,
                    "docId": doc_id("funcdef", funcdef),
                    "reason": "runtime reflection does not expose a safe full funcdef signature",
                })
                declaration = ""
            if declaration and not declaration.startswith("funcdef "):
                declaration = f"funcdef {declaration}"
            record_bad_decl(quality, "funcdef", funcdef, declaration)
            emit_topic(body, "Function Type", name, metadata.summary_for("funcdef", funcdef), declaration)
            if valid_decl(declaration):
                body.append(f"{declaration};")
            body.append("")
        with_namespace(lines, namespace, body)


def section_title(section: str) -> str:
    return {
        "types": "Types",
        "functions": "Functions",
        "properties": "Constants and Variables",
        "enums": "Enums",
        "typedefs": "Typedefs",
        "funcdefs": "Function Types",
    }[section]


def emit_section(group: Group, family: Family, section: str, items: list[dict[str, Any]], metadata: Metadata, quality: Quality) -> str:
    lines = [
        f"// Title: {group.title} - {family.title} - {section_title(section)}",
        "",
        f"// Topic: {family.title}",
    ]
    if family.overview:
        emit_comment(lines)
        emit_comment(lines, family.overview)
    if group.overview:
        emit_comment(lines)
        emit_comment(lines, group.overview)
    lines.append("")
    if section == "types":
        emit_types(lines, items, metadata, quality)
    elif section == "functions":
        emit_functions(lines, items, metadata, quality)
    elif section == "properties":
        emit_properties(lines, items, metadata, quality)
    elif section == "enums":
        emit_enums(lines, items, metadata, quality)
    elif section == "typedefs":
        emit_typedefs(lines, items, metadata, quality)
    elif section == "funcdefs":
        emit_funcdefs(lines, items, metadata, quality)
    return "\n".join(lines).rstrip() + "\n"


def emit_overview(api: dict[str, Any], metadata: Metadata, grouped: dict[str, dict[str, dict[str, list[dict[str, Any]]]]]) -> str:
    total = sum(len(api.get(collection, [])) for collection in COLLECTIONS)
    lines = [
        f"// Title: {metadata.title}",
        "",
        "// Topic: Generated Reference",
        "//",
        f"// {metadata.subtitle}",
        "//",
        f"// AngelScript version: {api.get('angelScriptVersion') or 'unknown'}",
        f"// Exported symbols: {total}",
        "//",
        "// Groups:",
    ]
    for group in sorted(metadata.groups, key=lambda value: value.order):
        count = sum(len(items) for family in grouped.get(group.id, {}).values() for items in family.values())
        if count:
            lines.append(f"//   {group.title}: {count}")
            for family in sorted(metadata.families_by_group.get(group.id, []), key=lambda value: value.order):
                family_count = sum(len(items) for items in grouped.get(group.id, {}).get(family.id, {}).values())
                if family_count:
                    lines.append(f"//     {family.title}: {family_count}")
    if metadata.links:
        lines.append("//")
        lines.append("// Related guides:")
        for label, path in sorted(metadata.links.items()):
            lines.append(f"//   {label}: {path}")
    lines.append("")
    return "\n".join(lines)


def collect_items(api: dict[str, Any], metadata: Metadata, quality: Quality) -> tuple[dict[str, dict[str, dict[str, list[dict[str, Any]]]]], set[str]]:
    grouped: dict[str, dict[str, dict[str, list[dict[str, Any]]]]] = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
    known_doc_ids: set[str] = set()
    mapping = {
        "globalFunctions": ("functions", "function"),
        "globalProperties": ("properties", "global-property"),
        "objectTypes": ("types", "type"),
        "enums": ("enums", "enum"),
        "typedefs": ("typedefs", "typedef"),
        "funcdefs": ("funcdefs", "funcdef"),
    }
    for collection, (section, kind) in mapping.items():
        for item in api.get(collection, []):
            known_doc_ids.add(doc_id(kind, item))
            if metadata.hidden(kind, item):
                quality.hidden_internals.append({"kind": kind, "name": item_name(item), "docId": doc_id(kind, item)})
                continue
            group_id = metadata.group_for(kind, item)
            if group_id not in metadata.group_by_id:
                quality.uncovered.append({"kind": kind, "name": item_name(item), "docId": doc_id(kind, item), "reason": f"unknown group {group_id}"})
                group_id = metadata.fallback_group
            family_id = metadata.family_for(kind, item, group_id)
            if family_id not in metadata.family_by_id or metadata.family_by_id[family_id].group != group_id:
                quality.uncovered.append({"kind": kind, "name": item_name(item), "docId": doc_id(kind, item), "reason": f"unknown family {family_id}"})
                family_id = metadata.default_family_for_group(group_id)
            quality.group_counts[group_id] += 1
            quality.family_counts[family_id] += 1
            grouped[group_id][family_id][section].append(item)
    return grouped, known_doc_ids


def check_public_files(out_dir: Path, quality: Quality) -> None:
    for path in sorted(out_dir.glob("*.as")):
        text = path.read_text(encoding="utf-8")
        if "$beh" in text:
            quality.public_files_with_internal_names.append(path.name)
        if "?" in text:
            quality.public_files_with_unknown_types.append(path.name)


def generate(api_json: Path, metadata_path: Path, out_dir: Path, quality_json: Path | None = None) -> Quality:
    api = json.loads(api_json.read_text(encoding="utf-8"))
    validate_schema(api)
    metadata = Metadata(load_structured(metadata_path))
    quality = Quality()

    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    grouped, known_doc_ids = collect_items(api, metadata, quality)
    quality.metadata_orphans = metadata.orphan_symbol_keys(known_doc_ids)

    files: dict[str, str] = {"overview.as": emit_overview(api, metadata, grouped)}
    section_order = ("types", "functions", "properties", "enums", "typedefs", "funcdefs")
    for group in sorted(metadata.groups, key=lambda value: value.order):
        for family in sorted(metadata.families_by_group.get(group.id, []), key=lambda value: value.order):
            sections = grouped.get(group.id, {}).get(family.id, {})
            for section_index, section in enumerate(section_order, start=1):
                items = sections.get(section, [])
                if not items:
                    continue
                file_name = f"{safe_name(group.id)}_{safe_name(family.id)}_{safe_name(section)}.as"
                files[file_name] = emit_section(group, family, section, items, metadata, quality)

    for name in sorted(files):
        (out_dir / name).write_text(files[name], encoding="utf-8", newline="\n")

    required_values = metadata.quality.get("requiredPrimaryTypes", metadata.quality.get("requiredHighLevelTypes", []))
    required = set(str(item) for item in required_values)
    if required:
        required_group = str(metadata.quality.get("requiredPrimaryGroup") or metadata.quality.get("requiredHighLevelGroup") or "script-api")
        primary_types = [
            item
            for family in grouped.get(required_group, {}).values()
            for item in family.get("types", [])
        ]
        present = {item_name(item) for item in primary_types} | {str(item.get("name") or "") for item in primary_types}
        missing = set()
        for required_name in required:
            base_name = required_name.split("<", 1)[0]
            if required_name not in present and base_name not in present:
                missing.add(required_name)
        quality.missing_required_primary_types = sorted(missing)

    check_public_files(out_dir, quality)
    if quality_json:
        quality_json.parent.mkdir(parents=True, exist_ok=True)
        quality_json.write_text(json.dumps(quality.to_dict(), indent=2, sort_keys=True) + "\n", encoding="utf-8")
    quality.assert_pass()
    return quality


def self_test() -> None:
    sample = {
        "schemaVersion": 1,
        "angelScriptVersion": "2.38.0",
        "engineProperties": {},
        "globalFunctions": [
            {"docId": "function::scene::findobject", "namespace": "Scene", "name": "FindObject", "declaration": "ObjectRef@ FindObject(const string &in name)"},
            {"namespace": "", "name": "DynLoad", "declaration": "NativePointer DynLoad(const string &in path)"},
            {"namespace": "Scene", "name": "FindObject", "declaration": "ObjectRef@ FindObject(uint id)"},
        ],
        "globalProperties": [{"namespace": "Runtime", "name": "FrameTime", "type": "float", "declaration": "float FrameTime"}],
        "objectTypes": [
            {
                "name": "AsyncTask",
                "displayName": "AsyncTask<T>",
                "declaration": "class AsyncTask<T>",
                "methods": [
                    {"name": "IsDone", "declaration": "bool IsDone() const", "memberRole": "method"},
                    {"name": "$beh5", "declaration": "void $beh5()", "memberRole": "internal"},
                ],
                "properties": [{"name": "State", "declaration": "int State"}],
                "factories": [],
                "behaviours": [{"memberRole": "internal", "behaviour": "addref", "function": {"name": "$beh5", "declaration": "void $beh5()", "memberRole": "internal"}}],
            }
        ],
        "enums": [{"namespace": "BB", "name": "PinDirection", "values": [{"name": "In", "value": 0}, {"name": "Out", "value": 1}]}],
        "typedefs": [{"namespace": "", "name": "size_t", "aliasedType": "uint"}],
        "funcdefs": [{"namespace": "", "name": "AsyncFloatFunc", "declaration": "AsyncFloatFunc"}],
    }
    metadata = {
        "schemaVersion": 1,
        "title": "Test",
        "fallbackGroup": "misc",
        "groups": [
            {"id": "script-api", "title": "Script Workflow API", "overview": "Script author API.", "order": 1},
            {"id": "native-interop", "title": "Native Interop", "order": 2},
            {"id": "misc", "title": "Miscellaneous Bindings", "order": 3},
        ],
        "families": [
            {"id": "scene", "group": "script-api", "title": "Scene", "order": 1},
            {"id": "async", "group": "script-api", "title": "Async", "order": 2},
            {"id": "native-calls", "group": "native-interop", "title": "Native Calls", "order": 1},
            {"id": "misc-general", "group": "misc", "title": "General", "order": 1},
        ],
        "namespaceGroups": {"Scene": "script-api", "Runtime": "script-api", "BB": "script-api"},
        "namespaceFamilies": {"Scene": "scene"},
        "typeGroups": {"AsyncTask<T>": {"group": "script-api", "family": "async", "summary": "Frame task handle."}},
        "symbolPatterns": [{"kind": "*", "pattern": "Dyn*", "group": "native-interop", "family": "native-calls"}],
        "typePatterns": [{"pattern": "size_t", "group": "misc", "family": "misc-general"}],
        "funcdefs": {"AsyncFloatFunc": {"declaration": "float AsyncFloatFunc()"}},
        "quality": {"requiredPrimaryGroup": "script-api", "requiredPrimaryTypes": ["AsyncTask<T>"]},
    }

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        api_json = root / "script-api.json"
        metadata_json = root / "api-reference.yml"
        out_dir = root / "out"
        quality_json = root / "quality.json"
        api_json.write_text(json.dumps(sample, sort_keys=True), encoding="utf-8")
        metadata_json.write_text(json.dumps(metadata, sort_keys=True), encoding="utf-8")
        first_quality = generate(api_json, metadata_json, out_dir, quality_json)
        first = {path.name: path.read_text(encoding="utf-8") for path in sorted(out_dir.glob("*.as"))}
        second_quality = generate(api_json, metadata_json, out_dir, quality_json)
        second = {path.name: path.read_text(encoding="utf-8") for path in sorted(out_dir.glob("*.as"))}
        assert first == second
        assert first_quality.to_dict() == second_quality.to_dict()
        combined = "\n".join(first.values())
        assert "// Function: FindObject" in combined
        assert "Overloads:" in combined
        assert "// Class: AsyncTask<T>" in combined
        assert "void $beh5" not in combined
        assert "funcdef float AsyncFloatFunc();" in combined
        assert any("script_api_scene" in name for name in first)
        assert any("native_interop_native_calls" in name for name in first)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-json", type=Path, help="Path to script-api.json exported from CKAS_EXPORT_SCRIPT_API.")
    parser.add_argument("--metadata", type=Path, default=Path("docs/api-reference.yml"), help="Path to API reference metadata.")
    parser.add_argument("--out-dir", type=Path, help="Output directory for Natural Docs .as sources.")
    parser.add_argument("--quality-json", type=Path, help="Optional output path for API reference quality report.")
    parser.add_argument("--self-test", action="store_true", help="Run generator self-tests.")
    args = parser.parse_args()

    if args.self_test:
        self_test()
        return

    if not args.api_json or not args.out_dir:
        parser.error("--api-json and --out-dir are required unless --self-test is used")
    generate(args.api_json, args.metadata, args.out_dir, args.quality_json)


if __name__ == "__main__":
    main()
