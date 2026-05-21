#ifndef CK_SCRIPT_COMPONENT_METADATA_H
#define CK_SCRIPT_COMPONENT_METADATA_H

#include "CKAll.h"

#include <string>
#include <vector>

#include "ScriptManager.h"
#include "ScriptParameterConversion.h"

namespace AngelScriptComponentInternal {

std::string TrimString(const std::string &value);
std::string ToLower(std::string value);
bool NameEquals(CKSTRING actual, const std::string &expected);
std::string StripQuotes(const std::string &value);
bool ParseBoolText(const std::string &value, bool fallback = false);
std::vector<std::string> TokenizeArguments(const std::string &args);

ScriptParamValueKind ValueKindFromComponentKind(ScriptComponentBindingKind kind);
ScriptComponentBindingKind ComponentKindFromValueKind(ScriptParamValueKind kind);
ScriptComponentBindingKind KindFromTypeName(const std::string &typeName);
CKGUID GuidFromTypeName(CKContext *context, const std::string &typeName, ScriptComponentBindingKind kind);
CKGUID GuidFromPropertyType(CKContext *context, asIScriptEngine *engine, int typeId);
CK_CLASSID ClassIdFromPropertyType(asIScriptEngine *engine, int typeId);
std::string ClassNameFromPropertyType(asIScriptEngine *engine, int typeId);
ScriptComponentBindingKind InferKindFromProperty(asIScriptEngine *engine, int typeId);
bool IsCompatiblePropertyType(asIScriptEngine *engine,
                              int typeId,
                              ScriptComponentBindingKind kind,
                              std::string &expected);

std::string BindingKindName(ScriptComponentBindingKind kind);
bool UsesComponentLifetime(const ScriptComponentBinding &binding);
std::string BBConfigLifetimeText(const ScriptComponentBinding &binding);
std::string BindingSummary(const ScriptComponentBinding &binding, CKContext *context = nullptr);
std::string PublicFieldCandidates(asIScriptEngine *engine, asITypeInfo *type);

std::string BuildBBConfigBindingCacheText(const ScriptComponentBinding &binding,
                                          CK_ID sourceId,
                                          const std::string &sourceText);
ScriptComponentBBStepPolicy ParseBBStepPolicy(const std::string &value);
bool ParseBindingMetadata(const std::string &metadata,
                          const std::string &defaultFieldName,
                          ScriptComponentBinding &binding);
bool ParseManifestLine(const std::string &line, ScriptComponentBinding &binding);
void ReplaceOrAppendBinding(std::vector<ScriptComponentBinding> &bindings, const ScriptComponentBinding &binding);
void MergeOrAppendMetadataBinding(std::vector<ScriptComponentBinding> &bindings, const ScriptComponentBinding &binding);
std::vector<ScriptComponentBinding> BuildComponentBindingSpecs(ScriptComponentState *state, asITypeInfo *type);
bool ResolveComponentBinding(asIScriptEngine *engine,
                             asITypeInfo *type,
                             CKContext *context,
                             ScriptComponentBinding &binding,
                             std::string &error);

} // namespace AngelScriptComponentInternal

#endif
