#include "ScriptSelfTests.h"

#include "angelscript.h"

#include "CKAll.h"
#include "CKAttributeManager.h"
#include "CKStateChunk.h"
#include "CKParameterOperation.h"
#include "CKParameterManager.h"
#include "ScriptParameterRegistry.h"

namespace {

bool ExecuteCKEnumStructProbe(asIScriptEngine *engine,
                              asIScriptFunction *function,
                              CKEnumStruct &input,
                              CKEnumStruct *other,
                              bool expectException,
                              const char *label,
                              std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, &input);
    }
    if (r >= 0 && other) {
        r = scriptContext->SetArgObject(1, other);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKFlagsStructProbe(asIScriptEngine *engine,
                               asIScriptFunction *function,
                               CKFlagsStruct &input,
                               bool expectException,
                               const char *label,
                               std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, &input);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKStructStructProbe(asIScriptEngine *engine,
                                asIScriptFunction *function,
                                CKStructStruct &input,
                                bool expectException,
                                const char *label,
                                std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, &input);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterTypeDescProbe(asIScriptEngine *engine,
                                     asIScriptFunction *function,
                                     CKContext *context,
                                     bool expectException,
                                     const char *label,
                                     std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, context);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteNoArgIntProbe(asIScriptEngine *engine,
                          asIScriptFunction *function,
                          bool expectException,
                          const char *label,
                          std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKObjectProbe(asIScriptEngine *engine,
                          asIScriptFunction *function,
                          CKContext *context,
                          CKObject *object,
                          bool expectException,
                          const char *label,
                          std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, context);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, object);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKBehaviorIOProbe(asIScriptEngine *engine,
                              asIScriptFunction *function,
                              CKBehavior *behavior,
                              CKBehaviorIO *input,
                              CKBehaviorIO *output,
                              bool expectException,
                              const char *label,
                              std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, behavior);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, input);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, output);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKBehaviorProbe(asIScriptEngine *engine,
                            asIScriptFunction *function,
                            CKBehavior *behavior,
                            bool expectException,
                            const char *label,
                            std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, behavior);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKSceneObjectProbe(asIScriptEngine *engine,
                               asIScriptFunction *function,
                               CKSceneObject *object,
                               CKScene *scene,
                               bool expectException,
                               const char *label,
                               std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, object);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, scene);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKBeObjectProbe(asIScriptEngine *engine,
                            asIScriptFunction *function,
                            CKBeObject *object,
                            CKBehavior *behavior,
                            CKGroup *group,
                            bool expectException,
                            const char *label,
                            std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, object);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, behavior);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, group);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKGroupProbe(asIScriptEngine *engine,
                         asIScriptFunction *function,
                         CKGroup *group,
                         CKBeObject *first,
                         CKBeObject *second,
                         CKBeObject *third,
                         bool expectException,
                         const char *label,
                         std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, group);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, first);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, second);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(3, third);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCK2dEntityProbe(asIScriptEngine *engine,
                            asIScriptFunction *function,
                            CK2dEntity *entity,
                            CK2dEntity *child,
                            CKMaterial *material,
                            bool expectException,
                            const char *label,
                            std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, entity);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, child);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, material);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCK2dEntityCopyNullProbe(asIScriptEngine *engine,
                                    asITypeInfo *entityType,
                                    CK2dEntity *entity,
                                    CKDependenciesContext &dependencies,
                                    std::string &error) {
    asIScriptFunction *copyMethod = entityType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)");
    if (!copyMethod) {
        error = "CK2dEntity Copy(null) probe could not find Copy method.";
        return false;
    }

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "CK2dEntity Copy(null) probe could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(copyMethod);
    if (r >= 0) {
        r = scriptContext->SetObject(entity);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, nullptr);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, &dependencies);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    const bool ok = r == asEXECUTION_EXCEPTION;
    if (!ok) {
        error = "CK2dEntity Copy(null) probe expected a script exception, got code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCK3dEntityProbe(asIScriptEngine *engine,
                            asIScriptFunction *function,
                            CK3dEntity *entity,
                            CK3dEntity *child,
                            CKMesh *mesh,
                            CKObjectAnimation *animation,
                            bool expectException,
                            const char *label,
                            std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, entity);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, child);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, mesh);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(3, animation);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCK3dEntityCopyNullProbe(asIScriptEngine *engine,
                                    asITypeInfo *entityType,
                                    CK3dEntity *entity,
                                    CKDependenciesContext &dependencies,
                                    std::string &error) {
    asIScriptFunction *copyMethod = entityType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)");
    if (!copyMethod) {
        error = "CK3dEntity Copy(null) probe could not find Copy method.";
        return false;
    }

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "CK3dEntity Copy(null) probe could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(copyMethod);
    if (r >= 0) {
        r = scriptContext->SetObject(entity);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, nullptr);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, &dependencies);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    const bool ok = r == asEXECUTION_EXCEPTION;
    if (!ok) {
        error = "CK3dEntity Copy(null) probe expected a script exception, got code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCK3dObjectProbe(asIScriptEngine *engine,
                            asIScriptFunction *function,
                            CK3dObject *object,
                            CK3dObject *child,
                            CKMesh *mesh,
                            CKObjectAnimation *animation,
                            bool expectException,
                            const char *label,
                            std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, object);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, child);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, mesh);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(3, animation);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCK3dObjectCopyNullProbe(asIScriptEngine *engine,
                                    asITypeInfo *objectType,
                                    CK3dObject *object,
                                    CKDependenciesContext &dependencies,
                                    std::string &error) {
    asIScriptFunction *copyMethod = objectType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)");
    if (!copyMethod) {
        error = "CK3dObject Copy(null) probe could not find Copy method.";
        return false;
    }

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "CK3dObject Copy(null) probe could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(copyMethod);
    if (r >= 0) {
        r = scriptContext->SetObject(object);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, nullptr);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, &dependencies);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    const bool ok = r == asEXECUTION_EXCEPTION;
    if (!ok) {
        error = "CK3dObject Copy(null) probe expected a script exception, got code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKCameraProbe(asIScriptEngine *engine,
                          asIScriptFunction *function,
                          CKCamera *camera,
                          CK3dEntity *target,
                          CKMesh *mesh,
                          CKObjectAnimation *animation,
                          bool expectException,
                          const char *label,
                          std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, camera);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, target);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, mesh);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(3, animation);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKCameraCopyNullProbe(asIScriptEngine *engine,
                                  asITypeInfo *cameraType,
                                  CKCamera *camera,
                                  CKDependenciesContext &dependencies,
                                  std::string &error) {
    asIScriptFunction *copyMethod = cameraType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)");
    if (!copyMethod) {
        error = "CKCamera Copy(null) probe could not find Copy method.";
        return false;
    }

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "CKCamera Copy(null) probe could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(copyMethod);
    if (r >= 0) {
        r = scriptContext->SetObject(camera);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, nullptr);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, &dependencies);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    const bool ok = r == asEXECUTION_EXCEPTION;
    if (!ok) {
        error = "CKCamera Copy(null) probe expected a script exception, got code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKLightProbe(asIScriptEngine *engine,
                         asIScriptFunction *function,
                         CKLight *light,
                         CKMesh *mesh,
                         CKObjectAnimation *animation,
                         bool expectException,
                         const char *label,
                         std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) r = scriptContext->SetArgObject(0, light);
    if (r >= 0) r = scriptContext->SetArgObject(1, mesh);
    if (r >= 0) r = scriptContext->SetArgObject(2, animation);
    if (r >= 0) r = scriptContext->Execute();

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKTargetLightProbe(asIScriptEngine *engine,
                               asIScriptFunction *function,
                               CKTargetLight *light,
                               CK3dEntity *target,
                               CKMesh *mesh,
                               CKObjectAnimation *animation,
                               bool expectException,
                               const char *label,
                               std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) r = scriptContext->SetArgObject(0, light);
    if (r >= 0) r = scriptContext->SetArgObject(1, target);
    if (r >= 0) r = scriptContext->SetArgObject(2, mesh);
    if (r >= 0) r = scriptContext->SetArgObject(3, animation);
    if (r >= 0) r = scriptContext->Execute();

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKSprite3DProbe(asIScriptEngine *engine,
                            asIScriptFunction *function,
                            CKSprite3D *sprite,
                            CKMaterial *material,
                            CKMesh *mesh,
                            CKObjectAnimation *animation,
                            bool expectException,
                            const char *label,
                            std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) r = scriptContext->SetArgObject(0, sprite);
    if (r >= 0) r = scriptContext->SetArgObject(1, material);
    if (r >= 0) r = scriptContext->SetArgObject(2, mesh);
    if (r >= 0) r = scriptContext->SetArgObject(3, animation);
    if (r >= 0) r = scriptContext->Execute();

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKGridProbe(asIScriptEngine *engine,
                        asIScriptFunction *function,
                        CKGrid *grid,
                        CK3dEntity *entity,
                        CKMesh *mesh,
                        CKObjectAnimation *animation,
                        bool expectException,
                        const char *label,
                        std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) r = scriptContext->SetArgObject(0, grid);
    if (r >= 0) r = scriptContext->SetArgObject(1, entity);
    if (r >= 0) r = scriptContext->SetArgObject(2, mesh);
    if (r >= 0) r = scriptContext->SetArgObject(3, animation);
    if (r >= 0) r = scriptContext->Execute();

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKCurveProbe(asIScriptEngine *engine,
                         asIScriptFunction *function,
                         CKCurve *curve,
                         CKCurve *otherCurve,
                         CKCurvePoint *point0,
                         CKCurvePoint *point1,
                         CKCurvePoint *point2,
                         CKCurvePoint *otherPoint,
                         CKMesh *mesh,
                         CKObjectAnimation *animation,
                         bool expectException,
                         const char *label,
                         std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) r = scriptContext->SetArgObject(0, curve);
    if (r >= 0) r = scriptContext->SetArgObject(1, otherCurve);
    if (r >= 0) r = scriptContext->SetArgObject(2, point0);
    if (r >= 0) r = scriptContext->SetArgObject(3, point1);
    if (r >= 0) r = scriptContext->SetArgObject(4, point2);
    if (r >= 0) r = scriptContext->SetArgObject(5, otherPoint);
    if (r >= 0) r = scriptContext->SetArgObject(6, mesh);
    if (r >= 0) r = scriptContext->SetArgObject(7, animation);
    if (r >= 0) r = scriptContext->Execute();

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKCurveCopyNullProbe(asIScriptEngine *engine,
                                 asITypeInfo *curveType,
                                 CKCurve *curve,
                                 CKDependenciesContext &dependencies,
                                 std::string &error) {
    asIScriptFunction *copyMethod = curveType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)");
    if (!copyMethod) {
        error = "CKCurve Copy(null) probe could not find Copy method.";
        return false;
    }

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "CKCurve Copy(null) probe could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(copyMethod);
    if (r >= 0) r = scriptContext->SetObject(curve);
    if (r >= 0) r = scriptContext->SetArgObject(0, nullptr);
    if (r >= 0) r = scriptContext->SetArgObject(1, &dependencies);
    if (r >= 0) r = scriptContext->Execute();

    const bool ok = r == asEXECUTION_EXCEPTION;
    if (!ok) {
        error = "CKCurve Copy(null) probe expected a script exception, got code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKCurvePointProbe(asIScriptEngine *engine,
                              asIScriptFunction *function,
                              CKCurve *curve,
                              CKCurvePoint *point0,
                              CKCurvePoint *point1,
                              bool expectException,
                              const char *label,
                              std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) r = scriptContext->SetArgObject(0, curve);
    if (r >= 0) r = scriptContext->SetArgObject(1, point0);
    if (r >= 0) r = scriptContext->SetArgObject(2, point1);
    if (r >= 0) r = scriptContext->Execute();

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKCurvePointCopyNullProbe(asIScriptEngine *engine,
                                      asITypeInfo *pointType,
                                      CKCurvePoint *point,
                                      CKDependenciesContext &dependencies,
                                      std::string &error) {
    asIScriptFunction *copyMethod = pointType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)");
    if (!copyMethod) {
        error = "CKCurvePoint Copy(null) probe could not find Copy method.";
        return false;
    }

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "CKCurvePoint Copy(null) probe could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(copyMethod);
    if (r >= 0) r = scriptContext->SetObject(point);
    if (r >= 0) r = scriptContext->SetArgObject(0, nullptr);
    if (r >= 0) r = scriptContext->SetArgObject(1, &dependencies);
    if (r >= 0) r = scriptContext->Execute();

    const bool ok = r == asEXECUTION_EXCEPTION;
    if (!ok) {
        error = "CKCurvePoint Copy(null) probe expected a script exception, got code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKCharacterProbe(asIScriptEngine *engine,
                             asIScriptFunction *function,
                             CKCharacter *character,
                             CKBodyPart *bodyPart,
                             CKAnimation *animation,
                             CK3dEntity *floorRef,
                             CKMesh *mesh,
                             CKObjectAnimation *objectAnimation,
                             bool expectException,
                             const char *label,
                             std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) r = scriptContext->SetArgObject(0, character);
    if (r >= 0) r = scriptContext->SetArgObject(1, bodyPart);
    if (r >= 0) r = scriptContext->SetArgObject(2, animation);
    if (r >= 0) r = scriptContext->SetArgObject(3, floorRef);
    if (r >= 0) r = scriptContext->SetArgObject(4, mesh);
    if (r >= 0) r = scriptContext->SetArgObject(5, objectAnimation);
    if (r >= 0) r = scriptContext->Execute();

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKCharacterCopyNullProbe(asIScriptEngine *engine,
                                     asITypeInfo *characterType,
                                     CKCharacter *character,
                                     CKDependenciesContext &dependencies,
                                     std::string &error) {
    asIScriptFunction *copyMethod = characterType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)");
    if (!copyMethod) {
        error = "CKCharacter Copy(null) probe could not find Copy method.";
        return false;
    }

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "CKCharacter Copy(null) probe could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(copyMethod);
    if (r >= 0) r = scriptContext->SetObject(character);
    if (r >= 0) r = scriptContext->SetArgObject(0, nullptr);
    if (r >= 0) r = scriptContext->SetArgObject(1, &dependencies);
    if (r >= 0) r = scriptContext->Execute();

    const bool ok = r == asEXECUTION_EXCEPTION;
    if (!ok) {
        error = "CKCharacter Copy(null) probe expected a script exception, got code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKBodyPartProbe(asIScriptEngine *engine,
                            asIScriptFunction *function,
                            CKBodyPart *bodyPart,
                            CKCharacter *character,
                            CKAnimation *animation,
                            bool expectException,
                            const char *label,
                            std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) r = scriptContext->SetArgObject(0, bodyPart);
    if (r >= 0) r = scriptContext->SetArgObject(1, character);
    if (r >= 0) r = scriptContext->SetArgObject(2, animation);
    if (r >= 0) r = scriptContext->Execute();

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKBodyPartCopyNullProbe(asIScriptEngine *engine,
                                    asITypeInfo *bodyPartType,
                                    CKBodyPart *bodyPart,
                                    CKDependenciesContext &dependencies,
                                    std::string &error) {
    asIScriptFunction *copyMethod = bodyPartType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)");
    if (!copyMethod) {
        error = "CKBodyPart Copy(null) probe could not find Copy method.";
        return false;
    }

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "CKBodyPart Copy(null) probe could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(copyMethod);
    if (r >= 0) r = scriptContext->SetObject(bodyPart);
    if (r >= 0) r = scriptContext->SetArgObject(0, nullptr);
    if (r >= 0) r = scriptContext->SetArgObject(1, &dependencies);
    if (r >= 0) r = scriptContext->Execute();

    const bool ok = r == asEXECUTION_EXCEPTION;
    if (!ok) {
        error = "CKBodyPart Copy(null) probe expected a script exception, got code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKAnimControllerProbe(asIScriptEngine *engine,
                                  asIScriptFunction *function,
                                  CKAnimController *positionController,
                                  CKAnimController *rotationController,
                                  bool expectException,
                                  const char *label,
                                  std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, positionController);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, rotationController);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKMorphControllerProbe(asIScriptEngine *engine,
                                   asIScriptFunction *function,
                                   CKMorphController *controller,
                                   bool expectException,
                                   const char *label,
                                   std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, controller);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKAnimationProbe(asIScriptEngine *engine,
                             asIScriptFunction *function,
                             CKAnimation *animation,
                             CKAnimation *other,
                             bool expectException,
                             const char *label,
                             std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, animation);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, other);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKAnimationCopyNullProbe(asIScriptEngine *engine,
                                     asITypeInfo *animationType,
                                     CKAnimation *animation,
                                     CKDependenciesContext &dependencies,
                                     std::string &error) {
    asIScriptFunction *copyMethod = animationType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)");
    if (!copyMethod) {
        error = "CKAnimation Copy(null) probe could not find Copy method.";
        return false;
    }

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "CKAnimation Copy(null) probe could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(copyMethod);
    if (r >= 0) {
        r = scriptContext->SetObject(animation);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, nullptr);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, &dependencies);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    const bool ok = r == asEXECUTION_EXCEPTION;
    if (!ok) {
        error = "CKAnimation Copy(null) probe expected a script exception, got code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKObjectAnimationProbe(asIScriptEngine *engine,
                                   asIScriptFunction *function,
                                   CKObjectAnimation *animation,
                                   CKObjectAnimation *other,
                                   bool expectException,
                                   const char *label,
                                   std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, animation);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, other);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKBehaviorLinkProbe(asIScriptEngine *engine,
                                asIScriptFunction *function,
                                CKBehavior *root,
                                CKBehaviorLink *link,
                                CKBehaviorIO *sourceOutput,
                                CKBehaviorIO *targetInput,
                                bool expectException,
                                const char *label,
                                std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, root);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, link);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, sourceOutput);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(3, targetInput);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKDataArrayProbe(asIScriptEngine *engine,
                             asIScriptFunction *function,
                             CKDataArray *array,
                             CKObject *object,
                             bool expectException,
                             const char *label,
                             std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, array);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, object);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKWaveSoundProbe(asIScriptEngine *engine,
                             asIScriptFunction *function,
                             CKWaveSound *sound,
                             bool expectException,
                             const char *label,
                             std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, sound);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterLocalProbe(asIScriptEngine *engine,
                                  asIScriptFunction *function,
                                  CKParameterLocal *parameter,
                                  bool expectException,
                                  const char *label,
                                  std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, parameter);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterLocalOwnerProbe(asIScriptEngine *engine,
                                       asIScriptFunction *function,
                                       CKBehavior *owner,
                                       CKParameterLocal *parameter,
                                       bool expectException,
                                       const char *label,
                                       std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, owner);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, parameter);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterInProbe(asIScriptEngine *engine,
                               asIScriptFunction *function,
                               CKParameterIn *parameter,
                               bool expectException,
                               const char *label,
                               std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, parameter);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterInSourceProbe(asIScriptEngine *engine,
                                     asIScriptFunction *function,
                                     CKParameterIn *parameter,
                                     CKParameter *sourceA,
                                     CKParameter *sourceB,
                                     CKParameterIn *shared,
                                     bool expectException,
                                     const char *label,
                                     std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, parameter);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, sourceA);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, sourceB);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(3, shared);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterInOwnerProbe(asIScriptEngine *engine,
                                    asIScriptFunction *function,
                                    CKParameterIn *parameter,
                                    CKBehavior *owner,
                                    bool expectException,
                                    const char *label,
                                    std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, parameter);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, owner);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterOperationProbe(asIScriptEngine *engine,
                                      asIScriptFunction *function,
                                      CKParameterOperation *operation,
                                      bool expectException,
                                      const char *label,
                                      std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, operation);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterOperationOwnerProbe(asIScriptEngine *engine,
                                           asIScriptFunction *function,
                                           CKParameterOperation *operation,
                                           CKBehavior *owner,
                                           bool expectException,
                                           const char *label,
                                           std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, operation);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, owner);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterOutDestinationProbe(asIScriptEngine *engine,
                                           asIScriptFunction *function,
                                           CKParameterOut *parameter,
                                           CKParameter *destinationA,
                                           CKParameter *destinationB,
                                           bool expectException,
                                           const char *label,
                                           std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, parameter);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, destinationA);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, destinationB);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterOutOwnerProbe(asIScriptEngine *engine,
                                     asIScriptFunction *function,
                                     CKBehavior *owner,
                                     CKParameterOut *parameter,
                                     CKParameterOut *replacement,
                                     bool expectException,
                                     const char *label,
                                     std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, owner);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, parameter);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(2, replacement);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKStateChunkProbe(asIScriptEngine *engine,
                              asIScriptFunction *function,
                              CKStateChunk *chunk,
                              CKContext *context,
                              bool expectException,
                              const char *label,
                              std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, chunk);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(1, context);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKAttributeDescProbe(asIScriptEngine *engine,
                                 asIScriptFunction *function,
                                 bool expectException,
                                 const char *label,
                                 std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool RunCKEnumStructScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKEnumStruct script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKEnumStructSelfTest";
    const char *source =
        "int ProbeEnumStruct(const CKEnumStruct &in input) {\n"
        "  if (input.GetNumEnums() != 2) return 1;\n"
        "  if (input.GetEnumValue(0) != 7 || input.GetEnumValue(1) != 9) return 2;\n"
        "  if (input.GetEnumDescription(0) != \"First\" || input.GetEnumDescription(1) != \"Second\") return 3;\n"
        "  CKEnumStruct copied(input);\n"
        "  if (copied.GetNumEnums() != 2 || copied.GetEnumValue(1) != 9) return 4;\n"
        "  CKEnumStruct assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.GetNumEnums() != 2 || assigned.GetEnumDescription(0) != \"First\") return 5;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeEnumStructOutOfRange(const CKEnumStruct &in input) {\n"
        "  return input.GetEnumValue(2);\n"
        "}\n"
        "\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKEnumStruct self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-enum-struct-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKEnumStruct self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKEnumStruct self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeEnumStruct(const CKEnumStruct &in)");
    asIScriptFunction *outOfRange = module->GetFunctionByDecl("int ProbeEnumStructOutOfRange(const CKEnumStruct &in)");
    if (!probe || !outOfRange) {
        engine->DiscardModule(moduleName);
        error = "CKEnumStruct self-test functions were not found.";
        return false;
    }

    int values[2] = {7, 9};
    CKSTRING descriptions[2] = {const_cast<CKSTRING>("First"), const_cast<CKSTRING>("Second")};
    CKEnumStruct input;
    input.NbData = 2;
    input.Vals = values;
    input.Desc = descriptions;

    bool ok = ExecuteCKEnumStructProbe(engine, probe, input, nullptr, false, "CKEnumStruct copy probe", error) &&
              ExecuteCKEnumStructProbe(engine, outOfRange, input, nullptr, true, "CKEnumStruct out-of-range probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKFlagsStructScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKFlagsStruct script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKFlagsStructSelfTest";
    const char *source =
        "int ProbeFlagsStruct(const CKFlagsStruct &in input) {\n"
        "  if (input.GetNumFlags() != 2) return 1;\n"
        "  if (input.GetFlagValue(0) != 1 || input.GetFlagValue(1) != 2) return 2;\n"
        "  if (input.GetFlagDescription(0) != \"Alpha\" || input.GetFlagDescription(1) != \"Beta\") return 3;\n"
        "  CKFlagsStruct copied(input);\n"
        "  if (copied.GetNumFlags() != 2 || copied.GetFlagValue(1) != 2) return 4;\n"
        "  CKFlagsStruct assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.GetNumFlags() != 2 || assigned.GetFlagDescription(0) != \"Alpha\") return 5;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeFlagsStructOutOfRange(const CKFlagsStruct &in input) {\n"
        "  return input.GetFlagValue(2);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKFlagsStruct self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-flags-struct-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKFlagsStruct self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKFlagsStruct self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeFlagsStruct(const CKFlagsStruct &in)");
    asIScriptFunction *outOfRange = module->GetFunctionByDecl("int ProbeFlagsStructOutOfRange(const CKFlagsStruct &in)");
    if (!probe || !outOfRange) {
        engine->DiscardModule(moduleName);
        error = "CKFlagsStruct self-test functions were not found.";
        return false;
    }

    int values[2] = {1, 2};
    CKSTRING descriptions[2] = {const_cast<CKSTRING>("Alpha"), const_cast<CKSTRING>("Beta")};
    CKFlagsStruct input;
    input.NbData = 2;
    input.Vals = values;
    input.Desc = descriptions;

    bool ok = ExecuteCKFlagsStructProbe(engine, probe, input, false, "CKFlagsStruct copy probe", error) &&
              ExecuteCKFlagsStructProbe(engine, outOfRange, input, true, "CKFlagsStruct out-of-range probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKStructStructScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKStructStruct script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKStructStructSelfTest";
    const char *source =
        "bool SameGuid(const CKGUID &in lhs, const CKGUID &in rhs) {\n"
        "  return lhs.d1 == rhs.d1 && lhs.d2 == rhs.d2;\n"
        "}\n"
        "int ProbeStructStruct(const CKStructStruct &in input) {\n"
        "  if (input.GetNumSubParam() != 2) return 1;\n"
        "  CKGUID first(0x11111111, 0x22222222);\n"
        "  CKGUID second(0x33333333, 0x44444444);\n"
        "  if (!SameGuid(input.GetSubParamGuid(0), first) || !SameGuid(input.GetSubParamGuid(1), second)) return 2;\n"
        "  if (input.GetSubParamDescription(0) != \"First\" || input.GetSubParamDescription(1) != \"Second\") return 3;\n"
        "  CKStructStruct copied(input);\n"
        "  if (copied.GetNumSubParam() != 2 || !SameGuid(copied.GetSubParamGuid(1), second)) return 4;\n"
        "  CKStructStruct assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.GetNumSubParam() != 2 || assigned.GetSubParamDescription(0) != \"First\") return 5;\n"
        "  CKGUID guidValue = assigned.GetSubParamGuid(0);\n"
        "  guidValue.d1 = 0;\n"
        "  if (!SameGuid(assigned.GetSubParamGuid(0), first)) return 6;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeStructStructOutOfRange(const CKStructStruct &in input) {\n"
        "  CKGUID guid = input.GetSubParamGuid(2);\n"
        "  return int(guid.d1);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKStructStruct self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-struct-struct-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKStructStruct self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKStructStruct self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeStructStruct(const CKStructStruct &in)");
    asIScriptFunction *outOfRange = module->GetFunctionByDecl("int ProbeStructStructOutOfRange(const CKStructStruct &in)");
    if (!probe || !outOfRange) {
        engine->DiscardModule(moduleName);
        error = "CKStructStruct self-test functions were not found.";
        return false;
    }

    CKGUID guids[2] = {CKGUID(0x11111111, 0x22222222), CKGUID(0x33333333, 0x44444444)};
    CKSTRING descriptions[2] = {const_cast<CKSTRING>("First"), const_cast<CKSTRING>("Second")};
    CKStructStruct input;
    input.NbData = 2;
    input.Guids = guids;
    input.Desc = descriptions;

    bool ok = ExecuteCKStructStructProbe(engine, probe, input, false, "CKStructStruct copy probe", error) &&
              ExecuteCKStructStructProbe(engine, outOfRange, input, true, "CKStructStruct out-of-range probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKGUIDScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKGUID script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *guidType = engine->GetTypeInfoByDecl("CKGUID");
    if (!guidType) {
        error = "CKGUID type is not registered.";
        return false;
    }
    if ((guidType->GetFlags() & asOBJ_APP_CLASS_ALLINTS) == 0) {
        error = "CKGUID type is missing asOBJ_APP_CLASS_ALLINTS.";
        return false;
    }
    if (!guidType->GetMethodByDecl("int opCmp(const CKGUID &in other) const")) {
        error = "CKGUID comparison method is not registered.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKGUIDSelfTest";
    const char *source =
        "int ProbeCKGUID() {\n"
        "  CKGUID empty;\n"
        "  if (empty.IsValid()) return 1;\n"
        "  CKGUID direct(0x11111111, 0x22222222);\n"
        "  if (direct.d1 != 0x11111111 || direct.d2 != 0x22222222) return 2;\n"
        "  CKGUID listed = {0x33333333, 0x44444444};\n"
        "  if (listed.d1 != 0x33333333 || listed.d2 != 0x44444444) return 3;\n"
        "  listed.d1 = 0x55555555;\n"
        "  listed.d2 = 0x66666666;\n"
        "  if (listed.d1 != 0x55555555 || listed.d2 != 0x66666666) return 4;\n"
        "  CKGUID copied(listed);\n"
        "  if (copied.d1 != listed.d1 || copied.d2 != listed.d2) return 5;\n"
        "  CKGUID assigned;\n"
        "  assigned = direct;\n"
        "  if (assigned.d1 != direct.d1 || assigned.d2 != direct.d2) return 6;\n"
        "  if (direct.opCmp(assigned) != 0) return 7;\n"
        "  if (direct.opCmp(listed) >= 0) return 8;\n"
        "  if (!direct.IsValid()) return 9;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKGUID self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-guid-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKGUID self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKGUID self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKGUID()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKGUID self-test function was not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKGUID value probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKSquareScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKSquare script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *squareType = engine->GetTypeInfoByDecl("CKSquare");
    if (!squareType) {
        error = "CKSquare self-test could not find the registered type.";
        return false;
    }
    bool hasValueProperty = false;
    for (asUINT i = 0; i < squareType->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        if (squareType->GetProperty(i, &propertyName) >= 0 && propertyName && std::strcmp(propertyName, "val") == 0) {
            hasValueProperty = true;
            break;
        }
    }
    if (!hasValueProperty ||
        !squareType->GetMethodByDecl("CKSquare& opAssign(const CKSquare &in other)")) {
        error = "CKSquare self-test could not find expected property or assignment method.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKSquareSelfTest";
    const char *source =
        "int ProbeCKSquare() {\n"
        "  CKSquare square;\n"
        "  square.val = 0x12345678;\n"
        "  if (square.val != 0x12345678) return 1;\n"
        "  CKSquare copied(square);\n"
        "  if (copied.val != 0x12345678) return 2;\n"
        "  CKSquare assigned;\n"
        "  assigned.val = 7;\n"
        "  assigned = copied;\n"
        "  if (assigned.val != 0x12345678) return 3;\n"
        "  copied.val = 0x87654321;\n"
        "  if (assigned.val != 0x12345678) return 4;\n"
        "  if (copied.val != 0x87654321) return 5;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKSquare self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("cksquare-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSquare self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSquare self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKSquare()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKSquare self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKSquare value probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKStatsScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKStats script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *statsType = engine->GetTypeInfoByDecl("CKStats");
    if (!statsType) {
        error = "CKStats self-test could not find the registered type.";
        return false;
    }
    if (!statsType->GetMethodByDecl("CKStats& opAssign(const CKStats &in other)") ||
        !statsType->GetMethodByDecl("float GetUserProfile(int index) const") ||
        !statsType->GetMethodByDecl("void SetUserProfile(int index, float value)")) {
        error = "CKStats self-test could not find assignment or user profile methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKStatsSelfTest";
    const char *source =
        "int ProbeCKStats() {\n"
        "  CKStats stats;\n"
        "  stats.TotalFrameTime = 16.0f;\n"
        "  stats.RenderTime = 7.0f;\n"
        "  stats.BehaviorsExecuted = 3;\n"
        "  stats.BehaviorDelayedLinks = 2;\n"
        "  stats.SetUserProfile(0, 1.5f);\n"
        "  stats.SetUserProfile(1, 2.5f);\n"
        "  if (stats.GetUserProfile(0) != 1.5f || stats.GetUserProfile(1) != 2.5f) return 1;\n"
        "  CKStats copied(stats);\n"
        "  if (copied.TotalFrameTime != 16.0f || copied.RenderTime != 7.0f) return 2;\n"
        "  if (copied.BehaviorsExecuted != 3 || copied.BehaviorDelayedLinks != 2) return 3;\n"
        "  if (copied.GetUserProfile(0) != 1.5f || copied.GetUserProfile(1) != 2.5f) return 4;\n"
        "  CKStats assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.TotalFrameTime != 16.0f || assigned.GetUserProfile(1) != 2.5f) return 5;\n"
        "  copied.SetUserProfile(0, 9.5f);\n"
        "  copied.RenderTime = 11.0f;\n"
        "  if (assigned.GetUserProfile(0) != 1.5f || assigned.RenderTime != 7.0f) return 6;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKStatsGetUserProfileLow() {\n"
        "  CKStats stats;\n"
        "  stats.GetUserProfile(-1);\n"
        "}\n"
        "void RejectCKStatsSetUserProfileHigh() {\n"
        "  CKStats stats;\n"
        "  stats.SetUserProfile(1000, 1.0f);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKStats self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckstats-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKStats self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKStats self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKStats()");
    asIScriptFunction *getLow = module->GetFunctionByDecl("void RejectCKStatsGetUserProfileLow()");
    asIScriptFunction *setHigh = module->GetFunctionByDecl("void RejectCKStatsSetUserProfileHigh()");
    if (!probe || !getLow || !setHigh) {
        engine->DiscardModule(moduleName);
        error = "CKStats self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKStats value probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, getLow, true, "CKStats low user profile rejection probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, setHigh, true, "CKStats high user profile rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPluginEntryBehaviorsDataScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPluginEntryBehaviorsData script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *dataType = engine->GetTypeInfoByDecl("CKPluginEntryBehaviorsData");
    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XGUIDArray");
    if (!dataType || !arrayType) {
        error = "CKPluginEntryBehaviorsData self-test could not find required registered types.";
        return false;
    }
    bool hasBehaviorGuidProperty = false;
    for (asUINT i = 0; i < dataType->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        if (dataType->GetProperty(i, &propertyName) >= 0 && propertyName && std::strcmp(propertyName, "m_BehaviorsGUID") == 0) {
            hasBehaviorGuidProperty = true;
            break;
        }
    }
    if (!hasBehaviorGuidProperty ||
        !dataType->GetMethodByDecl("CKPluginEntryBehaviorsData& opAssign(const CKPluginEntryBehaviorsData &in other)") ||
        !arrayType->GetMethodByDecl("void PushBack(const CKGUID &in o)") ||
        !arrayType->GetMethodByDecl("int Size() const")) {
        error = "CKPluginEntryBehaviorsData self-test could not find expected members.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPluginEntryBehaviorsDataSelfTest";
    const char *source =
        "int ProbeCKPluginEntryBehaviorsData() {\n"
        "  CKGUID first(0x11111111, 0x22222222);\n"
        "  CKGUID second(0x33333333, 0x44444444);\n"
        "  CKPluginEntryBehaviorsData data;\n"
        "  if (data.m_BehaviorsGUID.Size() != 0) return 1;\n"
        "  data.m_BehaviorsGUID.PushBack(first);\n"
        "  if (data.m_BehaviorsGUID.Size() != 1 || !(data.m_BehaviorsGUID[0] == first)) return 2;\n"
        "  CKPluginEntryBehaviorsData copied(data);\n"
        "  if (copied.m_BehaviorsGUID.Size() != 1 || !(copied.m_BehaviorsGUID[0] == first)) return 3;\n"
        "  copied.m_BehaviorsGUID.PushBack(second);\n"
        "  if (copied.m_BehaviorsGUID.Size() != 2 || data.m_BehaviorsGUID.Size() != 1) return 4;\n"
        "  CKPluginEntryBehaviorsData assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.m_BehaviorsGUID.Size() != 2) return 5;\n"
        "  if (!(assigned.m_BehaviorsGUID[0] == first) || !(assigned.m_BehaviorsGUID[1] == second)) return 6;\n"
        "  copied.m_BehaviorsGUID[0] = second;\n"
        "  if (!(assigned.m_BehaviorsGUID[0] == first)) return 7;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPluginEntryBehaviorsData self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-plugin-entry-behaviors-data-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginEntryBehaviorsData self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginEntryBehaviorsData self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPluginEntryBehaviorsData()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKPluginEntryBehaviorsData self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPluginEntryBehaviorsData value probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPluginInfoScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPluginInfo script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *infoType = engine->GetTypeInfoByDecl("CKPluginInfo");
    if (!infoType) {
        error = "CKPluginInfo self-test could not find the registered type.";
        return false;
    }
    if (!infoType->GetMethodByDecl("NativePointer get_m_InitInstanceFct() const") ||
        !infoType->GetMethodByDecl("void set_m_InitInstanceFct(NativePointer ptr)") ||
        !infoType->GetMethodByDecl("NativePointer get_m_ExitInstanceFct() const") ||
        !infoType->GetMethodByDecl("void set_m_ExitInstanceFct(NativePointer ptr)") ||
        !infoType->GetMethodByDecl("CKPluginInfo& opAssign(const CKPluginInfo &in other)")) {
        error = "CKPluginInfo self-test could not find expected guarded callback methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPluginInfoSelfTest";
    const char *source =
        "int ProbeCKPluginInfo() {\n"
        "  CKPluginInfo info;\n"
        "  if (info.m_GUID.d1 != 0 || info.m_GUID.d2 != 0) return 1;\n"
        "  if (info.m_Version != 0) return 2;\n"
        "  if (!info.m_InitInstanceFct.IsNull() || !info.m_ExitInstanceFct.IsNull()) return 3;\n"
        "  info.m_GUID = CKGUID(0x11111111, 0x22222222);\n"
        "  info.m_Version = 42;\n"
        "  info.m_InitInstanceFct = NativePointer();\n"
        "  info.m_ExitInstanceFct = NativePointer(uintptr_t(0));\n"
        "  CKPluginInfo copied(info);\n"
        "  if (copied.m_GUID.d1 != 0x11111111 || copied.m_GUID.d2 != 0x22222222) return 4;\n"
        "  if (copied.m_Version != 42 || !copied.m_InitInstanceFct.IsNull() || !copied.m_ExitInstanceFct.IsNull()) return 5;\n"
        "  CKPluginInfo assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.m_Version != 42 || assigned.m_GUID.d1 != 0x11111111) return 6;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKPluginInfoInitPointerWrite() {\n"
        "  CKPluginInfo info;\n"
        "  info.m_InitInstanceFct = NativePointer(uintptr_t(1));\n"
        "}\n"
        "void RejectCKPluginInfoExitPointerWrite() {\n"
        "  CKPluginInfo info;\n"
        "  info.m_ExitInstanceFct = NativePointer(uintptr_t(1));\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPluginInfo self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-plugin-info-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginInfo self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginInfo self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPluginInfo()");
    asIScriptFunction *rejectInit = module->GetFunctionByDecl("void RejectCKPluginInfoInitPointerWrite()");
    asIScriptFunction *rejectExit = module->GetFunctionByDecl("void RejectCKPluginInfoExitPointerWrite()");
    if (!probe || !rejectInit || !rejectExit) {
        engine->DiscardModule(moduleName);
        error = "CKPluginInfo self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPluginInfo value probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, rejectInit, true, "CKPluginInfo init callback rejection probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, rejectExit, true, "CKPluginInfo exit callback rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPluginEntryReadersDataScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPluginEntryReadersData script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *readerType = engine->GetTypeInfoByDecl("CKPluginEntryReadersData");
    if (!readerType) {
        error = "CKPluginEntryReadersData self-test could not find the registered type.";
        return false;
    }
    if (!readerType->GetMethodByDecl("NativePointer get_m_GetReaderFct() const") ||
        !readerType->GetMethodByDecl("void set_m_GetReaderFct(NativePointer ptr)") ||
        !readerType->GetMethodByDecl("CKPluginEntryReadersData& opAssign(const CKPluginEntryReadersData &in other)")) {
        error = "CKPluginEntryReadersData self-test could not find expected guarded callback methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPluginEntryReadersDataSelfTest";
    const char *source =
        "int ProbeCKPluginEntryReadersData() {\n"
        "  CKPluginEntryReadersData data;\n"
        "  if (data.m_SettingsParameterGuid.d1 != 0 || data.m_SettingsParameterGuid.d2 != 0) return 1;\n"
        "  if (data.m_OptionCount != 0 || data.m_ReaderFlags != 0) return 2;\n"
        "  if (!data.m_GetReaderFct.IsNull()) return 3;\n"
        "  data.m_SettingsParameterGuid = CKGUID(0x33333333, 0x44444444);\n"
        "  data.m_OptionCount = 3;\n"
        "  data.m_ReaderFlags = CK_DATAREADER_FILELOAD;\n"
        "  data.m_GetReaderFct = NativePointer();\n"
        "  CKPluginEntryReadersData copied(data);\n"
        "  if (copied.m_SettingsParameterGuid.d1 != 0x33333333 || copied.m_OptionCount != 3) return 4;\n"
        "  if (copied.m_ReaderFlags != CK_DATAREADER_FILELOAD || !copied.m_GetReaderFct.IsNull()) return 5;\n"
        "  CKPluginEntryReadersData assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.m_OptionCount != 3 || assigned.m_SettingsParameterGuid.d2 != 0x44444444) return 6;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKPluginEntryReadersDataFactoryWrite() {\n"
        "  CKPluginEntryReadersData data;\n"
        "  data.m_GetReaderFct = NativePointer(uintptr_t(1));\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPluginEntryReadersData self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-plugin-entry-readers-data-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginEntryReadersData self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginEntryReadersData self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPluginEntryReadersData()");
    asIScriptFunction *rejectFactory = module->GetFunctionByDecl("void RejectCKPluginEntryReadersDataFactoryWrite()");
    if (!probe || !rejectFactory) {
        engine->DiscardModule(moduleName);
        error = "CKPluginEntryReadersData self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPluginEntryReadersData value probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, rejectFactory, true, "CKPluginEntryReadersData factory rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPluginEntryScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPluginEntry script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *entryType = engine->GetTypeInfoByDecl("CKPluginEntry");
    if (!entryType) {
        error = "CKPluginEntry self-test could not find the registered type.";
        return false;
    }
    if (!entryType->GetMethodByDecl("bool HasReadersInfo() const") ||
        !entryType->GetMethodByDecl("bool HasBehaviorsInfo() const") ||
        !entryType->GetMethodByDecl("const CKPluginEntryReadersData& get_m_ReadersInfo() const") ||
        !entryType->GetMethodByDecl("const CKPluginEntryBehaviorsData& get_m_BehaviorsInfo() const") ||
        !entryType->GetMethodByDecl("CKPluginEntry& opAssign(const CKPluginEntry &in other)")) {
        error = "CKPluginEntry self-test could not find guarded metadata accessors.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPluginEntrySelfTest";
    const char *source =
        "int ProbeCKPluginEntry() {\n"
        "  CKPluginEntry entry;\n"
        "  if (entry.HasReadersInfo() || entry.HasBehaviorsInfo()) return 1;\n"
        "  entry.m_PluginDllIndex = 7;\n"
        "  entry.m_PositionInDll = 8;\n"
        "  entry.m_PluginInfo.m_Version = 9;\n"
        "  CKPluginEntry copied(entry);\n"
        "  if (copied.m_PluginDllIndex != 7 || copied.m_PositionInDll != 8) return 2;\n"
        "  if (copied.m_PluginInfo.m_Version != 9) return 3;\n"
        "  if (copied.HasReadersInfo() || copied.HasBehaviorsInfo()) return 4;\n"
        "  CKPluginEntry assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.m_PluginDllIndex != 7 || assigned.m_PluginInfo.m_Version != 9) return 5;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKPluginEntryReadersInfoAccess() {\n"
        "  CKPluginEntry entry;\n"
        "  entry.m_ReadersInfo.m_OptionCount;\n"
        "}\n"
        "void RejectCKPluginEntryBehaviorsInfoAccess() {\n"
        "  CKPluginEntry entry;\n"
        "  entry.m_BehaviorsInfo.m_BehaviorsGUID.Size();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPluginEntry self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-plugin-entry-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginEntry self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginEntry self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPluginEntry()");
    asIScriptFunction *rejectReaders = module->GetFunctionByDecl("void RejectCKPluginEntryReadersInfoAccess()");
    asIScriptFunction *rejectBehaviors = module->GetFunctionByDecl("void RejectCKPluginEntryBehaviorsInfoAccess()");
    if (!probe || !rejectReaders || !rejectBehaviors) {
        engine->DiscardModule(moduleName);
        error = "CKPluginEntry self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPluginEntry value probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, rejectReaders, true, "CKPluginEntry readers info rejection probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, rejectBehaviors, true, "CKPluginEntry behaviors info rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKFileExtensionScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKFileExtension script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *extensionType = engine->GetTypeInfoByDecl("CKFileExtension");
    if (!extensionType) {
        error = "CKFileExtension self-test could not find the registered type.";
        return false;
    }
    if (extensionType->GetMethodByDecl("string opImplConv() const") == nullptr) {
        error = "CKFileExtension self-test could not find the string conversion method.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKFileExtensionSelfTest";
    const char *source =
        "int ProbeCKFileExtension() {\n"
        "  CKFileExtension empty;\n"
        "  string emptyText = empty;\n"
        "  if (emptyText != \"\") return 1;\n"
        "  CKFileExtension bmp(\"bmp\");\n"
        "  string bmpText = bmp;\n"
        "  if (bmpText != \"bmp\") return 2;\n"
        "  CKFileExtension dotted(\".jpeg\");\n"
        "  string dottedText = dotted;\n"
        "  if (dottedText != \"jpe\") return 3;\n"
        "  CKFileExtension upper(\"WAV\");\n"
        "  string upperText = upper;\n"
        "  if (upperText != \"WAV\") return 4;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKFileExtension self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-file-extension-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKFileExtension self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKFileExtension self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKFileExtension()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKFileExtension self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKFileExtension value probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

#if CKVERSION == 0x13022002
bool RunCKPICKRESULTScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPICKRESULT script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *pickType = engine->GetTypeInfoByDecl("CKPICKRESULT");
    if (!pickType) {
        error = "CKPICKRESULT self-test could not find the registered type.";
        return false;
    }
    if (!pickType->GetMethodByDecl("CKPICKRESULT& opAssign(const CKPICKRESULT &in other)")) {
        error = "CKPICKRESULT self-test could not find assignment method.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPICKRESULTSelfTest";
    const char *source =
        "int ProbeCKPICKRESULT() {\n"
        "  CKPICKRESULT result;\n"
        "  result.IntersectionPoint = VxVector(1.0f, 2.0f, 3.0f);\n"
        "  result.IntersectionNormal = VxVector(0.0f, 1.0f, 0.0f);\n"
        "  result.TexU = 0.25f;\n"
        "  result.TexV = 0.75f;\n"
        "  result.Distance = 42.0f;\n"
        "  result.FaceIndex = 5;\n"
        "  result.Sprite = 1234;\n"
        "  CKPICKRESULT copied(result);\n"
        "  if (copied.IntersectionPoint.x != 1.0f || copied.IntersectionPoint.y != 2.0f || copied.IntersectionPoint.z != 3.0f) return 1;\n"
        "  if (copied.IntersectionNormal.y != 1.0f) return 2;\n"
        "  if (copied.TexU != 0.25f || copied.TexV != 0.75f || copied.Distance != 42.0f) return 3;\n"
        "  if (copied.FaceIndex != 5 || copied.Sprite != 1234) return 4;\n"
        "  CKPICKRESULT assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.Distance != 42.0f || assigned.FaceIndex != 5 || assigned.Sprite != 1234) return 5;\n"
        "  copied.Distance = 7.0f;\n"
        "  copied.Sprite = 77;\n"
        "  if (assigned.Distance != 42.0f || assigned.Sprite != 1234) return 6;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPICKRESULT self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckpickresult-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPICKRESULT self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPICKRESULT self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPICKRESULT()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKPICKRESULT self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPICKRESULT value probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}
#endif

bool RunCKDataRowItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKDataRowIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("CKDataRowIt");
    if (!iteratorType) {
        error = "CKDataRowIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const CKDataRowIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const CKDataRowIt &in other) const")) {
        error = "CKDataRowIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *rowType = engine->GetTypeInfoByDecl("CKDataRow");
    if (!rowType) {
        error = "CKDataRowIt self-test could not find CKDataRow.";
        return false;
    }
    if (!rowType->GetMethodByDecl("CKDataRowIt Begin() const") ||
        !rowType->GetMethodByDecl("CKDataRowIt End() const")) {
        error = "CKDataRowIt self-test could not find CKDataRow Begin/End producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKDataRowItSelfTest";
    const char *source =
        "int ProbeCKDataRowIt() {\n"
        "  CKDataRow row;\n"
        "  uint first = 7;\n"
        "  uint second = 11;\n"
        "  row.PushBack(first);\n"
        "  row.PushBack(second);\n"
        "  CKDataRowIt begin = row.Begin();\n"
        "  CKDataRowIt end = row.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() != first) return 3;\n"
        "  CKDataRowIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 4;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 5;\n"
        "  if (copied.Get() != second) return 6;\n"
        "  CKDataRowIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 7;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKDataRowIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-datarowit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDataRowIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDataRowIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKDataRowIt()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKDataRowIt self-test function was not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKDataRowIt iterator probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXIntArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XIntArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XIntArrayIt");
    if (!iteratorType) {
        error = "XIntArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XIntArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XIntArrayIt &in other) const")) {
        error = "XIntArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XIntArray");
    if (!arrayType) {
        error = "XIntArrayIt self-test could not find XIntArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XIntArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XIntArrayIt End() const") ||
        !arrayType->GetMethodByDecl("XIntArrayIt RBegin() const") ||
        !arrayType->GetMethodByDecl("XIntArrayIt REnd() const")) {
        error = "XIntArrayIt self-test could not find XIntArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XIntArrayItSelfTest";
    const char *source =
        "int ProbeXIntArrayIt() {\n"
        "  XIntArray values;\n"
        "  int first = 19;\n"
        "  int second = 23;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XIntArrayIt begin = values.Begin();\n"
        "  XIntArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() != first) return 3;\n"
        "  XIntArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 4;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 5;\n"
        "  if (copied.Get() != second) return 6;\n"
        "  XIntArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 7;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 8;\n"
        "  XIntArrayIt rbegin = values.RBegin();\n"
        "  XIntArrayIt rend = values.REnd();\n"
        "  if (rbegin == rend) return 9;\n"
        "  if (rbegin.Get() != second) return 10;\n"
        "  --rbegin;\n"
        "  if (rbegin.Get() != first) return 11;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XIntArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xintarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XIntArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XIntArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXIntArrayIt()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "XIntArrayIt self-test function was not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XIntArrayIt iterator probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXDwordArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XDwordArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XDwordArrayIt");
    if (!iteratorType) {
        error = "XDwordArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XDwordArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XDwordArrayIt &in other) const")) {
        error = "XDwordArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XDwordArray");
    if (!arrayType) {
        error = "XDwordArrayIt self-test could not find XDwordArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XDwordArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XDwordArrayIt End() const")) {
        error = "XDwordArrayIt self-test could not find XDwordArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XDwordArrayItSelfTest";
    const char *source =
        "int ProbeXDwordArrayIt() {\n"
        "  XDwordArray values;\n"
        "  uint first = 29;\n"
        "  uint second = 31;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XDwordArrayIt begin = values.Begin();\n"
        "  XDwordArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() != first) return 3;\n"
        "  XDwordArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 4;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 5;\n"
        "  if (copied.Get() != second) return 6;\n"
        "  XDwordArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 7;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XDwordArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xdwordarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XDwordArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XDwordArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXDwordArrayIt()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "XDwordArrayIt self-test function was not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XDwordArrayIt iterator probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXClassIDArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XClassIDArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XClassIDArrayIt");
    if (!iteratorType) {
        error = "XClassIDArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XClassIDArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XClassIDArrayIt &in other) const")) {
        error = "XClassIDArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XClassIDArray");
    if (!arrayType) {
        error = "XClassIDArrayIt self-test could not find XClassIDArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XClassIDArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XClassIDArrayIt End() const")) {
        error = "XClassIDArrayIt self-test could not find XClassIDArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XClassIDArrayItSelfTest";
    const char *source =
        "int ProbeXClassIDArrayIt() {\n"
        "  XClassIDArray values;\n"
        "  int first = CKCID_OBJECT;\n"
        "  int second = CKCID_BEOBJECT;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XClassIDArrayIt begin = values.Begin();\n"
        "  XClassIDArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() != first) return 3;\n"
        "  XClassIDArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 4;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 5;\n"
        "  if (copied.Get() != second) return 6;\n"
        "  XClassIDArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 7;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XClassIDArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xclassidarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XClassIDArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XClassIDArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXClassIDArrayIt()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "XClassIDArrayIt self-test function was not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XClassIDArrayIt iterator probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXGUIDArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XGUIDArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XGUIDArrayIt");
    if (!iteratorType) {
        error = "XGUIDArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XGUIDArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XGUIDArrayIt &in other) const")) {
        error = "XGUIDArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XGUIDArray");
    if (!arrayType) {
        error = "XGUIDArrayIt self-test could not find XGUIDArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XGUIDArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XGUIDArrayIt End() const") ||
        !arrayType->GetMethodByDecl("XGUIDArrayIt RBegin() const") ||
        !arrayType->GetMethodByDecl("XGUIDArrayIt REnd() const")) {
        error = "XGUIDArrayIt self-test could not find XGUIDArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XGUIDArrayItSelfTest";
    const char *source =
        "int ProbeXGUIDArrayIt() {\n"
        "  XGUIDArray values;\n"
        "  CKGUID first(0x11111111, 0x22222222);\n"
        "  CKGUID second(0x33333333, 0x44444444);\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XGUIDArrayIt begin = values.Begin();\n"
        "  XGUIDArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (!(begin.Get() == first)) return 3;\n"
        "  XGUIDArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 4;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 5;\n"
        "  if (!(copied.Get() == second)) return 6;\n"
        "  XGUIDArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 7;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 8;\n"
        "  XGUIDArrayIt rbegin = values.RBegin();\n"
        "  XGUIDArrayIt rend = values.REnd();\n"
        "  if (rbegin == rend) return 9;\n"
        "  if (!(rbegin.Get() == second)) return 10;\n"
        "  --rbegin;\n"
        "  if (!(rbegin.Get() == first)) return 11;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXGUIDArrayItGet() {\n"
        "  XGUIDArrayIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultXGUIDArrayItInc() {\n"
        "  XGUIDArrayIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultXGUIDArrayItDec() {\n"
        "  XGUIDArrayIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyXGUIDArrayItGet() {\n"
        "  XGUIDArray values;\n"
        "  XGUIDArrayIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XGUIDArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xguidarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XGUIDArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XGUIDArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXGUIDArrayIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXGUIDArrayItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXGUIDArrayItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultXGUIDArrayItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyXGUIDArrayItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "XGUIDArrayIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XGUIDArrayIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XGUIDArrayIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XGUIDArrayIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "XGUIDArrayIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "XGUIDArrayIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXImageArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XImageArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XImageArrayIt");
    if (!iteratorType) {
        error = "XImageArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XImageArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XImageArrayIt &in other) const")) {
        error = "XImageArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XImageArray");
    if (!arrayType) {
        error = "XImageArrayIt self-test could not find XImageArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XImageArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XImageArrayIt End() const")) {
        error = "XImageArrayIt self-test could not find XImageArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XImageArrayItSelfTest";
    const char *source =
        "int ProbeXImageArrayIt() {\n"
        "  XImageArray values;\n"
        "  VxImageDescEx first;\n"
        "  first.Width = 64;\n"
        "  first.Height = 32;\n"
        "  first.BitsPerPixel = 16;\n"
        "  VxImageDescEx second;\n"
        "  second.Width = 128;\n"
        "  second.Height = 16;\n"
        "  second.BitsPerPixel = 32;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XImageArrayIt begin = values.Begin();\n"
        "  XImageArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get().Width != first.Width || begin.Get().Height != first.Height) return 3;\n"
        "  begin.Get().Width = 96;\n"
        "  if (values[0].Width != 96) return 4;\n"
        "  XImageArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 5;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 6;\n"
        "  if (copied.Get().Width != second.Width || copied.Get().BitsPerPixel != second.BitsPerPixel) return 7;\n"
        "  XImageArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 8;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 9;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXImageArrayItGet() {\n"
        "  XImageArrayIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultXImageArrayItInc() {\n"
        "  XImageArrayIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultXImageArrayItDec() {\n"
        "  XImageArrayIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyXImageArrayItGet() {\n"
        "  XImageArray values;\n"
        "  XImageArrayIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XImageArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ximagearrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XImageArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XImageArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXImageArrayIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXImageArrayItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXImageArrayItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultXImageArrayItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyXImageArrayItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "XImageArrayIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XImageArrayIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XImageArrayIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XImageArrayIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "XImageArrayIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "XImageArrayIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXObjectArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XObjectArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XObjectArrayIt");
    if (!iteratorType) {
        error = "XObjectArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XObjectArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XObjectArrayIt &in other) const")) {
        error = "XObjectArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XObjectArray");
    if (!arrayType) {
        error = "XObjectArrayIt self-test could not find XObjectArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XObjectArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XObjectArrayIt End() const") ||
        !arrayType->GetMethodByDecl("XObjectArrayIt RBegin() const") ||
        !arrayType->GetMethodByDecl("XObjectArrayIt REnd() const")) {
        error = "XObjectArrayIt self-test could not find XObjectArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XObjectArrayItSelfTest";
    const char *source =
        "int ProbeXObjectArrayIt() {\n"
        "  XObjectArray values;\n"
        "  uint first = 101;\n"
        "  uint second = 202;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XObjectArrayIt begin = values.Begin();\n"
        "  XObjectArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() != first) return 3;\n"
        "  begin.Get() = 303;\n"
        "  if (values[0] != 303) return 4;\n"
        "  XObjectArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 5;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 6;\n"
        "  if (copied.Get() != second) return 7;\n"
        "  XObjectArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 8;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 9;\n"
        "  XObjectArrayIt rbegin = values.RBegin();\n"
        "  XObjectArrayIt rend = values.REnd();\n"
        "  if (rbegin == rend) return 10;\n"
        "  if (rbegin.Get() != second) return 11;\n"
        "  --rbegin;\n"
        "  if (rbegin.Get() != 303) return 12;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXObjectArrayItGet() {\n"
        "  XObjectArrayIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultXObjectArrayItInc() {\n"
        "  XObjectArrayIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultXObjectArrayItDec() {\n"
        "  XObjectArrayIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyXObjectArrayItGet() {\n"
        "  XObjectArray values;\n"
        "  XObjectArrayIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XObjectArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xobjectarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XObjectArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XObjectArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXObjectArrayIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXObjectArrayItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXObjectArrayItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultXObjectArrayItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyXObjectArrayItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "XObjectArrayIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XObjectArrayIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XObjectArrayIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XObjectArrayIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "XObjectArrayIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "XObjectArrayIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXSObjectArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XSObjectArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XSObjectArrayIt");
    if (!iteratorType) {
        error = "XSObjectArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XSObjectArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XSObjectArrayIt &in other) const")) {
        error = "XSObjectArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XSObjectArray");
    if (!arrayType) {
        error = "XSObjectArrayIt self-test could not find XSObjectArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XSObjectArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XSObjectArrayIt End() const")) {
        error = "XSObjectArrayIt self-test could not find XSObjectArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XSObjectArrayItSelfTest";
    const char *source =
        "int ProbeXSObjectArrayIt() {\n"
        "  XSObjectArray values;\n"
        "  uint first = 404;\n"
        "  uint second = 505;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XSObjectArrayIt begin = values.Begin();\n"
        "  XSObjectArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() != first) return 3;\n"
        "  begin.Get() = 606;\n"
        "  if (values[0] != 606) return 4;\n"
        "  XSObjectArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 5;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 6;\n"
        "  if (copied.Get() != second) return 7;\n"
        "  XSObjectArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 8;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 9;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXSObjectArrayItGet() {\n"
        "  XSObjectArrayIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultXSObjectArrayItInc() {\n"
        "  XSObjectArrayIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultXSObjectArrayItDec() {\n"
        "  XSObjectArrayIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyXSObjectArrayItGet() {\n"
        "  XSObjectArray values;\n"
        "  XSObjectArrayIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XSObjectArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xsobjectarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XSObjectArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XSObjectArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXSObjectArrayIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXSObjectArrayItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXSObjectArrayItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultXSObjectArrayItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyXSObjectArrayItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "XSObjectArrayIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XSObjectArrayIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XSObjectArrayIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XSObjectArrayIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "XSObjectArrayIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "XSObjectArrayIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXObjectPointerArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XObjectPointerArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XObjectPointerArrayIt");
    if (!iteratorType) {
        error = "XObjectPointerArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XObjectPointerArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XObjectPointerArrayIt &in other) const")) {
        error = "XObjectPointerArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XObjectPointerArray");
    if (!arrayType) {
        error = "XObjectPointerArrayIt self-test could not find XObjectPointerArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XObjectPointerArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XObjectPointerArrayIt End() const") ||
        !arrayType->GetMethodByDecl("XObjectPointerArrayIt RBegin() const") ||
        !arrayType->GetMethodByDecl("XObjectPointerArrayIt REnd() const")) {
        error = "XObjectPointerArrayIt self-test could not find XObjectPointerArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XObjectPointerArrayItSelfTest";
    const char *source =
        "int ProbeXObjectPointerArrayIt() {\n"
        "  XObjectPointerArray values;\n"
        "  CKObject@ first = null;\n"
        "  CKObject@ second = null;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XObjectPointerArrayIt begin = values.Begin();\n"
        "  XObjectPointerArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() !is null) return 3;\n"
        "  @begin.Get() = null;\n"
        "  if (values[0] !is null) return 4;\n"
        "  XObjectPointerArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 5;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 6;\n"
        "  if (copied.Get() !is null) return 7;\n"
        "  XObjectPointerArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 8;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 9;\n"
        "  XObjectPointerArrayIt rbegin = values.RBegin();\n"
        "  XObjectPointerArrayIt rend = values.REnd();\n"
        "  if (rbegin == rend) return 10;\n"
        "  if (rbegin.Get() !is null) return 11;\n"
        "  --rbegin;\n"
        "  if (!(rbegin == begin) || rbegin.Get() !is null) return 12;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXObjectPointerArrayItGet() {\n"
        "  XObjectPointerArrayIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultXObjectPointerArrayItInc() {\n"
        "  XObjectPointerArrayIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultXObjectPointerArrayItDec() {\n"
        "  XObjectPointerArrayIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyXObjectPointerArrayItGet() {\n"
        "  XObjectPointerArray values;\n"
        "  XObjectPointerArrayIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XObjectPointerArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xobjectpointerarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XObjectPointerArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XObjectPointerArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXObjectPointerArrayIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXObjectPointerArrayItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXObjectPointerArrayItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultXObjectPointerArrayItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyXObjectPointerArrayItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "XObjectPointerArrayIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XObjectPointerArrayIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XObjectPointerArrayIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XObjectPointerArrayIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "XObjectPointerArrayIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "XObjectPointerArrayIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXSObjectPointerArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XSObjectPointerArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XSObjectPointerArrayIt");
    if (!iteratorType) {
        error = "XSObjectPointerArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XSObjectPointerArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XSObjectPointerArrayIt &in other) const")) {
        error = "XSObjectPointerArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XSObjectPointerArray");
    if (!arrayType) {
        error = "XSObjectPointerArrayIt self-test could not find XSObjectPointerArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XSObjectPointerArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XSObjectPointerArrayIt End() const")) {
        error = "XSObjectPointerArrayIt self-test could not find XSObjectPointerArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XSObjectPointerArrayItSelfTest";
    const char *source =
        "int ProbeXSObjectPointerArrayIt() {\n"
        "  XSObjectPointerArray values;\n"
        "  CKObject@ first = null;\n"
        "  CKObject@ second = null;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XSObjectPointerArrayIt begin = values.Begin();\n"
        "  XSObjectPointerArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() !is null) return 3;\n"
        "  @begin.Get() = null;\n"
        "  if (values[0] !is null) return 4;\n"
        "  XSObjectPointerArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 5;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 6;\n"
        "  if (copied.Get() !is null) return 7;\n"
        "  XSObjectPointerArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 8;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 9;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXSObjectPointerArrayItGet() {\n"
        "  XSObjectPointerArrayIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultXSObjectPointerArrayItInc() {\n"
        "  XSObjectPointerArrayIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultXSObjectPointerArrayItDec() {\n"
        "  XSObjectPointerArrayIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyXSObjectPointerArrayItGet() {\n"
        "  XSObjectPointerArray values;\n"
        "  XSObjectPointerArrayIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XSObjectPointerArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xsobjectpointerarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XSObjectPointerArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XSObjectPointerArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXSObjectPointerArrayIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXSObjectPointerArrayItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXSObjectPointerArrayItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultXSObjectPointerArrayItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyXSObjectPointerArrayItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "XSObjectPointerArrayIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XSObjectPointerArrayIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XSObjectPointerArrayIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XSObjectPointerArrayIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "XSObjectPointerArrayIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "XSObjectPointerArrayIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXObjectDeclarationArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XObjectDeclarationArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XObjectDeclarationArrayIt");
    if (!iteratorType) {
        error = "XObjectDeclarationArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XObjectDeclarationArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XObjectDeclarationArrayIt &in other) const")) {
        error = "XObjectDeclarationArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XObjectDeclarationArray");
    if (!arrayType) {
        error = "XObjectDeclarationArrayIt self-test could not find XObjectDeclarationArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XObjectDeclarationArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XObjectDeclarationArrayIt End() const") ||
        !arrayType->GetMethodByDecl("XObjectDeclarationArrayIt RBegin() const") ||
        !arrayType->GetMethodByDecl("XObjectDeclarationArrayIt REnd() const")) {
        error = "XObjectDeclarationArrayIt self-test could not find XObjectDeclarationArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XObjectDeclarationArrayItSelfTest";
    const char *source =
        "int ProbeXObjectDeclarationArrayIt() {\n"
        "  XObjectDeclarationArray values;\n"
        "  CKObjectDeclaration@ first = null;\n"
        "  CKObjectDeclaration@ second = null;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  XObjectDeclarationArrayIt begin = values.Begin();\n"
        "  XObjectDeclarationArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() !is null) return 3;\n"
        "  @begin.Get() = null;\n"
        "  if (values[0] !is null) return 4;\n"
        "  XObjectDeclarationArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 5;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 6;\n"
        "  if (copied.Get() !is null) return 7;\n"
        "  XObjectDeclarationArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 8;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 9;\n"
        "  XObjectDeclarationArrayIt rbegin = values.RBegin();\n"
        "  XObjectDeclarationArrayIt rend = values.REnd();\n"
        "  if (rbegin == rend) return 10;\n"
        "  if (rbegin.Get() !is null) return 11;\n"
        "  --rbegin;\n"
        "  if (!(rbegin == begin) || rbegin.Get() !is null) return 12;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXObjectDeclarationArrayItGet() {\n"
        "  XObjectDeclarationArrayIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultXObjectDeclarationArrayItInc() {\n"
        "  XObjectDeclarationArrayIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultXObjectDeclarationArrayItDec() {\n"
        "  XObjectDeclarationArrayIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyXObjectDeclarationArrayItGet() {\n"
        "  XObjectDeclarationArray values;\n"
        "  XObjectDeclarationArrayIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XObjectDeclarationArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xobjectdeclarationarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XObjectDeclarationArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XObjectDeclarationArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXObjectDeclarationArrayIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXObjectDeclarationArrayItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXObjectDeclarationArrayItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultXObjectDeclarationArrayItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyXObjectDeclarationArrayItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "XObjectDeclarationArrayIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XObjectDeclarationArrayIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XObjectDeclarationArrayIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XObjectDeclarationArrayIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "XObjectDeclarationArrayIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "XObjectDeclarationArrayIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXStringArrayItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XStringArrayIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XStringArrayIt");
    if (!iteratorType) {
        error = "XStringArrayIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const XStringArrayIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const XStringArrayIt &in other) const")) {
        error = "XStringArrayIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XStringArray");
    if (!arrayType) {
        error = "XStringArrayIt self-test could not find XStringArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("XStringArrayIt Begin() const") ||
        !arrayType->GetMethodByDecl("XStringArrayIt End() const")) {
        error = "XStringArrayIt self-test could not find XStringArray iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XStringArrayItSelfTest";
    const char *source =
        "int ProbeXStringArrayIt() {\n"
        "  XStringArray values;\n"
        "  values.PushBack(XString(\"first\"));\n"
        "  values.PushBack(XString(\"second\"));\n"
        "  XStringArrayIt begin = values.Begin();\n"
        "  XStringArrayIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (!(begin.Get() == XString(\"first\"))) return 3;\n"
        "  begin.Get() = XString(\"changed\");\n"
        "  if (!(values[0] == XString(\"changed\"))) return 4;\n"
        "  XStringArrayIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 5;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 6;\n"
        "  if (!(copied.Get() == XString(\"second\"))) return 7;\n"
        "  XStringArrayIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 8;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 9;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXStringArrayItGet() {\n"
        "  XStringArrayIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultXStringArrayItInc() {\n"
        "  XStringArrayIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultXStringArrayItDec() {\n"
        "  XStringArrayIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyXStringArrayItGet() {\n"
        "  XStringArray values;\n"
        "  XStringArrayIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XStringArrayIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xstringarrayit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XStringArrayIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XStringArrayIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXStringArrayIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXStringArrayItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXStringArrayItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultXStringArrayItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyXStringArrayItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "XStringArrayIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XStringArrayIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XStringArrayIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XStringArrayIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "XStringArrayIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "XStringArrayIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXIntArrayScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XIntArray script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XIntArray");
    if (!arrayType) {
        error = "XIntArray self-test could not find XIntArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("int& opIndex(uint index)") ||
        !arrayType->GetMethodByDecl("const int& opIndex(uint index) const") ||
        !arrayType->GetMethodByDecl("bool RemoveAt(uint pos, int &out old)") ||
        !arrayType->GetMethodByDecl("bool EraseAt(int pos)") ||
        !arrayType->GetMethodByDecl("bool IsHere(const int &in o) const") ||
        !arrayType->GetMethodByDecl("const int& Front() const") ||
        !arrayType->GetMethodByDecl("int& Back()") ||
        !arrayType->GetMethodByDecl("int GetMemoryOccupation(bool addStatic = false) const")) {
        error = "XIntArray self-test found missing hardened container declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XIntArraySelfTest";
    const char *source =
        "int ProbeXIntArray() {\n"
        "  XIntArray values;\n"
        "  values.PushBack(10);\n"
        "  values.Insert(1, 20);\n"
        "  if (values.Size() != 2) return 1;\n"
        "  if (values[0] != 10 || values[1] != 20) return 2;\n"
        "  if (values.Front() != 10 || values.Back() != 20) return 3;\n"
        "  if (!values.IsHere(20) || values.IsHere(30)) return 4;\n"
        "  if (values.EraseAt(-1)) return 5;\n"
        "  int old = 0;\n"
        "  if (!values.RemoveAt(0, old) || old != 10) return 6;\n"
        "  values.PopBack();\n"
        "  if (values.Size() != 0) return 7;\n"
        "  values.Resize(2);\n"
        "  values[0] = 1;\n"
        "  values[1] = 2;\n"
        "  if (values.GetMemoryOccupation(false) < 0) return 8;\n"
        "  if (values.GetMemoryOccupation(true) < 0) return 9;\n"
        "  return 0;\n"
        "}\n"
        "void RejectXIntArrayNegativeIndex() {\n"
        "  XIntArray values;\n"
        "  values.PushBack(1);\n"
        "  values[uint(-1)] = 2;\n"
        "}\n"
        "void RejectXIntArrayOutOfRangeIndex() {\n"
        "  XIntArray values;\n"
        "  values.PushBack(1);\n"
        "  values[1] = 2;\n"
        "}\n"
        "void RejectXIntArrayConstOutOfRangeIndex() {\n"
        "  const XIntArray values;\n"
        "  values[0];\n"
        "}\n"
        "void RejectXIntArrayEmptyFront() {\n"
        "  XIntArray values;\n"
        "  values.Front();\n"
        "}\n"
        "void RejectXIntArrayEmptyBack() {\n"
        "  XIntArray values;\n"
        "  values.Back();\n"
        "}\n"
        "void RejectXIntArrayEmptyPopBack() {\n"
        "  XIntArray values;\n"
        "  values.PopBack();\n"
        "}\n"
        "void RejectXIntArrayNegativeResize() {\n"
        "  XIntArray values;\n"
        "  values.Resize(-1);\n"
        "}\n"
        "void RejectXIntArrayBadInsert() {\n"
        "  XIntArray values;\n"
        "  values.Insert(1, 3);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XIntArray self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xintarray-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XIntArray self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XIntArray self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXIntArray()");
    asIScriptFunction *negativeIndex = module->GetFunctionByDecl("void RejectXIntArrayNegativeIndex()");
    asIScriptFunction *outOfRangeIndex = module->GetFunctionByDecl("void RejectXIntArrayOutOfRangeIndex()");
    asIScriptFunction *constOutOfRangeIndex = module->GetFunctionByDecl("void RejectXIntArrayConstOutOfRangeIndex()");
    asIScriptFunction *emptyFront = module->GetFunctionByDecl("void RejectXIntArrayEmptyFront()");
    asIScriptFunction *emptyBack = module->GetFunctionByDecl("void RejectXIntArrayEmptyBack()");
    asIScriptFunction *emptyPopBack = module->GetFunctionByDecl("void RejectXIntArrayEmptyPopBack()");
    asIScriptFunction *negativeResize = module->GetFunctionByDecl("void RejectXIntArrayNegativeResize()");
    asIScriptFunction *badInsert = module->GetFunctionByDecl("void RejectXIntArrayBadInsert()");
    if (!probe || !negativeIndex || !outOfRangeIndex || !constOutOfRangeIndex || !emptyFront ||
        !emptyBack || !emptyPopBack || !negativeResize || !badInsert) {
        engine->DiscardModule(moduleName);
        error = "XIntArray self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XIntArray container probe", error) &&
              ExecuteCKAttributeDescProbe(engine, negativeIndex, true, "XIntArray negative index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, outOfRangeIndex, true, "XIntArray out-of-range index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, constOutOfRangeIndex, true, "XIntArray const out-of-range index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyFront, true, "XIntArray empty Front rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyBack, true, "XIntArray empty Back rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyPopBack, true, "XIntArray empty PopBack rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, negativeResize, true, "XIntArray negative Resize rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, badInsert, true, "XIntArray bad Insert rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXDwordArrayScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XDwordArray script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XDwordArray");
    if (!arrayType) {
        error = "XDwordArray self-test could not find XDwordArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("CKDWORD& opIndex(uint index)") ||
        !arrayType->GetMethodByDecl("const CKDWORD& opIndex(uint index) const") ||
        !arrayType->GetMethodByDecl("bool IsHere(const CKDWORD &in o) const") ||
        !arrayType->GetMethodByDecl("int GetMemoryOccupation(bool addStatic = false) const")) {
        error = "XDwordArray self-test found missing hardened XSArray declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XDwordArraySelfTest";
    const char *source =
        "int ProbeXDwordArray() {\n"
        "  XDwordArray values;\n"
        "  values.PushBack(10);\n"
        "  values.Insert(1, 20);\n"
        "  if (values.Size() != 2) return 1;\n"
        "  if (values[0] != 10 || values[1] != 20) return 2;\n"
        "  if (!values.IsHere(20) || values.IsHere(30)) return 3;\n"
        "  values.PopBack();\n"
        "  values.Resize(2);\n"
        "  if (values.GetMemoryOccupation(false) < 0) return 4;\n"
        "  if (values.GetMemoryOccupation(true) < 0) return 5;\n"
        "  return 0;\n"
        "}\n"
        "void RejectXDwordArrayNegativeIndex() {\n"
        "  XDwordArray values;\n"
        "  values.PushBack(1);\n"
        "  values[uint(-1)] = 2;\n"
        "}\n"
        "void RejectXDwordArrayOutOfRangeIndex() {\n"
        "  XDwordArray values;\n"
        "  values.PushBack(1);\n"
        "  values[1] = 2;\n"
        "}\n"
        "void RejectXDwordArrayNegativeResize() {\n"
        "  XDwordArray values;\n"
        "  values.Resize(-1);\n"
        "}\n"
        "void RejectXDwordArrayBadInsert() {\n"
        "  XDwordArray values;\n"
        "  values.Insert(1, 3);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XDwordArray self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xdwordarray-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XDwordArray self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XDwordArray self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXDwordArray()");
    asIScriptFunction *negativeIndex = module->GetFunctionByDecl("void RejectXDwordArrayNegativeIndex()");
    asIScriptFunction *outOfRangeIndex = module->GetFunctionByDecl("void RejectXDwordArrayOutOfRangeIndex()");
    asIScriptFunction *negativeResize = module->GetFunctionByDecl("void RejectXDwordArrayNegativeResize()");
    asIScriptFunction *badInsert = module->GetFunctionByDecl("void RejectXDwordArrayBadInsert()");
    if (!probe || !negativeIndex || !outOfRangeIndex || !negativeResize || !badInsert) {
        engine->DiscardModule(moduleName);
        error = "XDwordArray self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XDwordArray container probe", error) &&
              ExecuteCKAttributeDescProbe(engine, negativeIndex, true, "XDwordArray negative index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, outOfRangeIndex, true, "XDwordArray out-of-range index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, negativeResize, true, "XDwordArray negative Resize rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, badInsert, true, "XDwordArray bad Insert rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXFileObjectsTableScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XFileObjectsTable script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *tableType = engine->GetTypeInfoByDecl("XFileObjectsTable");
    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XFileObjectsTableIt");
    if (!tableType || !iteratorType) {
        error = "XFileObjectsTable self-test could not find registered table types.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("const CK_ID& GetKey() const") ||
        !iteratorType->GetMethodByDecl("int& GetValue()") ||
        !iteratorType->GetMethodByDecl("XFileObjectsTableIt& opPreInc()") ||
        !tableType->GetMethodByDecl("bool Insert(const CK_ID &in, const int &in, bool override)") ||
        !tableType->GetMethodByDecl("XFileObjectsTableIt Remove(const XFileObjectsTableIt &in)") ||
        !tableType->GetMethodByDecl("bool LookUp(const CK_ID &in, int &out) const") ||
        !tableType->GetMethodByDecl("bool IsHere(const CK_ID &in) const") ||
        !tableType->GetMethodByDecl("int GetMemoryOccupation(bool = false) const")) {
        error = "XFileObjectsTable self-test found missing hardened hash declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XFileObjectsTableSelfTest";
    const char *source =
        "int ProbeXFileObjectsTable() {\n"
        "  XFileObjectsTable table;\n"
        "  CK_ID key1 = 1;\n"
        "  CK_ID key2 = 2;\n"
        "  CK_ID missing = 99;\n"
        "  int inserted = 10;\n"
        "  int assigned = 20;\n"
        "  if (!table.Insert(key1, inserted, true)) return 1;\n"
        "  if (!table.IsHere(key1) || table.IsHere(key2)) return 2;\n"
        "  int value = 0;\n"
        "  if (!table.LookUp(key1, value) || value != inserted) return 3;\n"
        "  table[key2] = assigned;\n"
        "  XFileObjectsTableIt it = table.Find(key2);\n"
        "  if (it.GetKey() != key2 || it.GetValue() != assigned) return 4;\n"
        "  it.GetValue() = 21;\n"
        "  if (!table.LookUp(key2, value) || value != 21) return 5;\n"
        "  table.Remove(it);\n"
        "  if (table.IsHere(key2)) return 6;\n"
        "  if (table.GetMemoryOccupation(false) < 0) return 7;\n"
        "  if (table.GetMemoryOccupation(true) < 0) return 8;\n"
        "  if (table.IsHere(missing)) return 9;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXFileObjectsTableItGet() {\n"
        "  XFileObjectsTableIt it;\n"
        "  it.GetValue();\n"
        "}\n"
        "void RejectDefaultXFileObjectsTableItInc() {\n"
        "  XFileObjectsTableIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectMissingXFileObjectsTableItGet() {\n"
        "  XFileObjectsTable table;\n"
        "  CK_ID missing = 99;\n"
        "  XFileObjectsTableIt it = table.Find(missing);\n"
        "  it.GetValue();\n"
        "}\n"
        "void RejectEndXFileObjectsTableItKey() {\n"
        "  XFileObjectsTable table;\n"
        "  XFileObjectsTableIt it = table.End();\n"
        "  it.GetKey();\n"
        "}\n"
        "void RejectDefaultXFileObjectsTableRemove() {\n"
        "  XFileObjectsTable table;\n"
        "  XFileObjectsTableIt it;\n"
        "  table.Remove(it);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XFileObjectsTable self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xfileobjectstable-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XFileObjectsTable self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XFileObjectsTable self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXFileObjectsTable()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXFileObjectsTableItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXFileObjectsTableItInc()");
    asIScriptFunction *missingGet = module->GetFunctionByDecl("void RejectMissingXFileObjectsTableItGet()");
    asIScriptFunction *endKey = module->GetFunctionByDecl("void RejectEndXFileObjectsTableItKey()");
    asIScriptFunction *defaultRemove = module->GetFunctionByDecl("void RejectDefaultXFileObjectsTableRemove()");
    if (!probe || !defaultGet || !defaultInc || !missingGet || !endKey || !defaultRemove) {
        engine->DiscardModule(moduleName);
        error = "XFileObjectsTable self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XFileObjectsTable container probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XFileObjectsTable default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XFileObjectsTable default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, missingGet, true, "XFileObjectsTable missing Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, endKey, true, "XFileObjectsTable end GetKey rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultRemove, true, "XFileObjectsTable default Remove rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXHashIDScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XHashID script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *tableType = engine->GetTypeInfoByDecl("XHashID");
    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("XHashIDIt");
    if (!tableType || !iteratorType) {
        error = "XHashID self-test could not find registered table types.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("const CK_ID& GetKey() const") ||
        !iteratorType->GetMethodByDecl("CK_ID& GetValue()") ||
        !iteratorType->GetMethodByDecl("XHashIDIt& opPreInc()") ||
        !tableType->GetMethodByDecl("bool Insert(const CK_ID &in, const CK_ID &in, bool override)") ||
        !tableType->GetMethodByDecl("XHashIDIt Remove(const XHashIDIt &in)") ||
        !tableType->GetMethodByDecl("bool LookUp(const CK_ID &in, CK_ID &out) const") ||
        !tableType->GetMethodByDecl("bool IsHere(const CK_ID &in) const")) {
        error = "XHashID self-test found missing hardened XNHash declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XHashIDSelfTest";
    const char *source =
        "int ProbeXHashID() {\n"
        "  XHashID table;\n"
        "  CK_ID key1 = 1;\n"
        "  CK_ID key2 = 2;\n"
        "  CK_ID inserted = 10;\n"
        "  CK_ID assigned = 20;\n"
        "  if (!table.Insert(key1, inserted, true)) return 1;\n"
        "  if (!table.IsHere(key1) || table.IsHere(key2)) return 2;\n"
        "  CK_ID value = 0;\n"
        "  if (!table.LookUp(key1, value) || value != inserted) return 3;\n"
        "  table[key2] = assigned;\n"
        "  XHashIDIt it = table.Find(key2);\n"
        "  if (it.GetKey() != key2 || it.GetValue() != assigned) return 4;\n"
        "  table.Remove(it);\n"
        "  if (table.IsHere(key2)) return 5;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultXHashIDItGet() {\n"
        "  XHashIDIt it;\n"
        "  it.GetValue();\n"
        "}\n"
        "void RejectDefaultXHashIDItInc() {\n"
        "  XHashIDIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectMissingXHashIDItGet() {\n"
        "  XHashID table;\n"
        "  CK_ID missing = 99;\n"
        "  XHashIDIt it = table.Find(missing);\n"
        "  it.GetValue();\n"
        "}\n"
        "void RejectEndXHashIDItKey() {\n"
        "  XHashID table;\n"
        "  XHashIDIt it = table.End();\n"
        "  it.GetKey();\n"
        "}\n"
        "void RejectDefaultXHashIDRemove() {\n"
        "  XHashID table;\n"
        "  XHashIDIt it;\n"
        "  table.Remove(it);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XHashID self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xhashid-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XHashID self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XHashID self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXHashID()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultXHashIDItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultXHashIDItInc()");
    asIScriptFunction *missingGet = module->GetFunctionByDecl("void RejectMissingXHashIDItGet()");
    asIScriptFunction *endKey = module->GetFunctionByDecl("void RejectEndXHashIDItKey()");
    asIScriptFunction *defaultRemove = module->GetFunctionByDecl("void RejectDefaultXHashIDRemove()");
    if (!probe || !defaultGet || !defaultInc || !missingGet || !endKey || !defaultRemove) {
        engine->DiscardModule(moduleName);
        error = "XHashID self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XHashID container probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "XHashID default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "XHashID default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, missingGet, true, "XHashID missing Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, endKey, true, "XHashID end GetKey rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultRemove, true, "XHashID default Remove rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXStringArrayScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XStringArray script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("XStringArray");
    if (!arrayType) {
        error = "XStringArray self-test could not find XStringArray.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("bool RemoveAt(int pos, XString &out old)") ||
        arrayType->GetMethodByDecl("XString& RemoveAt(int pos)") ||
        !arrayType->GetMethodByDecl("XString& opIndex(int index)") ||
        !arrayType->GetMethodByDecl("XString& Back()") ||
        !arrayType->GetMethodByDecl("const XString& Back() const") ||
        !arrayType->GetMethodByDecl("int GetMemoryOccupation(bool addStatic = false) const")) {
        error = "XStringArray self-test found stale or missing method declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XStringArraySelfTest";
    const char *source =
        "int ProbeXStringArray() {\n"
        "  XStringArray values;\n"
        "  values.PushBack(XString(\"first\"));\n"
        "  values.PushBack(XString(\"second\"));\n"
        "  if (values.Size() != 2) return 1;\n"
        "  if (!(values.Back() == XString(\"second\"))) return 2;\n"
        "  values[0] = XString(\"changed\");\n"
        "  if (!(values[0] == XString(\"changed\"))) return 3;\n"
        "  XString old;\n"
        "  if (!values.RemoveAt(0, old)) return 4;\n"
        "  if (!(old == XString(\"changed\"))) return 5;\n"
        "  if (values.Size() != 1 || !(values[0] == XString(\"second\"))) return 6;\n"
        "  if (values.RemoveAt(-1, old)) return 7;\n"
        "  if (values.RemoveAt(99, old)) return 8;\n"
        "  if (values.GetMemoryOccupation(false) < 0) return 9;\n"
        "  if (values.GetMemoryOccupation(true) < 0) return 10;\n"
        "  return 0;\n"
        "}\n"
        "void RejectXStringArrayNegativeIndex() {\n"
        "  XStringArray values;\n"
        "  values.PushBack(XString(\"value\"));\n"
        "  values[-1] = XString(\"bad\");\n"
        "}\n"
        "void RejectXStringArrayOutOfRangeIndex() {\n"
        "  XStringArray values;\n"
        "  values.PushBack(XString(\"value\"));\n"
        "  values[1] = XString(\"bad\");\n"
        "}\n"
        "void RejectXStringArrayEmptyBack() {\n"
        "  XStringArray values;\n"
        "  values.Back();\n"
        "}\n"
        "void RejectXStringArrayConstEmptyBack() {\n"
        "  const XStringArray values;\n"
        "  values.Back();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XStringArray self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xstringarray-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XStringArray self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XStringArray self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeXStringArray()");
    asIScriptFunction *negativeIndex = module->GetFunctionByDecl("void RejectXStringArrayNegativeIndex()");
    asIScriptFunction *outOfRangeIndex = module->GetFunctionByDecl("void RejectXStringArrayOutOfRangeIndex()");
    asIScriptFunction *emptyBack = module->GetFunctionByDecl("void RejectXStringArrayEmptyBack()");
    asIScriptFunction *constEmptyBack = module->GetFunctionByDecl("void RejectXStringArrayConstEmptyBack()");
    if (!probe || !negativeIndex || !outOfRangeIndex || !emptyBack || !constEmptyBack) {
        engine->DiscardModule(moduleName);
        error = "XStringArray self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "XStringArray container probe", error) &&
              ExecuteCKAttributeDescProbe(engine, negativeIndex, true, "XStringArray negative index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, outOfRangeIndex, true, "XStringArray out-of-range index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyBack, true, "XStringArray empty Back rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, constEmptyBack, true, "XStringArray const empty Back rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPathEntryVectorScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPATHENTRYVECTOR script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("CKPATHENTRYVECTOR");
    if (!arrayType) {
        error = "CKPATHENTRYVECTOR self-test could not find CKPATHENTRYVECTOR.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("bool RemoveAt(int pos, XString &out old)") ||
        arrayType->GetMethodByDecl("XString& RemoveAt(int pos)") ||
        !arrayType->GetMethodByDecl("XString& opIndex(int index)") ||
        !arrayType->GetMethodByDecl("XString& Back()") ||
        !arrayType->GetMethodByDecl("const XString& Back() const") ||
        !arrayType->GetMethodByDecl("int GetMemoryOccupation(bool addStatic = false) const")) {
        error = "CKPATHENTRYVECTOR self-test found stale or missing method declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPATHENTRYVECTORSelfTest";
    const char *source =
        "int ProbeCKPathEntryVector() {\n"
        "  CKPATHENTRYVECTOR values;\n"
        "  values.PushBack(XString(\"first\"));\n"
        "  values.PushBack(XString(\"second\"));\n"
        "  if (values.Size() != 2) return 1;\n"
        "  if (!(values.Back() == XString(\"second\"))) return 2;\n"
        "  values[0] = XString(\"changed\");\n"
        "  if (!(values[0] == XString(\"changed\"))) return 3;\n"
        "  XString old;\n"
        "  if (!values.RemoveAt(0, old)) return 4;\n"
        "  if (!(old == XString(\"changed\"))) return 5;\n"
        "  if (values.Size() != 1 || !(values[0] == XString(\"second\"))) return 6;\n"
        "  if (values.RemoveAt(-1, old)) return 7;\n"
        "  if (values.RemoveAt(99, old)) return 8;\n"
        "  if (values.GetMemoryOccupation(false) < 0) return 9;\n"
        "  if (values.GetMemoryOccupation(true) < 0) return 10;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKPathEntryVectorNegativeIndex() {\n"
        "  CKPATHENTRYVECTOR values;\n"
        "  values.PushBack(XString(\"value\"));\n"
        "  values[-1] = XString(\"bad\");\n"
        "}\n"
        "void RejectCKPathEntryVectorOutOfRangeIndex() {\n"
        "  CKPATHENTRYVECTOR values;\n"
        "  values.PushBack(XString(\"value\"));\n"
        "  values[1] = XString(\"bad\");\n"
        "}\n"
        "void RejectCKPathEntryVectorEmptyBack() {\n"
        "  CKPATHENTRYVECTOR values;\n"
        "  values.Back();\n"
        "}\n"
        "void RejectCKPathEntryVectorConstEmptyBack() {\n"
        "  const CKPATHENTRYVECTOR values;\n"
        "  values.Back();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPATHENTRYVECTOR self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckpathentryvector-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPATHENTRYVECTOR self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPATHENTRYVECTOR self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPathEntryVector()");
    asIScriptFunction *negativeIndex = module->GetFunctionByDecl("void RejectCKPathEntryVectorNegativeIndex()");
    asIScriptFunction *outOfRangeIndex = module->GetFunctionByDecl("void RejectCKPathEntryVectorOutOfRangeIndex()");
    asIScriptFunction *emptyBack = module->GetFunctionByDecl("void RejectCKPathEntryVectorEmptyBack()");
    asIScriptFunction *constEmptyBack = module->GetFunctionByDecl("void RejectCKPathEntryVectorConstEmptyBack()");
    if (!probe || !negativeIndex || !outOfRangeIndex || !emptyBack || !constEmptyBack) {
        engine->DiscardModule(moduleName);
        error = "CKPATHENTRYVECTOR self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPATHENTRYVECTOR container probe", error) &&
              ExecuteCKAttributeDescProbe(engine, negativeIndex, true, "CKPATHENTRYVECTOR negative index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, outOfRangeIndex, true, "CKPATHENTRYVECTOR out-of-range index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyBack, true, "CKPATHENTRYVECTOR empty Back rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, constEmptyBack, true, "CKPATHENTRYVECTOR const empty Back rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPathCategoryVectorScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPATHCATEGORYVECTOR script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("CKPATHCATEGORYVECTOR");
    if (!arrayType) {
        error = "CKPATHCATEGORYVECTOR self-test could not find CKPATHCATEGORYVECTOR.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("bool RemoveAt(int pos, CKPATHCATEGORY &out old)") ||
        arrayType->GetMethodByDecl("CKPATHCATEGORY& RemoveAt(int pos)") ||
        arrayType->GetMethodByDecl("void FastRemove(const CKPATHCATEGORY &in o)") ||
        arrayType->GetMethodByDecl("int GetPosition(const CKPATHCATEGORY &in o) const") ||
        !arrayType->GetMethodByDecl("CKPATHCATEGORY& opIndex(int index)") ||
        !arrayType->GetMethodByDecl("CKPATHCATEGORY& Back()") ||
        !arrayType->GetMethodByDecl("const CKPATHCATEGORY& Back() const") ||
        !arrayType->GetMethodByDecl("int GetMemoryOccupation(bool addStatic = false) const")) {
        error = "CKPATHCATEGORYVECTOR self-test found stale or missing method declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPATHCATEGORYVECTORSelfTest";
    const char *source =
        "int ProbeCKPathCategoryVector() {\n"
        "  CKPATHCATEGORYVECTOR values;\n"
        "  CKPATHCATEGORY first;\n"
        "  CKPATHCATEGORY second;\n"
        "  first.m_Name = XString(\"first\");\n"
        "  second.m_Name = XString(\"second\");\n"
        "  first.m_Entries.PushBack(XString(\"entry\"));\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  if (values.Size() != 2) return 1;\n"
        "  if (!(values.Back().m_Name == XString(\"second\"))) return 2;\n"
        "  values[0].m_Name = XString(\"changed\");\n"
        "  if (!(values[0].m_Name == XString(\"changed\"))) return 3;\n"
        "  if (values[0].m_Entries.Size() != 1) return 4;\n"
        "  CKPATHCATEGORY old;\n"
        "  if (!values.RemoveAt(0, old)) return 5;\n"
        "  if (!(old.m_Name == XString(\"changed\"))) return 6;\n"
        "  if (old.m_Entries.Size() != 1) return 7;\n"
        "  if (values.Size() != 1 || !(values[0].m_Name == XString(\"second\"))) return 8;\n"
        "  if (values.RemoveAt(-1, old)) return 9;\n"
        "  if (values.RemoveAt(99, old)) return 10;\n"
        "  if (values.GetMemoryOccupation(false) < 0) return 11;\n"
        "  if (values.GetMemoryOccupation(true) < 0) return 12;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKPathCategoryVectorNegativeIndex() {\n"
        "  CKPATHCATEGORYVECTOR values;\n"
        "  CKPATHCATEGORY value;\n"
        "  values.PushBack(value);\n"
        "  values[-1].m_Name = XString(\"bad\");\n"
        "}\n"
        "void RejectCKPathCategoryVectorOutOfRangeIndex() {\n"
        "  CKPATHCATEGORYVECTOR values;\n"
        "  CKPATHCATEGORY value;\n"
        "  values.PushBack(value);\n"
        "  values[1].m_Name = XString(\"bad\");\n"
        "}\n"
        "void RejectCKPathCategoryVectorEmptyBack() {\n"
        "  CKPATHCATEGORYVECTOR values;\n"
        "  values.Back();\n"
        "}\n"
        "void RejectCKPathCategoryVectorConstEmptyBack() {\n"
        "  const CKPATHCATEGORYVECTOR values;\n"
        "  values.Back();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPATHCATEGORYVECTOR self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckpathcategoryvector-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPATHCATEGORYVECTOR self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPATHCATEGORYVECTOR self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPathCategoryVector()");
    asIScriptFunction *negativeIndex = module->GetFunctionByDecl("void RejectCKPathCategoryVectorNegativeIndex()");
    asIScriptFunction *outOfRangeIndex = module->GetFunctionByDecl("void RejectCKPathCategoryVectorOutOfRangeIndex()");
    asIScriptFunction *emptyBack = module->GetFunctionByDecl("void RejectCKPathCategoryVectorEmptyBack()");
    asIScriptFunction *constEmptyBack = module->GetFunctionByDecl("void RejectCKPathCategoryVectorConstEmptyBack()");
    if (!probe || !negativeIndex || !outOfRangeIndex || !emptyBack || !constEmptyBack) {
        engine->DiscardModule(moduleName);
        error = "CKPATHCATEGORYVECTOR self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPATHCATEGORYVECTOR container probe", error) &&
              ExecuteCKAttributeDescProbe(engine, negativeIndex, true, "CKPATHCATEGORYVECTOR negative index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, outOfRangeIndex, true, "CKPATHCATEGORYVECTOR out-of-range index rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyBack, true, "CKPATHCATEGORYVECTOR empty Back rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, constEmptyBack, true, "CKPATHCATEGORYVECTOR const empty Back rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunXClassArrayRemoveAtScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "XClassArray RemoveAt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *classInfoArrayType = engine->GetTypeInfoByDecl("XClassInfoArray");
    if (!classInfoArrayType) {
        error = "XClassArray RemoveAt self-test could not find XClassInfoArray.";
        return false;
    }
    if (!classInfoArrayType->GetMethodByDecl("bool RemoveAt(int pos, CKClassDesc &out old)") ||
        classInfoArrayType->GetMethodByDecl("CKClassDesc& RemoveAt(int pos)")) {
        error = "XClassInfoArray self-test found stale or missing RemoveAt declarations.";
        return false;
    }

    asITypeInfo *pluginDepsArrayType = engine->GetTypeInfoByDecl("XFilePluginDependenciesArray");
    if (!pluginDepsArrayType) {
        error = "XClassArray RemoveAt self-test could not find XFilePluginDependenciesArray.";
        return false;
    }
    if (!pluginDepsArrayType->GetMethodByDecl("bool RemoveAt(int pos, CKFilePluginDependencies &out old)") ||
        pluginDepsArrayType->GetMethodByDecl("CKFilePluginDependencies& RemoveAt(int pos)")) {
        error = "XFilePluginDependenciesArray self-test found stale or missing RemoveAt declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_XClassArrayRemoveAtSelfTest";
    const char *source =
        "int ProbeXClassInfoArrayRemoveAt() {\n"
        "  XClassInfoArray values;\n"
        "  CKClassDesc first;\n"
        "  CKClassDesc second;\n"
        "  first.Done = 11;\n"
        "  second.Done = 22;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  CKClassDesc old;\n"
        "  if (!values.RemoveAt(0, old) || old.Done != 11) return 1;\n"
        "  if (values.Size() != 1 || values[0].Done != 22) return 2;\n"
        "  if (values.RemoveAt(-1, old)) return 3;\n"
        "  if (values.RemoveAt(99, old)) return 4;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeXFilePluginDependenciesArrayRemoveAt() {\n"
        "  XFilePluginDependenciesArray values;\n"
        "  CKFilePluginDependencies first;\n"
        "  CKFilePluginDependencies second;\n"
        "  first.m_PluginCategory = 11;\n"
        "  second.m_PluginCategory = 22;\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  CKFilePluginDependencies old;\n"
        "  if (!values.RemoveAt(0, old) || old.m_PluginCategory != 11) return 1;\n"
        "  if (values.Size() != 1 || values[0].m_PluginCategory != 22) return 2;\n"
        "  if (values.RemoveAt(-1, old)) return 3;\n"
        "  if (values.RemoveAt(99, old)) return 4;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "XClassArray RemoveAt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("xclassarray-removeat-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XClassArray RemoveAt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "XClassArray RemoveAt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *classInfoProbe = module->GetFunctionByDecl("int ProbeXClassInfoArrayRemoveAt()");
    asIScriptFunction *pluginDepsProbe = module->GetFunctionByDecl("int ProbeXFilePluginDependenciesArrayRemoveAt()");
    if (!classInfoProbe || !pluginDepsProbe) {
        engine->DiscardModule(moduleName);
        error = "XClassArray RemoveAt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, classInfoProbe, false, "XClassInfoArray RemoveAt probe", error) &&
              ExecuteCKAttributeDescProbe(engine, pluginDepsProbe, false, "XFilePluginDependenciesArray RemoveAt probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPathCategoryVectorItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPATHCATEGORYVECTORIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("CKPATHCATEGORYVECTORIt");
    if (!iteratorType) {
        error = "CKPATHCATEGORYVECTORIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const CKPATHCATEGORYVECTORIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const CKPATHCATEGORYVECTORIt &in other) const")) {
        error = "CKPATHCATEGORYVECTORIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("CKPATHCATEGORYVECTOR");
    if (!arrayType) {
        error = "CKPATHCATEGORYVECTORIt self-test could not find CKPATHCATEGORYVECTOR.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("CKPATHCATEGORYVECTORIt Begin() const") ||
        !arrayType->GetMethodByDecl("CKPATHCATEGORYVECTORIt End() const")) {
        error = "CKPATHCATEGORYVECTORIt self-test could not find CKPATHCATEGORYVECTOR iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPATHCATEGORYVECTORItSelfTest";
    const char *source =
        "int ProbeCKPathCategoryVectorIt() {\n"
        "  CKPATHCATEGORYVECTOR values;\n"
        "  CKPATHCATEGORY first;\n"
        "  CKPATHCATEGORY second;\n"
        "  first.m_Name = XString(\"first\");\n"
        "  second.m_Name = XString(\"second\");\n"
        "  values.PushBack(first);\n"
        "  values.PushBack(second);\n"
        "  CKPATHCATEGORYVECTORIt begin = values.Begin();\n"
        "  CKPATHCATEGORYVECTORIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (!(begin.Get().m_Name == XString(\"first\"))) return 3;\n"
        "  begin.Get().m_Name = XString(\"changed\");\n"
        "  if (!(values[0].m_Name == XString(\"changed\"))) return 4;\n"
        "  CKPATHCATEGORYVECTORIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 5;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 6;\n"
        "  if (!(copied.Get().m_Name == XString(\"second\"))) return 7;\n"
        "  CKPATHCATEGORYVECTORIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 8;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 9;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultCKPathCategoryVectorItGet() {\n"
        "  CKPATHCATEGORYVECTORIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultCKPathCategoryVectorItInc() {\n"
        "  CKPATHCATEGORYVECTORIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultCKPathCategoryVectorItDec() {\n"
        "  CKPATHCATEGORYVECTORIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyCKPathCategoryVectorItGet() {\n"
        "  CKPATHCATEGORYVECTOR values;\n"
        "  CKPATHCATEGORYVECTORIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPATHCATEGORYVECTORIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckpathcategoryvectorit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPATHCATEGORYVECTORIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPATHCATEGORYVECTORIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPathCategoryVectorIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultCKPathCategoryVectorItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultCKPathCategoryVectorItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultCKPathCategoryVectorItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyCKPathCategoryVectorItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "CKPATHCATEGORYVECTORIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPATHCATEGORYVECTORIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "CKPATHCATEGORYVECTORIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "CKPATHCATEGORYVECTORIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "CKPATHCATEGORYVECTORIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "CKPATHCATEGORYVECTORIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPathEntryVectorItScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPATHENTRYVECTORIt script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("CKPATHENTRYVECTORIt");
    if (!iteratorType) {
        error = "CKPATHENTRYVECTORIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const CKPATHENTRYVECTORIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const CKPATHENTRYVECTORIt &in other) const")) {
        error = "CKPATHENTRYVECTORIt self-test could not find iterator comparison methods.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("CKPATHENTRYVECTOR");
    if (!arrayType) {
        error = "CKPATHENTRYVECTORIt self-test could not find CKPATHENTRYVECTOR.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("CKPATHENTRYVECTORIt Begin() const") ||
        !arrayType->GetMethodByDecl("CKPATHENTRYVECTORIt End() const")) {
        error = "CKPATHENTRYVECTORIt self-test could not find CKPATHENTRYVECTOR iterator producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPATHENTRYVECTORItSelfTest";
    const char *source =
        "int ProbeCKPathEntryVectorIt() {\n"
        "  CKPATHENTRYVECTOR values;\n"
        "  values.PushBack(XString(\"first\"));\n"
        "  values.PushBack(XString(\"second\"));\n"
        "  CKPATHENTRYVECTORIt begin = values.Begin();\n"
        "  CKPATHENTRYVECTORIt end = values.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (!(begin.Get() == XString(\"first\"))) return 3;\n"
        "  begin.Get() = XString(\"changed\");\n"
        "  if (!(values[0] == XString(\"changed\"))) return 4;\n"
        "  CKPATHENTRYVECTORIt copied(begin);\n"
        "  if (!(copied == begin) || copied != begin) return 5;\n"
        "  ++copied;\n"
        "  if (copied == begin || !(copied != begin)) return 6;\n"
        "  if (!(copied.Get() == XString(\"second\"))) return 7;\n"
        "  CKPATHENTRYVECTORIt assigned;\n"
        "  assigned = copied;\n"
        "  if (!(assigned == copied)) return 8;\n"
        "  ++assigned;\n"
        "  if (!(assigned == end) || assigned != end) return 9;\n"
        "  return 0;\n"
        "}\n"
        "void RejectDefaultCKPathEntryVectorItGet() {\n"
        "  CKPATHENTRYVECTORIt it;\n"
        "  it.Get();\n"
        "}\n"
        "void RejectDefaultCKPathEntryVectorItInc() {\n"
        "  CKPATHENTRYVECTORIt it;\n"
        "  ++it;\n"
        "}\n"
        "void RejectDefaultCKPathEntryVectorItDec() {\n"
        "  CKPATHENTRYVECTORIt it;\n"
        "  --it;\n"
        "}\n"
        "void RejectEmptyCKPathEntryVectorItGet() {\n"
        "  CKPATHENTRYVECTOR values;\n"
        "  CKPATHENTRYVECTORIt it = values.Begin();\n"
        "  it.Get();\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPATHENTRYVECTORIt self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckpathentryvectorit-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPATHENTRYVECTORIt self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPATHENTRYVECTORIt self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPathEntryVectorIt()");
    asIScriptFunction *defaultGet = module->GetFunctionByDecl("void RejectDefaultCKPathEntryVectorItGet()");
    asIScriptFunction *defaultInc = module->GetFunctionByDecl("void RejectDefaultCKPathEntryVectorItInc()");
    asIScriptFunction *defaultDec = module->GetFunctionByDecl("void RejectDefaultCKPathEntryVectorItDec()");
    asIScriptFunction *emptyGet = module->GetFunctionByDecl("void RejectEmptyCKPathEntryVectorItGet()");
    if (!probe || !defaultGet || !defaultInc || !defaultDec || !emptyGet) {
        engine->DiscardModule(moduleName);
        error = "CKPATHENTRYVECTORIt self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPATHENTRYVECTORIt iterator probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultGet, true, "CKPATHENTRYVECTORIt default Get rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultInc, true, "CKPATHENTRYVECTORIt default increment rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultDec, true, "CKPATHENTRYVECTORIt default decrement rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, emptyGet, true, "CKPATHENTRYVECTORIt empty Begin Get rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKAttributeDescScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKAttributeDesc script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAttributeDescSelfTest";
    const char *source =
        "int ProbeAttributeDesc() {\n"
        "  CKAttributeDesc desc;\n"
        "  string longName = \"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ABCDEFGH\";\n"
        "  desc.Name = longName;\n"
        "  if (desc.Name.length() != 63) return 1;\n"
        "  if (desc.Name != longName.substr(0, 63)) return 2;\n"
        "  if (!desc.CallbackFct.IsNull() || !desc.CallbackArg.IsNull()) return 3;\n"
        "  if (!desc.DefaultValuePointer.IsNull() || desc.DefaultValue != \"\") return 4;\n"
        "  if (!desc.CreatorDll.IsNull()) return 5;\n"
        "  NativePointer empty;\n"
        "  desc.CallbackFct = empty;\n"
        "  desc.CallbackArg = empty;\n"
        "  desc.DefaultValuePointer = empty;\n"
        "  desc.CreatorDll = empty;\n"
        "  CKAttributeDesc copied(desc);\n"
        "  if (copied.Name != desc.Name || !copied.CallbackFct.IsNull()) return 6;\n"
        "  CKAttributeDesc assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.Name != desc.Name || !assigned.CreatorDll.IsNull()) return 7;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeDescCallbackFctReject() {\n"
        "  CKAttributeDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.CallbackFct = ptr;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeDescCallbackArgReject() {\n"
        "  CKAttributeDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.CallbackArg = ptr;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeDescDefaultValueReject() {\n"
        "  CKAttributeDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.DefaultValuePointer = ptr;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeDescCreatorDllReject() {\n"
        "  CKAttributeDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.CreatorDll = ptr;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAttributeDesc self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-attribute-desc-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeDesc self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeDesc self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeAttributeDesc()");
    asIScriptFunction *callbackFctReject = module->GetFunctionByDecl("int ProbeAttributeDescCallbackFctReject()");
    asIScriptFunction *callbackArgReject = module->GetFunctionByDecl("int ProbeAttributeDescCallbackArgReject()");
    asIScriptFunction *defaultValueReject = module->GetFunctionByDecl("int ProbeAttributeDescDefaultValueReject()");
    asIScriptFunction *creatorDllReject = module->GetFunctionByDecl("int ProbeAttributeDescCreatorDllReject()");
    if (!probe || !callbackFctReject || !callbackArgReject || !defaultValueReject || !creatorDllReject) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeDesc self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKAttributeDesc value probe", error) &&
              ExecuteCKAttributeDescProbe(engine, callbackFctReject, true, "CKAttributeDesc CallbackFct rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, callbackArgReject, true, "CKAttributeDesc CallbackArg rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultValueReject, true, "CKAttributeDesc DefaultValuePointer rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, creatorDllReject, true, "CKAttributeDesc CreatorDll rejection probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKAttributeCategoryDescScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKAttributeCategoryDesc script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAttributeCategoryDescSelfTest";
    const char *source =
        "int ProbeAttributeCategoryDesc() {\n"
        "  CKAttributeCategoryDesc desc;\n"
        "  if (desc.Name != \"\" || !desc.NamePointer.IsNull() || desc.Flags != 0) return 1;\n"
        "  desc.Name = \"Gameplay\";\n"
        "  desc.Flags = 42;\n"
        "  if (desc.Name != \"Gameplay\" || desc.NamePointer.IsNull()) return 2;\n"
        "  CKAttributeCategoryDesc copied(desc);\n"
        "  if (copied.Name != \"Gameplay\" || copied.Flags != 42 || copied.NamePointer.IsNull()) return 3;\n"
        "  desc.Name = \"Changed\";\n"
        "  if (copied.Name != \"Gameplay\") return 4;\n"
        "  CKAttributeCategoryDesc assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.Name != \"Gameplay\" || assigned.Flags != 42) return 5;\n"
        "  copied.Name = \"Other\";\n"
        "  if (assigned.Name != \"Gameplay\") return 6;\n"
        "  NativePointer empty;\n"
        "  assigned.NamePointer = empty;\n"
        "  if (assigned.Name != \"\" || !assigned.NamePointer.IsNull()) return 7;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeCategoryDescNamePointerReject() {\n"
        "  CKAttributeCategoryDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.NamePointer = ptr;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAttributeCategoryDesc self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-attribute-category-desc-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeCategoryDesc self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeCategoryDesc self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeAttributeCategoryDesc()");
    asIScriptFunction *namePointerReject = module->GetFunctionByDecl("int ProbeAttributeCategoryDescNamePointerReject()");
    if (!probe || !namePointerReject) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeCategoryDesc self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKAttributeCategoryDesc value probe", error) &&
              ExecuteCKAttributeDescProbe(engine, namePointerReject, true, "CKAttributeCategoryDesc NamePointer rejection probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCK2dCurvePointScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CK2dCurvePoint script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *pointType = engine->GetTypeInfoByDecl("CK2dCurvePoint");
    if (!pointType) {
        error = "CK2dCurvePoint type is not registered.";
        return false;
    }
    if (!pointType->GetMethodByDecl("NativePointer GetCurve() const")) {
        error = "CK2dCurvePoint NativePointer GetCurve declaration is not registered.";
        return false;
    }
    if (pointType->GetMethodByDecl("CK2dCurve &GetCurve() const")) {
        error = "CK2dCurvePoint still exposes the old non-null GetCurve reference declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CK2dCurvePointSelfTest";
    const char *source =
        "int ProbeCurvePointDetachedOwner() {\n"
        "  CK2dCurvePoint point;\n"
        "  if (!point.GetCurve().IsNull()) return 1;\n"
        "  point.SetBias(0.25f);\n"
        "  point.SetTension(0.5f);\n"
        "  point.SetContinuity(0.75f);\n"
        "  point.SetLinear(true);\n"
        "  if (!point.IsLinear()) return 2;\n"
        "  point.SetLinear(false);\n"
        "  if (point.IsLinear()) return 3;\n"
        "  point.UseTCB(false);\n"
        "  if (point.IsTCB()) return 4;\n"
        "  point.UseTCB(true);\n"
        "  if (!point.IsTCB()) return 5;\n"
        "  point.SetPosition(Vx2DVector(1.0f, 2.0f));\n"
        "  point.SetInTangent(Vx2DVector(3.0f, 4.0f));\n"
        "  point.SetOutTangent(Vx2DVector(5.0f, 6.0f));\n"
        "  point.NotifyUpdate();\n"
        "  CK2dCurvePoint copied(point);\n"
        "  if (!copied.GetCurve().IsNull()) return 6;\n"
        "  CK2dCurvePoint assigned;\n"
        "  assigned = point;\n"
        "  if (!assigned.GetCurve().IsNull()) return 7;\n"
        "  if (assigned.GetBias() != point.GetBias()) return 8;\n"
        "  if (assigned.GetTension() != point.GetTension()) return 9;\n"
        "  if (assigned.GetContinuity() != point.GetContinuity()) return 10;\n"
        "  if (copied.GetPosition().x != 1.0f || copied.GetPosition().y != 2.0f) return 11;\n"
        "  if (assigned.GetInTangent().x != 3.0f || assigned.GetInTangent().y != 4.0f) return 12;\n"
        "  if (assigned.GetOutTangent().x != 5.0f || assigned.GetOutTangent().y != 6.0f) return 13;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CK2dCurvePoint self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-2d-curve-point-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK2dCurvePoint self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK2dCurvePoint self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCurvePointDetachedOwner()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CK2dCurvePoint self-test function was not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CK2dCurvePoint detached owner probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCK2dCurveScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CK2dCurve script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *curveType = engine->GetTypeInfoByDecl("CK2dCurve");
    if (!curveType) {
        error = "CK2dCurve type is not registered.";
        return false;
    }
    if (curveType->GetMethodByDecl("CKStateChunk@ Dump()") ||
        curveType->GetMethodByDecl("CKERROR Read(CKStateChunk@ chunk)")) {
        error = "CK2dCurve still exposes internal Dump/Read persistence methods.";
        return false;
    }
    if (curveType->GetMethodByDecl("void DeleteControlPoint(CK2dCurvePoint &in cpt)")) {
        error = "CK2dCurve still exposes stale reference-based DeleteControlPoint.";
        return false;
    }
    if (!curveType->GetMethodByDecl("bool DeleteControlPoint(int pos)")) {
        error = "CK2dCurve indexed DeleteControlPoint declaration is not registered.";
        return false;
    }

    return true;
}

bool RunVxColorScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "VxColor script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *colorType = engine->GetTypeInfoByDecl("VxColor");
    if (!colorType) {
        error = "VxColor type is not registered.";
        return false;
    }
    if (!colorType->GetMethodByDecl("bool opEquals(const VxColor &in color) const")) {
        error = "VxColor bool opEquals declaration is not registered.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_VxColorSelfTest";
    const char *source =
        "int ProbeVxColorEquals() {\n"
        "  VxColor a(0.1f, 0.2f, 0.3f, 1.0f);\n"
        "  VxColor b(0.1f, 0.2f, 0.3f, 1.0f);\n"
        "  VxColor c(0.1f, 0.2f, 0.4f, 1.0f);\n"
        "  if (!(a == b)) return 1;\n"
        "  if (a == c) return 2;\n"
        "  bool equal = a.opEquals(b);\n"
        "  if (!equal) return 3;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "VxColor self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("vx-color-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "VxColor self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "VxColor self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeVxColorEquals()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "VxColor self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "VxColor equality probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunVxEffectDescriptionScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "VxEffectDescription script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *effectType = engine->GetTypeInfoByDecl("VxEffectDescription");
    if (!effectType) {
        error = "VxEffectDescription type is not registered.";
        return false;
    }
    for (asUINT i = 0; i < effectType->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        if (effectType->GetProperty(i, &propertyName) >= 0 && propertyName &&
            (std::strcmp(propertyName, "SetCallback") == 0 || std::strcmp(propertyName, "CallbackArg") == 0)) {
            error = "VxEffectDescription still exposes direct callback pointer properties.";
            return false;
        }
    }
    if (!effectType->GetMethodByDecl("NativePointer get_SetCallback() const") ||
        !effectType->GetMethodByDecl("void set_SetCallback(NativePointer ptr)") ||
        !effectType->GetMethodByDecl("NativePointer get_CallbackArg() const") ||
        !effectType->GetMethodByDecl("void set_CallbackArg(NativePointer ptr)")) {
        error = "VxEffectDescription callback NativePointer accessors are not registered.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_VxEffectDescriptionSelfTest";
    const char *source =
        "int ProbeVxEffectDescription() {\n"
        "  VxEffectDescription desc;\n"
        "  NativePointer empty;\n"
        "  desc.SetCallback = empty;\n"
        "  desc.CallbackArg = empty;\n"
        "  if (!desc.SetCallback.IsNull() || !desc.CallbackArg.IsNull()) return 1;\n"
        "  desc.Summary = \"effect-summary\";\n"
        "  desc.MaxTextureCount = 2;\n"
        "  VxEffectDescription copied(desc);\n"
        "  if (copied.Summary != \"effect-summary\" || !copied.SetCallback.IsNull() || !copied.CallbackArg.IsNull()) return 3;\n"
        "  VxEffectDescription assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.MaxTextureCount != 2 || !assigned.CallbackArg.IsNull()) return 4;\n"
        "  assigned.CallbackArg = empty;\n"
        "  if (!assigned.CallbackArg.IsNull()) return 5;\n"
        "  return 0;\n"
        "}\n"
        "void RejectVxEffectDescriptionSetCallback() {\n"
        "  VxEffectDescription desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.SetCallback = ptr;\n"
        "}\n"
        "void RejectVxEffectDescriptionCallbackArg() {\n"
        "  VxEffectDescription desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.CallbackArg = ptr;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "VxEffectDescription self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("vx-effect-description-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "VxEffectDescription self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "VxEffectDescription self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeVxEffectDescription()");
    asIScriptFunction *rejectCallback = module->GetFunctionByDecl("void RejectVxEffectDescriptionSetCallback()");
    asIScriptFunction *rejectCallbackArg = module->GetFunctionByDecl("void RejectVxEffectDescriptionCallbackArg()");
    if (!probe || !rejectCallback || !rejectCallbackArg) {
        engine->DiscardModule(moduleName);
        error = "VxEffectDescription self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "VxEffectDescription value probe", error) &&
              ExecuteCKAttributeDescProbe(engine, rejectCallback, true, "VxEffectDescription SetCallback rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, rejectCallbackArg, true, "VxEffectDescription CallbackArg rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKOperationDescScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKOperationDesc script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *operationType = engine->GetTypeInfoByDecl("CKOperationDesc");
    if (!operationType) {
        error = "CKOperationDesc type is not registered.";
        return false;
    }
    for (asUINT i = 0; i < operationType->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        if (operationType->GetProperty(i, &propertyName) >= 0 && propertyName && std::strcmp(propertyName, "Fct") == 0) {
            error = "CKOperationDesc still exposes direct function pointer property.";
            return false;
        }
    }
    if (!operationType->GetMethodByDecl("NativePointer get_Fct() const") ||
        !operationType->GetMethodByDecl("void set_Fct(NativePointer ptr)")) {
        error = "CKOperationDesc Fct NativePointer accessors are not registered.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKOperationDescSelfTest";
    const char *source =
        "int ProbeCKOperationDesc() {\n"
        "  CKOperationDesc desc;\n"
        "  NativePointer empty;\n"
        "  desc.Fct = empty;\n"
        "  if (!desc.Fct.IsNull()) return 1;\n"
        "  desc.OpGuid = CKGUID(1, 2);\n"
        "  CKOperationDesc copied(desc);\n"
        "  if (copied.OpGuid != desc.OpGuid || !copied.Fct.IsNull()) return 2;\n"
        "  CKOperationDesc assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.OpGuid != desc.OpGuid || !assigned.Fct.IsNull()) return 3;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKOperationDescFct() {\n"
        "  CKOperationDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.Fct = ptr;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKOperationDesc self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-operation-desc-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKOperationDesc self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKOperationDesc self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKOperationDesc()");
    asIScriptFunction *rejectFct = module->GetFunctionByDecl("void RejectCKOperationDescFct()");
    if (!probe || !rejectFct) {
        engine->DiscardModule(moduleName);
        error = "CKOperationDesc self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKOperationDesc value probe", error) &&
              ExecuteCKAttributeDescProbe(engine, rejectFct, true, "CKOperationDesc Fct rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKClassDescScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKClassDesc script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *classType = engine->GetTypeInfoByDecl("CKClassDesc");
    if (!classType) {
        error = "CKClassDesc type is not registered.";
        return false;
    }
    for (asUINT i = 0; i < classType->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        if (classType->GetProperty(i, &propertyName) >= 0 && propertyName &&
            (std::strcmp(propertyName, "RegisterFct") == 0 ||
             std::strcmp(propertyName, "CreationFct") == 0 ||
             std::strcmp(propertyName, "NameFct") == 0 ||
             std::strcmp(propertyName, "DependsFct") == 0 ||
             std::strcmp(propertyName, "DependsCountFct") == 0)) {
            error = "CKClassDesc still exposes direct callback pointer properties.";
            return false;
        }
    }
    if (!classType->GetMethodByDecl("void set_RegisterFct(NativePointer ptr)") ||
        !classType->GetMethodByDecl("void set_CreationFct(NativePointer ptr)") ||
        !classType->GetMethodByDecl("void set_NameFct(NativePointer ptr)") ||
        !classType->GetMethodByDecl("void set_DependsFct(NativePointer ptr)") ||
        !classType->GetMethodByDecl("void set_DependsCountFct(NativePointer ptr)")) {
        error = "CKClassDesc callback NativePointer setters are not registered.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKClassDescSelfTest";
    const char *source =
        "int ProbeCKClassDesc() {\n"
        "  CKClassDesc desc;\n"
        "  NativePointer empty;\n"
        "  desc.RegisterFct = empty;\n"
        "  desc.CreationFct = empty;\n"
        "  desc.NameFct = empty;\n"
        "  desc.DependsFct = empty;\n"
        "  desc.DependsCountFct = empty;\n"
        "  CKClassDesc copied(desc);\n"
        "  CKClassDesc assigned;\n"
        "  assigned = copied;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKClassDescRegisterFct() { CKClassDesc d; NativePointer p; p += 1; d.RegisterFct = p; }\n"
        "void RejectCKClassDescCreationFct() { CKClassDesc d; NativePointer p; p += 1; d.CreationFct = p; }\n"
        "void RejectCKClassDescNameFct() { CKClassDesc d; NativePointer p; p += 1; d.NameFct = p; }\n"
        "void RejectCKClassDescDependsFct() { CKClassDesc d; NativePointer p; p += 1; d.DependsFct = p; }\n"
        "void RejectCKClassDescDependsCountFct() { CKClassDesc d; NativePointer p; p += 1; d.DependsCountFct = p; }\n"
        "void RejectCKClassDescToNotifyBounds() { CKClassDesc d; d.GetToNotify(0); }\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKClassDesc self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-class-desc-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKClassDesc self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKClassDesc self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKClassDesc()");
    asIScriptFunction *rejectRegister = module->GetFunctionByDecl("void RejectCKClassDescRegisterFct()");
    asIScriptFunction *rejectCreation = module->GetFunctionByDecl("void RejectCKClassDescCreationFct()");
    asIScriptFunction *rejectName = module->GetFunctionByDecl("void RejectCKClassDescNameFct()");
    asIScriptFunction *rejectDepends = module->GetFunctionByDecl("void RejectCKClassDescDependsFct()");
    asIScriptFunction *rejectDependsCount = module->GetFunctionByDecl("void RejectCKClassDescDependsCountFct()");
    asIScriptFunction *toNotifyBounds = module->GetFunctionByDecl("void RejectCKClassDescToNotifyBounds()");
    if (!probe || !rejectRegister || !rejectCreation || !rejectName || !rejectDepends ||
        !rejectDependsCount || !toNotifyBounds) {
        engine->DiscardModule(moduleName);
        error = "CKClassDesc self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKClassDesc value probe", error) &&
              ExecuteCKAttributeDescProbe(engine, rejectRegister, true, "CKClassDesc RegisterFct rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, rejectCreation, true, "CKClassDesc CreationFct rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, rejectName, true, "CKClassDesc NameFct rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, rejectDepends, true, "CKClassDesc DependsFct rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, rejectDependsCount, true, "CKClassDesc DependsCountFct rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, toNotifyBounds, true, "CKClassDesc ToNotify bounds probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKObjectDeclarationScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKObjectDeclaration script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *declType = engine->GetTypeInfoByDecl("CKObjectDeclaration");
    if (!declType) {
        error = "CKObjectDeclaration type is not registered.";
        return false;
    }
    for (asUINT i = 0; i < declType->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        if (declType->GetProperty(i, &propertyName) >= 0 && propertyName && std::strcmp(propertyName, "m_Proto") == 0) {
            error = "CKObjectDeclaration still exposes writable m_Proto.";
            return false;
        }
    }
    if (declType->GetMethodByDecl("void SetProto(CKBehaviorPrototype@ proto)")) {
        error = "CKObjectDeclaration still exposes SetProto.";
        return false;
    }
    if (!declType->GetMethodByDecl("CKBehaviorPrototype@ GetProto()") ||
        !declType->GetMethodByDecl("NativePointer GetCreationFunction()") ||
        !declType->GetMethodByDecl("void SetCreationFunction(NativePointer f)")) {
        error = "CKObjectDeclaration read-only prototype/creation accessors are not registered.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKObjectDeclarationSelfTest";
    const char *source =
        "int ProbeCKObjectDeclaration() {\n"
        "  int count = CKGetPrototypeDeclarationCount();\n"
        "  if (count <= 0) return 0;\n"
        "  CKObjectDeclaration@ decl = CKGetPrototypeDeclaration(0);\n"
        "  if (decl is null) return 1;\n"
        "  NativePointer empty;\n"
        "  decl.SetCreationFunction(empty);\n"
        "  decl.GetCreationFunction();\n"
        "  decl.GetProto();\n"
        "  decl.GetName();\n"
        "  decl.GetDescription();\n"
        "  decl.GetAuthorName();\n"
        "  decl.GetCategory();\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKObjectDeclarationCreationFunction() {\n"
        "  CKObjectDeclaration@ decl = CKGetPrototypeDeclaration(0);\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  decl.SetCreationFunction(ptr);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKObjectDeclaration self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-object-declaration-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKObjectDeclaration self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKObjectDeclaration self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKObjectDeclaration()");
    asIScriptFunction *rejectCreation = module->GetFunctionByDecl("void RejectCKObjectDeclarationCreationFunction()");
    if (!probe || !rejectCreation) {
        engine->DiscardModule(moduleName);
        error = "CKObjectDeclaration self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKObjectDeclaration value probe", error) &&
              ExecuteCKAttributeDescProbe(engine, rejectCreation, true, "CKObjectDeclaration creation-function rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKDependenciesScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKDependencies script self-test requires an AngelScript engine.";
        return false;
    }

    if (!engine->GetGlobalFunctionByDecl("CKDependencies CKGetDefaultClassDependencies(CK_DEPENDENCIES_OPMODE mode)")) {
        error = "CKDependencies value-returning CKGetDefaultClassDependencies declaration is not registered.";
        return false;
    }
    if (engine->GetGlobalFunctionByDecl("CKDependencies &CKGetDefaultClassDependencies(CK_DEPENDENCIES_OPMODE mode)")) {
        error = "CKDependencies still exposes CKGetDefaultClassDependencies as a non-null reference.";
        return false;
    }
    if (!engine->GetGlobalFunctionByDecl("void CKCopyDefaultClassDependencies(CKDependencies &out d, CK_DEPENDENCIES_OPMODE mode)")) {
        error = "CKCopyDefaultClassDependencies declaration is not registered.";
        return false;
    }
    asITypeInfo *iteratorType = engine->GetTypeInfoByDecl("CKDependenciesIt");
    if (!iteratorType) {
        error = "CKDependenciesIt self-test could not find the registered type.";
        return false;
    }
    if (!iteratorType->GetMethodByDecl("bool opEquals(const CKDependenciesIt &in other) const") ||
        !iteratorType->GetMethodByDecl("bool opNotEquals(const CKDependenciesIt &in other) const")) {
        error = "CKDependenciesIt self-test could not find iterator comparison methods.";
        return false;
    }
    asITypeInfo *dependenciesType = engine->GetTypeInfoByDecl("CKDependencies");
    if (!dependenciesType) {
        error = "CKDependenciesIt self-test could not find CKDependencies.";
        return false;
    }
    if (!dependenciesType->GetMethodByDecl("CKDependenciesIt Begin() const") ||
        !dependenciesType->GetMethodByDecl("CKDependenciesIt End() const")) {
        error = "CKDependenciesIt self-test could not find CKDependencies Begin/End producers.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKDependenciesSelfTest";
    const char *source =
        "int ProbeDependenciesDefaultCopy() {\n"
        "  CKDependencies copied;\n"
        "  CKCopyDefaultClassDependencies(copied, CK_DEPENDENCIES_COPY);\n"
        "  if (copied.Size() < 0) return 1;\n"
        "  CKDependencies value = CKGetDefaultClassDependencies(CK_DEPENDENCIES_COPY);\n"
        "  if (value.Size() < 0) return 2;\n"
        "  CKDependencies unsupported = CKGetDefaultClassDependencies(CK_DEPENDENCIES_OPERATIONMODE);\n"
        "  if (unsupported.Size() < 0) return 3;\n"
        "  unsupported.ModifyOptions(CKCID_OBJECT, 0, 0);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeDependenciesIterator() {\n"
        "  CKDependencies deps;\n"
        "  uint first = 13;\n"
        "  uint second = 17;\n"
        "  deps.PushBack(first);\n"
        "  deps.PushBack(second);\n"
        "  CKDependenciesIt begin = deps.Begin();\n"
        "  CKDependenciesIt end = deps.End();\n"
        "  if (!begin.IsValid()) return 1;\n"
        "  if (begin == end) return 2;\n"
        "  if (begin.Get() != first) return 3;\n"
        "  CKDependenciesIt copiedIt(begin);\n"
        "  if (!(copiedIt == begin) || copiedIt != begin) return 4;\n"
        "  ++copiedIt;\n"
        "  if (copiedIt == begin || !(copiedIt != begin)) return 5;\n"
        "  if (copiedIt.Get() != second) return 6;\n"
        "  CKDependenciesIt assignedIt;\n"
        "  assignedIt = copiedIt;\n"
        "  if (!(assignedIt == copiedIt)) return 7;\n"
        "  ++assignedIt;\n"
        "  if (!(assignedIt == end) || assignedIt != end) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKDependencies self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-dependencies-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDependencies self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDependencies self-test script failed to build.";
        return false;
    }

    if (!module->GetFunctionByDecl("int ProbeDependenciesDefaultCopy()")) {
        engine->DiscardModule(moduleName);
        error = "CKDependencies self-test function was not found.";
        return false;
    }
    asIScriptFunction *iteratorProbe = module->GetFunctionByDecl("int ProbeDependenciesIterator()");
    if (!iteratorProbe) {
        engine->DiscardModule(moduleName);
        error = "CKDependencies iterator self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, iteratorProbe, false, "CKDependencies iterator probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKTimeProfilerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKTimeProfiler script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTimeProfilerSelfTest";
    const char *source =
        "int ProbeCKTimeProfiler(CKContext@ ctx) {\n"
        "  CKTimeProfiler profiler(\"ckas-time-profiler\", ctx, -4);\n"
        "  string longName = \"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\";\n"
        "  profiler(longName);\n"
        "  profiler(\"second\");\n"
        "  string dump;\n"
        "  profiler.Dump(dump, \" / \");\n"
        "  if (dump.findFirst(\"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUV\") < 0) return 1;\n"
        "  if (dump.findFirst(\"WXYZ0123456789\") >= 0) return 2;\n"
        "  if (dump.findFirst(\"second\") < 0) return 3;\n"
        "  if (dump.findFirst(\" / \") < 0) return 4;\n"
        "  profiler.Reset();\n"
        "  profiler.Dump(dump);\n"
        "  if (dump.length() != 0) return 5;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKTimeProfilerNullContext() {\n"
        "  CKTimeProfiler profiler(\"bad\", null);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTimeProfiler self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-time-profiler-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTimeProfiler self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTimeProfiler self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKTimeProfiler(CKContext@)");
    asIScriptFunction *nullContext = module->GetFunctionByDecl("int ProbeCKTimeProfilerNullContext()");
    if (!probe || !nullContext) {
        engine->DiscardModule(moduleName);
        error = "CKTimeProfiler self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKTimeProfiler probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, nullContext, true, "CKTimeProfiler null-context probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKAttributeManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKAttributeManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAttributeManagerSelfTest";
    const char *source =
        "int ProbeAttributeManager(CKContext@ ctx) {\n"
        "  if (ctx is null) return 1;\n"
        "  CKAttributeManager@ am = ctx.GetAttributeManager();\n"
        "  if (am is null) return 2;\n"
        "  if (am.GetAttributeCount() < 0) return 3;\n"
        "  if (am.GetCategoriesCount() < 0) return 4;\n"
        "  am.GetName();\n"
        "  am.GetGuid();\n"
        "  am.IsAttributeIndexValid(-2147483647);\n"
        "  am.IsCategoryIndexValid(-2147483647);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeManagerCallbackReject(CKContext@ ctx) {\n"
        "  CKAttributeManager@ am = ctx.GetAttributeManager();\n"
        "  NativePointer ptr;\n"
        "  NativePointer empty;\n"
        "  ptr += 1;\n"
        "  am.SetAttributeCallbackFunction(-2147483647, ptr, empty);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeManagerCallbackArgReject(CKContext@ ctx) {\n"
        "  CKAttributeManager@ am = ctx.GetAttributeManager();\n"
        "  NativePointer ptr;\n"
        "  NativePointer empty;\n"
        "  ptr += 1;\n"
        "  am.SetAttributeCallbackFunction(-2147483647, empty, ptr);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAttributeManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-attribute-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeAttributeManager(CKContext@)");
    asIScriptFunction *callbackReject = module->GetFunctionByDecl("int ProbeAttributeManagerCallbackReject(CKContext@)");
    asIScriptFunction *callbackArgReject = module->GetFunctionByDecl("int ProbeAttributeManagerCallbackArgReject(CKContext@)");
    if (!probe || !callbackReject || !callbackArgReject) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeManager self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKAttributeManager value probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, callbackReject, context, true, "CKAttributeManager callback rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, callbackArgReject, context, true, "CKAttributeManager callback argument rejection probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBaseManagerCastScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBaseManager cast self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *baseManagerType = engine->GetTypeInfoByDecl("CKBaseManager");
    if (!baseManagerType) {
        error = "CKBaseManager self-test could not find the registered type.";
        return false;
    }
    if (baseManagerType->GetMethodByDecl("CKGUID GetGuid()") == nullptr ||
        baseManagerType->GetMethodByDecl("string GetName()") == nullptr ||
        baseManagerType->GetMethodByDecl("CKStateChunk@ SaveData(CKFile@ savedFile)") == nullptr ||
        baseManagerType->GetMethodByDecl("CKDWORD GetValidFunctionsMask()") == nullptr) {
        error = "CKBaseManager self-test could not find expected non-const manager methods.";
        return false;
    }
    if (baseManagerType->GetMethodByDecl("CKGUID GetGuid() const") != nullptr ||
        baseManagerType->GetMethodByDecl("string GetName() const") != nullptr ||
        baseManagerType->GetMethodByDecl("CKStateChunk@ SaveData(CKFile@ savedFile) const") != nullptr ||
        baseManagerType->GetMethodByDecl("CKDWORD GetValidFunctionsMask() const") != nullptr) {
        error = "CKBaseManager self-test found stale const manager method declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBaseManagerCastSelfTest";
    const char *source =
        "int ProbeBaseManagerCasts(CKContext@ ctx) {\n"
        "  if (ctx is null) return 1;\n"
        "  CKAttributeManager@ attr = ctx.GetAttributeManager();\n"
        "  CKTimeManager@ time = ctx.GetTimeManager();\n"
        "  if (attr is null || time is null) return 2;\n"
        "  CKBaseManager@ attrBase = attr;\n"
        "  CKBaseManager@ timeBase = time;\n"
        "  if (attrBase is null || timeBase is null) return 3;\n"
        "  CKAttributeManager@ attrAgain = cast<CKAttributeManager>(attrBase);\n"
        "  CKTimeManager@ timeAgain = cast<CKTimeManager>(timeBase);\n"
        "  if (attrAgain is null || timeAgain is null) return 4;\n"
        "  if (cast<CKTimeManager>(attrBase) !is null) return 5;\n"
        "  if (cast<CKAttributeManager>(timeBase) !is null) return 6;\n"
        "  if (cast<CKParameterManager>(attrBase) !is null) return 7;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBaseManager cast self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-base-manager-cast-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBaseManager cast self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBaseManager cast self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeBaseManagerCasts(CKContext@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBaseManager cast self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKBaseManager checked-cast probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKTimeManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKTimeManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *timeManagerType = engine->GetTypeInfoByDecl("CKTimeManager");
    if (!timeManagerType) {
        error = "CKTimeManager self-test could not find the registered type.";
        return false;
    }
    if (timeManagerType->GetMethodByDecl("uint GetMainTickCount()") == nullptr ||
        timeManagerType->GetMethodByDecl("float GetTime()") == nullptr ||
        timeManagerType->GetMethodByDecl("float GetLastDeltaTime()") == nullptr ||
        timeManagerType->GetMethodByDecl("float GetLastDeltaTimeFree()") == nullptr ||
        timeManagerType->GetMethodByDecl("float GetAbsoluteTime()") == nullptr ||
        timeManagerType->GetMethodByDecl("void GetTimeToWaitForLimits(float &out timeBeforeRender, float &out timeBeforeBeh)") == nullptr ||
        timeManagerType->GetMethodByDecl("void ResetChronos(bool renderChrono, bool behavioralChrono)") == nullptr) {
        error = "CKTimeManager self-test could not find expected time manager methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTimeManagerSelfTest";
    const char *source =
        "int ProbeTimeManager(CKContext@ ctx) {\n"
        "  CKTimeManager@ time = ctx.GetTimeManager();\n"
        "  if (time is null) return 1;\n"
        "  CKBaseManager@ base = time;\n"
        "  if (base is null) return 2;\n"
        "  if (cast<CKTimeManager>(base) is null) return 3;\n"
        "  uint tick = time.GetMainTickCount();\n"
        "  float current = time.GetTime();\n"
        "  float absolute = time.GetAbsoluteTime();\n"
        "  float delta = time.GetLastDeltaTime();\n"
        "  float freeDelta = time.GetLastDeltaTimeFree();\n"
        "  float renderWait = 0.0f;\n"
        "  float behaviorWait = 0.0f;\n"
        "  time.GetTimeToWaitForLimits(renderWait, behaviorWait);\n"
        "  time.ResetChronos(false, false);\n"
        "  if (time.GetTimeScaleFactor() <= 0.0f) return 4;\n"
        "  if (time.GetMinimumDeltaTime() < 0.0f) return 5;\n"
        "  if (time.GetMaximumDeltaTime() < 0.0f) return 6;\n"
        "  if (time.GetFrameRateLimit() < 0.0f) return 7;\n"
        "  if (time.GetBehavioralRateLimit() < 0.0f) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTimeManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-time-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTimeManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTimeManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeTimeManager(CKContext@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKTimeManager self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKTimeManager accessors probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBehaviorManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBehaviorManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *behaviorManagerType = engine->GetTypeInfoByDecl("CKBehaviorManager");
    if (!behaviorManagerType) {
        error = "CKBehaviorManager self-test could not find the registered type.";
        return false;
    }
    if (behaviorManagerType->GetMethodByDecl("CKERROR Execute(float delta)") == nullptr ||
        behaviorManagerType->GetMethodByDecl("int GetObjectsCount()") == nullptr ||
        behaviorManagerType->GetMethodByDecl("CKBeObject@ GetObject(int pos)") == nullptr ||
        behaviorManagerType->GetMethodByDecl("int GetBehaviorMaxIteration()") == nullptr ||
        behaviorManagerType->GetMethodByDecl("void SetBehaviorMaxIteration(int n)") == nullptr) {
        error = "CKBehaviorManager self-test could not find expected behavior manager methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBehaviorManagerSelfTest";
    const char *source =
        "int ProbeBehaviorManager(CKContext@ ctx) {\n"
        "  CKBehaviorManager@ manager = ctx.GetBehaviorManager();\n"
        "  if (manager is null) return 1;\n"
        "  CKBaseManager@ base = manager;\n"
        "  if (base is null) return 2;\n"
        "  if (cast<CKBehaviorManager>(base) is null) return 3;\n"
        "  int objectCount = manager.GetObjectsCount();\n"
        "  if (objectCount < 0) return 4;\n"
        "  int maxIteration = manager.GetBehaviorMaxIteration();\n"
        "  if (maxIteration < 0) return 5;\n"
        "  manager.SetBehaviorMaxIteration(maxIteration);\n"
        "  if (objectCount > 0) {\n"
        "    CKBeObject@ object = manager.GetObject(0);\n"
        "    if (object is null) return 6;\n"
        "  }\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBehaviorManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-behavior-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeBehaviorManager(CKContext@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorManager self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKBehaviorManager accessors probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKMessageManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKMessageManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *messageManagerType = engine->GetTypeInfoByDecl("CKMessageManager");
    if (!messageManagerType) {
        error = "CKMessageManager self-test could not find the registered type.";
        return false;
    }
    if (messageManagerType->GetMethodByDecl("CKMessageType AddMessageType(const string &in msgName)") == nullptr ||
        messageManagerType->GetMethodByDecl("string GetMessageTypeName(CKMessageType msgType)") == nullptr ||
        messageManagerType->GetMethodByDecl("int GetMessageTypeCount()") == nullptr ||
        messageManagerType->GetMethodByDecl("CKMessage@ SendMessageSingle(int MsgType, CKBeObject@ dest, CKBeObject@ sender = null)") == nullptr ||
        messageManagerType->GetMethodByDecl("CKERROR RegisterWait(CKMessageType msgType, CKBehavior@ beh, int outputToActivate, CKBeObject@ obj)") == nullptr ||
        messageManagerType->GetMethodByDecl("CKERROR UnRegisterWait(CKMessageType msgType, CKBehavior@ beh, int OutputToActivate)") == nullptr) {
        error = "CKMessageManager self-test could not find expected message manager methods.";
        return false;
    }

    asITypeInfo *waitingObjectType = engine->GetTypeInfoByDecl("CKWaitingObject");
    if (!waitingObjectType) {
        error = "CKMessageManager self-test could not find CKWaitingObject.";
        return false;
    }
    if (waitingObjectType->GetPropertyCount() != 0) {
        error = "CKWaitingObject self-test found exposed raw handle properties.";
        return false;
    }
    if (waitingObjectType->GetMethodByDecl("bool HasBeObject() const") == nullptr ||
        waitingObjectType->GetMethodByDecl("bool HasBehavior() const") == nullptr ||
        waitingObjectType->GetMethodByDecl("bool HasOutput() const") == nullptr ||
        waitingObjectType->GetMethodByDecl("CK_ID BeObjectId() const") == nullptr ||
        waitingObjectType->GetMethodByDecl("CK_ID BehaviorId() const") == nullptr ||
        waitingObjectType->GetMethodByDecl("CK_ID OutputId() const") == nullptr) {
        error = "CKWaitingObject self-test could not find expected safe accessors.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMessageManagerSelfTest";
    const char *source =
        "int ProbeMessageManager(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  if (manager is null) return 1;\n"
        "  CKBaseManager@ base = manager;\n"
        "  if (base is null) return 2;\n"
        "  if (cast<CKMessageManager>(base) is null) return 3;\n"
        "  if (manager.GetMessageTypeCount() < 0) return 4;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMessageManagerNullSingle(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  manager.SendMessageSingle(0, null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMessageManagerNullGroup(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  manager.SendMessageGroup(0, null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMessageManagerNullWait(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  manager.RegisterWait(0, null, 0, null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMessageManagerNullUnwait(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  manager.UnRegisterWait(0, null, 0);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeWaitingObject(CKContext@ ctx) {\n"
        "  if (ctx is null) return 9;\n"
        "  CKWaitingObject waiting;\n"
        "  if (waiting.HasBeObject() || waiting.HasBehavior() || waiting.HasOutput()) return 1;\n"
        "  if (waiting.BeObjectId() != 0 || waiting.BehaviorId() != 0 || waiting.OutputId() != 0) return 2;\n"
        "  CKWaitingObject copied(waiting);\n"
        "  if (copied.HasBeObject() || copied.HasBehavior() || copied.HasOutput()) return 3;\n"
        "  CKWaitingObject assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.BeObjectId() != 0 || assigned.BehaviorId() != 0 || assigned.OutputId() != 0) return 4;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMessageManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-message-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMessageManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMessageManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeMessageManager(CKContext@)");
    asIScriptFunction *nullSingle = module->GetFunctionByDecl("int ProbeMessageManagerNullSingle(CKContext@)");
    asIScriptFunction *nullGroup = module->GetFunctionByDecl("int ProbeMessageManagerNullGroup(CKContext@)");
    asIScriptFunction *nullWait = module->GetFunctionByDecl("int ProbeMessageManagerNullWait(CKContext@)");
    asIScriptFunction *nullUnwait = module->GetFunctionByDecl("int ProbeMessageManagerNullUnwait(CKContext@)");
    asIScriptFunction *waitingProbe = module->GetFunctionByDecl("int ProbeWaitingObject(CKContext@)");
    if (!probe || !nullSingle || !nullGroup || !nullWait || !nullUnwait || !waitingProbe) {
        engine->DiscardModule(moduleName);
        error = "CKMessageManager self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKMessageManager accessors probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, waitingProbe, context, false, "CKWaitingObject safe accessors probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, nullSingle, context, true, "CKMessageManager null single-message probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, nullGroup, context, true, "CKMessageManager null group-message probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, nullWait, context, true, "CKMessageManager null wait probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, nullUnwait, context, true, "CKMessageManager null unregister-wait probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKGridManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKGridManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *gridManagerType = engine->GetTypeInfoByDecl("CKGridManager");
    if (!gridManagerType) {
        error = "CKGridManager self-test could not find the registered type.";
        return false;
    }
    if (gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int layerType, int &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int layerType, float &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int layerType, const CKSquare &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int solidLayerType, int shapeLayerType, int &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int solidLayerType, int shapeLayerType, float &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int solidLayerType, int shapeLayerType, const CKSquare &in fillVal)") == nullptr) {
        error = "CKGridManager self-test could not find expected typed FillGridWithObjectShape overloads.";
        return false;
    }

    return true;
}

bool RunCKFloorManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKFloorManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *floorManagerType = engine->GetTypeInfoByDecl("CKFloorManager");
    if (!floorManagerType) {
        error = "CKFloorManager self-test could not find the registered type.";
        return false;
    }
    if (floorManagerType->GetMethodByDecl("bool ReadAttributeValues(CK3dEntity@ ent, CKDWORD &out geo = void, bool &out moving = void, int &out type = void, bool &out hiera = void, bool &out first = void)") == nullptr) {
        error = "CKFloorManager self-test could not find guarded ReadAttributeValues declaration.";
        return false;
    }

    return true;
}

bool RunCKInterfaceManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKInterfaceManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *interfaceManagerType = engine->GetTypeInfoByDecl("CKInterfaceManager");
    if (!interfaceManagerType) {
        error = "CKInterfaceManager self-test could not find the registered type.";
        return false;
    }
    if (interfaceManagerType->GetMethodByDecl("int DoRenameDialog(string &inout name, CK_CLASSID cid)") == nullptr) {
        error = "CKInterfaceManager self-test could not find mutable DoRenameDialog declaration.";
        return false;
    }
    if (interfaceManagerType->GetMethodByDecl("int DoRenameDialog(const string &in name, CK_CLASSID cid)") != nullptr) {
        error = "CKInterfaceManager self-test found stale const DoRenameDialog declaration.";
        return false;
    }

    return true;
}

bool RunCKMidiManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKMidiManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *midiManagerType = engine->GetTypeInfoByDecl("CKMidiManager");
    if (!midiManagerType) {
        error = "CKMidiManager self-test could not find the registered type.";
        return false;
    }
    if (midiManagerType->GetMethodByDecl("NativePointer Create(NativePointer hwnd)") == nullptr ||
        midiManagerType->GetMethodByDecl("void Release(NativePointer source)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKERROR SetSoundFileName(NativePointer source, const string &in filename)") == nullptr ||
        midiManagerType->GetMethodByDecl("string GetSoundFileName(NativePointer source)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKERROR Play(NativePointer source)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKERROR Pause(NativePointer source, bool pause = true)") == nullptr ||
        midiManagerType->GetMethodByDecl("bool IsPlaying(NativePointer source)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKERROR Time(NativePointer source, CKDWORD &out ticks)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKDWORD MillisecsToTicks(NativePointer source, CKDWORD msOffset)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKDWORD TicksToMillisecs(NativePointer source, CKDWORD tkOffset)") == nullptr) {
        error = "CKMidiManager self-test could not find expected guarded NativePointer methods.";
        return false;
    }

    return true;
}

bool RunCKSoundManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKSoundManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *soundManagerType = engine->GetTypeInfoByDecl("CKSoundManager");
    if (!soundManagerType) {
        error = "CKSoundManager self-test could not find the registered type.";
        return false;
    }
    if (soundManagerType->GetMethodByDecl("void Play(CKWaveSound@ sound, NativePointer source, bool loop)") == nullptr ||
        soundManagerType->GetMethodByDecl("void Pause(CKWaveSound@ sound, NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("void Stop(CKWaveSound@ sound, NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("NativePointer CreateSource(CK_WAVESOUND_TYPE flags, CKWaveFormat &in wf, CKDWORD bytes, bool streamed)") == nullptr ||
        soundManagerType->GetMethodByDecl("NativePointer DuplicateSource(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("void ReleaseSource(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("void SetPlayPosition(NativePointer source, int pos)") == nullptr ||
        soundManagerType->GetMethodByDecl("int GetPlayPosition(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("bool IsPlaying(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("CKERROR SetWaveFormat(NativePointer source, CKWaveFormat &in wf)") == nullptr ||
        soundManagerType->GetMethodByDecl("CKERROR GetWaveFormat(NativePointer source, CKWaveFormat &out wf)") == nullptr ||
        soundManagerType->GetMethodByDecl("int GetWaveSize(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("CKERROR Lock(NativePointer source, CKDWORD writeCursor, CKDWORD numBytes, NativePointer &out audioPtr1, CKDWORD &out audioBytes1, NativePointer &out audioPtr2, CKDWORD &out audioBytes2, CK_WAVESOUND_LOCKMODE flags)") == nullptr ||
        soundManagerType->GetMethodByDecl("CKERROR Unlock(NativePointer source, NativePointer audioPtr1, CKDWORD numBytes1, NativePointer audioPtr2, CKDWORD audioBytes2)") == nullptr ||
        soundManagerType->GetMethodByDecl("void SetType(NativePointer source, CK_WAVESOUND_TYPE type)") == nullptr ||
        soundManagerType->GetMethodByDecl("CK_WAVESOUND_TYPE GetType(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("void UpdateSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSoundSettings &inout settings, bool set = true)") == nullptr ||
        soundManagerType->GetMethodByDecl("void Update3DSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSound3DSettings &inout settings, bool set = true)") == nullptr ||
        soundManagerType->GetMethodByDecl("void UpdateListenerSettings(CK_SOUNDMANAGER_CAPS settingsoptions, CKListenerSettings &inout settings, bool set = true)") == nullptr) {
        error = "CKSoundManager self-test could not find expected guarded source/playback/settings declarations.";
        return false;
    }
    if (soundManagerType->GetMethodByDecl("void UpdateSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSoundSettings &out settings, bool set = true)") != nullptr ||
        soundManagerType->GetMethodByDecl("void Update3DSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSound3DSettings &out settings, bool set = true)") != nullptr ||
        soundManagerType->GetMethodByDecl("void UpdateListenerSettings(CK_SOUNDMANAGER_CAPS settingsoptions, CKListenerSettings &out settings, bool set = true)") != nullptr) {
        error = "CKSoundManager self-test found stale out-only settings declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKSoundManagerSelfTest";
    const char *source =
        "void ProbeCKSoundManagerSource(CKSoundManager@ manager, CKWaveFormat &in wf, CKWaveFormat &out outWf, CKWaveSoundSettings &inout settings, CKWaveSound3DSettings &inout settings3d, CKListenerSettings &inout listener) {\n"
        "  if (manager is null) return;\n"
        "  NativePointer source;\n"
        "  NativePointer created = manager.CreateSource(CK_WAVESOUND_BACKGROUND, wf, 0, false);\n"
        "  NativePointer duplicate = manager.DuplicateSource(source);\n"
        "  manager.ReleaseSource(source);\n"
        "  manager.SetPlayPosition(source, 0);\n"
        "  int pos = manager.GetPlayPosition(source);\n"
        "  bool playing = manager.IsPlaying(source);\n"
        "  manager.SetWaveFormat(source, wf);\n"
        "  manager.GetWaveFormat(source, outWf);\n"
        "  int waveSize = manager.GetWaveSize(source);\n"
        "  NativePointer audioPtr1;\n"
        "  NativePointer audioPtr2;\n"
        "  CKDWORD audioBytes1 = 0;\n"
        "  CKDWORD audioBytes2 = 0;\n"
        "  manager.Lock(source, 0, 0, audioPtr1, audioBytes1, audioPtr2, audioBytes2, CK_WAVESOUND_LOCKFROMWRITE);\n"
        "  manager.Unlock(source, audioPtr1, audioBytes1, audioPtr2, audioBytes2);\n"
        "  manager.SetType(source, CK_WAVESOUND_BACKGROUND);\n"
        "  CK_WAVESOUND_TYPE type = manager.GetType(source);\n"
        "  manager.UpdateSettings(source, CK_WAVESOUND_SETTINGS_GAIN, settings, false);\n"
        "  manager.UpdateSettings(source, CK_WAVESOUND_SETTINGS_GAIN, settings, true);\n"
        "  manager.Update3DSettings(source, CK_WAVESOUND_3DSETTINGS_POSITION, settings3d, false);\n"
        "  manager.Update3DSettings(source, CK_WAVESOUND_3DSETTINGS_POSITION, settings3d, true);\n"
        "  manager.UpdateListenerSettings(CK_WAVESOUND_3DSETTINGS_POSITION, listener, false);\n"
        "  manager.UpdateListenerSettings(CK_WAVESOUND_3DSETTINGS_POSITION, listener, true);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKSoundManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-sound-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSoundManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSoundManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKSoundManagerSource(CKSoundManager@, CKWaveFormat &in, CKWaveFormat &out, CKWaveSoundSettings &inout, CKWaveSound3DSettings &inout, CKListenerSettings &inout)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKSoundManager self-test function was not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKListenerSettingsScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKListenerSettings script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *settingsType = engine->GetTypeInfoByDecl("CKListenerSettings");
    if (!settingsType) {
        error = "CKListenerSettings self-test could not find the registered type.";
        return false;
    }
    if (settingsType->GetPropertyCount() != 6 ||
        settingsType->GetMethodByDecl("CKListenerSettings &opAssign(const CKListenerSettings &in other)") == nullptr) {
        error = "CKListenerSettings self-test found an unexpected value surface.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKListenerSettingsSelfTest";
    const char *source =
        "bool Same(float lhs, float rhs) {\n"
        "  return lhs > rhs - 0.0001f && lhs < rhs + 0.0001f;\n"
        "}\n"
        "int ProbeCKListenerSettings() {\n"
        "  CKListenerSettings settings;\n"
        "  if (!Same(settings.m_DistanceFactor, 1.0f)) return 1;\n"
        "  if (!Same(settings.m_DopplerFactor, 1.0f)) return 2;\n"
        "  if (!Same(settings.m_RollOff, 1.0f)) return 3;\n"
        "  if (!Same(settings.m_GlobalGain, 1.0f)) return 4;\n"
        "  if (!Same(settings.m_PriorityBias, 1.0f)) return 5;\n"
        "  if (settings.m_SoftwareSources != 16) return 6;\n"
        "  settings.m_DistanceFactor = 2.0f;\n"
        "  settings.m_DopplerFactor = 3.0f;\n"
        "  settings.m_RollOff = 4.0f;\n"
        "  settings.m_GlobalGain = 5.0f;\n"
        "  settings.m_PriorityBias = 6.0f;\n"
        "  settings.m_SoftwareSources = 7;\n"
        "  CKListenerSettings copied(settings);\n"
        "  if (!Same(copied.m_DistanceFactor, 2.0f)) return 7;\n"
        "  if (!Same(copied.m_DopplerFactor, 3.0f)) return 8;\n"
        "  if (!Same(copied.m_RollOff, 4.0f)) return 9;\n"
        "  if (!Same(copied.m_GlobalGain, 5.0f)) return 10;\n"
        "  if (!Same(copied.m_PriorityBias, 6.0f)) return 11;\n"
        "  if (copied.m_SoftwareSources != 7) return 12;\n"
        "  CKListenerSettings assigned;\n"
        "  assigned = copied;\n"
        "  if (!Same(assigned.m_DistanceFactor, 2.0f)) return 14;\n"
        "  if (!Same(assigned.m_DopplerFactor, 3.0f)) return 15;\n"
        "  if (!Same(assigned.m_RollOff, 4.0f)) return 16;\n"
        "  if (!Same(assigned.m_GlobalGain, 5.0f)) return 17;\n"
        "  if (!Same(assigned.m_PriorityBias, 6.0f)) return 18;\n"
        "  if (assigned.m_SoftwareSources != 7) return 19;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKListenerSettings self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-listener-settings-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKListenerSettings self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKListenerSettings self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKListenerSettings()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKListenerSettings self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteNoArgIntProbe(engine, probe, false, "CKListenerSettings value probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKWaveSoundScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKWaveSound script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *waveSoundType = engine->GetTypeInfoByDecl("CKWaveSound");
    if (!waveSoundType) {
        error = "CKWaveSound type is not registered.";
        return false;
    }

    if (waveSoundType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr) {
        error = "CKWaveSound self-test found unsafe ApplyPatchForOlderVersion array-pointer binding.";
        return false;
    }
    if (waveSoundType->GetMethodByDecl("CKERROR WriteData(NativePointer buffer, int size)") == nullptr ||
        waveSoundType->GetMethodByDecl("CKERROR WriteData(NativeBuffer@ buffer)") == nullptr ||
        waveSoundType->GetMethodByDecl("CKERROR Lock(CKDWORD writeCursor, CKDWORD numBytes, NativePointer &out ptr1, CKDWORD &out bytes1, NativePointer &out ptr2, CKDWORD &out bytes2, CK_WAVESOUND_LOCKMODE flags)") == nullptr ||
        waveSoundType->GetMethodByDecl("CKERROR Unlock(NativePointer ptr1, CKDWORD bytes1, NativePointer ptr2, CKDWORD bytes2)") == nullptr ||
        waveSoundType->GetMethodByDecl("CKSoundReader@ GetReader()") == nullptr ||
        waveSoundType->GetMethodByDecl("CK3dEntity@ GetAttachedEntity()") == nullptr ||
        waveSoundType->GetMethodByDecl("NativePointer GetAppData()") == nullptr ||
        waveSoundType->GetMethodByDecl("void SetAppData(NativePointer data)") == nullptr) {
        error = "CKWaveSound self-test could not find expected guarded audio-buffer and borrowed-handle methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKWaveSoundSelfTest";
    const char *source =
        "int ProbeCKWaveSoundAudio(CKWaveSound@ sound) {\n"
        "  if (sound is null) return 2;\n"
        "  if (sound.GetClassID() != CKCID_WAVESOUND) return 3;\n"
        "  if (sound.GetType() != CK_WAVESOUND_BACKGROUND) return 4;\n"
        "  if (sound.GetReader() !is null) return 5;\n"
        "  if (sound.GetAttachedEntity() !is null) return 6;\n"
        "  NativePointer appData = sound.GetAppData();\n"
        "  if (!appData.IsNull()) return 7;\n"
        "  sound.SetAppData(appData);\n"
        "  CKWaveFormat format;\n"
        "  if (sound.GetSoundFormat(format) != CK_OK) return 8;\n"
        "  if (format.wFormatTag != 1 || format.nChannels != 1 || format.wBitsPerSample != 16) return 9;\n"
        "  sound.WriteData(NativePointer(), 0);\n"
        "  NativeBuffer@ audio = NativeBuffer(4);\n"
        "  audio.WriteShort(0);\n"
        "  audio.WriteShort(0);\n"
        "  sound.WriteData(audio);\n"
        "  NativePointer ptr1;\n"
        "  NativePointer ptr2;\n"
        "  CKDWORD bytes1 = 0;\n"
        "  CKDWORD bytes2 = 0;\n"
        "  if (sound.Lock(0, 4, ptr1, bytes1, ptr2, bytes2, CK_WAVESOUND_LOCKFROMWRITE) != CK_OK) return 11;\n"
        "  if (bytes1 + bytes2 != 4) return 12;\n"
        "  if (bytes1 > 0 && ptr1.IsNull()) return 13;\n"
        "  if (bytes2 > 0 && ptr2.IsNull()) return 14;\n"
        "  if (sound.Unlock(ptr1, bytes1, ptr2, bytes2) != CK_OK) return 15;\n"
        "  if (sound.CKGetObject(sound.GetID()) !is sound) return 16;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKWaveSound self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-wave-sound-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKWaveSound self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKWaveSound self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKWaveSoundAudio(CKWaveSound@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKWaveSound self-test function was not found.";
        return false;
    }

    CKWaveSound *sound = CKWaveSound::Cast(context->CreateObject(CKCID_WAVESOUND,
                                                                  const_cast<CKSTRING>("__CKAS_CKWaveSoundSelfTest"),
                                                                  CK_OBJECTCREATION_DYNAMIC));
    if (!sound) {
        engine->DiscardModule(moduleName);
        error = "CKWaveSound self-test could not create a temporary CKWaveSound.";
        return false;
    }

    CKWaveFormat format = {};
    format.wFormatTag = CKWAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = 8000;
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<CKWORD>((format.nChannels * format.wBitsPerSample) / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    const CKERROR createErr = sound->Create(CK_WAVESOUND_BACKGROUND, &format, 4);
    if (createErr != CK_OK) {
        context->DestroyObject(sound);
        engine->DiscardModule(moduleName);
        error = "CKWaveSound self-test could not create a temporary PCM source.";
        return false;
    }

    const bool ok = ExecuteCKWaveSoundProbe(engine, probe, sound, false, "CKWaveSound audio surface probe", error);

    context->DestroyObject(sound);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKContextScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKContext script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *contextType = engine->GetTypeInfoByDecl("CKContext");
    if (!contextType) {
        error = "CKContext self-test could not find the registered type.";
        return false;
    }
    if (contextType->GetMethodByDecl("CKObject@ CreateObject(CK_CLASSID cid, const string &in name = void, CK_OBJECTCREATION_OPTIONS options = CK_OBJECTCREATION_NONAMECHECK, CK_LOADMODE &out res = void)") == nullptr ||
        contextType->GetMethodByDecl("CKERROR DestroyObject(CKObject@ obj, CKDWORD flags = 0, CKDependencies &in depoptions = void)") == nullptr ||
        contextType->GetMethodByDecl("CKObject@ GetObject(CK_ID id)") == nullptr ||
        contextType->GetMethodByDecl("CKERROR Load(int size, NativePointer buffer, CKObjectArray@ objects, CK_LOAD_FLAGS loadFlags = CK_LOAD_DEFAULT)") == nullptr ||
        contextType->GetMethodByDecl("CKERROR Load(NativeBuffer@ buffer, CKObjectArray@ objects, CK_LOAD_FLAGS loadFlags = CK_LOAD_DEFAULT)") == nullptr ||
        contextType->GetMethodByDecl("CKERROR GetFileInfo(int size, NativePointer buffer, CKFileInfo &out fileInfo)") == nullptr ||
        contextType->GetMethodByDecl("CKERROR GetFileInfo(NativeBuffer@ buffer, CKFileInfo &out fileInfo)") == nullptr ||
        contextType->GetMethodByDecl("CKERROR LoadAnimationOnCharacter(int size, NativePointer buffer, CKObjectArray@ objects, CKCharacter@ carac, bool asDynamicObjects = false)") == nullptr ||
        contextType->GetMethodByDecl("CKERROR LoadAnimationOnCharacter(NativeBuffer@ buffer, CKObjectArray@ objects, CKCharacter@ carac, bool asDynamicObjects = false)") == nullptr) {
        error = "CKContext self-test could not find expected object and guarded buffer methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKContextSelfTest";
    const char *source =
        "int ProbeCKContextSurface(CKContext@ ctx) {\n"
        "  if (ctx is null) return 2;\n"
        "  int beforeCount = ctx.GetObjectCount();\n"
        "  CK_LOADMODE mode = CKLOAD_INVALID;\n"
        "  CKObject@ obj = ctx.CreateObject(CKCID_BEOBJECT, \"__CKAS_CKContextSelfTest\", CK_OBJECTCREATION_DYNAMIC, mode);\n"
        "  if (obj is null) return 3;\n"
        "  if (obj.GetCKContext() !is ctx) return 4;\n"
        "  if (obj.GetClassID() != CKCID_BEOBJECT) return 5;\n"
        "  if (mode == CKLOAD_INVALID) return 6;\n"
        "  CK_ID id = obj.GetID();\n"
        "  if (ctx.GetObject(id) !is obj) return 7;\n"
        "  if (ctx.GetObjectByName(\"__CKAS_CKContextSelfTest\") !is obj) return 8;\n"
        "  if (ctx.GetObjectByNameAndClass(\"__CKAS_CKContextSelfTest\", CKCID_BEOBJECT) !is obj) return 9;\n"
        "  if (ctx.GetObjectByNameAndParentClass(\"__CKAS_CKContextSelfTest\", CKCID_OBJECT, null) !is obj) return 10;\n"
        "  if (ctx.GetObjectSize(obj) < 0) return 11;\n"
        "  if (ctx.GetObjectsCountByClassID(CKCID_BEOBJECT) <= 0) return 12;\n"
        "  if (ctx.GetRenderManager() is null || ctx.GetParameterManager() is null) return 13;\n"
        "  if (ctx.GetManagerCount() <= 0) return 14;\n"
        "  CKDependencies deps;\n"
        "  if (ctx.DestroyObject(obj, 0, deps) != CK_OK) return 15;\n"
        "  if (ctx.GetObject(id) !is null) return 16;\n"
        "  if (ctx.GetObjectCount() > beforeCount + 1) return 17;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKContextNativeBufferLoaders(CKContext@ ctx) {\n"
        "  NativeBuffer@ bytes = NativeBuffer(1);\n"
        "  bytes.WriteUChar(0);\n"
        "  if (ctx.Load(bytes, null) == CK_OK) return 20;\n"
        "  CKFileInfo info;\n"
        "  if (ctx.GetFileInfo(bytes, info) == CK_OK) return 21;\n"
        "  if (ctx.LoadAnimationOnCharacter(bytes, null, null) == CK_OK) return 22;\n"
        "  return 0;\n"
        "}\n"
        "void ProbeCKContextLoadNullBuffer(CKContext@ ctx) {\n"
        "  ctx.Load(1, NativePointer(), null);\n"
        "}\n"
        "void ProbeCKContextFileInfoNullBuffer(CKContext@ ctx) {\n"
        "  CKFileInfo info;\n"
        "  ctx.GetFileInfo(1, NativePointer(), info);\n"
        "}\n"
        "void ProbeCKContextAnimationNullBuffer(CKContext@ ctx) {\n"
        "  ctx.LoadAnimationOnCharacter(1, NativePointer(), null, null);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKContext self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckcontext-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKContext self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKContext self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKContextSurface(CKContext@)");
    asIScriptFunction *bufferLoaders = module->GetFunctionByDecl("int ProbeCKContextNativeBufferLoaders(CKContext@)");
    asIScriptFunction *loadNull = module->GetFunctionByDecl("void ProbeCKContextLoadNullBuffer(CKContext@)");
    asIScriptFunction *fileInfoNull = module->GetFunctionByDecl("void ProbeCKContextFileInfoNullBuffer(CKContext@)");
    asIScriptFunction *animationNull = module->GetFunctionByDecl("void ProbeCKContextAnimationNullBuffer(CKContext@)");
    if (!probe || !bufferLoaders || !loadNull || !fileInfoNull || !animationNull) {
        engine->DiscardModule(moduleName);
        error = "CKContext self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKContext surface probe", error) &&
                    ExecuteCKParameterTypeDescProbe(engine, bufferLoaders, context, false, "CKContext NativeBuffer loader probe", error) &&
                    ExecuteCKParameterTypeDescProbe(engine, loadNull, context, true, "CKContext Load null-buffer probe", error) &&
                    ExecuteCKParameterTypeDescProbe(engine, fileInfoNull, context, true, "CKContext GetFileInfo null-buffer probe", error) &&
                    ExecuteCKParameterTypeDescProbe(engine, animationNull, context, true, "CKContext LoadAnimationOnCharacter null-buffer probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKObjectManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKObjectManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *objectManagerType = engine->GetTypeInfoByDecl("CKObjectManager");
    if (!objectManagerType) {
        error = "CKObjectManager self-test could not find the registered type.";
        return false;
    }
    if (objectManagerType->GetMethodByDecl("CKObject@ GetObject(CK_ID id)") != nullptr) {
        error = "CKObjectManager self-test found the removed GetObject(CK_ID) alias.";
        return false;
    }
    if (objectManagerType->GetMethodByDecl("CKObject@ CKGetObject(CK_ID id)") == nullptr ||
        objectManagerType->GetMethodByDecl("int GetObjectsCount()") == nullptr) {
        error = "CKObjectManager self-test could not find expected object manager methods.";
        return false;
    }

    return true;
}

bool RunCKObjectScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKObject script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *objectType = engine->GetTypeInfoByDecl("CKObject");
    if (!objectType) {
        error = "CKObject self-test could not find the registered type.";
        return false;
    }
    if (!objectType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext &in context)") ||
        !objectType->GetMethodByDecl("NativePointer GetAppData()") ||
        !objectType->GetMethodByDecl("void SetAppData(NativePointer data)") ||
        !objectType->GetMethodByDecl("CKObject@ CKGetObject(CK_ID id)") ||
        !objectType->GetMethodByDecl("CKBeObject@ opCast()")) {
        error = "CKObject self-test could not find expected object methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKObjectSelfTest";
    const char *source =
        "int ProbeCKObjectSurface(CKContext@ ctx, CKObject@ obj) {\n"
        "  if (ctx is null || obj is null) return 2;\n"
        "  CK_ID id = obj.GetID();\n"
        "  if (id == 0) return 3;\n"
        "  if (obj.GetCKContext() !is ctx) return 4;\n"
        "  if (obj.CKGetObject(id) !is obj) return 5;\n"
        "  obj.SetName(\"__CKAS_CKObjectSelfTest\");\n"
        "  if (obj.GetName() != \"__CKAS_CKObjectSelfTest\") return 6;\n"
        "  NativePointer appData = obj.GetAppData();\n"
        "  if (!appData.IsNull()) return 7;\n"
        "  obj.SetAppData(appData);\n"
        "  CKBeObject@ beObject = cast<CKBeObject@>(obj);\n"
        "  if (beObject is null) return 8;\n"
        "  CKObject@ asObject = beObject;\n"
        "  if (asObject !is obj) return 9;\n"
        "  if (obj.GetClassID() != CKCID_BEOBJECT) return 10;\n"
        "  obj.ModifyObjectFlags(0, 0);\n"
        "  obj.IsVisible();\n"
        "  obj.IsDynamic();\n"
        "  obj.GetMemoryOccupation();\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKObject self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckobject-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKObject self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKObject self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKObjectSurface(CKContext@, CKObject@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKObject self-test functions were not found.";
        return false;
    }

    CKObject *object = context->CreateObject(CKCID_BEOBJECT,
                                             const_cast<CKSTRING>("__CKAS_CKObjectSelfTest"),
                                             CK_OBJECTCREATION_DYNAMIC);
    if (!object) {
        engine->DiscardModule(moduleName);
        error = "CKObject self-test could not create a temporary CKBeObject.";
        return false;
    }

    const bool ok = ExecuteCKObjectProbe(engine, probe, context, object, false, "CKObject surface probe", error);

    context->DestroyObject(object);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKSceneObjectScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKSceneObject script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *sceneObjectType = engine->GetTypeInfoByDecl("CKSceneObject");
    if (!sceneObjectType) {
        error = "CKSceneObject self-test could not find the registered type.";
        return false;
    }
    if (!sceneObjectType->GetMethodByDecl("bool IsActiveInScene(CKScene@ scene)") ||
        !sceneObjectType->GetMethodByDecl("bool IsActiveInCurrentScene()") ||
        !sceneObjectType->GetMethodByDecl("bool IsInScene(CKScene@ scene)") ||
        !sceneObjectType->GetMethodByDecl("int GetSceneInCount()") ||
        !sceneObjectType->GetMethodByDecl("CKScene@ GetSceneIn(int index)") ||
        !sceneObjectType->GetMethodByDecl("NativePointer GetAppData()") ||
        !sceneObjectType->GetMethodByDecl("void SetAppData(NativePointer data)") ||
        !sceneObjectType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)")) {
        error = "CKSceneObject self-test could not find expected object methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKSceneObjectSelfTest";
    const char *source =
        "int ProbeCKSceneObjectSurface(CKSceneObject@ object, CKScene@ scene) {\n"
        "  if (object is null || scene is null) return 2;\n"
        "  if (!object.IsInScene(scene)) return 3;\n"
        "  if (object.GetSceneInCount() < 1) return 4;\n"
        "  bool found = false;\n"
        "  for (int i = 0; i < object.GetSceneInCount(); ++i) {\n"
        "    if (object.GetSceneIn(i) is scene) found = true;\n"
        "  }\n"
        "  if (!found) return 5;\n"
        "  object.IsActiveInScene(scene);\n"
        "  object.IsActiveInCurrentScene();\n"
        "  NativePointer appData = object.GetAppData();\n"
        "  if (!appData.IsNull()) return 6;\n"
        "  object.SetAppData(appData);\n"
        "  CKObject@ asObject = object;\n"
        "  if (cast<CKSceneObject@>(asObject) !is object) return 7;\n"
        "  if (object.GetClassID() != CKCID_BEOBJECT) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKSceneObject self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("cksceneobject-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSceneObject self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSceneObject self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKSceneObjectSurface(CKSceneObject@, CKScene@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKSceneObject self-test function was not found.";
        return false;
    }

    CKScene *scene = CKScene::Cast(context->CreateObject(CKCID_SCENE,
                                                        const_cast<CKSTRING>("__CKAS_CKSceneObjectSelfTestScene"),
                                                        CK_OBJECTCREATION_DYNAMIC));
    CKBeObject *beObject = CKBeObject::Cast(context->CreateObject(CKCID_BEOBJECT,
                                                                  const_cast<CKSTRING>("__CKAS_CKSceneObjectSelfTestObject"),
                                                                  CK_OBJECTCREATION_DYNAMIC));
    CKSceneObject *sceneObject = CKSceneObject::Cast(beObject);
    if (!scene || !beObject || !sceneObject) {
        if (beObject) context->DestroyObject(beObject);
        if (scene) context->DestroyObject(scene);
        engine->DiscardModule(moduleName);
        error = "CKSceneObject self-test could not create temporary scene objects.";
        return false;
    }

    scene->AddObjectToScene(sceneObject, TRUE);
    const bool ok = ExecuteCKSceneObjectProbe(engine, probe, sceneObject, scene, false, "CKSceneObject surface probe", error);

    scene->RemoveObjectFromScene(sceneObject, TRUE);
    context->DestroyObject(beObject);
    context->DestroyObject(scene);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBeObjectScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBeObject script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *beObjectType = engine->GetTypeInfoByDecl("CKBeObject");
    if (!beObjectType) {
        error = "CKBeObject self-test could not find the registered type.";
        return false;
    }
    if (beObjectType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr) {
        error = "CKBeObject self-test found unsafe ApplyPatchForOlderVersion array-pointer binding.";
        return false;
    }
    if (!beObjectType->GetMethodByDecl("void ExecuteBehaviors(float delta)") ||
        !beObjectType->GetMethodByDecl("bool IsInGroup(CKGroup@ group)") ||
        !beObjectType->GetMethodByDecl("bool HasAttribute(CKAttributeType attribType)") ||
        !beObjectType->GetMethodByDecl("int GetAttributeCount()") ||
        !beObjectType->GetMethodByDecl("CKParameterOut@ GetAttributeParameterByIndex(int index)") ||
        !beObjectType->GetMethodByDecl("CKERROR AddScript(CKBehavior@ ckb)") ||
        !beObjectType->GetMethodByDecl("CKBehavior@ RemoveScript(CK_ID id)") ||
        !beObjectType->GetMethodByDecl("CKBehavior@ RemoveScript(int pos)") ||
        !beObjectType->GetMethodByDecl("CKERROR RemoveAllScripts()") ||
        !beObjectType->GetMethodByDecl("CKBehavior@ GetScript(int pos)") ||
        !beObjectType->GetMethodByDecl("int GetScriptCount()") ||
        !beObjectType->GetMethodByDecl("int GetPriority()") ||
        !beObjectType->GetMethodByDecl("void SetPriority(int priority)") ||
        !beObjectType->GetMethodByDecl("int GetLastFrameMessageCount()") ||
        !beObjectType->GetMethodByDecl("CKMessage@ GetLastFrameMessage(int pos)") ||
        !beObjectType->GetMethodByDecl("void SetAsWaitingForMessages(bool wait = true)") ||
        !beObjectType->GetMethodByDecl("bool IsWaitingForMessages()") ||
        !beObjectType->GetMethodByDecl("int CallBehaviorCallbackFunction(CKDWORD message, CKGUID &in behGuid = void)") ||
        !beObjectType->GetMethodByDecl("float GetLastExecutionTime()")) {
        error = "CKBeObject self-test could not find expected object methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBeObjectSelfTest";
    const char *source =
        "int ProbeCKBeObjectSurface(CKBeObject@ object, CKBehavior@ behavior, CKGroup@ group) {\n"
        "  if (object is null || behavior is null || group is null) return 2;\n"
        "  if (object.GetClassID() != CKCID_BEOBJECT) return 3;\n"
        "  if (group.AddObject(object) != CK_OK) return 4;\n"
        "  if (!object.IsInGroup(group)) return 5;\n"
        "  group.Clear();\n"
        "  if (object.IsInGroup(group)) return 6;\n"
        "  int priority = object.GetPriority();\n"
        "  object.SetPriority(priority + 1);\n"
        "  if (object.GetPriority() != priority + 1) return 7;\n"
        "  object.SetPriority(priority);\n"
        "  object.SetAsWaitingForMessages(true);\n"
        "  if (!object.IsWaitingForMessages()) return 8;\n"
        "  object.SetAsWaitingForMessages(false);\n"
        "  if (object.IsWaitingForMessages()) return 9;\n"
        "  object.GetLastFrameMessageCount();\n"
        "  object.GetLastExecutionTime();\n"
        "  object.ExecuteBehaviors(0.0f);\n"
        "  if (object.GetScriptCount() != 0) return 10;\n"
        "  if (object.AddScript(behavior) != CK_OK) return 11;\n"
        "  if (object.GetScriptCount() != 1) return 12;\n"
        "  if (object.GetScript(0) !is behavior) return 13;\n"
        "  CK_ID id = behavior.GetID();\n"
        "  CKBehavior@ removedById = object.RemoveScript(id);\n"
        "  if (removedById !is behavior) return 14;\n"
        "  if (object.GetScriptCount() != 0) return 15;\n"
        "  if (object.AddScript(behavior) != CK_OK) return 16;\n"
        "  CKBehavior@ removedByPos = object.RemoveScript(0);\n"
        "  if (removedByPos !is behavior) return 17;\n"
        "  if (object.AddScript(behavior) != CK_OK) return 18;\n"
        "  if (object.RemoveAllScripts() != CK_OK) return 19;\n"
        "  if (object.GetScriptCount() != 0) return 20;\n"
        "  object.GetAttributeCount();\n"
        "  CKObject@ asObject = object;\n"
        "  if (cast<CKBeObject@>(asObject) !is object) return 21;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBeObject self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckbeobject-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBeObject self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBeObject self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKBeObjectSurface(CKBeObject@, CKBehavior@, CKGroup@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBeObject self-test function was not found.";
        return false;
    }

    CKBeObject *object = CKBeObject::Cast(context->CreateObject(CKCID_BEOBJECT,
                                                                const_cast<CKSTRING>("__CKAS_CKBeObjectSelfTestObject"),
                                                                CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR,
                                                                  const_cast<CKSTRING>("__CKAS_CKBeObjectSelfTestBehavior"),
                                                                  CK_OBJECTCREATION_DYNAMIC));
    CKGroup *group = CKGroup::Cast(context->CreateObject(CKCID_GROUP,
                                                        const_cast<CKSTRING>("__CKAS_CKBeObjectSelfTestGroup"),
                                                        CK_OBJECTCREATION_DYNAMIC));
    if (!object || !behavior || !group) {
        if (group) context->DestroyObject(group);
        if (behavior) context->DestroyObject(behavior);
        if (object) context->DestroyObject(object);
        engine->DiscardModule(moduleName);
        error = "CKBeObject self-test could not create temporary objects.";
        return false;
    }

    const bool ok = ExecuteCKBeObjectProbe(engine, probe, object, behavior, group, false, "CKBeObject surface probe", error);

    group->Clear();
    object->RemoveAllScripts();
    context->DestroyObject(group);
    context->DestroyObject(behavior);
    context->DestroyObject(object);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKGroupScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKGroup script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *groupType = engine->GetTypeInfoByDecl("CKGroup");
    if (!groupType) {
        error = "CKGroup self-test could not find the registered type.";
        return false;
    }
    if (!groupType->GetMethodByDecl("CKERROR AddObject(CKBeObject@ obj)") ||
        !groupType->GetMethodByDecl("CKERROR AddObjectFront(CKBeObject@ obj)") ||
        !groupType->GetMethodByDecl("CKERROR InsertObjectAt(CKBeObject@ obj, int pos)") ||
        !groupType->GetMethodByDecl("CKBeObject@ RemoveObject(int pos)") ||
        !groupType->GetMethodByDecl("void RemoveObject(CKBeObject@ obj)") ||
        !groupType->GetMethodByDecl("void Clear()") ||
        !groupType->GetMethodByDecl("void MoveObjectUp(CKBeObject@ obj)") ||
        !groupType->GetMethodByDecl("void MoveObjectDown(CKBeObject@ obj)") ||
        !groupType->GetMethodByDecl("CKBeObject@ GetObject(int)") ||
        !groupType->GetMethodByDecl("int GetObjectCount()") ||
        !groupType->GetMethodByDecl("CK_CLASSID GetCommonClassID()") ||
        !groupType->GetMethodByDecl("NativePointer GetAppData()") ||
        !groupType->GetMethodByDecl("void SetAppData(NativePointer data)") ||
        !groupType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)")) {
        error = "CKGroup self-test could not find expected object methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKGroupSelfTest";
    const char *source =
        "int ProbeCKGroupSurface(CKGroup@ group, CKBeObject@ first, CKBeObject@ second, CKBeObject@ third) {\n"
        "  if (group is null || first is null || second is null || third is null) return 2;\n"
        "  if (group.GetClassID() != CKCID_GROUP) return 3;\n"
        "  if (group.GetObjectCount() != 0) return 4;\n"
        "  if (group.AddObject(first) != CK_OK) return 5;\n"
        "  if (group.AddObjectFront(second) != CK_OK) return 6;\n"
        "  if (group.GetObjectCount() != 2) return 7;\n"
        "  if (group.GetObject(0) !is second || group.GetObject(1) !is first) return 8;\n"
        "  if (!first.IsInGroup(group) || !second.IsInGroup(group)) return 9;\n"
        "  if (group.InsertObjectAt(third, 1) != CK_OK) return 10;\n"
        "  if (group.GetObjectCount() != 3) return 11;\n"
        "  if (group.GetObject(1) !is third) return 12;\n"
        "  CKBeObject@ removed = group.RemoveObject(1);\n"
        "  if (removed !is third || group.GetObjectCount() != 2) return 13;\n"
        "  if (third.IsInGroup(group)) return 14;\n"
        "  if (group.AddObject(third) != CK_OK) return 15;\n"
        "  group.MoveObjectUp(third);\n"
        "  group.MoveObjectDown(second);\n"
        "  if (!first.IsInGroup(group) || !second.IsInGroup(group) || !third.IsInGroup(group)) return 20;\n"
        "  group.RemoveObject(first);\n"
        "  if (first.IsInGroup(group) || group.GetObjectCount() != 2) return 16;\n"
        "  group.Clear();\n"
        "  if (group.GetObjectCount() != 0) return 17;\n"
        "  NativePointer appData = group.GetAppData();\n"
        "  if (!appData.IsNull()) return 18;\n"
        "  group.SetAppData(appData);\n"
        "  CKObject@ asObject = group;\n"
        "  if (cast<CKGroup@>(asObject) !is group) return 19;\n"
        "  group.GetCommonClassID();\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKGroup self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckgroup-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKGroup self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKGroup self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKGroupSurface(CKGroup@, CKBeObject@, CKBeObject@, CKBeObject@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKGroup self-test function was not found.";
        return false;
    }

    CKGroup *group = CKGroup::Cast(context->CreateObject(CKCID_GROUP,
                                                        const_cast<CKSTRING>("__CKAS_CKGroupSelfTestGroup"),
                                                        CK_OBJECTCREATION_DYNAMIC));
    CKBeObject *first = CKBeObject::Cast(context->CreateObject(CKCID_BEOBJECT,
                                                               const_cast<CKSTRING>("__CKAS_CKGroupSelfTestFirst"),
                                                               CK_OBJECTCREATION_DYNAMIC));
    CKBeObject *second = CKBeObject::Cast(context->CreateObject(CKCID_BEOBJECT,
                                                                const_cast<CKSTRING>("__CKAS_CKGroupSelfTestSecond"),
                                                                CK_OBJECTCREATION_DYNAMIC));
    CKBeObject *third = CKBeObject::Cast(context->CreateObject(CKCID_BEOBJECT,
                                                               const_cast<CKSTRING>("__CKAS_CKGroupSelfTestThird"),
                                                               CK_OBJECTCREATION_DYNAMIC));
    if (!group || !first || !second || !third) {
        if (group) context->DestroyObject(group);
        if (first) context->DestroyObject(first);
        if (second) context->DestroyObject(second);
        if (third) context->DestroyObject(third);
        engine->DiscardModule(moduleName);
        error = "CKGroup self-test could not create temporary group objects.";
        return false;
    }

    const bool ok = ExecuteCKGroupProbe(engine, probe, group, first, second, third, false, "CKGroup surface probe", error);

    group->Clear();
    context->DestroyObject(group);
    context->DestroyObject(first);
    context->DestroyObject(second);
    context->DestroyObject(third);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKRenderContextScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKRenderContext script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *renderContextType = engine->GetTypeInfoByDecl("CKRenderContext");
    if (!renderContextType) {
        error = "CKRenderContext self-test could not find the registered type.";
        return false;
    }
    if (renderContextType->GetMethodByDecl("VX_PIXELFORMAT GetPixelFormat(int &out bpp = void, int &out zbpp = void, int &out stencilBpp = void)") == nullptr) {
        error = "CKRenderContext self-test could not find output GetPixelFormat declaration.";
        return false;
    }
    if (renderContextType->GetMethodByDecl("bool LockCurrentVB(CKDWORD vertexCount, VxDrawPrimitiveData &out data)") == nullptr) {
        error = "CKRenderContext self-test could not find safe LockCurrentVB declaration.";
        return false;
    }
    if (renderContextType->GetMethodByDecl("NativeBuffer@ GetDrawPrimitiveIndices(int indicesCount)") == nullptr) {
        error = "CKRenderContext self-test could not find buffer GetDrawPrimitiveIndices declaration.";
        return false;
    }
    if (renderContextType->GetMethodByDecl("void AddPreRenderCallBack(CK_RENDERCALLBACK@ callback, bool temporary = false)") == nullptr ||
        renderContextType->GetMethodByDecl("void RemovePreRenderCallBack(CK_RENDERCALLBACK@ callback)") == nullptr ||
        renderContextType->GetMethodByDecl("void AddPostRenderCallBack(CK_RENDERCALLBACK@ callback, bool temporary = false)") == nullptr ||
        renderContextType->GetMethodByDecl("void RemovePostRenderCallBack(CK_RENDERCALLBACK@ callback)") == nullptr ||
        renderContextType->GetMethodByDecl("void AddPostSpriteRenderCallBack(CK_RENDERCALLBACK@ callback, bool temporary = false)") == nullptr ||
        renderContextType->GetMethodByDecl("void RemovePostSpriteRenderCallBack(CK_RENDERCALLBACK@ callback)") == nullptr) {
        error = "CKRenderContext self-test could not find expected callback declarations.";
        return false;
    }
    if (renderContextType->GetMethodByDecl("VX_PIXELFORMAT GetPixelFormat(int &in bpp = void, int &in zbpp = void, int &in stencilBpp = void)") != nullptr) {
        error = "CKRenderContext self-test found stale input GetPixelFormat declaration.";
        return false;
    }
    if (renderContextType->GetMethodByDecl("VxDrawPrimitiveData &LockCurrentVB(CKDWORD vertexCount)") != nullptr) {
        error = "CKRenderContext self-test found stale reference-return LockCurrentVB declaration.";
        return false;
    }
    if (renderContextType->GetMethodByDecl("CKWORD &GetDrawPrimitiveIndices(int indicesCount)") != nullptr) {
        error = "CKRenderContext self-test found stale reference-return GetDrawPrimitiveIndices declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKRenderContextSelfTest";
    const char *source =
        "void CKAS_RenderContextCallback(CKRenderContext@ dev) {\n"
        "}\n"
        "int ProbeCKRenderContextPixelFormat(CKContext@ ctx) {\n"
        "  CKRenderContext@ dev = ctx.GetPlayerRenderContext();\n"
        "  if (dev is null) return 1;\n"
        "  int bpp = -123;\n"
        "  int zbpp = -456;\n"
        "  int stencil = -789;\n"
        "  VX_PIXELFORMAT format = dev.GetPixelFormat(bpp, zbpp, stencil);\n"
        "  dev.GetPixelFormat();\n"
        "  if (bpp == -123 && zbpp == -456 && stencil == -789) return 2;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKRenderContextIndices(CKContext@ ctx) {\n"
        "  CKRenderContext@ dev = ctx.GetPlayerRenderContext();\n"
        "  if (dev is null) return 1;\n"
        "  NativeBuffer@ empty = dev.GetDrawPrimitiveIndices(0);\n"
        "  if (empty is null || !empty.IsEmpty()) return 2;\n"
        "  NativeBuffer@ indices = dev.GetDrawPrimitiveIndices(3);\n"
        "  if (indices is null || indices.Size() != 6) return 3;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKRenderContextCurrentVB(CKContext@ ctx) {\n"
        "  CKRenderContext@ dev = ctx.GetPlayerRenderContext();\n"
        "  if (dev is null) return 1;\n"
        "  VxDrawPrimitiveData data;\n"
        "  bool locked = dev.LockCurrentVB(1, data);\n"
        "  if (locked) {\n"
        "    dev.ReleaseCurrentVB();\n"
        "    return 2;\n"
        "  }\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKRenderContextCallbacks(CKContext@ ctx) {\n"
        "  CKRenderContext@ dev = ctx.GetPlayerRenderContext();\n"
        "  if (dev is null) return 1;\n"
        "  dev.RemovePreRenderCallBack(CKAS_RenderContextCallback);\n"
        "  dev.AddPreRenderCallBack(CKAS_RenderContextCallback);\n"
        "  dev.RemovePreRenderCallBack(CKAS_RenderContextCallback);\n"
        "  dev.RemovePostRenderCallBack(CKAS_RenderContextCallback);\n"
        "  dev.AddPostRenderCallBack(CKAS_RenderContextCallback);\n"
        "  dev.RemovePostRenderCallBack(CKAS_RenderContextCallback);\n"
        "  dev.RemovePostSpriteRenderCallBack(CKAS_RenderContextCallback);\n"
        "  dev.AddPostSpriteRenderCallBack(CKAS_RenderContextCallback);\n"
        "  dev.RemovePostSpriteRenderCallBack(CKAS_RenderContextCallback);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKRenderContextTemporaryCallback(CKContext@ ctx) {\n"
        "  CKRenderContext@ dev = ctx.GetPlayerRenderContext();\n"
        "  if (dev is null) return 1;\n"
        "  dev.AddPreRenderCallBack(CKAS_RenderContextCallback, true);\n"
        "  return 2;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKRenderContext self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-render-context-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRenderContext self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRenderContext self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKRenderContextPixelFormat(CKContext@)");
    asIScriptFunction *indices = module->GetFunctionByDecl("int ProbeCKRenderContextIndices(CKContext@)");
    asIScriptFunction *currentVB = module->GetFunctionByDecl("int ProbeCKRenderContextCurrentVB(CKContext@)");
    asIScriptFunction *callbacks = module->GetFunctionByDecl("int ProbeCKRenderContextCallbacks(CKContext@)");
    asIScriptFunction *temporary = module->GetFunctionByDecl("int ProbeCKRenderContextTemporaryCallback(CKContext@)");
    if (!probe || !indices || !currentVB || !callbacks || !temporary) {
        engine->DiscardModule(moduleName);
        error = "CKRenderContext self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKRenderContext GetPixelFormat probe", error) &&
                    ExecuteCKParameterTypeDescProbe(engine, indices, context, false, "CKRenderContext GetDrawPrimitiveIndices probe", error) &&
                    ExecuteCKParameterTypeDescProbe(engine, currentVB, context, false, "CKRenderContext LockCurrentVB probe", error) &&
                    ExecuteCKParameterTypeDescProbe(engine, callbacks, context, false, "CKRenderContext callback registration probe", error) &&
                    ExecuteCKParameterTypeDescProbe(engine, temporary, context, true, "CKRenderContext temporary callback probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKRenderObjectScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKRenderObject script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *renderObjectType = engine->GetTypeInfoByDecl("CKRenderObject");
    if (!renderObjectType) {
        error = "CKRenderObject self-test could not find the registered type.";
        return false;
    }
    if (renderObjectType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr) {
        error = "CKRenderObject self-test found unsafe inherited ApplyPatchForOlderVersion array-pointer binding.";
        return false;
    }
    if (!renderObjectType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext &in context)") ||
        !renderObjectType->GetMethodByDecl("CKObject@ opImplCast()") ||
        !renderObjectType->GetMethodByDecl("CKSceneObject@ opImplCast()") ||
        !renderObjectType->GetMethodByDecl("CKBeObject@ opImplCast()") ||
        !renderObjectType->GetMethodByDecl("bool IsInRenderContext(CKRenderContext@ context)") ||
        !renderObjectType->GetMethodByDecl("bool IsRootObject()") ||
        !renderObjectType->GetMethodByDecl("bool IsToBeRendered()") ||
        !renderObjectType->GetMethodByDecl("void SetZOrder(int z)") ||
        !renderObjectType->GetMethodByDecl("int GetZOrder()") ||
        !renderObjectType->GetMethodByDecl("bool IsToBeRenderedLast()") ||
        !renderObjectType->GetMethodByDecl("bool AddPreRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback, bool temporary = false)") ||
        !renderObjectType->GetMethodByDecl("bool RemovePreRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !renderObjectType->GetMethodByDecl("bool SetRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !renderObjectType->GetMethodByDecl("bool RemoveRenderCallBack()") ||
        !renderObjectType->GetMethodByDecl("bool AddPostRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback, bool temporary = false)") ||
        !renderObjectType->GetMethodByDecl("bool RemovePostRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !renderObjectType->GetMethodByDecl("void RemoveAllCallbacks()") ||
        !renderObjectType->GetMethodByDecl("NativePointer GetAppData()") ||
        !renderObjectType->GetMethodByDecl("void SetAppData(NativePointer data)")) {
        error = "CKRenderObject self-test could not find expected object methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKRenderObjectSelfTest";
    const char *source =
        "CKBOOL ProbeCKRenderObjectCallback(CKRenderContext@ context, CKRenderObject@ object) {\n"
        "  return context !is null && object !is null;\n"
        "}\n"
        "int ProbeCKRenderObjectSurface(CKContext@ ctx, CKObject@ object) {\n"
        "  if (ctx is null || object is null) return 2;\n"
        "  CKRenderObject@ renderObject = cast<CKRenderObject@>(object);\n"
        "  if (renderObject is null) return 3;\n"
        "  CKObject@ asObject = renderObject;\n"
        "  CKSceneObject@ asSceneObject = renderObject;\n"
        "  CKBeObject@ asBeObject = renderObject;\n"
        "  if (asObject !is object || asSceneObject is null || asBeObject is null) return 4;\n"
        "  if (cast<CKRenderObject@>(asObject) !is renderObject) return 5;\n"
        "  if (renderObject.GetCKContext() !is ctx) return 6;\n"
        "  renderObject.SetZOrder(9);\n"
        "  if (renderObject.GetZOrder() != 9) return 7;\n"
        "  renderObject.IsInRenderContext(null);\n"
        "  renderObject.IsRootObject();\n"
        "  renderObject.IsToBeRendered();\n"
        "  renderObject.IsToBeRenderedLast();\n"
        "  renderObject.RemoveAllCallbacks();\n"
        "  if (!renderObject.GetAppData().IsNull()) return 8;\n"
        "  renderObject.SetAppData(NativePointer());\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKRenderObject self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckrenderobject-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRenderObject self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRenderObject self-test script failed to build.";
        return false;
    }

    asIScriptFunction *callback = module->GetFunctionByDecl("CKBOOL ProbeCKRenderObjectCallback(CKRenderContext@, CKRenderObject@)");
    if (!callback) {
        engine->DiscardModule(moduleName);
        error = "CKRenderObject self-test functions were not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCK2dEntityScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CK2dEntity script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *entityType = engine->GetTypeInfoByDecl("CK2dEntity");
    if (!entityType) {
        error = "CK2dEntity self-test could not find the registered type.";
        return false;
    }
    if (entityType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr) {
        error = "CK2dEntity self-test found unsafe inherited ApplyPatchForOlderVersion array-pointer binding.";
        return false;
    }
    if (!entityType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)") ||
        !entityType->GetMethodByDecl("int GetPosition(Vx2DVector&out vect, bool hom = false, CK2dEntity@ ref = null)") ||
        !entityType->GetMethodByDecl("void SetPosition(const Vx2DVector&in vect, bool hom = false, bool keepChildren = false, CK2dEntity@ ref = null)") ||
        !entityType->GetMethodByDecl("bool SetParent(CK2dEntity@ parent)") ||
        !entityType->GetMethodByDecl("CK2dEntity@ GetParent() const") ||
        !entityType->GetMethodByDecl("CK2dEntity@ GetChild(int i) const") ||
        !entityType->GetMethodByDecl("CK2dEntity@ HierarchyParser(CK2dEntity@ current) const") ||
        !entityType->GetMethodByDecl("void SetMaterial(CKMaterial@ mat)") ||
        !entityType->GetMethodByDecl("CKMaterial@ GetMaterial()") ||
        !entityType->GetMethodByDecl("bool AddPreRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback, bool temporary = false)") ||
        !entityType->GetMethodByDecl("bool RemovePreRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !entityType->GetMethodByDecl("bool SetRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !entityType->GetMethodByDecl("bool RemoveRenderCallBack()") ||
        !entityType->GetMethodByDecl("bool AddPostRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback, bool temporary = false)") ||
        !entityType->GetMethodByDecl("bool RemovePostRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !entityType->GetMethodByDecl("void RemoveAllCallbacks()") ||
        !entityType->GetMethodByDecl("NativePointer GetAppData()") ||
        !entityType->GetMethodByDecl("void SetAppData(NativePointer data)")) {
        error = "CK2dEntity self-test could not find expected object methods.";
        return false;
    }

    asITypeInfo *spriteType = engine->GetTypeInfoByDecl("CKSprite");
    asITypeInfo *spriteTextType = engine->GetTypeInfoByDecl("CKSpriteText");
    if (!spriteType || !spriteTextType) {
        error = "CK2dEntity self-test could not find registered CKSprite types.";
        return false;
    }
    if (spriteType->GetMethodByDecl("CKBitmapProperties &GetSaveFormat()") ||
        spriteType->GetMethodByDecl("void SetSaveFormat(CKBitmapProperties &in format)") ||
        spriteTextType->GetMethodByDecl("CKBitmapProperties &GetSaveFormat()") ||
        spriteTextType->GetMethodByDecl("void SetSaveFormat(CKBitmapProperties &in format)")) {
        error = "CK2dEntity self-test found stale CKSprite save-format reference bindings.";
        return false;
    }
    if (!spriteType->GetMethodByDecl("CKBitmapProperties@ GetSaveFormat()") ||
        !spriteType->GetMethodByDecl("void SetSaveFormat(CKBitmapProperties@ format)") ||
        !spriteTextType->GetMethodByDecl("CKBitmapProperties@ GetSaveFormat()") ||
        !spriteTextType->GetMethodByDecl("void SetSaveFormat(CKBitmapProperties@ format)")) {
        error = "CK2dEntity self-test could not find expected CKSprite save-format handle bindings.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CK2dEntitySelfTest";
    const char *source =
        "int ProbeCK2dEntitySurface(CK2dEntity@ entity, CK2dEntity@ child, CKMaterial@ material) {\n"
        "  if (entity is null || child is null || material is null) return 2;\n"
        "  CKObject@ asObject = entity;\n"
        "  CKSceneObject@ asSceneObject = entity;\n"
        "  CKBeObject@ asBeObject = entity;\n"
        "  CKRenderObject@ asRenderObject = entity;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null) return 3;\n"
        "  if (cast<CK2dEntity>(asObject) !is entity) return 4;\n"
        "  if (cast<CK2dEntity>(asRenderObject) !is entity) return 5;\n"
        "  entity.SetName(\"__CKAS_CK2dEntitySelfTest\", false);\n"
        "  if (entity.GetName() == \"\") return 6;\n"
        "  entity.SetPosition(Vx2DVector(12.0f, 34.0f));\n"
        "  Vx2DVector pos;\n"
        "  if (entity.GetPosition(pos) != CK_OK) return 7;\n"
        "  if (pos.x != 12.0f || pos.y != 34.0f) return 8;\n"
        "  entity.SetSize(Vx2DVector(48.0f, 64.0f));\n"
        "  Vx2DVector size;\n"
        "  entity.GetSize(size);\n"
        "  if (size.x != 48.0f || size.y != 64.0f) return 9;\n"
        "  VxRect rect(1.0f, 2.0f, 9.0f, 10.0f);\n"
        "  entity.SetRect(rect);\n"
        "  VxRect gotRect;\n"
        "  entity.GetRect(gotRect);\n"
        "  if (gotRect.left != 1.0f || gotRect.top != 2.0f || gotRect.right != 9.0f || gotRect.bottom != 10.0f) return 10;\n"
        "  VxRect src(0.0f, 0.0f, 16.0f, 16.0f);\n"
        "  entity.SetSourceRect(src);\n"
        "  entity.UseSourceRect(true);\n"
        "  if (!entity.IsUsingSourceRect()) return 11;\n"
        "  entity.SetPickable(false);\n"
        "  if (entity.IsPickable()) return 12;\n"
        "  entity.SetPickable(true);\n"
        "  entity.SetBackground(true);\n"
        "  if (!entity.IsBackground()) return 13;\n"
        "  entity.SetBackground(false);\n"
        "  entity.SetClipToParent(true);\n"
        "  if (!entity.IsClipToParent()) return 14;\n"
        "  entity.SetClipToParent(false);\n"
        "  entity.EnableRatioOffset(true);\n"
        "  if (!entity.IsRatioOffset()) return 15;\n"
        "  entity.EnableRatioOffset(false);\n"
        "  if (!child.SetParent(entity)) return 16;\n"
        "  if (child.GetParent() !is entity) return 17;\n"
        "  if (entity.GetChildrenCount() < 1) return 18;\n"
        "  if (entity.GetChild(0) !is child) return 19;\n"
        "  if (entity.HierarchyParser(null) !is child) return 20;\n"
        "  if (!child.SetParent(null)) return 21;\n"
        "  if (child.GetParent() !is null) return 22;\n"
        "  entity.SetMaterial(material);\n"
        "  if (entity.GetMaterial() !is material) return 23;\n"
        "  entity.SetMaterial(null);\n"
        "  if (entity.GetMaterial() !is null) return 24;\n"
        "  entity.SetHomogeneousCoordinates(true);\n"
        "  if (!entity.IsHomogeneousCoordinates()) return 25;\n"
        "  entity.SetHomogeneousCoordinates(false);\n"
        "  entity.EnableClipToCamera(true);\n"
        "  if (!entity.IsClippedToCamera()) return 26;\n"
        "  entity.EnableClipToCamera(false);\n"
        "  entity.SetZOrder(7);\n"
        "  if (entity.GetZOrder() != 7) return 27;\n"
        "  entity.GetExtents(src, gotRect);\n"
        "  entity.SetExtents(src, gotRect);\n"
        "  if (!entity.GetAppData().IsNull()) return 28;\n"
        "  entity.SetAppData(NativePointer());\n"
        "  CKSprite@ sprite = cast<CKSprite>(entity);\n"
        "  if (sprite is null) return 29;\n"
        "  if (sprite.GetSaveFormat() !is null) return 30;\n"
        "  sprite.SetSaveFormat(null);\n"
        "  if (sprite.GetSaveFormat() !is null) return 31;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CK2dEntity self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck2dentity-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK2dEntity self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK2dEntity self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCK2dEntitySurface(CK2dEntity@, CK2dEntity@, CKMaterial@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CK2dEntity self-test function was not found.";
        return false;
    }

    CK2dEntity *entity = CK2dEntity::Cast(context->CreateObject(
        CKCID_SPRITE, const_cast<CKSTRING>("__CKAS_CK2dEntitySelfTestEntity"), CK_OBJECTCREATION_DYNAMIC));
    CK2dEntity *child = CK2dEntity::Cast(context->CreateObject(
        CKCID_SPRITE, const_cast<CKSTRING>("__CKAS_CK2dEntitySelfTestChild"), CK_OBJECTCREATION_DYNAMIC));
    CKMaterial *material = CKMaterial::Cast(context->CreateObject(
        CKCID_MATERIAL, const_cast<CKSTRING>("__CKAS_CK2dEntitySelfTestMaterial"), CK_OBJECTCREATION_DYNAMIC));
    if (!entity || !child || !material) {
        if (material) context->DestroyObject(material);
        if (child) context->DestroyObject(child);
        if (entity) context->DestroyObject(entity);
        engine->DiscardModule(moduleName);
        error = "CK2dEntity self-test could not create temporary objects.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCK2dEntityProbe(engine, probe, entity, child, material, false, "CK2dEntity surface probe", error) &&
                    ExecuteCK2dEntityCopyNullProbe(engine, entityType, entity, dependencies, error);

    child->SetParent(nullptr);
    entity->RemoveAllCallbacks();
    context->DestroyObject(material);
    context->DestroyObject(child);
    context->DestroyObject(entity);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCK3dEntityScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CK3dEntity script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *entityType = engine->GetTypeInfoByDecl("CK3dEntity");
    if (!entityType) {
        error = "CK3dEntity self-test could not find the registered type.";
        return false;
    }
    if (entityType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        entityType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        entityType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr) {
        error = "CK3dEntity self-test found stale unsafe inherited or vector-array bindings.";
        return false;
    }
    if (!entityType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)") ||
        !entityType->GetMethodByDecl("bool SetParent(CK3dEntity@ parent, bool keepWorldPos = true)") ||
        !entityType->GetMethodByDecl("CK3dEntity@ GetParent() const") ||
        !entityType->GetMethodByDecl("bool AddChild(CK3dEntity@ child, bool keepWorldPos = true)") ||
        !entityType->GetMethodByDecl("bool RemoveChild(CK3dEntity@ mov)") ||
        !entityType->GetMethodByDecl("CK3dEntity@ HierarchyParser(CK3dEntity@ current) const") ||
        !entityType->GetMethodByDecl("CKERROR AddMesh(CKMesh@ mesh)") ||
        !entityType->GetMethodByDecl("CKERROR RemoveMesh(CKMesh@ mesh)") ||
        !entityType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !entityType->GetMethodByDecl("void InverseTransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !entityType->GetMethodByDecl("bool AddPreRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback, bool temporary = false)") ||
        !entityType->GetMethodByDecl("bool RemovePreRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !entityType->GetMethodByDecl("bool SetRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !entityType->GetMethodByDecl("bool RemoveRenderCallBack()") ||
        !entityType->GetMethodByDecl("NativePointer GetAppData()") ||
        !entityType->GetMethodByDecl("void SetAppData(NativePointer data)")) {
        error = "CK3dEntity self-test could not find expected object methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CK3dEntitySelfTest";
    const char *source =
        "int ProbeCK3dEntitySurface(CK3dEntity@ entity, CK3dEntity@ child, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (entity is null || child is null || mesh is null || animation is null) return 2;\n"
        "  CKObject@ asObject = entity;\n"
        "  CKSceneObject@ asSceneObject = entity;\n"
        "  CKBeObject@ asBeObject = entity;\n"
        "  CKRenderObject@ asRenderObject = entity;\n"
        "  CK3dObject@ as3dObject = cast<CK3dObject>(entity);\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || as3dObject is null) return 3;\n"
        "  CK3dEntity@ entityAgain = as3dObject;\n"
        "  if (entityAgain !is entity) return 4;\n"
        "  if (cast<CK3dEntity>(asObject) !is entity) return 5;\n"
        "  if (cast<CK3dEntity>(asRenderObject) !is entity) return 6;\n"
        "  entity.SetName(\"__CKAS_CK3dEntitySelfTest\", false);\n"
        "  if (entity.GetName() == \"\") return 7;\n"
        "  entity.SetPosition(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  VxVector pos;\n"
        "  entity.GetPosition(pos);\n"
        "  if (pos.x != 1.0f || pos.y != 2.0f || pos.z != 3.0f) return 8;\n"
        "  entity.SetScale(VxVector(2.0f, 3.0f, 4.0f));\n"
        "  VxVector scale;\n"
        "  entity.GetScale(scale);\n"
        "  if (scale.x != 2.0f || scale.y != 3.0f || scale.z != 4.0f) return 9;\n"
        "  entity.SetQuaternion(VxQuaternion(0.0f, 0.0f, 0.0f, 1.0f));\n"
        "  VxQuaternion quat;\n"
        "  entity.GetQuaternion(quat);\n"
        "  entity.Translate(VxVector(1.0f, 0.0f, 0.0f));\n"
        "  entity.Rotate(VxVector(0.0f, 1.0f, 0.0f), 0.0f);\n"
        "  entity.AddScale(VxVector(1.0f, 1.0f, 1.0f));\n"
        "  VxVector dir; VxVector up; VxVector right;\n"
        "  entity.GetOrientation(dir, up, right);\n"
        "  entity.SetOrientation(dir, up, right);\n"
        "  VxVector src(1.0f, 2.0f, 3.0f);\n"
        "  VxVector dst;\n"
        "  entity.Transform(dst, src);\n"
        "  entity.InverseTransform(dst, src);\n"
        "  entity.TransformVector(dst, src);\n"
        "  entity.InverseTransformVector(dst, src);\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  if (source.Write(src) != 12) return 10;\n"
        "  if (source.Write(VxVector(4.0f, 5.0f, 6.0f)) != 12) return 11;\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  entity.TransformMany(transformed, source, 2);\n"
        "  transformed.Reset();\n"
        "  entity.InverseTransformMany(source, transformed, 2);\n"
        "  if (!child.SetParent(entity)) return 12;\n"
        "  if (child.GetParent() !is entity) return 13;\n"
        "  if (entity.GetChildrenCount() < 1) return 14;\n"
        "  if (entity.GetChild(0) !is child) return 15;\n"
        "  if (entity.HierarchyParser(null) !is child) return 16;\n"
        "  if (!entity.RemoveChild(child)) return 17;\n"
        "  if (child.GetParent() !is null) return 18;\n"
        "  if (entity.AddMesh(mesh) != CK_OK) return 19;\n"
        "  if (entity.GetMeshCount() < 1) return 20;\n"
        "  if (entity.GetMesh(0) !is mesh) return 21;\n"
        "  entity.SetCurrentMesh(mesh);\n"
        "  if (entity.GetCurrentMesh() !is mesh) return 23;\n"
        "  if (entity.RemoveMesh(mesh) != CK_OK) return 24;\n"
        "  entity.AddObjectAnimation(animation);\n"
        "  if (entity.GetObjectAnimationCount() < 1) return 25;\n"
        "  if (entity.GetObjectAnimation(0) !is animation) return 26;\n"
        "  entity.RemoveObjectAnimation(animation);\n"
        "  entity.SetPickable(false);\n"
        "  if (entity.IsPickable()) return 27;\n"
        "  entity.SetPickable(true);\n"
        "  entity.IgnoreAnimations(true);\n"
        "  if (!entity.AreAnimationIgnored()) return 28;\n"
        "  entity.IgnoreAnimations(false);\n"
        "  entity.SetRenderAsTransparent(true);\n"
        "  entity.SetRenderAsTransparent(false);\n"
        "  entity.SetMoveableFlags(entity.GetMoveableFlags());\n"
        "  entity.ModifyMoveableFlags(0, 0);\n"
        "  VxRect extents;\n"
        "  entity.GetRenderExtents(extents);\n"
        "  entity.GetLastFrameMatrix();\n"
        "  entity.GetLocalMatrix();\n"
        "  entity.GetWorldMatrix();\n"
        "  entity.GetInverseWorldMatrix();\n"
        "  entity.UpdateBox(true);\n"
        "  entity.GetBoundingBox(false);\n"
        "  entity.GetHierarchicalBox(false);\n"
        "  VxVector bary;\n"
        "  entity.GetBaryCenter(bary);\n"
        "  entity.GetRadius();\n"
        "  if (!entity.GetAppData().IsNull()) return 29;\n"
        "  entity.SetAppData(NativePointer());\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCK3dEntitySmallTransform(CK3dEntity@ entity, CK3dEntity@ child, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  entity.TransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CK3dEntity self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck3dentity-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK3dEntity self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK3dEntity self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCK3dEntitySurface(CK3dEntity@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCK3dEntitySmallTransform(CK3dEntity@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    if (!probe || !smallTransform) {
        engine->DiscardModule(moduleName);
        error = "CK3dEntity self-test functions were not found.";
        return false;
    }

    CK3dEntity *entity = CK3dEntity::Cast(context->CreateObject(
        CKCID_3DOBJECT, const_cast<CKSTRING>("__CKAS_CK3dEntitySelfTestEntity"), CK_OBJECTCREATION_DYNAMIC));
    CK3dEntity *child = CK3dEntity::Cast(context->CreateObject(
        CKCID_3DOBJECT, const_cast<CKSTRING>("__CKAS_CK3dEntitySelfTestChild"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CK3dEntitySelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CK3dEntitySelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!entity || !child || !mesh || !animation) {
        if (animation) context->DestroyObject(animation);
        if (mesh) context->DestroyObject(mesh);
        if (child) context->DestroyObject(child);
        if (entity) context->DestroyObject(entity);
        engine->DiscardModule(moduleName);
        error = "CK3dEntity self-test could not create temporary objects.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCK3dEntityProbe(engine, probe, entity, child, mesh, animation, false, "CK3dEntity surface probe", error) &&
                    ExecuteCK3dEntityProbe(engine, smallTransform, entity, child, mesh, animation, true, "CK3dEntity small TransformMany probe", error) &&
                    ExecuteCK3dEntityCopyNullProbe(engine, entityType, entity, dependencies, error);

    child->SetParent(nullptr);
    entity->RemoveAllCallbacks();
    entity->RemoveMesh(mesh);
    entity->RemoveObjectAnimation(animation);
    context->DestroyObject(animation);
    context->DestroyObject(mesh);
    context->DestroyObject(child);
    context->DestroyObject(entity);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCK3dObjectScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CK3dObject script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *objectType = engine->GetTypeInfoByDecl("CK3dObject");
    if (!objectType) {
        error = "CK3dObject self-test could not find the registered type.";
        return false;
    }
    if (objectType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        objectType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        objectType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr) {
        error = "CK3dObject self-test found stale unsafe inherited declarations.";
        return false;
    }
    if (!objectType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)") ||
        !objectType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !objectType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !objectType->GetMethodByDecl("void InverseTransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !objectType->GetMethodByDecl("bool SetParent(CK3dEntity@ parent, bool keepWorldPos = true)") ||
        !objectType->GetMethodByDecl("CKERROR AddMesh(CKMesh@ mesh)") ||
        !objectType->GetMethodByDecl("void AddObjectAnimation(CKObjectAnimation@ anim)") ||
        !objectType->GetMethodByDecl("bool SetRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !objectType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CK3dObject self-test could not find expected inherited methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CK3dObjectSelfTest";
    const char *source =
        "int ProbeCK3dObjectSurface(CK3dObject@ object, CK3dObject@ child, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (object is null || child is null || mesh is null || animation is null) return 1;\n"
        "  CKObject@ asObject = object;\n"
        "  CKSceneObject@ asSceneObject = object;\n"
        "  CKBeObject@ asBeObject = object;\n"
        "  CKRenderObject@ asRenderObject = object;\n"
        "  CK3dEntity@ asEntity = object;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null) return 2;\n"
        "  if (cast<CK3dObject>(asObject) !is object) return 3;\n"
        "  if (cast<CK3dObject>(asRenderObject) !is object) return 4;\n"
        "  if (cast<CK3dObject>(asEntity) !is object) return 5;\n"
        "  object.SetName(\"__CKAS_CK3dObjectSelfTest\", false);\n"
        "  if (object.GetName() == \"\") return 6;\n"
        "  object.SetPosition(VxVector(7.0f, 8.0f, 9.0f));\n"
        "  VxVector pos;\n"
        "  object.GetPosition(pos);\n"
        "  if (pos.x != 7.0f || pos.y != 8.0f || pos.z != 9.0f) return 7;\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  object.TransformMany(transformed, source, 2);\n"
        "  transformed.Reset();\n"
        "  object.InverseTransformMany(source, transformed, 2);\n"
        "  if (!child.SetParent(object)) return 8;\n"
        "  if (child.GetParent() !is object) return 9;\n"
        "  if (object.GetChildrenCount() < 1) return 10;\n"
        "  if (!object.RemoveChild(child)) return 11;\n"
        "  if (object.AddMesh(mesh) != CK_OK) return 12;\n"
        "  object.SetCurrentMesh(mesh);\n"
        "  if (object.GetCurrentMesh() !is mesh) return 13;\n"
        "  if (object.RemoveMesh(mesh) != CK_OK) return 14;\n"
        "  object.AddObjectAnimation(animation);\n"
        "  if (object.GetObjectAnimationCount() < 1) return 15;\n"
        "  if (object.GetObjectAnimation(0) !is animation) return 16;\n"
        "  object.RemoveObjectAnimation(animation);\n"
        "  if (!object.GetAppData().IsNull()) return 17;\n"
        "  object.SetAppData(NativePointer());\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCK3dObjectSmallTransform(CK3dObject@ object, CK3dObject@ child, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  object.InverseTransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CK3dObject self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck3dobject-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK3dObject self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK3dObject self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCK3dObjectSurface(CK3dObject@, CK3dObject@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCK3dObjectSmallTransform(CK3dObject@, CK3dObject@, CKMesh@, CKObjectAnimation@)");
    if (!probe || !smallTransform) {
        engine->DiscardModule(moduleName);
        error = "CK3dObject self-test functions were not found.";
        return false;
    }

    CK3dObject *object = CK3dObject::Cast(context->CreateObject(
        CKCID_3DOBJECT, const_cast<CKSTRING>("__CKAS_CK3dObjectSelfTestObject"), CK_OBJECTCREATION_DYNAMIC));
    CK3dObject *child = CK3dObject::Cast(context->CreateObject(
        CKCID_3DOBJECT, const_cast<CKSTRING>("__CKAS_CK3dObjectSelfTestChild"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CK3dObjectSelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CK3dObjectSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!object || !child || !mesh || !animation) {
        if (animation) context->DestroyObject(animation);
        if (mesh) context->DestroyObject(mesh);
        if (child) context->DestroyObject(child);
        if (object) context->DestroyObject(object);
        engine->DiscardModule(moduleName);
        error = "CK3dObject self-test could not create temporary objects.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCK3dObjectProbe(engine, probe, object, child, mesh, animation, false, "CK3dObject surface probe", error) &&
                    ExecuteCK3dObjectProbe(engine, smallTransform, object, child, mesh, animation, true, "CK3dObject small TransformMany probe", error) &&
                    ExecuteCK3dObjectCopyNullProbe(engine, objectType, object, dependencies, error);

    child->SetParent(nullptr);
    object->RemoveAllCallbacks();
    object->RemoveMesh(mesh);
    object->RemoveObjectAnimation(animation);
    context->DestroyObject(animation);
    context->DestroyObject(mesh);
    context->DestroyObject(child);
    context->DestroyObject(object);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKCameraScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKCamera script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *cameraType = engine->GetTypeInfoByDecl("CKCamera");
    if (!cameraType) {
        error = "CKCamera self-test could not find the registered type.";
        return false;
    }
    if (cameraType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        cameraType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        cameraType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        cameraType->GetMethodByDecl("CK3dEntity@ GetTarget() const") != nullptr) {
        error = "CKCamera self-test found stale unsafe inherited or const-drift declarations.";
        return false;
    }
    if (!cameraType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)") ||
        !cameraType->GetMethodByDecl("float GetFrontPlane() const") ||
        !cameraType->GetMethodByDecl("void SetFrontPlane(float front)") ||
        !cameraType->GetMethodByDecl("float GetBackPlane() const") ||
        !cameraType->GetMethodByDecl("void SetBackPlane(float back)") ||
        !cameraType->GetMethodByDecl("float GetFov() const") ||
        !cameraType->GetMethodByDecl("void SetFov(float fov)") ||
        !cameraType->GetMethodByDecl("int GetProjectionType() const") ||
        !cameraType->GetMethodByDecl("void SetProjectionType(int proj)") ||
        !cameraType->GetMethodByDecl("void SetAspectRatio(int width, int height)") ||
        !cameraType->GetMethodByDecl("void GetAspectRatio(int&out width, int&out height)") ||
        !cameraType->GetMethodByDecl("void ComputeProjectionMatrix(VxMatrix&out mat)") ||
        !cameraType->GetMethodByDecl("CK3dEntity@ GetTarget()") ||
        !cameraType->GetMethodByDecl("void SetTarget(CK3dEntity@ target)") ||
        !cameraType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !cameraType->GetMethodByDecl("bool SetRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !cameraType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKCamera self-test could not find expected camera methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKCameraSelfTest";
    const char *source =
        "bool CloseEnough(float a, float b) {\n"
        "  float d = a - b;\n"
        "  return d > -0.0001f && d < 0.0001f;\n"
        "}\n"
        "int ProbeCKCameraSurface(CKCamera@ camera, CK3dEntity@ target, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (camera is null || target is null || mesh is null || animation is null) return 1;\n"
        "  CKObject@ asObject = camera;\n"
        "  CKSceneObject@ asSceneObject = camera;\n"
        "  CKBeObject@ asBeObject = camera;\n"
        "  CKRenderObject@ asRenderObject = camera;\n"
        "  CK3dEntity@ asEntity = camera;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null) return 2;\n"
        "  if (cast<CKCamera>(asObject) !is camera) return 3;\n"
        "  if (cast<CKCamera>(asRenderObject) !is camera) return 4;\n"
        "  if (cast<CKCamera>(asEntity) !is camera) return 5;\n"
        "  camera.SetName(\"__CKAS_CKCameraSelfTest\", false);\n"
        "  if (camera.GetName() == \"\") return 6;\n"
        "  camera.SetFrontPlane(0.5f);\n"
        "  camera.SetBackPlane(500.0f);\n"
        "  if (!CloseEnough(camera.GetFrontPlane(), 0.5f)) return 7;\n"
        "  if (!CloseEnough(camera.GetBackPlane(), 500.0f)) return 8;\n"
        "  camera.SetFov(0.75f);\n"
        "  if (!CloseEnough(camera.GetFov(), 0.75f)) return 9;\n"
        "  int projection = camera.GetProjectionType();\n"
        "  camera.SetProjectionType(projection);\n"
        "  if (camera.GetProjectionType() != projection) return 10;\n"
        "  camera.SetOrthographicZoom(1.25f);\n"
        "  if (!CloseEnough(camera.GetOrthographicZoom(), 1.25f)) return 11;\n"
        "  camera.SetAspectRatio(16, 9);\n"
        "  int width = 0;\n"
        "  int height = 0;\n"
        "  camera.GetAspectRatio(width, height);\n"
        "  if (width != 16 || height != 9) return 12;\n"
        "  VxMatrix projectionMatrix;\n"
        "  camera.ComputeProjectionMatrix(projectionMatrix);\n"
        "  camera.Roll(0.0f);\n"
        "  camera.ResetRoll();\n"
        "  camera.SetTarget(target);\n"
        "  camera.GetTarget();\n"
        "  camera.SetTarget(null);\n"
        "  camera.SetPosition(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  VxVector pos;\n"
        "  camera.GetPosition(pos);\n"
        "  if (pos.x != 1.0f || pos.y != 2.0f || pos.z != 3.0f) return 13;\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  camera.TransformMany(transformed, source, 2);\n"
        "  if (camera.AddMesh(mesh) != CK_OK) return 14;\n"
        "  camera.SetCurrentMesh(mesh);\n"
        "  if (camera.GetCurrentMesh() !is mesh) return 15;\n"
        "  if (camera.RemoveMesh(mesh) != CK_OK) return 16;\n"
        "  camera.AddObjectAnimation(animation);\n"
        "  if (camera.GetObjectAnimationCount() < 1) return 17;\n"
        "  camera.RemoveObjectAnimation(animation);\n"
        "  if (!camera.GetAppData().IsNull()) return 18;\n"
        "  camera.SetAppData(NativePointer());\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKCameraSmallTransform(CKCamera@ camera, CK3dEntity@ target, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  camera.TransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKCamera self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckcamera-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKCamera self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKCamera self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKCameraSurface(CKCamera@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCKCameraSmallTransform(CKCamera@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    if (!probe || !smallTransform) {
        engine->DiscardModule(moduleName);
        error = "CKCamera self-test functions were not found.";
        return false;
    }

    CKCamera *camera = CKCamera::Cast(context->CreateObject(
        CKCID_CAMERA, const_cast<CKSTRING>("__CKAS_CKCameraSelfTestCamera"), CK_OBJECTCREATION_DYNAMIC));
    CK3dEntity *target = CK3dEntity::Cast(context->CreateObject(
        CKCID_3DOBJECT, const_cast<CKSTRING>("__CKAS_CKCameraSelfTestTarget"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CKCameraSelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKCameraSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!camera || !target || !mesh || !animation) {
        if (animation) context->DestroyObject(animation);
        if (mesh) context->DestroyObject(mesh);
        if (target) context->DestroyObject(target);
        if (camera) context->DestroyObject(camera);
        engine->DiscardModule(moduleName);
        error = "CKCamera self-test could not create temporary objects.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCKCameraProbe(engine, probe, camera, target, mesh, animation, false, "CKCamera surface probe", error) &&
                    ExecuteCKCameraProbe(engine, smallTransform, camera, target, mesh, animation, true, "CKCamera small TransformMany probe", error) &&
                    ExecuteCKCameraCopyNullProbe(engine, cameraType, camera, dependencies, error);

    camera->RemoveAllCallbacks();
    camera->RemoveMesh(mesh);
    camera->RemoveObjectAnimation(animation);
    camera->SetTarget(nullptr);
    context->DestroyObject(animation);
    context->DestroyObject(mesh);
    context->DestroyObject(target);
    context->DestroyObject(camera);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKTargetCameraScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKTargetCamera script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *targetCameraType = engine->GetTypeInfoByDecl("CKTargetCamera");
    if (!targetCameraType) {
        error = "CKTargetCamera self-test could not find the registered type.";
        return false;
    }
    if (targetCameraType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        targetCameraType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        targetCameraType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        targetCameraType->GetMethodByDecl("CK3dEntity@ GetTarget() const") != nullptr) {
        error = "CKTargetCamera self-test found stale unsafe inherited or const-drift declarations.";
        return false;
    }
    if (!targetCameraType->GetMethodByDecl("CK3dEntity@ GetTarget()") ||
        !targetCameraType->GetMethodByDecl("void SetTarget(CK3dEntity@ target)") ||
        !targetCameraType->GetMethodByDecl("CKCamera@ opImplCast()") ||
        !targetCameraType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !targetCameraType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !targetCameraType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKTargetCamera self-test could not find expected target-camera methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTargetCameraSelfTest";
    const char *source =
        "bool CloseEnough(float a, float b) {\n"
        "  float d = a - b;\n"
        "  return d > -0.0001f && d < 0.0001f;\n"
        "}\n"
        "int ProbeCKTargetCameraSurface(CKTargetCamera@ camera, CK3dEntity@ target, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (camera is null || target is null || mesh is null || animation is null) return 1;\n"
        "  CKObject@ asObject = camera;\n"
        "  CKSceneObject@ asSceneObject = camera;\n"
        "  CKBeObject@ asBeObject = camera;\n"
        "  CKRenderObject@ asRenderObject = camera;\n"
        "  CK3dEntity@ asEntity = camera;\n"
        "  CKCamera@ asCamera = camera;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null || asCamera is null) return 2;\n"
        "  if (cast<CKTargetCamera>(asObject) !is camera) return 3;\n"
        "  if (cast<CKTargetCamera>(asRenderObject) !is camera) return 4;\n"
        "  if (cast<CKTargetCamera>(asEntity) !is camera) return 5;\n"
        "  if (cast<CKTargetCamera>(asCamera) !is camera) return 6;\n"
        "  camera.SetTarget(target);\n"
        "  if (camera.GetTarget() !is target) return 7;\n"
        "  camera.SetFrontPlane(0.5f);\n"
        "  camera.SetBackPlane(500.0f);\n"
        "  if (!CloseEnough(camera.GetFrontPlane(), 0.5f) || !CloseEnough(camera.GetBackPlane(), 500.0f)) return 8;\n"
        "  camera.SetFov(0.75f);\n"
        "  if (!CloseEnough(camera.GetFov(), 0.75f)) return 9;\n"
        "  int projection = camera.GetProjectionType();\n"
        "  camera.SetProjectionType(projection);\n"
        "  if (camera.GetProjectionType() != projection) return 10;\n"
        "  camera.SetAspectRatio(4, 3);\n"
        "  int width = 0;\n"
        "  int height = 0;\n"
        "  camera.GetAspectRatio(width, height);\n"
        "  if (width != 4 || height != 3) return 11;\n"
        "  VxMatrix projectionMatrix;\n"
        "  camera.ComputeProjectionMatrix(projectionMatrix);\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  camera.TransformMany(transformed, source, 2);\n"
        "  if (camera.AddMesh(mesh) != CK_OK) return 12;\n"
        "  camera.SetCurrentMesh(mesh);\n"
        "  if (camera.GetCurrentMesh() !is mesh) return 13;\n"
        "  if (camera.RemoveMesh(mesh) != CK_OK) return 14;\n"
        "  camera.AddObjectAnimation(animation);\n"
        "  if (camera.GetObjectAnimationCount() < 1) return 15;\n"
        "  camera.RemoveObjectAnimation(animation);\n"
        "  if (!camera.GetAppData().IsNull()) return 16;\n"
        "  camera.SetAppData(NativePointer());\n"
        "  camera.SetTarget(null);\n"
        "  if (camera.GetTarget() !is null) return 17;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKTargetCameraSmallTransform(CKTargetCamera@ camera, CK3dEntity@ target, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  camera.TransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTargetCamera self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("cktargetcamera-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTargetCamera self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTargetCamera self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKTargetCameraSurface(CKTargetCamera@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCKTargetCameraSmallTransform(CKTargetCamera@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    if (!probe || !smallTransform) {
        engine->DiscardModule(moduleName);
        error = "CKTargetCamera self-test functions were not found.";
        return false;
    }

    CKTargetCamera *camera = CKTargetCamera::Cast(context->CreateObject(
        CKCID_TARGETCAMERA, const_cast<CKSTRING>("__CKAS_CKTargetCameraSelfTestCamera"), CK_OBJECTCREATION_DYNAMIC));
    CK3dEntity *target = CK3dEntity::Cast(context->CreateObject(
        CKCID_3DOBJECT, const_cast<CKSTRING>("__CKAS_CKTargetCameraSelfTestTarget"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CKTargetCameraSelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKTargetCameraSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!camera || !target || !mesh || !animation) {
        if (animation) context->DestroyObject(animation);
        if (mesh) context->DestroyObject(mesh);
        if (target) context->DestroyObject(target);
        if (camera) context->DestroyObject(camera);
        engine->DiscardModule(moduleName);
        error = "CKTargetCamera self-test could not create temporary objects.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCKCameraProbe(engine, probe, camera, target, mesh, animation, false, "CKTargetCamera surface probe", error) &&
                    ExecuteCKCameraProbe(engine, smallTransform, camera, target, mesh, animation, true, "CKTargetCamera small TransformMany probe", error) &&
                    ExecuteCKCameraCopyNullProbe(engine, targetCameraType, camera, dependencies, error);

    camera->RemoveAllCallbacks();
    camera->RemoveMesh(mesh);
    camera->RemoveObjectAnimation(animation);
    camera->SetTarget(nullptr);
    context->DestroyObject(animation);
    context->DestroyObject(mesh);
    context->DestroyObject(target);
    context->DestroyObject(camera);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKLightScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKLight script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *lightType = engine->GetTypeInfoByDecl("CKLight");
    if (!lightType) {
        error = "CKLight self-test could not find the registered type.";
        return false;
    }
    if (lightType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        lightType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        lightType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        lightType->GetMethodByDecl("const VxColor& GetColor() const") != nullptr ||
        lightType->GetMethodByDecl("VXLIGHT_TYPE GetType() const") != nullptr ||
        lightType->GetMethodByDecl("float GetRange() const") != nullptr ||
        lightType->GetMethodByDecl("float GetHotSpot() const") != nullptr ||
        lightType->GetMethodByDecl("bool GetActivity() const") != nullptr ||
        lightType->GetMethodByDecl("CK3dEntity@ GetTarget() const") != nullptr ||
        lightType->GetMethodByDecl("void SetTarget(CK3dEntity@ target)") != nullptr ||
        lightType->GetMethodByDecl("float GetLightPower() const") != nullptr) {
        error = "CKLight self-test found stale unsafe, target, or const-drift declarations.";
        return false;
    }
    if (!lightType->GetMethodByDecl("const VxColor& GetColor()") ||
        !lightType->GetMethodByDecl("float GetConstantAttenuation()") ||
        !lightType->GetMethodByDecl("float GetLinearAttenuation()") ||
        !lightType->GetMethodByDecl("float GetQuadraticAttenuation()") ||
        !lightType->GetMethodByDecl("VXLIGHT_TYPE GetType()") ||
        !lightType->GetMethodByDecl("float GetRange()") ||
        !lightType->GetMethodByDecl("float GetHotSpot()") ||
        !lightType->GetMethodByDecl("float GetFallOff()") ||
        !lightType->GetMethodByDecl("float GetFallOffShape()") ||
        !lightType->GetMethodByDecl("bool GetActivity()") ||
        !lightType->GetMethodByDecl("bool GetSpecularFlag()") ||
        !lightType->GetMethodByDecl("float GetLightPower()") ||
        !lightType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !lightType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !lightType->GetMethodByDecl("bool SetRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !lightType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKLight self-test could not find expected light methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKLightSelfTest";
    const char *source =
        "bool CloseEnough(float a, float b) {\n"
        "  float d = a - b;\n"
        "  return d > -0.0001f && d < 0.0001f;\n"
        "}\n"
        "int ProbeCKLightSurface(CKLight@ light, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (light is null || mesh is null || animation is null) return 1;\n"
        "  CKObject@ asObject = light;\n"
        "  CKSceneObject@ asSceneObject = light;\n"
        "  CKBeObject@ asBeObject = light;\n"
        "  CKRenderObject@ asRenderObject = light;\n"
        "  CK3dEntity@ asEntity = light;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null) return 2;\n"
        "  if (cast<CKLight>(asObject) !is light) return 3;\n"
        "  if (cast<CKLight>(asRenderObject) !is light) return 4;\n"
        "  if (cast<CKLight>(asEntity) !is light) return 5;\n"
        "  light.SetName(\"__CKAS_CKLightSelfTest\", false);\n"
        "  if (light.GetName() == \"\") return 6;\n"
        "  light.SetColor(VxColor(0.1f, 0.2f, 0.3f, 1.0f));\n"
        "  VxColor color = light.GetColor();\n"
        "  if (!CloseEnough(color.r, 0.1f) || !CloseEnough(color.g, 0.2f) || !CloseEnough(color.b, 0.3f)) return 7;\n"
        "  light.SetConstantAttenuation(1.0f);\n"
        "  light.SetLinearAttenuation(0.25f);\n"
        "  light.SetQuadraticAttenuation(0.125f);\n"
        "  if (!CloseEnough(light.GetConstantAttenuation(), 1.0f)) return 8;\n"
        "  if (!CloseEnough(light.GetLinearAttenuation(), 0.25f)) return 9;\n"
        "  if (!CloseEnough(light.GetQuadraticAttenuation(), 0.125f)) return 10;\n"
        "  VXLIGHT_TYPE oldType = light.GetType();\n"
        "  light.SetType(oldType);\n"
        "  if (light.GetType() != oldType) return 11;\n"
        "  light.SetRange(42.0f);\n"
        "  if (!CloseEnough(light.GetRange(), 42.0f)) return 12;\n"
        "  light.SetHotSpot(0.25f);\n"
        "  light.SetFallOff(0.5f);\n"
        "  light.SetFallOffShape(0.75f);\n"
        "  if (!CloseEnough(light.GetHotSpot(), 0.25f)) return 13;\n"
        "  if (!CloseEnough(light.GetFallOff(), 0.5f)) return 14;\n"
        "  if (!CloseEnough(light.GetFallOffShape(), 0.75f)) return 15;\n"
        "  light.Active(true);\n"
        "  if (!light.GetActivity()) return 16;\n"
        "  light.Active(false);\n"
        "  if (light.GetActivity()) return 17;\n"
        "  light.Active(true);\n"
        "  light.SetSpecularFlag(true);\n"
        "  if (!light.GetSpecularFlag()) return 18;\n"
        "  light.SetSpecularFlag(false);\n"
        "  if (light.GetSpecularFlag()) return 19;\n"
        "  light.SetLightPower(1.5f);\n"
        "  if (!CloseEnough(light.GetLightPower(), 1.5f)) return 20;\n"
        "  light.SetPosition(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  VxVector pos;\n"
        "  light.GetPosition(pos);\n"
        "  if (pos.x != 1.0f || pos.y != 2.0f || pos.z != 3.0f) return 21;\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  light.TransformMany(transformed, source, 2);\n"
        "  if (light.AddMesh(mesh) != CK_OK) return 22;\n"
        "  light.SetCurrentMesh(mesh);\n"
        "  if (light.GetCurrentMesh() !is mesh) return 23;\n"
        "  if (light.RemoveMesh(mesh) != CK_OK) return 24;\n"
        "  light.AddObjectAnimation(animation);\n"
        "  if (light.GetObjectAnimationCount() < 1) return 25;\n"
        "  light.RemoveObjectAnimation(animation);\n"
        "  if (!light.GetAppData().IsNull()) return 26;\n"
        "  light.SetAppData(NativePointer());\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKLightSmallTransform(CKLight@ light, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  light.TransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKLight self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("cklight-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKLight self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKLight self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKLightSurface(CKLight@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCKLightSmallTransform(CKLight@, CKMesh@, CKObjectAnimation@)");
    if (!probe || !smallTransform) {
        engine->DiscardModule(moduleName);
        error = "CKLight self-test functions were not found.";
        return false;
    }

    CKLight *light = CKLight::Cast(context->CreateObject(
        CKCID_LIGHT, const_cast<CKSTRING>("__CKAS_CKLightSelfTestLight"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CKLightSelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKLightSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!light || !mesh || !animation) {
        if (animation) context->DestroyObject(animation);
        if (mesh) context->DestroyObject(mesh);
        if (light) context->DestroyObject(light);
        engine->DiscardModule(moduleName);
        error = "CKLight self-test could not create temporary objects.";
        return false;
    }

    const bool ok = ExecuteCKLightProbe(engine, probe, light, mesh, animation, false, "CKLight surface probe", error) &&
                    ExecuteCKLightProbe(engine, smallTransform, light, mesh, animation, true, "CKLight small TransformMany probe", error);

    light->RemoveAllCallbacks();
    light->RemoveMesh(mesh);
    light->RemoveObjectAnimation(animation);
    context->DestroyObject(animation);
    context->DestroyObject(mesh);
    context->DestroyObject(light);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKTargetLightScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKTargetLight script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *targetLightType = engine->GetTypeInfoByDecl("CKTargetLight");
    if (!targetLightType) {
        error = "CKTargetLight self-test could not find the registered type.";
        return false;
    }
    if (targetLightType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        targetLightType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        targetLightType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        targetLightType->GetMethodByDecl("CK3dEntity@ GetTarget() const") != nullptr ||
        targetLightType->GetMethodByDecl("float GetLightPower() const") != nullptr ||
        targetLightType->GetMethodByDecl("VXLIGHT_TYPE GetType() const") != nullptr) {
        error = "CKTargetLight self-test found stale unsafe or const-drift declarations.";
        return false;
    }
    if (!targetLightType->GetMethodByDecl("CK3dEntity@ GetTarget()") ||
        !targetLightType->GetMethodByDecl("void SetTarget(CK3dEntity@ target)") ||
        !targetLightType->GetMethodByDecl("CKLight@ opImplCast()") ||
        !targetLightType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !targetLightType->GetMethodByDecl("const VxColor& GetColor()") ||
        !targetLightType->GetMethodByDecl("VXLIGHT_TYPE GetType()") ||
        !targetLightType->GetMethodByDecl("float GetLightPower()") ||
        !targetLightType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !targetLightType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKTargetLight self-test could not find expected target-light methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTargetLightSelfTest";
    const char *source =
        "bool CloseEnough(float a, float b) {\n"
        "  float d = a - b;\n"
        "  return d > -0.0001f && d < 0.0001f;\n"
        "}\n"
        "int ProbeCKTargetLightSurface(CKTargetLight@ light, CK3dEntity@ target, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (light is null || target is null || mesh is null || animation is null) return 1;\n"
        "  CKObject@ asObject = light;\n"
        "  CKSceneObject@ asSceneObject = light;\n"
        "  CKBeObject@ asBeObject = light;\n"
        "  CKRenderObject@ asRenderObject = light;\n"
        "  CK3dEntity@ asEntity = light;\n"
        "  CKLight@ asLight = light;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null || asLight is null) return 2;\n"
        "  if (cast<CKTargetLight>(asObject) !is light) return 3;\n"
        "  if (cast<CKTargetLight>(asRenderObject) !is light) return 4;\n"
        "  if (cast<CKTargetLight>(asEntity) !is light) return 5;\n"
        "  if (cast<CKTargetLight>(asLight) !is light) return 6;\n"
        "  light.SetTarget(target);\n"
        "  if (light.GetTarget() !is target) return 7;\n"
        "  light.SetColor(VxColor(0.2f, 0.3f, 0.4f, 1.0f));\n"
        "  VxColor color = light.GetColor();\n"
        "  if (!CloseEnough(color.r, 0.2f) || !CloseEnough(color.g, 0.3f) || !CloseEnough(color.b, 0.4f)) return 8;\n"
        "  VXLIGHT_TYPE oldType = light.GetType();\n"
        "  light.SetType(oldType);\n"
        "  if (light.GetType() != oldType) return 9;\n"
        "  light.SetLightPower(0.75f);\n"
        "  if (!CloseEnough(light.GetLightPower(), 0.75f)) return 10;\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  light.TransformMany(transformed, source, 2);\n"
        "  if (light.AddMesh(mesh) != CK_OK) return 11;\n"
        "  light.SetCurrentMesh(mesh);\n"
        "  if (light.GetCurrentMesh() !is mesh) return 12;\n"
        "  if (light.RemoveMesh(mesh) != CK_OK) return 13;\n"
        "  light.AddObjectAnimation(animation);\n"
        "  if (light.GetObjectAnimationCount() < 1) return 14;\n"
        "  light.RemoveObjectAnimation(animation);\n"
        "  if (!light.GetAppData().IsNull()) return 15;\n"
        "  light.SetAppData(NativePointer());\n"
        "  light.SetTarget(null);\n"
        "  if (light.GetTarget() !is null) return 16;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKTargetLightSmallTransform(CKTargetLight@ light, CK3dEntity@ target, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  light.TransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTargetLight self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("cktargetlight-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTargetLight self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTargetLight self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKTargetLightSurface(CKTargetLight@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCKTargetLightSmallTransform(CKTargetLight@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    if (!probe || !smallTransform) {
        engine->DiscardModule(moduleName);
        error = "CKTargetLight self-test functions were not found.";
        return false;
    }

    CKTargetLight *light = CKTargetLight::Cast(context->CreateObject(
        CKCID_TARGETLIGHT, const_cast<CKSTRING>("__CKAS_CKTargetLightSelfTestLight"), CK_OBJECTCREATION_DYNAMIC));
    CK3dEntity *target = CK3dEntity::Cast(context->CreateObject(
        CKCID_3DOBJECT, const_cast<CKSTRING>("__CKAS_CKTargetLightSelfTestTarget"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CKTargetLightSelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKTargetLightSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!light || !target || !mesh || !animation) {
        if (animation) context->DestroyObject(animation);
        if (mesh) context->DestroyObject(mesh);
        if (target) context->DestroyObject(target);
        if (light) context->DestroyObject(light);
        engine->DiscardModule(moduleName);
        error = "CKTargetLight self-test could not create temporary objects.";
        return false;
    }

    const bool ok = ExecuteCKTargetLightProbe(engine, probe, light, target, mesh, animation, false, "CKTargetLight surface probe", error) &&
                    ExecuteCKTargetLightProbe(engine, smallTransform, light, target, mesh, animation, true, "CKTargetLight small TransformMany probe", error);

    light->RemoveAllCallbacks();
    light->RemoveMesh(mesh);
    light->RemoveObjectAnimation(animation);
    light->SetTarget(nullptr);
    context->DestroyObject(animation);
    context->DestroyObject(mesh);
    context->DestroyObject(target);
    context->DestroyObject(light);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKSprite3DScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKSprite3D script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *spriteType = engine->GetTypeInfoByDecl("CKSprite3D");
    if (!spriteType) {
        error = "CKSprite3D self-test could not find the registered type.";
        return false;
    }
    if (spriteType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        spriteType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        spriteType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        spriteType->GetMethodByDecl("CKMaterial@ GetMaterial() const") != nullptr ||
        spriteType->GetMethodByDecl("void GetSize(Vx2DVector &out vect) const") != nullptr ||
        spriteType->GetMethodByDecl("void GetOffset(Vx2DVector &out vect) const") != nullptr ||
        spriteType->GetMethodByDecl("void GetUVMapping(VxRect &out rect) const") != nullptr ||
        spriteType->GetMethodByDecl("VXSPRITE3D_TYPE GetMode() const") != nullptr) {
        error = "CKSprite3D self-test found stale unsafe or const-drift declarations.";
        return false;
    }
    if (!spriteType->GetMethodByDecl("CKMaterial@ GetMaterial()") ||
        !spriteType->GetMethodByDecl("void SetSize(const Vx2DVector &in vect)") ||
        !spriteType->GetMethodByDecl("void GetSize(Vx2DVector &out vect)") ||
        !spriteType->GetMethodByDecl("void SetOffset(const Vx2DVector &in vect)") ||
        !spriteType->GetMethodByDecl("void GetOffset(Vx2DVector &out vect)") ||
        !spriteType->GetMethodByDecl("void SetUVMapping(const VxRect &in rect)") ||
        !spriteType->GetMethodByDecl("void GetUVMapping(VxRect &out rect)") ||
        !spriteType->GetMethodByDecl("VXSPRITE3D_TYPE GetMode()") ||
        !spriteType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !spriteType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !spriteType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKSprite3D self-test could not find expected sprite methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKSprite3DSelfTest";
    const char *source =
        "bool CloseEnough(float a, float b) {\n"
        "  float d = a - b;\n"
        "  return d > -0.0001f && d < 0.0001f;\n"
        "}\n"
        "int ProbeCKSprite3DSurface(CKSprite3D@ sprite, CKMaterial@ material, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (sprite is null || material is null || mesh is null || animation is null) return 1;\n"
        "  CKObject@ asObject = sprite;\n"
        "  CKSceneObject@ asSceneObject = sprite;\n"
        "  CKBeObject@ asBeObject = sprite;\n"
        "  CKRenderObject@ asRenderObject = sprite;\n"
        "  CK3dEntity@ asEntity = sprite;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null) return 2;\n"
        "  if (cast<CKSprite3D>(asObject) !is sprite) return 3;\n"
        "  if (cast<CKSprite3D>(asRenderObject) !is sprite) return 4;\n"
        "  if (cast<CKSprite3D>(asEntity) !is sprite) return 5;\n"
        "  sprite.SetMaterial(material);\n"
        "  if (sprite.GetMaterial() !is material) return 6;\n"
        "  Vx2DVector sizeIn(2.0f, 3.0f);\n"
        "  sprite.SetSize(sizeIn);\n"
        "  if (!CloseEnough(sizeIn.x, 2.0f) || !CloseEnough(sizeIn.y, 3.0f)) return 7;\n"
        "  Vx2DVector sizeOut;\n"
        "  sprite.GetSize(sizeOut);\n"
        "  if (!CloseEnough(sizeOut.x, 2.0f) || !CloseEnough(sizeOut.y, 3.0f)) return 8;\n"
        "  Vx2DVector offsetIn(0.25f, -0.5f);\n"
        "  sprite.SetOffset(offsetIn);\n"
        "  if (!CloseEnough(offsetIn.x, 0.25f) || !CloseEnough(offsetIn.y, -0.5f)) return 9;\n"
        "  Vx2DVector offsetOut;\n"
        "  sprite.GetOffset(offsetOut);\n"
        "  if (!CloseEnough(offsetOut.x, 0.25f) || !CloseEnough(offsetOut.y, -0.5f)) return 10;\n"
        "  VxRect rectIn(0.0f, 0.0f, 0.5f, 0.75f);\n"
        "  sprite.SetUVMapping(rectIn);\n"
        "  if (!CloseEnough(rectIn.right, 0.5f) || !CloseEnough(rectIn.bottom, 0.75f)) return 11;\n"
        "  VxRect rectOut;\n"
        "  sprite.GetUVMapping(rectOut);\n"
        "  if (!CloseEnough(rectOut.right, 0.5f) || !CloseEnough(rectOut.bottom, 0.75f)) return 12;\n"
        "  VXSPRITE3D_TYPE oldMode = sprite.GetMode();\n"
        "  sprite.SetMode(oldMode);\n"
        "  if (sprite.GetMode() != oldMode) return 13;\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  sprite.TransformMany(transformed, source, 2);\n"
        "  if (sprite.AddMesh(mesh) != CK_OK) return 14;\n"
        "  sprite.SetCurrentMesh(mesh);\n"
        "  if (sprite.GetCurrentMesh() !is mesh) return 15;\n"
        "  if (sprite.RemoveMesh(mesh) != CK_OK) return 16;\n"
        "  sprite.AddObjectAnimation(animation);\n"
        "  if (sprite.GetObjectAnimationCount() < 1) return 17;\n"
        "  sprite.RemoveObjectAnimation(animation);\n"
        "  if (!sprite.GetAppData().IsNull()) return 18;\n"
        "  sprite.SetAppData(NativePointer());\n"
        "  sprite.SetMaterial(null);\n"
        "  if (sprite.GetMaterial() !is null) return 19;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKSprite3DSmallTransform(CKSprite3D@ sprite, CKMaterial@ material, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  sprite.TransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKSprite3D self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("cksprite3d-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSprite3D self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSprite3D self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKSprite3DSurface(CKSprite3D@, CKMaterial@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCKSprite3DSmallTransform(CKSprite3D@, CKMaterial@, CKMesh@, CKObjectAnimation@)");
    if (!probe || !smallTransform) {
        engine->DiscardModule(moduleName);
        error = "CKSprite3D self-test functions were not found.";
        return false;
    }

    CKSprite3D *sprite = CKSprite3D::Cast(context->CreateObject(
        CKCID_SPRITE3D, const_cast<CKSTRING>("__CKAS_CKSprite3DSelfTestSprite"), CK_OBJECTCREATION_DYNAMIC));
    CKMaterial *material = CKMaterial::Cast(context->CreateObject(
        CKCID_MATERIAL, const_cast<CKSTRING>("__CKAS_CKSprite3DSelfTestMaterial"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CKSprite3DSelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKSprite3DSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!sprite || !material || !mesh || !animation) {
        if (animation) context->DestroyObject(animation);
        if (mesh) context->DestroyObject(mesh);
        if (material) context->DestroyObject(material);
        if (sprite) context->DestroyObject(sprite);
        engine->DiscardModule(moduleName);
        error = "CKSprite3D self-test could not create temporary objects.";
        return false;
    }

    const bool ok = ExecuteCKSprite3DProbe(engine, probe, sprite, material, mesh, animation, false, "CKSprite3D surface probe", error) &&
                    ExecuteCKSprite3DProbe(engine, smallTransform, sprite, material, mesh, animation, true, "CKSprite3D small TransformMany probe", error);

    sprite->RemoveAllCallbacks();
    sprite->RemoveMesh(mesh);
    sprite->RemoveObjectAnimation(animation);
    sprite->SetMaterial(nullptr);
    context->DestroyObject(animation);
    context->DestroyObject(mesh);
    context->DestroyObject(material);
    context->DestroyObject(sprite);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKGridScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKGrid script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *gridType = engine->GetTypeInfoByDecl("CKGrid");
    if (!gridType) {
        error = "CKGrid self-test could not find the registered type.";
        return false;
    }
    asITypeInfo *layerType = engine->GetTypeInfoByDecl("CKLayer");
    if (!layerType) {
        error = "CKLayer self-test could not find the registered type.";
        return false;
    }
    if (layerType->GetMethodByDecl("CKSquare &GetSquareArray()") != nullptr ||
        layerType->GetMethodByDecl("void SetSquareArray(CKSquare &sqArray)") != nullptr) {
        error = "CKLayer self-test found stale reference square-array declarations.";
        return false;
    }
    if (layerType->GetMethodByDecl("NativePointer GetSquareArray()") == nullptr ||
        layerType->GetMethodByDecl("void SetSquareArray(NativePointer sqArray)") == nullptr) {
        error = "CKLayer self-test could not find pointer square-array declarations.";
        return false;
    }
    if (!layerType->GetMethodByDecl("void SetValue(int x, int y, NativeBuffer@ val)") ||
        !layerType->GetMethodByDecl("void GetValue(int x, int y, NativeBuffer@ val)") ||
        !layerType->GetMethodByDecl("bool SetValue2(int x, int y, NativeBuffer@ val)") ||
        !layerType->GetMethodByDecl("bool GetValue2(int x, int y, NativeBuffer@ val)") ||
        !layerType->GetMethodByDecl("bool SetSquareArray(NativeBuffer@ sqArray)") ||
        !layerType->GetMethodByDecl("NativeBuffer@ CopySquareArray()")) {
        error = "CKLayer self-test could not find NativeBuffer layer declarations.";
        return false;
    }
    if (gridType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        gridType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        gridType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        gridType->GetMethodByDecl("bool IsActive() const") != nullptr ||
        gridType->GetMethodByDecl("float GetHeightValidity() const") != nullptr ||
        gridType->GetMethodByDecl("int GetWidth() const") != nullptr ||
        gridType->GetMethodByDecl("int GetLength() const") != nullptr ||
        gridType->GetMethodByDecl("float Get2dCoordsFrom3dPos(const VxVector &in pos3d, int &out x, int &out y) const") != nullptr ||
        gridType->GetMethodByDecl("void Get3dPosFrom2dCoords(VxVector &out pos3d, int x, int z) const") != nullptr ||
        gridType->GetMethodByDecl("bool HasCompatibleClass(CK3dEntity@ entity) const") != nullptr ||
        gridType->GetMethodByDecl("int GetGridPriority() const") != nullptr ||
        gridType->GetMethodByDecl("CK_GRIDORIENTATION GetOrientationMode() const") != nullptr ||
        gridType->GetMethodByDecl("CKLayer@ GetLayer(int type) const") != nullptr ||
        gridType->GetMethodByDecl("CKLayer@ GetLayer(const string &in typeName) const") != nullptr ||
        gridType->GetMethodByDecl("int GetLayerCount() const") != nullptr ||
        gridType->GetMethodByDecl("CKLayer@ GetLayerByIndex(int index) const") != nullptr) {
        error = "CKGrid self-test found stale unsafe or const-drift declarations.";
        return false;
    }
    if (!gridType->GetMethodByDecl("bool IsActive()") ||
        !gridType->GetMethodByDecl("float GetHeightValidity()") ||
        !gridType->GetMethodByDecl("int GetWidth()") ||
        !gridType->GetMethodByDecl("int GetLength()") ||
        !gridType->GetMethodByDecl("float Get2dCoordsFrom3dPos(const VxVector &in pos3d, int &out x, int &out y)") ||
        !gridType->GetMethodByDecl("void Get3dPosFrom2dCoords(VxVector &out pos3d, int x, int z)") ||
        !gridType->GetMethodByDecl("bool HasCompatibleClass(CK3dEntity@ entity)") ||
        !gridType->GetMethodByDecl("int GetGridPriority()") ||
        !gridType->GetMethodByDecl("CK_GRIDORIENTATION GetOrientationMode()") ||
        !gridType->GetMethodByDecl("CKLayer@ AddLayer(const string &in typeName, int format = CKGRID_LAYER_FORMAT_NORMAL)") ||
        !gridType->GetMethodByDecl("CKLayer@ AddDefaultLayer(int format = CKGRID_LAYER_FORMAT_NORMAL)") ||
        !gridType->GetMethodByDecl("CKLayer@ GetLayer(int type)") ||
        !gridType->GetMethodByDecl("CKLayer@ GetLayer(const string &in typeName)") ||
        !gridType->GetMethodByDecl("int GetLayerCount()") ||
        !gridType->GetMethodByDecl("CKLayer@ GetLayerByIndex(int index)") ||
        !gridType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !gridType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !gridType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKGrid self-test could not find expected grid methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKGridSelfTest";
    const char *source =
        "bool CloseEnough(float a, float b) {\n"
        "  float d = a - b;\n"
        "  return d > -0.0001f && d < 0.0001f;\n"
        "}\n"
        "int ProbeCKGridSurface(CKGrid@ grid, CK3dEntity@ entity, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (grid is null || entity is null || mesh is null || animation is null) return 1;\n"
        "  CKObject@ asObject = grid;\n"
        "  CKSceneObject@ asSceneObject = grid;\n"
        "  CKBeObject@ asBeObject = grid;\n"
        "  CKRenderObject@ asRenderObject = grid;\n"
        "  CK3dEntity@ asEntity = grid;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null) return 2;\n"
        "  if (cast<CKGrid>(asObject) !is grid) return 3;\n"
        "  if (cast<CKGrid>(asRenderObject) !is grid) return 4;\n"
        "  if (cast<CKGrid>(asEntity) !is grid) return 5;\n"
        "  grid.IsActive();\n"
        "  grid.SetHeightValidity(1.25f);\n"
        "  if (!CloseEnough(grid.GetHeightValidity(), 1.25f)) return 6;\n"
        "  grid.SetDimensions(2, 3, 20.0f, 30.0f);\n"
        "  if (grid.GetWidth() != 2 || grid.GetLength() != 3) return 7;\n"
        "  int x = 0;\n"
        "  int y = 0;\n"
        "  grid.Get2dCoordsFrom3dPos(VxVector(0.0f, 0.0f, 0.0f), x, y);\n"
        "  VxVector pos;\n"
        "  grid.Get3dPosFrom2dCoords(pos, 0, 0);\n"
        "  grid.HasCompatibleClass(entity);\n"
        "  grid.SetGridPriority(4);\n"
        "  if (grid.GetGridPriority() != 4) return 8;\n"
        "  CK_GRIDORIENTATION orientation = grid.GetOrientationMode();\n"
        "  grid.SetOrientationMode(orientation);\n"
        "  if (grid.GetOrientationMode() != orientation) return 9;\n"
        "  CKLayer@ defaultLayer = grid.AddDefaultLayer();\n"
        "  CKLayer@ namedLayer = grid.AddLayer(\"__ckas_missing_grid_layer\");\n"
        "  if (defaultLayer !is null) {\n"
        "    NativePointer squares = defaultLayer.GetSquareArray();\n"
        "    if (squares.IsNull()) return 15;\n"
        "    defaultLayer.SetSquareArray(squares);\n"
        "    NativeBuffer@ value = NativeBuffer(4);\n"
        "    value.Write(int(123));\n"
        "    if (!defaultLayer.SetValue2(0, 0, value)) return 16;\n"
        "    NativeBuffer@ outValue = NativeBuffer(4);\n"
        "    if (!defaultLayer.GetValue2(0, 0, outValue)) return 17;\n"
        "    int readValue = 0;\n"
        "    outValue.Read(readValue);\n"
        "    if (readValue != 123) return 18;\n"
        "    NativeBuffer@ squareArray = NativeBuffer(24);\n"
        "    squareArray.Write(int(456));\n"
        "    if (!defaultLayer.SetSquareArray(squareArray)) return 19;\n"
        "    NativeBuffer@ copiedSquares = defaultLayer.CopySquareArray();\n"
        "    if (copiedSquares is null || copiedSquares.Size() != 24) return 20;\n"
        "    int copiedValue = 0;\n"
        "    copiedSquares.Read(copiedValue);\n"
        "    if (copiedValue != 456) return 21;\n"
        "  }\n"
        "  grid.GetLayerCount();\n"
        "  grid.GetLayerByIndex(0);\n"
        "  grid.GetLayer(0);\n"
        "  grid.GetLayer(\"__ckas_missing_grid_layer\");\n"
        "  grid.RemoveLayer(0);\n"
        "  grid.RemoveLayer(\"__ckas_missing_grid_layer\");\n"
        "  grid.RemoveAllLayers();\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  grid.TransformMany(transformed, source, 2);\n"
        "  if (grid.AddMesh(mesh) != CK_OK) return 10;\n"
        "  grid.SetCurrentMesh(mesh);\n"
        "  if (grid.GetCurrentMesh() !is mesh) return 11;\n"
        "  if (grid.RemoveMesh(mesh) != CK_OK) return 12;\n"
        "  grid.AddObjectAnimation(animation);\n"
        "  if (grid.GetObjectAnimationCount() < 1) return 13;\n"
        "  grid.RemoveObjectAnimation(animation);\n"
        "  if (!grid.GetAppData().IsNull()) return 14;\n"
        "  grid.SetAppData(NativePointer());\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKGridSmallTransform(CKGrid@ grid, CK3dEntity@ entity, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  grid.TransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKGrid self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckgrid-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKGrid self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKGrid self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKGridSurface(CKGrid@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCKGridSmallTransform(CKGrid@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    if (!probe || !smallTransform) {
        engine->DiscardModule(moduleName);
        error = "CKGrid self-test functions were not found.";
        return false;
    }

    CKGrid *grid = CKGrid::Cast(context->CreateObject(
        CKCID_GRID, const_cast<CKSTRING>("__CKAS_CKGridSelfTestGrid"), CK_OBJECTCREATION_DYNAMIC));
    CK3dEntity *entity = CK3dEntity::Cast(context->CreateObject(
        CKCID_3DOBJECT, const_cast<CKSTRING>("__CKAS_CKGridSelfTestEntity"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CKGridSelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKGridSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!grid || !entity || !mesh || !animation) {
        if (animation) context->DestroyObject(animation);
        if (mesh) context->DestroyObject(mesh);
        if (entity) context->DestroyObject(entity);
        if (grid) context->DestroyObject(grid);
        engine->DiscardModule(moduleName);
        error = "CKGrid self-test could not create temporary objects.";
        return false;
    }

    const bool ok = ExecuteCKGridProbe(engine, probe, grid, entity, mesh, animation, false, "CKGrid surface probe", error) &&
                    ExecuteCKGridProbe(engine, smallTransform, grid, entity, mesh, animation, true, "CKGrid small TransformMany probe", error);

    grid->RemoveAllCallbacks();
    grid->RemoveMesh(mesh);
    grid->RemoveObjectAnimation(animation);
    grid->RemoveAllLayers();
    context->DestroyObject(animation);
    context->DestroyObject(mesh);
    context->DestroyObject(entity);
    context->DestroyObject(grid);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKCurveScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKCurve script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *curveType = engine->GetTypeInfoByDecl("CKCurve");
    if (!curveType) {
        error = "CKCurve self-test could not find the registered type.";
        return false;
    }
    if (curveType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        curveType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        curveType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr) {
        error = "CKCurve self-test found stale unsafe inherited declarations.";
        return false;
    }
    if (!curveType->GetMethodByDecl("CKERROR AddControlPoint(CKCurvePoint@ pt)") ||
        !curveType->GetMethodByDecl("CKERROR InsertControlPoint(CKCurvePoint@ prev, CKCurvePoint@ pt)") ||
        !curveType->GetMethodByDecl("CKERROR RemoveControlPoint(CKCurvePoint@ pt, bool removeAll = false)") ||
        !curveType->GetMethodByDecl("CKERROR GetTangents(CKCurvePoint@ pt, VxVector&out input, VxVector&out output) const") ||
        !curveType->GetMethodByDecl("CKERROR SetTangents(CKCurvePoint@ pt, const VxVector&in input, const VxVector&in output)") ||
        !curveType->GetMethodByDecl("CKERROR GetTangents(int index, VxVector&out input, VxVector&out output) const") ||
        !curveType->GetMethodByDecl("CKERROR SetTangents(int index, const VxVector&in input, const VxVector&in output)") ||
        !curveType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !curveType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !curveType->GetMethodByDecl("bool SetRenderCallBack(CK_RENDEROBJECT_CALLBACK@ callback)") ||
        !curveType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKCurve self-test could not find expected curve methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKCurveSelfTest";
    const char *source =
        "bool CKCurveHasPoint(CKCurve@ curve, CKCurvePoint@ point) {\n"
        "  for (int i = 0; i < curve.GetControlPointCount(); ++i) {\n"
        "    if (curve.GetControlPoint(i) is point) return true;\n"
        "  }\n"
        "  return false;\n"
        "}\n"
        "int ProbeCKCurveSurface(CKCurve@ curve, CKCurve@ otherCurve, CKCurvePoint@ p0, CKCurvePoint@ p1, CKCurvePoint@ p2, CKCurvePoint@ otherPoint, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (curve is null || otherCurve is null || p0 is null || p1 is null || p2 is null || otherPoint is null || mesh is null || animation is null) return 1;\n"
        "  CKObject@ asObject = curve;\n"
        "  CKSceneObject@ asSceneObject = curve;\n"
        "  CKBeObject@ asBeObject = curve;\n"
        "  CKRenderObject@ asRenderObject = curve;\n"
        "  CK3dEntity@ asEntity = curve;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null) return 2;\n"
        "  if (cast<CKCurve>(asObject) !is curve) return 3;\n"
        "  if (cast<CKCurve>(asEntity) !is curve) return 4;\n"
        "  p0.SetPosition(VxVector(0.0f, 0.0f, 0.0f));\n"
        "  p1.SetPosition(VxVector(1.0f, 0.0f, 0.0f));\n"
        "  p2.SetPosition(VxVector(0.5f, 1.0f, 0.0f));\n"
        "  if (curve.AddControlPoint(p0) != CK_OK) return 5;\n"
        "  if (curve.AddControlPoint(p1) != CK_OK) return 6;\n"
        "  if (curve.InsertControlPoint(p0, p2) != CK_OK) return 7;\n"
        "  if (curve.GetControlPointCount() != 3) return 8;\n"
        "  if (!CKCurveHasPoint(curve, p0)) return 9;\n"
        "  if (!CKCurveHasPoint(curve, p2)) return 10;\n"
        "  if (p0.GetCurve() !is curve || p2.GetCurve() !is curve) return 11;\n"
        "  curve.Open();\n"
        "  if (!curve.IsOpen()) return 12;\n"
        "  curve.Close();\n"
        "  if (curve.IsOpen()) return 13;\n"
        "  curve.SetFittingCoeff(0.25f);\n"
        "  if (curve.GetFittingCoeff() < 0.24f || curve.GetFittingCoeff() > 0.26f) return 14;\n"
        "  if (curve.SetStepCount(8) != CK_OK) return 15;\n"
        "  if (curve.GetStepCount() != 8) return 16;\n"
        "  curve.SetColor(VxColor(0.1f, 0.2f, 0.3f, 1.0f));\n"
        "  curve.GetColor();\n"
        "  VxVector tin(0.0f, 1.0f, 0.0f);\n"
        "  VxVector tout(1.0f, 0.0f, 0.0f);\n"
        "  if (curve.SetTangents(0, tin, tout) != CK_OK) return 17;\n"
        "  if (tin.x != 0.0f || tin.y != 1.0f || tin.z != 0.0f || tout.x != 1.0f || tout.y != 0.0f || tout.z != 0.0f) return 18;\n"
        "  VxVector gotIn;\n"
        "  VxVector gotOut;\n"
        "  if (curve.GetTangents(0, gotIn, gotOut) != CK_OK) return 19;\n"
        "  if (curve.SetTangents(p2, tin, tout) != CK_OK) return 20;\n"
        "  if (tin.x != 0.0f || tin.y != 1.0f || tin.z != 0.0f || tout.x != 1.0f || tout.y != 0.0f || tout.z != 0.0f) return 21;\n"
        "  if (curve.GetTangents(p2, gotIn, gotOut) != CK_OK) return 22;\n"
        "  curve.Update();\n"
        "  VxVector pos;\n"
        "  VxVector dir;\n"
        "  if (curve.GetPos(0.5f, pos, dir) != CK_OK) return 23;\n"
        "  if (curve.GetLocalPos(0.5f, pos, dir) != CK_OK) return 24;\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  curve.TransformMany(transformed, source, 2);\n"
        "  if (curve.AddMesh(mesh) != CK_OK) return 25;\n"
        "  curve.SetCurrentMesh(mesh);\n"
        "  if (curve.GetCurrentMesh() !is mesh) return 26;\n"
        "  if (curve.RemoveMesh(mesh) != CK_OK) return 27;\n"
        "  curve.AddObjectAnimation(animation);\n"
        "  if (curve.GetObjectAnimationCount() < 1) return 28;\n"
        "  curve.RemoveObjectAnimation(animation);\n"
        "  if (!curve.GetAppData().IsNull()) return 29;\n"
        "  curve.SetAppData(NativePointer());\n"
        "  if (curve.RemoveControlPoint(p2) != CK_OK) return 30;\n"
        "  if (curve.GetControlPointCount() != 2) return 31;\n"
        "  if (curve.RemoveAllControlPoints() != CK_OK) return 32;\n"
        "  if (curve.GetControlPointCount() != 0) return 33;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKCurveSmallTransform(CKCurve@ curve, CKCurve@ otherCurve, CKCurvePoint@ p0, CKCurvePoint@ p1, CKCurvePoint@ p2, CKCurvePoint@ otherPoint, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  curve.TransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKCurveNullPoint(CKCurve@ curve, CKCurve@ otherCurve, CKCurvePoint@ p0, CKCurvePoint@ p1, CKCurvePoint@ p2, CKCurvePoint@ otherPoint, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  curve.AddControlPoint(null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKCurveCrossCurvePoint(CKCurve@ curve, CKCurve@ otherCurve, CKCurvePoint@ p0, CKCurvePoint@ p1, CKCurvePoint@ p2, CKCurvePoint@ otherPoint, CKMesh@ mesh, CKObjectAnimation@ animation) {\n"
        "  if (otherCurve.AddControlPoint(otherPoint) != CK_OK) return 1;\n"
        "  VxVector tin(0.0f, 1.0f, 0.0f);\n"
        "  VxVector tout(1.0f, 0.0f, 0.0f);\n"
        "  curve.SetTangents(otherPoint, tin, tout);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKCurve self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckcurve-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKCurve self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKCurve self-test script failed to build.";
        return false;
    }

    asIScriptFunction *surface = module->GetFunctionByDecl("int ProbeCKCurveSurface(CKCurve@, CKCurve@, CKCurvePoint@, CKCurvePoint@, CKCurvePoint@, CKCurvePoint@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCKCurveSmallTransform(CKCurve@, CKCurve@, CKCurvePoint@, CKCurvePoint@, CKCurvePoint@, CKCurvePoint@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *nullPoint = module->GetFunctionByDecl("int ProbeCKCurveNullPoint(CKCurve@, CKCurve@, CKCurvePoint@, CKCurvePoint@, CKCurvePoint@, CKCurvePoint@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *crossCurvePoint = module->GetFunctionByDecl("int ProbeCKCurveCrossCurvePoint(CKCurve@, CKCurve@, CKCurvePoint@, CKCurvePoint@, CKCurvePoint@, CKCurvePoint@, CKMesh@, CKObjectAnimation@)");
    if (!surface || !smallTransform || !nullPoint || !crossCurvePoint) {
        engine->DiscardModule(moduleName);
        error = "CKCurve self-test functions were not found.";
        return false;
    }

    CKCurve *curve = CKCurve::Cast(context->CreateObject(
        CKCID_CURVE, const_cast<CKSTRING>("__CKAS_CKCurveSelfTestCurve"), CK_OBJECTCREATION_DYNAMIC));
    CKCurve *otherCurve = CKCurve::Cast(context->CreateObject(
        CKCID_CURVE, const_cast<CKSTRING>("__CKAS_CKCurveSelfTestOtherCurve"), CK_OBJECTCREATION_DYNAMIC));
    CKCurvePoint *point0 = CKCurvePoint::Cast(context->CreateObject(
        CKCID_CURVEPOINT, const_cast<CKSTRING>("__CKAS_CKCurveSelfTestPoint0"), CK_OBJECTCREATION_DYNAMIC));
    CKCurvePoint *point1 = CKCurvePoint::Cast(context->CreateObject(
        CKCID_CURVEPOINT, const_cast<CKSTRING>("__CKAS_CKCurveSelfTestPoint1"), CK_OBJECTCREATION_DYNAMIC));
    CKCurvePoint *point2 = CKCurvePoint::Cast(context->CreateObject(
        CKCID_CURVEPOINT, const_cast<CKSTRING>("__CKAS_CKCurveSelfTestPoint2"), CK_OBJECTCREATION_DYNAMIC));
    CKCurvePoint *otherPoint = CKCurvePoint::Cast(context->CreateObject(
        CKCID_CURVEPOINT, const_cast<CKSTRING>("__CKAS_CKCurveSelfTestOtherPoint"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CKCurveSelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKCurveSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!curve || !otherCurve || !point0 || !point1 || !point2 || !otherPoint || !mesh || !animation) {
        if (animation) context->DestroyObject(animation);
        if (mesh) context->DestroyObject(mesh);
        if (otherPoint) context->DestroyObject(otherPoint);
        if (point2) context->DestroyObject(point2);
        if (point1) context->DestroyObject(point1);
        if (point0) context->DestroyObject(point0);
        if (otherCurve) context->DestroyObject(otherCurve);
        if (curve) context->DestroyObject(curve);
        engine->DiscardModule(moduleName);
        error = "CKCurve self-test could not create temporary objects.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCKCurveProbe(engine, surface, curve, otherCurve, point0, point1, point2, otherPoint, mesh, animation, false, "CKCurve surface probe", error) &&
                    ExecuteCKCurveProbe(engine, smallTransform, curve, otherCurve, point0, point1, point2, otherPoint, mesh, animation, true, "CKCurve small TransformMany probe", error) &&
                    ExecuteCKCurveProbe(engine, nullPoint, curve, otherCurve, point0, point1, point2, otherPoint, mesh, animation, true, "CKCurve null point probe", error) &&
                    ExecuteCKCurveProbe(engine, crossCurvePoint, curve, otherCurve, point0, point1, point2, otherPoint, mesh, animation, true, "CKCurve cross-curve point probe", error) &&
                    ExecuteCKCurveCopyNullProbe(engine, curveType, curve, dependencies, error);

    curve->RemoveAllCallbacks();
    curve->RemoveAllControlPoints();
    curve->RemoveMesh(mesh);
    curve->RemoveObjectAnimation(animation);
    otherCurve->RemoveAllCallbacks();
    otherCurve->RemoveAllControlPoints();
    context->DestroyObject(animation);
    context->DestroyObject(mesh);
    context->DestroyObject(otherPoint);
    context->DestroyObject(point2);
    context->DestroyObject(point1);
    context->DestroyObject(point0);
    context->DestroyObject(otherCurve);
    context->DestroyObject(curve);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKCurvePointScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKCurvePoint script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *pointType = engine->GetTypeInfoByDecl("CKCurvePoint");
    if (!pointType) {
        error = "CKCurvePoint self-test could not find the registered type.";
        return false;
    }
    if (pointType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        pointType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        pointType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        pointType->GetMethodByDecl("CKCurve@ GetCurve() const") != nullptr ||
        pointType->GetMethodByDecl("float GetLength() const") != nullptr ||
        pointType->GetMethodByDecl("void GetTangents(VxVector&out input, VxVector&out output) const") != nullptr) {
        error = "CKCurvePoint self-test found stale declarations.";
        return false;
    }
    if (!pointType->GetMethodByDecl("CKCurve@ GetCurve()") ||
        !pointType->GetMethodByDecl("float GetBias()") ||
        !pointType->GetMethodByDecl("float GetTension()") ||
        !pointType->GetMethodByDecl("float GetContinuity()") ||
        !pointType->GetMethodByDecl("bool IsLinear()") ||
        !pointType->GetMethodByDecl("bool IsTCB()") ||
        !pointType->GetMethodByDecl("float GetLength()") ||
        !pointType->GetMethodByDecl("void GetTangents(VxVector&out input, VxVector&out output)") ||
        !pointType->GetMethodByDecl("void SetTangents(const VxVector&in input, const VxVector&in output)") ||
        !pointType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !pointType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !pointType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKCurvePoint self-test could not find expected curve-point methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKCurvePointSelfTest";
    const char *source =
        "bool CloseEnough(float a, float b) {\n"
        "  float d = a - b;\n"
        "  return d > -0.0001f && d < 0.0001f;\n"
        "}\n"
        "int ProbeCKCurvePointSurface(CKCurve@ curve, CKCurvePoint@ point0, CKCurvePoint@ point1) {\n"
        "  if (curve is null || point0 is null || point1 is null) return 1;\n"
        "  CKObject@ asObject = point0;\n"
        "  CKSceneObject@ asSceneObject = point0;\n"
        "  CKBeObject@ asBeObject = point0;\n"
        "  CKRenderObject@ asRenderObject = point0;\n"
        "  CK3dEntity@ asEntity = point0;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null) return 2;\n"
        "  if (cast<CKCurvePoint>(asObject) !is point0) return 3;\n"
        "  if (cast<CKCurvePoint>(asEntity) !is point0) return 4;\n"
        "  point0.SetPosition(VxVector(0.0f, 0.0f, 0.0f));\n"
        "  point1.SetPosition(VxVector(1.0f, 0.0f, 0.0f));\n"
        "  if (curve.AddControlPoint(point0) != CK_OK) return 5;\n"
        "  if (curve.AddControlPoint(point1) != CK_OK) return 6;\n"
        "  if (point0.GetCurve() !is curve) return 7;\n"
        "  point0.SetBias(0.25f);\n"
        "  point0.SetTension(0.5f);\n"
        "  point0.SetContinuity(-0.25f);\n"
        "  if (!CloseEnough(point0.GetBias(), 0.25f)) return 8;\n"
        "  if (!CloseEnough(point0.GetTension(), 0.5f)) return 9;\n"
        "  if (!CloseEnough(point0.GetContinuity(), -0.25f)) return 10;\n"
        "  point0.SetLinear(true);\n"
        "  if (!point0.IsLinear()) return 11;\n"
        "  point0.SetLinear(false);\n"
        "  point0.UseTCB(true);\n"
        "  if (!point0.IsTCB()) return 12;\n"
        "  point0.UseTCB(false);\n"
        "  VxVector input(0.0f, 1.0f, 0.0f);\n"
        "  VxVector output(1.0f, 0.0f, 0.0f);\n"
        "  point0.SetTangents(input, output);\n"
        "  if (input.y != 1.0f || output.x != 1.0f) return 13;\n"
        "  VxVector gotIn;\n"
        "  VxVector gotOut;\n"
        "  point0.GetTangents(gotIn, gotOut);\n"
        "  curve.Update();\n"
        "  point0.NotifyUpdate();\n"
        "  point0.GetLength();\n"
        "  if (!point0.GetAppData().IsNull()) return 14;\n"
        "  point0.SetAppData(NativePointer());\n"
        "  if (curve.RemoveAllControlPoints() != CK_OK) return 15;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKCurvePoint self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckcurvepoint-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKCurvePoint self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKCurvePoint self-test script failed to build.";
        return false;
    }

    asIScriptFunction *surface = module->GetFunctionByDecl("int ProbeCKCurvePointSurface(CKCurve@, CKCurvePoint@, CKCurvePoint@)");
    if (!surface) {
        engine->DiscardModule(moduleName);
        error = "CKCurvePoint self-test functions were not found.";
        return false;
    }

    CKCurve *curve = CKCurve::Cast(context->CreateObject(
        CKCID_CURVE, const_cast<CKSTRING>("__CKAS_CKCurvePointSelfTestCurve"), CK_OBJECTCREATION_DYNAMIC));
    CKCurvePoint *point0 = CKCurvePoint::Cast(context->CreateObject(
        CKCID_CURVEPOINT, const_cast<CKSTRING>("__CKAS_CKCurvePointSelfTestPoint0"), CK_OBJECTCREATION_DYNAMIC));
    CKCurvePoint *point1 = CKCurvePoint::Cast(context->CreateObject(
        CKCID_CURVEPOINT, const_cast<CKSTRING>("__CKAS_CKCurvePointSelfTestPoint1"), CK_OBJECTCREATION_DYNAMIC));
    if (!curve || !point0 || !point1) {
        if (point1) context->DestroyObject(point1);
        if (point0) context->DestroyObject(point0);
        if (curve) context->DestroyObject(curve);
        engine->DiscardModule(moduleName);
        error = "CKCurvePoint self-test could not create temporary objects.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCKCurvePointProbe(engine, surface, curve, point0, point1, false, "CKCurvePoint surface probe", error) &&
                    ExecuteCKCurvePointCopyNullProbe(engine, pointType, point0, dependencies, error);

    curve->RemoveAllCallbacks();
    curve->RemoveAllControlPoints();
    context->DestroyObject(point1);
    context->DestroyObject(point0);
    context->DestroyObject(curve);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKCharacterScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKCharacter script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *characterType = engine->GetTypeInfoByDecl("CKCharacter");
    if (!characterType) {
        error = "CKCharacter self-test could not find the registered type.";
        return false;
    }
    if (characterType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        characterType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        characterType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr) {
        error = "CKCharacter self-test found stale unsafe inherited declarations.";
        return false;
    }
    if (!characterType->GetMethodByDecl("CKERROR AddBodyPart(CKBodyPart@ part)") ||
        !characterType->GetMethodByDecl("CKERROR RemoveBodyPart(CKBodyPart@ part)") ||
        !characterType->GetMethodByDecl("CKERROR SetRootBodyPart(CKBodyPart@ part)") ||
        !characterType->GetMethodByDecl("CKERROR AddAnimation(CKAnimation@ anim)") ||
        !characterType->GetMethodByDecl("CKERROR RemoveAnimation(CKAnimation@ anim)") ||
        !characterType->GetMethodByDecl("CKERROR SetActiveAnimation(CKAnimation@ anim)") ||
        !characterType->GetMethodByDecl("CKERROR SetNextActiveAnimation(CKAnimation@ anim, CKDWORD transitionMode, float warpLength = 0.0)") ||
        !characterType->GetMethodByDecl("CKERROR PlaySecondaryAnimation(CKAnimation@ anim, float startingFrame = 0.0, CK_SECONDARYANIMATION_FLAGS playFlags = CKSECONDARYANIMATION_ONESHOT, float warpLength = 5.0, int loopCount = 0)") ||
        !characterType->GetMethodByDecl("CKERROR StopSecondaryAnimation(CKAnimation@ anim, bool warp = false, float warpLength = 5.0)") ||
        !characterType->GetMethodByDecl("CKERROR StopSecondaryAnimation(CKAnimation@ anim, float warpLength)") ||
        !characterType->GetMethodByDecl("void GetWarperParameters(CKDWORD&out transitionMode, CKAnimation@&out animSrc, float&out frameSrc, CKAnimation@&out animDest, float&out frameDest)") ||
        !characterType->GetMethodByDecl("void GetEstimatedVelocity(float delta, VxVector&out velocity)") ||
        !characterType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !characterType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !characterType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKCharacter self-test could not find expected character methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKCharacterSelfTest";
    const char *source =
        "bool CKCharacterHasBodyPart(CKCharacter@ character, CKBodyPart@ part) {\n"
        "  for (int i = 0; i < character.GetBodyPartCount(); ++i) {\n"
        "    if (character.GetBodyPart(i) is part) return true;\n"
        "  }\n"
        "  return false;\n"
        "}\n"
        "bool CKCharacterHasAnimation(CKCharacter@ character, CKAnimation@ animation) {\n"
        "  for (int i = 0; i < character.GetAnimationCount(); ++i) {\n"
        "    if (character.GetAnimation(i) is animation) return true;\n"
        "  }\n"
        "  return false;\n"
        "}\n"
        "int ProbeCKCharacterSurface(CKCharacter@ character, CKBodyPart@ bodyPart, CKAnimation@ animation, CK3dEntity@ floorRef, CKMesh@ mesh, CKObjectAnimation@ objectAnimation) {\n"
        "  if (character is null || bodyPart is null || animation is null || floorRef is null || mesh is null || objectAnimation is null) return 1;\n"
        "  CKObject@ asObject = character;\n"
        "  CKSceneObject@ asSceneObject = character;\n"
        "  CKBeObject@ asBeObject = character;\n"
        "  CKRenderObject@ asRenderObject = character;\n"
        "  CK3dEntity@ asEntity = character;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null) return 2;\n"
        "  if (cast<CKCharacter>(asObject) !is character) return 3;\n"
        "  if (cast<CKCharacter>(asEntity) !is character) return 4;\n"
        "  if (character.AddBodyPart(bodyPart) != CK_OK) return 5;\n"
        "  if (!CKCharacterHasBodyPart(character, bodyPart)) return 6;\n"
        "  character.SetRootBodyPart(bodyPart);\n"
        "  character.GetRootBodyPart();\n"
        "  if (character.AddAnimation(animation) != CK_OK) return 7;\n"
        "  if (!CKCharacterHasAnimation(character, animation)) return 8;\n"
        "  character.SetActiveAnimation(animation);\n"
        "  character.GetActiveAnimation();\n"
        "  character.SetNextActiveAnimation(animation, 0, 0.0f);\n"
        "  character.GetNextActiveAnimation();\n"
        "  character.PlaySecondaryAnimation(animation);\n"
        "  character.GetSecondaryAnimationsCount();\n"
        "  character.GetSecondaryAnimation(0);\n"
        "  character.StopSecondaryAnimation(animation, false, 5.0f);\n"
        "  character.StopSecondaryAnimation(animation, 5.0f);\n"
        "  character.FlushSecondaryAnimations();\n"
        "  character.ProcessAnimation(0.0f);\n"
        "  character.SetAutomaticProcess(false);\n"
        "  if (character.IsAutomaticProcess()) return 9;\n"
        "  character.SetAutomaticProcess(true);\n"
        "  VxVector velocity;\n"
        "  character.GetEstimatedVelocity(1.0f, velocity);\n"
        "  character.SetFloorReferenceObject(floorRef);\n"
        "  character.GetFloorReferenceObject();\n"
        "  character.SetFloorReferenceObject(null);\n"
        "  character.SetAnimationLevelOfDetail(0.5f);\n"
        "  float lod = character.GetAnimationLevelOfDetail();\n"
        "  if (lod < 0.0f || lod > 1.0f) return 10;\n"
        "  CKDWORD transitionMode = 0;\n"
        "  CKAnimation@ src;\n"
        "  CKAnimation@ dest;\n"
        "  float frameSrc = 0.0f;\n"
        "  float frameDest = 0.0f;\n"
        "  character.GetWarperParameters(transitionMode, src, frameSrc, dest, frameDest);\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  source.Reset();\n"
        "  NativeBuffer@ transformed = NativeBuffer(24);\n"
        "  character.TransformMany(transformed, source, 2);\n"
        "  if (character.AddMesh(mesh) != CK_OK) return 11;\n"
        "  character.SetCurrentMesh(mesh);\n"
        "  if (character.GetCurrentMesh() !is mesh) return 12;\n"
        "  if (character.RemoveMesh(mesh) != CK_OK) return 13;\n"
        "  character.AddObjectAnimation(objectAnimation);\n"
        "  if (character.GetObjectAnimationCount() < 1) return 14;\n"
        "  character.RemoveObjectAnimation(objectAnimation);\n"
        "  if (!character.GetAppData().IsNull()) return 15;\n"
        "  character.SetAppData(NativePointer());\n"
        "  character.RemoveAnimation(animation);\n"
        "  character.RemoveBodyPart(bodyPart);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKCharacterSmallTransform(CKCharacter@ character, CKBodyPart@ bodyPart, CKAnimation@ animation, CK3dEntity@ floorRef, CKMesh@ mesh, CKObjectAnimation@ objectAnimation) {\n"
        "  NativeBuffer@ source = NativeBuffer(24);\n"
        "  source.Write(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  source.Write(VxVector(4.0f, 5.0f, 6.0f));\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  character.TransformMany(tooSmall, source, 2);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKCharacterNullBodyPart(CKCharacter@ character, CKBodyPart@ bodyPart, CKAnimation@ animation, CK3dEntity@ floorRef, CKMesh@ mesh, CKObjectAnimation@ objectAnimation) {\n"
        "  character.AddBodyPart(null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKCharacterNullAnimation(CKCharacter@ character, CKBodyPart@ bodyPart, CKAnimation@ animation, CK3dEntity@ floorRef, CKMesh@ mesh, CKObjectAnimation@ objectAnimation) {\n"
        "  character.AddAnimation(null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKCharacterNullSecondaryStop(CKCharacter@ character, CKBodyPart@ bodyPart, CKAnimation@ animation, CK3dEntity@ floorRef, CKMesh@ mesh, CKObjectAnimation@ objectAnimation) {\n"
        "  character.StopSecondaryAnimation(null, 5.0f);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKCharacter self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckcharacter-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKCharacter self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKCharacter self-test script failed to build.";
        return false;
    }

    asIScriptFunction *surface = module->GetFunctionByDecl("int ProbeCKCharacterSurface(CKCharacter@, CKBodyPart@, CKAnimation@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *smallTransform = module->GetFunctionByDecl("int ProbeCKCharacterSmallTransform(CKCharacter@, CKBodyPart@, CKAnimation@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *nullBodyPart = module->GetFunctionByDecl("int ProbeCKCharacterNullBodyPart(CKCharacter@, CKBodyPart@, CKAnimation@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *nullAnimation = module->GetFunctionByDecl("int ProbeCKCharacterNullAnimation(CKCharacter@, CKBodyPart@, CKAnimation@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    asIScriptFunction *nullSecondaryStop = module->GetFunctionByDecl("int ProbeCKCharacterNullSecondaryStop(CKCharacter@, CKBodyPart@, CKAnimation@, CK3dEntity@, CKMesh@, CKObjectAnimation@)");
    if (!surface || !smallTransform || !nullBodyPart || !nullAnimation || !nullSecondaryStop) {
        engine->DiscardModule(moduleName);
        error = "CKCharacter self-test functions were not found.";
        return false;
    }

    CKCharacter *character = CKCharacter::Cast(context->CreateObject(
        CKCID_CHARACTER, const_cast<CKSTRING>("__CKAS_CKCharacterSelfTestCharacter"), CK_OBJECTCREATION_DYNAMIC));
    CKBodyPart *bodyPart = CKBodyPart::Cast(context->CreateObject(
        CKCID_BODYPART, const_cast<CKSTRING>("__CKAS_CKCharacterSelfTestBodyPart"), CK_OBJECTCREATION_DYNAMIC));
    CKAnimation *animation = CKAnimation::Cast(context->CreateObject(
        CKCID_KEYEDANIMATION, const_cast<CKSTRING>("__CKAS_CKCharacterSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    CK3dEntity *floorRef = CK3dEntity::Cast(context->CreateObject(
        CKCID_3DOBJECT, const_cast<CKSTRING>("__CKAS_CKCharacterSelfTestFloorRef"), CK_OBJECTCREATION_DYNAMIC));
    CKMesh *mesh = CKMesh::Cast(context->CreateObject(
        CKCID_MESH, const_cast<CKSTRING>("__CKAS_CKCharacterSelfTestMesh"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *objectAnimation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKCharacterSelfTestObjectAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!character || !bodyPart || !animation || !floorRef || !mesh || !objectAnimation) {
        if (objectAnimation) context->DestroyObject(objectAnimation);
        if (mesh) context->DestroyObject(mesh);
        if (floorRef) context->DestroyObject(floorRef);
        if (animation) context->DestroyObject(animation);
        if (bodyPart) context->DestroyObject(bodyPart);
        if (character) context->DestroyObject(character);
        engine->DiscardModule(moduleName);
        error = "CKCharacter self-test could not create temporary objects.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCKCharacterProbe(engine, surface, character, bodyPart, animation, floorRef, mesh, objectAnimation, false, "CKCharacter surface probe", error) &&
                    ExecuteCKCharacterProbe(engine, smallTransform, character, bodyPart, animation, floorRef, mesh, objectAnimation, true, "CKCharacter small TransformMany probe", error) &&
                    ExecuteCKCharacterProbe(engine, nullBodyPart, character, bodyPart, animation, floorRef, mesh, objectAnimation, true, "CKCharacter null body-part probe", error) &&
                    ExecuteCKCharacterProbe(engine, nullAnimation, character, bodyPart, animation, floorRef, mesh, objectAnimation, true, "CKCharacter null animation probe", error) &&
                    ExecuteCKCharacterProbe(engine, nullSecondaryStop, character, bodyPart, animation, floorRef, mesh, objectAnimation, true, "CKCharacter null StopSecondaryAnimation probe", error) &&
                    ExecuteCKCharacterCopyNullProbe(engine, characterType, character, dependencies, error);

    character->RemoveAllCallbacks();
    character->FlushSecondaryAnimations();
    character->RemoveAnimation(animation);
    character->RemoveBodyPart(bodyPart);
    character->RemoveMesh(mesh);
    character->RemoveObjectAnimation(objectAnimation);
    character->SetFloorReferenceObject(nullptr);
    context->DestroyObject(objectAnimation);
    context->DestroyObject(mesh);
    context->DestroyObject(floorRef);
    context->DestroyObject(animation);
    context->DestroyObject(bodyPart);
    context->DestroyObject(character);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBodyPartScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBodyPart script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *bodyPartType = engine->GetTypeInfoByDecl("CKBodyPart");
    if (!bodyPartType) {
        error = "CKBodyPart self-test could not find the registered type.";
        return false;
    }
    if (bodyPartType->GetMethodByDecl("void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)") != nullptr ||
        bodyPartType->GetMethodByDecl("void TransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr ||
        bodyPartType->GetMethodByDecl("void InverseTransformMany(VxVector&out dest, const VxVector&in src, int count, CK3dEntity@ ref = null) const") != nullptr) {
        error = "CKBodyPart self-test found stale unsafe inherited declarations.";
        return false;
    }
    if (!bodyPartType->GetMethodByDecl("CKCharacter@ GetCharacter() const") ||
        !bodyPartType->GetMethodByDecl("void SetExclusiveAnimation(const CKAnimation@ anim)") ||
        !bodyPartType->GetMethodByDecl("CKAnimation@ GetExclusiveAnimation() const") ||
        !bodyPartType->GetMethodByDecl("void GetRotationJoint(CKIkJoint&out rotJoint) const") ||
        !bodyPartType->GetMethodByDecl("void SetRotationJoint(const CKIkJoint&in rotJoint)") ||
        !bodyPartType->GetMethodByDecl("CKERROR FitToJoint()") ||
        !bodyPartType->GetMethodByDecl("CK3dEntity@ opImplCast()") ||
        !bodyPartType->GetMethodByDecl("CK3dObject@ opImplCast()") ||
        !bodyPartType->GetMethodByDecl("void TransformMany(NativeBuffer@ dest, NativeBuffer@ src, int count, CK3dEntity@ ref = null) const") ||
        !bodyPartType->GetMethodByDecl("NativePointer GetAppData()")) {
        error = "CKBodyPart self-test could not find expected body-part methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBodyPartSelfTest";
    const char *source =
        "int ProbeCKBodyPartSurface(CKBodyPart@ part, CKCharacter@ character, CKAnimation@ animation) {\n"
        "  if (part is null || character is null || animation is null) return 1;\n"
        "  CKObject@ asObject = part;\n"
        "  CKSceneObject@ asSceneObject = part;\n"
        "  CKBeObject@ asBeObject = part;\n"
        "  CKRenderObject@ asRenderObject = part;\n"
        "  CK3dEntity@ asEntity = part;\n"
        "  CK3dObject@ as3dObject = part;\n"
        "  if (asObject is null || asSceneObject is null || asBeObject is null || asRenderObject is null || asEntity is null || as3dObject is null) return 2;\n"
        "  if (cast<CKBodyPart>(asObject) !is part) return 3;\n"
        "  if (cast<CKBodyPart>(asEntity) !is part) return 4;\n"
        "  if (cast<CKBodyPart>(as3dObject) !is part) return 5;\n"
        "  if (character.AddBodyPart(part) != CK_OK) return 6;\n"
        "  if (part.GetCharacter() !is character) return 7;\n"
        "  character.SetRootBodyPart(part);\n"
        "  part.SetExclusiveAnimation(animation);\n"
        "  if (part.GetExclusiveAnimation() !is animation) return 8;\n"
        "  part.SetExclusiveAnimation(null);\n"
        "  if (part.GetExclusiveAnimation() !is null) return 9;\n"
        "  if (!part.GetAppData().IsNull()) return 10;\n"
        "  part.SetAppData(NativePointer());\n"
        "  character.RemoveBodyPart(part);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBodyPart self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckbodypart-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBodyPart self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBodyPart self-test script failed to build.";
        return false;
    }

    asIScriptFunction *surface = module->GetFunctionByDecl("int ProbeCKBodyPartSurface(CKBodyPart@, CKCharacter@, CKAnimation@)");
    if (!surface) {
        engine->DiscardModule(moduleName);
        error = "CKBodyPart self-test functions were not found.";
        return false;
    }

    CKBodyPart *bodyPart = CKBodyPart::Cast(context->CreateObject(
        CKCID_BODYPART, const_cast<CKSTRING>("__CKAS_CKBodyPartSelfTestBodyPart"), CK_OBJECTCREATION_DYNAMIC));
    CKCharacter *character = CKCharacter::Cast(context->CreateObject(
        CKCID_CHARACTER, const_cast<CKSTRING>("__CKAS_CKBodyPartSelfTestCharacter"), CK_OBJECTCREATION_DYNAMIC));
    CKAnimation *animation = CKAnimation::Cast(context->CreateObject(
        CKCID_KEYEDANIMATION, const_cast<CKSTRING>("__CKAS_CKBodyPartSelfTestAnimation"), CK_OBJECTCREATION_DYNAMIC));
    if (!bodyPart || !character || !animation) {
        if (animation) context->DestroyObject(animation);
        if (character) context->DestroyObject(character);
        if (bodyPart) context->DestroyObject(bodyPart);
        engine->DiscardModule(moduleName);
        error = "CKBodyPart self-test could not create temporary objects.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCKBodyPartProbe(engine, surface, bodyPart, character, animation, false, "CKBodyPart surface probe", error) &&
                    ExecuteCKBodyPartCopyNullProbe(engine, bodyPartType, bodyPart, dependencies, error);

    bodyPart->RemoveAllCallbacks();
    bodyPart->SetExclusiveAnimation(nullptr);
    character->RemoveBodyPart(bodyPart);
    context->DestroyObject(animation);
    context->DestroyObject(character);
    context->DestroyObject(bodyPart);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBehaviorScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBehavior script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *behaviorType = engine->GetTypeInfoByDecl("CKBehavior");
    if (!behaviorType) {
        error = "CKBehavior self-test could not find the registered type.";
        return false;
    }
    if (!behaviorType->GetMethodByDecl("CKBehaviorPrototype@ GetPrototype()") ||
        behaviorType->GetMethodByDecl("CKBehaviorPrototype &GetPrototype()") ||
        !behaviorType->GetMethodByDecl("CKERROR GetInputParameterValue(int pos, ?&out value)") ||
        !behaviorType->GetMethodByDecl("CKERROR GetOutputParameterValue(int pos, ?&out value)") ||
        !behaviorType->GetMethodByDecl("CKERROR SetOutputParameterValue(int pos, ?&in value)") ||
        !behaviorType->GetMethodByDecl("CKERROR GetLocalParameterValue(int pos, ?&out value)") ||
        !behaviorType->GetMethodByDecl("CKERROR SetLocalParameterValue(int pos, ?&in value)") ||
        !behaviorType->GetMethodByDecl("void SetFunction(NativePointer fct)") ||
        !behaviorType->GetMethodByDecl("NativePointer GetFunction()") ||
        !behaviorType->GetMethodByDecl("void SetCallbackFunction(NativePointer fct)") ||
        !behaviorType->GetMethodByDecl("NativePointer GetInputParameterReadDataPtr(int pos)") ||
        !behaviorType->GetMethodByDecl("NativePointer GetOutputParameterWriteDataPtr(int pos)") ||
        !behaviorType->GetMethodByDecl("NativePointer GetLocalParameterReadDataPtr(int pos)") ||
        !behaviorType->GetMethodByDecl("NativePointer GetLocalParameterWriteDataPtr(int pos)") ||
        !behaviorType->GetMethodByDecl("CKERROR SetOutputParameterObject(int pos, CKObject@ obj)") ||
        !behaviorType->GetMethodByDecl("CKObject@ GetOutputParameterObject(int pos)") ||
        !behaviorType->GetMethodByDecl("CKERROR SetLocalParameterObject(int pos, CKObject@ obj)") ||
        !behaviorType->GetMethodByDecl("CKObject@ GetLocalParameterObject(int pos)")) {
        error = "CKBehavior self-test could not find expected object methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBehaviorSelfTest";
    const char *source =
        "int ProbeCKBehaviorSurface(CKBehavior@ behavior, CKBeObject@ owner, CKObject@ payload) {\n"
        "  if (behavior is null || owner is null || payload is null) return 2;\n"
        "  behavior.SetName(\"__CKAS_CKBehaviorSelfTest\");\n"
        "  if (behavior.GetName() != \"__CKAS_CKBehaviorSelfTest\") return 3;\n"
        "  if (behavior.GetInputCount() < 1 || behavior.GetOutputCount() < 1) return 6;\n"
        "  if (behavior.GetInput(0) is null || behavior.GetOutput(0) is null) return 7;\n"
        "  behavior.ActivateInput(0, true);\n"
        "  if (!behavior.IsInputActive(0)) return 8;\n"
        "  behavior.ActivateInput(0, false);\n"
        "  behavior.ActivateOutput(0, true);\n"
        "  if (!behavior.IsOutputActive(0)) return 9;\n"
        "  behavior.ActivateOutput(0, false);\n"
        "  int outValue = 41;\n"
        "  if (behavior.SetOutputParameterValue(0, outValue) != CK_OK) return 11;\n"
        "  outValue = 0;\n"
        "  if (behavior.GetOutputParameterValue(0, outValue) != CK_OK || outValue != 41) return 12;\n"
        "  string outText = \"behavior-output\";\n"
        "  if (behavior.SetOutputParameterValue(1, outText) != CK_OK) return 13;\n"
        "  outText = \"\";\n"
        "  if (behavior.GetOutputParameterValue(1, outText) != CK_OK || outText != \"behavior-output\") return 14;\n"
        "  int localValue = 23;\n"
        "  if (behavior.SetLocalParameterValue(0, localValue) != CK_OK) return 15;\n"
        "  localValue = 0;\n"
        "  if (behavior.GetLocalParameterValue(0, localValue) != CK_OK || localValue != 23) return 16;\n"
        "  string localText = \"behavior-local\";\n"
        "  if (behavior.SetLocalParameterValue(1, localText) != CK_OK) return 17;\n"
        "  localText = \"\";\n"
        "  if (behavior.GetLocalParameterValue(1, localText) != CK_OK || localText != \"behavior-local\") return 18;\n"
        "  if (behavior.GetOutputParameterWriteDataPtr(0).IsNull()) return 24;\n"
        "  if (behavior.GetLocalParameterReadDataPtr(0).IsNull()) return 25;\n"
        "  if (behavior.GetLocalParameterWriteDataPtr(0).IsNull()) return 26;\n"
        "  if (behavior.GetInputParameter(0) is null || behavior.GetOutputParameter(0) is null || behavior.GetLocalParameter(0) is null) return 27;\n"
        "  behavior.GetFlags();\n"
        "  behavior.GetType();\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKBehaviorFunctionPointers(CKBehavior@ behavior) {\n"
        "  if (behavior is null) return 2;\n"
        "  NativePointer empty;\n"
        "  behavior.SetFunction(empty);\n"
        "  behavior.SetCallbackFunction(empty);\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKBehaviorFunctionPointer(CKBehavior@ behavior) {\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  behavior.SetFunction(ptr);\n"
        "}\n"
        "void RejectCKBehaviorCallbackPointer(CKBehavior@ behavior) {\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  behavior.SetCallbackFunction(ptr);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBehavior self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckbehavior-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehavior self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehavior self-test script failed to build.";
        return false;
    }

    asIScriptFunction *pointerProbe = module->GetFunctionByDecl("int ProbeCKBehaviorFunctionPointers(CKBehavior@)");
    asIScriptFunction *rejectFunction = module->GetFunctionByDecl("void RejectCKBehaviorFunctionPointer(CKBehavior@)");
    asIScriptFunction *rejectCallback = module->GetFunctionByDecl("void RejectCKBehaviorCallbackPointer(CKBehavior@)");
    if (!pointerProbe || !rejectFunction || !rejectCallback) {
        engine->DiscardModule(moduleName);
        error = "CKBehavior self-test function pointer probes were not found.";
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR,
                                                                  const_cast<CKSTRING>("__CKAS_CKBehaviorFunctionSelfTest"),
                                                                  CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        engine->DiscardModule(moduleName);
        error = "CKBehavior self-test could not create a temporary behavior.";
        return false;
    }

    const bool ok = ExecuteCKBehaviorProbe(engine, pointerProbe, behavior, false, "CKBehavior function pointer clear probe", error) &&
                    ExecuteCKBehaviorProbe(engine, rejectFunction, behavior, true, "CKBehavior function pointer rejection probe", error) &&
                    ExecuteCKBehaviorProbe(engine, rejectCallback, behavior, true, "CKBehavior callback pointer rejection probe", error);

    context->DestroyObject(behavior);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBehaviorIOScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBehaviorIO script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *behaviorIoType = engine->GetTypeInfoByDecl("CKBehaviorIO");
    asITypeInfo *behaviorType = engine->GetTypeInfoByDecl("CKBehavior");
    if (!behaviorIoType || !behaviorType) {
        error = "CKBehaviorIO self-test could not find the registered behavior types.";
        return false;
    }
    if (!behaviorIoType->GetMethodByDecl("void SetType(int type)") ||
        !behaviorIoType->GetMethodByDecl("int GetType()") ||
        !behaviorIoType->GetMethodByDecl("void Activate(bool active = true)") ||
        !behaviorIoType->GetMethodByDecl("bool IsActive()") ||
        !behaviorIoType->GetMethodByDecl("CKBehavior@ GetOwner()") ||
        !behaviorIoType->GetMethodByDecl("void SetOwner(CKBehavior@ owner)") ||
        !behaviorIoType->GetMethodByDecl("NativePointer GetAppData()") ||
        !behaviorIoType->GetMethodByDecl("void SetAppData(NativePointer data)")) {
        error = "CKBehaviorIO self-test could not find expected object methods.";
        return false;
    }
    if (!behaviorType->GetMethodByDecl("CKBehaviorIO@ GetInput(int pos)") ||
        !behaviorType->GetMethodByDecl("CKBehaviorIO@ GetOutput(int pos)") ||
        !behaviorType->GetMethodByDecl("int GetInputPosition(CKBehaviorIO@ pbio)") ||
        !behaviorType->GetMethodByDecl("int GetOutputPosition(CKBehaviorIO@ pbio)")) {
        error = "CKBehaviorIO self-test could not find expected owner behavior methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBehaviorIOSelfTest";
    const char *source =
        "int ProbeCKBehaviorIOSurface(CKBehavior@ owner, CKBehaviorIO@ input, CKBehaviorIO@ output) {\n"
        "  if (owner is null || input is null || output is null) return 2;\n"
        "  if (input.GetOwner() !is owner || output.GetOwner() !is owner) return 3;\n"
        "  if (owner.GetInput(0) !is input) return 4;\n"
        "  if (owner.GetOutput(0) !is output) return 5;\n"
        "  if (owner.GetInputPosition(input) != 0 || owner.GetOutputPosition(output) != 0) return 6;\n"
        "  input.SetName(\"__CKAS_CKBehaviorIOInput\");\n"
        "  if (input.GetName() != \"__CKAS_CKBehaviorIOInput\") return 7;\n"
        "  int oldType = input.GetType();\n"
        "  input.SetType(oldType);\n"
        "  if (input.GetType() != oldType) return 8;\n"
        "  input.Activate(true);\n"
        "  if (!input.IsActive()) return 9;\n"
        "  input.Activate(false);\n"
        "  if (input.IsActive()) return 10;\n"
        "  NativePointer appData = input.GetAppData();\n"
        "  if (!appData.IsNull()) return 11;\n"
        "  input.SetAppData(appData);\n"
        "  if (input.CKGetObject(input.GetID()) !is input) return 12;\n"
        "  input.SetOwner(owner);\n"
        "  if (input.GetOwner() !is owner) return 13;\n"
        "  output.Activate();\n"
        "  if (!output.IsActive()) return 14;\n"
        "  output.Activate(false);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBehaviorIO self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckbehaviorio-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorIO self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorIO self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKBehaviorIOSurface(CKBehavior@, CKBehaviorIO@, CKBehaviorIO@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorIO self-test function was not found.";
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR,
                                                                   const_cast<CKSTRING>("__CKAS_CKBehaviorIOSelfTest"),
                                                                   CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorIO self-test could not create a temporary CKBehavior.";
        return false;
    }
    CKBehaviorIO *input = behavior->CreateInput(const_cast<CKSTRING>("__CKAS_CKBehaviorIOInput"));
    CKBehaviorIO *output = behavior->CreateOutput(const_cast<CKSTRING>("__CKAS_CKBehaviorIOOutput"));
    if (!input || !output) {
        context->DestroyObject(behavior);
        engine->DiscardModule(moduleName);
        error = "CKBehaviorIO self-test could not create temporary behavior IOs.";
        return false;
    }

    const bool ok = ExecuteCKBehaviorIOProbe(engine, probe, behavior, input, output, false, "CKBehaviorIO surface probe", error);

    context->DestroyObject(behavior);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBehaviorLinkScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBehaviorLink script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *linkType = engine->GetTypeInfoByDecl("CKBehaviorLink");
    asITypeInfo *behaviorType = engine->GetTypeInfoByDecl("CKBehavior");
    if (!linkType || !behaviorType) {
        error = "CKBehaviorLink self-test could not find the registered behavior types.";
        return false;
    }
    if (!linkType->GetMethodByDecl("CKERROR SetOutBehaviorIO(CKBehaviorIO@ ckbioin)") ||
        !linkType->GetMethodByDecl("CKERROR SetInBehaviorIO(CKBehaviorIO@ ckbioout)") ||
        !linkType->GetMethodByDecl("CKBehaviorIO@ GetOutBehaviorIO()") ||
        !linkType->GetMethodByDecl("CKBehaviorIO@ GetInBehaviorIO()") ||
        !linkType->GetMethodByDecl("int GetActivationDelay()") ||
        !linkType->GetMethodByDecl("void SetActivationDelay(int delay)") ||
        !linkType->GetMethodByDecl("void ResetActivationDelay()") ||
        !linkType->GetMethodByDecl("void SetInitialActivationDelay(int delay)") ||
        !linkType->GetMethodByDecl("int GetInitialActivationDelay()") ||
        !linkType->GetMethodByDecl("CKDWORD GetFlags()") ||
        !linkType->GetMethodByDecl("void SetFlags(CKDWORD flags)") ||
        !linkType->GetMethodByDecl("NativePointer GetAppData()") ||
        !linkType->GetMethodByDecl("void SetAppData(NativePointer data)")) {
        error = "CKBehaviorLink self-test could not find expected object methods.";
        return false;
    }
    if (!behaviorType->GetMethodByDecl("CKBehaviorLink@ GetSubBehaviorLink(int pos)") ||
        !behaviorType->GetMethodByDecl("CKERROR AddSubBehaviorLink(CKBehaviorLink@ cbkl)")) {
        error = "CKBehaviorLink self-test could not find expected owner behavior link methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBehaviorLinkSelfTest";
    const char *source =
        "int ProbeCKBehaviorLinkSurface(CKBehavior@ root, CKBehaviorLink@ link, CKBehaviorIO@ sourceOutput, CKBehaviorIO@ targetInput) {\n"
        "  if (root is null || link is null || sourceOutput is null || targetInput is null) return 2;\n"
        "  if (root.GetSubBehaviorLink(0) !is link) return 3;\n"
        "  if (link.GetInBehaviorIO() !is sourceOutput) return 4;\n"
        "  if (link.GetOutBehaviorIO() !is targetInput) return 5;\n"
        "  if (link.SetInBehaviorIO(sourceOutput) != CK_OK) return 6;\n"
        "  if (link.SetOutBehaviorIO(targetInput) != CK_OK) return 7;\n"
        "  if (link.GetInBehaviorIO() !is sourceOutput || link.GetOutBehaviorIO() !is targetInput) return 8;\n"
        "  link.SetName(\"__CKAS_CKBehaviorLinkSelfTest\");\n"
        "  if (link.GetName() != \"__CKAS_CKBehaviorLinkSelfTest\") return 9;\n"
        "  link.SetActivationDelay(2);\n"
        "  if (link.GetActivationDelay() != 2) return 10;\n"
        "  link.ResetActivationDelay();\n"
        "  link.SetInitialActivationDelay(1);\n"
        "  if (link.GetInitialActivationDelay() != 1) return 11;\n"
        "  CKDWORD flags = link.GetFlags();\n"
        "  link.SetFlags(flags);\n"
        "  if (link.GetFlags() != flags) return 12;\n"
        "  NativePointer appData = link.GetAppData();\n"
        "  if (!appData.IsNull()) return 13;\n"
        "  link.SetAppData(appData);\n"
        "  if (link.CKGetObject(link.GetID()) !is link) return 14;\n"
        "  if (link.GetClassID() != CKCID_BEHAVIORLINK) return 15;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBehaviorLink self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckbehaviorlink-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorLink self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorLink self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKBehaviorLinkSurface(CKBehavior@, CKBehaviorLink@, CKBehaviorIO@, CKBehaviorIO@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorLink self-test function was not found.";
        return false;
    }

    CKBehavior *root = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, const_cast<CKSTRING>("__CKAS_CKBehaviorLinkRoot"), CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *sourceBehavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, const_cast<CKSTRING>("__CKAS_CKBehaviorLinkSource"), CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *targetBehavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, const_cast<CKSTRING>("__CKAS_CKBehaviorLinkTarget"), CK_OBJECTCREATION_DYNAMIC));
    CKBehaviorLink *link = CKBehaviorLink::Cast(context->CreateObject(CKCID_BEHAVIORLINK, const_cast<CKSTRING>("__CKAS_CKBehaviorLinkSelfTest"), CK_OBJECTCREATION_DYNAMIC));
    if (!root || !sourceBehavior || !targetBehavior || !link) {
        if (link) context->DestroyObject(link);
        if (targetBehavior) context->DestroyObject(targetBehavior);
        if (sourceBehavior) context->DestroyObject(sourceBehavior);
        if (root) context->DestroyObject(root);
        engine->DiscardModule(moduleName);
        error = "CKBehaviorLink self-test could not create temporary graph objects.";
        return false;
    }

    root->UseGraph();
    CKBehaviorIO *sourceOutput = sourceBehavior->CreateOutput(const_cast<CKSTRING>("Out"));
    CKBehaviorIO *targetInput = targetBehavior->CreateInput(const_cast<CKSTRING>("In"));
    CKERROR err = root->AddSubBehavior(sourceBehavior);
    if (err == CK_OK) {
        err = root->AddSubBehavior(targetBehavior);
    }
    if (err == CK_OK && sourceOutput && targetInput) {
        err = link->SetInBehaviorIO(sourceOutput);
    }
    if (err == CK_OK) {
        err = link->SetOutBehaviorIO(targetInput);
    }
    if (err == CK_OK) {
        err = root->AddSubBehaviorLink(link);
    }
    if (err != CK_OK || !sourceOutput || !targetInput) {
        context->DestroyObject(link);
        context->DestroyObject(targetBehavior);
        context->DestroyObject(sourceBehavior);
        context->DestroyObject(root);
        engine->DiscardModule(moduleName);
        error = "CKBehaviorLink self-test could not prepare the temporary graph.";
        return false;
    }

    const bool ok = ExecuteCKBehaviorLinkProbe(engine, probe, root, link, sourceOutput, targetInput, false, "CKBehaviorLink surface probe", error);

    root->RemoveSubBehaviorLink(link);
    root->RemoveSubBehavior(sourceBehavior);
    root->RemoveSubBehavior(targetBehavior);
    context->DestroyObject(link);
    context->DestroyObject(targetBehavior);
    context->DestroyObject(sourceBehavior);
    context->DestroyObject(root);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKDataArrayScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKDataArray script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *arrayType = engine->GetTypeInfoByDecl("CKDataArray");
    if (!arrayType) {
        error = "CKDataArray self-test could not find the registered type.";
        return false;
    }
    if (!arrayType->GetMethodByDecl("bool SetElementValue(int i, int c, ?&in value)") ||
        !arrayType->GetMethodByDecl("bool GetElementValue(int i, int c, ?&out value)") ||
        !arrayType->GetMethodByDecl("NativePointer GetElement(int i, int c)") ||
        !arrayType->GetMethodByDecl("CKObject@ GetElementObject(int i, int c)") ||
        !arrayType->GetMethodByDecl("bool SetElementObject(int i, int c, CKObject@ obj)") ||
        !arrayType->GetMethodByDecl("CKDataRow &GetRow(int n)") ||
        !arrayType->GetMethodByDecl("CKDataRow &InsertRow(int n = -1)") ||
        !arrayType->GetMethodByDecl("CKDataRow &FindRow(int c, CK_COMPOPERATOR op, CKDWORD key, int size = 0, int startIndex = 0)") ||
        !arrayType->GetMethodByDecl("void RemoveRow(int row)") ||
        !arrayType->GetMethodByDecl("void Clear(bool params = true)") ||
        !arrayType->GetMethodByDecl("NativePointer GetAppData()") ||
        !arrayType->GetMethodByDecl("void SetAppData(NativePointer data)")) {
        error = "CKDataArray self-test could not find expected object methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKDataArraySelfTest";
    const char *source =
        "int ProbeCKDataArraySurface(CKDataArray@ array, CKObject@ object) {\n"
        "  if (array is null || object is null) return 2;\n"
        "  if (array.GetClassID() != CKCID_DATAARRAY) return 3;\n"
        "  if (array.GetColumnCount() != 3) return 4;\n"
        "  if (array.GetRowCount() != 1) return 5;\n"
        "  if (array.GetColumnName(0) != \"Int\" || array.GetColumnName(1) != \"Text\") return 6;\n"
        "  CKDWORD intValue = 42;\n"
        "  if (!array.SetElementValue(0, 0, intValue)) return 7;\n"
        "  CKDWORD intRead = 0;\n"
        "  if (!array.GetElementValue(0, 0, intRead) || intRead != 42) return 8;\n"
        "  string textValue = \"ckas\";\n"
        "  if (!array.SetElementValue(0, 1, textValue)) return 9;\n"
        "  string textRead;\n"
        "  if (!array.GetElementValue(0, 1, textRead) || textRead != \"ckas\") return 10;\n"
        "  if (!array.SetElementObject(0, 2, object)) return 11;\n"
        "  if (array.GetElementObject(0, 2) !is object) return 12;\n"
        "  NativePointer element = array.GetElement(0, 0);\n"
        "  if (element.IsNull()) return 13;\n"
        "  int nearest = -1;\n"
        "  if (!array.GetNearest(0, element, nearest) || nearest != 0) return 14;\n"
        "  if (array.GetRow(0).Size() != 3) return 15;\n"
        "  if (array.FindRow(0, CKEQUAL, intValue).Size() != 3) return 16;\n"
        "  if (array.InsertRow(-1).Size() != 3 || array.GetRowCount() != 2) return 17;\n"
        "  array.RemoveRow(1);\n"
        "  if (array.GetRowCount() != 1) return 18;\n"
        "  NativePointer appData = array.GetAppData();\n"
        "  if (!appData.IsNull()) return 19;\n"
        "  array.SetAppData(appData);\n"
        "  if (array.CKGetObject(array.GetID()) !is array) return 20;\n"
        "  array.Clear();\n"
        "  if (array.GetRowCount() != 0) return 21;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKDataArrayGetRowInvalid(CKDataArray@ array, CKObject@ object) {\n"
        "  array.GetRow(-1);\n"
        "}\n"
        "void RejectCKDataArrayInsertRowInvalid(CKDataArray@ array, CKObject@ object) {\n"
        "  array.InsertRow(-2);\n"
        "}\n"
        "void RejectCKDataArrayFindRowMissing(CKDataArray@ array, CKObject@ object) {\n"
        "  array.FindRow(0, CKEQUAL, 999999);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKDataArray self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckdataarray-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDataArray self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDataArray self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKDataArraySurface(CKDataArray@, CKObject@)");
    asIScriptFunction *invalidGetRow = module->GetFunctionByDecl("void RejectCKDataArrayGetRowInvalid(CKDataArray@, CKObject@)");
    asIScriptFunction *invalidInsertRow = module->GetFunctionByDecl("void RejectCKDataArrayInsertRowInvalid(CKDataArray@, CKObject@)");
    asIScriptFunction *missingFindRow = module->GetFunctionByDecl("void RejectCKDataArrayFindRowMissing(CKDataArray@, CKObject@)");
    if (!probe || !invalidGetRow || !invalidInsertRow || !missingFindRow) {
        engine->DiscardModule(moduleName);
        error = "CKDataArray self-test function was not found.";
        return false;
    }

    CKDataArray *array = CKDataArray::Cast(context->CreateObject(CKCID_DATAARRAY,
                                                                  const_cast<CKSTRING>("__CKAS_CKDataArraySelfTest"),
                                                                  CK_OBJECTCREATION_DYNAMIC));
    CKObject *object = context->CreateObject(CKCID_BEOBJECT,
                                             const_cast<CKSTRING>("__CKAS_CKDataArrayObject"),
                                             CK_OBJECTCREATION_DYNAMIC);
    if (!array || !object) {
        if (object) {
            context->DestroyObject(object);
        }
        if (array) {
            context->DestroyObject(array);
        }
        engine->DiscardModule(moduleName);
        error = "CKDataArray self-test could not create temporary objects.";
        return false;
    }

    array->InsertColumn(-1, CKARRAYTYPE_INT, const_cast<CKSTRING>("Int"));
    array->InsertColumn(-1, CKARRAYTYPE_STRING, const_cast<CKSTRING>("Text"));
    array->InsertColumn(-1, CKARRAYTYPE_OBJECT, const_cast<CKSTRING>("Object"));
    array->AddRow();

    const bool ok = ExecuteCKDataArrayProbe(engine, probe, array, object, false, "CKDataArray surface probe", error) &&
                    ExecuteCKDataArrayProbe(engine, invalidGetRow, array, object, true, "CKDataArray invalid GetRow probe", error) &&
                    ExecuteCKDataArrayProbe(engine, invalidInsertRow, array, object, true, "CKDataArray invalid InsertRow probe", error) &&
                    ExecuteCKDataArrayProbe(engine, missingFindRow, array, object, true, "CKDataArray missing FindRow probe", error);

    context->DestroyObject(object);
    context->DestroyObject(array);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPathManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKPathManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *pathManagerType = engine->GetTypeInfoByDecl("CKPathManager");
    if (!pathManagerType) {
        error = "CKPathManager self-test could not find the registered type.";
        return false;
    }
    if (pathManagerType->GetMethodByDecl("CKERROR RenameCategory(int catIdx, XString &in newName)") == nullptr ||
        pathManagerType->GetMethodByDecl("int GetPathIndex(int catIdx, XString &in path)") == nullptr ||
        pathManagerType->GetMethodByDecl("CKERROR RenamePath(int catIdx, int pathIdx, XString &in path)") == nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsAbsolute(XString &in file)") == nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsUNC(XString &in file)") == nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsURL(XString &in file)") == nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsFile(XString &in file)") == nullptr) {
        error = "CKPathManager self-test could not find expected mutable XString methods.";
        return false;
    }
    if (pathManagerType->GetMethodByDecl("CKERROR RenameCategory(int catIdx, const XString &in newName)") != nullptr ||
        pathManagerType->GetMethodByDecl("int GetPathIndex(int catIdx, const XString &in path)") != nullptr ||
        pathManagerType->GetMethodByDecl("CKERROR RenamePath(int catIdx, int pathIdx, const XString &in path)") != nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsAbsolute(const XString &in file)") != nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsUNC(const XString &in file)") != nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsURL(const XString &in file)") != nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsFile(const XString &in file)") != nullptr) {
        error = "CKPathManager self-test found stale const XString declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPathManagerSelfTest";
    const char *source =
        "void RemovePathCategoryIfPresent(CKPathManager@ paths, XString name) {\n"
        "  int existing = paths.GetCategoryIndex(name);\n"
        "  if (existing >= 0) paths.RemoveCategory(existing);\n"
        "}\n"
        "int FailPathProbe(CKPathManager@ paths, XString categoryName, XString renamedCategoryName, int code) {\n"
        "  if (paths !is null) {\n"
        "    RemovePathCategoryIfPresent(paths, categoryName);\n"
        "    RemovePathCategoryIfPresent(paths, renamedCategoryName);\n"
        "  }\n"
        "  return code;\n"
        "}\n"
        "int ProbeCKPathManagerSurface(CKContext@ ctx) {\n"
        "  if (ctx is null) return 2;\n"
        "  CKPathManager@ paths = ctx.GetPathManager();\n"
        "  if (paths is null) return 3;\n"
        "  XString categoryName(\"__CKAS_PathSmoke\");\n"
        "  XString renamedCategoryName(\"__CKAS_PathSmokeRenamed\");\n"
        "  RemovePathCategoryIfPresent(paths, categoryName);\n"
        "  RemovePathCategoryIfPresent(paths, renamedCategoryName);\n"
        "  int baseCount = paths.GetCategoryCount();\n"
        "  int category = paths.AddCategory(categoryName);\n"
        "  if (category < 0) return FailPathProbe(paths, categoryName, renamedCategoryName, 4);\n"
        "  if (paths.GetCategoryCount() != baseCount + 1) return FailPathProbe(paths, categoryName, renamedCategoryName, 5);\n"
        "  XString readCategory;\n"
        "  if (paths.GetCategoryName(category, readCategory) != CK_OK || readCategory != categoryName) return FailPathProbe(paths, categoryName, renamedCategoryName, 6);\n"
        "  if (paths.GetCategoryIndex(categoryName) != category) return FailPathProbe(paths, categoryName, renamedCategoryName, 7);\n"
        "  if (paths.RenameCategory(category, renamedCategoryName) != CK_OK) return FailPathProbe(paths, categoryName, renamedCategoryName, 8);\n"
        "  if (paths.GetCategoryIndex(renamedCategoryName) != category) return FailPathProbe(paths, categoryName, renamedCategoryName, 9);\n"
        "  XString firstPath(\"C:\\\\__ckas_path_probe_a\\\\\");\n"
        "  XString secondPath(\"C:\\\\__ckas_path_probe_b\\\\\");\n"
        "  XString renamedPath(\"C:\\\\__ckas_path_probe_c\\\\\");\n"
        "  int firstIndex = paths.AddPath(category, firstPath);\n"
        "  int secondIndex = paths.AddPath(category, secondPath);\n"
        "  if (firstIndex < 0 || secondIndex < 0) return FailPathProbe(paths, categoryName, renamedCategoryName, 10);\n"
        "  if (paths.GetPathCount(category) != 2) return FailPathProbe(paths, categoryName, renamedCategoryName, 11);\n"
        "  XString readPath;\n"
        "  if (paths.GetPathName(category, firstIndex, readPath) != CK_OK || readPath != firstPath) return FailPathProbe(paths, categoryName, renamedCategoryName, 12);\n"
        "  if (paths.GetPathIndex(category, secondPath) != secondIndex) return FailPathProbe(paths, categoryName, renamedCategoryName, 13);\n"
        "  if (paths.RenamePath(category, firstIndex, renamedPath) != CK_OK) return FailPathProbe(paths, categoryName, renamedCategoryName, 14);\n"
        "  if (paths.GetPathIndex(category, renamedPath) != firstIndex) return FailPathProbe(paths, categoryName, renamedCategoryName, 15);\n"
        "  if (paths.SwapPaths(category, firstIndex, secondIndex) != CK_OK) return FailPathProbe(paths, categoryName, renamedCategoryName, 16);\n"
        "  if (paths.GetPathName(category, secondIndex, readPath) != CK_OK || readPath != renamedPath) return FailPathProbe(paths, categoryName, renamedCategoryName, 17);\n"
        "  XString absolutePath(\"C:\\\\__ckas_path_probe\");\n"
        "  XString uncPath(\"\\\\\\\\server\\\\share\\\\file.txt\");\n"
        "  XString urlPath(\"http://example.invalid/file.txt\");\n"
        "  XString relativePath(\"relative\\\\file.txt\");\n"
        "  if (!paths.PathIsAbsolute(absolutePath)) return FailPathProbe(paths, categoryName, renamedCategoryName, 18);\n"
        "  if (!paths.PathIsUNC(uncPath)) return FailPathProbe(paths, categoryName, renamedCategoryName, 19);\n"
        "  if (!paths.PathIsURL(urlPath)) return FailPathProbe(paths, categoryName, renamedCategoryName, 20);\n"
        "  paths.PathIsFile(relativePath);\n"
        "  XString spaced(\"alpha beta\");\n"
        "  XString escaped;\n"
        "  paths.AddEscapedSpace(spaced, escaped);\n"
        "  XString unescaped;\n"
        "  paths.RemoveEscapedSpace(escaped, unescaped);\n"
        "  if (paths.RemovePath(category, secondIndex) != CK_OK) return FailPathProbe(paths, categoryName, renamedCategoryName, 21);\n"
        "  if (paths.RemovePath(category, firstIndex) != CK_OK) return FailPathProbe(paths, categoryName, renamedCategoryName, 22);\n"
        "  if (paths.RemoveCategory(category) != CK_OK) return FailPathProbe(paths, categoryName, renamedCategoryName, 23);\n"
        "  if (paths.GetCategoryIndex(renamedCategoryName) >= 0) return FailPathProbe(paths, categoryName, renamedCategoryName, 24);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPathManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ckpathmanager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPathManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPathManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPathManagerSurface(CKContext@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKPathManager self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKPathManager surface probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKInputManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKInputManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

#if CKVERSION == 0x13022002
    asITypeInfo *inputManagerType = engine->GetTypeInfoByDecl("CKInputManager");
    if (!inputManagerType) {
        error = "CKInputManager self-test could not find the registered type.";
        return false;
    }
    asIScriptFunction *isKeyDown = inputManagerType->GetMethodByDecl("bool IsKeyDown(CKDWORD key, CKDWORD &out stamp = void)");
    asIScriptFunction *isKeyToggled = inputManagerType->GetMethodByDecl("bool IsKeyToggled(CKDWORD key, CKDWORD &out stamp = void)");
    if (inputManagerType->GetMethodByDecl("int GetKeyName(CKDWORD key, string &out keyName)") == nullptr ||
        inputManagerType->GetMethodByDecl("CKDWORD GetKeyFromName(const string &in keyName)") == nullptr ||
        isKeyDown == nullptr ||
        isKeyToggled == nullptr ||
        inputManagerType->GetMethodByDecl("void GetMouseButtonsState(CKBYTE &out left, CKBYTE &out right, CKBYTE &out middle, CKBYTE &out extra)") == nullptr) {
        error = "CKInputManager self-test could not find expected key or mouse-state methods.";
        return false;
    }
    const std::string isKeyDownDecl = isKeyDown->GetDeclaration(false, false, true);
    const std::string isKeyToggledDecl = isKeyToggled->GetDeclaration(false, false, true);
    if (isKeyDownDecl.find("stamp = void") == std::string::npos ||
        isKeyToggledDecl.find("stamp = void") == std::string::npos ||
        inputManagerType->GetMethodByDecl("void GetMouseButtonsState(CKDWORD &out states)") != nullptr) {
        error = "CKInputManager self-test found stale key or mouse-state declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKInputManagerSelfTest";
    const char *source =
        "void ProbeCKInputManager(CKInputManager@ input) {\n"
        "  if (input is null) return;\n"
        "  string keyName;\n"
        "  input.GetKeyName(0, keyName);\n"
        "  input.GetKeyFromName(keyName);\n"
        "  input.IsKeyDown(0);\n"
        "  input.IsKeyToggled(0);\n"
        "  CKDWORD stamp = 0;\n"
        "  input.IsKeyDown(0, stamp);\n"
        "  input.IsKeyToggled(0, stamp);\n"
        "  CKBYTE left = 0;\n"
        "  CKBYTE right = 0;\n"
        "  CKBYTE middle = 0;\n"
        "  CKBYTE extra = 0;\n"
        "  input.GetMouseButtonsState(left, right, middle, extra);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKInputManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-input-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKInputManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKInputManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKInputManager(CKInputManager@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKInputManager self-test function was not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
#else
    return true;
#endif

    return true;
}

bool RunCKRenderManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKRenderManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *renderManagerType = engine->GetTypeInfoByDecl("CKRenderManager");
    if (!renderManagerType) {
        error = "CKRenderManager self-test could not find the registered type.";
        return false;
    }
    if (renderManagerType->GetMethodByDecl("VxDriverDesc &GetRenderDriverDescription(int driver)") == nullptr) {
        error = "CKRenderManager self-test could not find GetRenderDriverDescription.";
        return false;
    }
    if (renderManagerType->GetMethodByDecl("CKRenderContext@ CreateRenderContext(WIN_HANDLE window, int driver = 0, CKRECT &in rect = void, bool fullscreen = false, int bpp = -1, int zbpp = -1, int stencilBpp = -1, int refreshRate = 0)") == nullptr) {
        error = "CKRenderManager self-test could not find CreateRenderContext with optional rect.";
        return false;
    }
    if (renderManagerType->GetMethodByDecl("CKVertexBuffer@ CreateVertexBuffer()") == nullptr ||
        renderManagerType->GetMethodByDecl("void DestroyVertexBuffer(CKVertexBuffer@ vb)") == nullptr) {
        error = "CKRenderManager self-test could not find vertex buffer creation/destruction declarations.";
        return false;
    }
    asITypeInfo *vertexBufferType = engine->GetTypeInfoByDecl("CKVertexBuffer");
    if (!vertexBufferType) {
        error = "CKVertexBuffer self-test could not find the registered type.";
        return false;
    }
    if ((vertexBufferType->GetFlags() & asOBJ_NOCOUNT) != 0) {
        error = "CKVertexBuffer self-test found stale no-count type registration.";
        return false;
    }
    if (vertexBufferType->GetMethodByDecl("bool get_valid() const") == nullptr ||
        vertexBufferType->GetMethodByDecl("bool IsValid() const") == nullptr) {
        error = "CKVertexBuffer self-test could not find validity accessors.";
        return false;
    }
    if (vertexBufferType->GetMethodByDecl("bool Lock(CKRenderContext@ ctx, CKDWORD startVertex, CKDWORD vertexCount, VxDrawPrimitiveData &out data, CKLOCKFLAGS lockFlags = CK_LOCK_DEFAULT)") == nullptr) {
        error = "CKVertexBuffer self-test could not find guarded Lock declaration.";
        return false;
    }
    if (vertexBufferType->GetMethodByDecl("VxDrawPrimitiveData &Lock(CKRenderContext@ ctx, CKDWORD startVertex, CKDWORD vertexCount, CKLOCKFLAGS lockFlags = CK_LOCK_DEFAULT)") != nullptr) {
        error = "CKVertexBuffer self-test found stale reference-return Lock declaration.";
        return false;
    }
    if (vertexBufferType->GetMethodByDecl("bool Draw(CKRenderContext@ ctx, VXPRIMITIVETYPE pType, NativePointer indices, int indexCount, CKDWORD startVertex, CKDWORD vertexCount)") != nullptr) {
        error = "CKVertexBuffer self-test found stale NativePointer Draw declaration.";
        return false;
    }
    if (vertexBufferType->GetMethodByDecl("bool Draw(CKRenderContext@ ctx, VXPRIMITIVETYPE pType, CKDWORD startVertex, CKDWORD vertexCount)") == nullptr ||
        vertexBufferType->GetMethodByDecl("bool Draw(CKRenderContext@ ctx, VXPRIMITIVETYPE pType, const array<uint16>&in indices, CKDWORD startVertex, CKDWORD vertexCount)") == nullptr) {
        error = "CKVertexBuffer self-test could not find safe Draw declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKRenderManagerSelfTest";
    const char *source =
        "int ProbeRenderManagerInvalidDriver(CKContext@ ctx) {\n"
        "  CKRenderManager@ rm = ctx.GetRenderManager();\n"
        "  if (rm is null) return 1;\n"
        "  rm.GetRenderDriverDescription(-1);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeVertexBuffer(CKContext@ ctx) {\n"
        "  CKRenderManager@ rm = ctx.GetRenderManager();\n"
        "  if (rm is null) return 1;\n"
        "  CKVertexBuffer@ vb = rm.CreateVertexBuffer();\n"
        "  if (vb is null) return 2;\n"
        "  if (!vb.valid || !vb.IsValid()) { rm.DestroyVertexBuffer(vb); return 3; }\n"
        "  VxDrawPrimitiveData data;\n"
        "  bool locked = vb.Lock(null, 0, 1, data);\n"
        "  if (locked) { vb.Unlock(null); rm.DestroyVertexBuffer(vb); return 4; }\n"
        "  if (vb.Draw(null, VX_POINTLIST, 0, 0)) { rm.DestroyVertexBuffer(vb); return 5; }\n"
        "  array<uint16> indices = {0, 1, 2};\n"
        "  if (vb.Draw(null, VX_TRIANGLELIST, indices, 0, 3)) { rm.DestroyVertexBuffer(vb); return 6; }\n"
        "  rm.DestroyVertexBuffer(vb);\n"
        "  if (vb.valid || vb.IsValid()) return 7;\n"
        "  if (vb.Lock(null, 0, 1, data)) return 8;\n"
        "  if (vb.Draw(null, VX_POINTLIST, 0, 0)) return 9;\n"
        "  if (vb.Draw(null, VX_TRIANGLELIST, indices, 0, 3)) return 10;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKRenderManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-render-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRenderManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRenderManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeRenderManagerInvalidDriver(CKContext@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKRenderManager self-test function was not found.";
        return false;
    }
    asIScriptFunction *vertexProbe = module->GetFunctionByDecl("int ProbeVertexBuffer(CKContext@)");
    if (!vertexProbe) {
        engine->DiscardModule(moduleName);
        error = "CKVertexBuffer self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, true, "CKRenderManager invalid driver probe", error) &&
                    ExecuteCKParameterTypeDescProbe(engine, vertexProbe, context, false, "CKVertexBuffer guarded Lock probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPluginManagerScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPluginManager script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *pluginManagerType = engine->GetTypeInfoByDecl("CKPluginManager");
    if (!pluginManagerType) {
        error = "CKPluginManager self-test could not find the registered type.";
        return false;
    }
    if (pluginManagerType->GetMethodByDecl("int GetCategoryCount()") == nullptr ||
        pluginManagerType->GetMethodByDecl("int GetPluginDllCount()") == nullptr ||
        pluginManagerType->GetMethodByDecl("CKERROR Save(CKContext@ context, const string &in fileName, CKObjectArray@ objects, CKDWORD saveFlags, CKGUID&out readerGuid = void)") == nullptr ||
        pluginManagerType->GetMethodByDecl("CKERROR Load(CKContext@ context, const string &in fileName, CKObjectArray@ objects, CK_LOAD_FLAGS loadFlags, CKCharacter@ carac = null, CKGUID&out readerGuid = void)") == nullptr) {
        error = "CKPluginManager self-test could not find expected non-const/count/save declarations.";
        return false;
    }
    if (pluginManagerType->GetMethodByDecl("int GetCategoryCount() const") != nullptr ||
        pluginManagerType->GetMethodByDecl("int GetPluginDllCount() const") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKERROR Save(CKContext@ context, const string &in fileName, CKObjectArray@ objects, CK_LOAD_FLAGS saveFlags, CKGUID &readerGuid = void)") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKERROR Save(CKContext@ context, const string &in fileName, CKObjectArray@ objects, CKDWORD saveFlags, CKGUID &readerGuid = void)") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKERROR Load(CKContext@ context, const string &in fileName, CKObjectArray@ objects, CK_LOAD_FLAGS loadFlags, CKCharacter@ carac = null, CKGUID &readerGuid = void)") != nullptr) {
        error = "CKPluginManager self-test found stale count/save declarations.";
        return false;
    }
    if (pluginManagerType->GetMethodByDecl("CKPluginEntry FindComponent(CKGUID component, int catIdx = -1)") == nullptr ||
        pluginManagerType->GetMethodByDecl("CKPluginDll GetPluginDllInfo(int idx)") == nullptr ||
        pluginManagerType->GetMethodByDecl("CKPluginDll GetPluginDllInfo(const string &in name, int &out idx = void)") == nullptr ||
        pluginManagerType->GetMethodByDecl("CKPluginEntry GetPluginInfo(int catIdx, int pluginIdx)") == nullptr) {
        error = "CKPluginManager self-test could not find expected metadata value-return methods.";
        return false;
    }
    if (pluginManagerType->GetMethodByDecl("CKPluginEntry &FindComponent(CKGUID component, int catIdx = -1)") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKPluginDll &GetPluginDllInfo(int idx)") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKPluginDll &GetPluginDllInfo(const string &in name, int &out idx = void)") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKPluginEntry &GetPluginInfo(int catIdx, int pluginIdx)") != nullptr) {
        error = "CKPluginManager self-test found stale manager-owned reference metadata declarations.";
        return false;
    }
    if (pluginManagerType->GetMethodByDecl("CKBitmapReader@ GetBitmapReader(CKFileExtension &in ext, CKGUID &in preferredGUID = void)") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKSoundReader@ GetSoundReader(CKFileExtension &in ext, CKGUID &in preferredGUID = void)") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKModelReader@ GetModelReader(CKFileExtension &in ext, CKGUID &in preferredGUID = void)") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKMovieReader@ GetMovieReader(CKFileExtension &in ext, CKGUID &in preferredGUID = void)") != nullptr) {
        error = "CKPluginManager self-test found stale no-count reader acquisition declarations.";
        return false;
    }
    if (pluginManagerType->GetMethodByDecl("bool SetReaderOptionData(CKContext@ context, NativePointer data, CKParameterOut@ param, CKFileExtension ext, CKGUID &in guid = void)") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKParameterOut@ GetReaderOptionData(CKContext@ context, NativePointer data, CKFileExtension ext, CKGUID &in guid = void)") != nullptr) {
        error = "CKPluginManager self-test found stale raw reader option data declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPluginManagerSelfTest";
    const char *source =
        "int ProbeCKPluginManagerSurface() {\n"
        "  CKPluginManager@ manager = CKGetPluginManager();\n"
        "  if (manager is null) return 1;\n"
        "  if (manager.GetCategoryCount() < 0) return 2;\n"
        "  if (manager.GetPluginDllCount() < 0) return 3;\n"
        "  int idx = manager.GetCategoryIndex(\"__ckas_missing_category__\");\n"
        "  if (idx < -1) return 4;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKPluginManagerMissingDllByIndex() {\n"
        "  CKPluginManager@ manager = CKGetPluginManager();\n"
        "  CKPluginDll dll = manager.GetPluginDllInfo(-1);\n"
        "}\n"
        "void RejectCKPluginManagerMissingDllByName() {\n"
        "  CKPluginManager@ manager = CKGetPluginManager();\n"
        "  int idx = -2;\n"
        "  CKPluginDll dll = manager.GetPluginDllInfo(\"__ckas_missing_plugin_dll__\", idx);\n"
        "}\n"
        "void RejectCKPluginManagerMissingPluginInfo() {\n"
        "  CKPluginManager@ manager = CKGetPluginManager();\n"
        "  CKPluginEntry entry = manager.GetPluginInfo(-1, -1);\n"
        "}\n"
        "void RejectCKPluginManagerMissingComponent() {\n"
        "  CKPluginManager@ manager = CKGetPluginManager();\n"
        "  CKGUID guid;\n"
        "  CKPluginEntry entry = manager.FindComponent(guid, -99999);\n"
        "}\n"
        "void RejectCKPluginManagerInvalidCategoryName() {\n"
        "  CKPluginManager@ manager = CKGetPluginManager();\n"
        "  string name = manager.GetCategoryName(-1);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPluginManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-plugin-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPluginManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKPluginManagerSurface()");
    asIScriptFunction *missingDllByIndex = module->GetFunctionByDecl("void RejectCKPluginManagerMissingDllByIndex()");
    asIScriptFunction *missingDllByName = module->GetFunctionByDecl("void RejectCKPluginManagerMissingDllByName()");
    asIScriptFunction *missingPluginInfo = module->GetFunctionByDecl("void RejectCKPluginManagerMissingPluginInfo()");
    asIScriptFunction *missingComponent = module->GetFunctionByDecl("void RejectCKPluginManagerMissingComponent()");
    asIScriptFunction *invalidCategoryName = module->GetFunctionByDecl("void RejectCKPluginManagerInvalidCategoryName()");
    if (!probe || !missingDllByIndex || !missingDllByName || !missingPluginInfo || !missingComponent || !invalidCategoryName) {
        engine->DiscardModule(moduleName);
        error = "CKPluginManager self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPluginManager surface probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, missingDllByIndex, true, "CKPluginManager missing DLL index probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, missingDllByName, true, "CKPluginManager missing DLL name probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, missingPluginInfo, true, "CKPluginManager missing plugin info probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, missingComponent, true, "CKPluginManager missing component probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, invalidCategoryName, true, "CKPluginManager invalid category name probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBehaviorPrototypeScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKBehaviorPrototype script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *prototypeType = engine->GetTypeInfoByDecl("CKBehaviorPrototype");
    if (!prototypeType) {
        error = "CKBehaviorPrototype self-test could not find the registered type.";
        return false;
    }
    if (prototypeType->GetMethodByDecl("void SetFunction(NativePointer fct)") == nullptr ||
        prototypeType->GetMethodByDecl("NativePointer GetFunction()") == nullptr ||
        prototypeType->GetMethodByDecl("void SetBehaviorCallbackFct(NativePointer fct, CKDWORD callbackMask, NativePointer param)") == nullptr ||
        prototypeType->GetMethodByDecl("NativePointer GetBehaviorCallbackFct()") == nullptr) {
        error = "CKBehaviorPrototype self-test could not find expected guarded pointer methods.";
        return false;
    }
    if (prototypeType->GetMethodByDecl("int DeclareInParameter(const string &in name, CKGUID guidType)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareInParameter(const string &in name, CKGUID guidType, const string &in defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareInParameter(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareInParameter(const string &in name, CKGUID guidType, NativeBuffer@ defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareOutParameter(const string &in name, CKGUID guidType)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareOutParameter(const string &in name, CKGUID guidType, const string &in defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareOutParameter(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareOutParameter(const string &in name, CKGUID guidType, NativeBuffer@ defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareLocalParameter(const string &in name, CKGUID guidType)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareLocalParameter(const string &in name, CKGUID guidType, const string &in defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareLocalParameter(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareLocalParameter(const string &in name, CKGUID guidType, NativeBuffer@ defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareSetting(const string &in name, CKGUID guidType)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareSetting(const string &in name, CKGUID guidType, const string &in defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareSetting(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareSetting(const string &in name, CKGUID guidType, NativeBuffer@ defaultVal)") == nullptr) {
        error = "CKBehaviorPrototype self-test could not find expected guarded parameter declaration overloads.";
        return false;
    }
    asITypeInfo *parameterDescType = engine->GetTypeInfoByDecl("CKPARAMETER_DESC");
    if (!parameterDescType) {
        error = "CKBehaviorPrototype self-test could not find CKPARAMETER_DESC.";
        return false;
    }
    if (parameterDescType->GetMethodByDecl("NativePointer get_DefaultValue() const") == nullptr ||
        parameterDescType->GetMethodByDecl("void set_DefaultValue(NativePointer ptr)") == nullptr ||
        parameterDescType->GetMethodByDecl("bool SetDefaultValue(NativeBuffer@ data)") == nullptr) {
        error = "CKPARAMETER_DESC self-test could not find guarded DefaultValue methods.";
        return false;
    }
#if CKVERSION == 0x13022002
    asITypeInfo *behaviorIoDescType = engine->GetTypeInfoByDecl("CKBEHAVIORIO_DESC");
    if (!behaviorIoDescType) {
        error = "CKBehaviorPrototype self-test could not find CKBEHAVIORIO_DESC.";
        return false;
    }
    if (behaviorIoDescType->GetPropertyCount() != 0) {
        error = "CKBEHAVIORIO_DESC self-test found exposed raw properties.";
        return false;
    }
    if (behaviorIoDescType->GetMethodByDecl("CKDWORD get_Flags() const") == nullptr ||
        behaviorIoDescType->GetMethodByDecl("void set_Flags(CKDWORD value)") == nullptr ||
        behaviorIoDescType->GetMethodByDecl("string get_Name() const") == nullptr ||
        behaviorIoDescType->GetMethodByDecl("void set_Name(const string &in value)") == nullptr) {
        error = "CKBEHAVIORIO_DESC self-test could not find expected guarded accessors.";
        return false;
    }
    if (prototypeType->GetMethodByDecl("CKBEHAVIORIO_DESC &GetInIOList(int index)") == nullptr ||
        prototypeType->GetMethodByDecl("CKBEHAVIORIO_DESC &GetOutIOList(int index)") == nullptr ||
        prototypeType->GetMethodByDecl("CKPARAMETER_DESC &GetInParameterList(int index)") == nullptr ||
        prototypeType->GetMethodByDecl("CKPARAMETER_DESC &GetOutParameterList(int index)") == nullptr ||
        prototypeType->GetMethodByDecl("CKPARAMETER_DESC &GetLocalParameterList(int index)") == nullptr) {
        error = "CKBehaviorPrototype self-test could not find expected guarded list methods.";
        return false;
    }
#endif

    constexpr const char *moduleName = "__CKAS_CKBehaviorPrototypeSelfTest";
    const char *source =
        "int ProbeCKBehaviorPrototypeRuntime() {\n"
        "  CKBehaviorPrototype@ proto = CreateCKBehaviorPrototypeRunTime(\"__CKAS_CKBehaviorPrototypeRuntimeSelfTest\");\n"
        "  if (proto is null) return 1;\n"
        "  NativePointer empty;\n"
        "  proto.SetFunction(empty);\n"
        "  NativePointer f = proto.GetFunction();\n"
        "  if (!f.IsNull()) return 2;\n"
        "  proto.SetBehaviorCallbackFct(empty, CKCB_BEHAVIORALL, empty);\n"
        "  NativePointer cb = proto.GetBehaviorCallbackFct();\n"
        "  if (!cb.IsNull()) return 3;\n"
        "  NativeBuffer@ data = NativeBuffer(4);\n"
        "  if (data.WriteInt(123) != 4) return 4;\n"
        "  if (proto.DeclareInParameter(\"In\", CKPGUID_INT) < 0) return 5;\n"
        "  if (proto.DeclareInParameter(\"InText\", CKPGUID_STRING, \"text\") < 0) return 6;\n"
        "  if (proto.DeclareInParameter(\"InRaw\", CKPGUID_INT, empty, 0) < 0) return 7;\n"
        "  if (proto.DeclareInParameter(\"InBuffer\", CKPGUID_INT, data) < 0) return 8;\n"
        "  if (proto.DeclareOutParameter(\"Out\", CKPGUID_INT) < 0) return 9;\n"
        "  if (proto.DeclareOutParameter(\"OutText\", CKPGUID_STRING, \"text\") < 0) return 10;\n"
        "  if (proto.DeclareOutParameter(\"OutRaw\", CKPGUID_INT, empty, 0) < 0) return 11;\n"
        "  if (proto.DeclareOutParameter(\"OutBuffer\", CKPGUID_INT, data) < 0) return 12;\n"
        "  if (proto.DeclareLocalParameter(\"Local\", CKPGUID_INT) < 0) return 13;\n"
        "  if (proto.DeclareLocalParameter(\"LocalText\", CKPGUID_STRING, \"text\") < 0) return 14;\n"
        "  if (proto.DeclareLocalParameter(\"LocalRaw\", CKPGUID_INT, empty, 0) < 0) return 15;\n"
        "  if (proto.DeclareLocalParameter(\"LocalBuffer\", CKPGUID_INT, data) < 0) return 16;\n"
        "  if (proto.DeclareSetting(\"Setting\", CKPGUID_BOOL) < 0) return 17;\n"
        "  if (proto.DeclareSetting(\"SettingText\", CKPGUID_STRING, \"text\") < 0) return 18;\n"
        "  if (proto.DeclareSetting(\"SettingRaw\", CKPGUID_BOOL, empty, 0) < 0) return 19;\n"
        "  if (proto.DeclareSetting(\"SettingBuffer\", CKPGUID_BOOL, data) < 0) return 20;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKBehaviorPrototypeRawDefaultValue() {\n"
        "  CKBehaviorPrototype@ proto = CreateCKBehaviorPrototypeRunTime(\"__CKAS_CKBehaviorPrototypeRawRejectSelfTest\");\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  proto.DeclareInParameter(\"BadRaw\", CKPGUID_INT, ptr, 4);\n"
        "}\n"
        "void RejectCKBehaviorPrototypeFunctionPointer() {\n"
        "  CKBehaviorPrototype@ proto = CreateCKBehaviorPrototypeRunTime(\"__CKAS_CKBehaviorPrototypeFunctionRejectSelfTest\");\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  proto.SetFunction(ptr);\n"
        "}\n"
        "void RejectCKBehaviorPrototypeCallbackPointer() {\n"
        "  CKBehaviorPrototype@ proto = CreateCKBehaviorPrototypeRunTime(\"__CKAS_CKBehaviorPrototypeCallbackRejectSelfTest\");\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  NativePointer empty;\n"
        "  proto.SetBehaviorCallbackFct(ptr, CKCB_BEHAVIORALL, empty);\n"
        "}\n";
    const char *parameterDescSource =
        "int ProbeCKParameterDescDefaultValue() {\n"
        "  CKPARAMETER_DESC desc;\n"
        "  if (!desc.DefaultValue.IsNull() || desc.DefaultValueSize != 0) return 1;\n"
        "  NativeBuffer@ data = NativeBuffer(4);\n"
        "  if (data.WriteInt(123) != 4) return 2;\n"
        "  if (!desc.SetDefaultValue(data)) return 3;\n"
        "  if (desc.DefaultValue.IsNull() || desc.DefaultValueSize != 4) return 4;\n"
        "  CKPARAMETER_DESC copied(desc);\n"
        "  if (copied.DefaultValue.IsNull() || copied.DefaultValueSize != 4) return 5;\n"
        "  NativePointer empty;\n"
        "  desc.DefaultValue = empty;\n"
        "  if (!desc.DefaultValue.IsNull() || desc.DefaultValueSize != 0) return 6;\n"
        "  if (copied.DefaultValue.IsNull() || copied.DefaultValueSize != 4) return 7;\n"
        "  if (!desc.SetDefaultValue(null)) return 8;\n"
        "  if (!desc.DefaultValue.IsNull() || desc.DefaultValueSize != 0) return 9;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCKParameterDescRawDefaultValue() {\n"
        "  CKPARAMETER_DESC desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.DefaultValue = ptr;\n"
        "}\n";
#if CKVERSION == 0x13022002
    const char *listSource =
        "int ProbeCKBehaviorIoDescValue() {\n"
        "  CKBEHAVIORIO_DESC first;\n"
        "  if (first.Flags != 0) return 1;\n"
        "  if (first.Name != \"\") return 2;\n"
        "  first.Flags = 0x10000000;\n"
        "  first.Name = \"input\";\n"
        "  CKBEHAVIORIO_DESC copied(first);\n"
        "  if (copied.Flags != 0x10000000 || copied.Name != \"input\") return 3;\n"
        "  copied.Name = \"copy\";\n"
        "  copied.Flags = 0x20000000;\n"
        "  if (first.Name != \"input\" || first.Flags != 0x10000000) return 4;\n"
        "  if (copied.Name != \"copy\" || copied.Flags != 0x20000000) return 5;\n"
        "  CKBEHAVIORIO_DESC assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.Name != \"copy\" || assigned.Flags != 0x20000000) return 6;\n"
        "  assigned.Name = \"assigned\";\n"
        "  assigned.Flags = 0x40000000;\n"
        "  if (copied.Name != \"copy\" || copied.Flags != 0x20000000) return 7;\n"
        "  return assigned.Name == \"assigned\" && assigned.Flags == 0x40000000 ? 0 : 8;\n"
        "}\n"
        "int ProbeCKBehaviorPrototypeLists() {\n"
        "  CKBehaviorPrototype@ proto = CreateCKBehaviorPrototypeRunTime(\"__CKAS_CKBehaviorPrototypeListSelfTest\");\n"
        "  if (proto is null) return 1;\n"
        "  proto.DeclareInput(\"Input\");\n"
        "  proto.DeclareOutput(\"Output\");\n"
        "  proto.DeclareInParameter(\"InParam\", CKPGUID_INT);\n"
        "  proto.DeclareOutParameter(\"OutParam\", CKPGUID_INT);\n"
        "  proto.DeclareLocalParameter(\"LocalParam\", CKPGUID_INT);\n"
        "  if (proto.GetInputCount() > 0) {\n"
        "    CKDWORD flags = proto.GetInIOList(0).Flags;\n"
        "  }\n"
        "  if (proto.GetOutputCount() > 0) {\n"
        "    CKDWORD flags = proto.GetOutIOList(0).Flags;\n"
        "  }\n"
        "  if (proto.GetInParameterCount() > 0) {\n"
        "    int owner = proto.GetInParameterList(0).Owner;\n"
        "  }\n"
        "  if (proto.GetOutParameterCount() > 0) {\n"
        "    int owner = proto.GetOutParameterList(0).Owner;\n"
        "  }\n"
        "  if (proto.GetLocalParameterCount() > 0) {\n"
        "    int owner = proto.GetLocalParameterList(0).Owner;\n"
        "  }\n"
        "  return 0;\n"
        "}\n";
#endif

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBehaviorPrototype self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-behavior-prototype-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test could not add its script section.";
        return false;
    }
    r = module->AddScriptSection("ck-parameter-desc-self-test", parameterDescSource);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test could not add its CKPARAMETER_DESC script section.";
        return false;
    }
#if CKVERSION == 0x13022002
    r = module->AddScriptSection("ck-behavior-prototype-list-self-test", listSource);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test could not add its list script section.";
        return false;
    }
#endif
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKBehaviorPrototypeRuntime()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test function was not found.";
        return false;
    }
    asIScriptFunction *rawRejectProbe = module->GetFunctionByDecl("void RejectCKBehaviorPrototypeRawDefaultValue()");
    if (!rawRejectProbe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype raw default rejection self-test function was not found.";
        return false;
    }
    asIScriptFunction *functionRejectProbe = module->GetFunctionByDecl("void RejectCKBehaviorPrototypeFunctionPointer()");
    if (!functionRejectProbe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype function pointer rejection self-test function was not found.";
        return false;
    }
    asIScriptFunction *callbackRejectProbe = module->GetFunctionByDecl("void RejectCKBehaviorPrototypeCallbackPointer()");
    if (!callbackRejectProbe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype callback pointer rejection self-test function was not found.";
        return false;
    }
    asIScriptFunction *parameterDescProbe = module->GetFunctionByDecl("int ProbeCKParameterDescDefaultValue()");
    if (!parameterDescProbe) {
        engine->DiscardModule(moduleName);
        error = "CKPARAMETER_DESC DefaultValue self-test function was not found.";
        return false;
    }
    asIScriptFunction *parameterDescRawReject = module->GetFunctionByDecl("void RejectCKParameterDescRawDefaultValue()");
    if (!parameterDescRawReject) {
        engine->DiscardModule(moduleName);
        error = "CKPARAMETER_DESC raw DefaultValue rejection self-test function was not found.";
        return false;
    }
#if CKVERSION == 0x13022002
    asIScriptFunction *ioDescProbe = module->GetFunctionByDecl("int ProbeCKBehaviorIoDescValue()");
    if (!ioDescProbe) {
        engine->DiscardModule(moduleName);
        error = "CKBEHAVIORIO_DESC value self-test function was not found.";
        return false;
    }
    asIScriptFunction *listProbe = module->GetFunctionByDecl("int ProbeCKBehaviorPrototypeLists()");
    if (!listProbe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype list self-test function was not found.";
        return false;
    }
#endif

#if CKVERSION == 0x13022002
    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKBehaviorPrototype runtime probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, rawRejectProbe, true, "CKBehaviorPrototype raw default value rejection probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, functionRejectProbe, true, "CKBehaviorPrototype function pointer rejection probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, callbackRejectProbe, true, "CKBehaviorPrototype callback pointer rejection probe", error) &&
                    ExecuteNoArgIntProbe(engine, parameterDescProbe, false, "CKPARAMETER_DESC DefaultValue probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, parameterDescRawReject, true, "CKPARAMETER_DESC raw DefaultValue rejection probe", error) &&
                    ExecuteNoArgIntProbe(engine, ioDescProbe, false, "CKBEHAVIORIO_DESC value probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, listProbe, false, "CKBehaviorPrototype list probe", error);
    engine->DiscardModule(moduleName);
    return ok;
#else
    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKBehaviorPrototype runtime probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, rawRejectProbe, true, "CKBehaviorPrototype raw default value rejection probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, functionRejectProbe, true, "CKBehaviorPrototype function pointer rejection probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, callbackRejectProbe, true, "CKBehaviorPrototype callback pointer rejection probe", error) &&
                    ExecuteNoArgIntProbe(engine, parameterDescProbe, false, "CKPARAMETER_DESC DefaultValue probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, parameterDescRawReject, true, "CKPARAMETER_DESC raw DefaultValue rejection probe", error);
    engine->DiscardModule(moduleName);
    return ok;
#endif
}

bool RunCKMaterialScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKMaterial script self-test requires an AngelScript engine.";
        return false;
    }

#if CKVERSION != 0x26052005
    asITypeInfo *materialType = engine->GetTypeInfoByDecl("CKMaterial");
    if (!materialType) {
        error = "CKMaterial self-test could not find the registered type.";
        return false;
    }
    if (materialType->GetMethodByDecl("void SetCallback(NativePointer fct, NativePointer argument)") == nullptr ||
        materialType->GetMethodByDecl("NativePointer GetCallback(NativePointer &out argument = void)") == nullptr) {
        error = "CKMaterial self-test could not find expected guarded callback methods.";
        return false;
    }
    if (materialType->GetMethodByDecl("NativePointer GetCallback(NativePointer &out argument = void) const") != nullptr) {
        error = "CKMaterial self-test found stale const callback declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMaterialSelfTest";
    const char *source =
        "void ProbeCKMaterialCallback(CKMaterial@ material) {\n"
        "  if (material is null) return;\n"
        "  NativePointer argument;\n"
        "  NativePointer callback = material.GetCallback(argument);\n"
        "  NativePointer empty;\n"
        "  material.SetCallback(empty, empty);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMaterial self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-material-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMaterial self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMaterial self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKMaterialCallback(CKMaterial@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKMaterial self-test function was not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
#endif
    return true;
}

bool RunCKBitmapReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKBitmapReader script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *bitmapReaderType = engine->GetTypeInfoByDecl("CKBitmapReader");
    if (!bitmapReaderType) {
        error = "CKBitmapReader self-test could not find the registered type.";
        return false;
    }
    if (bitmapReaderType->GetMethodByDecl("int ReadMemory(NativeBuffer@ memory, CKBitmapProperties@ &out bp)") == nullptr ||
        bitmapReaderType->GetMethodByDecl("int SaveMemory(NativePointer &out memory, CKBitmapProperties@ bp)") == nullptr ||
        bitmapReaderType->GetMethodByDecl("void ReleaseMemory(NativePointer memory)") == nullptr) {
        error = "CKBitmapReader self-test could not find expected memory methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBitmapReaderSelfTest";
    const char *source =
        "void ProbeBitmapReaderSaveMemory(CKBitmapReader@ reader, CKBitmapProperties@ bp) {\n"
        "  NativePointer memory;\n"
        "  if (reader is null) return;\n"
        "  int result = reader.SaveMemory(memory, bp);\n"
        "  if (result > 0) reader.ReleaseMemory(memory);\n"
        "}\n"
        "void ProbeBitmapReaderNativeBuffer(CKBitmapReader@ reader) {\n"
        "  if (reader is null) return;\n"
        "  NativeBuffer@ memory = NativeBuffer(1);\n"
        "  CKBitmapProperties@ bp;\n"
        "  reader.ReadMemory(memory, bp);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBitmapReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-bitmap-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKBitmapPropertiesScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBitmapProperties script self-test requires a CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *propertiesType = engine->GetTypeInfoByDecl("CKBitmapProperties");
    if (!propertiesType) {
        error = "CKBitmapProperties self-test could not find the registered type.";
        return false;
    }
    for (asUINT i = 0; i < propertiesType->GetPropertyCount(); ++i) {
        const char *decl = propertiesType->GetPropertyDeclaration(i, true);
        const std::string propertyDecl = decl ? decl : "";
        if (propertyDecl == "NativePointer m_Data" ||
            propertyDecl == "uint64 m_Data" ||
            propertyDecl == "uint m_Data") {
            error = "CKBitmapProperties self-test found stale raw m_Data property.";
            return false;
        }
    }
    if (propertiesType->GetMethodByDecl("NativePointer get_m_Data() const") == nullptr ||
        propertiesType->GetMethodByDecl("void set_m_Data(NativePointer ptr)") == nullptr) {
        error = "CKBitmapProperties self-test could not find guarded m_Data accessors.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBitmapPropertiesSelfTest";
    const char *source =
        "int ProbeBitmapProperties(CKContext@ ctx) {\n"
        "  CKBitmapProperties@ borrowed = ctx.GetGlobalImagesSaveFormat();\n"
        "  if (borrowed is null) return 1;\n"
        "  CKBitmapProperties@ copy = CKCopyBitmapProperties(borrowed);\n"
        "  NativePointer empty;\n"
        "  borrowed.m_Data = empty;\n"
        "  NativePointer data = borrowed.m_Data;\n"
        "  if (copy !is null) CKDeleteBitmapProperties(copy);\n"
        "  CKDeleteBitmapProperties(null);\n"
        "  return 0;\n"
        "}\n"
        "void RejectBitmapPropertiesBorrowedDelete(CKContext@ ctx) {\n"
        "  CKBitmapProperties@ borrowed = ctx.GetGlobalImagesSaveFormat();\n"
        "  CKDeleteBitmapProperties(borrowed);\n"
        "}\n"
        "void RejectBitmapPropertiesDataWrite(CKContext@ ctx) {\n"
        "  CKBitmapProperties@ borrowed = ctx.GetGlobalImagesSaveFormat();\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  borrowed.m_Data = ptr;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBitmapProperties self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-bitmap-properties-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapProperties self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapProperties self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeBitmapProperties(CKContext@)");
    asIScriptFunction *rejectBorrowed = module->GetFunctionByDecl("void RejectBitmapPropertiesBorrowedDelete(CKContext@)");
    asIScriptFunction *rejectDataWrite = module->GetFunctionByDecl("void RejectBitmapPropertiesDataWrite(CKContext@)");
    if (!probe || !rejectBorrowed || !rejectDataWrite) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapProperties self-test functions were not found.";
        return false;
    }

    if (!ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKBitmapProperties probe", error) ||
        !ExecuteCKParameterTypeDescProbe(engine, rejectBorrowed, context, true, "CKBitmapProperties borrowed delete rejection probe", error) ||
        !ExecuteCKParameterTypeDescProbe(engine, rejectDataWrite, context, true, "CKBitmapProperties data write rejection probe", error)) {
        engine->DiscardModule(moduleName);
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKMoviePropertiesScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKMovieProperties script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *propertiesType = engine->GetTypeInfoByDecl("CKMovieProperties");
    if (!propertiesType) {
        error = "CKMovieProperties self-test could not find the registered type.";
        return false;
    }
    for (asUINT i = 0; i < propertiesType->GetPropertyCount(); ++i) {
        const char *decl = propertiesType->GetPropertyDeclaration(i, true);
        const std::string propertyDecl = decl ? decl : "";
        if (propertyDecl == "NativePointer m_Data" ||
            propertyDecl == "uint64 m_Data" ||
            propertyDecl == "uint m_Data") {
            error = "CKMovieProperties self-test found stale raw m_Data property.";
            return false;
        }
    }
    if (propertiesType->GetMethodByDecl("NativePointer get_m_Data() const") == nullptr ||
        propertiesType->GetMethodByDecl("void set_m_Data(NativePointer ptr)") == nullptr) {
        error = "CKMovieProperties self-test could not find guarded m_Data accessors.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMoviePropertiesSelfTest";
    const char *source =
        "int ProbeMovieProperties(CKMovieProperties@ properties) {\n"
        "  if (properties is null) return 1;\n"
        "  NativePointer empty;\n"
        "  properties.m_Data = empty;\n"
        "  if (!properties.m_Data.IsNull()) return 2;\n"
        "  return 0;\n"
        "}\n"
        "void RejectMoviePropertiesDataWrite(CKMovieProperties@ properties) {\n"
        "  if (properties is null) return;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  properties.m_Data = ptr;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMovieProperties self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-movie-properties-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMovieProperties self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMovieProperties self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeMovieProperties(CKMovieProperties@)");
    asIScriptFunction *rejectDataWrite = module->GetFunctionByDecl("void RejectMoviePropertiesDataWrite(CKMovieProperties@)");
    if (!probe || !rejectDataWrite) {
        engine->DiscardModule(moduleName);
        error = "CKMovieProperties self-test functions were not found.";
        return false;
    }

    auto executeProbe = [&](asIScriptFunction *function, bool expectException, const char *label) -> bool {
        CKMovieProperties properties;
        asIScriptContext *scriptContext = engine->RequestContext();
        if (!scriptContext) {
            error = std::string(label) + " could not create an execution context.";
            return false;
        }

        int r = scriptContext->Prepare(function);
        if (r >= 0) {
            r = scriptContext->SetArgObject(0, &properties);
        }
        if (r >= 0) {
            r = scriptContext->Execute();
        }

        bool ok = false;
        if (expectException) {
            ok = r == asEXECUTION_EXCEPTION;
            if (!ok) {
                error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
            }
        } else if (r == asEXECUTION_FINISHED) {
            const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
            ok = returnCode == 0;
            if (!ok) {
                error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
            }
        } else if (r == asEXECUTION_EXCEPTION) {
            const char *exception = scriptContext->GetExceptionString();
            error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
        } else {
            error = std::string(label) + " failed with code " + std::to_string(r) + ".";
        }

        scriptContext->Unprepare();
        engine->ReturnContext(scriptContext);
        return ok;
    };

    const bool ok = executeProbe(probe, false, "CKMovieProperties probe") &&
                    executeProbe(rejectDataWrite, true, "CKMovieProperties data write rejection probe");
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKMeshScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKMesh script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *meshType = engine->GetTypeInfoByDecl("CKMesh");
    if (!meshType) {
        error = "CKMesh self-test could not find the registered type.";
        return false;
    }
    if (!meshType->GetMethodByDecl("void TranslateVertices(const VxVector&in vector)") ||
        !meshType->GetMethodByDecl("void SetFaceMaterial(NativeBuffer@ faceIndices, int faceCount, CKMaterial@ mat)")) {
        error = "CKMesh self-test could not find NativeBuffer/vector mesh declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMeshSelfTest";
    const char *source =
        "void ProbeCKMeshTypedBuffers(CKMesh@ mesh, CKMaterial@ material) {\n"
        "  if (mesh is null) return;\n"
        "  mesh.SetFaceVertexIndex(0, 0, 1, 2);\n"
        "  mesh.SetVertexPosition(0, VxVector(0.0f, 0.0f, 0.0f));\n"
        "  mesh.SetVertexPosition(1, VxVector(1.0f, 0.0f, 0.0f));\n"
        "  mesh.SetVertexPosition(2, VxVector(0.0f, 1.0f, 0.0f));\n"
        "  mesh.TranslateVertices(VxVector(1.0f, 2.0f, 3.0f));\n"
        "  NativeBuffer@ faceIndices = NativeBuffer(4);\n"
        "  faceIndices.WriteInt(0);\n"
        "  mesh.SetFaceMaterial(faceIndices, 1, material);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMesh self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-mesh-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMesh self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMesh self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKMeshTypedBuffers(CKMesh@, CKMaterial@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKMesh self-test function was not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKTextureScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKTexture script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *textureType = engine->GetTypeInfoByDecl("CKTexture");
    if (!textureType) {
        error = "CKTexture self-test could not find the registered type.";
        return false;
    }
    if (textureType->GetMethodByDecl("bool IsInVideoMemory() const") == nullptr ||
        textureType->GetMethodByDecl("int GetMipmapCount() const") == nullptr ||
        textureType->GetMethodByDecl("VX_PIXELFORMAT GetVideoPixelFormat() const") == nullptr ||
        textureType->GetMethodByDecl("VX_PIXELFORMAT GetDesiredVideoFormat() const") == nullptr ||
        textureType->GetMethodByDecl("int GetRstTextureIndex() const") == nullptr ||
        textureType->GetMethodByDecl("CKObject@ opImplCast()") == nullptr ||
        textureType->GetMethodByDecl("CKBeObject@ opImplCast()") == nullptr ||
        textureType->GetMethodByDecl("bool GetVideoTextureDesc(VxImageDescEx&out desc)") == nullptr ||
        textureType->GetMethodByDecl("bool GetSystemTextureDesc(VxImageDescEx&out desc)") == nullptr ||
        textureType->GetMethodByDecl("bool GetUserMipMapLevel(int level, VxImageDescEx&out resultImage)") == nullptr ||
        textureType->GetMethodByDecl("bool SetSlotImage(int slot, NativeBuffer@ buffer, const VxImageDescEx&in desc)") == nullptr) {
        error = "CKTexture self-test could not find expected texture methods.";
        return false;
    }

    asITypeInfo *objectType = engine->GetTypeInfoByDecl("CKObject");
    if (!objectType) {
        error = "CKTexture self-test could not find CKObject.";
        return false;
    }
    if (objectType->GetMethodByDecl("CKTexture@ opCast()") == nullptr) {
        error = "CKTexture self-test could not find expected checked CKObject cast.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTextureSelfTest";
    const char *source =
        "int ProbeCKTextureSurface(CKContext@ ctx, CKObject@ object) {\n"
        "  if (ctx is null || object is null) return 1;\n"
        "  CKTexture@ texture = cast<CKTexture>(object);\n"
        "  if (texture is null) return 2;\n"
        "  CKObject@ asObject = texture;\n"
        "  CKBeObject@ asBeObject = texture;\n"
        "  if (asObject is null || asBeObject is null) return 3;\n"
        "  if (!texture.CreateImage(2, 2, 32, 0)) return 4;\n"
        "  if (texture.GetWidth() != 2 || texture.m_Width != 2) return 5;\n"
        "  if (texture.GetHeight() != 2 || texture.m_Height != 2) return 6;\n"
        "  VxImageDescEx imageDesc;\n"
        "  if (!texture.GetImageDesc(imageDesc)) return 23;\n"
        "  NativeBuffer@ slotData = NativeBuffer(16);\n"
        "  slotData.Fill(0x44, 16);\n"
        "  if (!texture.SetSlotImage(0, slotData, imageDesc)) return 24;\n"
        "  texture.IsInVideoMemory();\n"
        "  texture.UseMipmap(false);\n"
        "  texture.GetMipmapCount();\n"
        "  VX_PIXELFORMAT videoFormat = texture.GetVideoPixelFormat();\n"
        "  texture.SetDesiredVideoFormat(videoFormat);\n"
        "  texture.GetDesiredVideoFormat();\n"
        "  texture.GetRstTextureIndex();\n"
        "  VxImageDescEx systemDesc;\n"
        "  bool systemOk = texture.GetSystemTextureDesc(systemDesc);\n"
        "  if (systemOk && (!systemDesc.Image.IsNull() || !systemDesc.ColorMap.IsNull() || systemDesc.ColorMapEntries != 0 || systemDesc.BytesPerColorEntry != 0)) return 7;\n"
        "  VxImageDescEx videoDesc;\n"
        "  bool videoOk = texture.GetVideoTextureDesc(videoDesc);\n"
        "  if (videoOk && (!videoDesc.Image.IsNull() || !videoDesc.ColorMap.IsNull() || videoDesc.ColorMapEntries != 0 || videoDesc.BytesPerColorEntry != 0)) return 8;\n"
        "  VxImageDescEx mipDesc;\n"
        "  bool mipOk = texture.GetUserMipMapLevel(0, mipDesc);\n"
        "  if (mipOk && (!mipDesc.Image.IsNull() || !mipDesc.ColorMap.IsNull() || mipDesc.ColorMapEntries != 0 || mipDesc.BytesPerColorEntry != 0)) return 9;\n"
        "  CKRenderContext@ dev = ctx.GetPlayerRenderContext();\n"
        "  if (dev !is null) {\n"
        "    texture.SystemToVideoMemory(dev, false);\n"
        "    texture.SetAsCurrent(dev, false, 0);\n"
        "    VxRect rect(0.0f, 0.0f, 1.0f, 1.0f);\n"
        "    texture.CopyContext(dev, rect, rect, 0);\n"
        "    texture.FreeVideoMemory();\n"
        "  }\n"
        "  texture.ReleaseAllSlots();\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTexture self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-texture-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTexture self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTexture self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKTextureSurface(CKContext@, CKObject@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKTexture self-test function was not found.";
        return false;
    }

    CKTexture *texture = CKTexture::Cast(context->CreateObject(
        CKCID_TEXTURE, const_cast<CKSTRING>("__CKAS_CKTextureSelfTestTexture"), CK_OBJECTCREATION_DYNAMIC));
    if (!texture) {
        engine->DiscardModule(moduleName);
        error = "CKTexture self-test could not create a temporary texture.";
        return false;
    }

    const bool ok = ExecuteCKObjectProbe(engine, probe, context, texture, false, "CKTexture surface probe", error);

    texture->ReleaseAllSlots();
    context->DestroyObject(texture);
    engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBitmapSlotScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKBitmapSlot script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *slotType = engine->GetTypeInfoByDecl("CKBitmapSlot");
    if (!slotType) {
        error = "CKBitmapSlot self-test could not find the registered type.";
        return false;
    }
    if (slotType->GetMethodByDecl("bool HasDataBuffer() const") == nullptr) {
        error = "CKBitmapSlot self-test could not find HasDataBuffer.";
        return false;
    }
    if (slotType->GetMethodByDecl("NativePointer get_m_DataBuffer() const") != nullptr ||
        slotType->GetMethodByDecl("void set_m_DataBuffer(NativePointer ptr)") != nullptr) {
        error = "CKBitmapSlot self-test found stale raw data-buffer accessors.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBitmapSlotSelfTest";
    const char *source =
        "int ProbeBitmapSlot() {\n"
        "  CKBitmapSlot slot;\n"
        "  if (slot.HasDataBuffer()) return 1;\n"
        "  slot.m_FileName = \"source.bmp\";\n"
        "  slot.Allocate(2, 2, 32);\n"
        "  if (!slot.HasDataBuffer()) return 2;\n"
        "  CKBitmapSlot copied(slot);\n"
        "  if (copied.HasDataBuffer()) return 3;\n"
        "  if (copied.m_FileName != \"source.bmp\") return 4;\n"
        "  CKBitmapSlot assigned;\n"
        "  assigned.Allocate(1, 1, 32);\n"
        "  if (!assigned.HasDataBuffer()) return 5;\n"
        "  assigned = slot;\n"
        "  if (assigned.HasDataBuffer()) return 6;\n"
        "  if (assigned.m_FileName != \"source.bmp\") return 7;\n"
        "  slot.Flush();\n"
        "  if (slot.HasDataBuffer()) return 8;\n"
        "  slot.Allocate(1, 1, 32);\n"
        "  slot.Free();\n"
        "  if (slot.HasDataBuffer()) return 9;\n"
        "  if (slot.m_FileName != \"\") return 10;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBitmapSlot self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-bitmap-slot-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapSlot self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapSlot self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeBitmapSlot()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapSlot self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKBitmapSlot buffer probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKSoundReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKSoundReader script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *soundReaderType = engine->GetTypeInfoByDecl("CKSoundReader");
    if (!soundReaderType) {
        error = "CKSoundReader self-test could not find the registered type.";
        return false;
    }
    if (soundReaderType->GetMethodByDecl("CKERROR GetDataBuffer(NativePointer &out buf, int &out size)") == nullptr ||
        soundReaderType->GetMethodByDecl("CKERROR ReadMemory(NativePointer memory, int size)") == nullptr ||
        soundReaderType->GetMethodByDecl("CKERROR ReadMemory(NativeBuffer@ memory)") == nullptr) {
        error = "CKSoundReader self-test could not find expected memory methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKSoundReaderSelfTest";
    const char *source =
        "void ProbeSoundReaderMemory(CKSoundReader@ reader, NativePointer memory, int size) {\n"
        "  NativePointer buffer;\n"
        "  int bufferSize = 0;\n"
        "  if (reader is null) return;\n"
        "  reader.GetDataBuffer(buffer, bufferSize);\n"
        "  reader.ReadMemory(memory, size);\n"
        "}\n"
        "void ProbeSoundReaderNativeBuffer(CKSoundReader@ reader) {\n"
        "  if (reader is null) return;\n"
        "  NativeBuffer@ memory = NativeBuffer(1);\n"
        "  reader.ReadMemory(memory);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKSoundReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-sound-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSoundReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSoundReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKMovieReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKMovieReader script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *movieReaderType = engine->GetTypeInfoByDecl("CKMovieReader");
    if (!movieReaderType) {
        error = "CKMovieReader self-test could not find the registered type.";
        return false;
    }
    if (movieReaderType->GetMethodByDecl("int GetMovieFrameCount()") == nullptr ||
        movieReaderType->GetMethodByDecl("int GetMovieLength()") == nullptr ||
        movieReaderType->GetMethodByDecl("CKERROR ReadFrame(int f, CKMovieProperties@ &out prop)") == nullptr) {
        error = "CKMovieReader self-test could not find expected reader methods.";
        return false;
    }
    if (movieReaderType->GetMethodByDecl("int GetMovieFrameCount() const") != nullptr ||
        movieReaderType->GetMethodByDecl("int GetMovieLength() const") != nullptr) {
        error = "CKMovieReader self-test found stale const frame-count declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMovieReaderSelfTest";
    const char *source =
        "void ProbeMovieReaderSurface(CKMovieReader@ reader) {\n"
        "  CKMovieProperties@ prop;\n"
        "  if (reader is null) return;\n"
        "  reader.GetMovieFrameCount();\n"
        "  reader.GetMovieLength();\n"
        "  reader.OpenFile(\"\");\n"
        "  reader.OpenMemory(\"\");\n"
        "  reader.OpenAsynchronousFile(\"\");\n"
        "  reader.ReadFrame(0, prop);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMovieReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-movie-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMovieReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMovieReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKModelReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKModelReader script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *modelReaderType = engine->GetTypeInfoByDecl("CKModelReader");
    if (!modelReaderType) {
        error = "CKModelReader self-test could not find the registered type.";
        return false;
    }
    if (modelReaderType->GetMethodByDecl("CKERROR Load(CKContext@ context, const string &in filename, CKObjectArray@ objArray, CKDWORD loadFlags, CKCharacter@ carac = null)") == nullptr ||
        modelReaderType->GetMethodByDecl("CKERROR Save(CKContext@ context, const string &in filename, CKObjectArray@ objArray, CKDWORD saveFlags)") == nullptr) {
        error = "CKModelReader self-test could not find expected load/save methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKModelReaderSelfTest";
    const char *source =
        "void ProbeModelReaderSurface(CKModelReader@ reader, CKContext@ ctx, CKObjectArray@ objects, CKCharacter@ carac) {\n"
        "  if (reader is null) return;\n"
        "  reader.Load(ctx, \"\", objects, 0);\n"
        "  reader.Load(ctx, \"\", objects, 0, carac);\n"
        "  reader.Save(ctx, \"\", objects, 0);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKModelReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-model-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKModelReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKModelReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKDataReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKDataReader script self-test requires an AngelScript engine.";
        return false;
    }

    const char *readerTypes[] = {
        "CKDataReader",
        "CKModelReader",
        "CKBitmapReader",
        "CKSoundReader",
        "CKMovieReader",
    };
    for (const char *readerTypeName : readerTypes) {
        asITypeInfo *readerType = engine->GetTypeInfoByDecl(readerTypeName);
        if (!readerType) {
            error = std::string("CKDataReader self-test could not find registered type ") + readerTypeName + ".";
            return false;
        }
        if (readerType->GetMethodByDecl("CKPluginInfo GetReaderInfo()") == nullptr) {
            error = std::string("CKDataReader self-test could not find value GetReaderInfo on ") + readerTypeName + ".";
            return false;
        }
        if (readerType->GetMethodByDecl("CKPluginInfo &GetReaderInfo()") != nullptr) {
            error = std::string("CKDataReader self-test found stale reference GetReaderInfo on ") + readerTypeName + ".";
            return false;
        }
        if (readerType->GetMethodByDecl("void Release()") != nullptr) {
            error = std::string("CKDataReader self-test found stale raw Release on ") + readerTypeName + ".";
            return false;
        }
    }

    asITypeInfo *dataReaderType = engine->GetTypeInfoByDecl("CKDataReader");
    if (!dataReaderType ||
        dataReaderType->GetMethodByDecl("CKBitmapReader@ opCast()") == nullptr ||
        dataReaderType->GetMethodByDecl("CKModelReader@ opCast()") == nullptr ||
        dataReaderType->GetMethodByDecl("CKSoundReader@ opCast()") == nullptr ||
        dataReaderType->GetMethodByDecl("CKMovieReader@ opCast()") == nullptr ||
        dataReaderType->GetMethodByDecl("const CKBitmapReader@ opCast() const") == nullptr ||
        dataReaderType->GetMethodByDecl("const CKModelReader@ opCast() const") == nullptr ||
        dataReaderType->GetMethodByDecl("const CKSoundReader@ opCast() const") == nullptr ||
        dataReaderType->GetMethodByDecl("const CKMovieReader@ opCast() const") == nullptr) {
        error = "CKDataReader self-test could not find expected checked derived cast declarations.";
        return false;
    }

    const char *derivedReaderTypes[] = {
        "CKModelReader",
        "CKBitmapReader",
        "CKSoundReader",
        "CKMovieReader",
    };
    for (const char *readerTypeName : derivedReaderTypes) {
        asITypeInfo *readerType = engine->GetTypeInfoByDecl(readerTypeName);
        if (!readerType ||
            readerType->GetMethodByDecl("CKDataReader@ opImplCast()") == nullptr ||
            readerType->GetMethodByDecl("const CKDataReader@ opImplCast() const") == nullptr) {
            error = std::string("CKDataReader self-test could not find base cast declarations on ") + readerTypeName + ".";
            return false;
        }
    }

    constexpr const char *moduleName = "__CKAS_CKDataReaderSelfTest";
    const char *source =
        "void ProbeDataReaderInfo(CKDataReader@ data, CKModelReader@ model) {\n"
        "  if (data !is null) {\n"
        "    CKPluginInfo info = data.GetReaderInfo();\n"
        "    info.m_Version = info.m_Version;\n"
        "  }\n"
        "  if (model !is null) {\n"
        "    CKPluginInfo modelInfo = model.GetReaderInfo();\n"
        "    modelInfo.m_Version = modelInfo.m_Version;\n"
        "    CKDataReader@ modelBase = model;\n"
        "    if (modelBase is null) return;\n"
        "  }\n"
        "  if (data !is null) {\n"
        "    CKBitmapReader@ bitmap = cast<CKBitmapReader>(data);\n"
        "    CKModelReader@ modelReader = cast<CKModelReader>(data);\n"
        "    CKSoundReader@ sound = cast<CKSoundReader>(data);\n"
        "    CKMovieReader@ movie = cast<CKMovieReader>(data);\n"
        "    if (bitmap !is null) bitmap.GetReaderInfo();\n"
        "    if (modelReader !is null) modelReader.GetReaderInfo();\n"
        "    if (sound !is null) sound.GetReaderInfo();\n"
        "    if (movie !is null) movie.GetReaderInfo();\n"
        "  }\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKDataReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-data-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDataReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDataReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *keyType = engine->GetTypeInfoByDecl("CKKey");
    if (!keyType) {
        error = "CKKey self-test could not find the registered type.";
        return false;
    }
    if (keyType->GetMethodByDecl("CKRotationKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKPositionKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKTCBRotationKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKTCBPositionKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKBezierPositionKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKMorphKey opConv() const") != nullptr) {
        error = "CKKey self-test found stale unsafe base-to-derived value conversion.";
        return false;
    }
    if (keyType->GetMethodByDecl("float GetTime()") == nullptr) {
        error = "CKKey self-test could not find expected non-const GetTime declaration.";
        return false;
    }
    if (keyType->GetMethodByDecl("float GetTime() const") != nullptr) {
        error = "CKKey self-test found stale const GetTime declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKKeySelfTest";
    const char *source =
        "int ProbeKey() {\n"
        "  CKKey key;\n"
        "  if (key.GetTime() != 0.0f) return 1;\n"
        "  key.SetTime(1.25f);\n"
        "  if (key.GetTime() != 1.25f) return 2;\n"
        "  CKKey copy(key);\n"
        "  if (copy.GetTime() != 1.25f) return 3;\n"
        "  CKKey assigned;\n"
        "  assigned = key;\n"
        "  if (assigned.GetTime() != 1.25f) return 4;\n"
        "  assigned.TimeStep = 2.5f;\n"
        "  if (assigned.GetTime() != 2.5f) return 5;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPositionKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPositionKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *keyType = engine->GetTypeInfoByDecl("CKKey");
    asITypeInfo *positionKeyType = engine->GetTypeInfoByDecl("CKPositionKey");
    if (!keyType || !positionKeyType) {
        error = "CKPositionKey self-test could not find key types.";
        return false;
    }
    if (positionKeyType->GetMethodByDecl("CKTCBPositionKey opConv() const") != nullptr ||
        positionKeyType->GetMethodByDecl("CKBezierPositionKey opConv() const") != nullptr) {
        error = "CKPositionKey self-test found stale unsafe position-key down-conversion.";
        return false;
    }
    if (positionKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr) {
        error = "CKPositionKey self-test could not find safe CKKey value conversion.";
        return false;
    }
    if (positionKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        positionKeyType->GetMethodByDecl("const VxVector& GetPosition()") == nullptr ||
        positionKeyType->GetMethodByDecl("bool Compare(CKPositionKey&in key, float threshold)") == nullptr) {
        error = "CKPositionKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (positionKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        positionKeyType->GetMethodByDecl("const VxVector& GetPosition() const") != nullptr ||
        positionKeyType->GetMethodByDecl("bool Compare(CKPositionKey&in key, float threshold) const") != nullptr) {
        error = "CKPositionKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPositionKeySelfTest";
    const char *source =
        "int ProbePositionKey() {\n"
        "  VxVector pos(1.0f, 2.0f, 3.0f);\n"
        "  CKPositionKey key(1.25f, pos);\n"
        "  if (pos.x != 1.0f || pos.y != 2.0f || pos.z != 3.0f) return 80;\n"
        "  if (key.GetTime() != 1.25f) return 1;\n"
        "  VxVector got = key.GetPosition();\n"
        "  if (got.x != 1.0f || got.y != 2.0f || got.z != 3.0f) return 2;\n"
        "  CKPositionKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 3;\n"
        "  copy.Pos.x = 4.0f;\n"
        "  if (key.Compare(copy, 0.0f)) return 4;\n"
        "  CKPositionKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 5;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 6;\n"
        "  key.SetTime(2.5f);\n"
        "  if (key.GetTime() != 2.5f) return 7;\n"
        "  VxVector moved(7.0f, 8.0f, 9.0f);\n"
        "  key.SetPosition(moved);\n"
        "  if (moved.x != 7.0f || moved.y != 8.0f || moved.z != 9.0f) return 81;\n"
        "  got = key.GetPosition();\n"
        "  if (got.x != 7.0f || got.y != 8.0f || got.z != 9.0f) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPositionKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-position-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPositionKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPositionKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbePositionKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKPositionKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPositionKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKRotationKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKRotationKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *keyType = engine->GetTypeInfoByDecl("CKKey");
    asITypeInfo *rotationKeyType = engine->GetTypeInfoByDecl("CKRotationKey");
    if (!keyType || !rotationKeyType) {
        error = "CKRotationKey self-test could not find key types.";
        return false;
    }
    if (rotationKeyType->GetMethodByDecl("CKTCBRotationKey opConv() const") != nullptr) {
        error = "CKRotationKey self-test found stale unsafe rotation-key down-conversion.";
        return false;
    }
    if (rotationKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr) {
        error = "CKRotationKey self-test could not find safe CKKey value conversion.";
        return false;
    }
    if (rotationKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        rotationKeyType->GetMethodByDecl("const VxQuaternion& GetRotation()") == nullptr ||
        rotationKeyType->GetMethodByDecl("bool Compare(CKRotationKey&in key, float threshold)") == nullptr) {
        error = "CKRotationKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (rotationKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        rotationKeyType->GetMethodByDecl("const VxQuaternion& GetRotation() const") != nullptr ||
        rotationKeyType->GetMethodByDecl("bool Compare(CKRotationKey&in key, float threshold) const") != nullptr) {
        error = "CKRotationKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKRotationKeySelfTest";
    const char *source =
        "int ProbeRotationKey() {\n"
        "  VxQuaternion rot(0.0f, 0.0f, 0.0f, 1.0f);\n"
        "  CKRotationKey key(1.25f, rot);\n"
        "  if (rot.x != 0.0f || rot.y != 0.0f || rot.z != 0.0f || rot.w != 1.0f) return 80;\n"
        "  if (key.GetTime() != 1.25f) return 1;\n"
        "  VxQuaternion got = key.GetRotation();\n"
        "  if (got.x != 0.0f || got.y != 0.0f || got.z != 0.0f || got.w != 1.0f) return 2;\n"
        "  CKRotationKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 3;\n"
        "  copy.Rot.w = 0.5f;\n"
        "  if (key.Compare(copy, 0.0f)) return 4;\n"
        "  CKRotationKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 5;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 6;\n"
        "  key.SetTime(2.5f);\n"
        "  if (key.GetTime() != 2.5f) return 7;\n"
        "  VxQuaternion moved(0.1f, 0.2f, 0.3f, 0.4f);\n"
        "  key.SetRotation(moved);\n"
        "  if (moved.x != 0.1f || moved.y != 0.2f || moved.z != 0.3f || moved.w != 0.4f) return 81;\n"
        "  got = key.GetRotation();\n"
        "  if (got.x != 0.1f || got.y != 0.2f || got.z != 0.3f || got.w != 0.4f) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKRotationKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-rotation-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRotationKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRotationKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeRotationKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKRotationKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKRotationKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKTCBPositionKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKTCBPositionKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *tcbPositionKeyType = engine->GetTypeInfoByDecl("CKTCBPositionKey");
    if (!tcbPositionKeyType) {
        error = "CKTCBPositionKey self-test could not find the registered type.";
        return false;
    }
    if (tcbPositionKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr ||
        tcbPositionKeyType->GetMethodByDecl("CKPositionKey opImplConv() const") == nullptr) {
        error = "CKTCBPositionKey self-test could not find safe base value conversions.";
        return false;
    }
    if (tcbPositionKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        tcbPositionKeyType->GetMethodByDecl("const VxVector& GetPosition()") == nullptr ||
        tcbPositionKeyType->GetMethodByDecl("bool Compare(CKTCBPositionKey&in key, float threshold)") == nullptr) {
        error = "CKTCBPositionKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (tcbPositionKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        tcbPositionKeyType->GetMethodByDecl("const VxVector& GetPosition() const") != nullptr ||
        tcbPositionKeyType->GetMethodByDecl("bool Compare(CKTCBPositionKey&in key, float threshold) const") != nullptr) {
        error = "CKTCBPositionKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTCBPositionKeySelfTest";
    const char *source =
        "int ProbeTCBPositionKey() {\n"
        "  VxVector pos(1.0f, 2.0f, 3.0f);\n"
        "  CKTCBPositionKey key(1.25f, pos, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f);\n"
        "  if (pos.x != 1.0f || pos.y != 2.0f || pos.z != 3.0f) return 80;\n"
        "  if (key.GetTime() != 1.25f) return 1;\n"
        "  if (key.tension != 0.1f || key.continuity != 0.2f || key.bias != 0.3f) return 2;\n"
        "  if (key.easeto != 0.4f || key.easefrom != 0.5f) return 3;\n"
        "  VxVector got = key.GetPosition();\n"
        "  if (got.x != 1.0f || got.y != 2.0f || got.z != 3.0f) return 4;\n"
        "  CKTCBPositionKey defaults(2.0f, pos);\n"
        "  if (defaults.tension != 0.0f || defaults.continuity != 0.0f || defaults.bias != 0.0f) return 5;\n"
        "  if (defaults.easeto != 0.0f || defaults.easefrom != 0.0f) return 6;\n"
        "  CKTCBPositionKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 7;\n"
        "  copy.tension = 0.9f;\n"
        "  if (key.Compare(copy, 0.0f)) return 8;\n"
        "  CKTCBPositionKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 9;\n"
        "  CKPositionKey positionBase = key;\n"
        "  if (positionBase.GetTime() != 1.25f) return 10;\n"
        "  VxVector basePos = positionBase.GetPosition();\n"
        "  if (basePos.x != 1.0f || basePos.y != 2.0f || basePos.z != 3.0f) return 11;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 12;\n"
        "  key.SetTime(2.5f);\n"
        "  if (key.GetTime() != 2.5f) return 13;\n"
        "  VxVector moved(7.0f, 8.0f, 9.0f);\n"
        "  key.SetPosition(moved);\n"
        "  if (moved.x != 7.0f || moved.y != 8.0f || moved.z != 9.0f) return 81;\n"
        "  got = key.GetPosition();\n"
        "  if (got.x != 7.0f || got.y != 8.0f || got.z != 9.0f) return 14;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTCBPositionKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-tcb-position-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTCBPositionKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTCBPositionKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeTCBPositionKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKTCBPositionKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKTCBPositionKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKTCBRotationKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKTCBRotationKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *tcbRotationKeyType = engine->GetTypeInfoByDecl("CKTCBRotationKey");
    if (!tcbRotationKeyType) {
        error = "CKTCBRotationKey self-test could not find the registered type.";
        return false;
    }
    if (tcbRotationKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr ||
        tcbRotationKeyType->GetMethodByDecl("CKRotationKey opImplConv() const") == nullptr) {
        error = "CKTCBRotationKey self-test could not find safe base value conversions.";
        return false;
    }
    if (tcbRotationKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        tcbRotationKeyType->GetMethodByDecl("const VxQuaternion& GetRotation()") == nullptr ||
        tcbRotationKeyType->GetMethodByDecl("bool Compare(CKTCBRotationKey&in key, float threshold)") == nullptr) {
        error = "CKTCBRotationKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (tcbRotationKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        tcbRotationKeyType->GetMethodByDecl("const VxQuaternion& GetRotation() const") != nullptr ||
        tcbRotationKeyType->GetMethodByDecl("bool Compare(CKTCBRotationKey&in key, float threshold) const") != nullptr) {
        error = "CKTCBRotationKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTCBRotationKeySelfTest";
    const char *source =
        "int ProbeTCBRotationKey() {\n"
        "  VxQuaternion rot(0.0f, 0.0f, 0.0f, 1.0f);\n"
        "  CKTCBRotationKey key(1.25f, rot, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f);\n"
        "  if (rot.x != 0.0f || rot.y != 0.0f || rot.z != 0.0f || rot.w != 1.0f) return 80;\n"
        "  if (key.GetTime() != 1.25f) return 1;\n"
        "  if (key.tension != 0.1f || key.continuity != 0.2f || key.bias != 0.3f) return 2;\n"
        "  if (key.easeto != 0.4f || key.easefrom != 0.5f) return 3;\n"
        "  VxQuaternion got = key.GetRotation();\n"
        "  if (got.x != 0.0f || got.y != 0.0f || got.z != 0.0f || got.w != 1.0f) return 4;\n"
        "  CKTCBRotationKey defaults(2.0f, rot);\n"
        "  if (defaults.tension != 0.0f || defaults.continuity != 0.0f || defaults.bias != 0.0f) return 5;\n"
        "  if (defaults.easeto != 0.0f || defaults.easefrom != 0.0f) return 6;\n"
        "  CKTCBRotationKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 7;\n"
        "  copy.tension = 0.9f;\n"
        "  if (key.Compare(copy, 0.0f)) return 8;\n"
        "  CKTCBRotationKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 9;\n"
        "  CKRotationKey rotationBase = key;\n"
        "  if (rotationBase.GetTime() != 1.25f) return 10;\n"
        "  VxQuaternion baseRot = rotationBase.GetRotation();\n"
        "  if (baseRot.x != 0.0f || baseRot.y != 0.0f || baseRot.z != 0.0f || baseRot.w != 1.0f) return 11;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 12;\n"
        "  key.SetTime(2.5f);\n"
        "  if (key.GetTime() != 2.5f) return 13;\n"
        "  VxQuaternion moved(0.1f, 0.2f, 0.3f, 0.4f);\n"
        "  key.SetRotation(moved);\n"
        "  if (moved.x != 0.1f || moved.y != 0.2f || moved.z != 0.3f || moved.w != 0.4f) return 81;\n"
        "  got = key.GetRotation();\n"
        "  if (got.x != 0.1f || got.y != 0.2f || got.z != 0.3f || got.w != 0.4f) return 14;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTCBRotationKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-tcb-rotation-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTCBRotationKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTCBRotationKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeTCBRotationKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKTCBRotationKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKTCBRotationKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKMorphKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKMorphKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *morphKeyType = engine->GetTypeInfoByDecl("CKMorphKey");
    if (!morphKeyType) {
        error = "CKMorphKey self-test could not find the registered type.";
        return false;
    }
    for (asUINT i = 0; i < morphKeyType->GetPropertyCount(); ++i) {
        const char *decl = morphKeyType->GetPropertyDeclaration(i, true);
        const std::string propertyDecl = decl ? decl : "";
        if (propertyDecl == "VxVector& PosArray" ||
            propertyDecl == "VxCompressedVector& NormArray") {
            error = "CKMorphKey self-test found stale raw pointer reference property.";
            return false;
        }
    }
    if (morphKeyType->GetMethodByDecl("NativePointer GetPosArray() const") == nullptr ||
        morphKeyType->GetMethodByDecl("void SetPosArray(NativePointer pointer)") == nullptr ||
        morphKeyType->GetMethodByDecl("NativePointer GetNormArray() const") == nullptr ||
        morphKeyType->GetMethodByDecl("void SetNormArray(NativePointer pointer)") == nullptr) {
        error = "CKMorphKey self-test could not find expected NativePointer array accessors.";
        return false;
    }
    if (morphKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr) {
        error = "CKMorphKey self-test could not find safe CKKey value conversion.";
        return false;
    }
    if (morphKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        morphKeyType->GetMethodByDecl("bool Compare(CKMorphKey&in key, int nbVertex, float threshold)") == nullptr) {
        error = "CKMorphKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (morphKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        morphKeyType->GetMethodByDecl("bool Compare(CKMorphKey&in key, int nbVertex, float threshold) const") != nullptr) {
        error = "CKMorphKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMorphKeySelfTest";
    const char *source =
        "int ProbeMorphKey() {\n"
        "  CKMorphKey key;\n"
        "  if (key.GetTime() != 0.0f) return 1;\n"
        "  if (!key.GetPosArray().IsNull()) return 2;\n"
        "  if (!key.GetNormArray().IsNull()) return 3;\n"
        "  NativePointer empty;\n"
        "  key.SetPosArray(empty);\n"
        "  key.SetNormArray(empty);\n"
        "  if (!key.GetPosArray().IsNull() || !key.GetNormArray().IsNull()) return 4;\n"
        "  key.SetTime(1.25f);\n"
        "  CKMorphKey copy(key);\n"
        "  if (copy.GetTime() != 1.25f) return 5;\n"
        "  CKMorphKey assigned;\n"
        "  assigned = key;\n"
        "  if (assigned.GetTime() != 1.25f) return 6;\n"
        "  if (!key.Compare(copy, 0, 0.0f)) return 7;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 8;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMorphKeyNegativeCount() {\n"
        "  CKMorphKey left;\n"
        "  CKMorphKey right;\n"
        "  left.Compare(right, -1, 0.0f);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMorphKeyRejectPosArray() {\n"
        "  CKMorphKey key;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  key.SetPosArray(ptr);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMorphKeyRejectNormArray() {\n"
        "  CKMorphKey key;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  key.SetNormArray(ptr);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMorphKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-morph-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMorphKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMorphKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeMorphKey()");
    asIScriptFunction *negativeCount = module->GetFunctionByDecl("int ProbeMorphKeyNegativeCount()");
    asIScriptFunction *rejectPosArray = module->GetFunctionByDecl("int ProbeMorphKeyRejectPosArray()");
    asIScriptFunction *rejectNormArray = module->GetFunctionByDecl("int ProbeMorphKeyRejectNormArray()");
    if (!probe || !negativeCount || !rejectPosArray || !rejectNormArray) {
        engine->DiscardModule(moduleName);
        error = "CKMorphKey self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKMorphKey probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, negativeCount, true, "CKMorphKey negative-count probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, rejectPosArray, true, "CKMorphKey PosArray rejection probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, rejectNormArray, true, "CKMorphKey NormArray rejection probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKAnimControllerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context) {
        error = "CKAnimController script self-test requires a CKContext.";
        return false;
    }
    if (!engine) {
        error = "CKAnimController script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *controllerType = engine->GetTypeInfoByDecl("CKAnimController");
    if (!controllerType) {
        error = "CKAnimController self-test could not find the registered type.";
        return false;
    }
    if (controllerType->GetMethodByDecl("bool Evaluate(float timeStep, NativePointer res)") != nullptr ||
        controllerType->GetMethodByDecl("int AddKey(CKKey&in key)") != nullptr ||
        controllerType->GetMethodByDecl("int DumpKeysTo(NativePointer buffer)") != nullptr ||
        controllerType->GetMethodByDecl("int ReadKeysFrom(NativePointer buffer)") != nullptr) {
        error = "CKAnimController self-test found stale raw-pointer or base-key declarations.";
        return false;
    }
    if (controllerType->GetMethodByDecl("bool EvaluateVector(float timeStep, VxVector&out result)") == nullptr ||
        controllerType->GetMethodByDecl("bool EvaluateQuaternion(float timeStep, VxQuaternion&out result)") == nullptr ||
        controllerType->GetMethodByDecl("int AddPositionKey(CKPositionKey&in key)") == nullptr ||
        controllerType->GetMethodByDecl("int AddRotationKey(CKRotationKey&in key)") == nullptr ||
        controllerType->GetMethodByDecl("NativeBuffer@ DumpKeys()") == nullptr ||
        controllerType->GetMethodByDecl("int DumpKeysTo(NativeBuffer@ buffer = null)") == nullptr ||
        controllerType->GetMethodByDecl("int ReadKeysFrom(NativeBuffer@ buffer)") == nullptr ||
        controllerType->GetMethodByDecl("CKKey& GetKey(int index)") == nullptr ||
        controllerType->GetMethodByDecl("bool Compare(CKAnimController@ control, float threshold = 0.0)") == nullptr ||
        controllerType->GetMethodByDecl("bool Clone(CKAnimController@ control)") == nullptr) {
        error = "CKAnimController self-test could not find expected guarded methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAnimControllerSelfTest";
    const char *source =
        "int ProbeAnimControllerSurface(CKAnimController@ posController, CKAnimController@ rotController) {\n"
        "  if (posController is null || rotController is null) return 1;\n"
        "  if (posController.GetType() != CKANIMATION_LINPOS_CONTROL) return 2;\n"
        "  if (rotController.GetType() != CKANIMATION_LINROT_CONTROL) return 3;\n"
        "  VxVector pos(1.0f, 2.0f, 3.0f);\n"
        "  CKPositionKey posKey(0.0f, pos);\n"
        "  if (posController.AddPositionKey(posKey) < 0) return 4;\n"
        "  if (posController.GetKeyCount() != 1) return 5;\n"
        "  CKKey gotPos = posController.GetKey(0);\n"
        "  if (gotPos.TimeStep != 0.0f) return 6;\n"
        "  VxVector evaluatedPos;\n"
        "  if (!posController.EvaluateVector(0.0f, evaluatedPos)) return 7;\n"
        "  int needed = posController.DumpKeysTo(null);\n"
        "  if (needed <= 0) return 8;\n"
        "  NativeBuffer@ explicitDump = NativeBuffer(needed);\n"
        "  if (posController.DumpKeysTo(explicitDump) != needed) return 9;\n"
        "  NativeBuffer@ ownedDump = posController.DumpKeys();\n"
        "  if (ownedDump is null || ownedDump.Size() != uint(needed)) return 10;\n"
        "  if (!posController.Compare(posController)) return 11;\n"
        "  if (!posController.Clone(posController)) return 12;\n"
        "  posController.RemoveKey(0);\n"
        "  if (posController.GetKeyCount() != 0) return 13;\n"
        "  if (posController.ReadKeysFrom(ownedDump) != needed) return 14;\n"
        "  if (posController.GetKeyCount() != 1) return 15;\n"
        "  VxQuaternion rot;\n"
        "  rot.FromEulerAngles(0.0f, 0.0f, 0.0f);\n"
        "  CKRotationKey rotKey(0.0f, rot);\n"
        "  if (rotController.AddRotationKey(rotKey) < 0) return 16;\n"
        "  VxQuaternion evaluatedRot;\n"
        "  if (!rotController.EvaluateQuaternion(0.0f, evaluatedRot)) return 17;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAnimControllerMismatchedKey(CKAnimController@ posController, CKAnimController@ rotController) {\n"
        "  VxQuaternion rot;\n"
        "  rot.FromEulerAngles(0.0f, 0.0f, 0.0f);\n"
        "  CKRotationKey rotKey(0.0f, rot);\n"
        "  posController.AddRotationKey(rotKey);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAnimControllerSmallDump(CKAnimController@ posController, CKAnimController@ rotController) {\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(1);\n"
        "  posController.DumpKeysTo(tooSmall);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAnimController self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-anim-controller-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAnimController self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAnimController self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeAnimControllerSurface(CKAnimController@, CKAnimController@)");
    asIScriptFunction *mismatchedKey = module->GetFunctionByDecl("int ProbeAnimControllerMismatchedKey(CKAnimController@, CKAnimController@)");
    asIScriptFunction *smallDump = module->GetFunctionByDecl("int ProbeAnimControllerSmallDump(CKAnimController@, CKAnimController@)");
    if (!probe || !mismatchedKey || !smallDump) {
        engine->DiscardModule(moduleName);
        error = "CKAnimController self-test functions were not found.";
        return false;
    }

    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKAnimControllerSelfTest"), CK_OBJECTCREATION_DYNAMIC));
    if (!animation) {
        engine->DiscardModule(moduleName);
        error = "CKAnimController self-test could not create a CKObjectAnimation.";
        return false;
    }

    CKAnimController *positionController = animation->CreateController(CKANIMATION_LINPOS_CONTROL);
    CKAnimController *rotationController = animation->CreateController(CKANIMATION_LINROT_CONTROL);
    if (!positionController || !rotationController) {
        context->DestroyObject(animation);
        engine->DiscardModule(moduleName);
        error = "CKAnimController self-test could not create native controllers.";
        return false;
    }

    const bool ok = ExecuteCKAnimControllerProbe(engine, probe, positionController, rotationController, false, "CKAnimController probe", error) &&
                    ExecuteCKAnimControllerProbe(engine, mismatchedKey, positionController, rotationController, true, "CKAnimController mismatched-key probe", error) &&
                    ExecuteCKAnimControllerProbe(engine, smallDump, positionController, rotationController, true, "CKAnimController small-dump probe", error);

    context->DestroyObject(animation);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKMorphControllerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context) {
        error = "CKMorphController script self-test requires a CKContext.";
        return false;
    }
    if (!engine) {
        error = "CKMorphController script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *morphControllerType = engine->GetTypeInfoByDecl("CKMorphController");
    if (!morphControllerType) {
        error = "CKMorphController self-test could not find the registered type.";
        return false;
    }
    if (morphControllerType->GetMethodByDecl("bool Evaluate(float timeStep, NativePointer res)") != nullptr ||
        morphControllerType->GetMethodByDecl("int AddKey(CKKey&in key)") != nullptr ||
        morphControllerType->GetMethodByDecl("int AddKey(CKKey&in key, bool allocateNormals)") != nullptr ||
        morphControllerType->GetMethodByDecl("bool Evaluate(float timeStep, int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride, NativePointer normalPtr)") != nullptr ||
        morphControllerType->GetMethodByDecl("CKKey& GetKey(int index)") != nullptr) {
        error = "CKMorphController self-test found stale raw-pointer or inherited base-key methods.";
        return false;
    }
    if (morphControllerType->GetMethodByDecl("int AddMorphKey(CKMorphKey&in key, bool allocateNormals = true)") == nullptr ||
        morphControllerType->GetMethodByDecl("int AddMorphKey(float timeStep, int vertexCount, NativeBuffer@ positions, NativeBuffer@ normals = null)") == nullptr ||
        morphControllerType->GetMethodByDecl("CKMorphKey& GetMorphKey(int index)") == nullptr ||
        morphControllerType->GetMethodByDecl("bool Evaluate(float timeStep, int vertexCount, NativeBuffer@ vertices, CKDWORD vertexStride, NativeBuffer@ normals = null)") == nullptr ||
        morphControllerType->GetMethodByDecl("NativeBuffer@ DumpKeys()") == nullptr ||
        morphControllerType->GetMethodByDecl("int DumpKeysTo(NativeBuffer@ buffer = null)") == nullptr ||
        morphControllerType->GetMethodByDecl("int ReadKeysFrom(NativeBuffer@ buffer)") == nullptr) {
        error = "CKMorphController self-test could not find expected morph-specific methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMorphControllerSelfTest";
    const char *source =
        "int ProbeMorphControllerSurface(CKMorphController@ controller) {\n"
        "  if (controller is null) return 1;\n"
        "  if (controller.GetType() != CKANIMATION_MORPH_CONTROL) return 2;\n"
        "  NativeBuffer@ sourceVertices = NativeBuffer(24);\n"
        "  VxVector first(1.0f, 2.0f, 3.0f);\n"
        "  VxVector second(4.0f, 5.0f, 6.0f);\n"
        "  if (sourceVertices.Write(first) != 12) return 3;\n"
        "  if (sourceVertices.Write(second) != 12) return 4;\n"
        "  sourceVertices.Reset();\n"
        "  if (controller.AddMorphKey(0.0f, 2, sourceVertices, null) < 0) return 5;\n"
        "  if (controller.GetKeyCount() != 1) return 6;\n"
        "  CKMorphKey got = controller.GetMorphKey(0);\n"
        "  if (got.TimeStep != 0.0f) return 7;\n"
        "  NativeBuffer@ outVertices = NativeBuffer(24);\n"
        "  if (!controller.Evaluate(0.0f, 2, outVertices, 12, null)) return 8;\n"
        "  NativeBuffer@ dumped = controller.DumpKeys();\n"
        "  if (dumped is null || dumped.Size() == 0) return 9;\n"
        "  int needed = controller.DumpKeysTo(null);\n"
        "  if (needed <= 0 || dumped.Size() != uint(needed)) return 10;\n"
        "  controller.RemoveKey(0);\n"
        "  if (controller.GetKeyCount() != 0) return 11;\n"
        "  if (controller.ReadKeysFrom(dumped) != needed) return 12;\n"
        "  if (controller.GetKeyCount() != 1) return 13;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMorphControllerSmallEvaluate(CKMorphController@ controller) {\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  controller.Evaluate(0.0f, 2, tooSmall, 12, null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMorphControllerSmallAddKey(CKMorphController@ controller) {\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  controller.AddMorphKey(0.0f, 2, tooSmall, null);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMorphController self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-morph-controller-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMorphController self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMorphController self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeMorphControllerSurface(CKMorphController@)");
    asIScriptFunction *smallEvaluate = module->GetFunctionByDecl("int ProbeMorphControllerSmallEvaluate(CKMorphController@)");
    asIScriptFunction *smallAddKey = module->GetFunctionByDecl("int ProbeMorphControllerSmallAddKey(CKMorphController@)");
    if (!probe || !smallEvaluate || !smallAddKey) {
        engine->DiscardModule(moduleName);
        error = "CKMorphController self-test functions were not found.";
        return false;
    }

    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKMorphControllerSelfTest"), CK_OBJECTCREATION_DYNAMIC));
    if (!animation) {
        engine->DiscardModule(moduleName);
        error = "CKMorphController self-test could not create a CKObjectAnimation.";
        return false;
    }

    CKMorphController *controller = static_cast<CKMorphController *>(animation->CreateController(CKANIMATION_MORPH_CONTROL));
    if (!controller) {
        context->DestroyObject(animation);
        engine->DiscardModule(moduleName);
        error = "CKMorphController self-test could not create a native morph controller.";
        return false;
    }

    const bool ok = ExecuteCKMorphControllerProbe(engine, probe, controller, false, "CKMorphController probe", error) &&
                    ExecuteCKMorphControllerProbe(engine, smallEvaluate, controller, true, "CKMorphController small-evaluate probe", error) &&
                    ExecuteCKMorphControllerProbe(engine, smallAddKey, controller, true, "CKMorphController small-add-key probe", error);

    context->DestroyObject(animation);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKObjectAnimationScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKObjectAnimation script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *animationType = engine->GetTypeInfoByDecl("CKObjectAnimation");
    if (!animationType) {
        error = "CKObjectAnimation self-test could not find the registered type.";
        return false;
    }
    if (animationType->GetMethodByDecl("bool EvaluateMorphTarget(float time, int vertexCount, NativePointer vertices, CKDWORD vStride, NativePointer normals)") != nullptr) {
        error = "CKObjectAnimation self-test found stale raw EvaluateMorphTarget declaration.";
        return false;
    }
    if (animationType->GetMethodByDecl("bool EvaluateMorphTarget(float time, int vertexCount, NativeBuffer@ vertices, CKDWORD vStride, NativeBuffer@ normals = null)") == nullptr ||
        animationType->GetMethodByDecl("bool Compare(CKObjectAnimation@ anim, float threshold = 0.0)") == nullptr ||
        animationType->GetMethodByDecl("bool ShareDataFrom(CKObjectAnimation@ anim)") == nullptr ||
        animationType->GetMethodByDecl("CKObjectAnimation@ CreateMergedAnimation(CKObjectAnimation@ subAnim2, bool dynamic = false)") == nullptr ||
        animationType->GetMethodByDecl("void CreateTransition(float length, CKObjectAnimation@ animIn, float stepFrom, CKObjectAnimation@ animOut, float stepTo, bool veloc, bool dontTurn, CKAnimKey&in startingSet = void)") == nullptr ||
        animationType->GetMethodByDecl("void Clone(CKObjectAnimation@ anim)") == nullptr) {
        error = "CKObjectAnimation self-test could not find expected guarded methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKObjectAnimationSelfTest";
    const char *source =
        "int ProbeObjectAnimationSurface(CKObjectAnimation@ anim, CKObjectAnimation@ other) {\n"
        "  if (anim is null || other is null) return 1;\n"
        "  NativeBuffer@ vertices = NativeBuffer(24);\n"
        "  if (anim.EvaluateMorphTarget(0.0f, 0, vertices, 12, null)) return 2;\n"
        "  anim.Compare(other);\n"
        "  anim.ShareDataFrom(other);\n"
        "  anim.Clone(other);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeObjectAnimationSmallMorphTarget(CKObjectAnimation@ anim, CKObjectAnimation@ other) {\n"
        "  NativeBuffer@ tooSmall = NativeBuffer(12);\n"
        "  anim.EvaluateMorphTarget(0.0f, 2, tooSmall, 12, null);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKObjectAnimation self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-object-animation-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKObjectAnimation self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKObjectAnimation self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeObjectAnimationSurface(CKObjectAnimation@, CKObjectAnimation@)");
    asIScriptFunction *smallMorphTarget = module->GetFunctionByDecl("int ProbeObjectAnimationSmallMorphTarget(CKObjectAnimation@, CKObjectAnimation@)");
    if (!probe || !smallMorphTarget) {
        engine->DiscardModule(moduleName);
        error = "CKObjectAnimation self-test functions were not found.";
        return false;
    }

    CKObjectAnimation *animation = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKObjectAnimationSelfTest"), CK_OBJECTCREATION_DYNAMIC));
    CKObjectAnimation *other = CKObjectAnimation::Cast(context->CreateObject(
        CKCID_OBJECTANIMATION, const_cast<CKSTRING>("__CKAS_CKObjectAnimationSelfTestOther"), CK_OBJECTCREATION_DYNAMIC));
    if (!animation || !other) {
        if (animation) context->DestroyObject(animation);
        if (other) context->DestroyObject(other);
        engine->DiscardModule(moduleName);
        error = "CKObjectAnimation self-test could not create native animations.";
        return false;
    }

    const bool ok = ExecuteCKObjectAnimationProbe(engine, probe, animation, other, false, "CKObjectAnimation probe", error) &&
                    ExecuteCKObjectAnimationProbe(engine, smallMorphTarget, animation, other, true, "CKObjectAnimation small morph target probe", error);

    context->DestroyObject(animation);
    context->DestroyObject(other);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKAnimationScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context) {
        error = "CKAnimation script self-test requires a CKContext.";
        return false;
    }
    if (!engine) {
        error = "CKAnimation script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *animationType = engine->GetTypeInfoByDecl("CKAnimation");
    if (!animationType) {
        error = "CKAnimation self-test could not find the registered type.";
        return false;
    }
    if (animationType->GetMethodByDecl("CKAnimation@ CreateMergedAnimation(CKAnimation@ anim2, bool dynamic = false)") == nullptr ||
        animationType->GetMethodByDecl("float CreateTransition(CKAnimation@ input, CKAnimation@ output, CKDWORD outTransitionMode, float length = 6.0, float frameTo = 0)") == nullptr ||
        animationType->GetMethodByDecl("CKERROR Copy(CKObject@ obj, CKDependenciesContext&in context)") == nullptr) {
        error = "CKAnimation self-test could not find expected guarded methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAnimationSelfTest";
    const char *source =
        "int ProbeAnimationSurface(CKAnimation@ anim, CKAnimation@ other) {\n"
        "  if (anim is null || other is null) return 1;\n"
        "  if (cast<CKKeyedAnimation>(anim) is null) return 2;\n"
        "  CKObject@ asObject = anim;\n"
        "  CKSceneObject@ asSceneObject = anim;\n"
        "  if (asObject is null || asSceneObject is null) return 3;\n"
        "  anim.SetName(\"__CKAS_CKAnimationSurface\", false);\n"
        "  if (anim.GetName() == \"\") return 4;\n"
        "  anim.SetLength(12.0f);\n"
        "  if (anim.GetLength() < 0.0f) return 5;\n"
        "  anim.SetFrame(1.0f);\n"
        "  anim.SetStep(0.25f);\n"
        "  anim.SetCurrentStep(0.5f);\n"
        "  anim.GetFrame(); anim.GetStep(); anim.GetNextFrame(16.0f);\n"
        "  anim.LinkToFrameRate(true, 24.0f);\n"
        "  if (!anim.IsLinkedToFrameRate()) return 6;\n"
        "  anim.LinkToFrameRate(false);\n"
        "  anim.SetTransitionMode(CK_TRANSITION_FROMANIMATION);\n"
        "  anim.GetTransitionMode();\n"
        "  anim.SetSecondaryAnimationMode(CKSECONDARYANIMATION_FROMANIMATION);\n"
        "  anim.GetSecondaryAnimationMode();\n"
        "  anim.SetCanBeInterrupt(true);\n"
        "  if (!anim.CanBeInterrupt()) return 7;\n"
        "  anim.SetCharacterOrientation(true);\n"
        "  anim.DoesCharacterTakeOrientation();\n"
        "  anim.SetFlags(CKANIMATION_CANBEBREAK);\n"
        "  if ((anim.GetFlags() & CKANIMATION_CANBEBREAK) == 0) return 8;\n"
        "  anim.SetMergeFactor(0.25f);\n"
        "  anim.GetMergeFactor(); anim.IsMerged();\n"
        "  if (anim.GetCharacter() !is null) return 9;\n"
        "  if (anim.GetRootEntity() !is null) return 10;\n"
        "  if (!anim.GetAppData().IsNull()) return 11;\n"
        "  anim.SetAppData(NativePointer());\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAnimationMergeNull(CKAnimation@ anim, CKAnimation@ other) {\n"
        "  anim.CreateMergedAnimation(null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAnimationTransitionNull(CKAnimation@ anim, CKAnimation@ other) {\n"
        "  anim.CreateTransition(anim, null, 0);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAnimation self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-animation-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAnimation self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAnimation self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeAnimationSurface(CKAnimation@, CKAnimation@)");
    asIScriptFunction *mergeNull = module->GetFunctionByDecl("int ProbeAnimationMergeNull(CKAnimation@, CKAnimation@)");
    asIScriptFunction *transitionNull = module->GetFunctionByDecl("int ProbeAnimationTransitionNull(CKAnimation@, CKAnimation@)");
    if (!probe || !mergeNull || !transitionNull) {
        engine->DiscardModule(moduleName);
        error = "CKAnimation self-test functions were not found.";
        return false;
    }

    CKAnimation *animation = CKAnimation::Cast(context->CreateObject(
        CKCID_KEYEDANIMATION, const_cast<CKSTRING>("__CKAS_CKAnimationSelfTestA"), CK_OBJECTCREATION_DYNAMIC));
    CKAnimation *other = CKAnimation::Cast(context->CreateObject(
        CKCID_KEYEDANIMATION, const_cast<CKSTRING>("__CKAS_CKAnimationSelfTestB"), CK_OBJECTCREATION_DYNAMIC));
    if (!animation || !other) {
        if (other) context->DestroyObject(other);
        if (animation) context->DestroyObject(animation);
        engine->DiscardModule(moduleName);
        error = "CKAnimation self-test could not create native keyed animations.";
        return false;
    }

    CKDependenciesContext dependencies(context);
    const bool ok = ExecuteCKAnimationProbe(engine, probe, animation, other, false, "CKAnimation probe", error) &&
                    ExecuteCKAnimationCopyNullProbe(engine, animationType, animation, dependencies, error) &&
                    ExecuteCKAnimationProbe(engine, mergeNull, animation, other, true, "CKAnimation merge-null probe", error) &&
                    ExecuteCKAnimationProbe(engine, transitionNull, animation, other, true, "CKAnimation transition-null probe", error);

    context->DestroyObject(other);
    context->DestroyObject(animation);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKKeyedAnimationScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKKeyedAnimation script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *objectType = engine->GetTypeInfoByDecl("CKObject");
    asITypeInfo *sceneObjectType = engine->GetTypeInfoByDecl("CKSceneObject");
    asITypeInfo *animationType = engine->GetTypeInfoByDecl("CKAnimation");
    asITypeInfo *keyedType = engine->GetTypeInfoByDecl("CKKeyedAnimation");
    if (!objectType || !sceneObjectType || !animationType || !keyedType) {
        error = "CKKeyedAnimation self-test could not find required object hierarchy types.";
        return false;
    }
    if (objectType->GetMethodByDecl("CKKeyedAnimation@ opCast()") == nullptr ||
        sceneObjectType->GetMethodByDecl("CKKeyedAnimation@ opCast()") == nullptr ||
        animationType->GetMethodByDecl("CKKeyedAnimation@ opCast()") == nullptr ||
        keyedType->GetMethodByDecl("CKObject@ opImplCast()") == nullptr ||
        keyedType->GetMethodByDecl("CKSceneObject@ opImplCast()") == nullptr ||
        keyedType->GetMethodByDecl("CKAnimation@ opImplCast()") == nullptr ||
        keyedType->GetMethodByDecl("CKERROR AddAnimation(CKObjectAnimation@ anim)") == nullptr ||
        keyedType->GetMethodByDecl("CKERROR RemoveAnimation(CKObjectAnimation@ anim)") == nullptr) {
        error = "CKKeyedAnimation self-test could not find expected casts or guarded methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKKeyedAnimationSelfTest";
    const char *source =
        "void ProbeKeyedAnimationSurface(CKObject@ object, CKSceneObject@ sceneObject, CKAnimation@ animation, CKKeyedAnimation@ keyed, CKObjectAnimation@ objectAnimation) {\n"
        "  CKKeyedAnimation@ fromObject = cast<CKKeyedAnimation>(object);\n"
        "  CKKeyedAnimation@ fromSceneObject = cast<CKKeyedAnimation>(sceneObject);\n"
        "  CKKeyedAnimation@ fromAnimation = cast<CKKeyedAnimation>(animation);\n"
        "  if (keyed is null) return;\n"
        "  CKObject@ asObject = keyed;\n"
        "  CKSceneObject@ asSceneObject = keyed;\n"
        "  CKAnimation@ asAnimation = keyed;\n"
        "  keyed.AddAnimation(objectAnimation);\n"
        "  keyed.RemoveAnimation(objectAnimation);\n"
        "  CKObjectAnimation@ byIndex = keyed.GetAnimation(0);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKKeyedAnimation self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-keyed-animation-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKKeyedAnimation self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKKeyedAnimation self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKBezierPositionKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKBezierPositionKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *keyType = engine->GetTypeInfoByDecl("CKKey");
    asITypeInfo *positionKeyType = engine->GetTypeInfoByDecl("CKPositionKey");
    asITypeInfo *bezierKeyType = engine->GetTypeInfoByDecl("CKBezierPositionKey");
    if (!keyType || !positionKeyType || !bezierKeyType) {
        error = "CKBezierPositionKey self-test could not find key types.";
        return false;
    }
    if (keyType->GetMethodByDecl("CKBezierPositionKey opConv() const") != nullptr ||
        positionKeyType->GetMethodByDecl("CKBezierPositionKey opConv() const") != nullptr) {
        error = "CKBezierPositionKey self-test found stale unsafe base-to-derived value conversion.";
        return false;
    }
    if (bezierKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr ||
        bezierKeyType->GetMethodByDecl("CKPositionKey opImplConv() const") == nullptr) {
        error = "CKBezierPositionKey self-test could not find safe derived-to-base value conversions.";
        return false;
    }
    if (bezierKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        bezierKeyType->GetMethodByDecl("const VxVector& GetPosition()") == nullptr ||
        bezierKeyType->GetMethodByDecl("bool Compare(CKBezierPositionKey&in key, float threshold)") == nullptr) {
        error = "CKBezierPositionKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (bezierKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        bezierKeyType->GetMethodByDecl("const VxVector& GetPosition() const") != nullptr ||
        bezierKeyType->GetMethodByDecl("bool Compare(CKBezierPositionKey&in key, float threshold) const") != nullptr) {
        error = "CKBezierPositionKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBezierPositionKeySelfTest";
    const char *source =
        "int ProbeBezierPositionKey() {\n"
        "  VxVector pos(1.0f, 2.0f, 3.0f);\n"
        "  VxVector input(0.1f, 0.2f, 0.3f);\n"
        "  VxVector output(0.4f, 0.5f, 0.6f);\n"
        "  CKBezierKeyFlags flags;\n"
        "  flags.SetInTangentMode(BEZIER_KEY_TANGENTS);\n"
        "  flags.SetOutTangentMode(BEZIER_KEY_TANGENTS);\n"
        "  CKBezierPositionKey key(2.5f, pos, flags, input, output);\n"
        "  if (pos.x != 1.0f || pos.y != 2.0f || pos.z != 3.0f) return 80;\n"
        "  if (input.x != 0.1f || input.y != 0.2f || input.z != 0.3f) return 81;\n"
        "  if (output.x != 0.4f || output.y != 0.5f || output.z != 0.6f) return 82;\n"
        "  if (key.GetTime() != 2.5f) return 1;\n"
        "  VxVector got = key.GetPosition();\n"
        "  if (got.x != 1.0f || got.y != 2.0f || got.z != 3.0f) return 2;\n"
        "  if (key.Flags.GetInTangentMode() != BEZIER_KEY_TANGENTS) return 3;\n"
        "  CKBezierPositionKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 4;\n"
        "  copy.Out.x = 9.0f;\n"
        "  if (key.Compare(copy, 0.0f)) return 5;\n"
        "  CKBezierPositionKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 6;\n"
        "  CKPositionKey positionBase = key;\n"
        "  if (positionBase.GetTime() != 2.5f) return 7;\n"
        "  VxVector basePos = positionBase.GetPosition();\n"
        "  if (basePos.x != 1.0f || basePos.y != 2.0f || basePos.z != 3.0f) return 8;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 2.5f) return 9;\n"
        "  CKKey baseFromPosition = positionBase;\n"
        "  if (baseFromPosition.GetTime() != 2.5f) return 10;\n"
        "  key.SetTime(3.5f);\n"
        "  if (key.GetTime() != 3.5f) return 11;\n"
        "  VxVector moved(7.0f, 8.0f, 9.0f);\n"
        "  key.SetPosition(moved);\n"
        "  if (moved.x != 7.0f || moved.y != 8.0f || moved.z != 9.0f) return 83;\n"
        "  got = key.GetPosition();\n"
        "  if (got.x != 7.0f || got.y != 8.0f || got.z != 9.0f) return 12;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBezierPositionKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-bezier-position-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBezierPositionKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBezierPositionKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeBezierPositionKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBezierPositionKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKBezierPositionKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

CKGUID FindCKParameterOperationSelfTestGuid(CKContext *context) {
    CKParameterManager *parameterManager = context ? context->GetParameterManager() : nullptr;
    if (!context || !parameterManager) {
        return CKGUID();
    }

    for (int i = 0; i < parameterManager->GetParameterOperationCount(); ++i) {
        const CKGUID guid = parameterManager->OperationCodeToGuid(i);
        if (!guid.IsValid()) {
            continue;
        }

        CKParameterOperation *operation = context->CreateCKParameterOperation(
            const_cast<CKSTRING>("__CKAS_CKParameterOperationGuidProbe"),
            guid,
            CKPGUID_INT,
            CKPGUID_INT,
            CKPGUID_INT);
        const bool usable = operation && operation->GetInParameter1() && operation->GetInParameter2() &&
                            operation->GetOutParameter() && operation->GetOperationFunction();
        if (operation) {
            context->DestroyObject(operation);
        }
        if (usable) {
            return guid;
        }
    }

    return CKGUID();
}

bool RunCKStateChunkScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKStateChunk script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *stateChunkType = engine->GetTypeInfoByDecl("CKStateChunk");
    if (!stateChunkType) {
        error = "CKStateChunk self-test could not find the registered type.";
        return false;
    }
    if (stateChunkType->GetMethodByDecl("int ReadString(string &out str)") == nullptr ||
        stateChunkType->GetMethodByDecl("CKObject@ ReadObject(CKContext@ context)") == nullptr ||
        stateChunkType->GetMethodByDecl("const XObjectPointerArray &ReadXObjectArray(CKContext@ context)") == nullptr) {
        error = "CKStateChunk self-test could not find the guarded read declarations.";
        return false;
    }
    if (stateChunkType->GetMethodByDecl("void AddChunkAndDelete(CKStateChunk@ chunk)") != nullptr) {
        error = "CKStateChunk self-test found stale AddChunkAndDelete declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKStateChunkSelfTest";
    const char *source =
        "int ProbeCKStateChunkReadString(CKStateChunk@ chunk, CKContext@ context) {\n"
        "  if (chunk is null || context is null) return 2;\n"
        "  chunk.StartRead();\n"
        "  string value;\n"
        "  int count = chunk.ReadString(value);\n"
        "  if (count <= 0) return 3;\n"
        "  if (value != \"ckas-statechunk\") return 4;\n"
        "  return 0;\n"
        "}\n"
        "void ProbeCKStateChunkReadObjectNull(CKStateChunk@ chunk, CKContext@ context) {\n"
        "  chunk.ReadObject(null);\n"
        "}\n"
        "void ProbeCKStateChunkReadXObjectArrayNull(CKStateChunk@ chunk, CKContext@ context) {\n"
        "  chunk.ReadXObjectArray(null);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKStateChunk self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-statechunk-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKStateChunk self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKStateChunk self-test script failed to build.";
        return false;
    }

    asIScriptFunction *readString = module->GetFunctionByDecl("int ProbeCKStateChunkReadString(CKStateChunk@, CKContext@)");
    asIScriptFunction *readObjectNull = module->GetFunctionByDecl("void ProbeCKStateChunkReadObjectNull(CKStateChunk@, CKContext@)");
    asIScriptFunction *readXObjectArrayNull = module->GetFunctionByDecl("void ProbeCKStateChunkReadXObjectArrayNull(CKStateChunk@, CKContext@)");
    if (!readString || !readObjectNull || !readXObjectArrayNull) {
        engine->DiscardModule(moduleName);
        error = "CKStateChunk self-test function was not found.";
        return false;
    }

    CKStateChunk *chunk = CreateCKStateChunk(CKCID_OBJECT, nullptr);
    if (!chunk) {
        engine->DiscardModule(moduleName);
        error = "CKStateChunk self-test could not create a native chunk.";
        return false;
    }

    chunk->StartWrite();
    chunk->WriteString(const_cast<CKSTRING>("ckas-statechunk"));
    chunk->CloseChunk();

    const bool ok = ExecuteCKStateChunkProbe(engine, readString, chunk, context, false, "CKStateChunk ReadString probe", error) &&
                    ExecuteCKStateChunkProbe(engine, readObjectNull, chunk, context, true, "CKStateChunk ReadObject null context probe", error) &&
                    ExecuteCKStateChunkProbe(engine, readXObjectArrayNull, chunk, context, true, "CKStateChunk ReadXObjectArray null context probe", error);

    DeleteCKStateChunk(chunk);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKParameterScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKParameter script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    const char *typeNames[] = {"CKParameter", "CKParameterOut", "CKParameterLocal"};
    for (const char *typeName : typeNames) {
        asITypeInfo *parameterType = engine->GetTypeInfoByDecl(typeName);
        if (!parameterType) {
            error = std::string(typeName) + " self-test could not find the registered type.";
            return false;
        }
        if (parameterType->GetMethodByDecl("int GetStringValue(string &out value, bool update = true)") == nullptr) {
            error = std::string(typeName) + " self-test could not find the guarded GetStringValue declaration.";
            return false;
        }
        if (parameterType->GetMethodByDecl("int GetStringValue(const string &in, bool = true)") != nullptr) {
            error = std::string(typeName) + " self-test found stale GetStringValue input-buffer declaration.";
            return false;
        }
        if (parameterType->GetMethodByDecl("NativeBuffer@ GetReadData(bool update = true)") == nullptr ||
            parameterType->GetMethodByDecl("NativeBuffer@ GetWriteData()") == nullptr) {
            error = std::string(typeName) + " self-test could not find sized NativeBuffer data accessors.";
            return false;
        }
        if (parameterType->GetMethodByDecl("NativePointer GetReadDataPtr(bool update = true)") != nullptr ||
            parameterType->GetMethodByDecl("NativePointer GetWriteDataPtr()") != nullptr) {
            error = std::string(typeName) + " self-test found stale raw NativePointer data accessors.";
            return false;
        }
        if (parameterType->GetMethodByDecl("CKParameterTypeDesc &GetParameterType()") == nullptr) {
            error = std::string(typeName) + " self-test could not find GetParameterType declaration.";
            return false;
        }
    }
    asITypeInfo *parameterInType = engine->GetTypeInfoByDecl("CKParameterIn");
    if (!parameterInType) {
        error = "CKParameterIn self-test could not find the registered type.";
        return false;
    }
    if (parameterInType->GetMethodByDecl("void SetType(CKParameterType type, bool updateSource = false)") == nullptr ||
        parameterInType->GetMethodByDecl("void SetType(CKParameterType type, bool updateSource, const string &in newName)") == nullptr ||
        parameterInType->GetMethodByDecl("void SetGUID(CKGUID guid, bool updateSource = false)") == nullptr ||
        parameterInType->GetMethodByDecl("void SetGUID(CKGUID guid, bool updateSource, const string &in newName)") == nullptr) {
        error = "CKParameterIn self-test could not find the guarded SetType/SetGUID overloads.";
        return false;
    }
    if (parameterInType->GetMethodByDecl("NativeBuffer@ GetReadData()") == nullptr) {
        error = "CKParameterIn self-test could not find the sized GetReadData declaration.";
        return false;
    }
    if (parameterInType->GetMethodByDecl("NativePointer GetReadDataPtr()") != nullptr) {
        error = "CKParameterIn self-test found stale raw GetReadDataPtr exposure.";
        return false;
    }
    asITypeInfo *operationType = engine->GetTypeInfoByDecl("CKParameterOperation");
    if (!operationType) {
        error = "CKParameterOperation self-test could not find the registered type.";
        return false;
    }
    if (operationType->GetMethodByDecl("bool HasOperationFunction()") == nullptr) {
        error = "CKParameterOperation self-test could not find HasOperationFunction.";
        return false;
    }
    if (operationType->GetMethodByDecl("NativePointer GetOperationFunction()") != nullptr) {
        error = "CKParameterOperation self-test found stale raw GetOperationFunction exposure.";
        return false;
    }
    constexpr const char *moduleName = "__CKAS_CKParameterSelfTest";
    const char *source =
        "void ProbeCKParameterStringValue(CKParameter@ param, CKParameterOut@ pout, CKParameterLocal@ local) {\n"
        "  string value;\n"
        "  if (param !is null) {\n"
        "    int count = param.GetStringValue(value);\n"
        "    count = param.GetStringValue(value, false);\n"
        "  }\n"
        "  if (pout !is null) {\n"
        "    int count = pout.GetStringValue(value);\n"
        "  }\n"
        "  if (local !is null) {\n"
        "    int count = local.GetStringValue(value);\n"
        "  }\n"
        "}\n"
        "int ProbeCKParameterType(CKParameterLocal@ local) {\n"
        "  if (local is null) return 2;\n"
        "  CKParameterTypeDesc desc = local.GetParameterType();\n"
        "  if (desc.Valid == 0) return 3;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterGenericStringValue(CKParameterLocal@ local) {\n"
        "  if (local is null) return 2;\n"
        "  string expected = \"generic text\";\n"
        "  string value;\n"
        "  if (local.GetValue(value) != CK_OK) return 3;\n"
        "  if (value != expected) return 4;\n"
        "  string directValue;\n"
        "  int count = local.GetStringValue(directValue);\n"
        "  if (count <= 0) return 5;\n"
        "  if (directValue != expected) return 6;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterSetStringValue(CKParameterLocal@ local) {\n"
        "  if (local is null) return 2;\n"
        "  string expected = \"script text\";\n"
        "  if (local.SetStringValue(expected) != CK_OK) return 3;\n"
        "  string directValue;\n"
        "  int count = local.GetStringValue(directValue);\n"
        "  if (count <= 0) return 4;\n"
        "  if (directValue != expected) return 5;\n"
        "  string genericValue;\n"
        "  if (local.GetValue(genericValue) != CK_OK) return 6;\n"
        "  if (genericValue != expected) return 7;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterGenericSetStringValue(CKParameterLocal@ local) {\n"
        "  if (local is null) return 2;\n"
        "  string expected = \"generic script text\";\n"
        "  if (local.SetValue(expected) != CK_OK) return 3;\n"
        "  string directValue;\n"
        "  int count = local.GetStringValue(directValue);\n"
        "  if (count <= 0) return 4;\n"
        "  if (directValue != expected) return 5;\n"
        "  string genericValue;\n"
        "  if (local.GetValue(genericValue) != CK_OK) return 6;\n"
        "  if (genericValue != expected) return 7;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterGenericIntValue(CKParameterLocal@ local) {\n"
        "  if (local is null) return 2;\n"
        "  int source = 12345;\n"
        "  if (local.SetValue(source) != CK_OK) return 3;\n"
        "  NativeBuffer@ writeData = local.GetWriteData();\n"
        "  if (writeData is null) return 6;\n"
        "  if (writeData.Size() != uint(local.GetDataSize())) return 7;\n"
        "  int rewritten = 23456;\n"
        "  if (writeData.WriteInt(rewritten) != 4) return 8;\n"
        "  int value = 0;\n"
        "  if (local.GetValue(value) != CK_OK) return 9;\n"
        "  if (value != rewritten) return 10;\n"
        "  NativeBuffer@ readData = local.GetReadData();\n"
        "  if (readData is null) return 11;\n"
        "  if (readData.Size() != uint(local.GetDataSize())) return 12;\n"
        "  value = 0;\n"
        "  if (readData.ReadInt(value) != 4) return 13;\n"
        "  if (value != rewritten) return 14;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterGenericVectorValue(CKParameterLocal@ local) {\n"
        "  if (local is null) return 2;\n"
        "  VxVector source(1.0f, 2.0f, 3.0f);\n"
        "  if (local.SetValue(source) != CK_OK) return 3;\n"
        "  VxVector value;\n"
        "  if (local.GetValue(value) != CK_OK) return 4;\n"
        "  if (value.x != source.x || value.y != source.y || value.z != source.z) return 5;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterLocalMyselfState(CKParameterLocal@ local) {\n"
        "  if (local is null) return 2;\n"
        "  if (local.IsMyselfParameter()) return 3;\n"
        "  CKParameterType originalType = local.GetType();\n"
        "  CKGUID originalGuid = local.GetGUID();\n"
        "  local.SetAsMyselfParameter(true);\n"
        "  if (!local.IsMyselfParameter()) return 4;\n"
        "  int blocked = 77;\n"
        "  if (local.SetValue(blocked) != CKERR_INVALIDPARAMETER) return 5;\n"
        "  NativeBuffer@ blockedData = local.GetWriteData();\n"
        "  if (blockedData is null || !blockedData.IsEmpty()) return 6;\n"
        "  local.SetAsMyselfParameter(false);\n"
        "  if (local.IsMyselfParameter()) return 7;\n"
        "  local.SetType(originalType);\n"
        "  local.SetGUID(originalGuid);\n"
        "  if (local.SetValue(blocked) != CK_OK) return 8;\n"
        "  int value = 0;\n"
        "  if (local.GetValue(value) != CK_OK) return 9;\n"
        "  if (value != blocked) return 10;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterLocalOwnerGraph(CKBehavior@ owner, CKParameterLocal@ local) {\n"
        "  if (owner is null || local is null) return 2;\n"
        "  if (owner.GetLocalParameterCount() < 1) return 3;\n"
        "  int pos = owner.GetLocalParameterPosition(local);\n"
        "  if (pos < 0) return 4;\n"
        "  if (owner.GetLocalParameter(pos) !is local) return 5;\n"
        "  int value = 31415;\n"
        "  if (local.SetValue(value) != CK_OK) return 6;\n"
        "  int read = 0;\n"
        "  if (local.GetValue(read) != CK_OK || read != value) return 7;\n"
        "  CKParameterLocal@ removed = owner.RemoveLocalParameter(pos);\n"
        "  if (removed !is local) return 8;\n"
        "  if (owner.GetLocalParameterPosition(local) >= 0) return 9;\n"
        "  owner.AddLocalParameter(local);\n"
        "  pos = owner.GetLocalParameterPosition(local);\n"
        "  if (pos < 0 || owner.GetLocalParameter(pos) !is local) return 10;\n"
        "  return 0;\n"
        "}\n"
        "class CKParameterGenericRejected { int value; }\n"
        "void ProbeCKParameterGenericSetScriptObject(CKParameterLocal@ local) {\n"
        "  CKParameterGenericRejected rejected;\n"
        "  local.SetValue(rejected);\n"
        "}\n"
        "void ProbeCKParameterGenericGetScriptObject(CKParameterLocal@ local) {\n"
        "  CKParameterGenericRejected rejected;\n"
        "  local.GetValue(rejected);\n"
        "}\n"
        "void ProbeCKParameterGenericSetObjectHandle(CKParameterLocal@ local) {\n"
        "  CKObject@ obj;\n"
        "  local.SetValue(@obj);\n"
        "}\n"
        "void ProbeCKParameterGenericGetObjectHandle(CKParameterLocal@ local) {\n"
        "  CKObject@ obj;\n"
        "  local.GetValue(@obj);\n"
        "}\n"
        "void ProbeCKParameterGenericSetNonPodObject(CKParameterLocal@ local) {\n"
        "  XString value(\"blocked\");\n"
        "  local.SetValue(value);\n"
        "}\n"
        "void ProbeCKParameterGenericGetNonPodObject(CKParameterLocal@ local) {\n"
        "  XString value;\n"
        "  local.GetValue(value);\n"
        "}\n"
        "int ProbeCKParameterInGenericStringValue(CKParameterIn@ pin) {\n"
        "  if (pin is null) return 2;\n"
        "  string value;\n"
        "  if (pin.GetValue(value) != CK_OK) return 3;\n"
        "  if (value != \"input text\") return 4;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterInGenericIntValue(CKParameterIn@ pin) {\n"
        "  if (pin is null) return 2;\n"
        "  int value = 0;\n"
        "  if (pin.GetValue(value) != CK_OK) return 3;\n"
        "  if (value != 2468) return 4;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterInGenericVectorValue(CKParameterIn@ pin) {\n"
        "  if (pin is null) return 2;\n"
        "  VxVector value;\n"
        "  if (pin.GetValue(value) != CK_OK) return 3;\n"
        "  if (value.x != 4.0f || value.y != 5.0f || value.z != 6.0f) return 4;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterInGenericMissingSource(CKParameterIn@ pin) {\n"
        "  if (pin is null) return 2;\n"
        "  int value = 123;\n"
        "  if (pin.GetValue(value) != CKERR_NOTINITIALIZED) return 3;\n"
        "  return 0;\n"
        "}\n"
        "void ProbeCKParameterInGenericGetScriptObject(CKParameterIn@ pin) {\n"
        "  CKParameterGenericRejected rejected;\n"
        "  pin.GetValue(rejected);\n"
        "}\n"
        "void ProbeCKParameterInGenericGetObjectHandle(CKParameterIn@ pin) {\n"
        "  CKObject@ obj;\n"
        "  pin.GetValue(@obj);\n"
        "}\n"
        "void ProbeCKParameterInGenericGetNonPodObject(CKParameterIn@ pin) {\n"
        "  XString value;\n"
        "  pin.GetValue(value);\n"
        "}\n"
        "int ProbeCKParameterInSetTypeAndGuid(CKParameterIn@ pin) {\n"
        "  if (pin is null) return 2;\n"
        "  CKParameterType type = pin.GetType();\n"
        "  if (type < 0) return 3;\n"
        "  CKGUID guid = pin.GetGUID();\n"
        "  pin.SetType(type);\n"
        "  if (pin.GetType() != type) return 4;\n"
        "  pin.SetType(type, false, \"TypeNameFromScript\");\n"
        "  if (pin.GetType() != type) return 5;\n"
        "  pin.SetGUID(guid);\n"
        "  if (pin.GetGUID() != guid) return 6;\n"
        "  pin.SetGUID(guid, false, \"GuidNameFromScript\");\n"
        "  if (pin.GetGUID() != guid) return 7;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterInSourceGraph(CKParameterIn@ pin, CKParameter@ sourceA, CKParameter@ sourceB, CKParameterIn@ shared) {\n"
        "  if (pin is null || sourceA is null || sourceB is null || shared is null) return 2;\n"
        "  NativeBuffer@ emptyData = pin.GetReadData();\n"
        "  if (emptyData is null || !emptyData.IsEmpty()) return 3;\n"
        "  if (pin.GetDirectSource() !is null) return 4;\n"
        "  if (pin.GetRealSource() !is null) return 5;\n"
        "  if (pin.SetDirectSource(sourceA) != CK_OK) return 6;\n"
        "  if (pin.GetDirectSource() !is sourceA) return 7;\n"
        "  if (pin.GetRealSource() !is sourceA) return 8;\n"
        "  NativeBuffer@ dataA = pin.GetReadData();\n"
        "  if (dataA is null || dataA.Size() != uint(sourceA.GetDataSize())) return 9;\n"
        "  int value = 0;\n"
        "  if (dataA.ReadInt(value) != 4 || value != 1111) return 10;\n"
        "  if (pin.SetDirectSource(sourceB) != CK_OK) return 11;\n"
        "  if (pin.GetDirectSource() !is sourceB) return 12;\n"
        "  if (pin.GetRealSource() !is sourceB) return 13;\n"
        "  NativeBuffer@ dataB = pin.GetReadData();\n"
        "  if (dataB is null || dataB.Size() != uint(sourceB.GetDataSize())) return 14;\n"
        "  value = 0;\n"
        "  if (dataB.ReadInt(value) != 4 || value != 2222) return 15;\n"
        "  if (shared.SetDirectSource(sourceA) != CK_OK) return 16;\n"
        "  if (pin.ShareSourceWith(shared) != CK_OK) return 17;\n"
        "  if (pin.GetSharedSource() !is shared) return 18;\n"
        "  if (pin.GetDirectSource() !is null) return 19;\n"
        "  if (pin.GetRealSource() !is sourceA) return 20;\n"
        "  value = 0;\n"
        "  NativeBuffer@ dataShared = pin.GetReadData();\n"
        "  if (dataShared is null || dataShared.Size() != uint(sourceA.GetDataSize())) return 21;\n"
        "  if (dataShared.ReadInt(value) != 4 || value != 1111) return 22;\n"
        "  return 0;\n"
        "}\n"
        "void ProbeCKParameterInShareNull(CKParameterIn@ pin, CKParameter@ sourceA, CKParameter@ sourceB, CKParameterIn@ shared) {\n"
        "  pin.ShareSourceWith(null);\n"
        "}\n"
        "void ProbeCKParameterInSetOwnerNull(CKParameterIn@ pin, CKParameter@ sourceA, CKParameter@ sourceB, CKParameterIn@ shared) {\n"
        "  pin.SetOwner(null);\n"
        "}\n"
        "void ProbeCKParameterInSetOwnerInvalid(CKParameterIn@ pin, CKParameter@ sourceA, CKParameter@ sourceB, CKParameterIn@ shared) {\n"
        "  pin.SetOwner(sourceA);\n"
        "}\n"
        "int ProbeCKParameterInSetOwnerValid(CKParameterIn@ pin, CKBehavior@ owner) {\n"
        "  if (pin is null || owner is null) return 2;\n"
        "  pin.SetOwner(owner);\n"
        "  if (pin.GetOwner() !is owner) return 3;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterOperationBasics(CKParameterOperation@ op) {\n"
        "  if (op is null) return 2;\n"
        "  if (op.GetInParameter1() is null) return 3;\n"
        "  if (op.GetInParameter2() is null) return 4;\n"
        "  if (op.GetOutParameter() is null) return 5;\n"
        "  if (!op.HasOperationFunction()) return 6;\n"
        "  CKGUID guid = op.GetOperationGuid();\n"
        "  if (!guid.IsValid()) return 7;\n"
        "  CKERROR err = op.DoOperation();\n"
        "  if (err != CK_OK && err != CKERR_NOTINITIALIZED) return 8;\n"
        "  op.Reconstruct(\"__CKAS_CKParameterOperationReconstruct\", guid, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT);\n"
        "  if (op.GetInParameter1() is null) return 9;\n"
        "  if (op.GetInParameter2() is null) return 10;\n"
        "  if (op.GetOutParameter() is null) return 11;\n"
        "  if (!op.HasOperationFunction()) return 12;\n"
        "  CKERROR errAfter = op.DoOperation();\n"
        "  if (errAfter != CK_OK && errAfter != CKERR_NOTINITIALIZED) return 13;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKParameterOperationOwner(CKParameterOperation@ op, CKBehavior@ owner) {\n"
        "  if (op is null || owner is null) return 2;\n"
        "  op.SetOwner(owner);\n"
        "  if (op.GetOwner() !is owner) return 3;\n"
        "  return 0;\n"
        "}\n"
        "void ProbeCKParameterOperationSetOwnerNull(CKParameterOperation@ op) {\n"
        "  op.SetOwner(null);\n"
        "}\n"
        "int ProbeCKParameterOutDestinations(CKParameterOut@ outp, CKParameter@ destA, CKParameter@ destB) {\n"
        "  if (outp is null || destA is null || destB is null) return 2;\n"
        "  outp.RemoveAllDestinations();\n"
        "  if (outp.GetDestinationCount() != 0) return 3;\n"
        "  if (outp.AddDestination(destA) != CK_OK) return 4;\n"
        "  if (outp.GetDestinationCount() != 1) return 5;\n"
        "  if (outp.GetDestination(0) !is destA) return 6;\n"
        "  if (outp.AddDestination(destB) != CK_OK) return 7;\n"
        "  if (outp.GetDestinationCount() != 2) return 8;\n"
        "  outp.DataChanged();\n"
        "  outp.RemoveDestination(destA);\n"
        "  if (outp.GetDestinationCount() != 1) return 9;\n"
        "  if (outp.GetDestination(0) !is destB) return 10;\n"
        "  outp.RemoveAllDestinations();\n"
        "  if (outp.GetDestinationCount() != 0) return 11;\n"
        "  return 0;\n"
        "}\n"
        "void ProbeCKParameterOutAddNull(CKParameterOut@ outp, CKParameter@ destA, CKParameter@ destB) {\n"
        "  outp.AddDestination(null);\n"
        "}\n"
        "void ProbeCKParameterOutRemoveNull(CKParameterOut@ outp, CKParameter@ destA, CKParameter@ destB) {\n"
        "  outp.RemoveDestination(null);\n"
        "}\n"
        "void ProbeCKParameterOutGetDestinationInvalid(CKParameterOut@ outp, CKParameter@ destA, CKParameter@ destB) {\n"
        "  outp.GetDestination(-1);\n"
        "}\n"
        "int ProbeCKParameterOutOwnerGraph(CKBehavior@ owner, CKParameterOut@ outp, CKParameterOut@ replacement) {\n"
        "  if (owner is null || outp is null || replacement is null) return 2;\n"
        "  if (owner.GetOutputParameterCount() < 2) return 3;\n"
        "  int pos = owner.GetOutputParameterPosition(outp);\n"
        "  if (pos < 0) return 4;\n"
        "  if (owner.GetOutputParameter(pos) !is outp) return 5;\n"
        "  int value = 27182;\n"
        "  if (outp.SetValue(value) != CK_OK) return 6;\n"
        "  int read = 0;\n"
        "  if (outp.GetValue(read) != CK_OK || read != value) return 7;\n"
        "  CKParameterOut@ old = owner.ReplaceOutputParameter(pos, replacement);\n"
        "  if (old !is outp) return 8;\n"
        "  if (owner.GetOutputParameter(pos) !is replacement) return 9;\n"
        "  CKParameterOut@ removed = owner.RemoveOutputParameter(pos);\n"
        "  if (removed !is replacement) return 10;\n"
        "  owner.AddOutputParameter(outp);\n"
        "  pos = owner.GetOutputParameterPosition(outp);\n"
        "  if (pos < 0 || owner.GetOutputParameter(pos) !is outp) return 11;\n"
        "  return 0;\n"
        "}\n"
        "void ProbeCKParameterCopyValueNull(CKParameterLocal@ local) {\n"
        "  local.CopyValue(null);\n"
        "}\n"
        "void ProbeCKParameterCompatibleNull(CKParameterLocal@ local) {\n"
        "  local.IsCompatibleWith(null);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKParameter self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-parameter-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKParameter self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKParameter self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKParameterStringValue(CKParameter@, CKParameterOut@, CKParameterLocal@)");
    asIScriptFunction *parameterTypeProbe = module->GetFunctionByDecl("int ProbeCKParameterType(CKParameterLocal@)");
    asIScriptFunction *genericString = module->GetFunctionByDecl("int ProbeCKParameterGenericStringValue(CKParameterLocal@)");
    asIScriptFunction *setString = module->GetFunctionByDecl("int ProbeCKParameterSetStringValue(CKParameterLocal@)");
    asIScriptFunction *genericSetString = module->GetFunctionByDecl("int ProbeCKParameterGenericSetStringValue(CKParameterLocal@)");
    asIScriptFunction *genericInt = module->GetFunctionByDecl("int ProbeCKParameterGenericIntValue(CKParameterLocal@)");
    asIScriptFunction *genericVector = module->GetFunctionByDecl("int ProbeCKParameterGenericVectorValue(CKParameterLocal@)");
    asIScriptFunction *localMyselfState = module->GetFunctionByDecl("int ProbeCKParameterLocalMyselfState(CKParameterLocal@)");
    asIScriptFunction *localOwnerGraph = module->GetFunctionByDecl("int ProbeCKParameterLocalOwnerGraph(CKBehavior@, CKParameterLocal@)");
    asIScriptFunction *genericSetScriptObject = module->GetFunctionByDecl("void ProbeCKParameterGenericSetScriptObject(CKParameterLocal@)");
    asIScriptFunction *genericGetScriptObject = module->GetFunctionByDecl("void ProbeCKParameterGenericGetScriptObject(CKParameterLocal@)");
    asIScriptFunction *genericSetObjectHandle = module->GetFunctionByDecl("void ProbeCKParameterGenericSetObjectHandle(CKParameterLocal@)");
    asIScriptFunction *genericGetObjectHandle = module->GetFunctionByDecl("void ProbeCKParameterGenericGetObjectHandle(CKParameterLocal@)");
    asIScriptFunction *genericSetNonPodObject = module->GetFunctionByDecl("void ProbeCKParameterGenericSetNonPodObject(CKParameterLocal@)");
    asIScriptFunction *genericGetNonPodObject = module->GetFunctionByDecl("void ProbeCKParameterGenericGetNonPodObject(CKParameterLocal@)");
    asIScriptFunction *pinString = module->GetFunctionByDecl("int ProbeCKParameterInGenericStringValue(CKParameterIn@)");
    asIScriptFunction *pinInt = module->GetFunctionByDecl("int ProbeCKParameterInGenericIntValue(CKParameterIn@)");
    asIScriptFunction *pinVector = module->GetFunctionByDecl("int ProbeCKParameterInGenericVectorValue(CKParameterIn@)");
    asIScriptFunction *pinMissingSource = module->GetFunctionByDecl("int ProbeCKParameterInGenericMissingSource(CKParameterIn@)");
    asIScriptFunction *pinGetScriptObject = module->GetFunctionByDecl("void ProbeCKParameterInGenericGetScriptObject(CKParameterIn@)");
    asIScriptFunction *pinGetObjectHandle = module->GetFunctionByDecl("void ProbeCKParameterInGenericGetObjectHandle(CKParameterIn@)");
    asIScriptFunction *pinGetNonPodObject = module->GetFunctionByDecl("void ProbeCKParameterInGenericGetNonPodObject(CKParameterIn@)");
    asIScriptFunction *pinSetTypeGuid = module->GetFunctionByDecl("int ProbeCKParameterInSetTypeAndGuid(CKParameterIn@)");
    asIScriptFunction *pinSourceGraph = module->GetFunctionByDecl("int ProbeCKParameterInSourceGraph(CKParameterIn@, CKParameter@, CKParameter@, CKParameterIn@)");
    asIScriptFunction *pinShareNull = module->GetFunctionByDecl("void ProbeCKParameterInShareNull(CKParameterIn@, CKParameter@, CKParameter@, CKParameterIn@)");
    asIScriptFunction *pinSetOwnerNull = module->GetFunctionByDecl("void ProbeCKParameterInSetOwnerNull(CKParameterIn@, CKParameter@, CKParameter@, CKParameterIn@)");
    asIScriptFunction *pinSetOwnerInvalid = module->GetFunctionByDecl("void ProbeCKParameterInSetOwnerInvalid(CKParameterIn@, CKParameter@, CKParameter@, CKParameterIn@)");
    asIScriptFunction *pinSetOwnerValid = module->GetFunctionByDecl("int ProbeCKParameterInSetOwnerValid(CKParameterIn@, CKBehavior@)");
    asIScriptFunction *operationBasics = module->GetFunctionByDecl("int ProbeCKParameterOperationBasics(CKParameterOperation@)");
    asIScriptFunction *operationOwner = module->GetFunctionByDecl("int ProbeCKParameterOperationOwner(CKParameterOperation@, CKBehavior@)");
    asIScriptFunction *operationOwnerNull = module->GetFunctionByDecl("void ProbeCKParameterOperationSetOwnerNull(CKParameterOperation@)");
    asIScriptFunction *outDestinations = module->GetFunctionByDecl("int ProbeCKParameterOutDestinations(CKParameterOut@, CKParameter@, CKParameter@)");
    asIScriptFunction *outAddNull = module->GetFunctionByDecl("void ProbeCKParameterOutAddNull(CKParameterOut@, CKParameter@, CKParameter@)");
    asIScriptFunction *outRemoveNull = module->GetFunctionByDecl("void ProbeCKParameterOutRemoveNull(CKParameterOut@, CKParameter@, CKParameter@)");
    asIScriptFunction *outGetInvalid = module->GetFunctionByDecl("void ProbeCKParameterOutGetDestinationInvalid(CKParameterOut@, CKParameter@, CKParameter@)");
    asIScriptFunction *outOwnerGraph = module->GetFunctionByDecl("int ProbeCKParameterOutOwnerGraph(CKBehavior@, CKParameterOut@, CKParameterOut@)");
    asIScriptFunction *copyValueNull = module->GetFunctionByDecl("void ProbeCKParameterCopyValueNull(CKParameterLocal@)");
    asIScriptFunction *compatibleNull = module->GetFunctionByDecl("void ProbeCKParameterCompatibleNull(CKParameterLocal@)");
    if (!probe || !parameterTypeProbe || !genericString || !setString || !genericSetString || !genericInt || !genericVector ||
        !localMyselfState || !localOwnerGraph ||
        !genericSetScriptObject || !genericGetScriptObject || !genericSetObjectHandle || !genericGetObjectHandle ||
        !genericSetNonPodObject || !genericGetNonPodObject || !pinString || !pinInt || !pinVector ||
        !pinMissingSource || !pinGetScriptObject || !pinGetObjectHandle || !pinGetNonPodObject ||
        !pinSetTypeGuid || !pinSourceGraph || !pinShareNull || !pinSetOwnerNull || !pinSetOwnerInvalid ||
        !pinSetOwnerValid || !operationBasics || !operationOwner || !operationOwnerNull || !outDestinations || !outAddNull || !outRemoveNull ||
        !outGetInvalid || !outOwnerGraph || !copyValueNull || !compatibleNull) {
        engine->DiscardModule(moduleName);
        error = "CKParameter self-test function was not found.";
        return false;
    }

    CKParameterLocal *local = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterGenericString"), CKPGUID_STRING, TRUE);
    if (!local) {
        engine->DiscardModule(moduleName);
        error = "CKParameter generic string probe could not create a local string parameter.";
        return false;
    }
    const CKERROR setErr = local->SetStringValue(const_cast<CKSTRING>("generic text"));
    bool ok = setErr == CK_OK;
    if (!ok) {
        error = "CKParameter generic string probe could not initialize the local string parameter.";
    } else {
        CKParameterLocal *intLocal = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterGenericInt"), CKPGUID_INT, TRUE);
        if (!intLocal) {
            ok = false;
            error = "CKParameter generic int probe could not create a local int parameter.";
        } else {
            CKParameterLocal *vectorLocal = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterGenericVector"), CKPGUID_VECTOR, TRUE);
            if (!vectorLocal) {
                ok = false;
                error = "CKParameter generic vector probe could not create a local vector parameter.";
            } else {
                CKParameterLocal *myselfLocal = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterLocalMyself"), CKPGUID_INT, TRUE);
                if (!myselfLocal) {
                    ok = false;
                    error = "CKParameterLocal myself probe could not create a local int parameter.";
                } else {
                    ok = ExecuteCKParameterLocalProbe(engine, parameterTypeProbe, local, false, "CKParameter GetParameterType probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericString, local, false, "CKParameter generic string probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, setString, local, false, "CKParameter SetStringValue probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericSetString, local, false, "CKParameter generic SetValue string probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericInt, intLocal, false, "CKParameter generic int probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericVector, vectorLocal, false, "CKParameter generic vector probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, localMyselfState, myselfLocal, false, "CKParameterLocal myself-state probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericSetScriptObject, local, true, "CKParameter generic SetValue script-object probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericGetScriptObject, local, true, "CKParameter generic GetValue script-object probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericSetObjectHandle, local, true, "CKParameter generic SetValue object-handle probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericGetObjectHandle, local, true, "CKParameter generic GetValue object-handle probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericSetNonPodObject, local, true, "CKParameter generic SetValue non-POD probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, genericGetNonPodObject, local, true, "CKParameter generic GetValue non-POD probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, copyValueNull, local, true, "CKParameter CopyValue null probe", error) &&
                     ExecuteCKParameterLocalProbe(engine, compatibleNull, local, true, "CKParameter IsCompatibleWith null probe", error);
                    context->DestroyObject(myselfLocal);
                }
                context->DestroyObject(vectorLocal);
            }
            context->DestroyObject(intLocal);
        }
    }
    context->DestroyObject(local);

    if (ok) {
        CKBehavior *owner = CKBehavior::Cast(context->CreateObject(
            CKCID_BEHAVIOR,
            const_cast<CKSTRING>("__CKAS_CKParameterLocalOwnerProbeBehavior"),
            CK_OBJECTCREATION_DYNAMIC));
        CKParameterLocal *ownedLocal = owner ? owner->CreateLocalParameter(const_cast<CKSTRING>("__CKAS_CKParameterLocalOwned"), CKPGUID_INT) : nullptr;
        if (!owner || !ownedLocal) {
            ok = false;
            error = "CKParameterLocal owner graph probe could not create its native behavior local.";
        } else {
            ok = ExecuteCKParameterLocalOwnerProbe(engine, localOwnerGraph, owner, ownedLocal, false, "CKParameterLocal owner graph probe", error);
        }
        if (owner) {
            context->DestroyObject(owner);
        }
    }

    if (ok) {
        CKParameterLocal *sourceString = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterInSourceString"), CKPGUID_STRING, TRUE);
        CKParameterLocal *sourceInt = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterInSourceInt"), CKPGUID_INT, TRUE);
        CKParameterLocal *sourceVector = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterInSourceVector"), CKPGUID_VECTOR, TRUE);
        CKParameterLocal *sourceGraphA = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterInSourceGraphA"), CKPGUID_INT, TRUE);
        CKParameterLocal *sourceGraphB = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterInSourceGraphB"), CKPGUID_INT, TRUE);
        CKParameterIn *inputString = context->CreateCKParameterIn(const_cast<CKSTRING>("__CKAS_CKParameterInString"), CKPGUID_STRING, TRUE);
        CKParameterIn *inputInt = context->CreateCKParameterIn(const_cast<CKSTRING>("__CKAS_CKParameterInInt"), CKPGUID_INT, TRUE);
        CKParameterIn *inputVector = context->CreateCKParameterIn(const_cast<CKSTRING>("__CKAS_CKParameterInVector"), CKPGUID_VECTOR, TRUE);
        CKParameterIn *inputMissing = context->CreateCKParameterIn(const_cast<CKSTRING>("__CKAS_CKParameterInMissing"), CKPGUID_INT, TRUE);
        CKParameterIn *inputGraph = context->CreateCKParameterIn(const_cast<CKSTRING>("__CKAS_CKParameterInGraph"), CKPGUID_INT, TRUE);
        CKParameterIn *inputShared = context->CreateCKParameterIn(const_cast<CKSTRING>("__CKAS_CKParameterInShared"), CKPGUID_INT, TRUE);
        if (!sourceString || !sourceInt || !sourceVector || !sourceGraphA || !sourceGraphB ||
            !inputString || !inputInt || !inputVector || !inputMissing || !inputGraph || !inputShared) {
            ok = false;
            error = "CKParameterIn generic probe could not create its native parameters.";
        } else {
            int intValue = 2468;
            int graphValueA = 1111;
            int graphValueB = 2222;
            VxVector vectorValue;
            vectorValue.x = 4.0f;
            vectorValue.y = 5.0f;
            vectorValue.z = 6.0f;
            if (sourceString->SetStringValue(const_cast<CKSTRING>("input text")) != CK_OK ||
                sourceInt->SetValue(&intValue, sizeof(intValue)) != CK_OK ||
                sourceVector->SetValue(&vectorValue, sizeof(vectorValue)) != CK_OK ||
                sourceGraphA->SetValue(&graphValueA, sizeof(graphValueA)) != CK_OK ||
                sourceGraphB->SetValue(&graphValueB, sizeof(graphValueB)) != CK_OK ||
                inputString->SetDirectSource(sourceString) != CK_OK ||
                inputInt->SetDirectSource(sourceInt) != CK_OK ||
                inputVector->SetDirectSource(sourceVector) != CK_OK) {
                ok = false;
                error = "CKParameterIn generic probe could not initialize its native sources.";
            } else {
                ok = ExecuteCKParameterInProbe(engine, pinString, inputString, false, "CKParameterIn generic string probe", error) &&
                     ExecuteCKParameterInProbe(engine, pinInt, inputInt, false, "CKParameterIn generic int probe", error) &&
                     ExecuteCKParameterInProbe(engine, pinVector, inputVector, false, "CKParameterIn generic vector probe", error) &&
                     ExecuteCKParameterInProbe(engine, pinMissingSource, inputMissing, false, "CKParameterIn generic missing-source probe", error) &&
                     ExecuteCKParameterInProbe(engine, pinGetScriptObject, inputString, true, "CKParameterIn generic script-object probe", error) &&
                     ExecuteCKParameterInProbe(engine, pinGetObjectHandle, inputString, true, "CKParameterIn generic object-handle probe", error) &&
                     ExecuteCKParameterInProbe(engine, pinGetNonPodObject, inputString, true, "CKParameterIn generic non-POD probe", error) &&
                     ExecuteCKParameterInProbe(engine, pinSetTypeGuid, inputInt, false, "CKParameterIn SetType/SetGUID probe", error) &&
                     ExecuteCKParameterInSourceProbe(engine, pinSourceGraph, inputGraph, sourceGraphA, sourceGraphB, inputShared, false, "CKParameterIn source graph probe", error) &&
                     ExecuteCKParameterInSourceProbe(engine, pinShareNull, inputGraph, sourceGraphA, sourceGraphB, inputShared, true, "CKParameterIn ShareSourceWith null probe", error) &&
                     ExecuteCKParameterInSourceProbe(engine, pinSetOwnerNull, inputGraph, sourceGraphA, sourceGraphB, inputShared, true, "CKParameterIn SetOwner null probe", error) &&
                     ExecuteCKParameterInSourceProbe(engine, pinSetOwnerInvalid, inputGraph, sourceGraphA, sourceGraphB, inputShared, true, "CKParameterIn SetOwner invalid probe", error);
            }
        }
        if (inputGraph)
            context->DestroyObject(inputGraph);
        if (inputShared)
            context->DestroyObject(inputShared);
        if (inputMissing)
            context->DestroyObject(inputMissing);
        if (inputVector)
            context->DestroyObject(inputVector);
        if (inputInt)
            context->DestroyObject(inputInt);
        if (inputString)
            context->DestroyObject(inputString);
        if (sourceVector)
            context->DestroyObject(sourceVector);
        if (sourceInt)
            context->DestroyObject(sourceInt);
        if (sourceString)
            context->DestroyObject(sourceString);
        if (sourceGraphB)
            context->DestroyObject(sourceGraphB);
        if (sourceGraphA)
            context->DestroyObject(sourceGraphA);
    }

    if (ok) {
        CKBehavior *owner = CKBehavior::Cast(context->CreateObject(
            CKCID_BEHAVIOR,
            const_cast<CKSTRING>("__CKAS_CKParameterInOwnerProbeBehavior"),
            CK_OBJECTCREATION_DYNAMIC));
        CKParameterIn *ownedInput = owner ? owner->CreateInputParameter(const_cast<CKSTRING>("__CKAS_CKParameterInOwnedInput"), CKPGUID_INT) : nullptr;
        if (!owner || !ownedInput) {
            ok = false;
            error = "CKParameterIn valid-owner probe could not create its native behavior input.";
        } else {
            ok = ExecuteCKParameterInOwnerProbe(engine, pinSetOwnerValid, ownedInput, owner, false, "CKParameterIn SetOwner valid probe", error);
        }
        if (owner && ownedInput) {
            const int pos = owner->GetInputParameterPosition(ownedInput);
            if (pos >= 0) {
                owner->RemoveInputParameter(pos);
            }
        }
        if (owner) {
            context->DestroyObject(owner);
        }
    }

    if (ok) {
        const CKGUID operationGuid = FindCKParameterOperationSelfTestGuid(context);
        if (operationGuid.IsValid()) {
            CKParameterOperation *operation = context->CreateCKParameterOperation(
                const_cast<CKSTRING>("__CKAS_CKParameterOperationScriptProbe"),
                operationGuid,
                CKPGUID_INT,
                CKPGUID_INT,
                CKPGUID_INT);
            CKBehavior *owner = CKBehavior::Cast(context->CreateObject(
                CKCID_BEHAVIOR,
                const_cast<CKSTRING>("__CKAS_CKParameterOperationOwnerProbe"),
                CK_OBJECTCREATION_DYNAMIC));
            if (!operation) {
                ok = false;
                error = "CKParameterOperation probe could not create its native operation.";
            } else if (!owner) {
                ok = false;
                error = "CKParameterOperation probe could not create its native owner behavior.";
            } else {
                ok = ExecuteCKParameterOperationProbe(engine, operationBasics, operation, false, "CKParameterOperation basics probe", error) &&
                     ExecuteCKParameterOperationOwnerProbe(engine, operationOwner, operation, owner, false, "CKParameterOperation owner probe", error) &&
                     ExecuteCKParameterOperationProbe(engine, operationOwnerNull, operation, true, "CKParameterOperation SetOwner null probe", error);
            }
            if (operation) {
                if (owner) {
                    owner->RemoveParameterOperation(operation);
                }
                context->DestroyObject(operation);
            }
            if (owner) {
                context->DestroyObject(owner);
            }
        }
    }

    if (ok) {
        CKBehavior *owner = CKBehavior::Cast(context->CreateObject(
            CKCID_BEHAVIOR,
            const_cast<CKSTRING>("__CKAS_CKParameterOutOwnerProbeBehavior"),
            CK_OBJECTCREATION_DYNAMIC));
        CKParameterOut *ownedOut = owner ? owner->CreateOutputParameter(const_cast<CKSTRING>("__CKAS_CKParameterOutOwned"), CKPGUID_INT) : nullptr;
        CKParameterOut *replacementOut = owner ? owner->CreateOutputParameter(const_cast<CKSTRING>("__CKAS_CKParameterOutReplacement"), CKPGUID_INT) : nullptr;
        if (!owner || !ownedOut || !replacementOut) {
            ok = false;
            error = "CKParameterOut owner graph probe could not create its native behavior outputs.";
        } else {
            ok = ExecuteCKParameterOutOwnerProbe(engine, outOwnerGraph, owner, ownedOut, replacementOut, false, "CKParameterOut owner graph probe", error);
        }
        if (owner) {
            context->DestroyObject(owner);
        }
    }

    if (ok) {
        CKParameterOut *outParam = context->CreateCKParameterOut(const_cast<CKSTRING>("__CKAS_CKParameterOutDestinationSource"), CKPGUID_INT, TRUE);
        CKParameterLocal *destinationA = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterOutDestinationA"), CKPGUID_INT, TRUE);
        CKParameterLocal *destinationB = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_CKParameterOutDestinationB"), CKPGUID_INT, TRUE);
        if (!outParam || !destinationA || !destinationB) {
            ok = false;
            error = "CKParameterOut destination probe could not create its native parameters.";
        } else {
            ok = ExecuteCKParameterOutDestinationProbe(engine, outDestinations, outParam, destinationA, destinationB, false, "CKParameterOut destination probe", error) &&
                 ExecuteCKParameterOutDestinationProbe(engine, outAddNull, outParam, destinationA, destinationB, true, "CKParameterOut AddDestination null probe", error) &&
                 ExecuteCKParameterOutDestinationProbe(engine, outRemoveNull, outParam, destinationA, destinationB, true, "CKParameterOut RemoveDestination null probe", error) &&
                 ExecuteCKParameterOutDestinationProbe(engine, outGetInvalid, outParam, destinationA, destinationB, true, "CKParameterOut GetDestination invalid probe", error);
        }
        if (outParam) {
            context->DestroyObject(outParam);
        }
        if (destinationB) {
            context->DestroyObject(destinationB);
        }
        if (destinationA) {
            context->DestroyObject(destinationA);
        }
    }

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKParameterTypeDescScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKParameterTypeDesc script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *parameterManagerType = engine->GetTypeInfoByDecl("CKParameterManager");
    if (!parameterManagerType) {
        error = "CKParameterManager type is not registered.";
        return false;
    }

    if (parameterManagerType->GetMethodByDecl("CKERROR RegisterOperationFunction(const CKGUID &in operation, const CKGUID &in paramRes, const CKGUID &in param1, const CKGUID &in param2, NativePointer op)") == nullptr ||
        parameterManagerType->GetMethodByDecl("NativePointer GetOperationFunction(const CKGUID &in operation, const CKGUID &in paramRes, const CKGUID &in param1, const CKGUID &in param2)") == nullptr ||
        parameterManagerType->GetMethodByDecl("CKERROR UnRegisterOperationFunction(const CKGUID &in operation, const CKGUID &in paramRes, const CKGUID &in param1, const CKGUID &in param2)") == nullptr) {
        error = "CKParameterManager operation function GUID input declarations are not registered.";
        return false;
    }
    if (parameterManagerType->GetMethodByDecl("array<CKOperationDesc>@ GetAvailableOperationsDesc(const CKGUID &in opGuid, CKParameterOut@ res = null, CKParameterIn@ p1 = null, CKParameterIn@ p2 = null)") == nullptr ||
        parameterManagerType->GetMethodByDecl("int GetAvailableOperationsDesc(const CKGUID &in opGuid, CKParameterOut@ res, CKParameterIn@ p1, CKParameterIn@ p2, CKOperationDesc &out list)") != nullptr) {
        error = "CKParameterManager operation description array declaration is stale or missing.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKParameterTypeDescSelfTest";
    const char *source =
        "int ProbeParameterTypeDesc(CKContext@ ctx) {\n"
        "  CKParameterTypeDesc desc;\n"
        "  if (!desc.CreatorDll.IsNull()) return 1;\n"
        "  NativePointer empty;\n"
        "  desc.CreatorDll = empty;\n"
        "  if (!desc.CreatorDll.IsNull()) return 2;\n"
        "  desc.CreateDefaultFunction = empty;\n"
        "  desc.DeleteFunction = empty;\n"
        "  desc.SaveLoadFunction = empty;\n"
        "  desc.CheckFunction = empty;\n"
        "  desc.CopyFunction = empty;\n"
        "  desc.StringFunction = empty;\n"
        "  desc.UICreatorFunction = empty;\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  if (pm is null) return 3;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCopiedParameterTypeDescClears(CKContext@ ctx) {\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  if (pm is null) return 1;\n"
        "  CKParameterTypeDesc desc = pm.GetParameterTypeDescription(CKPGUID_INT);\n"
        "  NativePointer empty;\n"
        "  desc.CreatorDll = empty;\n"
        "  desc.CreateDefaultFunction = empty;\n"
        "  desc.DeleteFunction = empty;\n"
        "  desc.SaveLoadFunction = empty;\n"
        "  desc.CheckFunction = empty;\n"
        "  desc.CopyFunction = empty;\n"
        "  desc.StringFunction = empty;\n"
        "  desc.UICreatorFunction = empty;\n"
        "  if (!desc.CreatorDll.IsNull()) return 2;\n"
        "  if (!desc.CreateDefaultFunction.IsNull() || !desc.DeleteFunction.IsNull()) return 3;\n"
        "  if (!desc.SaveLoadFunction.IsNull() || !desc.CheckFunction.IsNull()) return 4;\n"
        "  if (!desc.CopyFunction.IsNull() || !desc.StringFunction.IsNull() || !desc.UICreatorFunction.IsNull()) return 5;\n"
        "  return 0;\n"
        "}\n"
        "void RejectCopiedParameterTypeRegister(CKContext@ ctx) {\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  CKParameterTypeDesc desc = pm.GetParameterTypeDescription(CKPGUID_INT);\n"
        "  desc.Guid = CKGUID(0x72656770, 0x74797065);\n"
        "  desc.TypeName = \"__CKAS_UnsafeCopiedParameterType\";\n"
        "  pm.RegisterParameterType(desc);\n"
        "}\n"
        "int ProbeMissingParameterType(CKContext@ ctx) {\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  pm.GetParameterTypeDescription(-2147483647);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMissingParameterGuid(CKContext@ ctx) {\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  CKGUID missing(0x7badc0de, 0x13572468);\n"
        "  pm.GetParameterTypeDescription(missing);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeOperationFunctionGuidInputs(CKContext@ ctx) {\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  if (pm is null) return 1;\n"
        "  CKGUID operation(0x7badc0de, 0x13572468);\n"
        "  CKGUID paramRes(0x11111111, 0x22222222);\n"
        "  CKGUID param1(0x33333333, 0x44444444);\n"
        "  CKGUID param2(0x55555555, 0x66666666);\n"
        "  CKGUID operationCopy = operation;\n"
        "  CKGUID paramResCopy = paramRes;\n"
        "  CKGUID param1Copy = param1;\n"
        "  CKGUID param2Copy = param2;\n"
        "  NativePointer empty;\n"
        "  pm.RegisterOperationFunction(operation, paramRes, param1, param2, empty);\n"
        "  pm.GetOperationFunction(operation, paramRes, param1, param2);\n"
        "  pm.UnRegisterOperationFunction(operation, paramRes, param1, param2);\n"
        "  if (operation.opCmp(operationCopy) != 0 || paramRes.opCmp(paramResCopy) != 0) return 2;\n"
        "  if (param1.opCmp(param1Copy) != 0 || param2.opCmp(param2Copy) != 0) return 3;\n"
        "  return 0;\n"
        "}\n"
        "void RejectOperationFunctionPointer(CKContext@ ctx) {\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  CKGUID operation(0x7badc0de, 0x13572468);\n"
        "  CKGUID paramRes(0x11111111, 0x22222222);\n"
        "  CKGUID param1(0x33333333, 0x44444444);\n"
        "  CKGUID param2(0x55555555, 0x66666666);\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  pm.RegisterOperationFunction(operation, paramRes, param1, param2, ptr);\n"
        "}\n"
        "int ProbeAvailableOperationDescriptions(CKContext@ ctx) {\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  if (pm is null) return 1;\n"
        "  CKGUID anyOperation;\n"
        "  array<CKOperationDesc>@ descriptions = pm.GetAvailableOperationsDesc(anyOperation, null, null, null);\n"
        "  if (descriptions is null) return 2;\n"
        "  descriptions.length();\n"
        "  return 0;\n"
        "}\n"
        "void RejectParameterTypeCreatorDll(CKContext@) { CKParameterTypeDesc d; NativePointer p; p += 1; d.CreatorDll = p; }\n"
        "void RejectParameterTypeCreateDefaultFunction(CKContext@) { CKParameterTypeDesc d; NativePointer p; p += 1; d.CreateDefaultFunction = p; }\n"
        "void RejectParameterTypeDeleteFunction(CKContext@) { CKParameterTypeDesc d; NativePointer p; p += 1; d.DeleteFunction = p; }\n"
        "void RejectParameterTypeSaveLoadFunction(CKContext@) { CKParameterTypeDesc d; NativePointer p; p += 1; d.SaveLoadFunction = p; }\n"
        "void RejectParameterTypeCheckFunction(CKContext@) { CKParameterTypeDesc d; NativePointer p; p += 1; d.CheckFunction = p; }\n"
        "void RejectParameterTypeCopyFunction(CKContext@) { CKParameterTypeDesc d; NativePointer p; p += 1; d.CopyFunction = p; }\n"
        "void RejectParameterTypeStringFunction(CKContext@) { CKParameterTypeDesc d; NativePointer p; p += 1; d.StringFunction = p; }\n"
        "void RejectParameterTypeUICreatorFunction(CKContext@) { CKParameterTypeDesc d; NativePointer p; p += 1; d.UICreatorFunction = p; }\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKParameterTypeDesc self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-parameter-type-desc-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKParameterTypeDesc self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKParameterTypeDesc self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeParameterTypeDesc(CKContext@)");
    asIScriptFunction *clearCopied = module->GetFunctionByDecl("int ProbeCopiedParameterTypeDescClears(CKContext@)");
    asIScriptFunction *rejectCopiedRegister = module->GetFunctionByDecl("void RejectCopiedParameterTypeRegister(CKContext@)");
    asIScriptFunction *missingType = module->GetFunctionByDecl("int ProbeMissingParameterType(CKContext@)");
    asIScriptFunction *missingGuid = module->GetFunctionByDecl("int ProbeMissingParameterGuid(CKContext@)");
    asIScriptFunction *operationGuidInputs = module->GetFunctionByDecl("int ProbeOperationFunctionGuidInputs(CKContext@)");
    asIScriptFunction *rejectOperationFunction = module->GetFunctionByDecl("void RejectOperationFunctionPointer(CKContext@)");
    asIScriptFunction *operationDescriptions = module->GetFunctionByDecl("int ProbeAvailableOperationDescriptions(CKContext@)");
    asIScriptFunction *rejectCreatorDll = module->GetFunctionByDecl("void RejectParameterTypeCreatorDll(CKContext@)");
    asIScriptFunction *rejectCreateDefault = module->GetFunctionByDecl("void RejectParameterTypeCreateDefaultFunction(CKContext@)");
    asIScriptFunction *rejectDelete = module->GetFunctionByDecl("void RejectParameterTypeDeleteFunction(CKContext@)");
    asIScriptFunction *rejectSaveLoad = module->GetFunctionByDecl("void RejectParameterTypeSaveLoadFunction(CKContext@)");
    asIScriptFunction *rejectCheck = module->GetFunctionByDecl("void RejectParameterTypeCheckFunction(CKContext@)");
    asIScriptFunction *rejectCopy = module->GetFunctionByDecl("void RejectParameterTypeCopyFunction(CKContext@)");
    asIScriptFunction *rejectString = module->GetFunctionByDecl("void RejectParameterTypeStringFunction(CKContext@)");
    asIScriptFunction *rejectUICreator = module->GetFunctionByDecl("void RejectParameterTypeUICreatorFunction(CKContext@)");
    if (!probe || !clearCopied || !rejectCopiedRegister || !missingType || !missingGuid || !operationGuidInputs || !rejectOperationFunction || !operationDescriptions || !rejectCreatorDll || !rejectCreateDefault ||
        !rejectDelete || !rejectSaveLoad || !rejectCheck || !rejectCopy || !rejectString || !rejectUICreator) {
        engine->DiscardModule(moduleName);
        error = "CKParameterTypeDesc self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKParameterTypeDesc CreatorDll probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, clearCopied, context, false, "CKParameterTypeDesc copied callback clear probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, missingType, context, true, "CKParameterTypeDesc missing type probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, missingGuid, context, true, "CKParameterTypeDesc missing guid probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, operationGuidInputs, context, false, "CKParameterManager operation GUID input probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, rejectOperationFunction, context, true, "CKParameterManager operation function rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, operationDescriptions, context, false, "CKParameterManager operation description array probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, rejectCreatorDll, context, true, "CKParameterTypeDesc CreatorDll rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, rejectCreateDefault, context, true, "CKParameterTypeDesc CreateDefaultFunction rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, rejectDelete, context, true, "CKParameterTypeDesc DeleteFunction rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, rejectSaveLoad, context, true, "CKParameterTypeDesc SaveLoadFunction rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, rejectCheck, context, true, "CKParameterTypeDesc CheckFunction rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, rejectCopy, context, true, "CKParameterTypeDesc CopyFunction rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, rejectString, context, true, "CKParameterTypeDesc StringFunction rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, rejectUICreator, context, true, "CKParameterTypeDesc UICreatorFunction rejection probe", error);

    if (ok) {
        if (CKParameterManager *pm = context->GetParameterManager()) {
            if (CKParameterTypeDesc *desc = pm->GetParameterTypeDescription(CKPGUID_INT)) {
                const bool hasNativeCallbacks = desc->CreatorDll ||
                                                desc->CreateDefaultFunction ||
                                                desc->DeleteFunction ||
                                                desc->SaveLoadFunction ||
                                                desc->CheckFunction ||
                                                desc->CopyFunction ||
                                                desc->StringFunction ||
                                                desc->UICreatorFunction;
                if (hasNativeCallbacks) {
                    ok = ExecuteCKParameterTypeDescProbe(engine, rejectCopiedRegister, context, true, "CKParameterTypeDesc unsafe copied registration probe", error);
                }
            }
        }
    }

    engine->DiscardModule(moduleName);
    return ok;
}

} // namespace

bool RunScriptParameterRegistrySelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    if (!registry) {
        error = "ScriptParameterRegistry is not available.";
        return false;
    }

    bool sawEnum = false;
    bool sawFlags = false;
    bool sawStruct = false;
    CKParameterManager *pm = registry->GetParameterManager();
    if (!pm) {
        error = "CKParameterManager is not available.";
        return false;
    }

    for (int i = 0; i < pm->GetParameterTypesCount(); ++i) {
        const ScriptParamTypeRecord *record = registry->GetType(i);
        if (!record) {
            continue;
        }

        if (!sawEnum && record->Has(ScriptParamTypeCaps::EnumLike) && !record->EnumEntries.empty()) {
            int value = 0;
            std::string parseError;
            if (!registry->ParseEnumValue(record->Guid, record->EnumEntries[0].Name, value, parseError) ||
                value != record->EnumEntries[0].Value) {
                error = "Enum registry lookup failed: " + parseError;
                return false;
            }
            if (registry->ParseEnumValue(record->Guid, "__missing_enum__", value, parseError)) {
                error = "Enum registry accepted an unknown token.";
                return false;
            }
            sawEnum = true;
        }

        if (!sawFlags && record->Has(ScriptParamTypeCaps::FlagsLike) && !record->FlagEntries.empty()) {
            CKDWORD value = 0;
            std::string parseError;
            if (!registry->ParseFlagsValue(record->Guid, record->FlagEntries[0].Name, value, parseError) ||
                value != static_cast<CKDWORD>(record->FlagEntries[0].Value)) {
                error = "Flags registry lookup failed: " + parseError;
                return false;
            }
            if (registry->ParseFlagsValue(record->Guid, "__missing_flag__", value, parseError)) {
                error = "Flags registry accepted an unknown token.";
                return false;
            }
            sawFlags = true;
        }

        if (!sawStruct && record->Has(ScriptParamTypeCaps::StructLike) && !record->StructMembers.empty()) {
            if (record->StructMembers[0].Guid == CKGUID() || record->StructMembers[0].Name.empty()) {
                error = "Struct registry member metadata is incomplete.";
                return false;
            }
            sawStruct = true;
        }
    }

    if (!sawEnum || !sawFlags || !sawStruct) {
        error = "Registry did not find enum, flags, and struct parameter metadata.";
        return false;
    }

    const CKGUID enumGuid(0x6a33ce51, 0x22705501);
    CKERROR err = registry->RegisterEnum(enumGuid, "__CKAS_TestEnum", "First=7,Second=9");
    if (err == CK_OK) {
        int value = 0;
        std::string parseError;
        if (!registry->ParseEnumValue(enumGuid, "Second", value, parseError) || value != 9) {
            error = "Registered enum did not refresh registry cache.";
            return false;
        }
    }

    if (!RunCKEnumStructScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKFlagsStructScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKStructStructScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKGUIDScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKSquareScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKStatsScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPluginEntryBehaviorsDataScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPluginInfoScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPluginEntryReadersDataScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPluginEntryScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKFileExtensionScriptSelfTest(engine, error)) {
        return false;
    }
#if CKVERSION == 0x13022002
    if (!RunCKPICKRESULTScriptSelfTest(engine, error)) {
        return false;
    }
#endif
    if (!RunCKDataRowItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXIntArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXIntArrayScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXDwordArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXDwordArrayScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXFileObjectsTableScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXHashIDScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXClassIDArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXGUIDArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXImageArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXObjectArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXSObjectArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXObjectPointerArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXSObjectPointerArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXObjectDeclarationArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXStringArrayItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXStringArrayScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPathEntryVectorScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPathCategoryVectorScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunXClassArrayRemoveAtScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPathCategoryVectorItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPathEntryVectorItScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAttributeDescScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAttributeCategoryDescScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCK2dCurvePointScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCK2dCurveScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunVxColorScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunVxEffectDescriptionScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKOperationDescScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKClassDescScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKObjectDeclarationScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKDependenciesScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKTimeProfilerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPositionKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKRotationKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKTCBPositionKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKTCBRotationKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKMorphKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAnimControllerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKMorphControllerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKObjectAnimationScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKAnimationScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKKeyedAnimationScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKBezierPositionKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAttributeManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBaseManagerCastScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKTimeManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBehaviorManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKMessageManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKGridManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKFloorManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKInterfaceManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKMidiManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKListenerSettingsScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKSoundManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKWaveSoundScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKContextScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKObjectScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKSceneObjectScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBeObjectScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKGroupScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKRenderContextScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKRenderObjectScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKMeshScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCK2dEntityScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCK3dEntityScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCK3dObjectScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKCameraScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKTargetCameraScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKLightScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKTargetLightScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKSprite3DScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKGridScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKCurveScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKCurvePointScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKCharacterScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBodyPartScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBehaviorScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBehaviorIOScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBehaviorLinkScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKDataArrayScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKObjectManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKPathManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKInputManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKRenderManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBitmapPropertiesScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKMoviePropertiesScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKTextureScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBitmapSlotScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKBitmapReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKSoundReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKMovieReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKModelReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKDataReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPluginManagerScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKBehaviorPrototypeScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKMaterialScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKStateChunkScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKParameterScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKParameterTypeDescScriptSelfTest(context, engine, error)) {
        return false;
    }

    error.clear();
    return true;
}
