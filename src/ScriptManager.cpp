#include "ScriptManager.h"

#include "CKPathManager.h"

#include "ScriptFormat.h"
#include "ScriptNativePointer.h"
#include "ScriptNativeBuffer.h"
#include "ScriptVxMath.h"
#include "ScriptCK2.h"

#include "add_on/scriptany/scriptany.h"
#include "add_on/scriptarray/scriptarray.h"
#include "add_on/scriptstdstring/scriptstdstring.h"
#include "add_on/scriptdictionary/scriptdictionary.h"
#include "add_on/scriptmath/scriptmath.h"
#include "add_on/scriptmath/scriptmathcomplex.h"
#include "add_on/scripthandle/scripthandle.h"
#include "add_on/weakref/weakref.h"
#include "add_on/datetime/datetime.h"
#include "add_on/scriptfile/scriptfile.h"
#include "add_on/scriptfile/scriptfilesystem.h"
#include "add_on/scripthelper/scripthelper.h"
#include "add_on/scriptbuilder/scriptbuilder.h"

ScriptManager::ScriptManager(CKContext *context) : CKBaseManager(context, SCRIPT_MANAGER_GUID, (CKSTRING) "AngelScript Manager") {
    context->RegisterNewManager(this);
    int r = Init();
    assert(r >= 0);
}

ScriptManager::~ScriptManager() {
    Shutdown();
}

CKStateChunk *ScriptManager::SaveData(CKFile *SavedFile) {
    return nullptr;
}

CKERROR ScriptManager::LoadData(CKStateChunk *chunk, CKFile *LoadedFile) {
    return CK_OK;
}

CKERROR ScriptManager::PostClearAll() {
    return CK_OK;
}

CKERROR ScriptManager::OnCKReset() {
    return CK_OK;
}

CKERROR ScriptManager::OnCKPause() {
    return CK_OK;
}

CKERROR ScriptManager::PostLoad() {
    return CK_OK;
}

CKERROR ScriptManager::OnPostCopy(CKDependenciesContext &context) {
    return CK_OK;
}

int ScriptManager::Init() {
    if (IsInited())
        return -2;

// #if CKVERSION == 0x13022002
//     asSetGlobalMemoryFunctions(
//         [](size_t size) { return VxMalloc(size); },
//         [](void *ptr) { VxFree(ptr); }
//     );
// #endif

    m_ScriptEngine = asCreateScriptEngine();
    if (!m_ScriptEngine) {
        m_Context->OutputToConsole(const_cast<char *>("Failed to create script engine."));
        return -1;
    }

    m_ScriptEngine->SetUserData(this);
    m_ScriptEngine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
    m_ScriptEngine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, true);
    m_ScriptEngine->SetEngineProperty(asEP_ALLOW_IMPLICIT_HANDLE_TYPES, true);
    m_ScriptEngine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);
    m_ScriptEngine->SetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE, 1);

    // The script compiler will send any compiler messages to the callback
    int r = m_ScriptEngine->SetMessageCallback(asMETHOD(ScriptManager, MessageCallback), this, asCALL_THISCALL);
    if (r < 0)
        return r;

    // Register the standard types
    RegisterStdTypes(m_ScriptEngine);

    // Register the standard add-ons
    RegisterStdAddons(m_ScriptEngine);

    // Register the native types
    RegisterNativePointer(m_ScriptEngine);
    RegisterNativeBuffer(m_ScriptEngine);

    // Register the function that we want the scripts to call
    RegisterScriptFormat(m_ScriptEngine);

    // Register the Virtools API
    RegisterVirtools(m_ScriptEngine);

    m_Flags |= AS_INITED;
    return 0;
}

int ScriptManager::Shutdown() {
    if (!IsInited())
        return -2;

    if (m_ScriptEngine) {
        m_ScriptEngine->ShutDownAndRelease();
        m_ScriptEngine = nullptr;
    }

    m_Flags &= ~AS_INITED;
    return 0;
}

asIScriptEngine *ScriptManager::CreateScriptEngine(asDWORD version) {
    return asCreateScriptEngine(version);
}

const char *ScriptManager::GetVersion() {
    return asGetLibraryVersion();
}

const char *ScriptManager::GetOptions() {
    return asGetLibraryOptions();
}

asIScriptContext *ScriptManager::GetActiveContext() {
    return asGetActiveContext();
}

int ScriptManager::PrepareMultithread(asIThreadManager *externalMgr) {
    return asPrepareMultithread(externalMgr);
}

void ScriptManager::UnprepareMultithread() {
    asUnprepareMultithread();
}

asIThreadManager *ScriptManager::GetThreadManager() {
    return asGetThreadManager();
}

void ScriptManager::AcquireExclusiveLock() {
    asAcquireExclusiveLock();
}

void ScriptManager::ReleaseExclusiveLock() {
    asReleaseExclusiveLock();
}

void ScriptManager::AcquireSharedLock() {
    asAcquireSharedLock();
}

void ScriptManager::ReleaseSharedLock() {
    asReleaseSharedLock();
}

int ScriptManager::AtomicInc(int &value) {
    return asAtomicInc(value);
}

int ScriptManager::AtomicDec(int &value) {
    return asAtomicDec(value);
}

int ScriptManager::ThreadCleanup() {
    return asThreadCleanup();
}

int ScriptManager::SetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc) {
    return asSetGlobalMemoryFunctions(allocFunc, freeFunc);
}

int ScriptManager::ResetGlobalMemoryFunctions() {
    return asResetGlobalMemoryFunctions();
}

void *ScriptManager::AllocMem(size_t size) {
    return asAllocMem(size);
}

void ScriptManager::FreeMem(void *mem) {
    asFreeMem(mem);
}

asILockableSharedBool *ScriptManager::CreateLockableSharedBool() {
    return asCreateLockableSharedBool();
}

asIScriptEngine *ScriptManager::GetScriptEngine() {
    return m_ScriptEngine;
}

int ScriptManager::LoadScript(const char *scriptName, const char *filename) {
    if (!scriptName || scriptName[0] == '\0')
        return -1;

    if (!m_ScriptEngine)
        return -2;

    XString scriptFilename;
    if (filename) {
        scriptFilename = filename;
    } else {
        scriptFilename = scriptName;
        scriptFilename += ".as";
    }

    auto cache = m_ScriptCache.LoadScript(m_ScriptEngine, scriptName, scriptFilename.CStr());
    if (!cache)
        return -3;
    return 0;
}

int ScriptManager::LoadScripts(const char *scriptName, const char **filenames, size_t count) {
    if (!scriptName || scriptName[0] == '\0')
        return -1;

    if (!m_ScriptEngine)
        return -2;

    std::vector<std::string> files;
    for (size_t i = 0; i < count; i++) {
        XString scriptFilename = filenames[i];
        if (scriptFilename.Find(".as") == XString::NOTFOUND)
            scriptFilename += ".as";
        ResolveScriptFileName(scriptFilename);
        files.emplace_back(scriptFilename.CStr());
    }

    auto cache = m_ScriptCache.LoadScript(m_ScriptEngine, scriptName, files);
    if (!cache)
        return -3;
    return 0;
}

int ScriptManager::CompileScript(const char *scriptName, const char *scriptCode) {
    if (!scriptName || scriptName[0] == '\0')
        return -1;

    if (!m_ScriptEngine)
        return -2;

    auto cache = m_ScriptCache.CompileScript(m_ScriptEngine, scriptName, scriptCode);
    if (!cache)
        return -3;
    return 0;
}

void ScriptManager::UnloadScript(const char *scriptName) {
    if (!scriptName || scriptName[0] == '\0')
        return;
    m_ScriptCache.Invalidate(scriptName);
}

asIScriptModule *ScriptManager::GetScript(const char *scriptName) {
    if (!m_ScriptEngine)
        return nullptr;
    return m_ScriptEngine->GetModule(scriptName, asGM_ONLY_IF_EXISTS);
}

CKERROR ScriptManager::ResolveScriptFileName(XString &filename) {
    CKPathManager *pm = m_Context->GetPathManager();
    if (m_ScriptPathCategoryIndex == -1) {
        XString category = "Script Paths";
        m_ScriptPathCategoryIndex = pm->GetCategoryIndex(category);
        if (m_ScriptPathCategoryIndex == -1)
            m_ScriptPathCategoryIndex = pm->AddCategory(category);
    }
    return pm->ResolveFileName(filename, m_ScriptPathCategoryIndex);
}

void ScriptManager::MessageCallback(const asSMessageInfo &msg) {
    const char *type = "NULL";
    switch (msg.type) {
        case asMSGTYPE_ERROR:
            type = "ERROR";
            break;
        case asMSGTYPE_WARNING:
            type = "WARN";
            break;
        case asMSGTYPE_INFORMATION:
            type = "INFO";
            break;
    }
    m_Context->OutputToConsoleEx(const_cast<char *>("%s(%d,%d): %s: %s"), msg.section, msg.row, msg.col, type, msg.message);
}

void ScriptManager::ExceptionCallback(asIScriptContext *context) {
    XString message;
    XString callStackString = GetCallStack(context);
    message.Format("Exception - '%s' in '%s'\n%s", context->GetExceptionString(),
                   context->GetExceptionFunction()->GetDeclaration(), callStackString.CStr());

    asSMessageInfo info = {};
    info.row = context->GetExceptionLineNumber(&info.col, &info.section);
    info.type = asMSGTYPE_ERROR;
    info.message = message.CStr();
    MessageCallback(info);
}

XString ScriptManager::GetCallStack(asIScriptContext *context) {
    XString str("AngelScript Callstack:\n");

    // Append the call stack
    for (asUINT i = 0; i < context->GetCallstackSize(); i++) {
        asIScriptFunction *func = context->GetFunction(i);
        int column;
        const char *scriptSection;
        int line = context->GetLineNumber(i, &column, &scriptSection);

        XString buf;
        buf.Format("\t%s at %s(%d,%d)\n",  func->GetDeclaration(), scriptSection, line, column);
        str << buf;
    }

    return str;
}

void ScriptManager::RegisterStdTypes(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    if (sizeof(void *) == 4) {
        r = engine->RegisterTypedef("size_t", "int"); assert(r >= 0);
        r = engine->RegisterTypedef("ptrdiff_t", "int"); assert(r >= 0);
        r = engine->RegisterTypedef("intptr_t", "int"); assert(r >= 0);
        r = engine->RegisterTypedef("uintptr_t", "uint"); assert(r >= 0);
    } else {
        r = engine->RegisterTypedef("size_t", "int64"); assert(r >= 0);
        r = engine->RegisterTypedef("ptrdiff_t", "int64"); assert(r >= 0);
        r = engine->RegisterTypedef("intptr_t", "int64"); assert(r >= 0);
        r = engine->RegisterTypedef("uintptr_t", "uint64"); assert(r >= 0);
    }
}

void ScriptManager::RegisterStdAddons(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterScriptAny(engine);
    RegisterScriptArray(engine, true);
    RegisterStdString(engine);
    RegisterStdStringUtils(engine);
    RegisterScriptDictionary(engine);
    RegisterScriptMath(engine);
    RegisterScriptMathComplex(engine);
    RegisterScriptHandle(engine);
    RegisterScriptWeakRef(engine);
    RegisterScriptDateTime(engine);
    RegisterScriptFile(engine);
    RegisterScriptFileSystem(engine);
    RegisterExceptionRoutines(engine);
}

void ScriptManager::RegisterVirtools(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterVxMath(m_ScriptEngine);
    RegisterCK2(m_ScriptEngine);
}