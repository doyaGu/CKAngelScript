#include "ScriptFormat.h"

#include <cassert>
#include <string>
#include <sstream>
#include <unordered_map>
#include <mutex>

#include <fmt/format.h>
#include <fmt/args.h>

#include "CKContext.h"

#include "RefCount.h"

#define TOSTRINGMAP_TYPE 0x40000

class ToStringMap {
public:
    static ToStringMap *Create() {
        void *self = asAllocMem(sizeof(ToStringMap));
        return new(self) ToStringMap();
    }

    ToStringMap() = default;

    ToStringMap(const ToStringMap &rhs) = delete;
    ToStringMap(ToStringMap &&rhs) noexcept = delete;

    ~ToStringMap() = default;

    ToStringMap &operator=(const ToStringMap &rhs) = delete;
    ToStringMap &operator=(ToStringMap &&rhs) noexcept = delete;

    int AddRef() const {
        return m_RefCount.AddRef();
    }

    int Release() const {
        const int r = m_RefCount.Release();
        if (r == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
            this->~ToStringMap();
            asFreeMem(const_cast<ToStringMap *>(this));
        }
        return r;
    }

    asIScriptFunction *GetToStringFunction(int typeId) const {
        const auto it = m_ToStringMap.find(typeId);
        if (it == m_ToStringMap.end()) {
            return nullptr;
        }
        return it->second;
    }

    void SetToStringFunction(int typeId, asIScriptFunction *func) {
        std::lock_guard<std::mutex> lock{m_Mutex};
        m_ToStringMap[typeId] = func;
    }

private:
    mutable RefCount m_RefCount;
    std::mutex m_Mutex;
    std::unordered_map<int, asIScriptFunction *> m_ToStringMap;
};

static std::string g_StringResult;

asIScriptFunction *FindToString(asIScriptEngine *engine, int typeId) {
    auto *map = static_cast<ToStringMap *>(engine->GetUserData(TOSTRINGMAP_TYPE));
    if (!map) {
        engine->WriteMessage("Print", 0, 0, asMSGTYPE_ERROR, "ToStringMap has been overwritten.");
        return nullptr;
    }
    map->AddRef();

    auto *func = map->GetToStringFunction(typeId);
    if (func) {
        map->Release();
        return func;
    }

    asITypeInfo *type = engine->GetTypeInfoById(typeId);
    assert(type);

    // Special case - cast to string
    const char *name = type->GetName();
    if (strcmp(name, "string") == 0) {
        map->SetToStringFunction(typeId, nullptr);
        map->Release();
        return nullptr;
    }

    func = type->GetMethodByDecl("string toString() const");
    if (!func) {
        func = type->GetMethodByDecl("string ToString() const");
    }
    if (!func) {
        func = type->GetMethodByDecl("string toString()");
    }
    if (!func) {
        func = type->GetMethodByDecl("string ToString()");
    }
    if (!func) {
        std::string msg("Missing ToString() for object '");
        msg.append(name).append("'.");
        engine->WriteMessage("Format", 0, 0, asMSGTYPE_ERROR, msg.c_str());
        map->SetToStringFunction(typeId, nullptr);
        map->Release();
        return nullptr;
    }

    map->SetToStringFunction(typeId, func);
    map->Release();
    return func;
}

static std::string &ToString(asIScriptGeneric *gen, void *adr, int typeId) {
    asIScriptEngine *engine = gen->GetEngine();
    asIScriptFunction *func = FindToString(engine, typeId);

    if (!func) {
        // cast to string
        std::string &value = **static_cast<std::string **>(adr);
        return value;
    }

    // call function
    asIScriptObject *obj = *static_cast<asIScriptObject **>(adr);
    asIScriptContext *ctx = engine->RequestContext();
    int r = ctx->Prepare(func);
    if (r >= 0) {
        r = ctx->SetObject(obj);
        if (r >= 0)
            r = ctx->Execute();
    }
    if (r < 0) {
        asITypeInfo *type = engine->GetTypeInfoById(typeId);
        const char *name = type->GetName();

        std::string msg("Failed to call toString() on object '");
        msg.append(name).append("' (").append(std::to_string(r)).append(").");
        engine->WriteMessage("Format", 0, 0, asMSGTYPE_ERROR, msg.c_str());
    }

    void *retAdr = ctx->GetReturnAddress();
    g_StringResult = *static_cast<std::string *>(retAdr);
    engine->ReturnContext(ctx);
    return g_StringResult;
}

static void ArgToStringStream(asIScriptGeneric *gen, int index, std::stringstream &stream) {
    auto typeId = gen->GetArgTypeId(index);
    void *adr = gen->GetAddressOfArg(index);

    switch (typeId) {
        case asTYPEID_BOOL: {
            bool value = **static_cast<bool **>(adr);
            stream << (value ? "true" : "false");
        }
        break;
        case asTYPEID_INT8: {
            int8_t value = **static_cast<int8_t **>(adr);
            stream << value;
        }
        break;
        case asTYPEID_INT16: {
            int16_t value = **static_cast<int16_t **>(adr);
            stream << value;
        }
        break;
        case asTYPEID_INT32: {
            int32_t value = **static_cast<int32_t **>(adr);
            stream << value;
        }
        break;
        case asTYPEID_INT64: {
            int64_t value = **static_cast<int64_t **>(adr);
            stream << value;
        }
        break;
        case asTYPEID_UINT8: {
            uint8_t value = **static_cast<uint8_t **>(adr);
            stream << value;
        }
        break;
        case asTYPEID_UINT16: {
            uint16_t value = **static_cast<uint16_t **>(adr);
            stream << value;
        }
        break;
        case asTYPEID_UINT32: {
            uint32_t value = **static_cast<uint32_t **>(adr);
            stream << value;
        }
        break;
        case asTYPEID_UINT64: {
            uint64_t value = **static_cast<uint64_t **>(adr);
            stream << value;
        }
        break;
        case asTYPEID_FLOAT: {
            float value = **static_cast<float **>(adr);
            stream << value;
        }
        break;
        case asTYPEID_DOUBLE: {
            double value = **static_cast<double **>(adr);
            stream << value;
        }
        break;
        default:
            stream << ToString(gen, adr, typeId);
            break;
    }
}

static std::string ArgToString(asIScriptGeneric *gen, int index) {
    std::stringstream stream;
    ArgToStringStream(gen, index, stream);
    return stream.str();
}

static void ToStringGeneric(asIScriptGeneric *gen) {
    new(gen->GetAddressOfReturnLocation()) std::string(std::move(ArgToString(gen, 0)));
}

static std::string FormatString(asIScriptGeneric *gen) {
    const std::string &fmt = **static_cast<std::string **>(gen->GetAddressOfArg(0));
    fmt::dynamic_format_arg_store<fmt::format_context> store;

    const int count = gen->GetArgCount();
    for (int i = 1; i < count; ++i) {
        auto typeId = gen->GetArgTypeId(i);
        void *adr = gen->GetAddressOfArg(i);

        switch (typeId) {
            case asTYPEID_BOOL: {
                bool value = **static_cast<bool **>(adr);
                store.push_back(value ? "true" : "false");
            }
            break;
            case asTYPEID_INT8: {
                int8_t value = **static_cast<int8_t **>(adr);
                store.push_back(value);
            }
            break;
            case asTYPEID_INT16: {
                int16_t value = **static_cast<int16_t **>(adr);
                store.push_back(value);
            }
            break;
            case asTYPEID_INT32: {
                int32_t value = **static_cast<int32_t **>(adr);
                store.push_back(value);
            }
            break;
            case asTYPEID_INT64: {
                int64_t value = **static_cast<int64_t **>(adr);
                store.push_back(value);
            }
            break;
            case asTYPEID_UINT8: {
                uint8_t value = **static_cast<uint8_t **>(adr);
                store.push_back(value);
            }
            break;
            case asTYPEID_UINT16: {
                uint16_t value = **static_cast<uint16_t **>(adr);
                store.push_back(value);
            }
            break;
            case asTYPEID_UINT32: {
                uint32_t value = **static_cast<uint32_t **>(adr);
                store.push_back(value);
            }
            break;
            case asTYPEID_UINT64: {
                uint64_t value = **static_cast<uint64_t **>(adr);
                store.push_back(value);
            }
            break;
            case asTYPEID_FLOAT: {
                float value = **static_cast<float **>(adr);
                store.push_back(value);
            }
            break;
            case asTYPEID_DOUBLE: {
                double value = **static_cast<double **>(adr);
                store.push_back(value);
            }
            break;
            default:
                store.push_back(ToString(gen, adr, typeId));
            break;
        }
    }

    return fmt::vformat(fmt, store);
}

static void FormatStringGeneric(asIScriptGeneric *gen) {
    new(gen->GetAddressOfReturnLocation()) std::string(std::move(FormatString(gen)));
}

static void Print(const std::string &str) {
    CKContext *context = GetCKContext(0);
    if (context) {
        context->OutputToConsole(const_cast<char *>(str.c_str()));
    }
}

static void PrintGeneric(asIScriptGeneric *gen) {
    const int count = gen->GetArgCount();
    if (count == 1) {
        Print(ArgToString(gen, 0));
    } else {
        Print(FormatString(gen));
    }
}

void RegisterScriptFormat(asIScriptEngine *engine) {
    assert(engine != nullptr);

    engine->SetUserData(ToStringMap::Create(), TOSTRINGMAP_TYPE);
    engine->SetEngineUserDataCleanupCallback([](asIScriptEngine *engine) {
        auto *map = static_cast<ToStringMap *>(engine->GetUserData(TOSTRINGMAP_TYPE));
        if (map) {
            while (map->Release() != 0);
        }
    }, TOSTRINGMAP_TYPE);

    int r = 0;

    r = engine->RegisterGlobalFunction("string toString(?&in)", asFUNCTION(ToStringGeneric), asCALL_GENERIC); assert(r >= 0);

    std::string decl = "string format(const string &in)";
    for (int i = 0; i <= 16; ++i) {
        r = engine->RegisterGlobalFunction(decl.c_str(), asFUNCTION(FormatStringGeneric), asCALL_GENERIC); assert(r >= 0);
        decl.pop_back();
        decl += ", ?&in)";
    }

    decl = "void print(?&in)";
    for (int i = 0; i <= 16; ++i) {
        r = engine->RegisterGlobalFunction(decl.c_str(), asFUNCTION(PrintGeneric), asCALL_GENERIC); assert(r >= 0);
        decl.pop_back();
        decl += ", ?&in)";
    }
}
