#include "ScriptNativePointer.h"

#include <cassert>
#include <sstream>

std::string NativePointer::ToString() const {
    std::stringstream stream;
    stream << m_Ptr;
    return stream.str();
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
    size_t len = strlen(reinterpret_cast<char *>(m_Ptr));
    if (len == 0) return 0;
    if (len > maxSize) return 0;
    memcpy(outStr, m_Ptr, len + 1);
    return len + 1;
}

size_t NativePointer::ReadString(std::string &str) {
    if (!m_Ptr) return 0;
    size_t len = strlen(reinterpret_cast<char *>(m_Ptr));
    if (len == 0) return 0;
    str.assign(reinterpret_cast<char *>(m_Ptr), len);
    return len + 1;
}

bool NativePointer::Fill(int value, size_t size) {
    if (!m_Ptr) return false;
    if (size == 0) return false;
    memset(m_Ptr, value, size);
    return true;
}

void NativePointerWriteGeneric(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();
    const int typeId = gen->GetArgTypeId(0);
    void *addr = static_cast<void**>(gen->GetAddressOfArg(0));
    auto *self = static_cast<NativePointer *>(gen->GetObject());
    size_t size = 0;

    if (typeId & asTYPEID_SCRIPTOBJECT) {
        // Not supported for now
        gen->SetReturnDWord(0);
        return;
    }

    if (typeId & asTYPEID_APPOBJECT) {
        if (typeId & asTYPEID_OBJHANDLE)
            addr = *static_cast<void **>(addr);

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
                    gen->SetReturnDWord(0);
                    return;
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

void NativePointerReadGeneric(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();
    const int typeId = gen->GetArgTypeId(0);
    void *addr = static_cast<void**>(gen->GetAddressOfArg(0));
    auto *self = static_cast<NativePointer *>(gen->GetObject());
    size_t size = 0;

    if (typeId & asTYPEID_SCRIPTOBJECT) {
        // Not supported for now
        gen->SetReturnDWord(0);
        return;
    }

    if (typeId & asTYPEID_APPOBJECT) {
        if (typeId & asTYPEID_OBJHANDLE)
            addr = *static_cast<void **>(addr);

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

    r = engine->RegisterObjectType("NativePointer", sizeof(NativePointer), asOBJ_VALUE | asGetTypeTraits<NativePointer>()); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](NativePointer *self) { new(self) NativePointer(); }, (NativePointer *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_CONSTRUCT, "void f(uintptr_t)", asFUNCTIONPR([](uintptr_t ptr, NativePointer *self) { new(self) NativePointer(ptr); }, (uintptr_t, NativePointer *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_CONSTRUCT, "void f(intptr_t)", asFUNCTIONPR([](intptr_t ptr, NativePointer *self) { new(self) NativePointer(ptr); }, (intptr_t, NativePointer *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_CONSTRUCT, "void f(const NativePointer &in)", asFUNCTIONPR([](const NativePointer &other, NativePointer *self) { new(self) NativePointer(other); }, (const NativePointer &, NativePointer *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("NativePointer", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](NativePointer *self) { self->~NativePointer(); }, (NativePointer *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    r= engine->RegisterObjectMethod("NativePointer", "NativePointer &opAssign(uintptr_t)", asMETHODPR(NativePointer, operator=, (uintptr_t), NativePointer &), asCALL_THISCALL); assert(r >= 0);
    r= engine->RegisterObjectMethod("NativePointer", "NativePointer &opAssign(intptr_t)", asMETHODPR(NativePointer, operator=, (intptr_t), NativePointer &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opAssign(const NativePointer &in)", asMETHODPR(NativePointer, operator=, (const NativePointer &), NativePointer &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "bool opEquals(const NativePointer &in) const", asMETHODPR(NativePointer, operator==, (const NativePointer &) const, bool), asCALL_THISCALL); assert(r >= 0);
   r = engine->RegisterObjectMethod("NativePointer", "int opCmp(const NativePointer &in) const", asMETHODPR(NativePointer, Compare, (const NativePointer &) const, int), asCALL_THISCALL); assert( r >= 0 );

    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opAddAssign(int)", asMETHODPR(NativePointer, operator+=, (int), NativePointer &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opSubAssign(int)", asMETHODPR(NativePointer, operator-=, (int), NativePointer &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opAndAssign(int)", asMETHODPR(NativePointer, operator&=, (int), NativePointer &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opOrAssign(int)", asMETHODPR(NativePointer, operator|=, (int), NativePointer &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opXorAssign(int)", asMETHODPR(NativePointer, operator^=, (int), NativePointer &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opShlAssign(int)", asMETHODPR(NativePointer, operator<<=, (int), NativePointer &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opShrAssign(int)", asMETHODPR(NativePointer, operator>>=, (int), NativePointer &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opAdd(int) const", asMETHODPR(NativePointer, operator+, (int) const, NativePointer), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opSub(int) const", asMETHODPR(NativePointer, operator-, (int) const, NativePointer), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opAnd(int) const", asMETHODPR(NativePointer, operator&, (int) const, NativePointer), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opOr(int) const", asMETHODPR(NativePointer, operator|, (int) const, NativePointer), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opXor(int) const", asMETHODPR(NativePointer, operator^, (int) const, NativePointer), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opShl(int) const", asMETHODPR(NativePointer, operator<<, (int) const, NativePointer), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer &opShr(int) const", asMETHODPR(NativePointer, operator>>, (int) const, NativePointer), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opNeg() const", asFUNCTIONPR([](const NativePointer &v) -> NativePointer { return -v; }, (const NativePointer &), NativePointer), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "NativePointer opCom() const", asFUNCTIONPR([](const NativePointer &v) -> NativePointer { return ~v; }, (const NativePointer &), NativePointer), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "uintptr_t ToUInt() const", asMETHODPR(NativePointer, ToUInt, () const, uintptr_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "intptr_t ToInt() const", asMETHODPR(NativePointer, ToInt, () const, intptr_t), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "string ToString() const", asMETHODPR(NativePointer, ToString, () const, std::string), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "NativePointer ReadPointer() const", asMETHODPR(NativePointer, ReadPointer, () const, NativePointer), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "void WritePointer(const NativePointer &in)", asMETHODPR(NativePointer, WritePointer, (const NativePointer &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "uint Write(?&in)", asFUNCTION(NativePointerWriteGeneric), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint Read(?&out)", asFUNCTION(NativePointerReadGeneric), asCALL_GENERIC); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "uint WriteInt(int)", asMETHODPR(NativePointer, WriteInt, (int), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteUInt(uint)", asMETHODPR(NativePointer, WriteUInt, (unsigned int), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteFloat(float)", asMETHODPR(NativePointer, WriteFloat, (float), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteDouble(double)", asMETHODPR(NativePointer, WriteDouble, (double), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteShort(int16)", asMETHODPR(NativePointer, WriteShort, (short), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteChar(int8)", asMETHODPR(NativePointer, WriteChar, (char), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteUChar(uint8)", asMETHODPR(NativePointer, WriteUChar, (unsigned char), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteLong(int64)", asMETHODPR(NativePointer, WriteLong, (long), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteULong(uint64)", asMETHODPR(NativePointer, WriteULong, (unsigned long), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint WriteString(const string &in)", asMETHODPR(NativePointer, WriteString, (const std::string &), size_t), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "uint ReadInt(int &out)", asMETHODPR(NativePointer, ReadInt, (int &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadUInt(uint &out)", asMETHODPR(NativePointer, ReadUInt, (unsigned int &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadFloat(float &out)", asMETHODPR(NativePointer, ReadFloat, (float &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadDouble(double &out)", asMETHODPR(NativePointer, ReadDouble, (double &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadShort(int16 &out)", asMETHODPR(NativePointer, ReadShort, (short &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadChar(int8 &out)", asMETHODPR(NativePointer, ReadChar, (char &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadUChar(uint8 &out)", asMETHODPR(NativePointer, ReadUChar, (unsigned char &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadLong(int64 &out)", asMETHODPR(NativePointer, ReadLong, (long &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadULong(uint64 &out)", asMETHODPR(NativePointer, ReadULong, (unsigned long &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativePointer", "uint ReadString(string &out)", asMETHODPR(NativePointer, ReadString, (std::string &), size_t), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "bool Fill(int, uint)", asMETHODPR(NativePointer, Fill, (int, size_t), bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativePointer", "bool IsNull() const", asMETHODPR(NativePointer, IsNull, () const, bool), asCALL_THISCALL); assert(r >= 0);
}
