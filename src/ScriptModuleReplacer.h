#ifndef CK_SCRIPT_MODULE_REPLACER_H
#define CK_SCRIPT_MODULE_REPLACER_H

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "CKAngelScript.h"

class ScriptImportBinder;
class ScriptApiDiagnostics;
class ScriptManager;
class ScriptModuleRegistry;
class ScriptModuleStateStore;
struct CapturedScriptMessage;
struct CachedScript;

class ScriptModuleReplacer {
public:
    CKAS_STATUS ReplaceFromSections(ScriptManager &manager,
                                    ScriptModuleRegistry &registry,
                                    ScriptModuleStateStore &stateStore,
                                    ScriptImportBinder &importBinder,
                                    ScriptApiDiagnostics &diagnostics,
                                    const char *moduleName,
                                    const std::vector<std::tuple<std::string, std::string>> &sections,
                                    bool sourceSnapshotSections,
                                    bool memorySections,
                                    CKAngelScriptResult *result);

    CKAS_STATUS ReplaceFromBytecode(ScriptManager &manager,
                                    ScriptModuleRegistry &registry,
                                    ScriptModuleStateStore &stateStore,
                                    ScriptImportBinder &importBinder,
                                    ScriptApiDiagnostics &diagnostics,
                                    const char *moduleName,
                                    const std::vector<unsigned char> &byteCode,
                                    CKAngelScriptResult *result);

private:
    struct Snapshot;

    bool CaptureSnapshot(ScriptManager &manager,
                         ScriptModuleRegistry &registry,
                         const ScriptModuleStateStore &stateStore,
                         const char *moduleName,
                         Snapshot &snapshot,
                         int &angelScriptCode,
                         std::string &errorMessage);
    void RemoveForReplacement(ScriptManager &manager,
                              ScriptModuleRegistry &registry,
                              ScriptModuleStateStore &stateStore,
                              const char *moduleName,
                              Snapshot &snapshot);
    bool RestoreSnapshot(ScriptManager &manager,
                         ScriptModuleRegistry &registry,
                         ScriptModuleStateStore &stateStore,
                         ScriptImportBinder &importBinder,
                         const char *moduleName,
                         Snapshot &snapshot,
                         int &angelScriptCode,
                         std::string &errorMessage);
    bool CommitBytecodeCandidate(ScriptManager &manager,
                                 ScriptModuleRegistry &registry,
                                 ScriptModuleStateStore &stateStore,
                                 ScriptImportBinder &importBinder,
                                 const char *moduleName,
                                 const std::vector<unsigned char> &candidateByteCode,
                                 const char *commitFailedMessage,
                                 const char *rollbackFailedPrefix,
                                 asIScriptModule **outCommittedModule,
                                 int &angelScriptCode,
                                 std::string &errorMessage);

    std::shared_ptr<CachedScript> BuildTransientModule(
        ScriptManager &manager,
        const char *moduleName,
        const std::vector<std::tuple<std::string, std::string>> &sections,
        bool sourceSnapshotSections,
        bool memorySections,
        ScriptApiDiagnostics &diagnosticsStore,
        int &angelScriptCode,
        std::string &diagnostics,
        std::vector<CapturedScriptMessage> *messages);
};

#endif // CK_SCRIPT_MODULE_REPLACER_H
