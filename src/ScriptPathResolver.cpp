#include "ScriptPathResolver.h"

#include "CKContext.h"
#include "CKPathManager.h"

void ScriptPathResolver::SetupScriptPathCategory(CKContext *context) {
    if (!context || m_ScriptPathCategoryIndex != -1) {
        return;
    }

    XString category = "Script Paths";
    CKPathManager *pathManager = context->GetPathManager();
    m_ScriptPathCategoryIndex = pathManager->GetCategoryIndex(category);
    if (m_ScriptPathCategoryIndex == -1) {
        m_ScriptPathCategoryIndex = pathManager->AddCategory(category);
    }
}

CKERROR ScriptPathResolver::ResolveScriptFileName(CKContext *context, XString &filename) {
    if (!context) {
        return CKERR_INVALIDPARAMETER;
    }

    SetupScriptPathCategory(context);
    CKPathManager *pathManager = context->GetPathManager();
    return pathManager->ResolveFileName(filename, m_ScriptPathCategoryIndex);
}
