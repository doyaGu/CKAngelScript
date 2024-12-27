#ifndef CK_SCRIPTUTILS_H
#define CK_SCRIPTUTILS_H

#include <cassert>
#include <string>

#include <angelscript.h>

#define AS_TEMPORARY_FLAG_TYPE 3011
#define AS_RELEASED_ONCE_FLAG_TYPE 3012

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

inline int ExecuteScriptFunction(asIScriptContext *ctx, asIScriptFunction *func) {
    int r = ctx->Execute();
    if (r == asEXECUTION_EXCEPTION) {
        const char *section;
        int col;
        int row = ctx->GetLineNumber(0, &col, &section);
        std::string funcName = func->GetDeclaration();
        const char *exception = ctx->GetExceptionString();
        std::string msg = "Script execution exception in function '" + funcName + "': " + (exception ? exception : "Unknown exception");

        asIScriptEngine *engine = func->GetEngine();
        engine->WriteMessage(section, row, col, asMSGTYPE_ERROR, msg.c_str());
    } else if (r != asEXECUTION_FINISHED) {
        const char *section;
        int col;
        int row = ctx->GetLineNumber(0, &col, &section);
        std::string msg = "Script execution failed with result code: " + std::to_string(r);

        asIScriptEngine *engine = func->GetEngine();
        engine->WriteMessage(section, row, col, asMSGTYPE_ERROR, msg.c_str());
    }

    return r;
}

inline void MarkAsTemporary(asIScriptFunction *func) {
    if (func)
        func->SetUserData(func, AS_TEMPORARY_FLAG_TYPE);
}

inline void ClearTemporaryMark(asIScriptFunction *func) {
    if (func)
        func->SetUserData(nullptr, AS_TEMPORARY_FLAG_TYPE);
}

inline bool IsMarkedAsTemporary(asIScriptFunction *func) {
    if (!func) return false;
    void *p = func->GetUserData(AS_TEMPORARY_FLAG_TYPE);
    if (p != func)
        return false;
    return true;
}

inline void MarkAsReleasedOnce(asIScriptFunction *func) {
    if (func)
        func->SetUserData(func, AS_RELEASED_ONCE_FLAG_TYPE);
}

inline void ClearReleasedOnceMark(asIScriptFunction *func) {
    if (func)
        func->SetUserData(nullptr, AS_RELEASED_ONCE_FLAG_TYPE);
}

inline bool IsMarkedAsReleasedOnce(asIScriptFunction *func) {
    if (!func) return false;
    void *p = func->GetUserData(AS_RELEASED_ONCE_FLAG_TYPE);
    if (p != func)
        return false;
    return true;
}

#endif // CK_SCRIPTUTILS_H
