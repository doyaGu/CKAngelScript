#ifndef CK_SCRIPTSCENE_H
#define CK_SCRIPTSCENE_H

#include <angelscript.h>

#include "ScriptObjectRef.h"

void RegisterScriptSceneCore(asIScriptEngine *engine);
void RegisterScriptSceneRuntime(asIScriptEngine *engine);
bool ScriptSceneIsObjectInScene(CKObject *object, CKScene *scene);

#endif // CK_SCRIPTSCENE_H
