#include "ScriptManager.h"

#include <cstring>
#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "ScriptApiSupport.h"
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

bool ScriptManager::HasBoundImportConsumersForModule(const char *moduleName,
                                                     std::string *consumerModule) const {
    return m_ModuleStateStore.HasBoundImportConsumersForModule(moduleName, consumerModule);
}

CKAS_STATUS ScriptManager::CheckModuleRuntimeHandlesReleased(const char *moduleName,
                                                             CKAngelScriptResult *result) {
    if (HasRuntimeHandleForModule(moduleName)) {
        return StoreResult(result,
                           CKAS_INUSE,
                           0,
                           "Module has live object or execution handles.");
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptManager::CheckModuleHasNoBoundImportConsumers(const char *moduleName,
                                                                CKAngelScriptResult *result) {
    std::string importConsumer;
    if (HasBoundImportConsumersForModule(moduleName, &importConsumer)) {
        return StoreResult(result,
                           CKAS_INUSE,
                           0,
                           fmt::format("Module is imported by bound module '{}'.",
                                       importConsumer));
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptManager::CheckModuleReplaceOrUnloadAllowed(const char *moduleName,
                                                             CKAngelScriptResult *result) {
    const CKAS_STATUS runtimeStatus = CheckModuleRuntimeHandlesReleased(moduleName, result);
    if (runtimeStatus != CKAS_OK) {
        return runtimeStatus;
    }
    return CheckModuleHasNoBoundImportConsumers(moduleName, result);
}

bool ScriptManager::IsModuleMutationBlockedByCallback() const {
    return m_PublicCallbackDepth > 0;
}

CKAS_STATUS ScriptManager::RejectModuleMutationDuringCallback(const char *apiName,
                                                              CKAngelScriptResult *result) {
    return StoreResult(result,
                       CKAS_INVALIDSTATE,
                       0,
                       fmt::format("{} cannot mutate modules while a CKAngelScript callback is active.",
                                   apiName ? apiName : "CKAngelScript"));
}

void ScriptManager::SetModuleKind(const char *moduleName, ScriptModuleKind kind) {
    m_ModuleStateStore.SetKind(moduleName, kind);
}

void ScriptManager::SetModuleIncludeEdges(const char *moduleName,
                                          const std::vector<ScriptIncludeEdge> &includeEdges) {
    m_ModuleStateStore.SetIncludeEdges(moduleName, includeEdges);
}

void ScriptManager::RefreshModuleIncludeEdgesFromCache(const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    const std::shared_ptr<CachedScript> cached = m_ModuleRegistry.GetCachedScript(moduleName);
    SetModuleIncludeEdges(moduleName, cached ? cached->includeEdges : std::vector<ScriptIncludeEdge>());
}

void ScriptManager::ClearModuleIncludeEdges(const char *moduleName) {
    m_ModuleStateStore.ClearIncludeEdges(moduleName);
}

unsigned long long ScriptManager::BuildModuleSourceHash(const char *moduleName) {
    return m_ModuleRegistry.BuildSourceHash(moduleName);
}

unsigned long long ScriptManager::BuildDeclaredImportHash(const char *moduleName) {
    unsigned long long declaredImportHash = ScriptApiSupport::kFnvOffsetBasis;
    asIScriptModule *module = GetModule(moduleName);
    if (module) {
        const asUINT importCount = module->GetImportedFunctionCount();
        ScriptApiSupport::HashValue(declaredImportHash, static_cast<unsigned long long>(importCount));
        for (asUINT i = 0; i < importCount; ++i) {
            ScriptApiSupport::HashValue(declaredImportHash, static_cast<CKDWORD>(i));
            ScriptApiSupport::HashString(declaredImportHash, module->GetImportedFunctionSourceModule(i));
            ScriptApiSupport::HashString(declaredImportHash, module->GetImportedFunctionDeclaration(i));
        }
    }
    return declaredImportHash;
}

void ScriptManager::MarkModuleStateDirty(const char *moduleName) {
    m_ModuleStateStore.MarkDirty(moduleName);
}

void ScriptManager::BumpModuleGeneration(const char *moduleName) {
    m_ModuleStateStore.BumpGeneration(moduleName);
}

CKAS_STATUS ScriptManager::ReplaceModuleFromSections(
    const char *moduleName,
    const std::vector<std::tuple<std::string, std::string>> &sections,
    bool sourceSnapshotSections,
    CKAngelScriptResult *result) {
    return m_ModuleReplacer.ReplaceFromSections(*this,
                                                moduleName,
                                                sections,
                                                sourceSnapshotSections,
                                                result);
}

CKAS_STATUS ScriptManager::LoadModule(const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result) {
    ScriptPublicOptions::LoadModuleRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeLoadOptions(options, request, errorMessage);
    if (optionStatus != CKAS_OK) {
        return StoreResult(result, optionStatus, 0, errorMessage);
    }
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("LoadModule", result);
    }
    if (!GetScriptEngine()) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    const bool replacingExisting = HasModule(request.ModuleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(request.Flags, CKAS_LOAD_REPLACEEXISTING)) {
            return StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        const CKAS_STATUS mutationStatus = CheckModuleReplaceOrUnloadAllowed(request.ModuleName, result);
        if (mutationStatus != CKAS_OK) {
            return mutationStatus;
        }
    }
    if (request.SourceKind == ScriptPublicOptions::LoadSourceKind::Code) {
        return CompileModule(request.ModuleName, request.Code, CKAS_COMPILE_REPLACEEXISTING, result);
    }
    if (request.SourceKind == ScriptPublicOptions::LoadSourceKind::Sections) {
        return ReplaceModuleFromSections(request.ModuleName, request.Sections, true, result);
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
                ResolveScriptFileName(scriptFilename);
                sections.emplace_back(scriptFilename.CStr(), std::string());
            }
            return ReplaceModuleFromSections(request.ModuleName, sections, false, result);
        }
        std::vector<CapturedScriptMessage> diagnosticMessages;
        BeginScriptMessageCapture();
        const int loadResult =
            m_ModuleRegistry.LoadFromFiles(*this, request.ModuleName, request.Filenames, request.FileCount);
        const std::string diagnostics = EndScriptMessageCapture(&diagnosticMessages);
        if (loadResult < 0) {
            return StoreResult(result,
                               CKAS_COMPILEERROR,
                               loadResult,
                               diagnostics.empty() ? "Failed to load script files." : diagnostics,
                               std::string(),
                               &diagnosticMessages);
        }
        RefreshModuleIncludeEdgesFromCache(request.ModuleName);
        SetModuleKind(request.ModuleName, ScriptModuleKind::Source);
        BumpModuleGeneration(request.ModuleName);
        return StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
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
        return ReplaceModuleFromSections(request.ModuleName, sections, false, result);
    }

    std::vector<CapturedScriptMessage> diagnosticMessages;
    BeginScriptMessageCapture();
    const int loadResult = m_ModuleRegistry.LoadFromDefaultOrFile(*this, request.ModuleName, request.Filename);
    const std::string diagnostics = EndScriptMessageCapture(&diagnosticMessages);
    if (loadResult < 0) {
        return StoreResult(result,
                           CKAS_COMPILEERROR,
                           loadResult,
                           diagnostics.empty() ? "Failed to load script file." : diagnostics,
                           std::string(),
                           &diagnosticMessages);
    }
    RefreshModuleIncludeEdgesFromCache(request.ModuleName);
    SetModuleKind(request.ModuleName, ScriptModuleKind::Source);
    BumpModuleGeneration(request.ModuleName);
    return StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
}

CKAS_STATUS ScriptManager::CompileModule(const char *moduleName,
                                               const char *scriptCode,
                                               CKDWORD flags,
                                               CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName) || !scriptCode) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name and script code are required.");
    }
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("CompileModule", result);
    }
    if (ScriptApiSupport::HasUnknownPublicFlags(flags, CKAS_COMPILE_REPLACEEXISTING)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown CompileModule flags.");
    }
    if (!GetScriptEngine()) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    const bool replacingExisting = HasModule(moduleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(flags, CKAS_COMPILE_REPLACEEXISTING)) {
            return StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        const CKAS_STATUS mutationStatus = CheckModuleReplaceOrUnloadAllowed(moduleName, result);
        if (mutationStatus != CKAS_OK) {
            return mutationStatus;
        }
        std::vector<std::tuple<std::string, std::string>> sections;
        sections.emplace_back(moduleName, scriptCode);
        return ReplaceModuleFromSections(moduleName, sections, false, result);
    }

    std::vector<CapturedScriptMessage> diagnosticMessages;
    BeginScriptMessageCapture();
    const int compileResult = m_ModuleRegistry.CompileFromMemory(*this, moduleName, scriptCode);
    const std::string diagnostics = EndScriptMessageCapture(&diagnosticMessages);
    if (compileResult < 0) {
        return StoreResult(result,
                           CKAS_COMPILEERROR,
                           compileResult,
                           diagnostics.empty() ? "Failed to compile script module." : diagnostics,
                           std::string(),
                           &diagnosticMessages);
    }
    RefreshModuleIncludeEdgesFromCache(moduleName);
    SetModuleKind(moduleName, ScriptModuleKind::Source);
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
}

CKAS_STATUS ScriptManager::UnloadModule(const char *moduleName, CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("UnloadModule", result);
    }
    const CKAS_STATUS mutationStatus = CheckModuleReplaceOrUnloadAllowed(moduleName, result);
    if (mutationStatus != CKAS_OK) {
        return mutationStatus;
    }
    if (!m_ModuleRegistry.Discard(*this, moduleName)) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module was not loaded.");
    }
    m_ModuleStateStore.RemoveImportBindingsForModule(moduleName);
    ClearModuleIncludeEdges(moduleName);
    SetModuleKind(moduleName, ScriptModuleKind::RawUnknown);
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK);
}

bool ScriptManager::HasModule(const char *moduleName) {
    return GetModule(moduleName) != nullptr;
}

CKDWORD ScriptManager::GetModuleGeneration(const char *moduleName) const {
    return m_ModuleStateStore.GetGeneration(moduleName);
}

asIScriptModule *ScriptManager::GetModule(const char *moduleName) {
    return GetScript(moduleName);
}

CKAS_STATUS ScriptManager::BorrowModule(const char *moduleName,
                                        asIScriptModule **outModule,
                                        CKAngelScriptResult *result) {
    if (outModule) {
        *outModule = nullptr;
    }
    if (!outModule) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module out pointer is required.");
    }
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (!GetScriptEngine()) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
    }
    *outModule = module;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::BorrowFunctionByName(const char *moduleName,
                                                const char *functionName,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function out pointer is required.");
    }
    if (!ScriptApiSupport::IsNonEmpty(functionName)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function name is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(moduleName, &module, result);
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
        return StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    if (matchCount > 1) {
        return StoreResult(result, CKAS_AMBIGUOUS, 0, "Function name matched multiple overloads.");
    }
    *outFunction = match;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::BorrowFunctionByDecl(const char *moduleName,
                                                const char *functionDecl,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function out pointer is required.");
    }
    if (!ScriptApiSupport::IsNonEmpty(functionDecl)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function declaration is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    asIScriptFunction *function = module->GetFunctionByDecl(functionDecl);
    if (!function) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    *outFunction = function;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::EnumerateMetadata(const char *moduleName,
                                             CKAngelScriptMetadataCallback callback,
                                             void *userData,
                                             CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (!callback) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Metadata callback is required.");
    }
    if (!GetScriptEngine()) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }

    std::shared_ptr<CachedScript> cached = GetCachedScript(moduleName);
    if (!cached || !cached->GetScriptModule()) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module metadata was not found.");
    }

    asIScriptModule *module = cached->GetScriptModule();
    auto finish = [this, result](CKAS_STATUS status) {
        return status == CKAS_OK
                   ? StoreResult(result, CKAS_OK)
                   : StoreResult(result, status, 0, "Metadata enumeration stopped by callback.");
    };
    auto dispatchMetadata = [this, callback, userData](
                                const CKAngelScriptMetadataEntry &entry,
                                CKDWORD metadataCount,
                                const std::function<const char *(CKDWORD)> &metadataAt) {
        ScriptApiSupport::CallbackDepthScope callbackScope(m_PublicCallbackDepth);
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

    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::GetImportedFunctionCount(const char *moduleName,
                                                    CKDWORD *outCount,
                                                    CKAngelScriptResult *result) {
    return m_ImportBinder.GetImportedFunctionCount(*this, moduleName, outCount, result);
}

CKAS_STATUS ScriptManager::EnumerateImportedFunctions(const char *moduleName,
                                                      CKAngelScriptImportCallback callback,
                                                      void *userData,
                                                      CKAngelScriptResult *result) {
    return m_ImportBinder.EnumerateImportedFunctions(*this, moduleName, callback, userData, result);
}

CKAS_STATUS ScriptManager::BindImportedFunction(const CKAngelScriptImportBindOptions &options,
                                                CKAngelScriptResult *result) {
    return m_ImportBinder.BindImportedFunction(*this, options, result);
}

CKAS_STATUS ScriptManager::BindAllImportedFunctions(const char *moduleName,
                                                    CKAngelScriptResult *result) {
    return m_ImportBinder.BindAllImportedFunctions(*this, moduleName, result);
}

CKAS_STATUS ScriptManager::UnbindImportedFunction(const char *moduleName,
                                                  CKDWORD importIndex,
                                                  CKAngelScriptResult *result) {
    return m_ImportBinder.UnbindImportedFunction(*this, moduleName, importIndex, result);
}

CKAS_STATUS ScriptManager::UnbindAllImportedFunctions(const char *moduleName,
                                                      CKAngelScriptResult *result) {
    return m_ImportBinder.UnbindAllImportedFunctions(*this, moduleName, result);
}

CKAS_STATUS ScriptManager::EnumerateBoundImportEdges(const char *moduleName,
                                                     CKAngelScriptBoundImportEdgeCallback callback,
                                                     void *userData,
                                                     CKAngelScriptResult *result) {
    return m_ImportBinder.EnumerateBoundImportEdges(*this, moduleName, callback, userData, result);
}

CKAS_STATUS ScriptManager::EnumerateModuleIncludeEdges(const char *moduleName,
                                                       CKAngelScriptIncludeEdgeCallback callback,
                                                       void *userData,
                                                       CKAngelScriptResult *result) {
    if (!callback) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Include edge callback is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    const std::vector<ScriptIncludeEdge> *includeEdges =
        m_ModuleStateStore.FindIncludeEdges(moduleName);
    if (!includeEdges) {
        return StoreResult(result, CKAS_OK);
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
            ScriptApiSupport::CallbackDepthScope callbackScope(m_PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchIncludeEdge(publicEdge, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return StoreResult(result,
                               callbackStatus,
                               0,
                               "Include edge enumeration stopped by callback.");
        }
    }
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::GetModuleFingerprint(const char *moduleName,
                                                CKAngelScriptModuleFingerprint *outFingerprint,
                                                CKAngelScriptResult *result) {
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::ValidateModuleFingerprintOutput(outFingerprint,
                                                                                   errorMessage);
    if (optionStatus != CKAS_OK) {
        return StoreResult(result, optionStatus, 0, errorMessage);
    }
    CKAngelScriptInitModuleFingerprint(outFingerprint);

    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    *outFingerprint = m_ModuleStateStore.GetFingerprint(moduleName,
                                                        CKAS_API_VERSION,
                                                        asGetLibraryVersion(),
                                                        asGetLibraryOptions(),
                                                        BuildModuleSourceHash(moduleName),
                                                        BuildDeclaredImportHash(moduleName));
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::SaveModuleBytecode(const CKAngelScriptBytecodeSaveOptions &options,
                                              CKAngelScriptResult *result) {
    return m_ModuleBytecodeStore.Save(*this, options, result);
}

CKAS_STATUS ScriptManager::LoadModuleBytecode(const CKAngelScriptBytecodeLoadOptions &options,
                                              CKAngelScriptResult *result) {
    return m_ModuleBytecodeStore.Load(*this, options, result);
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

