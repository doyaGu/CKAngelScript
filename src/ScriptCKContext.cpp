#include "ScriptCKContext.h"

#include <cassert>
#include <limits.h>
#include <string>

#include "angelscript.h"

#include "CKContext.h"

#include "ScriptUtils.h"
#include "ScriptNativeBuffer.h"
#include "ScriptNativePointer.h"
#include "ScriptRegistration.h"
#include "ScriptCKVertexBuffer.h"
#include "ScriptManager.h"

static void SetCKContextException(const char *message) {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
}

static bool ValidateCKContextMemoryBuffer(CKContext *self, int size, NativePointer buffer, const char *method) {
    if (!self) {
        SetCKContextException("CKContext method called with a null context.");
        return false;
    }
    if (size < 0) {
        SetCKContextException((std::string(method) + " requires a non-negative buffer size.").c_str());
        return false;
    }
    if (size > 0 && buffer.IsNull()) {
        SetCKContextException((std::string(method) + " requires a non-null buffer when size is positive.").c_str());
        return false;
    }
    return true;
}

static bool RejectCKContextMutation(const char *method) {
    return ScriptManager::RejectActiveHostMutation(method);
}

static bool ValidateCKContextNativeBuffer(CKContext *self, NativeBuffer *buffer, const char *method, int &size) {
    size = 0;
    if (!self) {
        SetCKContextException("CKContext method called with a null context.");
        return false;
    }
    if (!buffer) {
        SetCKContextException((std::string(method) + " requires a NativeBuffer.").c_str());
        return false;
    }
    if (buffer->Size() > static_cast<size_t>(INT_MAX)) {
        SetCKContextException((std::string(method) + " NativeBuffer is too large for the Virtools memory-load API.").c_str());
        return false;
    }
    if (buffer->Size() > 0 && !buffer->Data()) {
        SetCKContextException((std::string(method) + " requires a valid NativeBuffer data pointer.").c_str());
        return false;
    }
    size = static_cast<int>(buffer->Size());
    return true;
}

static CKERROR ClearCKContextAll(CKContext *self) {
    if (!self) {
        return CKERR_INVALIDPARAMETER;
    }
    if (RejectCKContextMutation("CKContext::ClearAll")) {
        return CKERR_INVALIDOPERATION;
    }
    InvalidateScriptCKVertexBuffersForManager(self->GetRenderManager());
    return self->ClearAll();
}

static CKERROR LoadCKContextFromBuffer(CKContext *self,
                                       int size,
                                       NativePointer buffer,
                                       CKObjectArray *objects,
                                       CK_LOAD_FLAGS loadFlags) {
    if (RejectCKContextMutation("CKContext::Load")) {
        return CKERR_INVALIDOPERATION;
    }
    if (!ValidateCKContextMemoryBuffer(self, size, buffer, "CKContext.Load")) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->Load(size, buffer.Get(), objects, loadFlags);
}

static CKERROR LoadCKContextFromNativeBuffer(CKContext *self,
                                             NativeBuffer *buffer,
                                             CKObjectArray *objects,
                                             CK_LOAD_FLAGS loadFlags) {
    if (RejectCKContextMutation("CKContext::Load")) {
        return CKERR_INVALIDOPERATION;
    }
    int size = 0;
    if (!ValidateCKContextNativeBuffer(self, buffer, "CKContext.Load", size)) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->Load(size, size > 0 ? buffer->Data() : nullptr, objects, loadFlags);
}

static CKERROR GetCKContextFileInfoFromBuffer(CKContext *self,
                                              int size,
                                              NativePointer buffer,
                                              CKFileInfo *fileInfo) {
    if (!ValidateCKContextMemoryBuffer(self, size, buffer, "CKContext.GetFileInfo")) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->GetFileInfo(size, buffer.Get(), fileInfo);
}

static CKERROR GetCKContextFileInfoFromNativeBuffer(CKContext *self,
                                                   NativeBuffer *buffer,
                                                   CKFileInfo *fileInfo) {
    int size = 0;
    if (!ValidateCKContextNativeBuffer(self, buffer, "CKContext.GetFileInfo", size)) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->GetFileInfo(size, size > 0 ? buffer->Data() : nullptr, fileInfo);
}

static CKERROR LoadCKContextAnimationOnCharacterFromBuffer(CKContext *self,
                                                          int size,
                                                          NativePointer buffer,
                                                          CKObjectArray *objects,
                                                          CKCharacter *character,
                                                          bool asDynamicObjects) {
    if (RejectCKContextMutation("CKContext::LoadAnimationOnCharacter")) {
        return CKERR_INVALIDOPERATION;
    }
    if (!ValidateCKContextMemoryBuffer(self, size, buffer, "CKContext.LoadAnimationOnCharacter")) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->LoadAnimationOnCharacter(size, buffer.Get(), objects, character, asDynamicObjects);
}

static CKERROR LoadCKContextAnimationOnCharacterFromNativeBuffer(CKContext *self,
                                                                NativeBuffer *buffer,
                                                                CKObjectArray *objects,
                                                                CKCharacter *character,
                                                                bool asDynamicObjects) {
    if (RejectCKContextMutation("CKContext::LoadAnimationOnCharacter")) {
        return CKERR_INVALIDOPERATION;
    }
    int size = 0;
    if (!ValidateCKContextNativeBuffer(self, buffer, "CKContext.LoadAnimationOnCharacter", size)) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->LoadAnimationOnCharacter(size, size > 0 ? buffer->Data() : nullptr, objects, character, asDynamicObjects);
}

static CKParameterIn *CreateCKContextParameterIn(CKContext *self,
                                                const std::string &name,
                                                CKParameterType type,
                                                bool dynamic) {
    return RejectCKContextMutation("CKContext::CreateCKParameterIn")
               ? nullptr
               : self->CreateCKParameterIn(const_cast<CKSTRING>(name.c_str()), type, dynamic);
}

static CKParameterIn *CreateCKContextParameterInGuid(CKContext *self,
                                                    const std::string &name,
                                                    CKGUID guid,
                                                    bool dynamic) {
    return RejectCKContextMutation("CKContext::CreateCKParameterIn")
               ? nullptr
               : self->CreateCKParameterIn(const_cast<CKSTRING>(name.c_str()), guid, dynamic);
}

static CKParameterIn *CreateCKContextParameterInName(CKContext *self,
                                                    const std::string &name,
                                                    const std::string &typeName,
                                                    bool dynamic) {
    return RejectCKContextMutation("CKContext::CreateCKParameterIn")
               ? nullptr
               : self->CreateCKParameterIn(const_cast<CKSTRING>(name.c_str()),
                                           const_cast<CKSTRING>(typeName.c_str()),
                                           dynamic);
}

static CKParameterOut *CreateCKContextParameterOut(CKContext *self,
                                                  const std::string &name,
                                                  CKParameterType type,
                                                  bool dynamic) {
    return RejectCKContextMutation("CKContext::CreateCKParameterOut")
               ? nullptr
               : self->CreateCKParameterOut(const_cast<CKSTRING>(name.c_str()), type, dynamic);
}

static CKParameterOut *CreateCKContextParameterOutGuid(CKContext *self,
                                                      const std::string &name,
                                                      CKGUID guid,
                                                      bool dynamic) {
    return RejectCKContextMutation("CKContext::CreateCKParameterOut")
               ? nullptr
               : self->CreateCKParameterOut(const_cast<CKSTRING>(name.c_str()), guid, dynamic);
}

static CKParameterOut *CreateCKContextParameterOutName(CKContext *self,
                                                      const std::string &name,
                                                      const std::string &typeName,
                                                      bool dynamic) {
    return RejectCKContextMutation("CKContext::CreateCKParameterOut")
               ? nullptr
               : self->CreateCKParameterOut(const_cast<CKSTRING>(name.c_str()),
                                            const_cast<CKSTRING>(typeName.c_str()),
                                            dynamic);
}

static CKParameterLocal *CreateCKContextParameterLocal(CKContext *self,
                                                      const std::string &name,
                                                      CKParameterType type,
                                                      bool dynamic) {
    return RejectCKContextMutation("CKContext::CreateCKParameterLocal")
               ? nullptr
               : self->CreateCKParameterLocal(const_cast<CKSTRING>(name.c_str()), type, dynamic);
}

static CKParameterLocal *CreateCKContextParameterLocalGuid(CKContext *self,
                                                          const std::string &name,
                                                          CKGUID guid,
                                                          bool dynamic) {
    return RejectCKContextMutation("CKContext::CreateCKParameterLocal")
               ? nullptr
               : self->CreateCKParameterLocal(const_cast<CKSTRING>(name.c_str()), guid, dynamic);
}

static CKParameterLocal *CreateCKContextParameterLocalName(CKContext *self,
                                                          const std::string &name,
                                                          const std::string &typeName,
                                                          bool dynamic) {
    return RejectCKContextMutation("CKContext::CreateCKParameterLocal")
               ? nullptr
               : self->CreateCKParameterLocal(const_cast<CKSTRING>(name.c_str()),
                                              const_cast<CKSTRING>(typeName.c_str()),
                                            dynamic);
}

static CKParameterOperation *CreateCKContextParameterOperation(CKContext *self,
                                                              const std::string &name,
                                                              CKGUID operationGuid,
                                                              CKGUID resultGuid,
                                                              CKGUID firstGuid,
                                                              CKGUID secondGuid) {
    return RejectCKContextMutation("CKContext::CreateCKParameterOperation")
               ? nullptr
               : self->CreateCKParameterOperation(const_cast<CKSTRING>(name.c_str()),
                                                  operationGuid,
                                                  resultGuid,
                                                  firstGuid,
                                                  secondGuid);
}

static CKERROR DestroyCKContextObject(CKContext *self,
                                      CKObject *object,
                                      CKDWORD flags,
                                      CKDependencies *dependencies) {
    return RejectCKContextMutation("CKContext::DestroyObject")
               ? CKERR_INVALIDOPERATION
               : self->DestroyObject(object, flags, dependencies);
}

static CKERROR DestroyCKContextObjectDefault(CKContext *self, CKObject *object) {
    return DestroyCKContextObject(self, object, 0, nullptr);
}

static CKERROR DestroyCKContextObjectWithFlags(CKContext *self,
                                               CKObject *object,
                                               CKDWORD flags) {
    return DestroyCKContextObject(self, object, flags, nullptr);
}

static CKERROR DestroyCKContextObjectById(CKContext *self,
                                          CK_ID id,
                                          CKDWORD flags,
                                          CKDependencies *dependencies) {
    return RejectCKContextMutation("CKContext::DestroyObject")
               ? CKERR_INVALIDOPERATION
               : self->DestroyObject(id, flags, dependencies);
}

static CKERROR DestroyCKContextObjectByIdDefault(CKContext *self, CK_ID id) {
    return DestroyCKContextObjectById(self, id, 0, nullptr);
}

static CKERROR DestroyCKContextObjectByIdWithFlags(CKContext *self,
                                                   CK_ID id,
                                                   CKDWORD flags) {
    return DestroyCKContextObjectById(self, id, flags, nullptr);
}

static CKERROR DestroyCKContextObjects(CKContext *self,
                                       const XObjectArray &objects,
                                       CKDWORD flags,
                                       CKDependencies *dependencies) {
    return RejectCKContextMutation("CKContext::DestroyObjects")
               ? CKERR_INVALIDOPERATION
               : self->DestroyObjects(objects.Begin(), objects.Size(), flags, dependencies);
}

static CKERROR DestroyCKContextObjectsDefault(CKContext *self,
                                              const XObjectArray &objects) {
    return DestroyCKContextObjects(self, objects, 0, nullptr);
}

static CKERROR DestroyCKContextObjectsWithFlags(CKContext *self,
                                                const XObjectArray &objects,
                                                CKDWORD flags) {
    return DestroyCKContextObjects(self, objects, flags, nullptr);
}

void RegisterCKContext(asIScriptEngine *engine) {
    int r = 0;

    // Objects Management
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ CreateObject(CK_CLASSID cid, const string &in name = \"\", CK_OBJECTCREATION_OPTIONS options = CK_OBJECTCREATION_NONAMECHECK, CK_LOADMODE &out res = void)", asFUNCTIONPR([](CKContext *self, CK_CLASSID cid, const std::string &name, CK_OBJECTCREATION_OPTIONS options, CK_CREATIONMODE &res) -> CKObject * { if (RejectCKContextMutation("CKContext::CreateObject")) return nullptr; return self->CreateObject(cid, name.empty() ? nullptr : const_cast<CKSTRING>(name.c_str()), options, &res); }, (CKContext *, CK_CLASSID, const std::string &, CK_OBJECTCREATION_OPTIONS, CK_CREATIONMODE &), CKObject*), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ CopyObject(CKObject@ src, CKDependencies &in dependencies = void, const string &in appendName = void, CK_OBJECTCREATION_OPTIONS options = CK_OBJECTCREATION_NONAMECHECK)", asFUNCTIONPR([](CKContext *self, CKObject *src, CKDependencies *dependencies, const std::string &appendName, CK_OBJECTCREATION_OPTIONS options) -> CKObject * { if (RejectCKContextMutation("CKContext::CopyObject")) return nullptr; return self->CopyObject(src, dependencies, appendName.empty() ? nullptr : const_cast<CKSTRING>(appendName.c_str()), options); }, (CKContext *, CKObject *, CKDependencies *, const std::string &, CK_OBJECTCREATION_OPTIONS), CKObject*), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "const XObjectArray &CopyObjects(const XObjectArray &in srcObjects, CKDependencies &in dependencies = void, CK_OBJECTCREATION_OPTIONS options = CK_OBJECTCREATION_NONAMECHECK, const string &in appendName = void)", asFUNCTIONPR([](CKContext *self, const XObjectArray &srcObjects, CKDependencies *dependencies, CK_OBJECTCREATION_OPTIONS options, const std::string &appendName) -> const XObjectArray & { if (RejectCKContextMutation("CKContext::CopyObjects")) { static XObjectArray empty; return empty; } return self->CopyObjects(srcObjects, dependencies, options, appendName.empty() ? nullptr : const_cast<CKSTRING>(appendName.c_str())); }, (CKContext *, const XObjectArray &, CKDependencies *, CK_OBJECTCREATION_OPTIONS, const std::string &), const XObjectArray &), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "CKObject@ GetObject(CK_ID id)", asMETHODPR(CKContext, GetObject, (CK_ID), CKObject*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "int GetObjectCount()", asMETHODPR(CKContext, GetObjectCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "int GetObjectSize(CKObject@ obj)", asMETHODPR(CKContext, GetObjectSize, (CKObject*), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CKObject@ obj)", asFUNCTION(DestroyCKContextObjectDefault), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CKObject@ obj, CKDWORD flags)", asFUNCTION(DestroyCKContextObjectWithFlags), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CKObject@ obj, CKDWORD flags, CKDependencies &in depoptions)", asFUNCTION(DestroyCKContextObject), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CK_ID id)", asFUNCTION(DestroyCKContextObjectByIdDefault), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CK_ID id, CKDWORD flags)", asFUNCTION(DestroyCKContextObjectByIdWithFlags), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CK_ID id, CKDWORD flags, CKDependencies &in depoptions)", asFUNCTION(DestroyCKContextObjectById), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObjects(const XObjectArray &in ids)", asFUNCTION(DestroyCKContextObjectsDefault), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObjects(const XObjectArray &in ids, CKDWORD flags)", asFUNCTION(DestroyCKContextObjectsWithFlags), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObjects(const XObjectArray &in ids, CKDWORD flags, CKDependencies &in depoptions)", asFUNCTION(DestroyCKContextObjects), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKContext", "void DestroyAllDynamicObjects()", asFUNCTIONPR([](CKContext *self) { if (RejectCKContextMutation("CKContext::DestroyAllDynamicObjects")) return; self->DestroyAllDynamicObjects(); }, (CKContext *), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKContext", "void DestroyAllDynamicObjects(CKScene@ scene = null)", asFUNCTIONPR([](CKContext *self, CKScene *scene) { if (RejectCKContextMutation("CKContext::DestroyAllDynamicObjects")) return; self->DestroyAllDynamicObjects(scene); }, (CKContext *, CKScene *), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKContext", "void ChangeObjectDynamic(CKObject@ obj, bool setDynamic = true)", asFUNCTIONPR([](CKContext *self, CKObject *obj, bool setDynamic) { if (RejectCKContextMutation("CKContext::ChangeObjectDynamic")) return; self->ChangeObjectDynamic(obj, setDynamic); }, (CKContext *, CKObject *, bool), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "const XObjectPointerArray &CKFillObjectsUnused()", asMETHODPR(CKContext, CKFillObjectsUnused, (), const XObjectPointerArray &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    // Object Access
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ GetObjectByName(const string &in name, CKObject@ previous = null)", asFUNCTIONPR([](CKContext *self, const std::string &name, CKObject *previous) { return self->GetObjectByName(const_cast<CKSTRING>(name.c_str()), previous); }, (CKContext *, const std::string &, CKObject *), CKObject*), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ GetObjectByNameAndClass(const string &in name, CK_CLASSID cid, CKObject@ previous = null)", asFUNCTIONPR([](CKContext *self, const std::string &name, CK_CLASSID cid, CKObject *previous) { return self->GetObjectByNameAndClass(const_cast<CKSTRING>(name.c_str()), cid, previous); }, (CKContext *, const std::string &, CK_CLASSID, CKObject *), CKObject*), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ GetObjectByNameAndParentClass(const string &in name, CK_CLASSID pcid, CKObject@ previous)", asFUNCTIONPR([](CKContext *self, const std::string &name, CK_CLASSID pcid, CKObject *previous) { return self->GetObjectByNameAndParentClass(const_cast<CKSTRING>(name.c_str()), pcid, previous); }, (CKContext *, const std::string &, CK_CLASSID, CKObject *), CKObject*), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "const XObjectPointerArray &GetObjectListByType(CK_CLASSID cid, bool derived)", asFUNCTIONPR([](CKContext *self, CK_CLASSID cid, bool derived) -> const XObjectPointerArray & { return self->GetObjectListByType(cid, derived); }, (CKContext *, CK_CLASSID, bool), const XObjectPointerArray &), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "int GetObjectsCountByClassID(CK_CLASSID cid)", asMETHODPR(CKContext, GetObjectsCountByClassID, (CK_CLASSID), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKContext", "uintptr_t GetObjectsListByClassID(int)", asMETHODPR(CKContext, GetObjectsListByClassID, (CK_CLASSID), CK_ID*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    // Engine runtime
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Play()", asFUNCTIONPR([](CKContext *self) -> CKERROR { if (RejectCKContextMutation("CKContext::Play")) return CKERR_INVALIDOPERATION; return self->Play(); }, (CKContext *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Pause()", asFUNCTIONPR([](CKContext *self) -> CKERROR { if (RejectCKContextMutation("CKContext::Pause")) return CKERR_INVALIDOPERATION; return self->Pause(); }, (CKContext *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Reset()", asFUNCTIONPR([](CKContext *self) -> CKERROR { if (RejectCKContextMutation("CKContext::Reset")) return CKERR_INVALIDOPERATION; return self->Reset(); }, (CKContext *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Reset(bool toLevelScene = false)", asFUNCTIONPR([](CKContext *self, bool toLevelScene) -> CKERROR { if (RejectCKContextMutation("CKContext::Reset")) return CKERR_INVALIDOPERATION; return self->Reset(toLevelScene); }, (CKContext *, bool), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKContext", "bool IsPlaying()", asFUNCTIONPR([](CKContext *self) -> bool { return self->IsPlaying(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "bool IsReseted()", asFUNCTIONPR([](CKContext *self) -> bool { return self->IsReseted(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Process()", asFUNCTIONPR([](CKContext *self) -> CKERROR { if (RejectCKContextMutation("CKContext::Process")) return CKERR_INVALIDOPERATION; return self->Process(); }, (CKContext *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR ClearAll()", asFUNCTION(ClearCKContextAll), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "bool IsInClearAll()", asFUNCTIONPR([](CKContext *self) -> bool { return self->IsInClearAll(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // Current Level & Scene functions
    r = engine->RegisterObjectMethod("CKContext", "CKLevel@ GetCurrentLevel()", asMETHODPR(CKContext, GetCurrentLevel, (), CKLevel*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKRenderContext@ GetPlayerRenderContext()", asMETHODPR(CKContext, GetPlayerRenderContext, (), CKRenderContext*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKScene@ GetCurrentScene()", asMETHODPR(CKContext, GetCurrentScene, (), CKScene*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void SetCurrentLevel(CKLevel@ level)", asFUNCTIONPR([](CKContext *self, CKLevel *level) { if (RejectCKContextMutation("CKContext::SetCurrentLevel")) return; self->SetCurrentLevel(level); }, (CKContext *, CKLevel *), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // Object Management functions
    r = engine->RegisterObjectMethod("CKContext", "CKParameterIn@ CreateCKParameterIn(const string &in name, CKParameterType type, bool dynamic = false)", asFUNCTION(CreateCKContextParameterIn), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterIn@ CreateCKParameterIn(const string &in name, CKGUID guid, bool dynamic = false)", asFUNCTION(CreateCKContextParameterInGuid), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterIn@ CreateCKParameterIn(const string &in name, const string &in typeName, bool dynamic = false)", asFUNCTION(CreateCKContextParameterInName), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterOut@ CreateCKParameterOut(const string &in name, CKParameterType type, bool dynamic = false)", asFUNCTION(CreateCKContextParameterOut), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterOut@ CreateCKParameterOut(const string &in name, CKGUID guid, bool dynamic = false)", asFUNCTION(CreateCKContextParameterOutGuid), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterOut@ CreateCKParameterOut(const string &in name, const string &in typeName, bool dynamic = false)", asFUNCTION(CreateCKContextParameterOutName), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterLocal@ CreateCKParameterLocal(const string &in name, CKParameterType type, bool dynamic = false)", asFUNCTION(CreateCKContextParameterLocal), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterLocal@ CreateCKParameterLocal(const string &in name, CKGUID guid, bool dynamic = false)", asFUNCTION(CreateCKContextParameterLocalGuid), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterLocal@ CreateCKParameterLocal(const string &in name, const string &in typeName, bool dynamic = false)", asFUNCTION(CreateCKContextParameterLocalName), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterOperation@ CreateCKParameterOperation(const string &in name, CKGUID opguid, CKGUID resGuid, CKGUID p1Guid, CKGUID p2Guid)", asFUNCTION(CreateCKContextParameterOperation), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "CKFile@ CreateCKFile()", asFUNCTIONPR([](CKContext *self) -> CKFile * { if (RejectCKContextMutation("CKContext::CreateCKFile")) return nullptr; return self->CreateCKFile(); }, (CKContext *), CKFile *), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DeleteCKFile(CKFile@ file)", asFUNCTIONPR([](CKContext *self, CKFile *file) -> CKERROR { if (RejectCKContextMutation("CKContext::DeleteCKFile")) return CKERR_INVALIDOPERATION; return self->DeleteCKFile(file); }, (CKContext *, CKFile *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // IHM
    // r = engine->RegisterObjectMethod("CKContext", "void SetInterfaceMode(bool mode = true, uintptr_t = 0, uintptr_t = 0)", asMETHODPR(CKContext, SetInterfaceMode, (CKBOOL, CKUICALLBACKFCT, void *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "bool IsInInterfaceMode()", asFUNCTIONPR([](CKContext *self) -> bool { return self->IsInInterfaceMode(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "CKERROR OutputToConsole(const string &in str, bool beep = true)", asFUNCTIONPR([](CKContext *self, const std::string &str, bool beep) { return self->OutputToConsole(const_cast<CKSTRING>(str.c_str()), beep); }, (CKContext *, const std::string &, bool), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR OutputToInfo(const string &in format)", asFUNCTIONPR([](CKContext *self, const std::string &info) { return self->OutputToInfo(const_cast<CKSTRING>(info.c_str())); }, (CKContext *, const std::string &), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR RefreshBuildingBlocks(const XGUIDArray &in guids)", asFUNCTIONPR([](CKContext *self, const XArray<CKGUID> &guids) -> CKERROR { if (RejectCKContextMutation("CKContext::RefreshBuildingBlocks")) return CKERR_INVALIDOPERATION; return self->RefreshBuildingBlocks(guids); }, (CKContext *, const XArray<CKGUID> &), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "CKERROR ShowSetup(int)", asFUNCTIONPR([](CKContext *self, CK_ID id) -> CKERROR { if (RejectCKContextMutation("CKContext::ShowSetup")) return CKERR_INVALIDOPERATION; return self->ShowSetup(id); }, (CKContext *, CK_ID), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKContext", "int ChooseObject(uintptr_t dialogParentWnd)", asMETHODPR(CKContext, ChooseObject, (void *), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Select(const XObjectArray &in objects, bool clearSelection = true)", asFUNCTIONPR([](CKContext *self, const XObjectArray &objects, bool clearSelection) -> CKERROR { if (RejectCKContextMutation("CKContext::Select")) return CKERR_INVALIDOPERATION; return self->Select(objects, clearSelection); }, (CKContext *, const XObjectArray &, bool), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#if CKVERSION != 0x26052005
    r = engine->RegisterObjectMethod("CKContext", "CKDWORD SendInterfaceMessage(CKDWORD reason, CKDWORD param1, CKDWORD param2)", asFUNCTIONPR([](CKContext *self, CKDWORD reason, CKDWORD param1, CKDWORD param2) -> CKDWORD { if (RejectCKContextMutation("CKContext::SendInterfaceMessage")) return 0; return self->SendInterfaceMessage(reason, param1, param2); }, (CKContext *, CKDWORD, CKDWORD, CKDWORD), CKDWORD), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKContext", "CKDWORD SendInterfaceMessage(CKDWORD reason, CKDWORD param1, CKDWORD param2, CKDWORD param3 = 0)", asFUNCTIONPR([](CKContext *self, CKDWORD reason, CKDWORD param1, CKDWORD param2, CKDWORD param3) -> CKDWORD { if (RejectCKContextMutation("CKContext::SendInterfaceMessage")) return 0; return self->SendInterfaceMessage(reason, param1, param2, param3); }, (CKContext *, CKDWORD, CKDWORD, CKDWORD, CKDWORD), CKDWORD), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKContext", "CKERROR UICopyObjects(const XObjectArray &in, bool iClearClipboard = true)", asFUNCTIONPR([](CKContext *self, const XObjectArray &objects, bool clearClipboard) -> CKERROR { if (RejectCKContextMutation("CKContext::UICopyObjects")) return CKERR_INVALIDOPERATION; return self->UICopyObjects(objects, clearClipboard); }, (CKContext *, const XObjectArray &, bool), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR UIPasteObjects(const XObjectArray &in)", asFUNCTIONPR([](CKContext *self, const XObjectArray &objects) -> CKERROR { if (RejectCKContextMutation("CKContext::UIPasteObjects")) return CKERR_INVALIDOPERATION; return self->UIPasteObjects(objects); }, (CKContext *, const XObjectArray &), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // Managers
    r = engine->RegisterObjectMethod("CKContext", "CKRenderManager@ GetRenderManager()", asMETHODPR(CKContext, GetRenderManager, (), CKRenderManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKBehaviorManager@ GetBehaviorManager()", asMETHODPR(CKContext, GetBehaviorManager, (), CKBehaviorManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterManager@ GetParameterManager()", asMETHODPR(CKContext, GetParameterManager, (), CKParameterManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKMessageManager@ GetMessageManager()", asMETHODPR(CKContext, GetMessageManager, (), CKMessageManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKTimeManager@ GetTimeManager()", asMETHODPR(CKContext, GetTimeManager, (), CKTimeManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKAttributeManager@ GetAttributeManager()", asMETHODPR(CKContext, GetAttributeManager, (), CKAttributeManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKPathManager@ GetPathManager()", asMETHODPR(CKContext, GetPathManager, (), CKPathManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "XManagerHashTableIt GetManagers()", asMETHODPR(CKContext, GetManagers, (), XManagerHashTableIt), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKBaseManager@ GetManagerByGuid(const CKGUID &in guid)", asMETHODPR(CKContext, GetManagerByGuid, (CKGUID), CKBaseManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKBaseManager@ GetManagerByName(const string &in name)", asFUNCTIONPR([](CKContext *self, const std::string &name) { return self->GetManagerByName(const_cast<CKSTRING>(name.c_str())); }, (CKContext *, const std::string &), CKBaseManager*), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "int GetManagerCount()", asMETHODPR(CKContext, GetManagerCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKBaseManager@ GetManager(int index)", asMETHODPR(CKContext, GetManager, (int), CKBaseManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "bool IsManagerActive(CKBaseManager@ manager)", asFUNCTIONPR([](CKContext *self, CKBaseManager *manager) -> bool { return self->IsManagerActive(manager); }, (CKContext *, CKBaseManager *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "bool HasManagerDuplicates(CKBaseManager@ manager)", asFUNCTIONPR([](CKContext *self, CKBaseManager *manager) -> bool { return self->HasManagerDuplicates(manager); }, (CKContext *, CKBaseManager *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void ActivateManager(CKBaseManager@ manager, bool active)", asFUNCTIONPR([](CKContext *self, CKBaseManager *manager, bool active) { if (RejectCKContextMutation("CKContext::ActivateManager")) return; self->ActivateManager(manager, active); }, (CKContext *, CKBaseManager *, bool), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "int GetInactiveManagerCount()", asMETHODPR(CKContext, GetInactiveManagerCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKBaseManager@ GetInactiveManager(int index)", asMETHODPR(CKContext, GetInactiveManager, (int), CKBaseManager*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "CKERROR RegisterNewManager(CKBaseManager@ manager)", asFUNCTIONPR([](CKContext *self, CKBaseManager *manager) -> CKERROR { if (RejectCKContextMutation("CKContext::RegisterNewManager")) return CKERR_INVALIDOPERATION; return self->RegisterNewManager(manager); }, (CKContext *, CKBaseManager *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // Profiling Functions
    r = engine->RegisterObjectMethod("CKContext", "void EnableProfiling(bool enable)", asFUNCTIONPR([](CKContext *self, bool enable) { if (RejectCKContextMutation("CKContext::EnableProfiling")) return; self->EnableProfiling(enable); }, (CKContext *, bool), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "bool IsProfilingEnable()", asFUNCTIONPR([](CKContext *self) -> bool { return self->IsProfilingEnable(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void GetProfileStats(CKStats &out stats)", asMETHODPR(CKContext, GetProfileStats, (CKStats*), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void UserProfileStart(CKDWORD slot)", asFUNCTIONPR([](CKContext *self, CKDWORD slot) { if (RejectCKContextMutation("CKContext::UserProfileStart")) return; self->UserProfileStart(slot); }, (CKContext *, CKDWORD), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "float UserProfileEnd(CKDWORD slot)", asFUNCTIONPR([](CKContext *self, CKDWORD slot) -> float { if (RejectCKContextMutation("CKContext::UserProfileEnd")) return 0.0f; return self->UserProfileEnd(slot); }, (CKContext *, CKDWORD), float), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "float GetLastUserProfileTime(CKDWORD slot)", asMETHODPR(CKContext, GetLastUserProfileTime, (CKDWORD), float), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    // Utilities
    r = engine->RegisterObjectMethod("CKContext", "CKGUID GetSecureGuid()", asMETHODPR(CKContext, GetSecureGuid, (), CKGUID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKDWORD GetStartOptions()", asMETHODPR(CKContext, GetStartOptions, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "WIN_HANDLE GetMainWindow()", asMETHODPR(CKContext, GetMainWindow, (), WIN_HANDLE), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "int GetSelectedRenderEngine()", asMETHODPR(CKContext, GetSelectedRenderEngine, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    // File Save/Load Options
    r = engine->RegisterObjectMethod("CKContext", "void SetCompressionLevel(int level)", asFUNCTIONPR([](CKContext *self, int level) { if (RejectCKContextMutation("CKContext::SetCompressionLevel")) return; self->SetCompressionLevel(level); }, (CKContext *, int), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "int GetCompressionLevel()", asMETHODPR(CKContext, GetCompressionLevel, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void SetFileWriteMode(CK_FILE_WRITEMODE mode)", asFUNCTIONPR([](CKContext *self, CK_FILE_WRITEMODE mode) { if (RejectCKContextMutation("CKContext::SetFileWriteMode")) return; self->SetFileWriteMode(mode); }, (CKContext *, CK_FILE_WRITEMODE), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CK_FILE_WRITEMODE GetFileWriteMode()", asMETHODPR(CKContext, GetFileWriteMode, (), CK_FILE_WRITEMODE), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CK_TEXTURE_SAVEOPTIONS GetGlobalImagesSaveOptions()", asMETHODPR(CKContext, GetGlobalImagesSaveOptions, (), CK_TEXTURE_SAVEOPTIONS), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void SetGlobalImagesSaveOptions(CK_TEXTURE_SAVEOPTIONS options)", asFUNCTIONPR([](CKContext *self, CK_TEXTURE_SAVEOPTIONS options) { if (RejectCKContextMutation("CKContext::SetGlobalImagesSaveOptions")) return; self->SetGlobalImagesSaveOptions(options); }, (CKContext *, CK_TEXTURE_SAVEOPTIONS), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKBitmapProperties@ GetGlobalImagesSaveFormat()", asMETHODPR(CKContext, GetGlobalImagesSaveFormat, (), CKBitmapProperties *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void SetGlobalImagesSaveFormat(CKBitmapProperties@ format)", asFUNCTIONPR([](CKContext *self, CKBitmapProperties *format) { if (RejectCKContextMutation("CKContext::SetGlobalImagesSaveFormat")) return; self->SetGlobalImagesSaveFormat(format); }, (CKContext *, CKBitmapProperties *), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CK_SOUND_SAVEOPTIONS GetGlobalSoundsSaveOptions()", asMETHODPR(CKContext, GetGlobalSoundsSaveOptions, (), CK_SOUND_SAVEOPTIONS), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void SetGlobalSoundsSaveOptions(CK_SOUND_SAVEOPTIONS options)", asFUNCTIONPR([](CKContext *self, CK_SOUND_SAVEOPTIONS options) { if (RejectCKContextMutation("CKContext::SetGlobalSoundsSaveOptions")) return; self->SetGlobalSoundsSaveOptions(options); }, (CKContext *, CK_SOUND_SAVEOPTIONS), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // Save/Load functions
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Load(const string &in fileName, CKObjectArray@ objects, CK_LOAD_FLAGS loadFlags = CK_LOAD_DEFAULT, CKGUID &in readerGuid = void)", asFUNCTIONPR([](CKContext *self, const std::string &fileName, CKObjectArray *objects, CK_LOAD_FLAGS loadFlags, CKGUID *readerGuid) -> CKERROR { if (RejectCKContextMutation("CKContext::Load")) return CKERR_INVALIDOPERATION; return self->Load(const_cast<CKSTRING>(fileName.c_str()), objects, loadFlags, readerGuid); }, (CKContext *, const std::string &, CKObjectArray *, CK_LOAD_FLAGS, CKGUID *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Load(int size, NativePointer buffer, CKObjectArray@ objects, CK_LOAD_FLAGS loadFlags = CK_LOAD_DEFAULT)", asFUNCTION(LoadCKContextFromBuffer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Load(NativeBuffer@ buffer, CKObjectArray@ objects, CK_LOAD_FLAGS loadFlags = CK_LOAD_DEFAULT)", asFUNCTION(LoadCKContextFromNativeBuffer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "string GetLastFileLoaded()", asFUNCTIONPR([](CKContext *self) -> std::string { return ScriptStringify(self->GetLastFileLoaded()); }, (CKContext *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "string GetLastCmoLoaded()", asFUNCTIONPR([](CKContext *self) -> std::string { return ScriptStringify(self->GetLastCmoLoaded()); }, (CKContext *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void SetLastCmoLoaded(const string &in str)", asFUNCTIONPR([](CKContext *self, const std::string &str) { if (RejectCKContextMutation("CKContext::SetLastCmoLoaded")) return; self->SetLastCmoLoaded(const_cast<CKSTRING>(str.c_str())); }, (CKContext *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR GetFileInfo(const string &in fileName, CKFileInfo &out fileInfo)", asFUNCTIONPR([](CKContext *self, const std::string &fileName, CKFileInfo *fileInfo) { return self->GetFileInfo(const_cast<CKSTRING>(fileName.c_str()), fileInfo); }, (CKContext *, const std::string &, CKFileInfo *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR GetFileInfo(int size, NativePointer buffer, CKFileInfo &out fileInfo)", asFUNCTION(GetCKContextFileInfoFromBuffer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR GetFileInfo(NativeBuffer@ buffer, CKFileInfo &out fileInfo)", asFUNCTION(GetCKContextFileInfoFromNativeBuffer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Save(const string &in fileName, CKObjectArray@ objects, CKDWORD saveFlags, CKDependencies &in dependencies = void, CKGUID &in readerGuid = void)", asFUNCTIONPR([](CKContext *self, const std::string &fileName, CKObjectArray *objects, CKDWORD saveFlags, CKDependencies *dependencies, CKGUID *readerGuid) -> CKERROR { if (RejectCKContextMutation("CKContext::Save")) return CKERR_INVALIDOPERATION; return self->Save(const_cast<CKSTRING>(fileName.c_str()), objects, saveFlags, dependencies, readerGuid); }, (CKContext *, const std::string &, CKObjectArray *, CKDWORD, CKDependencies *, CKGUID *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR LoadAnimationOnCharacter(const string &in fileName, CKObjectArray@ objects, CKCharacter@ carac, CKGUID &in readerGuid = void, bool asDynamicObjects = false)", asFUNCTIONPR([](CKContext *self, const std::string &fileName, CKObjectArray *objects, CKCharacter *character, CKGUID *readerGuid, bool asDynamicObjects) -> CKERROR { if (RejectCKContextMutation("CKContext::LoadAnimationOnCharacter")) return CKERR_INVALIDOPERATION; return self->LoadAnimationOnCharacter(const_cast<CKSTRING>(fileName.c_str()), objects, character, readerGuid, asDynamicObjects); }, (CKContext *, const std::string &, CKObjectArray *, CKCharacter *, CKGUID *, bool), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR LoadAnimationOnCharacter(int size, NativePointer buffer, CKObjectArray@ objects, CKCharacter@ carac, bool asDynamicObjects = false)", asFUNCTION(LoadCKContextAnimationOnCharacterFromBuffer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR LoadAnimationOnCharacter(NativeBuffer@ buffer, CKObjectArray@ objects, CKCharacter@ carac, bool asDynamicObjects = false)", asFUNCTION(LoadCKContextAnimationOnCharacterFromNativeBuffer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // r = engine->RegisterObjectMethod("CKContext", "void SetAutomaticLoadMode(CK_LOADMODE generalMode, CK_LOADMODE _3DObjectsMode, CK_LOADMODE meshMode, CK_LOADMODE matTexturesMode)", asMETHODPR(CKContext, SetAutomaticLoadMode, (CK_LOADMODE, CK_LOADMODE, CK_LOADMODE, CK_LOADMODE), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKContext", "void SetUserLoadCallback(uintptr_t, uintptr_t)", asMETHODPR(CKContext, SetUserLoadCallback, (CK_USERLOADCALLBACK, void *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKContext", "CK_LOADMODE LoadVerifyObjectUnicity(const string &in oldName, CK_CLASSID cid, const string &in newName, CKObject@ &out newObj)", asFUNCTIONPR([](CKContext *self, const std::string &oldName, CK_CLASSID cid, const std::string &newName, CKObject **newObj) { return self->LoadVerifyObjectUnicity(const_cast<CKSTRING>(oldName.c_str()), cid, const_cast<CKSTRING>(newName.c_str()), newObj); }, (CKContext *, const std::string &, CK_CLASSID, const std::string &, CKObject **), CK_LOADMODE), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "bool IsInLoad()", asFUNCTIONPR([](CKContext *self) -> bool { return self->IsInLoad(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "bool IsInSave()", asFUNCTIONPR([](CKContext *self) -> bool { return self->IsInSave(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "bool IsRunTime()", asFUNCTIONPR([](CKContext *self) -> bool { return self->IsRunTime(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // Render Engine Implementation Specific
    r = engine->RegisterObjectMethod("CKContext", "void ExecuteManagersOnPreRender(CKRenderContext@ dev)", asFUNCTIONPR([](CKContext *self, CKRenderContext *renderContext) { if (RejectCKContextMutation("CKContext::ExecuteManagersOnPreRender")) return; self->ExecuteManagersOnPreRender(renderContext); }, (CKContext *, CKRenderContext *), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void ExecuteManagersOnPostRender(CKRenderContext@ dev)", asFUNCTIONPR([](CKContext *self, CKRenderContext *renderContext) { if (RejectCKContextMutation("CKContext::ExecuteManagersOnPostRender")) return; self->ExecuteManagersOnPostRender(renderContext); }, (CKContext *, CKRenderContext *), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "void ExecuteManagersOnPostSpriteRender(CKRenderContext@ dev)", asFUNCTIONPR([](CKContext *self, CKRenderContext *renderContext) { if (RejectCKContextMutation("CKContext::ExecuteManagersOnPostSpriteRender")) return; self->ExecuteManagersOnPostSpriteRender(renderContext); }, (CKContext *, CKRenderContext *), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "void AddProfileTime(CK_PROFILE_CATEGORY category, float time)", asFUNCTIONPR([](CKContext *self, CK_PROFILE_CATEGORY category, float time) { if (RejectCKContextMutation("CKContext::AddProfileTime")) return; self->AddProfileTime(category, time); }, (CKContext *, CK_PROFILE_CATEGORY, float), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // Runtime Debug Mode
    r = engine->RegisterObjectMethod("CKContext", "CKERROR ProcessDebugStart(float deltaTime = 20.0)", asFUNCTIONPR([](CKContext *self, float deltaTime) -> CKERROR { if (RejectCKContextMutation("CKContext::ProcessDebugStart")) return CKERR_INVALIDOPERATION; return self->ProcessDebugStart(deltaTime); }, (CKContext *, float), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "bool ProcessDebugStep()", asFUNCTIONPR([](CKContext *self) -> bool { if (RejectCKContextMutation("CKContext::ProcessDebugStep")) return false; return self->ProcessDebugStep(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR ProcessDebugEnd()", asFUNCTIONPR([](CKContext *self) -> CKERROR { if (RejectCKContextMutation("CKContext::ProcessDebugEnd")) return CKERR_INVALIDOPERATION; return self->ProcessDebugEnd(); }, (CKContext *), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "CKDebugContext@ GetDebugContext()", asMETHODPR(CKContext, GetDebugContext, (), CKDebugContext *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKContext", "void SetVirtoolsVersion(CK_VIRTOOLS_VERSION ver, CKDWORD build)", asFUNCTIONPR([](CKContext *self, CK_VIRTOOLS_VERSION version, CKDWORD build) { if (RejectCKContextMutation("CKContext::SetVirtoolsVersion")) return; self->SetVirtoolsVersion(version, build); }, (CKContext *, CK_VIRTOOLS_VERSION, CKDWORD), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "int GetPVInformation()", asMETHODPR(CKContext, GetPVInformation, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKContext", "bool IsInDynamicCreationMode()", asFUNCTIONPR([](CKContext *self) -> bool { return self->IsInDynamicCreationMode(); }, (CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}
