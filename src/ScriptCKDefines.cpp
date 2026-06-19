#include "ScriptCKDefines.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "CKAll.h"

#include "ScriptUtils.h"
#include "ScriptXArray.h"
#include "ScriptNativeBuffer.h"
#include "ScriptNativePointer.h"
#include "ScriptRegistration.h"
#include "ScriptCKVertexBuffer.h"

static const int g_MAX_USER_PROFILE = MAX_USER_PROFILE;

static const uint32_t g_CKVERSION = CKVERSION;
static const uint32_t g_DEVVERSION = DEVVERSION;
static const CKGUID g_VIRTOOLS_GUID = VIRTOOLS_GUID;
static const float g_CK_ZERO = CK_ZERO;
static const int g_CKMAX_PATH = CKMAX_PATH;
static const int g_CKMAX_URL = CKMAX_URL;
static const int g_CKMAX_MANAGERFUNCTIONS = CKMAX_MANAGERFUNCTIONS;
static const int g_CKANIMATION_FORCESETSTEP = CKANIMATION_FORCESETSTEP;

static const int g_CKOBJECT_PRIORITYMAX = CKOBJECT_PRIORITYMAX;
static const int g_CKOBJECT_PRIORITYLEVEL = CKOBJECT_PRIORITYLEVEL;
static const int g_CKOBJECT_PRIORITYSCENE = CKOBJECT_PRIORITYSCENE;
static const int g_CKOBJECT_PRIORITYPLACE = CKOBJECT_PRIORITYPLACE;
static const int g_CKOBJECT_PRIORITYDEFAULT = CKOBJECT_PRIORITYDEFAULT;
static const int g_CKOBJECT_PRIORITYMIN = CKOBJECT_PRIORITYMIN;
static const int g_CKBEHAVIOR_PRIORITYMAX = CKBEHAVIOR_PRIORITYMAX;
static const int g_CKBEHAVIOR_PRIORITYMIN = CKBEHAVIOR_PRIORITYMIN;

static CKClassDesc &CKGetClassDescChecked(CK_CLASSID cid);

static std::mutex g_CKBitmapPropertiesOwnedMutex;
static std::unordered_set<CKBitmapProperties *> g_CKBitmapPropertiesOwned;

static std::string CKStruprString(const std::string &value) {
    std::string copy = value;
    if (!copy.empty()) {
        CKStrupr(&copy[0]);
    }
    return copy;
}

static std::string CKStrlwrString(const std::string &value) {
    std::string copy = value;
    if (!copy.empty()) {
        CKStrlwr(&copy[0]);
    }
    return copy;
}

static CKBitmapProperties *CKCopyBitmapPropertiesForScript(CKBitmapProperties *bp) {
    CKBitmapProperties *copy = CKCopyBitmapProperties(bp);
    if (copy) {
        std::lock_guard<std::mutex> lock(g_CKBitmapPropertiesOwnedMutex);
        g_CKBitmapPropertiesOwned.insert(copy);
    }
    return copy;
}

static void CKDeleteBitmapPropertiesForScript(CKBitmapProperties *bp) {
    if (!bp) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_CKBitmapPropertiesOwnedMutex);
        auto it = g_CKBitmapPropertiesOwned.find(bp);
        if (it == g_CKBitmapPropertiesOwned.end()) {
            if (asIScriptContext *ctx = asGetActiveContext()) {
                ctx->SetException("CKDeleteBitmapProperties only accepts handles returned by CKCopyBitmapProperties.");
            }
            return;
        }
        g_CKBitmapPropertiesOwned.erase(it);
    }

    CKDeleteBitmapProperties(bp);
}

static NativePointer GetCKBitmapPropertiesData(const CKBitmapProperties *self) {
    return NativePointer(self ? self->m_Data : nullptr);
}

static void SetCKBitmapPropertiesData(CKBitmapProperties *self, NativePointer ptr) {
    if (ptr.IsNull()) {
        return;
    }

    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException("CKBitmapProperties.m_Data only accepts a null NativePointer from script.");
    }
}

static const int g_CKM_BASE = CKM_BASE;
static const int g_CKM_BEHAVIORPRESAVE = CKM_BEHAVIORPRESAVE;
static const int g_CKM_BEHAVIORDELETE = CKM_BEHAVIORDELETE;
static const int g_CKM_BEHAVIORATTACH = CKM_BEHAVIORATTACH;
static const int g_CKM_BEHAVIORDETACH = CKM_BEHAVIORDETACH;
static const int g_CKM_BEHAVIORPAUSE = CKM_BEHAVIORPAUSE;
static const int g_CKM_BEHAVIORRESUME = CKM_BEHAVIORRESUME;
static const int g_CKM_BEHAVIORCREATE = CKM_BEHAVIORCREATE;
static const int g_CKM_BEHAVIORRESET = CKM_BEHAVIORRESET;
static const int g_CKM_BEHAVIORPOSTSAVE = CKM_BEHAVIORPOSTSAVE;
static const int g_CKM_BEHAVIORLOAD = CKM_BEHAVIORLOAD;
static const int g_CKM_BEHAVIOREDITED = CKM_BEHAVIOREDITED;
static const int g_CKM_BEHAVIORSETTINGSEDITED = CKM_BEHAVIORSETTINGSEDITED;
static const int g_CKM_BEHAVIORREADSTATE = CKM_BEHAVIORREADSTATE;
static const int g_CKM_BEHAVIORNEWSCENE = CKM_BEHAVIORNEWSCENE;
static const int g_CKM_BEHAVIORACTIVATESCRIPT = CKM_BEHAVIORACTIVATESCRIPT;
static const int g_CKM_BEHAVIORDEACTIVATESCRIPT = CKM_BEHAVIORDEACTIVATESCRIPT;
static const int g_CKM_BEHAVIORRESETINBREAKBPOINT = CKM_BEHAVIORRESETINBREAKBPOINT;
static const int g_CKM_MAX_BEHAVIOR_CALLBACKS = CKM_MAX_BEHAVIOR_CALLBACKS;

static const int g_CHUNKDATA_OLDVERSION = CHUNKDATA_OLDVERSION;
static const int g_CHUNKDATA_BASEVERSION = CHUNKDATA_BASEVERSION;
static const int g_CHUNK_WAVESOUND_VERSION2 = CHUNK_WAVESOUND_VERSION2;
static const int g_CHUNK_WAVESOUND_VERSION3 = CHUNK_WAVESOUND_VERSION3;
static const int g_CHUNK_MATERIAL_VERSION_ZTEST = CHUNK_MATERIAL_VERSION_ZTEST;
static const int g_CHUNK_MAJORCHANGE_VERSION = CHUNK_MAJORCHANGE_VERSION;
static const int g_CHUNK_MACCHANGE_VERSION = CHUNK_MACCHANGE_VERSION;
static const int g_CHUNK_WAVESOUND_VERSION4 = CHUNK_WAVESOUND_VERSION4;
static const int g_CHUNK_SCENECHANGE_VERSION = CHUNK_SCENECHANGE_VERSION;
static const int g_CHUNK_MESHCHANGE_VERSION = CHUNK_MESHCHANGE_VERSION;
static const int g_CHUNK_DEV_2_1 = CHUNK_DEV_2_1;
static const int g_CHUNKDATA_CURRENTVERSION = CHUNKDATA_CURRENTVERSION;

static const int g_CKDLL_BEHAVIORPROTOTYPE = CKDLL_BEHAVIORPROTOTYPE;

static const int g_CKCID_OBJECT = CKCID_OBJECT;
static const int g_CKCID_PARAMETERIN = CKCID_PARAMETERIN;
static const int g_CKCID_PARAMETEROPERATION = CKCID_PARAMETEROPERATION;
static const int g_CKCID_STATE = CKCID_STATE;
static const int g_CKCID_BEHAVIORLINK = CKCID_BEHAVIORLINK;
static const int g_CKCID_BEHAVIOR = CKCID_BEHAVIOR;
static const int g_CKCID_BEHAVIORIO = CKCID_BEHAVIORIO;
static const int g_CKCID_RENDERCONTEXT = CKCID_RENDERCONTEXT;
static const int g_CKCID_KINEMATICCHAIN = CKCID_KINEMATICCHAIN;
static const int g_CKCID_SCENEOBJECT = CKCID_SCENEOBJECT;
static const int g_CKCID_OBJECTANIMATION = CKCID_OBJECTANIMATION;
static const int g_CKCID_ANIMATION = CKCID_ANIMATION;
static const int g_CKCID_KEYEDANIMATION = CKCID_KEYEDANIMATION;
static const int g_CKCID_BEOBJECT = CKCID_BEOBJECT;
static const int g_CKCID_DATAARRAY = CKCID_DATAARRAY;
static const int g_CKCID_SCENE = CKCID_SCENE;
static const int g_CKCID_LEVEL = CKCID_LEVEL;
static const int g_CKCID_PLACE = CKCID_PLACE;
static const int g_CKCID_GROUP = CKCID_GROUP;
static const int g_CKCID_SOUND = CKCID_SOUND;
static const int g_CKCID_WAVESOUND = CKCID_WAVESOUND;
static const int g_CKCID_MIDISOUND = CKCID_MIDISOUND;
static const int g_CKCID_MATERIAL = CKCID_MATERIAL;
static const int g_CKCID_TEXTURE = CKCID_TEXTURE;
static const int g_CKCID_MESH = CKCID_MESH;
static const int g_CKCID_PATCHMESH = CKCID_PATCHMESH;
static const int g_CKCID_RENDEROBJECT = CKCID_RENDEROBJECT;
static const int g_CKCID_2DENTITY = CKCID_2DENTITY;
static const int g_CKCID_SPRITE = CKCID_SPRITE;
static const int g_CKCID_SPRITETEXT = CKCID_SPRITETEXT;
static const int g_CKCID_3DENTITY = CKCID_3DENTITY;
static const int g_CKCID_GRID = CKCID_GRID;
static const int g_CKCID_CURVEPOINT = CKCID_CURVEPOINT;
static const int g_CKCID_SPRITE3D = CKCID_SPRITE3D;
static const int g_CKCID_CURVE = CKCID_CURVE;
static const int g_CKCID_CAMERA = CKCID_CAMERA;
static const int g_CKCID_TARGETCAMERA = CKCID_TARGETCAMERA;
static const int g_CKCID_LIGHT = CKCID_LIGHT;
static const int g_CKCID_TARGETLIGHT = CKCID_TARGETLIGHT;
static const int g_CKCID_CHARACTER = CKCID_CHARACTER;
static const int g_CKCID_3DOBJECT = CKCID_3DOBJECT;
static const int g_CKCID_BODYPART = CKCID_BODYPART;
static const int g_CKCID_PARAMETER = CKCID_PARAMETER;
static const int g_CKCID_PARAMETERLOCAL = CKCID_PARAMETERLOCAL;
#if CKVERSION != 0x13022002
static const int g_CKCID_PARAMETERVARIABLE = CKCID_PARAMETERVARIABLE;
#endif
static const int g_CKCID_PARAMETEROUT = CKCID_PARAMETEROUT;
static const int g_CKCID_INTERFACEOBJECTMANAGER = CKCID_INTERFACEOBJECTMANAGER;
static const int g_CKCID_CRITICALSECTION = CKCID_CRITICALSECTION;
static const int g_CKCID_LAYER = CKCID_LAYER;
static const int g_CKCID_MAXCLASSID = CKCID_MAXCLASSID;
static const int g_CKCID_SYNCHRO = CKCID_SYNCHRO;

static const int g_CKCID_OBJECTARRAY = CKCID_OBJECTARRAY;
static const int g_CKCID_SCENEOBJECTDESC = CKCID_SCENEOBJECTDESC;
static const int g_CKCID_ATTRIBUTEMANAGER = CKCID_ATTRIBUTEMANAGER;
static const int g_CKCID_MESSAGEMANAGER = CKCID_MESSAGEMANAGER;
static const int g_CKCID_COLLISIONMANAGER = CKCID_COLLISIONMANAGER;
static const int g_CKCID_OBJECTMANAGER = CKCID_OBJECTMANAGER;
static const int g_CKCID_FLOORMANAGER = CKCID_FLOORMANAGER;
static const int g_CKCID_RENDERMANAGER = CKCID_RENDERMANAGER;
static const int g_CKCID_BEHAVIORMANAGER = CKCID_BEHAVIORMANAGER;
static const int g_CKCID_INPUTMANAGER = CKCID_INPUTMANAGER;
static const int g_CKCID_PARAMETERMANAGER = CKCID_PARAMETERMANAGER;
static const int g_CKCID_GRIDMANAGER = CKCID_GRIDMANAGER;
static const int g_CKCID_SOUNDMANAGER = CKCID_SOUNDMANAGER;
static const int g_CKCID_TIMEMANAGER = CKCID_TIMEMANAGER;
static const int g_CKCID_CUIKBEHDATA = CKCID_CUIKBEHDATA;

static const int g_CK_GENERALOPTIONS_NODUPLICATENAMECHECK = CK_GENERALOPTIONS_NODUPLICATENAMECHECK;
static const int g_CK_GENERALOPTIONS_CANUSECURRENTOBJECT = CK_GENERALOPTIONS_CANUSECURRENTOBJECT;
static const int g_CK_GENERALOPTIONS_AUTOMATICUSECURRENT = CK_GENERALOPTIONS_AUTOMATICUSECURRENT;

static const int g_CKCID_MAXMAXCLASSID = CKCID_MAXMAXCLASSID;

static const int g_CKWM_BASE = CKWM_BASE;
static const int g_CKWM_OK = CKWM_OK;
static const int g_CKWM_CANCEL = CKWM_CANCEL;
static const int g_CKWM_SETVALUE = CKWM_SETVALUE;
static const int g_CKWM_GETVALUE = CKWM_GETVALUE;
static const int g_CKWM_INIT = CKWM_INIT;
static const int g_CKWM_PARAMPICK = CKWM_PARAMPICK;
static const int g_CKWM_PICK = CKWM_PICK;
static const int g_CKWM_SETPARAMTEXT = CKWM_SETPARAMTEXT;
static const int g_CKWM_SIZECHANGED = CKWM_SIZECHANGED;
static const int g_CKWM_PARAMMODIFIED = CKWM_PARAMMODIFIED;
static const int g_CKWM_CREATEBEHAVIORLOCALS = CKWM_CREATEBEHAVIORLOCALS;
static const int g_CKWM_SETMARGIN = CKWM_SETMARGIN;
static const int g_CKWM_GETMARGIN = CKWM_GETMARGIN;
static const int g_CKWM_STARTPICK = CKWM_STARTPICK;
static const int g_CKWM_ENDPICK = CKWM_ENDPICK;

static const CKGUID g_OBJECT_MANAGER_GUID = OBJECT_MANAGER_GUID;
static const CKGUID g_ATTRIBUTE_MANAGER_GUID = ATTRIBUTE_MANAGER_GUID;
static const CKGUID g_MESSAGE_MANAGER_GUID = MESSAGE_MANAGER_GUID;
static const CKGUID g_TIME_MANAGER_GUID = TIME_MANAGER_GUID;
static const CKGUID g_SOUND_MANAGER_GUID = SOUND_MANAGER_GUID;
static const CKGUID g_MIDI_MANAGER_GUID = MIDI_MANAGER_GUID;
static const CKGUID g_INPUT_MANAGER_GUID = INPUT_MANAGER_GUID;
static const CKGUID g_BEHAVIOR_MANAGER_GUID = BEHAVIOR_MANAGER_GUID;
static const CKGUID g_FLOOR_MANAGER_GUID = FLOOR_MANAGER_GUID;
static const CKGUID g_COLLISION_MANAGER_GUID = COLLISION_MANAGER_GUID;
static const CKGUID g_GRID_MANAGER_GUID = GRID_MANAGER_GUID;
static const CKGUID g_INTERFACE_MANAGER_GUID = INTERFACE_MANAGER_GUID;
static const CKGUID g_RENDER_MANAGER_GUID = RENDER_MANAGER_GUID;
static const CKGUID g_PARAMETER_MANAGER_GUID = PARAMETER_MANAGER_GUID;
static const CKGUID g_PATH_MANAGER_GUID = PATH_MANAGER_GUID;

static const CKGUID g_CKPGUID_NONE = CKPGUID_NONE;
static const CKGUID g_CKPGUID_VOIDBUF = CKPGUID_VOIDBUF;
static const CKGUID g_CKPGUID_FLOAT = CKPGUID_FLOAT;
static const CKGUID g_CKPGUID_ANGLE = CKPGUID_ANGLE;
static const CKGUID g_CKPGUID_PERCENTAGE = CKPGUID_PERCENTAGE;
static const CKGUID g_CKPGUID_INT = CKPGUID_INT;
static const CKGUID g_CKPGUID_KEY = CKPGUID_KEY;
static const CKGUID g_CKPGUID_BOOL = CKPGUID_BOOL;
static const CKGUID g_CKPGUID_STRING = CKPGUID_STRING;
static const CKGUID g_CKPGUID_RECT = CKPGUID_RECT;
static const CKGUID g_CKPGUID_VECTOR = CKPGUID_VECTOR;
static const CKGUID g_CKPGUID_2DVECTOR = CKPGUID_2DVECTOR;
static const CKGUID g_CKPGUID_QUATERNION = CKPGUID_QUATERNION;
static const CKGUID g_CKPGUID_EULERANGLES = CKPGUID_EULERANGLES;
static const CKGUID g_CKPGUID_MATRIX = CKPGUID_MATRIX;
static const CKGUID g_CKPGUID_COLOR = CKPGUID_COLOR;
static const CKGUID g_CKPGUID_BOX = CKPGUID_BOX;
static const CKGUID g_CKPGUID_OBJECTARRAY = CKPGUID_OBJECTARRAY;
static const CKGUID g_CKPGUID_OBJECT = CKPGUID_OBJECT;
static const CKGUID g_CKPGUID_BEOBJECT = CKPGUID_BEOBJECT;
static const CKGUID g_CKPGUID_MESH = CKPGUID_MESH;
static const CKGUID g_CKPGUID_MATERIAL = CKPGUID_MATERIAL;
static const CKGUID g_CKPGUID_TEXTURE = CKPGUID_TEXTURE;
static const CKGUID g_CKPGUID_SPRITE = CKPGUID_SPRITE;
static const CKGUID g_CKPGUID_3DENTITY = CKPGUID_3DENTITY;
static const CKGUID g_CKPGUID_CURVEPOINT = CKPGUID_CURVEPOINT;
static const CKGUID g_CKPGUID_LIGHT = CKPGUID_LIGHT;
static const CKGUID g_CKPGUID_TARGETLIGHT = CKPGUID_TARGETLIGHT;
static const CKGUID g_CKPGUID_ID = CKPGUID_ID;
static const CKGUID g_CKPGUID_CAMERA = CKPGUID_CAMERA;
static const CKGUID g_CKPGUID_TARGETCAMERA = CKPGUID_TARGETCAMERA;
static const CKGUID g_CKPGUID_SPRITE3D = CKPGUID_SPRITE3D;
static const CKGUID g_CKPGUID_OBJECT3D = CKPGUID_OBJECT3D;
static const CKGUID g_CKPGUID_BODYPART = CKPGUID_BODYPART;
static const CKGUID g_CKPGUID_CHARACTER = CKPGUID_CHARACTER;
static const CKGUID g_CKPGUID_CURVE = CKPGUID_CURVE;
static const CKGUID g_CKPGUID_2DCURVE = CKPGUID_2DCURVE;
static const CKGUID g_CKPGUID_LEVEL = CKPGUID_LEVEL;
static const CKGUID g_CKPGUID_PLACE = CKPGUID_PLACE;
static const CKGUID g_CKPGUID_GROUP = CKPGUID_GROUP;
static const CKGUID g_CKPGUID_2DENTITY = CKPGUID_2DENTITY;
static const CKGUID g_CKPGUID_RENDEROBJECT = CKPGUID_RENDEROBJECT;
static const CKGUID g_CKPGUID_SPRITETEXT = CKPGUID_SPRITETEXT;
static const CKGUID g_CKPGUID_SOUND = CKPGUID_SOUND;
static const CKGUID g_CKPGUID_WAVESOUND = CKPGUID_WAVESOUND;
static const CKGUID g_CKPGUID_MIDISOUND = CKPGUID_MIDISOUND;
static const CKGUID g_CKPGUID_OBJECTANIMATION = CKPGUID_OBJECTANIMATION;
static const CKGUID g_CKPGUID_ANIMATION = CKPGUID_ANIMATION;
static const CKGUID g_CKPGUID_KINEMATICCHAIN = CKPGUID_KINEMATICCHAIN;
static const CKGUID g_CKPGUID_SCENE = CKPGUID_SCENE;
static const CKGUID g_CKPGUID_BEHAVIOR = CKPGUID_BEHAVIOR;
static const CKGUID g_CKPGUID_MESSAGE = CKPGUID_MESSAGE;
static const CKGUID g_CKPGUID_SYNCHRO = CKPGUID_SYNCHRO;
static const CKGUID g_CKPGUID_CRITICALSECTION = CKPGUID_CRITICALSECTION;
static const CKGUID g_CKPGUID_STATE = CKPGUID_STATE;
static const CKGUID g_CKPGUID_ATTRIBUTE = CKPGUID_ATTRIBUTE;
static const CKGUID g_CKPGUID_CLASSID = CKPGUID_CLASSID;
static const CKGUID g_CKPGUID_DIRECTION = CKPGUID_DIRECTION;
static const CKGUID g_CKPGUID_BLENDMODE = CKPGUID_BLENDMODE;
static const CKGUID g_CKPGUID_FILTERMODE = CKPGUID_FILTERMODE;
static const CKGUID g_CKPGUID_BLENDFACTOR = CKPGUID_BLENDFACTOR;
static const CKGUID g_CKPGUID_FILLMODE = CKPGUID_FILLMODE;
static const CKGUID g_CKPGUID_LITMODE = CKPGUID_LITMODE;
static const CKGUID g_CKPGUID_SHADEMODE = CKPGUID_SHADEMODE;
static const CKGUID g_CKPGUID_GLOBALEXMODE = CKPGUID_GLOBALEXMODE;
static const CKGUID g_CKPGUID_ZFUNC = CKPGUID_ZFUNC;
static const CKGUID g_CKPGUID_ADDRESSMODE = CKPGUID_ADDRESSMODE;
static const CKGUID g_CKPGUID_WRAPMODE = CKPGUID_WRAPMODE;
static const CKGUID g_CKPGUID_3DSPRITEMODE = CKPGUID_3DSPRITEMODE;
static const CKGUID g_CKPGUID_FOGMODE = CKPGUID_FOGMODE;
static const CKGUID g_CKPGUID_LIGHTTYPE = CKPGUID_LIGHTTYPE;
static const CKGUID g_CKPGUID_SPRITEALIGN = CKPGUID_SPRITEALIGN;
static const CKGUID g_CKPGUID_SCRIPT = CKPGUID_SCRIPT;
static const CKGUID g_CKPGUID_LAYERTYPE = CKPGUID_LAYERTYPE;
static const CKGUID g_CKPGUID_STATECHUNK = CKPGUID_STATECHUNK;
static const CKGUID g_CKPGUID_DATAARRAY = CKPGUID_DATAARRAY;
static const CKGUID g_CKPGUID_COMPOPERATOR = CKPGUID_COMPOPERATOR;
static const CKGUID g_CKPGUID_BINARYOPERATOR = CKPGUID_BINARYOPERATOR;
static const CKGUID g_CKPGUID_SETOPERATOR = CKPGUID_SETOPERATOR;
static const CKGUID g_CKPGUID_SPRITETEXTALIGNMENT = CKPGUID_SPRITETEXTALIGNMENT;
static const CKGUID g_CKPGUID_OBSTACLEPRECISION = CKPGUID_OBSTACLEPRECISION;
static const CKGUID g_CKPGUID_OBSTACLEPRECISIONBEH = CKPGUID_OBSTACLEPRECISIONBEH;
static const CKGUID g_CKPGUID_OBSTACLE = CKPGUID_OBSTACLE;
static const CKGUID g_CKPGUID_PATCHMESH = CKPGUID_PATCHMESH;
static const CKGUID g_CKPGUID_POINTER = CKPGUID_POINTER;
static const CKGUID g_CKPGUID_ENUMS = CKPGUID_ENUMS;
static const CKGUID g_CKPGUID_STRUCTS = CKPGUID_STRUCTS;
static const CKGUID g_CKPGUID_FLAGS = CKPGUID_FLAGS;
static const CKGUID g_CKPGUID_FILTER = CKPGUID_FILTER;
static const CKGUID g_CKPGUID_TIME = CKPGUID_TIME;
static const CKGUID g_CKPGUID_OLDTIME = CKPGUID_OLDTIME;
static const CKGUID g_CKPGUID_COPYDEPENDENCIES = CKPGUID_COPYDEPENDENCIES;
static const CKGUID g_CKPGUID_DELETEDEPENDENCIES = CKPGUID_DELETEDEPENDENCIES;
static const CKGUID g_CKPGUID_SAVEDEPENDENCIES = CKPGUID_SAVEDEPENDENCIES;
static const CKGUID g_CKPGUID_REPLACEDEPENDENCIES = CKPGUID_REPLACEDEPENDENCIES;
static const CKGUID g_CKPGUID_SCENEACTIVITYFLAGS = CKPGUID_SCENEACTIVITYFLAGS;
static const CKGUID g_CKPGUID_SCENEOBJECT = CKPGUID_SCENEOBJECT;
static const CKGUID g_CKPGUID_SCENERESETFLAGS = CKPGUID_SCENERESETFLAGS;
static const CKGUID g_CKPGUID_ARRAYTYPE = CKPGUID_ARRAYTYPE;
static const CKGUID g_CKPGUID_RENDEROPTIONS = CKPGUID_RENDEROPTIONS;
static const CKGUID g_CKPGUID_PARAMETERTYPE = CKPGUID_PARAMETERTYPE;
static const CKGUID g_CKPGUID_MATERIALEFFECT = CKPGUID_MATERIALEFFECT;
static const CKGUID g_CKPGUID_TEXGENEFFECT = CKPGUID_TEXGENEFFECT;
static const CKGUID g_CKPGUID_TEXGENREFEFFECT = CKPGUID_TEXGENREFEFFECT;
static const CKGUID g_CKPGUID_COMBINE2TEX = CKPGUID_COMBINE2TEX;
static const CKGUID g_CKPGUID_COMBINE3TEX = CKPGUID_COMBINE3TEX;
static const CKGUID g_CKPGUID_BUMPMAPPARAM = CKPGUID_BUMPMAPPARAM;
static const CKGUID g_CKPGUID_TEXCOMBINE = CKPGUID_TEXCOMBINE;
static const CKGUID g_CKPGUID_PIXELFORMAT = CKPGUID_PIXELFORMAT;
static const CKGUID g_CKPGUID_AXIS = CKPGUID_AXIS;
static const CKGUID g_CKPGUID_SUPPORT = CKPGUID_SUPPORT;
static const CKGUID g_CKPGUID_BITMAP_SYSTEMCACHING = CKPGUID_BITMAP_SYSTEMCACHING;
static const CKGUID g_CKPGUID_OLDMESSAGE = CKPGUID_OLDMESSAGE;
static const CKGUID g_CKPGUID_OLDATTRIBUTE = CKPGUID_OLDATTRIBUTE;

void RegisterCKGlobalVariables(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterGlobalProperty("const int MAX_USER_PROFILE", (void *)&g_MAX_USER_PROFILE); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalProperty("const uint CKVERSION", (void *)&g_CKVERSION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const uint DEVVERSION", (void *)&g_DEVVERSION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID VIRTOOLS_GUID", (void *)&g_VIRTOOLS_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const float CK_ZERO", (void *)&g_CK_ZERO); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKMAX_PATH", (void *)&g_CKMAX_PATH); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKMAX_URL", (void *)&g_CKMAX_URL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKMAX_MANAGERFUNCTIONS", (void *)&g_CKMAX_MANAGERFUNCTIONS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKANIMATION_FORCESETSTEP", (void *)&g_CKANIMATION_FORCESETSTEP); CKAS_CHECK_REGISTER(r);

    // Default priority values
    r = engine->RegisterGlobalProperty("const int CKOBJECT_PRIORITYMAX", (void *)&g_CKOBJECT_PRIORITYMAX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKOBJECT_PRIORITYLEVEL", (void *)&g_CKOBJECT_PRIORITYLEVEL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKOBJECT_PRIORITYSCENE", (void *)&g_CKOBJECT_PRIORITYSCENE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKOBJECT_PRIORITYPLACE", (void *)&g_CKOBJECT_PRIORITYPLACE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKOBJECT_PRIORITYDEFAULT", (void *)&g_CKOBJECT_PRIORITYDEFAULT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKOBJECT_PRIORITYMIN", (void *)&g_CKOBJECT_PRIORITYMIN); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKBEHAVIOR_PRIORITYMAX", (void *)&g_CKBEHAVIOR_PRIORITYMAX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKBEHAVIOR_PRIORITYMIN", (void *)&g_CKBEHAVIOR_PRIORITYMIN); CKAS_CHECK_REGISTER(r);

    // Behavior callback messages
    r = engine->RegisterGlobalProperty("const int CKM_BASE", (void *)&g_CKM_BASE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORPRESAVE", (void *)&g_CKM_BEHAVIORPRESAVE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORDELETE", (void *)&g_CKM_BEHAVIORDELETE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORATTACH", (void *)&g_CKM_BEHAVIORATTACH); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORDETACH", (void *)&g_CKM_BEHAVIORDETACH); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORPAUSE", (void *)&g_CKM_BEHAVIORPAUSE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORRESUME", (void *)&g_CKM_BEHAVIORRESUME); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORCREATE", (void *)&g_CKM_BEHAVIORCREATE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORRESET", (void *)&g_CKM_BEHAVIORRESET); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORPOSTSAVE", (void *)&g_CKM_BEHAVIORPOSTSAVE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORLOAD", (void *)&g_CKM_BEHAVIORLOAD); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIOREDITED", (void *)&g_CKM_BEHAVIOREDITED); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORSETTINGSEDITED", (void *)&g_CKM_BEHAVIORSETTINGSEDITED); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORREADSTATE", (void *)&g_CKM_BEHAVIORREADSTATE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORNEWSCENE", (void *)&g_CKM_BEHAVIORNEWSCENE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORACTIVATESCRIPT", (void *)&g_CKM_BEHAVIORACTIVATESCRIPT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORDEACTIVATESCRIPT", (void *)&g_CKM_BEHAVIORDEACTIVATESCRIPT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_BEHAVIORRESETINBREAKBPOINT", (void *)&g_CKM_BEHAVIORRESETINBREAKBPOINT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKM_MAX_BEHAVIOR_CALLBACKS", (void *)&g_CKM_MAX_BEHAVIOR_CALLBACKS); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalProperty("const int CHUNKDATA_OLDVERSION", (void *)&g_CHUNKDATA_OLDVERSION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNKDATA_BASEVERSION", (void *)&g_CHUNKDATA_BASEVERSION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNK_WAVESOUND_VERSION2", (void *)&g_CHUNK_WAVESOUND_VERSION2); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNK_WAVESOUND_VERSION3", (void *)&g_CHUNK_WAVESOUND_VERSION3); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNK_MATERIAL_VERSION_ZTEST", (void *)&g_CHUNK_MATERIAL_VERSION_ZTEST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNK_MAJORCHANGE_VERSION", (void *)&g_CHUNK_MAJORCHANGE_VERSION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNK_MACCHANGE_VERSION", (void *)&g_CHUNK_MACCHANGE_VERSION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNK_WAVESOUND_VERSION4", (void *)&g_CHUNK_WAVESOUND_VERSION4); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNK_SCENECHANGE_VERSION", (void *)&g_CHUNK_SCENECHANGE_VERSION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNK_MESHCHANGE_VERSION", (void *)&g_CHUNK_MESHCHANGE_VERSION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNK_DEV_2_1", (void *)&g_CHUNK_DEV_2_1); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CHUNKDATA_CURRENTVERSION", (void *)&g_CHUNKDATA_CURRENTVERSION); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalProperty("const int CKDLL_BEHAVIORPROTOTYPE", (void *)&g_CKDLL_BEHAVIORPROTOTYPE); CKAS_CHECK_REGISTER(r);

    // Class Identifier List
    r = engine->RegisterGlobalProperty("const int CKCID_OBJECT", (void *)&g_CKCID_OBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_PARAMETERIN", (void *)&g_CKCID_PARAMETERIN); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_PARAMETEROPERATION", (void *)&g_CKCID_PARAMETEROPERATION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_STATE", (void *)&g_CKCID_STATE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_BEHAVIORLINK", (void *)&g_CKCID_BEHAVIORLINK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_BEHAVIOR", (void *)&g_CKCID_BEHAVIOR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_BEHAVIORIO", (void *)&g_CKCID_BEHAVIORIO); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_RENDERCONTEXT", (void *)&g_CKCID_RENDERCONTEXT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_KINEMATICCHAIN", (void *)&g_CKCID_KINEMATICCHAIN); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_SCENEOBJECT", (void *)&g_CKCID_SCENEOBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_OBJECTANIMATION", (void *)&g_CKCID_OBJECTANIMATION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_ANIMATION", (void *)&g_CKCID_ANIMATION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_KEYEDANIMATION", (void *)&g_CKCID_KEYEDANIMATION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_BEOBJECT", (void *)&g_CKCID_BEOBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_DATAARRAY", (void *)&g_CKCID_DATAARRAY); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_SCENE", (void *)&g_CKCID_SCENE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_LEVEL", (void *)&g_CKCID_LEVEL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_PLACE", (void *)&g_CKCID_PLACE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_GROUP", (void *)&g_CKCID_GROUP); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_SOUND", (void *)&g_CKCID_SOUND); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_WAVESOUND", (void *)&g_CKCID_WAVESOUND); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_MIDISOUND", (void *)&g_CKCID_MIDISOUND); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_MATERIAL", (void *)&g_CKCID_MATERIAL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_TEXTURE", (void *)&g_CKCID_TEXTURE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_MESH", (void *)&g_CKCID_MESH); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_PATCHMESH", (void *)&g_CKCID_PATCHMESH); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_RENDEROBJECT", (void *)&g_CKCID_RENDEROBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_2DENTITY", (void *)&g_CKCID_2DENTITY); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_SPRITE", (void *)&g_CKCID_SPRITE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_SPRITETEXT", (void *)&g_CKCID_SPRITETEXT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_3DENTITY", (void *)&g_CKCID_3DENTITY); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_GRID", (void *)&g_CKCID_GRID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_CURVEPOINT", (void *)&g_CKCID_CURVEPOINT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_SPRITE3D", (void *)&g_CKCID_SPRITE3D); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_CURVE", (void *)&g_CKCID_CURVE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_CAMERA", (void *)&g_CKCID_CAMERA); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_TARGETCAMERA", (void *)&g_CKCID_TARGETCAMERA); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_LIGHT", (void *)&g_CKCID_LIGHT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_TARGETLIGHT", (void *)&g_CKCID_TARGETLIGHT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_CHARACTER", (void *)&g_CKCID_CHARACTER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_3DOBJECT", (void *)&g_CKCID_3DOBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_BODYPART", (void *)&g_CKCID_BODYPART); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_PARAMETER", (void *)&g_CKCID_PARAMETER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_PARAMETERLOCAL", (void *)&g_CKCID_PARAMETERLOCAL); CKAS_CHECK_REGISTER(r);
#if CKVERSION != 0x13022002
    r = engine->RegisterGlobalProperty("const int CKCID_PARAMETERVARIABLE", (void *)&g_CKCID_PARAMETERVARIABLE); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterGlobalProperty("const int CKCID_PARAMETEROUT", (void *)&g_CKCID_PARAMETEROUT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_INTERFACEOBJECTMANAGER", (void *)&g_CKCID_INTERFACEOBJECTMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_CRITICALSECTION", (void *)&g_CKCID_CRITICALSECTION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_LAYER", (void *)&g_CKCID_LAYER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_MAXCLASSID", (void *)&g_CKCID_MAXCLASSID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_SYNCHRO", (void *)&g_CKCID_SYNCHRO); CKAS_CHECK_REGISTER(r);

    // Not CKObject derived classes
    r = engine->RegisterGlobalProperty("const int CKCID_OBJECTARRAY", (void *)&g_CKCID_OBJECTARRAY); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_SCENEOBJECTDESC", (void *)&g_CKCID_SCENEOBJECTDESC); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_ATTRIBUTEMANAGER", (void *)&g_CKCID_ATTRIBUTEMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_MESSAGEMANAGER", (void *)&g_CKCID_MESSAGEMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_COLLISIONMANAGER", (void *)&g_CKCID_COLLISIONMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_OBJECTMANAGER", (void *)&g_CKCID_OBJECTMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_FLOORMANAGER", (void *)&g_CKCID_FLOORMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_RENDERMANAGER", (void *)&g_CKCID_RENDERMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_BEHAVIORMANAGER", (void *)&g_CKCID_BEHAVIORMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_INPUTMANAGER", (void *)&g_CKCID_INPUTMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_PARAMETERMANAGER", (void *)&g_CKCID_PARAMETERMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_GRIDMANAGER", (void *)&g_CKCID_GRIDMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_SOUNDMANAGER", (void *)&g_CKCID_SOUNDMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_TIMEMANAGER", (void *)&g_CKCID_TIMEMANAGER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKCID_CUIKBEHDATA", (void *)&g_CKCID_CUIKBEHDATA); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalProperty("const int CK_GENERALOPTIONS_NODUPLICATENAMECHECK", (void *)&g_CK_GENERALOPTIONS_NODUPLICATENAMECHECK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CK_GENERALOPTIONS_CANUSECURRENTOBJECT", (void *)&g_CK_GENERALOPTIONS_CANUSECURRENTOBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CK_GENERALOPTIONS_AUTOMATICUSECURRENT", (void *)&g_CK_GENERALOPTIONS_AUTOMATICUSECURRENT); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalProperty("const int CKCID_MAXMAXCLASSID", (void *)&g_CKCID_MAXMAXCLASSID); CKAS_CHECK_REGISTER(r);

    // Windows messages
    r = engine->RegisterGlobalProperty("const int CKWM_BASE", (void *)&g_CKWM_BASE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_OK", (void *)&g_CKWM_OK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_CANCEL", (void *)&g_CKWM_CANCEL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_SETVALUE", (void *)&g_CKWM_SETVALUE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_GETVALUE", (void *)&g_CKWM_GETVALUE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_INIT", (void *)&g_CKWM_INIT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_PARAMPICK", (void *)&g_CKWM_PARAMPICK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_PICK", (void *)&g_CKWM_PICK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_SETPARAMTEXT", (void *)&g_CKWM_SETPARAMTEXT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_SIZECHANGED", (void *)&g_CKWM_SIZECHANGED); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_PARAMMODIFIED", (void *)&g_CKWM_PARAMMODIFIED); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_CREATEBEHAVIORLOCALS", (void *)&g_CKWM_CREATEBEHAVIORLOCALS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_SETMARGIN", (void *)&g_CKWM_SETMARGIN); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_GETMARGIN", (void *)&g_CKWM_GETMARGIN); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_STARTPICK", (void *)&g_CKWM_STARTPICK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const int CKWM_ENDPICK", (void *)&g_CKWM_ENDPICK); CKAS_CHECK_REGISTER(r);

    // Preregistered Managers
    r = engine->RegisterGlobalProperty("const CKGUID OBJECT_MANAGER_GUID", (void *)&g_OBJECT_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID ATTRIBUTE_MANAGER_GUID", (void *)&g_ATTRIBUTE_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID MESSAGE_MANAGER_GUID", (void *)&g_MESSAGE_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID TIME_MANAGER_GUID", (void *)&g_TIME_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID SOUND_MANAGER_GUID", (void *)&g_SOUND_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID MIDI_MANAGER_GUID", (void *)&g_MIDI_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID INPUT_MANAGER_GUID", (void *)&g_INPUT_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID BEHAVIOR_MANAGER_GUID", (void *)&g_BEHAVIOR_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID FLOOR_MANAGER_GUID", (void *)&g_FLOOR_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID COLLISION_MANAGER_GUID", (void *)&g_COLLISION_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID GRID_MANAGER_GUID", (void *)&g_GRID_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID INTERFACE_MANAGER_GUID", (void *)&g_INTERFACE_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID RENDER_MANAGER_GUID", (void *)&g_RENDER_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID PARAMETER_MANAGER_GUID", (void *)&g_PARAMETER_MANAGER_GUID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID PATH_MANAGER_GUID", (void *)&g_PATH_MANAGER_GUID); CKAS_CHECK_REGISTER(r);

    // Preregistered parameter types
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_NONE", (void *)&g_CKPGUID_NONE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_VOIDBUF", (void *)&g_CKPGUID_VOIDBUF); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_FLOAT", (void *)&g_CKPGUID_FLOAT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_ANGLE", (void *)&g_CKPGUID_ANGLE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_PERCENTAGE", (void *)&g_CKPGUID_PERCENTAGE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_INT", (void *)&g_CKPGUID_INT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_KEY", (void *)&g_CKPGUID_KEY); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BOOL", (void *)&g_CKPGUID_BOOL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_STRING", (void *)&g_CKPGUID_STRING); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_RECT", (void *)&g_CKPGUID_RECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_VECTOR", (void *)&g_CKPGUID_VECTOR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_2DVECTOR", (void *)&g_CKPGUID_2DVECTOR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_QUATERNION", (void *)&g_CKPGUID_QUATERNION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_EULERANGLES", (void *)&g_CKPGUID_EULERANGLES); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_MATRIX", (void *)&g_CKPGUID_MATRIX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_COLOR", (void *)&g_CKPGUID_COLOR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BOX", (void *)&g_CKPGUID_BOX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OBJECTARRAY", (void *)&g_CKPGUID_OBJECTARRAY); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OBJECT", (void *)&g_CKPGUID_OBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BEOBJECT", (void *)&g_CKPGUID_BEOBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_MESH", (void *)&g_CKPGUID_MESH); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_MATERIAL", (void *)&g_CKPGUID_MATERIAL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_TEXTURE", (void *)&g_CKPGUID_TEXTURE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SPRITE", (void *)&g_CKPGUID_SPRITE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_3DENTITY", (void *)&g_CKPGUID_3DENTITY); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_CURVEPOINT", (void *)&g_CKPGUID_CURVEPOINT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_LIGHT", (void *)&g_CKPGUID_LIGHT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_TARGETLIGHT", (void *)&g_CKPGUID_TARGETLIGHT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_ID", (void *)&g_CKPGUID_ID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_CAMERA", (void *)&g_CKPGUID_CAMERA); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_TARGETCAMERA", (void *)&g_CKPGUID_TARGETCAMERA); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SPRITE3D", (void *)&g_CKPGUID_SPRITE3D); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OBJECT3D", (void *)&g_CKPGUID_OBJECT3D); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BODYPART", (void *)&g_CKPGUID_BODYPART); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_CHARACTER", (void *)&g_CKPGUID_CHARACTER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_CURVE", (void *)&g_CKPGUID_CURVE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_2DCURVE", (void *)&g_CKPGUID_2DCURVE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_LEVEL", (void *)&g_CKPGUID_LEVEL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_PLACE", (void *)&g_CKPGUID_PLACE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_GROUP", (void *)&g_CKPGUID_GROUP); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_2DENTITY", (void *)&g_CKPGUID_2DENTITY); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_RENDEROBJECT", (void *)&g_CKPGUID_RENDEROBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SPRITETEXT", (void *)&g_CKPGUID_SPRITETEXT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SOUND", (void *)&g_CKPGUID_SOUND); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_WAVESOUND", (void *)&g_CKPGUID_WAVESOUND); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_MIDISOUND", (void *)&g_CKPGUID_MIDISOUND); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OBJECTANIMATION", (void *)&g_CKPGUID_OBJECTANIMATION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_ANIMATION", (void *)&g_CKPGUID_ANIMATION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_KINEMATICCHAIN", (void *)&g_CKPGUID_KINEMATICCHAIN); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SCENE", (void *)&g_CKPGUID_SCENE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BEHAVIOR", (void *)&g_CKPGUID_BEHAVIOR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_MESSAGE", (void *)&g_CKPGUID_MESSAGE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SYNCHRO", (void *)&g_CKPGUID_SYNCHRO); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_CRITICALSECTION", (void *)&g_CKPGUID_CRITICALSECTION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_STATE", (void *)&g_CKPGUID_STATE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_ATTRIBUTE", (void *)&g_CKPGUID_ATTRIBUTE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_CLASSID", (void *)&g_CKPGUID_CLASSID); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_DIRECTION", (void *)&g_CKPGUID_DIRECTION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BLENDMODE", (void *)&g_CKPGUID_BLENDMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_FILTERMODE", (void *)&g_CKPGUID_FILTERMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BLENDFACTOR", (void *)&g_CKPGUID_BLENDFACTOR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_FILLMODE", (void *)&g_CKPGUID_FILLMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_LITMODE", (void *)&g_CKPGUID_LITMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SHADEMODE", (void *)&g_CKPGUID_SHADEMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_GLOBALEXMODE", (void *)&g_CKPGUID_GLOBALEXMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_ZFUNC", (void *)&g_CKPGUID_ZFUNC); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_ADDRESSMODE", (void *)&g_CKPGUID_ADDRESSMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_WRAPMODE", (void *)&g_CKPGUID_WRAPMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_3DSPRITEMODE", (void *)&g_CKPGUID_3DSPRITEMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_FOGMODE", (void *)&g_CKPGUID_FOGMODE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_LIGHTTYPE", (void *)&g_CKPGUID_LIGHTTYPE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SPRITEALIGN", (void *)&g_CKPGUID_SPRITEALIGN); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SCRIPT", (void *)&g_CKPGUID_SCRIPT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_LAYERTYPE", (void *)&g_CKPGUID_LAYERTYPE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_STATECHUNK", (void *)&g_CKPGUID_STATECHUNK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_DATAARRAY", (void *)&g_CKPGUID_DATAARRAY); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_COMPOPERATOR", (void *)&g_CKPGUID_COMPOPERATOR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BINARYOPERATOR", (void *)&g_CKPGUID_BINARYOPERATOR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SETOPERATOR", (void *)&g_CKPGUID_SETOPERATOR); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SPRITETEXTALIGNMENT", (void *)&g_CKPGUID_SPRITETEXTALIGNMENT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OBSTACLEPRECISION", (void *)&g_CKPGUID_OBSTACLEPRECISION); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OBSTACLEPRECISIONBEH", (void *)&g_CKPGUID_OBSTACLEPRECISIONBEH); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OBSTACLE", (void *)&g_CKPGUID_OBSTACLE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_PATCHMESH", (void *)&g_CKPGUID_PATCHMESH); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_POINTER", (void *)&g_CKPGUID_POINTER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_ENUMS", (void *)&g_CKPGUID_ENUMS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_STRUCTS", (void *)&g_CKPGUID_STRUCTS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_FLAGS", (void *)&g_CKPGUID_FLAGS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_FILTER", (void *)&g_CKPGUID_FILTER); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_TIME", (void *)&g_CKPGUID_TIME); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OLDTIME", (void *)&g_CKPGUID_OLDTIME); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_COPYDEPENDENCIES", (void *)&g_CKPGUID_COPYDEPENDENCIES); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_DELETEDEPENDENCIES", (void *)&g_CKPGUID_DELETEDEPENDENCIES); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SAVEDEPENDENCIES", (void *)&g_CKPGUID_SAVEDEPENDENCIES); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_REPLACEDEPENDENCIES", (void *)&g_CKPGUID_REPLACEDEPENDENCIES); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SCENEACTIVITYFLAGS", (void *)&g_CKPGUID_SCENEACTIVITYFLAGS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SCENEOBJECT", (void *)&g_CKPGUID_SCENEOBJECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SCENERESETFLAGS", (void *)&g_CKPGUID_SCENERESETFLAGS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_ARRAYTYPE", (void *)&g_CKPGUID_ARRAYTYPE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_RENDEROPTIONS", (void *)&g_CKPGUID_RENDEROPTIONS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_PARAMETERTYPE", (void *)&g_CKPGUID_PARAMETERTYPE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_MATERIALEFFECT", (void *)&g_CKPGUID_MATERIALEFFECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_TEXGENEFFECT", (void *)&g_CKPGUID_TEXGENEFFECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_TEXGENREFEFFECT", (void *)&g_CKPGUID_TEXGENREFEFFECT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_COMBINE2TEX", (void *)&g_CKPGUID_COMBINE2TEX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_COMBINE3TEX", (void *)&g_CKPGUID_COMBINE3TEX); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BUMPMAPPARAM", (void *)&g_CKPGUID_BUMPMAPPARAM); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_TEXCOMBINE", (void *)&g_CKPGUID_TEXCOMBINE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_PIXELFORMAT", (void *)&g_CKPGUID_PIXELFORMAT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_AXIS", (void *)&g_CKPGUID_AXIS); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_SUPPORT", (void *)&g_CKPGUID_SUPPORT); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_BITMAP_SYSTEMCACHING", (void *)&g_CKPGUID_BITMAP_SYSTEMCACHING); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OLDMESSAGE", (void *)&g_CKPGUID_OLDMESSAGE); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalProperty("const CKGUID CKPGUID_OLDATTRIBUTE", (void *)&g_CKPGUID_OLDATTRIBUTE); CKAS_CHECK_REGISTER(r);
}

static CKDependencies CopyDefaultClassDependenciesValue(CK_DEPENDENCIES_OPMODE mode) {
    if (CKDependencies *dependencies = CKGetDefaultClassDependencies(mode)) {
        return *dependencies;
    }
    return CKDependencies();
}

void RegisterCKGlobalFunctions(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterGlobalFunction("string CKErrorToString(CKERROR err)", asFUNCTIONPR([](CKERROR err) -> std::string { return ScriptStringify(CKErrorToString(err)); }, (CKERROR), std::string), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKERROR CKStartUp()", asFUNCTION(CKStartUp), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKERROR CKShutdown()", asFUNCTION(CKShutdown), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKContext@ GetCKContext(int pos)", asFUNCTION(GetCKContext), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKObject@ CKGetObject(CKContext@ context, CK_ID id)", asFUNCTION(CKGetObject), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKERROR CKCreateContext(CKContext@ &out context, WIN_HANDLE win, int renderEngine, CKDWORD flags)", asFUNCTION(CKCreateContext), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKERROR CKCloseContext(CKContext@ context)", asFUNCTION(CKCloseContext), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("string CKGetStartPath()", asFUNCTIONPR([]() -> std::string { return ScriptStringify(CKGetStartPath()); }, (), std::string), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("string CKGetPluginsPath()", asFUNCTIONPR([]() -> std::string { return ScriptStringify(CKGetPluginsPath()); }, (), std::string), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("void CKDestroyObject(CKObject@ obj, CKDWORD flags = 0, CKDependencies &in dep = void)", asFUNCTION(CKDestroyObject), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKDWORD CKGetVersion()", asFUNCTION(CKGetVersion), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("void CKBuildClassHierarchyTable()", asFUNCTION(CKBuildClassHierarchyTable), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKPluginManager@ CKGetPluginManager()", asFUNCTION(CKGetPluginManager), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("int CKGetPrototypeDeclarationCount()", asFUNCTION(CKGetPrototypeDeclarationCount), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKObjectDeclaration@ CKGetPrototypeDeclaration(int n)", asFUNCTION(CKGetPrototypeDeclaration), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("XObjDeclHashTableIt CKGetPrototypeDeclarationStartIterator()", asFUNCTION(CKGetPrototypeDeclarationStartIterator), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("XObjDeclHashTableIt CKGetPrototypeDeclarationEndIterator()", asFUNCTION(CKGetPrototypeDeclarationEndIterator), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKObjectDeclaration@ CKGetObjectDeclarationFromGuid(CKGUID guid)", asFUNCTION(CKGetObjectDeclarationFromGuid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKBehaviorPrototype@ CKGetPrototypeFromGuid(CKGUID guid)", asFUNCTION(CKGetPrototypeFromGuid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKERROR CKRemovePrototypeDeclaration(CKObjectDeclaration@ decl)", asFUNCTION(CKRemovePrototypeDeclaration), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKObjectDeclaration@ CreateCKObjectDeclaration(const string &in name)", asFUNCTIONPR([](const std::string &name) { return CreateCKObjectDeclaration(const_cast<CKSTRING>(name.c_str())); }, (const std::string &), CKObjectDeclaration *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKBehaviorPrototype@ CreateCKBehaviorPrototype(const string &in name)", asFUNCTIONPR([](const std::string &name) { return CreateCKBehaviorPrototype(const_cast<CKSTRING>(name.c_str())); }, (const std::string &), CKBehaviorPrototype *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKBehaviorPrototype@ CreateCKBehaviorPrototypeRunTime(const string &in name)", asFUNCTIONPR([](const std::string &name) { return CreateCKBehaviorPrototypeRunTime(const_cast<CKSTRING>(name.c_str())); }, (const std::string &), CKBehaviorPrototype *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("int CKGetClassCount()", asFUNCTION(CKGetClassCount), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKClassDesc &CKGetClassDesc(CK_CLASSID cid)", asFUNCTION(CKGetClassDescChecked), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("string CKClassIDToString(CK_CLASSID cid)", asFUNCTIONPR([](CK_CLASSID cid) -> std::string { return ScriptStringify(CKClassIDToString(cid)); }, (CK_CLASSID), std::string), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CK_CLASSID CKStringToClassID(const string &in className)", asFUNCTIONPR([](const std::string &className) { return CKStringToClassID(const_cast<CKSTRING>(className.c_str())); }, (const std::string &), CK_CLASSID), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("bool CKIsChildClassOf(CK_CLASSID child, CK_CLASSID parent)", asFUNCTIONPR([](CK_CLASSID child, CK_CLASSID parent) -> bool { return CKIsChildClassOf(child, parent); }, (CK_CLASSID, CK_CLASSID), bool), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool CKIsChildClassOf(CKObject@ obj, CK_CLASSID parent)", asFUNCTIONPR([](CKObject *obj, CK_CLASSID parent) -> bool { return CKIsChildClassOf(obj, parent); }, (CKObject *, CK_CLASSID), bool), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CK_CLASSID CKGetParentClassID(CK_CLASSID child)", asFUNCTIONPR(CKGetParentClassID, (CK_CLASSID), CK_CLASSID), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CK_CLASSID CKGetParentClassID(CKObject@ obj)", asFUNCTIONPR(CKGetParentClassID, (CKObject *), CK_CLASSID), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CK_CLASSID CKGetCommonParent(CK_CLASSID cid1, CK_CLASSID cid2)", asFUNCTION(CKGetCommonParent), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKObjectArray@ CreateCKObjectArray()", asFUNCTION(CreateCKObjectArray), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void DeleteCKObjectArray(CKObjectArray@ obj)", asFUNCTION(DeleteCKObjectArray), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKStateChunk@ CreateCKStateChunk(CK_CLASSID id, CKFile@ file = null)", asFUNCTIONPR(CreateCKStateChunk, (CK_CLASSID, CKFile *), CKStateChunk *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKStateChunk@ CreateCKStateChunk(CKStateChunk@ chunk)", asFUNCTIONPR(CreateCKStateChunk, (CKStateChunk *), CKStateChunk *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void DeleteCKStateChunk(CKStateChunk@ chunk)", asFUNCTIONPR(DeleteCKStateChunk, (CKStateChunk *), void), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKStateChunk@ CKSaveObjectState(CKObject@ obj, CKDWORD flags = CK_STATESAVE_ALL)", asFUNCTIONPR(CKSaveObjectState, (CKObject *, CKDWORD), CKStateChunk *), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKERROR CKReadObjectState(CKObject@ obj, CKStateChunk@ chunk)", asFUNCTIONPR(CKReadObjectState, (CKObject *, CKStateChunk *), CKERROR), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("BITMAP_HANDLE CKLoadBitmap(const string &in filename)", asFUNCTIONPR([](const std::string &filename) { return CKLoadBitmap(const_cast<CKSTRING>(filename.c_str())); }, (const std::string &), BITMAP_HANDLE), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool CKSaveBitmap(const string &in filename, BITMAP_HANDLE bm)", asFUNCTIONPR([](const std::string &filename, BITMAP_HANDLE bm) -> bool { return CKSaveBitmap(const_cast<CKSTRING>(filename.c_str()), bm); }, (const std::string &, BITMAP_HANDLE), bool), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool CKSaveBitmap(const string &in filename, VxImageDescEx &in desc)", asFUNCTIONPR([](const std::string &filename, VxImageDescEx &desc) -> bool { return CKSaveBitmap(const_cast<CKSTRING>(filename.c_str()), desc); }, (const std::string &, VxImageDescEx &), bool), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("void CKConvertEndianArray32(NativePointer buf, int dwordCount)", asFUNCTIONPR([](NativePointer buf, int dwordCount) { CKConvertEndianArray32(buf.Get(), dwordCount); }, (NativePointer, int), void), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void CKConvertEndianArray16(NativePointer buf, int dwordCount)", asFUNCTIONPR([](NativePointer buf, int dwordCount) { CKConvertEndianArray16(buf.Get(), dwordCount); }, (NativePointer, int), void), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKDWORD CKConvertEndian32(CKDWORD dw)", asFUNCTION(CKConvertEndian32), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKWORD CKConvertEndian16(CKWORD w)", asFUNCTION(CKConvertEndian16), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKDWORD CKComputeDataCRC(NativePointer data, int size, CKDWORD previousCRC = 0)", asFUNCTIONPR([](NativePointer data, int size, CKDWORD previousCRC) { return CKComputeDataCRC(data.Get(), size, previousCRC); }, (NativePointer, int, CKDWORD), CKDWORD), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("NativePointer CKPackData(NativePointer data, int size, int &out newSize, int compressionLevel)", asFUNCTIONPR([](NativePointer data, int size, int &newSize, int compressionlevel) { return NativePointer(CKPackData(data.Get(), size, newSize, compressionlevel)); }, (NativePointer, int, int&, int), NativePointer), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("NativePointer CKUnPackData(int destSize, NativePointer srcBuffer, int srcSize)", asFUNCTIONPR([](int destSize, NativePointer srcBuffer, int srcSize) { return NativePointer(CKUnPackData(destSize, srcBuffer.Get(), srcSize)); }, (int, NativePointer, int), NativePointer), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("NativePointer CKStrdup(NativePointer str)", asFUNCTIONPR([](NativePointer str) { return NativePointer(CKStrdup(str.Get())); }, (NativePointer), NativePointer), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("NativePointer CKStrdup(const string &in str)", asFUNCTIONPR([](const std::string &str) { return NativePointer(CKStrdup(const_cast<CKSTRING>(str.c_str()))); }, (const std::string &), NativePointer), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("NativePointer CKStrupr(NativePointer str)", asFUNCTIONPR([](NativePointer str) { return NativePointer(CKStrupr(str.Get())); }, (NativePointer), NativePointer), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("string CKStrupr(const string &in str)", asFUNCTION(CKStruprString), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("NativePointer CKStrlwr(NativePointer str)", asFUNCTIONPR([](NativePointer str) { return NativePointer(CKStrlwr(str.Get())); }, (NativePointer), NativePointer), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("string CKStrlwr(const string &in str)", asFUNCTION(CKStrlwrString), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKBitmapProperties@ CKCopyBitmapProperties(CKBitmapProperties@ bp)", asFUNCTION(CKCopyBitmapPropertiesForScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void CKDeleteBitmapProperties(CKBitmapProperties@ bp)", asFUNCTION(CKDeleteBitmapPropertiesForScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("void CKCopyDefaultClassDependencies(CKDependencies &out d, CK_DEPENDENCIES_OPMODE mode)", asFUNCTION(CKCopyDefaultClassDependencies), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKDependencies CKGetDefaultClassDependencies(CK_DEPENDENCIES_OPMODE mode)", asFUNCTION(CopyDefaultClassDependenciesValue), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("void CKDeletePointer(NativePointer ptr)", asFUNCTIONPR([](NativePointer ptr) { CKDeletePointer(ptr.Get()); }, (NativePointer), void), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKERROR CKCopyAllAttributes(CKBeObject@ src, CKBeObject@ dest)", asFUNCTION(CKCopyAllAttributes), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKERROR CKMoveAllScripts(CKBeObject@ src, CKBeObject@ dest)", asFUNCTION(CKMoveAllScripts), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKERROR CKMoveScript(CKBeObject@ src, CKBeObject@ dest, CKBehavior@ beh)", asFUNCTION(CKMoveScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("void CKRemapObjectParameterValue(CKContext@ context, CK_ID oldID, CK_ID newID, CK_CLASSID cid = CKCID_OBJECT, bool derived = true)", asFUNCTIONPR([](CKContext *context, CK_ID oldID, CK_ID newID, CK_CLASSID cid, bool derived) { CKRemapObjectParameterValue(context, oldID, newID, cid, derived); }, (CKContext *, CK_ID, CK_ID, CK_CLASSID, bool), void), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("void CKStoreDeclaration(XObjectDeclarationArray &out reg, CKObjectDeclaration@ a)", asFUNCTION(CKStoreDeclaration), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("CKDWORD GetCurrentFileLoadOption()", asFUNCTION(GetCurrentFileLoadOption), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("CKDWORD GetCurrentFileVersion()", asFUNCTION(GetCurrentFileVersion), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    // r = engine->RegisterGlobalFunction("void CKClassNeedNotificationFrom(CK_CLASSID cid1, CK_CLASSID cid2)", asFUNCTION(CKClassNeedNotificationFrom), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterGlobalFunction("void CKClassRegisterAssociatedParameter(CK_CLASSID cid, CKGUID guid)", asFUNCTION(CKClassRegisterAssociatedParameter), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterGlobalFunction("void CKClassRegisterDefaultDependencies(CK_CLASSID cid, CKDWORD dependMask, int mode)", asFUNCTION(CKClassRegisterDefaultDependencies), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterGlobalFunction("void CKClassRegisterDefaultOptions(CK_CLASSID cid, CKDWORD optionsMask)", asFUNCTION(CKClassRegisterDefaultOptions), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterGlobalFunction("CK_CLASSID CKClassGetNewIdentifier()", asFUNCTION(CKClassGetNewIdentifier), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterGlobalFunction("void CKClassRegister(CK_CLASSID cid, CK_CLASSID parentCid, CKCLASSREGISTERFCT, CKCLASSCREATIONFCT, CKCLASSNAMEFCT, CKCLASSDEPENDENCIESFCT, CKCLASSDEPENDENCIESCOUNTFCT)", asFUNCTION(CKClassRegister), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
}

// CKFileInfo

void RegisterCKFileInfo(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD ProductVersion", offsetof(CKFileInfo, ProductVersion)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD ProductBuild", offsetof(CKFileInfo, ProductBuild)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD FileWriteMode", offsetof(CKFileInfo, FileWriteMode)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD FileVersion", offsetof(CKFileInfo, FileVersion)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD CKVersion", offsetof(CKFileInfo, CKVersion)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD FileSize", offsetof(CKFileInfo, FileSize)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD ObjectCount", offsetof(CKFileInfo, ObjectCount)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD ManagerCount", offsetof(CKFileInfo, ManagerCount)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD MaxIDSaved", offsetof(CKFileInfo, MaxIDSaved)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD Crc", offsetof(CKFileInfo, Crc)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD Hdr1PackSize", offsetof(CKFileInfo, Hdr1PackSize)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD Hdr1UnPackSize", offsetof(CKFileInfo, Hdr1UnPackSize)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD DataPackSize", offsetof(CKFileInfo, DataPackSize)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileInfo", "CKDWORD DataUnPackSize", offsetof(CKFileInfo, DataUnPackSize)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFileInfo", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKFileInfo *self) { new(self) CKFileInfo(); }, (CKFileInfo*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKFileInfo", asBEHAVE_CONSTRUCT, "void f(const CKFileInfo &in other)", asFUNCTIONPR([](const CKFileInfo &info, CKFileInfo *self) { new(self) CKFileInfo(info); }, (const CKFileInfo &, CKFileInfo *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFileInfo", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKFileInfo *self) { self->~CKFileInfo(); }, (CKFileInfo *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFileInfo", "CKFileInfo &opAssign(const CKFileInfo &in other)", asMETHODPR(CKFileInfo, operator=, (const CKFileInfo &), CKFileInfo &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKStats

static float CKStatsGetUserProfile(const CKStats &stats, int index) {
    if (index < 0 || index >= MAX_USER_PROFILE) {
        // Set a script exception
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Out of range");
        return 0.0f;
    }
    return stats.UserProfiles[index];
}

static void CKStatsSetUserProfile(CKStats &stats, int index, float value) {
    if (index < 0 || index >= MAX_USER_PROFILE) {
        // Set a script exception
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Out of range");
        return;
    }
    stats.UserProfiles[index] = value;
}

void RegisterCKStats(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKStats", "float TotalFrameTime", offsetof(CKStats, TotalFrameTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "float EstimatedInterfaceTime", offsetof(CKStats, EstimatedInterfaceTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "float ProcessTime", offsetof(CKStats, ProcessTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "float RenderTime", offsetof(CKStats, RenderTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "float ParametricOperations", offsetof(CKStats, ParametricOperations)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "float TotalBehaviorExecution", offsetof(CKStats, TotalBehaviorExecution)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "float AnimationManagement", offsetof(CKStats, AnimationManagement)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "float IKManagement", offsetof(CKStats, IKManagement)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "float BehaviorCodeExecution", offsetof(CKStats, BehaviorCodeExecution)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "int ActiveObjectsExecuted", offsetof(CKStats, ActiveObjectsExecuted)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "int BehaviorsExecuted", offsetof(CKStats, BehaviorsExecuted)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "int BuildingBlockExecuted", offsetof(CKStats, BuildingBlockExecuted)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "int BehaviorLinksParsed", offsetof(CKStats, BehaviorLinksParsed)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKStats", "int BehaviorDelayedLinks", offsetof(CKStats, BehaviorDelayedLinks)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKStats", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKStats *self) { new(self) CKStats(); }, (CKStats*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKStats", asBEHAVE_CONSTRUCT, "void f(const CKStats &in other)", asFUNCTIONPR([](const CKStats &stats, CKStats *self) { new(self) CKStats(stats); }, (const CKStats &, CKStats *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKStats", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKStats *self) { self->~CKStats(); }, (CKStats *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStats", "CKStats &opAssign(const CKStats &in other)", asMETHODPR(CKStats, operator=, (const CKStats &), CKStats &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStats", "float GetUserProfile(int index) const", asFUNCTION(CKStatsGetUserProfile), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStats", "void SetUserProfile(int index, float value)", asFUNCTION(CKStatsSetUserProfile), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

}

// VxDriverDesc

static std::string VxDriverDescGetDriverDesc(const VxDriverDesc &desc) {
#if CKVERSION == 0x13022002
    return desc.DriverDesc;
#else
    return desc.DriverDesc.CStr();
#endif
}

static void VxDriverDescSetDriverDesc(VxDriverDesc &desc, const std::string &value) {
#if CKVERSION == 0x13022002
    strncpy(desc.DriverDesc, value.c_str(), sizeof(desc.DriverDesc) - 1);
    desc.DriverDesc[sizeof(desc.DriverDesc) - 1] = '\0';
#else
    desc.DriverDesc = value.c_str();
#endif
}

static std::string VxDriverDescGetDriverName(const VxDriverDesc &desc) {
#if CKVERSION == 0x13022002
    return desc.DriverName;
#else
    return desc.DriverName.CStr();
#endif
}

static void VxDriverDescSetDriverName(VxDriverDesc &desc, const std::string &value) {
#if CKVERSION == 0x13022002
    strncpy(desc.DriverName, value.c_str(), sizeof(desc.DriverName) - 1);
    desc.DriverName[sizeof(desc.DriverName) - 1] = '\0';
#else
    desc.DriverName = value.c_str();
#endif
}

static VxDisplayMode *VxDriverDescGetDisplayMode(VxDriverDesc &desc, int index) {
#if CKVERSION == 0x13022002
    if (index < 0 || index >= desc.DisplayModeCount) {
#else
    if (index < 0 || index >= desc.DisplayModes.Size()) {
#endif
        // Set a script exception
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Out of range");
        return nullptr;
    }
    return &desc.DisplayModes[index];
}

static void VxDriverDescSetDisplayMode(VxDriverDesc &desc, int index, const VxDisplayMode &value) {
#if CKVERSION == 0x13022002
    if (index < 0 || index >= desc.DisplayModeCount) {
#else
    if (index < 0 || index >= desc.DisplayModes.Size()) {
#endif
        // Set a script exception
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Out of range");
        return;
    }
    desc.DisplayModes[index] = value;
}

static VxImageDescEx *VxDriverDescGetTextureFormat(const VxDriverDesc &desc, int index) {
    if (index < 0 || index >= desc.TextureFormats.Size()) {
        // Set a script exception if the index is out of range
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx) ctx->SetException("Index out of range");
        return nullptr;
    }
    return &desc.TextureFormats[index];
}

static void VxDriverDescSetTextureFormat(VxDriverDesc &desc, int index, const VxImageDescEx &value) {
    if (index < 0 || index >= desc.TextureFormats.Size()) {
        // Set a script exception if the index is out of range
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx) ctx->SetException("Index out of range");
        return;
    }
    desc.TextureFormats[index] = value;
}

void RegisterVxDriverDesc(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("VxDriverDesc", "int IsHardware", offsetof(VxDriverDesc, IsHardware)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxDriverDesc", "Vx2DCapsDesc Caps2D", offsetof(VxDriverDesc, Caps2D)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxDriverDesc", "Vx3DCapsDesc Caps3D", offsetof(VxDriverDesc, Caps3D)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("VxDriverDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxDriverDesc *self) { new(self) VxDriverDesc(); }, (VxDriverDesc*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("VxDriverDesc", asBEHAVE_CONSTRUCT, "void f(const VxDriverDesc &in other)", asFUNCTIONPR([](const VxDriverDesc &desc, VxDriverDesc *self) { new(self) VxDriverDesc(desc); }, (const VxDriverDesc &, VxDriverDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("VxDriverDesc", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxDriverDesc *self) { self->~VxDriverDesc(); }, (VxDriverDesc *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("VxDriverDesc", "VxDriverDesc &opAssign(const VxDriverDesc &in other)", asMETHODPR(VxDriverDesc, operator=, (const VxDriverDesc &), VxDriverDesc &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("VxDriverDesc", "string GetDriverDesc() const", asFUNCTION(VxDriverDescGetDriverDesc), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("VxDriverDesc", "void SetDriverDesc(const string &in desc)", asFUNCTION(VxDriverDescSetDriverDesc), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("VxDriverDesc", "string GetDriverName() const", asFUNCTION(VxDriverDescGetDriverName), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("VxDriverDesc", "void SetDriverName(const string &in name)", asFUNCTION(VxDriverDescSetDriverName), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("VxDriverDesc", "VxDisplayMode &GetDisplayMode(int index) const", asFUNCTION(VxDriverDescGetDisplayMode), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("VxDriverDesc", "void SetDisplayMode(int index, const VxDisplayMode &in mode)", asFUNCTION(VxDriverDescSetDisplayMode), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("VxDriverDesc", "VxImageDescEx &GetTextureFormat(int index) const", asFUNCTION(VxDriverDescGetTextureFormat), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("VxDriverDesc", "void SetTextureFormat(int index, const VxImageDescEx &in format)", asFUNCTION(VxDriverDescSetTextureFormat), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// VxIntersectionDesc

void RegisterVxIntersectionDesc(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("VxIntersectionDesc", "CKRenderObject@ Object", offsetof(VxIntersectionDesc, Object)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxIntersectionDesc", "VxVector IntersectionPoint", offsetof(VxIntersectionDesc, IntersectionPoint)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxIntersectionDesc", "VxVector IntersectionNormal", offsetof(VxIntersectionDesc, IntersectionNormal)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxIntersectionDesc", "float TexU", offsetof(VxIntersectionDesc, TexU)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxIntersectionDesc", "float TexV", offsetof(VxIntersectionDesc, TexV)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxIntersectionDesc", "float Distance", offsetof(VxIntersectionDesc, Distance)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxIntersectionDesc", "int FaceIndex", offsetof(VxIntersectionDesc, FaceIndex)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("VxIntersectionDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxIntersectionDesc *self) { new(self) VxIntersectionDesc(); }, (VxIntersectionDesc*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("VxIntersectionDesc", asBEHAVE_CONSTRUCT, "void f(const VxIntersectionDesc &in other)", asFUNCTIONPR([](const VxIntersectionDesc &desc, VxIntersectionDesc *self) { new(self) VxIntersectionDesc(desc); }, (const VxIntersectionDesc &, VxIntersectionDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("VxIntersectionDesc", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxIntersectionDesc *self) { self->~VxIntersectionDesc(); }, (VxIntersectionDesc *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("VxIntersectionDesc", "VxIntersectionDesc &opAssign(const VxIntersectionDesc &in other)", asMETHODPR(VxIntersectionDesc, operator=, (const VxIntersectionDesc &), VxIntersectionDesc &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// VxStats

void RegisterVxStats(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("VxStats", "int NbTrianglesDrawn", offsetof(VxStats, NbTrianglesDrawn)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "int NbPointsDrawn", offsetof(VxStats, NbPointsDrawn)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "int NbLinesDrawn", offsetof(VxStats, NbLinesDrawn)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "int NbVerticesProcessed", offsetof(VxStats, NbVerticesProcessed)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "int NbObjectDrawn", offsetof(VxStats, NbObjectDrawn)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float SmoothedFps", offsetof(VxStats, SmoothedFps)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "int RenderStateCacheHit", offsetof(VxStats, RenderStateCacheHit)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "int RenderStateCacheMiss", offsetof(VxStats, RenderStateCacheMiss)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float DevicePreCallbacks", offsetof(VxStats, DevicePreCallbacks)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float SceneTraversalTime", offsetof(VxStats, SceneTraversalTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float TransparentObjectsSortTime", offsetof(VxStats, TransparentObjectsSortTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float ObjectsRenderTime", offsetof(VxStats, ObjectsRenderTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float ObjectsCallbacksTime", offsetof(VxStats, ObjectsCallbacksTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float SkinTime", offsetof(VxStats, SkinTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float SpriteTime", offsetof(VxStats, SpriteTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float SpriteCallbacksTime", offsetof(VxStats, SpriteCallbacksTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float DevicePostCallbacks", offsetof(VxStats, DevicePostCallbacks)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxStats", "float BackToFrontTime", offsetof(VxStats, BackToFrontTime)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("VxStats", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxStats *self) { new(self) VxStats(); }, (VxStats*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("VxStats", asBEHAVE_CONSTRUCT, "void f(const VxStats &in other)", asFUNCTIONPR([](const VxStats &stats, VxStats *self) { new(self) VxStats(stats); }, (const VxStats &, VxStats *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("VxStats", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxStats *self) { self->~VxStats(); }, (VxStats *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("VxStats", "VxStats &opAssign(const VxStats &in other)", asMETHODPR(VxStats, operator=, (const VxStats &), VxStats &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKGUID

void RegisterCKGUID(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKGUID", "CKDWORD d1", offsetof(CKGUID, d1)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKGUID", "CKDWORD d2", offsetof(CKGUID, d2)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKGUID", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKGUID *self) { new(self) CKGUID(); }, (CKGUID *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKGUID", asBEHAVE_CONSTRUCT, "void f(CKDWORD gd1 = 0, CKDWORD gd2 = 0)", asFUNCTIONPR([](CKDWORD gd1, CKDWORD gd2, CKGUID *self) { new(self) CKGUID(gd1, gd2); }, (CKDWORD, CKDWORD, CKGUID *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKGUID", asBEHAVE_CONSTRUCT, "void f(const CKGUID &in other)", asFUNCTIONPR([](const CKGUID &guid, CKGUID *self) { new(self) CKGUID(guid); }, (const CKGUID &, CKGUID *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKGUID", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {CKDWORD, CKDWORD}", asFUNCTIONPR([](CKDWORD *list, CKGUID *self) { new(self) CKGUID(list[0], list[1]); }, (CKDWORD *, CKGUID *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKGUID", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKGUID *self) { self->~CKGUID(); }, (CKGUID *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKGUID", "CKGUID &opAssign(const CKGUID &in other)", asMETHODPR(CKGUID, operator=, (const CKGUID &), CKGUID &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKGUID", "int opCmp(const CKGUID &in other) const", asFUNCTIONPR([](const CKGUID &lhs, const CKGUID &rhs) -> int { if (lhs == rhs) return 0; else if (lhs < rhs) return -1; else return 1; }, (const CKGUID &, const CKGUID &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKGUID", "bool IsValid() const", asFUNCTIONPR([](const CKGUID &guid) -> bool { return const_cast<CKGUID &>(guid).IsValid(); }, (const CKGUID &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// VxEffectDescription

static bool RejectNativeCallbackPointerWrite(const NativePointer &ptr, const char *message) {
    if (ptr.IsNull()) {
        return false;
    }
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
    return true;
}

static void SetVxEffectDescriptionCallback(VxEffectDescription *self, NativePointer ptr) {
    if (RejectNativeCallbackPointerWrite(ptr, "VxEffectDescription.SetCallback only accepts a null NativePointer from script.")) {
        return;
    }
    self->SetCallback = nullptr;
}

static void SetVxEffectDescriptionCallbackArg(VxEffectDescription *self, NativePointer ptr) {
    self->CallbackArg = ptr.Get();
}

void RegisterVxEffectDescription(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("VxEffectDescription", "VX_EFFECT EffectIndex", asOFFSET(VxEffectDescription, EffectIndex)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "XString Summary", asOFFSET(VxEffectDescription, Summary)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "XString Description", asOFFSET(VxEffectDescription, Description)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "XString DescImage", asOFFSET(VxEffectDescription, DescImage)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "int MaxTextureCount", asOFFSET(VxEffectDescription, MaxTextureCount)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "int NeededTextureCoordsCount", asOFFSET(VxEffectDescription, NeededTextureCoordsCount)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "XString Tex1Description", asOFFSET(VxEffectDescription, Tex1Description)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "XString Tex2Description", asOFFSET(VxEffectDescription, Tex2Description)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "XString Tex3Description", asOFFSET(VxEffectDescription, Tex3Description)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "CKGUID ParameterType", asOFFSET(VxEffectDescription, ParameterType)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "XString ParameterDescription", asOFFSET(VxEffectDescription, ParameterDescription)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("VxEffectDescription", "XString ParameterDefaultValue", asOFFSET(VxEffectDescription, ParameterDefaultValue)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("VxEffectDescription", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxEffectDescription *self) { new(self) VxEffectDescription(); }, (VxEffectDescription *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("VxEffectDescription", asBEHAVE_CONSTRUCT, "void f(const VxEffectDescription &in other)", asFUNCTIONPR([](const VxEffectDescription &desc, VxEffectDescription *self) { new(self) VxEffectDescription(desc); }, (const VxEffectDescription &, VxEffectDescription *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("VxEffectDescription", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxEffectDescription *self) { self->~VxEffectDescription(); }, (VxEffectDescription *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("VxEffectDescription", "VxEffectDescription &opAssign(const VxEffectDescription &in other)", asMETHODPR(VxEffectDescription, operator=, (const VxEffectDescription &), VxEffectDescription &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("VxEffectDescription", "NativePointer get_SetCallback() const", asFUNCTIONPR([](const VxEffectDescription *self) { return NativePointer(self->SetCallback); }, (const VxEffectDescription *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("VxEffectDescription", "void set_SetCallback(NativePointer ptr)", asFUNCTION(SetVxEffectDescriptionCallback), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("VxEffectDescription", "NativePointer get_CallbackArg() const", asFUNCTIONPR([](const VxEffectDescription *self) { return NativePointer(self->CallbackArg); }, (const VxEffectDescription *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("VxEffectDescription", "void set_CallbackArg(NativePointer ptr)", asFUNCTION(SetVxEffectDescriptionCallbackArg), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKBehaviorContext

static void ConstructCKBehaviorContext(CKBehaviorContext *self) {
    new(self) CKBehaviorContext();
    self->Behavior = nullptr;
    self->DeltaTime = 0.0f;
    self->Context = nullptr;
    self->CurrentLevel = nullptr;
    self->CurrentScene = nullptr;
    self->PreviousScene = nullptr;
    self->CurrentRenderContext = nullptr;
    self->ParameterManager = nullptr;
    self->MessageManager = nullptr;
    self->AttributeManager = nullptr;
    self->TimeManager = nullptr;
    self->CallbackMessage = 0;
    self->CallbackArg = nullptr;
}

void RegisterCKBehaviorContext(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKBehavior@ Behavior", asOFFSET(CKBehaviorContext, Behavior)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "float DeltaTime", asOFFSET(CKBehaviorContext, DeltaTime)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKContext@ Context", asOFFSET(CKBehaviorContext, Context)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKLevel@ CurrentLevel", asOFFSET(CKBehaviorContext, CurrentLevel)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKScene@ CurrentScene", asOFFSET(CKBehaviorContext, CurrentScene)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKScene@ PreviousScene", asOFFSET(CKBehaviorContext, PreviousScene)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKRenderContext@ CurrentRenderContext", asOFFSET(CKBehaviorContext, CurrentRenderContext)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKParameterManager@ ParameterManager", asOFFSET(CKBehaviorContext, ParameterManager)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKMessageManager@ MessageManager", asOFFSET(CKBehaviorContext, MessageManager)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKAttributeManager@ AttributeManager", asOFFSET(CKBehaviorContext, AttributeManager)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKTimeManager@ TimeManager", asOFFSET(CKBehaviorContext, TimeManager)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "CKDWORD CallbackMessage", asOFFSET(CKBehaviorContext, CallbackMessage)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBehaviorContext", "uintptr_t CallbackArg", asOFFSET(CKBehaviorContext, CallbackArg)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKBehaviorContext", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCKBehaviorContext), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKBehaviorContext", asBEHAVE_CONSTRUCT, "void f(const CKBehaviorContext &in other)", asFUNCTIONPR([](const CKBehaviorContext &ctx, CKBehaviorContext *self) { new(self) CKBehaviorContext(ctx); }, (const CKBehaviorContext &, CKBehaviorContext *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKBehaviorContext", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKBehaviorContext *self) { self->~CKBehaviorContext(); }, (CKBehaviorContext *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBehaviorContext", "CKBehaviorContext &opAssign(const CKBehaviorContext &in other)", asMETHODPR(CKBehaviorContext, operator=, (const CKBehaviorContext &), CKBehaviorContext &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBehaviorContext", "NativePointer get_CallbackArg() const", asFUNCTIONPR([](const CKBehaviorContext *self) { return NativePointer(self->CallbackArg); }, (const CKBehaviorContext *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorContext", "void set_CallbackArg(NativePointer ptr)", asFUNCTIONPR([](CKBehaviorContext *self, NativePointer ptr) { self->CallbackArg = ptr.Get(); }, (CKBehaviorContext *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKUICallbackStruct

static std::mutex g_CKUICallbackStringMutex;
static std::unordered_set<CKSTRING> g_CKUICallbackOwnedStrings;

static void ReleaseCKUICallbackString(CKSTRING value) {
    if (!value) {
        return;
    }

    bool owned = false;
    {
        std::lock_guard<std::mutex> lock(g_CKUICallbackStringMutex);
        owned = g_CKUICallbackOwnedStrings.erase(value) > 0;
    }
    if (owned) {
        CKDeletePointer(value);
    }
}

static CKSTRING DuplicateCKUICallbackString(const std::string &value) {
    CKSTRING copy = CKStrdup(const_cast<CKSTRING>(value.c_str()));
    if (copy) {
        std::lock_guard<std::mutex> lock(g_CKUICallbackStringMutex);
        g_CKUICallbackOwnedStrings.insert(copy);
    }
    return copy;
}

static CKSTRING DuplicateCKUICallbackString(CKSTRING value) {
    CKSTRING copy = CKStrdup(value);
    if (copy) {
        std::lock_guard<std::mutex> lock(g_CKUICallbackStringMutex);
        g_CKUICallbackOwnedStrings.insert(copy);
    }
    return copy;
}

static bool CKUICallbackReasonUsesConsoleString(CKDWORD reason) {
    return reason == CKUIM_OUTTOCONSOLE || reason == CKUIM_OUTTOINFOBAR;
}

static bool IsOwnedCKUICallbackString(CKSTRING value) {
    if (!value) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_CKUICallbackStringMutex);
    return g_CKUICallbackOwnedStrings.find(value) != g_CKUICallbackOwnedStrings.end();
}

static bool ShouldDuplicateCKUICallbackString(const CKUICallbackStruct &value) {
    return value.ConsoleString &&
           (CKUICallbackReasonUsesConsoleString(value.Reason) || IsOwnedCKUICallbackString(value.ConsoleString));
}

static void ConstructCKUICallbackStruct(CKUICallbackStruct *self) {
    std::memset(self, 0, sizeof(CKUICallbackStruct));
}

static void ConstructCKUICallbackStructCopy(const CKUICallbackStruct &other, CKUICallbackStruct *self) {
    const bool duplicateConsoleString = ShouldDuplicateCKUICallbackString(other);
    *self = other;
    if (duplicateConsoleString) {
        self->ConsoleString = DuplicateCKUICallbackString(other.ConsoleString);
    }
}

static void DestructCKUICallbackStruct(CKUICallbackStruct *self) {
    ReleaseCKUICallbackString(self->ConsoleString);
    std::memset(self, 0, sizeof(CKUICallbackStruct));
}

static CKUICallbackStruct &AssignCKUICallbackStruct(CKUICallbackStruct *self, const CKUICallbackStruct &other) {
    if (self == &other) {
        return *self;
    }

    CKSTRING consoleStringCopy = nullptr;
    const bool duplicateConsoleString = ShouldDuplicateCKUICallbackString(other);
    if (duplicateConsoleString) {
        consoleStringCopy = DuplicateCKUICallbackString(other.ConsoleString);
    }

    ReleaseCKUICallbackString(self->ConsoleString);
    *self = other;
    if (duplicateConsoleString) {
        self->ConsoleString = consoleStringCopy;
    }
    return *self;
}

static void SetCKUICallbackConsoleString(CKUICallbackStruct *self, const std::string &value) {
    ReleaseCKUICallbackString(self->ConsoleString);
    self->ConsoleString = DuplicateCKUICallbackString(value);
}

void RegisterCKUICallbackStruct(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectBehaviour("CKUICallbackStruct", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCKUICallbackStruct), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKUICallbackStruct", asBEHAVE_CONSTRUCT, "void f(const CKUICallbackStruct &in other)", asFUNCTION(ConstructCKUICallbackStructCopy), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKUICallbackStruct", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructCKUICallbackStruct), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "CKUICallbackStruct &opAssign(const CKUICallbackStruct &in other)", asFUNCTION(AssignCKUICallbackStruct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectProperty("CKUICallbackStruct", "CKDWORD Reason", asOFFSET(CKUICallbackStruct, Reason)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKUICallbackStruct", "CKDWORD Param3", asOFFSET(CKUICallbackStruct, Param3)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKUICallbackStruct", "CKDWORD get_Param1() const", asFUNCTIONPR([](const CKUICallbackStruct *self) { return self->Param1; }, (const CKUICallbackStruct *), CKDWORD), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "void set_Param1(CKDWORD value)", asFUNCTIONPR([](CKUICallbackStruct *self, CKDWORD value) { self->Param1 = value; }, (CKUICallbackStruct *, CKDWORD), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "int get_NbObjectsLoaded() const", asFUNCTIONPR([](const CKUICallbackStruct *self) { return self->NbObjectsLoaded; }, (const CKUICallbackStruct *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "void set_NbObjectsLoaded(int value)", asFUNCTIONPR([](CKUICallbackStruct *self, int value) { self->NbObjectsLoaded = value; }, (CKUICallbackStruct *, int), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "int get_NbObjectsLoaded() const", asFUNCTIONPR([](const CKUICallbackStruct *self) { return self->NbObjetsLoaded; }, (const CKUICallbackStruct *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "void set_NbObjectsLoaded(int value)", asFUNCTIONPR([](CKUICallbackStruct *self, int value) { self->NbObjetsLoaded = value; }, (CKUICallbackStruct *, int), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif

    r = engine->RegisterObjectMethod("CKUICallbackStruct", "bool get_DoBeep() const", asFUNCTIONPR([](const CKUICallbackStruct *self) -> bool { return self->DoBeep; }, (const CKUICallbackStruct *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "void set_DoBeep(bool value)", asFUNCTIONPR([](CKUICallbackStruct *self, bool value) { self->DoBeep = value; }, (CKUICallbackStruct *, bool), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKUICallbackStruct", "CKDWORD get_Param2() const", asFUNCTIONPR([](const CKUICallbackStruct *self) { return self->Param2; }, (const CKUICallbackStruct *), CKDWORD), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "void set_Param2(CKDWORD value)", asFUNCTIONPR([](CKUICallbackStruct *self, CKDWORD value) { self->Param2 = value; }, (CKUICallbackStruct *, CKDWORD), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "int get_NbObjectsToLoad() const", asFUNCTIONPR([](const CKUICallbackStruct *self) { return self->NbObjectsToLoad; }, (const CKUICallbackStruct *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "void set_NbObjectsToLoad(int value)", asFUNCTIONPR([](CKUICallbackStruct *self, int value) { self->NbObjectsToLoad = value; }, (CKUICallbackStruct *, int), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "int get_NbObjectsToLoad() const", asFUNCTIONPR([](const CKUICallbackStruct *self) { return self->NbObjetsToLoad; }, (const CKUICallbackStruct *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "void set_NbObjectsToLoad(int value)", asFUNCTIONPR([](CKUICallbackStruct *self, int value) { self->NbObjetsToLoad = value; }, (CKUICallbackStruct *, int), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif

    r = engine->RegisterObjectMethod("CKUICallbackStruct", "string get_ConsoleString() const", asFUNCTIONPR([](const CKUICallbackStruct *self) -> std::string { return ScriptStringify(self->ConsoleString); }, (const CKUICallbackStruct *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKUICallbackStruct", "void set_ConsoleString(const string &in value)", asFUNCTION(SetCKUICallbackConsoleString), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKClassDesc

static CKClassDesc &MissingCKClassDesc(const char *message) {
    static thread_local CKClassDesc dummy;
    dummy = CKClassDesc();
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
    return dummy;
}

static CKClassDesc &CKGetClassDescChecked(CK_CLASSID cid) {
    if (CKClassDesc *desc = CKGetClassDesc(cid)) {
        return *desc;
    }
    return MissingCKClassDesc("CKGetClassDesc did not find a matching class.");
}

static CK_CLASSID CKClassDescGetToNotify(const CKClassDesc &desc, int index) {
    if (index < 0 || index >= desc.ToNotify.Size()) {
        // Set a script exception
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Out of range");
        return -1;
    }
    return desc.ToNotify[index];
}

static void CKClassDescSetToNotify(CKClassDesc &desc, int index, CK_CLASSID value) {
    if (index < 0 || index >= desc.ToNotify.Size()) {
        // Set a script exception
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Out of range");
        return;
    }
    desc.ToNotify[index] = value;
}

static bool RejectNativeFunctionPointerInstall(const NativePointer &ptr, const char *message) {
    if (ptr.IsNull()) {
        return false;
    }
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
    return true;
}

static void SetCKClassDescRegisterFct(CKClassDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKClassDesc.RegisterFct is read-only from script.");
}

static void SetCKClassDescCreationFct(CKClassDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKClassDesc.CreationFct is read-only from script.");
}

static void SetCKClassDescNameFct(CKClassDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKClassDesc.NameFct is read-only from script.");
}

static void SetCKClassDescDependsFct(CKClassDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKClassDesc.DependsFct is read-only from script.");
}

static void SetCKClassDescDependsCountFct(CKClassDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKClassDesc.DependsCountFct is read-only from script.");
}

void RegisterCKClassDesc(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKClassDesc", "int Done", asOFFSET(CKClassDesc, Done)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "CK_CLASSID Parent", asOFFSET(CKClassDesc, Parent)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKClassDesc", "uintptr_t RegisterFct", asOFFSET(CKClassDesc, RegisterFct)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKClassDesc", "uintptr_t CreationFct", asOFFSET(CKClassDesc, CreationFct)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKClassDesc", "uintptr_t NameFct", asOFFSET(CKClassDesc, NameFct)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKClassDesc", "uintptr_t DependsFct", asOFFSET(CKClassDesc, DependsFct)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKClassDesc", "uintptr_t DependsCountFct", asOFFSET(CKClassDesc, DependsCountFct)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "CKDWORD DefaultOptions", asOFFSET(CKClassDesc, DefaultOptions)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "CKDWORD DefaultCopyDependencies", asOFFSET(CKClassDesc, DefaultCopyDependencies)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "CKDWORD DefaultDeleteDependencies", asOFFSET(CKClassDesc, DefaultDeleteDependencies)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "CKDWORD DefaultReplaceDependencies", asOFFSET(CKClassDesc, DefaultReplaceDependencies)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "CKDWORD DefaultSaveDependencies", asOFFSET(CKClassDesc, DefaultSaveDependencies)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "CKGUID Parameter", asOFFSET(CKClassDesc, Parameter)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "int DerivationLevel", asOFFSET(CKClassDesc, DerivationLevel)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "XBitArray Parents", asOFFSET(CKClassDesc, Parents)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "XBitArray Children", asOFFSET(CKClassDesc, Children)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "XBitArray ToBeNotify", asOFFSET(CKClassDesc, ToBeNotify)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "XBitArray CommonToBeNotify", asOFFSET(CKClassDesc, CommonToBeNotify)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKClassDesc", "XClassIDArray ToNotify", asOFFSET(CKClassDesc, ToNotify)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKClassDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKClassDesc *self) { new(self) CKClassDesc(); }, (CKClassDesc*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKClassDesc", asBEHAVE_CONSTRUCT, "void f(const CKClassDesc &in other)", asFUNCTIONPR([](const CKClassDesc &desc, CKClassDesc *self) { new(self) CKClassDesc(desc); }, (const CKClassDesc &, CKClassDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKClassDesc", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKClassDesc *self) { self->~CKClassDesc(); }, (CKClassDesc *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKClassDesc", "CKClassDesc &opAssign(const CKClassDesc &in other)", asMETHODPR(CKClassDesc, operator=, (const CKClassDesc &), CKClassDesc &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKClassDesc", "CK_CLASSID GetToNotify(int index) const", asFUNCTION(CKClassDescGetToNotify), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKClassDesc", "void SetToNotify(int index, CK_CLASSID cid)", asFUNCTION(CKClassDescSetToNotify), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKClassDesc", "NativePointer get_RegisterFct() const", asFUNCTIONPR([](const CKClassDesc *self) { return NativePointer(self->RegisterFct); }, (const CKClassDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKClassDesc", "void set_RegisterFct(NativePointer ptr)", asFUNCTION(SetCKClassDescRegisterFct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKClassDesc", "NativePointer get_CreationFct() const", asFUNCTIONPR([](const CKClassDesc *self) { return NativePointer(self->CreationFct); }, (const CKClassDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKClassDesc", "void set_CreationFct(NativePointer ptr)", asFUNCTION(SetCKClassDescCreationFct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKClassDesc", "NativePointer get_NameFct() const", asFUNCTIONPR([](const CKClassDesc *self) { return NativePointer(self->NameFct); }, (const CKClassDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKClassDesc", "void set_NameFct(NativePointer ptr)", asFUNCTION(SetCKClassDescNameFct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKClassDesc", "NativePointer get_DependsFct() const", asFUNCTIONPR([](const CKClassDesc *self) { return NativePointer(self->DependsFct); }, (const CKClassDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKClassDesc", "void set_DependsFct(NativePointer ptr)", asFUNCTION(SetCKClassDescDependsFct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKClassDesc", "NativePointer get_DependsCountFct() const", asFUNCTIONPR([](const CKClassDesc *self) { return NativePointer(self->DependsCountFct); }, (const CKClassDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKClassDesc", "void set_DependsCountFct(NativePointer ptr)", asFUNCTION(SetCKClassDescDependsCountFct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKPluginInfo

static bool RejectNonNullPluginCallbackPointer(const NativePointer &ptr, const char *message) {
    if (ptr.IsNull()) {
        return false;
    }
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
    return true;
}

static void SetCKPluginInfoInitInstanceFct(CKPluginInfo *self, NativePointer ptr) {
    if (RejectNonNullPluginCallbackPointer(ptr, "CKPluginInfo.m_InitInstanceFct only accepts a null NativePointer from script.")) {
        return;
    }
    self->m_InitInstanceFct = nullptr;
}

static void SetCKPluginInfoExitInstanceFct(CKPluginInfo *self, NativePointer ptr) {
    if (RejectNonNullPluginCallbackPointer(ptr, "CKPluginInfo.m_ExitInstanceFct only accepts a null NativePointer from script.")) {
        return;
    }
    self->m_ExitInstanceFct = nullptr;
}

static void ConstructCKPluginInfo(CKPluginInfo *self) {
    std::memset(self, 0, sizeof(CKPluginInfo));
    new(self) CKPluginInfo();
}

void RegisterCKPluginInfo(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKPluginInfo", "CKGUID m_GUID", asOFFSET(CKPluginInfo, m_GUID)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginInfo", "CKFileExtension m_Extension", asOFFSET(CKPluginInfo, m_Extension)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginInfo", "XString m_Description", asOFFSET(CKPluginInfo, m_Description)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginInfo", "XString m_Author", asOFFSET(CKPluginInfo, m_Author)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginInfo", "XString m_Summary", asOFFSET(CKPluginInfo, m_Summary)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginInfo", "CKDWORD m_Version", asOFFSET(CKPluginInfo, m_Version)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginInfo", "CK_PLUGIN_TYPE m_Type", asOFFSET(CKPluginInfo, m_Type)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginInfo", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCKPluginInfo), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPluginInfo", asBEHAVE_CONSTRUCT, "void f(const CKPluginInfo &in other)", asFUNCTIONPR([](const CKPluginInfo &info, CKPluginInfo *self) { new(self) CKPluginInfo(info); }, (const CKPluginInfo &, CKPluginInfo *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginInfo", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPluginInfo *self) { self->~CKPluginInfo(); }, (CKPluginInfo *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginInfo", "CKPluginInfo &opAssign(const CKPluginInfo &in other)", asMETHODPR(CKPluginInfo, operator=, (const CKPluginInfo &), CKPluginInfo &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginInfo", "NativePointer get_m_InitInstanceFct() const", asFUNCTIONPR([](const CKPluginInfo *self) { return NativePointer(self->m_InitInstanceFct); }, (const CKPluginInfo *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPluginInfo", "void set_m_InitInstanceFct(NativePointer ptr)", asFUNCTION(SetCKPluginInfoInitInstanceFct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginInfo", "NativePointer get_m_ExitInstanceFct() const", asFUNCTIONPR([](const CKPluginInfo *self) { return NativePointer(self->m_ExitInstanceFct); }, (const CKPluginInfo *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPluginInfo", "void set_m_ExitInstanceFct(NativePointer ptr)", asFUNCTION(SetCKPluginInfoExitInstanceFct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKEnumStruct

static std::mutex g_CKEnumStructOwnedMutex;
static std::unordered_set<CKEnumStruct *> g_CKEnumStructOwnedValues;

static void TrackCKEnumStructValue(CKEnumStruct *self) {
    std::lock_guard<std::mutex> lock(g_CKEnumStructOwnedMutex);
    g_CKEnumStructOwnedValues.insert(self);
}

static bool UntrackCKEnumStructValue(CKEnumStruct *self) {
    std::lock_guard<std::mutex> lock(g_CKEnumStructOwnedMutex);
    return g_CKEnumStructOwnedValues.erase(self) > 0;
}

static bool IsTrackedCKEnumStructValue(CKEnumStruct *self) {
    std::lock_guard<std::mutex> lock(g_CKEnumStructOwnedMutex);
    return g_CKEnumStructOwnedValues.find(self) != g_CKEnumStructOwnedValues.end();
}

static void ReleaseCKEnumStructStorage(CKEnumStruct *self) {
    if (!self) {
        return;
    }

    if (self->Desc) {
        for (int i = 0; i < self->NbData; ++i) {
            CKDeletePointer(self->Desc[i]);
        }
        delete[] self->Desc;
    }
    delete[] self->Vals;

    self->NbData = 0;
    self->Vals = nullptr;
    self->Desc = nullptr;
}

static void CopyCKEnumStructStorage(CKEnumStruct *self, const CKEnumStruct &other) {
    self->NbData = 0;
    self->Vals = nullptr;
    self->Desc = nullptr;

    if (other.NbData <= 0) {
        return;
    }

    self->NbData = other.NbData;
    self->Vals = new int[other.NbData];
    self->Desc = new CKSTRING[other.NbData];
    for (int i = 0; i < other.NbData; ++i) {
        self->Vals[i] = other.Vals ? other.Vals[i] : 0;
        self->Desc[i] = other.Desc && other.Desc[i] ? CKStrdup(other.Desc[i]) : nullptr;
    }
}

static void ConstructCKEnumStruct(CKEnumStruct *self) {
    self->NbData = 0;
    self->Vals = nullptr;
    self->Desc = nullptr;
    TrackCKEnumStructValue(self);
}

static void ConstructCKEnumStructCopy(const CKEnumStruct &other, CKEnumStruct *self) {
    CopyCKEnumStructStorage(self, other);
    TrackCKEnumStructValue(self);
}

static void DestructCKEnumStruct(CKEnumStruct *self) {
    if (UntrackCKEnumStructValue(self)) {
        ReleaseCKEnumStructStorage(self);
    }
}

static CKEnumStruct &AssignCKEnumStruct(CKEnumStruct *self, const CKEnumStruct &other) {
    if (self == &other) {
        return *self;
    }

    if (!IsTrackedCKEnumStructValue(self)) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("Cannot assign to a borrowed CKEnumStruct descriptor.");
        }
        return *self;
    }

    ReleaseCKEnumStructStorage(self);
    CopyCKEnumStructStorage(self, other);
    return *self;
}

static int GetCKEnumStructValue(const CKEnumStruct *self, int index) {
    if (!self || index < 0 || index >= self->NbData || !self->Vals) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKEnumStruct enum index out of range.");
        }
        return 0;
    }
    return self->Vals[index];
}

static std::string GetCKEnumStructDescription(const CKEnumStruct *self, int index) {
    if (!self || index < 0 || index >= self->NbData || !self->Desc) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKEnumStruct enum index out of range.");
        }
        return {};
    }
    return ScriptStringify(self->Desc[index]);
}

void RegisterCKEnumStruct(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectBehaviour("CKEnumStruct", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCKEnumStruct), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKEnumStruct", asBEHAVE_CONSTRUCT, "void f(const CKEnumStruct &in other)", asFUNCTION(ConstructCKEnumStructCopy), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKEnumStruct", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructCKEnumStruct), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKEnumStruct", "CKEnumStruct &opAssign(const CKEnumStruct &in other)", asFUNCTION(AssignCKEnumStruct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKEnumStruct", "int GetNumEnums() const", asMETHOD(CKEnumStruct, GetNumEnums), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKEnumStruct", "int GetEnumValue(int index) const", asFUNCTION(GetCKEnumStructValue), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKEnumStruct", "string GetEnumDescription(int index) const", asFUNCTION(GetCKEnumStructDescription), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKFlagsStruct

static std::mutex g_CKFlagsStructOwnedMutex;
static std::unordered_set<CKFlagsStruct *> g_CKFlagsStructOwnedValues;

static void TrackCKFlagsStructValue(CKFlagsStruct *self) {
    std::lock_guard<std::mutex> lock(g_CKFlagsStructOwnedMutex);
    g_CKFlagsStructOwnedValues.insert(self);
}

static bool UntrackCKFlagsStructValue(CKFlagsStruct *self) {
    std::lock_guard<std::mutex> lock(g_CKFlagsStructOwnedMutex);
    return g_CKFlagsStructOwnedValues.erase(self) > 0;
}

static bool IsTrackedCKFlagsStructValue(CKFlagsStruct *self) {
    std::lock_guard<std::mutex> lock(g_CKFlagsStructOwnedMutex);
    return g_CKFlagsStructOwnedValues.find(self) != g_CKFlagsStructOwnedValues.end();
}

static void ReleaseCKFlagsStructStorage(CKFlagsStruct *self) {
    if (!self) {
        return;
    }

    if (self->Desc) {
        for (int i = 0; i < self->NbData; ++i) {
            CKDeletePointer(self->Desc[i]);
        }
        delete[] self->Desc;
    }
    delete[] self->Vals;

    self->NbData = 0;
    self->Vals = nullptr;
    self->Desc = nullptr;
}

static void CopyCKFlagsStructStorage(CKFlagsStruct *self, const CKFlagsStruct &other) {
    self->NbData = 0;
    self->Vals = nullptr;
    self->Desc = nullptr;

    if (other.NbData <= 0) {
        return;
    }

    self->NbData = other.NbData;
    self->Vals = new int[other.NbData];
    self->Desc = new CKSTRING[other.NbData];
    for (int i = 0; i < other.NbData; ++i) {
        self->Vals[i] = other.Vals ? other.Vals[i] : 0;
        self->Desc[i] = other.Desc && other.Desc[i] ? CKStrdup(other.Desc[i]) : nullptr;
    }
}

static void ConstructCKFlagsStruct(CKFlagsStruct *self) {
    self->NbData = 0;
    self->Vals = nullptr;
    self->Desc = nullptr;
    TrackCKFlagsStructValue(self);
}

static void ConstructCKFlagsStructCopy(const CKFlagsStruct &other, CKFlagsStruct *self) {
    CopyCKFlagsStructStorage(self, other);
    TrackCKFlagsStructValue(self);
}

static void DestructCKFlagsStruct(CKFlagsStruct *self) {
    if (UntrackCKFlagsStructValue(self)) {
        ReleaseCKFlagsStructStorage(self);
    }
}

static CKFlagsStruct &AssignCKFlagsStruct(CKFlagsStruct *self, const CKFlagsStruct &other) {
    if (self == &other) {
        return *self;
    }

    if (!IsTrackedCKFlagsStructValue(self)) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("Cannot assign to a borrowed CKFlagsStruct descriptor.");
        }
        return *self;
    }

    ReleaseCKFlagsStructStorage(self);
    CopyCKFlagsStructStorage(self, other);
    return *self;
}

static int GetCKFlagsStructValue(const CKFlagsStruct *self, int index) {
    if (!self || index < 0 || index >= self->NbData || !self->Vals) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKFlagsStruct flag index out of range.");
        }
        return 0;
    }
    return self->Vals[index];
}

static std::string GetCKFlagsStructDescription(const CKFlagsStruct *self, int index) {
    if (!self || index < 0 || index >= self->NbData || !self->Desc) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKFlagsStruct flag index out of range.");
        }
        return {};
    }
    return ScriptStringify(self->Desc[index]);
}

void RegisterCKFlagsStruct(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectBehaviour("CKFlagsStruct", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCKFlagsStruct), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKFlagsStruct", asBEHAVE_CONSTRUCT, "void f(const CKFlagsStruct &in other)", asFUNCTION(ConstructCKFlagsStructCopy), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFlagsStruct", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructCKFlagsStruct), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFlagsStruct", "CKFlagsStruct &opAssign(const CKFlagsStruct &in other)", asFUNCTION(AssignCKFlagsStruct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFlagsStruct", "int GetNumFlags() const", asMETHOD(CKFlagsStruct, GetNumFlags), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFlagsStruct", "int GetFlagValue(int index) const", asFUNCTION(GetCKFlagsStructValue), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFlagsStruct", "string GetFlagDescription(int index) const", asFUNCTION(GetCKFlagsStructDescription), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKStructStruct

static std::mutex g_CKStructStructOwnedMutex;
static std::unordered_set<CKStructStruct *> g_CKStructStructOwnedValues;

static void TrackCKStructStructValue(CKStructStruct *self) {
    std::lock_guard<std::mutex> lock(g_CKStructStructOwnedMutex);
    g_CKStructStructOwnedValues.insert(self);
}

static bool UntrackCKStructStructValue(CKStructStruct *self) {
    std::lock_guard<std::mutex> lock(g_CKStructStructOwnedMutex);
    return g_CKStructStructOwnedValues.erase(self) > 0;
}

static bool IsTrackedCKStructStructValue(CKStructStruct *self) {
    std::lock_guard<std::mutex> lock(g_CKStructStructOwnedMutex);
    return g_CKStructStructOwnedValues.find(self) != g_CKStructStructOwnedValues.end();
}

static void ReleaseCKStructStructStorage(CKStructStruct *self) {
    if (!self) {
        return;
    }

    if (self->Desc) {
        for (int i = 0; i < self->NbData; ++i) {
            CKDeletePointer(self->Desc[i]);
        }
        delete[] self->Desc;
    }
    delete[] self->Guids;

    self->NbData = 0;
    self->Guids = nullptr;
    self->Desc = nullptr;
}

static void CopyCKStructStructStorage(CKStructStruct *self, const CKStructStruct &other) {
    self->NbData = 0;
    self->Guids = nullptr;
    self->Desc = nullptr;

    if (other.NbData <= 0) {
        return;
    }

    self->NbData = other.NbData;
    self->Guids = new CKGUID[other.NbData];
    self->Desc = new CKSTRING[other.NbData];
    for (int i = 0; i < other.NbData; ++i) {
        self->Guids[i] = other.Guids ? other.Guids[i] : CKGUID();
        self->Desc[i] = other.Desc && other.Desc[i] ? CKStrdup(other.Desc[i]) : nullptr;
    }
}

static void ConstructCKStructStruct(CKStructStruct *self) {
    self->NbData = 0;
    self->Guids = nullptr;
    self->Desc = nullptr;
    TrackCKStructStructValue(self);
}

static void ConstructCKStructStructCopy(const CKStructStruct &other, CKStructStruct *self) {
    CopyCKStructStructStorage(self, other);
    TrackCKStructStructValue(self);
}

static void DestructCKStructStruct(CKStructStruct *self) {
    if (UntrackCKStructStructValue(self)) {
        ReleaseCKStructStructStorage(self);
    }
}

static CKStructStruct &AssignCKStructStruct(CKStructStruct *self, const CKStructStruct &other) {
    if (self == &other) {
        return *self;
    }

    if (!IsTrackedCKStructStructValue(self)) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("Cannot assign to a borrowed CKStructStruct descriptor.");
        }
        return *self;
    }

    ReleaseCKStructStructStorage(self);
    CopyCKStructStructStorage(self, other);
    return *self;
}

static CKGUID GetCKStructStructGuid(const CKStructStruct *self, int index) {
    if (!self || index < 0 || index >= self->NbData || !self->Guids) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKStructStruct sub-parameter index out of range.");
        }
        return CKGUID();
    }
    return self->Guids[index];
}

static std::string GetCKStructStructDescription(const CKStructStruct *self, int index) {
    if (!self || index < 0 || index >= self->NbData || !self->Desc) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKStructStruct sub-parameter index out of range.");
        }
        return {};
    }
    return ScriptStringify(self->Desc[index]);
}

void RegisterCKStructStruct(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectBehaviour("CKStructStruct", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCKStructStruct), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKStructStruct", asBEHAVE_CONSTRUCT, "void f(const CKStructStruct &in other)", asFUNCTION(ConstructCKStructStructCopy), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKStructStruct", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructCKStructStruct), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStructStruct", "CKStructStruct &opAssign(const CKStructStruct &in other)", asFUNCTION(AssignCKStructStruct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStructStruct", "int GetNumSubParam() const", asMETHOD(CKStructStruct, GetNumSubParam), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStructStruct", "CKGUID GetSubParamGuid(int index) const", asFUNCTION(GetCKStructStructGuid), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStructStruct", "string GetSubParamDescription(int index) const", asFUNCTION(GetCKStructStructDescription), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKParameterTypeDesc

static void SetCKParameterTypeDescCreatorDll(CKParameterTypeDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKParameterTypeDesc.CreatorDll is read-only from script.");
}

static void SetCKParameterTypeDescCreateDefaultFunction(CKParameterTypeDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKParameterTypeDesc.CreateDefaultFunction is read-only from script.");
}

static void SetCKParameterTypeDescDeleteFunction(CKParameterTypeDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKParameterTypeDesc.DeleteFunction is read-only from script.");
}

static void SetCKParameterTypeDescSaveLoadFunction(CKParameterTypeDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKParameterTypeDesc.SaveLoadFunction is read-only from script.");
}

static void SetCKParameterTypeDescCheckFunction(CKParameterTypeDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKParameterTypeDesc.CheckFunction is read-only from script.");
}

static void SetCKParameterTypeDescCopyFunction(CKParameterTypeDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKParameterTypeDesc.CopyFunction is read-only from script.");
}

static void SetCKParameterTypeDescStringFunction(CKParameterTypeDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKParameterTypeDesc.StringFunction is read-only from script.");
}

static void SetCKParameterTypeDescUICreatorFunction(CKParameterTypeDesc *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKParameterTypeDesc.UICreatorFunction is read-only from script.");
}

void RegisterCKParameterTypeDesc(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "CKParameterType Index", asOFFSET(CKParameterTypeDesc, Index)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "CKGUID Guid", asOFFSET(CKParameterTypeDesc, Guid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "CKGUID DerivedFrom", asOFFSET(CKParameterTypeDesc, DerivedFrom)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "XString TypeName", asOFFSET(CKParameterTypeDesc, TypeName)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "int Valid", asOFFSET(CKParameterTypeDesc, Valid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "int DefaultSize", asOFFSET(CKParameterTypeDesc, DefaultSize)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKParameterTypeDesc", "uintptr_t CreateDefaultFunction", asOFFSET(CKParameterTypeDesc, CreateDefaultFunction)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKParameterTypeDesc", "uintptr_t DeleteFunction", asOFFSET(CKParameterTypeDesc, DeleteFunction)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKParameterTypeDesc", "uintptr_t SaveLoadFunction", asOFFSET(CKParameterTypeDesc, SaveLoadFunction)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKParameterTypeDesc", "uintptr_t CheckFunction", asOFFSET(CKParameterTypeDesc, CheckFunction)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKParameterTypeDesc", "uintptr_t CopyFunction", asOFFSET(CKParameterTypeDesc, CopyFunction)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKParameterTypeDesc", "uintptr_t StringFunction", asOFFSET(CKParameterTypeDesc, StringFunction)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKParameterTypeDesc", "uintptr_t UICreatorFunction", asOFFSET(CKParameterTypeDesc, UICreatorFunction)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "CKDWORD dwParam", asOFFSET(CKParameterTypeDesc, dwParam)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "CKDWORD dwFlags", asOFFSET(CKParameterTypeDesc, dwFlags)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "CKDWORD Cid", asOFFSET(CKParameterTypeDesc, Cid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "XBitArray DerivationMask", asOFFSET(CKParameterTypeDesc, DerivationMask)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKParameterTypeDesc", "CKGUID Saver_Manager", asOFFSET(CKParameterTypeDesc, Saver_Manager)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "NativePointer get_CreatorDll() const", asFUNCTIONPR([](const CKParameterTypeDesc *self) { return NativePointer(self->CreatorDll); }, (const CKParameterTypeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "void set_CreatorDll(NativePointer ptr)", asFUNCTION(SetCKParameterTypeDescCreatorDll), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKParameterTypeDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKParameterTypeDesc *self) { new(self) CKParameterTypeDesc(); }, (CKParameterTypeDesc*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKParameterTypeDesc", asBEHAVE_CONSTRUCT, "void f(const CKParameterTypeDesc &in other)", asFUNCTIONPR([](const CKParameterTypeDesc &desc, CKParameterTypeDesc *self) { new(self) CKParameterTypeDesc(desc); }, (const CKParameterTypeDesc &, CKParameterTypeDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKParameterTypeDesc", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKParameterTypeDesc *self) { self->~CKParameterTypeDesc(); }, (CKParameterTypeDesc *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "CKParameterTypeDesc &opAssign(const CKParameterTypeDesc &in other)", asMETHODPR(CKParameterTypeDesc, operator=, (const CKParameterTypeDesc &), CKParameterTypeDesc &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "NativePointer get_CreateDefaultFunction() const", asFUNCTIONPR([](const CKParameterTypeDesc *self) { return NativePointer(self->CreateDefaultFunction); }, (const CKParameterTypeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "void set_CreateDefaultFunction(NativePointer ptr)", asFUNCTION(SetCKParameterTypeDescCreateDefaultFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "NativePointer get_DeleteFunction() const", asFUNCTIONPR([](const CKParameterTypeDesc *self) { return NativePointer(self->DeleteFunction); }, (const CKParameterTypeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "void set_DeleteFunction(NativePointer ptr)", asFUNCTION(SetCKParameterTypeDescDeleteFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "NativePointer get_SaveLoadFunction() const", asFUNCTIONPR([](const CKParameterTypeDesc *self) { return NativePointer(self->SaveLoadFunction); }, (const CKParameterTypeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "void set_SaveLoadFunction(NativePointer ptr)", asFUNCTION(SetCKParameterTypeDescSaveLoadFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "NativePointer get_CheckFunction() const", asFUNCTIONPR([](const CKParameterTypeDesc *self) { return NativePointer(self->CheckFunction); }, (const CKParameterTypeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "void set_CheckFunction(NativePointer ptr)", asFUNCTION(SetCKParameterTypeDescCheckFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "NativePointer get_CopyFunction() const", asFUNCTIONPR([](const CKParameterTypeDesc *self) { return NativePointer(self->CopyFunction); }, (const CKParameterTypeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "void set_CopyFunction(NativePointer ptr)", asFUNCTION(SetCKParameterTypeDescCopyFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "NativePointer get_StringFunction() const", asFUNCTIONPR([](const CKParameterTypeDesc *self) { return NativePointer(self->StringFunction); }, (const CKParameterTypeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "void set_StringFunction(NativePointer ptr)", asFUNCTION(SetCKParameterTypeDescStringFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "NativePointer get_UICreatorFunction() const", asFUNCTIONPR([](const CKParameterTypeDesc *self) { return NativePointer(self->UICreatorFunction); }, (const CKParameterTypeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKParameterTypeDesc", "void set_UICreatorFunction(NativePointer ptr)", asFUNCTION(SetCKParameterTypeDescUICreatorFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKBitmapProperties

void RegisterCKBitmapProperties(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKBitmapProperties", "int m_Size", asOFFSET(CKBitmapProperties, m_Size)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBitmapProperties", "CKGUID m_ReaderGuid", asOFFSET(CKBitmapProperties, m_ReaderGuid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBitmapProperties", "CKFileExtension m_Ext", asOFFSET(CKBitmapProperties, m_Ext)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBitmapProperties", "VxImageDescEx m_Format", asOFFSET(CKBitmapProperties, m_Format)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKBitmapProperties", "uintptr_t m_Data", asOFFSET(CKBitmapProperties, m_Data)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapProperties", "NativePointer get_m_Data() const", asFUNCTION(GetCKBitmapPropertiesData), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapProperties", "void set_m_Data(NativePointer ptr)", asFUNCTION(SetCKBitmapPropertiesData), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKMovieProperties

void RegisterCKMovieProperties(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKMovieProperties", "int m_Size", asOFFSET(CKMovieProperties, m_Size)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKMovieProperties", "CKGUID m_ReaderGuid", asOFFSET(CKMovieProperties, m_ReaderGuid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKMovieProperties", "CKFileExtension m_Ext", asOFFSET(CKMovieProperties, m_Ext)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKMovieProperties", "VxImageDescEx m_Format", asOFFSET(CKMovieProperties, m_Format)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKMovieProperties", "NativePointer m_Data", asOFFSET(CKMovieProperties, m_Data)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMovieProperties", "NativePointer get_m_Data() const", asFUNCTIONPR([](const CKMovieProperties *self) { return NativePointer(self->m_Data); }, (const CKMovieProperties *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMovieProperties", "void set_m_Data(NativePointer ptr)", asFUNCTIONPR([](CKMovieProperties *self, NativePointer ptr) { self->m_Data = ptr.Get(); }, (CKMovieProperties *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKDataReader

template<typename T>
static void RegisterCKDataReaderMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    r = engine->RegisterObjectMethod(name, "void Release()", asMETHODPR(T, Release, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "CKPluginInfo GetReaderInfo()", asFUNCTIONPR([](T *self) -> CKPluginInfo {
        CKPluginInfo *info = self ? self->GetReaderInfo() : nullptr;
        return info ? *info : CKPluginInfo();
    }, (T *), CKPluginInfo), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "int GetOptionsCount()", asMETHODPR(T, GetOptionsCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "string GetOptionDescription(int i)", asFUNCTIONPR([](T *self, int i) -> std::string { return ScriptStringify(self->GetOptionDescription(i)); }, (T *, int), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "CK_DATAREADER_FLAGS GetFlags()", asMETHODPR(T, GetFlags, (), CK_DATAREADER_FLAGS), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

template <typename T>
static CK_PLUGIN_TYPE CKDataReaderPluginType();

template <>
CK_PLUGIN_TYPE CKDataReaderPluginType<CKBitmapReader>() {
    return CKPLUGIN_BITMAP_READER;
}

template <>
CK_PLUGIN_TYPE CKDataReaderPluginType<CKModelReader>() {
    return CKPLUGIN_MODEL_READER;
}

template <>
CK_PLUGIN_TYPE CKDataReaderPluginType<CKSoundReader>() {
    return CKPLUGIN_SOUND_READER;
}

template <>
CK_PLUGIN_TYPE CKDataReaderPluginType<CKMovieReader>() {
    return CKPLUGIN_MOVIE_READER;
}

template <typename T>
static T *CastCKDataReaderTo(CKDataReader *reader) {
    if (!reader) {
        return nullptr;
    }

    CKPluginInfo *info = reader->GetReaderInfo();
    if (!info || info->m_Type != CKDataReaderPluginType<T>()) {
        return nullptr;
    }

    return static_cast<T *>(reader);
}

template <typename T>
static const T *CastConstCKDataReaderTo(const CKDataReader *reader) {
    return CastCKDataReaderTo<T>(const_cast<CKDataReader *>(reader));
}

template <typename T>
static CKDataReader *CastCKReaderToData(T *reader) {
    return reader;
}

template <typename T>
static const CKDataReader *CastConstCKReaderToData(const T *reader) {
    return reader;
}

template <typename T>
static void RegisterCKDataReaderCast(asIScriptEngine *engine, const char *derived) {
    int r = 0;

    std::string decl = derived;
    decl.append("@ opCast()");
    r = engine->RegisterObjectMethod("CKDataReader", decl.c_str(), asFUNCTIONPR((CastCKDataReaderTo<T>), (CKDataReader *), T *), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl = "CKDataReader@ opImplCast()";
    r = engine->RegisterObjectMethod(derived, decl.c_str(), asFUNCTIONPR((CastCKReaderToData<T>), (T *), CKDataReader *), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl = "const ";
    decl.append(derived).append("@ opCast() const");
    r = engine->RegisterObjectMethod("CKDataReader", decl.c_str(), asFUNCTIONPR((CastConstCKDataReaderTo<T>), (const CKDataReader *), const T *), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl = "const CKDataReader@ opImplCast() const";
    r = engine->RegisterObjectMethod(derived, decl.c_str(), asFUNCTIONPR((CastConstCKReaderToData<T>), (const T *), const CKDataReader *), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
}

static int SaveCKBitmapReaderMemory(CKBitmapReader *self, NativePointer *memory, CKBitmapProperties *bp) {
    if (!self || !memory) {
        return CKERR_INVALIDPARAMETER;
    }

    void *rawMemory = nullptr;
    const int result = self->SaveMemory(&rawMemory, bp);
    *memory = NativePointer(rawMemory);
    return result;
}

static CKERROR GetCKSoundReaderDataBuffer(CKSoundReader *self, NativePointer *buf, int *size) {
    if (!buf || !size) {
        return CKERR_INVALIDPARAMETER;
    }

    *buf = NativePointer();
    *size = 0;
    if (!self) {
        return CKERR_INVALIDPARAMETER;
    }

    CKBYTE *rawBuffer = nullptr;
    int rawSize = 0;
    const CKERROR result = self->GetDataBuffer(&rawBuffer, &rawSize);
    *buf = NativePointer(rawBuffer);
    *size = rawSize;
    return result;
}

static CKERROR ReadCKSoundReaderMemory(CKSoundReader *self, NativePointer memory, int size) {
    if (!self || size < 0 || (size > 0 && memory.IsNull())) {
        return CKERR_INVALIDPARAMETER;
    }

    return self->ReadMemory(memory.Get(), size);
}

static CKERROR OpenCKMovieReaderFile(CKMovieReader *self, const std::string &name) {
    if (!self) {
        return CKERR_INVALIDPARAMETER;
    }

    std::string mutableName = name;
    mutableName.push_back('\0');
    return self->OpenFile(&mutableName[0]);
}

static CKERROR OpenCKMovieReaderMemory(CKMovieReader *self, const std::string &name) {
    if (!self) {
        return CKERR_INVALIDPARAMETER;
    }

    std::string mutableName = name;
    mutableName.push_back('\0');
    return self->OpenMemory(&mutableName[0]);
}

static CKERROR OpenCKMovieReaderAsynchronousFile(CKMovieReader *self, const std::string &name) {
    if (!self) {
        return CKERR_INVALIDPARAMETER;
    }

    std::string mutableName = name;
    mutableName.push_back('\0');
    return self->OpenAsynchronousFile(&mutableName[0]);
}

static CKERROR ReadCKMovieReaderFrame(CKMovieReader *self, int frame, CKMovieProperties **prop) {
    if (prop) {
        *prop = nullptr;
    }
    if (!self || !prop || frame < 0) {
        return CKERR_INVALIDPARAMETER;
    }

    return self->ReadFrame(frame, prop);
}

static CKERROR LoadCKModelReader(CKModelReader *self,
                                 CKContext *context,
                                 const std::string &filename,
                                 CKObjectArray *objArray,
                                 CKDWORD loadFlags,
                                 CKCharacter *carac) {
    if (!self || !context || !objArray) {
        return CKERR_INVALIDPARAMETER;
    }

    std::string mutableFilename = filename;
    mutableFilename.push_back('\0');
    return self->Load(context, &mutableFilename[0], objArray, loadFlags, carac);
}

static CKERROR SaveCKModelReader(CKModelReader *self,
                                 CKContext *context,
                                 const std::string &filename,
                                 CKObjectArray *objArray,
                                 CKDWORD saveFlags) {
    if (!self || !context || !objArray) {
        return CKERR_INVALIDPARAMETER;
    }

    std::string mutableFilename = filename;
    mutableFilename.push_back('\0');
    return self->Save(context, &mutableFilename[0], objArray, saveFlags);
}

void RegisterCKDataReader(asIScriptEngine *engine) {
    RegisterCKDataReaderMembers<CKDataReader>(engine, "CKDataReader");
}

// CKModelReader

void RegisterCKModelReader(asIScriptEngine *engine) {
    int r = 0;

    RegisterCKDataReaderMembers<CKModelReader>(engine, "CKModelReader");
    RegisterCKDataReaderCast<CKModelReader>(engine, "CKModelReader");

    r = engine->RegisterObjectMethod("CKModelReader", "CKERROR Load(CKContext@ context, const string &in filename, CKObjectArray@ objArray, CKDWORD loadFlags, CKCharacter@ carac = null)", asFUNCTION(LoadCKModelReader), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKModelReader", "CKERROR Save(CKContext@ context, const string &in filename, CKObjectArray@ objArray, CKDWORD saveFlags)", asFUNCTION(SaveCKModelReader), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKBitmapReader

void RegisterCKBitmapReader(asIScriptEngine *engine) {
    int r = 0;

    RegisterCKDataReaderMembers<CKBitmapReader>(engine, "CKBitmapReader");
    RegisterCKDataReaderCast<CKBitmapReader>(engine, "CKBitmapReader");

    r = engine->RegisterObjectMethod("CKBitmapReader", "bool IsAlphaSaved(CKBitmapProperties@ bp)", asFUNCTIONPR([](CKBitmapReader *self, CKBitmapProperties *bp) -> bool { return self->IsAlphaSaved(bp); }, (CKBitmapReader *, CKBitmapProperties *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapReader", "void GetBitmapDefaultProperties(CKBitmapProperties@ &out bp)", asMETHODPR(CKBitmapReader, GetBitmapDefaultProperties, (CKBitmapProperties**), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapReader", "void SetBitmapDefaultProperties(CKBitmapProperties@ bp)", asMETHODPR(CKBitmapReader, SetBitmapDefaultProperties, (CKBitmapProperties*), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapReader", "int ReadFile(const string &in name, CKBitmapProperties@ &out bp)", asFUNCTIONPR([](CKBitmapReader *self, const std::string &name, CKBitmapProperties **bp) -> int { return self->ReadFile(const_cast<char*>(name.c_str()), bp); }, (CKBitmapReader *, const std::string &, CKBitmapProperties **), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapReader", "int ReadMemory(NativePointer memory, int size, CKBitmapProperties@ &out bp)", asFUNCTIONPR([](CKBitmapReader* self, NativePointer memory, int size, CKBitmapProperties **bp) { return self->ReadMemory(memory.Get(), size, bp); }, (CKBitmapReader *, NativePointer, int, CKBitmapProperties **), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapReader", "int ReadASynchronousFile(const string &in name, CKBitmapProperties@ &out bp)", asFUNCTIONPR([](CKBitmapReader *self, const std::string &name, CKBitmapProperties **bp) -> int { return self->ReadASynchronousFile(const_cast<char*>(name.c_str()), bp); }, (CKBitmapReader *, const std::string &, CKBitmapProperties **), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapReader", "int SaveFile(const string &in name, CKBitmapProperties@ bp)", asFUNCTIONPR([](CKBitmapReader *self, const std::string &name, CKBitmapProperties *bp) -> int { return self->SaveFile(const_cast<char*>(name.c_str()), bp); }, (CKBitmapReader *, const std::string &, CKBitmapProperties *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapReader", "int SaveMemory(NativePointer &out memory, CKBitmapProperties@ bp)", asFUNCTION(SaveCKBitmapReaderMemory), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapReader", "void ReleaseMemory(NativePointer memory)", asFUNCTIONPR([](CKBitmapReader *self, NativePointer memory) { self->ReleaseMemory(memory.Get()); }, (CKBitmapReader *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKSoundReader

void RegisterCKSoundReader(asIScriptEngine *engine) {
    int r = 0;

    RegisterCKDataReaderMembers<CKSoundReader>(engine, "CKSoundReader");
    RegisterCKDataReaderCast<CKSoundReader>(engine, "CKSoundReader");

    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR OpenFile(const string &in filename)", asFUNCTIONPR([](CKSoundReader *self, const std::string &filename) -> CKERROR { return self->OpenFile(const_cast<char *>(filename.c_str())); }, (CKSoundReader *, const std::string &), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR Decode()", asMETHODPR(CKSoundReader, Decode, (), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR GetDataBuffer(NativePointer &out buf, int &out size)", asFUNCTION(GetCKSoundReaderDataBuffer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR GetWaveFormat(CKWaveFormat &out wfe)", asMETHODPR(CKSoundReader, GetWaveFormat, (CKWaveFormat *), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "int GetDataSize()", asMETHODPR(CKSoundReader, GetDataSize, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "int GetDuration()", asMETHODPR(CKSoundReader, GetDuration, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR Play()", asMETHODPR(CKSoundReader, Play, (), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR Stop()", asMETHODPR(CKSoundReader, Stop, (), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR Pause()", asMETHODPR(CKSoundReader, Pause, (), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR Resume()", asMETHODPR(CKSoundReader, Resume, (), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR Seek(int pos)", asMETHODPR(CKSoundReader, Seek, (int), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSoundReader", "CKERROR ReadMemory(NativePointer memory, int size)", asFUNCTION(ReadCKSoundReaderMemory), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKMovieReader

void RegisterCKMovieReader(asIScriptEngine *engine) {
    int r = 0;

    RegisterCKDataReaderMembers<CKMovieReader>(engine, "CKMovieReader");
    RegisterCKDataReaderCast<CKMovieReader>(engine, "CKMovieReader");

    r = engine->RegisterObjectMethod("CKMovieReader", "int GetMovieFrameCount()", asMETHODPR(CKMovieReader, GetMovieFrameCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMovieReader", "int GetMovieLength()", asMETHODPR(CKMovieReader, GetMovieLength, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMovieReader", "CKERROR OpenFile(const string &in name)", asFUNCTION(OpenCKMovieReaderFile), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMovieReader", "CKERROR OpenMemory(const string &in name)", asFUNCTION(OpenCKMovieReaderMemory), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMovieReader", "CKERROR OpenAsynchronousFile(const string &in name)", asFUNCTION(OpenCKMovieReaderAsynchronousFile), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMovieReader", "CKERROR ReadFrame(int f, CKMovieProperties@ &out prop)", asFUNCTION(ReadCKMovieReaderFrame), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKPluginDll

void RegisterCKPluginDll(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKPluginDll", "XString m_DllFileName", asOFFSET(CKPluginDll, m_DllFileName)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginDll", "INSTANCE_HANDLE m_DllInstance", asOFFSET(CKPluginDll, m_DllInstance)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginDll", "int m_PluginInfoCount", asOFFSET(CKPluginDll, m_PluginInfoCount)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginDll", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKPluginDll *self) { new(self) CKPluginDll(); }, (CKPluginDll*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPluginDll", asBEHAVE_CONSTRUCT, "void f(const CKPluginDll &in other)", asFUNCTIONPR([](const CKPluginDll &dll, CKPluginDll *self) { new(self) CKPluginDll(dll); }, (const CKPluginDll &, CKPluginDll *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginDll", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPluginDll *self) { self->~CKPluginDll(); }, (CKPluginDll *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginDll", "CKPluginDll &opAssign(const CKPluginDll &in other)", asMETHODPR(CKPluginDll, operator=, (const CKPluginDll &), CKPluginDll &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginDll", "NativePointer GetFunctionPtr(const string &in functionName) const", asFUNCTIONPR([](CKPluginDll *self, const std::string &functionName) { return NativePointer(self->GetFunctionPtr(const_cast<char*>(functionName.c_str()))); }, (CKPluginDll *, const std::string &), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKPluginEntryReadersData

void RegisterCKPluginEntryReadersData(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKPluginEntryReadersData", "CKGUID m_SettingsParameterGuid", asOFFSET(CKPluginEntryReadersData, m_SettingsParameterGuid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginEntryReadersData", "int m_OptionCount", asOFFSET(CKPluginEntryReadersData, m_OptionCount)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginEntryReadersData", "CK_DATAREADER_FLAGS m_ReaderFlags", asOFFSET(CKPluginEntryReadersData, m_ReaderFlags)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKPluginEntryReadersData", "uintptr_t m_GetReaderFct", asOFFSET(CKPluginEntryReadersData, m_GetReaderFct)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginEntryReadersData", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKPluginEntryReadersData *self) { new(self) CKPluginEntryReadersData(); }, (CKPluginEntryReadersData*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPluginEntryReadersData", asBEHAVE_CONSTRUCT, "void f(const CKPluginEntryReadersData &in other)", asFUNCTIONPR([](const CKPluginEntryReadersData &data, CKPluginEntryReadersData *self) { new(self) CKPluginEntryReadersData(data); }, (const CKPluginEntryReadersData &, CKPluginEntryReadersData *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginEntryReadersData", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPluginEntryReadersData *self) { self->~CKPluginEntryReadersData(); }, (CKPluginEntryReadersData *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginEntryReadersData", "CKPluginEntryReadersData &opAssign(const CKPluginEntryReadersData &in other)", asMETHODPR(CKPluginEntryReadersData, operator=, (const CKPluginEntryReadersData &), CKPluginEntryReadersData &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginEntryReadersData", "NativePointer get_m_GetReaderFct() const", asFUNCTIONPR([](const CKPluginEntryReadersData *self) { return NativePointer(self->m_GetReaderFct); }, (const CKPluginEntryReadersData *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPluginEntryReadersData", "void set_m_GetReaderFct(NativePointer ptr)", asFUNCTIONPR([](CKPluginEntryReadersData *self, NativePointer ptr) {
        if (RejectNonNullPluginCallbackPointer(ptr, "CKPluginEntryReadersData.m_GetReaderFct only accepts a null NativePointer from script.")) {
            return;
        }
        self->m_GetReaderFct = nullptr;
    }, (CKPluginEntryReadersData *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

}

// CKPluginEntryBehaviorsData

void RegisterCKPluginEntryBehaviorsData(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKPluginEntryBehaviorsData", "XGUIDArray m_BehaviorsGUID", asOFFSET(CKPluginEntryBehaviorsData, m_BehaviorsGUID)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginEntryBehaviorsData", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKPluginEntryBehaviorsData *self) { new(self) CKPluginEntryBehaviorsData(); }, (CKPluginEntryBehaviorsData*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPluginEntryBehaviorsData", asBEHAVE_CONSTRUCT, "void f(const CKPluginEntryBehaviorsData &in other)", asFUNCTIONPR([](const CKPluginEntryBehaviorsData &data, CKPluginEntryBehaviorsData *self) { new(self) CKPluginEntryBehaviorsData(data); }, (const CKPluginEntryBehaviorsData &, CKPluginEntryBehaviorsData *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginEntryBehaviorsData", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPluginEntryBehaviorsData *self) { self->~CKPluginEntryBehaviorsData(); }, (CKPluginEntryBehaviorsData *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginEntryBehaviorsData", "CKPluginEntryBehaviorsData &opAssign(const CKPluginEntryBehaviorsData &in other)", asMETHODPR(CKPluginEntryBehaviorsData, operator=, (const CKPluginEntryBehaviorsData &), CKPluginEntryBehaviorsData &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKPluginEntry

static bool HasCKPluginEntryReadersInfo(const CKPluginEntry *self) {
    return self && self->m_ReadersInfo;
}

static bool HasCKPluginEntryBehaviorsInfo(const CKPluginEntry *self) {
    return self && self->m_BehaviorsInfo;
}

static const CKPluginEntryReadersData &GetCKPluginEntryReadersInfo(const CKPluginEntry *self) {
    static thread_local CKPluginEntryReadersData dummy;
    if (self && self->m_ReadersInfo) {
        return *self->m_ReadersInfo;
    }
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException("CKPluginEntry.m_ReadersInfo is null for this plugin entry.");
    }
    return dummy;
}

static const CKPluginEntryBehaviorsData &GetCKPluginEntryBehaviorsInfo(const CKPluginEntry *self) {
    static thread_local CKPluginEntryBehaviorsData dummy;
    dummy.m_BehaviorsGUID.Clear();
    if (self && self->m_BehaviorsInfo) {
        return *self->m_BehaviorsInfo;
    }
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException("CKPluginEntry.m_BehaviorsInfo is null for this plugin entry.");
    }
    return dummy;
}

void RegisterCKPluginEntry(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKPluginEntry", "int m_PluginDllIndex", asOFFSET(CKPluginEntry, m_PluginDllIndex)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginEntry", "int m_PositionInDll", asOFFSET(CKPluginEntry, m_PositionInDll)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginEntry", "CKPluginInfo m_PluginInfo", asOFFSET(CKPluginEntry, m_PluginInfo)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginEntry", "int m_Active", asOFFSET(CKPluginEntry, m_Active)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginEntry", "int m_IndexInCategory", asOFFSET(CKPluginEntry, m_IndexInCategory)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPluginEntry", "int m_NeededByFile", asOFFSET(CKPluginEntry, m_NeededByFile)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginEntry", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKPluginEntry *self) { new(self) CKPluginEntry(); }, (CKPluginEntry*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPluginEntry", asBEHAVE_CONSTRUCT, "void f(const CKPluginEntry &in other)", asFUNCTIONPR([](const CKPluginEntry &entry, CKPluginEntry *self) { new(self) CKPluginEntry(entry); }, (const CKPluginEntry &, CKPluginEntry *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginEntry", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPluginEntry *self) { self->~CKPluginEntry(); }, (CKPluginEntry *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginEntry", "CKPluginEntry &opAssign(const CKPluginEntry &in other)", asMETHODPR(CKPluginEntry, operator=, (const CKPluginEntry &), CKPluginEntry &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginEntry", "bool HasReadersInfo() const", asFUNCTION(HasCKPluginEntryReadersInfo), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPluginEntry", "bool HasBehaviorsInfo() const", asFUNCTION(HasCKPluginEntryBehaviorsInfo), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPluginEntry", "const CKPluginEntryReadersData &get_m_ReadersInfo() const", asFUNCTION(GetCKPluginEntryReadersInfo), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPluginEntry", "const CKPluginEntryBehaviorsData &get_m_BehaviorsInfo() const", asFUNCTION(GetCKPluginEntryBehaviorsInfo), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKPluginCategory

void RegisterCKPluginCategory(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKPluginCategory", "XString m_Name", asOFFSET(CKPluginCategory, m_Name)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKPluginCategory", "XArray<CKPluginEntry *> m_Entries", asOFFSET(CKPluginCategory, m_Entries)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginCategory", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKPluginCategory *self) { new(self) CKPluginCategory(); }, (CKPluginCategory*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPluginCategory", asBEHAVE_CONSTRUCT, "void f(const CKPluginCategory &in other)", asFUNCTIONPR([](const CKPluginCategory &cate, CKPluginCategory *self) { new(self) CKPluginCategory(cate); }, (const CKPluginCategory &, CKPluginCategory *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPluginCategory", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPluginCategory *self) { self->~CKPluginCategory(); }, (CKPluginCategory *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPluginCategory", "CKPluginCategory &opAssign(const CKPluginCategory &in other)", asMETHODPR(CKPluginCategory, operator=, (const CKPluginCategory &), CKPluginCategory &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKOperationDesc

static void SetCKOperationDescFunction(CKOperationDesc *self, NativePointer ptr) {
    if (RejectNativeCallbackPointerWrite(ptr, "CKOperationDesc.Fct only accepts a null NativePointer from script.")) {
        return;
    }
    self->Fct = nullptr;
}

void RegisterCKOperationDesc(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKOperationDesc", "CKGUID OpGuid", asOFFSET(CKOperationDesc, OpGuid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKOperationDesc", "CKGUID P1Guid", asOFFSET(CKOperationDesc, P1Guid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKOperationDesc", "CKGUID P2Guid", asOFFSET(CKOperationDesc, P2Guid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKOperationDesc", "CKGUID ResGuid", asOFFSET(CKOperationDesc, ResGuid)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKOperationDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKOperationDesc *self) { new(self) CKOperationDesc(); }, (CKOperationDesc*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKOperationDesc", asBEHAVE_CONSTRUCT, "void f(const CKOperationDesc &in other)", asFUNCTIONPR([](const CKOperationDesc &desc, CKOperationDesc *self) { new(self) CKOperationDesc(desc); }, (const CKOperationDesc &, CKOperationDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKOperationDesc", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKOperationDesc *self) { self->~CKOperationDesc(); }, (CKOperationDesc *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKOperationDesc", "CKOperationDesc &opAssign(const CKOperationDesc &in other)", asMETHODPR(CKOperationDesc, operator=, (const CKOperationDesc &), CKOperationDesc &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKOperationDesc", "NativePointer get_Fct() const", asFUNCTIONPR([](const CKOperationDesc *self) { return NativePointer(self->Fct); }, (const CKOperationDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKOperationDesc", "void set_Fct(NativePointer ptr)", asFUNCTION(SetCKOperationDescFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKAttributeVal

void RegisterCKAttributeVal(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKAttributeVal", "CKAttributeType AttribType", asOFFSET(CKAttributeVal, AttribType)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKAttributeVal", "CK_ID Parameter", asOFFSET(CKAttributeVal, Parameter)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKAttributeVal", asBEHAVE_CONSTRUCT, "void f(const CKAttributeVal &in other)", asFUNCTIONPR([](const CKAttributeVal &val, CKAttributeVal *self) { new(self) CKAttributeVal(val); }, (const CKAttributeVal &, CKAttributeVal *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeVal", "CKAttributeVal &opAssign(const CKAttributeVal &in other)", asMETHODPR(CKAttributeVal, operator=, (const CKAttributeVal &), CKAttributeVal &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKAttributeDesc

#if CKVERSION == 0x13022002
static bool RejectCKAttributeDescRawPointerWrite(const NativePointer &ptr, const char *message) {
    if (ptr.IsNull()) {
        return false;
    }
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
    return true;
}

static void SetCKAttributeDescCallbackFct(CKAttributeDesc *self, NativePointer ptr) {
    if (RejectCKAttributeDescRawPointerWrite(ptr, "CKAttributeDesc.CallbackFct only accepts a null NativePointer from script.")) {
        return;
    }
    self->CallbackFct = nullptr;
}

static void SetCKAttributeDescCallbackArg(CKAttributeDesc *self, NativePointer ptr) {
    if (RejectCKAttributeDescRawPointerWrite(ptr, "CKAttributeDesc.CallbackArg only accepts a null NativePointer from script.")) {
        return;
    }
    self->CallbackArg = nullptr;
}

static void SetCKAttributeDescDefaultValuePointer(CKAttributeDesc *self, NativePointer ptr) {
    if (RejectCKAttributeDescRawPointerWrite(ptr, "CKAttributeDesc.DefaultValuePointer only accepts a null NativePointer from script.")) {
        return;
    }
    self->DefaultValue = nullptr;
}

static void SetCKAttributeDescCreatorDll(CKAttributeDesc *self, NativePointer ptr) {
    if (RejectCKAttributeDescRawPointerWrite(ptr, "CKAttributeDesc.CreatorDll only accepts a null NativePointer from script.")) {
        return;
    }
    self->CreatorDll = nullptr;
}

void RegisterCKAttributeDesc(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKAttributeDesc", "CKGUID ParameterType", asOFFSET(CKAttributeDesc, ParameterType)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKAttributeDesc", "XObjectPointerArray GlobalAttributeList", asOFFSET(CKAttributeDesc, GlobalAttributeList)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKAttributeDesc", "XObjectPointerArray AttributeList", asOFFSET(CKAttributeDesc, AttributeList)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKAttributeDesc", "CKAttributeCategory AttributeCategory", asOFFSET(CKAttributeDesc, AttributeCategory)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKAttributeDesc", "CK_CLASSID CompatibleCid", asOFFSET(CKAttributeDesc, CompatibleCid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKAttributeDesc", "CKDWORD Flags", asOFFSET(CKAttributeDesc, Flags)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKAttributeDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKAttributeDesc *self) { new(self) CKAttributeDesc(); }, (CKAttributeDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKAttributeDesc", asBEHAVE_CONSTRUCT, "void f(const CKAttributeDesc &in other)", asFUNCTIONPR([](const CKAttributeDesc &desc, CKAttributeDesc *self) { new(self) CKAttributeDesc(desc); }, (const CKAttributeDesc &, CKAttributeDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKAttributeDesc", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKAttributeDesc *self) { self->~CKAttributeDesc(); }, (CKAttributeDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeDesc", "CKAttributeDesc &opAssign(const CKAttributeDesc &in other)", asFUNCTIONPR([](CKAttributeDesc *self, const CKAttributeDesc &other) -> CKAttributeDesc & { *self = other; return *self; }, (CKAttributeDesc *, const CKAttributeDesc &), CKAttributeDesc &), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKAttributeDesc", "string get_Name() const", asFUNCTIONPR([](const CKAttributeDesc *self) -> std::string { return ScriptStringify(self->Name); }, (const CKAttributeDesc *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeDesc", "void set_Name(const string &in value)", asFUNCTIONPR([](CKAttributeDesc *self, const std::string &value) {
        std::strncpy(self->Name, value.c_str(), sizeof(self->Name) - 1);
        self->Name[sizeof(self->Name) - 1] = '\0';
    }, (CKAttributeDesc *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKAttributeDesc", "NativePointer get_CallbackFct() const", asFUNCTIONPR([](const CKAttributeDesc *self) { return NativePointer(self->CallbackFct); }, (const CKAttributeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeDesc", "void set_CallbackFct(NativePointer ptr)", asFUNCTION(SetCKAttributeDescCallbackFct), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKAttributeDesc", "NativePointer get_CallbackArg() const", asFUNCTIONPR([](const CKAttributeDesc *self) { return NativePointer(self->CallbackArg); }, (const CKAttributeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeDesc", "void set_CallbackArg(NativePointer ptr)", asFUNCTION(SetCKAttributeDescCallbackArg), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKAttributeDesc", "string get_DefaultValue() const", asFUNCTIONPR([](const CKAttributeDesc *self) -> std::string { return ScriptStringify(self->DefaultValue); }, (const CKAttributeDesc *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeDesc", "NativePointer get_DefaultValuePointer() const", asFUNCTIONPR([](const CKAttributeDesc *self) { return NativePointer(self->DefaultValue); }, (const CKAttributeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeDesc", "void set_DefaultValuePointer(NativePointer ptr)", asFUNCTION(SetCKAttributeDescDefaultValuePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKAttributeDesc", "NativePointer get_CreatorDll() const", asFUNCTIONPR([](const CKAttributeDesc *self) { return NativePointer(self->CreatorDll); }, (const CKAttributeDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeDesc", "void set_CreatorDll(NativePointer ptr)", asFUNCTION(SetCKAttributeDescCreatorDll), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKAttributeCategoryDesc

static std::mutex g_CKAttributeCategoryDescOwnedMutex;
static std::unordered_set<CKAttributeCategoryDesc *> g_CKAttributeCategoryDescOwnedValues;

static void TrackCKAttributeCategoryDescValue(CKAttributeCategoryDesc *self) {
    std::lock_guard<std::mutex> lock(g_CKAttributeCategoryDescOwnedMutex);
    g_CKAttributeCategoryDescOwnedValues.insert(self);
}

static bool UntrackCKAttributeCategoryDescValue(CKAttributeCategoryDesc *self) {
    std::lock_guard<std::mutex> lock(g_CKAttributeCategoryDescOwnedMutex);
    return g_CKAttributeCategoryDescOwnedValues.erase(self) > 0;
}

static bool IsTrackedCKAttributeCategoryDescValue(CKAttributeCategoryDesc *self) {
    std::lock_guard<std::mutex> lock(g_CKAttributeCategoryDescOwnedMutex);
    return g_CKAttributeCategoryDescOwnedValues.find(self) != g_CKAttributeCategoryDescOwnedValues.end();
}

static void ReleaseCKAttributeCategoryDescStorage(CKAttributeCategoryDesc *self) {
    if (!self) {
        return;
    }

    CKDeletePointer(self->Name);
    self->Name = nullptr;
}

static void CopyCKAttributeCategoryDescStorage(CKAttributeCategoryDesc *self, const CKAttributeCategoryDesc &other) {
    self->Name = other.Name ? CKStrdup(other.Name) : nullptr;
    self->Flags = other.Flags;
}

static void ConstructCKAttributeCategoryDesc(CKAttributeCategoryDesc *self) {
    self->Name = nullptr;
    self->Flags = 0;
    TrackCKAttributeCategoryDescValue(self);
}

static void ConstructCKAttributeCategoryDescCopy(const CKAttributeCategoryDesc &other, CKAttributeCategoryDesc *self) {
    CopyCKAttributeCategoryDescStorage(self, other);
    TrackCKAttributeCategoryDescValue(self);
}

static void DestructCKAttributeCategoryDesc(CKAttributeCategoryDesc *self) {
    if (UntrackCKAttributeCategoryDescValue(self)) {
        ReleaseCKAttributeCategoryDescStorage(self);
    }
}

static CKAttributeCategoryDesc &AssignCKAttributeCategoryDesc(CKAttributeCategoryDesc *self, const CKAttributeCategoryDesc &other) {
    if (self == &other) {
        return *self;
    }

    if (!IsTrackedCKAttributeCategoryDescValue(self)) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("Cannot assign to a borrowed CKAttributeCategoryDesc descriptor.");
        }
        return *self;
    }

    ReleaseCKAttributeCategoryDescStorage(self);
    CopyCKAttributeCategoryDescStorage(self, other);
    return *self;
}

static void SetCKAttributeCategoryDescName(CKAttributeCategoryDesc *self, const std::string &value) {
    if (!IsTrackedCKAttributeCategoryDescValue(self)) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("Cannot assign to a borrowed CKAttributeCategoryDesc descriptor.");
        }
        return;
    }

    ReleaseCKAttributeCategoryDescStorage(self);
    self->Name = value.empty() ? nullptr : CKStrdup(const_cast<CKSTRING>(value.c_str()));
}

static void SetCKAttributeCategoryDescNamePointer(CKAttributeCategoryDesc *self, NativePointer ptr) {
    if (!IsTrackedCKAttributeCategoryDescValue(self)) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("Cannot assign to a borrowed CKAttributeCategoryDesc descriptor.");
        }
        return;
    }
    if (RejectCKAttributeDescRawPointerWrite(ptr, "CKAttributeCategoryDesc.NamePointer only accepts a null NativePointer from script.")) {
        return;
    }

    ReleaseCKAttributeCategoryDescStorage(self);
}

void RegisterCKAttributeCategoryDesc(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKAttributeCategoryDesc", "CKDWORD Flags", asOFFSET(CKAttributeCategoryDesc, Flags)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKAttributeCategoryDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCKAttributeCategoryDesc), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKAttributeCategoryDesc", asBEHAVE_CONSTRUCT, "void f(const CKAttributeCategoryDesc &in other)", asFUNCTION(ConstructCKAttributeCategoryDescCopy), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKAttributeCategoryDesc", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructCKAttributeCategoryDesc), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeCategoryDesc", "CKAttributeCategoryDesc &opAssign(const CKAttributeCategoryDesc &in other)", asFUNCTION(AssignCKAttributeCategoryDesc), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKAttributeCategoryDesc", "string get_Name() const", asFUNCTIONPR([](const CKAttributeCategoryDesc *self) -> std::string { return ScriptStringify(self->Name); }, (const CKAttributeCategoryDesc *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeCategoryDesc", "void set_Name(const string &in value)", asFUNCTION(SetCKAttributeCategoryDescName), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeCategoryDesc", "NativePointer get_NamePointer() const", asFUNCTIONPR([](const CKAttributeCategoryDesc *self) { return NativePointer(self->Name); }, (const CKAttributeCategoryDesc *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKAttributeCategoryDesc", "void set_NamePointer(NativePointer ptr)", asFUNCTION(SetCKAttributeCategoryDescNamePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}
#endif

// CKTimeProfiler

namespace {

constexpr size_t kCKTimeProfilerMaxMarkNameLength = 48;
constexpr size_t kCKTimeProfilerMaxDumpBuffer = 64 * 1024;

struct CKTimeProfilerScriptState {
    CKSTRING Title = nullptr;
    std::vector<CKSTRING> Marks;
    bool Constructed = false;
};

std::mutex g_CKTimeProfilerMutex;
std::unordered_map<CKTimeProfiler *, CKTimeProfilerScriptState> g_CKTimeProfilerStates;

static void SetCKTimeProfilerException(const char *message) {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
}

static CKSTRING DuplicateCKTimeProfilerString(const std::string &value, size_t maxLength = 0) {
    std::string copy = value;
    if (maxLength > 0 && copy.length() > maxLength) {
        copy.resize(maxLength);
    }
    return CKStrdup(const_cast<CKSTRING>(copy.c_str()));
}

static void ReleaseCKTimeProfilerMarkStorage(CKTimeProfilerScriptState &state) {
    for (CKSTRING mark : state.Marks) {
        CKDeletePointer(mark);
    }
    state.Marks.clear();
}

static void ConstructCKTimeProfiler(const std::string &title,
                                    CKContext *context,
                                    int startingCount,
                                    CKTimeProfiler *self) {
    if (!self) {
        return;
    }

    if (!context) {
        SetCKTimeProfilerException("CKTimeProfiler requires a non-null CKContext.");
        std::lock_guard<std::mutex> lock(g_CKTimeProfilerMutex);
        g_CKTimeProfilerStates[self] = CKTimeProfilerScriptState{};
        return;
    }

    const int safeStartingCount = std::max(0, startingCount);
    CKSTRING titleCopy = DuplicateCKTimeProfilerString(title);
    new (self) CKTimeProfiler(titleCopy ? titleCopy : const_cast<CKSTRING>(""), context, safeStartingCount);

    CKTimeProfilerScriptState state;
    state.Title = titleCopy;
    state.Constructed = true;

    std::lock_guard<std::mutex> lock(g_CKTimeProfilerMutex);
    auto it = g_CKTimeProfilerStates.find(self);
    if (it != g_CKTimeProfilerStates.end()) {
        ReleaseCKTimeProfilerMarkStorage(it->second);
        CKDeletePointer(it->second.Title);
        g_CKTimeProfilerStates.erase(it);
    }
    g_CKTimeProfilerStates.emplace(self, std::move(state));
}

static void DestructCKTimeProfiler(CKTimeProfiler *self) {
    if (!self) {
        return;
    }

    CKTimeProfilerScriptState state;
    {
        std::lock_guard<std::mutex> lock(g_CKTimeProfilerMutex);
        auto it = g_CKTimeProfilerStates.find(self);
        if (it == g_CKTimeProfilerStates.end()) {
            return;
        }
        state = std::move(it->second);
        g_CKTimeProfilerStates.erase(it);
    }

    if (state.Constructed) {
        // CKTimeProfiler::~CKTimeProfiler dumps into a fixed 512-byte buffer. Clear
        // script-owned marks first so destruction cannot overflow that SDK buffer.
        self->Reset();
        self->~CKTimeProfiler();
    }

    ReleaseCKTimeProfilerMarkStorage(state);
    CKDeletePointer(state.Title);
}

static bool IsCKTimeProfilerConstructed(CKTimeProfiler *self) {
    std::lock_guard<std::mutex> lock(g_CKTimeProfilerMutex);
    auto it = g_CKTimeProfilerStates.find(self);
    return it != g_CKTimeProfilerStates.end() && it->second.Constructed;
}

static size_t GetCKTimeProfilerMarkCount(CKTimeProfiler *self) {
    std::lock_guard<std::mutex> lock(g_CKTimeProfilerMutex);
    auto it = g_CKTimeProfilerStates.find(self);
    return it != g_CKTimeProfilerStates.end() ? it->second.Marks.size() : 0;
}

static void ResetCKTimeProfiler(CKTimeProfiler *self) {
    if (!IsCKTimeProfilerConstructed(self)) {
        SetCKTimeProfilerException("CKTimeProfiler is not constructed.");
        return;
    }

    self->Reset();

    std::lock_guard<std::mutex> lock(g_CKTimeProfilerMutex);
    auto it = g_CKTimeProfilerStates.find(self);
    if (it != g_CKTimeProfilerStates.end()) {
        ReleaseCKTimeProfilerMarkStorage(it->second);
    }
}

static void MarkCKTimeProfiler(CKTimeProfiler *self, const std::string &name) {
    if (!IsCKTimeProfilerConstructed(self)) {
        SetCKTimeProfilerException("CKTimeProfiler is not constructed.");
        return;
    }

    CKSTRING markName = DuplicateCKTimeProfilerString(name, kCKTimeProfilerMaxMarkNameLength);
    (*self)(markName ? markName : const_cast<CKSTRING>(""));

    std::lock_guard<std::mutex> lock(g_CKTimeProfilerMutex);
    auto it = g_CKTimeProfilerStates.find(self);
    if (it != g_CKTimeProfilerStates.end()) {
        it->second.Marks.push_back(markName);
    } else {
        CKDeletePointer(markName);
    }
}

static void DumpCKTimeProfiler(CKTimeProfiler *self, std::string &buffer, const std::string &separator) {
    buffer.clear();
    if (!IsCKTimeProfilerConstructed(self)) {
        SetCKTimeProfilerException("CKTimeProfiler is not constructed.");
        return;
    }

    const size_t markCount = GetCKTimeProfilerMarkCount(self);
    const size_t required = 64 + markCount * (64 + separator.length());
    if (required > kCKTimeProfilerMaxDumpBuffer) {
        SetCKTimeProfilerException("CKTimeProfiler dump output is too large.");
        return;
    }

    std::vector<char> output(std::max<size_t>(required, 1), '\0');
    std::string separatorCopy = separator;
    self->Dump(output.data(), const_cast<char *>(separatorCopy.c_str()));
    buffer.assign(output.data());
}

} // namespace

void RegisterCKTimeProfiler(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectBehaviour("CKTimeProfiler", asBEHAVE_CONSTRUCT, "void f(const string &in title, CKContext@ context, int startingCount = 4)", asFUNCTION(ConstructCKTimeProfiler), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKTimeProfiler", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructCKTimeProfiler), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKTimeProfiler", "void opCall(const string &in str)", asFUNCTION(MarkCKTimeProfiler), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKTimeProfiler", "void Reset()", asFUNCTION(ResetCKTimeProfiler), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKTimeProfiler", "void Dump(string &out buffer, const string &in separator = \" | \")", asFUNCTION(DumpCKTimeProfiler), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKMessage

void RegisterCKMessage(asIScriptEngine *engine) {
    int r = 0;

    // Not existing in Virtools 2.1
    // r = engine->RegisterObjectBehaviour("CKMessage", asBEHAVE_ADDREF, "void f()", asMETHOD(CKMessage, AddRef), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectBehaviour("CKMessage", asBEHAVE_RELEASE, "void f()", asMETHOD(CKMessage, Release), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMessage", "CKERROR SetBroadcastObjectType(CK_CLASSID type = CKCID_BEOBJECT)", asMETHOD(CKMessage, SetBroadcastObjectType), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMessage", "CKERROR AddParameter(CKParameter@ param, bool deleteParameterWithMessage = false)", asFUNCTIONPR([](CKMessage *self, CKParameter *param, bool deleteParam) { return self->AddParameter(param, deleteParam); }, (CKMessage *, CKParameter *, bool), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMessage", "CKERROR RemoveParameter(CKParameter@ param)", asMETHOD(CKMessage, RemoveParameter), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMessage", "int GetParameterCount()", asMETHOD(CKMessage, GetParameterCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMessage", "CKParameter@ GetParameter(int pos)", asMETHOD(CKMessage, GetParameter), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMessage", "void SetSender(CKBeObject@ obj)", asMETHOD(CKMessage, SetSender), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMessage", "CKBeObject@ GetSender()", asMETHOD(CKMessage, GetSender), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMessage", "void SetRecipient(CKObject@ recipient)", asMETHOD(CKMessage, SetRecipient), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMessage", "CKObject@ GetRecipient()", asMETHOD(CKMessage, GetRecipient), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMessage", "CK_MESSAGE_SENDINGTYPE GetSendingType()", asMETHOD(CKMessage, GetSendingType), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMessage", "void SetSendingType(CK_MESSAGE_SENDINGTYPE type)", asMETHOD(CKMessage, SetSendingType), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMessage", "void SetMsgType(CKMessageType type)", asMETHOD(CKMessage, SetMsgType), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMessage", "CKMessageType GetMsgType()", asMETHOD(CKMessage, GetMsgType), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKWaitingObject

#if CKVERSION == 0x13022002
static bool CKWaitingObjectHasBeObject(const CKWaitingObject &self) {
    return self.m_BeObject != nullptr;
}

static bool CKWaitingObjectHasBehavior(const CKWaitingObject &self) {
    return self.m_Behavior != nullptr;
}

static bool CKWaitingObjectHasOutput(const CKWaitingObject &self) {
    return self.m_Output != nullptr;
}

static CK_ID CKWaitingObjectBeObjectId(const CKWaitingObject &self) {
    return self.m_BeObject ? self.m_BeObject->GetID() : 0;
}

static CK_ID CKWaitingObjectBehaviorId(const CKWaitingObject &self) {
    return self.m_Behavior ? self.m_Behavior->GetID() : 0;
}

static CK_ID CKWaitingObjectOutputId(const CKWaitingObject &self) {
    return self.m_Output ? self.m_Output->GetID() : 0;
}
#endif

void RegisterCKWaitingObject(asIScriptEngine *engine) {
#if CKVERSION == 0x13022002
    int r = 0;

    r = engine->RegisterObjectBehaviour("CKWaitingObject", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKWaitingObject *self) { new(self) CKWaitingObject(); }, (CKWaitingObject*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKWaitingObject", asBEHAVE_CONSTRUCT, "void f(const CKWaitingObject &in other)", asFUNCTIONPR([](const CKWaitingObject &obj, CKWaitingObject *self) { new(self) CKWaitingObject(obj); }, (const CKWaitingObject &, CKWaitingObject *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKWaitingObject", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKWaitingObject *self) { self->~CKWaitingObject(); }, (CKWaitingObject *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKWaitingObject", "CKWaitingObject &opAssign(const CKWaitingObject &in other)", asMETHODPR(CKWaitingObject, operator=, (const CKWaitingObject &), CKWaitingObject &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKWaitingObject", "bool HasBeObject() const", asFUNCTION(CKWaitingObjectHasBeObject), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKWaitingObject", "bool HasBehavior() const", asFUNCTION(CKWaitingObjectHasBehavior), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKWaitingObject", "bool HasOutput() const", asFUNCTION(CKWaitingObjectHasOutput), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKWaitingObject", "CK_ID BeObjectId() const", asFUNCTION(CKWaitingObjectBeObjectId), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKWaitingObject", "CK_ID BehaviorId() const", asFUNCTION(CKWaitingObjectBehaviorId), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKWaitingObject", "CK_ID OutputId() const", asFUNCTION(CKWaitingObjectOutputId), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
}

// CKPATHCATEGORY

void RegisterCKPATHCATEGORY(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKPATHCATEGORY", "XString m_Name", asOFFSET(CKPATHCATEGORY, m_Name)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPATHCATEGORY", "CKPATHENTRYVECTOR m_Entries", asOFFSET(CKPATHCATEGORY, m_Entries)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPATHCATEGORY", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKPATHCATEGORY *self) { new(self) CKPATHCATEGORY(); }, (CKPATHCATEGORY*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPATHCATEGORY", asBEHAVE_CONSTRUCT, "void f(const CKPATHCATEGORY &in other)", asFUNCTIONPR([](const CKPATHCATEGORY &cate, CKPATHCATEGORY *self) { new(self) CKPATHCATEGORY(cate); }, (const CKPATHCATEGORY &, CKPATHCATEGORY *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPATHCATEGORY", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPATHCATEGORY *self) { self->~CKPATHCATEGORY(); }, (CKPATHCATEGORY *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPATHCATEGORY", "CKPATHCATEGORY &opAssign(const CKPATHCATEGORY &in other)", asMETHODPR(CKPATHCATEGORY, operator=, (const CKPATHCATEGORY &), CKPATHCATEGORY &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

void RegisterCKPARAMETER_DESC(asIScriptEngine *engine) {
    int r = 0;

    // r = engine->RegisterObjectProperty("CKPARAMETER_DESC", "uintptr_t Name", asOFFSET(CKPARAMETER_DESC, Name)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPARAMETER_DESC", "CKGUID Guid", asOFFSET(CKPARAMETER_DESC, Guid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPARAMETER_DESC", "int Type", asOFFSET(CKPARAMETER_DESC, Type)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKPARAMETER_DESC", "uintptr_t DefaultValueString", asOFFSET(CKPARAMETER_DESC, DefaultValueString)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKPARAMETER_DESC", "uintptr_t DefaultValue", asOFFSET(CKPARAMETER_DESC, DefaultValue)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPARAMETER_DESC", "int DefaultValueSize", asOFFSET(CKPARAMETER_DESC, DefaultValueSize)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPARAMETER_DESC", "int Owner", asOFFSET(CKPARAMETER_DESC, Owner)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPARAMETER_DESC", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKPARAMETER_DESC *self) { new(self) CKPARAMETER_DESC(); }, (CKPARAMETER_DESC*), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPARAMETER_DESC", asBEHAVE_CONSTRUCT, "void f(const CKPARAMETER_DESC &in other)", asFUNCTIONPR([](const CKPARAMETER_DESC &desc, CKPARAMETER_DESC *self) { new(self) CKPARAMETER_DESC(desc); }, (const CKPARAMETER_DESC &, CKPARAMETER_DESC *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPARAMETER_DESC", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPARAMETER_DESC *self) { self->~CKPARAMETER_DESC(); }, (CKPARAMETER_DESC *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPARAMETER_DESC", "CKPARAMETER_DESC &opAssign(const CKPARAMETER_DESC &in other)", asMETHODPR(CKPARAMETER_DESC, operator=, (const CKPARAMETER_DESC &), CKPARAMETER_DESC &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPARAMETER_DESC", "string get_Name() const", asFUNCTIONPR([](const CKPARAMETER_DESC *self) -> std::string { return ScriptStringify(self->Name); }, (const CKPARAMETER_DESC *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPARAMETER_DESC", "void set_Name(const string &in value)", asFUNCTIONPR([](CKPARAMETER_DESC *self, const std::string &value) { CKDeletePointer(self->Name); self->Name = CKStrdup(const_cast<CKSTRING>(value.c_str())); }, (CKPARAMETER_DESC *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPARAMETER_DESC", "string get_DefaultValueString() const", asFUNCTIONPR([](const CKPARAMETER_DESC *self) -> std::string { return ScriptStringify(self->DefaultValueString); }, (const CKPARAMETER_DESC *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPARAMETER_DESC", "void set_DefaultValueString(const string &in value)", asFUNCTIONPR([](CKPARAMETER_DESC *self, const std::string &value) { CKDeletePointer(self->DefaultValueString); self->DefaultValueString = CKStrdup(const_cast<CKSTRING>(value.c_str())); }, (CKPARAMETER_DESC *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPARAMETER_DESC", "NativePointer get_DefaultValue() const", asFUNCTIONPR([](const CKPARAMETER_DESC *self) { return NativePointer(self->DefaultValue); }, (const CKPARAMETER_DESC *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPARAMETER_DESC", "void set_DefaultValue(NativePointer ptr)", asFUNCTIONPR([](CKPARAMETER_DESC *self, NativePointer ptr) { self->DefaultValue = reinterpret_cast<CKBYTE *>(ptr.Get()); }, (CKPARAMETER_DESC *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

static std::mutex g_CKBehaviorIoDescOwnedMutex;
static std::unordered_set<CKBEHAVIORIO_DESC *> g_CKBehaviorIoDescOwnedValues;

static void TrackCKBehaviorIoDescValue(CKBEHAVIORIO_DESC *self) {
    std::lock_guard<std::mutex> lock(g_CKBehaviorIoDescOwnedMutex);
    g_CKBehaviorIoDescOwnedValues.insert(self);
}

static bool UntrackCKBehaviorIoDescValue(CKBEHAVIORIO_DESC *self) {
    std::lock_guard<std::mutex> lock(g_CKBehaviorIoDescOwnedMutex);
    return g_CKBehaviorIoDescOwnedValues.erase(self) > 0;
}

static bool IsTrackedCKBehaviorIoDescValue(CKBEHAVIORIO_DESC *self) {
    std::lock_guard<std::mutex> lock(g_CKBehaviorIoDescOwnedMutex);
    return g_CKBehaviorIoDescOwnedValues.find(self) != g_CKBehaviorIoDescOwnedValues.end();
}

static void RejectBorrowedCKBehaviorIoDescWrite() {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException("Cannot assign to a borrowed CKBEHAVIORIO_DESC descriptor.");
    }
}

static void ReleaseCKBehaviorIoDescStorage(CKBEHAVIORIO_DESC *self) {
    if (!self) {
        return;
    }

    CKDeletePointer(self->Name);
    self->Name = nullptr;
}

static void CopyCKBehaviorIoDescStorage(CKBEHAVIORIO_DESC *self, const CKBEHAVIORIO_DESC &other) {
    self->Name = other.Name ? CKStrdup(other.Name) : nullptr;
    self->Flags = other.Flags;
}

static void ConstructCKBehaviorIoDesc(CKBEHAVIORIO_DESC *self) {
    self->Name = nullptr;
    self->Flags = 0;
    TrackCKBehaviorIoDescValue(self);
}

static void ConstructCKBehaviorIoDescCopy(const CKBEHAVIORIO_DESC &other, CKBEHAVIORIO_DESC *self) {
    CopyCKBehaviorIoDescStorage(self, other);
    TrackCKBehaviorIoDescValue(self);
}

static void DestructCKBehaviorIoDesc(CKBEHAVIORIO_DESC *self) {
    if (UntrackCKBehaviorIoDescValue(self)) {
        ReleaseCKBehaviorIoDescStorage(self);
    }
}

static CKBEHAVIORIO_DESC &AssignCKBehaviorIoDesc(CKBEHAVIORIO_DESC *self, const CKBEHAVIORIO_DESC &other) {
    if (self == &other) {
        return *self;
    }
    if (!IsTrackedCKBehaviorIoDescValue(self)) {
        RejectBorrowedCKBehaviorIoDescWrite();
        return *self;
    }

    CKSTRING nameCopy = other.Name ? CKStrdup(other.Name) : nullptr;
    ReleaseCKBehaviorIoDescStorage(self);
    self->Name = nameCopy;
    self->Flags = other.Flags;
    return *self;
}

static CKDWORD GetCKBehaviorIoDescFlags(const CKBEHAVIORIO_DESC *self) {
    return self->Flags;
}

static void SetCKBehaviorIoDescFlags(CKBEHAVIORIO_DESC *self, CKDWORD value) {
    if (!IsTrackedCKBehaviorIoDescValue(self)) {
        RejectBorrowedCKBehaviorIoDescWrite();
        return;
    }
    self->Flags = value;
}

static std::string GetCKBehaviorIoDescName(const CKBEHAVIORIO_DESC *self) {
    return ScriptStringify(self->Name);
}

static void SetCKBehaviorIoDescName(CKBEHAVIORIO_DESC *self, const std::string &value) {
    if (!IsTrackedCKBehaviorIoDescValue(self)) {
        RejectBorrowedCKBehaviorIoDescWrite();
        return;
    }

    ReleaseCKBehaviorIoDescStorage(self);
    self->Name = value.empty() ? nullptr : CKStrdup(const_cast<CKSTRING>(value.c_str()));
}

void RegisterCKBEHAVIORIO_DESC(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectBehaviour("CKBEHAVIORIO_DESC", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCKBehaviorIoDesc), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKBEHAVIORIO_DESC", asBEHAVE_CONSTRUCT, "void f(const CKBEHAVIORIO_DESC &in other)", asFUNCTION(ConstructCKBehaviorIoDescCopy), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKBEHAVIORIO_DESC", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructCKBehaviorIoDesc), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBEHAVIORIO_DESC", "CKBEHAVIORIO_DESC &opAssign(const CKBEHAVIORIO_DESC &in other)", asFUNCTION(AssignCKBehaviorIoDesc), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBEHAVIORIO_DESC", "CKDWORD get_Flags() const", asFUNCTION(GetCKBehaviorIoDescFlags), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBEHAVIORIO_DESC", "void set_Flags(CKDWORD value)", asFUNCTION(SetCKBehaviorIoDescFlags), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBEHAVIORIO_DESC", "string get_Name() const", asFUNCTION(GetCKBehaviorIoDescName), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBEHAVIORIO_DESC", "void set_Name(const string &in value)", asFUNCTION(SetCKBehaviorIoDescName), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKBehaviorPrototype

static bool RejectCKBehaviorPrototypeRawPointerWrite(const NativePointer &ptr, const char *message) {
    if (ptr.IsNull()) {
        return false;
    }
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
    return true;
}

static void SetCKBehaviorPrototypeFunction(CKBehaviorPrototype *self, NativePointer fct) {
    if (RejectCKBehaviorPrototypeRawPointerWrite(fct, "CKBehaviorPrototype.SetFunction only accepts a null NativePointer from script.")) {
        return;
    }
    self->SetFunction(nullptr);
}

static NativePointer GetCKBehaviorPrototypeFunction(CKBehaviorPrototype *self) {
    return NativePointer(reinterpret_cast<uintptr_t>(self->GetFunction()));
}

static void SetCKBehaviorPrototypeCallback(CKBehaviorPrototype *self, NativePointer fct, CKDWORD callbackMask, NativePointer param) {
    if (RejectCKBehaviorPrototypeRawPointerWrite(fct, "CKBehaviorPrototype.SetBehaviorCallbackFct only accepts a null function NativePointer from script.") ||
        RejectCKBehaviorPrototypeRawPointerWrite(param, "CKBehaviorPrototype.SetBehaviorCallbackFct only accepts a null parameter NativePointer from script.")) {
        return;
    }
    self->SetBehaviorCallbackFct(nullptr, callbackMask, nullptr);
}

static NativePointer GetCKBehaviorPrototypeCallback(CKBehaviorPrototype *self) {
    return NativePointer(reinterpret_cast<uintptr_t>(self->GetBehaviorCallbackFct()));
}

static int SetCKBehaviorPrototypeParameterException(const char *message) {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
    return CKERR_INVALIDPARAMETER;
}

static bool GetCKBehaviorPrototypeRawDefaultValue(NativePointer defaultVal, int valSize, void *&value) {
    value = nullptr;
    if (valSize < 0) {
        SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype raw default value size must be non-negative.");
        return false;
    }
    if (valSize == 0) {
        return true;
    }
    if (defaultVal.IsNull()) {
        SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype raw default value requires a non-null NativePointer when size is positive.");
        return false;
    }
    value = defaultVal.Get();
    return true;
}

static int DeclareCKBehaviorPrototypeInParameter(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareInParameter requires a valid prototype.");
    }
    return self->DeclareInParameter(const_cast<CKSTRING>(name.c_str()), guidType, static_cast<CKSTRING>(nullptr));
}

static int DeclareCKBehaviorPrototypeInParameterString(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType, const std::string &defaultVal) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareInParameter requires a valid prototype.");
    }
    return self->DeclareInParameter(const_cast<CKSTRING>(name.c_str()), guidType, const_cast<CKSTRING>(defaultVal.c_str()));
}

static int DeclareCKBehaviorPrototypeInParameterRaw(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType, NativePointer defaultVal, int valSize) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareInParameter requires a valid prototype.");
    }
    void *value = nullptr;
    if (!GetCKBehaviorPrototypeRawDefaultValue(defaultVal, valSize, value)) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->DeclareInParameter(const_cast<CKSTRING>(name.c_str()), guidType, value, valSize);
}

static int DeclareCKBehaviorPrototypeOutParameter(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareOutParameter requires a valid prototype.");
    }
    return self->DeclareOutParameter(const_cast<CKSTRING>(name.c_str()), guidType, static_cast<CKSTRING>(nullptr));
}

static int DeclareCKBehaviorPrototypeOutParameterString(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType, const std::string &defaultVal) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareOutParameter requires a valid prototype.");
    }
    return self->DeclareOutParameter(const_cast<CKSTRING>(name.c_str()), guidType, const_cast<CKSTRING>(defaultVal.c_str()));
}

static int DeclareCKBehaviorPrototypeOutParameterRaw(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType, NativePointer defaultVal, int valSize) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareOutParameter requires a valid prototype.");
    }
    void *value = nullptr;
    if (!GetCKBehaviorPrototypeRawDefaultValue(defaultVal, valSize, value)) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->DeclareOutParameter(const_cast<CKSTRING>(name.c_str()), guidType, value, valSize);
}

static int DeclareCKBehaviorPrototypeLocalParameter(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareLocalParameter requires a valid prototype.");
    }
    return self->DeclareLocalParameter(const_cast<CKSTRING>(name.c_str()), guidType, static_cast<CKSTRING>(nullptr));
}

static int DeclareCKBehaviorPrototypeLocalParameterString(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType, const std::string &defaultVal) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareLocalParameter requires a valid prototype.");
    }
    return self->DeclareLocalParameter(const_cast<CKSTRING>(name.c_str()), guidType, const_cast<CKSTRING>(defaultVal.c_str()));
}

static int DeclareCKBehaviorPrototypeLocalParameterRaw(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType, NativePointer defaultVal, int valSize) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareLocalParameter requires a valid prototype.");
    }
    void *value = nullptr;
    if (!GetCKBehaviorPrototypeRawDefaultValue(defaultVal, valSize, value)) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->DeclareLocalParameter(const_cast<CKSTRING>(name.c_str()), guidType, value, valSize);
}

static int DeclareCKBehaviorPrototypeSetting(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareSetting requires a valid prototype.");
    }
    return self->DeclareSetting(const_cast<CKSTRING>(name.c_str()), guidType, static_cast<CKSTRING>(nullptr));
}

static int DeclareCKBehaviorPrototypeSettingString(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType, const std::string &defaultVal) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareSetting requires a valid prototype.");
    }
    return self->DeclareSetting(const_cast<CKSTRING>(name.c_str()), guidType, const_cast<CKSTRING>(defaultVal.c_str()));
}

static int DeclareCKBehaviorPrototypeSettingRaw(CKBehaviorPrototype *self, const std::string &name, CKGUID guidType, NativePointer defaultVal, int valSize) {
    if (!self) {
        return SetCKBehaviorPrototypeParameterException("CKBehaviorPrototype.DeclareSetting requires a valid prototype.");
    }
    void *value = nullptr;
    if (!GetCKBehaviorPrototypeRawDefaultValue(defaultVal, valSize, value)) {
        return CKERR_INVALIDPARAMETER;
    }
    return self->DeclareSetting(const_cast<CKSTRING>(name.c_str()), guidType, value, valSize);
}

#if CKVERSION == 0x13022002
static CKBEHAVIORIO_DESC *GetInvalidCKBehaviorPrototypeIODesc(const char *message) {
    static thread_local CKBEHAVIORIO_DESC dummy;
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
    return &dummy;
}

static CKPARAMETER_DESC *GetInvalidCKBehaviorPrototypeParameterDesc(const char *message) {
    static thread_local CKPARAMETER_DESC dummy;
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
    return &dummy;
}

static CKBEHAVIORIO_DESC *GetCKBehaviorPrototypeInIOListEntry(CKBehaviorPrototype *self, int index) {
    if (!self) {
        return GetInvalidCKBehaviorPrototypeIODesc("CKBehaviorPrototype.GetInIOList requires a valid prototype.");
    }
    CKBEHAVIORIO_DESC **list = self->GetInIOList();
    if (index < 0 || index >= self->GetInputCount() || !list || !list[index]) {
        return GetInvalidCKBehaviorPrototypeIODesc("CKBehaviorPrototype.GetInIOList index out of range.");
    }
    return list[index];
}

static CKBEHAVIORIO_DESC *GetCKBehaviorPrototypeOutIOListEntry(CKBehaviorPrototype *self, int index) {
    if (!self) {
        return GetInvalidCKBehaviorPrototypeIODesc("CKBehaviorPrototype.GetOutIOList requires a valid prototype.");
    }
    CKBEHAVIORIO_DESC **list = self->GetOutIOList();
    if (index < 0 || index >= self->GetOutputCount() || !list || !list[index]) {
        return GetInvalidCKBehaviorPrototypeIODesc("CKBehaviorPrototype.GetOutIOList index out of range.");
    }
    return list[index];
}

static CKPARAMETER_DESC *GetCKBehaviorPrototypeInParameterListEntry(CKBehaviorPrototype *self, int index) {
    if (!self) {
        return GetInvalidCKBehaviorPrototypeParameterDesc("CKBehaviorPrototype.GetInParameterList requires a valid prototype.");
    }
    CKPARAMETER_DESC **list = self->GetInParameterList();
    if (index < 0 || index >= self->GetInParameterCount() || !list || !list[index]) {
        return GetInvalidCKBehaviorPrototypeParameterDesc("CKBehaviorPrototype.GetInParameterList index out of range.");
    }
    return list[index];
}

static CKPARAMETER_DESC *GetCKBehaviorPrototypeOutParameterListEntry(CKBehaviorPrototype *self, int index) {
    if (!self) {
        return GetInvalidCKBehaviorPrototypeParameterDesc("CKBehaviorPrototype.GetOutParameterList requires a valid prototype.");
    }
    CKPARAMETER_DESC **list = self->GetOutParameterList();
    if (index < 0 || index >= self->GetOutParameterCount() || !list || !list[index]) {
        return GetInvalidCKBehaviorPrototypeParameterDesc("CKBehaviorPrototype.GetOutParameterList index out of range.");
    }
    return list[index];
}

static CKPARAMETER_DESC *GetCKBehaviorPrototypeLocalParameterListEntry(CKBehaviorPrototype *self, int index) {
    if (!self) {
        return GetInvalidCKBehaviorPrototypeParameterDesc("CKBehaviorPrototype.GetLocalParameterList requires a valid prototype.");
    }
    CKPARAMETER_DESC **list = self->GetLocalParameterList();
    if (index < 0 || index >= self->GetLocalParameterCount() || !list || !list[index]) {
        return GetInvalidCKBehaviorPrototypeParameterDesc("CKBehaviorPrototype.GetLocalParameterList index out of range.");
    }
    return list[index];
}
#endif

void RegisterCKBehaviorPrototype(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareInput(const string &in name)", asFUNCTIONPR([](CKBehaviorPrototype *self, const std::string &name) { return self->DeclareInput(const_cast<CKSTRING>(name.c_str())); }, (CKBehaviorPrototype *, const std::string &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareOutput(const string &in name)", asFUNCTIONPR([](CKBehaviorPrototype *self, const std::string &name) { return self->DeclareOutput(const_cast<CKSTRING>(name.c_str())); }, (CKBehaviorPrototype *, const std::string &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareInParameter(const string &in name, CKGUID guidType)", asFUNCTION(DeclareCKBehaviorPrototypeInParameter), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareInParameter(const string &in name, CKGUID guidType, const string &in defaultVal)", asFUNCTION(DeclareCKBehaviorPrototypeInParameterString), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareInParameter(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)", asFUNCTION(DeclareCKBehaviorPrototypeInParameterRaw), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareOutParameter(const string &in name, CKGUID guidType)", asFUNCTION(DeclareCKBehaviorPrototypeOutParameter), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareOutParameter(const string &in name, CKGUID guidType, const string &in defaultVal)", asFUNCTION(DeclareCKBehaviorPrototypeOutParameterString), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareOutParameter(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)", asFUNCTION(DeclareCKBehaviorPrototypeOutParameterRaw), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareLocalParameter(const string &in name, CKGUID guidType)", asFUNCTION(DeclareCKBehaviorPrototypeLocalParameter), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareLocalParameter(const string &in name, CKGUID guidType, const string &in defaultVal)", asFUNCTION(DeclareCKBehaviorPrototypeLocalParameterString), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareLocalParameter(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)", asFUNCTION(DeclareCKBehaviorPrototypeLocalParameterRaw), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareSetting(const string &in name, CKGUID guidType)", asFUNCTION(DeclareCKBehaviorPrototypeSetting), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareSetting(const string &in name, CKGUID guidType, const string &in defaultVal)", asFUNCTION(DeclareCKBehaviorPrototypeSettingString), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int DeclareSetting(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)", asFUNCTION(DeclareCKBehaviorPrototypeSettingRaw), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "void SetGuid(const CKGUID &in guid)", asMETHODPR(CKBehaviorPrototype, SetGuid, (CKGUID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CKGUID GetGuid()", asMETHODPR(CKBehaviorPrototype, GetGuid, (), CKGUID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "void SetFlags(CK_BEHAVIORPROTOTYPE_FLAGS flags)", asMETHODPR(CKBehaviorPrototype, SetFlags, (CK_BEHAVIORPROTOTYPE_FLAGS), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CK_BEHAVIORPROTOTYPE_FLAGS GetFlags()", asMETHODPR(CKBehaviorPrototype, GetFlags, (), CK_BEHAVIORPROTOTYPE_FLAGS), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "void SetApplyToClassID(CK_CLASSID cid)", asMETHODPR(CKBehaviorPrototype, SetApplyToClassID, (CK_CLASSID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CK_CLASSID GetApplyToClassID()", asMETHODPR(CKBehaviorPrototype, GetApplyToClassID, (), CK_CLASSID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "void SetFunction(NativePointer fct)", asFUNCTION(SetCKBehaviorPrototypeFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "NativePointer GetFunction()", asFUNCTION(GetCKBehaviorPrototypeFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "void SetBehaviorCallbackFct(NativePointer fct, CKDWORD callbackMask, NativePointer param)", asFUNCTION(SetCKBehaviorPrototypeCallback), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "NativePointer GetBehaviorCallbackFct()", asFUNCTION(GetCKBehaviorPrototypeCallback), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "void SetBehaviorFlags(CK_BEHAVIOR_FLAGS flags)", asMETHODPR(CKBehaviorPrototype, SetBehaviorFlags, (CK_BEHAVIOR_FLAGS), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CK_BEHAVIOR_FLAGS GetBehaviorFlags()", asMETHODPR(CKBehaviorPrototype, GetBehaviorFlags, (), CK_BEHAVIOR_FLAGS), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "string GetName()", asFUNCTIONPR([](CKBehaviorPrototype *self) -> std::string { return ScriptStringify(self->GetName()); }, (CKBehaviorPrototype *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetInputCount()", asMETHODPR(CKBehaviorPrototype, GetInputCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetOutputCount()", asMETHODPR(CKBehaviorPrototype, GetOutputCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetInParameterCount()", asMETHODPR(CKBehaviorPrototype, GetInParameterCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetOutParameterCount()", asMETHODPR(CKBehaviorPrototype, GetOutParameterCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetLocalParameterCount()", asMETHODPR(CKBehaviorPrototype, GetLocalParameterCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CKBEHAVIORIO_DESC &GetInIOList(int index)", asFUNCTION(GetCKBehaviorPrototypeInIOListEntry), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CKBEHAVIORIO_DESC &GetOutIOList(int index)", asFUNCTION(GetCKBehaviorPrototypeOutIOListEntry), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CKPARAMETER_DESC &GetInParameterList(int index)", asFUNCTION(GetCKBehaviorPrototypeInParameterListEntry), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CKPARAMETER_DESC &GetOutParameterList(int index)", asFUNCTION(GetCKBehaviorPrototypeOutParameterListEntry), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CKPARAMETER_DESC &GetLocalParameterList(int index)", asFUNCTION(GetCKBehaviorPrototypeLocalParameterListEntry), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif

    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "CKObjectDeclaration@ GetSourceObjectDeclaration()", asMETHODPR(CKBehaviorPrototype, GetSoureObjectDeclaration, (), CKObjectDeclaration*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "void SetSourceObjectDeclaration(CKObjectDeclaration@ decl)", asMETHODPR(CKBehaviorPrototype, SetSourceObjectDeclaration, (CKObjectDeclaration*), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif

    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetInIOIndex(const string &in name)", asFUNCTIONPR([](CKBehaviorPrototype *self, const std::string &name) { return self->GetInIOIndex(const_cast<CKSTRING>(name.c_str())); }, (CKBehaviorPrototype *, const std::string &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetOutIOIndex(const string &in name)", asFUNCTIONPR([](CKBehaviorPrototype *self, const std::string &name) { return self->GetOutIOIndex(const_cast<CKSTRING>(name.c_str())); }, (CKBehaviorPrototype *, const std::string &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetInParamIndex(const string &in name)", asFUNCTIONPR([](CKBehaviorPrototype *self, const std::string &name) { return self->GetInParamIndex(const_cast<CKSTRING>(name.c_str())); }, (CKBehaviorPrototype *, const std::string &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetOutParamIndex(const string &in name)", asFUNCTIONPR([](CKBehaviorPrototype *self, const std::string &name) { return self->GetOutParamIndex(const_cast<CKSTRING>(name.c_str())); }, (CKBehaviorPrototype *, const std::string &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "int GetLocalParamIndex(const string &in name)", asFUNCTIONPR([](CKBehaviorPrototype *self, const std::string &name) { return self->GetLocalParamIndex(const_cast<CKSTRING>(name.c_str())); }, (CKBehaviorPrototype *, const std::string &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBehaviorPrototype", "bool IsRunTime()", asFUNCTIONPR([](CKBehaviorPrototype *self) -> bool { return self->IsRunTime(); }, (CKBehaviorPrototype *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKBitmapSlot

static void ConstructCKBitmapSlot(CKBitmapSlot *self) {
    new(self) CKBitmapSlot();
}

static void ConstructCKBitmapSlotCopy(const CKBitmapSlot &other, CKBitmapSlot *self) {
    new(self) CKBitmapSlot();
    self->m_FileName = other.m_FileName;
}

static CKBitmapSlot &AssignCKBitmapSlot(CKBitmapSlot *self, const CKBitmapSlot &other) {
    if (self == &other) {
        return *self;
    }

    self->Flush();
    self->m_FileName = other.m_FileName;
    return *self;
}

static bool CKBitmapSlotHasDataBuffer(const CKBitmapSlot *self) {
    return self->m_DataBuffer != nullptr;
}

void RegisterCKBitmapSlot(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKBitmapSlot", "XString m_FileName", asOFFSET(CKBitmapSlot, m_FileName)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKBitmapSlot", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCKBitmapSlot), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKBitmapSlot", asBEHAVE_CONSTRUCT, "void f(const CKBitmapSlot &in other)", asFUNCTION(ConstructCKBitmapSlotCopy), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKBitmapSlot", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKBitmapSlot *self) { self->~CKBitmapSlot(); }, (CKBitmapSlot *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapSlot", "CKBitmapSlot &opAssign(const CKBitmapSlot &in other)", asFUNCTION(AssignCKBitmapSlot), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapSlot", "void Allocate(int, int, int)", asMETHOD(CKBitmapSlot, Allocate), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapSlot", "void Free()", asMETHOD(CKBitmapSlot, Free), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapSlot", "void Resize(VxImageDescEx & src, VxImageDescEx & dest)", asMETHOD(CKBitmapSlot, Resize), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapSlot", "void Flush()", asMETHOD(CKBitmapSlot, Flush), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapSlot", "bool HasDataBuffer() const", asFUNCTION(CKBitmapSlotHasDataBuffer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKMovieInfo

void RegisterCKMovieInfo(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKMovieInfo", "XString m_MovieFileName", asOFFSET(CKMovieInfo, m_MovieFileName)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKMovieInfo", "CKMovieReader@ m_MovieReader", asOFFSET(CKMovieInfo, m_MovieReader)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKMovieInfo", "int m_MovieCurrentSlot", asOFFSET(CKMovieInfo, m_MovieCurrentSlot)); CKAS_CHECK_REGISTER(r);
}

// CKBitmapData

void RegisterCKBitmapData(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKBitmapData", "CKMovieInfo@ m_MovieInfo", asOFFSET(CKBitmapData, m_MovieInfo)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKBitmapData", "XArray<CKBitmapSlot *> m_Slots", asOFFSET(CKBitmapData, m_Slots)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBitmapData", "int m_Width", asOFFSET(CKBitmapData, m_Width)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBitmapData", "int m_Height", asOFFSET(CKBitmapData, m_Height)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBitmapData", "int m_CurrentSlot", asOFFSET(CKBitmapData, m_CurrentSlot)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBitmapData", "int m_PickThreshold", asOFFSET(CKBitmapData, m_PickThreshold)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBitmapData", "CKDWORD m_BitmapFlags", asOFFSET(CKBitmapData, m_BitmapFlags)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBitmapData", "CKDWORD m_TransColor", asOFFSET(CKBitmapData, m_TransColor)); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectBehaviour("CKBitmapData", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKBitmapData *self) { new(self) CKBitmapData(); }, (CKBitmapData *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectBehaviour("CKBitmapData", asBEHAVE_CONSTRUCT, "void f(CKContext@ context)", asFUNCTIONPR([](CKBitmapData *self, CKContext *context) { new(self) CKBitmapData(context); }, (CKBitmapData *, CKContext *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectBehaviour("CKBitmapData", asBEHAVE_CONSTRUCT, "void f(const CKBitmapData &in other)", asFUNCTIONPR([](const CKBitmapData &data, CKBitmapData *self) { new(self) CKBitmapData(data); }, (const CKBitmapData &, CKBitmapData *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKBitmapData", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKBitmapData *self) { self->~CKBitmapData(); }, (CKBitmapData *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "CKBitmapData &opAssign(const CKBitmapData &in other)", asMETHODPR(CKBitmapData, operator=, (const CKBitmapData &), CKBitmapData &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKBitmapData", "bool CreateImage(int width, int height, int bpp = 32, int slot = 0)", asFUNCTIONPR([](CKBitmapData *self, int width, int height, int bpp, int slot) -> bool { return self->CreateImage(width, height, bpp, slot); }, (CKBitmapData *, int, int, int, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKBitmapData", "bool CreateImage(int width, int height, int bpp = 32, int slot = 0, NativePointer imagePointer = 0)", asFUNCTIONPR([](CKBitmapData *self, int width, int height, int bpp, int slot, NativePointer imagePointer) -> bool { return self->CreateImage(width, height, bpp, slot, imagePointer.Get()); }, (CKBitmapData *, int, int, int, int, NativePointer), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKBitmapData", "bool SaveImage(const string &in name, int slot = 0, bool useFormat = false)", asFUNCTIONPR([](CKBitmapData *self, const std::string &name, int slot, bool useFormat) -> bool { return self->SaveImage(const_cast<CKSTRING>(name.c_str()), slot, useFormat); }, (CKBitmapData *, const std::string &, int, bool), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool SaveImageAlpha(const string &in name, int slot = 0)", asFUNCTIONPR([](CKBitmapData *self, const std::string &name, int slot) -> bool { return self->SaveImageAlpha(const_cast<CKSTRING>(name.c_str()), slot); }, (CKBitmapData *, const std::string &, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "string GetMovieFileName()", asFUNCTIONPR([](CKBitmapData *self) -> std::string { return ScriptStringify(self->GetMovieFileName()); }, (CKBitmapData *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "CKMovieReader@ GetMovieReader()", asMETHODPR(CKBitmapData, GetMovieReader, (), CKMovieReader *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "NativePointer LockSurfacePtr(int slot = -1)", asFUNCTIONPR([](CKBitmapData *self, int slot) { return NativePointer(self->LockSurfacePtr(slot)); }, (CKBitmapData *, int), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool ReleaseSurfacePtr(int slot = -1)", asFUNCTIONPR([](CKBitmapData *self, int slot) -> bool { return self->ReleaseSurfacePtr(slot); }, (CKBitmapData *, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "string GetSlotFileName(int slot)", asFUNCTIONPR([](CKBitmapData *self, int slot) -> std::string { return ScriptStringify(self->GetSlotFileName(slot)); }, (CKBitmapData *, int), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool SetSlotFileName(int slot, const string &in filename)", asFUNCTIONPR([](CKBitmapData *self, int slot, const std::string &filename) -> bool { return self->SetSlotFileName(slot, const_cast<CKSTRING>(filename.c_str())); }, (CKBitmapData *, int, const std::string &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "int GetWidth()", asMETHODPR(CKBitmapData, GetWidth, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "int GetHeight()", asMETHODPR(CKBitmapData, GetHeight, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool GetImageDesc(VxImageDescEx &out desc)", asFUNCTIONPR([](CKBitmapData *self, VxImageDescEx &desc) -> bool { return self->GetImageDesc(desc); }, (CKBitmapData *, VxImageDescEx &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "int GetSlotCount()", asMETHODPR(CKBitmapData, GetSlotCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool SetSlotCount(int count)", asFUNCTIONPR([](CKBitmapData *self, int count) -> bool { return self->SetSlotCount(count); }, (CKBitmapData *, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool SetCurrentSlot(int slot)", asFUNCTIONPR([](CKBitmapData *self, int slot) -> bool { return self->SetCurrentSlot(slot); }, (CKBitmapData *, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "int GetCurrentSlot()", asMETHODPR(CKBitmapData, GetCurrentSlot, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool ReleaseSlot(int slot)", asFUNCTIONPR([](CKBitmapData *self, int slot) -> bool { return self->ReleaseSlot(slot); }, (CKBitmapData *, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool ReleaseAllSlots()", asFUNCTIONPR([](CKBitmapData *self) -> bool { return self->ReleaseAllSlots(); }, (CKBitmapData *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "bool SetPixel(int x, int y, CKDWORD color, int slot = -1)", asFUNCTIONPR([](CKBitmapData *self, int x, int y, CKDWORD color, int slot) -> bool { return self->SetPixel(x, y, color, slot); }, (CKBitmapData *, int, int, CKDWORD, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "CKDWORD GetPixel(int x, int y, int slot = -1)", asMETHODPR(CKBitmapData, GetPixel, (int, int, int), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "CKDWORD GetTransparentColor()", asMETHODPR(CKBitmapData, GetTransparentColor, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "void SetTransparentColor(CKDWORD color)", asMETHODPR(CKBitmapData, SetTransparentColor, (CKDWORD), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "void SetTransparent(bool transparency)", asFUNCTIONPR([](CKBitmapData *self, bool transparency) { self->SetTransparent(transparency); }, (CKBitmapData *, bool), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool IsTransparent()", asFUNCTIONPR([](CKBitmapData *self) -> bool { return self->IsTransparent(); }, (CKBitmapData *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "CK_TEXTURE_SAVEOPTIONS GetSaveOptions()", asMETHODPR(CKBitmapData, GetSaveOptions, (), CK_BITMAP_SAVEOPTIONS), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "void SetSaveOptions(CK_TEXTURE_SAVEOPTIONS options)", asMETHODPR(CKBitmapData, SetSaveOptions, (CK_BITMAP_SAVEOPTIONS), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "CKBitmapProperties@ GetSaveFormat()", asMETHODPR(CKBitmapData, GetSaveFormat, (), CKBitmapProperties *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "void SetSaveFormat(CKBitmapProperties@ format)", asMETHODPR(CKBitmapData, SetSaveFormat, (CKBitmapProperties *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "void SetPickThreshold(int pt)", asMETHODPR(CKBitmapData, SetPickThreshold, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "int GetPickThreshold()", asMETHODPR(CKBitmapData, GetPickThreshold, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "void SetCubeMap(bool cubeMap)", asFUNCTIONPR([](CKBitmapData *self, bool cubeMap) { self->SetCubeMap(cubeMap); }, (CKBitmapData *, bool), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool IsCubeMap()", asFUNCTIONPR([](CKBitmapData *self) -> bool { return self->IsCubeMap(); }, (CKBitmapData *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "bool ResizeImages(int width, int height)", asFUNCTIONPR([](CKBitmapData *self, int width, int height) -> bool { return self->ResizeImages(width, height); }, (CKBitmapData *, int, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "void SetDynamicHint(bool dynamic)", asFUNCTIONPR([](CKBitmapData *self, bool dynamic) { self->SetDynamicHint(dynamic); }, (CKBitmapData *, bool), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool GetDynamicHint()", asFUNCTIONPR([](CKBitmapData *self) -> bool { return self->GetDynamicHint(); }, (CKBitmapData *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "bool ToRestore()", asFUNCTIONPR([](CKBitmapData *self) -> bool { return self->ToRestore(); }, (CKBitmapData *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKBitmapData", "bool LoadSlotImage(XString name, int slot = 0)", asFUNCTIONPR([](CKBitmapData *self, XString name, int slot) -> bool { return self->LoadSlotImage(name, slot); }, (CKBitmapData *, XString, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool LoadMovieFile(XString name)", asFUNCTIONPR([](CKBitmapData *self, XString name) -> bool { return self->LoadMovieFile(name); }, (CKBitmapData *, XString), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "CKMovieInfo@ CreateMovieInfo(XString s, CKMovieProperties@ &out mp)", asMETHODPR(CKBitmapData, CreateMovieInfo, (XString, CKMovieProperties**), CKMovieInfo*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKBitmapData", "bool LoadSlotImage(const XString &name, int slot = 0)", asFUNCTIONPR([](CKBitmapData *self, const XString &name, int slot) -> bool { return self->LoadSlotImage(name, slot); }, (CKBitmapData *, const XString &, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool LoadMovieFile(const XString &name)", asFUNCTIONPR([](CKBitmapData *self, const XString &name) -> bool { return self->LoadMovieFile(name); }, (CKBitmapData *, const XString &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "CKMovieInfo@ CreateMovieInfo(const XString &s, CKMovieProperties@ &out mp)", asMETHODPR(CKBitmapData, CreateMovieInfo, (const XString &, CKMovieProperties **), CKMovieInfo *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKBitmapData", "void SetMovieInfo(CKMovieInfo@ mi)", asMETHODPR(CKBitmapData, SetMovieInfo, (CKMovieInfo *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBitmapData", "void SetAlphaForTransparentColor(const VxImageDescEx &in desc)", asMETHODPR(CKBitmapData, SetAlphaForTransparentColor, (const VxImageDescEx &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "void SetBorderColorForClamp(const VxImageDescEx &in desc)", asMETHODPR(CKBitmapData, SetBorderColorForClamp, (const VxImageDescEx &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBitmapData", "bool SetSlotImage(int slot, NativePointer buffer, const VxImageDescEx &in desc)", asFUNCTIONPR([](CKBitmapData *self, int slot, NativePointer buffer, VxImageDescEx &desc) -> bool { return self->SetSlotImage(slot, buffer.Get(), desc); }, (CKBitmapData *, int, NativePointer, VxImageDescEx &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // r = engine->RegisterObjectMethod("CKBitmapData", "bool DumpToChunk(CKStateChunk@ chunk, CKContext@ context, CKFile@ file, NativeBuffer Identifiers)", asFUNCTIONPR([](CKBitmapData *self, CKStateChunk *chunk, CKContext *context, CKFile *file, NativeBuffer identifiers) -> bool { return self->DumpToChunk(chunk, context, file, identifiers); }, (CKBitmapData *, CKStateChunk *, CKContext *, CKFile *, NativeBuffer), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKBitmapData", "bool ReadFromChunk(CKStateChunk@ chunk, CKContext@ context, CKFile@ file, NativeBuffer Identifiers)", asFUNCTIONPR([](CKBitmapData *self, CKStateChunk *chunk, CKContext *context, CKFile *file, NativeBuffer identifiers) -> bool { return self->ReadFromChunk(chunk, context, file, identifiers); }, (CKBitmapData *, CKStateChunk *, CKContext *, CKFile *, NativeBuffer), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

void RegisterCKVertexBuffer(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectBehaviour("CKVertexBuffer", asBEHAVE_ADDREF, "void f()", asMETHOD(ScriptCKVertexBuffer, AddRef), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKVertexBuffer", asBEHAVE_RELEASE, "void f()", asMETHOD(ScriptCKVertexBuffer, Release), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKVertexBuffer", "bool get_valid() const", asMETHOD(ScriptCKVertexBuffer, IsValid), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKVertexBuffer", "bool IsValid() const", asMETHOD(ScriptCKVertexBuffer, IsValid), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKVertexBuffer", "void Destroy()", asMETHOD(ScriptCKVertexBuffer, Destroy), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKVertexBuffer", "CKVB_STATE Check(CKRenderContext@ ctx, uint maxVertexCount, CKRST_DPFLAGS format, bool dynamic = false)", asMETHOD(ScriptCKVertexBuffer, Check), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKVertexBuffer", "bool Lock(CKRenderContext@ ctx, CKDWORD startVertex, CKDWORD vertexCount, VxDrawPrimitiveData &out data, CKLOCKFLAGS lockFlags = CK_LOCK_DEFAULT)", asMETHOD(ScriptCKVertexBuffer, Lock), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKVertexBuffer", "void Unlock(CKRenderContext@ ctx)", asMETHOD(ScriptCKVertexBuffer, Unlock), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKVertexBuffer", "bool Draw(CKRenderContext@ ctx, VXPRIMITIVETYPE pType, CKDWORD startVertex, CKDWORD vertexCount)", asMETHODPR(ScriptCKVertexBuffer, Draw, (CKRenderContext *, VXPRIMITIVETYPE, CKDWORD, CKDWORD), bool), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKVertexBuffer", "bool Draw(CKRenderContext@ ctx, VXPRIMITIVETYPE pType, const array<uint16> &in indices, CKDWORD startVertex, CKDWORD vertexCount)", asMETHOD(ScriptCKVertexBuffer, DrawIndexed), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKFloorPoint

void RegisterCKFloorPoint(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKFloorPoint", "CK_ID m_UpFloor", offsetof(CKFloorPoint, m_UpFloor)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFloorPoint", "int m_UpFaceIndex", offsetof(CKFloorPoint, m_UpFaceIndex)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFloorPoint", "VxVector m_UpNormal", offsetof(CKFloorPoint, m_UpNormal)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFloorPoint", "float m_UpDistance", offsetof(CKFloorPoint, m_UpDistance)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFloorPoint", "CK_ID m_DownFloor", offsetof(CKFloorPoint, m_DownFloor)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFloorPoint", "int m_DownFaceIndex", offsetof(CKFloorPoint, m_DownFaceIndex)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFloorPoint", "VxVector m_DownNormal", offsetof(CKFloorPoint, m_DownNormal)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFloorPoint", "float m_DownDistance", offsetof(CKFloorPoint, m_DownDistance)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFloorPoint", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKFloorPoint *self) { new(self) CKFloorPoint(); }, (CKFloorPoint *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKFloorPoint", asBEHAVE_CONSTRUCT, "void f(const CKFloorPoint &in other)", asFUNCTIONPR([](const CKFloorPoint &point, CKFloorPoint *self) { new(self) CKFloorPoint(point); }, (const CKFloorPoint &, CKFloorPoint *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFloorPoint", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKFloorPoint *self) { self->~CKFloorPoint(); }, (CKFloorPoint *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFloorPoint", "CKFloorPoint &opAssign(const CKFloorPoint &in other)", asMETHODPR(CKFloorPoint, operator=, (const CKFloorPoint &), CKFloorPoint &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFloorPoint", "void Clear()", asMETHOD(CKFloorPoint, Clear), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// SoundMinion

void RegisterSoundMinion(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("SoundMinion", "CKSOUNDHANDLE m_Source", asOFFSET(SoundMinion, m_Source)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("SoundMinion", "CKSOUNDHANDLE m_OriginalSource", asOFFSET(SoundMinion, m_OriginalSource)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("SoundMinion", "CK_ID m_Entity", asOFFSET(SoundMinion, m_Entity)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("SoundMinion", "CK_ID m_OriginalSound", asOFFSET(SoundMinion, m_OriginalSound)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("SoundMinion", "VxVector m_Position", asOFFSET(SoundMinion, m_Position)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("SoundMinion", "VxVector m_Direction", asOFFSET(SoundMinion, m_Direction)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("SoundMinion", "VxVector m_OldPosition", asOFFSET(SoundMinion, m_OldPosition)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("SoundMinion", "float m_TimeStamp", asOFFSET(SoundMinion, m_TimeStamp)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("SoundMinion", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](SoundMinion *self) { new(self) SoundMinion(); }, (SoundMinion *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("SoundMinion", asBEHAVE_CONSTRUCT, "void f(const SoundMinion &in other)", asFUNCTIONPR([](const SoundMinion &sm, SoundMinion *self) { new(self) SoundMinion(sm); }, (const SoundMinion &, SoundMinion *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("SoundMinion", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](SoundMinion *self) { self->~SoundMinion(); }, (SoundMinion *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("SoundMinion", "SoundMinion &opAssign(const SoundMinion &in other)", asMETHODPR(SoundMinion, operator=, (const SoundMinion &), SoundMinion &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKWaveSoundSettings

void RegisterCKWaveSoundSettings(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKWaveSoundSettings", "float m_Gain", offsetof(CKWaveSoundSettings, m_Gain)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSoundSettings", "float m_Eq", offsetof(CKWaveSoundSettings, m_Eq)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSoundSettings", "float m_Pitch", offsetof(CKWaveSoundSettings, m_Pitch)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSoundSettings", "float m_Priority", offsetof(CKWaveSoundSettings, m_Priority)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSoundSettings", "float m_Pan", offsetof(CKWaveSoundSettings, m_Pan)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKWaveSoundSettings", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKWaveSoundSettings *self) { new(self) CKWaveSoundSettings(); }, (CKWaveSoundSettings *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKWaveSoundSettings", asBEHAVE_CONSTRUCT, "void f(const CKWaveSoundSettings &in other)", asFUNCTIONPR([](const CKWaveSoundSettings &s, CKWaveSoundSettings *self) { new(self) CKWaveSoundSettings(s); }, (const CKWaveSoundSettings &, CKWaveSoundSettings *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKWaveSoundSettings", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKWaveSoundSettings *self) { self->~CKWaveSoundSettings(); }, (CKWaveSoundSettings *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKWaveSoundSettings", "CKWaveSoundSettings &opAssign(const CKWaveSoundSettings &in other)", asMETHODPR(CKWaveSoundSettings, operator=, (const CKWaveSoundSettings &), CKWaveSoundSettings &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKWaveSound3DSettings

void RegisterCKWaveSound3DSettings(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "uint16 m_HeadRelative", offsetof(CKWaveSound3DSettings, m_HeadRelative)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "uint16 m_MuteAfterMax", offsetof(CKWaveSound3DSettings, m_MuteAfterMax)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "float m_InAngle", offsetof(CKWaveSound3DSettings, m_InAngle)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "float m_OutAngle", offsetof(CKWaveSound3DSettings, m_OutAngle)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "float m_OutsideGain", offsetof(CKWaveSound3DSettings, m_OutsideGain)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "float m_MinDistance", offsetof(CKWaveSound3DSettings, m_MinDistance)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "float m_MaxDistance", offsetof(CKWaveSound3DSettings, m_MaxDistance)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "VxVector m_Position", offsetof(CKWaveSound3DSettings, m_Position)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "VxVector m_Velocity", offsetof(CKWaveSound3DSettings, m_Velocity)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "VxVector m_OrientationDir", offsetof(CKWaveSound3DSettings, m_OrientationDir)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveSound3DSettings", "VxVector m_OrientationUp", offsetof(CKWaveSound3DSettings, m_OrientationUp)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKWaveSound3DSettings", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKWaveSound3DSettings *self) { new(self) CKWaveSound3DSettings(); }, (CKWaveSound3DSettings *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKWaveSound3DSettings", asBEHAVE_CONSTRUCT, "void f(const CKWaveSound3DSettings &in other)", asFUNCTIONPR([](const CKWaveSound3DSettings &s, CKWaveSound3DSettings *self) { new(self) CKWaveSound3DSettings(s); }, (const CKWaveSound3DSettings &, CKWaveSound3DSettings *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKWaveSound3DSettings", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKWaveSound3DSettings *self) { self->~CKWaveSound3DSettings(); }, (CKWaveSound3DSettings *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKWaveSound3DSettings", "CKWaveSound3DSettings &opAssign(const CKWaveSound3DSettings &in other)", asMETHODPR(CKWaveSound3DSettings, operator=, (const CKWaveSound3DSettings &), CKWaveSound3DSettings &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKListenerSettings

void RegisterCKListenerSettings(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKListenerSettings", "float m_DistanceFactor", offsetof(CKListenerSettings, m_DistanceFactor)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKListenerSettings", "float m_DopplerFactor", offsetof(CKListenerSettings, m_DopplerFactor)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKListenerSettings", "float m_RollOff", offsetof(CKListenerSettings, m_RollOff)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKListenerSettings", "float m_GlobalGain", offsetof(CKListenerSettings, m_GlobalGain)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKListenerSettings", "float m_PriorityBias", offsetof(CKListenerSettings, m_PriorityBias)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKListenerSettings", "uint m_SoftwareSources", offsetof(CKListenerSettings, m_SoftwareSources)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKListenerSettings", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKListenerSettings *self) { new(self) CKListenerSettings(); }, (CKListenerSettings *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKListenerSettings", asBEHAVE_CONSTRUCT, "void f(const CKListenerSettings &in other)", asFUNCTIONPR([](const CKListenerSettings &s, CKListenerSettings *self) { new(self) CKListenerSettings(s); }, (const CKListenerSettings &, CKListenerSettings *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKListenerSettings", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKListenerSettings *self) { self->~CKListenerSettings(); }, (CKListenerSettings *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKListenerSettings", "CKListenerSettings &opAssign(const CKListenerSettings &in other)", asMETHODPR(CKListenerSettings, operator=, (const CKListenerSettings &), CKListenerSettings &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKWaveFormat

void RegisterCKWaveFormat(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKWaveFormat", "uint16 wFormatTag", offsetof(CKWaveFormat, wFormatTag)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveFormat", "uint16 nChannels", offsetof(CKWaveFormat, nChannels)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveFormat", "uint nSamplesPerSec", offsetof(CKWaveFormat, nSamplesPerSec)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveFormat", "uint nAvgBytesPerSec", offsetof(CKWaveFormat, nAvgBytesPerSec)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveFormat", "uint16 nBlockAlign", offsetof(CKWaveFormat, nBlockAlign)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveFormat", "uint16 wBitsPerSample", offsetof(CKWaveFormat, wBitsPerSample)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKWaveFormat", "uint16 cbSize", offsetof(CKWaveFormat, cbSize)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKWaveFormat", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKWaveFormat *self) { new(self) CKWaveFormat(); }, (CKWaveFormat *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKWaveFormat", asBEHAVE_CONSTRUCT, "void f(const CKWaveFormat &in other)", asFUNCTIONPR([](const CKWaveFormat &s, CKWaveFormat *self) { new(self) CKWaveFormat(s); }, (const CKWaveFormat &, CKWaveFormat *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKWaveFormat", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKWaveFormat *self) { self->~CKWaveFormat(); }, (CKWaveFormat *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKWaveFormat", "CKWaveFormat &opAssign(const CKWaveFormat &in other)", asMETHODPR(CKWaveFormat, operator=, (const CKWaveFormat &), CKWaveFormat &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// ImpactDesc

void RegisterImpactDesc(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("ImpactDesc", "CK_ID m_OwnerEntity", offsetof(ImpactDesc, m_OwnerEntity)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "CK_ID m_ObstacleTouched", offsetof(ImpactDesc, m_ObstacleTouched)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "CK_ID m_SubObstacleTouched", offsetof(ImpactDesc, m_SubObstacleTouched)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "int m_TouchedVertex", offsetof(ImpactDesc, m_TouchedVertex)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "int m_TouchingVertex", offsetof(ImpactDesc, m_TouchingVertex)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "int m_TouchedFace", offsetof(ImpactDesc, m_TouchedFace)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "int m_TouchingFace", offsetof(ImpactDesc, m_TouchingFace)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "VxMatrix m_ImpactWorldMatrix", offsetof(ImpactDesc, m_ImpactWorldMatrix)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "VxVector m_ImpactPoint", offsetof(ImpactDesc, m_ImpactPoint)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "VxVector m_ImpactNormal", offsetof(ImpactDesc, m_ImpactNormal)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("ImpactDesc", "CK_ID m_Entity", offsetof(ImpactDesc, m_Entity)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("ImpactDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](ImpactDesc *self) { new(self) ImpactDesc(); }, (ImpactDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("ImpactDesc", asBEHAVE_CONSTRUCT, "void f(const ImpactDesc &in other)", asFUNCTIONPR([](const ImpactDesc &desc, ImpactDesc *self) { new(self) ImpactDesc(desc); }, (const ImpactDesc &, ImpactDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("ImpactDesc", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](ImpactDesc *self) { self->~ImpactDesc(); }, (ImpactDesc *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("ImpactDesc", "ImpactDesc &opAssign(const ImpactDesc &in other)", asMETHODPR(ImpactDesc, operator=, (const ImpactDesc &), ImpactDesc &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKPICKRESULT

void RegisterCKPICKRESULT(asIScriptEngine *engine) {
#if CKVERSION == 0x13022002
    int r = 0;

    r = engine->RegisterObjectProperty("CKPICKRESULT", "VxVector IntersectionPoint", asOFFSET(CKPICKRESULT, IntersectionPoint)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPICKRESULT", "VxVector IntersectionNormal", asOFFSET(CKPICKRESULT, IntersectionNormal)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPICKRESULT", "float TexU", asOFFSET(CKPICKRESULT, TexU)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPICKRESULT", "float TexV", asOFFSET(CKPICKRESULT, TexV)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPICKRESULT", "float Distance", asOFFSET(CKPICKRESULT, Distance)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPICKRESULT", "int FaceIndex", asOFFSET(CKPICKRESULT, FaceIndex)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPICKRESULT", "CK_ID Sprite", asOFFSET(CKPICKRESULT, Sprite)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPICKRESULT", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKPICKRESULT *self) { new(self) CKPICKRESULT(); }, (CKPICKRESULT *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPICKRESULT", asBEHAVE_CONSTRUCT, "void f(const CKPICKRESULT &in other)", asFUNCTIONPR([](const CKPICKRESULT &res, CKPICKRESULT *self) { new(self) CKPICKRESULT(res); }, (const CKPICKRESULT &, CKPICKRESULT *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPICKRESULT", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPICKRESULT *self) { self->~CKPICKRESULT(); }, (CKPICKRESULT *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPICKRESULT", "CKPICKRESULT &opAssign(const CKPICKRESULT &in other)", asMETHODPR(CKPICKRESULT, operator=, (const CKPICKRESULT &), CKPICKRESULT &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif
}

// CKSquare

void RegisterCKSquare(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKSquare", "CKDWORD val", asOFFSET(CKSquare, dval)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKSquare", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKSquare *self) { new(self) CKSquare(); }, (CKSquare *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKSquare", asBEHAVE_CONSTRUCT, "void f(const CKSquare &in other)", asFUNCTIONPR([](const CKSquare &s, CKSquare *self) { new(self) CKSquare(s); }, (const CKSquare &, CKSquare *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKSquare", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKSquare *self) { self->~CKSquare(); }, (CKSquare *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKSquare", "CKSquare &opAssign(const CKSquare &in other)", asMETHODPR(CKSquare, operator=, (const CKSquare &), CKSquare &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CK2dCurvePoint

struct CK2dCurvePointStorage {
    CK2dCurve *Curve;
    float Tension;
    float Continuity;
    float Bias;
    float Length;
    Vx2DVector Position;
    Vx2DVector InTang;
    Vx2DVector OutTang;
    Vx2DVector RCurvePos;
    CKDWORD Flags;
};

static_assert(sizeof(CK2dCurvePointStorage) == sizeof(CK2dCurvePoint),
              "CK2dCurvePoint storage mirror must match the SDK layout.");

static CK2dCurvePointStorage &GetCK2dCurvePointStorage(CK2dCurvePoint &point) {
    return *reinterpret_cast<CK2dCurvePointStorage *>(&point);
}

static const CK2dCurvePointStorage &GetCK2dCurvePointStorage(const CK2dCurvePoint &point) {
    return *reinterpret_cast<const CK2dCurvePointStorage *>(&point);
}

static void CopyDetachedCK2dCurvePoint(const CK2dCurvePoint &source, CK2dCurvePoint &target) {
    std::memcpy(&target, &source, sizeof(CK2dCurvePoint));
    GetCK2dCurvePointStorage(target).Curve = nullptr;
}

static void ConstructCK2dCurvePointCopy(const CK2dCurvePoint &source, CK2dCurvePoint *self) {
    new(self) CK2dCurvePoint();
    CopyDetachedCK2dCurvePoint(source, *self);
}

static CK2dCurvePoint &AssignCK2dCurvePoint(CK2dCurvePoint &self, const CK2dCurvePoint &source) {
    if (&self != &source) {
        CopyDetachedCK2dCurvePoint(source, self);
    }
    return self;
}

static NativePointer GetCK2dCurvePointOwner(const CK2dCurvePoint &point) {
    return NativePointer(point.GetCurve());
}

static void SetCK2dCurvePointBias(CK2dCurvePoint &point, float bias) {
    if (point.GetCurve()) {
        point.SetBias(bias);
        return;
    }
    GetCK2dCurvePointStorage(point).Bias = bias;
}

static void SetCK2dCurvePointTension(CK2dCurvePoint &point, float tension) {
    if (point.GetCurve()) {
        point.SetTension(tension);
        return;
    }
    GetCK2dCurvePointStorage(point).Tension = tension;
}

static void SetCK2dCurvePointContinuity(CK2dCurvePoint &point, float continuity) {
    if (point.GetCurve()) {
        point.SetContinuity(continuity);
        return;
    }
    GetCK2dCurvePointStorage(point).Continuity = continuity;
}

static void SetCK2dCurvePointLinear(CK2dCurvePoint &point, bool linear) {
    if (point.GetCurve()) {
        point.SetLinear(linear);
        return;
    }

    CKDWORD &flags = GetCK2dCurvePointStorage(point).Flags;
    if (linear) {
        flags |= CK2DCURVEPOINT_LINEAR;
    } else {
        flags &= ~CK2DCURVEPOINT_LINEAR;
    }
}

static void UseCK2dCurvePointTCB(CK2dCurvePoint &point, bool use) {
    CKDWORD &flags = GetCK2dCurvePointStorage(point).Flags;
    if (use) {
        flags &= ~CK2DCURVEPOINT_USETANGENTS;
    } else {
        flags |= CK2DCURVEPOINT_USETANGENTS;
    }
}

static void SetCK2dCurvePointPosition(CK2dCurvePoint &point, const Vx2DVector &position) {
    if (point.GetCurve()) {
        point.SetPosition(position);
        return;
    }
    GetCK2dCurvePointStorage(point).Position = position;
}

static void SetCK2dCurvePointInTangent(CK2dCurvePoint &point, const Vx2DVector &input) {
    if (point.GetCurve()) {
        point.SetInTangent(input);
        return;
    }
    GetCK2dCurvePointStorage(point).InTang = input;
}

static void SetCK2dCurvePointOutTangent(CK2dCurvePoint &point, const Vx2DVector &output) {
    if (point.GetCurve()) {
        point.SetOutTangent(output);
        return;
    }
    GetCK2dCurvePointStorage(point).OutTang = output;
}

static void NotifyCK2dCurvePointUpdate(CK2dCurvePoint &point) {
    if (point.GetCurve()) {
        point.NotifyUpdate();
    }
}

void RegisterCK2dCurvePoint(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectBehaviour("CK2dCurvePoint", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CK2dCurvePoint *self) { new(self) CK2dCurvePoint(); }, (CK2dCurvePoint *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CK2dCurvePoint", asBEHAVE_CONSTRUCT, "void f(const CK2dCurvePoint &in other)", asFUNCTION(ConstructCK2dCurvePointCopy), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CK2dCurvePoint", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CK2dCurvePoint *self) { self->~CK2dCurvePoint(); }, (CK2dCurvePoint *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurvePoint", "CK2dCurvePoint &opAssign(const CK2dCurvePoint &in other)", asFUNCTION(AssignCK2dCurvePoint), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurvePoint", "NativePointer GetCurve() const", asFUNCTION(GetCK2dCurvePointOwner), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurvePoint", "float GetBias() const", asMETHOD(CK2dCurvePoint, GetBias), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "void SetBias(float b)", asFUNCTION(SetCK2dCurvePointBias), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "float GetTension() const", asMETHOD(CK2dCurvePoint, GetTension), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "void SetTension(float t)", asFUNCTION(SetCK2dCurvePointTension), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "float GetContinuity() const", asMETHOD(CK2dCurvePoint, GetContinuity), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "void SetContinuity(float c)", asFUNCTION(SetCK2dCurvePointContinuity), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurvePoint", "bool IsLinear() const", asFUNCTIONPR([](CK2dCurvePoint *self) -> bool { return self->IsLinear(); }, (CK2dCurvePoint *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "void SetLinear(bool linear = false)", asFUNCTION(SetCK2dCurvePointLinear), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "bool IsTCB() const", asFUNCTIONPR([](CK2dCurvePoint *self) -> bool { return self->IsTCB(); }, (CK2dCurvePoint *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "void UseTCB(bool use = true)", asFUNCTION(UseCK2dCurvePointTCB), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurvePoint", "float GetLength() const", asMETHOD(CK2dCurvePoint, GetLength), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "Vx2DVector &GetPosition()", asMETHOD(CK2dCurvePoint, GetPosition), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "void SetPosition(const Vx2DVector &in pos)", asFUNCTION(SetCK2dCurvePointPosition), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurvePoint", "Vx2DVector &GetInTangent()", asMETHOD(CK2dCurvePoint, GetInTangent), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "Vx2DVector &GetOutTangent()", asMETHOD(CK2dCurvePoint, GetOutTangent), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "void SetInTangent(const Vx2DVector &in input)", asFUNCTION(SetCK2dCurvePointInTangent), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurvePoint", "void SetOutTangent(const Vx2DVector &in output)", asFUNCTION(SetCK2dCurvePointOutTangent), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurvePoint", "void NotifyUpdate()", asFUNCTION(NotifyCK2dCurvePointUpdate), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CK2dCurve

static CK2dCurvePoint &GetCK2dCurveControlPoint(CK2dCurve *self, int pos) {
    static thread_local CK2dCurvePoint dummy;
    if (!self) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CK2dCurve is null.");
        }
        return dummy;
    }

    const int count = self->GetControlPointCount();
    if (pos < 0 || pos >= count) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CK2dCurve control point index out of range.");
        }
        return dummy;
    }
    return *self->GetControlPoint(pos);
}

static bool DeleteCK2dCurveControlPoint(CK2dCurve *self, int pos) {
    if (!self) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CK2dCurve is null.");
        }
        return false;
    }

    const int count = self->GetControlPointCount();
    if (pos < 0 || pos >= count) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CK2dCurve control point index out of range.");
        }
        return false;
    }
    self->DeleteControlPoint(self->GetControlPoint(pos));
    return true;
}

void RegisterCK2dCurve(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectBehaviour("CK2dCurve", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CK2dCurve *self) { new(self) CK2dCurve(); }, (CK2dCurve *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CK2dCurve", asBEHAVE_CONSTRUCT, "void f(const CK2dCurve &in other)", asFUNCTIONPR([](const CK2dCurve &c, CK2dCurve *self) { new(self) CK2dCurve(c); }, (const CK2dCurve &, CK2dCurve *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CK2dCurve", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CK2dCurve *self) { self->~CK2dCurve(); }, (CK2dCurve *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurve", "CK2dCurve &opAssign(const CK2dCurve &in other)", asMETHODPR(CK2dCurve, operator=, (const CK2dCurve &), CK2dCurve &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurve", "float GetLength()", asMETHOD(CK2dCurve, GetLength), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurve", "CKERROR GetPos(float step, Vx2DVector &out pos)", asMETHOD(CK2dCurve, GetPos), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurve", "float GetY(float x)", asMETHOD(CK2dCurve, GetY), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurve", "bool DeleteControlPoint(int pos)", asFUNCTION(DeleteCK2dCurveControlPoint), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurve", "void AddControlPoint(const Vx2DVector &in pos)", asMETHOD(CK2dCurve, AddControlPoint), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurve", "int GetControlPointCount()", asMETHOD(CK2dCurve, GetControlPointCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CK2dCurve", "CK2dCurvePoint &GetControlPoint(int pos)", asFUNCTION(GetCK2dCurveControlPoint), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CK2dCurve", "void Update()", asMETHOD(CK2dCurve, Update), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

}

// CKKeyframeData

template <typename T>
static void ConstructCKKeyValue(T *self) {
    new(self) T();
}

template <>
void ConstructCKKeyValue(CKMorphKey *self) {
    new(self) CKMorphKey();
    self->PosArray = nullptr;
    self->NormArray = nullptr;
}

template <typename T>
static void RegisterCKKeyMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    XString decl;

    r = engine->RegisterObjectProperty(name, "float TimeStep", asOFFSET(T, TimeStep)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstructCKKeyValue<T>, (T *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("void f(const %s &in other)", name);
    r = engine->RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, decl.CStr(), asFUNCTIONPR([](const T &k, T *self) { new(self) T(k); }, (const T &, T *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour(name, asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](T *self) { self->~T(); }, (T *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl.Format("%s &opAssign(const %s &in)", name, name);
    r = engine->RegisterObjectMethod(name, decl.CStr(), asMETHODPR(T, operator=, (const T &), T &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(name, "float GetTime()", asMETHODPR(T, GetTime, (), float), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "void SetTime(float t)", asMETHODPR(T, SetTime, (float), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    if (strcmp(name, "CKKey") != 0) {
        r = RegisterClassValueCast<T, CKKey>(engine, name, "CKKey"); CKAS_CHECK_REGISTER(r);
    }
}

template <typename T>
static void RegisterCKRotationKeyMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    XString decl;

    RegisterCKKeyMembers<T>(engine, name);

    r = engine->RegisterObjectProperty(name, "VxQuaternion Rot", asOFFSET(T, Rot)); CKAS_CHECK_REGISTER(r);


    r = engine->RegisterObjectMethod(name, "const VxQuaternion &GetRotation()", asMETHODPR(T, GetRotation, (), const VxQuaternion &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "void SetRotation(const VxQuaternion &in)", asMETHODPR(T, SetRotation, (const VxQuaternion &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool Compare(%s &in key, float threshold)", name);
    r = engine->RegisterObjectMethod(name, decl.CStr(), asFUNCTIONPR([](T *self, T &key, float threshold) -> bool { return self->Compare(key, threshold); }, (T *, T &, float), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    if (strcmp(name, "CKRotationKey") != 0) {
        r = RegisterClassValueCast<T, CKRotationKey>(engine, name, "CKRotationKey"); CKAS_CHECK_REGISTER(r);
    }
}

template <typename T>
static void RegisterCKPositionKeyMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    XString decl;

    RegisterCKKeyMembers<T>(engine, name);

    r = engine->RegisterObjectProperty(name, "VxVector Pos", asOFFSET(T, Pos)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(name, "const VxVector &GetPosition()", asMETHODPR(T, GetPosition, (), const VxVector &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "void SetPosition(const VxVector &in)", asMETHODPR(T, SetPosition, (const VxVector &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    decl.Format("bool Compare(%s &in key, float threshold)", name);
    r = engine->RegisterObjectMethod(name, decl.CStr(), asFUNCTIONPR([](T *self, T &key, float threshold) -> bool { return self->Compare(key, threshold); }, (T *, T &, float), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    if (strcmp(name, "CKPositionKey") != 0) {
        r = RegisterClassValueCast<T, CKPositionKey>(engine, name, "CKPositionKey"); CKAS_CHECK_REGISTER(r);
    }
}

static CKKey &InvalidCKKey(const char *message) {
    static thread_local CKKey dummy;
    dummy = CKKey();
    if (message) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException(message);
        }
    }
    return dummy;
}

static void SetCKAnimControllerException(const char *message) {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message);
    }
}

static bool IsCKAnimControllerVectorType(CKDWORD type) {
    const CKDWORD baseType = type & CKANIMATION_CONTROLLER_MASK;
    return baseType == CKANIMATION_CONTROLLER_POS || baseType == CKANIMATION_CONTROLLER_SCL;
}

static bool IsCKAnimControllerQuaternionType(CKDWORD type) {
    const CKDWORD baseType = type & CKANIMATION_CONTROLLER_MASK;
    return baseType == CKANIMATION_CONTROLLER_ROT || baseType == CKANIMATION_CONTROLLER_SCLAXIS;
}

static bool IsCKAnimControllerExactPositionKeyType(CKDWORD type) {
    return type == CKANIMATION_LINPOS_CONTROL || type == CKANIMATION_LINSCL_CONTROL;
}

static bool IsCKAnimControllerExactRotationKeyType(CKDWORD type) {
    return type == CKANIMATION_LINROT_CONTROL || type == CKANIMATION_LINSCLAXIS_CONTROL;
}

static bool IsCKAnimControllerExactTCBPositionKeyType(CKDWORD type) {
    return type == CKANIMATION_TCBPOS_CONTROL || type == CKANIMATION_TCBSCL_CONTROL;
}

static bool IsCKAnimControllerExactTCBRotationKeyType(CKDWORD type) {
    return type == CKANIMATION_TCBROT_CONTROL || type == CKANIMATION_TCBSCLAXIS_CONTROL;
}

static bool IsCKAnimControllerExactBezierPositionKeyType(CKDWORD type) {
    return type == CKANIMATION_BEZIERPOS_CONTROL || type == CKANIMATION_BEZIERSCL_CONTROL;
}

template<typename T>
static CKKey &GetCKAnimControllerKey(T *self, int index) {
    if (!self) {
        return InvalidCKKey("CKAnimController.GetKey called with a null controller.");
    }
    if (index < 0 || index >= self->GetKeyCount()) {
        return InvalidCKKey("CKAnimController.GetKey index out of range.");
    }
    CKKey *key = self->GetKey(index);
    if (!key) {
        return InvalidCKKey("CKAnimController.GetKey returned a null native key.");
    }
    return *key;
}

template<typename T>
static bool EvaluateCKAnimControllerVector(T *self, float timeStep, VxVector &result) {
    if (!self) {
        SetCKAnimControllerException("CKAnimController.EvaluateVector called with a null controller.");
        return false;
    }
    if (!IsCKAnimControllerVectorType(self->GetType())) {
        SetCKAnimControllerException("CKAnimController.EvaluateVector requires a position or scale controller.");
        return false;
    }
    return self->Evaluate(timeStep, &result) != FALSE;
}

template<typename T>
static bool EvaluateCKAnimControllerQuaternion(T *self, float timeStep, VxQuaternion &result) {
    if (!self) {
        SetCKAnimControllerException("CKAnimController.EvaluateQuaternion called with a null controller.");
        return false;
    }
    if (!IsCKAnimControllerQuaternionType(self->GetType())) {
        SetCKAnimControllerException("CKAnimController.EvaluateQuaternion requires a rotation or scale-axis controller.");
        return false;
    }
    return self->Evaluate(timeStep, &result) != FALSE;
}

template<typename T, typename KeyT, bool (*AcceptType)(CKDWORD)>
static int AddTypedCKAnimControllerKey(T *self, KeyT &key) {
    if (!self) {
        SetCKAnimControllerException("CKAnimController typed AddKey called with a null controller.");
        return -1;
    }
    if (!AcceptType(self->GetType())) {
        SetCKAnimControllerException("CKAnimController typed AddKey received a key incompatible with the controller type.");
        return -1;
    }
    return self->AddKey(&key);
}

template<typename T>
static int DumpCKAnimControllerKeysTo(T *self, NativeBuffer *buffer) {
    if (!self) {
        SetCKAnimControllerException("CKAnimController.DumpKeysTo called with a null controller.");
        return 0;
    }

    const int required = self->DumpKeysTo(nullptr);
    if (!buffer) {
        return required;
    }
    if (required < 0) {
        return required;
    }
    if (buffer->Size() < static_cast<size_t>(required)) {
        SetCKAnimControllerException("CKAnimController.DumpKeysTo requires a NativeBuffer large enough for the serialized keys.");
        return 0;
    }
    return self->DumpKeysTo(buffer->Data());
}

template<typename T>
static NativeBuffer *DumpCKAnimControllerKeys(T *self) {
    if (!self) {
        SetCKAnimControllerException("CKAnimController.DumpKeys called with a null controller.");
        return nullptr;
    }

    const int required = self->DumpKeysTo(nullptr);
    if (required < 0) {
        return nullptr;
    }

    NativeBuffer *buffer = NativeBuffer::Create(static_cast<size_t>(required));
    if (!buffer || (required > 0 && !buffer->Data())) {
        if (buffer) {
            buffer->Release();
        }
        return nullptr;
    }

    const int written = self->DumpKeysTo(buffer->Data());
    if (written < 0 || written > required) {
        buffer->Release();
        return nullptr;
    }
    return buffer;
}

template<typename T>
static int ReadCKAnimControllerKeysFrom(T *self, NativeBuffer *buffer) {
    if (!self) {
        SetCKAnimControllerException("CKAnimController.ReadKeysFrom called with a null controller.");
        return 0;
    }
    if (!buffer || !buffer->IsValid()) {
        SetCKAnimControllerException("CKAnimController.ReadKeysFrom requires a valid NativeBuffer.");
        return 0;
    }
    return self->ReadKeysFrom(buffer->Data());
}

template<typename T>
static bool CompareCKAnimController(T *self, CKAnimController *control, float threshold) {
    if (!self) {
        SetCKAnimControllerException("CKAnimController.Compare called with a null controller.");
        return false;
    }
    if (!control) {
        SetCKAnimControllerException("CKAnimController.Compare requires a non-null controller.");
        return false;
    }
    return self->Compare(control, threshold) != FALSE;
}

template<typename T>
static bool CloneCKAnimController(T *self, CKAnimController *control) {
    if (!self) {
        SetCKAnimControllerException("CKAnimController.Clone called with a null controller.");
        return false;
    }
    if (!control) {
        SetCKAnimControllerException("CKAnimController.Clone requires a non-null controller.");
        return false;
    }
    return self->Clone(control) != FALSE;
}

template<typename T>
void RegisterCKAnimControllerMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    if constexpr (!std::is_same_v<T, CKMorphController>) {
        r = engine->RegisterObjectMethod(name, "bool EvaluateVector(float timeStep, VxVector &out result)", asFUNCTIONPR(EvaluateCKAnimControllerVector<T>, (T *, float, VxVector &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterObjectMethod(name, "bool EvaluateQuaternion(float timeStep, VxQuaternion &out result)", asFUNCTIONPR(EvaluateCKAnimControllerQuaternion<T>, (T *, float, VxQuaternion &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterObjectMethod(name, "int AddPositionKey(CKPositionKey &in key)", asFUNCTIONPR((AddTypedCKAnimControllerKey<T, CKPositionKey, IsCKAnimControllerExactPositionKeyType>), (T *, CKPositionKey &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterObjectMethod(name, "int AddRotationKey(CKRotationKey &in key)", asFUNCTIONPR((AddTypedCKAnimControllerKey<T, CKRotationKey, IsCKAnimControllerExactRotationKeyType>), (T *, CKRotationKey &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterObjectMethod(name, "int AddTCBPositionKey(CKTCBPositionKey &in key)", asFUNCTIONPR((AddTypedCKAnimControllerKey<T, CKTCBPositionKey, IsCKAnimControllerExactTCBPositionKeyType>), (T *, CKTCBPositionKey &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterObjectMethod(name, "int AddTCBRotationKey(CKTCBRotationKey &in key)", asFUNCTIONPR((AddTypedCKAnimControllerKey<T, CKTCBRotationKey, IsCKAnimControllerExactTCBRotationKeyType>), (T *, CKTCBRotationKey &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterObjectMethod(name, "int AddBezierPositionKey(CKBezierPositionKey &in key)", asFUNCTIONPR((AddTypedCKAnimControllerKey<T, CKBezierPositionKey, IsCKAnimControllerExactBezierPositionKeyType>), (T *, CKBezierPositionKey &), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterObjectMethod(name, "CKKey &GetKey(int index)", asFUNCTIONPR(GetCKAnimControllerKey<T>, (T *, int), CKKey &), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    }
    r = engine->RegisterObjectMethod(name, "void RemoveKey(int index)", asMETHODPR(T, RemoveKey, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "int DumpKeysTo(NativeBuffer@ buffer = null)", asFUNCTIONPR(DumpCKAnimControllerKeysTo<T>, (T *, NativeBuffer *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "NativeBuffer@ DumpKeys()", asFUNCTIONPR(DumpCKAnimControllerKeys<T>, (T *), NativeBuffer *), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "int ReadKeysFrom(NativeBuffer@ buffer)", asFUNCTIONPR(ReadCKAnimControllerKeysFrom<T>, (T *, NativeBuffer *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "bool Compare(CKAnimController@ control, float threshold = 0.0)", asFUNCTIONPR(CompareCKAnimController<T>, (T *, CKAnimController *, float), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "bool Clone(CKAnimController@ control)", asFUNCTIONPR(CloneCKAnimController<T>, (T *, CKAnimController *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod(name, "int GetKeyCount()", asMETHODPR(T, GetKeyCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "CKDWORD GetType()", asMETHODPR(T, GetType, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "void SetLength(float l)", asMETHODPR(T, SetLength, (float), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(name, "float GetLength()", asMETHODPR(T, GetLength, (), float), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    if (strcmp(name, "CKAnimController") != 0) {
        r = RegisterClassRefCast<T, CKAnimController>(engine, name, "CKAnimController"); CKAS_CHECK_REGISTER(r);
    }
}

static NativePointer GetCKMorphKeyPosArray(CKMorphKey *self) {
    return NativePointer(self ? self->PosArray : nullptr);
}

static void SetCKMorphKeyPosArray(CKMorphKey *self, NativePointer pointer) {
    if (self) {
        self->PosArray = reinterpret_cast<VxVector *>(pointer.Get());
    }
}

static NativePointer GetCKMorphKeyNormArray(CKMorphKey *self) {
    return NativePointer(self ? self->NormArray : nullptr);
}

static void SetCKMorphKeyNormArray(CKMorphKey *self, NativePointer pointer) {
    if (self) {
        self->NormArray = reinterpret_cast<VxCompressedVector *>(pointer.Get());
    }
}

static bool CompareCKMorphKey(CKMorphKey *self, CKMorphKey &key, int nbVertex, float threshold) {
    if (!self) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKMorphKey.Compare called with a null self pointer.");
        }
        return false;
    }
    if (nbVertex < 0) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKMorphKey.Compare requires a non-negative vertex count.");
        }
        return false;
    }
    if (nbVertex > 0) {
        const bool hasPositionArrays = self->PosArray && key.PosArray;
        const bool hasNormalArrays = self->NormArray && key.NormArray;
        if ((self->PosArray || key.PosArray) && !hasPositionArrays) {
            return false;
        }
        if ((self->NormArray || key.NormArray) && !hasNormalArrays) {
            return false;
        }
    }
    return self->Compare(key, nbVertex, threshold) != FALSE;
}

static CKMorphKey &InvalidCKMorphKey(const char *message) {
    static thread_local CKMorphKey dummy;
    dummy = CKMorphKey();
    dummy.PosArray = nullptr;
    dummy.NormArray = nullptr;
    if (message) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException(message);
        }
    }
    return dummy;
}

static int AddCKMorphControllerKey(CKMorphController *self, CKMorphKey &key, bool allocateNormals) {
    if (!self) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKMorphController.AddMorphKey called with a null controller.");
        }
        return -1;
    }
    if (!key.PosArray) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKMorphController.AddMorphKey requires CKMorphKey.PosArray to reference morph vertex data.");
        }
        return -1;
    }
    return self->AddKey(&key, allocateNormals);
}

static CKMorphKey &GetCKMorphControllerKey(CKMorphController *self, int index) {
    if (!self) {
        return InvalidCKMorphKey("CKMorphController.GetMorphKey called with a null controller.");
    }
    if (index < 0 || index >= self->GetKeyCount()) {
        return InvalidCKMorphKey("CKMorphController.GetMorphKey index out of range.");
    }
    CKKey *key = self->GetKey(index);
    if (!key) {
        return InvalidCKMorphKey("CKMorphController.GetMorphKey returned a null native key.");
    }
    return *static_cast<CKMorphKey *>(key);
}

static bool EvaluateCKMorphController(CKMorphController *self,
                                      float timeStep,
                                      int vertexCount,
                                      NativeBuffer *vertices,
                                      CKDWORD vertexStride,
                                      NativeBuffer *normals) {
    if (!self) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKMorphController.Evaluate called with a null controller.");
        }
        return false;
    }
    if (vertexCount < 0) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKMorphController.Evaluate requires a non-negative vertex count.");
        }
        return false;
    }
    if (vertexCount > 0 && !vertices) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKMorphController.Evaluate requires a vertex output buffer when vertexCount is positive.");
        }
        return false;
    }
    if (vertexCount > 0 && vertexStride < sizeof(VxVector)) {
        if (asIScriptContext *ctx = asGetActiveContext()) {
            ctx->SetException("CKMorphController.Evaluate vertexStride is smaller than VxVector.");
        }
        return false;
    }
    if (vertexCount > 0) {
        const size_t requiredVertexBytes = static_cast<size_t>(vertexCount - 1) * vertexStride + sizeof(VxVector);
        if (!vertices->IsValid() || vertices->Size() < requiredVertexBytes) {
            if (asIScriptContext *ctx = asGetActiveContext()) {
                ctx->SetException("CKMorphController.Evaluate vertex output buffer is too small.");
            }
            return false;
        }
        if (normals) {
            const size_t requiredNormalBytes = static_cast<size_t>(vertexCount) * sizeof(VxCompressedVector);
            if (!normals->IsValid() || normals->Size() < requiredNormalBytes) {
                if (asIScriptContext *ctx = asGetActiveContext()) {
                    ctx->SetException("CKMorphController.Evaluate normal output buffer is too small.");
                }
                return false;
            }
        }
    }
    return self->Evaluate(timeStep,
                          vertexCount,
                          vertices ? vertices->Data() : nullptr,
                          vertexStride,
                          normals ? reinterpret_cast<VxCompressedVector *>(normals->Data()) : nullptr) != FALSE;
}

void RegisterCKKeyframeData(asIScriptEngine *engine) {
    int r = 0;

    // CKKey
    RegisterCKKeyMembers<CKKey>(engine, "CKKey");

    // CKRotationKey
    RegisterCKRotationKeyMembers<CKRotationKey>(engine, "CKRotationKey");

    r = engine->RegisterObjectBehaviour("CKRotationKey", asBEHAVE_CONSTRUCT, "void f(float time, VxQuaternion &in rot)", asFUNCTIONPR([](CKRotationKey *self, float time, VxQuaternion &rot) { new (self) CKRotationKey(time, rot); }, (CKRotationKey *, float, VxQuaternion &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // CKPositionKey
    RegisterCKPositionKeyMembers<CKPositionKey>(engine, "CKPositionKey");

    r = engine->RegisterObjectBehaviour("CKPositionKey", asBEHAVE_CONSTRUCT, "void f(float time, VxVector &in pos)", asFUNCTIONPR([](CKPositionKey *self, float time, VxVector &pos) { new (self) CKPositionKey(time, pos); }, (CKPositionKey *, float, VxVector &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // CKTCBPositionKey
    RegisterCKPositionKeyMembers<CKTCBPositionKey>(engine, "CKTCBPositionKey");

    r = engine->RegisterObjectProperty("CKTCBPositionKey", "float tension", asOFFSET(CKTCBPositionKey, tension)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKTCBPositionKey", "float continuity", asOFFSET(CKTCBPositionKey, continuity)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKTCBPositionKey", "float bias", asOFFSET(CKTCBPositionKey, bias)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKTCBPositionKey", "float easeto", asOFFSET(CKTCBPositionKey, easeto)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKTCBPositionKey", "float easefrom", asOFFSET(CKTCBPositionKey, easefrom)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKTCBPositionKey", asBEHAVE_CONSTRUCT, "void f(float time, VxVector &in pos, float t = 0, float c = 0, float b = 0, float easeTo = 0, float easeFrom = 0)", asFUNCTIONPR([](CKTCBPositionKey *self, float time, VxVector &pos, float t, float c, float b, float easeTo, float easeFrom) { new (self) CKTCBPositionKey(time, pos, t, c, b, easeTo, easeFrom); }, (CKTCBPositionKey *, float, VxVector &, float, float, float, float, float), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // CKRotationKey
    RegisterCKRotationKeyMembers<CKTCBRotationKey>(engine, "CKTCBRotationKey");

    r = engine->RegisterObjectProperty("CKTCBRotationKey", "float tension", asOFFSET(CKTCBRotationKey, tension)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKTCBRotationKey", "float continuity", asOFFSET(CKTCBRotationKey, continuity)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKTCBRotationKey", "float bias", asOFFSET(CKTCBRotationKey, bias)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKTCBRotationKey", "float easeto", asOFFSET(CKTCBRotationKey, easeto)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKTCBRotationKey", "float easefrom", asOFFSET(CKTCBRotationKey, easefrom)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKTCBRotationKey", asBEHAVE_CONSTRUCT, "void f(float time, VxQuaternion &in rot, float t = 0, float c = 0, float b = 0, float easeTo = 0, float easeFrom = 0)", asFUNCTIONPR([](CKTCBRotationKey *self, float time, VxQuaternion &rot, float t, float c, float b, float easeTo, float easeFrom) { new (self) CKTCBRotationKey(time, rot, t, c, b, easeTo, easeFrom); }, (CKTCBRotationKey *, float, VxQuaternion &, float, float, float, float, float), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // CKBezierKeyFlags
    r = engine->RegisterObjectBehaviour("CKBezierKeyFlags", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKBezierKeyFlags *self) { new(self) CKBezierKeyFlags(); }, (CKBezierKeyFlags *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKBezierKeyFlags", asBEHAVE_CONSTRUCT, "void f(const CKBezierKeyFlags &in other)", asFUNCTIONPR([](const CKBezierKeyFlags &f, CKBezierKeyFlags *self) { new(self) CKBezierKeyFlags(f); }, (const CKBezierKeyFlags &, CKBezierKeyFlags *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKBezierKeyFlags", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKBezierKeyFlags *self) { self->~CKBezierKeyFlags(); }, (CKBezierKeyFlags *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBezierKeyFlags", "CKBezierKeyFlags &opAssign(const CKBezierKeyFlags &in other)", asMETHODPR(CKBezierKeyFlags, operator=, (const CKBezierKeyFlags &), CKBezierKeyFlags &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBezierKeyFlags", "bool opIneq(const CKBezierKeyFlags &in) const", asFUNCTIONPR([](const CKBezierKeyFlags &lhs, const CKBezierKeyFlags &rhs) -> bool { return lhs != rhs; }, (const CKBezierKeyFlags &, const CKBezierKeyFlags &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBezierKeyFlags", "CKBEZIERKEY_FLAGS GetInTangentMode()", asMETHODPR(CKBezierKeyFlags, GetInTangentMode, (), CKBEZIERKEY_FLAGS), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBezierKeyFlags", "CKBEZIERKEY_FLAGS GetOutTangentMode()", asMETHODPR(CKBezierKeyFlags, GetOutTangentMode, (), CKBEZIERKEY_FLAGS), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBezierKeyFlags", "void SetInTangentMode(CKBEZIERKEY_FLAGS f)", asMETHODPR(CKBezierKeyFlags, SetInTangentMode, (CKBEZIERKEY_FLAGS), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKBezierKeyFlags", "void SetOutTangentMode(CKBEZIERKEY_FLAGS f)", asMETHODPR(CKBezierKeyFlags, SetOutTangentMode, (CKBEZIERKEY_FLAGS), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    // CKBezierPositionKey
    RegisterCKPositionKeyMembers<CKBezierPositionKey>(engine, "CKBezierPositionKey");

    r = engine->RegisterObjectProperty("CKBezierPositionKey", "CKBezierKeyFlags Flags", asOFFSET(CKBezierPositionKey, Flags)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBezierPositionKey", "VxVector In", asOFFSET(CKBezierPositionKey, In)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKBezierPositionKey", "VxVector Out", asOFFSET(CKBezierPositionKey, Out)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKBezierPositionKey", asBEHAVE_CONSTRUCT, "void f(float time, VxVector &in pos, const CKBezierKeyFlags &in flags, VxVector &in input, VxVector &in output)", asFUNCTIONPR([](CKBezierPositionKey *self, float time, VxVector &pos, const CKBezierKeyFlags &flags, VxVector &in, VxVector &out) { new (self) CKBezierPositionKey(time, pos, flags, in, out); }, (CKBezierPositionKey *, float, VxVector &, const CKBezierKeyFlags &, VxVector &, VxVector &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKBezierPositionKey", asBEHAVE_CONSTRUCT, "void f(float time, VxVector &in pos, const CKBezierKeyFlags &in flags)", asFUNCTIONPR([](CKBezierPositionKey *self, float time, VxVector &pos, const CKBezierKeyFlags &flags) { new (self) CKBezierPositionKey(time, pos, flags); }, (CKBezierPositionKey *, float, VxVector &, const CKBezierKeyFlags &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKBezierPositionKey", asBEHAVE_CONSTRUCT, "void f(float time, VxVector &in pos, VxVector &in input, VxVector &in output)", asFUNCTIONPR([](CKBezierPositionKey *self, float time, VxVector &pos, VxVector &in, VxVector &out) { new (self) CKBezierPositionKey(time, pos, in, out); }, (CKBezierPositionKey *, float, VxVector &, VxVector &, VxVector &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKBezierPositionKey", "CKBezierKeyFlags &GetFlags()", asMETHODPR(CKBezierPositionKey, GetFlags, (), CKBezierKeyFlags &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    // CKMorphKey
    RegisterCKKeyMembers<CKMorphKey>(engine, "CKMorphKey");

    r = engine->RegisterObjectMethod("CKMorphKey", "NativePointer GetPosArray() const", asFUNCTION(GetCKMorphKeyPosArray), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMorphKey", "void SetPosArray(NativePointer pointer)", asFUNCTION(SetCKMorphKeyPosArray), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMorphKey", "NativePointer GetNormArray() const", asFUNCTION(GetCKMorphKeyNormArray), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMorphKey", "void SetNormArray(NativePointer pointer)", asFUNCTION(SetCKMorphKeyNormArray), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKMorphKey", "bool Compare(CKMorphKey &in key, int nbVertex, float threshold)", asFUNCTION(CompareCKMorphKey), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // CKAnimController
    RegisterCKAnimControllerMembers<CKAnimController>(engine, "CKAnimController");

    // CKMorphController
    RegisterCKAnimControllerMembers<CKMorphController>(engine, "CKMorphController");

    r = engine->RegisterObjectMethod("CKMorphController", "int AddKey(float timeStep, bool allocateNormals)", asFUNCTIONPR([](CKMorphController *self, float timeStep, bool allocateNormals) { return self->AddKey(timeStep, allocateNormals); }, (CKMorphController *, float, bool), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMorphController", "int AddMorphKey(CKMorphKey &in key, bool allocateNormals = true)", asFUNCTION(AddCKMorphControllerKey), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMorphController", "CKMorphKey &GetMorphKey(int index)", asFUNCTION(GetCKMorphControllerKey), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMorphController", "bool Evaluate(float timeStep, int vertexCount, NativeBuffer@ vertices, CKDWORD vertexStride, NativeBuffer@ normals = null)", asFUNCTION(EvaluateCKMorphController), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKMorphController", "void SetMorphVertexCount(int count)", asMETHODPR(CKMorphController, SetMorphVertexCount, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKAnimKey

void RegisterCKAnimKey(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKAnimKey", "VxVector m_Pos", asOFFSET(CKAnimKey, m_Pos)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKAnimKey", "VxVector m_Scl", asOFFSET(CKAnimKey, m_Scl)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKAnimKey", "VxQuaternion m_Rot", asOFFSET(CKAnimKey, m_Rot)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKAnimKey", "VxQuaternion m_SclRot", asOFFSET(CKAnimKey, m_SclRot)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKAnimKey", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKAnimKey *self) { new(self) CKAnimKey(); }, (CKAnimKey *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKAnimKey", asBEHAVE_CONSTRUCT, "void f(const CKAnimKey &in other)", asFUNCTIONPR([](const CKAnimKey &k, CKAnimKey *self) { new(self) CKAnimKey(k); }, (const CKAnimKey &, CKAnimKey *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKAnimKey", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKAnimKey *self) { self->~CKAnimKey(); }, (CKAnimKey *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKAnimKey", "CKAnimKey &opAssign(const CKAnimKey &in other)", asMETHODPR(CKAnimKey, operator=, (const CKAnimKey &), CKAnimKey &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKSceneObjectDesc

void RegisterCKSceneObjectDesc(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKSceneObjectDesc", "CK_ID m_Object", asOFFSET(CKSceneObjectDesc, m_Object)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKSceneObjectDesc", "CKStateChunk@ m_InitialValue", asOFFSET(CKSceneObjectDesc, m_InitialValue)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKSceneObjectDesc", "CKDWORD m_Flags", asOFFSET(CKSceneObjectDesc, m_Flags)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKSceneObjectDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKSceneObjectDesc *self) { new(self) CKSceneObjectDesc(); }, (CKSceneObjectDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectBehaviour("CKSceneObjectDesc", asBEHAVE_CONSTRUCT, "void f(CKObject@ obj, CKStateChunk@ initialValue = null, CKDWORD flags = 0)", asFUNCTIONPR([](CKObject *obj, CKStateChunk *initialValue, CKDWORD flags, CKSceneObjectDesc *self) { new(self) CKSceneObjectDesc(obj, initialValue, flags); }, (CKObject *, CKStateChunk *, CKDWORD, CKSceneObjectDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectBehaviour("CKSceneObjectDesc", asBEHAVE_CONSTRUCT, "void f(const CKSceneObjectDesc &in other)", asFUNCTIONPR([](const CKSceneObjectDesc &k, CKSceneObjectDesc *self) { new(self) CKSceneObjectDesc(k); }, (const CKSceneObjectDesc &, CKSceneObjectDesc *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKSceneObjectDesc", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKSceneObjectDesc *self) { self->~CKSceneObjectDesc(); }, (CKSceneObjectDesc *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKSceneObjectDesc", "CKSceneObjectDesc &opAssign(const CKSceneObjectDesc &in other)", asMETHODPR(CKSceneObjectDesc, operator=, (const CKSceneObjectDesc &), CKSceneObjectDesc &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKSceneObjectDesc", "bool opEquals(const CKSceneObjectDesc &in other) const", asFUNCTIONPR([](const CKSceneObjectDesc &lhs, const CKSceneObjectDesc &rhs) -> bool { return lhs == rhs; }, (const CKSceneObjectDesc &, const CKSceneObjectDesc &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif

    // r = engine->RegisterObjectMethod("CKSceneObjectDesc", "CKERROR ReadState(CKStateChunk@ chunk)", asMETHODPR(CKSceneObjectDesc, ReadState, (CKStateChunk*), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKSceneObjectDesc", "int GetSize()", asMETHODPR(CKSceneObjectDesc, GetSize, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKSceneObjectDesc", "void Clear()", asMETHODPR(CKSceneObjectDesc, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKSceneObjectDesc", "void Init(CKObject@ obj = null)", asMETHODPR(CKSceneObjectDesc, Init, (CKObject *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKSceneObjectDesc", "CKDWORD ActiveAtStart()", asMETHODPR(CKSceneObjectDesc, ActiveAtStart, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSceneObjectDesc", "CKDWORD DeActiveAtStart()", asMETHODPR(CKSceneObjectDesc, DeActiveAtStart, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSceneObjectDesc", "CKDWORD NothingAtStart()", asMETHODPR(CKSceneObjectDesc, NothingAtStart, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSceneObjectDesc", "CKDWORD ResetAtStart()", asMETHODPR(CKSceneObjectDesc, ResetAtStart, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSceneObjectDesc", "CKDWORD IsActive()", asMETHODPR(CKSceneObjectDesc, IsActive, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif
}

// CKPatch

static short int CKPatchGetV(CKPatch &patch, int index) {
    if (index < 0 || index >= 4) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Index out of range for 'v'");
        return 0;
    }
    return patch.v[index];
}

static void CKPatchSetV(CKPatch &patch, int index, short int value) {
    if (index < 0 || index >= 4) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Index out of range for 'v'");
        return;
    }
    patch.v[index] = value;
}

static short int CKPatchGetVec(CKPatch &patch, int index) {
    if (index < 0 || index >= 8) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Index out of range for 'vec'");
        return 0;
    }
    return patch.vec[index];
}

static void CKPatchSetVec(CKPatch &patch, int index, short int value) {
    if (index < 0 || index >= 8) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Index out of range for 'vec'");
        return;
    }
    patch.vec[index] = value;
}

static short int CKPatchGetInterior(CKPatch &patch, int index) {
    if (index < 0 || index >= 4) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Index out of range for 'interior'");
        return 0;
    }
    return patch.interior[index];
}

static void CKPatchSetInterior(CKPatch &patch, int index, short int value) {
    if (index < 0 || index >= 4) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Index out of range for 'interior'");
        return;
    }
    patch.interior[index] = value;
}

static short int CKPatchGetEdge(CKPatch &patch, int index) {
    if (index < 0 || index >= 4) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Index out of range for 'edge'");
        return 0;
    }
    return patch.edge[index];
}

static void CKPatchSetEdge(CKPatch &patch, int index, short int value) {
    if (index < 0 || index >= 4) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Index out of range for 'edge'");
        return;
    }
    patch.edge[index] = value;
}

void RegisterCKPatch(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKPatch", "CKDWORD type", asOFFSET(CKPatch, type)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPatch", "CKDWORD SmoothingGroup", asOFFSET(CKPatch, SmoothingGroup)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPatch", "CK_ID Material", asOFFSET(CKPatch, Material)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPatch", "VxVector &auxs", asOFFSET(CKPatch, auxs)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPatch", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKPatch *self) { new(self) CKPatch(); }, (CKPatch *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKPatch", asBEHAVE_CONSTRUCT, "void f(const CKPatch &in other)", asFUNCTIONPR([](const CKPatch &p, CKPatch *self) { new(self) CKPatch(p); }, (const CKPatch &, CKPatch *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKPatch", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPatch *self) { self->~CKPatch(); }, (CKPatch *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPatch", "CKPatch &opAssign(const CKPatch &in other)", asMETHODPR(CKPatch, operator=, (const CKPatch &), CKPatch &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPatch", "int16 GetV(int index)", asFUNCTION(CKPatchGetV), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPatch", "void SetV(int index, int16 value)", asFUNCTION(CKPatchSetV), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPatch", "int16 GetVec(int index)", asFUNCTION(CKPatchGetVec), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPatch", "void SetVec(int index, int16 value)", asFUNCTION(CKPatchSetVec), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPatch", "int16 GetInterior(int index)", asFUNCTION(CKPatchGetInterior), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPatch", "void SetInterior(int index, int16 value)", asFUNCTION(CKPatchSetInterior), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKPatch", "int16 GetEdge(int index)", asFUNCTION(CKPatchGetEdge), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKPatch", "void SetEdge(int index, int16 value)", asFUNCTION(CKPatchSetEdge), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
}

void RegisterCKPatchEdge(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKPatchEdge", "int16 v1", asOFFSET(CKPatchEdge, v1)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPatchEdge", "int16 vec12", asOFFSET(CKPatchEdge, vec12)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPatchEdge", "int16 vec21", asOFFSET(CKPatchEdge, vec21)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPatchEdge", "int16 v2", asOFFSET(CKPatchEdge, v2)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPatchEdge", "int16 patch1", asOFFSET(CKPatchEdge, patch1)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKPatchEdge", "int16 patch2", asOFFSET(CKPatchEdge, patch2)); CKAS_CHECK_REGISTER(r);
}

static short int CKTVPatchGetTV(CKTVPatch &patch, int index) {
    if (index < 0 || index >= 4) {
        // Set a script exception
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Out of range");
        return 0;
    }
    return patch.tv[index];
}

static void CKTVPatchSetTV(CKTVPatch &patch, int index, short int tv) {
    if (index < 0 || index >= 4) {
        // Set a script exception
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx)
            ctx->SetException("Out of range");
        return;
    }
    patch.tv[index] = tv;
}

void RegisterCKTVPatch(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectMethod("CKTVPatch", "int16 GetTV(int index)", asFUNCTION(CKTVPatchGetTV), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKTVPatch", "void SetTV(int index, int16 value)", asFUNCTION(CKTVPatchSetTV), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKSkinBoneData

void RegisterCKSkinBoneData(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectMethod("CKSkinBoneData", "void SetBone(CK3dEntity@ ent)", asMETHOD(CKSkinBoneData, SetBone), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkinBoneData", "CK3dEntity@ GetBone() const", asMETHOD(CKSkinBoneData, GetBone), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkinBoneData", "void SetBoneInitialInverseMatrix(const VxMatrix &in mat)", asMETHOD(CKSkinBoneData, SetBoneInitialInverseMatrix), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKSkinVertexData

void RegisterCKSkinVertexData(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectMethod("CKSkinVertexData", "void SetBoneCount(int boneCount)", asMETHOD(CKSkinVertexData, SetBoneCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkinVertexData", "int GetBoneCount() const", asMETHOD(CKSkinVertexData, GetBoneCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkinVertexData", "int GetBone(int n) const", asMETHOD(CKSkinVertexData, GetBone), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkinVertexData", "void SetBone(int n, int boneIdx)", asMETHOD(CKSkinVertexData, SetBone), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkinVertexData", "float GetWeight(int n) const", asMETHOD(CKSkinVertexData, GetWeight), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkinVertexData", "void SetWeight(int n, float weight)", asMETHOD(CKSkinVertexData, SetWeight), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkinVertexData", "VxVector &GetInitialPos()", asMETHOD(CKSkinVertexData, GetInitialPos), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkinVertexData", "void SetInitialPos(const VxVector &in pos)", asMETHOD(CKSkinVertexData, SetInitialPos), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKSkin

void RegisterCKSkin(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectMethod("CKSkin", "void SetObjectInitMatrix(const VxMatrix &in mat)", asMETHOD(CKSkin, SetObjectInitMatrix), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "void SetBoneCount(int boneCount)", asMETHOD(CKSkin, SetBoneCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "void SetVertexCount(int count)", asMETHOD(CKSkin, SetVertexCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "int GetBoneCount() const", asMETHOD(CKSkin, GetBoneCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "int GetVertexCount() const", asMETHOD(CKSkin, GetVertexCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "void ConstructBoneTransfoMatrices(CKContext@ context)", asMETHOD(CKSkin, ConstructBoneTransfoMatrices), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "bool CalcPoints(int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride)", asFUNCTIONPR([](CKSkin *self, int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride) -> bool { return self->CalcPoints(vertexCount, reinterpret_cast<CKBYTE *>(vertexPtr.Get()), vertexStride); }, (CKSkin *, int, NativePointer, CKDWORD), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "CKSkinBoneData@ GetBoneData(int boneIdx) const", asMETHOD(CKSkin, GetBoneData), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "CKSkinVertexData@ GetVertexData(int vertexIdx) const", asMETHOD(CKSkin, GetVertexData), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "void RemapVertices(const XIntArray &in permutation)", asMETHOD(CKSkin, RemapVertices), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "void SetNormalCount(int count)", asMETHOD(CKSkin, SetNormalCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "int GetNormalCount() const", asMETHOD(CKSkin, GetNormalCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "void SetNormal(int index, const VxVector &in norm)", asMETHOD(CKSkin, SetNormal), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "VxVector &GetNormal(int index)", asMETHOD(CKSkin, GetNormal), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKSkin", "bool CalcPoints(int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride, NativePointer normalPtr, CKDWORD normalStride)", asFUNCTIONPR([](CKSkin *self, int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride, NativePointer normalPtr, CKDWORD normalStride) -> bool { return self->CalcPointsEx(vertexCount, reinterpret_cast<CKBYTE *>(vertexPtr.Get()), vertexStride, reinterpret_cast<CKBYTE *>(normalPtr.Get()), normalStride); }, (CKSkin *, int, NativePointer, CKDWORD, NativePointer, CKDWORD), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKSkin", "bool CalcPointsEx(int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride, NativePointer normalPtr, CKDWORD normalStride)", asFUNCTIONPR([](CKSkin *self, int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride, NativePointer normalPtr, CKDWORD normalStride) -> bool { return self->CalcPointsEx(vertexCount, reinterpret_cast<CKBYTE *>(vertexPtr.Get()), vertexStride, reinterpret_cast<CKBYTE *>(normalPtr.Get()), normalStride); }, (CKSkin *, int, NativePointer, CKDWORD, NativePointer, CKDWORD), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKSkin", "bool CalcPoints(int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride, NativePointer normalPtr, CKDWORD normalStride)", asFUNCTIONPR([](CKSkin *self, int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride, NativePointer normalPtr, CKDWORD normalStride) -> bool { return self->CalcPoints(vertexCount, reinterpret_cast<CKBYTE *>(vertexPtr.Get()), vertexStride, reinterpret_cast<CKBYTE *>(normalPtr.Get()), normalStride); }, (CKSkin *, int, NativePointer, CKDWORD, NativePointer, CKDWORD), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
}

// CKIkJoint

void RegisterCKIkJoint(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKIkJoint", "CKDWORD m_Flags", offsetof(CKIkJoint, m_Flags)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKIkJoint", "VxVector m_Min", offsetof(CKIkJoint, m_Min)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKIkJoint", "VxVector m_Max", offsetof(CKIkJoint, m_Max)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKIkJoint", "VxVector m_Damping", offsetof(CKIkJoint, m_Damping)); CKAS_CHECK_REGISTER(r);
}

// CKFileManagerData

void RegisterCKFileManagerData(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKFileManagerData", "CKStateChunk@ data", asOFFSET(CKFileManagerData, data)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileManagerData", "CKGUID Manager", asOFFSET(CKFileManagerData, Manager)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFileManagerData", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKFileManagerData *self) { new(self) CKFileManagerData(); }, (CKFileManagerData *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKFileManagerData", asBEHAVE_CONSTRUCT, "void f(const CKFileManagerData &in other)", asFUNCTIONPR([](const CKFileManagerData &data, CKFileManagerData *self) { new(self) CKFileManagerData(data); }, (const CKFileManagerData &, CKFileManagerData *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFileManagerData", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKFileManagerData *self) { self->~CKFileManagerData(); }, (CKFileManagerData *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFileManagerData", "CKFileManagerData &opAssign(const CKFileManagerData &in other)", asMETHODPR(CKFileManagerData, operator=, (const CKFileManagerData &), CKFileManagerData &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKFilePluginDependencies

void RegisterCKFilePluginDependencies(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKFilePluginDependencies", "int m_PluginCategory", asOFFSET(CKFilePluginDependencies, m_PluginCategory)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKFilePluginDependencies", "XArray<CKGUID> m_Guids", asOFFSET(CKFilePluginDependencies, m_Guids)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFilePluginDependencies", "XBitArray ValidGuids", asOFFSET(CKFilePluginDependencies, ValidGuids)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFilePluginDependencies", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKFilePluginDependencies *self) { new(self) CKFilePluginDependencies(); }, (CKFilePluginDependencies *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKFilePluginDependencies", asBEHAVE_CONSTRUCT, "void f(const CKFilePluginDependencies &in other)", asFUNCTIONPR([](const CKFilePluginDependencies &deps, CKFilePluginDependencies *self) { new(self) CKFilePluginDependencies(deps); }, (const CKFilePluginDependencies &, CKFilePluginDependencies *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFilePluginDependencies", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKFilePluginDependencies *self) { self->~CKFilePluginDependencies(); }, (CKFilePluginDependencies *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFilePluginDependencies", "CKFilePluginDependencies &opAssign(const CKFilePluginDependencies &in other)", asMETHODPR(CKFilePluginDependencies, operator=, (const CKFilePluginDependencies &), CKFilePluginDependencies &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKFileObject

void RegisterCKFileObject(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKFileObject", "CK_ID Object", asOFFSET(CKFileObject, Object)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileObject", "CK_ID CreatedObject", asOFFSET(CKFileObject, CreatedObject)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileObject", "CK_CLASSID ObjectCid", asOFFSET(CKFileObject, ObjectCid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileObject", "CKObject@ ObjPtr", asOFFSET(CKFileObject, ObjPtr)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKFileObject", "uintptr_t Name", asOFFSET(CKFileObject, Name)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileObject", "CKStateChunk@ Data", asOFFSET(CKFileObject, Data)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileObject", "int PostPackSize", asOFFSET(CKFileObject, PostPackSize)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileObject", "int PrePackSize", asOFFSET(CKFileObject, PrePackSize)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileObject", "CK_FO_OPTIONS Options", asOFFSET(CKFileObject, Options)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileObject", "int FileIndex", asOFFSET(CKFileObject, FileIndex)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKFileObject", "CKDWORD SaveFlags", asOFFSET(CKFileObject, SaveFlags)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFileObject", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](CKFileObject *self) { new(self) CKFileObject(); }, (CKFileObject *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("CKFileObject", asBEHAVE_CONSTRUCT, "void f(const CKFileObject &in other)", asFUNCTIONPR([](const CKFileObject &obj, CKFileObject *self) { new(self) CKFileObject(obj); }, (const CKFileObject &, CKFileObject *), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectBehaviour("CKFileObject", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKFileObject *self) { self->~CKFileObject(); }, (CKFileObject *self), void), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFileObject", "CKFileObject &opAssign(const CKFileObject &in other)", asMETHODPR(CKFileObject, operator=, (const CKFileObject &), CKFileObject &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFileObject", "bool CanBeLoad()", asFUNCTIONPR([](CKFileObject *self) -> bool { return self->CanBeLoad(); }, (CKFileObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFileObject", "void CleanData()", asMETHOD(CKFileObject, CleanData), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFileObject", "string get_Name() const", asFUNCTIONPR([](const CKFileObject *self) -> std::string { return ScriptStringify(self->Name); }, (const CKFileObject *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFileObject", "void set_Name(const string &in value)", asFUNCTIONPR([](CKFileObject *self, const std::string &value) { CKDeletePointer(self->Name); self->Name = CKStrdup(const_cast<CKSTRING>(value.c_str())); }, (CKFileObject *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
}

// CKStateChunk

static CKObject *ReadCKStateChunkObject(CKStateChunk *self, CKContext *context) {
    if (!self || !context) {
        if (asIScriptContext *ctx = asGetActiveContext())
            ctx->SetException("CKStateChunk.ReadObject requires a non-null CKContext.");
        return nullptr;
    }
    return self->ReadObject(context);
}

static const XObjectPointerArray &ReadCKStateChunkXObjectArray(CKStateChunk *self, CKContext *context) {
    static XObjectPointerArray empty;
    if (!self || !context) {
        if (asIScriptContext *ctx = asGetActiveContext())
            ctx->SetException("CKStateChunk.ReadXObjectArray requires a non-null CKContext.");
        empty.Clear();
        return empty;
    }
    return self->ReadXObjectArray(context);
}

static int ReadCKStateChunkString(CKStateChunk *self, std::string &str) {
    if (!self) {
        if (asIScriptContext *ctx = asGetActiveContext())
            ctx->SetException("CKStateChunk.ReadString requires a valid state chunk.");
        return 0;
    }

    CKSTRING buffer = nullptr;
    const int result = self->ReadString(&buffer);
    if (buffer) {
        str = buffer;
        CKDeletePointer(buffer);
    } else {
        str.clear();
    }
    return result;
}

void RegisterCKStateChunk(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectMethod("CKStateChunk", "void StartRead()", asMETHODPR(CKStateChunk, StartRead, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void StartWrite()", asMETHODPR(CKStateChunk, StartWrite, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void CloseChunk()", asMETHODPR(CKStateChunk, CloseChunk, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void Clear()", asMETHODPR(CKStateChunk, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void UpdateDataSize()", asMETHODPR(CKStateChunk, UpdateDataSize, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "CK_CLASSID GetChunkClassID()", asMETHODPR(CKStateChunk, GetChunkClassID, (), CK_CLASSID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void Clone(CKStateChunk@ chunk)", asMETHODPR(CKStateChunk, Clone, (CKStateChunk*), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "int16 GetDataVersion()", asMETHODPR(CKStateChunk, GetDataVersion, (), short), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void SetDataVersion(int16 version)", asMETHODPR(CKStateChunk, SetDataVersion, (short), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int16 GetChunkVersion()", asMETHODPR(CKStateChunk, GetChunkVersion, (), short), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteIdentifier(CKDWORD id)", asMETHODPR(CKStateChunk, WriteIdentifier, (CKDWORD), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "CKDWORD ReadIdentifier()", asMETHODPR(CKStateChunk, ReadIdentifier, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "bool SeekIdentifier(CKDWORD identifier)", asFUNCTIONPR([](CKStateChunk *self, CKDWORD identifier) -> bool { return self->SeekIdentifier(identifier); }, (CKStateChunk *, CKDWORD), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int SeekIdentifierAndReturnSize(CKDWORD identifier)", asMETHODPR(CKStateChunk, SeekIdentifierAndReturnSize, (CKDWORD), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "int GetCurrentPos()", asMETHODPR(CKStateChunk, GetCurrentPos, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void Skip(int dwordCount)", asMETHODPR(CKStateChunk, Skip, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void Goto(int dwordCount)", asMETHODPR(CKStateChunk, Goto, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int GetDataSize()", asMETHODPR(CKStateChunk, GetDataSize, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKStateChunk", "CKDWORD ComputeCRC(CKDWORD adler)", asMETHODPR(CKStateChunk, ComputeCRC, (CKDWORD), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void Pack(int compressionLevel)", asMETHODPR(CKStateChunk, Pack, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "bool UnPack(int destSize)", asFUNCTIONPR([](CKStateChunk *self, int destSize) -> bool { return self->UnPack(destSize); }, (CKStateChunk *, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif

    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteByte(uint8 byte)", asMETHODPR(CKStateChunk, WriteByte, (CKCHAR), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteWord(uint16 data)", asMETHODPR(CKStateChunk, WriteWord, (CKWORD), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteDword(uint32 data)", asMETHODPR(CKStateChunk, WriteDword, (CKDWORD), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteDwordAsWords(uint32 data)", asMETHODPR(CKStateChunk, WriteDwordAsWords, (CKDWORD), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteInt(int data)", asMETHODPR(CKStateChunk, WriteInt, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteFloat(float data)", asMETHODPR(CKStateChunk, WriteFloat, (float), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteString(const string &in str)", asFUNCTIONPR([](CKStateChunk *self, const std::string &str) { self->WriteString(const_cast<CKSTRING>(str.c_str())); }, (CKStateChunk *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteString(const string &in str)", asFUNCTIONPR([](CKStateChunk *self, const std::string &str) { self->WriteString(str.c_str()); }, (CKStateChunk *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteObjectID(CK_ID id)", asMETHODPR(CKStateChunk, WriteObjectID, (CK_ID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteObject(CKObject@ obj)", asMETHODPR(CKStateChunk, WriteObject, (CKObject *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteGuid(CKGUID data)", asMETHODPR(CKStateChunk, WriteGuid, (CKGUID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteVector(const VxVector &in v)", asMETHODPR(CKStateChunk, WriteVector, (const VxVector &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteMatrix(const VxMatrix &in mat)", asMETHODPR(CKStateChunk, WriteMatrix, (const VxMatrix &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteObjectArray(CKObjectArray@ objArray, CKContext@ context = null)", asMETHODPR(CKStateChunk, WriteObjectArray, (CKObjectArray *, CKContext *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteSubChunk(CKStateChunk@ sub)", asMETHODPR(CKStateChunk, WriteSubChunk, (CKStateChunk*), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteBitmap(BITMAP_HANDLE bitmap, const string &in ext = void)", asFUNCTIONPR([](CKStateChunk *self, BITMAP_HANDLE bitmap, const std::string &ext) { self->WriteBitmap(bitmap, ext.empty() ? nullptr : const_cast<CKSTRING>(ext.c_str())); }, (CKStateChunk *, BITMAP_HANDLE, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteReaderBitmap(const VxImageDescEx &in desc, CKBitmapReader@ reader, CKBitmapProperties@ bp)", asMETHODPR(CKStateChunk, WriteReaderBitmap, (const VxImageDescEx&, CKBitmapReader *, CKBitmapProperties *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteManagerInt(CKGUID manager, int val)", asMETHODPR(CKStateChunk, WriteManagerInt, (CKGUID, int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteBuffer(int size, NativePointer buf)", asFUNCTIONPR([](CKStateChunk *self, int size, NativePointer buf) { self->WriteBuffer(size, buf.Get()); }, (CKStateChunk *, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteBufferNoSize(int size, NativePointer buf)", asFUNCTIONPR([](CKStateChunk *self, int size, NativePointer buf) { self->WriteBufferNoSize(size, buf.Get()); }, (CKStateChunk *, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteBuffer_LEndian(int size, NativePointer buf)", asFUNCTIONPR([](CKStateChunk *self, int size, NativePointer buf) { self->WriteBuffer_LEndian(size, buf.Get()); }, (CKStateChunk *, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteBuffer_LEndian16(int size, NativePointer buf)", asFUNCTIONPR([](CKStateChunk *self, int size, NativePointer buf) { self->WriteBuffer_LEndian16(size, buf.Get()); }, (CKStateChunk *, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteArray_LEndian(int elementCount, int elementSize, NativePointer srcData)", asFUNCTIONPR([](CKStateChunk *self, int elementCount, int elementSize, NativePointer srcData) { self->WriteArray_LEndian(elementCount, elementSize, srcData.Get()); }, (CKStateChunk *, int, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteArray_LEndian16(int elementCount, int elementSize, NativePointer srcData)", asFUNCTIONPR([](CKStateChunk *self, int elementCount, int elementSize, NativePointer srcData) { self->WriteArray_LEndian16(elementCount, elementSize, srcData.Get()); }, (CKStateChunk *, int, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif

    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteBufferNoSize_LEndian(int size, NativePointer buf)", asFUNCTIONPR([](CKStateChunk *self, int size, NativePointer buf) { self->WriteBufferNoSize_LEndian(size, buf.Get()); }, (CKStateChunk *, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteBufferNoSize_LEndian16(int size, NativePointer buf)", asFUNCTIONPR([](CKStateChunk *self, int size, NativePointer buf) { self->WriteBufferNoSize_LEndian16(size, buf.Get()); }, (CKStateChunk *, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void StartObjectIDSequence(int count)", asMETHODPR(CKStateChunk, StartObjectIDSequence, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteObjectIDSequence(CK_ID id)", asMETHODPR(CKStateChunk, WriteObjectIDSequence, (CK_ID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteObjectSequence(CKObject@ obj)", asMETHODPR(CKStateChunk, WriteObjectSequence, (CKObject *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void StartSubChunkSequence(int count)", asMETHODPR(CKStateChunk, StartSubChunkSequence, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteSubChunkSequence(CKStateChunk@ sub)", asMETHODPR(CKStateChunk, WriteSubChunkSequence, (CKStateChunk *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void StartManagerSequence(CKGUID man, int count)", asMETHODPR(CKStateChunk, StartManagerSequence, (CKGUID, int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteManagerSequence(int val)", asMETHODPR(CKStateChunk, WriteManagerSequence, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "int StartReadSequence()", asMETHODPR(CKStateChunk, StartReadSequence, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "int StartManagerReadSequence(CKGUID &out guid)", asMETHODPR(CKStateChunk, StartManagerReadSequence, (CKGUID *), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int ReadManagerIntSequence()", asMETHODPR(CKStateChunk, ReadManagerIntSequence, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "CK_ID ReadObjectID()", asMETHODPR(CKStateChunk, ReadObjectID, (), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "CKObject@ ReadObject(CKContext@ context)", asFUNCTION(ReadCKStateChunkObject), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "uint8 ReadByte()", asMETHODPR(CKStateChunk, ReadByte, (), CKBYTE), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "uint16 ReadWord()", asMETHODPR(CKStateChunk, ReadWord, (), CKWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "CKGUID ReadGuid()", asMETHODPR(CKStateChunk, ReadGuid, (), CKGUID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "uint32 ReadDword()", asMETHODPR(CKStateChunk, ReadDword, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "uint32 ReadDwordAsWords()", asMETHODPR(CKStateChunk, ReadDwordAsWords, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int ReadInt()", asMETHODPR(CKStateChunk, ReadInt, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "float ReadFloat()", asMETHODPR(CKStateChunk, ReadFloat, (), float), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void ReadVector(VxVector &out v)", asMETHODPR(CKStateChunk, ReadVector, (VxVector &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void ReadMatrix(VxMatrix &out mat)", asMETHODPR(CKStateChunk, ReadMatrix, (VxMatrix &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int ReadManagerInt(CKGUID &out guid)", asMETHODPR(CKStateChunk, ReadManagerInt, (CKGUID *), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKStateChunk", "int ReadArray_LEndian(NativePointer &out array)", asFUNCTIONPR([](CKStateChunk *self, NativePointer *array) { void *ptr = nullptr; int result = self->ReadArray_LEndian(&ptr); *array = ptr; return result; }, (CKStateChunk *, NativePointer *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int ReadArray_LEndian16(NativePointer &out array)", asFUNCTIONPR([](CKStateChunk *self, NativePointer *array) { void *ptr = nullptr; int result = self->ReadArray_LEndian16(&ptr); *array = ptr; return result; }, (CKStateChunk *, NativePointer *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKStateChunk", "const XObjectArray &ReadXObjectArray()", asMETHODPR(CKStateChunk, ReadXObjectArray, (), const XObjectArray &), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "const XObjectPointerArray &ReadXObjectArray(CKContext@ context)", asFUNCTION(ReadCKStateChunkXObjectArray), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void ReadObjectArray(CKObjectArray@ objArray)", asMETHODPR(CKStateChunk, ReadObjectArray, (CKObjectArray *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "CKObjectArray@ ReadObjectArray()", asMETHODPR(CKStateChunk, ReadObjectArray, (), CKObjectArray *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void ReadAndFillBuffer(NativePointer buffer)", asFUNCTIONPR([](CKStateChunk *self, NativePointer buffer) { self->ReadAndFillBuffer(buffer.Get()); }, (CKStateChunk *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void ReadAndFillBuffer(int size, NativePointer buffer)", asFUNCTIONPR([](CKStateChunk *self, int size, NativePointer buffer) { self->ReadAndFillBuffer(size, buffer.Get()); }, (CKStateChunk *, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void ReadAndFillBuffer_LEndian(NativePointer buffer)", asFUNCTIONPR([](CKStateChunk *self, NativePointer buffer) { self->ReadAndFillBuffer_LEndian(buffer.Get()); }, (CKStateChunk *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void ReadAndFillBuffer_LEndian(int size, NativePointer buffer)", asFUNCTIONPR([](CKStateChunk *self, int size, NativePointer buffer) { self->ReadAndFillBuffer_LEndian(size, buffer.Get()); }, (CKStateChunk *, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void ReadAndFillBuffer_LEndian16(NativePointer buffer)", asFUNCTIONPR([](CKStateChunk *self, NativePointer buffer) { self->ReadAndFillBuffer_LEndian16(buffer.Get()); }, (CKStateChunk *, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void ReadAndFillBuffer_LEndian16(int size, NativePointer buffer)", asFUNCTIONPR([](CKStateChunk *self, int size, NativePointer buffer) { self->ReadAndFillBuffer_LEndian16(size, buffer.Get()); }, (CKStateChunk *, int, NativePointer), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKStateChunk", "CKStateChunk@ ReadSubChunk()", asMETHODPR(CKStateChunk, ReadSubChunk, (), CKStateChunk *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#else
    r = engine->RegisterObjectMethod("CKStateChunk", "CKStateChunk@ ReadSubChunk(CK_READSUBCHUNK_FLAGS flags = CK_RSC_DEFAULT)", asMETHODPR(CKStateChunk, ReadSubChunk, (CK_READSUBCHUNK_FLAGS), CKStateChunk *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKStateChunk", "int ReadBuffer(NativePointer &out buffer)", asFUNCTIONPR([](CKStateChunk *self, NativePointer *buffer) { void *ptr = nullptr; int result = self->ReadBuffer(&ptr); *buffer = ptr; return result; }, (CKStateChunk *, NativePointer *), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int ReadString(string &out str)", asFUNCTION(ReadCKStateChunkString), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "BITMAP_HANDLE ReadBitmap()", asMETHODPR(CKStateChunk, ReadBitmap, (), BITMAP_HANDLE), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKStateChunk", "NativePointer ReadBitmap2(VxImageDescEx &out desc)", asFUNCTIONPR([](CKStateChunk *self, VxImageDescEx &desc) { return NativePointer(self->ReadBitmap2(desc)); }, (CKStateChunk *, VxImageDescEx &), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKStateChunk", "bool ReadReaderBitmap(const VxImageDescEx &in desc)", asFUNCTIONPR([](CKStateChunk *self, const VxImageDescEx &desc) -> bool { return self->ReadReaderBitmap(desc); }, (CKStateChunk *, const VxImageDescEx &), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int RemapObject(CK_ID oldID, CK_ID newID)", asMETHODPR(CKStateChunk, RemapObject, (CK_ID, CK_ID), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int RemapObjects(CKContext@ context, CKDependenciesContext@ depContext = null)", asMETHODPR(CKStateChunk, RemapObjects, (CKContext *, CKDependenciesContext *), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int RemapManagerInt(CKGUID manager, NativePointer conversionTable, int nbEntries)", asFUNCTIONPR([](CKStateChunk *self, CKGUID manager, NativePointer conversionTable, int nbEntries) { return self->RemapManagerInt(manager, reinterpret_cast<int*>(conversionTable.Get()), nbEntries); }, (CKStateChunk *, CKGUID, NativePointer, int), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "int RemapParameterInt(CKGUID parameterType, NativePointer conversionTable, int nbEntries)", asFUNCTIONPR([](CKStateChunk *self, CKGUID parameterType, NativePointer conversionTable, int nbEntries) { return self->RemapParameterInt(parameterType, reinterpret_cast<int*>(conversionTable.Get()), nbEntries); }, (CKStateChunk *, CKGUID, NativePointer, int), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "void AddChunk(CKStateChunk@ chunk)", asMETHODPR(CKStateChunk, AddChunk, (CKStateChunk *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "int ConvertToBuffer(NativePointer buffer)", asFUNCTIONPR([](CKStateChunk *self, NativePointer buffer) { return self->ConvertToBuffer(buffer.Get()); }, (CKStateChunk *, NativePointer), int), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "bool ConvertFromBuffer(NativePointer buffer)", asFUNCTIONPR([](CKStateChunk *self, NativePointer buffer) -> bool { return self->ConvertFromBuffer(buffer.Get()); }, (CKStateChunk *, NativePointer), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "NativePointer LockWriteBuffer(int dwordCount)", asFUNCTIONPR([](CKStateChunk *self, int dwordCount) { return NativePointer(self->LockWriteBuffer(dwordCount)); }, (CKStateChunk *, int), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "NativePointer LockReadBuffer()", asFUNCTIONPR([](CKStateChunk *self) { return NativePointer(self->LockReadBuffer()); }, (CKStateChunk *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKStateChunk", "NativePointer ReadRawBitmap(VxImageDescEx &out desc)", asFUNCTIONPR([](CKStateChunk *self, VxImageDescEx& desc) { return NativePointer(self->ReadRawBitmap(desc)); }, (CKStateChunk *, VxImageDescEx &), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKStateChunk", "void WriteRawBitmap(const VxImageDescEx &in desc)", asMETHODPR(CKStateChunk, WriteRawBitmap, (const VxImageDescEx &), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKStateChunk", "void SetDynamic(bool dynamic)", asFUNCTIONPR([](CKStateChunk *self, bool dynamic) { self->SetDynamic(dynamic); }, (CKStateChunk *, bool), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#endif
}

// CKFile

void RegisterCKFile(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectMethod("CKFile", "CKERROR OpenFile(const string &in filename, CK_LOAD_FLAGS flags = CK_LOAD_DEFAULT)", asFUNCTIONPR([](CKFile *self, const std::string &filename, CK_LOAD_FLAGS flags) { return self->OpenFile(const_cast<CKSTRING>(filename.c_str()), flags); }, (CKFile *, const std::string &, CK_LOAD_FLAGS), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "CKERROR OpenMemory(NativePointer buffer, int bufferSize, CK_LOAD_FLAGS flags = CK_LOAD_DEFAULT)", asFUNCTIONPR([](CKFile *self, NativePointer buffer, int bufferSize, CK_LOAD_FLAGS flags) { return self->OpenMemory(buffer.Get(), bufferSize, flags); }, (CKFile *, NativePointer, int, CK_LOAD_FLAGS), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFile", "CKERROR LoadFileData(CKObjectArray@ objArray)", asMETHODPR(CKFile, LoadFileData, (CKObjectArray *), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFile", "CKERROR Load(const string &in filename, CKObjectArray@ objArray, CK_LOAD_FLAGS flags = CK_LOAD_DEFAULT)", asFUNCTIONPR([](CKFile *self, const std::string &filename, CKObjectArray *objArray, CK_LOAD_FLAGS flags) { return self->Load(const_cast<CKSTRING>(filename.c_str()), objArray, flags); }, (CKFile *, const std::string &, CKObjectArray *, CK_LOAD_FLAGS), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "CKERROR Load(NativePointer buffer, int bufferSize, CKObjectArray@ objArray, CK_LOAD_FLAGS flags = CK_LOAD_DEFAULT)", asFUNCTIONPR([](CKFile *self, NativePointer buffer, int bufferSize, CKObjectArray * objArray, CK_LOAD_FLAGS flags) { return self->Load(buffer.Get(), bufferSize, objArray, flags); }, (CKFile *, NativePointer, int, CKObjectArray *, CK_LOAD_FLAGS), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFile", "void UpdateAndApplyAnimationsTo(CKCharacter@ character)", asMETHODPR(CKFile, UpdateAndApplyAnimationsTo, (CKCharacter*), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFile", "CKERROR StartSave(const string &in filename, CKDWORD flags = CK_STATESAVE_ALL)", asFUNCTIONPR([](CKFile *self, const std::string &filename, CKDWORD flags) { return self->StartSave(const_cast<CKSTRING>(filename.c_str()), flags); }, (CKFile *, const std::string &, CKDWORD), CKERROR), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "void SaveObject(CKObject@ obj, CKDWORD flags = CK_STATESAVE_ALL)", asMETHODPR(CKFile, SaveObject, (CKObject *, CKDWORD), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "void SaveObjects(CKObjectArray@ objArray, CKDWORD flags = CK_STATESAVE_ALL)", asMETHODPR(CKFile, SaveObjects, (CKObjectArray *, CKDWORD), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "void SaveObjects(const XObjectArray &in ids, CKDWORD flags = CK_STATESAVE_ALL)", asFUNCTIONPR([](CKFile *file, const XObjectArray &objects, CKDWORD flags) { file->SaveObjects(objects.Begin(), objects.Size(), flags); }, (CKFile *, const XObjectArray &, CKDWORD), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "void SaveObjectAsReference(CKObject@ obj)", asMETHODPR(CKFile, SaveObjectAsReference, (CKObject *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "CKERROR EndSave()", asMETHODPR(CKFile, EndSave, (), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKFile", "bool IncludeFile(const string &in file, int searchPathCategory = -1)", asFUNCTIONPR([](CKFile *self, const std::string &file, int searchPathCategory) -> bool { return self->IncludeFile(const_cast<CKSTRING>(file.c_str()), searchPathCategory); }, (CKFile *, const std::string &, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "bool IsObjectToBeSaved(CK_ID id)", asFUNCTIONPR([](CKFile *self, CK_ID id) -> bool { return self->IsObjectToBeSaved(id); }, (CKFile *, CK_ID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "void LoadAndSave(const string &in filename, const string &in newFileName)", asFUNCTIONPR([](CKFile *self, const std::string &filename, const std::string &newFileName) { self->LoadAndSave(const_cast<CKSTRING>(filename.c_str()), const_cast<CKSTRING>(newFileName.c_str())); }, (CKFile *, const std::string &, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "void RemapManagerInt(CKGUID manager, NativePointer conversionTable, int tableSize)", asFUNCTIONPR([](CKFile *self, CKGUID manager, NativePointer conversionTable, int tableSize) { self->RemapManagerInt(manager, reinterpret_cast<int *>(conversionTable.Get()), tableSize); }, (CKFile *, CKGUID, NativePointer, int), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKFile", "XFilePluginDependenciesArray &GetMissingPlugins()", asMETHODPR(CKFile, GetMissingPlugins, (), XClassArray<CKFilePluginDependencies>*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKDependencies

void RegisterCKDependencies(asIScriptEngine *engine) {
    int r = 0;

    RegisterXSArray<CKDependencies, CKDWORD>(engine, "CKDependencies", "CKDWORD");

    r = engine->RegisterObjectProperty("CKDependencies", "CK_DEPENDENCIES_FLAGS m_Flags", asOFFSET(CKDependencies, m_Flags)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKDependencies", "void ModifyOptions(CK_CLASSID cid, CKDWORD add, CKDWORD remove)", asMETHOD(CKDependencies, ModifyOptions), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKDependenciesContext

static void CopyCKDependenciesContext(CKDependenciesContext *self, const std::string &appendString) {
    if (self) {
        self->Copy(appendString.empty() ? nullptr : const_cast<CKSTRING>(appendString.c_str()));
    }
}

void RegisterCKDependenciesContext(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectMethod("CKDependenciesContext", "void AddObjects(NativePointer ids, int count)", asFUNCTIONPR([](CKDependenciesContext *self, NativePointer ids, int count) { self->AddObjects(reinterpret_cast<CK_ID *>(ids.Get()), count); }, (CKDependenciesContext *, NativePointer, int), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKDependenciesContext", "int GetObjectsCount()", asMETHODPR(CKDependenciesContext, GetObjectsCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "CKObject@ GetObjects(int i)", asMETHODPR(CKDependenciesContext, GetObjects, (int), CKObject *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "CK_ID RemapID(CK_ID &in id)", asMETHODPR(CKDependenciesContext, RemapID, (CK_ID &), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "CKObject@ Remap(CKObject@ obj)", asMETHODPR(CKDependenciesContext, Remap, (const CKObject *), CKObject *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "bool IsDependenciesHere(CK_ID id)", asFUNCTIONPR([](CKDependenciesContext *self, CK_ID id) -> bool { return self->IsDependenciesHere(id); }, (CKDependenciesContext *, CK_ID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "XObjectArray FillDependencies()", asMETHODPR(CKDependenciesContext, FillDependencies, (), XObjectArray), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "XObjectArray FillRemappedDependencies()", asMETHODPR(CKDependenciesContext, FillRemappedDependencies, (), XObjectArray), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "void StartDependencies(CKDependencies &in d)", asMETHODPR(CKDependenciesContext, StartDependencies, (CKDependencies*), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "void StopDependencies()", asMETHODPR(CKDependenciesContext, StopDependencies, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "CKDWORD GetClassDependencies(int c)", asMETHODPR(CKDependenciesContext, GetClassDependencies, (int), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "void Copy(const string &in appendString = void)", asFUNCTION(CopyCKDependenciesContext), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "void SetOperationMode(CK_DEPENDENCIES_OPMODE m)", asMETHODPR(CKDependenciesContext, SetOperationMode, (CK_DEPENDENCIES_OPMODE), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "bool IsInMode(CK_DEPENDENCIES_OPMODE m)", asFUNCTIONPR([](CKDependenciesContext *self, CK_DEPENDENCIES_OPMODE m) -> bool { return self->IsInMode(m); }, (CKDependenciesContext *, CK_DEPENDENCIES_OPMODE), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "void SetCreationMode(CK_OBJECTCREATION_OPTIONS m)", asMETHODPR(CKDependenciesContext, SetCreationMode, (CK_OBJECTCREATION_OPTIONS), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKDependenciesContext", "bool ContainClassID(CK_CLASSID cid)", asFUNCTIONPR([](CKDependenciesContext *self, CK_CLASSID cid) -> bool { return self->ContainClassID(cid); }, (CKDependenciesContext *, CK_CLASSID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKDependenciesContext", "CKERROR FinishPrepareDependencies(CKObject@ self, CK_CLASSID cid)", asMETHODPR(CKDependenciesContext, FinishPrepareDependencies, (CKObject *, CK_CLASSID), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
#endif
    r = engine->RegisterObjectMethod("CKDependenciesContext", "const XHashID &GetDependenciesMap() const", asMETHODPR(CKDependenciesContext, GetDependenciesMap, () const, const XHashID&), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKDebugContext

void RegisterCKDebugContext(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKDebugContext", "float delta", offsetof(CKDebugContext, delta)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "CKBeObject@ CurrentObject", offsetof(CKDebugContext, CurrentObject)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "CKBehavior@ CurrentScript", offsetof(CKDebugContext, CurrentScript)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "CKBehavior@ CurrentBehavior", offsetof(CKDebugContext, CurrentBehavior)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "CKBehavior@ SubBehavior", offsetof(CKDebugContext, SubBehavior)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "CKObjectArray ObjectsToExecute", offsetof(CKDebugContext, ObjectsToExecute)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "CKObjectArray ScriptsToExecute", offsetof(CKDebugContext, ScriptsToExecute)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "CKObjectArray BehaviorStack", offsetof(CKDebugContext, BehaviorStack)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "CKContext@ m_Context", offsetof(CKDebugContext, m_Context)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "CKDEBUG_STATE CurrentBehaviorAction", offsetof(CKDebugContext, CurrentBehaviorAction)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKDebugContext", "int InDebug", offsetof(CKDebugContext, InDebug)); CKAS_CHECK_REGISTER(r);

    // Not implemented
    // r = engine->RegisterObjectMethod("CKDebugContext", "void Init(XObjectPointerArray &in array, float delta)", asMETHODPR(CKDebugContext, Init, (XObjectPointerArray &, float), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKDebugContext", "void StepInto(CKBehavior@ beh)", asMETHODPR(CKDebugContext, StepInto, (CKBehavior *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKDebugContext", "void StepBehavior()", asMETHODPR(CKDebugContext, StepBehavior, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKDebugContext", "bool DebugStep()", asMETHODPR(CKDebugContext, DebugStep, (), CKBOOL), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKDebugContext", "void Clear()", asMETHODPR(CKDebugContext, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKObjectArray

void RegisterCKObjectArray(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectMethod("CKObjectArray", "int GetCount()", asMETHODPR(CKObjectArray, GetCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "int GetCurrentPos()", asMETHODPR(CKObjectArray, GetCurrentPos, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "CKObject@ GetData(CKContext@ context)", asMETHODPR(CKObjectArray, GetData, (CKContext *), CKObject *), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID GetDataId()", asMETHODPR(CKObjectArray, GetDataId, (), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID SetDataId(CK_ID id)", asMETHODPR(CKObjectArray, SetDataId, (CK_ID), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID SetData(CKObject@ obj)", asMETHODPR(CKObjectArray, SetData, (CKObject *), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "void Reset()", asMETHODPR(CKObjectArray, Reset, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "bool PtrSeek(CKObject@ obj)", asFUNCTIONPR([](CKObjectArray *self, CKObject *obj) -> bool { return self->PtrSeek(obj); }, (CKObjectArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "bool IDSeek(CK_ID id)", asFUNCTIONPR([](CKObjectArray *self, CK_ID id) -> bool { return self->IDSeek(id); }, (CKObjectArray *, CK_ID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "bool PositionSeek(int pos)", asFUNCTIONPR([](CKObjectArray *self, int pos) -> bool { return self->PositionSeek(pos); }, (CKObjectArray *, int), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID Seek(int pos)", asMETHODPR(CKObjectArray, Seek, (int), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "void Next()", asMETHODPR(CKObjectArray, Next, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "void Previous()", asMETHODPR(CKObjectArray, Previous, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "int GetPosition(CKObject@ obj)", asMETHODPR(CKObjectArray, GetPosition, (CKObject *), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "int GetPosition(CK_ID id)", asMETHODPR(CKObjectArray, GetPosition, (CK_ID), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID PtrFind(CKObject@ obj)", asMETHODPR(CKObjectArray, PtrFind, (CKObject *), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID IDFind(CK_ID id)", asMETHODPR(CKObjectArray, IDFind, (CK_ID), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID PositionFind(int pos)", asMETHODPR(CKObjectArray, PositionFind, (int), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "void InsertFront(CKObject@ obj)", asMETHODPR(CKObjectArray, InsertFront, (CKObject *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "void InsertRear(CKObject@ obj)", asMETHODPR(CKObjectArray, InsertRear, (CKObject *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "void InsertAt(CKObject@ obj)", asMETHODPR(CKObjectArray, InsertAt, (CKObject *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "bool AddIfNotHere(CKObject@ obj)", asFUNCTIONPR([](CKObjectArray *self, CKObject *obj) -> bool { return self->AddIfNotHere(obj); }, (CKObjectArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKObjectArray", "bool AddIfNotHereSorted(CKObject@ obj, OBJECTARRAYCMPFCT cmpFct, CKContext@ context)", asFUNCTIONPR([](CKObjectArray *self, CKObject *obj, OBJECTARRAYCMPFCT cmpFct, CKContext *context) -> bool { return self->AddIfNotHereSorted(obj, cmpFct, context); }, (CKObjectArray *, CKObject *, OBJECTARRAYCMPFCT, CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "void InsertFront(CK_ID id)", asMETHODPR(CKObjectArray, InsertFront, (CK_ID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "void InsertRear(CK_ID id)", asMETHODPR(CKObjectArray, InsertRear, (CK_ID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "void InsertAt(CK_ID id)", asMETHODPR(CKObjectArray, InsertAt, (CK_ID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "bool AddIfNotHere(CK_ID id)", asFUNCTIONPR([](CKObjectArray *self, CK_ID id) -> bool { return self->AddIfNotHere(id); }, (CKObjectArray *, CK_ID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKObjectArray", "bool AddIfNotHereSorted(CK_ID id, OBJECTARRAYCMPFCT cmpFc, CKContext@ context)", asFUNCTIONPR([](CKObjectArray *self, CK_ID id, OBJECTARRAYCMPFCT cmpFc, CKContext *context) -> bool { return self->AddIfNotHereSorted(id, cmpFc, context); }, (CKObjectArray *, CK_ID, OBJECTARRAYCMPFCT, CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "CKERROR Append(CKObjectArray@ objArray)", asMETHODPR(CKObjectArray, Append, (CKObjectArray *), CKERROR), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID RemoveFront()", asMETHODPR(CKObjectArray, RemoveFront, (), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID RemoveRear()", asMETHODPR(CKObjectArray, RemoveRear, (), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "CK_ID RemoveAt()", asMETHODPR(CKObjectArray, RemoveAt, (), CK_ID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "bool Remove(CKObject@ obj)", asFUNCTIONPR([](CKObjectArray *self, CKObject *obj) -> bool { return self->Remove(obj); }, (CKObjectArray *, CKObject *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "bool Remove(CK_ID id)", asFUNCTIONPR([](CKObjectArray *self, CK_ID id) -> bool { return self->Remove(id); }, (CKObjectArray *, CK_ID), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "void Clear()", asMETHODPR(CKObjectArray, Clear, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "bool EndOfList()", asFUNCTIONPR([](CKObjectArray *self) -> bool { return self->EndOfList(); }, (CKObjectArray *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "bool ListEmpty()", asFUNCTIONPR([](CKObjectArray *self) -> bool { return self->ListEmpty(); }, (CKObjectArray *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "void SwapCurrentWithNext()", asMETHODPR(CKObjectArray, SwapCurrentWithNext, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectArray", "void SwapCurrentWithPrevious()", asMETHODPR(CKObjectArray, SwapCurrentWithPrevious, (), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectArray", "bool Check(CKContext@ context)", asFUNCTIONPR([](CKObjectArray *self, CKContext *context) -> bool { return self->Check(context); }, (CKObjectArray *, CKContext *), bool), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    // r = engine->RegisterObjectMethod("CKObjectArray", "void Sort(OBJECTARRAYCMPFCT cmpFct, CKContext@ context)", asMETHODPR(CKObjectArray, Sort, (OBJECTARRAYCMPFCT, CKContext *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKObjectArray", "void InsertSorted(CKObject@ obj, OBJECTARRAYCMPFCT cmpFct, CKContext@ context)", asMETHODPR(CKObjectArray, InsertSorted, (CKObject *, OBJECTARRAYCMPFCT, CKContext *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectMethod("CKObjectArray", "void InsertSorted(CK_ID id, OBJECTARRAYCMPFCT cmpFct, CKContext@ context)", asMETHODPR(CKObjectArray, InsertSorted, (CK_ID, OBJECTARRAYCMPFCT, CKContext *), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}

// CKObjectDeclaration

static void SetCKObjectDeclarationCreationFunction(CKObjectDeclaration *self, NativePointer ptr) {
    (void)self;
    RejectNativeFunctionPointerInstall(ptr, "CKObjectDeclaration.SetCreationFunction is read-only from script.");
}

void RegisterCKObjectDeclaration(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectProperty("CKObjectDeclaration", "CKGUID m_Guid", offsetof(CKObjectDeclaration, m_Guid)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKObjectDeclaration", "CK_CLASSID m_CompatibleClassID", offsetof(CKObjectDeclaration, m_CompatibleClassID)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKObjectDeclaration", "uintptr_t m_CreationFunction", offsetof(CKObjectDeclaration, m_CreationFunction)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKObjectDeclaration", "CKDWORD m_Version", offsetof(CKObjectDeclaration, m_Version)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKObjectDeclaration", "uintptr_t m_Description", offsetof(CKObjectDeclaration, m_Description)); CKAS_CHECK_REGISTER(r);
    // m_Proto is owned by the global declaration table and must not be script-writable.
    r = engine->RegisterObjectProperty("CKObjectDeclaration", "int m_Type", offsetof(CKObjectDeclaration, m_Type)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKObjectDeclaration", "CKGUID m_AuthorGuid", offsetof(CKObjectDeclaration, m_AuthorGuid)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKObjectDeclaration", "uintptr_t m_AuthorName", offsetof(CKObjectDeclaration, m_AuthorName)); CKAS_CHECK_REGISTER(r);
    // r = engine->RegisterObjectProperty("CKObjectDeclaration", "uintptr_t m_Category", offsetof(CKObjectDeclaration, m_Category)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKObjectDeclaration", "XString m_Name", offsetof(CKObjectDeclaration, m_Name)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKObjectDeclaration", "int m_PluginIndex", offsetof(CKObjectDeclaration, m_PluginIndex)); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectProperty("CKObjectDeclaration", "XGUIDArray m_ManagersGuid", offsetof(CKObjectDeclaration, m_ManagersGuid)); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetDescription(const string &in description)", asFUNCTIONPR([](CKObjectDeclaration *self, const std::string &description) { self->SetDescription(const_cast<CKSTRING>(description.c_str())); }, (CKObjectDeclaration *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "string GetDescription()", asFUNCTIONPR([](CKObjectDeclaration *self) -> std::string { return ScriptStringify(self->GetDescription()); }, (CKObjectDeclaration *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetGuid(CKGUID guid)", asMETHODPR(CKObjectDeclaration, SetGuid, (CKGUID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "CKGUID GetGuid()", asMETHODPR(CKObjectDeclaration, GetGuid, (), CKGUID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetType(int type)", asMETHODPR(CKObjectDeclaration, SetType, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "int GetType()", asMETHODPR(CKObjectDeclaration, GetType, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void NeedManager(CKGUID manager)", asMETHODPR(CKObjectDeclaration, NeedManager, (CKGUID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetCreationFunction(NativePointer f)", asFUNCTION(SetCKObjectDeclarationCreationFunction), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "NativePointer GetCreationFunction()", asFUNCTIONPR([](CKObjectDeclaration *self) { return NativePointer(self->GetCreationFunction()); }, (CKObjectDeclaration *), NativePointer), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetAuthorGuid(CKGUID guid)", asMETHODPR(CKObjectDeclaration, SetAuthorGuid, (CKGUID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "CKGUID GetAuthorGuid()", asMETHODPR(CKObjectDeclaration, GetAuthorGuid, (), CKGUID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetAuthorName(const string &in name)", asFUNCTIONPR([](CKObjectDeclaration *self, const std::string &name) { self->SetAuthorName(const_cast<CKSTRING>(name.c_str())); }, (CKObjectDeclaration *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "string GetAuthorName()", asFUNCTIONPR([](CKObjectDeclaration *self) -> std::string { return ScriptStringify(self->GetAuthorName()); }, (CKObjectDeclaration *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetVersion(CKDWORD verion)", asMETHODPR(CKObjectDeclaration, SetVersion, (CKDWORD), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "CKDWORD GetVersion()", asMETHODPR(CKObjectDeclaration, GetVersion, (), CKDWORD), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetCompatibleClassId(CK_CLASSID cid)", asMETHODPR(CKObjectDeclaration, SetCompatibleClassId, (CK_CLASSID), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "CK_CLASSID GetCompatibleClassId()", asMETHODPR(CKObjectDeclaration, GetCompatibleClassId, (), CK_CLASSID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetCategory(const string &in category)", asFUNCTIONPR([](CKObjectDeclaration *self, const std::string &category) { self->SetCategory(const_cast<CKSTRING>(category.c_str())); }, (CKObjectDeclaration *, const std::string &), void), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "string GetCategory()", asFUNCTIONPR([](CKObjectDeclaration *self) -> std::string { return ScriptStringify(self->GetCategory()); }, (CKObjectDeclaration *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "string GetName()", asFUNCTIONPR([](CKObjectDeclaration *self) -> std::string { return ScriptStringify(self->GetName()); }, (CKObjectDeclaration *), std::string), asCALL_CDECL_OBJFIRST); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "int GetPluginIndex()", asMETHODPR(CKObjectDeclaration, GetPluginIndex, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "void SetPluginIndex(int)", asMETHODPR(CKObjectDeclaration, SetPluginIndex, (int), void), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "CKBehaviorPrototype@ GetProto()", asMETHODPR(CKObjectDeclaration, GetProto, (), CKBehaviorPrototype*), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectMethod("CKObjectDeclaration", "int GetManagerNeededCount()", asMETHODPR(CKObjectDeclaration, GetManagerNeededCount, (), int), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("CKObjectDeclaration", "CKGUID GetManagerNeeded(int index)", asMETHODPR(CKObjectDeclaration, GetManagerNeeded, (int), CKGUID), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
}
