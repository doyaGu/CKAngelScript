#ifndef CK_SCRIPTPARAMETERREGISTRY_H
#define CK_SCRIPTPARAMETERREGISTRY_H

#include <string>
#include <unordered_map>
#include <vector>

#include <angelscript.h>

#include "CKTypes.h"
#include "ScriptRefCounted.h"

class CKContext;
class CKParameter;
class CKParameterManager;
class ParamValue;

struct ScriptParamEnumEntry {
    std::string Name;
    int Value = 0;
};

struct ScriptParamStructMember {
    std::string Name;
    CKGUID Guid;
    CKParameterType Type = -1;
};

enum class ScriptParamTypeCaps : CKDWORD {
    None = 0,
    Valid = 1u << 0,
    Stringable = 1u << 1,
    FixedSize = 1u << 2,
    VariableSize = 1u << 3,
    HasLifecycle = 1u << 4,
    ObjectLike = 1u << 5,
    CollectionLike = 1u << 6,
    StructLike = 1u << 7,
    EnumLike = 1u << 8,
    FlagsLike = 1u << 9,
    IntLike = 1u << 10,
    FloatLike = 1u << 11,
    BoolLike = 1u << 12,
    StringLike = 1u << 13,
};

enum class ScriptParamTypeFamily : CKBYTE {
    Unknown,
    Scalar,
    Text,
    Object,
    Collection,
    Enum,
    Flags,
    Struct,
    Custom,
};

inline CKDWORD ScriptParamTypeCapMask(ScriptParamTypeCaps cap) {
    return static_cast<CKDWORD>(cap);
}

inline bool HasScriptParamTypeCap(CKDWORD caps, ScriptParamTypeCaps cap) {
    return (caps & ScriptParamTypeCapMask(cap)) != 0;
}

inline void SetScriptParamTypeCap(CKDWORD &caps, ScriptParamTypeCaps cap, bool enabled) {
    if (enabled) {
        caps |= ScriptParamTypeCapMask(cap);
    } else {
        caps &= ~ScriptParamTypeCapMask(cap);
    }
}

struct ScriptParamTypeRecord {
    CKParameterType Type = -1;
    CKGUID Guid;
    CKGUID DerivedFrom;
    std::string Name;
    int DefaultSize = 0;
    CKDWORD CkFlags = 0;
    CKDWORD Caps = 0;
    CKDWORD Generation = 0;
    CK_CLASSID ClassId = 0;
    ScriptParamTypeFamily Family = ScriptParamTypeFamily::Unknown;
    std::vector<ScriptParamEnumEntry> EnumEntries;
    std::vector<ScriptParamEnumEntry> FlagEntries;
    std::vector<ScriptParamStructMember> StructMembers;

    bool Has(ScriptParamTypeCaps cap) const { return HasScriptParamTypeCap(Caps, cap); }
};

namespace ScriptParameterText {
std::string Trim(const std::string &value);
std::string StripQuotes(const std::string &value);
std::string ToLower(std::string value);
std::vector<std::string> SplitList(const std::string &value);
bool ParseInteger(const std::string &value, int &out);
}

class ScriptParameterRegistry {
public:
    explicit ScriptParameterRegistry(CKContext *context);

    CKContext *GetContext() const { return m_Context; }
    CKParameterManager *GetParameterManager() const;

    void Invalidate();

    CKParameterType ResolveType(const std::string &typeName);
    CKParameterType ResolveType(CKGUID guid);
    CKGUID ResolveGuid(const std::string &typeName, CKGUID fallbackGuid = CKGUID());

    const ScriptParamTypeRecord *GetType(CKParameterType type);
    const ScriptParamTypeRecord *GetType(CKGUID guid);
    const ScriptParamTypeRecord *GetType(const std::string &typeName);
    const ScriptParamTypeRecord *GetType(CKParameter *param);

    bool IsTypeCompatible(CKGUID a, CKGUID b);

    bool ParseEnumValue(CKGUID typeGuid, const std::string &nameOrValue, int &value, std::string &error);
    bool EnumNameOf(CKGUID typeGuid, int value, std::string &name);
    bool ParseFlagsValue(CKGUID typeGuid, const std::string &namesOrMask, CKDWORD &value, std::string &error);
    std::string FlagsText(CKGUID typeGuid, CKDWORD value);

    CKERROR RegisterEnum(CKGUID guid, const std::string &name, const std::string &data);
    CKERROR RegisterFlags(CKGUID guid, const std::string &name, const std::string &data);
    CKERROR RegisterStruct(CKGUID guid, const std::string &name, const std::string &memberNames, const std::vector<CKGUID> &memberGuids);

    static ScriptParameterRegistry *FromContext(CKContext *context);

private:
    const ScriptParamTypeRecord *BuildType(CKParameterType type);
    bool PopulateFromTypeDesc(ScriptParamTypeRecord &record, CKParameterType type);
    void SetRecordCap(ScriptParamTypeRecord &record, ScriptParamTypeCaps cap, bool enabled) const;
    void UpdateRecordFamily(ScriptParamTypeRecord &record) const;
    int FindEnumEntry(const std::vector<ScriptParamEnumEntry> &entries, const std::string &name) const;

    CKContext *m_Context = nullptr;
    CKDWORD m_Generation = 1;
    std::unordered_map<int, ScriptParamTypeRecord> m_TypeCache;
};

class ParamEnumInfo;
class ParamFlagsInfo;
class ParamStructInfo;

class ParamTypeInfo final : public ScriptRefCounted {
public:
    ParamTypeInfo(ScriptParameterRegistry *registry, CKParameterType type);

    bool IsValid() const;
    CKParameterType Type() const;
    CKGUID Guid() const;
    std::string Name() const;
    int Flags() const;
    int DefaultSize() const;
    int ClassId() const;
    bool IsEnum() const;
    bool IsFlags() const;
    bool IsStruct() const;
    bool IsObject() const;
    bool IsCollection() const;
    bool IsStringable() const;
    bool IsFixedSize() const;
    ParamEnumInfo *Enum() const;
    ParamFlagsInfo *FlagsInfo() const;
    ParamStructInfo *Struct() const;
    std::string Describe() const;

private:
    const ScriptParamTypeRecord *Record() const;

    ScriptParameterRegistry *m_Registry = nullptr;
    CKParameterType m_Type = -1;
};

class ParamEnumInfo final : public ScriptRefCounted {
public:
    ParamEnumInfo(ScriptParameterRegistry *registry, CKParameterType type);

    bool IsValid() const;
    int Count() const;
    std::string Name(int index) const;
    int Value(int index) const;
    int Find(const std::string &nameOrValue) const;
    std::string NameOf(int value) const;
    std::string Describe() const;

private:
    const ScriptParamTypeRecord *Record() const;

    ScriptParameterRegistry *m_Registry = nullptr;
    CKParameterType m_Type = -1;
};

class ParamFlagsInfo final : public ScriptRefCounted {
public:
    ParamFlagsInfo(ScriptParameterRegistry *registry, CKParameterType type);

    bool IsValid() const;
    int Count() const;
    std::string Name(int index) const;
    CKDWORD Value(int index) const;
    CKDWORD Parse(const std::string &namesOrMask) const;
    std::string Text(CKDWORD value) const;
    bool Has(CKDWORD mask, const std::string &flagName) const;
    std::string Describe() const;

private:
    const ScriptParamTypeRecord *Record() const;

    ScriptParameterRegistry *m_Registry = nullptr;
    CKParameterType m_Type = -1;
};

class ParamStructInfo final : public ScriptRefCounted {
public:
    ParamStructInfo(ScriptParameterRegistry *registry, CKParameterType type);

    bool IsValid() const;
    int Count() const;
    std::string MemberName(int index) const;
    CKGUID MemberGuid(int index) const;
    ParamTypeInfo *MemberType(int index) const;
    int FindMember(const std::string &name, int occurrence = 0) const;
    std::string Describe() const;

private:
    const ScriptParamTypeRecord *Record() const;

    ScriptParameterRegistry *m_Registry = nullptr;
    CKParameterType m_Type = -1;
};

void RegisterScriptParameterRegistry(asIScriptEngine *engine);
bool RunScriptParameterRegistrySelfTest(CKContext *context, std::string &error);

#endif // CK_SCRIPTPARAMETERREGISTRY_H
