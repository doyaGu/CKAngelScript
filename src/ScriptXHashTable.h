#ifndef CK_SCRIPTXHASHTABLE_H
#define CK_SCRIPTXHASHTABLE_H

#include <angelscript.h>
#include <type_traits>
#include <utility>

#include "XString.h"
#include "XHashTable.h"
#include "XNHashTable.h"
#include "XSHashTable.h"
#include "ScriptRegistration.h"

static void SetXHashTableException(const char *operation, const char *detail) {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        XString message;
        message.Format("%s %s", operation, detail);
        ctx->SetException(message.CStr());
    }
}

template<typename C, typename = void>
struct XHashTableIteratorState {
    static bool HasNode(const C &self) {
        C invalid;
        return !(self == invalid);
    }

    template<typename Table>
    static bool BelongsTo(const C &, const Table &) {
        return true;
    }
};

template<typename C>
struct XHashTableIteratorState<C, std::void_t<decltype(std::declval<const C &>().m_Node),
                                             decltype(std::declval<const C &>().m_Table)>> {
    static bool HasNode(const C &self) {
        return self.m_Node != nullptr && self.m_Table != nullptr;
    }

    template<typename Table>
    static bool BelongsTo(const C &self, const Table &table) {
        return static_cast<const void *>(self.m_Table) == static_cast<const void *>(&table);
    }
};

template<typename C>
static bool CheckXHashTableIteratorValid(const C &self, const char *operation) {
    if (XHashTableIteratorState<C>::HasNode(self)) {
        return true;
    }
    SetXHashTableException(operation, "requires a valid iterator.");
    return false;
}

template<typename Table, typename It>
static bool CheckXHashTableIteratorForTable(const Table &table, const It &it, const char *operation) {
    if (!CheckXHashTableIteratorValid(it, operation)) {
        return false;
    }
    if (!XHashTableIteratorState<It>::BelongsTo(it, table)) {
        SetXHashTableException(operation, "requires an iterator from the same table.");
        return false;
    }
    return true;
}

template<typename C>
static bool EqualXHashTableIterator(const C &self, const C &other) {
    return self == other;
}

template<typename C, typename K>
static const K &GetXHashTableIteratorKey(const C &self) {
    static const K empty = K();
    if (!CheckXHashTableIteratorValid(self, "XHashTableIt.GetKey")) {
        return empty;
    }
    return self.GetKey();
}

template<typename C, typename T>
static const T &GetXHashTableIteratorValueConst(const C &self) {
    static const T empty = T();
    if (!CheckXHashTableIteratorValid(self, "XHashTableIt.GetValue")) {
        return empty;
    }
    return *self;
}

template<typename C, typename T>
static T &GetXHashTableIteratorValue(C &self) {
    static T empty = T();
    if (!CheckXHashTableIteratorValid(self, "XHashTableIt.GetValue")) {
        return empty;
    }
    return *self;
}

template<typename C>
static C &IncrementXHashTableIterator(C &self) {
    if (CheckXHashTableIteratorValid(self, "XHashTableIt.opPreInc")) {
        ++self;
    }
    return self;
}

template<typename C>
static C PostIncrementXHashTableIterator(C &self, int) {
    C old(self);
    if (CheckXHashTableIteratorValid(self, "XHashTableIt.opPostInc")) {
        ++self;
    }
    return old;
}

template<typename Table, typename It>
static It RemoveXHashTableIterator(Table &table, const It &it) {
    if (!CheckXHashTableIteratorForTable(table, it, "XHashTable.Remove")) {
        return table.End();
    }
    return table.Remove(it);
}

template<typename Table, typename K, typename T>
static bool InsertXHashTableValue(Table &table, const K &key, const T &value, bool overrideValue) {
    return table.Insert(key, value, overrideValue ? 1 : 0) != 0;
}

template<typename Table, typename K, typename T>
static bool LookUpXHashTableValue(const Table &table, const K &key, T &value) {
    return table.LookUp(key, value) != 0;
}

template<typename Table, typename K>
static bool IsXHashTableKeyHere(const Table &table, const K &key) {
    return table.IsHere(key) != 0;
}

template<typename Table>
static int GetXHashTableMemoryOccupationBool(const Table &table, bool addStatic) {
    return table.GetMemoryOccupation(addStatic ? 1 : 0);
}

template<typename C, typename T, typename K>
void RegisterXHashTableEntry(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;
    XString name = decl.Format("%sEntry", className);

    r = engine->RegisterObjectType(name.CStr(), sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>());
    if (r == asALREADY_REGISTERED)
        return;
    CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in, const %s &in)", keyType, elementType);
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const K &k, const T &v, C *self) { new(self) C(k, v); }, (const K &, const T &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in)", name.CStr());
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    decl.Format("%s m_Key", keyType);
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, m_Key)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s m_Data", elementType);
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, m_Data)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &m_Next", name.CStr());
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, m_Next)); CKAS_CHECK_REGISTER(r);
#else
    decl.Format("%s key", keyType);
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, key)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s data", elementType);
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, data)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &next", name.CStr());
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, next)); CKAS_CHECK_REGISTER(r);
#endif

    decl.Format("%s &opAssign(const %s &in)", name.CStr(), name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T, typename K>
void RegisterXNHashTableEntry(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;
    XString name = decl.Format("%sEntry", className);

    r = engine->RegisterObjectType(name.CStr(), sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>());
    if (r == asALREADY_REGISTERED)
        return;
    CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in, const %s &in)", keyType, elementType);
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const K &k, const T &v, C *self) { new(self) C(k, v); }, (const K &, const T &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in)", name.CStr());
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s m_Key", keyType);
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, m_Key)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s m_Data", elementType);
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, m_Data)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &m_Next", name.CStr());
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, m_Next)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", name.CStr(), name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T, typename K>
void RegisterXHashTableIt(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;

    XString name = decl.Format("%sIt", className);
    r = engine->RegisterObjectType(name.CStr(), sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>());
    if (r == asALREADY_REGISTERED)
        return;
    CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](C *self) { new(self) C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in)", name.CStr());
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", name.CStr(), name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool opEquals(const %s &in) const", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((EqualXHashTableIterator<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("const %s &GetKey() const", keyType);
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((GetXHashTableIteratorKey<C, K>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("const %s &GetValue() const", elementType);
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((GetXHashTableIteratorValueConst<C, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &GetValue()", elementType);
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((GetXHashTableIteratorValue<C, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opPreInc()", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((IncrementXHashTableIterator<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s opPostInc(int)", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((PostIncrementXHashTableIterator<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T, typename K>
void RegisterXHashTableConstIt(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;

    XString name = decl.Format("%sConstIt", className);
    r = engine->RegisterObjectType(name.CStr(), sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>());
    if (r == asALREADY_REGISTERED)
        return;
    CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](C *self) { new(self) C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in)", name.CStr());
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", name.CStr(), name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool opEquals(const %s &in) const", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((EqualXHashTableIterator<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("const %s &GetKey() const", keyType);
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((GetXHashTableIteratorKey<C, K>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("const %s &GetValue() const", elementType);
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((GetXHashTableIteratorValueConst<C, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opPreInc()", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((IncrementXHashTableIterator<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s opPostInc(int)", name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asFUNCTION((PostIncrementXHashTableIterator<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T, typename K>
void RegisterXHashTablePair(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;
    XString name = decl.Format("%sPair", className);

    r = engine->RegisterObjectType(name.CStr(), sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>());
    if (r == asALREADY_REGISTERED)
        return;
    CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    using HashTableIt = XHashTableIt<T, K>;
#else
    using HashTableIt = XHashTable<T, K>::Iterator;
#endif
    
    decl.Format("void f(%sIt, int)", className);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](XHashTableIt<T, K> it, int n, C *self) { new(self) C(it, n); }, (XHashTableIt<T, K>, int, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](HashTableIt it, int n, C *self) { new(self) C(it, n); }, (HashTableIt, int, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
#endif
    decl.Format("void f(const %s &in)", name.CStr());
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt m_Iterator", className);
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, m_Iterator)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectProperty(name.CStr(), "int m_New", asOFFSET(C, m_New)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", name.CStr(), name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T, typename K>
void RegisterXNHashTablePair(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;
    XString name = decl.Format("%sPair", className);

    r = engine->RegisterObjectType(name.CStr(), sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>());
    if (r == asALREADY_REGISTERED)
        return;
    CKAS_CHECK_REGISTER(r);

    decl.Format("void f(%sIt, int)", className);
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](XNHashTableIt<T, K> it, int n, C *self) { new(self) C(it, n); }, (XNHashTableIt<T, K>, int, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in)", name.CStr());
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt m_Iterator", className);
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, m_Iterator)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectProperty(name.CStr(), "int m_New", asOFFSET(C, m_New)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", name.CStr(), name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T, typename K>
void RegisterXSHashTablePair(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;
    XString name = decl.Format("%sPair", className);

    r = engine->RegisterObjectType(name.CStr(), sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>());
    if (r == asALREADY_REGISTERED)
        return;
    CKAS_CHECK_REGISTER(r);

    decl.Format("void f(%sIt, int)", className);
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](XSHashTableIt<T, K> it, int n, C *self) { new(self) C(it, n); }, (XSHashTableIt<T, K>, int, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in)", name.CStr());
    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name.CStr(), asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt m_Iterator", className);
    r = engine->RegisterObjectProperty(name.CStr(), decl.CStr(), asOFFSET(C, m_Iterator)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectProperty(name.CStr(), "int m_New", asOFFSET(C, m_New)); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", name.CStr(), name.CStr());
    r = engine->RegisterObjectMethod(name.CStr(), decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T, typename K>
void RegisterXHashTable(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;

#if CKVERSION == 0x13022002
    using HashTableEntry = XHashTableEntry<T, K>;
#else
    using HashTableEntry = XHashTable<T, K>::Entry;
#endif
    RegisterXHashTableEntry<HashTableEntry, T, K>(engine, className, keyType, elementType);

#if CKVERSION == 0x13022002
    using HashTableIt = XHashTableIt<T, K>;
#else
    using HashTableIt = XHashTable<T, K>::Iterator;
#endif
    RegisterXHashTableIt<HashTableIt, T, K>(engine, className, keyType, elementType);

#if CKVERSION == 0x13022002
    using HashTableConstIt = XHashTableConstIt<T, K>;
#else
    using HashTableConstIt = XHashTable<T, K>::ConstIterator;
#endif
    RegisterXHashTableConstIt<HashTableConstIt, T, K>(engine, className, keyType, elementType);

#if CKVERSION == 0x13022002
    using HashTablePair = XHashTablePair<T, K>;
#else
    using HashTablePair = XHashTable<T, K>::Pair;
#endif
    RegisterXHashTablePair<HashTablePair, T, K>(engine, className, keyType, elementType);

    r = engine->RegisterObjectType(className, sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>()); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](C *self) { new(self) C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR([](int init, C *self) { new(self) C(init); }, (int, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in)", className);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opIndex(const %s &)", elementType, keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, operator[], (const K &), T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "void Clear()", asMETHODPR(C, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool Insert(const %s &in, const %s &in, bool override)", keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((InsertXHashTableValue<C, K, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Insert(const %s &in, const %s &in)", className, keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Insert, (const K &, const T &), HashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sPair TestInsert(const %s &in, const %s &in)", className, keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, TestInsert, (const K &, const T &), HashTablePair), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt InsertUnique(const %s &in, const %s &in)", className, keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, InsertUnique, (const K &, const T &), HashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Remove(const %s &in)", keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Remove, (const K &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Remove(const %sIt &in)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((RemoveXHashTableIterator<C, HashTableIt>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Find(const %s &in)", className, keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Find, (const K &), HashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sConstIt Find(const %s &in) const", className, keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Find, (const K &) const, HashTableConstIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool LookUp(const %s &in, %s &out) const", keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((LookUpXHashTableValue<C, K, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("bool IsHere(const %s &in) const", keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((IsXHashTableKeyHere<C, K>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Begin()", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](C &table) -> HashTableIt { return table.Begin(); }, (C &), HashTableIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sConstIt Begin() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &table) -> HashTableConstIt { return table.Begin(); }, (const C &), HashTableConstIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt End()", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](C &table) -> HashTableIt { return table.End(); }, (C &), HashTableIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sConstIt End() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const C &table) -> HashTableConstIt { return table.End(); }, (const C &), HashTableConstIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("int Index(const %s &in) const", keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, Index, (const K &) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "int Size() const", asMETHODPR(C, Size, () const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "int GetMemoryOccupation(bool = false) const", asFUNCTION((GetXHashTableMemoryOccupationBool<C>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(className, "void Reserve(int)", asMETHODPR(C, Reserve, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template<typename C, typename T, typename K>
void RegisterXNHashTable(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;

    using HashTable = XNHashTable<T, K>;

    using HashTableEntry = XNHashTableEntry<T, K>;
    RegisterXNHashTableEntry<HashTableEntry, T, K>(engine, className, keyType, elementType);

    using HashTableIt = typename XNHashTable<T, K>::Iterator;
    RegisterXHashTableIt<HashTableIt, T, K>(engine, className, keyType, elementType);

    using HashTableConstIt = typename XNHashTable<T, K>::ConstIterator;
    RegisterXHashTableConstIt<HashTableConstIt, T, K>(engine, className, keyType, elementType);

    using HashTablePair = typename XNHashTable<T, K>::Pair;
    RegisterXNHashTablePair<HashTablePair, T, K>(engine, className, keyType, elementType);

    r = engine->RegisterObjectType(className, sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>()); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](C *self) { new(self) C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR([](int init, C *self) { new(self) C(init); }, (int, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in)", className);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opIndex(const %s &)", elementType, keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, operator[], (const K &), T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "void Clear()", asMETHODPR(HashTable, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool Insert(const %s &in, const %s &in, bool override)", keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((InsertXHashTableValue<HashTable, K, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Insert(const %s &in, const %s &in)", className, keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Insert, (const K &, const T &), HashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sPair TestInsert(const %s &in, const %s &in)", className, keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, TestInsert, (const K &, const T &), HashTablePair), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt InsertUnique(const %s &in, const %s &in)", className, keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, InsertUnique, (const K &, const T &), HashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Remove(const %s &in)", keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Remove, (const K &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Remove(const %sIt &in)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((RemoveXHashTableIterator<HashTable, HashTableIt>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Find(const %s &in)", className, keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Find, (const K &), HashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sConstIt Find(const %s &in) const", className, keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Find, (const K &) const, HashTableConstIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool LookUp(const %s &in, %s &out) const", keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((LookUpXHashTableValue<HashTable, K, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("bool IsHere(const %s &in) const", keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((IsXHashTableKeyHere<HashTable, K>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Begin()", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](HashTable &table) -> HashTableIt { return table.Begin(); }, (HashTable &), HashTableIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sConstIt Begin() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const HashTable &table) -> HashTableConstIt { return table.Begin(); }, (const HashTable &), HashTableConstIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt End()", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](HashTable &table) -> HashTableIt { return table.End(); }, (HashTable &), HashTableIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sConstIt End() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const HashTable &table) -> HashTableConstIt { return table.End(); }, (const HashTable &), HashTableConstIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("int Index(const %s &in) const", keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Index, (const K &) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "int Size() const", asMETHODPR(HashTable, Size, () const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}


template<typename C, typename T, typename K>
void RegisterXSHashTable(asIScriptEngine *engine, const char *className, const char *keyType, const char *elementType) {
    int r = 0;
    XString decl;

    using HashTable = XSHashTable<T, K>;

    using HashTableEntry = XSHashTableEntry<T, K>;
    RegisterXNHashTableEntry<HashTableEntry, T, K>(engine, className, keyType, elementType);

    using HashTableIt = XSHashTableIt<T, K>;
    RegisterXHashTableIt<HashTableIt, T, K>(engine, className, keyType, elementType);

    using HashTableConstIt = XSHashTableConstIt<T, K>;
    RegisterXHashTableConstIt<HashTableConstIt, T, K>(engine, className, keyType, elementType);

    using HashTablePair = XSHashTablePair<T, K>;
    RegisterXSHashTablePair<HashTablePair, T, K>(engine, className, keyType, elementType);

    r = engine->RegisterObjectType(className, sizeof(C), asOBJ_VALUE | asGetTypeTraits<C>()); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](C *self) { new(self) C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR([](int init, C *self) { new(self) C(init); }, (int, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in)", className);
    r = engine->RegisterObjectBehaviour(className, asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const C &c, C *self) { new(self) C(c); }, (const C &, C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(className, asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](C *self) { self->~C(); }, (C *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(C, operator=, (const C &), C &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opIndex(const %s &)", elementType, keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, operator[], (const K &), T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "void Clear()", asMETHODPR(HashTable, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool Insert(const %s &in, const %s &in, bool override)", keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((InsertXHashTableValue<HashTable, K, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Insert(const %s &in, const %s &in)", className, keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Insert, (const K &, const T &), HashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sPair TestInsert(const %s &in, const %s &in)", className, keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, TestInsert, (const K &, const T &), HashTablePair), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt InsertUnique(const %s &in, const %s &in)", className, keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, InsertUnique, (const K &, const T &), HashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("void Remove(const %s &in)", keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Remove, (const K &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Remove(const %sIt &in)", className, className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((RemoveXHashTableIterator<HashTable, HashTableIt>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Find(const %s &in)", className, keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Find, (const K &), HashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("%sConstIt Find(const %s &in) const", className, keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Find, (const K &) const, HashTableConstIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool LookUp(const %s &in, %s &out) const", keyType, elementType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((LookUpXHashTableValue<HashTable, K, T>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("bool IsHere(const %s &in) const", keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTION((IsXHashTableKeyHere<HashTable, K>)), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt Begin()", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](HashTable &table) -> HashTableIt { return table.Begin(); }, (HashTable &), HashTableIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sConstIt Begin() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const HashTable &table) -> HashTableConstIt { return table.Begin(); }, (const HashTable &), HashTableConstIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sIt End()", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](HashTable &table) -> HashTableIt { return table.End(); }, (HashTable &), HashTableIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("%sConstIt End() const", className);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asFUNCTIONPR([](const HashTable &table) -> HashTableConstIt { return table.End(); }, (const HashTable &), HashTableConstIt), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    decl.Format("int Index(const %s &in) const", keyType);
    r = engine->RegisterObjectMethod(className, decl.CStr(), asMETHODPR(HashTable, Index, (const K &) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(className, "int Size() const", asMETHODPR(HashTable, Size, () const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

#endif // CK_SCRIPTXHASHTABLE_H
