#include "ScriptManager.h"

CKAS_STATUS ScriptManager::LoadModule(const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result) {
    ScriptModuleRegistry::MutationContext context = {
        *this,
        m_ModuleStateStore,
        m_HandleRegistry,
        m_AsyncScheduler.get(),
        m_ImportBinder,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ModuleRegistry.Load(context, options, result);
}

CKAS_STATUS ScriptManager::CompileModule(const char *moduleName,
                                         const char *scriptCode,
                                         CKDWORD flags,
                                         CKAngelScriptResult *result) {
    ScriptModuleRegistry::MutationContext context = {
        *this,
        m_ModuleStateStore,
        m_HandleRegistry,
        m_AsyncScheduler.get(),
        m_ImportBinder,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ModuleRegistry.Compile(context, moduleName, scriptCode, flags, result);
}

CKAS_STATUS ScriptManager::UnloadModule(const char *moduleName, CKAngelScriptResult *result) {
    ScriptModuleRegistry::MutationContext context = {
        *this,
        m_ModuleStateStore,
        m_HandleRegistry,
        m_AsyncScheduler.get(),
        m_ImportBinder,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ModuleRegistry.Unload(context, moduleName, result);
}

bool ScriptManager::HasModule(const char *moduleName) {
    return m_ModuleRegistry.Has(*this, moduleName);
}

CKDWORD ScriptManager::GetModuleGeneration(const char *moduleName) const {
    return m_ModuleRegistry.GetGeneration(m_ModuleStateStore, moduleName);
}

asIScriptModule *ScriptManager::GetModule(const char *moduleName) {
    return GetScript(moduleName);
}

CKAS_STATUS ScriptManager::BorrowModule(const char *moduleName,
                                        asIScriptModule **outModule,
                                        CKAngelScriptResult *result) {
    ScriptModuleRegistry::BorrowContext context = {
        *this,
        m_Diagnostics};
    return m_ModuleRegistry.BorrowModule(context, moduleName, outModule, result);
}

CKAS_STATUS ScriptManager::BorrowFunctionByName(const char *moduleName,
                                                const char *functionName,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    ScriptModuleRegistry::BorrowContext context = {
        *this,
        m_Diagnostics};
    return m_ModuleRegistry.BorrowFunctionByName(context, moduleName, functionName, outFunction, result);
}

CKAS_STATUS ScriptManager::BorrowFunctionByDecl(const char *moduleName,
                                                const char *functionDecl,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    ScriptModuleRegistry::BorrowContext context = {
        *this,
        m_Diagnostics};
    return m_ModuleRegistry.BorrowFunctionByDecl(context, moduleName, functionDecl, outFunction, result);
}

CKAS_STATUS ScriptManager::EnumerateMetadata(const char *moduleName,
                                             CKAngelScriptMetadataCallback callback,
                                             void *userData,
                                             CKAngelScriptResult *result) {
    ScriptModuleRegistry::MetadataContext context = {
        *this,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ModuleRegistry.EnumerateMetadata(context, moduleName, callback, userData, result);
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

CKAS_STATUS ScriptManager::EnumerateModuleIncludeEdges(const char *moduleName,
                                                       CKAngelScriptIncludeEdgeCallback callback,
                                                       void *userData,
                                                       CKAngelScriptResult *result) {
    ScriptModuleRegistry::QueryContext context = {
        *this,
        m_ModuleStateStore,
        m_ImportBinder,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ModuleRegistry.EnumerateIncludeEdges(context, moduleName, callback, userData, result);
}

CKAS_STATUS ScriptManager::GetModuleFingerprint(const char *moduleName,
                                                CKAngelScriptModuleFingerprint *outFingerprint,
                                                CKAngelScriptResult *result) {
    ScriptModuleRegistry::QueryContext context = {
        *this,
        m_ModuleStateStore,
        m_ImportBinder,
        m_Diagnostics,
        m_PublicCallbackDepth};
    return m_ModuleRegistry.GetFingerprint(context, moduleName, outFingerprint, result);
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
        m_AsyncScheduler.get(),
        m_ImportBinder,
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
