#include "ScriptManager.h"

#include <cstring>
#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ScriptApiDiagnostics.h"
#include "ScriptApiSupport.h"
#include "ScriptAngelScriptGc.h"
#include "ScriptModuleMutationPolicy.h"
#include "ScriptModuleRegistry.h"
#include "ScriptPublicOptions.h"

namespace {

std::string MakeQualifiedTypeDeclaration(const char *typeNamespace, const char *typeName) {
    std::string declaration;
    if (typeNamespace && typeNamespace[0] != '\0') {
        declaration += typeNamespace;
        declaration += "::";
    }
    declaration += typeName ? typeName : "";
    return declaration;
}

void InvalidateUnloadedCache(ScriptModuleRegistry &registry, ScriptManager &manager, const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName) || manager.GetModule(moduleName)) {
        return;
    }
    if (registry.GetCachedScript(moduleName)) {
        registry.Invalidate(moduleName);
    }
}

} // namespace

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
        InvalidateUnloadedCache(*this, manager, moduleName);
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
    InvalidateUnloadedCache(*this, manager, moduleName);

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
    InvalidateUnloadedCache(*this, manager, moduleName);

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
    bool discarded = false;
    if (DiscardCached(moduleName)) {
        discarded = true;
    } else {
        asIScriptModule *module = manager.GetModule(moduleName);
        if (!module) {
            return false;
        }
        ScriptDiscardModuleWithGarbageCollection(module);
        discarded = true;
    }
    return discarded;
}

bool ScriptModuleRegistry::RestoreFromChunk(const char *scriptName, CKStateChunk *chunk) {
    if (!ScriptApiSupport::IsNonEmpty(scriptName) || !chunk) {
        return false;
    }

    auto restored = std::make_shared<CachedScript>();
    if (!restored->LoadFromChunk(chunk) || restored->name != scriptName) {
        return false;
    }
    CacheScript(scriptName, std::move(restored));
    return true;
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
    stateStore.SetIncludeEdges(moduleName, cached ? cached->GetIncludeEdges() : std::vector<ScriptIncludeEdge>());
}

CKAS_STATUS ScriptModuleRegistry::CompleteSourceLoad(
    MutationContext &context,
    const char *moduleName,
    const std::vector<CapturedScriptMessage> &diagnosticMessages,
    CKAngelScriptResult *result) {
    ApplyCachedIncludeEdges(context.StateStore, moduleName);
    context.StateStore.SetKind(moduleName, ScriptModuleKind::Source);
    context.StateStore.BumpGeneration(moduleName);
    return context.Diagnostics.StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
}

unsigned long long ScriptModuleRegistry::BuildSourceHash(const char *moduleName) {
    unsigned long long sourceHash = ScriptApiSupport::kFnvOffsetBasis;
    const std::shared_ptr<CachedScript> cached = GetCachedScript(moduleName ? moduleName : "");
    if (cached) {
        const auto &sections = cached->GetSections();
        ScriptApiSupport::HashBool(sourceHash, cached->IsSourceSnapshotSections());
        ScriptApiSupport::HashValue(sourceHash, static_cast<unsigned long long>(sections.size()));
        for (size_t i = 0; i < sections.size(); ++i) {
            const auto &section = sections[i];
            const bool hasCode = cached->HasSectionCode(i);
            ScriptApiSupport::HashString(sourceHash, std::get<0>(section));
            ScriptApiSupport::HashBool(sourceHash, hasCode);
            if (hasCode) {
                ScriptApiSupport::HashString(sourceHash, std::get<1>(section));
            }
        }
    }
    return sourceHash;
}

CKAS_STATUS ScriptModuleRegistry::ReplaceFromSections(
    MutationContext &context,
    const char *moduleName,
    const std::vector<std::tuple<std::string, std::string>> &sections,
    bool sourceSnapshotSections,
    bool memorySections,
    CKAngelScriptResult *result) {
    return m_Replacer.ReplaceFromSections(context.Manager,
                                          *this,
                                          context.StateStore,
                                          context.ImportBinder,
                                          context.Diagnostics,
                                          moduleName,
                                          sections,
                                          sourceSnapshotSections,
                                          memorySections,
                                          result);
}

CKAS_STATUS ScriptModuleRegistry::ReplaceFromBytecode(MutationContext &context,
                                                      const char *moduleName,
                                                      const std::vector<unsigned char> &byteCode,
                                                      CKAngelScriptResult *result) {
    return m_Replacer.ReplaceFromBytecode(context.Manager,
                                          *this,
                                          context.StateStore,
                                          context.ImportBinder,
                                          context.Diagnostics,
                                          moduleName,
                                          byteCode,
                                          result);
}

CKAS_STATUS ScriptModuleRegistry::Load(MutationContext &context,
                                       const CKAngelScriptLoadOptions &options,
                                       CKAngelScriptResult *result) {
    ScriptPublicOptions::LoadModuleRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeLoadOptions(options, request, errorMessage);
    if (optionStatus != CKAS_OK) {
        return context.Diagnostics.StoreResult(result, optionStatus, 0, errorMessage);
    }
    const CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(context.Diagnostics,
                                                         context.PublicCallbackDepth,
                                                         "LoadModule",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    if (!context.Manager.GetScriptEngine()) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_NOTINITIALIZED,
                                               0,
                                               "AngelScript engine is not initialized.");
    }
    const bool replacingExisting = Has(context.Manager, request.ModuleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(request.Flags, CKAS_LOAD_REPLACEEXISTING)) {
            return context.Diagnostics.StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        const CKAS_STATUS mutationStatus = ScriptModuleMutationPolicy::CheckReplaceOrUnloadAllowed(
            context.HandleRegistry,
            context.AsyncScheduler,
            context.StateStore,
            context.Diagnostics,
            request.ModuleName,
            result);
        if (mutationStatus != CKAS_OK) {
            return mutationStatus;
        }
    }
    if (request.SourceKind == ScriptPublicOptions::LoadSourceKind::Code) {
        return Compile(context, request.ModuleName, request.Code, CKAS_COMPILE_REPLACEEXISTING, result);
    }
    if (request.SourceKind == ScriptPublicOptions::LoadSourceKind::Sections) {
        return ReplaceFromSections(context, request.ModuleName, request.Sections, true, true, result);
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
                context.Manager.ResolveScriptFileName(scriptFilename);
                sections.emplace_back(scriptFilename.CStr(), std::string());
            }
            return ReplaceFromSections(context, request.ModuleName, sections, false, false, result);
        }
        std::vector<CapturedScriptMessage> diagnosticMessages;
        context.Diagnostics.BeginScriptMessageCapture();
        const int loadResult =
            LoadFromFiles(context.Manager, request.ModuleName, request.Filenames, request.FileCount);
        const std::string diagnostics = context.Diagnostics.EndScriptMessageCapture(&diagnosticMessages);
        if (loadResult < 0) {
            return context.Diagnostics.StoreResult(
                result,
                CKAS_COMPILEERROR,
                loadResult,
                diagnostics.empty() ? "Failed to load script files." : diagnostics,
                std::string(),
                &diagnosticMessages);
        }
        return CompleteSourceLoad(context, request.ModuleName, diagnosticMessages, result);
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
        return ReplaceFromSections(context, request.ModuleName, sections, false, false, result);
    }

    std::vector<CapturedScriptMessage> diagnosticMessages;
    context.Diagnostics.BeginScriptMessageCapture();
    const int loadResult = LoadFromDefaultOrFile(context.Manager, request.ModuleName, request.Filename);
    const std::string diagnostics = context.Diagnostics.EndScriptMessageCapture(&diagnosticMessages);
    if (loadResult < 0) {
        return context.Diagnostics.StoreResult(
            result,
            CKAS_COMPILEERROR,
            loadResult,
            diagnostics.empty() ? "Failed to load script file." : diagnostics,
            std::string(),
            &diagnosticMessages);
    }
    return CompleteSourceLoad(context, request.ModuleName, diagnosticMessages, result);
}

CKAS_STATUS ScriptModuleRegistry::Compile(MutationContext &context,
                                          const char *moduleName,
                                          const char *scriptCode,
                                          CKDWORD flags,
                                          CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName) || !scriptCode) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Module name and script code are required.");
    }
    const CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(context.Diagnostics,
                                                         context.PublicCallbackDepth,
                                                         "CompileModule",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    if (ScriptApiSupport::HasUnknownPublicFlags(flags, CKAS_COMPILE_REPLACEEXISTING)) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Unknown CompileModule flags.");
    }
    if (!context.Manager.GetScriptEngine()) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_NOTINITIALIZED,
                                               0,
                                               "AngelScript engine is not initialized.");
    }
    const bool replacingExisting = Has(context.Manager, moduleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(flags, CKAS_COMPILE_REPLACEEXISTING)) {
            return context.Diagnostics.StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        const CKAS_STATUS mutationStatus = ScriptModuleMutationPolicy::CheckReplaceOrUnloadAllowed(
            context.HandleRegistry,
            context.AsyncScheduler,
            context.StateStore,
            context.Diagnostics,
            moduleName,
            result);
        if (mutationStatus != CKAS_OK) {
            return mutationStatus;
        }
        std::vector<std::tuple<std::string, std::string>> sections;
        sections.emplace_back(moduleName, scriptCode);
        return ReplaceFromSections(context, moduleName, sections, false, true, result);
    }

    std::vector<CapturedScriptMessage> diagnosticMessages;
    context.Diagnostics.BeginScriptMessageCapture();
    const int compileResult = CompileFromMemory(context.Manager, moduleName, scriptCode);
    const std::string diagnostics = context.Diagnostics.EndScriptMessageCapture(&diagnosticMessages);
    if (compileResult < 0) {
        return context.Diagnostics.StoreResult(
            result,
            CKAS_COMPILEERROR,
            compileResult,
            diagnostics.empty() ? "Failed to compile script module." : diagnostics,
            std::string(),
            &diagnosticMessages);
    }
    return CompleteSourceLoad(context, moduleName, diagnosticMessages, result);
}

CKAS_STATUS ScriptModuleRegistry::Unload(MutationContext &context,
                                         const char *moduleName,
                                         CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return context.Diagnostics.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    const CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(context.Diagnostics,
                                                         context.PublicCallbackDepth,
                                                         "UnloadModule",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    const CKAS_STATUS mutationStatus = ScriptModuleMutationPolicy::CheckReplaceOrUnloadAllowed(
        context.HandleRegistry,
        context.AsyncScheduler,
        context.StateStore,
        context.Diagnostics,
        moduleName,
        result);
    if (mutationStatus != CKAS_OK) {
        return mutationStatus;
    }
    if (!Discard(context.Manager, moduleName)) {
        return context.Diagnostics.StoreResult(result, CKAS_NOTFOUND, 0, "Module was not loaded.");
    }
    context.StateStore.RemoveImportBindingsForModule(moduleName);
    context.StateStore.ClearIncludeEdges(moduleName);
    context.StateStore.SetKind(moduleName, ScriptModuleKind::RawUnknown);
    context.StateStore.BumpGeneration(moduleName);
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

bool ScriptModuleRegistry::Has(ScriptManager &manager, const char *moduleName) {
    return manager.GetModule(moduleName) != nullptr;
}

CKDWORD ScriptModuleRegistry::GetGeneration(const ScriptModuleStateStore &stateStore, const char *moduleName) const {
    return stateStore.GetGeneration(moduleName);
}

CKAS_STATUS ScriptModuleRegistry::BorrowModule(BorrowContext &context,
                                               const char *moduleName,
                                               asIScriptModule **outModule,
                                               CKAngelScriptResult *result) {
    if (outModule) {
        *outModule = nullptr;
    }
    if (!outModule) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Module out pointer is required.");
    }
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return context.Diagnostics.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (!context.Manager.GetScriptEngine()) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_NOTINITIALIZED,
                                               0,
                                               "AngelScript engine is not initialized.");
    }
    asIScriptModule *module = context.Manager.GetModule(moduleName);
    if (!module) {
        return context.Diagnostics.StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
    }
    *outModule = module;
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptModuleRegistry::BorrowFunctionByName(BorrowContext &context,
                                                       const char *moduleName,
                                                       const char *functionName,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Function out pointer is required.");
    }
    if (!ScriptApiSupport::IsNonEmpty(functionName)) {
        return context.Diagnostics.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function name is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(context, moduleName, &module, result);
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
        return context.Diagnostics.StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    if (matchCount > 1) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_AMBIGUOUS,
                                               0,
                                               "Function name matched multiple overloads.");
    }
    *outFunction = match;
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptModuleRegistry::BorrowFunctionByDecl(BorrowContext &context,
                                                       const char *moduleName,
                                                       const char *functionDecl,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Function out pointer is required.");
    }
    if (!ScriptApiSupport::IsNonEmpty(functionDecl)) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Function declaration is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(context, moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    asIScriptFunction *function = ScriptApiSupport::FindFunctionByDecl(module, functionDecl);
    if (!function) {
        return context.Diagnostics.StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    *outFunction = function;
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptModuleRegistry::EnumerateMetadata(MetadataContext &context,
                                                    const char *moduleName,
                                                    CKAngelScriptMetadataCallback callback,
                                                    void *userData,
                                                    CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return context.Diagnostics.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (!callback) {
        return context.Diagnostics.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Metadata callback is required.");
    }
    if (!context.Manager.GetScriptEngine()) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_NOTINITIALIZED,
                                               0,
                                               "AngelScript engine is not initialized.");
    }

    std::shared_ptr<CachedScript> cached = GetCachedScript(moduleName);
    if (!cached || !cached->GetScriptModule()) {
        return context.Diagnostics.StoreResult(result, CKAS_NOTFOUND, 0, "Module metadata was not found.");
    }

    asIScriptModule *module = cached->GetScriptModule();
    auto finish = [&context, result](CKAS_STATUS status) {
        return status == CKAS_OK
                   ? context.Diagnostics.StoreResult(result, CKAS_OK)
                   : context.Diagnostics.StoreResult(result,
                                                     status,
                                                     0,
                                                     "Metadata enumeration stopped by callback.");
    };
    auto dispatchMetadata = [&context, callback, userData](
                                const CKAngelScriptMetadataEntry &entry,
                                CKDWORD metadataCount,
                                const std::function<const char *(CKDWORD)> &metadataAt) {
        ScriptApiSupport::CallbackDepthScope callbackScope(context.PublicCallbackDepth);
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
        const std::string typeDeclaration = MakeQualifiedTypeDeclaration(typeNamespace, typeName);
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

    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptModuleRegistry::EnumerateIncludeEdges(QueryContext &context,
                                                        const char *moduleName,
                                                        CKAngelScriptIncludeEdgeCallback callback,
                                                        void *userData,
                                                        CKAngelScriptResult *result) {
    if (!callback) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Include edge callback is required.");
    }
    asIScriptModule *module = nullptr;
    BorrowContext borrowContext = {
        context.Manager,
        context.Diagnostics};
    const CKAS_STATUS borrowStatus = BorrowModule(borrowContext, moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    const std::vector<ScriptIncludeEdge> *includeEdges =
        context.StateStore.FindIncludeEdges(moduleName);
    if (!includeEdges) {
        return context.Diagnostics.StoreResult(result, CKAS_OK);
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
            ScriptApiSupport::CallbackDepthScope callbackScope(context.PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchIncludeEdge(publicEdge, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return context.Diagnostics.StoreResult(result,
                                                   callbackStatus,
                                                   0,
                                                   "Include edge enumeration stopped by callback.");
        }
    }
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptModuleRegistry::GetFingerprint(QueryContext &context,
                                                 const char *moduleName,
                                                 CKAngelScriptModuleFingerprint *outFingerprint,
                                                 CKAngelScriptResult *result) {
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::ValidateModuleFingerprintOutput(outFingerprint,
                                                                                   errorMessage);
    if (optionStatus != CKAS_OK) {
        return context.Diagnostics.StoreResult(result, optionStatus, 0, errorMessage);
    }
    CKAngelScriptInitModuleFingerprint(outFingerprint);

    asIScriptModule *module = nullptr;
    BorrowContext borrowContext = {
        context.Manager,
        context.Diagnostics};
    const CKAS_STATUS borrowStatus = BorrowModule(borrowContext, moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    *outFingerprint = context.StateStore.GetFingerprint(
        moduleName,
        CKAS_API_VERSION,
        asGetLibraryVersion(),
        asGetLibraryOptions(),
        BuildSourceHash(moduleName),
        context.ImportBinder.BuildDeclaredImportHash(context.Manager, moduleName));
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}
