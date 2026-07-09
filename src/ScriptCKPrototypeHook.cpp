#include "ScriptCKPrototypeHook.h"

#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>

namespace {

constexpr const char *kCK2ModuleName = "CK2.dll";
constexpr const char *kRuntimePrototypeSymbol = "?CreateCKBehaviorPrototypeRunTime@@YAPAVCKBehaviorPrototype@@PAD@Z";
constexpr const char *kPrototypeSymbol = "?CreateCKBehaviorPrototype@@YAPAVCKBehaviorPrototype@@PAD@Z";
constexpr int kJumpPatchSize = 5;

struct PrototypeHookState {
    void *Target = nullptr;
    unsigned char Original[kJumpPatchSize] = {};
    bool Installed = false;
    bool ExternalHook = false;
    bool ChainedExternalHook = false;
};

PrototypeHookState g_HookState;
std::mutex g_HookMutex;

class SuspendedThreadSet {
public:
    ~SuspendedThreadSet() {
        Resume();
    }

    bool Suspend() {
        const DWORD processId = GetCurrentProcessId();
        const DWORD currentThreadId = GetCurrentThreadId();
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return false;
        }

        THREADENTRY32 entry = {};
        entry.dwSize = sizeof(entry);
        BOOL hasEntry = Thread32First(snapshot, &entry);
        while (hasEntry) {
            if (entry.th32OwnerProcessID == processId && entry.th32ThreadID != currentThreadId) {
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                                           FALSE,
                                           entry.th32ThreadID);
                if (thread) {
                    m_Threads.push_back(thread);
                } else if (GetLastError() != ERROR_INVALID_PARAMETER) {
                    CloseHandle(snapshot);
                    Resume();
                    return false;
                }
            }
            hasEntry = Thread32Next(snapshot, &entry);
        }
        CloseHandle(snapshot);

        std::size_t suspended = 0;
        for (; suspended < m_Threads.size(); ++suspended) {
            if (SuspendThread(m_Threads[suspended]) == static_cast<DWORD>(-1)) {
                for (std::size_t i = suspended; i > 0; --i) {
                    ResumeThread(m_Threads[i - 1]);
                }
                for (HANDLE thread : m_Threads) {
                    CloseHandle(thread);
                }
                m_Threads.clear();
                return false;
            }
        }
        m_Suspended = true;
        return true;
    }

    bool CanPatchRange(const void *address, std::size_t size) const {
        const std::uintptr_t begin = reinterpret_cast<std::uintptr_t>(address);
        const std::uintptr_t end = begin + size;
        for (HANDLE thread : m_Threads) {
            CONTEXT context = {};
            context.ContextFlags = CONTEXT_CONTROL;
            if (!GetThreadContext(thread, &context)) {
                return false;
            }
#if defined(_M_X64)
            const std::uintptr_t instruction = static_cast<std::uintptr_t>(context.Rip);
#else
            const std::uintptr_t instruction = static_cast<std::uintptr_t>(context.Eip);
#endif
            if (instruction >= begin && instruction < end) {
                return false;
            }
        }
        return true;
    }

    void Resume() {
        if (m_Suspended) {
            for (auto it = m_Threads.rbegin(); it != m_Threads.rend(); ++it) {
                ResumeThread(*it);
            }
        }
        for (HANDLE thread : m_Threads) {
            CloseHandle(thread);
        }
        m_Threads.clear();
        m_Suspended = false;
    }

private:
    std::vector<HANDLE> m_Threads;
    bool m_Suspended = false;
};

bool EnvFlagEnabled(const char *name) {
    char value[16] = {};
    const DWORD length = GetEnvironmentVariableA(name, value, static_cast<DWORD>(sizeof(value)));
    if (length == 0 || length >= sizeof(value)) {
        return false;
    }

    return value[0] == '1' ||
           value[0] == 'y' || value[0] == 'Y' ||
           value[0] == 't' || value[0] == 'T' ||
           value[0] == 'o' || value[0] == 'O';
}

void SetError(std::string *error, const char *message) {
    if (error) {
        *error = message ? message : "";
    }
}

bool ResolvePrototypeExports(void **runtimePrototype, void **normalPrototype, std::string *error) {
    if (runtimePrototype) {
        *runtimePrototype = nullptr;
    }
    if (normalPrototype) {
        *normalPrototype = nullptr;
    }

    HMODULE ck2 = GetModuleHandleA(kCK2ModuleName);
    if (!ck2) {
        SetError(error, "CK2.dll is not loaded.");
        return false;
    }

    void *runtime = reinterpret_cast<void *>(GetProcAddress(ck2, kRuntimePrototypeSymbol));
    void *normal = reinterpret_cast<void *>(GetProcAddress(ck2, kPrototypeSymbol));
    if (!runtime || !normal) {
        SetError(error, "CreateCKBehaviorPrototype exports were not found in CK2.dll.");
        return false;
    }

    if (runtimePrototype) {
        *runtimePrototype = runtime;
    }
    if (normalPrototype) {
        *normalPrototype = normal;
    }
    SetError(error, "");
    return true;
}

bool ReadRelativeJumpTarget(void *source, void **target) {
    if (!source || !target) {
        return false;
    }

    auto *bytes = static_cast<unsigned char *>(source);
    if (bytes[0] != 0xE9) {
        return false;
    }

    std::int32_t displacement = 0;
    std::memcpy(&displacement, bytes + 1, sizeof(displacement));
    *target = bytes + kJumpPatchSize + displacement;
    return true;
}

bool RuntimePrototypeRedirectsToNormal(void *runtimePrototype, void *normalPrototype) {
    if (!runtimePrototype || !normalPrototype) {
        return false;
    }
    if (runtimePrototype == normalPrototype) {
        return true;
    }

    void *existingJumpTarget = nullptr;
    return ReadRelativeJumpTarget(runtimePrototype, &existingJumpTarget) &&
           existingJumpTarget == normalPrototype;
}

bool RuntimePrototypeHasExternalJump(void *runtimePrototype, void *normalPrototype) {
    void *existingJumpTarget = nullptr;
    return ReadRelativeJumpTarget(runtimePrototype, &existingJumpTarget) &&
           existingJumpTarget != normalPrototype;
}

bool WriteRelativeJump(void *source, void *target, unsigned char *saved, std::string *error) {
    if (!source || !target) {
        SetError(error, "CreateCKBehaviorPrototype hook source or target is not available.");
        return false;
    }

    auto *sourceBytes = static_cast<unsigned char *>(source);
    auto *targetBytes = static_cast<unsigned char *>(target);
    const std::intptr_t displacement = targetBytes - (sourceBytes + kJumpPatchSize);
    if (displacement < (std::numeric_limits<std::int32_t>::min)() ||
        displacement > (std::numeric_limits<std::int32_t>::max)()) {
        SetError(error, "CreateCKBehaviorPrototype hook target is out of relative jump range.");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(sourceBytes, kJumpPatchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        SetError(error, "VirtualProtect failed while installing CreateCKBehaviorPrototype hook.");
        return false;
    }

    SuspendedThreadSet threads;
    if (!threads.Suspend()) {
        DWORD ignored = 0;
        VirtualProtect(sourceBytes, kJumpPatchSize, oldProtect, &ignored);
        SetError(error, "Failed to suspend process threads while installing CreateCKBehaviorPrototype hook.");
        return false;
    }
    if (!threads.CanPatchRange(sourceBytes, kJumpPatchSize)) {
        threads.Resume();
        DWORD ignored = 0;
        VirtualProtect(sourceBytes, kJumpPatchSize, oldProtect, &ignored);
        SetError(error, "A process thread is executing the CreateCKBehaviorPrototypeRunTime entry point; hook installation was deferred.");
        return false;
    }

    unsigned char patch[kJumpPatchSize] = {0xE9, 0, 0, 0, 0};
    const std::int32_t rel32 = static_cast<std::int32_t>(displacement);
    std::memcpy(patch + 1, &rel32, sizeof(rel32));
    if (saved) {
        std::memcpy(saved, sourceBytes, kJumpPatchSize);
    }
    std::memcpy(sourceBytes, patch, kJumpPatchSize);
    FlushInstructionCache(GetCurrentProcess(), sourceBytes, kJumpPatchSize);
    threads.Resume();

    DWORD ignored = 0;
    VirtualProtect(sourceBytes, kJumpPatchSize, oldProtect, &ignored);
    return true;
}

bool RestoreBytes(void *target, const unsigned char *original) {
    if (!target || !original) {
        return false;
    }

    auto *targetBytes = static_cast<unsigned char *>(target);
    DWORD oldProtect = 0;
    if (!VirtualProtect(targetBytes, kJumpPatchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    SuspendedThreadSet threads;
    if (!threads.Suspend()) {
        DWORD ignored = 0;
        VirtualProtect(targetBytes, kJumpPatchSize, oldProtect, &ignored);
        return false;
    }
    if (!threads.CanPatchRange(targetBytes, kJumpPatchSize)) {
        threads.Resume();
        DWORD ignored = 0;
        VirtualProtect(targetBytes, kJumpPatchSize, oldProtect, &ignored);
        return false;
    }

    std::memcpy(targetBytes, original, kJumpPatchSize);
    FlushInstructionCache(GetCurrentProcess(), targetBytes, kJumpPatchSize);
    threads.Resume();

    DWORD ignored = 0;
    VirtualProtect(targetBytes, kJumpPatchSize, oldProtect, &ignored);
    return true;
}

} // namespace

bool InstallCKBehaviorPrototypeRuntimeHook(std::string *error) {
    const std::lock_guard<std::mutex> lock(g_HookMutex);
    if (EnvFlagEnabled("CKAS_DISABLE_CK2_PROTOTYPE_HOOK")) {
        SetError(error, "");
        return true;
    }

    void *runtimePrototype = nullptr;
    void *normalPrototype = nullptr;
    if (!ResolvePrototypeExports(&runtimePrototype, &normalPrototype, error)) {
        return false;
    }

    if (g_HookState.Installed) {
        if (RuntimePrototypeHasExternalJump(runtimePrototype, normalPrototype)) {
            g_HookState.ChainedExternalHook = true;
        }
        SetError(error, "");
        return true;
    }

    if (RuntimePrototypeRedirectsToNormal(runtimePrototype, normalPrototype)) {
        g_HookState.ExternalHook = true;
        SetError(error, "");
        return true;
    }

    void *existingJumpTarget = nullptr;
    if (ReadRelativeJumpTarget(runtimePrototype, &existingJumpTarget)) {
        if (existingJumpTarget == normalPrototype) {
            g_HookState.ExternalHook = true;
            SetError(error, "");
            return true;
        }

        SetError(error, "CreateCKBehaviorPrototypeRunTime already appears to be hooked by another module.");
        return false;
    }

    g_HookState = PrototypeHookState();
    if (!WriteRelativeJump(runtimePrototype, normalPrototype, g_HookState.Original, error)) {
        return false;
    }

    g_HookState.Target = runtimePrototype;
    g_HookState.Installed = true;
    SetError(error, "");
    return true;
}

void UninstallCKBehaviorPrototypeRuntimeHook() {
    const std::lock_guard<std::mutex> lock(g_HookMutex);
    if (!g_HookState.Installed) {
        return;
    }

    void *runtimePrototype = nullptr;
    void *normalPrototype = nullptr;
    if (!g_HookState.ChainedExternalHook &&
        ResolvePrototypeExports(&runtimePrototype, &normalPrototype, nullptr) &&
        runtimePrototype == g_HookState.Target &&
        RuntimePrototypeRedirectsToNormal(runtimePrototype, normalPrototype)) {
        RestoreBytes(g_HookState.Target, g_HookState.Original);
    }
    g_HookState = PrototypeHookState();
}

bool IsCKBehaviorPrototypeRuntimeHookInstalled() {
    const std::lock_guard<std::mutex> lock(g_HookMutex);
    void *runtimePrototype = nullptr;
    void *normalPrototype = nullptr;
    return ResolvePrototypeExports(&runtimePrototype, &normalPrototype, nullptr) &&
           RuntimePrototypeRedirectsToNormal(runtimePrototype, normalPrototype);
}
