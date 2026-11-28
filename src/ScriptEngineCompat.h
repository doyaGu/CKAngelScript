#pragma once

#include <cstring>

#include "CKAll.h"

#if CKAS_RUNTIME_SDL
// The script API retains the old SDK's named constants. These are D3D state
// identifiers, not a claim that every rasterizer implements the corresponding
// feature. Ballanced owns capability checks and state validation.
// https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3drenderstatetype
constexpr int CKMAX_PATH = 260; // Source compatibility only; SDL paths are unbounded strings.
using CKGUIDCONSTREF = CKGUID;
using XULONG = XDWORD;
constexpr VXRENDERSTATETYPE VXRENDERSTATE_SCISSORTESTENABLE = static_cast<VXRENDERSTATETYPE>(174);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_SLOPESCALEDEPTHBIAS = static_cast<VXRENDERSTATETYPE>(175);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_ANTIALIASEDLINEENABLE = static_cast<VXRENDERSTATETYPE>(176);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_TWOSIDEDSTENCILMODE = static_cast<VXRENDERSTATETYPE>(185);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_CCW_STENCILFAIL = static_cast<VXRENDERSTATETYPE>(186);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_CCW_STENCILZFAIL = static_cast<VXRENDERSTATETYPE>(187);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_CCW_STENCILPASS = static_cast<VXRENDERSTATETYPE>(188);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_CCW_STENCILFUNC = static_cast<VXRENDERSTATETYPE>(189);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_COLORWRITEENABLE1 = static_cast<VXRENDERSTATETYPE>(190);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_COLORWRITEENABLE2 = static_cast<VXRENDERSTATETYPE>(191);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_COLORWRITEENABLE3 = static_cast<VXRENDERSTATETYPE>(192);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_BLENDFACTOR = static_cast<VXRENDERSTATETYPE>(193);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_SRGBWRITEENABLE = static_cast<VXRENDERSTATETYPE>(194);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_DEPTHBIAS = static_cast<VXRENDERSTATETYPE>(195);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_SEPARATEALPHABLENDENABLE = static_cast<VXRENDERSTATETYPE>(206);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_SRCBLENDALPHA = static_cast<VXRENDERSTATETYPE>(207);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_DESTBLENDALPHA = static_cast<VXRENDERSTATETYPE>(208);
constexpr VXRENDERSTATETYPE VXRENDERSTATE_BLENDOPALPHA = static_cast<VXRENDERSTATETYPE>(209);

// The SDL class table may contain unused slots with no name callback.
inline CKSTRING ScriptClassNameFromId(CK_CLASSID id) {
    if (id >= 0 && id < CKGetClassCount()) {
        const auto *desc = CKGetClassDesc(id);
        if (desc && desc->NameFct) {
            if (const auto name = desc->NameFct()) {
                return name;
            }
        }
    }
    return "Invalid Class Identifier";
}

inline CK_CLASSID ScriptClassIdFromName(const char *name) {
    if (name) {
        const int count = CKGetClassCount();
        for (CK_CLASSID id = 1; id < count; ++id) {
            const auto *desc = CKGetClassDesc(id);
            if (desc && desc->NameFct) {
                const auto current = desc->NameFct();
                if (current && std::strcmp(current, name) == 0) {
                    return id;
                }
            }
        }
    }
    return 0;
}
#else
inline CKSTRING ScriptClassNameFromId(CK_CLASSID id) {
    return CKClassIDToString(id);
}

inline CK_CLASSID ScriptClassIdFromName(const char *name) {
    return CKStringToClassID(const_cast<CKSTRING>(name));
}
#endif
