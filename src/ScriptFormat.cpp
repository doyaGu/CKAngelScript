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

#define TOSTRINGMAP_TYPE 3001

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

asIScriptFunction *FindToStringCallback(asIScriptEngine *engine, int typeId) {
    auto *map = static_cast<ToStringMap *>(engine->GetUserData(TOSTRINGMAP_TYPE));
    if (!map) {
        engine->WriteMessage("print", 0, 0, asMSGTYPE_ERROR, "ToStringMap has been overwritten.");
        return nullptr;
    }

    auto *func = map->GetToStringFunction(typeId);
    if (func) {
        return func;
    }

    const asITypeInfo *type = engine->GetTypeInfoById(typeId);
    if (!type) {
        engine->WriteMessage("print", 0, 0, asMSGTYPE_ERROR, "Type not found.");
        return nullptr;
    }

    // Special case - cast to string
    const char *name = type->GetName();
    if (strcmp(name, "string") == 0) {
        map->SetToStringFunction(typeId, nullptr);
        return nullptr;
    }

    func = type->GetMethodByDecl("string opImplConv() const");
    if (!func) {
        func = type->GetMethodByDecl("string opConv() const");
    }
    if (!func) {
        func = type->GetMethodByDecl("string opImplConv()");
    }
    if (!func) {
        func = type->GetMethodByDecl("string opConv()");
    }

    map->SetToStringFunction(typeId, func);
    return func;
}

static void ArgToStringStream(asIScriptGeneric *gen, int index, std::stringstream &stream) {
    asIScriptEngine *engine = gen->GetEngine();
    const int typeId = gen->GetArgTypeId(index);
    void *addr = *static_cast<void **>(gen->GetAddressOfArg(index));

    if (addr == nullptr) {
        stream << "null";
        return;
    }

    if (typeId == asTYPEID_VOID) {
        stream << "void";
    } else if (typeId >= asTYPEID_BOOL && typeId <= asTYPEID_DOUBLE) {
        switch (typeId) {
            case asTYPEID_BOOL: {
                const bool value = *static_cast<bool *>(addr);
                stream << (value ? "true" : "false");
            }
            break;
            case asTYPEID_INT8: {
                const int8_t value = *static_cast<int8_t *>(addr);
                stream << value;
            }
            break;
            case asTYPEID_INT16: {
                const int16_t value = *static_cast<int16_t *>(addr);
                stream << value;
            }
            break;
            case asTYPEID_INT32: {
                const int32_t value = *static_cast<int32_t *>(addr);
                stream << value;
            }
            break;
            case asTYPEID_INT64: {
                const int64_t value = *static_cast<int64_t *>(addr);
                stream << value;
            }
            break;
            case asTYPEID_UINT8: {
                const uint8_t value = *static_cast<uint8_t *>(addr);
                stream << value;
            }
            break;
            case asTYPEID_UINT16: {
                const uint16_t value = *static_cast<uint16_t *>(addr);
                stream << value;
            }
            break;
            case asTYPEID_UINT32: {
                const uint32_t value = *static_cast<uint32_t *>(addr);
                stream << value;
            }
            break;
            case asTYPEID_UINT64: {
                const uint64_t value = *static_cast<uint64_t *>(addr);
                stream << value;
            }
            break;
            case asTYPEID_FLOAT: {
                const float value = *static_cast<float *>(addr);
                stream << value;
            }
            break;
            case asTYPEID_DOUBLE: {
                const double value = *static_cast<double *>(addr);
                stream << value;
            }
            break;
            default:
                break;
        }
    } else if (!(typeId & asTYPEID_MASK_OBJECT)) {
        // The type is an enum, check if the value matches one of the defined enums
        bool found = false;
        const asITypeInfo *type = engine->GetTypeInfoById(typeId);
        const int value = *static_cast<int *>(addr);
        for (int n = static_cast<int>(type->GetEnumValueCount()); --n > 0;) {
            int enumVal;
            const char *enumName = type->GetEnumValueByIndex(n, &enumVal);
            if (enumVal == value) {
                stream << enumName;
                found = true;
                break;
            }
        }
        if (!found) {
            stream << value;
        }
    } else if (typeId & asTYPEID_MASK_OBJECT) {
        // Dereference handles, so we can see what it points to
        if (typeId & asTYPEID_OBJHANDLE)
            addr = *static_cast<void **>(addr);

        asITypeInfo *type = engine->GetTypeInfoById(typeId);
        if (!type) {
            stream << "unknown";
            return;
        }

        const char *typeName = type->GetName();
        if (strcmp(typeName, "string") == 0) {
            stream << *static_cast<std::string *>(addr);
            return;
        }

        asIScriptFunction *func = FindToStringCallback(engine, typeId);
        if (func) {
            // Call function
            asIScriptContext *ctx = engine->RequestContext();
            int r = ctx->Prepare(func);
            if (r >= 0) {
                r = ctx->SetObject(addr);
                if (r >= 0)
                    r = ctx->Execute();
            }
            if (r < 0) {
                stream << "<" << typeName << ":" << addr << ">";
                return;
            }

            void *retAddr = ctx->GetReturnAddress();
            stream << *static_cast<std::string *>(retAddr);
            engine->ReturnContext(ctx);
        } else {
            stream << "<" << typeName << ":" << addr << ">";
        }
    } else {
        asITypeInfo *type = engine->GetTypeInfoById(typeId);
        if (!type) {
            stream << "unknown";
            return;
        }
        const char *typeName = type->GetName();
        stream << "<" << typeName << ":" << addr << ">";
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
    asIScriptEngine *engine = gen->GetEngine();
    const std::string &fmt = **static_cast<std::string **>(gen->GetAddressOfArg(0));
    fmt::dynamic_format_arg_store<fmt::format_context> store;

    const int count = gen->GetArgCount();
    for (int i = 1; i < count; ++i) {
        const int typeId = gen->GetArgTypeId(i);
        void *addr = *static_cast<void**>(gen->GetAddressOfArg(i));

        if (addr == nullptr) {
            store.push_back("null");
            continue;
        }

        if (typeId == asTYPEID_VOID) {
            store.push_back("void");
        } else if (typeId >= asTYPEID_BOOL && typeId <= asTYPEID_DOUBLE) {
            switch (typeId) {
                case asTYPEID_BOOL: {
                    const bool value = *static_cast<bool *>(addr);
                    store.push_back(value ? "true" : "false");
                }
                break;
                case asTYPEID_INT8: {
                    const int8_t value = *static_cast<int8_t *>(addr);
                    store.push_back(value);
                }
                break;
                case asTYPEID_INT16: {
                    const int16_t value = *static_cast<int16_t *>(addr);
                    store.push_back(value);
                }
                break;
                case asTYPEID_INT32: {
                    const int32_t value = *static_cast<int32_t *>(addr);
                    store.push_back(value);
                }
                break;
                case asTYPEID_INT64: {
                    const int64_t value = *static_cast<int64_t *>(addr);
                    store.push_back(value);
                }
                break;
                case asTYPEID_UINT8: {
                    const uint8_t value = *static_cast<uint8_t *>(addr);
                    store.push_back(value);
                }
                break;
                case asTYPEID_UINT16: {
                    const uint16_t value = *static_cast<uint16_t *>(addr);
                    store.push_back(value);
                }
                break;
                case asTYPEID_UINT32: {
                    const uint32_t value = *static_cast<uint32_t *>(addr);
                    store.push_back(value);
                }
                break;
                case asTYPEID_UINT64: {
                    const uint64_t value = *static_cast<uint64_t *>(addr);
                    store.push_back(value);
                }
                break;
                case asTYPEID_FLOAT: {
                    const float value = *static_cast<float *>(addr);
                    store.push_back(value);
                }
                break;
                case asTYPEID_DOUBLE: {
                    const double value = *static_cast<double *>(addr);
                    store.push_back(value);
                }
                break;
                default:
                    break;
            }
        } else if (!(typeId & asTYPEID_MASK_OBJECT)) {
            // The type is an enum, check if the value matches one of the defined enums
            bool found = false;
            const asITypeInfo *type = engine->GetTypeInfoById(typeId);
            if (!type) {
                store.push_back("unknown");
                continue;
            }
            const int value = *static_cast<int *>(addr);
            for (int n = static_cast<int>(type->GetEnumValueCount()); --n > 0;) {
                int enumVal;
                const char *enumName = type->GetEnumValueByIndex(n, &enumVal);
                if (enumVal == value) {
                    store.push_back(enumName);
                    found = true;
                    break;
                }
            }
            if (!found) {
                store.push_back(value);
            }
        } else if (typeId & asTYPEID_MASK_OBJECT) {
            // Dereference handles, so we can see what it points to
            if (typeId & asTYPEID_OBJHANDLE)
                addr = *static_cast<void **>(addr);

            asITypeInfo *type = engine->GetTypeInfoById(typeId);
            if (!type) {
                store.push_back("unknown");
                continue;
            }
            const char *typeName = type->GetName();
            if (strcmp(typeName, "string") == 0) {
                store.push_back(*static_cast<std::string *>(addr));
                continue;
            }

            asIScriptFunction *func = FindToStringCallback(engine, typeId);
            if (func) {
                // Call function
                asIScriptContext *ctx = engine->RequestContext();
                int r = ctx->Prepare(func);
                if (r >= 0) {
                    r = ctx->SetObject(addr);
                    if (r >= 0)
                        r = ctx->Execute();
                }
                if (r < 0) {
                    std::stringstream stream;
                    stream << "<" << typeName << ":" << addr << ">";
                    store.push_back(stream.str());
                    continue;
                }

                void *retAddr = ctx->GetReturnAddress();
                store.push_back(*static_cast<std::string *>(retAddr));
                engine->ReturnContext(ctx);
            } else {
                std::stringstream stream;
                stream << "<" << typeName << ":" << addr << ">";
                store.push_back(stream.str());
            }
        } else {
            asITypeInfo *type = engine->GetTypeInfoById(typeId);
            if (!type) {
                store.push_back("unknown");
                continue;
            }
            const char *typeName = type->GetName();

            std::stringstream stream;
            stream << "<" << typeName << ":" << addr << ">";
            store.push_back(stream.str());
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

static std::string TypeOf(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();
    const auto typeId = gen->GetArgTypeId(0);
    void *addr = *static_cast<void **>(gen->GetAddressOfArg(0));

    if (addr == nullptr) {
        return "null";
    }

    if (typeId == asTYPEID_VOID) {
        return "void";
    }

    if (typeId >= asTYPEID_BOOL && typeId <= asTYPEID_DOUBLE) {
        switch (typeId) {
            case asTYPEID_BOOL:
                return "bool";
            case asTYPEID_INT8:
                return "int8";
            case asTYPEID_INT16:
                return "int16";
            case asTYPEID_INT32:
                return "int32";
            case asTYPEID_INT64:
                return "int64";
            case asTYPEID_UINT8:
                return "uint8";
            case asTYPEID_UINT16:
                return "uint16";
            case asTYPEID_UINT32:
                return "uint32";
            case asTYPEID_UINT64:
                return "uint64";
            case asTYPEID_FLOAT:
                return "float";
            case asTYPEID_DOUBLE:
                return "double";
            default:
                break;
        }
    }

    asITypeInfo *type = engine->GetTypeInfoById(typeId);
    if (!type) {
        return "unknown";
    }

    if (typeId & asTYPEID_OBJHANDLE)
        return std::string(type->GetName()) + "@";
    return type->GetName();
}

static void TypeOfGeneric(asIScriptGeneric *gen) {
    new(gen->GetAddressOfReturnLocation()) std::string(std::move(TypeOf(gen)));
}

static size_t SizeOf(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();
    const int typeId = gen->GetArgTypeId(0);
    size_t size = engine->GetSizeOfPrimitiveType(typeId);
    if (size == 0) {
        asITypeInfo *type = engine->GetTypeInfoById(typeId);
        if (!type) {
            return 0;
        }
        size = type->GetSize();
    }
    return size;
}

static void SizeOfGeneric(asIScriptGeneric *gen) {
    gen->SetReturnDWord(SizeOf(gen));
}

void RegisterScriptFormat(asIScriptEngine *engine, int argc) {
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
    for (int i = 0; i <= argc; ++i) {
        r = engine->RegisterGlobalFunction(decl.c_str(), asFUNCTION(FormatStringGeneric), asCALL_GENERIC); assert(r >= 0);
        decl.pop_back();
        decl += ", ?&in)";
    }

    r = engine->RegisterGlobalFunction("void print(?&in)", asFUNCTION(PrintGeneric), asCALL_GENERIC); assert(r >= 0);

    decl = "void print(const string &in)";
    for (int i = 0; i <= argc; ++i) {
        r = engine->RegisterGlobalFunction(decl.c_str(), asFUNCTION(PrintGeneric), asCALL_GENERIC); assert(r >= 0);
        decl.pop_back();
        decl += ", ?&in)";
    }

    r = engine->RegisterGlobalFunction("string typeof(?&in)", asFUNCTION(TypeOfGeneric), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint sizeof(?&in)", asFUNCTION(SizeOfGeneric), asCALL_GENERIC); assert(r >= 0);
}
