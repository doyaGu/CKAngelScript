#include "ScriptDynCall.h"

#include <cassert>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <dyncall.h>
#include <dyncall_callback.h>
#include <dynload.h>

#include "RefCount.h"
#include "ScriptNativePointer.h"
#include "ScriptRegistration.h"

namespace {

void SetActiveScriptException(const char *message) {
    if (auto *ctx = asGetActiveContext())
        ctx->SetException(message);
}

#if defined(DC__Feature_AggrByVal)
constexpr bool kDyncallSupportsAggregateByValue = true;
#else
constexpr bool kDyncallSupportsAggregateByValue = false;
#endif

constexpr const char *kUnsupportedAggregateByValueMessage =
    "Dyncall aggregate-by-value calls are not supported on this platform.";

} // namespace

class DynAggregate {
public:
    static DynAggregate *Create(size_t maxFieldCount, size_t size) {
        void *self = asAllocMem(sizeof(DynAggregate));
        return new(self) DynAggregate(maxFieldCount, size);
    }

    DynAggregate(size_t maxFieldCount, size_t size)
        : aggr(dcNewAggr(maxFieldCount, size)), m_MaxFieldCount(maxFieldCount) {
        if (!aggr) {
            SetActiveScriptException("Failed to create aggregate.");
        }
    }

    ~DynAggregate() {
        if (aggr)
            dcFreeAggr(aggr);
        for (const DynAggregate *nested : m_NestedAggregates) {
            if (nested)
                nested->Release();
        }
        if (m_WeakRefFlag) {
            m_WeakRefFlag->Set(true);
            m_WeakRefFlag->Release();
        }
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
        if (!CanAddField())
            return *this;
        dcAggrField(aggr, type, offset, arrayLength);
        ++m_FieldCount;
        return *this;
    }

    DynAggregate &AggregateField(const DynAggregate &nestedAggr, int offset, size_t arrayLength = 1) {
        if (!CanAddField())
            return *this;
        if (!nestedAggr.Get()) {
            SetActiveScriptException("Nested DynAggregate has no native descriptor.");
            return *this;
        }
        if (&nestedAggr == this || nestedAggr.ContainsNestedAggregate(this)) {
            SetActiveScriptException("DynAggregate nesting cycle is not allowed.");
            return *this;
        }
        m_NestedAggregates.push_back(&nestedAggr);
        nestedAggr.AddRef();
        dcAggrField(aggr, DC_SIGCHAR_AGGREGATE, offset, arrayLength, nestedAggr.Get());
        ++m_FieldCount;
        return *this;
    }

    void Close() {
        if (!aggr || m_Closed)
            return;
        dcCloseAggr(aggr);
        m_Closed = true;
    }

    DCaggr *aggr;

private:
    bool CanAddField() const {
        if (!aggr) {
            SetActiveScriptException("DynAggregate has no native descriptor.");
            return false;
        }
        if (m_Closed) {
            SetActiveScriptException("Cannot add fields to a closed DynAggregate.");
            return false;
        }
        if (m_FieldCount >= m_MaxFieldCount) {
            SetActiveScriptException("DynAggregate field capacity exceeded.");
            return false;
        }
        return true;
    }

    bool ContainsNestedAggregate(const DynAggregate *candidate) const {
        for (const DynAggregate *nested : m_NestedAggregates) {
            if (nested == candidate || (nested && nested->ContainsNestedAggregate(candidate)))
                return true;
        }
        return false;
    }

    mutable RefCount m_RefCount;
    asILockableSharedBool *m_WeakRefFlag = nullptr;
    size_t m_MaxFieldCount = 0;
    size_t m_FieldCount = 0;
    bool m_Closed = false;
    std::vector<const DynAggregate *> m_NestedAggregates;
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
        if (m_WeakRefFlag) {
            m_WeakRefFlag->Set(true);
            m_WeakRefFlag->Release();
        }
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

    DynCall &Reset() {
        dcReset(vm);
        return *this;
    }

    DynCall &Mode(int mode) {
        dcMode(vm, mode);
        return *this;
    }

    DynCall &BeginCallAggr(const DynAggregate &aggr) {
        if (!kDyncallSupportsAggregateByValue) {
            SetActiveScriptException(kUnsupportedAggregateByValueMessage);
            return *this;
        }
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

    DynCall &ArgString(std::string &value) {
        dcArgPointer(vm, value.data());
        return *this;
    }

    DynCall &ArgAggregate(const DynAggregate &aggr, const void *value) {
        if (!kDyncallSupportsAggregateByValue) {
            SetActiveScriptException(kUnsupportedAggregateByValueMessage);
            return *this;
        }
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

    std::string CallString(void *funcptr) {
        auto *str = static_cast<char *>(dcCallPointer(vm, funcptr));
        return str ? str : "";
    }

    void *CallAggregate(void *funcptr, const DynAggregate &aggr, void *ret) {
        if (!kDyncallSupportsAggregateByValue) {
            SetActiveScriptException(kUnsupportedAggregateByValueMessage);
            return nullptr;
        }
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
        if (!func) {
            SetActiveScriptException("DynCallback requires a handler.");
            return nullptr;
        }

        void *self = asAllocMem(sizeof(DynCallback));
        auto *callback = new(self) DynCallback(signature, func);
        callback->NotifyGarbageCollector();
        return callback;
    }

    static DynCallback *Create(const std::string &signature, asIScriptFunction *func, const DynAggregate &aggregate) {
        if (!kDyncallSupportsAggregateByValue) {
            SetActiveScriptException(kUnsupportedAggregateByValueMessage);
            return nullptr;
        }
        if (!func) {
            SetActiveScriptException("DynCallback requires a handler.");
            return nullptr;
        }

        void *self = asAllocMem(sizeof(DynCallback));
        auto *callback = new(self) DynCallback(signature, func, aggregate);
        callback->NotifyGarbageCollector();
        return callback;
    }

    DynCallback(std::string signature, asIScriptFunction *func)
        : m_Signature(std::move(signature)) {
        RetainHandler(func);
        callback = dcbNewCallback(m_Signature.c_str(), ScriptCallbackHandler, m_Handler);
        if (!callback) {
            ReleaseHandler();
            SetActiveScriptException("Failed to create callback.");
        }
    }

    DynCallback(std::string signature, asIScriptFunction *func, const DynAggregate &aggregate)
        : m_Signature(std::move(signature)) {
        if (!kDyncallSupportsAggregateByValue) {
            SetActiveScriptException(kUnsupportedAggregateByValueMessage);
            return;
        }
        RetainHandler(func);
        aggregate.AddRef();
        m_Aggregate = &aggregate;
        callback = dcbNewCallback2(m_Signature.c_str(), ScriptCallbackHandler, m_Handler, &aggregate.aggr);
        if (!callback) {
            ReleaseAggregate();
            ReleaseHandler();
            SetActiveScriptException("Failed to create callback.");
        }
    }

    ~DynCallback() {
        ReleaseHandler();
        ReleaseAggregate();
        if (callback) {
            dcbFreeCallback(callback);
        }
        if (m_WeakRefFlag) {
            m_WeakRefFlag->Set(true);
            m_WeakRefFlag->Release();
        }
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
        m_GCFlag = false;
        return m_RefCount.AddRef();
    }

    int Release() const {
        m_GCFlag = false;
        const int r = m_RefCount.Release();
        if (r == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
            this->~DynCallback();
            asFreeMem(const_cast<DynCallback *>(this));
        }
        return r;
    }

    int GetRefCount() const {
        return const_cast<RefCount &>(m_RefCount).GetCount();
    }

    void SetGCFlag() const {
        m_GCFlag = true;
    }

    bool GetGCFlag() const {
        return m_GCFlag;
    }

    void EnumReferences(asIScriptEngine *engine) const {
        if (engine && m_Handler)
            engine->GCEnumCallback(m_Handler);
    }

    void ReleaseAllReferences(asIScriptEngine *) {
        ReleaseHandler();
        ReinitCallbackUserData();
    }

    asILockableSharedBool *GetWeakRefFlag() {
        if(!m_WeakRefFlag)
            m_WeakRefFlag = asCreateLockableSharedBool();
        return m_WeakRefFlag;
    }

    void Init(const std::string &signature, asIScriptFunction *func) {
        if (!callback) {
            SetActiveScriptException("Callback is not initialized.");
            return;
        }
        if (!func) {
            SetActiveScriptException("DynCallback.Init requires a handler.");
            return;
        }

        func->AddRef();
        ReleaseHandler();
        ReleaseAggregate();
        m_Signature = signature;
        m_Handler = func;
        dcbInitCallback(callback, m_Signature.c_str(), ScriptCallbackHandler, m_Handler);
    }

    void Init(const std::string &signature, asIScriptFunction *func, const DynAggregate &aggregate) {
        if (!kDyncallSupportsAggregateByValue) {
            SetActiveScriptException(kUnsupportedAggregateByValueMessage);
            return;
        }
        if (!callback) {
            SetActiveScriptException("Callback is not initialized.");
            return;
        }
        if (!func) {
            SetActiveScriptException("DynCallback.Init requires a handler.");
            return;
        }

        func->AddRef();
        aggregate.AddRef();
        ReleaseHandler();
        ReleaseAggregate();
        m_Signature = signature;
        m_Handler = func;
        m_Aggregate = &aggregate;
        dcbInitCallback2(callback, m_Signature.c_str(), ScriptCallbackHandler, m_Handler, &aggregate.aggr);
    }

    asIScriptFunction *GetHandler() const {
        if (!m_Handler) {
            return nullptr;
        }
        m_Handler->AddRef();
        return m_Handler;
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
        if (!ctx)
            return DC_SIGCHAR_VOID;

        auto finish = [engine, ctx](DCsigchar signature) {
            ctx->Unprepare();
            engine->ReturnContext(ctx);
            return signature;
        };

        int r = 0;
        if (func->GetFuncType() == asFUNC_DELEGATE) {
            asIScriptFunction *callback = func->GetDelegateFunction();
            void *callbackObject = func->GetDelegateObject();
            r = ctx->Prepare(callback);
            if (r >= 0)
                ctx->SetObject(callbackObject);
        } else {
            r = ctx->Prepare(func);
        }

        if (r < 0) {
            engine->ReturnContext(ctx);
            return DC_SIGCHAR_VOID;
        }

        NativePointer callbackPointer(cb);
        r = ctx->SetArgObject(0, &callbackPointer);
        if (r < 0)
            return finish(DC_SIGCHAR_VOID);
        r = ctx->SetArgObject(1, args);
        if (r < 0)
            return finish(DC_SIGCHAR_VOID);
        r = ctx->SetArgObject(2, result);
        if (r < 0)
            return finish(DC_SIGCHAR_VOID);

        r = ctx->Execute();
        if (r != asEXECUTION_FINISHED)
            return finish(DC_SIGCHAR_VOID);

        auto signature = static_cast<DCsigchar>(ctx->GetReturnByte());
        return finish(signature);
    }

    DCCallback *callback = nullptr;

private:
    void NotifyGarbageCollector() {
        auto *ctx = asGetActiveContext();
        if (!ctx)
            return;

        asIScriptEngine *engine = ctx->GetEngine();
        if (!engine)
            return;

        if (asITypeInfo *type = engine->GetTypeInfoByDecl("DynCallback"))
            engine->NotifyGarbageCollectorOfNewObject(this, type);
    }

    void ReleaseHandler() {
        if (m_Handler) {
            m_Handler->Release();
            m_Handler = nullptr;
        }
    }

    void RetainHandler(asIScriptFunction *handler) {
        m_Handler = handler;
        if (m_Handler)
            m_Handler->AddRef();
    }

    void ReleaseAggregate() {
        if (m_Aggregate) {
            m_Aggregate->Release();
            m_Aggregate = nullptr;
        }
    }

    void ReinitCallbackUserData() {
        if (!callback)
            return;
        if (m_Aggregate) {
            dcbInitCallback2(callback, m_Signature.c_str(), ScriptCallbackHandler, nullptr, &m_Aggregate->aggr);
        } else {
            dcbInitCallback(callback, m_Signature.c_str(), ScriptCallbackHandler, nullptr);
        }
    }

    mutable RefCount m_RefCount;
    asILockableSharedBool *m_WeakRefFlag = nullptr;
    mutable bool m_GCFlag = false;
    std::string m_Signature;
    asIScriptFunction *m_Handler = nullptr;
    const DynAggregate *m_Aggregate = nullptr;
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
        if (m_WeakRefFlag) {
            m_WeakRefFlag->Set(true);
            m_WeakRefFlag->Release();
        }
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
            return nullptr;
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
        if (m_WeakRefFlag) {
            m_WeakRefFlag->Set(true);
            m_WeakRefFlag->Release();
        }
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
        const char *name = dlSymsName(syms, index);
        return name ? std::string(name) : std::string();
    }

    std::string GetNameByValue(void *value) const {
        if (!syms) {
            return "";
        }
        const char *name = dlSymsNameFromValue(syms, value);
        return name ? std::string(name) : std::string();
    }

    DLSyms *syms;

private:
    mutable RefCount m_RefCount;
    asILockableSharedBool *m_WeakRefFlag = nullptr;
};

static void RetainDynLibraryOwner(void *owner) {
    if (owner) {
        static_cast<DynLibrary *>(owner)->AddRef();
    }
}

static void ReleaseDynLibraryOwner(void *owner) {
    if (owner) {
        static_cast<DynLibrary *>(owner)->Release();
    }
}

static void RegisterDCEnums(asIScriptEngine* engine) {
    int r = 0;

    r = engine->RegisterEnum("DCCallMode"); CKAS_CHECK_REGISTER(r);

    // Default calling modes
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_DEFAULT", DC_CALL_C_DEFAULT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_DEFAULT_THIS", DC_CALL_C_DEFAULT_THIS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ELLIPSIS", DC_CALL_C_ELLIPSIS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ELLIPSIS_VARARGS", DC_CALL_C_ELLIPSIS_VARARGS); CKAS_CHECK_REGISTER(r);

    // Platform-specific calling modes
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_CDECL", DC_CALL_C_X86_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_STD", DC_CALL_C_X86_WIN32_STD); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_FAST_MS", DC_CALL_C_X86_WIN32_FAST_MS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_FAST_GNU", DC_CALL_C_X86_WIN32_FAST_GNU); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_THIS_MS", DC_CALL_C_X86_WIN32_THIS_MS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_WIN32_THIS_GNU", DC_CALL_C_X86_WIN32_THIS_GNU); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X64_WIN64", DC_CALL_C_X64_WIN64); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X64_WIN64_THIS", DC_CALL_C_X64_WIN64_THIS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X64_SYSV", DC_CALL_C_X64_SYSV); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X64_SYSV_THIS", DC_CALL_C_X64_SYSV_THIS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC32_DARWIN", DC_CALL_C_PPC32_DARWIN); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC32_OSX", DC_CALL_C_PPC32_OSX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_ARM_EABI", DC_CALL_C_ARM_ARM_EABI); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_THUMB_EABI", DC_CALL_C_ARM_THUMB_EABI); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_ARMHF", DC_CALL_C_ARM_ARMHF); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS32_EABI", DC_CALL_C_MIPS32_EABI); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS32_PSPSDK", DC_CALL_C_MIPS32_PSPSDK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC32_SYSV", DC_CALL_C_PPC32_SYSV); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC32_LINUX", DC_CALL_C_PPC32_LINUX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_ARM", DC_CALL_C_ARM_ARM); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM_THUMB", DC_CALL_C_ARM_THUMB); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS32_O32", DC_CALL_C_MIPS32_O32); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS64_N32", DC_CALL_C_MIPS64_N32); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_MIPS64_N64", DC_CALL_C_MIPS64_N64); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_X86_PLAN9", DC_CALL_C_X86_PLAN9); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_SPARC32", DC_CALL_C_SPARC32); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_SPARC64", DC_CALL_C_SPARC64); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_ARM64", DC_CALL_C_ARM64); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC64", DC_CALL_C_PPC64); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_C_PPC64_LINUX", DC_CALL_C_PPC64_LINUX); CKAS_CHECK_REGISTER(r);

    // Default syscall mode
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_DEFAULT", DC_CALL_SYS_DEFAULT); CKAS_CHECK_REGISTER(r);

    // Platform-specific syscall modes
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_X86_INT80H_LINUX", DC_CALL_SYS_X86_INT80H_LINUX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_X86_INT80H_BSD", DC_CALL_SYS_X86_INT80H_BSD); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_X64_SYSCALL_SYSV", DC_CALL_SYS_X64_SYSCALL_SYSV); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_PPC32", DC_CALL_SYS_PPC32); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCCallMode", "DC_CALL_SYS_PPC64", DC_CALL_SYS_PPC64); CKAS_CHECK_REGISTER(r);

    // Error codes
    r = engine->RegisterEnum("DCError"); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCError", "DC_ERROR_NONE", DC_ERROR_NONE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterEnumValue("DCError", "DC_ERROR_UNSUPPORTED_MODE", DC_ERROR_UNSUPPORTED_MODE); CKAS_CHECK_REGISTER(r);
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

    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_VOID", (void*)&g_DC_SIGCHAR_VOID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_BOOL", (void*)&g_DC_SIGCHAR_BOOL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CHAR", (void*)&g_DC_SIGCHAR_CHAR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_UCHAR", (void*)&g_DC_SIGCHAR_UCHAR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_SHORT", (void*)&g_DC_SIGCHAR_SHORT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_USHORT", (void*)&g_DC_SIGCHAR_USHORT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_INT", (void*)&g_DC_SIGCHAR_INT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_UINT", (void*)&g_DC_SIGCHAR_UINT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_LONG", (void*)&g_DC_SIGCHAR_LONG); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_ULONG", (void*)&g_DC_SIGCHAR_ULONG); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_LONGLONG", (void*)&g_DC_SIGCHAR_LONGLONG); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_ULONGLONG", (void*)&g_DC_SIGCHAR_ULONGLONG); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_FLOAT", (void*)&g_DC_SIGCHAR_FLOAT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_DOUBLE", (void*)&g_DC_SIGCHAR_DOUBLE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_POINTER", (void*)&g_DC_SIGCHAR_POINTER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_STRING", (void*)&g_DC_SIGCHAR_STRING); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_AGGREGATE", (void*)&g_DC_SIGCHAR_AGGREGATE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_ENDARG", (void*)&g_DC_SIGCHAR_ENDARG); CKAS_CHECK_REGISTER(r);

    // Calling convention/mode signature
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_PREFIX", (void*)&g_DC_SIGCHAR_CC_PREFIX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_DEFAULT", (void*)&g_DC_SIGCHAR_CC_DEFAULT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_THISCALL", (void*)&g_DC_SIGCHAR_CC_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_ELLIPSIS", (void*)&g_DC_SIGCHAR_CC_ELLIPSIS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_ELLIPSIS_VARARGS", (void*)&g_DC_SIGCHAR_CC_ELLIPSIS_VARARGS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_CDECL", (void*)&g_DC_SIGCHAR_CC_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_STDCALL", (void*)&g_DC_SIGCHAR_CC_STDCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_FASTCALL_MS", (void*)&g_DC_SIGCHAR_CC_FASTCALL_MS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_FASTCALL_GNU", (void*)&g_DC_SIGCHAR_CC_FASTCALL_GNU); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_THISCALL_MS", (void*)&g_DC_SIGCHAR_CC_THISCALL_MS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_THISCALL_GNU", (void*)&g_DC_SIGCHAR_CC_THISCALL_GNU); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_ARM_ARM", (void*)&g_DC_SIGCHAR_CC_ARM_ARM); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_ARM_THUMB", (void*)&g_DC_SIGCHAR_CC_ARM_THUMB); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int8 DC_SIGCHAR_CC_SYSCALL", (void*)&g_DC_SIGCHAR_CC_SYSCALL); CKAS_CHECK_REGISTER(r);
}

void RegisterDynAggregate(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynAggregate", 0, asOBJ_REF); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynAggregate", asBEHAVE_FACTORY, "DynAggregate@ f(size_t buf, size_t size)", asFUNCTIONPR(DynAggregate::Create, (size_t, size_t), DynAggregate *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("DynAggregate", asBEHAVE_ADDREF, "void f()", asMETHOD(DynAggregate, AddRef), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynAggregate", asBEHAVE_RELEASE, "void f()", asMETHOD(DynAggregate, Release), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynAggregate", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynAggregate, GetWeakRefFlag), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynAggregate", "DynAggregate &Field(int8 type, int offset, size_t arrayLength = 1)", asMETHODPR(DynAggregate, Field, (char type, int offset, size_t), DynAggregate &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynAggregate", "DynAggregate &AggregateField(const DynAggregate &in aggr, int offset, size_t arrayLength = 1)", asMETHODPR(DynAggregate, AggregateField, (const DynAggregate &, int offset, size_t), DynAggregate &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynAggregate", "void Close()", asMETHODPR(DynAggregate, Close, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

static void RegisterDynCall(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynCall", 0, asOBJ_REF); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_FACTORY, "DynCall@ f()", asFUNCTIONPR(DynCall::Create, (), DynCall *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_FACTORY, "DynCall@ f(size_t size)", asFUNCTIONPR(DynCall::Create, (size_t), DynCall *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_ADDREF, "void f()", asMETHOD(DynCall, AddRef), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_RELEASE, "void f()", asMETHOD(DynCall, Release), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCall", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynCall, GetWeakRefFlag), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynCall", "DynCall &Reset()", asMETHOD(DynCall, Reset), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynCall", "DynCall &Mode(int mode)", asMETHOD(DynCall, Mode), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynCall", "DynCall &BeginCallAggr(const DynAggregate &in aggr)", asMETHODPR(DynCall, BeginCallAggr, (const DynAggregate &), DynCall &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgBool(bool value)", asFUNCTIONPR([](DynCall *self, bool value) -> DynCall & { return self->ArgBool(value); }, (DynCall *, bool), DynCall &), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgChar(int8 value)", asMETHODPR(DynCall, ArgChar, (char), DynCall &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgShort(int16 value)", asMETHODPR(DynCall, ArgShort, (short), DynCall &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgInt(int value)", asMETHODPR(DynCall, ArgInt, (int), DynCall &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgLong(int32 value)", asMETHODPR(DynCall, ArgLong, (long), DynCall &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgLongLong(int64 value)", asMETHODPR(DynCall, ArgLongLong, (long long), DynCall &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgFloat(float value)", asMETHODPR(DynCall, ArgFloat, (float), DynCall &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgDouble(double value)", asMETHODPR(DynCall, ArgDouble, (double), DynCall &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgPointer(NativePointer value)", asFUNCTIONPR([](DynCall *self, NativePointer value) -> DynCall & { self->ArgPointer(value.Get()); return *self; }, (DynCall *, NativePointer), DynCall &), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgString(string &in value)", asFUNCTIONPR([](DynCall *self, std::string &value) -> DynCall & { self->ArgString(value); return *self; }, (DynCall *, std::string &), DynCall &), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "DynCall &ArgAggregate(const DynAggregate &in ag, NativePointer value)", asFUNCTIONPR([](DynCall *self, const DynAggregate &aggr, NativePointer value) -> DynCall & { self->ArgAggregate(aggr, value.Get()); return *self; }, (DynCall *, const DynAggregate &, NativePointer), DynCall &), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynCall", "void CallVoid(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { self->CallVoid(funcptr.Get()); }, (DynCall *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "int CallBool(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallBool(funcptr.Get()); }, (DynCall *, NativePointer), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "int8 CallChar(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallChar(funcptr.Get()); }, (DynCall *, NativePointer), char), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "int16 CallShort(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallShort(funcptr.Get()); }, (DynCall *, NativePointer), short), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "int CallInt(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallInt(funcptr.Get()); }, (DynCall *, NativePointer), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "int32 CallLong(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallLong(funcptr.Get()); }, (DynCall *, NativePointer), long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "int64 CallLongLong(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallLongLong(funcptr.Get()); }, (DynCall *, NativePointer), long long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "float CallFloat(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallFloat(funcptr.Get()); }, (DynCall *, NativePointer), float), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "double CallDouble(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return self->CallDouble(funcptr.Get()); }, (DynCall *, NativePointer), double), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "NativePointer CallPointer(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr) { return NativePointer(self->CallPointer(funcptr.Get())); }, (DynCall *, NativePointer), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "string CallString(NativePointer funcptr)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr)  { return self->CallString(funcptr.Get()); }, (DynCall *, NativePointer), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCall", "NativePointer CallAggregate(NativePointer funcptr, const DynAggregate &in aggr, NativePointer ret)", asFUNCTIONPR([](DynCall *self, NativePointer funcptr, const DynAggregate &aggr, NativePointer ret) { return NativePointer(self->CallAggregate(funcptr.Get(), aggr, ret.Get())); }, (DynCall *, NativePointer, const DynAggregate &, NativePointer), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynCall", "int GetError() const", asFUNCTIONPR([](const DynCall *self) -> int { return self->GetError(); }, (const DynCall *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

static void RegisterDynValue(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynValue", 0, asOBJ_REF | asOBJ_NOCOUNT); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "bool GetBool() const", asFUNCTIONPR([](const DCValue *obj) -> bool { return obj->B; }, (const DCValue *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetBool(bool value)", asFUNCTIONPR([](DCValue *obj, bool b) { obj->B = b; }, (DCValue *, bool), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "int8 GetChar() const", asFUNCTIONPR([](const DCValue *obj) -> char { return obj->c; }, (const DCValue *), char), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetChar(int8 value)", asFUNCTIONPR([](DCValue *obj, char c) { obj->c = c; }, (DCValue *, char), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "uint8 GetUChar() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned char { return obj->C; }, (const DCValue *), unsigned char), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetUChar(uint8 value)", asFUNCTIONPR([](DCValue *obj, unsigned char C) { obj->C = C; }, (DCValue *, unsigned char), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "int16 GetShort() const", asFUNCTIONPR([](const DCValue *obj) -> short { return obj->s; }, (const DCValue *), short), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetShort(int16 value)", asFUNCTIONPR([](DCValue *obj, short s) { obj->s = s; }, (DCValue *, short), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "uint16 GetUShort() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned short { return obj->S; }, (const DCValue *), unsigned short), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetUShort(uint16 value)", asFUNCTIONPR([](DCValue *obj, unsigned short S) { obj->S = S; }, (DCValue *, unsigned short), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "int GetInt() const", asFUNCTIONPR([](const DCValue *obj) -> int { return obj->i; }, (const DCValue *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetInt(int value)", asFUNCTIONPR([](DCValue *obj, int i) { obj->i = i; }, (DCValue *, int), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "uint GetUInt() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned int { return obj->I; }, (const DCValue *), unsigned int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetUInt(uint value)", asFUNCTIONPR([](DCValue *obj, unsigned int I) { obj->I = I; }, (DCValue *, unsigned int), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "int32 GetLong() const", asFUNCTIONPR([](const DCValue *obj) -> long { return obj->j; }, (const DCValue *), long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetLong(int32 value)", asFUNCTIONPR([](DCValue *obj, long j) { obj->j = j; }, (DCValue *, long), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "uint32 GetULong() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned long { return obj->J; }, (const DCValue *), unsigned long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetULong(uint32 value)", asFUNCTIONPR([](DCValue *obj, unsigned long J) { obj->J = J; }, (DCValue *, unsigned long), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "int64 GetLongLong() const", asFUNCTIONPR([](const DCValue *obj) -> long long { return obj->l; }, (const DCValue *), long long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetLongLong(int64 value)", asFUNCTIONPR([](DCValue *obj, long long l) { obj->l = l; }, (DCValue *, long long), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "uint64 GetULongLong() const", asFUNCTIONPR([](const DCValue *obj) -> unsigned long long { return obj->L; }, (const DCValue *), unsigned long long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetULongLong(uint64 value)", asFUNCTIONPR([](DCValue *obj, unsigned long long L) { obj->L = L; }, (DCValue *, unsigned long long), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "float GetFloat() const", asFUNCTIONPR([](const DCValue *obj) -> float { return obj->f; }, (const DCValue *), float), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetFloat(float value)", asFUNCTIONPR([](DCValue *obj, float f) { obj->f = f; }, (DCValue *, float), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "double GetDouble() const", asFUNCTIONPR([](const DCValue *obj) -> double { return obj->d; }, (const DCValue *), double), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetDouble(double value)", asFUNCTIONPR([](DCValue *obj, double d) { obj->d = d; }, (DCValue *, double), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "NativePointer GetPointer() const", asFUNCTIONPR([](const DCValue *obj) -> NativePointer { return NativePointer(obj->p); }, (const DCValue *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetPointer(NativePointer value)", asFUNCTIONPR([](DCValue *obj, NativePointer p) { obj->p = p.Get(); }, (DCValue *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynValue", "string GetString() const", asFUNCTIONPR([](const DCValue *obj) -> std::string { return obj->Z ? std::string(obj->Z) : ""; }, (const DCValue *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynValue", "void SetString(const string &in value)", asFUNCTIONPR([](DCValue *obj, const std::string &str) { static thread_local std::string value; value = str; obj->Z = value.c_str(); }, (DCValue *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

static void RegisterDynArgs(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynArgs", 0, asOBJ_REF | asOBJ_NOHANDLE); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynArgs", "bool ArgBool()", asFUNCTIONPR([](DCArgs *args) -> bool { return dcbArgBool(args); }, (DCArgs *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "int8 ArgChar()", asFUNCTIONPR([](DCArgs *args) { return dcbArgChar(args); }, (DCArgs *), char), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "int16 ArgShort()", asFUNCTIONPR([](DCArgs *args) { return dcbArgShort(args); }, (DCArgs *), short), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "int ArgInt()", asFUNCTIONPR([](DCArgs *args) { return dcbArgInt(args); }, (DCArgs *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "int32 ArgLong()", asFUNCTIONPR([](DCArgs *args) { return dcbArgLong(args); }, (DCArgs *), long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "int64 ArgLongLong()", asFUNCTIONPR([](DCArgs *args) { return dcbArgLongLong(args); }, (DCArgs *), long long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "uint8 ArgUChar()", asFUNCTIONPR([](DCArgs *args) { return dcbArgUChar(args); }, (DCArgs *), unsigned char), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "uint16 ArgUShort()", asFUNCTIONPR([](DCArgs *args) { return dcbArgUShort(args); }, (DCArgs *), unsigned short), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "uint ArgUInt()", asFUNCTIONPR([](DCArgs *args) { return dcbArgUInt(args); }, (DCArgs *), unsigned int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "uint32 ArgULong()", asFUNCTIONPR([](DCArgs *args) { return dcbArgULong(args); }, (DCArgs *), unsigned long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "uint64 ArgULongLong()", asFUNCTIONPR([](DCArgs *args) { return dcbArgULongLong(args); }, (DCArgs *), unsigned long long), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "float ArgFloat()", asFUNCTIONPR([](DCArgs *args) { return dcbArgFloat(args); }, (DCArgs *), float), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "double ArgDouble()", asFUNCTIONPR([](DCArgs *args) { return dcbArgDouble(args); }, (DCArgs *), double), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "NativePointer ArgPointer()", asFUNCTIONPR([](DCArgs *args) { return NativePointer(dcbArgPointer(args)); }, (DCArgs *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "string ArgString()", asFUNCTIONPR([](DCArgs *args) -> std::string { auto *str = static_cast<char *>(dcbArgPointer(args)); return str ? str : ""; }, (DCArgs *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "NativePointer ArgAggregate(NativePointer target)", asFUNCTIONPR([](DCArgs *args, NativePointer target) {
        if (!kDyncallSupportsAggregateByValue) {
            SetActiveScriptException(kUnsupportedAggregateByValueMessage);
            return NativePointer();
        }
        return NativePointer(dcbArgAggr(args, target.Get()));
    }, (DCArgs *, NativePointer), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynArgs", "void ReturnAggregate(DynValue@ result, NativePointer ret)", asFUNCTIONPR([](DCArgs *args, DCValue *result, NativePointer ret) {
        if (!kDyncallSupportsAggregateByValue) {
            SetActiveScriptException(kUnsupportedAggregateByValueMessage);
            return;
        }
        if (!result) {
            if (auto *ctx = asGetActiveContext()) {
                ctx->SetException("DynArgs.ReturnAggregate requires a DynValue result.");
            }
            return;
        }
        dcbReturnAggr(args, result, ret.Get());
    }, (DCArgs *, DCValue *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

static void RegisterDynCallback(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterFuncdef("int8 DynCallbackHandler(NativePointer pcb, DynArgs &args, DynValue &result)"); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectType("DynCallback", 0, asOBJ_REF | asOBJ_GC); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_FACTORY, "DynCallback@ f(const string &in signature, DynCallbackHandler@+ handler)", asFUNCTIONPR(DynCallback::Create, (const std::string &, asIScriptFunction *), DynCallback *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_FACTORY, "DynCallback@ f(const string &in signature, DynCallbackHandler@+ handler, const DynAggregate &in aggrs)", asFUNCTIONPR(DynCallback::Create, (const std::string &, asIScriptFunction *, const DynAggregate &), DynCallback *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_ADDREF, "void f()", asMETHOD(DynCallback, AddRef), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_RELEASE, "void f()", asMETHOD(DynCallback, Release), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynCallback, GetWeakRefFlag), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(DynCallback, GetRefCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(DynCallback, SetGCFlag), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(DynCallback, GetGCFlag), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(DynCallback, EnumReferences), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynCallback", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(DynCallback, ReleaseAllReferences), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynCallback", "void Init(const string &in signature, DynCallbackHandler@+ handler)", asMETHODPR(DynCallback, Init, (const std::string &, asIScriptFunction *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCallback", "void Init(const string &in signature, DynCallbackHandler@+ handler, const DynAggregate &in aggrs)", asMETHODPR(DynCallback, Init, (const std::string &, asIScriptFunction *, const DynAggregate &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynCallback", "NativePointer GetCallback() const", asFUNCTIONPR([](const DynCallback *cb) { return NativePointer(cb->Get()); }, (const DynCallback *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynCallback", "DynCallbackHandler@ GetHandler() const", asFUNCTIONPR([](const DynCallback *cb) { return cb->GetHandler(); }, (const DynCallback *), asIScriptFunction *), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

static void RegisterDynLibrary(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynLibrary", 0, asOBJ_REF); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynLibrary", asBEHAVE_FACTORY, "DynLibrary@ f()", asFUNCTIONPR(DynLibrary::Create, (), DynLibrary *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("DynLibrary", asBEHAVE_ADDREF, "void f()", asMETHOD(DynLibrary, AddRef), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynLibrary", asBEHAVE_RELEASE, "void f()", asMETHOD(DynLibrary, Release), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynLibrary", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynLibrary, GetWeakRefFlag), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynLibrary", "bool IsLoaded() const", asMETHODPR(DynLibrary, IsLoaded, () const, bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynLibrary", "bool Load(const string &in libPath)", asMETHODPR(DynLibrary, Load, (const std::string &), bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynLibrary", "NativePointer FindSymbol(const string &in symbolName) const", asFUNCTIONPR([](const DynLibrary *lib, const std::string &symbolName) {
        void *symbol = lib->FindSymbol(symbolName);
        if (!symbol) {
            return NativePointer();
        }
        return NativePointer(symbol, const_cast<DynLibrary *>(lib), RetainDynLibraryOwner, ReleaseDynLibraryOwner);
    }, (const DynLibrary *, const std::string &), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynLibrary", "string GetLibraryPath() const", asMETHODPR(DynLibrary, GetLibraryPath, () const, std::string), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

static void RegisterDynSymbols(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("DynSymbols", 0, asOBJ_REF); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynSymbols", asBEHAVE_FACTORY, "DynSymbols@ f()", asFUNCTIONPR(DynSymbols::Create, (), DynSymbols *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("DynSymbols", asBEHAVE_ADDREF, "void f()", asMETHOD(DynSymbols, AddRef), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynSymbols", asBEHAVE_RELEASE, "void f()", asMETHOD(DynSymbols, Release), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("DynSymbols", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(DynSymbols, GetWeakRefFlag), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynSymbols", "bool IsInited() const", asMETHODPR(DynSymbols, IsInited, () const, bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynSymbols", "bool Init(const string &in libPath)", asMETHODPR(DynSymbols, Init, (const std::string &), bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("DynSymbols", "int GetCount() const", asMETHODPR(DynSymbols, GetCount, () const, int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynSymbols", "string GetName(int index) const", asMETHODPR(DynSymbols, GetName, (int) const, std::string), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("DynSymbols", "string GetName(NativePointer value) const", asFUNCTIONPR([](const DynSymbols *syms, NativePointer value) { return syms->GetNameByValue(value.Get()); }, (const DynSymbols *, NativePointer), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

void RegisterScriptDynCall(asIScriptEngine *engine) {
    RegisterDCEnums(engine);
    RegisterDCSigChars(engine);
    RegisterDynAggregate(engine);
    RegisterDynCall(engine);

    int r = engine->RegisterGlobalFunction("int DynGetModeFromCCSigChar(int8 sigChar)", asFUNCTION(dcGetModeFromCCSigChar), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
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
