#include "ScriptDynCall.h"

#include <cassert>
#include <string>
#include <type_traits>
#include <stdexcept>

#include <dyncall.h>
#include <dyncall_callback.h>
#include <dynload.h>

#include "RefCount.h"
#include "ScriptNativePointer.h"

class DynAggregate {
public:
    static DynAggregate *Create(size_t maxFieldCount, size_t size) {
        void *self = asAllocMem(sizeof(DynAggregate));
        return new(self) DynAggregate(maxFieldCount, size);
    }

    DynAggregate(size_t maxFieldCount, size_t size) {
        aggr = dcNewAggr(maxFieldCount, size);
        if (!aggr) {
            auto *ctx = asGetActiveContext();
            if (ctx)
                ctx->SetException("Failed to create aggregate.");
        }
    }

    ~DynAggregate() {
        dcFreeAggr(aggr);
    }

    DynAggregate(const DynAggregate &other) = delete;
    DynAggregate(DynAggregate &&other) noexcept = delete;
    DynAggregate &operator=(const DynAggregate &) = delete;
    DynAggregate &operator=(DynAggregate &&) noexcept = delete;

    bool operator==(const DynAggregate &rhs) const {
        return aggr == rhs.aggr;
    }

    bool operator!=(const DynAggregate &rhs) const {
        return !(*this == rhs);
    }

    int AddRef() const {
        return m_RefCount.AddRef();
    }

    int Release() const {
        const int r = m_RefCount.Release();
        if (r == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
            this->~DynAggregate();
            asFreeMem(const_cast<DynAggregate *>(this));
        }
        return r;
    }

    asILockableSharedBool *GetWeakRefFlag() {
        if(!m_WeakRefFlag)
            m_WeakRefFlag = asCreateLockableSharedBool();
        return m_WeakRefFlag;
    }

    DCaggr *Get() const {
        return aggr;
    }

    DynAggregate &Field(char type, int offset, size_t arrayLength = 1) {
        dcAggrField(aggr, type, offset, arrayLength);
        return *this;
    }

    DynAggregate &AggregateField(const DynAggregate &nestedAggr, int offset, size_t arrayLength = 1) {
        dcAggrField(aggr, DC_SIGCHAR_AGGREGATE, offset, arrayLength, nestedAggr.Get());
        return *this;
    }

    void Close() {
        dcCloseAggr(aggr);
    }

    DCaggr *aggr;

private:
    mutable RefCount m_RefCount;
    asILockableSharedBool *m_WeakRefFlag = nullptr;
};

class DynCall {
public:
    static DynCall *Create() {
        void *self = asAllocMem(sizeof(DynCall));
        return new(self) DynCall();
    }

    static DynCall *Create(size_t size) {
        void *self = asAllocMem(sizeof(DynCall));
        return new(self) DynCall(size);
    }

    explicit DynCall(size_t size = 4096) {
        vm = dcNewCallVM(size);
        if (!vm) {
            auto *ctx = asGetActiveContext();
            if (ctx)
                ctx->SetException("Failed to create vm.");
        }
    }

    ~DynCall() {
        dcFree(vm);
    }

    DynCall(const DynCall &other) = delete;
    DynCall(DynCall &&other) noexcept = delete;
    DynCall &operator=(const DynCall &) = delete;
    DynCall &operator=(DynCall &&) noexcept = delete;

    bool operator==(const DynCall &rhs) const {
        return vm == rhs.vm;
    }

    bool operator!=(const DynCall &rhs) const {
        return !(*this == rhs);
    }

    int AddRef() const {
        return m_RefCount.AddRef();
    }

    int Release() const {
        const int r = m_RefCount.Release();
        if (r == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
            this->~DynCall();
            asFreeMem(const_cast<DynCall *>(this));
        }
        return r;
    }

    asILockableSharedBool *GetWeakRefFlag() {
        if(!m_WeakRefFlag)
            m_WeakRefFlag = asCreateLockableSharedBool();
        return m_WeakRefFlag;
    }

    void Reset() {
        dcReset(vm);
    }

    void SetMode(int mode) {
        dcMode(vm, mode);
    }

    DynCall &BeginCallAggr(const DynAggregate &aggr) {
        dcBeginCallAggr(vm, aggr.Get());
        return *this;
    }

    DynCall &ArgBool(int value) {
        dcArgBool(vm, value);
        return *this;
    }

    DynCall &ArgChar(char value) {
        dcArgChar(vm, value);
        return *this;
    }

    DynCall &ArgShort(short value) {
        dcArgShort(vm, value);
        return *this;
    }

    DynCall &ArgInt(int value) {
        dcArgInt(vm, value);
        return *this;
    }

    DynCall &ArgLong(long value) {
        dcArgLong(vm, value);
        return *this;
    }

    DynCall &ArgLongLong(long long value) {
        dcArgLongLong(vm, value);
        return *this;
    }

    DynCall &ArgFloat(float value) {
        dcArgFloat(vm, value);
        return *this;
    }

    DynCall &ArgDouble(double value) {
        dcArgDouble(vm, value);
        return *this;
    }

    DynCall &ArgPointer(void *value) {
        dcArgPointer(vm, value);
        return *this;
    }

    DynCall &ArgAggregate(const DynAggregate &aggr, const void *value) {
        dcArgAggr(vm, aggr.Get(), value);
        return *this;
    }

    void CallVoid(void *funcptr) {
        dcCallVoid(vm, funcptr);
    }

    int CallBool(void *funcptr) {
        return dcCallBool(vm, funcptr);
    }

    char CallChar(void *funcptr) {
        return dcCallChar(vm, funcptr);
    }

    short CallShort(void *funcptr) {
        return dcCallShort(vm, funcptr);
    }

    int CallInt(void *funcptr) {
        return dcCallInt(vm, funcptr);
    }

    long CallLong(void *funcptr) {
        return dcCallLong(vm, funcptr);
    }

    long long CallLongLong(void *funcptr) {
        return dcCallLongLong(vm, funcptr);
    }

    float CallFloat(void *funcptr) {
        return dcCallFloat(vm, funcptr);
    }

    double CallDouble(void *funcptr) {
        return dcCallDouble(vm, funcptr);
    }

    void *CallPointer(void *funcptr) {
        return dcCallPointer(vm, funcptr);
    }

    void *CallAggregate(void *funcptr, const DynAggregate &aggr, void *ret) {
        return dcCallAggr(vm, funcptr, aggr.Get(), ret);
    }

    int GetError() const {
        return dcGetError(vm);
    }

    DCCallVM *vm;

private:
    mutable RefCount m_RefCount;
    asILockableSharedBool *m_WeakRefFlag = nullptr;
};

class DynCallback {
public:
    static DynCallback *Create(const std::string &signature, asIScriptFunction *func) {
        if (!func)
            return nullptr;

        void *self = asAllocMem(sizeof(DynCallback));
        return new(self) DynCallback(signature, func);
    }

    static DynCallback *Create(const std::string &signature, asIScriptFunction *func, const DynAggregate &aggregate) {
        if (!func)
            return nullptr;

        void *self = asAllocMem(sizeof(DynCallback));
        return new(self) DynCallback(signature, func, aggregate);
    }

    DynCallback(const std::string &signature, asIScriptFunction *func) {
        if (func) {
            func->AddRef();
        }

        callback = dcbNewCallback(signature.c_str(), ScriptCallbackHandler, func);
        if (!callback) {
            auto *ctx = asGetActiveContext();
            if (ctx)
                ctx->SetException("Failed to create callback.");
        }
    }

    DynCallback(const std::string &signature, asIScriptFunction *func, const DynAggregate &aggregate) {
        if (func) {
            func->AddRef();
        }

        callback = dcbNewCallback2(signature.c_str(), ScriptCallbackHandler, func, &aggregate.aggr);
        if (!callback) {
            auto *ctx = asGetActiveContext();
            if (ctx)
                ctx->SetException("Failed to create callback.");
        }
    }

    ~DynCallback() {
        dcbFreeCallback(callback);
    }

    DynCallback(const DynCallback &other) = delete;
    DynCallback(DynCallback &&other) noexcept = delete;
    DynCallback &operator=(const DynCallback &) = delete;
    DynCallback &operator=(DynCallback &&) noexcept = delete;

    bool operator==(const DynCallback &rhs) const {
        return callback == rhs.callback;
    }

    bool operator!=(const DynCallback &rhs) const {
        return !(*this == rhs);
    }

    int AddRef() const {
        return m_RefCount.AddRef();
    }

    int Release() const {
        const int r = m_RefCount.Release();
        if (r == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
            this->~DynCallback();
            asFreeMem(const_cast<DynCallback *>(this));
        }
        return r;
    }

    asILockableSharedBool *GetWeakRefFlag() {
        if(!m_WeakRefFlag)
            m_WeakRefFlag = asCreateLockableSharedBool();
        return m_WeakRefFlag;
    }

    void Init(const std::string &signature, asIScriptFunction *func) {
        auto *handler = GetHandler();
        if (handler) {
            handler->Release();
        }

        if (func) {
            func->AddRef();
        }

        dcbInitCallback(callback, signature.c_str(), ScriptCallbackHandler, func);
    }

    void Init(const std::string &signature, asIScriptFunction *func, const DynAggregate &aggregate) {
        auto *handler = GetHandler();
        if (handler) {
            handler->Release();
        }

        if (func) {
            func->AddRef();
        }

        dcbInitCallback2(callback, signature.c_str(), ScriptCallbackHandler, func, &aggregate.aggr);
    }

    asIScriptFunction *GetHandler() const {
        return static_cast<asIScriptFunction *>(dcbGetUserData(callback));
    }

    DCCallback *Get() const {
        return callback;
    }

    static DCsigchar ScriptCallbackHandler(DCCallback *cb, DCArgs *args, DCValue *result, void *userdata) {
        auto *func = static_cast<asIScriptFunction *>(userdata);
        if (!func)
            return DC_SIGCHAR_VOID;

        asIScriptEngine *engine = func->GetEngine();
        asIScriptContext *ctx = engine->RequestContext();

        int r = 0;
        if (func->GetFuncType() == asFUNC_DELEGATE) {
            asIScriptFunction *callback = func->GetDelegateFunction();
            void *callbackObject = func->GetDelegateObject();
            r = ctx->Prepare(callback);
            ctx->SetObject(callbackObject);
        } else {
            r = ctx->Prepare(func);
        }

        if (r < 0) {
            engine->ReturnContext(ctx);
            return DC_SIGCHAR_VOID;
        }

        ctx->SetArgObject(0, &cb);
        ctx->SetArgObject(1, args);
        ctx->SetArgObject(2, result);

        ctx->Execute();

        auto signature = static_cast<DCsigchar>(ctx->GetReturnByte());

        engine->ReturnContext(ctx);

        return signature;
    }

    DCCallback *callback;

private:
    mutable RefCount m_RefCount;
    asILockableSharedBool *m_WeakRefFlag = nullptr;
};

class DynLibrary {
public:
    static DynLibrary *Create() {
        void *self = asAllocMem(sizeof(DynLibrary));
        return new(self) DynLibrary();
    }

    explicit DynLibrary() : lib(nullptr) {}

    ~DynLibrary() {
        dlFreeLibrary(lib);
    }

    DynLibrary(const DynLibrary &other) = delete;
    DynLibrary(DynLibrary &&other) noexcept = delete;
    DynLibrary &operator=(const DynLibrary &) = delete;
    DynLibrary &operator=(DynLibrary &&) noexcept = delete;

    bool operator==(const DynLibrary &rhs) const {
        return lib == rhs.lib;
    }

    bool operator!=(const DynLibrary &rhs) const {
        return !(*this == rhs);
    }

    int AddRef() const {
        return m_RefCount.AddRef();
    }

    int Release() const {
        const int r = m_RefCount.Release();
        if (r == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
            this->~DynLibrary();
            asFreeMem(const_cast<DynLibrary *>(this));
        }
        return r;
    }

    asILockableSharedBool *GetWeakRefFlag() {
        if(!m_WeakRefFlag)
            m_WeakRefFlag = asCreateLockableSharedBool();
        return m_WeakRefFlag;
    }

    bool IsLoaded() const {
        return lib != nullptr;
    }

    bool Load(const std::string &libPath) {
        if (lib) {
            return false;
        }

        lib = dlLoadLibrary(libPath.c_str());
        if (!lib) {
            return false;
        }
        return true;
    }

    // Find a function symbol by name
    void *FindSymbol(const std::string &symbolName) const {
        if (!lib) {
            return nullptr;
        }

        void *symbol = dlFindSymbol(lib, symbolName.c_str());
        if (!symbol) {
            throw std::runtime_error("Failed to find symbol: " + symbolName);
        }
        return symbol;
    }

    // Get the path of the loaded library
    std::string GetLibraryPath() const {
        if (!lib)
            return "";

        char path[512];
        if (dlGetLibraryPath(lib, path, sizeof(path)) == 0) {
            return "";
        }
        return std::move(std::string(path));
    }

    DLLib *lib;

private:
    mutable RefCount m_RefCount;
    asILockableSharedBool *m_WeakRefFlag = nullptr;
};

class DynSymbols {
public:
    static DynSymbols *Create() {
        void *self = asAllocMem(sizeof(DynSymbols));
        return new(self) DynSymbols();
    }

    explicit DynSymbols() : syms(nullptr) {}

    ~DynSymbols() {
        dlSymsCleanup(syms);
    }

    DynSymbols(const DynSymbols &other) = delete;
    DynSymbols(DynSymbols &&other) noexcept = delete;
    DynSymbols &operator=(const DynSymbols &) = delete;
    DynSymbols &operator=(DynSymbols &&) noexcept = delete;

    bool operator==(const DynSymbols &rhs) const {
        return syms == rhs.syms;
    }

    bool operator!=(const DynSymbols &rhs) const {
        return !(*this == rhs);
    }

    int AddRef() const {
        return m_RefCount.AddRef();
    }

    int Release() const {
        const int r = m_RefCount.Release();
        if (r == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
            this->~DynSymbols();
            asFreeMem(const_cast<DynSymbols *>(this));
        }
        return r;
    }

    asILockableSharedBool *GetWeakRefFlag() {
        if(!m_WeakRefFlag)
            m_WeakRefFlag = asCreateLockableSharedBool();
        return m_WeakRefFlag;
    }

    int GetCount() const {
        if (!syms) {
            return 0;
        }
        return dlSymsCount(syms);
    }

    bool IsInited() const {
        return syms != nullptr;
    }

    bool Init(const std::string &libPath) {
        if (syms) {
            return false;
        }

        syms = dlSymsInit(libPath.c_str());
        if (!syms) {
            return false;
        }
        return true;
    }

    std::string GetName(int index) const {
        if (!syms) {
            return "";
        }
        return std::move(std::string(dlSymsName(syms, index)));
    }

    std::string GetNameByValue(void *value) const {
        if (!syms) {
            return "";
        }
        return std::move(std::string(dlSymsNameFromValue(syms, value)));
    }

    DLSyms *syms;

private:
    mutable RefCount m_RefCount;
    asILockableSharedBool *m_WeakRefFlag = nullptr;
};

static void RegisterDCEnums(asIScriptEngine* engine) {
    int r = 0;

    r = engine->RegisterEnum("DCCallMode"); assert(r >= 0);

    // Default calling modes
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_DEFAULT", DC_CALL_C_DEFAULT); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_DEFAULT_THIS", DC_CALL_C_DEFAULT_THIS); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ELLIPSIS", DC_CALL_C_ELLIPSIS); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ELLIPSIS_VARARGS", DC_CALL_C_ELLIPSIS_VARARGS); assert(r >= 0);

    // Platform-specific calling modes
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_CDECL", DC_CALL_C_X86_CDECL); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_STD", DC_CALL_C_X86_WIN32_STD); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_FAST_MS", DC_CALL_C_X86_WIN32_FAST_MS); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_FAST_GNU", DC_CALL_C_X86_WIN32_FAST_GNU); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_THIS_MS", DC_CALL_C_X86_WIN32_THIS_MS); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_THIS_GNU", DC_CALL_C_X86_WIN32_THIS_GNU); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X64_WIN64", DC_CALL_C_X64_WIN64); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X64_WIN64_THIS", DC_CALL_C_X64_WIN64_THIS); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X64_SYSV", DC_CALL_C_X64_SYSV); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X64_SYSV_THIS", DC_CALL_C_X64_SYSV_THIS); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC32_DARWIN", DC_CALL_C_PPC32_DARWIN); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC32_OSX", DC_CALL_C_PPC32_OSX); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_ARM_EABI", DC_CALL_C_ARM_ARM_EABI); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_THUMB_EABI", DC_CALL_C_ARM_THUMB_EABI); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_ARMHF", DC_CALL_C_ARM_ARMHF); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS32_EABI", DC_CALL_C_MIPS32_EABI); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS32_PSPSDK", DC_CALL_C_MIPS32_PSPSDK); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC32_SYSV", DC_CALL_C_PPC32_SYSV); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC32_LINUX", DC_CALL_C_PPC32_LINUX); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_ARM", DC_CALL_C_ARM_ARM); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_THUMB", DC_CALL_C_ARM_THUMB); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS32_O32", DC_CALL_C_MIPS32_O32); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS64_N32", DC_CALL_C_MIPS64_N32); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS64_N64", DC_CALL_C_MIPS64_N64); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_PLAN9", DC_CALL_C_X86_PLAN9); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_SPARC32", DC_CALL_C_SPARC32); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_SPARC64", DC_CALL_C_SPARC64); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM64", DC_CALL_C_ARM64); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC64", DC_CALL_C_PPC64); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC64_LINUX", DC_CALL_C_PPC64_LINUX); assert(r >= 0);

    // Default syscall mode
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_DEFAULT", DC_CALL_SYS_DEFAULT); assert(r >= 0);

    // Platform-specific syscall modes
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_X86_INT80H_LINUX", DC_CALL_SYS_X86_INT80H_LINUX); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_X86_INT80H_BSD", DC_CALL_SYS_X86_INT80H_BSD); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_X64_SYSCALL_SYSV", DC_CALL_SYS_X64_SYSCALL_SYSV); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_PPC32", DC_CALL_SYS_PPC32); assert(r >= 0);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_PPC64", DC_CALL_SYS_PPC64); assert(r >= 0);

    // Error codes
    r = engine->RegisterEnum("DCError"); assert(r >= 0);
    r = engine->RegisterEnumValue("DCError", "DC_ERROR_NONE", DC_ERROR_NONE); assert(r >= 0);
    r = engine->RegisterEnumValue("DCError", "DC_ERROR_UNSUPPORTED_MODE", DC_ERROR_UNSUPPORTED_MODE); assert(r >= 0);
}

static const char g_DC_SIGCHAR_VOID = DC_SIGCHAR_VOID;
static const char g_DC_SIGCHAR_BOOL = DC_SIGCHAR_BOOL;
static const char g_DC_SIGCHAR_CHAR = DC_SIGCHAR_CHAR;
static const char g_DC_SIGCHAR_UCHAR = DC_SIGCHAR_UCHAR;
static const char g_DC_SIGCHAR_SHORT = DC_SIGCHAR_SHORT;
static const char g_DC_SIGCHAR_USHORT = DC_SIGCHAR_USHORT;
static const char g_DC_SIGCHAR_INT = DC_SIGCHAR_INT;
static const char g_DC_SIGCHAR_UINT = DC_SIGCHAR_UINT;
static const char g_DC_SIGCHAR_LONG = DC_SIGCHAR_LONG;
static const char g_DC_SIGCHAR_ULONG = DC_SIGCHAR_ULONG;
static const char g_DC_SIGCHAR_LONGLONG = DC_SIGCHAR_LONGLONG;
static const char g_DC_SIGCHAR_ULONGLONG = DC_SIGCHAR_ULONGLONG;
static const char g_DC_SIGCHAR_FLOAT = DC_SIGCHAR_FLOAT;
static const char g_DC_SIGCHAR_DOUBLE = DC_SIGCHAR_DOUBLE;
static const char g_DC_SIGCHAR_POINTER = DC_SIGCHAR_POINTER;
static const char g_DC_SIGCHAR_STRING = DC_SIGCHAR_STRING;
static const char g_DC_SIGCHAR_AGGREGATE = DC_SIGCHAR_AGGREGATE;
static const char g_DC_SIGCHAR_ENDARG = DC_SIGCHAR_ENDARG;

// Calling convention / mode signature
static const char g_DC_SIGCHAR_CC_PREFIX = DC_SIGCHAR_CC_PREFIX;
static const char g_DC_SIGCHAR_CC_DEFAULT = DC_SIGCHAR_CC_DEFAULT;
static const char g_DC_SIGCHAR_CC_THISCALL = DC_SIGCHAR_CC_THISCALL;
static const char g_DC_SIGCHAR_CC_ELLIPSIS = DC_SIGCHAR_CC_ELLIPSIS;
static const char g_DC_SIGCHAR_CC_ELLIPSIS_VARARGS = DC_SIGCHAR_CC_ELLIPSIS_VARARGS;
static const char g_DC_SIGCHAR_CC_CDECL = DC_SIGCHAR_CC_CDECL;
static const char g_DC_SIGCHAR_CC_STDCALL = DC_SIGCHAR_CC_STDCALL;
static const char g_DC_SIGCHAR_CC_FASTCALL_MS = DC_SIGCHAR_CC_FASTCALL_MS;
static const char g_DC_SIGCHAR_CC_FASTCALL_GNU = DC_SIGCHAR_CC_FASTCALL_GNU;
static const char g_DC_SIGCHAR_CC_THISCALL_MS = DC_SIGCHAR_CC_THISCALL_MS;
static const char g_DC_SIGCHAR_CC_THISCALL_GNU = DC_SIGCHAR_CC_THISCALL_GNU;
static const char g_DC_SIGCHAR_CC_ARM_ARM = DC_SIGCHAR_CC_ARM_ARM;
static const char g_DC_SIGCHAR_CC_ARM_THUMB = DC_SIGCHAR_CC_ARM_THUMB;
static const char g_DC_SIGCHAR_CC_SYSCALL = DC_SIGCHAR_CC_SYSCALL;

static void RegisterDCSigChars(asIScriptEngine* engine) {
    int r = 0;

    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_VOID", (void*)&g_DC_SIGCHAR_VOID); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_BOOL", (void*)&g_DC_SIGCHAR_BOOL); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CHAR", (void*)&g_DC_SIGCHAR_CHAR); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_UCHAR", (void*)&g_DC_SIGCHAR_UCHAR); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_SHORT", (void*)&g_DC_SIGCHAR_SHORT); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_USHORT", (void*)&g_DC_SIGCHAR_USHORT); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_INT", (void*)&g_DC_SIGCHAR_INT); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_UINT", (void*)&g_DC_SIGCHAR_UINT); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_LONG", (void*)&g_DC_SIGCHAR_LONG); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_ULONG", (void*)&g_DC_SIGCHAR_ULONG); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_LONGLONG", (void*)&g_DC_SIGCHAR_LONGLONG); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_ULONGLONG", (void*)&g_DC_SIGCHAR_ULONGLONG); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_FLOAT", (void*)&g_DC_SIGCHAR_FLOAT); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_DOUBLE", (void*)&g_DC_SIGCHAR_DOUBLE); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_POINTER", (void*)&g_DC_SIGCHAR_POINTER); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_STRING", (void*)&g_DC_SIGCHAR_STRING); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_AGGREGATE", (void*)&g_DC_SIGCHAR_AGGREGATE); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_ENDARG", (void*)&g_DC_SIGCHAR_ENDARG); assert(r >= 0);

    // Calling convention/mode signature
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_PREFIX", (void*)&g_DC_SIGCHAR_CC_PREFIX); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_DEFAULT", (void*)&g_DC_SIGCHAR_CC_DEFAULT); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_THISCALL", (void*)&g_DC_SIGCHAR_CC_THISCALL); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_ELLIPSIS", (void*)&g_DC_SIGCHAR_CC_ELLIPSIS); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_ELLIPSIS_VARARGS", (void*)&g_DC_SIGCHAR_CC_ELLIPSIS_VARARGS); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_CDECL", (void*)&g_DC_SIGCHAR_CC_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_STDCALL", (void*)&g_DC_SIGCHAR_CC_STDCALL); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_FASTCALL_MS", (void*)&g_DC_SIGCHAR_CC_FASTCALL_MS); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_FASTCALL_GNU", (void*)&g_DC_SIGCHAR_CC_FASTCALL_GNU); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_THISCALL_MS", (void*)&g_DC_SIGCHAR_CC_THISCALL_MS); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_THISCALL_GNU", (void*)&g_DC_SIGCHAR_CC_THISCALL_GNU); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_ARM_ARM", (void*)&g_DC_SIGCHAR_CC_ARM_ARM); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_ARM_THUMB", (void*)&g_DC_SIGCHAR_CC_ARM_THUMB); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_SYSCALL", (void*)&g_DC_SIGCHAR_CC_SYSCALL); assert(r >= 0);
}

void RegisterDynAggregate(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynAggregate", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynAggregate", asBEHAVE_FACTORY, "DynAggregate@ f(size_t buf, size_t size)", asFUNCTIONPR(DynAggregate::Create, (size_t, size_t), DynAggregate *), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("DynAggregate", asBEHAVE_ADDREF, "void f()", asMETHOD(DynAggregate, AddRef), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynAggregate", asBEHAVE_RELEASE, "void f()", asMETHOD(DynAggregate, Release), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynAggregate", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynAggregate, GetWeakRefFlag), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynAggregate", "DynAggregate &Field(int8 type, int offset, size_t arrayLength = 1)", asMETHODPR(DynAggregate, Field, (char type, int offset, size_t), DynAggregate &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynAggregate", "DynAggregate &AggregateField(const DynAggregate &in aggr, int offset, size_t arrayLength = 1)", asMETHODPR(DynAggregate, AggregateField, (const DynAggregate &, int offset, size_t), DynAggregate &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynAggregate", "void Close()", asMETHODPR(DynAggregate, Close, (), void), asCALL_THISCALL); assert(r >= 0);
}

static void RegisterDynCall(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynCall", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_FACTORY, "DynCall@ f()", asFUNCTIONPR(DynCall::Create, (), DynCall *), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_FACTORY, "DynCall@ f(size_t size)", asFUNCTIONPR(DynCall::Create, (size_t), DynCall *), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_ADDREF, "void f()", asMETHOD(DynCall, AddRef), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_RELEASE, "void f()", asMETHOD(DynCall, Release), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynCall, GetWeakRefFlag), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynCall", "void Reset()", asMETHOD(DynCall, Reset), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynCall", "void SetMode(int mode)", asMETHOD(DynCall, SetMode), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynCall", "DynCall &BeginCallAggr(const DynAggregate&)", asMETHODPR(DynCall, BeginCallAggr, (const DynAggregate &), DynCall &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgBool(bool value)", asFUNCTIONPR([](DynCall *self, bool value) -> DynCall & { return self->ArgBool(value); }, (DynCall *, bool), DynCall &), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgChar(int8 value)", asMETHODPR(DynCall, ArgChar, (char), DynCall &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgShort(int16 value)", asMETHODPR(DynCall, ArgShort, (short), DynCall &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgInt(int value)", asMETHODPR(DynCall, ArgInt, (int), DynCall &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgLong(int32 value)", asMETHODPR(DynCall, ArgLong, (long), DynCall &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgLongLong(int64 value)", asMETHODPR(DynCall, ArgLongLong, (long long), DynCall &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgFloat(float value)", asMETHODPR(DynCall, ArgFloat, (float), DynCall &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgDouble(double value)", asMETHODPR(DynCall, ArgDouble, (double), DynCall &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgPointer(NativePointer value)", asFUNCTIONPR([](DynCall *self, NativePointer value) -> DynCall & { self->ArgPointer(value.Get()); return *self; }, (DynCall *, NativePointer), DynCall &), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgAggregate(const DynAggregate &in ag, NativePointer value)", asFUNCTIONPR([](DynCall *self, const DynAggregate &aggr, NativePointer value) -> DynCall & { self->ArgAggregate(aggr, value.Get()); return *self; }, (DynCall *, const DynAggregate &, NativePointer), DynCall &), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynCall", "void CallVoid(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { self->CallVoid(funcptr.Get()); }, (DynCall *, NativePointer), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "int CallBool(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallBool(funcptr.Get()); }, (DynCall *, NativePointer), int), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "int8 CallChar(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallChar(funcptr.Get()); }, (DynCall *, NativePointer), char), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "int16 CallShort(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallShort(funcptr.Get()); }, (DynCall *, NativePointer), short), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "int CallInt(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallInt(funcptr.Get()); }, (DynCall *, NativePointer), int), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "int32 CallLong(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallLong(funcptr.Get()); }, (DynCall *, NativePointer), long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "int64 CallLongLong(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallLongLong(funcptr.Get()); }, (DynCall *, NativePointer), long long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "float CallFloat(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallFloat(funcptr.Get()); }, (DynCall *, NativePointer), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "double CallDouble(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallDouble(funcptr.Get()); }, (DynCall *, NativePointer), double), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "NativePointer CallPointer(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return NativePointer(self->CallPointer(funcptr.Get())); }, (DynCall *, NativePointer), NativePointer), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCall", "NativePointer CallAggregate(NativePointer, const DynAggregate &, NativePointer)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr, const DynAggregate &aggr, NativePointer ret) { return NativePointer(self->CallAggregate(funcptr.Get(), aggr, ret.Get())); }, (DynCall *, NativePointer, const DynAggregate &, NativePointer), NativePointer), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynCall", "int GetError() const", asFUNCTIONPR([](const DynCall *self) -> int { return self->GetError(); }, (const DynCall *), int), asCALL_CDECL_OBJFIRST); assert(r >= 0);
}

static void RegisterDynValue(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynValue", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "bool GetBool() const", asFUNCTIONPR([](const DCValue *obj) -> bool { return obj->B; }, (const DCValue *), bool), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetBool(bool value)", asFUNCTIONPR([](DCValue *obj, bool b) { obj->B = b; }, (DCValue *, bool), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "int8 GetChar() const", asFUNCTIONPR([](const DCValue *obj) -> char { return obj->c; }, (const DCValue *), char), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetChar(int8 value)", asFUNCTIONPR([](DCValue *obj, char c) { obj->c = c; }, (DCValue *, char), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "uint8 GetUChar() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned char { return obj->C; }, (const DCValue *), unsigned char), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetUChar(uint8 value)", asFUNCTIONPR([](DCValue *obj, unsigned char C) { obj->C = C; }, (DCValue *, unsigned char), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "int16 GetShort() const", asFUNCTIONPR([](const DCValue *obj) -> short { return obj->s; }, (const DCValue *), short), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetShort(int16 value)", asFUNCTIONPR([](DCValue *obj, short s) { obj->s = s; }, (DCValue *, short), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "uint16 GetUShort() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned short { return obj->S; }, (const DCValue *), unsigned short), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetUShort(uint16 value)", asFUNCTIONPR([](DCValue *obj, unsigned short S) { obj->S = S; }, (DCValue *, unsigned short), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "int GetInt() const", asFUNCTIONPR([](const DCValue *obj) -> int { return obj->i; }, (const DCValue *), int), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetInt(int value)", asFUNCTIONPR([](DCValue *obj, int i) { obj->i = i; }, (DCValue *, int), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "uint GetUInt() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned int { return obj->I; }, (const DCValue *), unsigned int), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetUInt(uint value)", asFUNCTIONPR([](DCValue *obj, unsigned int I) { obj->I = I; }, (DCValue *, unsigned int), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "int32 GetLong() const", asFUNCTIONPR([](const DCValue *obj) -> long { return obj->j; }, (const DCValue *), long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetLong(int32 value)", asFUNCTIONPR([](DCValue *obj, long j) { obj->j = j; }, (DCValue *, long), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "uint32 GetULong() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned long { return obj->J; }, (const DCValue *), unsigned long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetULong(uint32 value)", asFUNCTIONPR([](DCValue *obj, unsigned long J) { obj->J = J; }, (DCValue *, unsigned long), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "int64 GetLongLong() const", asFUNCTIONPR([](const DCValue *obj) -> long long { return obj->l; }, (const DCValue *), long long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetLongLong(int64 value)", asFUNCTIONPR([](DCValue *obj, long long l) { obj->l = l; }, (DCValue *, long long), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "uint64 GetULongLong() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned long long { return obj->L; }, (const DCValue *), unsigned long long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetULongLong(uint64 value)", asFUNCTIONPR([](DCValue *obj, unsigned long long L) { obj->L = L; }, (DCValue *, unsigned long long), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "float GetFloat() const", asFUNCTIONPR([](const DCValue *obj) -> float { return obj->f; }, (const DCValue *), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetFloat(float value)", asFUNCTIONPR([](DCValue *obj, float f) { obj->f = f; }, (DCValue *, float), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "double GetDouble() const", asFUNCTIONPR([](const DCValue *obj) -> double { return obj->d; }, (const DCValue *), double), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetDouble(double value)", asFUNCTIONPR([](DCValue *obj, double d) { obj->d = d; }, (DCValue *, double), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "NativePointer GetPointer() const", asFUNCTIONPR([](const DCValue *obj) -> NativePointer { return NativePointer(obj->p); }, (const DCValue *), NativePointer), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetPointer(NativePointer ptr)", asFUNCTIONPR([](DCValue *obj, NativePointer p) { obj->p = p.Get(); }, (DCValue *, NativePointer), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynValue", "string GetString() const", asFUNCTIONPR([](const DCValue *obj) -> std::string { return obj->Z ? std::string(obj->Z) : ""; }, (const DCValue *), std::string), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynValue", "void SetString(NativePointer str)", asFUNCTIONPR([](DCValue *obj, NativePointer str) { obj->Z = str.Get(); }, (DCValue *, NativePointer), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
}

static void RegisterDynArgs(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynArgs", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynArgs", "int ArgBool()", asFUNCTIONPR([](DCArgs *args) -> bool { return dcbArgBool(args); }, (DCArgs *), bool), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "int8 ArgChar()", asFUNCTIONPR([](DCArgs *args) { return dcbArgChar(args); }, (DCArgs *), char), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "int16 ArgShort()", asFUNCTIONPR([](DCArgs *args) { return dcbArgShort(args); }, (DCArgs *), short), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "int ArgInt()", asFUNCTIONPR([](DCArgs *args) { return dcbArgInt(args); }, (DCArgs *), int), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "int32 ArgLong()", asFUNCTIONPR([](DCArgs *args) { return dcbArgLong(args); }, (DCArgs *), long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "int64 ArgLongLong()", asFUNCTIONPR([](DCArgs *args) { return dcbArgLongLong(args); }, (DCArgs *), long long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "uint8 ArgUChar()", asFUNCTIONPR([](DCArgs *args) { return dcbArgUChar(args); }, (DCArgs *), unsigned char), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "uint16 ArgUShort()", asFUNCTIONPR([](DCArgs *args) { return dcbArgUShort(args); }, (DCArgs *), unsigned short), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "uint ArgUInt()", asFUNCTIONPR([](DCArgs *args) { return dcbArgUInt(args); }, (DCArgs *), unsigned int), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "uint32 ArgULong()", asFUNCTIONPR([](DCArgs *args) { return dcbArgULong(args); }, (DCArgs *), unsigned long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "uint64 ArgULongLong()", asFUNCTIONPR([](DCArgs *args) { return dcbArgULongLong(args); }, (DCArgs *), unsigned long long), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "float ArgFloat()", asFUNCTIONPR([](DCArgs *args) { return dcbArgFloat(args); }, (DCArgs *), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "double ArgDouble()", asFUNCTIONPR([](DCArgs *args) { return dcbArgDouble(args); }, (DCArgs *), double), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "NativePointer ArgPointer()", asFUNCTIONPR([](DCArgs *args) { return NativePointer(dcbArgPointer(args)); }, (DCArgs *), NativePointer), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "NativePointer ArgAggregate(NativePointer target)", asFUNCTIONPR([](DCArgs *args, NativePointer target) { return NativePointer(dcbArgAggr(args, target.Get())); }, (DCArgs *, NativePointer), NativePointer), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynArgs", "void ReturnAggregate(DynValue@ result, NativePointer ret)", asFUNCTIONPR([](DCArgs *args, DCValue *result, NativePointer ret) { dcbReturnAggr(args, result, ret.Get()); }, (DCArgs *, DCValue *, NativePointer), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
}

static void RegisterDynCallback(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterFuncdef("int8 DynCallbackHandler(NativePointer pcb, DynArgs &args, DynValue &result)"); assert(r >= 0);

    r = engine->RegisterObjectType("DynCallback", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_FACTORY, "DynCallback@ f(const string &in signature, DynCallbackHandler@ handler)", asFUNCTIONPR(DynCallback::Create, (const std::string &, asIScriptFunction *), DynCallback *), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_FACTORY, "DynCallback@ f(size_t size, DynCallbackHandler@ handler, const DynAggregate &in aggrs)", asFUNCTIONPR(DynCallback::Create, (const std::string &, asIScriptFunction *, const DynAggregate &), DynCallback *), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_ADDREF, "void f()", asMETHOD(DynCallback, AddRef), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_RELEASE, "void f()", asMETHOD(DynCallback, Release), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynCallback, GetWeakRefFlag), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynCallback", "void Init(const string &in signature, DynCallbackHandler@ handler)", asMETHODPR(DynCallback, Init, (const std::string &, asIScriptFunction *), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCallback", "void Init(const string &in signature, DynCallbackHandler@ handler, const DynAggregate &in aggrs)", asMETHODPR(DynCallback, Init, (const std::string &, asIScriptFunction *, const DynAggregate &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynCallback", "NativePointer GetCallback() const", asFUNCTIONPR([](const DynCallback *cb) { return NativePointer(cb->Get()); }, (const DynCallback *), NativePointer), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynCallback", "DynCallbackHandler@ GetHandler() const", asFUNCTIONPR([](const DynCallback *cb) { return cb->GetHandler(); }, (const DynCallback *), asIScriptFunction *), asCALL_CDECL_OBJFIRST); assert(r >= 0);
}

static void RegisterDynLibrary(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynLibrary", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynLibrary", asBEHAVE_FACTORY, "DynLibrary@ f()", asFUNCTIONPR(DynLibrary::Create, (), DynLibrary *), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("DynLibrary", asBEHAVE_ADDREF, "void f()", asMETHOD(DynLibrary, AddRef), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynLibrary", asBEHAVE_RELEASE, "void f()", asMETHOD(DynLibrary, Release), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynLibrary", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynLibrary, GetWeakRefFlag), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynLibrary", "bool IsLoaded() const", asMETHODPR(DynLibrary, IsLoaded, () const, bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynLibrary", "bool Load(const string &in libPath)", asMETHODPR(DynLibrary, Load, (const std::string &), bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynLibrary", "NativePointer FindSymbol(const string &in symbolName) const", asFUNCTIONPR([](const DynLibrary *lib, const std::string &symbolName) { return NativePointer(lib->FindSymbol(symbolName)); }, (const DynLibrary *, const std::string &), NativePointer), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynLibrary", "string GetLibraryPath() const", asMETHODPR(DynLibrary, GetLibraryPath, () const, std::string), asCALL_THISCALL); assert(r >= 0);
}

static void RegisterDynSymbols(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynSymbols", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynSymbols", asBEHAVE_FACTORY, "DynSymbols@ f()", asFUNCTIONPR(DynSymbols::Create, (), DynSymbols *), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("DynSymbols", asBEHAVE_ADDREF, "void f()", asMETHOD(DynSymbols, AddRef), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynSymbols", asBEHAVE_RELEASE, "void f()", asMETHOD(DynSymbols, Release), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("DynSymbols", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynSymbols, GetWeakRefFlag), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynSymbols", "bool IsInited() const", asMETHODPR(DynSymbols, IsInited, () const, bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynSymbols", "bool Init(const string &in libPath)", asMETHODPR(DynSymbols, Init, (const std::string &), bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("DynSymbols", "int GetCount() const", asMETHODPR(DynSymbols, GetCount, () const, int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynSymbols", "string GetName(int index) const", asMETHODPR(DynSymbols, GetName, (int) const, std::string), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("DynSymbols", "string GetName(NativePointer value) const", asFUNCTIONPR([](const DynSymbols *syms, NativePointer value) { return syms->GetNameByValue(value.Get()); }, (const DynSymbols *, NativePointer), std::string), asCALL_CDECL_OBJFIRST); assert(r >= 0);
}

void RegisterScriptDynCall(asIScriptEngine *engine) {
    RegisterDCEnums(engine);
    RegisterDCSigChars(engine);
    RegisterDynAggregate(engine);
    RegisterDynCall(engine);

    int r = engine->RegisterGlobalFunction("int DynGetModeFromCCSigChar(int8 sigChar)", asFUNCTION(dcGetModeFromCCSigChar), asCALL_CDECL); assert(r >= 0);
}

void RegisterScriptDynCallback(asIScriptEngine *engine) {
    RegisterDynValue(engine);
    RegisterDynArgs(engine);
    RegisterDynCallback(engine);
}

void RegisterScriptDynLoad(asIScriptEngine *engine) {
    RegisterDynLibrary(engine);
    RegisterDynSymbols(engine);
}
