#include "ScriptManager.h"

#include "CKPathManager.h"

#include "ScriptFormat.h"
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

static int PragmaCallback(const std::string &pragmaText, CScriptBuilder &builder, void * /*userParam*/) {
    asIScriptEngine *engine = builder.GetEngine();

    // Filter the pragmaText so only what is of interest remains
    // With this the user can add comments and use different whitespaces without affecting the result
    asUINT pos = 0;
    asUINT length = 0;
    std::string cleanText;
    while (pos < pragmaText.size()) {
        asETokenClass tokenClass = engine->ParseToken(pragmaText.c_str() + pos, 0, &length);
        if (tokenClass == asTC_IDENTIFIER || tokenClass == asTC_KEYWORD || tokenClass == asTC_VALUE) {
            std::string token = pragmaText.substr(pos, length);
            cleanText += " " + token;
        }
        if (tokenClass == asTC_UNKNOWN)
            return -1;
        pos += length;
    }

    // The #pragma directive was not accepted
    return -1;
}

ScriptManager::ScriptManager(CKContext *context) : CKBaseManager(context, SCRIPT_MANAGER_GUID, "AngelScript Manager") {
    context->RegisterNewManager(this);
    int r = Init();
    assert(r >= 0);
}

ScriptManager::~ScriptManager() {
    Shutdown();
}

CKStateChunk *ScriptManager::SaveData(CKFile *SavedFile) {
    return CK_OK;
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

    asSetGlobalMemoryFunctions([](size_t size) { return VxMalloc(size); },
                               [](void *ptr) { VxFree(ptr); });

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

    RegisterStdAddons(m_ScriptEngine);

    // Register the Virtools API
    RegisterVirtools(m_ScriptEngine);

    // Register the function that we want the scripts to call
    RegisterScriptFormat(m_ScriptEngine);

    m_ScriptContext = m_ScriptEngine->CreateContext();
    if (!m_ScriptContext) {
        m_Context->OutputToConsole(const_cast<char *>("Failed to create script context."));
        return -1;
    }
    m_ScriptContext->SetExceptionCallback(asMETHOD(ScriptManager, ExceptionCallback), this, asCALL_THISCALL);

    m_Flags |= AS_INITED;
    return 0;
}

int ScriptManager::Shutdown() {
    if (!IsInited())
        return -2;

    if (m_ScriptContext) {
        m_ScriptContext->Release();
        m_ScriptContext = nullptr;
    }

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

asIScriptContext *ScriptManager::GetScriptContext() {
    return m_ScriptContext;
}

int ScriptManager::LoadScript(const char *moduleName, const char *filename) {
    if (!moduleName || moduleName[0] == '\0')
        return -1;

    if (!m_ScriptEngine || !m_ScriptContext)
        return -2;

    if (filename == nullptr)
        filename = moduleName;

    int r = 0;

    // We will only initialize the global variables once we're
    // ready to execute, so disable the automatic initialization
    // m_ScriptEngine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);

    CScriptBuilder builder;

    // Set the pragma callback, so we can detect if the script needs debugging
    builder.SetPragmaCallback(PragmaCallback, nullptr);

    // Compile the script
    r = builder.StartNewModule(m_ScriptEngine, moduleName);
    if (r < 0) {
        m_ScriptEngine->WriteMessage(moduleName, 0, 0, asMSGTYPE_ERROR, "Failed to create module");
        return -1;
    }

    XString scriptFilename = filename;
    ResolveScriptFileName(scriptFilename);

    r = builder.AddSectionFromFile(scriptFilename.CStr());
    if (r < 0) {
        m_ScriptEngine->WriteMessage(moduleName, 0, 0, asMSGTYPE_ERROR, "Failed to process file");
        return -1;
    }

    r = builder.BuildModule();
    if (r < 0) {
        m_ScriptEngine->WriteMessage(moduleName, 0, 0, asMSGTYPE_ERROR, "Failed to build module");
        return -1;
    }

    return r;
}

void ScriptManager::UnloadScript(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0')
        return;

    asIScriptModule *module = m_ScriptEngine->GetModule(moduleName, asGM_ONLY_IF_EXISTS);
    if (!module)
        return;

    module->Discard();
}

asIScriptModule *ScriptManager::GetScript(const char *moduleName) {
    return m_ScriptEngine->GetModule(moduleName, asGM_ONLY_IF_EXISTS);
}

int ScriptManager::ExecuteScript(const char *moduleName, const char *decl) {
    int r = 0;

    if (!moduleName || moduleName[0] == '\0')
        return -1;

    if (!m_ScriptEngine || !m_ScriptContext)
        return -2;

    asIScriptModule *module = m_ScriptEngine->GetModule(moduleName, asGM_ONLY_IF_EXISTS);
    if (!module)
        return -1;

    // Find the function
    asIScriptFunction *func = nullptr;
    if (decl) {
        func = module->GetFunctionByDecl(decl);
    } else {
        func = module->GetFunctionByDecl("int main()");
        if (!func) {
            // Try again with "void main()"
            func = module->GetFunctionByDecl("void main()");
        }
    }

    if (!func) {
        m_ScriptEngine->WriteMessage(moduleName, 0, 0, asMSGTYPE_ERROR, "Cannot find 'int main()' or 'void main()'");
        return -1;
    }

    // Prepare the script context with the function we wish to execute. Prepare()
    // must be called on the context before each new script function that will be
    // executed. Note, that if you intend to execute the same function several
    // times, it might be a good idea to store the function id returned by
    // GetFunctionIDByDecl(), so that this relatively slow call can be skipped.
    r = m_ScriptContext->Prepare(func);
    if (r < 0) {
        m_ScriptEngine->WriteMessage(moduleName, 0, 0, asMSGTYPE_ERROR,
                                     "Failed while preparing the context for execution");
        return -1;
    }

    // Execute the `main()` function in the script.
    return m_ScriptContext->Execute();
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

CKERROR ScriptManager::ResolveScriptFileName(XString &filename) {
    CKPathManager *pm = m_Context->GetPathManager();
    if (m_ScriptPathCategoryIndex == -1) {
        XString category = "Script Paths";
        m_ScriptPathCategoryIndex = pm->GetCategoryIndex(category);
    }
    return pm->ResolveFileName(filename, m_ScriptPathCategoryIndex);
}
