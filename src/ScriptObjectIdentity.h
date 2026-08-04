#ifndef CK_SCRIPT_OBJECT_IDENTITY_H
#define CK_SCRIPT_OBJECT_IDENTITY_H

#include <cstdint>

#include "CKTypes.h"

class CKContext;
class CKObject;

enum class ScriptBridgeObjectStampFlags : CKDWORD {
    None = 0,
    ClassId = 1u << 0,
    TypeGuid = 1u << 1,
    PrototypeGuid = 1u << 2,
    OperationGuid = 1u << 3,
    OwnerId = 1u << 4,
};

inline CKDWORD ScriptBridgeObjectStampFlagMask(ScriptBridgeObjectStampFlags flag) {
    return static_cast<CKDWORD>(flag);
}

inline bool HasScriptBridgeObjectStampFlag(CKDWORD flags, ScriptBridgeObjectStampFlags flag) {
    return (flags & ScriptBridgeObjectStampFlagMask(flag)) != 0;
}

inline void SetScriptBridgeObjectStampFlag(CKDWORD &flags,
                                           ScriptBridgeObjectStampFlags flag,
                                           bool enabled) {
    if (enabled) {
        flags |= ScriptBridgeObjectStampFlagMask(flag);
    } else {
        flags &= ~ScriptBridgeObjectStampFlagMask(flag);
    }
}

struct ScriptBridgeObjectStamp {
    std::uintptr_t Address = 0;
    CKDWORD Flags = 0;
    CK_CLASSID ClassId = 0;
    CK_ID OwnerId = 0;
    CKGUID TypeGuid;
    CKGUID PrototypeGuid;
    CKGUID OperationGuid;

    bool IsCaptured() const {
        return Address != 0 || Flags != 0;
    }

    bool Has(ScriptBridgeObjectStampFlags flag) const {
        return HasScriptBridgeObjectStampFlag(Flags, flag);
    }

    void Set(ScriptBridgeObjectStampFlags flag, bool enabled) {
        SetScriptBridgeObjectStampFlag(Flags, flag, enabled);
    }
};

struct ScriptBridgeStampedObjectId {
    CK_ID Id = 0;
    ScriptBridgeObjectStamp Stamp;
};

using ScriptBridgeStampedObjectRef = ScriptBridgeStampedObjectId;

ScriptBridgeObjectStamp CaptureBridgeObjectStamp(CKObject *object);
bool BridgeObjectStampMatches(CKObject *object, const ScriptBridgeObjectStamp &stamp);
CKObject *GetStampedCKObjectById(CKContext *context,
                                 CK_ID id,
                                 const ScriptBridgeObjectStamp &stamp);

#endif // CK_SCRIPT_OBJECT_IDENTITY_H
