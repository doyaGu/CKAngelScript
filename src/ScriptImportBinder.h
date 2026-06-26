#ifndef CK_SCRIPT_IMPORT_BINDER_H
#define CK_SCRIPT_IMPORT_BINDER_H

#include <string>
#include <vector>

#include "ScriptModuleStateStore.h"

class ScriptManager;

class ScriptImportBinder {
public:
    bool Rebind(ScriptManager &manager,
                const std::vector<ScriptImportBindingEdge> &bindings,
                int &angelScriptCode,
                std::string &errorMessage);
};

#endif // CK_SCRIPT_IMPORT_BINDER_H
