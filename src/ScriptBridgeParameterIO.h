#ifndef CK_SCRIPTBRIDGEPARAMETERIO_H
#define CK_SCRIPTBRIDGEPARAMETERIO_H

#include "ScriptBridgeCommon.h"

// Parameter IO is intentionally index-first in bridge v2. The implementation
// lives in ScriptBridgeCommon.cpp for value/source binding and
// ScriptBridgeParameterIO.cpp for CKParameterOperation wiring.

ParamOperationRef *ConnectOperationToInput(ScriptBehaviorBridge *bridge,
                                           CKBehavior *behavior,
                                           int pinIndex,
                                           const ScriptBridgeOperationSpec &request,
                                           std::string &error,
                                           bool allowOwnerOnly,
                                           std::vector<CK_ID> *operationIds);

bool ValidateOperationSpec(CKContext *context,
                           CKGUID targetTypeGuid,
                           const ScriptBridgeOperationSpec &request,
                           std::string &error);

#endif // CK_SCRIPTBRIDGEPARAMETERIO_H
