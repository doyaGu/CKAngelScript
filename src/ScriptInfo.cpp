#include "ScriptInfo.h"

#include <cassert>
#include <fstream>

#include <fmt/format.h>

void PrintEnumList(asIScriptEngine *engine, std::ofstream &file) {
    for (int i = 0; i < engine->GetEnumCount(); i++) {
        const auto e = engine->GetEnumByIndex(i);
        if (!e) continue;
        const std::string_view ns = e->GetNamespace();
        if (!ns.empty()) file << fmt::format("namespace {} {{\n", ns);
        file << fmt::format("enum {} {{\n", e->GetName());
        for (int j = 0; j < e->GetEnumValueCount(); ++j) {
            file << fmt::format("\t{}", e->GetEnumValueByIndex(j, nullptr));
            if (j < e->GetEnumValueCount() - 1) file << ",";
            file << "\n";
        }
        file << "}\n";
        if (!ns.empty()) file << "}\n";
    }
}

void PrintClassTypeList(asIScriptEngine *engine, std::ofstream &file) {
    for (int i = 0; i < engine->GetObjectTypeCount(); i++) {
        const auto t = engine->GetObjectTypeByIndex(i);
        if (!t) continue;

        const std::string_view ns = t->GetNamespace();
        if (!ns.empty()) file << fmt::format("namespace {} {{\n", ns);

        file << fmt::format("class {}", t->GetName());
        if (t->GetSubTypeCount() > 0) {
            file << "<";
            for (int sub = 0; sub < t->GetSubTypeCount(); ++sub) {
                if (sub < t->GetSubTypeCount() - 1) file << ", ";
                const auto st = t->GetSubType(sub);
                file << st->GetName();
            }

            file << ">";
        }

        file << "{\n";
        for (int j = 0; j < t->GetBehaviourCount(); ++j) {
            asEBehaviours behaviours;
            const auto f = t->GetBehaviourByIndex(j, &behaviours);
            if (behaviours == asBEHAVE_CONSTRUCT || behaviours == asBEHAVE_DESTRUCT) {
                file << fmt::format("\t{};\n", f->GetDeclaration(false, true, true));
            }
        }
        for (int j = 0; j < t->GetMethodCount(); ++j) {
            const auto m = t->GetMethodByIndex(j);
            file << fmt::format("\t{};\n", m->GetDeclaration(false, true, true));
        }
        for (int j = 0; j < t->GetPropertyCount(); ++j) {
            file << fmt::format("\t{};\n", t->GetPropertyDeclaration(j, true));
        }
        for (int j = 0; j < t->GetChildFuncdefCount(); ++j) {
            file << fmt::format("\tfuncdef {};\n", t->GetChildFuncdef(j)->GetFuncdefSignature()->GetDeclaration(false));
        }
        file << "}\n";
        if (!ns.empty()) file << "}\n";
    }
}

void PrintGlobalFunctionList(asIScriptEngine *engine, std::ofstream &file) {
    for (int i = 0; i < engine->GetGlobalFunctionCount(); i++) {
        const auto f = engine->GetGlobalFunctionByIndex(i);
        if (!f) continue;
        const std::string_view ns = f->GetNamespace();
        if (!ns.empty()) file << fmt::format("namespace {} {{ ", ns);
        file << fmt::format("{};", f->GetDeclaration(false, false, true));
        if (!ns.empty()) file << " }";
        file << "\n";
    }
}

void PrintGlobalPropertyList(asIScriptEngine *engine, std::ofstream &file) {
    for (int i = 0; i < engine->GetGlobalPropertyCount(); i++) {
        const char *name;
        const char *ns0;
        int type;
        engine->GetGlobalPropertyByIndex(i, &name, &ns0, &type, nullptr, nullptr, nullptr, nullptr);

        const std::string t = engine->GetTypeDeclaration(type, true);
        if (t.empty()) continue;

        std::string_view ns = ns0;
        if (!ns.empty()) file << fmt::format("namespace {} {{ ", ns);

        file << fmt::format("{} {};", t, name);
        if (!ns.empty()) file << " }";
        file << "\n";
    }
}

void PrintGlobalTypedef(asIScriptEngine *engine, std::ofstream &file) {
    for (int i = 0; i < engine->GetTypedefCount(); ++i) {
        const auto type = engine->GetTypedefByIndex(i);
        if (!type) continue;
        const std::string_view ns = type->GetNamespace();
        if (!ns.empty()) file << fmt::format("namespace {} {{\n", ns);
        file << fmt::format(
            "typedef {} {};\n", engine->GetTypeDeclaration(type->GetTypedefTypeId()), type->GetName());
        if (!ns.empty()) file << "}\n";
    }
}

void PrintAngelScriptInfo(asIScriptEngine *engine, std::ofstream &file) {
    PrintEnumList(engine, file);
    PrintClassTypeList(engine, file);
    PrintGlobalFunctionList(engine, file);
    PrintGlobalPropertyList(engine, file);
    PrintGlobalTypedef(engine, file);
}

void DumpAngelScriptDefinitions(const std::string &filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        PrintAngelScriptInfo(asGetActiveContext()->GetEngine(), file);
    }
}

void RegisterScriptInfo(asIScriptEngine *engine) {
    int r = engine->RegisterGlobalFunction("void DumpAngelScriptDefinitions(const string &in filename)", asFUNCTION(DumpAngelScriptDefinitions), asCALL_CDECL); assert(r >= 0);
}
