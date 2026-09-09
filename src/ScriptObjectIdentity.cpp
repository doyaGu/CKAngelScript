#include "ScriptObjectIdentity.h"

#include <unordered_map>

#include "CKAll.h"

namespace {

using ObjectGenerationMap = std::unordered_map<CK_ID, std::uint64_t>;
std::unordered_map<CKContext *, ObjectGenerationMap> g_ObjectIdentityGenerations;

} // namespace

std::uint64_t GetBridgeObjectIdentityGeneration(CKContext *context, CK_ID id) {
    const auto contextIt = g_ObjectIdentityGenerations.find(context);
    if (contextIt == g_ObjectIdentityGenerations.end()) {
        return 0;
    }
    const auto objectIt = contextIt->second.find(id);
    return objectIt == contextIt->second.end() ? 0 : objectIt->second;
}

void AdvanceBridgeObjectIdentityGeneration(CKContext *context, CK_ID id) {
    if (context && id) {
        ++g_ObjectIdentityGenerations[context][id];
    }
}

void ClearBridgeObjectIdentityGenerations(CKContext *context) {
    g_ObjectIdentityGenerations.erase(context);
}

ScriptBridgeObjectStamp CaptureBridgeObjectStamp(CKObject *object) {
    ScriptBridgeObjectStamp stamp;
    if (!object) {
        return stamp;
    }

    stamp.Address = reinterpret_cast<std::uintptr_t>(object);
    stamp.Generation =
        GetBridgeObjectIdentityGeneration(object->GetCKContext(), object->GetID());
    stamp.Set(ScriptBridgeObjectStampFlags::Generation, true);
    stamp.ClassId = object->GetClassID();
    stamp.Set(ScriptBridgeObjectStampFlags::ClassId, true);

    if (CKBehavior *behavior = CKBehavior::Cast(object)) {
        stamp.PrototypeGuid = behavior->GetPrototypeGuid();
        stamp.Set(ScriptBridgeObjectStampFlags::PrototypeGuid,
                  stamp.PrototypeGuid.IsValid() != FALSE);
        CKObject *owner = behavior->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    if (CKParameterIn *input = CKParameterIn::Cast(object)) {
        stamp.TypeGuid = input->GetGUID();
        stamp.Set(ScriptBridgeObjectStampFlags::TypeGuid, stamp.TypeGuid.IsValid() != FALSE);
        CKObject *owner = input->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    if (CKParameterOperation *operation = CKParameterOperation::Cast(object)) {
        stamp.OperationGuid = operation->GetOperationGuid();
        stamp.Set(ScriptBridgeObjectStampFlags::OperationGuid,
                  stamp.OperationGuid.IsValid() != FALSE);
        CKBehavior *owner = operation->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    if (CKParameter *parameter = CKParameter::Cast(object)) {
        stamp.TypeGuid = parameter->GetGUID();
        stamp.Set(ScriptBridgeObjectStampFlags::TypeGuid, stamp.TypeGuid.IsValid() != FALSE);
        CKObject *owner = parameter->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
    }

    return stamp;
}

bool BridgeObjectStampMatches(CKObject *object,
                              const ScriptBridgeObjectStamp &stamp,
                              ScriptBridgeObjectResolvePolicy policy) {
    if (!object ||
        (policy == ScriptBridgeObjectResolvePolicy::LiveOnly && object->IsToBeDeleted())) {
        return false;
    }
    if (stamp.Address && reinterpret_cast<std::uintptr_t>(object) != stamp.Address) {
        return false;
    }
    if (stamp.Has(ScriptBridgeObjectStampFlags::Generation)) {
        const std::uint64_t generation =
            GetBridgeObjectIdentityGeneration(object->GetCKContext(), object->GetID());
        const bool resolvingDeletion =
            policy == ScriptBridgeObjectResolvePolicy::IncludePendingDeletion &&
            object->IsToBeDeleted() &&
            generation == stamp.Generation + 1;
        if (generation != stamp.Generation && !resolvingDeletion) {
            return false;
        }
    }
    if (stamp.Has(ScriptBridgeObjectStampFlags::ClassId) && object->GetClassID() != stamp.ClassId) {
        return false;
    }

    if (stamp.Has(ScriptBridgeObjectStampFlags::PrototypeGuid)) {
        CKBehavior *behavior = CKBehavior::Cast(object);
        if (!behavior || behavior->GetPrototypeGuid() != stamp.PrototypeGuid) {
            return false;
        }
    }
    if (stamp.Has(ScriptBridgeObjectStampFlags::TypeGuid)) {
        if (CKParameterIn *input = CKParameterIn::Cast(object)) {
            if (input->GetGUID() != stamp.TypeGuid) {
                return false;
            }
        } else if (CKParameter *parameter = CKParameter::Cast(object)) {
            if (parameter->GetGUID() != stamp.TypeGuid) {
                return false;
            }
        } else {
            return false;
        }
    }
    if (stamp.Has(ScriptBridgeObjectStampFlags::OperationGuid)) {
        CKParameterOperation *operation = CKParameterOperation::Cast(object);
        if (!operation || operation->GetOperationGuid() != stamp.OperationGuid) {
            return false;
        }
    }
    if (stamp.Has(ScriptBridgeObjectStampFlags::OwnerId)) {
        CK_ID ownerId = 0;
        if (CKBehavior *behavior = CKBehavior::Cast(object)) {
            ownerId = behavior->GetOwner() ? behavior->GetOwner()->GetID() : 0;
        } else if (CKParameterIn *input = CKParameterIn::Cast(object)) {
            ownerId = input->GetOwner() ? input->GetOwner()->GetID() : 0;
        } else if (CKParameterOperation *operation = CKParameterOperation::Cast(object)) {
            ownerId = operation->GetOwner() ? operation->GetOwner()->GetID() : 0;
        } else if (CKParameter *parameter = CKParameter::Cast(object)) {
            ownerId = parameter->GetOwner() ? parameter->GetOwner()->GetID() : 0;
        }
        if (ownerId != stamp.OwnerId) {
            return false;
        }
    }
    return true;
}

CKObject *GetStampedCKObjectById(CKContext *context,
                                 CK_ID id,
                                 const ScriptBridgeObjectStamp &stamp,
                                 ScriptBridgeObjectResolvePolicy policy) {
    CKObject *object = context && id ? CKGetObject(context, id) : nullptr;
    return BridgeObjectStampMatches(object, stamp, policy) ? object : nullptr;
}
