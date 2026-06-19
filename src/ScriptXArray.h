#ifndef CK_SCRIPTXSARRAY_H
#define CK_SCRIPTXSARRAY_H

#include <angelscript.h>
#include <type_traits>
#include <utility>

#include "XString.h"
#include "XArray.h"
#include "XSArray.h"
#include "XClassArray.h"
#include "ScriptRegistration.h"

template<typename T>
class XIterator {
public:
    XIterator() : m_Ptr(nullptr) {}

    XIterator(T *ptr) : m_Ptr(ptr) {}

    XIterator(const XIterator &it) : m_Ptr(it.m_Ptr) {}

    int operator==(const XIterator &it) const { return m_Ptr == it.m_Ptr; }
    int operator!=(const XIterator &it) const { return m_Ptr != it.m_Ptr; }

    const T &operator*() const { return *m_Ptr; }
    T &operator*() { return *m_Ptr; }

    explicit operator const T *() const { return m_Ptr; }
    explicit operator T *() { return m_Ptr; }

    XIterator &operator++() {
        ++m_Ptr;
        return *this;
    }

    XIterator &operator--() {
        --m_Ptr;
        return *this;
    }

    XIterator operator++(int) {
        XIterator tmp = *this;
        ++*this;
        return tmp;
    }

    XIterator operator--(int) {
        XIterator tmp = *this;
        --*this;
        return tmp;
    }

    bool IsValid() const { return m_Ptr != nullptr; }

    T *m_Ptr;
};

template<typename C>
static bool CheckXIteratorValid(const C &self, const char *operation) {
    if (self.IsValid()) {
        return true;
    }
    if (asIScriptContext *ctx = asGetActiveContext()) {
        XString message;
        message.Format("%s requires a valid iterator.", operation);
        ctx->SetException(message.CStr());
    }
    return false;
}

template<typename C, typename T>
static const T &GetXIteratorValueConst(const C &self) {
    static const T empty = T();
    if (!CheckXIteratorValid(self, "Iterator.Get")) {
        return empty;
    }
    return *self;
}

template<typename C, typename T>
static T &GetXIteratorValue(C &self) {
    static T empty = T();
    if (!CheckXIteratorValid(self, "Iterator.Get")) {
        return empty;
    }
    return *self;
}

template<typename C>
static C &IncrementXIterator(C &self) {
    if (CheckXIteratorValid(self, "Iterator.opPreInc")) {
        ++self;
    }
    return self;
}

template<typename C>
static C &DecrementXIterator(C &self) {
    if (CheckXIteratorValid(self, "Iterator.opPreDec")) {
        --self;
    }
    return self;
}

static void SetXArrayException(const char *operation) {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        XString message;
        message.Format("%s index is out of range.", operation);
        ctx->SetException(message.CStr());
    }
}

template<typename C, typename T>
static T &GetXClassArrayValue(C &self, int index) {
    static T empty = T();
    if (index < 0 || index >= self.Size()) {
        SetXArrayException("XClassArray.opIndex");
        return empty;
    }
    return self[index];
}

template<typename C, typename T>
static const T &GetXClassArrayBackConst(const C &self) {
    static const T empty = T();
    if (self.Size() <= 0) {
        SetXArrayException("XClassArray.Back");
        return empty;
    }
    return self.Back();
}

template<typename C, typename T>
static T &GetXClassArrayBack(C &self) {
    static T empty = T();
    if (self.Size() <= 0) {
        SetXArrayException("XClassArray.Back");
        return empty;
    }
    return self.Back();
}

template<typename C, typename T>
static bool RemoveXClassArrayValueAt(C &self, int index, T &old) {
    if (index < 0 || index >= self.Size()) {
        return false;
    }
    old = self[index];
    self.RemoveAt(index);
    return true;
}

template<typename C>
static int GetXClassArrayMemoryOccupationBool(const C &self, bool addStatic) {
    return self.GetMemoryOccupation(addStatic ? 1 : 0);
}

template<typename C, typename T>
void RegisterXIterator(asIScriptEngine *engine, const char *className, const char *elementType) {
    int r = 0;
    XString decl;
    XString name = decl.Format("%sIt", className);

    r = engine->RegisterObjectType(name.CStr(), sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>());
    if (r == asALREADY_REGISTERED)
        return;
    CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](C *self) { new(self) C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in other)", name.CStr());
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in other)", name.CStr(), name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool opEquals(const %s &in other) const", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTIONPR([](const C &self, const C &other) -> bool { return self == other; }, (const C &, const C &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("bool opNotEquals(const %s &in other) const", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTIONPR([](const C &self, const C &other) -> bool { return self != other; }, (const C &, const C &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("const %s &Get() const", elementType);
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((GetXIteratorValueConst<C, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &Get()", elementType);
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((GetXIteratorValue<C, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opPreInc()", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((IncrementXIterator<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opPreDec()", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((DecrementXIterator<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // decl.Format("%s opPostInc(int)", name.CStr());
    // r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(XIterator<T>, operator++, (int), XIterator<T> &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    //
    // decl.Format("%s opPostDec(int)", name.CStr());
    // r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(XIterator<T>, operator--, (int), XIterator<T> &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(name.CStr(), "bool IsValid() const", asMETHODPR(C, IsValid, () const, bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T>
void RegisterXArray(asIScriptEngine *engine, const char *className, const char *elementType) {
    int r = 0;
    XString decl;

    RegisterXIterator<XIterator<T>, T>(engine, className, elementType);

    r = engine->RegisterObjectType(className, sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>()); assert(r >= 0 || r == asALREADY_REGISTERED);
    if (r == asALREADY_REGISTERED)
        return;

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](C *self) { new(self) C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f(int size)", asFUNCTIONPR([](int ss, C *self) { new(self) C(ss); }, (int, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in other)", className);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in other)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XArray<T>, operator=, (const XArray<T> &), XArray<T> &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAddAssign(const %s &in other)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XArray<T>, operator+=, (const XArray<T> &), XArray<T> &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opSubAssign(const %s &in other)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XArray<T>, operator-=, (const XArray<T> &), XArray<T> &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opIndex(uint index)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](C &array, int i) -> T & { return array[i]; }, (C &, int), T &), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("const %s &opIndex(uint index) const", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array, int i) -> const T & { return array[i]; }, (const C &, int), const T &), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

    r = engine->RegisterObjectMethod(className, "void Clear()", asMETHODPR(C, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void Compact()", asMETHODPR(C, Compact, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void Reserve(int size)", asMETHODPR(C, Reserve, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void Resize(int size)", asMETHODPR(C, Resize, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void Expand(int e = 1)", asMETHODPR(C, Expand, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void Compress(int e = 1)", asMETHODPR(C, Compress, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void PushBack(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, PushBack, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void PushFront(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, PushFront, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Insert(int pos, const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Insert, (int, const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void InsertSorted(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, InsertSorted, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "void PopBack()", asMETHODPR(XArray<T>, PopBack, (), T), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void PopFront()", asMETHODPR(XArray<T>, PopFront, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool RemoveAt(uint pos, %s &out old)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, RemoveAt, (unsigned int, T &), XBOOL), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "bool EraseAt(int pos)", asMETHODPR(C, EraseAt, (int), XBOOL), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool Erase(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Erase, (const T &), XBOOL), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void FastRemove(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, FastRemove, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod(className, "void FastRemoveAt(int pos)", asMETHODPR(C, FastRemoveAt, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif

    decl.Format("void Fill(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Fill, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "void Memset(uint8 val)", asMETHODPR(C, Memset, (XBYTE), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool IsHere(const %s &in o) const", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, IsHere, (const T &) const, XBOOL), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("int GetPosition(const %s &in o) const", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, GetPosition, (const T &) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod(className, "void Swap(int pos1, int pos2)", asMETHODPR(C, Swap, (int, int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Swap(%s &inout other)", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XArray<T>, Swap, (XArray<T> &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif

    decl.Format("const %s &Front() const", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Front, () const, const T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &Front()", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Front, (), T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("const %s &Back() const", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Back, () const, const T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &Back()", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Back, (), T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Begin() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array) -> XIterator<T> { return array.Begin(); }, (const C &), XIterator<T>), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt RBegin() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array) -> XIterator<T> { return array.RBegin(); }, (const C &), XIterator<T>), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt End() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array) -> XIterator<T> { return array.End(); }, (const C &), XIterator<T>), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt REnd() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array) -> XIterator<T> { return array.REnd(); }, (const C &), XIterator<T>), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "int Size() const", asMETHODPR(C, Size, () const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod(className, "bool IsEmpty() const", asMETHODPR(C, IsEmpty, () const, bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod(className, "int GetMemoryOccupation(bool addStatic = false) const", asMETHODPR(C, GetMemoryOccupation, (XBOOL) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T>
void RegisterXSArray(asIScriptEngine *engine, const char *className, const char *elementType) {
    int r = 0;
    XString decl;

    RegisterXIterator<XIterator<T>, T>(engine, className, elementType);

    r = engine->RegisterObjectType(className, sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>()); assert(r >= 0 || r == asALREADY_REGISTERED);
    if (r == asALREADY_REGISTERED)
        return;

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](C *self) { new(self) C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in other)", className);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in other)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XSArray<T>, operator=, (const XSArray<T> &), XSArray<T> &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAddAssign(const %s &in other)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XSArray<T>, operator+=, (const XSArray<T> &), XSArray<T> &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opSubAssign(const %s &in other)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XSArray<T>, operator-=, (const XSArray<T> &), XSArray<T> &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opIndex(uint index)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XSArray<T>, operator[], (unsigned int) const, T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("const %s &opIndex(uint index) const", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array, int i) -> const T & { return array[i]; }, (const C &, int), const T &), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

    r = engine->RegisterObjectMethod(className, "void Clear()", asMETHODPR(C, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Fill(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Fill, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "void Resize(int size)", asMETHODPR(C, Resize, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void PushBack(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, PushBack, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void PushFront(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, PushFront, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Insert(int pos, const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Insert, (int, const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "void PopBack()", asMETHODPR(C, PopBack, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void PopFront()", asMETHODPR(C, PopFront, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    // decl.Format("bool At(uint pos, %s &out o) const", elementType);
    // r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array, unsigned int i, T &value) -> bool {
    //     auto *e = array.At(i);
    //     if (!e) return false;
    //     value = *e;
    //     return true;
    // }, (const C &, int, T &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("bool IsHere(const %s &in o) const", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XSArray<T>, IsHere, (const T &) const, XBOOL), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("int GetPosition(const %s &in o) const", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, GetPosition, (const T &) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod(className, "void Swap(int pos1, int pos2)", asMETHODPR(C, Swap, (int, int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Swap(%s &inout other)", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XSArray<T>, Swap, (XSArray<T> &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif

    decl.Format("%sIt Begin() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array) -> XIterator<T> { return array.Begin(); }, (const C &), XIterator<T>), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt End() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array) -> XIterator<T> { return array.End(); }, (const C &), XIterator<T>), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "int Size() const", asMETHODPR(C, Size, () const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "int GetMemoryOccupation(bool addStatic = false) const", asMETHODPR(XSArray<T>, GetMemoryOccupation, (XBOOL) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T, bool GuardElementAccess = false>
void RegisterXClassArray(asIScriptEngine *engine,
                         const char *className,
                         const char *elementType,
                         bool registerIteratorProducers = false) {
    int r = 0;
    XString decl;

    RegisterXIterator<XIterator<T>, T>(engine, className, elementType);

    r = engine->RegisterObjectType(className, sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>()); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](C *self) { new(self) C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f(int size)", asFUNCTIONPR([](int ss, C *self) { new(self) C(ss); }, (int, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in other)", className);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in other)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, operator=, (const XClassArray<T> &), XClassArray<T> &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    if constexpr (GuardElementAccess) {
        decl.Format("%s &opIndex(int index)", elementType);
        r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((GetXClassArrayValue<C, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    } else {
        decl.Format("%s &opIndex(int index)", elementType);
        r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, operator[], (int) const, T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }

    r = engine->RegisterObjectMethod(className, "void Clear()", asMETHODPR(C, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void Reserve(int size)", asMETHODPR(C, Reserve, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void Resize(int e = 1)", asMETHODPR(C, Resize, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void Expand(int e = 1)", asMETHODPR(C, Expand, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void PushBack(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, PushBack, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void PushFront(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, PushFront, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Insert(int pos, const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, Insert, (int, const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "void PopBack()", asMETHODPR(C, PopBack, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void PopFront()", asMETHODPR(C, PopFront, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    if constexpr (GuardElementAccess) {
        decl.Format("bool RemoveAt(int pos, %s &out old)", elementType);
        r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((RemoveXClassArrayValueAt<C, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    } else {
        decl.Format("%s &RemoveAt(int pos)", elementType);
        r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, RemoveAt, (int), T *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }

    decl.Format("void FastRemove(const %s &in o)", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, FastRemove, (const T &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("int GetPosition(const %s &in o) const", elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, GetPosition, (const T &) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod(className, "void Swap(int pos1, int pos2)", asMETHODPR(C, Swap, (int, int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Swap(%s &inout other)", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, Swap, (XClassArray<T> &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif

    if constexpr (GuardElementAccess) {
        decl.Format("const %s &Back() const", elementType);
        r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((GetXClassArrayBackConst<C, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

        decl.Format("%s &Back()", elementType);
        r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((GetXClassArrayBack<C, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    } else {
        decl.Format("const %s &Back() const", elementType);
        r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, Back, () const, const T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

        decl.Format("%s &Back()", elementType);
        r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(XClassArray<T>, Back, (), T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }

    if constexpr (std::is_convertible_v<decltype(std::declval<const C &>().Begin()), T *> &&
                  std::is_convertible_v<decltype(std::declval<const C &>().End()), T *>) {
        if (registerIteratorProducers) {
            decl.Format("%sIt Begin() const", className);
            r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array) -> XIterator<T> { return XIterator<T>(array.Begin()); }, (const C &), XIterator<T>), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

            decl.Format("%sIt End() const", className);
            r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &array) -> XIterator<T> { return XIterator<T>(array.End()); }, (const C &), XIterator<T>), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
        }
    }

    r = engine->RegisterObjectMethod(className, "int Size() const", asMETHODPR(C, Size, () const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "int Allocated() const", asMETHODPR(C, Allocated, () const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    if constexpr (GuardElementAccess) {
        r = engine->RegisterObjectMethod(className, "int GetMemoryOccupation(bool addStatic = false) const", asFUNCTION((GetXClassArrayMemoryOccupationBool<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    } else {
        r = engine->RegisterObjectMethod(className, "int GetMemoryOccupation(bool addStatic = false) const", asMETHODPR(C, GetMemoryOccupation, (XBOOL) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }
}

#endif // CK_SCRIPTXSARRAY_H
