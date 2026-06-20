#include "ScriptInfo.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>

#ifndef CKAS_ENABLE_API_EXPORT
#define CKAS_ENABLE_API_EXPORT 0
#endif

namespace {

struct JsonField {
    std::string Name;
    std::string Value;
    bool Raw = false;
};

std::string SafeString(const char *value) {
    return value ? value : "";
}

std::string JsonEscape(const std::string &value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const unsigned char ch : value) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) {
                    char buffer[7] = {};
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x", ch);
                    out += buffer;
                } else {
                    out.push_back(static_cast<char>(ch));
                }
                break;
        }
    }
    return out;
}

std::string JsonString(const std::string &value) {
    return "\"" + JsonEscape(value) + "\"";
}

std::string JsonBool(bool value) {
    return value ? "true" : "false";
}

std::string JsonInt(long long value) {
    return std::to_string(value);
}

std::string JsonUInt(unsigned long long value) {
    return std::to_string(value);
}

std::string TypeDeclaration(asIScriptEngine *engine, int typeId) {
    if (!engine) {
        return "";
    }
    return SafeString(engine->GetTypeDeclaration(typeId, true));
}

std::string NormalizeDocIdPart(const std::string &value) {
    std::string normalized;
    normalized.reserve(value.size());
    bool lastWasSpace = false;
    for (const unsigned char ch : value) {
        if (std::isspace(ch)) {
            if (!lastWasSpace && !normalized.empty()) {
                normalized.push_back(' ');
            }
            lastWasSpace = true;
            continue;
        }
        lastWasSpace = false;
        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
    if (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }
    return normalized;
}

std::string JoinDocId(std::initializer_list<std::string> parts) {
    std::ostringstream out;
    bool first = true;
    for (const std::string &part : parts) {
        if (!first) {
            out << "::";
        }
        first = false;
        out << NormalizeDocIdPart(part);
    }
    return out.str();
}

std::string FunctionDeclaration(asIScriptFunction *func, bool withNamespace) {
    if (!func) {
        return "";
    }
    return SafeString(func->GetDeclaration(false, withNamespace, true));
}

std::string FunctionDocId(asIScriptFunction *func) {
    if (!func) {
        return "";
    }
    const std::string kind = SafeString(func->GetObjectName()).empty() ? "function" : "method";
    return JoinDocId({
        kind,
        SafeString(func->GetNamespace()),
        SafeString(func->GetObjectName()),
        FunctionDeclaration(func, true)
    });
}

std::string TypeDisplayName(asIScriptEngine *engine, asITypeInfo *type) {
    if (!type) {
        return "";
    }
    std::string declaration = TypeDeclaration(engine, type->GetTypeId());
    if (!declaration.empty()) {
        return declaration;
    }
    std::string name = SafeString(type->GetName());
    if ((type->GetFlags() & asOBJ_TEMPLATE) != 0 && name.find('<') == std::string::npos) {
        name += "<T>";
    }
    return name;
}

std::string TypeDocId(asIScriptEngine *engine, asITypeInfo *type) {
    if (!type) {
        return "";
    }
    return JoinDocId({"type", SafeString(type->GetNamespace()), TypeDisplayName(engine, type)});
}

std::string FunctionTypeName(asEFuncType type) {
    switch (type) {
        case asFUNC_DUMMY: return "dummy";
        case asFUNC_SYSTEM: return "system";
        case asFUNC_SCRIPT: return "script";
        case asFUNC_INTERFACE: return "interface";
        case asFUNC_VIRTUAL: return "virtual";
        case asFUNC_FUNCDEF: return "funcdef";
        case asFUNC_IMPORTED: return "imported";
        case asFUNC_DELEGATE: return "delegate";
        default: return std::to_string(static_cast<int>(type));
    }
}

std::string BehaviourName(asEBehaviours behaviour) {
    switch (behaviour) {
        case asBEHAVE_CONSTRUCT: return "construct";
        case asBEHAVE_LIST_CONSTRUCT: return "list_construct";
        case asBEHAVE_DESTRUCT: return "destruct";
        case asBEHAVE_FACTORY: return "factory";
        case asBEHAVE_LIST_FACTORY: return "list_factory";
        case asBEHAVE_ADDREF: return "addref";
        case asBEHAVE_RELEASE: return "release";
        case asBEHAVE_GET_WEAKREF_FLAG: return "get_weakref_flag";
        case asBEHAVE_TEMPLATE_CALLBACK: return "template_callback";
        case asBEHAVE_GETREFCOUNT: return "get_ref_count";
        case asBEHAVE_SETGCFLAG: return "set_gc_flag";
        case asBEHAVE_GETGCFLAG: return "get_gc_flag";
        case asBEHAVE_ENUMREFS: return "enum_refs";
        case asBEHAVE_RELEASEREFS: return "release_refs";
        default: return std::to_string(static_cast<int>(behaviour));
    }
}

std::string Indent(int count) {
    return std::string(static_cast<size_t>(count), ' ');
}

std::string WriteArray(const std::vector<std::string> &items, int indent) {
    if (items.empty()) {
        return "[]";
    }
    std::ostringstream out;
    out << "[\n";
    for (size_t i = 0; i < items.size(); ++i) {
        out << Indent(indent + 2) << items[i];
        if (i + 1 < items.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << Indent(indent) << "]";
    return out.str();
}

std::string WriteStringArray(const std::vector<std::string> &items, int indent) {
    std::vector<std::string> escaped;
    escaped.reserve(items.size());
    for (const std::string &item : items) {
        escaped.push_back(JsonString(item));
    }
    return WriteArray(escaped, indent);
}

std::string WriteObject(const std::vector<JsonField> &fields, int indent) {
    std::ostringstream out;
    out << "{\n";
    for (size_t i = 0; i < fields.size(); ++i) {
        out << Indent(indent + 2) << JsonString(fields[i].Name) << ": ";
        out << (fields[i].Raw ? fields[i].Value : JsonString(fields[i].Value));
        if (i + 1 < fields.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << Indent(indent) << "}";
    return out.str();
}

struct ApiCategory {
    std::string Key;
    std::string Name;
    std::string Audience;
    std::string SourceArea;
};

std::string NamespaceKey(const std::string &nameSpace) {
    if (nameSpace.empty()) {
        return "global";
    }
    std::string key;
    key.reserve(nameSpace.size());
    for (char ch : nameSpace) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        } else {
            key.push_back('_');
        }
    }
    return key;
}

ApiCategory ClassifyApi(const std::string &nameSpace) {
    if (nameSpace.empty()) {
        return {"global", "Global namespace", "global", ""};
    }
    return {NamespaceKey(nameSpace), "Namespace " + nameSpace, "namespace", ""};
}

std::vector<JsonField> ApiCategoryFields(const ApiCategory &category) {
    return {
        {"apiCategory", category.Key},
        {"apiCategoryName", category.Name},
        {"apiAudience", category.Audience},
        {"apiSourceArea", category.SourceArea}
    };
}

std::vector<JsonField> WithApiCategory(std::vector<JsonField> fields, const ApiCategory &category) {
    std::vector<JsonField> categoryFields = ApiCategoryFields(category);
    fields.insert(fields.end(), categoryFields.begin(), categoryFields.end());
    return fields;
}

std::vector<std::string> ParamFlagNames(asDWORD flags) {
    std::vector<std::string> names;
    switch (flags & asTM_INOUTREF) {
        case asTM_INREF: names.push_back("inref"); break;
        case asTM_OUTREF: names.push_back("outref"); break;
        case asTM_INOUTREF: names.push_back("inoutref"); break;
        default: break;
    }
    if ((flags & asTM_CONST) != 0) {
        names.push_back("const");
    }
    return names;
}

std::vector<std::string> TypeFlagNames(asQWORD flags) {
    std::vector<std::string> names;
    const std::pair<asQWORD, const char *> mappings[] = {
        {asOBJ_REF, "ref"},
        {asOBJ_VALUE, "value"},
        {asOBJ_GC, "gc"},
        {asOBJ_POD, "pod"},
        {asOBJ_TEMPLATE, "template"},
        {asOBJ_NOCOUNT, "nocount"},
        {asOBJ_TEMPLATE_SUBTYPE, "template_subtype"},
        {asOBJ_APP_CLASS, "app_class"},
        {asOBJ_APP_PRIMITIVE, "app_primitive"},
        {asOBJ_APP_FLOAT, "app_float"},
        {asOBJ_APP_ARRAY, "app_array"}
    };
    for (const auto &mapping : mappings) {
        if ((flags & mapping.first) != 0) {
            names.push_back(mapping.second);
        }
    }
    return names;
}

std::string TypeKind(asQWORD flags) {
    if ((flags & asOBJ_TEMPLATE) != 0) {
        return "template";
    }
    if ((flags & asOBJ_REF) != 0) {
        return ((flags & asOBJ_NOCOUNT) != 0) ? "borrowed_reference" : "reference";
    }
    if ((flags & asOBJ_VALUE) != 0) {
        return "value";
    }
    return "unknown";
}

std::string ExportFunction(asIScriptEngine *engine, asIScriptFunction *func, const std::string &memberRole = "function") {
    if (!func) {
        return "{}";
    }

    std::vector<std::string> params;
    for (asUINT i = 0; i < func->GetParamCount(); ++i) {
        int typeId = 0;
        asDWORD flags = 0;
        const char *name = nullptr;
        const char *defaultArg = nullptr;
        func->GetParam(i, &typeId, &flags, &name, &defaultArg);
        params.push_back(WriteObject({
            {"index", JsonUInt(i), true},
            {"name", SafeString(name)},
            {"typeId", JsonInt(typeId), true},
            {"type", TypeDeclaration(engine, typeId)},
            {"flags", JsonUInt(flags), true},
            {"flagNames", WriteStringArray(ParamFlagNames(flags), 10), true},
            {"defaultArg", SafeString(defaultArg)}
        }, 8));
    }

    return WriteObject(WithApiCategory({
        {"docId", FunctionDocId(func)},
        {"id", JsonInt(func->GetId()), true},
        {"name", SafeString(func->GetName())},
        {"namespace", SafeString(func->GetNamespace())},
        {"declaration", FunctionDeclaration(func, false)},
        {"declarationWithNamespace", FunctionDeclaration(func, true)},
        {"memberRole", memberRole},
        {"objectName", SafeString(func->GetObjectName())},
        {"functionType", FunctionTypeName(func->GetFuncType())},
        {"moduleName", SafeString(func->GetModuleName())},
        {"configGroup", SafeString(func->GetConfigGroup())},
        {"accessMask", JsonUInt(func->GetAccessMask()), true},
        {"returnTypeId", JsonInt(func->GetReturnTypeId()), true},
        {"returnType", TypeDeclaration(engine, func->GetReturnTypeId())},
        {"typeId", JsonInt(func->GetTypeId()), true},
        {"isReadOnly", JsonBool(func->IsReadOnly()), true},
        {"isPrivate", JsonBool(func->IsPrivate()), true},
        {"isProtected", JsonBool(func->IsProtected()), true},
        {"isFinal", JsonBool(func->IsFinal()), true},
        {"isOverride", JsonBool(func->IsOverride()), true},
        {"isShared", JsonBool(func->IsShared()), true},
        {"isExplicit", JsonBool(func->IsExplicit()), true},
        {"isProperty", JsonBool(func->IsProperty()), true},
        {"isVariadic", JsonBool(func->IsVariadic()), true},
        {"params", WriteArray(params, 6), true}
    }, ClassifyApi(SafeString(func->GetNamespace()))), 6);
}

bool IsInternalBehaviour(asEBehaviours behaviour, asIScriptFunction *func) {
    const std::string declaration = FunctionDeclaration(func, false);
    if (declaration.find("$beh") != std::string::npos) {
        return true;
    }
    switch (behaviour) {
        case asBEHAVE_ADDREF:
        case asBEHAVE_RELEASE:
        case asBEHAVE_GET_WEAKREF_FLAG:
        case asBEHAVE_TEMPLATE_CALLBACK:
        case asBEHAVE_GETREFCOUNT:
        case asBEHAVE_SETGCFLAG:
        case asBEHAVE_GETGCFLAG:
        case asBEHAVE_ENUMREFS:
        case asBEHAVE_RELEASEREFS:
            return true;
        default:
            return false;
    }
}

std::string FunctionSortKey(asIScriptFunction *func) {
    if (!func) {
        return "";
    }
    return SafeString(func->GetNamespace()) + "\n" +
           SafeString(func->GetObjectName()) + "\n" +
           SafeString(func->GetName()) + "\n" +
           FunctionDeclaration(func, true) + "\n" +
           std::to_string(func->GetId());
}

std::string ExportObjectType(asIScriptEngine *engine, asITypeInfo *type) {
    if (!type) {
        return "{}";
    }

    std::vector<asIScriptFunction *> factories;
    for (asUINT i = 0; i < type->GetFactoryCount(); ++i) {
        factories.push_back(type->GetFactoryByIndex(i));
    }
    std::sort(factories.begin(), factories.end(), [](asIScriptFunction *a, asIScriptFunction *b) {
        return FunctionSortKey(a) < FunctionSortKey(b);
    });

    std::vector<asIScriptFunction *> methods;
    for (asUINT i = 0; i < type->GetMethodCount(); ++i) {
        methods.push_back(type->GetMethodByIndex(i));
    }
    std::sort(methods.begin(), methods.end(), [](asIScriptFunction *a, asIScriptFunction *b) {
        return FunctionSortKey(a) < FunctionSortKey(b);
    });

    std::vector<std::string> factoryItems;
    for (asIScriptFunction *func : factories) {
        factoryItems.push_back(ExportFunction(engine, func, "factory"));
    }

    std::vector<std::string> methodItems;
    for (asIScriptFunction *func : methods) {
        methodItems.push_back(ExportFunction(engine, func, "method"));
    }

    std::vector<std::string> propertyItems;
    for (asUINT i = 0; i < type->GetPropertyCount(); ++i) {
        const char *name = nullptr;
        int typeId = 0;
        bool isPrivate = false;
        bool isProtected = false;
        int offset = 0;
        bool isReference = false;
        asDWORD accessMask = 0;
        int compositeOffset = 0;
        bool isCompositeIndirect = false;
        bool isConst = false;
        type->GetProperty(i, &name, &typeId, &isPrivate, &isProtected, &offset, &isReference, &accessMask,
                          &compositeOffset, &isCompositeIndirect, &isConst);
        const std::string declaration = SafeString(type->GetPropertyDeclaration(i, true));
        propertyItems.push_back(WriteObject({
            {"docId", JoinDocId({"property", SafeString(type->GetNamespace()), TypeDisplayName(engine, type), declaration})},
            {"name", SafeString(name)},
            {"declaration", declaration},
            {"memberRole", "property"},
            {"typeId", JsonInt(typeId), true},
            {"type", TypeDeclaration(engine, typeId)},
            {"isPrivate", JsonBool(isPrivate), true},
            {"isProtected", JsonBool(isProtected), true},
            {"isReference", JsonBool(isReference), true},
            {"isConst", JsonBool(isConst), true},
            {"accessMask", JsonUInt(accessMask), true},
            {"offset", JsonInt(offset), true},
            {"compositeOffset", JsonInt(compositeOffset), true},
            {"isCompositeIndirect", JsonBool(isCompositeIndirect), true}
        }, 8));
    }
    std::sort(propertyItems.begin(), propertyItems.end());

    std::vector<std::string> behaviourItems;
    for (asUINT i = 0; i < type->GetBehaviourCount(); ++i) {
        asEBehaviours behaviour = asBEHAVE_MAX;
        asIScriptFunction *func = type->GetBehaviourByIndex(i, &behaviour);
        const bool internal = IsInternalBehaviour(behaviour, func);
        behaviourItems.push_back(WriteObject({
            {"behaviour", BehaviourName(behaviour)},
            {"behaviourId", JsonInt(static_cast<int>(behaviour)), true},
            {"memberRole", internal ? "internal" : "behaviour"},
            {"function", ExportFunction(engine, func, internal ? "internal" : "behaviour"), true}
        }, 8));
    }
    std::sort(behaviourItems.begin(), behaviourItems.end());

    std::vector<std::string> interfaceItems;
    for (asUINT i = 0; i < type->GetInterfaceCount(); ++i) {
        asITypeInfo *iface = type->GetInterface(i);
        interfaceItems.push_back(WriteObject({
            {"name", iface ? SafeString(iface->GetName()) : ""},
            {"namespace", iface ? SafeString(iface->GetNamespace()) : ""},
            {"typeId", JsonInt(iface ? iface->GetTypeId() : 0), true}
        }, 8));
    }
    std::sort(interfaceItems.begin(), interfaceItems.end());

    std::vector<std::string> subTypeItems;
    for (asUINT i = 0; i < type->GetSubTypeCount(); ++i) {
        const int subTypeId = type->GetSubTypeId(i);
        subTypeItems.push_back(WriteObject({
            {"typeId", JsonInt(subTypeId), true},
            {"type", TypeDeclaration(engine, subTypeId)}
        }, 8));
    }

    asITypeInfo *baseType = type->GetBaseType();
    const asQWORD flags = type->GetFlags();
    const std::string displayName = TypeDisplayName(engine, type);
    return WriteObject(WithApiCategory({
        {"docId", TypeDocId(engine, type)},
        {"name", SafeString(type->GetName())},
        {"displayName", displayName},
        {"declaration", "class " + displayName},
        {"namespace", SafeString(type->GetNamespace())},
        {"typeId", JsonInt(type->GetTypeId()), true},
        {"flags", JsonUInt(flags), true},
        {"flagNames", WriteStringArray(TypeFlagNames(flags), 8), true},
        {"kind", TypeKind(flags)},
        {"size", JsonUInt(type->GetSize()), true},
        {"configGroup", SafeString(type->GetConfigGroup())},
        {"accessMask", JsonUInt(type->GetAccessMask()), true},
        {"baseType", baseType ? SafeString(baseType->GetName()) : ""},
        {"baseTypeId", JsonInt(baseType ? baseType->GetTypeId() : 0), true},
        {"subTypes", WriteArray(subTypeItems, 6), true},
        {"interfaces", WriteArray(interfaceItems, 6), true},
        {"factories", WriteArray(factoryItems, 6), true},
        {"methods", WriteArray(methodItems, 6), true},
        {"properties", WriteArray(propertyItems, 6), true},
        {"behaviours", WriteArray(behaviourItems, 6), true}
    }, ClassifyApi(SafeString(type->GetNamespace()))), 6);
}

std::string ExportEnum(asITypeInfo *type) {
    if (!type) {
        return "{}";
    }
    std::vector<std::string> values;
    for (asUINT i = 0; i < type->GetEnumValueCount(); ++i) {
        int value = 0;
        const char *name = type->GetEnumValueByIndex(i, &value);
        values.push_back(WriteObject({
            {"name", SafeString(name)},
            {"value", JsonInt(value), true}
        }, 8));
    }
    std::sort(values.begin(), values.end());

    return WriteObject(WithApiCategory({
        {"docId", JoinDocId({"enum", SafeString(type->GetNamespace()), SafeString(type->GetName())})},
        {"name", SafeString(type->GetName())},
        {"namespace", SafeString(type->GetNamespace())},
        {"typeId", JsonInt(type->GetTypeId()), true},
        {"configGroup", SafeString(type->GetConfigGroup())},
        {"accessMask", JsonUInt(type->GetAccessMask()), true},
        {"values", WriteArray(values, 6), true}
    }, ClassifyApi(SafeString(type->GetNamespace()))), 6);
}

std::string ExportTypedef(asIScriptEngine *engine, asITypeInfo *type) {
    if (!type) {
        return "{}";
    }
    const int aliasedTypeId = type->GetTypedefTypeId();
    return WriteObject(WithApiCategory({
        {"docId", JoinDocId({"typedef", SafeString(type->GetNamespace()), SafeString(type->GetName())})},
        {"name", SafeString(type->GetName())},
        {"namespace", SafeString(type->GetNamespace())},
        {"typeId", JsonInt(type->GetTypeId()), true},
        {"configGroup", SafeString(type->GetConfigGroup())},
        {"accessMask", JsonUInt(type->GetAccessMask()), true},
        {"aliasedTypeId", JsonInt(aliasedTypeId), true},
        {"aliasedType", TypeDeclaration(engine, aliasedTypeId)}
    }, ClassifyApi(SafeString(type->GetNamespace()))), 6);
}

std::string ExportFuncdef(asIScriptEngine *engine, asITypeInfo *type) {
    if (!type) {
        return "{}";
    }
    const std::string typeDeclaration = TypeDeclaration(engine, type->GetTypeId());
    return WriteObject(WithApiCategory({
        {"docId", JoinDocId({"funcdef", SafeString(type->GetNamespace()), SafeString(type->GetName())})},
        {"name", SafeString(type->GetName())},
        {"namespace", SafeString(type->GetNamespace())},
        {"typeId", JsonInt(type->GetTypeId()), true},
        {"configGroup", SafeString(type->GetConfigGroup())},
        {"accessMask", JsonUInt(type->GetAccessMask()), true},
        {"declaration", typeDeclaration},
        {"declarationWithNamespace", typeDeclaration},
        {"typeDeclaration", typeDeclaration}
    }, ClassifyApi(SafeString(type->GetNamespace()))), 6);
}

std::string ExportGlobalProperty(asIScriptEngine *engine, asUINT index) {
    const char *name = nullptr;
    const char *nameSpace = nullptr;
    int typeId = 0;
    bool isConst = false;
    const char *configGroup = nullptr;
    void *pointer = nullptr;
    asDWORD accessMask = 0;
    engine->GetGlobalPropertyByIndex(index, &name, &nameSpace, &typeId, &isConst, &configGroup, &pointer, &accessMask);
    const std::string declaration = TypeDeclaration(engine, typeId) + " " + SafeString(name);
    return WriteObject(WithApiCategory({
        {"docId", JoinDocId({"global-property", SafeString(nameSpace), declaration})},
        {"name", SafeString(name)},
        {"namespace", SafeString(nameSpace)},
        {"typeId", JsonInt(typeId), true},
        {"type", TypeDeclaration(engine, typeId)},
        {"declaration", declaration},
        {"isConst", JsonBool(isConst), true},
        {"configGroup", SafeString(configGroup)},
        {"accessMask", JsonUInt(accessMask), true},
        {"address", JsonUInt(reinterpret_cast<uintptr_t>(pointer)), true}
    }, ClassifyApi(SafeString(nameSpace))), 6);
}

template <typename T>
void SortByNameNamespace(std::vector<T *> &items) {
    std::sort(items.begin(), items.end(), [](T *a, T *b) {
        const std::string keyA = std::string(SafeString(a ? a->GetNamespace() : "")) + "\n" +
                                 SafeString(a ? a->GetName() : "") + "\n" +
                                 std::to_string(a ? a->GetTypeId() : 0);
        const std::string keyB = std::string(SafeString(b ? b->GetNamespace() : "")) + "\n" +
                                 SafeString(b ? b->GetName() : "") + "\n" +
                                 std::to_string(b ? b->GetTypeId() : 0);
        return keyA < keyB;
    });
}

void WriteExportStage(const char *pathValue, const char *stage) {
    if (!pathValue || !stage) {
        return;
    }
    try {
        std::ofstream out(std::string(pathValue) + ".stage", std::ios::binary | std::ios::trunc);
        out << stage << "\n";
    } catch (...) {
    }
}

} // namespace

bool ExportScriptApiIfRequested(asIScriptEngine *engine, std::string &error) {
    error.clear();
#if !CKAS_ENABLE_API_EXPORT
    (void)engine;
    return true;
#else
    const char *pathValue = std::getenv("CKAS_EXPORT_SCRIPT_API");
    if (!pathValue || pathValue[0] == '\0') {
        return true;
    }
    if (!engine) {
        error = "CKAS_EXPORT_SCRIPT_API is set but the AngelScript engine is null.";
        return false;
    }

    try {
        WriteExportStage(pathValue, "globalFunctions");
        std::vector<asIScriptFunction *> globalFunctions;
        for (asUINT i = 0; i < engine->GetGlobalFunctionCount(); ++i) {
            globalFunctions.push_back(engine->GetGlobalFunctionByIndex(i));
        }
        std::sort(globalFunctions.begin(), globalFunctions.end(), [](asIScriptFunction *a, asIScriptFunction *b) {
            return FunctionSortKey(a) < FunctionSortKey(b);
        });

        std::vector<std::string> globalFunctionItems;
        for (asIScriptFunction *func : globalFunctions) {
            globalFunctionItems.push_back(ExportFunction(engine, func));
        }

        WriteExportStage(pathValue, "globalProperties");
        std::vector<std::string> globalPropertyItems;
        for (asUINT i = 0; i < engine->GetGlobalPropertyCount(); ++i) {
            globalPropertyItems.push_back(ExportGlobalProperty(engine, i));
        }
        std::sort(globalPropertyItems.begin(), globalPropertyItems.end());

        WriteExportStage(pathValue, "objectTypes");
        std::vector<asITypeInfo *> objectTypes;
        for (asUINT i = 0; i < engine->GetObjectTypeCount(); ++i) {
            objectTypes.push_back(engine->GetObjectTypeByIndex(i));
        }
        SortByNameNamespace(objectTypes);

        std::vector<std::string> objectTypeItems;
        for (asITypeInfo *type : objectTypes) {
            objectTypeItems.push_back(ExportObjectType(engine, type));
        }

        WriteExportStage(pathValue, "enums");
        std::vector<asITypeInfo *> enums;
        for (asUINT i = 0; i < engine->GetEnumCount(); ++i) {
            enums.push_back(engine->GetEnumByIndex(i));
        }
        SortByNameNamespace(enums);

        std::vector<std::string> enumItems;
        for (asITypeInfo *type : enums) {
            enumItems.push_back(ExportEnum(type));
        }

        WriteExportStage(pathValue, "typedefs");
        std::vector<asITypeInfo *> typedefs;
        for (asUINT i = 0; i < engine->GetTypedefCount(); ++i) {
            typedefs.push_back(engine->GetTypedefByIndex(i));
        }
        SortByNameNamespace(typedefs);

        std::vector<std::string> typedefItems;
        for (asITypeInfo *type : typedefs) {
            typedefItems.push_back(ExportTypedef(engine, type));
        }

        WriteExportStage(pathValue, "funcdefs");
        std::vector<asITypeInfo *> funcdefs;
        const asUINT funcdefCount = engine->GetFuncdefCount();
        WriteExportStage(pathValue, fmt::format("funcdefs:count={}", funcdefCount).c_str());
        for (asUINT i = 0; i < funcdefCount; ++i) {
            const std::string stage = fmt::format("funcdefs:get:{}", i);
            WriteExportStage(pathValue, stage.c_str());
            funcdefs.push_back(engine->GetFuncdefByIndex(i));
        }
        SortByNameNamespace(funcdefs);

        std::vector<std::string> funcdefItems;
        for (size_t i = 0; i < funcdefs.size(); ++i) {
            const std::string stage = fmt::format("funcdefs:export:{}", i);
            WriteExportStage(pathValue, stage.c_str());
            asITypeInfo *type = funcdefs[i];
            funcdefItems.push_back(ExportFuncdef(engine, type));
        }

        const std::vector<JsonField> engineProperties = {
            {"useCharacterLiterals", JsonUInt(engine->GetEngineProperty(asEP_USE_CHARACTER_LITERALS)), true},
            {"allowUnsafeReferences", JsonUInt(engine->GetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES)), true},
            {"allowImplicitHandleTypes", JsonUInt(engine->GetEngineProperty(asEP_ALLOW_IMPLICIT_HANDLE_TYPES)), true},
            {"buildWithoutLineCues", JsonUInt(engine->GetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES)), true},
            {"propertyAccessorMode", JsonUInt(engine->GetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE)), true}
        };

        WriteExportStage(pathValue, "write");
        const std::string document = WriteObject({
            {"schemaVersion", "1", true},
            {"angelScriptVersion", SafeString(asGetLibraryVersion())},
            {"engineProperties", WriteObject(engineProperties, 2), true},
            {"globalFunctions", WriteArray(globalFunctionItems, 2), true},
            {"globalProperties", WriteArray(globalPropertyItems, 2), true},
            {"objectTypes", WriteArray(objectTypeItems, 2), true},
            {"enums", WriteArray(enumItems, 2), true},
            {"typedefs", WriteArray(typedefItems, 2), true},
            {"funcdefs", WriteArray(funcdefItems, 2), true}
        }, 0);

        const std::filesystem::path outputPath(pathValue);
        const std::filesystem::path parent = outputPath.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            error = "Failed to open " + outputPath.string() + " for writing.";
            return false;
        }
        out << document << "\n";
        if (!out) {
            error = "Failed to write " + outputPath.string() + ".";
            return false;
        }
        WriteExportStage(pathValue, "complete");
        std::filesystem::remove(std::string(pathValue) + ".stage");
    } catch (const std::exception &ex) {
        error = ex.what();
        return false;
    } catch (...) {
        error = "Unknown exception while exporting AngelScript API.";
        return false;
    }

    return true;
#endif
}
