#include "ScriptNativePointer.h"

#include <cassert>
#include <cstring>
#include <fmt/format.h>

#include <add_on/scriptarray/scriptarray.h>
#include "ScriptRegistration.h"

std::string NativePointer::ToString() const {
    return fmt::format("0x{:X}", reinterpret_cast<uintptr_t>(m_Ptr));
}

size_t NativePointer::Write(void *x, size_t size) {
    if (!m_Ptr) return 0;
    if (!x) return 0;
    if (size == 0) return 0;
    memcpy(m_Ptr, x, size);
    return size;
}

size_t NativePointer::Read(void *x, size_t size) {
    if (!m_Ptr) return 0;
    if (!x) return 0;
    if (size == 0) return 0;
    memcpy(x, m_Ptr, size);
    return size;
}

size_t NativePointer::WriteString(const char *str) {
    if (!m_Ptr) return 0;
    if (!str) return 0;
    size_t len = strlen(str);
    if (len == 0) return 0;
    memcpy(m_Ptr, str, len + 1);
    return len + 1;
}

size_t NativePointer::WriteString(const std::string &str) {
    return WriteString(str.c_str());
}

size_t NativePointer::ReadString(char *outStr, size_t maxSize) {
    if (!m_Ptr) return 0;
    if (!outStr) return 0;
    if (maxSize == 0) return 0;
    size_t len = strlen(m_Ptr);
    if (len == 0) return 0;
    if (len + 1 > maxSize) return 0;
    memcpy(outStr, m_Ptr, len + 1);
    return len + 1;
}

size_t NativePointer::ReadString(std::string &str) {
    if (!m_Ptr) return 0;
    size_t len = strlen(m_Ptr);
    if (len == 0) return 0;
    str.assign(m_Ptr, len);
    return len + 1;
}

bool NativePointer::Fill(int value, size_t size) {
    if (!m_Ptr) return false;
    if (size == 0) return false;
    memset(m_Ptr, value, size);
    return true;
}

static void ConstructNativePointerGeneric(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();

    const int typeId = gen->GetArgTypeId(0);
    if ((typeId & asTYPEID_OBJHANDLE) || (typeId & asTYPEID_SCRIPTOBJECT)) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx) {
            ctx->SetException("Cannot create NativePointer from script objects or object handles");
        }
        new (gen->GetObject()) NativePointer();
        return;
    }

    void *addr = gen->GetArgAddress(0);
    asITypeInfo *type = engine->GetTypeInfoById(typeId);
    if (type && strcmp(type->GetName(), "array") == 0) {
        auto *array = static_cast<CScriptArray *>(addr);
        new (gen->GetObject()) NativePointer(array ? array->GetBuffer() : nullptr);
    } else {
        new (gen->GetObject()) NativePointer(addr);
    }
}

static void NativePointerWriteGeneric(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();
    const int typeId = gen->GetArgTypeId(0);
    auto *self = static_cast<NativePointer *>(gen->GetObject());
    size_t size = 0;

    if (typeId & asTYPEID_OBJHANDLE) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Cannot write object handle to buffer");
        gen->SetReturnDWord(0);
        return;
    }

    if (typeId & asTYPEID_SCRIPTOBJECT) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Cannot write script objects to buffer");
        gen->SetReturnDWord(0);
        return;
    }

    void *addr = gen->GetArgAddress(0);
    if (typeId & asTYPEID_APPOBJECT) {
        asITypeInfo *type = engine->GetTypeInfoById(typeId);
        if (!type) {
            gen->SetReturnDWord(0);
            return;
        }

        if (strcmp(type->GetName(), "string") == 0) {
            std::string &str = *static_cast<std::string *>(addr);
            size = self->WriteString(str);
        } else {
            size = engine->GetSizeOfPrimitiveType(typeId);
            if (size == 0) {
                if (type->GetFlags() & asOBJ_POD) {
                    size = type->GetSize();
                } else {
                    asIScriptContext *ctx = asGetActiveContext();
                    if (ctx)
                        ctx->SetException("Cannot write non-POD object to buffer");
                    gen->SetReturnDWord(0);
                }
            }
            size = self->Write(addr, size);
        }
    } else {
        size = engine->GetSizeOfPrimitiveType(typeId);
        if (size != 0)
            size = self->Write(addr, size);
    }

    gen->SetReturnDWord(size);
}

static void NativePointerReadGeneric(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();
    const int typeId = gen->GetArgTypeId(0);
    auto *self = static_cast<NativePointer *>(gen->GetObject());
    size_t size = 0;

    if (typeId & asTYPEID_OBJHANDLE) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Cannot read object handle from buffer");
        gen->SetReturnDWord(0);
        return;
    }

    if (typeId & asTYPEID_SCRIPTOBJECT) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Cannot read script objects from buffer");
        gen->SetReturnDWord(0);
        return;
    }

    void *addr = gen->GetArgAddress(0);
    if (typeId & asTYPEID_APPOBJECT) {
        asITypeInfo *type = engine->GetTypeInfoById(typeId);
        if (!type) {
            gen->SetReturnDWord(0);
            return;
        }

        if (strcmp(type->GetName(), "string") == 0) {
            std::string &str = *static_cast<std::string *>(addr);
            size = self->ReadString(str);
        } else {
            size = engine->GetSizeOfPrimitiveType(typeId);
            if (size == 0) {
                if (type->GetFlags() & asOBJ_POD) {
                    size = type->GetSize();
                } else {
                    asIScriptContext *ctx = asGetActiveContext();
                    if (ctx)
                        ctx->SetException("Cannot read non-POD object from buffer");
                    gen->SetReturnDWord(0);
                    return;
                }
            }
            size = self->Read(addr, size);
        }
    } else {
        size = engine->GetSizeOfPrimitiveType(typeId);
        if (size != 0)
            size = self->Read(addr, size);
    }

    gen->SetReturnDWord(size);
}

void RegisterNativePointer(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    r = engine->RegisterObjectType("NativePointer", sizeof(NativePointer), asOBJ_VALUE | asGetTypeTraits<NativePointer>()); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](NativePointer *self) { new(self) NativePointer(); }, (NativePointer *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_CONSTRUCT, "void f(?&in)", asFUNCTION(ConstructNativePointerGeneric), asCALL_GENERIC); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_CONSTRUCT, "void f(uintptr_t ptr)", asFUNCTIONPR([](uintptr_t ptr, NativePointer *self) { new(self) NativePointer(ptr); }, (uintptr_t, NativePointer *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_CONSTRUCT, "void f(intptr_t ptr)", asFUNCTIONPR([](intptr_t ptr, NativePointer *self) { new(self) NativePointer(ptr); }, (intptr_t, NativePointer *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_CONSTRUCT, "void f(const NativePointer &in other)", asFUNCTIONPR([](const NativePointer &other, NativePointer *self) { new(self) NativePointer(other); }, (const NativePointer &, NativePointer *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](NativePointer *self) { self->~NativePointer(); }, (NativePointer *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r= engine->RegisterObjectMethod("NativePointer", "NativePointer &opAssign(uintptr_t ptr)", asMETHODPR(NativePointer, operator=, (uintptr_t), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r= engine->RegisterObjectMethod("NativePointer", "NativePointer &opAssign(intptr_t ptr)", asMETHODPR(NativePointer, operator=, (intptr_t), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opAssign(const NativePointer &in other)", asMETHODPR(NativePointer, operator=, (const NativePointer &), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "bool opEquals(const NativePointer &in other) const", asMETHODPR(NativePointer, operator==, (const NativePointer &) const, bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "int opCmp(const NativePointer &in other) const", asMETHODPR(NativePointer, Compare, (const NativePointer &) const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opAddAssign(int rhs)", asMETHODPR(NativePointer, operator+=, (int), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opSubAssign(int rhs)", asMETHODPR(NativePointer, operator-=, (int), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opAndAssign(int rhs)", asMETHODPR(NativePointer, operator&=, (int), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opOrAssign(int rhs)", asMETHODPR(NativePointer, operator|=, (int), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opXorAssign(int rhs)", asMETHODPR(NativePointer, operator^=, (int), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opShlAssign(int rhs)", asMETHODPR(NativePointer, operator<<=, (int), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opShrAssign(int rhs)", asMETHODPR(NativePointer, operator>>=, (int), NativePointer &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opAdd(int rhs) const", asMETHODPR(NativePointer, operator+, (int) const, NativePointer), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opSub(int rhs) const", asMETHODPR(NativePointer, operator-, (int) const, NativePointer), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opAnd(int rhs) const", asMETHODPR(NativePointer, operator&, (int) const, NativePointer), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opOr(int rhs) const", asMETHODPR(NativePointer, operator|, (int) const, NativePointer), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opXor(int rhs) const", asMETHODPR(NativePointer, operator^, (int) const, NativePointer), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opShl(int rhs) const", asMETHODPR(NativePointer, operator<<, (int) const, NativePointer), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opShr(int rhs) const", asMETHODPR(NativePointer, operator>>, (int) const, NativePointer), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opNeg() const", asFUNCTIONPR([](const NativePointer &v) -> NativePointer { return -v; }, (const NativePointer &), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opCom() const", asFUNCTIONPR([](const NativePointer &v) -> NativePointer { return ~v; }, (const NativePointer &), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "uintptr_t ToUInt() const", asMETHODPR(NativePointer, ToUInt, () const, uintptr_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "intptr_t ToInt() const", asMETHODPR(NativePointer, ToInt, () const, intptr_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "string ToString() const", asMETHODPR(NativePointer, ToString, () const, std::string), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "NativePointer ReadPointer() const", asMETHODPR(NativePointer, ReadPointer, () const, NativePointer), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "void WritePointer(const NativePointer &in ptr)", asMETHODPR(NativePointer, WritePointer, (const NativePointer &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "uint Write(?&in)", asFUNCTION(NativePointerWriteGeneric), asCALL_GENERIC); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint Read(?&out)", asFUNCTION(NativePointerReadGeneric), asCALL_GENERIC); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "uint WriteInt(int value)", asMETHODPR(NativePointer, WriteInt, (int), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteUInt(uint value)", asMETHODPR(NativePointer, WriteUInt, (unsigned int), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteFloat(float value)", asMETHODPR(NativePointer, WriteFloat, (float), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteDouble(double value)", asMETHODPR(NativePointer, WriteDouble, (double), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteShort(int16 value)", asMETHODPR(NativePointer, WriteShort, (short), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteChar(int8 value)", asMETHODPR(NativePointer, WriteChar, (char), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteUChar(uint8 value)", asMETHODPR(NativePointer, WriteUChar, (unsigned char), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteLong(int32 value)", asMETHODPR(NativePointer, WriteLong, (long), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteULong(uint32 value)", asMETHODPR(NativePointer, WriteULong, (unsigned long), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteString(const string &in value)", asMETHODPR(NativePointer, WriteString, (const std::string &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "uint ReadInt(int &out value)", asMETHODPR(NativePointer, ReadInt, (int &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadUInt(uint &out value)", asMETHODPR(NativePointer, ReadUInt, (unsigned int &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadFloat(float &out value)", asMETHODPR(NativePointer, ReadFloat, (float &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadDouble(double &out value)", asMETHODPR(NativePointer, ReadDouble, (double &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadShort(int16 &out value)", asMETHODPR(NativePointer, ReadShort, (short &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadChar(int8 &out value)", asMETHODPR(NativePointer, ReadChar, (char &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadUChar(uint8 &out value)", asMETHODPR(NativePointer, ReadUChar, (unsigned char &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadLong(int32 &out value)", asMETHODPR(NativePointer, ReadLong, (long &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadULong(uint32 &out value)", asMETHODPR(NativePointer, ReadULong, (unsigned long &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadString(string &out value)", asMETHODPR(NativePointer, ReadString, (std::string &), size_t), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "bool Fill(int value, uint size)", asMETHODPR(NativePointer, Fill, (int, size_t), bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("NativePointer", "bool IsNull() const", asMETHODPR(NativePointer, IsNull, () const, bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("NativePointer malloc(size_t size)", asFUNCTIONPR([](size_t size) { return NativePointer(asAllocMem(size)); }, (size_t), NativePointer), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void free(NativePointer ptr)", asFUNCTIONPR([](NativePointer ptr) { asFreeMem(ptr.Get()); }, (NativePointer), void), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
}
