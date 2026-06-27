#ifndef CK_SCRIPT_ANGELSCRIPT_GC_H
#define CK_SCRIPT_ANGELSCRIPT_GC_H

#include <angelscript.h>

class ScriptAutoGarbageCollectScope {
public:
    ScriptAutoGarbageCollectScope(asIScriptEngine *engine, bool enabled)
        : m_Engine(engine),
          m_Previous(engine ? engine->GetEngineProperty(asEP_AUTO_GARBAGE_COLLECT) : 0),
          m_Changed(engine && m_Previous != static_cast<asPWORD>(enabled ? 1 : 0)) {
        if (m_Changed) {
            m_Engine->SetEngineProperty(asEP_AUTO_GARBAGE_COLLECT, enabled ? 1 : 0);
        }
    }

    ~ScriptAutoGarbageCollectScope() {
        if (m_Changed && m_Engine) {
            m_Engine->SetEngineProperty(asEP_AUTO_GARBAGE_COLLECT, m_Previous);
        }
    }

    ScriptAutoGarbageCollectScope(const ScriptAutoGarbageCollectScope &) = delete;
    ScriptAutoGarbageCollectScope &operator=(const ScriptAutoGarbageCollectScope &) = delete;

private:
    asIScriptEngine *m_Engine = nullptr;
    asPWORD m_Previous = 0;
    bool m_Changed = false;
};

inline void ScriptRunBoundedGarbageCollection(asIScriptEngine *engine) {
    if (engine) {
        engine->GarbageCollect(asGC_ONE_STEP | asGC_DETECT_GARBAGE | asGC_DESTROY_GARBAGE, 1);
    }
}

#endif // CK_SCRIPT_ANGELSCRIPT_GC_H
