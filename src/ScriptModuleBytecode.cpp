#include "ScriptModuleBytecode.h"

#include <cstring>

#include <fmt/format.h>

#include "ScriptAngelScriptGc.h"

namespace ScriptModuleBytecode {

namespace {

class MemoryByteCodeStream : public asIBinaryStream {
public:
    explicit MemoryByteCodeStream(std::vector<unsigned char> *buffer)
        : m_Buffer(buffer) {}

    int Read(void *ptr, asUINT size) override {
        if (!m_Buffer || (!ptr && size > 0) || m_ReadOffset > m_Buffer->size() ||
            size > m_Buffer->size() - m_ReadOffset) {
            return -1;
        }
        if (size > 0) {
            std::memcpy(ptr, m_Buffer->data() + m_ReadOffset, size);
            m_ReadOffset += size;
        }
        return 0;
    }

    int Write(const void *ptr, asUINT size) override {
        if (!m_Buffer || (!ptr && size > 0)) {
            return -1;
        }
        const size_t offset = m_Buffer->size();
        m_Buffer->resize(offset + size);
        if (size > 0) {
            std::memcpy(m_Buffer->data() + offset, ptr, size);
        }
        return 0;
    }

private:
    std::vector<unsigned char> *m_Buffer = nullptr;
    size_t m_ReadOffset = 0;
};

class CallbackByteCodeWriteStream : public asIBinaryStream {
public:
    CallbackByteCodeWriteStream(CKAngelScriptBytecodeWriteCallback callback, void *userData)
        : m_Callback(callback), m_UserData(userData) {}

    int Read(void *, asUINT) override {
        m_Status = CKAS_INVALIDSTATE;
        return -1;
    }

    int Write(const void *ptr, asUINT size) override {
        if (m_Status != CKAS_OK) {
            return -1;
        }
        if (!m_Callback || (!ptr && size > 0)) {
            m_Status = CKAS_INVALIDARGUMENT;
            return -1;
        }
        const CKAS_STATUS status = m_Callback(ptr, size, m_UserData);
        if (status != CKAS_OK) {
            m_Status = status;
        }
        return status == CKAS_OK ? 0 : -1;
    }

    CKAS_STATUS Status() const {
        return m_Status;
    }

private:
    CKAngelScriptBytecodeWriteCallback m_Callback = nullptr;
    void *m_UserData = nullptr;
    CKAS_STATUS m_Status = CKAS_OK;
};

class CallbackByteCodeReadStream : public asIBinaryStream {
public:
    CallbackByteCodeReadStream(CKAngelScriptBytecodeReadCallback callback, void *userData)
        : m_Callback(callback), m_UserData(userData) {}

    int Read(void *ptr, asUINT size) override {
        if (m_Status != CKAS_OK) {
            return -1;
        }
        if (!m_Callback || (!ptr && size > 0)) {
            m_Status = CKAS_INVALIDARGUMENT;
            return -1;
        }
        const CKAS_STATUS status = m_Callback(ptr, size, m_UserData);
        if (status != CKAS_OK) {
            m_Status = status;
        }
        return status == CKAS_OK ? 0 : -1;
    }

    int Write(const void *, asUINT) override {
        m_Status = CKAS_INVALIDSTATE;
        return -1;
    }

    CKAS_STATUS Status() const {
        return m_Status;
    }

private:
    CKAngelScriptBytecodeReadCallback m_Callback = nullptr;
    void *m_UserData = nullptr;
    CKAS_STATUS m_Status = CKAS_OK;
};

} // namespace

bool SaveModuleByteCode(asIScriptModule *module,
                        std::vector<unsigned char> &byteCode,
                        int &angelScriptCode,
                        bool stripDebugInfo) {
    byteCode.clear();
    if (!module) {
        angelScriptCode = -1;
        return false;
    }
    MemoryByteCodeStream stream(&byteCode);
    angelScriptCode = module->SaveByteCode(&stream, stripDebugInfo);
    if (angelScriptCode < 0) {
        byteCode.clear();
        return false;
    }
    return true;
}

bool SaveModuleByteCode(asIScriptModule *module,
                        CKAngelScriptBytecodeWriteCallback callback,
                        void *userData,
                        bool stripDebugInfo,
                        int &angelScriptCode,
                        CKAS_STATUS &callbackStatus) {
    callbackStatus = CKAS_OK;
    if (!module || !callback) {
        angelScriptCode = -1;
        callbackStatus = CKAS_INVALIDARGUMENT;
        return false;
    }
    CallbackByteCodeWriteStream stream(callback, userData);
    angelScriptCode = module->SaveByteCode(&stream, stripDebugInfo);
    callbackStatus = stream.Status();
    return angelScriptCode >= 0 && callbackStatus == CKAS_OK;
}

bool LoadModuleByteCode(asIScriptEngine *engine,
                        const char *moduleName,
                        std::vector<unsigned char> &byteCode,
                        asIScriptModule **outModule,
                        int &angelScriptCode) {
    if (outModule) {
        *outModule = nullptr;
    }
    if (!engine || !moduleName || moduleName[0] == '\0') {
        angelScriptCode = -1;
        return false;
    }
    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        angelScriptCode = -1;
        return false;
    }
    MemoryByteCodeStream stream(&byteCode);
    angelScriptCode = module->LoadByteCode(&stream);
    if (angelScriptCode < 0) {
        ScriptDiscardModuleWithGarbageCollection(module);
        return false;
    }
    if (outModule) {
        *outModule = module;
    }
    return true;
}

bool LoadModuleByteCode(asIScriptEngine *engine,
                        const char *moduleName,
                        CKAngelScriptBytecodeReadCallback callback,
                        void *userData,
                        asIScriptModule **outModule,
                        int &angelScriptCode,
                        CKAS_STATUS &callbackStatus) {
    if (outModule) {
        *outModule = nullptr;
    }
    callbackStatus = CKAS_OK;
    if (!engine || !moduleName || moduleName[0] == '\0' || !callback) {
        angelScriptCode = -1;
        callbackStatus = CKAS_INVALIDARGUMENT;
        return false;
    }
    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        angelScriptCode = -1;
        return false;
    }
    CallbackByteCodeReadStream stream(callback, userData);
    angelScriptCode = module->LoadByteCode(&stream);
    callbackStatus = stream.Status();
    if (angelScriptCode < 0 || callbackStatus != CKAS_OK) {
        ScriptDiscardModuleWithGarbageCollection(module);
        return false;
    }
    if (outModule) {
        *outModule = module;
    }
    return true;
}

std::string MakeTransientModuleName(asIScriptEngine *engine, const char *moduleName) {
    static unsigned int counter = 0;
    std::string candidate;
    do {
        candidate = fmt::format("__ckas_replace_candidate_{}_{}", moduleName ? moduleName : "module", ++counter);
    } while (engine && engine->GetModule(candidate.c_str(), asGM_ONLY_IF_EXISTS));
    return candidate;
}

} // namespace ScriptModuleBytecode
