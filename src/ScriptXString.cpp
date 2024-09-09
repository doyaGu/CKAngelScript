#include "ScriptXString.h"

#include <unordered_map>

#include "XString.h"

namespace std {
    // BKDR hash function for XString
    template<>
    struct hash<XString> {
        size_t operator()(const XString &str) const {
            size_t hash = 0;

            if (!str.Empty()) {
                const char *pch = str.CStr();
                while (char ch = *pch++) {
                    hash = hash * 131 + ch;
                }
            }

            return hash;
        }
    };
}

class CXStringFactory : public asIStringFactory {
public:
    CXStringFactory() = default;

    ~CXStringFactory() override {
        // The script engine must release each string 
        // constant that it has requested
        assert(m_StringCache.empty());
    }

    const void *GetStringConstant(const char *data, asUINT length) override {
        // The string factory might be modified from multiple 
        // threads, so it is necessary to use a mutex.
        asAcquireExclusiveLock();

        XString str(data, (int) length);
        auto it = m_StringCache.find(str);
        if (it != m_StringCache.end())
            it->second++;
        else
            it = m_StringCache.insert(XStringCache::value_type(str, 1)).first;

        asReleaseExclusiveLock();

        return reinterpret_cast<const void *>(&it->first);
    }

    int ReleaseStringConstant(const void *str) override {
        if (!str)
            return asERROR;

        int ret = asSUCCESS;

        // The string factory might be modified from multiple 
        // threads, so it is necessary to use a mutex.
        asAcquireExclusiveLock();

        auto it = m_StringCache.find(*reinterpret_cast<const XString *>(str));
        if (it == m_StringCache.end())
            ret = asERROR;
        else {
            it->second--;
            if (it->second == 0)
                m_StringCache.erase(it);
        }

        asReleaseExclusiveLock();

        return ret;
    }

    int GetRawStringData(const void *str, char *data, asUINT *length) const override {
        if (!str)
            return asERROR;

        if (length)
            *length = (asUINT) reinterpret_cast<const XString *>(str)->Length();

        if (data)
            memcpy(data, reinterpret_cast<const XString *>(str)->CStr(), reinterpret_cast<const XString *>(str)->Length());

        return asSUCCESS;
    }

    // THe access to the string cache is protected with the common mutex provided by AngelScript
    typedef std::unordered_map<XString, int> XStringCache;
    XStringCache m_StringCache;
};

static CXStringFactory *s_StringFactory = nullptr;

CXStringFactory *GetXStringFactorySingleton() {
    if (!s_StringFactory) {
        // The following instance will be destroyed by the global 
        // CXStringFactoryCleaner instance upon application shutdown
        s_StringFactory = new CXStringFactory();
    }
    return s_StringFactory;
}

class CXStringFactoryCleaner {
public:
    ~CXStringFactoryCleaner() {
        if (s_StringFactory) {
            // Only delete the string factory if the m_StringCache is empty
            // If it is not empty, it means that someone might still attempt
            // to release string constants, so if we delete the string factory
            // the application might crash. Not deleting the cache would
            // lead to a memory leak, but since this is only happens when the
            // application is shutting down anyway, it is not important.
            if (s_StringFactory->m_StringCache.empty()) {
                delete s_StringFactory;
                s_StringFactory = nullptr;
            }
        }
    }
};

static CXStringFactoryCleaner s_StringCleaner;

static void ConstructString(XString *ptr) {
    new(ptr) XString();
}

static void CopyConstructString(const XString &other, XString *ptr) {
    new(ptr) XString(other);
}

static void DestructString(XString *ptr) {
    ptr->~XString();
}

static XString &AddAssignStringToString(const XString &str, XString &dest) {
    // We don't register the method directly because some compilers
    // and standard libraries inline the definition, resulting in the
    // linker being unable to find the declaration.
    // Example: CLang/LLVM with XCode 4.3 on OSX 10.7
    dest += str;
    return dest;
}

static bool StringIsEmpty(const XString &str) {
    // We don't register the method directly because some compilers
    // and standard libraries inline the definition, resulting in the
    // linker being unable to find the declaration
    // Example: CLang/LLVM with XCode 4.3 on OSX 10.7
    return str.Empty();
}

static XString StringAdd(const XString &lhs, const XString &rhs) {
    return lhs + rhs;
}

static bool StringEquals(const XString &lhs, const XString &rhs) {
    return lhs == rhs;
}

static int StringCmp(const XString &a, const XString &b) {
    int cmp = 0;
    if (a < b) cmp = -1;
    else if (a > b) cmp = 1;
    return cmp;
}

static asUINT StringLength(const XString &str) {
    return (asUINT) str.Length();
}

static void StringResize(asUINT l, XString &str) {
    str.Resize(l);
}

static bool StringEmpty(const XString &str) {
    return str.Empty() == TRUE;
}

static char *StringCharAt(unsigned int i, XString &str) {
    if (i >= str.Length()) {
        // Set a script exception
        asIScriptContext *ctx = asGetActiveContext();
        ctx->SetException("Out of range");

        // Return a null pointer
        return nullptr;
    }

    return &str[(int) i];
}

void RegisterXString(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    r = engine->RegisterObjectType("XString", sizeof(XString), asOBJ_VALUE | asGetTypeTraits<XString>()); assert(r >= 0);

    // r = engine->RegisterStringFactory("XString", GetXStringFactorySingleton()); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("XString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructString), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("XString", asBEHAVE_CONSTRUCT, "void f(const XString &in)", asFUNCTION(CopyConstructString), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("XString", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructString), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("XString", "XString &opAssign(const XString &in)", asMETHODPR(XString, operator=, (const XString&), XString &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XString", "XString &opAddAssign(const XString &in)", asFUNCTION(AddAssignStringToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("XString", "bool opEquals(const XString &in) const", asFUNCTIONPR(StringEquals, (const XString &, const XString &), bool), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("XString", "int opCmp(const XString &in) const", asFUNCTION(StringCmp), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("XString", "XString opAdd(const XString &in) const", asFUNCTIONPR(StringAdd, (const XString &, const XString &), XString), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("XString", "uint Length() const", asFUNCTION(StringLength), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("XString", "void Resize(uint)", asFUNCTION(StringResize), asCALL_CDECL_OBJLAST); assert(r >= 0);

    r = engine->RegisterObjectMethod("XString", "bool Empty() const", asFUNCTION(StringEmpty), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Register the index operator, both as a mutator and as an inspector
    // Note that we don't register the operator[] directly, as it doesn't do bounds checking
    r = engine->RegisterObjectMethod("XString", "uint8 &opIndex(uint)", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("XString", "const uint8 &opIndex(uint) const", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert(r >= 0);
}
