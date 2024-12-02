#include "ScriptNativeBuffer.h"

#include <cassert>
#include <cstring>
#include <cstdio>

NativeBuffer::NativeBuffer(): m_Buffer(nullptr), m_Size(0), m_CursorPos(0) {}
NativeBuffer::NativeBuffer(void *buffer, size_t size): m_CursorPos(0), m_Buffer(static_cast<char *>(buffer)), m_Size(size) {}
NativeBuffer::NativeBuffer(const NativeBuffer &other): m_Buffer(other.m_Buffer), m_Size(other.m_Size), m_CursorPos(0) {}

NativeBuffer &NativeBuffer::operator=(const NativeBuffer &other) {
    if (this != &other) {
        m_Buffer = other.m_Buffer;
        m_Size = other.m_Size;
        m_CursorPos = other.m_CursorPos;
    }
    return *this;
}

char &NativeBuffer::operator[](size_t index) {
    static char dummy = 0;
    if (index >= m_Size) {
        asIScriptContext *ctx = asGetActiveContext();
        ctx->SetException("Index out of bounds");
        return dummy;
    }
    return m_Buffer[index];
}

const char &NativeBuffer::operator[](size_t index) const {
    static constexpr char dummy = 0;
    if (index >= m_Size) {
        asIScriptContext *ctx = asGetActiveContext();
        ctx->SetException("Index out of bounds");
        return dummy;
    }
    return m_Buffer[index];
}

size_t NativeBuffer::Write(void *x, size_t size) {
    if (!m_Buffer || m_CursorPos + size > m_Size || !x)
        return 0;
    memcpy(&m_Buffer[m_CursorPos], x, size);
    m_CursorPos += size;
    return size;
}

size_t NativeBuffer::Read(void *x, size_t size) {
    if (!m_Buffer || m_CursorPos + size > m_Size || !x)
        return 0;
    memcpy(x, &m_Buffer[m_CursorPos], size);
    m_CursorPos += size;
    return size;
}

size_t NativeBuffer::WriteString(const char *str) {
    if (!str)
        return 0;
    const size_t len = strlen(str) + 1;
    return Write((void *)str, static_cast<int>(len));
}

size_t NativeBuffer::WriteString(const std::string &str) {
    return WriteString(str.c_str());
}

size_t NativeBuffer::ReadString(char *outStr, size_t maxSize) {
    if (!m_Buffer || m_CursorPos >= m_Size)
        return 0;

    size_t i = 0;
    for (; i < maxSize - 1 && m_CursorPos < m_Size; ++i) {
        outStr[i] = m_Buffer[m_CursorPos++];
        if (outStr[i] == '\0')
            break;
    }

    outStr[i] = '\0';
    m_CursorPos += i + 1;
    return i + 1;
}

size_t NativeBuffer::ReadString(std::string &str) {
    if (!m_Buffer || m_CursorPos >= m_Size)
        return 0;

    size_t count = 0;
    str.clear();
    while (m_CursorPos < m_Size) {
        const char c = m_Buffer[m_CursorPos++];
        if (c == '\0')
            break;
        ++count;
        str.push_back(c);
    }

    m_CursorPos += count + 1;
    return count;
}

bool NativeBuffer::Fill(int value, size_t size) {
    if (!m_Buffer || m_CursorPos + size >= m_Size)
        return false;

    memset(&m_Buffer[m_CursorPos], value, size);
    m_CursorPos += size;
    return true;
}

bool NativeBuffer::Seek(size_t pos) {
    if (pos >= m_Size)
        return false;
    m_CursorPos = pos;
    return true;
}

bool NativeBuffer::Skip(size_t offset) {
    return Seek(m_CursorPos + offset);
}

int NativeBuffer::Compare(const NativeBuffer &other, size_t size) const {
    if (!m_Buffer || m_CursorPos + size > m_Size)
        return -1;
    if (other.CursorPos() + size > other.Size())
        return 1;
    return memcmp(&m_Buffer[m_CursorPos], other.Cursor(), size);
}

size_t NativeBuffer::Merge(const NativeBuffer &other, bool truncate) {
    int size = other.Size() - other.CursorPos();
    if (size == 0 || !m_Buffer)
        return 0;

    if (truncate) {
        if (m_CursorPos + size > m_Size)
            size = m_Size - m_CursorPos;
    } else if (m_CursorPos + size > m_Size) {
        return 0;
    }

    memcpy(&m_Buffer[m_CursorPos], other.Cursor(), size);
    m_CursorPos += size;
    return size;
}

NativeBuffer NativeBuffer::Extract(size_t size) {
    if (!m_Buffer || m_CursorPos + size > m_Size)
        return {};

    auto buffer = NativeBuffer(&m_Buffer[m_CursorPos], size);
    m_CursorPos += size;
    return buffer;
}

size_t NativeBuffer::Load(const char *filename, size_t size, int offset) {
    if (!m_Buffer || m_CursorPos + size > m_Size)
        return 0;

    FILE *fp = fopen(filename, "rb");
    if (!fp)
        return 0;

    if (offset > 0)
        fseek(fp, offset, SEEK_SET);
    size = fread(&m_Buffer[m_CursorPos], sizeof(char), size, fp);
    fclose(fp);
    m_CursorPos += size;
    return size;
}

size_t NativeBuffer::Save(const char *filename, size_t size) {
    if (!m_Buffer || m_CursorPos + size > m_Size)
        return 0;

    FILE *fp = fopen(filename, "wb");
    if (!fp)
        return 0;

    size = fwrite(&m_Buffer[m_CursorPos], sizeof(char), size, fp);
    fclose(fp);
    m_CursorPos += size;
    return size;
}

void NativeBufferWriteGeneric(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();
    const int typeId = gen->GetArgTypeId(0);
    void *addr = static_cast<void**>(gen->GetAddressOfArg(0));;
    auto *self = static_cast<NativeBuffer *>(gen->GetObject());
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

            if (size != 0 && self->CursorPos() + size <= self->Size()) {
                size = self->Write(addr, size);
            } else {
                gen->SetReturnDWord(0);
                return;
            }
        }
    } else {
        size = engine->GetSizeOfPrimitiveType(typeId);
        if (size != 0 && self->CursorPos() + size <= self->Size()) {
            size = self->Write(addr, size);
        }
    }

    gen->SetReturnDWord(size);
}

void NativeBufferReadGeneric(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();
    const int typeId = gen->GetArgTypeId(0);
    void *addr = static_cast<void**>(gen->GetAddressOfArg(0));;
    auto *self = static_cast<NativeBuffer *>(gen->GetObject());
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

            if (size != 0 && self->CursorPos() + size <= self->Size()) {
                size = self->Read(addr, size);
            } else {
                gen->SetReturnDWord(0);
                return;
            }
        }
    } else {
        size = engine->GetSizeOfPrimitiveType(typeId);
        if (size != 0 && self->CursorPos() + size <= self->Size()) {
            size = self->Read(addr, size);
        }
    }

    gen->SetReturnDWord(size);
}


void RegisterNativeBuffer(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("NativeBuffer", sizeof(NativeBuffer), asOBJ_VALUE | asGetTypeTraits<NativeBuffer>()); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("NativeBuffer", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](NativeBuffer *self) { new(self) NativeBuffer(); }, (NativeBuffer *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("NativeBuffer", asBEHAVE_CONSTRUCT, "void f(uint addr, int size)", asFUNCTIONPR([](uint32_t addr, int size, NativeBuffer *self) { new(self) NativeBuffer(reinterpret_cast<void *>(addr), size); }, (uint32_t, int, NativeBuffer *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("NativeBuffer", asBEHAVE_CONSTRUCT, "void f(const NativeBuffer &in other)", asFUNCTIONPR([](const NativeBuffer &other, NativeBuffer *self) { new(self) NativeBuffer(other); }, (const NativeBuffer &, NativeBuffer *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("NativeBuffer", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](NativeBuffer *self) { self->~NativeBuffer(); }, (NativeBuffer *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "NativeBuffer &opAssign(const NativeBuffer &in other)", asMETHODPR(NativeBuffer, operator=, (const NativeBuffer &), NativeBuffer &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "uint8 &opIndex(uint index)", asMETHODPR(NativeBuffer, operator[], (size_t), char &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "const uint8 &opIndex(uint index) const", asMETHODPR(NativeBuffer, operator[], (size_t) const, const char &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "uint Write(?&in)", asFUNCTION(NativeBufferWriteGeneric), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint Read(?&out)", asFUNCTION(NativeBufferReadGeneric), asCALL_GENERIC); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteInt(int value)", asMETHODPR(NativeBuffer, WriteInt, (int), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteUInt(uint value)", asMETHODPR(NativeBuffer, WriteUInt, (unsigned int), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteFloat(float value)", asMETHODPR(NativeBuffer, WriteFloat, (float), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteDouble(double value)", asMETHODPR(NativeBuffer, WriteDouble, (double), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteShort(int16 value)", asMETHODPR(NativeBuffer, WriteShort, (short), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteChar(int8 value)", asMETHODPR(NativeBuffer, WriteChar, (char), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteUChar(uint8 value)", asMETHODPR(NativeBuffer, WriteUChar, (unsigned char), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteLong(int64 value)", asMETHODPR(NativeBuffer, WriteLong, (long), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteULong(uint64 value)", asMETHODPR(NativeBuffer, WriteULong, (unsigned long), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint WriteString(const string &in value)", asMETHODPR(NativeBuffer, WriteString, (const std::string &), size_t), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadInt(int &out value)", asMETHODPR(NativeBuffer, ReadInt, (int &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadUInt(uint &out value)", asMETHODPR(NativeBuffer, ReadUInt, (unsigned int &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadFloat(float &out value)", asMETHODPR(NativeBuffer, ReadFloat, (float &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadDouble(double &out value)", asMETHODPR(NativeBuffer, ReadDouble, (double &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadShort(int16 &out value)", asMETHODPR(NativeBuffer, ReadShort, (short &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadChar(int8 &out value)", asMETHODPR(NativeBuffer, ReadChar, (char &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadUChar(uint8 &out value)", asMETHODPR(NativeBuffer, ReadUChar, (unsigned char &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadLong(int64 &out value)", asMETHODPR(NativeBuffer, ReadLong, (long &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadULong(uint64 &out value)", asMETHODPR(NativeBuffer, ReadULong, (unsigned long &), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint ReadString(string &out value)", asMETHODPR(NativeBuffer, ReadString, (std::string &), size_t), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "bool Fill(int value, uint size)", asMETHODPR(NativeBuffer, Fill, (int, size_t), bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "bool Seek(uint pos)", asMETHODPR(NativeBuffer, Seek, (size_t), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "bool Skip(uint offset)", asMETHODPR(NativeBuffer, Skip, (size_t), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "void Reset()", asMETHODPR(NativeBuffer, Reset, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "uint Size() const", asMETHODPR(NativeBuffer, Size, () const, size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint CursorPos() const", asMETHODPR(NativeBuffer, CursorPos, () const, size_t), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "bool IsValid() const", asMETHODPR(NativeBuffer, IsValid, () const, bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "bool IsEmpty() const", asMETHODPR(NativeBuffer, IsEmpty, () const, bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "int Compare(const NativeBuffer &in other, uint size) const", asMETHODPR(NativeBuffer, Compare, (const NativeBuffer &, size_t) const, int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint Merge(const NativeBuffer &in other, bool truncate = false)", asMETHODPR(NativeBuffer, Merge, (const NativeBuffer &, bool), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "NativeBuffer Extract(uint)", asMETHODPR(NativeBuffer, Extract, (size_t), NativeBuffer), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("NativeBuffer", "uint Load(const string &in filename, uint size, int offset = 0)", asMETHODPR(NativeBuffer, Load, (const char *, size_t, int), size_t), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("NativeBuffer", "uint Save(const string &in filename, uint size)", asMETHODPR(NativeBuffer, Save, (const char *, size_t), size_t), asCALL_THISCALL); assert(r >= 0);
}
