#include "CKAll.h"

#include "Logger.h"
#include "AngelScriptEventHook.h"
#include "ScriptManager.h"
#include "Version.h"

#define ANGELSCRIPT_BEHAVIOR CKGUID(0x2d9c2922,0x29a5c30)
#define CKAS_PLUGIN_VERSION ((CKDWORD) ((CKAS_VERSION_MAJOR << 24) | (CKAS_VERSION_MINOR << 16) | (CKAS_VERSION_PATCH << 8) | CKAS_VERSION_TWEAK))

CKObjectDeclaration *FillBehaviorAngelScriptComponentDecl();

static bool InitAngelScriptLogger() {
    static bool initialized = false;
    if (initialized) {
        return true;
    }

#ifndef NDEBUG
    initialized = Logger::Get().Init("AngelScript.log", LOG_LEVEL_DEBUG);
#else
    initialized = Logger::Get().Init("AngelScript.log", LOG_LEVEL_INFO);
#endif
    return initialized;
}

CKERROR InitInstance(CKContext *context) {
    InitAngelScriptLogger();
    new ScriptManager(context);

    return CK_OK;
}

CKERROR ExitInstance(CKContext *context) {
    ScriptManager *man = ScriptManager::GetManager(context);
    delete man;

    return CK_OK;
}

CKPluginInfo g_PluginInfo[2];

PLUGIN_EXPORT int CKGetPluginInfoCount() { return 2; }

PLUGIN_EXPORT CKPluginInfo *CKGetPluginInfo(int Index) {
    g_PluginInfo[0].m_Author = "Kakuty";
    g_PluginInfo[0].m_Description = "AngelScript Building Blocks";
    g_PluginInfo[0].m_Extension = "";
    g_PluginInfo[0].m_Type = CKPLUGIN_BEHAVIOR_DLL;
    // Packed as major.minor.patch.tweak across four bytes, sourced from CMake project(VERSION).
    g_PluginInfo[0].m_Version = CKAS_PLUGIN_VERSION;
    g_PluginInfo[0].m_InitInstanceFct = NULL;
    g_PluginInfo[0].m_ExitInstanceFct = NULL;
    g_PluginInfo[0].m_GUID = ANGELSCRIPT_BEHAVIOR;
    g_PluginInfo[0].m_Summary = "AngelScript Building Blocks";

    g_PluginInfo[1].m_Author = "Kakuty";
    g_PluginInfo[1].m_Description = "AngelScript Manager";
    g_PluginInfo[1].m_Extension = "";
    g_PluginInfo[1].m_Type = CKPLUGIN_MANAGER_DLL;
    g_PluginInfo[1].m_Version = CKAS_PLUGIN_VERSION;
    g_PluginInfo[1].m_InitInstanceFct = InitInstance;
    g_PluginInfo[1].m_ExitInstanceFct = ExitInstance;
    g_PluginInfo[1].m_GUID = SCRIPT_MANAGER_GUID;
    g_PluginInfo[1].m_Summary = "AngelScript Manager";
    return &g_PluginInfo[Index];
}

//	This function should be present and exported for Nemo
//	to be able to retrieve objects declarations.
//	Nemo will call this function at initialization
PLUGIN_EXPORT void RegisterBehaviorDeclarations(XObjectDeclarationArray *reg);

void RegisterBehaviorDeclarations(XObjectDeclarationArray *reg) {
    InitAngelScriptLogger();
    RegisterBehavior(reg, FillBehaviorAngelScriptComponentDecl);
    RegisterBehavior(reg, FillBehaviorAngelScriptEventHookDecl);
}
