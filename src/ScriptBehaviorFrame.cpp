#include <angelscript.h>

#include <fmt/format.h>

#include "ScriptBehaviorFrame.h"

#include "CKAll.h"
#include "ScriptAsync.h"
#include "ScriptBridgeHandles.h"
#include "ScriptManager.h"
#include "ScriptParameterConversion.h"
#include "XObjectArray.h"

#undef GetObject

namespace {

CKBehavior *RequireBehavior(CKBehavior *behavior, const char *method) {
    if (!behavior) {
        SetScriptException(fmt::format("{} requires a live behavior.", method ? method : "BehaviorFrame"));
    }
    return behavior;
}

bool ValidIndex(int index, int count, const char *method, const char *kind) {
    if (index >= 0 && index < count) {
        return true;
    }
    SetScriptException(fmt::format("{} index {} is outside {} range 0..{}.",
        method ? method : "BehaviorFrame",
        index,
        kind ? kind : "slot",
        count > 0 ? count - 1 : -1));
    return false;
}

ScriptAsyncScheduler *RequireAsyncScheduler(CKContext *context, const char *method) {
    if (!context) {
        SetScriptException(fmt::format("{} requires a CKContext.", method ? method : "BehaviorFrame async API"));
        return nullptr;
    }
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptAsyncScheduler *scheduler = manager ? manager->GetAsyncScheduler() : nullptr;
    if (!scheduler) {
        SetScriptException(fmt::format("{} requires an async scheduler.", method ? method : "BehaviorFrame async API"));
        return nullptr;
    }
    return scheduler;
}

CKBeObject *ResolveMessageWaitObject(CKBehavior *behavior, CKBeObject *explicitObject) {
    if (explicitObject) {
        return explicitObject;
    }
    if (!behavior) {
        return nullptr;
    }
    if (CKBeObject *target = behavior->GetTarget()) {
        return target;
    }
    return behavior->GetOwner();
}

template <typename ParamGetter>
int ResolveNamedParameter(CKBehavior *behavior,
                          const std::string &name,
                          int occurrence,
                          int count,
                          ParamGetter getter) {
    if (!behavior || occurrence < 0) {
        return -1;
    }
    if (name.empty()) {
        return count > 0 && occurrence == 0 ? 0 : -1;
    }

    int seen = 0;
    for (int i = 0; i < count; ++i) {
        auto *param = getter(i);
        if (!param || !NameEquals(param->GetName(), name)) {
            continue;
        }
        if (seen == occurrence) {
            return i;
        }
        ++seen;
    }
    return -1;
}

int ResolveNamedIo(CKBehavior *behavior, const std::string &name, int occurrence, bool input) {
    if (!behavior || occurrence < 0) {
        return -1;
    }
    const int count = input ? behavior->GetInputCount() : behavior->GetOutputCount();
    if (name.empty()) {
        return count > 0 && occurrence == 0 ? 0 : -1;
    }

    int seen = 0;
    for (int i = 0; i < count; ++i) {
        CKBehaviorIO *io = input ? behavior->GetInput(i) : behavior->GetOutput(i);
        if (!io || !NameEquals(io->GetName(), name)) {
            continue;
        }
        if (seen == occurrence) {
            return i;
        }
        ++seen;
    }
    return -1;
}

std::string DescribeIoNames(CKBehavior *behavior, bool input) {
    if (!behavior) {
        return "<behavior is null>";
    }
    const int count = input ? behavior->GetInputCount() : behavior->GetOutputCount();
    std::string result;
    for (int i = 0; i < count; ++i) {
        CKBehaviorIO *io = input ? behavior->GetInput(i) : behavior->GetOutput(i);
        if (!result.empty()) {
            result += ", ";
        }
        result += fmt::format("#{} '{}'", i, SafeString(io ? io->GetName() : nullptr));
    }
    return result.empty() ? "<none>" : result;
}

int SettingCount(CKBehavior *behavior) {
    if (!behavior) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        if (behavior->IsLocalParameterSetting(i)) {
            ++count;
        }
    }
    return count;
}

int FindSettingIndex(CKBehavior *behavior, const std::string &name, int occurrence) {
    if (!behavior || occurrence < 0) {
        return -1;
    }
    if (name.empty()) {
        if (occurrence != 0) {
            return -1;
        }
        for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
            if (behavior->IsLocalParameterSetting(i)) {
                return i;
            }
        }
        return -1;
    }

    int seen = 0;
    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        if (!behavior->IsLocalParameterSetting(i)) {
            continue;
        }
        CKParameterLocal *param = behavior->GetLocalParameter(i);
        if (param && NameEquals(param->GetName(), name)) {
            if (seen == occurrence) {
                return i;
            }
            ++seen;
        }
    }
    return -1;
}

std::string DescribeParameterNames(CKBehavior *behavior, ScriptBridgeSlotKind kind) {
    if (!behavior) {
        return "<behavior is null>";
    }

    std::string result;
    auto append = [&](const std::string &text) {
        if (!result.empty()) {
            result += ", ";
        }
        result += text;
    };

    if (kind == ScriptBridgeSlotKind::Pin) {
        for (int i = 0; i < behavior->GetInputParameterCount(); ++i) {
            CKParameterIn *param = behavior->GetInputParameter(i);
            append(fmt::format("#{} '{}'", i, SafeString(param ? param->GetName() : nullptr)));
        }
    } else if (kind == ScriptBridgeSlotKind::Pout) {
        for (int i = 0; i < behavior->GetOutputParameterCount(); ++i) {
            CKParameterOut *param = behavior->GetOutputParameter(i);
            append(fmt::format("#{} '{}'", i, SafeString(param ? param->GetName() : nullptr)));
        }
    } else if (kind == ScriptBridgeSlotKind::Setting) {
        for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
            if (!behavior->IsLocalParameterSetting(i)) {
                continue;
            }
            CKParameterLocal *param = behavior->GetLocalParameter(i);
            append(fmt::format("#{} '{}'",
                i,
                SafeString(param ? param->GetName() : nullptr)));
        }
    } else {
        for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
            CKParameterLocal *param = behavior->GetLocalParameter(i);
            append(fmt::format("#{} '{}'{}",
                i,
                SafeString(param ? param->GetName() : nullptr),
                behavior->IsLocalParameterSetting(i) ? " (setting)" : ""));
        }
    }

    return result.empty() ? "<none>" : result;
}

template <typename Setter>
bool ApplyParamRef(ParamRef *ref, Setter setter) {
    if (!ref) {
        return false;
    }
    const bool ok = setter(ref);
    ref->Release();
    return ok;
}

template <int SlotKind>
void BehaviorFrameGetParameterValueGeneric(asIScriptGeneric *generic) {
    auto *frame = static_cast<BehaviorFrame *>(generic->GetObject());
    CKBehavior *behavior = RequireBehavior(frame ? frame->Behavior() : nullptr, "BehaviorFrame.GetParameterValue");
    CKERROR err = ReadBehaviorParameterValue(behavior,
                                             static_cast<ScriptBehaviorParameterSlot>(SlotKind),
                                             generic->GetArgDWord(0),
                                             generic->GetArgTypeId(1),
                                             generic->GetArgAddress(1),
                                             generic->GetEngine(),
                                             asGetActiveContext());
    generic->SetReturnDWord(err);
}

template <int SlotKind>
void BehaviorFrameSetParameterValueGeneric(asIScriptGeneric *generic) {
    auto *frame = static_cast<BehaviorFrame *>(generic->GetObject());
    CKBehavior *behavior = RequireBehavior(frame ? frame->Behavior() : nullptr, "BehaviorFrame.SetParameterValue");
    CKERROR err = WriteBehaviorParameterValue(behavior,
                                              static_cast<ScriptBehaviorParameterSlot>(SlotKind),
                                              generic->GetArgDWord(0),
                                              generic->GetArgTypeId(1),
                                              generic->GetArgAddress(1),
                                              generic->GetEngine(),
                                              asGetActiveContext());
    generic->SetReturnDWord(err);
}

} // namespace

void BehaviorFrameGetInputParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameGetParameterValueGeneric<0>(generic);
}

void BehaviorFrameGetOutputParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameGetParameterValueGeneric<1>(generic);
}

void BehaviorFrameSetOutputParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameSetParameterValueGeneric<1>(generic);
}

void BehaviorFrameGetLocalParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameGetParameterValueGeneric<2>(generic);
}

void BehaviorFrameSetLocalParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameSetParameterValueGeneric<2>(generic);
}

BehaviorFrame::BehaviorFrame(const CKBehaviorContext &ctx) {
    Refresh(ctx);
}

void BehaviorFrame::Refresh(const CKBehaviorContext &ctx) {
    m_ContextSnapshot = ctx;
    m_Context = ctx.Context ? ctx.Context : (ctx.Behavior ? ctx.Behavior->GetCKContext() : nullptr);
    m_BehaviorId = ctx.Behavior ? ctx.Behavior->GetID() : 0;
    m_BehaviorStamp = CaptureBridgeObjectStamp(ctx.Behavior);
    m_ContextSnapshot.Behavior = nullptr;
    m_ContextSnapshot.Context = m_Context;
}

CKBehavior *BehaviorFrame::Behavior() const {
    return CKBehavior::Cast(GetStampedCKObjectById(m_Context, m_BehaviorId, m_BehaviorStamp));
}

CKContext *BehaviorFrame::Context() const {
    return m_Context;
}

CKBehaviorContext BehaviorFrame::ToBehaviorContext() const {
    return MakeContext();
}

float BehaviorFrame::DeltaTime() const {
    return m_ContextSnapshot.DeltaTime;
}

CKLevel *BehaviorFrame::CurrentLevel() const {
    return m_ContextSnapshot.CurrentLevel ? m_ContextSnapshot.CurrentLevel : (m_Context ? m_Context->GetCurrentLevel() : nullptr);
}

CKScene *BehaviorFrame::CurrentScene() const {
    return m_ContextSnapshot.CurrentScene ? m_ContextSnapshot.CurrentScene : (m_Context ? m_Context->GetCurrentScene() : nullptr);
}

CKScene *BehaviorFrame::PreviousScene() const {
    return m_ContextSnapshot.PreviousScene;
}

CKRenderContext *BehaviorFrame::CurrentRenderContext() const {
    return m_ContextSnapshot.CurrentRenderContext ? m_ContextSnapshot.CurrentRenderContext : (m_Context ? m_Context->GetPlayerRenderContext() : nullptr);
}

CKParameterManager *BehaviorFrame::ParameterManager() const {
    return m_ContextSnapshot.ParameterManager ? m_ContextSnapshot.ParameterManager : (m_Context ? m_Context->GetParameterManager() : nullptr);
}

CKMessageManager *BehaviorFrame::MessageManager() const {
    return m_ContextSnapshot.MessageManager ? m_ContextSnapshot.MessageManager : (m_Context ? m_Context->GetMessageManager() : nullptr);
}

CKAttributeManager *BehaviorFrame::AttributeManager() const {
    return m_ContextSnapshot.AttributeManager ? m_ContextSnapshot.AttributeManager : (m_Context ? m_Context->GetAttributeManager() : nullptr);
}

CKTimeManager *BehaviorFrame::TimeManager() const {
    return m_ContextSnapshot.TimeManager ? m_ContextSnapshot.TimeManager : (m_Context ? m_Context->GetTimeManager() : nullptr);
}

CKDWORD BehaviorFrame::CallbackMessage() const {
    return m_ContextSnapshot.CallbackMessage;
}

bool BehaviorFrame::IsValid() const {
    return Behavior() != nullptr;
}

CK_ID BehaviorFrame::Id() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetID() : 0;
}

std::string BehaviorFrame::Name() const {
    CKBehavior *behavior = Behavior();
    return behavior ? SafeString(behavior->GetName()) : std::string();
}

void BehaviorFrame::SetName(const std::string &name, bool shared) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetName")) {
        behavior->SetName(const_cast<CKSTRING>(name.c_str()), shared);
        InvalidateLayout();
    }
}

CK_CLASSID BehaviorFrame::ClassId() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetClassID() : 0;
}

CKDWORD BehaviorFrame::ObjectFlags() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetObjectFlags() : 0;
}

void BehaviorFrame::ModifyObjectFlags(CKDWORD add, CKDWORD remove) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.ModifyObjectFlags")) {
        behavior->ModifyObjectFlags(add, remove);
    }
}

bool BehaviorFrame::IsDynamic() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsDynamic();
}

bool BehaviorFrame::IsToBeDeleted() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsToBeDeleted();
}

bool BehaviorFrame::IsNotToBeSaved() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsNotToBeSaved();
}

bool BehaviorFrame::IsPrivate() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsPrivate();
}

bool BehaviorFrame::IsInterfaceObj() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsInterfaceObj();
}

bool BehaviorFrame::IsUpToDate() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsUpToDate();
}

bool BehaviorFrame::IsObjectUsed(CKObject *object, CK_CLASSID cid) const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsObjectUsed(object, cid);
}

int BehaviorFrame::MemoryOccupation() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetMemoryOccupation() : 0;
}

NativePointer BehaviorFrame::AppData() const {
    CKBehavior *behavior = Behavior();
    return NativePointer(behavior ? behavior->GetAppData() : nullptr);
}

void BehaviorFrame::SetAppData(NativePointer data) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetAppData")) {
        behavior->SetAppData(data.Get());
    }
}

CKERROR BehaviorFrame::DestroyObject(CKObject *object, CKDWORD flags, CKDependencies *dependencies) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CKDestroyObject")) {
        return CKERR_INVALIDOBJECT;
    }
    return behavior->CKDestroyObject(object, flags, dependencies);
}

CKERROR BehaviorFrame::DestroyObjectById(CK_ID id, CKDWORD flags, CKDependencies *dependencies) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CKDestroyObject")) {
        return CKERR_INVALIDOBJECT;
    }
    return behavior->CKDestroyObject(id, flags, dependencies);
}

CKERROR BehaviorFrame::DestroyObjects(const XObjectArray &objects, CKDWORD flags, CKDependencies *dependencies) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CKDestroyObjects")) {
        return CKERR_INVALIDOBJECT;
    }
    return behavior->CKDestroyObjects(objects.Begin(), objects.Size(), flags, dependencies);
}

CKObject *BehaviorFrame::GetObjectById(CK_ID id) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CKGetObject")) {
        return nullptr;
    }
    return behavior->CKGetObject(id);
}

CKERROR BehaviorFrame::Copy(CKObject *object, CKDependenciesContext &context) {
    CKBehavior *behavior = Behavior();
    return behavior && object ? behavior->Copy(*object, context) : CKERR_INVALIDOBJECT;
}

CKERROR BehaviorFrame::PrepareDependencies(CKDependenciesContext &context) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->PrepareDependencies(context) : CKERR_INVALIDOBJECT;
}

CKERROR BehaviorFrame::RemapDependencies(CKDependenciesContext &context) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->RemapDependencies(context) : CKERR_INVALIDOBJECT;
}

CKStateChunk *BehaviorFrame::Save(CKFile *file, CKDWORD flags) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->Save(file, flags) : nullptr;
}

CKERROR BehaviorFrame::Load(CKStateChunk *chunk, CKFile *file) {
    CKBehavior *behavior = Behavior();
    if (!behavior) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->Load(chunk, file);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

void BehaviorFrame::PreSave(CKFile *file, CKDWORD flags) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.PreSave")) {
        behavior->PreSave(file, flags);
    }
}

void BehaviorFrame::PreDelete() {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.PreDelete")) {
        behavior->PreDelete();
    }
}

void BehaviorFrame::CheckPreDeletion() {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.CheckPreDeletion")) {
        behavior->CheckPreDeletion();
    }
}

void BehaviorFrame::CheckPostDeletion() {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.CheckPostDeletion")) {
        behavior->CheckPostDeletion();
    }
}

void BehaviorFrame::PostLoad() {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.PostLoad")) {
        behavior->PostLoad();
        InvalidateLayout();
    }
}

CKScene *BehaviorFrame::SceneIn(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetSceneIn(index) : nullptr;
}

int BehaviorFrame::SceneInCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetSceneInCount() : 0;
}

bool BehaviorFrame::IsInScene(CKScene *scene) const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsInScene(scene);
}

bool BehaviorFrame::IsActiveInCurrentScene() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsActiveInCurrentScene();
}

bool BehaviorFrame::IsActiveInScene(CKScene *scene) const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsActiveInScene(scene);
}

void BehaviorFrame::Show(CK_OBJECT_SHOWOPTION show) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.Show")) {
        behavior->Show(show);
    }
}

bool BehaviorFrame::IsVisible() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsVisible();
}

int BehaviorFrame::CanBeHide() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->CanBeHide() : 0;
}

bool BehaviorFrame::IsHiddenByParent() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsHiddenByParent();
}

bool BehaviorFrame::IsHierarchicallyHide() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsHierarchicallyHide();
}

CKBeObject *BehaviorFrame::Owner() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetOwner() : nullptr;
}

CKBeObject *BehaviorFrame::Target() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetTarget() : nullptr;
}

CKParameterIn *BehaviorFrame::TargetParameter() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetTargetParameter() : nullptr;
}

BehaviorRef *BehaviorFrame::Self() const {
    return WrapBehavior(Behavior());
}

BehaviorRef *BehaviorFrame::Parent() const {
    CKBehavior *behavior = Behavior();
    return WrapBehavior(behavior ? behavior->GetParent() : nullptr);
}

BehaviorRef *BehaviorFrame::OwnerScript() const {
    CKBehavior *behavior = Behavior();
    return WrapBehavior(behavior ? behavior->GetOwnerScript() : nullptr);
}

BehaviorFrame *BehaviorFrame::FrameFor(CKBehavior *target) const {
    if (!target) {
        SetScriptException("BehaviorFrame.FrameFor requires a live behavior.");
        return nullptr;
    }

    CKContext *targetContext = target->GetCKContext();
    if (m_Context && targetContext && targetContext != m_Context) {
        SetScriptException("BehaviorFrame.FrameFor requires a behavior from the same CKContext.");
        return nullptr;
    }

    CKBehaviorContext ctx = MakeContext();
    ctx.Context = targetContext ? targetContext : m_Context;
    ctx.Behavior = target;
    return new BehaviorFrame(ctx);
}

BehaviorFrame *BehaviorFrame::FrameForRef(BehaviorRef *target) const {
    return FrameFor(target ? target->Get() : nullptr);
}

BehaviorFrame *BehaviorFrame::SubFrame(int index) const {
    CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SubFrame");
    if (!behavior) {
        return nullptr;
    }
    if (index < 0 || index >= behavior->GetSubBehaviorCount()) {
        SetScriptException(fmt::format(
            "BehaviorFrame.SubFrame index {} is out of range 0..{}.",
            index,
            behavior->GetSubBehaviorCount() - 1));
        return nullptr;
    }
    return FrameFor(behavior->GetSubBehavior(index));
}

CKBehavior *BehaviorFrame::ParentBehavior() const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.GetParent")) {
        return nullptr;
    }
    return behavior->GetParent();
}

CKBehavior *BehaviorFrame::OwnerScriptBehavior() const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.GetOwnerScript")) {
        return nullptr;
    }
    return behavior->GetOwnerScript();
}

BehaviorLayout *BehaviorFrame::Layout() const {
    ScriptBehaviorBridge *bridge = Bridge();
    CKBehavior *behavior = Behavior();
    return bridge && behavior ? new BehaviorLayout(bridge, behavior->GetID()) : nullptr;
}

BehaviorGraph *BehaviorFrame::Graph() const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame.Graph requires ScriptBehaviorBridge.");
        return nullptr;
    }
    BehaviorBridge *behavior = bridge->CreateBehaviorBridge(MakeContext());
    if (!behavior) {
        return nullptr;
    }
    BehaviorGraph *graph = behavior->Graph();
    behavior->Release();
    return graph;
}

BehaviorGraph *BehaviorFrame::AsGraph() const {
    BehaviorRef *self = Self();
    BehaviorGraph *graph = self ? self->AsGraph() : nullptr;
    if (self) {
        self->Release();
    }
    return graph;
}

CK_BEHAVIOR_TYPE BehaviorFrame::Type() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetType() : CKBEHAVIORTYPE_BASE;
}

void BehaviorFrame::SetType(CK_BEHAVIOR_TYPE type) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetType")) {
        behavior->SetType(type);
        InvalidateLayout();
    }
}

CK_BEHAVIOR_FLAGS BehaviorFrame::Flags() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetFlags() : CKBEHAVIOR_NONE;
}

void BehaviorFrame::SetFlags(CK_BEHAVIOR_FLAGS flags) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetFlags")) {
        behavior->SetFlags(flags);
        InvalidateLayout();
    }
}

CK_BEHAVIOR_FLAGS BehaviorFrame::ModifyFlags(CKDWORD add, CKDWORD remove) {
    CKBehavior *behavior = Behavior();
    if (!behavior) {
        return CKBEHAVIOR_NONE;
    }
    CK_BEHAVIOR_FLAGS flags = behavior->ModifyFlags(add, remove);
    InvalidateLayout();
    return flags;
}

void BehaviorFrame::UseGraph() {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.UseGraph")) {
        behavior->UseGraph();
        InvalidateLayout();
    }
}

void BehaviorFrame::UseFunction() {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.UseFunction")) {
        behavior->UseFunction();
        InvalidateLayout();
    }
}

bool BehaviorFrame::IsUsingFunction() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsUsingFunction();
}

bool BehaviorFrame::IsTargetable() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsTargetable();
}

void BehaviorFrame::SetAsTargetable(bool target) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetAsTargetable")) {
        behavior->SetAsTargetable(target);
        InvalidateLayout();
    }
}

CKERROR BehaviorFrame::UseTarget(bool use) {
    CKBehavior *behavior = Behavior();
    if (!behavior) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->UseTarget(use);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

bool BehaviorFrame::IsUsingTarget() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsUsingTarget();
}

CKParameterIn *BehaviorFrame::ReplaceTargetParameter(CKParameterIn *targetParam) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.ReplaceTargetParameter")) {
        return nullptr;
    }
    CKParameterIn *previous = behavior->ReplaceTargetParameter(targetParam);
    InvalidateLayout();
    return previous;
}

CKParameterIn *BehaviorFrame::RemoveTargetParameter() {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveTargetParameter")) {
        return nullptr;
    }
    CKParameterIn *previous = behavior->RemoveTargetParameter();
    InvalidateLayout();
    return previous;
}

CK_CLASSID BehaviorFrame::CompatibleClassId() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetCompatibleClassID() : CKCID_OBJECT;
}

void BehaviorFrame::SetCompatibleClassId(CK_CLASSID cid) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetCompatibleClassId")) {
        behavior->SetCompatibleClassID(cid);
        InvalidateLayout();
    }
}

void BehaviorFrame::SetFunction(NativePointer fct) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetFunction")) {
        behavior->SetFunction(reinterpret_cast<CKBEHAVIORFCT>(fct.Get()));
        InvalidateLayout();
    }
}

NativePointer BehaviorFrame::Function() const {
    CKBehavior *behavior = Behavior();
    return NativePointer(behavior ? reinterpret_cast<void *>(behavior->GetFunction()) : nullptr);
}

void BehaviorFrame::SetCallbackFunction(NativePointer fct) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetCallbackFunction")) {
        behavior->SetCallbackFunction(reinterpret_cast<CKBEHAVIORCALLBACKFCT>(fct.Get()));
    }
}

int BehaviorFrame::CallCallbackFunction(CKDWORD message) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->CallCallbackFunction(message) : CKERR_INVALIDOBJECT;
}

int BehaviorFrame::CallSubBehaviorsCallbackFunction(CKDWORD message) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->CallSubBehaviorsCallbackFunction(message, nullptr) : CKERR_INVALIDOBJECT;
}

int BehaviorFrame::CallSubBehaviorsCallbackFunction(CKDWORD message, CKGUID guid) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->CallSubBehaviorsCallbackFunction(message, &guid) : CKERR_INVALIDOBJECT;
}

bool BehaviorFrame::IsActive() const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsActive();
}

int BehaviorFrame::Execute(float delta) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->Execute(delta) : CKBR_PARAMETERERROR;
}

void BehaviorFrame::Activate(bool active, bool reset) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.Activate")) {
        behavior->Activate(active, reset);
    }
}

bool BehaviorFrame::ParentScriptActiveInScene(CKScene *scene) const {
    CKBehavior *behavior = Behavior();
    return behavior && behavior->IsParentScriptActiveInScene(scene);
}

int BehaviorFrame::ShortestDelay(CKBehavior *target) const {
    CKBehavior *behavior = Behavior();
    return behavior && target ? behavior->GetShortestDelay(target) : -1;
}

CKERROR BehaviorFrame::InitFromPrototype(CKBehaviorPrototype *prototype) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.InitFromPrototype")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->InitFromPrototype(prototype);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKERROR BehaviorFrame::InitFromGuid(CKGUID guid) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.InitFromGuid")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->InitFromGuid(guid);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKERROR BehaviorFrame::InitFctPtrFromGuid(CKGUID guid) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.InitFctPtrFromGuid")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->InitFctPtrFromGuid(guid);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKERROR BehaviorFrame::InitFctPtrFromPrototype(CKBehaviorPrototype *prototype) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.InitFctPtrFromPrototype")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->InitFctPtrFromPrototype(prototype);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKGUID BehaviorFrame::PrototypeGuid() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetPrototypeGuid() : CKGUID();
}

CKBehaviorPrototype *BehaviorFrame::Prototype() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetPrototype() : nullptr;
}

std::string BehaviorFrame::PrototypeName() const {
    CKBehavior *behavior = Behavior();
    return behavior ? SafeString(behavior->GetPrototypeName()) : std::string();
}

CKDWORD BehaviorFrame::Version() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetVersion() : 0;
}

void BehaviorFrame::SetVersion(CKDWORD version) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetVersion")) {
        behavior->SetVersion(version);
        InvalidateLayout();
    }
}

int BehaviorFrame::Priority() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetPriority() : 0;
}

void BehaviorFrame::SetPriority(int priority) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetPriority")) {
        behavior->SetPriority(priority);
    }
}

float BehaviorFrame::LastExecutionTime() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetLastExecutionTime() : 0.0f;
}

int BehaviorFrame::InputCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetInputCount() : 0;
}

int BehaviorFrame::OutputCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetOutputCount() : 0;
}

int BehaviorFrame::PinCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetInputParameterCount() : 0;
}

int BehaviorFrame::PoutCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetOutputParameterCount() : 0;
}

int BehaviorFrame::LocalCount() const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        return bridge->FrameLocalCount(m_BehaviorId);
    }
    return BehaviorLocalCount();
}

int BehaviorFrame::BehaviorLocalCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetLocalParameterCount() : 0;
}

int BehaviorFrame::SettingCount() const {
    return ::SettingCount(Behavior());
}

int BehaviorFrame::SubBehaviorCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetSubBehaviorCount() : 0;
}

int BehaviorFrame::SubBehaviorLinkCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetSubBehaviorLinkCount() : 0;
}

int BehaviorFrame::ParameterOperationCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetParameterOperationCount() : 0;
}

std::string BehaviorFrame::InputName(int index) const {
    CKBehavior *behavior = Behavior();
    CKBehaviorIO *io = behavior && index >= 0 && index < behavior->GetInputCount() ? behavior->GetInput(index) : nullptr;
    return io ? SafeString(io->GetName()) : std::string();
}

std::string BehaviorFrame::OutputName(int index) const {
    CKBehavior *behavior = Behavior();
    CKBehaviorIO *io = behavior && index >= 0 && index < behavior->GetOutputCount() ? behavior->GetOutput(index) : nullptr;
    return io ? SafeString(io->GetName()) : std::string();
}

int BehaviorFrame::FindInput(const std::string &name, int occurrence) const {
    return ResolveNamedIo(Behavior(), name, occurrence, true);
}

int BehaviorFrame::FindOutput(const std::string &name, int occurrence) const {
    return ResolveNamedIo(Behavior(), name, occurrence, false);
}

int BehaviorFrame::FindPin(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    return ResolveNamedParameter(behavior,
                                 name,
                                 occurrence,
                                 behavior ? behavior->GetInputParameterCount() : 0,
                                 [behavior](int i) { return behavior->GetInputParameter(i); });
}

int BehaviorFrame::FindPout(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    return ResolveNamedParameter(behavior,
                                 name,
                                 occurrence,
                                 behavior ? behavior->GetOutputParameterCount() : 0,
                                 [behavior](int i) { return behavior->GetOutputParameter(i); });
}

int BehaviorFrame::FindLocal(const std::string &name, int occurrence) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        return bridge->FindFrameLocal(m_BehaviorId, name, occurrence);
    }
    return FindBehaviorLocal(name, occurrence);
}

int BehaviorFrame::FindBehaviorLocal(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    return ResolveNamedParameter(behavior,
                                 name,
                                 occurrence,
                                 behavior ? behavior->GetLocalParameterCount() : 0,
                                 [behavior](int i) { return behavior->GetLocalParameter(i); });
}

int BehaviorFrame::FindSetting(const std::string &name, int occurrence) const {
    return FindSettingIndex(Behavior(), name, occurrence);
}

bool BehaviorFrame::InputActive(const std::string &name, int occurrence) const {
    const int index = ResolveInput(name, occurrence, "BehaviorFrame.InputActive");
    return InputActiveIndex(index);
}

bool BehaviorFrame::InputActiveIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetInputCount() && behavior->IsInputActive(index);
}

bool BehaviorFrame::Take(const std::string &name, int occurrence) {
    const int index = ResolveInput(name, occurrence, "BehaviorFrame.Take");
    return TakeIndex(index);
}

bool BehaviorFrame::TakeIndex(int index) {
    CKBehavior *behavior = Behavior();
    if (!behavior || index < 0 || index >= behavior->GetInputCount()) {
        return false;
    }
    const bool active = behavior->IsInputActive(index);
    if (active) {
        behavior->ActivateInput(index, FALSE);
    }
    return active;
}

void BehaviorFrame::Emit(const std::string &name, int occurrence) {
    const int index = ResolveOutput(name, occurrence, "BehaviorFrame.Emit");
    EmitIndex(index);
}

void BehaviorFrame::EmitIndex(int index) {
    CKBehavior *behavior = Behavior();
    if (behavior && index >= 0 && index < behavior->GetOutputCount()) {
        behavior->ActivateOutput(index, TRUE);
    }
}

bool BehaviorFrame::OutputActive(const std::string &name, int occurrence) const {
    const int index = ResolveOutput(name, occurrence, "BehaviorFrame.OutputActive");
    return OutputActiveIndex(index);
}

bool BehaviorFrame::OutputActiveIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetOutputCount() && behavior->IsOutputActive(index);
}

void BehaviorFrame::ActivateInput(const std::string &name, bool active, int occurrence) {
    const int index = ResolveInput(name, occurrence, "BehaviorFrame.ActivateInput");
    ActivateInputIndex(index, active);
}

void BehaviorFrame::ActivateInputIndex(int index, bool active) {
    CKBehavior *behavior = Behavior();
    if (behavior && ValidIndex(index, behavior->GetInputCount(), "BehaviorFrame.ActivateInput", "input")) {
        behavior->ActivateInput(index, active);
    }
}

void BehaviorFrame::ActivateOutput(const std::string &name, bool active, int occurrence) {
    const int index = ResolveOutput(name, occurrence, "BehaviorFrame.ActivateOutput");
    ActivateOutputIndex(index, active);
}

void BehaviorFrame::ActivateOutputIndex(int index, bool active) {
    CKBehavior *behavior = Behavior();
    if (behavior && ValidIndex(index, behavior->GetOutputCount(), "BehaviorFrame.ActivateOutput", "output")) {
        behavior->ActivateOutput(index, active);
    }
}

CKBehaviorIO *BehaviorFrame::Input(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetInputCount() ? behavior->GetInput(index) : nullptr;
}

CKBehaviorIO *BehaviorFrame::Output(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetOutputCount() ? behavior->GetOutput(index) : nullptr;
}

CKParameterIn *BehaviorFrame::InputParameter(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetInputParameterCount() ? behavior->GetInputParameter(index) : nullptr;
}

CKParameterOut *BehaviorFrame::OutputParameter(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
}

CKParameterLocal *BehaviorFrame::LocalParameter(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetLocalParameterCount() ? behavior->GetLocalParameter(index) : nullptr;
}

int BehaviorFrame::AddInput(const std::string &name) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.AddInput")) {
        return -1;
    }
    const int index = behavior->AddInput(const_cast<CKSTRING>(name.c_str()));
    if (index >= 0) {
        InvalidateLayout();
    }
    return index;
}

int BehaviorFrame::AddOutput(const std::string &name) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.AddOutput")) {
        return -1;
    }
    const int index = behavior->AddOutput(const_cast<CKSTRING>(name.c_str()));
    if (index >= 0) {
        InvalidateLayout();
    }
    return index;
}

CKBehaviorIO *BehaviorFrame::CreateInput(const std::string &name) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CreateInput")) {
        return nullptr;
    }
    CKBehaviorIO *io = behavior->CreateInput(const_cast<CKSTRING>(name.c_str()));
    if (io) {
        InvalidateLayout();
    }
    return io;
}

CKBehaviorIO *BehaviorFrame::CreateOutput(const std::string &name) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CreateOutput")) {
        return nullptr;
    }
    CKBehaviorIO *io = behavior->CreateOutput(const_cast<CKSTRING>(name.c_str()));
    if (io) {
        InvalidateLayout();
    }
    return io;
}

CKBehaviorIO *BehaviorFrame::RemoveInput(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveInput")) {
        return nullptr;
    }
    CKBehaviorIO *io = behavior->RemoveInput(index);
    if (io) {
        InvalidateLayout();
    }
    return io;
}

CKBehaviorIO *BehaviorFrame::RemoveOutput(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveOutput")) {
        return nullptr;
    }
    CKBehaviorIO *io = behavior->RemoveOutput(index);
    if (io) {
        InvalidateLayout();
    }
    return io;
}

CKERROR BehaviorFrame::DeleteInput(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.DeleteInput")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->DeleteInput(index);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKERROR BehaviorFrame::DeleteOutput(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.DeleteOutput")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->DeleteOutput(index);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKBehaviorIO *BehaviorFrame::ReplaceInput(int index, CKBehaviorIO *io) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.ReplaceInput")) {
        return nullptr;
    }
    CKBehaviorIO *previous = behavior->ReplaceInput(index, io);
    InvalidateLayout();
    return previous;
}

CKBehaviorIO *BehaviorFrame::ReplaceOutput(int index, CKBehaviorIO *io) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.ReplaceOutput")) {
        return nullptr;
    }
    CKBehaviorIO *previous = behavior->ReplaceOutput(index, io);
    InvalidateLayout();
    return previous;
}

int BehaviorFrame::InputPosition(CKBehaviorIO *io) const {
    CKBehavior *behavior = Behavior();
    return behavior && io ? behavior->GetInputPosition(io) : -1;
}

int BehaviorFrame::OutputPosition(CKBehaviorIO *io) const {
    CKBehavior *behavior = Behavior();
    return behavior && io ? behavior->GetOutputPosition(io) : -1;
}

BehaviorRef *BehaviorFrame::SubBehavior(int index) const {
    CKBehavior *behavior = Behavior();
    return WrapBehavior(behavior && index >= 0 && index < behavior->GetSubBehaviorCount() ? behavior->GetSubBehavior(index) : nullptr);
}

CKBehavior *BehaviorFrame::SubBehaviorObject(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetSubBehaviorCount() ? behavior->GetSubBehavior(index) : nullptr;
}

CKERROR BehaviorFrame::AddSubBehavior(CKBehavior *subBehavior) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.AddSubBehavior")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->AddSubBehavior(subBehavior);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

BehaviorRef *BehaviorFrame::RemoveSubBehavior(CKBehavior *subBehavior) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveSubBehavior")) {
        return nullptr;
    }
    CKBehavior *removed = behavior->RemoveSubBehavior(subBehavior);
    if (removed) {
        InvalidateLayout();
    }
    return WrapBehavior(removed);
}

BehaviorRef *BehaviorFrame::RemoveSubBehaviorIndex(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveSubBehavior")) {
        return nullptr;
    }
    CKBehavior *removed = behavior->RemoveSubBehavior(index);
    if (removed) {
        InvalidateLayout();
    }
    return WrapBehavior(removed);
}

CKBehaviorLink *BehaviorFrame::SubBehaviorLink(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetSubBehaviorLinkCount() ? behavior->GetSubBehaviorLink(index) : nullptr;
}

CKERROR BehaviorFrame::AddSubBehaviorLink(CKBehaviorLink *link) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.AddSubBehaviorLink")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->AddSubBehaviorLink(link);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKBehaviorLink *BehaviorFrame::RemoveSubBehaviorLink(CKBehaviorLink *link) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveSubBehaviorLink")) {
        return nullptr;
    }
    CKBehaviorLink *removed = behavior->RemoveSubBehaviorLink(link);
    if (removed) {
        InvalidateLayout();
    }
    return removed;
}

CKBehaviorLink *BehaviorFrame::RemoveSubBehaviorLinkIndex(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveSubBehaviorLink")) {
        return nullptr;
    }
    CKBehaviorLink *removed = behavior->RemoveSubBehaviorLink(index);
    if (removed) {
        InvalidateLayout();
    }
    return removed;
}

CKParameterOperation *BehaviorFrame::ParameterOperation(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetParameterOperationCount() ? behavior->GetParameterOperation(index) : nullptr;
}

CKERROR BehaviorFrame::AddParameterOperation(CKParameterOperation *operation) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.AddParameterOperation")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->AddParameterOperation(operation);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKParameterOperation *BehaviorFrame::RemoveParameterOperation(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveParameterOperation")) {
        return nullptr;
    }
    CKParameterOperation *removed = behavior->RemoveParameterOperation(index);
    if (removed) {
        InvalidateLayout();
    }
    return removed;
}

CKParameterOperation *BehaviorFrame::RemoveParameterOperationObject(CKParameterOperation *operation) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveParameterOperation")) {
        return nullptr;
    }
    CKParameterOperation *removed = behavior->RemoveParameterOperation(operation);
    if (removed) {
        InvalidateLayout();
    }
    return removed;
}

ParamRef *BehaviorFrame::Pin(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolvePin(name, occurrence, "BehaviorFrame.Pin");
    return behavior ? PinIndex(index) : nullptr;
}

ParamRef *BehaviorFrame::PinIndex(int index) const {
    CKBehavior *behavior = Behavior();
    CKParameterIn *pin = behavior && index >= 0 && index < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(index)
        : nullptr;
    return WrapParameter(pin, ScriptBridgeSlotKind::Pin, index);
}

ParamRef *BehaviorFrame::Pout(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolvePout(name, occurrence, "BehaviorFrame.Pout");
    return behavior ? PoutIndex(index) : nullptr;
}

ParamRef *BehaviorFrame::PoutIndex(int index) const {
    CKBehavior *behavior = Behavior();
    CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount()
        ? behavior->GetOutputParameter(index)
        : nullptr;
    return WrapParameter(pout, ScriptBridgeSlotKind::Pout, index);
}

ParamRef *BehaviorFrame::Local(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolveLocal(name, occurrence, "BehaviorFrame.Local");
    return behavior ? LocalIndex(index) : nullptr;
}

ParamRef *BehaviorFrame::LocalIndex(int index) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        std::string error;
        CKParameter *local = bridge->ResolveFrameLocal(m_Context, m_BehaviorId, index, error);
        if (!local) {
            SetScriptException(error.empty()
                ? fmt::format("BehaviorFrame.Local could not resolve logical local index {}.", index)
                : error);
            return nullptr;
        }
        return WrapParameter(local, ScriptBridgeSlotKind::Local, index);
    }
    return BehaviorLocalIndex(index);
}

ParamRef *BehaviorFrame::BehaviorLocal(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolveBehaviorLocal(name, occurrence, "BehaviorFrame.BehaviorLocal");
    return behavior ? BehaviorLocalIndex(index) : nullptr;
}

ParamRef *BehaviorFrame::BehaviorLocalIndex(int index) const {
    CKBehavior *behavior = Behavior();
    CKParameterLocal *local = behavior && index >= 0 && index < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameter(index)
        : nullptr;
    return WrapParameter(local, ScriptBridgeSlotKind::Local, index);
}

ParamRef *BehaviorFrame::Setting(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int localIndex = ResolveSetting(name, occurrence, "BehaviorFrame.Setting");
    CKParameterLocal *setting = behavior && localIndex >= 0 && localIndex < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameter(localIndex)
        : nullptr;
    return WrapParameter(setting, ScriptBridgeSlotKind::Setting, localIndex);
}

ParamRef *BehaviorFrame::SettingIndex(int index) const {
    CKBehavior *behavior = Behavior();
    const int localIndex = ResolveSettingIndex(index, "BehaviorFrame.Setting");
    CKParameterLocal *setting = behavior && localIndex >= 0 && localIndex < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameter(localIndex)
        : nullptr;
    return WrapParameter(setting, ScriptBridgeSlotKind::Setting, localIndex);
}

std::string BehaviorFrame::PinString(const std::string &name, const std::string &defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::String || value.Kind == ScriptParamValueKind::Text) {
        return value.Text();
    }
    return defaultValue;
}

std::string BehaviorFrame::PinStringIndex(int index, const std::string &defaultValue) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::String || value.Kind == ScriptParamValueKind::Text) {
        return value.Text();
    }
    return defaultValue;
}

int BehaviorFrame::PinInt(const std::string &name, int defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::Int) {
        return value.Data.IntValue;
    }
    if (value.Kind == ScriptParamValueKind::Enum || value.Kind == ScriptParamValueKind::Flags) {
        return static_cast<int>(value.Data.DwordValue);
    }
    return defaultValue;
}

int BehaviorFrame::PinIntIndex(int index, int defaultValue) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::Int) {
        return value.Data.IntValue;
    }
    if (value.Kind == ScriptParamValueKind::Enum || value.Kind == ScriptParamValueKind::Flags) {
        return static_cast<int>(value.Data.DwordValue);
    }
    return defaultValue;
}

bool BehaviorFrame::PinBool(const std::string &name, bool defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
    return value.Kind == ScriptParamValueKind::Bool ? value.Data.BoolValue : defaultValue;
}

bool BehaviorFrame::PinBoolIndex(int index, bool defaultValue) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(index);
    return value.Kind == ScriptParamValueKind::Bool ? value.Data.BoolValue : defaultValue;
}

float BehaviorFrame::PinFloat(const std::string &name, float defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::Float) {
        return value.Data.FloatValue;
    }
    if (value.Kind == ScriptParamValueKind::Int) {
        return static_cast<float>(value.Data.IntValue);
    }
    return defaultValue;
}

float BehaviorFrame::PinFloatIndex(int index, float defaultValue) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::Float) {
        return value.Data.FloatValue;
    }
    if (value.Kind == ScriptParamValueKind::Int) {
        return static_cast<float>(value.Data.IntValue);
    }
    return defaultValue;
}

CKObject *BehaviorFrame::PinObject(const std::string &name, int occurrence) const {
    ParamRef *ref = Pin(name, occurrence);
    if (!ref) {
        return nullptr;
    }
    CKParameter *source = ref->Source();
    CKObject *object = source ? source->GetValueObject(TRUE) : nullptr;
    ref->Release();
    return object;
}

CKObject *BehaviorFrame::PinObjectIndex(int index) const {
    ParamRef *ref = PinIndex(index);
    if (!ref) {
        return nullptr;
    }
    CKParameter *source = ref->Source();
    CKObject *object = source ? source->GetValueObject(TRUE) : nullptr;
    ref->Release();
    return object;
}

std::string BehaviorFrame::LocalString(const std::string &name, const std::string &defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::String || value.Kind == ScriptParamValueKind::Text) {
        return value.Text();
    }
    return defaultValue;
}

std::string BehaviorFrame::LocalStringIndex(int index, const std::string &defaultValue) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::String || value.Kind == ScriptParamValueKind::Text) {
        return value.Text();
    }
    return defaultValue;
}

int BehaviorFrame::LocalInt(const std::string &name, int defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::Int) {
        return value.Data.IntValue;
    }
    if (value.Kind == ScriptParamValueKind::Enum || value.Kind == ScriptParamValueKind::Flags) {
        return static_cast<int>(value.Data.DwordValue);
    }
    return defaultValue;
}

int BehaviorFrame::LocalIntIndex(int index, int defaultValue) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::Int) {
        return value.Data.IntValue;
    }
    if (value.Kind == ScriptParamValueKind::Enum || value.Kind == ScriptParamValueKind::Flags) {
        return static_cast<int>(value.Data.DwordValue);
    }
    return defaultValue;
}

bool BehaviorFrame::LocalBool(const std::string &name, bool defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(name, occurrence);
    return value.Kind == ScriptParamValueKind::Bool ? value.Data.BoolValue : defaultValue;
}

bool BehaviorFrame::LocalBoolIndex(int index, bool defaultValue) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(index);
    return value.Kind == ScriptParamValueKind::Bool ? value.Data.BoolValue : defaultValue;
}

float BehaviorFrame::LocalFloat(const std::string &name, float defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::Float) {
        return value.Data.FloatValue;
    }
    if (value.Kind == ScriptParamValueKind::Int) {
        return static_cast<float>(value.Data.IntValue);
    }
    return defaultValue;
}

float BehaviorFrame::LocalFloatIndex(int index, float defaultValue) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::Float) {
        return value.Data.FloatValue;
    }
    if (value.Kind == ScriptParamValueKind::Int) {
        return static_cast<float>(value.Data.IntValue);
    }
    return defaultValue;
}

CKObject *BehaviorFrame::LocalObject(const std::string &name, int occurrence) const {
    CKParameter *local = FindLocalSource(name, occurrence);
    return local ? local->GetValueObject(TRUE) : nullptr;
}

CKObject *BehaviorFrame::BehaviorLocalObject(const std::string &name, int occurrence) const {
    CKParameterLocal *local = FindLocalParameter(name, occurrence);
    return local ? local->GetValueObject(TRUE) : nullptr;
}

CKObject *BehaviorFrame::BehaviorLocalObjectIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameterObject(index)
        : nullptr;
}

bool BehaviorFrame::PinEnabled(const std::string &name, int occurrence) const {
    const int index = ResolvePin(name, occurrence, "BehaviorFrame.PinEnabled");
    return PinEnabledIndex(index);
}

bool BehaviorFrame::PinEnabledIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetInputParameterCount() && behavior->IsInputParameterEnabled(index);
}

void BehaviorFrame::EnablePin(const std::string &name, bool enable, int occurrence) {
    const int index = ResolvePin(name, occurrence, "BehaviorFrame.EnablePin");
    EnablePinIndex(index, enable);
}

void BehaviorFrame::EnablePinIndex(int index, bool enable) {
    CKBehavior *behavior = Behavior();
    if (behavior && ValidIndex(index, behavior->GetInputParameterCount(), "BehaviorFrame.EnablePin", "pin")) {
        behavior->EnableInputParameter(index, enable);
    }
}

CKERROR BehaviorFrame::ExportPin(CKParameterIn *pin) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.ExportPin")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->ExportInputParameter(pin);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKParameterIn *BehaviorFrame::CreatePin(const std::string &name, CKParameterType type) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CreatePin")) {
        return nullptr;
    }
    CKParameterIn *pin = behavior->CreateInputParameter(const_cast<CKSTRING>(name.c_str()), type);
    if (pin) {
        InvalidateLayout();
    }
    return pin;
}

CKParameterIn *BehaviorFrame::CreatePinGuid(const std::string &name, CKGUID guid) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CreatePin")) {
        return nullptr;
    }
    CKParameterIn *pin = behavior->CreateInputParameter(const_cast<CKSTRING>(name.c_str()), guid);
    if (pin) {
        InvalidateLayout();
    }
    return pin;
}

CKParameterIn *BehaviorFrame::InsertPin(int index, const std::string &name, CKParameterType type) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.InsertPin")) {
        return nullptr;
    }
    CKParameterIn *pin = behavior->InsertInputParameter(index, const_cast<CKSTRING>(name.c_str()), type);
    if (pin) {
        InvalidateLayout();
    }
    return pin;
}

void BehaviorFrame::AddPin(CKParameterIn *pin) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.AddPin")) {
        behavior->AddInputParameter(pin);
        InvalidateLayout();
    }
}

int BehaviorFrame::PinPosition(CKParameterIn *pin) const {
    CKBehavior *behavior = Behavior();
    return behavior && pin ? behavior->GetInputParameterPosition(pin) : -1;
}

CKParameterIn *BehaviorFrame::RemovePin(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemovePin")) {
        return nullptr;
    }
    CKParameterIn *pin = behavior->RemoveInputParameter(index);
    if (pin) {
        InvalidateLayout();
    }
    return pin;
}

CKParameterIn *BehaviorFrame::ReplacePin(int index, CKParameterIn *pin) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.ReplacePin")) {
        return nullptr;
    }
    CKParameterIn *previous = behavior->ReplaceInputParameter(index, pin);
    InvalidateLayout();
    return previous;
}

NativePointer BehaviorFrame::PinReadDataPtr(int index) const {
    CKBehavior *behavior = Behavior();
    return NativePointer(behavior && index >= 0 && index < behavior->GetInputParameterCount()
        ? behavior->GetInputParameterReadDataPtr(index)
        : nullptr);
}

bool BehaviorFrame::PoutEnabled(const std::string &name, int occurrence) const {
    const int index = ResolvePout(name, occurrence, "BehaviorFrame.PoutEnabled");
    return PoutEnabledIndex(index);
}

bool BehaviorFrame::PoutEnabledIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetOutputParameterCount() && behavior->IsOutputParameterEnabled(index);
}

void BehaviorFrame::EnablePout(const std::string &name, bool enable, int occurrence) {
    const int index = ResolvePout(name, occurrence, "BehaviorFrame.EnablePout");
    EnablePoutIndex(index, enable);
}

void BehaviorFrame::EnablePoutIndex(int index, bool enable) {
    CKBehavior *behavior = Behavior();
    if (behavior && ValidIndex(index, behavior->GetOutputParameterCount(), "BehaviorFrame.EnablePout", "pout")) {
        behavior->EnableOutputParameter(index, enable);
    }
}

CKERROR BehaviorFrame::ExportPout(CKParameterOut *pout) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.ExportPout")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->ExportOutputParameter(pout);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKParameterOut *BehaviorFrame::CreatePout(const std::string &name, CKParameterType type) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CreatePout")) {
        return nullptr;
    }
    CKParameterOut *pout = behavior->CreateOutputParameter(const_cast<CKSTRING>(name.c_str()), type);
    if (pout) {
        InvalidateLayout();
    }
    return pout;
}

CKParameterOut *BehaviorFrame::CreatePoutGuid(const std::string &name, CKGUID guid) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CreatePout")) {
        return nullptr;
    }
    CKParameterOut *pout = behavior->CreateOutputParameter(const_cast<CKSTRING>(name.c_str()), guid);
    if (pout) {
        InvalidateLayout();
    }
    return pout;
}

CKParameterOut *BehaviorFrame::InsertPout(int index, const std::string &name, CKParameterType type) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.InsertPout")) {
        return nullptr;
    }
    CKParameterOut *pout = behavior->InsertOutputParameter(index, const_cast<CKSTRING>(name.c_str()), type);
    if (pout) {
        InvalidateLayout();
    }
    return pout;
}

void BehaviorFrame::AddPout(CKParameterOut *pout) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.AddPout")) {
        behavior->AddOutputParameter(pout);
        InvalidateLayout();
    }
}

int BehaviorFrame::PoutPosition(CKParameterOut *pout) const {
    CKBehavior *behavior = Behavior();
    return behavior && pout ? behavior->GetOutputParameterPosition(pout) : -1;
}

CKParameterOut *BehaviorFrame::RemovePout(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemovePout")) {
        return nullptr;
    }
    CKParameterOut *pout = behavior->RemoveOutputParameter(index);
    if (pout) {
        InvalidateLayout();
    }
    return pout;
}

CKParameterOut *BehaviorFrame::ReplacePout(int index, CKParameterOut *pout) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.ReplacePout")) {
        return nullptr;
    }
    CKParameterOut *previous = behavior->ReplaceOutputParameter(index, pout);
    InvalidateLayout();
    return previous;
}

NativePointer BehaviorFrame::PoutWriteDataPtr(int index) const {
    CKBehavior *behavior = Behavior();
    return NativePointer(behavior && index >= 0 && index < behavior->GetOutputParameterCount()
        ? behavior->GetOutputParameterWriteDataPtr(index)
        : nullptr);
}

CKObject *BehaviorFrame::PoutObject(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetOutputParameterCount()
        ? behavior->GetOutputParameterObject(index)
        : nullptr;
}

CKERROR BehaviorFrame::SetOutputParameterObject(int index, CKObject *object) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->SetOutputParameterObject(index, object) : CKERR_INVALIDOBJECT;
}

bool BehaviorFrame::SetPoutInt(const std::string &name, int value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [value](ParamRef *ref) { return ref->SetInt(value); });
}

bool BehaviorFrame::SetPoutIntIndex(int index, int value) {
    return ApplyParamRef(PoutIndex(index), [value](ParamRef *ref) { return ref->SetInt(value); });
}

bool BehaviorFrame::SetPoutFloat(const std::string &name, float value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [value](ParamRef *ref) { return ref->SetFloat(value); });
}

bool BehaviorFrame::SetPoutFloatIndex(int index, float value) {
    return ApplyParamRef(PoutIndex(index), [value](ParamRef *ref) { return ref->SetFloat(value); });
}

bool BehaviorFrame::SetPoutBool(const std::string &name, bool value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [value](ParamRef *ref) { return ref->SetBool(value); });
}

bool BehaviorFrame::SetPoutBoolIndex(int index, bool value) {
    return ApplyParamRef(PoutIndex(index), [value](ParamRef *ref) { return ref->SetBool(value); });
}

bool BehaviorFrame::SetPoutString(const std::string &name, const std::string &value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [&value](ParamRef *ref) { return ref->SetString(value); });
}

bool BehaviorFrame::SetPoutStringIndex(int index, const std::string &value) {
    return ApplyParamRef(PoutIndex(index), [&value](ParamRef *ref) { return ref->SetString(value); });
}

bool BehaviorFrame::SetPoutObject(const std::string &name, CKObject *value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [value](ParamRef *ref) { return ref->SetObject(value); });
}

bool BehaviorFrame::SetPoutObjectIndex(int index, CKObject *value) {
    return ApplyParamRef(PoutIndex(index), [value](ParamRef *ref) { return ref->SetObject(value); });
}

bool BehaviorFrame::SetLocalInt(const std::string &name, int value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [value](ParamRef *ref) { return ref->SetInt(value); });
}

bool BehaviorFrame::SetLocalIntIndex(int index, int value) {
    return ApplyParamRef(LocalIndex(index), [value](ParamRef *ref) { return ref->SetInt(value); });
}

bool BehaviorFrame::SetLocalFloat(const std::string &name, float value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [value](ParamRef *ref) { return ref->SetFloat(value); });
}

bool BehaviorFrame::SetLocalFloatIndex(int index, float value) {
    return ApplyParamRef(LocalIndex(index), [value](ParamRef *ref) { return ref->SetFloat(value); });
}

bool BehaviorFrame::SetLocalBool(const std::string &name, bool value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [value](ParamRef *ref) { return ref->SetBool(value); });
}

bool BehaviorFrame::SetLocalBoolIndex(int index, bool value) {
    return ApplyParamRef(LocalIndex(index), [value](ParamRef *ref) { return ref->SetBool(value); });
}

bool BehaviorFrame::SetLocalString(const std::string &name, const std::string &value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [&value](ParamRef *ref) { return ref->SetString(value); });
}

bool BehaviorFrame::SetLocalStringIndex(int index, const std::string &value) {
    return ApplyParamRef(LocalIndex(index), [&value](ParamRef *ref) { return ref->SetString(value); });
}

bool BehaviorFrame::SetLocalObject(const std::string &name, CKObject *value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [value](ParamRef *ref) { return ref->SetObject(value); });
}

bool BehaviorFrame::SetLocalObjectIndex(int index, CKObject *value) {
    return ApplyParamRef(LocalIndex(index), [value](ParamRef *ref) { return ref->SetObject(value); });
}

CKParameterLocal *BehaviorFrame::CreateLocal(const std::string &name, CKParameterType type) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CreateLocal")) {
        return nullptr;
    }
    CKParameterLocal *local = behavior->CreateLocalParameter(const_cast<CKSTRING>(name.c_str()), type);
    if (local) {
        InvalidateLayout();
    }
    return local;
}

CKParameterLocal *BehaviorFrame::CreateLocalGuid(const std::string &name, CKGUID guid) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.CreateLocal")) {
        return nullptr;
    }
    CKParameterLocal *local = behavior->CreateLocalParameter(const_cast<CKSTRING>(name.c_str()), guid);
    if (local) {
        InvalidateLayout();
    }
    return local;
}

CKParameterLocal *BehaviorFrame::RemoveLocal(int index) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.RemoveLocal")) {
        return nullptr;
    }
    CKParameterLocal *local = behavior->RemoveLocalParameter(index);
    if (local) {
        InvalidateLayout();
    }
    return local;
}

void BehaviorFrame::AddLocal(CKParameterLocal *local) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.AddLocal")) {
        behavior->AddLocalParameter(local);
        InvalidateLayout();
    }
}

int BehaviorFrame::LocalPosition(CKParameterLocal *local) const {
    CKBehavior *behavior = Behavior();
    return behavior && local ? behavior->GetLocalParameterPosition(local) : -1;
}

NativePointer BehaviorFrame::LocalWriteDataPtr(int index) const {
    CKParameter *local = FindLocalSourceIndex(index);
    return NativePointer(local ? local->GetWriteDataPtr() : nullptr);
}

NativePointer BehaviorFrame::LocalReadDataPtr(int index) const {
    CKParameter *local = FindLocalSourceIndex(index);
    return NativePointer(local ? local->GetReadDataPtr(TRUE) : nullptr);
}

CKObject *BehaviorFrame::LocalObject(int index) const {
    CKParameter *local = FindLocalSourceIndex(index);
    return local ? local->GetValueObject(TRUE) : nullptr;
}

NativePointer BehaviorFrame::BehaviorLocalWriteDataPtr(int index) const {
    CKBehavior *behavior = Behavior();
    return NativePointer(behavior && index >= 0 && index < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameterWriteDataPtr(index)
        : nullptr);
}

NativePointer BehaviorFrame::BehaviorLocalReadDataPtr(int index) const {
    CKBehavior *behavior = Behavior();
    return NativePointer(behavior && index >= 0 && index < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameterReadDataPtr(index)
        : nullptr);
}

CKParameterLocal *BehaviorFrame::BehaviorLocalParameter(int index) const {
    return LocalParameter(index);
}

CKERROR BehaviorFrame::SetLocalObject(int index, CKObject *object) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->SetLocalParameterObject(index, object) : CKERR_INVALIDOBJECT;
}

bool BehaviorFrame::SetSettingInt(const std::string &name, int value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamInt(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingIntIndex(int index, int value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamInt(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingFloat(const std::string &name, float value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamFloat(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingFloatIndex(int index, float value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamFloat(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingBool(const std::string &name, bool value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamBool(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingBoolIndex(int index, bool value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamBool(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingString(const std::string &name, const std::string &value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamString(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingStringIndex(int index, const std::string &value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamString(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingObject(const std::string &name, CKObject *value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamObject(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingObjectIndex(int index, CKObject *value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamObject(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::IsSetting(int localIndex) const {
    CKBehavior *behavior = Behavior();
    return behavior && localIndex >= 0 && localIndex < behavior->GetLocalParameterCount() && behavior->IsLocalParameterSetting(localIndex);
}

CKERROR BehaviorFrame::SetOwner(CKBeObject *owner, bool callback) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.SetOwner")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->SetOwner(owner, callback);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

CKERROR BehaviorFrame::SetSubBehaviorOwner(CKBeObject *owner, bool callback) {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, "BehaviorFrame.SetSubBehaviorOwner")) {
        return CKERR_INVALIDOBJECT;
    }
    CKERROR err = behavior->SetSubBehaviorOwner(owner, callback);
    if (err == CK_OK) {
        InvalidateLayout();
    }
    return err;
}

void BehaviorFrame::NotifyEdition() {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.NotifyEdition")) {
        behavior->NotifyEdition();
        InvalidateLayout();
    }
}

void BehaviorFrame::NotifySettingsEdition() {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.NotifySettingsEdition")) {
        behavior->NotifySettingsEdition();
        InvalidateLayout();
    }
}

CKStateChunk *BehaviorFrame::InterfaceChunk() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetInterfaceChunk() : nullptr;
}

void BehaviorFrame::SetInterfaceChunk(CKStateChunk *state) {
    if (CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SetInterfaceChunk")) {
        behavior->SetInterfaceChunk(state);
        InvalidateLayout();
    }
}

BBCallBuilder *BehaviorFrame::CallName(const std::string &prototype) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame.Call requires ScriptBehaviorBridge.");
        return nullptr;
    }
    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(MakeContext());
    request.PrototypeKind = ScriptBridgePrototypeKind::Name;
    request.PrototypeName = prototype;
    return new BBCallBuilder(bridge, MakeContext(), request);
}

BBCallBuilder *BehaviorFrame::CallGuid(CKGUID guid) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame.Call requires ScriptBehaviorBridge.");
        return nullptr;
    }
    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(MakeContext());
    request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
    request.Guid = guid;
    return new BBCallBuilder(bridge, MakeContext(), request);
}

BBBridge *BehaviorFrame::BB() const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame.BB requires ScriptBehaviorBridge.");
        return nullptr;
    }
    return new BBBridge(bridge, MakeContext());
}

ScriptAsyncTaskBase *BehaviorFrame::LinkDelay(int frames) const {
    if (frames < 0) {
        SetScriptException(fmt::format("BehaviorFrame.LinkDelay requires a non-negative frame count, got {}.", frames));
        return nullptr;
    }
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.LinkDelay");
    return scheduler ? scheduler->CreateDelay(frames) : nullptr;
}

ScriptAsyncTaskBase *BehaviorFrame::WaitBBTask(BBTask *task, int inputIndex) const {
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.Wait");
    if (!scheduler) {
        return nullptr;
    }
    CKBehaviorContext ctx = MakeContext();
    return scheduler->CreateBBTask(task, &ctx, inputIndex);
}

ScriptAsyncTaskBase *BehaviorFrame::WaitGraphTask(GraphTask *task) const {
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.Wait");
    if (!scheduler) {
        return nullptr;
    }
    CKBehaviorContext ctx = MakeContext();
    return scheduler->CreateGraphTask(task, &ctx);
}

ScriptAsyncTaskBase *BehaviorFrame::WaitMessageName(const std::string &message, CKBeObject *object) const {
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.WaitMessage");
    if (!scheduler) {
        return nullptr;
    }
    CKBeObject *waitObject = ResolveMessageWaitObject(Behavior(), object);
    if (!waitObject) {
        SetScriptException("BehaviorFrame.WaitMessage requires a non-null object or behavior target/owner.");
        return nullptr;
    }
    std::string error;
    ScriptAsyncTaskBase *task = scheduler->CreateMessageWait(m_Context, message, waitObject, &error);
    if (!task) {
        SetScriptException(error.empty() ? "BehaviorFrame.WaitMessage failed to create a message wait task." : error);
    }
    return task;
}

ScriptAsyncTaskBase *BehaviorFrame::WaitMessageType(CKMessageType messageType, CKBeObject *object) const {
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.WaitMessage");
    if (!scheduler) {
        return nullptr;
    }
    CKBeObject *waitObject = ResolveMessageWaitObject(Behavior(), object);
    if (!waitObject) {
        SetScriptException("BehaviorFrame.WaitMessage requires a non-null object or behavior target/owner.");
        return nullptr;
    }
    std::string error;
    ScriptAsyncTaskBase *task = scheduler->CreateMessageWait(m_Context, messageType, waitObject, &error);
    if (!task) {
        SetScriptException(error.empty() ? "BehaviorFrame.WaitMessage failed to create a message wait task." : error);
    }
    return task;
}

void BehaviorFrame::Trace(const std::string &message) const {
    CKContext *context = Context();
    if (!context) {
        return;
    }
    CKBehavior *behavior = Behavior();
    const std::string prefix = behavior
        ? fmt::format("[CKAS:{}#{}] ", SafeString(behavior->GetName()), behavior->GetID())
        : "[CKAS:<behavior unavailable>] ";
    const std::string line = prefix + message;
    context->OutputToConsoleEx(const_cast<char *>("%s"), line.c_str());
}

CKBehaviorContext BehaviorFrame::MakeContext() const {
    CKBehaviorContext ctx = m_ContextSnapshot;
    ctx.Context = m_Context;
    ctx.Behavior = Behavior();
    ctx.CurrentLevel = CurrentLevel();
    ctx.CurrentScene = CurrentScene();
    ctx.CurrentRenderContext = CurrentRenderContext();
    ctx.ParameterManager = ParameterManager();
    ctx.MessageManager = MessageManager();
    ctx.AttributeManager = AttributeManager();
    ctx.TimeManager = TimeManager();
    return ctx;
}

ScriptBehaviorBridge *BehaviorFrame::Bridge() const {
    const CKBehaviorContext context = MakeContext();
    if (!context.Behavior) {
        return nullptr;
    }
    ScriptManager *manager = ManagerFromContext(context);
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

void BehaviorFrame::InvalidateLayout() const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0) {
        bridge->InvalidateBehaviorLayout(m_BehaviorId);
    }
}

BehaviorRef *BehaviorFrame::WrapBehavior(CKBehavior *behavior) const {
    ScriptBehaviorBridge *bridge = Bridge();
    return bridge && behavior ? bridge->WrapBehavior(behavior, m_BehaviorId) : nullptr;
}

ParamRef *BehaviorFrame::WrapParameter(CKObject *parameter, ScriptBridgeSlotKind kind, int index) const {
    if (!parameter) {
        return nullptr;
    }
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame parameter access requires ScriptBehaviorBridge.");
        return nullptr;
    }
    CKBehavior *behavior = Behavior();
    return new ParamRef(bridge,
                        parameter->GetID(),
                        kind,
                        index,
                        behavior ? behavior->GetID() : 0,
                        parameter->GetCKContext());
}

bool BehaviorFrame::SetSettingValue(int localIndex, const ScriptParamValue &value, const char *method) {
    if (localIndex < 0) {
        return false;
    }

    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return false;
    }
    if (localIndex >= behavior->GetLocalParameterCount() || !behavior->IsLocalParameterSetting(localIndex)) {
        SetScriptException(fmt::format("{} could not find setting local index {} on behavior '{}' (candidates: {}).",
            method ? method : "BehaviorFrame.SetSetting",
            localIndex,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Setting)));
        return false;
    }

    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException(fmt::format("{} requires ScriptBehaviorBridge.", method ? method : "BehaviorFrame.SetSetting"));
        return false;
    }

    std::string error;
    CKBehaviorContext ctx = MakeContext();
    if (!bridge->SetBehaviorSetting(behavior, localIndex, value, ctx, error)) {
        SetScriptException(error.empty()
            ? fmt::format("{} failed to set setting local index {}.", method ? method : "BehaviorFrame.SetSetting", localIndex)
            : error);
        return false;
    }
    return true;
}

int BehaviorFrame::ResolveInput(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedIo(behavior, name, occurrence, true);
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find input '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeIoNames(behavior, true)));
    }
    return index;
}

int BehaviorFrame::ResolveOutput(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedIo(behavior, name, occurrence, false);
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find output '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeIoNames(behavior, false)));
    }
    return index;
}

int BehaviorFrame::ResolvePin(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            behavior->GetInputParameterCount(),
                                            [behavior](int i) { return behavior->GetInputParameter(i); });
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find pin '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Pin)));
    }
    return index;
}

int BehaviorFrame::ResolvePout(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            behavior->GetOutputParameterCount(),
                                            [behavior](int i) { return behavior->GetOutputParameter(i); });
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find pout '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Pout)));
    }
    return index;
}

int BehaviorFrame::ResolveLocal(const std::string &name, int occurrence, const char *method) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        const int index = bridge->FindFrameLocal(m_BehaviorId, name, occurrence);
        if (index < 0) {
            CKBehavior *behavior = Behavior();
            SetScriptException(fmt::format("{} could not find logical local '{}' occurrence {} on behavior '{}' (candidates: {}).",
                method,
                name,
                occurrence,
                SafeString(behavior ? behavior->GetName() : nullptr),
                bridge->DescribeFrameLocals(m_BehaviorId)));
        }
        return index;
    }
    return ResolveBehaviorLocal(name, occurrence, method);
}

int BehaviorFrame::ResolveBehaviorLocal(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            behavior->GetLocalParameterCount(),
                                            [behavior](int i) { return behavior->GetLocalParameter(i); });
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find local '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Local)));
    }
    return index;
}

int BehaviorFrame::ResolveSetting(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int localIndex = FindSettingIndex(behavior, name, occurrence);
    if (localIndex < 0) {
        SetScriptException(fmt::format("{} could not find setting '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Setting)));
    }
    return localIndex;
}

int BehaviorFrame::ResolveSettingIndex(int index, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    if (index < 0 ||
        index >= behavior->GetLocalParameterCount() ||
        !behavior->IsLocalParameterSetting(index)) {
        SetScriptException(fmt::format("{} could not find setting local index {} on behavior '{}' (candidates: {}).",
            method,
            index,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Setting)));
        return -1;
    }
    return index;
}

CKParameterIn *BehaviorFrame::FindPinParameter(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    if (!behavior) {
        return nullptr;
    }
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            behavior->GetInputParameterCount(),
                                            [behavior](int i) { return behavior->GetInputParameter(i); });
    return index >= 0 && index < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(index)
        : nullptr;
}

CKParameterIn *BehaviorFrame::FindPinParameterIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(index)
        : nullptr;
}

CKParameterLocal *BehaviorFrame::FindLocalParameter(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int count = behavior ? behavior->GetLocalParameterCount() : 0;
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            count,
                                            [behavior](int i) { return behavior->GetLocalParameter(i); });
    return index >= 0 && index < count
        ? behavior->GetLocalParameter(index)
        : nullptr;
}

CKParameterLocal *BehaviorFrame::FindLocalParameterIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameter(index)
        : nullptr;
}

CKParameter *BehaviorFrame::FindLocalSource(const std::string &name, int occurrence) const {
    const int index = FindLocal(name, occurrence);
    return FindLocalSourceIndex(index);
}

CKParameter *BehaviorFrame::FindLocalSourceIndex(int index) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        std::string error;
        return bridge->ResolveFrameLocal(m_Context, m_BehaviorId, index, error);
    }
    return FindLocalParameterIndex(index);
}

ScriptParamValue BehaviorFrame::ReadPinValueOrEmpty(const std::string &name, int occurrence) const {
    CKParameterIn *pin = FindPinParameter(name, occurrence);
    CKParameter *source = pin ? ParameterSourceForConnection(pin) : nullptr;
    if (!source) {
        return ScriptParamValue();
    }
    std::string ignoredError;
    ScriptParamValue value = ReadParameterValue(source, &ignoredError);
    if (value.Kind == ScriptParamValueKind::Empty) {
        return value;
    }
    value.TypeGuid = source->GetGUID();
    value.Type = source->GetType();
    return value;
}

ScriptParamValue BehaviorFrame::ReadLocalValueOrEmpty(const std::string &name, int occurrence) const {
    CKParameter *local = FindLocalSource(name, occurrence);
    if (!local) {
        return ScriptParamValue();
    }
    std::string ignoredError;
    ScriptParamValue value = ReadParameterValue(local, &ignoredError);
    if (value.Kind == ScriptParamValueKind::Empty) {
        return value;
    }
    value.TypeGuid = local->GetGUID();
    value.Type = local->GetType();
    return value;
}

ScriptParamValue BehaviorFrame::ReadLocalValueOrEmpty(int index) const {
    CKParameter *local = FindLocalSourceIndex(index);
    if (!local) {
        return ScriptParamValue();
    }
    std::string ignoredError;
    ScriptParamValue value = ReadParameterValue(local, &ignoredError);
    if (value.Kind == ScriptParamValueKind::Empty) {
        return value;
    }
    value.TypeGuid = local->GetGUID();
    value.Type = local->GetType();
    return value;
}

ScriptParamValue BehaviorFrame::ReadPinValueOrEmpty(int index) const {
    CKParameterIn *pin = FindPinParameterIndex(index);
    CKParameter *source = pin ? ParameterSourceForConnection(pin) : nullptr;
    if (!source) {
        return ScriptParamValue();
    }
    std::string ignoredError;
    ScriptParamValue value = ReadParameterValue(source, &ignoredError);
    if (value.Kind == ScriptParamValueKind::Empty) {
        return value;
    }
    value.TypeGuid = source->GetGUID();
    value.Type = source->GetType();
    return value;
}
