#include "ScriptManager.h"

#include <cstring>
#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ScriptApiSupport.h"
#include "ScriptModuleMutationPolicy.h"
#include "ScriptModuleRegistry.h"
#include "ScriptPublicOptions.h"

void ScriptModuleRegistry::Clear() {
    m_Cache.Clear();
}

std::shared_ptr<CachedScript> ScriptModuleRegistry::GetCachedScript(const char *scriptName) {
    if (!ScriptApiSupport::IsNonEmpty(scriptName)) {
        return nullptr;
    }
    return m_Cache.GetCachedScript(scriptName);
}

std::shared_ptr<CachedScript> ScriptModuleRegistry::NewCachedScript(const char *scriptName) {
    if (!ScriptApiSupport::IsNonEmpty(scriptName)) {
        return nullptr;
    }
    return m_Cache.NewCachedScript(scriptName);
}

void ScriptModuleRegistry::CacheScript(const char *scriptName, std::shared_ptr<CachedScript> script) {
    if (!ScriptApiSupport::IsNonEmpty(scriptName)) {
        return;
    }
    m_Cache.CacheScript(scriptName, std::move(script));
}

void ScriptModuleRegistry::Invalidate(const char *scriptName) {
    if (!ScriptApiSupport::IsNonEmpty(scriptName)) {
        return;
    }
    m_Cache.Invalidate(scriptName);
}

int ScriptModuleRegistry::LoadFromDefaultOrFile(ScriptManager &manager,
                                                const char *moduleName,
                                                const char *filename) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return -1;
    }

    asIScriptEngine *engine = manager.GetScriptEngine();
    if (!engine) {
        return -2;
    }

    XString scriptFilename;
    if (filename) {
        scriptFilename = filename;
    } else {
        scriptFilename = moduleName;
        scriptFilename += ".as";
    }

    auto cache = m_Cache.LoadScript(engine, moduleName, scriptFilename.CStr());
    if (!cache) {
        return -3;
    }
    return 0;
}

int ScriptModuleRegistry::LoadFromFiles(ScriptManager &manager,
                                        const char *moduleName,
                                        const char **filenames,
                                        size_t count) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return -1;
    }

    asIScriptEngine *engine = manager.GetScriptEngine();
    if (!engine) {
        return -2;
    }

    std::vector<std::string> files;
    for (size_t i = 0; i < count; i++) {
        XString scriptFilename = filenames[i];
        if (scriptFilename.Find(".as") == XString::NOTFOUND) {
            scriptFilename += ".as";
        }
        manager.ResolveScriptFileName(scriptFilename);
        files.emplace_back(scriptFilename.CStr());
    }

    auto cache = m_Cache.LoadScript(engine, moduleName, files);
    if (!cache) {
        return -3;
    }
    return 0;
}

int ScriptModuleRegistry::CompileFromMemory(ScriptManager &manager,
                                            const char *moduleName,
                                            const char *scriptCode) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName) || !scriptCode) {
        return -1;
    }

    asIScriptEngine *engine = manager.GetScriptEngine();
    if (!engine) {
        return -2;
    }

    auto cache = m_Cache.CompileScript(engine, moduleName, scriptCode);
    if (!cache) {
        return -3;
    }
    return 0;
}

bool ScriptModuleRegistry::DiscardCached(const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return false;
    }
    return m_Cache.UnloadScript(moduleName);
}

bool ScriptModuleRegistry::Discard(ScriptManager &manager, const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return false;
    }
    if (DiscardCached(moduleName)) {
        return true;
    }
    asIScriptModule *module = manager.GetModule(moduleName);
    if (!module) {
        return false;
    }
    module->Discard();
    return true;
}

bool ScriptModuleRegistry::RestoreFromChunk(const char *scriptName, CKStateChunk *chunk) {
    if (!chunk) {
        return false;
    }
    std::shared_ptr<CachedScript> script = NewCachedScript(scriptName);
    if (!script) {
        return false;
    }
    if (script->module) {
        return true;
    }
    return script->LoadFromChunk(chunk);
}

bool ScriptModuleRegistry::SaveToChunk(const char *scriptName, CKStateChunk *chunk) {
    if (!chunk) {
        return false;
    }
    std::shared_ptr<CachedScript> script = GetCachedScript(scriptName);
    return script ? script->SaveToChunk(chunk) : false;
}

bool ScriptModuleRegistry::ClearCode(const char *scriptName) {
    std::shared_ptr<CachedScript> script = GetCachedScript(scriptName);
    if (!script) {
        return false;
    }
    script->ClearCodeCache();
    return true;
}

void ScriptModuleRegistry::ApplyCachedIncludeEdges(ScriptModuleStateStore &stateStore, const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    const std::shared_ptr<CachedScript> cached = GetCachedScript(moduleName);
    stateStore.SetIncludeEdges(moduleName, cached ? cached->includeEdges : std::vector<ScriptIncludeEdge>());
}

CKAS_STATUS ScriptModuleRegistry::CompleteSourceLoad(
    ScriptManager &manager,
    const char *moduleName,
    const std::vector<CapturedScriptMessage> &diagnosticMessages,
    CKAngelScriptResult *result) {
    ApplyCachedIncludeEdges(manager.m_ModuleStateStore, moduleName);
    manager.m_ModuleStateStore.SetKind(moduleName, ScriptModuleKind::Source);
    manager.m_ModuleStateStore.BumpGeneration(moduleName);
    return manager.StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
}

unsigned long long ScriptModuleRegistry::BuildSourceHash(const char *moduleName) {
    unsigned long long sourceHash = ScriptApiSupport::kFnvOffsetBasis;
    const std::shared_ptr<CachedScript> cached = GetCachedScript(moduleName ? moduleName : "");
    if (cached) {
        ScriptApiSupport::HashBool(sourceHash, cached->sourceSnapshotSections);
        ScriptApiSupport::HashValue(sourceHash, static_cast<unsigned long long>(cached->sections.size()));
        for (const auto &section : cached->sections) {
            ScriptApiSupport::HashString(sourceHash, std::get<0>(section));
            ScriptApiSupport::HashString(sourceHash, std::get<1>(section));
        }
    }
    return sourceHash;
}

CKAS_STATUS ScriptModuleRegistry::ReplaceFromSections(
    ScriptManager &manager,
    const char *moduleName,
    const std::vector<std::tuple<std::string, std::string>> &sections,
    bool sourceSnapshotSections,
    CKAngelScriptResult *result) {
    return m_Replacer.ReplaceFromSections(manager,
                                          *this,
                                          manager.m_ModuleStateStore,
                                          manager.m_ImportBinder,
                                          manager.m_Diagnostics,
                                          moduleName,
                                          sections,
                                          sourceSnapshotSections,
                                          result);
}

CKAS_STATUS ScriptModuleRegistry::ReplaceFromBytecode(ScriptManager &manager,
                                                      const char *moduleName,
                                                      const std::vector<unsigned char> &byteCode,
                                                      CKAngelScriptResult *result) {
    return m_Replacer.ReplaceFromBytecode(manager,
                                          *this,
                                          manager.m_ModuleStateStore,
                                          manager.m_ImportBinder,
                                          manager.m_Diagnostics,
                                          moduleName,
                                          byteCode,
                                          result);
}

CKAS_STATUS ScriptModuleRegistry::Load(ScriptManager &manager,
                                       const CKAngelScriptLoadOptions &options,
                                       CKAngelScriptResult *result) {
    ScriptPublicOptions::LoadModuleRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeLoadOptions(options, request, errorMessage);
    if (optionStatus != CKAS_OK) {
        return manager.StoreResult(result, optionStatus, 0, errorMessage);
    }
    const CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(manager.m_Diagnostics,
                                                         manager.m_PublicCallbackDepth,
                                                         "LoadModule",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    if (!manager.GetScriptEngine()) {
        return manager.StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    const bool replacingExisting = Has(manager, request.ModuleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(request.Flags, CKAS_LOAD_REPLACEEXISTING)) {
            return manager.StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        const CKAS_STATUS mutationStatus = ScriptModuleMutationPolicy::CheckReplaceOrUnloadAllowed(
            manager.m_HandleRegistry,
            manager.m_ModuleStateStore,
            manager.m_Diagnostics,
            request.ModuleName,
            result);
        if (mutationStatus != CKAS_OK) {
            return mutationStatus;
        }
    }
    if (request.SourceKind == ScriptPublicOptions::LoadSourceKind::Code) {
        return Compile(manager, request.ModuleName, request.Code, CKAS_COMPILE_REPLACEEXISTING, result);
    }
    if (request.SourceKind == ScriptPublicOptions::LoadSourceKind::Sections) {
        return ReplaceFromSections(manager, request.ModuleName, request.Sections, true, result);
    }
    if (request.SourceKind == ScriptPublicOptions::LoadSourceKind::Files) {
        if (replacingExisting) {
            std::vector<std::tuple<std::string, std::string>> sections;
            sections.reserve(request.FileCount);
            for (size_t i = 0; i < request.FileCount; ++i) {
                XString scriptFilename = request.Filenames[i];
                if (scriptFilename.Find(".as") == XString::NOTFOUND) {
                    scriptFilename += ".as";
                }
                manager.ResolveScriptFileName(scriptFilename);
                sections.emplace_back(scriptFilename.CStr(), std::string());
            }
            return ReplaceFromSections(manager, request.ModuleName, sections, false, result);
        }
        std::vector<CapturedScriptMessage> diagnosticMessages;
        manager.BeginScriptMessageCapture();
        const int loadResult = LoadFromFiles(manager, request.ModuleName, request.Filenames, request.FileCount);
        const std::string diagnostics = manager.EndScriptMessageCapture(&diagnosticMessages);
        if (loadResult < 0) {
            return manager.StoreResult(result,
                                       CKAS_COMPILEERROR,
                                       loadResult,
                                       diagnostics.empty() ? "Failed to load script files." : diagnostics,
                                       std::string(),
                                       &diagnosticMessages);
        }
        return CompleteSourceLoad(manager, request.ModuleName, diagnosticMessages, result);
    }

    if (replacingExisting) {
        std::string scriptFilename;
        if (request.Filename) {
            scriptFilename = request.Filename;
        } else {
            scriptFilename = request.ModuleName;
            scriptFilename += ".as";
        }
        std::vector<std::tuple<std::string, std::string>> sections;
        sections.emplace_back(std::move(scriptFilename), std::string());
        return ReplaceFromSections(manager, request.ModuleName, sections, false, result);
    }

    std::vector<CapturedScriptMessage> diagnosticMessages;
    manager.BeginScriptMessageCapture();
    const int loadResult = LoadFromDefaultOrFile(manager, request.ModuleName, request.Filename);
    const std::string diagnostics = manager.EndScriptMessageCapture(&diagnosticMessages);
    if (loadResult < 0) {
        return manager.StoreResult(result,
                                   CKAS_COMPILEERROR,
                                   loadResult,
                                   diagnostics.empty() ? "Failed to load script file." : diagnostics,
                                   std::string(),
                                   &diagnosticMessages);
    }
    return CompleteSourceLoad(manager, request.ModuleName, diagnosticMessages, result);
}

CKAS_STATUS ScriptModuleRegistry::Compile(ScriptManager &manager,
                                          const char *moduleName,
                                          const char *scriptCode,
                                          CKDWORD flags,
                                          CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName) || !scriptCode) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name and script code are required.");
    }
    const CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(manager.m_Diagnostics,
                                                         manager.m_PublicCallbackDepth,
                                                         "CompileModule",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    if (ScriptApiSupport::HasUnknownPublicFlags(flags, CKAS_COMPILE_REPLACEEXISTING)) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown CompileModule flags.");
    }
    if (!manager.GetScriptEngine()) {
        return manager.StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    const bool replacingExisting = Has(manager, moduleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(flags, CKAS_COMPILE_REPLACEEXISTING)) {
            return manager.StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        const CKAS_STATUS mutationStatus = ScriptModuleMutationPolicy::CheckReplaceOrUnloadAllowed(
            manager.m_HandleRegistry,
            manager.m_ModuleStateStore,
            manager.m_Diagnostics,
            moduleName,
            result);
        if (mutationStatus != CKAS_OK) {
            return mutationStatus;
        }
        std::vector<std::tuple<std::string, std::string>> sections;
        sections.emplace_back(moduleName, scriptCode);
        return ReplaceFromSections(manager, moduleName, sections, false, result);
    }

    std::vector<CapturedScriptMessage> diagnosticMessages;
    manager.BeginScriptMessageCapture();
    const int compileResult = CompileFromMemory(manager, moduleName, scriptCode);
    const std::string diagnostics = manager.EndScriptMessageCapture(&diagnosticMessages);
    if (compileResult < 0) {
        return manager.StoreResult(result,
                                   CKAS_COMPILEERROR,
                                   compileResult,
                                   diagnostics.empty() ? "Failed to compile script module." : diagnostics,
                                   std::string(),
                                   &diagnosticMessages);
    }
    return CompleteSourceLoad(manager, moduleName, diagnosticMessages, result);
}

CKAS_STATUS ScriptModuleRegistry::Unload(ScriptManager &manager,
                                         const char *moduleName,
                                         CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    const CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(manager.m_Diagnostics,
                                                         manager.m_PublicCallbackDepth,
                                                         "UnloadModule",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    const CKAS_STATUS mutationStatus = ScriptModuleMutationPolicy::CheckReplaceOrUnloadAllowed(
        manager.m_HandleRegistry,
        manager.m_ModuleStateStore,
        manager.m_Diagnostics,
        moduleName,
        result);
    if (mutationStatus != CKAS_OK) {
        return mutationStatus;
    }
    if (!Discard(manager, moduleName)) {
        return manager.StoreResult(result, CKAS_NOTFOUND, 0, "Module was not loaded.");
    }
    manager.m_ModuleStateStore.RemoveImportBindingsForModule(moduleName);
    manager.m_ModuleStateStore.ClearIncludeEdges(moduleName);
    manager.m_ModuleStateStore.SetKind(moduleName, ScriptModuleKind::RawUnknown);
    manager.m_ModuleStateStore.BumpGeneration(moduleName);
    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::LoadModule(const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result) {
    return m_ModuleRegistry.Load(*this, options, result);
}

CKAS_STATUS ScriptManager::CompileModule(const char *moduleName,
                                         const char *scriptCode,
                                         CKDWORD flags,
                                         CKAngelScriptResult *result) {
    return m_ModuleRegistry.Compile(*this, moduleName, scriptCode, flags, result);
}

CKAS_STATUS ScriptManager::UnloadModule(const char *moduleName, CKAngelScriptResult *result) {
    return m_ModuleRegistry.Unload(*this, moduleName, result);
}

bool ScriptModuleRegistry::Has(ScriptManager &manager, const char *moduleName) {
    return manager.GetModule(moduleName) != nullptr;
}

CKDWORD ScriptModuleRegistry::GetGeneration(const ScriptManager &manager, const char *moduleName) const {
    return manager.m_ModuleStateStore.GetGeneration(moduleName);
}

bool ScriptManager::HasModule(const char *moduleName) {
    return m_ModuleRegistry.Has(*this, moduleName);
}

CKDWORD ScriptManager::GetModuleGeneration(const char *moduleName) const {
    return m_ModuleRegistry.GetGeneration(*this, moduleName);
}

asIScriptModule *ScriptManager::GetModule(const char *moduleName) {
    return GetScript(moduleName);
}

CKAS_STATUS ScriptModuleRegistry::BorrowModule(ScriptManager &manager,
                                               const char *moduleName,
                                               asIScriptModule **outModule,
                                               CKAngelScriptResult *result) {
    if (outModule) {
        *outModule = nullptr;
    }
    if (!outModule) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module out pointer is required.");
    }
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (!manager.GetScriptEngine()) {
        return manager.StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptModule *module = manager.GetModule(moduleName);
    if (!module) {
        return manager.StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
    }
    *outModule = module;
    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptModuleRegistry::BorrowFunctionByName(ScriptManager &manager,
                                                       const char *moduleName,
                                                       const char *functionName,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function out pointer is required.");
    }
    if (!ScriptApiSupport::IsNonEmpty(functionName)) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function name is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(manager, moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    asIScriptFunction *match = nullptr;
    asUINT matchCount = 0;
    const asUINT count = module->GetFunctionCount();
    for (asUINT i = 0; i < count; ++i) {
        asIScriptFunction *function = module->GetFunctionByIndex(i);
        const char *name = function ? function->GetName() : nullptr;
        if (name && std::strcmp(name, functionName) == 0) {
            match = function;
            ++matchCount;
        }
    }
    if (matchCount == 0) {
        return manager.StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    if (matchCount > 1) {
        return manager.StoreResult(result, CKAS_AMBIGUOUS, 0, "Function name matched multiple overloads.");
    }
    *outFunction = match;
    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptModuleRegistry::BorrowFunctionByDecl(ScriptManager &manager,
                                                       const char *moduleName,
                                                       const char *functionDecl,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function out pointer is required.");
    }
    if (!ScriptApiSupport::IsNonEmpty(functionDecl)) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function declaration is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(manager, moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    asIScriptFunction *function = module->GetFunctionByDecl(functionDecl);
    if (!function) {
        return manager.StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    *outFunction = function;
    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::BorrowModule(const char *moduleName,
                                        asIScriptModule **outModule,
                                        CKAngelScriptResult *result) {
    return m_ModuleRegistry.BorrowModule(*this, moduleName, outModule, result);
}

CKAS_STATUS ScriptManager::BorrowFunctionByName(const char *moduleName,
                                                const char *functionName,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    return m_ModuleRegistry.BorrowFunctionByName(*this, moduleName, functionName, outFunction, result);
}

CKAS_STATUS ScriptManager::BorrowFunctionByDecl(const char *moduleName,
                                                const char *functionDecl,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    return m_ModuleRegistry.BorrowFunctionByDecl(*this, moduleName, functionDecl, outFunction, result);
}

CKAS_STATUS ScriptModuleRegistry::EnumerateMetadata(ScriptManager &manager,
                                                    const char *moduleName,
                                                    CKAngelScriptMetadataCallback callback,
                                                    void *userData,
                                                    CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (!callback) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Metadata callback is required.");
    }
    if (!manager.GetScriptEngine()) {
        return manager.StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }

    std::shared_ptr<CachedScript> cached = GetCachedScript(moduleName);
    if (!cached || !cached->GetScriptModule()) {
        return manager.StoreResult(result, CKAS_NOTFOUND, 0, "Module metadata was not found.");
    }

    asIScriptModule *module = cached->GetScriptModule();
    auto finish = [&manager, result](CKAS_STATUS status) {
        return status == CKAS_OK
                   ? manager.StoreResult(result, CKAS_OK)
                   : manager.StoreResult(result, status, 0, "Metadata enumeration stopped by callback.");
    };
    auto dispatchMetadata = [&manager, callback, userData](
                                const CKAngelScriptMetadataEntry &entry,
                                CKDWORD metadataCount,
                                const std::function<const char *(CKDWORD)> &metadataAt) {
        ScriptApiSupport::CallbackDepthScope callbackScope(manager.m_PublicCallbackDepth);
        return ScriptApiSupport::DispatchMetadata(entry, metadataCount, metadataAt, callback, userData);
    };

    const asUINT typeCount = module->GetObjectTypeCount();
    for (asUINT typeIndex = 0; typeIndex < typeCount; ++typeIndex) {
        asITypeInfo *type = module->GetObjectTypeByIndex(typeIndex);
        if (!type) {
            continue;
        }
        const int typeId = type->GetTypeId();
        const char *typeName = type->GetName();
        const char *typeNamespace = type->GetNamespace();
        const std::string typeDeclaration = typeName ? typeName : "";
        const int rawTypeMetadataCount = cached->GetTypeMetadataCount(typeId);
        const CKDWORD typeMetadataCount =
            static_cast<CKDWORD>(XMax(0, rawTypeMetadataCount));
        if (typeMetadataCount > 0) {
            CKAngelScriptMetadataEntry entry = {};
            entry.Size = sizeof(entry);
            entry.Target = CKAS_METADATA_TYPE;
            entry.Name = typeName;
            entry.Namespace = typeNamespace;
            entry.Declaration = typeDeclaration.c_str();
            const CKAS_STATUS status = dispatchMetadata(
                entry,
                typeMetadataCount,
                [cached, typeId](CKDWORD index) {
                    return cached->GetTypeMetadata(typeId, static_cast<int>(index));
                });
            if (status != CKAS_OK) {
                return finish(status);
            }
        }

        const asUINT methodCount = type->GetMethodCount();
        for (asUINT methodIndex = 0; methodIndex < methodCount; ++methodIndex) {
            asIScriptFunction *method = type->GetMethodByIndex(methodIndex);
            const int rawMetadataCount = cached->GetClassMethodMetadataCount(typeId, method);
            const CKDWORD metadataCount =
                static_cast<CKDWORD>(XMax(0, rawMetadataCount));
            if (!method || metadataCount == 0) {
                continue;
            }
            const std::string declaration = method->GetDeclaration(false, false, true);
            CKAngelScriptMetadataEntry entry = {};
            entry.Size = sizeof(entry);
            entry.Target = CKAS_METADATA_TYPE_METHOD;
            entry.Name = method->GetName();
            entry.Namespace = typeNamespace;
            entry.Declaration = declaration.c_str();
            entry.ParentTypeName = typeName;
            entry.ParentTypeNamespace = typeNamespace;
            const CKAS_STATUS status = dispatchMetadata(
                entry,
                metadataCount,
                [cached, typeId, method](CKDWORD index) {
                    return cached->GetClassMethodMetadata(typeId, method, static_cast<int>(index));
                });
            if (status != CKAS_OK) {
                return finish(status);
            }
        }

        const asUINT propertyCount = type->GetPropertyCount();
        for (asUINT propertyIndex = 0; propertyIndex < propertyCount; ++propertyIndex) {
            const int rawMetadataCount = cached->GetClassVarMetadataCount(typeId, static_cast<int>(propertyIndex));
            const CKDWORD metadataCount =
                static_cast<CKDWORD>(XMax(0, rawMetadataCount));
            if (metadataCount == 0) {
                continue;
            }
            const char *propertyName = nullptr;
            type->GetProperty(propertyIndex, &propertyName);
            const char *declaration = type->GetPropertyDeclaration(propertyIndex, false);
            CKAngelScriptMetadataEntry entry = {};
            entry.Size = sizeof(entry);
            entry.Target = CKAS_METADATA_TYPE_PROPERTY;
            entry.Name = propertyName;
            entry.Namespace = typeNamespace;
            entry.Declaration = declaration;
            entry.ParentTypeName = typeName;
            entry.ParentTypeNamespace = typeNamespace;
            const CKAS_STATUS status = dispatchMetadata(
                entry,
                metadataCount,
                [cached, typeId, propertyIndex](CKDWORD index) {
                    return cached->GetClassVarMetadata(typeId,
                                                       static_cast<int>(propertyIndex),
                                                       static_cast<int>(index));
                });
            if (status != CKAS_OK) {
                return finish(status);
            }
        }
    }

    const asUINT functionCount = module->GetFunctionCount();
    for (asUINT functionIndex = 0; functionIndex < functionCount; ++functionIndex) {
        asIScriptFunction *function = module->GetFunctionByIndex(functionIndex);
        const int rawMetadataCount = cached->GetFuncMetadataCount(function);
        const CKDWORD metadataCount =
            static_cast<CKDWORD>(XMax(0, rawMetadataCount));
        if (!function || metadataCount == 0) {
            continue;
        }
        const std::string declaration = function->GetDeclaration(false, true, true);
        CKAngelScriptMetadataEntry entry = {};
        entry.Size = sizeof(entry);
        entry.Target = CKAS_METADATA_GLOBAL_FUNCTION;
        entry.Name = function->GetName();
        entry.Namespace = function->GetNamespace();
        entry.Declaration = declaration.c_str();
        const CKAS_STATUS status = dispatchMetadata(
            entry,
            metadataCount,
            [cached, function](CKDWORD index) {
                return cached->GetFuncMetadata(function, static_cast<int>(index));
            });
        if (status != CKAS_OK) {
            return finish(status);
        }
    }

    const asUINT globalCount = module->GetGlobalVarCount();
    for (asUINT globalIndex = 0; globalIndex < globalCount; ++globalIndex) {
        const int rawMetadataCount = cached->GetVarMetadataCount(static_cast<int>(globalIndex));
        const CKDWORD metadataCount =
            static_cast<CKDWORD>(XMax(0, rawMetadataCount));
        if (metadataCount == 0) {
            continue;
        }
        const char *name = nullptr;
        const char *nameSpace = nullptr;
        module->GetGlobalVar(globalIndex, &name, &nameSpace);
        const char *declaration = module->GetGlobalVarDeclaration(globalIndex, true);
        CKAngelScriptMetadataEntry entry = {};
        entry.Size = sizeof(entry);
        entry.Target = CKAS_METADATA_GLOBAL_VARIABLE;
        entry.Name = name;
        entry.Namespace = nameSpace;
        entry.Declaration = declaration;
        const CKAS_STATUS status = dispatchMetadata(
            entry,
            metadataCount,
            [cached, globalIndex](CKDWORD index) {
                return cached->GetVarMetadata(static_cast<int>(globalIndex), static_cast<int>(index));
            });
        if (status != CKAS_OK) {
            return finish(status);
        }
    }

    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::EnumerateMetadata(const char *moduleName,
                                             CKAngelScriptMetadataCallback callback,
                                             void *userData,
                                             CKAngelScriptResult *result) {
    return m_ModuleRegistry.EnumerateMetadata(*this, moduleName, callback, userData, result);
}

CKAS_STATUS ScriptManager::GetImportedFunctionCount(const char *moduleName,
                                                    CKDWORD *outCount,
                                                    CKAngelScriptResult *result) {
    ScriptImportBinder::ReadContext context = {
        *this,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ImportBinder.GetImportedFunctionCount(context, moduleName, outCount, result);
}

CKAS_STATUS ScriptManager::EnumerateImportedFunctions(const char *moduleName,
                                                      CKAngelScriptImportCallback callback,
                                                      void *userData,
                                                      CKAngelScriptResult *result) {
    ScriptImportBinder::ReadContext context = {
        *this,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ImportBinder.EnumerateImportedFunctions(context, moduleName, callback, userData, result);
}

CKAS_STATUS ScriptManager::BindImportedFunction(const CKAngelScriptImportBindOptions &options,
                                                CKAngelScriptResult *result) {
    ScriptImportBinder::BindContext context = {
        *this,
        m_ModuleStateStore,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ImportBinder.BindImportedFunction(context, options, result);
}

CKAS_STATUS ScriptManager::BindAllImportedFunctions(const char *moduleName,
                                                    CKAngelScriptResult *result) {
    ScriptImportBinder::BindContext context = {
        *this,
        m_ModuleStateStore,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ImportBinder.BindAllImportedFunctions(context, moduleName, result);
}

CKAS_STATUS ScriptManager::UnbindImportedFunction(const char *moduleName,
                                                  CKDWORD importIndex,
                                                  CKAngelScriptResult *result) {
    ScriptImportBinder::BindContext context = {
        *this,
        m_ModuleStateStore,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ImportBinder.UnbindImportedFunction(context, moduleName, importIndex, result);
}

CKAS_STATUS ScriptManager::UnbindAllImportedFunctions(const char *moduleName,
                                                      CKAngelScriptResult *result) {
    ScriptImportBinder::BindContext context = {
        *this,
        m_ModuleStateStore,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ImportBinder.UnbindAllImportedFunctions(context, moduleName, result);
}

CKAS_STATUS ScriptManager::EnumerateBoundImportEdges(const char *moduleName,
                                                     CKAngelScriptBoundImportEdgeCallback callback,
                                                     void *userData,
                                                     CKAngelScriptResult *result) {
    ScriptImportBinder::BindContext context = {
        *this,
        m_ModuleStateStore,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ImportBinder.EnumerateBoundImportEdges(context, moduleName, callback, userData, result);
}

CKAS_STATUS ScriptModuleRegistry::EnumerateIncludeEdges(ScriptManager &manager,
                                                        const char *moduleName,
                                                        CKAngelScriptIncludeEdgeCallback callback,
                                                        void *userData,
                                                        CKAngelScriptResult *result) {
    if (!callback) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Include edge callback is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = BorrowModule(manager, moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    const std::vector<ScriptIncludeEdge> *includeEdges =
        manager.m_ModuleStateStore.FindIncludeEdges(moduleName);
    if (!includeEdges) {
        return manager.StoreResult(result, CKAS_OK);
    }

    for (const ScriptIncludeEdge &edge : *includeEdges) {
        CKAngelScriptIncludeEdge publicEdge = {};
        publicEdge.Size = sizeof(publicEdge);
        publicEdge.ModuleName = moduleName;
        publicEdge.FromSection = edge.FromSection.c_str();
        publicEdge.ToSection = edge.ToSection.c_str();
        publicEdge.ResolvedFromSnapshot = edge.ResolvedFromSnapshot ? TRUE : FALSE;
        CKAS_STATUS callbackStatus = CKAS_OK;
        {
            ScriptApiSupport::CallbackDepthScope callbackScope(manager.m_PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchIncludeEdge(publicEdge, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return manager.StoreResult(result,
                                       callbackStatus,
                                       0,
                                       "Include edge enumeration stopped by callback.");
        }
    }
    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptModuleRegistry::GetFingerprint(ScriptManager &manager,
                                                 const char *moduleName,
                                                 CKAngelScriptModuleFingerprint *outFingerprint,
                                                 CKAngelScriptResult *result) {
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::ValidateModuleFingerprintOutput(outFingerprint,
                                                                                   errorMessage);
    if (optionStatus != CKAS_OK) {
        return manager.StoreResult(result, optionStatus, 0, errorMessage);
    }
    CKAngelScriptInitModuleFingerprint(outFingerprint);

    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = BorrowModule(manager, moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    *outFingerprint = manager.m_ModuleStateStore.GetFingerprint(
        moduleName,
        CKAS_API_VERSION,
        asGetLibraryVersion(),
        asGetLibraryOptions(),
        BuildSourceHash(moduleName),
        manager.m_ImportBinder.BuildDeclaredImportHash(manager, moduleName));
    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::EnumerateModuleIncludeEdges(const char *moduleName,
                                                       CKAngelScriptIncludeEdgeCallback callback,
                                                       void *userData,
                                                       CKAngelScriptResult *result) {
    return m_ModuleRegistry.EnumerateIncludeEdges(*this, moduleName, callback, userData, result);
}

CKAS_STATUS ScriptManager::GetModuleFingerprint(const char *moduleName,
                                                CKAngelScriptModuleFingerprint *outFingerprint,
                                                CKAngelScriptResult *result) {
    return m_ModuleRegistry.GetFingerprint(*this, moduleName, outFingerprint, result);
}

CKAS_STATUS ScriptManager::SaveModuleBytecode(const CKAngelScriptBytecodeSaveOptions &options,
                                              CKAngelScriptResult *result) {
    ScriptModuleBytecodeStore::SaveContext context = {
        *this,
        m_Diagnostics,
        m_PublicCallbackDepth,
        m_BytecodeCallbackDepth};
    return m_ModuleBytecodeStore.Save(context, options, result);
}

CKAS_STATUS ScriptManager::LoadModuleBytecode(const CKAngelScriptBytecodeLoadOptions &options,
                                              CKAngelScriptResult *result) {
    ScriptModuleBytecodeStore::LoadContext context = {
        *this,
        m_ModuleRegistry,
        m_ModuleStateStore,
        m_HandleRegistry,
        m_Diagnostics,
        m_PublicCallbackDepth,
        m_BytecodeCallbackDepth};
    return m_ModuleBytecodeStore.Load(context, options, result);
}

asIScriptModule *ScriptManager::GetScript(const char *scriptName) {
    if (!GetScriptEngine())
        return nullptr;
    return GetScriptEngine()->GetModule(scriptName, asGM_ONLY_IF_EXISTS);
}

std::shared_ptr<CachedScript> ScriptManager::GetCachedScript(const char *scriptName) {
    return m_ModuleRegistry.GetCachedScript(scriptName);
}

std::shared_ptr<CachedScript> ScriptManager::NewCachedScript(const char *scriptName) {
    return m_ModuleRegistry.NewCachedScript(scriptName);
}

bool ScriptManager::RestoreCachedScriptFromChunk(const char *scriptName, CKStateChunk *chunk) {
    return m_ModuleRegistry.RestoreFromChunk(scriptName, chunk);
}

bool ScriptManager::SaveCachedScriptToChunk(const char *scriptName, CKStateChunk *chunk) {
    return m_ModuleRegistry.SaveToChunk(scriptName, chunk);
}

bool ScriptManager::ClearCachedScriptCode(const char *scriptName) {
    return m_ModuleRegistry.ClearCode(scriptName);
}

