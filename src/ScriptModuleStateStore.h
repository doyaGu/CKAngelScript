#ifndef CK_SCRIPT_MODULE_STATE_STORE_H
#define CK_SCRIPT_MODULE_STATE_STORE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "CKAngelScript.h"
#include "ScriptCache.h"

struct ScriptImportBindingEdge {
    std::string ImportModuleName;
    CKDWORD ImportIndex = 0;
    std::string SourceModuleName;
    std::string FunctionDecl;
};

enum class ScriptModuleKind {
    RawUnknown,
    Source,
    Bytecode
};

class ScriptModuleStateStore {
public:
    void Clear();

    CKDWORD GetGeneration(const char *moduleName) const;
    void BumpGeneration(const char *moduleName);

    void SetKind(const char *moduleName, ScriptModuleKind kind);
    void SetIncludeEdges(const char *moduleName, const std::vector<ScriptIncludeEdge> &includeEdges);
    void ClearIncludeEdges(const char *moduleName);
    const std::vector<ScriptIncludeEdge> *FindIncludeEdges(const char *moduleName) const;

    bool HasBoundImportConsumersForModule(const char *moduleName, std::string *consumerModule = nullptr) const;
    std::vector<ScriptImportBindingEdge> GetImportBindingsForModule(const char *moduleName) const;
    std::vector<ScriptImportBindingEdge> GetImportBindingForModuleIndex(const char *moduleName,
                                                                        CKDWORD importIndex) const;
    bool RemoveImportBinding(const char *moduleName, CKDWORD importIndex);
    bool RemoveImportBindingsForModule(const char *moduleName);
    void RestoreImportBindingsForModule(const char *moduleName,
                                        const std::vector<ScriptImportBindingEdge> &bindings);
    void RecordImportBinding(const char *importModuleName,
                             CKDWORD importIndex,
                             const char *sourceModuleName,
                             const char *functionDecl);

    void MarkDirty(const char *moduleName);
    CKAngelScriptModuleFingerprint GetFingerprint(const char *moduleName,
                                                  CKDWORD apiVersion,
                                                  const char *angelScriptVersion,
                                                  const char *angelScriptOptions,
                                                  unsigned long long sourceHash,
                                                  unsigned long long declaredImportHash);

    static CKAS_MODULEKIND ToPublicKind(ScriptModuleKind kind);

private:
    struct ModuleState {
        CKDWORD Generation = 0;
        ScriptModuleKind Kind = ScriptModuleKind::RawUnknown;
        std::vector<ScriptImportBindingEdge> BoundImports;
        std::vector<ScriptIncludeEdge> IncludeEdges;
        CKAngelScriptModuleFingerprint Fingerprint = {
            static_cast<CKDWORD>(sizeof(CKAngelScriptModuleFingerprint))};
        bool FingerprintDirty = true;
    };

    ModuleState *FindState(const char *moduleName);
    const ModuleState *FindState(const char *moduleName) const;
    ModuleState &EnsureState(const char *moduleName);

    std::unordered_map<std::string, ModuleState> m_States;
};

#endif // CK_SCRIPT_MODULE_STATE_STORE_H
