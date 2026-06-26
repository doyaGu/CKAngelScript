#ifndef CK_SCRIPT_PATH_RESOLVER_H
#define CK_SCRIPT_PATH_RESOLVER_H

#include "CKAll.h"

class CKContext;

class ScriptPathResolver {
public:
    CKERROR ResolveScriptFileName(CKContext *context, XString &filename);

private:
    void SetupScriptPathCategory(CKContext *context);

    int m_ScriptPathCategoryIndex = -1;
};

#endif // CK_SCRIPT_PATH_RESOLVER_H
