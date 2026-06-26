#ifndef CK_SCRIPT_MODULE_REPLACER_H
#define CK_SCRIPT_MODULE_REPLACER_H

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "CKAngelScript.h"
#include "ScriptApiDiagnostics.h"
#include "ScriptCache.h"
#include "ScriptModuleStateStore.h"

class ScriptManager;

class ScriptModuleReplacer {
public:
    struct Snapshot {
        std::shared_ptr<CachedScript> Cache;
        std::vector<std::tuple<std::string, std::string>> Sections;
        ScriptMetadata Metadata;
        std::vector<ScriptImportBindingEdge> ImportBindings;
        std::vector<unsigned char> ByteCode;
        bool SourceSnapshotSections = false;
        bool HasModule = false;
    };

    CKAS_STATUS ReplaceFromSections(ScriptManager &manager,
                                    const char *moduleName,
                                    const std::vector<std::tuple<std::string, std::string>> &sections,
                                    bool sourceSnapshotSections,
                                    CKAngelScriptResult *result);

    bool CaptureSnapshot(ScriptManager &manager,
                         const char *moduleName,
                         Snapshot &snapshot,
                         int &angelScriptCode,
                         std::string &errorMessage);
    void RemoveForReplacement(ScriptManager &manager, const char *moduleName, Snapshot &snapshot);
    bool RestoreSnapshot(ScriptManager &manager,
                         const char *moduleName,
                         Snapshot &snapshot,
                         int &angelScriptCode,
                         std::string &errorMessage);

private:
    std::shared_ptr<CachedScript> BuildTransientModule(
        ScriptManager &manager,
        const char *moduleName,
        const std::vector<std::tuple<std::string, std::string>> &sections,
        bool sourceSnapshotSections,
        int &angelScriptCode,
        std::string &diagnostics,
        std::vector<CapturedScriptMessage> *messages);
};

#endif // CK_SCRIPT_MODULE_REPLACER_H
