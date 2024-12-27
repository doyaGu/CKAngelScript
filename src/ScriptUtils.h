#ifndef CK_SCRIPTUTILS_H
#define CK_SCRIPTUTILS_H

#include <cassert>
#include <string>

#include <angelscript.h>

template<class A, class B>
B *StaticCast(A *a) {
    if (!a) return nullptr;
    return static_cast<B *>(a);
}

template <typename D, typename B>
static void RegisterClassValueCast(asIScriptEngine *engine, const char *derived, const char *base) {
    int r = 0;

    std::string decl = derived;
    decl.append(" opConv() const");
    r = engine->RegisterObjectMethod(base, decl.c_str(), asFUNCTIONPR((StaticCast<B, D>), (B *), D *), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    decl = base;
    decl.append(" opImplConv() const");
    r = engine->RegisterObjectMethod(derived, decl.c_str(), asFUNCTIONPR((StaticCast<D, B>), (D *), B *), asCALL_CDECL_OBJLAST); assert( r >= 0 );
}

template<class A, class B>
B *RefCast(A *a) {
    if (!a) return nullptr;
    return dynamic_cast<B *>(a);
}

template <typename D, typename B>
static void RegisterClassRefCast(asIScriptEngine *engine, const char *derived, const char *base) {
    int r = 0;

    std::string decl = derived;
    decl.append("@ opCast()");
    r = engine->RegisterObjectMethod(base, decl.c_str(), asFUNCTIONPR((RefCast<B, D>), (B *), D *), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    decl = base;
    decl.append("@ opImplCast()");
    r = engine->RegisterObjectMethod(derived, decl.c_str(), asFUNCTIONPR((RefCast<D, B>), (D *), B *), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    decl = "const ";
    decl.append(derived).append("@ opCast() const");
    r = engine->RegisterObjectMethod(base, decl.c_str(), asFUNCTIONPR((RefCast<B, D>), (B *), D *), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    decl = "const ";
    decl.append(base).append("@ opImplCast() const");
    r = engine->RegisterObjectMethod(derived, decl.c_str(), asFUNCTIONPR((RefCast<D, B>), (D *), B *), asCALL_CDECL_OBJLAST); assert( r >= 0 );
}

inline std::string ScriptStringify(const char *str) {
    if (str == nullptr)
        return {};
    return str;
}

#endif // CK_SCRIPTUTILS_H
