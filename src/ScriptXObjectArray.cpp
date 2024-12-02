#include "ScriptXObjectArray.h"

#include "XObjectArray.h"

#include "ScriptXArray.h"

void RegisterXSObjectPointerArray(asIScriptEngine *engine) {
    int r = 0;

    RegisterXSArray<XSObjectPointerArray, CKObject *>(engine, "XSObjectPointerArray", "CKObject@");

    r = engine->RegisterObjectMethod("XSObjectPointerArray", "bool AddIfNotHere(CKObject@)", asMETHODPR(XSObjectPointerArray, AddIfNotHere, (CKObject *), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "CKObject@ GetObject(uint i) const", asMETHODPR(XSObjectPointerArray, GetObject, (unsigned int) const, CKObject *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "int RemoveObject(CKObject@)", asMETHODPR(XSObjectPointerArray, RemoveObject, (CKObject *), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "bool FindObject(CKObject@) const", asMETHODPR(XSObjectPointerArray, FindObject, (CKObject *) const, CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "bool Check()", asMETHODPR(XSObjectPointerArray, Check, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "void Load(CKContext@, CKStateChunk@)", asMETHODPR(XSObjectPointerArray, Load, (CKContext *, CKStateChunk *), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "void Save(CKStateChunk@) const", asMETHODPR(XSObjectPointerArray, Save, (CKStateChunk *) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "void Prepare(CKDependenciesContext &out) const", asMETHODPR(XSObjectPointerArray, Prepare, (CKDependenciesContext &) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "void Remap(CKDependenciesContext &out)", asMETHODPR(XSObjectPointerArray, Remap, (CKDependenciesContext &), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterXSObjectArray(asIScriptEngine *engine) {
    int r = 0;

    RegisterXSArray<XSObjectArray, CK_ID>(engine, "XSObjectArray", "CK_ID");

    r = engine->RegisterObjectMethod("XSObjectArray", "void ConvertToObjects(CKContext@, XSObjectPointerArray &out) const", asMETHODPR(XSObjectArray, ConvertToObjects, (CKContext *Context, XSArray<CKObject *> &) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "void ConvertFromObjects(const XSObjectPointerArray &in)", asMETHODPR(XSObjectArray, ConvertFromObjects, (const XSArray<CKObject *> &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool Check(CKContext@)", asMETHODPR(XSObjectArray, Check, (CKContext *), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool AddIfNotHere(CK_ID)", asMETHODPR(XSObjectArray, AddIfNotHere, (CK_ID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool AddIfNotHere(CKObject@)", asMETHODPR(XSObjectArray, AddIfNotHere, (CKObject *), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "CKObject@ GetObject(CKContext@, uint i) const", asMETHODPR(XSObjectArray, GetObject, (CKContext *, unsigned int) const, CKObject *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "CK_ID GetObjectID(uint i) const", asMETHODPR(XSObjectArray, GetObjectID, (unsigned int) const, CK_ID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "int RemoveObject(CKObject@)", asMETHODPR(XSObjectArray, RemoveObject, (CKObject *), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool FindObject(CKObject@) const", asMETHODPR(XSObjectArray, FindObject, (CKObject *) const, CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool FindID(CK_ID) const", asMETHODPR(XSObjectArray, FindID, (CK_ID) const, CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "void Load(CKStateChunk@)", asMETHODPR(XSObjectArray, Load, (CKStateChunk *), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "void Save(CKStateChunk@, CKContext@) const", asMETHODPR(XSObjectArray, Save, (CKStateChunk *, CKContext *) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "void Prepare(CKDependenciesContext &out) const", asMETHODPR(XSObjectArray, Prepare, (CKDependenciesContext &) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XSObjectArray", "void Remap(CKDependenciesContext &out)", asMETHODPR(XSObjectArray, Remap, (CKDependenciesContext &), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterXObjectPointerArray(asIScriptEngine *engine) {
    int r = 0;

    RegisterXArray<XObjectPointerArray, CKObject *>(engine, "XObjectPointerArray", "CKObject@");

    r = engine->RegisterObjectMethod("XObjectPointerArray", "bool AddIfNotHere(CKObject@)", asMETHODPR(XObjectPointerArray, AddIfNotHere, (CKObject *), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "CKObject@ GetObject(uint i) const", asMETHODPR(XObjectPointerArray, GetObject, (unsigned int) const, CKObject *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "int RemoveObject(CKObject@)", asMETHODPR(XObjectPointerArray, RemoveObject, (CKObject *), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "bool FindObject(CKObject@) const", asMETHODPR(XObjectPointerArray, FindObject, (CKObject *) const, CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "bool Check()", asMETHODPR(XObjectPointerArray, Check, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "void Load(CKContext@, CKStateChunk@)", asMETHODPR(XObjectPointerArray, Load, (CKContext *, CKStateChunk *), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "void Save(CKStateChunk@) const", asMETHODPR(XObjectPointerArray, Save, (CKStateChunk *) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "void Prepare(CKDependenciesContext &out) const", asMETHODPR(XObjectPointerArray, Prepare, (CKDependenciesContext &) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "void Remap(CKDependenciesContext &out)", asMETHODPR(XObjectPointerArray, Remap, (CKDependenciesContext &), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterXObjectArray(asIScriptEngine *engine) {
    int r = 0;

    RegisterXArray<XObjectArray, CK_ID>(engine, "XObjectArray", "CK_ID");

    r = engine->RegisterObjectMethod("XObjectArray", "void ConvertToObjects(CKContext@, XObjectPointerArray &out) const", asMETHODPR(XObjectArray, ConvertToObjects, (CKContext *Context, XObjectPointerArray &) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "void ConvertFromObjects(const XObjectPointerArray &in)", asMETHODPR(XObjectArray, ConvertFromObjects, (const XObjectPointerArray &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "bool Check(CKContext@)", asMETHODPR(XObjectArray, Check, (CKContext *), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "bool AddIfNotHere(CK_ID)", asMETHODPR(XObjectArray, AddIfNotHere, (CK_ID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "bool AddIfNotHere(CKObject@)", asMETHODPR(XObjectArray, AddIfNotHere, (CKObject *), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "CKObject@ GetObject(CKContext@, uint i) const", asMETHODPR(XObjectArray, GetObject, (CKContext *, unsigned int) const, CKObject *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "CK_ID GetObjectID(uint i) const", asMETHODPR(XObjectArray, GetObjectID, (unsigned int) const, CK_ID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "int RemoveObject(CKObject@)", asMETHODPR(XObjectArray, RemoveObject, (CKObject *), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "bool FindObject(CKObject@) const", asMETHODPR(XObjectArray, FindObject, (CKObject *) const, CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "bool FindID(CK_ID) const", asMETHODPR(XObjectArray, FindID, (CK_ID) const, CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "void Load(CKStateChunk@)", asMETHODPR(XObjectArray, Load, (CKStateChunk *), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "void Save(CKStateChunk@, CKContext@) const", asMETHODPR(XObjectArray, Save, (CKStateChunk *, CKContext *) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "void Prepare(CKDependenciesContext &out) const", asMETHODPR(XObjectArray, Prepare, (CKDependenciesContext &) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("XObjectArray", "void Remap(CKDependenciesContext &out)", asMETHODPR(XObjectArray, Remap, (CKDependenciesContext &), void), asCALL_THISCALL); assert(r >= 0);
}
