#include "ScriptXObjectArray.h"

#include "XObjectArray.h"

#include "ScriptXArray.h"
#include "ScriptRegistration.h"

namespace {

template <typename T>
bool CKBoolToBool(T value) {
    return value != FALSE;
}

template <typename T>
bool XObjectPointerArrayAddIfNotHere(T *self, CKObject *obj) {
    return CKBoolToBool(self->AddIfNotHere(obj));
}

template <typename T>
bool XObjectPointerArrayFindObject(const T *self, CKObject *obj) {
    return CKBoolToBool(self->FindObject(obj));
}

template <typename T>
bool XObjectPointerArrayCheck(T *self) {
    return CKBoolToBool(self->Check());
}

void XSObjectArrayConvertToObjects(const XSObjectArray *self, CKContext *context, XSObjectPointerArray &objArray) {
    self->ConvertToObjects(context, objArray);
}

void XObjectArrayConvertToObjects(const XObjectArray *self, CKContext *context, XObjectPointerArray &objArray) {
    self->ConvertToObjects(context, objArray);
}

template <typename T>
bool XObjectArrayCheckWithContext(T *self, CKContext *context) {
    return CKBoolToBool(self->Check(context));
}

bool XSObjectArrayCheck(XSObjectArray *self, CKContext *context) {
    return XObjectArrayCheckWithContext(self, context);
}

bool XObjectArrayCheck(XObjectArray *self, CKContext *context) {
    return XObjectArrayCheckWithContext(self, context);
}

template <typename T>
bool XObjectArrayAddIfNotHereId(T *self, CK_ID id) {
    return CKBoolToBool(self->AddIfNotHere(id));
}

template <typename T>
bool XObjectArrayAddIfNotHereObject(T *self, CKObject *obj) {
    return CKBoolToBool(self->AddIfNotHere(obj));
}

template <typename T>
CKObject *XObjectArrayGetObjectWithContext(const T *self, CKContext *context, unsigned int index) {
    return self->GetObject(context, index);
}

CKObject *XSObjectArrayGetObject(const XSObjectArray *self, CKContext *context, unsigned int index) {
    return XObjectArrayGetObjectWithContext(self, context, index);
}

CKObject *XObjectArrayGetObject(const XObjectArray *self, CKContext *context, unsigned int index) {
    return XObjectArrayGetObjectWithContext(self, context, index);
}

template <typename T>
bool XObjectArrayFindObject(const T *self, CKObject *obj) {
    return CKBoolToBool(self->FindObject(obj));
}

template <typename T>
bool XObjectArrayFindID(const T *self, CK_ID id) {
    return CKBoolToBool(self->FindID(id));
}

} // namespace

void RegisterXSObjectPointerArray(asIScriptEngine *engine) {
    int r = 0;

    RegisterXSArray<XSObjectPointerArray, CKObject *>(engine, "XSObjectPointerArray", "CKObject@");

    r = engine->RegisterObjectMethod("XSObjectPointerArray", "bool AddIfNotHere(CKObject@ obj)", asFUNCTIONPR(XObjectPointerArrayAddIfNotHere<XSObjectPointerArray>, (XSObjectPointerArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "CKObject@ GetObject(uint i) const", asMETHODPR(XSObjectPointerArray, GetObject, (unsigned int) const, CKObject *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "int RemoveObject(CKObject@ obj)", asMETHODPR(XSObjectPointerArray, RemoveObject, (CKObject *), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "bool FindObject(CKObject@ obj) const", asFUNCTIONPR(XObjectPointerArrayFindObject<XSObjectPointerArray>, (const XSObjectPointerArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "bool Check()", asFUNCTIONPR(XObjectPointerArrayCheck<XSObjectPointerArray>, (XSObjectPointerArray *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "void Load(CKContext@ context, CKStateChunk@ chunk)", asMETHODPR(XSObjectPointerArray, Load, (CKContext *, CKStateChunk *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "void Save(CKStateChunk@ chunk) const", asMETHODPR(XSObjectPointerArray, Save, (CKStateChunk *) const, void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "void Prepare(CKDependenciesContext &out context) const", asMETHODPR(XSObjectPointerArray, Prepare, (CKDependenciesContext &) const, void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectPointerArray", "void Remap(CKDependenciesContext &out context)", asMETHODPR(XSObjectPointerArray, Remap, (CKDependenciesContext &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

void RegisterXSObjectArray(asIScriptEngine *engine) {
    int r = 0;

    RegisterXSArray<XSObjectArray, CK_ID>(engine, "XSObjectArray", "CK_ID");

    r = engine->RegisterObjectMethod("XSObjectArray", "void ConvertToObjects(CKContext@ context, XSObjectPointerArray &out objArray) const", asFUNCTION(XSObjectArrayConvertToObjects), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "void ConvertFromObjects(const XSObjectPointerArray &in objArray)", asMETHODPR(XSObjectArray, ConvertFromObjects, (const XSArray<CKObject *> &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool Check(CKContext@ context)", asFUNCTION(XSObjectArrayCheck), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool AddIfNotHere(CK_ID id)", asFUNCTIONPR(XObjectArrayAddIfNotHereId<XSObjectArray>, (XSObjectArray *, CK_ID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool AddIfNotHere(CKObject@ obj)", asFUNCTIONPR(XObjectArrayAddIfNotHereObject<XSObjectArray>, (XSObjectArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "CKObject@ GetObject(CKContext@ context, uint i) const", asFUNCTION(XSObjectArrayGetObject), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "CK_ID GetObjectID(uint i) const", asMETHODPR(XSObjectArray, GetObjectID, (unsigned int) const, CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "int RemoveObject(CKObject@ obj)", asMETHODPR(XSObjectArray, RemoveObject, (CKObject *), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool FindObject(CKObject@ obj) const", asFUNCTIONPR(XObjectArrayFindObject<XSObjectArray>, (const XSObjectArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "bool FindID(CK_ID id) const", asFUNCTIONPR(XObjectArrayFindID<XSObjectArray>, (const XSObjectArray *, CK_ID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "void Load(CKStateChunk@ chunk)", asMETHODPR(XSObjectArray, Load, (CKStateChunk *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "void Save(CKStateChunk@ chunk, CKContext@ context) const", asMETHODPR(XSObjectArray, Save, (CKStateChunk *, CKContext *) const, void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "void Prepare(CKDependenciesContext &out context) const", asMETHODPR(XSObjectArray, Prepare, (CKDependenciesContext &) const, void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XSObjectArray", "void Remap(CKDependenciesContext &out context)", asMETHODPR(XSObjectArray, Remap, (CKDependenciesContext &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

void RegisterXObjectPointerArray(asIScriptEngine *engine) {
    int r = 0;

    RegisterXArray<XObjectPointerArray, CKObject *>(engine, "XObjectPointerArray", "CKObject@");

    r = engine->RegisterObjectMethod("XObjectPointerArray", "bool AddIfNotHere(CKObject@ obj)", asFUNCTIONPR(XObjectPointerArrayAddIfNotHere<XObjectPointerArray>, (XObjectPointerArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "CKObject@ GetObject(uint i) const", asMETHODPR(XObjectPointerArray, GetObject, (unsigned int) const, CKObject *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "int RemoveObject(CKObject@ obj)", asMETHODPR(XObjectPointerArray, RemoveObject, (CKObject *), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "bool FindObject(CKObject@ obj) const", asFUNCTIONPR(XObjectPointerArrayFindObject<XObjectPointerArray>, (const XObjectPointerArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "bool Check()", asFUNCTIONPR(XObjectPointerArrayCheck<XObjectPointerArray>, (XObjectPointerArray *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "void Load(CKContext@ context, CKStateChunk@ chunk)", asMETHODPR(XObjectPointerArray, Load, (CKContext *, CKStateChunk *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "void Save(CKStateChunk@ chunk) const", asMETHODPR(XObjectPointerArray, Save, (CKStateChunk *) const, void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "void Prepare(CKDependenciesContext &out context) const", asMETHODPR(XObjectPointerArray, Prepare, (CKDependenciesContext &) const, void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectPointerArray", "void Remap(CKDependenciesContext &out context)", asMETHODPR(XObjectPointerArray, Remap, (CKDependenciesContext &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

void RegisterXObjectArray(asIScriptEngine *engine) {
    int r = 0;

    RegisterXArray<XObjectArray, CK_ID>(engine, "XObjectArray", "CK_ID");

    r = engine->RegisterObjectMethod("XObjectArray", "void ConvertToObjects(CKContext@ context, XObjectPointerArray &out objArray) const", asFUNCTION(XObjectArrayConvertToObjects), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "void ConvertFromObjects(const XObjectPointerArray &in objArray)", asMETHODPR(XObjectArray, ConvertFromObjects, (const XObjectPointerArray &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "bool Check(CKContext@ obj)", asFUNCTION(XObjectArrayCheck), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "bool AddIfNotHere(CK_ID id)", asFUNCTIONPR(XObjectArrayAddIfNotHereId<XObjectArray>, (XObjectArray *, CK_ID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "bool AddIfNotHere(CKObject@ obj)", asFUNCTIONPR(XObjectArrayAddIfNotHereObject<XObjectArray>, (XObjectArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "CKObject@ GetObject(CKContext@ context, uint i) const", asFUNCTION(XObjectArrayGetObject), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "CK_ID GetObjectID(uint i) const", asMETHODPR(XObjectArray, GetObjectID, (unsigned int) const, CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "int RemoveObject(CKObject@ obj)", asMETHODPR(XObjectArray, RemoveObject, (CKObject *), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "bool FindObject(CKObject@ obj) const", asFUNCTIONPR(XObjectArrayFindObject<XObjectArray>, (const XObjectArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "bool FindID(CK_ID id) const", asFUNCTIONPR(XObjectArrayFindID<XObjectArray>, (const XObjectArray *, CK_ID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "void Load(CKStateChunk@ chunk)", asMETHODPR(XObjectArray, Load, (CKStateChunk *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "void Save(CKStateChunk@ chunk, CKContext@ context) const", asMETHODPR(XObjectArray, Save, (CKStateChunk *, CKContext *) const, void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "void Prepare(CKDependenciesContext &out context) const", asMETHODPR(XObjectArray, Prepare, (CKDependenciesContext &) const, void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("XObjectArray", "void Remap(CKDependenciesContext &out context)", asMETHODPR(XObjectArray, Remap, (CKDependenciesContext &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}
