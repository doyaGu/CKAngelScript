#include "ScriptVxMath.h"

#include <cassert>
#include <new>
#include <string>

#include "VxDefines.h"
#include "VxMath.h"

#include "add_on/scriptarray/scriptarray.h"

static float s_EPSILON = EPSILON;
static float s_PI = PI;
static float s_HALFPI = HALFPI;
static float s_NB_STDPIXEL_FORMATS = NB_STDPIXEL_FORMATS;
static float s_MAX_PIXEL_FORMATS = MAX_PIXEL_FORMATS;

static void RegisterVxMathTypedefs(asIScriptEngine *engine) {
    int r = 0;

    if (sizeof(void *) == 4) {
        r = engine->RegisterTypedef("FUNC_PTR", "uint"); assert(r >= 0);
        r = engine->RegisterTypedef("WIN_HANDLE", "uint"); assert(r >= 0);
        r = engine->RegisterTypedef("INSTANCE_HANDLE", "uint"); assert(r >= 0);
        r = engine->RegisterTypedef("GENERIC_HANDLE", "uint"); assert(r >= 0);
        r = engine->RegisterTypedef("BITMAP_HANDLE", "uint"); assert(r >= 0);
        r = engine->RegisterTypedef("FONT_HANDLE", "uint"); assert(r >= 0);
    } else {
        r = engine->RegisterTypedef("FUNC_PTR", "uint64"); assert(r >= 0);
        r = engine->RegisterTypedef("WIN_HANDLE", "uint64"); assert(r >= 0);
        r = engine->RegisterTypedef("INSTANCE_HANDLE", "uint64"); assert(r >= 0);
        r = engine->RegisterTypedef("GENERIC_HANDLE", "uint64"); assert(r >= 0);
        r = engine->RegisterTypedef("BITMAP_HANDLE", "uint64"); assert(r >= 0);
        r = engine->RegisterTypedef("FONT_HANDLE", "uint64"); assert(r >= 0);
    }
}

static void RegisterVxMathEnums(asIScriptEngine *engine) {
    int r = 0;

    // ProcessorsType
    r = engine->RegisterEnum("ProcessorsType"); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_UNKNOWN", PROC_UNKNOWN); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PENTIUM", PROC_PENTIUM); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PENTIUMMMX", PROC_PENTIUMMMX); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PENTIUMPRO", PROC_PENTIUMPRO); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_K63DNOW", PROC_K63DNOW); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PENTIUM2", PROC_PENTIUM2); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PENTIUM2XEON", PROC_PENTIUM2XEON); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PENTIUM2CELERON", PROC_PENTIUM2CELERON); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PENTIUM3", PROC_PENTIUM3); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_ATHLON", PROC_ATHLON); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PENTIUM4", PROC_PENTIUM4); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PPC_ARM", PROC_PPC_ARM); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PPC_MIPS", PROC_PPC_MIPS); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PPC_G3", PROC_PPC_G3); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PPC_G4", PROC_PPC_G4); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PSX2", PROC_PSX2); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_XBOX2", PROC_XBOX2); assert(r >= 0);
    r = engine->RegisterEnumValue("ProcessorsType", "PROC_PSP", PROC_PSP); assert(r >= 0);

    // VX_OSINFO
    r = engine->RegisterEnum("VX_OSINFO"); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_UNKNOWN", VXOS_UNKNOWN); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WIN31", VXOS_WIN31); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WIN95", VXOS_WIN95); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WIN98", VXOS_WIN98); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WINME", VXOS_WINME); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WINNT4", VXOS_WINNT4); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WIN2K", VXOS_WIN2K); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WINXP", VXOS_WINXP); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_MACOS9", VXOS_MACOS9); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_MACOSX", VXOS_MACOSX); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_XBOX", VXOS_XBOX); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_LINUXX86", VXOS_LINUXX86); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WINCE1", VXOS_WINCE1); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WINCE2", VXOS_WINCE2); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WINCE3", VXOS_WINCE3); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_PSX2", VXOS_PSX2); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_XBOX2", VXOS_XBOX2); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WINVISTA", VXOS_WINVISTA); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_PSP", VXOS_PSP); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_XBOX360", VXOS_XBOX360); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WII", VXOS_WII); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_OSINFO", "VXOS_WINSEVEN", VXOS_WINSEVEN); assert(r >= 0);

    // VX_PLATFORMINFO
    r = engine->RegisterEnum("VX_PLATFORMINFO"); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_UNKNOWN", VXPLATFORM_UNKNOWN); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_WINDOWS", VXPLATFORM_WINDOWS); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_MAC", VXPLATFORM_MAC); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_XBOX", VXPLATFORM_XBOX); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_WINCE", VXPLATFORM_WINCE); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_LINUX", VXPLATFORM_LINUX); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_PSX2", VXPLATFORM_PSX2); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_XBOX2", VXPLATFORM_XBOX2); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_PSP", VXPLATFORM_PSP); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PLATFORMINFO", "VXPLATFORM_WII", VXPLATFORM_WII); assert(r >= 0);

    // VX_PIXELFORMAT
    r = engine->RegisterEnum("VX_PIXELFORMAT"); assert(r >= 0);

    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "UNKNOWN_PF", UNKNOWN_PF); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_32_ARGB8888", _32_ARGB8888); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_32_RGB888", _32_RGB888); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_24_RGB888", _24_RGB888); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_RGB565", _16_RGB565); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_RGB555", _16_RGB555); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_ARGB1555", _16_ARGB1555); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_ARGB4444", _16_ARGB4444); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_8_RGB332", _8_RGB332); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_8_ARGB2222", _8_ARGB2222); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_32_ABGR8888", _32_ABGR8888); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_32_RGBA8888", _32_RGBA8888); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_32_BGRA8888", _32_BGRA8888); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_32_BGR888", _32_BGR888); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_24_BGR888", _24_BGR888); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_BGR565", _16_BGR565); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_BGR555", _16_BGR555); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_ABGR1555", _16_ABGR1555); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_ABGR4444", _16_ABGR4444); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_DXT1", _DXT1); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_DXT2", _DXT2); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_DXT3", _DXT3); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_DXT4", _DXT4); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_DXT5", _DXT5); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_V8U8", _16_V8U8); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_32_V16U16", _32_V16U16); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_16_L6V5U5", _16_L6V5U5); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_32_X8L8V8U8", _32_X8L8V8U8); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_8_ABGR8888_CLUT", _8_ABGR8888_CLUT); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_8_ARGB8888_CLUT", _8_ARGB8888_CLUT); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_4_ABGR8888_CLUT", _4_ABGR8888_CLUT); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_PIXELFORMAT", "_4_ARGB8888_CLUT", _4_ARGB8888_CLUT); assert(r >= 0);

    // VXCLIP_FLAGS
    r = engine->RegisterEnum("VXCLIP_FLAGS"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_FLAGS", "VXCLIP_LEFT", VXCLIP_LEFT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_FLAGS", "VXCLIP_RIGHT", VXCLIP_RIGHT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_FLAGS", "VXCLIP_TOP", VXCLIP_TOP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_FLAGS", "VXCLIP_BOTTOM", VXCLIP_BOTTOM); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_FLAGS", "VXCLIP_FRONT", VXCLIP_FRONT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_FLAGS", "VXCLIP_BACK", VXCLIP_BACK); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_FLAGS", "VXCLIP_BACKFRONT", VXCLIP_BACKFRONT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_FLAGS", "VXCLIP_ALL", VXCLIP_ALL); assert(r >= 0);

    // VXCLIP_BOXFLAGS
    r = engine->RegisterEnum("VXCLIP_BOXFLAGS"); assert(r >= 0);

    r = engine->RegisterEnumValue("VXCLIP_BOXFLAGS", "VXCLIP_BOXLEFT", VXCLIP_BOXLEFT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_BOXFLAGS", "VXCLIP_BOXBOTTOM", VXCLIP_BOXBOTTOM); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_BOXFLAGS", "VXCLIP_BOXBACK", VXCLIP_BOXBACK); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_BOXFLAGS", "VXCLIP_BOXRIGHT", VXCLIP_BOXRIGHT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_BOXFLAGS", "VXCLIP_BOXTOP", VXCLIP_BOXTOP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCLIP_BOXFLAGS", "VXCLIP_BOXFRONT", VXCLIP_BOXFRONT); assert(r >= 0);

    // CKRST_DPFLAGS
    r = engine->RegisterEnum("CKRST_DPFLAGS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TRANSFORM", CKRST_DP_TRANSFORM); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_LIGHT", CKRST_DP_LIGHT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_DOCLIP", CKRST_DP_DOCLIP); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_DIFFUSE", CKRST_DP_DIFFUSE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_SPECULAR", CKRST_DP_SPECULAR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_STAGESMASK", CKRST_DP_STAGESMASK); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_STAGES0", CKRST_DP_STAGES0); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_STAGES1", CKRST_DP_STAGES1); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_STAGES2", CKRST_DP_STAGES2); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_STAGES3", CKRST_DP_STAGES3); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_STAGES4", CKRST_DP_STAGES4); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_STAGES5", CKRST_DP_STAGES5); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_STAGES6", CKRST_DP_STAGES6); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_STAGES7", CKRST_DP_STAGES7); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_WEIGHTMASK", CKRST_DP_WEIGHTMASK); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_WEIGHTS1", CKRST_DP_WEIGHTS1); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_WEIGHTS2", CKRST_DP_WEIGHTS2); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_WEIGHTS3", CKRST_DP_WEIGHTS3); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_WEIGHTS4", CKRST_DP_WEIGHTS4); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_WEIGHTS5", CKRST_DP_WEIGHTS5); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_MATRIXPAL", CKRST_DP_MATRIXPAL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_VBUFFER", CKRST_DP_VBUFFER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_CL_VNT", CKRST_DP_TR_CL_VNT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_CL_VCST", CKRST_DP_TR_CL_VCST); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_CL_VCT", CKRST_DP_TR_CL_VCT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_CL_VCS", CKRST_DP_TR_CL_VCS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_CL_VC", CKRST_DP_TR_CL_VC); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_CL_V", CKRST_DP_TR_CL_V); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_CL_VCST", CKRST_DP_CL_VCST); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_CL_VCT", CKRST_DP_CL_VCT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_CL_VC", CKRST_DP_CL_VC); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_CL_V", CKRST_DP_CL_V); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_VNT", CKRST_DP_TR_VNT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_VCST", CKRST_DP_TR_VCST); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_VCT", CKRST_DP_TR_VCT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_VCS", CKRST_DP_TR_VCS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_VC", CKRST_DP_TR_VC); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_TR_V", CKRST_DP_TR_V); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_V", CKRST_DP_V); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_VC", CKRST_DP_VC); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_VCT", CKRST_DP_VCT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_DPFLAGS", "CKRST_DP_VCST", CKRST_DP_VCST); assert(r >= 0);

    // VX_LOCKFLAGS
    r = engine->RegisterEnum("VX_LOCKFLAGS"); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_LOCKFLAGS", "VX_LOCK_DEFAULT", VX_LOCK_DEFAULT); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_LOCKFLAGS", "VX_LOCK_WRITEONLY", VX_LOCK_WRITEONLY); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_LOCKFLAGS", "VX_LOCK_READONLY", VX_LOCK_READONLY); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_LOCKFLAGS", "VX_LOCK_DISCARD", VX_LOCK_DISCARD); assert(r >= 0);

    // VX_RESIZE_FLAGS
    r = engine->RegisterEnum("VX_RESIZE_FLAGS"); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_RESIZE_FLAGS", "VX_RESIZE_NOMOVE", VX_RESIZE_NOMOVE); assert(r >= 0);
    r = engine->RegisterEnumValue("VX_RESIZE_FLAGS", "VX_RESIZE_NOSIZE", VX_RESIZE_NOSIZE); assert(r >= 0);

    // VXLIGHT_TYPE
    r = engine->RegisterEnum("VXLIGHT_TYPE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXLIGHT_TYPE", "VX_LIGHTPOINT", VX_LIGHTPOINT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXLIGHT_TYPE", "VX_LIGHTSPOT", VX_LIGHTSPOT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXLIGHT_TYPE", "VX_LIGHTDIREC", VX_LIGHTDIREC); assert(r >= 0);
    r = engine->RegisterEnumValue("VXLIGHT_TYPE", "VX_LIGHTPARA", VX_LIGHTPARA); assert(r >= 0);

    // VXPRIMITIVETYPE
    r = engine->RegisterEnum("VXPRIMITIVETYPE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXPRIMITIVETYPE", "VX_POINTLIST", VX_POINTLIST); assert(r >= 0);
    r = engine->RegisterEnumValue("VXPRIMITIVETYPE", "VX_LINELIST", VX_LINELIST); assert(r >= 0);
    r = engine->RegisterEnumValue("VXPRIMITIVETYPE", "VX_LINESTRIP", VX_LINESTRIP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXPRIMITIVETYPE", "VX_TRIANGLELIST", VX_TRIANGLELIST); assert(r >= 0);
    r = engine->RegisterEnumValue("VXPRIMITIVETYPE", "VX_TRIANGLESTRIP", VX_TRIANGLESTRIP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXPRIMITIVETYPE", "VX_TRIANGLEFAN", VX_TRIANGLEFAN); assert(r >= 0);

    // VXBUFFER_TYPE
    r = engine->RegisterEnum("VXBUFFER_TYPE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBUFFER_TYPE", "VXBUFFER_BACKBUFFER", VXBUFFER_BACKBUFFER); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBUFFER_TYPE", "VXBUFFER_ZBUFFER", VXBUFFER_ZBUFFER); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBUFFER_TYPE", "VXBUFFER_STENCILBUFFER", VXBUFFER_STENCILBUFFER); assert(r >= 0);

    // VXTEXTURE_BLENDMODE
    r = engine->RegisterEnum("VXTEXTURE_BLENDMODE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_DECAL", VXTEXTUREBLEND_DECAL); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_MODULATE", VXTEXTUREBLEND_MODULATE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_DECALALPHA", VXTEXTUREBLEND_DECALALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_MODULATEALPHA", VXTEXTUREBLEND_MODULATEALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_DECALMASK", VXTEXTUREBLEND_DECALMASK); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_MODULATEMASK", VXTEXTUREBLEND_MODULATEMASK); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_COPY", VXTEXTUREBLEND_COPY); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_ADD", VXTEXTUREBLEND_ADD); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_DOTPRODUCT3", VXTEXTUREBLEND_DOTPRODUCT3); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_MAX", VXTEXTUREBLEND_MAX); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_BLENDMODE", "VXTEXTUREBLEND_MASK", VXTEXTUREBLEND_MASK); assert(r >= 0);

    // VXTEXTURE_FILTERMODE
    r = engine->RegisterEnum("VXTEXTURE_FILTERMODE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_FILTERMODE", "VXTEXTUREFILTER_NEAREST", VXTEXTUREFILTER_NEAREST); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_FILTERMODE", "VXTEXTUREFILTER_LINEAR", VXTEXTUREFILTER_LINEAR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_FILTERMODE", "VXTEXTUREFILTER_MIPNEAREST", VXTEXTUREFILTER_MIPNEAREST); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_FILTERMODE", "VXTEXTUREFILTER_MIPLINEAR", VXTEXTUREFILTER_MIPLINEAR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_FILTERMODE", "VXTEXTUREFILTER_LINEARMIPNEAREST", VXTEXTUREFILTER_LINEARMIPNEAREST); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_FILTERMODE", "VXTEXTUREFILTER_LINEARMIPLINEAR", VXTEXTUREFILTER_LINEARMIPLINEAR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_FILTERMODE", "VXTEXTUREFILTER_ANISOTROPIC", VXTEXTUREFILTER_ANISOTROPIC); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_FILTERMODE", "VXTEXTUREFILTER_MASK", VXTEXTUREFILTER_MASK); assert(r >= 0);

    // VXBLEND_MODE
    r = engine->RegisterEnum("VXBLEND_MODE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_ZERO", VXBLEND_ZERO); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_ONE", VXBLEND_ONE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_SRCCOLOR", VXBLEND_SRCCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_INVSRCCOLOR", VXBLEND_INVSRCCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_SRCALPHA", VXBLEND_SRCALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_INVSRCALPHA", VXBLEND_INVSRCALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_DESTALPHA", VXBLEND_DESTALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_INVDESTALPHA", VXBLEND_INVDESTALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_DESTCOLOR", VXBLEND_DESTCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_INVDESTCOLOR", VXBLEND_INVDESTCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_SRCALPHASAT", VXBLEND_SRCALPHASAT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_BOTHSRCALPHA", VXBLEND_BOTHSRCALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_BOTHINVSRCALPHA", VXBLEND_BOTHINVSRCALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLEND_MODE", "VXBLEND_MASK", VXBLEND_MASK); assert(r >= 0);

    // VXTEXTURE_ADDRESSMODE
    r = engine->RegisterEnum("VXTEXTURE_ADDRESSMODE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_ADDRESSMODE", "VXTEXTURE_ADDRESSWRAP", VXTEXTURE_ADDRESSWRAP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_ADDRESSMODE", "VXTEXTURE_ADDRESSMIRROR", VXTEXTURE_ADDRESSMIRROR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_ADDRESSMODE", "VXTEXTURE_ADDRESSCLAMP", VXTEXTURE_ADDRESSCLAMP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_ADDRESSMODE", "VXTEXTURE_ADDRESSBORDER", VXTEXTURE_ADDRESSBORDER); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_ADDRESSMODE", "VXTEXTURE_ADDRESSMIRRORONCE", VXTEXTURE_ADDRESSMIRRORONCE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXTURE_ADDRESSMODE", "VXTEXTURE_ADDRESSMASK", VXTEXTURE_ADDRESSMASK); assert(r >= 0);

    // VXFILL_MODE
    r = engine->RegisterEnum("VXFILL_MODE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXFILL_MODE", "VXFILL_POINT", VXFILL_POINT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXFILL_MODE", "VXFILL_WIREFRAME", VXFILL_WIREFRAME); assert(r >= 0);
    r = engine->RegisterEnumValue("VXFILL_MODE", "VXFILL_SOLID", VXFILL_SOLID); assert(r >= 0);
    r = engine->RegisterEnumValue("VXFILL_MODE", "VXFILL_MASK", VXFILL_MASK); assert(r >= 0);

    // VXSHADE_MODE
    r = engine->RegisterEnum("VXSHADE_MODE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSHADE_MODE", "VXSHADE_FLAT", VXSHADE_FLAT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSHADE_MODE", "VXSHADE_GOURAUD", VXSHADE_GOURAUD); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSHADE_MODE", "VXSHADE_PHONG", VXSHADE_PHONG); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSHADE_MODE", "VXSHADE_MASK", VXSHADE_MASK); assert(r >= 0);

    // VXCULL
    r = engine->RegisterEnum("VXCULL"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCULL", "VXCULL_NONE", VXCULL_NONE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCULL", "VXCULL_CW", VXCULL_CW); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCULL", "VXCULL_CCW", VXCULL_CCW); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCULL", "VXCULL_MASK", VXCULL_MASK); assert(r >= 0);

    // VXCMPFUNC
    r = engine->RegisterEnum("VXCMPFUNC"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCMPFUNC", "VXCMP_NEVER", VXCMP_NEVER); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCMPFUNC", "VXCMP_LESS", VXCMP_LESS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCMPFUNC", "VXCMP_EQUAL", VXCMP_EQUAL); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCMPFUNC", "VXCMP_LESSEQUAL", VXCMP_LESSEQUAL); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCMPFUNC", "VXCMP_GREATER", VXCMP_GREATER); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCMPFUNC", "VXCMP_NOTEQUAL", VXCMP_NOTEQUAL); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCMPFUNC", "VXCMP_GREATEREQUAL", VXCMP_GREATEREQUAL); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCMPFUNC", "VXCMP_ALWAYS", VXCMP_ALWAYS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCMPFUNC", "VXCMP_MASK", VXCMP_MASK); assert(r >= 0);

    // VXSPRITE_RENDEROPTIONS
    r = engine->RegisterEnum("VXSPRITE_RENDEROPTIONS"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSPRITE_RENDEROPTIONS", "VXSPRITE_NONE", VXSPRITE_NONE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSPRITE_RENDEROPTIONS", "VXSPRITE_ALPHATEST", VXSPRITE_ALPHATEST); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSPRITE_RENDEROPTIONS", "VXSPRITE_BLEND", VXSPRITE_BLEND); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSPRITE_RENDEROPTIONS", "VXSPRITE_MODULATE", VXSPRITE_MODULATE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSPRITE_RENDEROPTIONS", "VXSPRITE_FILTER", VXSPRITE_FILTER); assert(r >= 0);

    // VXSPRITE_RENDEROPTIONS2
    r = engine->RegisterEnum("VXSPRITE_RENDEROPTIONS2"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSPRITE_RENDEROPTIONS2", "VXSPRITE2_NONE", VXSPRITE2_NONE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSPRITE_RENDEROPTIONS2", "VXSPRITE2_DISABLE_AA_CORRECTION", VXSPRITE2_DISABLE_AA_CORRECTION); assert(r >= 0);

    // VXSTENCILOP
    r = engine->RegisterEnum("VXSTENCILOP"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSTENCILOP", "VXSTENCILOP_KEEP", VXSTENCILOP_KEEP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSTENCILOP", "VXSTENCILOP_ZERO", VXSTENCILOP_ZERO); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSTENCILOP", "VXSTENCILOP_REPLACE", VXSTENCILOP_REPLACE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSTENCILOP", "VXSTENCILOP_INCRSAT", VXSTENCILOP_INCRSAT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSTENCILOP", "VXSTENCILOP_DECRSAT", VXSTENCILOP_DECRSAT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSTENCILOP", "VXSTENCILOP_INVERT", VXSTENCILOP_INVERT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSTENCILOP", "VXSTENCILOP_INCR", VXSTENCILOP_INCR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSTENCILOP", "VXSTENCILOP_DECR", VXSTENCILOP_DECR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXSTENCILOP", "VXSTENCILOP_MASK", VXSTENCILOP_MASK); assert(r >= 0);

    // VXFOG_MODE
    r = engine->RegisterEnum("VXFOG_MODE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXFOG_MODE", "VXFOG_NONE", VXFOG_NONE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXFOG_MODE", "VXFOG_EXP", VXFOG_EXP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXFOG_MODE", "VXFOG_EXP2", VXFOG_EXP2); assert(r >= 0);
    r = engine->RegisterEnumValue("VXFOG_MODE", "VXFOG_LINEAR", VXFOG_LINEAR); assert(r >= 0);

    // CKRST_TEXTUREOP
    r = engine->RegisterEnum("CKRST_TEXTUREOP"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_DISABLE", CKRST_TOP_DISABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_SELECTARG1", CKRST_TOP_SELECTARG1); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_SELECTARG2", CKRST_TOP_SELECTARG2); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_MODULATE", CKRST_TOP_MODULATE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_MODULATE2X", CKRST_TOP_MODULATE2X); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_MODULATE4X", CKRST_TOP_MODULATE4X); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_ADD", CKRST_TOP_ADD); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_ADDSIGNED", CKRST_TOP_ADDSIGNED); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_ADDSIGNED2X", CKRST_TOP_ADDSIGNED2X); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_SUBTRACT", CKRST_TOP_SUBTRACT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_ADDSMOOTH", CKRST_TOP_ADDSMOOTH); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_BLENDDIFFUSEALPHA", CKRST_TOP_BLENDDIFFUSEALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_BLENDTEXTUREALPHA", CKRST_TOP_BLENDTEXTUREALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_BLENDFACTORALPHA", CKRST_TOP_BLENDFACTORALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_BLENDTEXTUREALPHAPM", CKRST_TOP_BLENDTEXTUREALPHAPM); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_BLENDCURRENTALPHA", CKRST_TOP_BLENDCURRENTALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_PREMODULATE", CKRST_TOP_PREMODULATE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_MODULATEALPHA_ADDCOLOR", CKRST_TOP_MODULATEALPHA_ADDCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_MODULATECOLOR_ADDALPHA", CKRST_TOP_MODULATECOLOR_ADDALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_MODULATEINVALPHA_ADDCOLOR", CKRST_TOP_MODULATEINVALPHA_ADDCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_MODULATEINVCOLOR_ADDALPHA", CKRST_TOP_MODULATEINVCOLOR_ADDALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_BUMPENVMAP", CKRST_TOP_BUMPENVMAP); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_BUMPENVMAPLUMINANCE", CKRST_TOP_BUMPENVMAPLUMINANCE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_DOTPRODUCT3", CKRST_TOP_DOTPRODUCT3); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_MULTIPLYADD", CKRST_TOP_MULTIPLYADD); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREOP", "CKRST_TOP_LERP", CKRST_TOP_LERP); assert(r >= 0);

    // CKRST_TEXTUREARG
    r = engine->RegisterEnum("CKRST_TEXTUREARG"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREARG", "CKRST_TA_DIFFUSE", CKRST_TA_DIFFUSE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREARG", "CKRST_TA_CURRENT", CKRST_TA_CURRENT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREARG", "CKRST_TA_TEXTURE", CKRST_TA_TEXTURE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREARG", "CKRST_TA_TFACTOR", CKRST_TA_TFACTOR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREARG", "CKRST_TA_SPECULAR", CKRST_TA_SPECULAR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREARG", "CKRST_TA_TEMP", CKRST_TA_TEMP); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREARG", "CKRST_TA_COMPLEMENT", CKRST_TA_COMPLEMENT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTUREARG", "CKRST_TA_ALPHAREPLICATE", CKRST_TA_ALPHAREPLICATE); assert(r >= 0);

    // CKRST_TEXTURETRANSFORMFLAGS
    r = engine->RegisterEnum("CKRST_TEXTURETRANSFORMFLAGS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURETRANSFORMFLAGS", "CKRST_TTF_NONE", CKRST_TTF_NONE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURETRANSFORMFLAGS", "CKRST_TTF_COUNT1", CKRST_TTF_COUNT1); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURETRANSFORMFLAGS", "CKRST_TTF_COUNT2", CKRST_TTF_COUNT2); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURETRANSFORMFLAGS", "CKRST_TTF_COUNT3", CKRST_TTF_COUNT3); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURETRANSFORMFLAGS", "CKRST_TTF_COUNT4", CKRST_TTF_COUNT4); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURETRANSFORMFLAGS", "CKRST_TTF_PROJECTED", CKRST_TTF_PROJECTED); assert(r >= 0);

    // CKRST_TEXTURESTAGESTATETYPE
    r = engine->RegisterEnum("CKRST_TEXTURESTAGESTATETYPE"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_OP", CKRST_TSS_OP); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_ARG1", CKRST_TSS_ARG1); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_ARG2", CKRST_TSS_ARG2); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_AOP", CKRST_TSS_AOP); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_AARG1", CKRST_TSS_AARG1); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_AARG2", CKRST_TSS_AARG2); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_BUMPENVMAT00", CKRST_TSS_BUMPENVMAT00); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_BUMPENVMAT01", CKRST_TSS_BUMPENVMAT01); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_BUMPENVMAT10", CKRST_TSS_BUMPENVMAT10); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_BUMPENVMAT11", CKRST_TSS_BUMPENVMAT11); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_TEXCOORDINDEX", CKRST_TSS_TEXCOORDINDEX); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_ADDRESS", CKRST_TSS_ADDRESS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_ADDRESSU", CKRST_TSS_ADDRESSU); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_ADDRESSV", CKRST_TSS_ADDRESSV); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_BORDERCOLOR", CKRST_TSS_BORDERCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_MAGFILTER", CKRST_TSS_MAGFILTER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_MINFILTER", CKRST_TSS_MINFILTER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_MIPMAPLODBIAS", CKRST_TSS_MIPMAPLODBIAS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_MAXMIPMLEVEL", CKRST_TSS_MAXMIPMLEVEL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_MAXANISOTROPY", CKRST_TSS_MAXANISOTROPY); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_BUMPENVLSCALE", CKRST_TSS_BUMPENVLSCALE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_BUMPENVLOFFSET", CKRST_TSS_BUMPENVLOFFSET); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_TEXTURETRANSFORMFLAGS", CKRST_TSS_TEXTURETRANSFORMFLAGS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_ADDRESW", CKRST_TSS_ADDRESW); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_COLORARG0", CKRST_TSS_COLORARG0); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_ALPHAARG0", CKRST_TSS_ALPHAARG0); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_RESULTARG0", CKRST_TSS_RESULTARG0); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_TEXTUREMAPBLEND", CKRST_TSS_TEXTUREMAPBLEND); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_STAGEBLEND", CKRST_TSS_STAGEBLEND); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURESTAGESTATETYPE", "CKRST_TSS_MAXSTATE", CKRST_TSS_MAXSTATE); assert(r >= 0);

    // VXTEXCOORD_GEN
    r = engine->RegisterEnum("VXTEXCOORD_GEN"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXCOORD_GEN", "VXTEXCOORD_SKIP", VXTEXCOORD_SKIP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXCOORD_GEN", "VXTEXCOORD_PROJNORMAL", VXTEXCOORD_PROJNORMAL); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXCOORD_GEN", "VXTEXCOORD_PROJPOSITION", VXTEXCOORD_PROJPOSITION); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXCOORD_GEN", "VXTEXCOORD_PROJREFLECT", VXTEXCOORD_PROJREFLECT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXCOORD_GEN", "VXTEXCOORD_MASK", VXTEXCOORD_MASK); assert(r >= 0);

    // VXWRAP_MODE
    r = engine->RegisterEnum("VXWRAP_MODE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXWRAP_MODE", "VXWRAP_U", VXWRAP_U); assert(r >= 0);
    r = engine->RegisterEnumValue("VXWRAP_MODE", "VXWRAP_V", VXWRAP_V); assert(r >= 0);
    r = engine->RegisterEnumValue("VXWRAP_MODE", "VXWRAP_S", VXWRAP_S); assert(r >= 0);
    r = engine->RegisterEnumValue("VXWRAP_MODE", "VXWRAP_T", VXWRAP_T); assert(r >= 0);
    r = engine->RegisterEnumValue("VXWRAP_MODE", "VXWRAP_MASK", VXWRAP_MASK); assert(r >= 0);

    // VXBLENDOP
    r = engine->RegisterEnum("VXBLENDOP"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLENDOP", "VXBLENDOP_ADD", VXBLENDOP_ADD); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLENDOP", "VXBLENDOP_SUBTRACT", VXBLENDOP_SUBTRACT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLENDOP", "VXBLENDOP_REVSUBTRACT", VXBLENDOP_REVSUBTRACT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLENDOP", "VXBLENDOP_MIN", VXBLENDOP_MIN); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLENDOP", "VXBLENDOP_MAX", VXBLENDOP_MAX); assert(r >= 0);
    r = engine->RegisterEnumValue("VXBLENDOP", "VXBLENDOP_MASK", VXBLENDOP_MASK); assert(r >= 0);

    // VXVERTEXBLENDFLAGS
    r = engine->RegisterEnum("VXVERTEXBLENDFLAGS"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXVERTEXBLENDFLAGS", "VXVBLEND_DISABLE", VXVBLEND_DISABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXVERTEXBLENDFLAGS", "VXVBLEND_1WEIGHTS", VXVBLEND_1WEIGHTS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXVERTEXBLENDFLAGS", "VXVBLEND_2WEIGHTS", VXVBLEND_2WEIGHTS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXVERTEXBLENDFLAGS", "VXVBLEND_3WEIGHTS", VXVBLEND_3WEIGHTS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXVERTEXBLENDFLAGS", "VXVBLEND_TWEENING", VXVBLEND_TWEENING); assert(r >= 0);
    r = engine->RegisterEnumValue("VXVERTEXBLENDFLAGS", "VXVBLEND_0WEIGHTS", VXVBLEND_0WEIGHTS); assert(r >= 0);

    // VXRENDERSTATETYPE
    r = engine->RegisterEnum("VXRENDERSTATETYPE"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_ANTIALIAS", VXRENDERSTATE_ANTIALIAS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_TEXTUREPERSPECTIVE", VXRENDERSTATE_TEXTUREPERSPECTIVE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_ZENABLE", VXRENDERSTATE_ZENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_FILLMODE", VXRENDERSTATE_FILLMODE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_SHADEMODE", VXRENDERSTATE_SHADEMODE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_LINEPATTERN", VXRENDERSTATE_LINEPATTERN); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_ZWRITEENABLE", VXRENDERSTATE_ZWRITEENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_ALPHATESTENABLE", VXRENDERSTATE_ALPHATESTENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_SRCBLEND", VXRENDERSTATE_SRCBLEND); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_DESTBLEND", VXRENDERSTATE_DESTBLEND); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_CULLMODE", VXRENDERSTATE_CULLMODE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_ZFUNC", VXRENDERSTATE_ZFUNC); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_ALPHAREF", VXRENDERSTATE_ALPHAREF); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_ALPHAFUNC", VXRENDERSTATE_ALPHAFUNC); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_DITHERENABLE", VXRENDERSTATE_DITHERENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_ALPHABLENDENABLE", VXRENDERSTATE_ALPHABLENDENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_FOGENABLE", VXRENDERSTATE_FOGENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_SPECULARENABLE", VXRENDERSTATE_SPECULARENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_FOGCOLOR", VXRENDERSTATE_FOGCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_FOGPIXELMODE", VXRENDERSTATE_FOGPIXELMODE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_FOGSTART", VXRENDERSTATE_FOGSTART); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_FOGEND", VXRENDERSTATE_FOGEND); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_FOGDENSITY", VXRENDERSTATE_FOGDENSITY); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_EDGEANTIALIAS", VXRENDERSTATE_EDGEANTIALIAS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_ZBIAS", VXRENDERSTATE_ZBIAS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_RANGEFOGENABLE", VXRENDERSTATE_RANGEFOGENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_STENCILENABLE", VXRENDERSTATE_STENCILENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_STENCILFAIL", VXRENDERSTATE_STENCILFAIL); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_STENCILZFAIL", VXRENDERSTATE_STENCILZFAIL); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_STENCILPASS", VXRENDERSTATE_STENCILPASS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_STENCILFUNC", VXRENDERSTATE_STENCILFUNC); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_STENCILREF", VXRENDERSTATE_STENCILREF); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_STENCILMASK", VXRENDERSTATE_STENCILMASK); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_STENCILWRITEMASK", VXRENDERSTATE_STENCILWRITEMASK); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_TEXTUREFACTOR", VXRENDERSTATE_TEXTUREFACTOR); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_WRAP0", VXRENDERSTATE_WRAP0); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_WRAP1", VXRENDERSTATE_WRAP1); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_WRAP2", VXRENDERSTATE_WRAP2); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_WRAP3", VXRENDERSTATE_WRAP3); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_WRAP4", VXRENDERSTATE_WRAP4); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_WRAP5", VXRENDERSTATE_WRAP5); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_WRAP6", VXRENDERSTATE_WRAP6); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_WRAP7", VXRENDERSTATE_WRAP7); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_CLIPPING", VXRENDERSTATE_CLIPPING); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_LIGHTING", VXRENDERSTATE_LIGHTING); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_AMBIENT", VXRENDERSTATE_AMBIENT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_FOGVERTEXMODE", VXRENDERSTATE_FOGVERTEXMODE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_COLORVERTEX", VXRENDERSTATE_COLORVERTEX); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_LOCALVIEWER", VXRENDERSTATE_LOCALVIEWER); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_NORMALIZENORMALS", VXRENDERSTATE_NORMALIZENORMALS); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_DIFFUSEFROMVERTEX", VXRENDERSTATE_DIFFUSEFROMVERTEX); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_SPECULARFROMVERTEX", VXRENDERSTATE_SPECULARFROMVERTEX); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_AMBIENTFROMVERTEX", VXRENDERSTATE_AMBIENTFROMVERTEX); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_EMISSIVEFROMVERTEX", VXRENDERSTATE_EMISSIVEFROMVERTEX); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_VERTEXBLEND", VXRENDERSTATE_VERTEXBLEND); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_SOFTWAREVPROCESSING", VXRENDERSTATE_SOFTWAREVPROCESSING); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_POINTSIZE", VXRENDERSTATE_POINTSIZE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_POINTSIZE_MIN", VXRENDERSTATE_POINTSIZE_MIN); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_POINTSIZE_MAX", VXRENDERSTATE_POINTSIZE_MAX); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_POINTSPRITEENABLE", VXRENDERSTATE_POINTSPRITEENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_POINTSCALEENABLE", VXRENDERSTATE_POINTSCALEENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_POINTSCALE_A", VXRENDERSTATE_POINTSCALE_A); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_POINTSCALE_B", VXRENDERSTATE_POINTSCALE_B); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_POINTSCALE_C", VXRENDERSTATETYPE::VXRENDERSTATE_POINTSCALE_C); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_CLIPPLANEENABLE", VXRENDERSTATE_CLIPPLANEENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_INDEXVBLENDENABLE", VXRENDERSTATE_INDEXVBLENDENABLE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_BLENDOP", VXRENDERSTATE_BLENDOP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_TEXTURETARGET", VXRENDERSTATE_TEXTURETARGET); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_INVERSEWINDING", VXRENDERSTATE_INVERSEWINDING); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_MAXSTATE", VXRENDERSTATE_MAXSTATE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRENDERSTATETYPE", "VXRENDERSTATE_FORCE_DWORD", VXRENDERSTATE_FORCE_DWORD); assert(r >= 0);

    // VxBpps
    r = engine->RegisterEnum("VxBpps"); assert(r >= 0);
    r = engine->RegisterEnumValue("VxBpps", "VX_BPP1", VX_BPP1); assert(r >= 0);
    r = engine->RegisterEnumValue("VxBpps", "VX_BPP2", VX_BPP2); assert(r >= 0);
    r = engine->RegisterEnumValue("VxBpps", "VX_BPP4", VX_BPP4); assert(r >= 0);
    r = engine->RegisterEnumValue("VxBpps", "VX_BPP8", VX_BPP8); assert(r >= 0);
    r = engine->RegisterEnumValue("VxBpps", "VX_BPP16", VX_BPP16); assert(r >= 0);
    r = engine->RegisterEnumValue("VxBpps", "VX_BPP24", VX_BPP24); assert(r >= 0);
    r = engine->RegisterEnumValue("VxBpps", "VX_BPP32", VX_BPP32); assert(r >= 0);

    // CKRST_RSTFAMILY
    r = engine->RegisterEnum("CKRST_RSTFAMILY"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RSTFAMILY", "CKRST_DIRECTX", CKRST_DIRECTX); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RSTFAMILY", "CKRST_OPENGL", CKRST_OPENGL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RSTFAMILY", "CKRST_SOFT", CKRST_SOFT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RSTFAMILY", "CKRST_ALCHEMY", CKRST_ALCHEMY); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RSTFAMILY", "CKRST_UNKNOWN", CKRST_UNKNOWN); assert(r >= 0);

    // CKRST_SPECIFICCAPS
    r = engine->RegisterEnum("CKRST_SPECIFICCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_SPRITEASTEXTURES", CKRST_SPECIFICCAPS_SPRITEASTEXTURES); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_CLAMPEDGEALPHA", CKRST_SPECIFICCAPS_CLAMPEDGEALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_CANDOVERTEXBUFFER", CKRST_SPECIFICCAPS_CANDOVERTEXBUFFER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_GLATTENUATIONMODEL", CKRST_SPECIFICCAPS_GLATTENUATIONMODEL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_SOFTWARE", CKRST_SPECIFICCAPS_SOFTWARE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_HARDWARE", CKRST_SPECIFICCAPS_HARDWARE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_HARDWARETL", CKRST_SPECIFICCAPS_HARDWARETL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_COPYTEXTURE", CKRST_SPECIFICCAPS_COPYTEXTURE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_DX5", CKRST_SPECIFICCAPS_DX5); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_DX7", CKRST_SPECIFICCAPS_DX7); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_DX8", CKRST_SPECIFICCAPS_DX8); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_DX9", CKRST_SPECIFICCAPS_DX9); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_SUPPORTSHADERS", CKRST_SPECIFICCAPS_SUPPORTSHADERS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_POINTSPRITES", CKRST_SPECIFICCAPS_POINTSPRITES); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_VERTEXCOLORABGR", CKRST_SPECIFICCAPS_VERTEXCOLORABGR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_BLENDTEXTEFFECT", CKRST_SPECIFICCAPS_BLENDTEXTEFFECT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_CANDOINDEXBUFFER", CKRST_SPECIFICCAPS_CANDOINDEXBUFFER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_HW_SKINNING", CKRST_SPECIFICCAPS_HW_SKINNING); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_SPECIFICCAPS", "CKRST_SPECIFICCAPS_AUTGENMIPMAP", CKRST_SPECIFICCAPS_AUTGENMIPMAP); assert(r >= 0);

    // CKRST_TFILTERCAPS
    r = engine->RegisterEnum("CKRST_TFILTERCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TFILTERCAPS", "CKRST_TFILTERCAPS_NEAREST", CKRST_TFILTERCAPS_NEAREST); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TFILTERCAPS", "CKRST_TFILTERCAPS_LINEAR", CKRST_TFILTERCAPS_LINEAR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TFILTERCAPS", "CKRST_TFILTERCAPS_MIPNEAREST", CKRST_TFILTERCAPS_MIPNEAREST); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TFILTERCAPS", "CKRST_TFILTERCAPS_MIPLINEAR", CKRST_TFILTERCAPS_MIPLINEAR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TFILTERCAPS", "CKRST_TFILTERCAPS_LINEARMIPNEAREST", CKRST_TFILTERCAPS_LINEARMIPNEAREST); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TFILTERCAPS", "CKRST_TFILTERCAPS_LINEARMIPLINEAR", CKRST_TFILTERCAPS_LINEARMIPLINEAR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TFILTERCAPS", "CKRST_TFILTERCAPS_ANISOTROPIC", CKRST_TFILTERCAPS_ANISOTROPIC); assert(r >= 0);

    // CKRST_TADDRESSCAPS
    r = engine->RegisterEnum("CKRST_TADDRESSCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TADDRESSCAPS", "CKRST_TADDRESSCAPS_WRAP", CKRST_TADDRESSCAPS_WRAP); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TADDRESSCAPS", "CKRST_TADDRESSCAPS_MIRROR", CKRST_TADDRESSCAPS_MIRROR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TADDRESSCAPS", "CKRST_TADDRESSCAPS_CLAMP", CKRST_TADDRESSCAPS_CLAMP); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TADDRESSCAPS", "CKRST_TADDRESSCAPS_BORDER", CKRST_TADDRESSCAPS_BORDER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TADDRESSCAPS", "CKRST_TADDRESSCAPS_INDEPENDENTUV", CKRST_TADDRESSCAPS_INDEPENDENTUV); assert(r >= 0);

    // CKRST_TEXTURECAPS
    r = engine->RegisterEnum("CKRST_TEXTURECAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURECAPS", "CKRST_TEXTURECAPS_PERSPECTIVE", CKRST_TEXTURECAPS_PERSPECTIVE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURECAPS", "CKRST_TEXTURECAPS_POW2", CKRST_TEXTURECAPS_POW2); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURECAPS", "CKRST_TEXTURECAPS_ALPHA", CKRST_TEXTURECAPS_ALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURECAPS", "CKRST_TEXTURECAPS_SQUAREONLY", CKRST_TEXTURECAPS_SQUAREONLY); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURECAPS", "CKRST_TEXTURECAPS_CONDITIONALNONPOW2", CKRST_TEXTURECAPS_CONDITIONALNONPOW2); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURECAPS", "CKRST_TEXTURECAPS_PROJECTED", CKRST_TEXTURECAPS_PROJECTED); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURECAPS", "CKRST_TEXTURECAPS_CUBEMAP", CKRST_TEXTURECAPS_CUBEMAP); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_TEXTURECAPS", "CKRST_TEXTURECAPS_VOLUMEMAP", CKRST_TEXTURECAPS_VOLUMEMAP); assert(r >= 0);

    // CKRST_STENCILCAPS
    r = engine->RegisterEnum("CKRST_STENCILCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_STENCILCAPS", "CKRST_STENCILCAPS_KEEP", CKRST_STENCILCAPS_KEEP); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_STENCILCAPS", "CKRST_STENCILCAPS_ZERO", CKRST_STENCILCAPS_ZERO); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_STENCILCAPS", "CKRST_STENCILCAPS_REPLACE", CKRST_STENCILCAPS_REPLACE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_STENCILCAPS", "CKRST_STENCILCAPS_INCRSAT", CKRST_STENCILCAPS_INCRSAT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_STENCILCAPS", "CKRST_STENCILCAPS_DECRSAT", CKRST_STENCILCAPS_DECRSAT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_STENCILCAPS", "CKRST_STENCILCAPS_INVERT", CKRST_STENCILCAPS_INVERT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_STENCILCAPS", "CKRST_STENCILCAPS_INCR", CKRST_STENCILCAPS_INCR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_STENCILCAPS", "CKRST_STENCILCAPS_DECR", CKRST_STENCILCAPS_DECR); assert(r >= 0);

    // CKRST_MISCCAPS
    r = engine->RegisterEnum("CKRST_MISCCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_MISCCAPS", "CKRST_MISCCAPS_MASKZ", CKRST_MISCCAPS_MASKZ); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_MISCCAPS", "CKRST_MISCCAPS_CONFORMANT", CKRST_MISCCAPS_CONFORMANT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_MISCCAPS", "CKRST_MISCCAPS_CULLNONE", CKRST_MISCCAPS_CULLNONE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_MISCCAPS", "CKRST_MISCCAPS_CULLCW", CKRST_MISCCAPS_CULLCW); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_MISCCAPS", "CKRST_MISCCAPS_CULLCCW", CKRST_MISCCAPS_CULLCCW); assert(r >= 0);

    // CKRST_RASTERCAPS
    r = engine->RegisterEnum("CKRST_RASTERCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_DITHER", CKRST_RASTERCAPS_DITHER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_ZTEST", CKRST_RASTERCAPS_ZTEST); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_SUBPIXEL", CKRST_RASTERCAPS_SUBPIXEL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_FOGVERTEX", CKRST_RASTERCAPS_FOGVERTEX); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_FOGPIXEL", CKRST_RASTERCAPS_FOGPIXEL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_ZBIAS", CKRST_RASTERCAPS_ZBIAS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_ZBUFFERLESSHSR", CKRST_RASTERCAPS_ZBUFFERLESSHSR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_FOGRANGE", CKRST_RASTERCAPS_FOGRANGE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_ANISOTROPY", CKRST_RASTERCAPS_ANISOTROPY); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_WBUFFER", CKRST_RASTERCAPS_WBUFFER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_WFOG", CKRST_RASTERCAPS_WFOG); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_RASTERCAPS", "CKRST_RASTERCAPS_ZFOG", CKRST_RASTERCAPS_ZFOG); assert(r >= 0);

    // CKRST_BLENDCAPS
    r = engine->RegisterEnum("CKRST_BLENDCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_ZERO", CKRST_BLENDCAPS_ZERO); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_ONE", CKRST_BLENDCAPS_ONE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_SRCCOLOR", CKRST_BLENDCAPS_SRCCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_INVSRCCOLOR", CKRST_BLENDCAPS_INVSRCCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_SRCALPHA", CKRST_BLENDCAPS_SRCALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_INVSRCALPHA", CKRST_BLENDCAPS_INVSRCALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_DESTALPHA", CKRST_BLENDCAPS_DESTALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_INVDESTALPHA", CKRST_BLENDCAPS_INVDESTALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_DESTCOLOR", CKRST_BLENDCAPS_DESTCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_INVDESTCOLOR", CKRST_BLENDCAPS_INVDESTCOLOR); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_SRCALPHASAT", CKRST_BLENDCAPS_SRCALPHASAT); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_BOTHSRCALPHA", CKRST_BLENDCAPS_BOTHSRCALPHA); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_BLENDCAPS", "CKRST_BLENDCAPS_BOTHINVSRCALPHA", CKRST_BLENDCAPS_BOTHINVSRCALPHA); assert(r >= 0);

    // CKRST_CMPCAPS
    r = engine->RegisterEnum("CKRST_CMPCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_CMPCAPS", "CKRST_CMPCAPS_NEVER", CKRST_CMPCAPS_NEVER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_CMPCAPS", "CKRST_CMPCAPS_LESS", CKRST_CMPCAPS_LESS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_CMPCAPS", "CKRST_CMPCAPS_EQUAL", CKRST_CMPCAPS_EQUAL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_CMPCAPS", "CKRST_CMPCAPS_LESSEQUAL", CKRST_CMPCAPS_LESSEQUAL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_CMPCAPS", "CKRST_CMPCAPS_GREATER", CKRST_CMPCAPS_GREATER); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_CMPCAPS", "CKRST_CMPCAPS_NOTEQUAL", CKRST_CMPCAPS_NOTEQUAL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_CMPCAPS", "CKRST_CMPCAPS_GREATEREQUAL", CKRST_CMPCAPS_GREATEREQUAL); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_CMPCAPS", "CKRST_CMPCAPS_ALWAYS", CKRST_CMPCAPS_ALWAYS); assert(r >= 0);

    // CKRST_VTXCAPS
    r = engine->RegisterEnum("CKRST_VTXCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_VTXCAPS", "CKRST_VTXCAPS_TEXGEN", CKRST_VTXCAPS_TEXGEN); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_VTXCAPS", "CKRST_VTXCAPS_MATERIALSOURCE", CKRST_VTXCAPS_MATERIALSOURCE); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_VTXCAPS", "CKRST_VTXCAPS_VERTEXFOG", CKRST_VTXCAPS_VERTEXFOG); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_VTXCAPS", "CKRST_VTXCAPS_DIRECTIONALLIGHTS", CKRST_VTXCAPS_DIRECTIONALLIGHTS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_VTXCAPS", "CKRST_VTXCAPS_POSITIONALLIGHTS", CKRST_VTXCAPS_POSITIONALLIGHTS); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_VTXCAPS", "CKRST_VTXCAPS_LOCALVIEWER", CKRST_VTXCAPS_LOCALVIEWER); assert(r >= 0);

    // CKRST_2DCAPS
    r = engine->RegisterEnum("CKRST_2DCAPS"); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_2DCAPS", "CKRST_2DCAPS_WINDOWED", CKRST_2DCAPS_WINDOWED); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_2DCAPS", "CKRST_2DCAPS_3D", CKRST_2DCAPS_3D); assert(r >= 0);
    r = engine->RegisterEnumValue("CKRST_2DCAPS", "CKRST_2DCAPS_GDI", CKRST_2DCAPS_GDI); assert(r >= 0);
}

static void RegisterVxMathObjectTypes(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterObjectType("CKRECT", sizeof(CKRECT), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<CKRECT>()); assert(r >= 0);
    r = engine->RegisterObjectProperty("CKRECT", "int left", asOFFSET(CKRECT, left)); assert(r >= 0);
    r = engine->RegisterObjectProperty("CKRECT", "int top", asOFFSET(CKRECT, top)); assert(r >= 0);
    r = engine->RegisterObjectProperty("CKRECT", "int right", asOFFSET(CKRECT, right)); assert(r >= 0);
    r = engine->RegisterObjectProperty("CKRECT", "int bottom", asOFFSET(CKRECT, bottom)); assert(r >= 0);

    r = engine->RegisterObjectType("CKPOINT", sizeof(CKPOINT), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<CKPOINT>()); assert(r >= 0);
    r = engine->RegisterObjectProperty("CKPOINT", "int x", asOFFSET(CKPOINT, x)); assert(r >= 0);
    r = engine->RegisterObjectProperty("CKPOINT", "int y", asOFFSET(CKPOINT, y)); assert(r >= 0);

    r = engine->RegisterObjectType("VxMutex", sizeof(VxMutex), asOBJ_VALUE | asGetTypeTraits<VxMutex>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxMutexLock", sizeof(VxMutexLock), asOBJ_VALUE | asGetTypeTraits<VxMutexLock>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxTimeProfiler", sizeof(VxTimeProfiler), asOBJ_VALUE | asGetTypeTraits<VxTimeProfiler>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxSharedLibrary", sizeof(VxSharedLibrary), asOBJ_VALUE | asGetTypeTraits<VxSharedLibrary>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxMemoryMappedFile", sizeof(VxMemoryMappedFile), asOBJ_VALUE | asGetTypeTraits<VxMemoryMappedFile>()); assert(r >= 0);

    r = engine->RegisterObjectType("CKPathSplitter", sizeof(CKPathSplitter), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<CKPathSplitter>()); assert(r >= 0);

    r = engine->RegisterObjectType("CKDirectoryParser", sizeof(CKDirectoryParser), asOBJ_VALUE | asGetTypeTraits<CKDirectoryParser>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxVector", sizeof(VxVector), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxVector>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxVector4", sizeof(VxVector4), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxVector4>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxBbox", sizeof(VxBbox), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxBbox>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxCompressedVector", sizeof(VxCompressedVector), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxCompressedVector>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxCompressedVectorOld", sizeof(VxCompressedVectorOld), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxCompressedVectorOld>()); assert(r >= 0);

    r = engine->RegisterObjectType("Vx2DVector", sizeof(Vx2DVector), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vx2DVector>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxMatrix", sizeof(VxMatrix), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxMatrix>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxQuaternion", sizeof(VxQuaternion), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxQuaternion>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxRect", sizeof(VxRect), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxRect>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxOBB", sizeof(VxOBB), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxOBB>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxRay", sizeof(VxRay), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxRay>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxSphere", sizeof(VxSphere), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxSphere>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxPlane", sizeof(VxPlane), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxPlane>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxFrustum", sizeof(VxFrustum), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxFrustum>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxColor", sizeof(VxColor), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VxColor>()); assert(r >= 0);

    r = engine->RegisterObjectType("VxImageDescEx", sizeof(VxImageDescEx), asOBJ_VALUE | asGetTypeTraits<VxImageDescEx>()); assert(r >= 0);
}

static void RegisterVxMathGlobalVariables(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterGlobalProperty("const float EPSILON", (void*)&s_EPSILON); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const float PI", (void*)&s_PI); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const float HALFPI", (void*)&s_HALFPI); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const float NB_STDPIXEL_FORMATS", (void*)&s_NB_STDPIXEL_FORMATS); assert(r >= 0);
    r = engine->RegisterGlobalProperty("const float MAX_PIXEL_FORMATS", (void*)&s_MAX_PIXEL_FORMATS); assert(r >= 0);
}

static void RegisterVxMathGlobalFunctions(asIScriptEngine *engine) {
    int r = 0;

    r = engine->RegisterGlobalFunction("int radToAngle(float)", asFUNCTION(radToAngle), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float Tsin(int)", asFUNCTION(Tsin), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float Tcos(int)", asFUNCTION(Tcos), asCALL_CDECL); assert(r >= 0);

    // Interpolation functions
    // r = engine->RegisterGlobalFunction("void InterpolateFloatArray(void *Res, void *array1, void *array2, float factor, int count)", asFUNCTION(InterpolateFloatArray), asCALL_CDECL); assert(r >= 0);
    // r = engine->RegisterGlobalFunction("void InterpolateVectorArray(void *Res, void *array1, void *array2, float factor, int count, XULONG StrideRes, XULONG StrideIn)", asFUNCTION(InterpolateVectorArray), asCALL_CDECL); assert(r >= 0);

    // Box and transformation functions
    r = engine->RegisterGlobalFunction("bool VxTransformBox2D(const VxMatrix &in, const VxBbox &in, VxRect &out, VxRect &out, VXCLIP_FLAGS &inout, VXCLIP_FLAGS &inout)", asFUNCTION(VxTransformBox2D), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxProjectBoxZExtents(const VxMatrix &in, const VxBbox &in, float &out, float &out)", asFUNCTION(VxProjectBoxZExtents), asCALL_CDECL); assert(r >= 0);

    // Structure copying functions
    // r = engine->RegisterGlobalFunction("XBOOL VxFillStructure(int Count, void *Dst, XULONG Stride, XULONG SizeSrc, void *Src)", asFUNCTION(VxFillStructure), asCALL_CDECL); assert(r >= 0);
    // r = engine->RegisterGlobalFunction("XBOOL VxCopyStructure(int Count, void *Dst, XULONG OutStride, XULONG SizeSrc, void *Src, XULONG InStride)", asFUNCTION(VxCopyStructure), asCALL_CDECL); assert(r >= 0);
    // r = engine->RegisterGlobalFunction("XBOOL VxIndexedCopy(const VxStridedData &Dst, const VxStridedData &Src, XULONG SizeSrc, int *Indices, int IndexCount)", asFUNCTION(VxIndexedCopy), asCALL_CDECL); assert(r >= 0);

    // Graphic utilities (Blitting)
    r = engine->RegisterGlobalFunction("void VxDoBlit(const VxImageDescEx &in, const VxImageDescEx &in)", asFUNCTION(VxDoBlit), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxDoBlitUpsideDown(const VxImageDescEx &in, const VxImageDescEx &in)", asFUNCTION(VxDoBlitUpsideDown), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxDoAlphaBlit(const VxImageDescEx &in, uint8 AlphaValue)", asFUNCTIONPR(VxDoAlphaBlit, (const VxImageDescEx &, XBYTE), void), asCALL_CDECL); assert(r >= 0);
    // r = engine->RegisterGlobalFunction("void VxDoAlphaBlit(const VxImageDescEx &in, uint8 *AlphaValues)", asFUNCTIONPR(VxDoAlphaBlit, (const VxImageDescEx &, XBYTE *), void), asCALL_CDECL); assert(r >= 0);

    // Inline functions
    r = engine->RegisterGlobalFunction("uint GetBitCount(uint)", asFUNCTION(GetBitCount), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint GetBitShift(uint)", asFUNCTION(GetBitShift), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxGetBitCounts(const VxImageDescEx &in, uint &out, uint &out, uint &out, uint &out)", asFUNCTION(VxGetBitCounts), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxGetBitShifts(const VxImageDescEx &in, uint &out, uint &out, uint &out, uint &out)", asFUNCTION(VxGetBitShifts), asCALL_CDECL); assert(r >= 0);

    // Graphic utilities (MipMaps and Resizing)
    // r = engine->RegisterGlobalFunction("void VxGenerateMipMap(const VxImageDescEx &in, XBYTE *out)", asFUNCTION(VxGenerateMipMap), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxResizeImage32(const VxImageDescEx &in, const VxImageDescEx &in)", asFUNCTION(VxResizeImage32), asCALL_CDECL); assert(r >= 0);

    // Conversion to normal/bump map
    r = engine->RegisterGlobalFunction("bool VxConvertToNormalMap(const VxImageDescEx &in, uint ColorMask)", asFUNCTION(VxConvertToNormalMap), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxConvertToBumpMap(const VxImageDescEx &in)", asFUNCTION(VxConvertToBumpMap), asCALL_CDECL); assert(r >= 0);

    // Pixel format conversion functions
    r = engine->RegisterGlobalFunction("VX_PIXELFORMAT VxImageDesc2PixelFormat(const VxImageDescEx &in)", asFUNCTION(VxImageDesc2PixelFormat), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxPixelFormat2ImageDesc(VX_PIXELFORMAT, VxImageDescEx &out)", asFUNCTION(VxPixelFormat2ImageDesc), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string VxPixelFormat2String(VX_PIXELFORMAT)", asFUNCTION(VxPixelFormat2String), asCALL_CDECL); assert(r >= 0);

    // Miscellaneous
    r = engine->RegisterGlobalFunction("int GetQuantizationSamplingFactor()", asFUNCTION(GetQuantizationSamplingFactor), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void SetQuantizationSamplingFactor(int)", asFUNCTION(SetQuantizationSamplingFactor), asCALL_CDECL); assert(r >= 0);

    // Processor features
    r = engine->RegisterGlobalFunction("string GetProcessorDescription()", asFUNCTION(GetProcessorDescription), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int GetProcessorFrequency()", asFUNCTION(GetProcessorFrequency), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint GetProcessorFeatures()", asFUNCTION(GetProcessorFeatures), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void ModifyProcessorFeatures(uint Add, uint Remove)", asFUNCTION(ModifyProcessorFeatures), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ProcessorsType GetProcessorType()", asFUNCTION(GetProcessorType), asCALL_CDECL); assert(r >= 0);

    // Utility function for point-in-rect check
    r = engine->RegisterGlobalFunction("bool VxPtInRect(const CKRECT &in, const CKPOINT &in)", asFUNCTION(VxPtInRect), asCALL_CDECL); assert(r >= 0);

    // Best-fit bounding box computation
    // r = engine->RegisterGlobalFunction("bool VxComputeBestFitBBox(const uint8 *in, uint Stride, int Count, VxMatrix &out, float AdditionalBorder)", asFUNCTION(VxComputeBestFitBBox), asCALL_CDECL); assert(r >= 0);
}

// VxMutex

static void RegisterVxMutex(asIScriptEngine *engine) {
    int r = 0;

    // Constructor and Destructor
    r = engine->RegisterObjectBehaviour("VxMutex", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxMutex *self) { new(self) VxMutex(); }, (VxMutex *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxMutex", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxMutex *self) { self->~VxMutex(); }, (VxMutex *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxMutex", "int EnterMutex()", asMETHOD(VxMutex, EnterMutex), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMutex", "int LeaveMutex()", asMETHOD(VxMutex, LeaveMutex), asCALL_THISCALL); assert(r >= 0);

    // Operator Overloads
    r = engine->RegisterObjectMethod("VxMutex", "int opPostInc()", asMETHOD(VxMutex, operator++), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMutex", "int opPostDec()", asMETHOD(VxMutex, operator--), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("VxMutexLock", asBEHAVE_CONSTRUCT, "void f(VxMutex &in)", asFUNCTIONPR([](VxMutexLock *self, VxMutex &mutex) { new(self) VxMutexLock(mutex); }, (VxMutexLock *, VxMutex &), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxMutexLock", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxMutexLock *self) { self->~VxMutexLock(); }, (VxMutexLock *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
}

// VxTimeProfiler

static void RegisterVxTimeProfiler(asIScriptEngine *engine) {
    int r = 0;

    // Constructor
    r = engine->RegisterObjectBehaviour("VxTimeProfiler", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxTimeProfiler *self) { new(self) VxTimeProfiler(); }, (VxTimeProfiler *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("VxTimeProfiler", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxTimeProfiler *self) { self->~VxTimeProfiler(); }, (VxTimeProfiler *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxTimeProfiler", "VxTimeProfiler &opAssign(const VxTimeProfiler &in)", asMETHOD(VxTimeProfiler, operator=), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxTimeProfiler", "void Reset()", asMETHOD(VxTimeProfiler, Reset), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxTimeProfiler", "float Current() const", asMETHOD(VxTimeProfiler, Current), asCALL_THISCALL); assert(r >= 0);
}

// VxSharedLibrary

static INSTANCE_HANDLE VxSharedLibraryLoad(const std::string &LibraryName, VxSharedLibrary &shl) {
    return shl.Load(const_cast<char *>(LibraryName.c_str()));
}

static FUNC_PTR VxSharedLibraryGetFunctionPtr(const std::string &FunctionName, VxSharedLibrary &shl) {
    return reinterpret_cast<FUNC_PTR>(shl.GetFunctionPtr(const_cast<char *>(FunctionName.c_str())));
}

static void RegisterVxSharedLibrary(asIScriptEngine *engine) {
    int r = 0;

    // Constructor
    r = engine->RegisterObjectBehaviour("VxSharedLibrary", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxSharedLibrary *self) { new(self) VxSharedLibrary(); }, (VxSharedLibrary *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("VxSharedLibrary", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxSharedLibrary *self) { self->~VxSharedLibrary(); }, (VxSharedLibrary *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxSharedLibrary", "void Attach(INSTANCE_HANDLE LibraryHandle)", asMETHODPR(VxSharedLibrary, Attach, (INSTANCE_HANDLE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxSharedLibrary", "INSTANCE_HANDLE Load(const string &in)", asMETHOD(VxSharedLibrary, Load), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxSharedLibrary", "INSTANCE_HANDLE Load(const string &LibraryName)", asFUNCTIONPR(VxSharedLibraryLoad, (const std::string &, VxSharedLibrary &), INSTANCE_HANDLE), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxSharedLibrary", "void ReleaseLibrary()", asMETHODPR(VxSharedLibrary, ReleaseLibrary, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxSharedLibrary", "FUNC_PTR GetFunctionPtr(const string &FunctionName)", asFUNCTIONPR(VxSharedLibraryGetFunctionPtr, (const std::string &, VxSharedLibrary &), FUNC_PTR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
}

// VxMemoryMappedFile

static void RegisterVxMemoryMappedFile(asIScriptEngine *engine) {
    int r = 0;

    // VxMMF_Error
    r = engine->RegisterEnum("VxMMF_Error"); assert(r >= 0);
    r = engine->RegisterEnumValue("VxMMF_Error", "VxMMF_NoError", VxMMF_NoError); assert(r >= 0);
    r = engine->RegisterEnumValue("VxMMF_Error", "VxMMF_FileOpen", VxMMF_FileOpen); assert(r >= 0);
    r = engine->RegisterEnumValue("VxMMF_Error", "VxMMF_FileMapping", VxMMF_FileMapping); assert(r >= 0);
    r = engine->RegisterEnumValue("VxMMF_Error", "VxMMF_MapView", VxMMF_MapView); assert(r >= 0);

    // Constructor
    r = engine->RegisterObjectBehaviour("VxMemoryMappedFile", asBEHAVE_CONSTRUCT, "void f(const string &in)", asFUNCTIONPR([](const std::string &fileName, VxMemoryMappedFile *self) { new(self) VxMemoryMappedFile((char *)fileName.c_str()); }, (const std::string &, VxMemoryMappedFile *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("VxMemoryMappedFile", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxMemoryMappedFile *self) { self->~VxMemoryMappedFile(); }, (VxMemoryMappedFile *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxMemoryMappedFile", "uint GetBase()", asMETHOD(VxMemoryMappedFile, GetBase), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMemoryMappedFile", "uint GetFileSize()", asMETHOD(VxMemoryMappedFile, GetFileSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMemoryMappedFile", "bool IsValid()", asMETHOD(VxMemoryMappedFile, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMemoryMappedFile", "VxMMF_Error GetErrorType()", asMETHOD(VxMemoryMappedFile, GetErrorType), asCALL_THISCALL); assert(r >= 0);
}

// CKPathSplitter

static void RegisterCKPathSplitter(asIScriptEngine *engine) {
    int r = 0;

    // Constructor
    r = engine->RegisterObjectBehaviour("CKPathSplitter", asBEHAVE_CONSTRUCT, "void f(const string &in)", asFUNCTIONPR([](const std::string &fileName, CKPathSplitter *self) { new(self) CKPathSplitter((char *)fileName.c_str()); }, (const std::string &, CKPathSplitter *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("CKPathSplitter", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKPathSplitter *self) { self->~CKPathSplitter(); }, (CKPathSplitter *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("CKPathSplitter", "string GetDrive() const", asMETHOD(CKPathSplitter, GetDrive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathSplitter", "string GetDir() const", asMETHOD(CKPathSplitter, GetDir), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathSplitter", "string GetName() const", asMETHOD(CKPathSplitter, GetName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathSplitter", "string GetExtension() const", asMETHOD(CKPathSplitter, GetExtension), asCALL_THISCALL); assert(r >= 0);
}

// CKDirectoryParser

static void ConstructCKDirectoryParser(const std::string &dir, const std::string &fileMask, bool recurse, CKDirectoryParser *self) {
    new(self) CKDirectoryParser(const_cast<char *>(dir.c_str()), const_cast<char *>(fileMask.c_str()), recurse);
}

static std::string CKDirectoryParserGetNextFile(CKDirectoryParser *self) {
    char *result = self->GetNextFile();
    return result ? std::string(result) : "";
}

static void CKDirectoryParserReset(const std::string &dir, const std::string &fileMask, bool recurse, CKDirectoryParser *self) {
    self->Reset(dir.empty() ? nullptr : const_cast<char *>(dir.c_str()), fileMask.empty() ? nullptr : const_cast<char *>(fileMask.c_str()), recurse);
}

static void RegisterCKDirectoryParser(asIScriptEngine *engine) {
    int r = 0;

    // Constructor
    r = engine->RegisterObjectBehaviour("CKDirectoryParser", asBEHAVE_CONSTRUCT, "void f(const string &in, const string &in, bool = false)", asFUNCTION(ConstructCKDirectoryParser), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("CKDirectoryParser", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](CKDirectoryParser *self) { self->~CKDirectoryParser(); }, (CKDirectoryParser *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("CKDirectoryParser", "string GetNextFile()", asFUNCTION(CKDirectoryParserGetNextFile), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDirectoryParser", "void Reset(const string &in = '', const string &in = '', bool = false)", asFUNCTION(CKDirectoryParserReset), asCALL_CDECL_OBJLAST); assert(r >= 0);
}

// VxWindowFunctions

static void RegisterVxWindowFunctions(asIScriptEngine *engine) {
    int r = 0;

    // VXCURSOR_POINTER
    r = engine->RegisterEnum("VXCURSOR_POINTER"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCURSOR_POINTER", "VXCURSOR_NORMALSELECT", VXCURSOR_NORMALSELECT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCURSOR_POINTER", "VXCURSOR_BUSY", VXCURSOR_BUSY); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCURSOR_POINTER", "VXCURSOR_MOVE", VXCURSOR_MOVE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXCURSOR_POINTER", "VXCURSOR_LINKSELECT", VXCURSOR_LINKSELECT); assert(r >= 0);

    // VXTEXT_ALIGNMENT
    r = engine->RegisterEnum("VXTEXT_ALIGNMENT"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXT_ALIGNMENT", "VXTEXT_CENTER", VXTEXT_CENTER); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXT_ALIGNMENT", "VXTEXT_LEFT", VXTEXT_LEFT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXT_ALIGNMENT", "VXTEXT_RIGHT", VXTEXT_RIGHT); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXT_ALIGNMENT", "VXTEXT_TOP", VXTEXT_TOP); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXT_ALIGNMENT", "VXTEXT_BOTTOM", VXTEXT_BOTTOM); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXT_ALIGNMENT", "VXTEXT_VCENTER", VXTEXT_VCENTER); assert(r >= 0);
    r = engine->RegisterEnumValue("VXTEXT_ALIGNMENT", "VXTEXT_HCENTER", VXTEXT_HCENTER); assert(r >= 0);

    // VXFONTINFO
    r = engine->RegisterObjectType("VXFONTINFO", sizeof(VXFONTINFO), asOBJ_VALUE | asGetTypeTraits<VXFONTINFO>()); assert(r >= 0);

    r = engine->RegisterObjectProperty("VXFONTINFO", "XString FaceName", asOFFSET(VXFONTINFO, FaceName)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VXFONTINFO", "int Height", asOFFSET(VXFONTINFO, Height)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VXFONTINFO", "int Weight", asOFFSET(VXFONTINFO, Weight)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VXFONTINFO", "bool Italic", asOFFSET(VXFONTINFO, Italic)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VXFONTINFO", "bool Underline", asOFFSET(VXFONTINFO, Underline)); assert(r >= 0);

    r = engine->RegisterObjectBehaviour("VXFONTINFO", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VXFONTINFO *self) { new(self) VXFONTINFO(); }, (VXFONTINFO*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VXFONTINFO", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VXFONTINFO *self) { self->~VXFONTINFO(); }, (VXFONTINFO*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Keyboard and cursor functions
    // r = engine->RegisterGlobalFunction("uint8 VxScanCodeToAscii(uint scancode, uint8[256] &in)", asFUNCTION(VxScanCodeToAscii), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int VxScanCodeToName(uint scancode, string &in)", asFUNCTION(VxScanCodeToName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int VxShowCursor(bool show)", asFUNCTION(VxShowCursor), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxSetCursor(VXCURSOR_POINTER cursorID)", asFUNCTION(VxSetCursor), asCALL_CDECL); assert(r >= 0);

    // FPU control
    r = engine->RegisterGlobalFunction("uint16 VxGetFPUControlWord()", asFUNCTION(VxGetFPUControlWord), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxSetFPUControlWord(uint16 Fpu)", asFUNCTION(VxSetFPUControlWord), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxSetBaseFPUControlWord()", asFUNCTION(VxSetBaseFPUControlWord), asCALL_CDECL); assert(r >= 0);

    // Library and environment functions
    r = engine->RegisterGlobalFunction("void VxAddLibrarySearchPath(string &in path)", asFUNCTION(VxAddLibrarySearchPath), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxGetEnvironmentVariable(string &in envName, string &out envValue)", asFUNCTION(VxGetEnvironmentVariable), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxSetEnvironmentVariable(string &in envName, string &in envValue)", asFUNCTION(VxSetEnvironmentVariable), asCALL_CDECL); assert(r >= 0);

    // Window functions
    r = engine->RegisterGlobalFunction("WIN_HANDLE VxWindowFromPoint(CKPOINT &in pt)", asFUNCTION(VxWindowFromPoint), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxGetClientRect(WIN_HANDLE Win, CKRECT &out rect)", asFUNCTION(VxGetClientRect), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxGetWindowRect(WIN_HANDLE Win, CKRECT &out rect)", asFUNCTION(VxGetWindowRect), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxScreenToClient(WIN_HANDLE Win, CKPOINT &inout pt)", asFUNCTION(VxScreenToClient), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxClientToScreen(WIN_HANDLE Win, CKPOINT &inout pt)", asFUNCTION(VxClientToScreen), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("WIN_HANDLE VxSetParent(WIN_HANDLE Child, WIN_HANDLE Parent)", asFUNCTION(VxSetParent), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("WIN_HANDLE VxGetParent(WIN_HANDLE Win)", asFUNCTION(VxGetParent), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxMoveWindow(WIN_HANDLE Win, int x, int y, int Width, int Height, bool Repaint)", asFUNCTION(VxMoveWindow), asCALL_CDECL); assert(r >= 0);

    // File and directory functions
    r = engine->RegisterGlobalFunction("bool VxMakeDirectory(string &in path)", asFUNCTION(VxMakeDirectory), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxRemoveDirectory(string &in path)", asFUNCTION(VxRemoveDirectory), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxDeleteDirectory(string &in path)", asFUNCTION(VxDeleteDirectory), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxGetCurrentDirectory(string &out path)", asFUNCTION(VxGetCurrentDirectory), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxSetCurrentDirectory(string &in path)", asFUNCTION(VxSetCurrentDirectory), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxTestDiskSpace(string &in dir, uint size)", asFUNCTION(VxTestDiskSpace), asCALL_CDECL); assert(r >= 0);

    // URL functions
    r = engine->RegisterGlobalFunction("uint VxURLDownloadToCacheFile(string &in File, string &out CachedFile, int szCachedFile)", asFUNCTION(VxURLDownloadToCacheFile), asCALL_CDECL); assert(r >= 0);

    // Bitmap functions
    r = engine->RegisterGlobalFunction("BITMAP_HANDLE VxCreateBitmap(const VxImageDescEx &in)", asFUNCTION(VxCreateBitmap), asCALL_CDECL); assert(r >= 0);
    // r = engine->RegisterGlobalFunction("uint8@ VxConvertBitmap(BITMAP_HANDLE, VxImageDescEx &out)", asFUNCTION(VxConvertBitmap), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxCopyBitmap(BITMAP_HANDLE, const VxImageDescEx &in)", asFUNCTION(VxCopyBitmap), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxDeleteBitmap(BITMAP_HANDLE)", asFUNCTION(VxDeleteBitmap), asCALL_CDECL); assert(r >= 0);

    // Font functions
    r = engine->RegisterGlobalFunction("FONT_HANDLE VxCreateFont(string &in FontName, int FontSize, int Weight, bool italic, bool underline)", asFUNCTION(VxCreateFont), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxGetFontInfo(FONT_HANDLE Font, VXFONTINFO &out desc)", asFUNCTION(VxGetFontInfo), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxDrawBitmapText(BITMAP_HANDLE Bitmap, FONT_HANDLE Font, string &in string, CKRECT &in rect, uint Align, uint BkColor, uint FontColor)", asFUNCTION(VxDrawBitmapText), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void VxDeleteFont(FONT_HANDLE Font)", asFUNCTION(VxDeleteFont), asCALL_CDECL); assert(r >= 0);
}

// VxVector

static void RegisterVxVector(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxVector", "float x", asOFFSET(VxVector, x)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxVector", "float y", asOFFSET(VxVector, y)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxVector", "float z", asOFFSET(VxVector, z)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxVector", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxVector *self) { new(self) VxVector(); }, (VxVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxVector", asBEHAVE_CONSTRUCT, "void f(const VxVector &in)", asFUNCTIONPR([](const VxVector &v, VxVector *self) { new(self) VxVector(v); }, (const VxVector &, VxVector *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxVector", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTIONPR([](float f, VxVector *self) { new(self) VxVector(f); }, (float, VxVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxVector", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTIONPR([](float x, float y, float z, VxVector *self) { new(self) VxVector(x, y, z); }, (float, float, float, VxVector *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxVector", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float}", asFUNCTIONPR([](float *list, VxVector *self) { new(self) VxVector(list[0], list[1], list[2]); }, (float *, VxVector *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("VxVector", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxVector* self) { self->~VxVector(); }, (VxVector *self), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxVector", "VxVector &opAssign(const VxVector &in)", asMETHODPR(VxVector, operator=, (const VxVector &), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector &opAssign(const VxCompressedVector &in)", asMETHODPR(VxVector, operator=, (const VxCompressedVector &), VxVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "bool opEquals(const VxVector &in) const", asFUNCTIONPR([](const VxVector &lhs, const VxVector &rhs) -> bool { return lhs == rhs; }, (const VxVector &, const VxVector &), bool), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    // r = engine->RegisterObjectMethod("VxVector", "int opCmp(const VxVector &in) const", asFUNCTIONPR([](const VxVector &lhs, const VxVector &rhs) -> int { if (lhs < rhs) return -1; else if (lhs == rhs) return 0; else return 1; }, (const VxVector &, const VxVector &), int), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

    r = engine->RegisterObjectMethod("VxVector", "VxVector &opAddAssign(const VxVector &in)", asMETHODPR(VxVector, operator+=, (const VxVector &), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector &opSubAssign(const VxVector &in)", asMETHODPR(VxVector, operator-=, (const VxVector &), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector &opMulAssign(const VxVector &in)", asMETHODPR(VxVector, operator*=, (const VxVector &), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector &opDivAssign(const VxVector &in)", asMETHODPR(VxVector, operator/=, (const VxVector &), VxVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "VxVector &opMulAssign(float)", asMETHODPR(VxVector, operator*=, (float), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector &opDivAssign(float)", asMETHODPR(VxVector, operator/=, (float), VxVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "VxVector opAdd(const VxVector &in) const", asFUNCTIONPR([](const VxVector &lhs, const VxVector &rhs) { return lhs + rhs; }, (const VxVector &, const VxVector &), VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector opSub(const VxVector &in) const", asFUNCTIONPR([](const VxVector &lhs, const VxVector &rhs) { return lhs - rhs; }, (const VxVector &, const VxVector &), VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector opMul(const VxVector &in) const", asFUNCTIONPR([](const VxVector &lhs, const VxVector &rhs) { return lhs * rhs; }, (const VxVector &, const VxVector &), VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector opDiv(const VxVector &in) const", asFUNCTIONPR([](const VxVector &lhs, const VxVector &rhs) { return lhs / rhs; }, (const VxVector &, const VxVector &), VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "VxVector opMul(float) const", asFUNCTIONPR([](const VxVector &lhs, float scalar) { return lhs * scalar; }, (const VxVector &, float), VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector opDiv(float) const", asFUNCTIONPR([](const VxVector &lhs, float scalar) { return lhs / scalar; }, (const VxVector &, float), VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "VxVector opMul(const VxMatrix &in) const", asFUNCTIONPR(operator*, (const VxVector&, const VxMatrix&), VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "VxVector opNeg() const", asFUNCTIONPR([](const VxVector &v) -> const VxVector { return -v; }, (const VxVector &), const VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "float &opIndex(int)", asFUNCTIONPR([](VxVector &v, int i) -> float & { return v[i]; }, (VxVector &, int), float &), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
    r = engine->RegisterObjectMethod("VxVector", "const float &opIndex(int) const", asFUNCTIONPR([](const VxVector &v, int i) -> const float & { return v[i]; }, (const VxVector &, int), const float &), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

    r = engine->RegisterObjectMethod("VxVector", "void Set(float, float, float)", asMETHOD(VxVector, Set), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "void Absolute()", asMETHOD(VxVector, Absolute), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "float SquareMagnitude() const", asMETHOD(VxVector, SquareMagnitude), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "float Magnitude() const", asMETHOD(VxVector, Magnitude), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "float InvSquareMagnitude() const", asFUNCTIONPR(InvSquareMagnitude, (const VxVector &), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "float InvMagnitude() const", asFUNCTIONPR(InvMagnitude, (const VxVector &), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "float Min(const VxVector &in) const", asFUNCTIONPR(Min, (const VxVector &), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "float Max(const VxVector &in) const", asFUNCTIONPR(Max, (const VxVector &), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "VxVector Minimize(const VxVector &in) const", asFUNCTIONPR(Minimize, (const VxVector &, const VxVector &), const VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector Maximize(const VxVector &in) const", asFUNCTIONPR(Maximize, (const VxVector &, const VxVector &), const VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector Interpolate(const VxVector &in, float step) const", asFUNCTIONPR([](const VxVector &lhs, const VxVector &rhs, float step) -> const VxVector { return Interpolate(step, lhs, rhs); }, (const VxVector &, const VxVector &, float step), const VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector", "VxVector Normalize() const", asFUNCTIONPR(Normalize, (const VxVector &), const VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    // r = engine->RegisterObjectMethod("VxVector", "float Dot(const VxVector &in) const", asMETHODPR(VxVector, Dot, (const VxVector &) const, float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "float Dot(const VxVector &in) const", asFUNCTIONPR(DotProduct, (const VxVector &, const VxVector &), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector Cross(const VxVector &in) const", asFUNCTIONPR(CrossProduct, (const VxVector &, const VxVector &), const VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector Reflect(const VxVector &in) const", asFUNCTIONPR(Reflect, (const VxVector &, const VxVector &), const VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector Rotate(const VxMatrix &in) const", asFUNCTIONPR(Rotate, (const VxMatrix &, const VxVector &), const VxVector), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "VxVector Rotate(const VxVector &in, float) const", asFUNCTIONPR(Rotate, (const VxVector &, const VxVector &, float), const VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    // Static properties
    r = engine->RegisterObjectMethod("VxVector", "const VxVector &get_axisX() const", asFUNCTIONPR([](const VxVector& lhs) -> const VxVector& { return VxVector::axisX(); }, (const VxVector &), const VxVector &), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "const VxVector &get_axisY() const", asFUNCTIONPR([](const VxVector& lhs) -> const VxVector& { return VxVector::axisY(); }, (const VxVector&), const VxVector&), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "const VxVector &get_axisZ() const", asFUNCTIONPR([](const VxVector& lhs) -> const VxVector& { return VxVector::axisZ(); }, (const VxVector&), const VxVector&), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "const VxVector &get_axis0() const", asFUNCTIONPR([](const VxVector& lhs) -> const VxVector& { return VxVector::axis0(); }, (const VxVector&), const VxVector&), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector", "const VxVector &get_axis1() const", asFUNCTIONPR([](const VxVector& lhs) -> const VxVector& { return VxVector::axis1(); }, (const VxVector&), const VxVector&), asCALL_CDECL_OBJFIRST); assert(r >= 0);
}

// VxVector4

static void RegisterVxVector4(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxVector4", "float x", asOFFSET(VxVector4, x)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxVector4", "float y", asOFFSET(VxVector4, y)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxVector4", "float z", asOFFSET(VxVector4, z)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxVector4", "float w", asOFFSET(VxVector4, w)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxVector4", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxVector4 *self) { new(self) VxVector4(); }, (VxVector4*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxVector4", asBEHAVE_CONSTRUCT, "void f(const VxVector4 &)", asFUNCTIONPR([](const VxVector4 &v, VxVector4 *self) { new(self) VxVector4(v); }, (const VxVector4 &, VxVector4*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxVector4", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTIONPR([](float f, VxVector4 *self) { new(self) VxVector4(f); }, (float, VxVector4*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxVector4", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTIONPR([](float x, float y, float z, float w, VxVector4 *self) { new(self) VxVector4(x, y, z, w); }, (float, float, float, float, VxVector4*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxVector4", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float}", asFUNCTIONPR([](float *list, VxVector4 *self) { new(self) VxVector4(list[0], list[1], list[2], list[3]); }, (float *, VxVector4 *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("VxVector4", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxVector4 *self) { self->~VxVector4(); }, (VxVector4*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opAssign(const VxVector4 &in)", asMETHODPR(VxVector4, operator=, (const VxVector4 &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opAssign(const VxVector &in)", asMETHODPR(VxVector4, operator=, (const VxVector &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector4", "bool opEquals(const VxVector4 &in) const", asFUNCTIONPR([](const VxVector4 &lhs, const VxVector4 &rhs) -> bool { return lhs == rhs; }, (const VxVector4 &, const VxVector4 &), bool), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "int opCmp(const VxVector4 &in) const", asFUNCTIONPR([](const VxVector4 &lhs, const VxVector4 &rhs) -> int { if (lhs < rhs) return -1; else if (lhs == rhs) return 0; else return 1; }, (const VxVector4 &, const VxVector4 &), int), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opAddAssign(const VxVector4 &in)", asMETHODPR(VxVector4, operator+=, (const VxVector4 &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opSubAssign(const VxVector4 &in)", asMETHODPR(VxVector4, operator-=, (const VxVector4 &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opMulAssign(const VxVector4 &in)", asMETHODPR(VxVector4, operator*=, (const VxVector4 &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opDivAssign(const VxVector4 &in)", asMETHODPR(VxVector4, operator/=, (const VxVector4 &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opAddAssign(const VxVector &in)", asMETHODPR(VxVector4, operator+=, (const VxVector &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opSubAssign(const VxVector &in)", asMETHODPR(VxVector4, operator-=, (const VxVector &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opMulAssign(const VxVector &in)", asMETHODPR(VxVector4, operator*=, (const VxVector &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opDivAssign(const VxVector &in)", asMETHODPR(VxVector4, operator/=, (const VxVector &), VxVector4 &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opMulAssign(float)", asMETHODPR(VxVector4, operator*=, (float), VxVector4 &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 &opDivAssign(float)", asMETHODPR(VxVector4, operator/=, (float), VxVector4 &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opAdd(const VxVector &in) const", asFUNCTIONPR([](const VxVector4 &lhs, const VxVector4 &rhs) { return lhs + rhs; }, (const VxVector4 &, const VxVector4 &), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opSub(const VxVector &in) const", asFUNCTIONPR([](const VxVector4 &lhs, const VxVector4 &rhs) { return lhs - rhs; }, (const VxVector4 &, const VxVector4 &), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opMul(const VxVector &in) const", asFUNCTIONPR([](const VxVector4 &lhs, const VxVector4 &rhs) { return lhs * rhs; }, (const VxVector4 &, const VxVector4 &), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opDiv(const VxVector &in) const", asFUNCTIONPR([](const VxVector4 &lhs, const VxVector4 &rhs) { return lhs / rhs; }, (const VxVector4 &, const VxVector4 &), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opAdd(float) const", asFUNCTIONPR([](const VxVector4 &lhs, float scalar) { return lhs + scalar; }, (const VxVector4 &, float), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opSub(float) const", asFUNCTIONPR([](const VxVector4 &lhs, float scalar) { return lhs - scalar; }, (const VxVector4 &, float), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opMul(float) const", asFUNCTIONPR([](const VxVector4 &lhs, float scalar) { return lhs * scalar; }, (const VxVector4 &, float), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opDiv(float) const", asFUNCTIONPR([](const VxVector4 &lhs, float scalar) { return lhs / scalar; }, (const VxVector4 &, float), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opMul(const VxMatrix &in) const", asFUNCTIONPR(operator*, (const VxVector4 &, const VxMatrix &), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector4", "VxVector4 opNeg() const", asFUNCTIONPR([](const VxVector4 &v) -> const VxVector4 { return -v; }, (const VxVector4 &), const VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector4", "float &opIndex(int)", asFUNCTIONPR([](VxVector4 &v, int i) -> float & { return v[i]; }, (VxVector4 &, int), float &), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
    r = engine->RegisterObjectMethod("VxVector4", "const float &opIndex(int) const", asFUNCTIONPR([](const VxVector4 &v, int i) -> const float & { return v[i]; }, (const VxVector4 &, int), const float &), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

    r = engine->RegisterObjectMethod("VxVector4", "void Set(float, float, float, float)", asMETHODPR(VxVector4, Set, (float, float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "void Set(float, float, float)", asMETHODPR(VxVector4, Set, (float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "float Dot(const VxVector4 &in) const", asMETHODPR(VxVector4, Dot, (const VxVector4 &) const, float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxVector4", "void Absolute()", asMETHOD(VxVector4, Absolute), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "float SquareMagnitude() const", asMETHOD(VxVector4, SquareMagnitude), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxVector4", "float Magnitude() const", asMETHOD(VxVector4, Magnitude), asCALL_THISCALL); assert(r >= 0);
}

// VxBbox

static void VxBboxClassifyVertices(const VxBbox &box, const CScriptArray &iVertices, const CScriptArray &oFlags) {
    int count = (int)iVertices.GetSize();
    if(count != 0) {
        XBYTE *vertices = (XBYTE*)iVertices.At(0);
        XULONG stride = sizeof(XBYTE);
        XULONG *flags = (XULONG*)oFlags.At(0);
        box.ClassifyVertices(count, vertices, stride, flags);
    }
}

static void VxBboxClassifyVerticesOneAxis(const VxBbox &box, const CScriptArray &iVertices, int iAxis, const CScriptArray &oFlags) {
    int count = (int)iVertices.GetSize();
    if(count != 0) {
        XBYTE *vertices = (XBYTE*)iVertices.At(0);
        XULONG stride = sizeof(XBYTE);
        XULONG *flags = (XULONG*)oFlags.At(0);
        box.ClassifyVerticesOneAxis(count, vertices, stride, iAxis, flags);
    }
}

static void RegisterVxBbox(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxBbox", "VxVector Min", asOFFSET(VxBbox, Min)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxBbox", "VxVector Max", asOFFSET(VxBbox, Max)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxBbox", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxBbox *self) { new(self) VxBbox(); }, (VxBbox*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxBbox", asBEHAVE_CONSTRUCT, "void f(const VxBbox &in)", asFUNCTIONPR([](const VxBbox &box, VxBbox *self) { new(self) VxBbox(box); }, (const VxBbox &, VxBbox*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxBbox", asBEHAVE_CONSTRUCT, "void f(const VxVector &in, const VxVector &in)", asFUNCTIONPR([](const VxVector &min, const VxVector &max, VxBbox *self) { new(self) VxBbox(min, max); }, (const VxVector &, const VxVector &, VxBbox*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxBbox", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTIONPR([](float value, VxBbox *self) { new(self) VxBbox(value); }, (float, VxBbox*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxBbox", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {VxVector, VxVector}", asFUNCTIONPR([](VxVector *list, VxBbox *self) { new(self) VxBbox(list[0], list[1]); }, (VxVector *, VxBbox *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("VxBbox", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxBbox *self) { self->~VxBbox(); }, (VxBbox*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxBbox", "VxBbox &opAssign(const VxBbox &in)", asMETHODPR(VxBbox, operator=, (const VxBbox &), VxBbox &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxBbox", "bool opEquals(const VxVector &in) const", asFUNCTIONPR([](const VxBbox &lhs, const VxBbox &rhs) -> bool { return lhs == rhs; }, (const VxBbox &, const VxBbox &), bool), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxBbox", "bool IsValid() const", asMETHOD(VxBbox, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "VxVector GetSize() const", asMETHOD(VxBbox, GetSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "VxVector GetHalfSize() const", asMETHOD(VxBbox, GetHalfSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "VxVector GetCenter() const", asMETHOD(VxBbox, GetCenter), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void SetCorners(const VxVector &in, const VxVector &in)", asMETHOD(VxBbox, SetCorners), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void SetDimension(const VxVector &in, const VxVector &in)", asMETHOD(VxBbox, SetDimension), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void SetCenter(const VxVector &in, const VxVector &in)", asMETHOD(VxBbox, SetCenter), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void Reset()", asMETHOD(VxBbox, Reset), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void Merge(const VxBbox &in)", asMETHODPR(VxBbox, Merge, (const VxBbox &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void Merge(const VxVector &in)", asMETHODPR(VxBbox, Merge, (const VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "uint Classify(const VxVector &in) const", asMETHODPR(VxBbox, Classify, (const VxVector &) const, XULONG), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "uint Classify(const VxBbox &in) const", asMETHODPR(VxBbox, Classify, (const VxBbox &) const, XULONG), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "int Classify(const VxBbox &in, const VxVector &in) const", asMETHODPR(VxBbox, Classify, (const VxBbox &, const VxVector &) const, int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void ClassifyVertices(const array<uint8> &in, const array<uint32> &in) const", asFUNCTIONPR(VxBboxClassifyVertices, (const VxBbox &, const CScriptArray &, const CScriptArray &), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void ClassifyVerticesOneAxis(const array<uint8> &in, int, const array<uint32> &in) const", asFUNCTIONPR(VxBboxClassifyVerticesOneAxis, (const VxBbox &, const CScriptArray &, int, const CScriptArray &), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void Intersect(const VxBbox &in)", asMETHODPR(VxBbox, Intersect, (const VxBbox &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "bool VectorIn(const VxVector &in) const", asMETHODPR(VxBbox, VectorIn, (const VxVector &) const, XBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "bool IsBoxInside(const VxBbox &in) const", asMETHODPR(VxBbox, IsBoxInside, (const VxBbox &) const, XBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxBbox", "void TransformTo(VxVector &out, const VxMatrix &in) const", asMETHODPR(VxBbox, TransformTo, (VxVector *, const VxMatrix &) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxBbox", "void TransformFrom(const VxBbox &in, const VxMatrix &in)", asMETHODPR(VxBbox, TransformFrom, (const VxBbox &, const VxMatrix &), void), asCALL_THISCALL); assert(r >= 0);
}

// VxCompressedVector

static void RegisterVxCompressedVector(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxCompressedVector", "int16 xa", asOFFSET(VxCompressedVector, xa)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxCompressedVector", "int16 ya", asOFFSET(VxCompressedVector, ya)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxCompressedVector", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxCompressedVector *self) { new(self) VxCompressedVector(); }, (VxCompressedVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxCompressedVector", asBEHAVE_CONSTRUCT, "void f(const VxCompressedVector &in)", asFUNCTIONPR([](const VxCompressedVector &v, VxCompressedVector *self) { new(self) VxCompressedVector(v); }, (const VxCompressedVector &, VxCompressedVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxCompressedVector", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTIONPR([](float x, float y, float z, VxCompressedVector *self) { new(self) VxCompressedVector(x, y, z); }, (float, float, float, VxCompressedVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxCompressedVector", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float}", asFUNCTIONPR([](float *list, VxCompressedVector *self) { new(self) VxCompressedVector(list[0], list[1], list[2]); }, (float *, VxCompressedVector *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("VxCompressedVector", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxCompressedVector *self) { self->~VxCompressedVector(); }, (VxCompressedVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxCompressedVector", "VxCompressedVector &opAssign(const VxCompressedVector &in)", asMETHODPR(VxCompressedVector, operator=, (const VxCompressedVector &), VxCompressedVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxCompressedVector", "VxCompressedVector &opAssign(const VxVector &in)", asMETHODPR(VxCompressedVector, operator=, (const VxVector &), VxCompressedVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxCompressedVector", "VxCompressedVector &opAssign(const VxCompressedVectorOld &in)", asMETHODPR(VxCompressedVector, operator=, (const VxCompressedVectorOld &), VxCompressedVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxCompressedVector", "void Set(float, float, float)", asMETHOD(VxCompressedVector, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxCompressedVector", "void Slerp(float, VxCompressedVector &in, VxCompressedVector &in)", asMETHOD(VxCompressedVector, Slerp), asCALL_THISCALL); assert(r >= 0);
}

// VxCompressedVectorOld

static void RegisterVxCompressedVectorOld(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxCompressedVectorOld", "int xa", asOFFSET(VxCompressedVectorOld, xa)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxCompressedVectorOld", "int ya", asOFFSET(VxCompressedVectorOld, ya)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxCompressedVectorOld", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxCompressedVectorOld *self) { new(self) VxCompressedVectorOld(); }, (VxCompressedVectorOld*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxCompressedVectorOld", asBEHAVE_CONSTRUCT, "void f(const VxCompressedVectorOld &in)", asFUNCTIONPR([](const VxCompressedVectorOld &v, VxCompressedVectorOld *self) { new(self) VxCompressedVectorOld(v); }, (const VxCompressedVectorOld &, VxCompressedVectorOld*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxCompressedVectorOld", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTIONPR([](float x, float y, float z, VxCompressedVectorOld *self) { new(self) VxCompressedVectorOld(x, y, z); }, (float, float, float, VxCompressedVectorOld*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxCompressedVectorOld", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float}", asFUNCTIONPR([](float *list, VxCompressedVectorOld *self) { new(self) VxCompressedVectorOld(list[0], list[1], list[2]); }, (float *, VxCompressedVectorOld *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("VxCompressedVectorOld", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxCompressedVectorOld *self) { self->~VxCompressedVectorOld(); }, (VxCompressedVectorOld*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxCompressedVectorOld", "VxCompressedVectorOld &opAssign(const VxCompressedVectorOld &in)", asMETHODPR(VxCompressedVectorOld, operator=, (const VxCompressedVectorOld &), VxCompressedVectorOld &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxCompressedVectorOld", "VxCompressedVectorOld &opAssign(const VxVector &in)", asMETHODPR(VxCompressedVectorOld, operator=, (const VxVector &), VxCompressedVectorOld &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxCompressedVectorOld", "VxCompressedVectorOld &opAssign(const VxCompressedVector &in)", asMETHODPR(VxCompressedVectorOld, operator=, (const VxCompressedVector &), VxCompressedVectorOld &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxCompressedVectorOld", "void Set(float, float, float)", asMETHOD(VxCompressedVectorOld, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxCompressedVectorOld", "void Slerp(float, VxCompressedVectorOld &in, VxCompressedVectorOld &in)", asMETHOD(VxCompressedVectorOld, Slerp), asCALL_THISCALL); assert(r >= 0);
}

// Vx2DVector

static void RegisterVx2DVector(asIScriptEngine *engine) {
    int r = 0;

    // Properties (x, y)
    r = engine->RegisterObjectProperty("Vx2DVector", "float x", asOFFSET(Vx2DVector, x)); assert(r >= 0);
    r = engine->RegisterObjectProperty("Vx2DVector", "float y", asOFFSET(Vx2DVector, y)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("Vx2DVector", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](Vx2DVector *self) { new(self) Vx2DVector(); }, (Vx2DVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("Vx2DVector", asBEHAVE_CONSTRUCT, "void f(const Vx2DVector &in)", asFUNCTIONPR([](const Vx2DVector &v, Vx2DVector *self) { new(self) Vx2DVector(v); }, (const Vx2DVector &, Vx2DVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("Vx2DVector", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTIONPR([](float f, Vx2DVector *self) { new(self) Vx2DVector(f); }, (float, Vx2DVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("Vx2DVector", asBEHAVE_CONSTRUCT, "void f(float, float)", asFUNCTIONPR([](float x, float y, Vx2DVector *self) { new(self) Vx2DVector(x, y); }, (float, float, Vx2DVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("Vx2DVector", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float}", asFUNCTIONPR([](float *list, Vx2DVector *self) { new(self) Vx2DVector(list[0], list[1]); }, (float *, Vx2DVector *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("Vx2DVector", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](Vx2DVector *self) { self->~Vx2DVector(); }, (Vx2DVector*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector &opAssign(const Vx2DVector &in)", asMETHODPR(Vx2DVector, operator=, (const Vx2DVector &), Vx2DVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector &opAddAssign(const Vx2DVector &in)", asMETHODPR(Vx2DVector, operator+=, (const Vx2DVector &), Vx2DVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector &opSubAssign(const Vx2DVector &in)", asMETHODPR(Vx2DVector, operator-=, (const Vx2DVector &), Vx2DVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector &opMulAssign(const Vx2DVector &in)", asMETHODPR(Vx2DVector, operator*=, (const Vx2DVector &), Vx2DVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector &opDivAssign(const Vx2DVector &in)", asMETHODPR(Vx2DVector, operator/=, (const Vx2DVector &), Vx2DVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector &opMulAssign(float)", asMETHODPR(Vx2DVector, operator*=, (float), Vx2DVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector &opDivAssign(float)", asMETHODPR(Vx2DVector, operator/=, (float), Vx2DVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("Vx2DVector", "bool opEquals(const Vx2DVector &in) const", asFUNCTIONPR([](const Vx2DVector &lhs, const Vx2DVector &rhs) { return lhs == rhs; }, (const Vx2DVector &, const Vx2DVector &), bool), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "int opCmp(const Vx2DVector &in) const", asFUNCTIONPR([](const Vx2DVector &lhs, const Vx2DVector &rhs) -> int { if (lhs < rhs) return -1; else if (lhs == rhs) return 0; else return 1; }, (const Vx2DVector &, const Vx2DVector &), int), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector opAdd(const Vx2DVector &in) const", asFUNCTIONPR([](const Vx2DVector &lhs, const Vx2DVector &rhs) { return lhs + rhs; }, (const Vx2DVector &, const Vx2DVector &), Vx2DVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector opSub(const Vx2DVector &in) const", asFUNCTIONPR([](const Vx2DVector &lhs, const Vx2DVector &rhs) { return lhs - rhs; }, (const Vx2DVector &, const Vx2DVector &), Vx2DVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector opMul(const Vx2DVector &in) const", asFUNCTIONPR([](const Vx2DVector &lhs, const Vx2DVector &rhs) { return lhs * rhs; }, (const Vx2DVector &, const Vx2DVector &), Vx2DVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector opDiv(const Vx2DVector &in) const", asFUNCTIONPR([](const Vx2DVector &lhs, const Vx2DVector &rhs) { return lhs / rhs; }, (const Vx2DVector &, const Vx2DVector &), Vx2DVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector opMul(float) const", asFUNCTIONPR([](const Vx2DVector &lhs, float scalar) { return lhs * scalar; }, (const Vx2DVector &, float), Vx2DVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector opDiv(float) const", asFUNCTIONPR([](const Vx2DVector &lhs, float scalar) { return lhs / scalar; }, (const Vx2DVector &, float), Vx2DVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector opNeg() const", asMETHODPR(Vx2DVector, operator-, () const, Vx2DVector), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("Vx2DVector", "const float &opIndex(int) const", asMETHODPR(Vx2DVector, operator[], (int) const, const float&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "float &opIndex(int)", asMETHODPR(Vx2DVector, operator[], (int), float&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("Vx2DVector", "float Magnitude() const", asMETHOD(Vx2DVector, Magnitude), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "float SquareMagnitude() const", asMETHOD(Vx2DVector, SquareMagnitude), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector &Normalize()", asMETHOD(Vx2DVector, Normalize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "float Dot(const Vx2DVector &in) const", asMETHOD(Vx2DVector, Dot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "Vx2DVector Cross() const", asMETHOD(Vx2DVector, Cross), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "float Min() const", asMETHOD(Vx2DVector, Min), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vx2DVector", "float Max() const", asMETHOD(Vx2DVector, Max), asCALL_THISCALL); assert(r >= 0);
}

// VxMatrix

static void RegisterVxMatrix(asIScriptEngine *engine) {
    int r = 0;

    // Constructors
    r = engine->RegisterObjectBehaviour("VxMatrix", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxMatrix* self) { new(self) VxMatrix(); }, (VxMatrix*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxMatrix", asBEHAVE_CONSTRUCT, "void f(const VxMatrix &in)", asFUNCTIONPR([](const VxMatrix &mat, VxMatrix *self) { new(self) VxMatrix(mat); }, (const VxMatrix &, VxMatrix*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("VxMatrix", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxMatrix *self) { self->~VxMatrix(); }, (VxMatrix*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxMatrix", "VxMatrix &opAssign(const VxMatrix &in)", asMETHODPR(VxMatrix, operator=, (const VxMatrix &), VxMatrix &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxMatrix", "bool opEquals(const VxMatrix &in) const", asMETHODPR(VxMatrix, operator==, (const VxMatrix&) const, XBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxMatrix", "VxMatrix &opMulAssign(const VxMatrix &in) const", asMETHODPR(VxMatrix, operator*=, (const VxMatrix&), VxMatrix&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMatrix", "VxMatrix opMul(const VxMatrix &in) const", asMETHODPR(VxMatrix, operator*, (const VxMatrix&) const, VxMatrix), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMatrix", "VxVector opMul(const VxVector &in) const", asFUNCTIONPR(operator*, (const VxMatrix&, const VxVector&), VxVector), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMatrix", "VxVector4 opMul(const VxVector4 &in) const", asFUNCTIONPR(operator*, (const VxMatrix&, const VxVector4&), VxVector4), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxMatrix", "VxVector4& opIndex(int)", asMETHODPR(VxMatrix, operator[], (int), VxVector4&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMatrix", "const VxVector4& opIndex(int) const", asMETHODPR(VxMatrix, operator[], (int) const, const VxVector4&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxMatrix", "void SetIdentity()", asMETHODPR(VxMatrix, SetIdentity, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMatrix", "void Clear()", asMETHODPR(VxMatrix, Clear, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMatrix", "void Perspective(float, float, float, float)", asMETHODPR(VxMatrix, Perspective, (float, float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMatrix", "void Orthographic(float, float, float, float)", asMETHODPR(VxMatrix, Orthographic, (float, float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMatrix", "void PerspectiveRect(float, float, float, float, float, float)", asMETHODPR(VxMatrix, PerspectiveRect, (float, float, float, float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxMatrix", "void OrthographicRect(float, float, float, float, float, float)", asMETHODPR(VxMatrix, OrthographicRect, (float, float, float, float, float, float), void), asCALL_THISCALL); assert(r >= 0);

    // Functions
    r = engine->RegisterGlobalFunction("void Vx3DMatrixIdentity(VxMatrix &out)", asFUNCTION(Vx3DMatrixIdentity), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DMultiplyMatrixVector(VxVector &out, const VxMatrix &in, const VxVector &in)", asFUNCTION(Vx3DMultiplyMatrixVector), asCALL_CDECL); assert(r >= 0);
    // r = engine->RegisterGlobalFunction("void Vx3DMultiplyMatrixVectorMany(VxVector &out, const VxMatrix &in, const VxVector &in)", asFUNCTION(Vx3DMultiplyMatrixVector), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DMultiplyMatrixVector4(VxVector4 &out, const VxMatrix &in, const VxVector4 &in)", asFUNCTIONPR(Vx3DMultiplyMatrixVector4, (VxVector4 *, const VxMatrix &, const VxVector4 *), void), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DMultiplyMatrixVector4(VxVector4 &out, const VxMatrix &in, const VxVector &in)", asFUNCTIONPR(Vx3DMultiplyMatrixVector4, (VxVector4 *, const VxMatrix &, const VxVector *), void), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DRotateVector(VxVector &out, const VxMatrix &in, const VxVector &in)", asFUNCTION(Vx3DRotateVector), asCALL_CDECL); assert(r >= 0);
    // r = engine->RegisterGlobalFunction("void Vx3DRotateVectorMany(VxVector &out, const VxMatrix &in, const VxVector &in, int count, int stride)", asFUNCTION(Vx3DRotateVectorMany), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DMultiplyMatrix(VxMatrix &out, const VxMatrix &in, const VxMatrix &in)", asFUNCTION(Vx3DMultiplyMatrix), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DMultiplyMatrix4(VxMatrix &out, const VxMatrix &in, const VxMatrix &in)", asFUNCTION(Vx3DMultiplyMatrix4), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DInverseMatrix(VxMatrix &out, const VxMatrix &in)", asFUNCTION(Vx3DInverseMatrix), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float Vx3DMatrixDeterminant(const VxMatrix &in)", asFUNCTION(Vx3DMatrixDeterminant), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DMatrixFromRotation(VxMatrix &out, const VxVector &in, float)", asFUNCTION(Vx3DMatrixFromRotation), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DMatrixFromRotationAndOrigin(VxMatrix &out, const VxVector &in, const VxVector &, float )", asFUNCTION(Vx3DMatrixFromRotationAndOrigin), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DMatrixFromEulerAngles(VxMatrix &out, float, float, float)", asFUNCTION(Vx3DMatrixFromEulerAngles), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DMatrixToEulerAngles(const VxMatrix &in, float &out, float &out, float &out)", asFUNCTION(Vx3DMatrixToEulerAngles), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DInterpolateMatrix(float step, VxMatrix &out, const VxMatrix &in, const VxMatrix &in)", asFUNCTION(Vx3DInterpolateMatrix), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DInterpolateMatrixNoScale(float step, VxMatrix &out, const VxMatrix &in, const VxMatrix &in)", asFUNCTION(Vx3DInterpolateMatrixNoScale), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DTransposeMatrix(VxMatrix &out, const VxMatrix &in)", asFUNCTION(Vx3DTransposeMatrix), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("void Vx3DDecomposeMatrix(const VxMatrix &in, VxQuaternion &out, VxVector &out, VxVector &out)", asFUNCTION(Vx3DDecomposeMatrix), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float Vx3DDecomposeMatrixTotal(const VxMatrix &in, VxQuaternion &out, VxVector &out, VxVector &out, VxQuaternion &out)", asFUNCTION(Vx3DDecomposeMatrixTotal), asCALL_CDECL); assert(r >= 0);
}

// VxQuaternion

static void RegisterVxQuaternion(asIScriptEngine *engine) {
    int r = 0;

    // Enum QuatPart
    r = engine->RegisterEnum("QuatPart"); assert(r >= 0);
    r = engine->RegisterEnumValue("QuatPart", "Quat_X", Quat_X); assert(r >= 0);
    r = engine->RegisterEnumValue("QuatPart", "Quat_Y", Quat_Y); assert(r >= 0);
    r = engine->RegisterEnumValue("QuatPart", "Quat_Z", Quat_Z); assert(r >= 0);
    r = engine->RegisterEnumValue("QuatPart", "Quat_W", Quat_W); assert(r >= 0);

    // Properties
    r = engine->RegisterObjectProperty("VxQuaternion", "float x", asOFFSET(VxQuaternion, x)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxQuaternion", "float y", asOFFSET(VxQuaternion, y)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxQuaternion", "float z", asOFFSET(VxQuaternion, z)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxQuaternion", "float w", asOFFSET(VxQuaternion, w)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxQuaternion", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxQuaternion *self) { new(self) VxQuaternion(); }, (VxQuaternion*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxQuaternion", asBEHAVE_CONSTRUCT, "void f(const VxQuaternion &in)", asFUNCTIONPR([](const VxQuaternion &quat, VxQuaternion *self) { new(self) VxQuaternion(quat); }, (const VxQuaternion &, VxQuaternion*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxQuaternion", asBEHAVE_CONSTRUCT, "void f(const VxVector &in, float)", asFUNCTIONPR([](const VxVector &v, float angle, VxQuaternion *self) { new(self) VxQuaternion(v, angle); }, (const VxVector &, float, VxQuaternion*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxQuaternion", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTIONPR([](float x, float y, float z, float w, VxQuaternion *self) { new(self) VxQuaternion(x, y, z, w); }, (float, float, float, float, VxQuaternion*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxQuaternion", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float, float}", asFUNCTIONPR([](float *list, VxQuaternion *self) { new(self) VxQuaternion(list[0], list[1], list[2], list[3]); }, (float *, VxQuaternion *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("VxQuaternion", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxQuaternion *self) { self->~VxQuaternion(); }, (VxQuaternion*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxQuaternion", "VxQuaternion &opAssign(const VxQuaternion &in)", asMETHODPR(VxQuaternion, operator=, (const VxQuaternion &), VxQuaternion &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxQuaternion", "bool opEquals(const VxQuaternion &in) const", asFUNCTIONPR(operator==, (const VxQuaternion &, const VxQuaternion &), int), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxQuaternion", "VxQuaternion &opMulAssign(float) const", asMETHODPR(VxQuaternion, operator*=, (float), VxQuaternion &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxQuaternion", "VxQuaternion opAdd(const VxQuaternion &in) const", asMETHODPR(VxQuaternion, operator+, (const VxQuaternion &) const, VxQuaternion), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "VxQuaternion opSub(const VxQuaternion &in) const", asMETHODPR(VxQuaternion, operator-, (const VxQuaternion &) const, VxQuaternion), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "VxQuaternion opMul(const VxQuaternion &in) const", asMETHODPR(VxQuaternion, operator*, (const VxQuaternion &) const, VxQuaternion), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "VxQuaternion opDiv(const VxQuaternion &in) const", asMETHODPR(VxQuaternion, operator/, (const VxQuaternion &) const, VxQuaternion), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxQuaternion", "VxQuaternion opMul(float) const", asFUNCTIONPR(operator*, (const VxQuaternion &, float), VxQuaternion), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxQuaternion", "VxQuaternion opNeg() const", asMETHODPR(VxQuaternion, operator-, () const, VxQuaternion), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxQuaternion", "float& opIndex(int)", asMETHODPR(VxQuaternion, operator[], (int), float&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "const float& opIndex(int) const", asMETHODPR(VxQuaternion, operator[], (int) const, const float&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxQuaternion", "void FromMatrix(const VxMatrix &in)", asMETHOD(VxQuaternion, FromMatrix), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "void ToMatrix(VxMatrix &out) const", asMETHOD(VxQuaternion, ToMatrix), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "void Multiply(const VxQuaternion &in)", asMETHOD(VxQuaternion, Multiply), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "void FromRotation(const VxVector &in, float)", asMETHOD(VxQuaternion, FromRotation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "void FromEulerAngles(float, float, float)", asMETHOD(VxQuaternion, FromEulerAngles), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "void ToEulerAngles(float &out, float &out, float &out) const", asMETHOD(VxQuaternion, ToEulerAngles), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "void Normalize()", asMETHOD(VxQuaternion, Normalize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "float Magnitude() const", asFUNCTIONPR(Magnitude, (const VxVector &), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxQuaternion", "float DotProduct(const VxQuaternion &in) const", asFUNCTIONPR(DotProduct, (const VxVector &, const VxVector &), float), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    
    // Function registration
    r = engine->RegisterGlobalFunction("VxQuaternion Vx3DQuaternionSnuggle(VxQuaternion &in, VxVector &in)", asFUNCTION(Vx3DQuaternionSnuggle), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("VxQuaternion Vx3DQuaternionFromMatrix(const VxMatrix &in)", asFUNCTION(Vx3DQuaternionFromMatrix), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("VxQuaternion Vx3DQuaternionConjugate(const VxQuaternion &in)", asFUNCTION(Vx3DQuaternionConjugate), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("VxQuaternion Vx3DQuaternionMultiply(const VxQuaternion &in, const VxQuaternion &in)", asFUNCTION(Vx3DQuaternionMultiply), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("VxQuaternion Vx3DQuaternionDivide(const VxQuaternion &in, const VxQuaternion &in)", asFUNCTION(Vx3DQuaternionDivide), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("VxQuaternion Slerp(float, const VxQuaternion &in, const VxQuaternion &in)", asFUNCTION(Slerp), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("VxQuaternion Squad(float, const VxQuaternion &in, const VxQuaternion &in, const VxQuaternion &in, const VxQuaternion &in)", asFUNCTION(Squad), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("VxQuaternion LnDif(const VxQuaternion &in, const VxQuaternion &in)", asFUNCTION(LnDif), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("VxQuaternion Ln(const VxQuaternion &in)", asFUNCTION(Ln), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("VxQuaternion Exp(const VxQuaternion &in)", asFUNCTION(Exp), asCALL_CDECL); assert(r >= 0);
}

// VxRect

static void RegisterVxRect(asIScriptEngine *engine) {
    int r = 0;

    // Enum VXRECT_INTERSECTION
    r = engine->RegisterEnum("VXRECT_INTERSECTION"); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRECT_INTERSECTION", "ALLOUTSIDE", ALLOUTSIDE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRECT_INTERSECTION", "ALLINSIDE", ALLINSIDE); assert(r >= 0);
    r = engine->RegisterEnumValue("VXRECT_INTERSECTION", "PARTINSIDE", PARTINSIDE); assert(r >= 0);

    // Properties
    r = engine->RegisterObjectProperty("VxRect", "float left", asOFFSET(VxRect, left)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxRect", "float top", asOFFSET(VxRect, top)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxRect", "float right", asOFFSET(VxRect, right)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxRect", "float bottom", asOFFSET(VxRect, bottom)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxRect", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxRect *self) { new(self) VxRect(); }, (VxRect*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxRect", asBEHAVE_CONSTRUCT, "void f(const VxRect &in)", asFUNCTIONPR([](const VxRect &rect, VxRect *self) { new(self) VxRect(rect); }, (const VxRect &, VxRect*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxRect", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTIONPR([](float l, float t, float r, float b, VxRect *self) { new(self) VxRect(l, t, r, b); }, (float, float, float, float, VxRect*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxRect", asBEHAVE_CONSTRUCT, "void f(Vx2DVector &in, Vx2DVector &in)", asFUNCTIONPR([](Vx2DVector &topleft, Vx2DVector &bottomright, VxRect *self) { new(self) VxRect(topleft, bottomright); }, (Vx2DVector &, Vx2DVector &, VxRect*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxRect", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float, float}", asFUNCTIONPR([](float *list, VxRect *self) { new(self) VxRect(list[0], list[1], list[2], list[3]); }, (float *, VxRect *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("VxRect", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxRect *self) { self->~VxRect(); }, (VxRect*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    
    // Methods
    r = engine->RegisterObjectMethod("VxRect", "VxRect &opAssign(const VxRect &in)", asMETHODPR(VxRect, operator=, (const VxRect &), VxRect &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxRect", "bool opEquals(const VxRect &in) const", asFUNCTIONPR([](const VxRect &lhs, const VxRect &rhs) -> bool { return lhs == rhs; }, (const VxRect &, const VxRect &), bool), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxRect", "VxRect &opAddAssign(const Vx2DVector &in)", asMETHODPR(VxRect, operator+=, (const Vx2DVector &), VxRect&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "VxRect &opSubAssign(const Vx2DVector &in)", asMETHODPR(VxRect, operator-=, (const Vx2DVector &), VxRect&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "VxRect &opMulAssign(const Vx2DVector &in)", asMETHODPR(VxRect, operator*=, (const Vx2DVector &), VxRect&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "VxRect &opDivAssign(const Vx2DVector &in)", asMETHODPR(VxRect, operator/=, (const Vx2DVector &), VxRect&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxRect", "void SetWidth(float)", asMETHOD(VxRect, SetWidth), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "float GetWidth() const", asMETHOD(VxRect, GetWidth), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetHeight(float)", asMETHOD(VxRect, SetHeight), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "float GetHeight() const", asMETHOD(VxRect, GetHeight), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "float GetHCenter() const", asMETHOD(VxRect, GetHCenter), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "float GetVCenter() const", asMETHOD(VxRect, GetVCenter), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetSize(const Vx2DVector &in)", asMETHOD(VxRect, SetSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "Vx2DVector GetSize() const", asMETHOD(VxRect, GetSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetHalfSize(const Vx2DVector &in)", asMETHOD(VxRect, SetHalfSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "Vx2DVector GetHalfSize() const", asMETHOD(VxRect, GetHalfSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetCenter(const Vx2DVector &in)", asMETHODPR(VxRect, SetCenter, (const Vx2DVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "Vx2DVector GetCenter() const", asMETHODPR(VxRect, GetCenter, () const, Vx2DVector), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetTopLeft(const Vx2DVector &in)", asMETHODPR(VxRect, SetTopLeft, (const Vx2DVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "const Vx2DVector &GetTopLeft() const", asMETHODPR(VxRect, GetTopLeft, () const, const Vx2DVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "Vx2DVector &GetTopLeft()", asMETHODPR(VxRect, GetTopLeft, (), Vx2DVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetBottomRight(const Vx2DVector &in)", asMETHOD(VxRect, SetBottomRight), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "const Vx2DVector &GetBottomRight() const", asMETHODPR(VxRect, GetBottomRight, () const, const Vx2DVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "Vx2DVector &GetBottomRight()", asMETHODPR(VxRect, GetBottomRight, (), Vx2DVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Clear()", asMETHOD(VxRect, Clear), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetCorners(const Vx2DVector &in, const Vx2DVector &in)", asMETHODPR(VxRect, SetCorners, (const Vx2DVector &, const Vx2DVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetCorners(float, float, float, float)", asMETHODPR(VxRect, SetCorners, (float, float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetDimension(const Vx2DVector &in, const Vx2DVector &in)", asMETHODPR(VxRect, SetDimension, (const Vx2DVector &, const Vx2DVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetDimension(float, float, float, float)", asMETHODPR(VxRect, SetDimension, (float, float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetCenter(const Vx2DVector &in, const Vx2DVector &in)", asMETHODPR(VxRect, SetCenter, (const Vx2DVector &, const Vx2DVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void SetCenter(float, float, float, float)", asMETHODPR(VxRect, SetCenter, (float, float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void CopyFrom(const CKRECT &in)", asMETHOD(VxRect, CopyFrom), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void CopyTo(CKRECT &out) const", asMETHOD(VxRect, CopyTo), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Bounding(const Vx2DVector &in, const Vx2DVector &in)", asMETHODPR(VxRect, Bounding, (const Vx2DVector &, const Vx2DVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Normalize()", asMETHOD(VxRect, Normalize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Move(const Vx2DVector &in)", asMETHOD(VxRect, Move), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Translate(const Vx2DVector &in)", asMETHOD(VxRect, Translate), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void HMove(float)", asMETHOD(VxRect, HMove), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void VMove(float)", asMETHOD(VxRect, VMove), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void HTranslate(float)", asMETHOD(VxRect, HTranslate), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void VTranslate(float)", asMETHOD(VxRect, VTranslate), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void TransformFromHomogeneous(Vx2DVector &out, const Vx2DVector &in) const", asMETHODPR(VxRect, TransformFromHomogeneous, (Vx2DVector &, const Vx2DVector &) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Scale(const Vx2DVector &in)", asMETHOD(VxRect, Scale), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Inflate(const Vx2DVector &in)", asMETHOD(VxRect, Inflate), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Interpolate(float, const VxRect &in)", asMETHOD(VxRect, Interpolate), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Merge(const VxRect &in)", asMETHOD(VxRect, Merge), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "int IsInside(const VxRect &in) const", asMETHODPR(VxRect, IsInside, (const VxRect &) const, int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "bool IsOutside(const VxRect &in) const", asMETHOD(VxRect, IsOutside), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "bool IsInside(const Vx2DVector &in) const", asMETHODPR(VxRect, IsInside, (const Vx2DVector &) const, XBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "bool IsNull() const", asMETHODPR(VxRect, IsNull, () const, XBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "bool IsEmpty() const", asMETHODPR(VxRect, IsEmpty, () const, XBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "bool Clip(const VxRect &in)", asMETHODPR(VxRect, Clip, (const VxRect &), XBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Clip(Vx2DVector &out, bool = true) const", asMETHODPR(VxRect, Clip, (Vx2DVector &, XBOOL) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Transform(const VxRect &in, const VxRect &in)", asMETHODPR(VxRect, Transform, (const VxRect &, const VxRect &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void Transform(const Vx2DVector &in, const Vx2DVector &in)", asMETHODPR(VxRect, Transform, (const Vx2DVector &, const Vx2DVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void TransformToHomogeneous(const VxRect &in)", asMETHODPR(VxRect, TransformToHomogeneous, (const VxRect &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRect", "void TransformFromHomogeneous(const VxRect &in)", asMETHODPR(VxRect, TransformFromHomogeneous, (const VxRect &), void), asCALL_THISCALL); assert(r >= 0);
}

// VxOBB

static void RegisterVxOBB(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxOBB", "VxVector center", asOFFSET(VxOBB, m_Center)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxOBB", "VxVector axisX", asOFFSET(VxOBB, m_Axis[0])); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxOBB", "VxVector axisY", asOFFSET(VxOBB, m_Axis[1])); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxOBB", "VxVector axisZ", asOFFSET(VxOBB, m_Axis[2])); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxOBB", "VxVector extents", asOFFSET(VxOBB, m_Extents)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxOBB", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxOBB *self) { new(self) VxOBB(); }, (VxOBB *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxOBB", asBEHAVE_CONSTRUCT, "void f(const VxOBB &in)", asFUNCTIONPR([](const VxOBB &obb, VxOBB *self) { new(self) VxOBB(obb); }, (const VxOBB &, VxOBB*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxOBB", asBEHAVE_CONSTRUCT, "void f(const VxBbox &in, const VxMatrix &in)", asFUNCTIONPR([](const VxBbox &box, const VxMatrix &mat, VxOBB *self) { new(self) VxOBB(box, mat); }, (const VxBbox &, const VxMatrix &, VxOBB *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("VxOBB", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxOBB *self) { self->~VxOBB(); }, (VxOBB*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxOBB", "VxOBB &opAssign(const VxOBB &in)", asMETHODPR(VxOBB, operator=, (const VxOBB &), VxOBB &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxOBB", "bool opEquals(const VxOBB &in) const", asMETHODPR(VxOBB, operator==, (const VxOBB &) const, bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxOBB", "VxVector &GetCenter()", asMETHODPR(VxOBB, GetCenter, (), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxOBB", "const VxVector &GetCenter() const", asMETHODPR(VxOBB, GetCenter, () const, const VxVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxOBB", "VxVector &GetAxis(int)", asMETHODPR(VxOBB, GetAxis, (int), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxOBB", "const VxVector &GetAxis(int) const", asMETHODPR(VxOBB, GetAxis, (int) const, const VxVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxOBB", "float &GetExtent(int)", asMETHODPR(VxOBB, GetExtent, (int), float &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxOBB", "const float &GetExtent(int) const", asMETHODPR(VxOBB, GetExtent, (int) const, const float &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxOBB", "void Create(const VxBbox &in, const VxMatrix &in)", asMETHODPR(VxOBB, Create, (const VxBbox &, const VxMatrix &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxOBB", "bool VectorIn(const VxVector &in) const", asMETHODPR(VxOBB, VectorIn, (const VxVector &) const, XBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxOBB", "bool IsBoxInside(const VxBbox &in) const", asMETHODPR(VxOBB, IsBoxInside, (const VxBbox &) const, XBOOL), asCALL_THISCALL); assert(r >= 0);
}

// VxRay

static void RegisterVxRay(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxRay", "VxVector origin", asOFFSET(VxRay, m_Origin)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxRay", "VxVector direction", asOFFSET(VxRay, m_Direction)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxRay", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxRay *self) { new(self) VxRay(); }, (VxRay *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxRay", asBEHAVE_CONSTRUCT, "void f(const VxRay &in)", asFUNCTIONPR([](const VxRay &r, VxRay *self) { new(self) VxRay(r); }, (const VxRay &, VxRay*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxRay", asBEHAVE_CONSTRUCT, "void f(const VxVector &in, const VxVector &in)", asFUNCTIONPR([](const VxVector &start, const VxVector &end, VxRay *self) { new(self) VxRay(start, end); }, (const VxVector &, const VxVector &, VxRay *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxRay", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {VxVector, VxVector}", asFUNCTIONPR([](VxVector *list, VxRay *self) { new(self) VxRay(list[0], list[1]); }, (VxVector *, VxRay *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("VxRay", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxRay *self) { self->~VxRay(); }, (VxRay*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxRay", "VxRay &opAssign(const VxRay &in)", asMETHODPR(VxRay, operator=, (const VxRay &), VxRay &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxRay", "bool opEquals(const VxRay &in) const", asMETHODPR(VxRay, operator==, (const VxRay &) const, bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxRay", "const VxVector &GetOrigin() const", asMETHODPR(VxRay, GetOrigin, () const, const VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRay", "VxVector &GetOrigin()", asMETHODPR(VxRay, GetOrigin, (), VxVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxRay", "const VxVector &GetDirection() const", asMETHODPR(VxRay, GetDirection, () const, const VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRay", "VxVector &GetDirection()", asMETHODPR(VxRay, GetDirection, (), VxVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxRay", "void Transform(VxRay &out, const VxMatrix &in)", asMETHODPR(VxRay, Transform, (VxRay &, const VxMatrix &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRay", "void Interpolate(VxVector &out, float) const", asMETHODPR(VxRay, Interpolate, (VxVector &, float) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRay", "float SquareDistance(const VxVector &in) const", asMETHODPR(VxRay, SquareDistance, (const VxVector &) const, float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxRay", "float Distance(const VxVector &in) const", asMETHODPR(VxRay, Distance, (const VxVector &) const, float), asCALL_THISCALL); assert(r >= 0);
}

// VxSphere

static void RegisterVxSphere(asIScriptEngine *engine) {
    int r = 0;

    // Constructors
    r = engine->RegisterObjectBehaviour("VxSphere", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxSphere *self) { new(self) VxSphere(); }, (VxSphere *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxSphere", asBEHAVE_CONSTRUCT, "void f(const VxSphere &in)", asFUNCTIONPR([](const VxSphere &s, VxSphere *self) { new(self) VxSphere(s); }, (const VxSphere &, VxSphere*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxSphere", asBEHAVE_CONSTRUCT, "void f(const VxVector &in, float)", asFUNCTIONPR([](const VxVector &center, float radius, VxSphere *self) { new(self) VxSphere(center, radius); }, (const VxVector &, float, VxSphere *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("VxSphere", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxSphere *self) { self->~VxSphere(); }, (VxSphere*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxSphere", "VxSphere &opAssign(const VxSphere &in)", asMETHODPR(VxSphere, operator=, (const VxSphere &), VxSphere &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxSphere", "bool opEquals(const VxSphere &in) const", asMETHODPR(VxSphere, operator==, (const VxSphere &) const, bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxSphere", "VxVector &Center()", asMETHODPR(VxSphere, Center, (), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxSphere", "const VxVector &Center() const", asMETHODPR(VxSphere, Center, () const, const VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxSphere", "float &Radius()", asMETHODPR(VxSphere, Radius, (), float &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxSphere", "const float &Radius() const", asMETHODPR(VxSphere, Radius, () const, const float &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxSphere", "bool IsPointInside(const VxVector &in)", asMETHODPR(VxSphere, IsPointInside, (const VxVector &), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxSphere", "bool IsBoxTotallyInside(const VxBbox &in)", asMETHODPR(VxSphere, IsBoxTotallyInside, (const VxBbox &), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxSphere", "bool IsPointOnSurface(const VxVector &in)", asMETHODPR(VxSphere, IsPointOnSurface, (const VxVector &), bool), asCALL_THISCALL); assert(r >= 0);
}

// VxPlane

static void RegisterVxPlane(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxPlane", "VxVector normal", asOFFSET(VxPlane, m_Normal)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxPlane", "float d", asOFFSET(VxPlane, m_D)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxPlane", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxPlane *self) { new(self) VxPlane(); }, (VxPlane *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxPlane", asBEHAVE_CONSTRUCT, "void f(const VxPlane &in)", asFUNCTIONPR([](const VxPlane &p, VxPlane *self) { new(self) VxPlane(p); }, (const VxPlane &, VxPlane*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxPlane", asBEHAVE_CONSTRUCT, "void f(const VxVector &in, float)", asFUNCTIONPR([](const VxVector &n, float d, VxPlane *self) { new(self) VxPlane(n, d); }, (const VxVector &, float, VxPlane *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxPlane", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTIONPR([](float a, float b, float c, float d, VxPlane *self) { new(self) VxPlane(a, b, c, d); }, (float, float, float, float, VxPlane *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxPlane", asBEHAVE_CONSTRUCT, "void f(const VxVector &in, const VxVector &in)", asFUNCTIONPR([](const VxVector &n, const VxVector &p, VxPlane *self) { new(self) VxPlane(n, p); }, (const VxVector &, const VxVector &, VxPlane *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxPlane", asBEHAVE_CONSTRUCT, "void f(const VxVector &in, const VxVector &in, const VxVector &in)", asFUNCTIONPR([](const VxVector &a, const VxVector &b, const VxVector &c, VxPlane *self) { new(self) VxPlane(a, b, c); }, (const VxVector &, const VxVector &, const VxVector &, VxPlane *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("VxPlane", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxPlane *self) { self->~VxPlane(); }, (VxPlane*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxPlane", "VxPlane &opAssign(const VxPlane &in)", asMETHODPR(VxPlane, operator=, (const VxPlane &), VxPlane &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxPlane", "bool opEquals(const VxPlane &in) const", asMETHODPR(VxPlane, operator==, (const VxPlane &) const, bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxPlane", "VxPlane opNeg() const", asFUNCTIONPR(operator-, (const VxPlane &), const VxPlane), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxPlane", "const VxVector &GetNormal() const", asMETHODPR(VxPlane, GetNormal, () const, const VxVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxPlane", "float Classify(const VxVector &in) const", asMETHODPR(VxPlane, Classify, (const VxVector &) const, float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxPlane", "float Classify(const VxBbox &in) const", asMETHODPR(VxPlane, Classify, (const VxBbox &) const, float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxPlane", "float Classify(const VxBbox &in, const VxMatrix &in) const", asMETHODPR(VxPlane, Classify, (const VxBbox &, const VxMatrix &) const, float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxPlane", "float ClassifyFace(const VxVector &in, const VxVector &in, const VxVector &in) const", asMETHODPR(VxPlane, ClassifyFace, (const VxVector &, const VxVector &, const VxVector &) const, float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxPlane", "float Distance(const VxVector &in) const", asMETHODPR(VxPlane, Distance, (const VxVector &) const, float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxPlane", "VxVector NearestPoint(const VxVector &in) const", asMETHODPR(VxPlane, NearestPoint, (const VxVector &) const, const VxVector), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxPlane", "void Create(const VxVector &in, const VxVector &in)", asMETHODPR(VxPlane, Create, (const VxVector &, const VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxPlane", "void Create(const VxVector &in, const VxVector &in, const VxVector &in)", asMETHODPR(VxPlane, Create, (const VxVector &, const VxVector &, const VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("VxPlane", "float XClassify(const VxVector[4] &in) const", asMETHODPR(VxPlane, XClassify, (const VxVector[4] &) const, float), asCALL_THISCALL); assert(r >= 0);
}

// VxIntersect

static void RegisterVxIntersect(asIScriptEngine *engine) {
    int r = 0;

    // Intersection Ray - Box
    r = engine->RegisterGlobalFunction("bool VxIntersectRayBox(const VxRay &in, const VxBbox &in)", asFUNCTIONPR(VxIntersect::RayBox, (const VxRay &, const VxBbox &), XBOOL), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxIntersectRayBox(const VxRay &in, const VxBbox &in, VxVector &out)", asFUNCTIONPR(VxIntersect::RayBox, (const VxRay &, const VxBbox &, VxVector &, VxVector *, VxVector *, VxVector *), XBOOL), asCALL_CDECL); assert(r >= 0);

    // Intersection Segment - Box
    r = engine->RegisterGlobalFunction("bool VxIntersectSegmentBox(const VxRay &in, const VxBbox &in)", asFUNCTIONPR(VxIntersect::SegmentBox, (const VxRay &, const VxBbox &), XBOOL), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxIntersectSegmentBox(const VxRay &in, const VxBbox &in, VxVector &out)", asFUNCTIONPR(VxIntersect::SegmentBox, (const VxRay &, const VxBbox &, VxVector &, VxVector *, VxVector *, VxVector *), XBOOL), asCALL_CDECL); assert(r >= 0);

    // Intersection Line - Box
    r = engine->RegisterGlobalFunction("bool VxIntersectLineBox(const VxRay &in, const VxBbox &in)", asFUNCTIONPR(VxIntersect::LineBox, (const VxRay &, const VxBbox &), XBOOL), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxIntersectLineBox(const VxRay &in, const VxBbox &in, VxVector &out)", asFUNCTIONPR(VxIntersect::LineBox, (const VxRay &, const VxBbox &, VxVector &, VxVector *, VxVector *, VxVector *), XBOOL), asCALL_CDECL); assert(r >= 0);

    // Intersection Box - Box
    r = engine->RegisterGlobalFunction("bool VxIntersectAABBAABB(const VxBbox &in, const VxBbox &in)", asFUNCTION(VxIntersect::AABBAABB), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxIntersectAABBOBB(const VxBbox &in, const VxOBB &in)", asFUNCTION(VxIntersect::AABBOBB), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxIntersectOBBOBB(const VxOBB &in, const VxOBB &in)", asFUNCTION(VxIntersect::OBBOBB), asCALL_CDECL); assert(r >= 0);

    // Intersection Ray - Plane
    r = engine->RegisterGlobalFunction("bool VxIntersectRayPlane(const VxRay &in, const VxPlane &in, VxVector &out, float &out)", asFUNCTION(VxIntersect::RayPlane), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxIntersectRayPlaneCulled(const VxRay &in, const VxPlane &in, VxVector &out, float &out)", asFUNCTION(VxIntersect::RayPlaneCulled), asCALL_CDECL); assert(r >= 0);

    // Intersection Segment - Plane
    r = engine->RegisterGlobalFunction("bool VxIntersectSegmentPlane(const VxRay &in, const VxPlane &in, VxVector &out, float &out)", asFUNCTION(VxIntersect::SegmentPlane), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxIntersectSegmentPlaneCulled(const VxRay &in, const VxPlane &in, VxVector &out, float &out)", asFUNCTION(VxIntersect::SegmentPlaneCulled), asCALL_CDECL); assert(r >= 0);

    // Intersection Line - Plane
    r = engine->RegisterGlobalFunction("bool VxIntersectLinePlane(const VxRay &in, const VxPlane &in, VxVector &out, float &out)", asFUNCTION(VxIntersect::LinePlane), asCALL_CDECL); assert(r >= 0);

    // Intersection Box - Plane
    r = engine->RegisterGlobalFunction("bool VxIntersectBoxPlane(const VxBbox &in, const VxPlane &in)", asFUNCTIONPR(VxIntersect::BoxPlane, (const VxBbox &, const VxPlane &), XBOOL), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxIntersectBoxPlane(const VxBbox &in, const VxMatrix &in, const VxPlane &in)", asFUNCTIONPR(VxIntersect::BoxPlane, (const VxBbox &, const VxMatrix &, const VxPlane &), XBOOL), asCALL_CDECL); assert(r >= 0);

    // Intersection of 3 Planes
    r = engine->RegisterGlobalFunction("bool VxIntersectPlanes(const VxPlane &in, const VxPlane &in, const VxPlane &in, VxVector &out)", asFUNCTION(VxIntersect::Planes), asCALL_CDECL); assert(r >= 0);

    // Intersection Face - Face
    r = engine->RegisterGlobalFunction("bool VxIntersectFaceFace(const VxVector &in, const VxVector &in, const VxVector &in, const VxVector &in, const VxVector &in, const VxVector &in, const VxVector &in, const VxVector &in)", asFUNCTION(VxIntersect::FaceFace), asCALL_CDECL); assert(r >= 0);

    // Intersection Ray - Face
    r = engine->RegisterGlobalFunction("bool VxIntersectRayFace(const VxRay &in, const VxVector &in, const VxVector &in, const VxVector &in, const VxVector &in, VxVector &out, float &out)", asFUNCTIONPR(VxIntersect::RayFace, (const VxRay &, const VxVector &, const VxVector &, const VxVector &, const VxVector &, VxVector &, float &), XBOOL), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool VxIntersectRayFaceCulled(const VxRay &in, const VxVector &in, const VxVector &in, const VxVector &in, const VxVector &in, VxVector &out, float &out)", asFUNCTION(VxIntersect::RayFaceCulled), asCALL_CDECL); assert(r >= 0);

    // Intersection Sphere - Sphere
    r = engine->RegisterGlobalFunction("bool VxIntersectSphereSphere(const VxSphere &in, const VxVector &in, const VxSphere &in, const VxVector &in, float &out, float &out)", asFUNCTION(VxIntersect::SphereSphere), asCALL_CDECL); assert(r >= 0);

    // Intersection Ray - Sphere
    r = engine->RegisterGlobalFunction("int VxIntersectRaySphere(const VxRay &in, const VxSphere &in, VxVector &out)", asFUNCTION(VxIntersect::RaySphere), asCALL_CDECL); assert(r >= 0);

    // Intersection Sphere - AABB
    r = engine->RegisterGlobalFunction("int VxIntersectSphereAABB(const VxSphere &in, const VxBbox &in)", asFUNCTION(VxIntersect::SphereAABB), asCALL_CDECL); assert(r >= 0);

    // Intersection Frustum - Face
    r = engine->RegisterGlobalFunction("bool VxIntersectFrustumFace(const VxFrustum &in, const VxVector &in, const VxVector &in, const VxVector &in)", asFUNCTION(VxIntersect::FrustumFace), asCALL_CDECL); assert(r >= 0);

    // Intersection Frustum - AABB
    r = engine->RegisterGlobalFunction("bool VxIntersectFrustumAABB(const VxFrustum &in, const VxBbox &in)", asFUNCTION(VxIntersect::FrustumAABB), asCALL_CDECL); assert(r >= 0);

    // Intersection Frustum - OBB
    r = engine->RegisterGlobalFunction("bool VxIntersectFrustumOBB(const VxFrustum &in, const VxBbox &in, const VxMatrix &in)", asFUNCTION(VxIntersect::FrustumOBB), asCALL_CDECL); assert(r >= 0);

    // Intersection Frustum - Box
    r = engine->RegisterGlobalFunction("bool VxIntersectFrustumBox(const VxFrustum &in, const VxBbox &in, const VxMatrix &in)", asFUNCTION(VxIntersect::FrustumBox), asCALL_CDECL); assert(r >= 0);
}

// VxDistance

static void RegisterVxDistance(asIScriptEngine *engine) {
    int r = 0;

    // Lines - Lines Distances
    r = engine->RegisterGlobalFunction("float VxLineLineSquareDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::LineLineSquareDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxLineRaySquareDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::LineRaySquareDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxLineSegmentSquareDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::LineSegmentSquareDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxRayRaySquareDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::RayRaySquareDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxRaySegmentSquareDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::RaySegmentSquareDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxSegmentSegmentSquareDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::SegmentSegmentSquareDistance), asCALL_CDECL); assert(r >= 0);

    // Lines - Lines Distances (Non-Square)
    r = engine->RegisterGlobalFunction("float VxLineLineDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::LineLineDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxLineRayDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::LineRayDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxLineSegmentDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::LineSegmentDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxRayRayDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::RayRayDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxRaySegmentDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::RaySegmentDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxSegmentSegmentDistance(const VxRay &in, const VxRay &in, float &out = void, float &out = void)", asFUNCTION(VxDistance::SegmentSegmentDistance), asCALL_CDECL); assert(r >= 0);

    // Point - Line distances
    r = engine->RegisterGlobalFunction("float VxPointLineSquareDistance(const VxVector &in, const VxRay &in, float &out = void)", asFUNCTION(VxDistance::PointLineSquareDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxPointRaySquareDistance(const VxVector &in, const VxRay &in, float &out = void)", asFUNCTION(VxDistance::PointRaySquareDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxPointSegmentSquareDistance(const VxVector &in, const VxRay &in, float &out = void)", asFUNCTION(VxDistance::PointSegmentSquareDistance), asCALL_CDECL); assert(r >= 0);

    // Point - Line distances (Non-Square)
    r = engine->RegisterGlobalFunction("float VxPointLineDistance(const VxVector &in, const VxRay &in, float &out = void)", asFUNCTION(VxDistance::PointLineDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxPointRayDistance(const VxVector &in, const VxRay &in, float &out = void)", asFUNCTION(VxDistance::PointRayDistance), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("float VxPointSegmentDistance(const VxVector &in, const VxRay &in, float &out = void)", asFUNCTION(VxDistance::PointSegmentDistance), asCALL_CDECL); assert(r >= 0);
}

// VxFrustum

static void RegisterVxFrustum(asIScriptEngine *engine) {
    int r = 0;

    // Constructors
    r = engine->RegisterObjectBehaviour("VxFrustum", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxFrustum *self) { new(self) VxFrustum(); }, (VxFrustum *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxFrustum", asBEHAVE_CONSTRUCT, "void f(const VxFrustum &in)", asFUNCTIONPR([](const VxFrustum &f, VxFrustum *self) { new(self) VxFrustum(f); }, (const VxFrustum &, VxFrustum*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxFrustum", asBEHAVE_CONSTRUCT, "void f(const VxVector &in, const VxVector &in, const VxVector &in, const VxVector &in, float, float, float, float)",
        asFUNCTIONPR([](const VxVector &origin, const VxVector &right, const VxVector &up, const VxVector &dir, float nearplane, float farplane, float fov, float aspectratio, VxFrustum *self)
        { new(self) VxFrustum(origin, right, up, dir, nearplane, farplane, fov, aspectratio); },
        (const VxVector &, const VxVector &, const VxVector &, const VxVector &, float, float, float, float, VxFrustum *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("VxFrustum", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxFrustum *self) { self->~VxFrustum(); }, (VxFrustum*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxFrustum", "VxFrustum &opAssign(const VxFrustum &in)", asMETHODPR(VxFrustum, operator=, (const VxFrustum &), VxFrustum &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxFrustum", "bool opEquals(const VxFrustum &in) const", asMETHODPR(VxFrustum, operator==, (const VxFrustum &) const, bool), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxFrustum", "VxVector &GetOrigin()", asMETHODPR(VxFrustum, GetOrigin, (), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "const VxVector &GetOrigin() const", asMETHODPR(VxFrustum, GetOrigin, () const, const VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "VxVector &GetRight()", asMETHODPR(VxFrustum, GetRight, (), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "const VxVector &GetRight() const", asMETHODPR(VxFrustum, GetRight, () const, const VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "VxVector &GetUp()", asMETHODPR(VxFrustum, GetUp, (), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "const VxVector &GetUp() const", asMETHODPR(VxFrustum, GetUp, () const, const VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "VxVector &GetDir()", asMETHODPR(VxFrustum, GetDir, (), VxVector &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "const VxVector &GetDir() const", asMETHODPR(VxFrustum, GetDir, () const, const VxVector &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxFrustum", "float &GetRBound()", asMETHODPR(VxFrustum, GetRBound, (), float &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "float &GetUBound()", asMETHODPR(VxFrustum, GetUBound, (), float &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "float &GetDMin()", asMETHODPR(VxFrustum, GetDMin, (), float &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "float &GetDMax()", asMETHODPR(VxFrustum, GetDMax, (), float &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "float GetDRatio() const", asMETHOD(VxFrustum, GetDRatio), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "float GetRF() const", asMETHOD(VxFrustum, GetRF), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "float GetUF() const", asMETHOD(VxFrustum, GetUF), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxFrustum", "const VxPlane &GetNearPlane() const", asMETHOD(VxFrustum, GetNearPlane), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "const VxPlane &GetFarPlane() const", asMETHOD(VxFrustum, GetFarPlane), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "const VxPlane &GetLeftPlane() const", asMETHOD(VxFrustum, GetLeftPlane), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "const VxPlane &GetRightPlane() const", asMETHOD(VxFrustum, GetRightPlane), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "const VxPlane &GetUpPlane() const", asMETHOD(VxFrustum, GetUpPlane), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "const VxPlane &GetBottomPlane() const", asMETHOD(VxFrustum, GetBottomPlane), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxFrustum", "uint Classify(const VxVector &in) const", asMETHODPR(VxFrustum, Classify, (const VxVector &) const, XULONG), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "float Classify(const VxBbox &in) const", asMETHODPR(VxFrustum, Classify, (const VxBbox &) const, float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "float Classify(const VxBbox &in, const VxMatrix &in) const", asMETHODPR(VxFrustum, Classify, (const VxBbox &, const VxMatrix &) const, float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "bool IsInside(const VxVector &in) const", asMETHODPR(VxFrustum, IsInside, (const VxVector &) const, XBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "void Transform(const VxMatrix &in)", asMETHODPR(VxFrustum, Transform, (const VxMatrix &), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("VxFrustum", "void ComputeVertices(VxVector[8]) const", asMETHOD(VxFrustum, ComputeVertices), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxFrustum", "void Update()", asMETHOD(VxFrustum, Update), asCALL_THISCALL); assert(r >= 0);
}

// VxColor

static void RegisterVxColor(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxColor", "float r", asOFFSET(VxColor, r)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxColor", "float g", asOFFSET(VxColor, g)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxColor", "float b", asOFFSET(VxColor, b)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxColor", "float a", asOFFSET(VxColor, a)); assert(r >= 0);

    // Constructors
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxColor *self) { new(self) VxColor(); }, (VxColor *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_CONSTRUCT, "void f(const VxColor &in)", asFUNCTIONPR([](const VxColor &f, VxColor *self) { new(self) VxColor(f); }, (const VxColor &, VxColor*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTIONPR([](float r, float g, float b, float a, VxColor *self) { new(self) VxColor(r, g, b, a); }, (float, float, float, float, VxColor *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTIONPR([](float r, float g, float b, VxColor *self) { new(self) VxColor(r, g, b); }, (float, float, float, VxColor *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTIONPR([](float r, VxColor *self) { new(self) VxColor(r); }, (float, VxColor *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_CONSTRUCT, "void f(uint)", asFUNCTIONPR([](unsigned long col, VxColor *self) { new(self) VxColor(col); }, (unsigned long, VxColor *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_CONSTRUCT, "void f(int, int, int, int)", asFUNCTIONPR([](int r, int g, int b, int a, VxColor *self) { new(self) VxColor(r, g, b, a); }, (int, int, int, int, VxColor *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_CONSTRUCT, "void f(int, int, int)", asFUNCTIONPR([](int r, int g, int b, VxColor *self) { new(self) VxColor(r, g, b); }, (int, int, int, VxColor *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float, float}", asFUNCTIONPR([](float *list, VxColor *self) { new(self) VxColor(list[0], list[1], list[2], list[3]); }, (float *, VxColor *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // Destructor
    r = engine->RegisterObjectBehaviour("VxColor", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxColor *self) { self->~VxColor(); }, (VxColor*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxColor", "VxColor &opAssign(const VxColor &in)", asMETHODPR(VxColor, operator=, (const VxColor &), VxColor &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxColor", "bool opEquals(const VxColor &in) const", asFUNCTIONPR(operator==, (const VxColor &, const VxColor &), int), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxColor", "VxColor &opAddAssign(const VxColor &in)", asMETHODPR(VxColor, operator+=, (const VxColor &), VxColor &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "VxColor &opSubAssign(const VxColor &in)", asMETHODPR(VxColor, operator-=, (const VxColor &), VxColor &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "VxColor &opMulAssign(const VxColor &in)", asMETHODPR(VxColor, operator*=, (const VxColor &), VxColor &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "VxColor &opDivAssign(const VxColor &in)", asMETHODPR(VxColor, operator/=, (const VxColor &), VxColor &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "VxColor &opMulAssign(float)", asMETHODPR(VxColor, operator*=, (float), VxColor &), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "VxColor &opDivAssign(float)", asMETHODPR(VxColor, operator/=, (float), VxColor &), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxColor", "VxColor opAdd(const VxColor &in) const", asFUNCTIONPR(operator+, (const VxColor &, const VxColor &), VxColor), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "VxColor opSub(const VxColor &in) const", asFUNCTIONPR(operator-, (const VxColor &, const VxColor &), VxColor), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "VxColor opMul(const VxColor &in) const", asFUNCTIONPR(operator*, (const VxColor &, const VxColor &), VxColor), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "VxColor opDiv(const VxColor &in) const", asFUNCTIONPR(operator/, (const VxColor &, const VxColor &), VxColor), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "VxColor opMul(float) const", asFUNCTIONPR(operator*, (const VxColor &, float), VxColor), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxColor", "void Clear()", asMETHOD(VxColor, Clear), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "void Check()", asMETHOD(VxColor, Check), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "void Set(float, float, float, float)", asMETHODPR(VxColor, Set, (float, float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "void Set(float, float, float)", asMETHODPR(VxColor, Set, (float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "void Set(uint)", asMETHODPR(VxColor, Set, (unsigned long), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "void Set(int, int, int, int)", asMETHODPR(VxColor, Set, (int, int, int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "void Set(int, int, int)", asMETHODPR(VxColor, Set, (int, int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "uint GetRGBA() const", asMETHOD(VxColor, GetRGBA), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "uint GetRGB() const", asMETHOD(VxColor, GetRGB), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxColor", "float GetSquareDistance(const VxColor &in) const", asMETHOD(VxColor, GetSquareDistance), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterGlobalFunction("uint VxColorConvert(float, float, float, float = 1.0)", asFUNCTIONPR(VxColor::Convert, (float, float, float, float), unsigned long), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint VxColorConvert(int, int, int, int = 255)", asFUNCTIONPR(VxColor::Convert, (int, int, int, int), unsigned long), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterGlobalFunction("uint RGBAFTOCOLOR(float, float, float, float)", asFUNCTIONPR(RGBAFTOCOLOR, (float, float, float, float), unsigned long), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint RGBAFTOCOLOR(const VxColor &in)", asFUNCTIONPR(RGBAFTOCOLOR, (const VxColor *), unsigned long), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterGlobalFunction("uint ColorGetRed(uint)", asFUNCTIONPR([](unsigned long rgb) { return ColorGetRed(rgb); }, (unsigned long), unsigned long), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint ColorGetAlpha(uint)", asFUNCTIONPR([](unsigned long rgb) { return ColorGetAlpha(rgb); }, (unsigned long), unsigned long), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint ColorGetGreen(uint)", asFUNCTIONPR([](unsigned long rgb) { return ColorGetGreen(rgb); }, (unsigned long), unsigned long), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint ColorGetBlue(uint)", asFUNCTIONPR([](unsigned long rgb) { return ColorGetBlue(rgb); }, (unsigned long), unsigned long), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint ColorSetAlpha(uint, uint)", asFUNCTIONPR([](unsigned long rgba, unsigned long x) { return ColorSetAlpha(rgba, x); }, (unsigned long, unsigned long), unsigned long), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint ColorSetRed(uint, uint)", asFUNCTIONPR([](unsigned long rgba, unsigned long x) { return ColorSetRed(rgba, x); }, (unsigned long, unsigned long), unsigned long), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint ColorSetGreen(uint, uint)", asFUNCTIONPR([](unsigned long rgba, unsigned long x) { return ColorSetGreen(rgba, x); }, (unsigned long, unsigned long), unsigned long), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("uint ColorSetBlue(uint, uint)", asFUNCTIONPR([](unsigned long rgba, unsigned long x) { return ColorSetBlue(rgba, x); }, (unsigned long, unsigned long), unsigned long), asCALL_CDECL); assert(r >= 0);
}

// VxImageDescEx

static void RegisterVxImageDescEx(asIScriptEngine *engine) {
    int r = 0;

    // Properties
    r = engine->RegisterObjectProperty("VxImageDescEx", "int Size", offsetof(VxImageDescEx, Size)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "uint Flags", offsetof(VxImageDescEx, Flags)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "int Width", offsetof(VxImageDescEx, Width)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "int Height", offsetof(VxImageDescEx, Height)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "int BytesPerLine", offsetof(VxImageDescEx, BytesPerLine)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "int BitsPerPixel", offsetof(VxImageDescEx, BitsPerPixel)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "uint RedMask", offsetof(VxImageDescEx, RedMask)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "uint GreenMask", offsetof(VxImageDescEx, GreenMask)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "uint BlueMask", offsetof(VxImageDescEx, BlueMask)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "uint AlphaMask", offsetof(VxImageDescEx, AlphaMask)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "int16 BytesPerColorEntry", offsetof(VxImageDescEx, BytesPerColorEntry)); assert(r >= 0);
    r = engine->RegisterObjectProperty("VxImageDescEx", "int16 ColorMapEntries", offsetof(VxImageDescEx, ColorMapEntries)); assert(r >= 0);

    // Constructor
    r = engine->RegisterObjectBehaviour("VxImageDescEx", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR([](VxImageDescEx *self) { new(self) VxImageDescEx(); }, (VxImageDescEx *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("VxImageDescEx", asBEHAVE_CONSTRUCT, "void f(const VxImageDescEx &in)", asFUNCTIONPR([](const VxImageDescEx &desc, VxImageDescEx *self) { new(self) VxImageDescEx(desc); }, (const VxImageDescEx &, VxImageDescEx*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Destructor
    r = engine->RegisterObjectBehaviour("VxImageDescEx", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR([](VxImageDescEx *self) { self->~VxImageDescEx(); }, (VxImageDescEx*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Methods
    r = engine->RegisterObjectMethod("VxImageDescEx", "VxImageDescEx &opAssign(const VxImageDescEx &in)", asFUNCTIONPR([](VxImageDescEx &lhs, const VxImageDescEx &rhs) -> VxImageDescEx & { if (&lhs != &rhs) { lhs.Set(rhs); } return lhs; },  (VxImageDescEx &, const VxImageDescEx &), VxImageDescEx &), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxImageDescEx", "bool opEquals(const VxImageDescEx &in) const", asMETHODPR(VxImageDescEx, operator==, (const VxImageDescEx &), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("VxImageDescEx", "void Set(const VxImageDescEx &in)", asMETHOD(VxImageDescEx, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("VxImageDescEx", "bool HasAlpha() const", asMETHOD(VxImageDescEx, HasAlpha), asCALL_THISCALL); assert(r >= 0);
}

void RegisterVxMath(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterVxMathTypedefs(engine);
    RegisterVxMathEnums(engine);
    RegisterVxMathObjectTypes(engine);
    RegisterVxMathGlobalVariables(engine);
    RegisterVxMathGlobalFunctions(engine);

    RegisterVxMutex(engine);
    RegisterVxTimeProfiler(engine);
    RegisterVxSharedLibrary(engine);
    RegisterVxMemoryMappedFile(engine);
    RegisterCKPathSplitter(engine);
    RegisterCKDirectoryParser(engine);
    RegisterVxWindowFunctions(engine);

    RegisterVxVector(engine);
    RegisterVxVector4(engine);
    RegisterVxBbox(engine);
    RegisterVxCompressedVector(engine);
    RegisterVxCompressedVectorOld(engine);
    RegisterVx2DVector(engine);
    RegisterVxMatrix(engine);
    RegisterVxQuaternion(engine);
    RegisterVxRect(engine);
    RegisterVxOBB(engine);
    RegisterVxRay(engine);
    RegisterVxSphere(engine);
    RegisterVxPlane(engine);
    RegisterVxIntersect(engine);
    RegisterVxDistance(engine);
    RegisterVxFrustum(engine);
    RegisterVxColor(engine);
    RegisterVxImageDescEx(engine);
}
