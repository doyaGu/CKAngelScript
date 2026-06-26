#ifndef CK_SCRIPT_CK_OBJECT_RETAINER_H
#define CK_SCRIPT_CK_OBJECT_RETAINER_H

#include <unordered_map>
#include <vector>

#include <angelscript.h>

#include "CKAll.h"

class ScriptCKObjectRetainer {
public:
    void *GetData(CK_ID id) const;
    void SetData(CK_ID id, void *data);
    void ReleaseData(CK_ID id);
    void Clear();

    void TrackCallback(CK_ID id, asIScriptFunction *func);
    bool UntrackCallback(CK_ID id, asIScriptFunction *func);
    void ReleaseCallbacks(CK_ID id);

private:
    void ReleaseCallbackList(std::vector<asIScriptFunction *> callbacks);

    std::unordered_map<CK_ID, void *> m_Data;
    std::unordered_map<CK_ID, std::vector<asIScriptFunction *> > m_Callbacks;
};

#endif // CK_SCRIPT_CK_OBJECT_RETAINER_H
