# AngelScript Component Metadata and Manifest

## Goal

`AngelScript Component` should be configured like a normal Virtools component, not by hand-written parameter lookup boilerplate. A script class can now declare editor-facing input parameters either in AngelScript metadata or in the Component `Manifest` parameter. The native BB creates those input parameters and injects their values into script object fields before lifecycle code runs.

## Component Inputs

Fixed inputs remain first:

1. `Script`
2. `Class`
3. `Source`
4. `File`
5. `Manifest`

Declared component fields create additional input parameters after these fixed inputs. Declared names must not conflict with fixed inputs.

## Metadata Syntax

Metadata is attached to public writable class fields:

```angelscript
class DoorComponent {
    [param type="float" default="1.0"]
    float Speed;

    [param name="Target" type="3dentity"]
    CK3dEntity@ Target;

    [behavior param="Open Graph"]
    BehaviorRef@ OpenGraph;

    [bb param="Delay BB" default="Logics/Time/Timer"]
    BBPrototype@ DelayBB;

    [param param="Text BB" type="bbdecl" default="Interface/Text/2D Text"]
    BBDecl@ TextBB;

    [bbconfig prototype="Interface/Text/2D Text"]
    [bbsetting "Text Properties"="Screen Proportionnal,WordWrap"]
    [bbpin "Text"="Ready"]
    BBConfig@ TextConfig;

    [bbslot from="TextConfig" pin="Text"]
    BBSlot@ TextPin;

    [bb param="Existing Delay" type="behavior"]
    BBPrototype@ ExistingDelayPrototype;

    [param type="Render Options"]
    ParamTypeInfo@ RenderOptionsType;

    [param type="color" default="#ffcc00ff"]
    VxColor Tint;

    [param type="guid" default="guid:0x4b67c5f2,0x6f5f5c90"]
    CKGUID EventId;

    [param type="Render Options" default="Clear ZBuffer,Buffer Swapping"]
    uint RenderOptions;

    void Update(const CKBehaviorContext &in ctx) {
        if (OpenGraph !is null) {
            OpenGraph.Trigger("In");
        }
    }
}
```

Recognized metadata keywords:

- `param`, `input`, `field`, `property`, `CKInput`, `CKParam`, `CKComponentInput`
- `behavior` for `BehaviorRef@`
- `bb` for `BBPrototype@`
- `bbdecl` for `BBDecl@`
- `bbslot` for `BBSlot@`
- `bbconfig` for `BBConfig@`
- `bbpin`, `bbsetting`, `bbsource`, `bbinput`, `bboutput`, `bbpout`, `bblocal` as short `BBConfig@` fragments

Recognized keys:

- `field`, `member`, `property`: script field name; normally inferred from the annotated field
- `name`, `param`, `parameter`: Virtools input parameter name
- `type`, `kind`: parameter/editor type
- `default`, `value`: default local source value; for `BBSlot@`, `default` applies pin defaults and `value` applies setting values
- `update`, `sync`: whether scalar/object values are refreshed before each lifecycle/update call
- `prototype`, `proto`, `bbname`, `bbquery`: prototype query for `BBDecl@`, `BBConfig@`, or standalone `BBSlot@`
- `from`, `config`, `bbconfig`: owning `BBConfig@` field name for `BBSlot@`; this is the recommended style
- `slot`, `slotKind`: one of `input`, `output`, `pin`, `pout`, `setting`, or `local`
- `slotName`, `input`, `output`, `pin`, `pout`, `setting`, `local`: slot selector for `BBSlot@`
- `occurrence`: duplicate-name occurrence for `BBSlot@`
- `lifetime`: for `BBConfig@`; `component` (default) lets the Component stop the latest owned `BBInstance@` on disable/deactivate/pause and destroy it on reset/delete/script rebuild. `manual` leaves cleanup to the script. `managed=true/false` is rejected; use `lifetime` explicitly.
- `owner`, `target`: object source for `BBConfig@`; accepted values include a public object field name, `$owner`, `$target`, and `$level`
- `start`, `stop`: for `BBSlot@` input fields, mark the config's default start/stop input; for `BBConfig@`, set default input names directly
- `required`: comma/semicolon/pipe separated required slots for `BBConfig@`, such as `in:On,pin:Text,pout:Font Created,setting:Text Properties`
- `inputs`, `outputs`, `pins`, `pouts`, `locals`, `requiredSettings`: shorthand required slot lists for `BBConfig@`; `pins` and `settings` also accept semicolon-separated `Name='value'` entries
- `sources`: semicolon-separated `Pin<-Field.Pout` or `Pin<-ParamRefField` source wiring for `BBConfig@`; the right side resolves during autostart/auto-step, so source configs declared earlier can create their instances first
- `autostart`: when true, the Component calls `EnsureStarted(ctx)` before the first `Update`
- `step`: `manual` (default), `eachUpdate`, or `onChange`

AngelScript metadata blocks can span multiple source lines, and multiple metadata blocks attached to the same field are merged. Prefer short stacked fragments for readable BB configs:

```angelscript
[bbconfig prototype="Interface/Text/2D Text" target="Target"]
[bbsetting "Text Properties"="Screen Proportionnal,WordWrap"]
[bbpin "Text"="Ready"]
[bbsource "Font"="FontConfig.Font Created"]
[bboutput "Out"]
BBConfig@ Text;
```

The long aggregate form remains valid for generated code or compact scripts:

```angelscript
[bbconfig prototype="Interface/Text/2D Text" pins="Text='Ready'" settings="Text Properties='Screen Proportionnal,WordWrap'" sources="Font<-FontConfig.Font Created"]
BBConfig@ Text;
```

## Manifest Syntax

`Manifest` is useful when the script module is shared or when a designer wants to override metadata without editing the script. One declaration per line:

```text
param field=Speed type=float name="Speed" default=1.0
param field=Target type=3dentity name="Target"
behavior field=OpenGraph param="Open Graph"
bb field=DelayBB param="Delay BB" default="Logics/Time/Timer"
param field=TextBB type=bbdecl param="Text BB" default="Interface/Text/2D Text"
bbconfig field=TextConfig prototype="Interface/Text/2D Text" settings="Text Properties='Screen Proportionnal,WordWrap'" pins="Text='Ready'" lifetime=component
bbslot field=TextOn from=TextConfig input=On start
bbslot field=TextPin from=TextConfig pin=Text
bb field=ExistingDelayPrototype type=behavior param="Existing Delay"
```

Short fragment form is also accepted and is preferred for readable BB configs:

```text
bbconfig field=TextConfig prototype="Interface/Text/2D Text" lifetime=component
bbsetting field=TextConfig "Text Properties"="Screen Proportionnal,WordWrap"
bbpin field=TextConfig "Text"="Ready"
bbsource field=TextConfig "Font"="FontConfig.Font Created"
bbslot field=TextPin from=TextConfig pin=Text
```

```text
Speed float "Speed" = 1.0
Target 3dentity "Target"
DelayBB bb "Delay BB" = "Logics/Time/Timer"
ExistingDelayPrototype behavior "Existing Delay"
```

Manifest declarations override metadata declarations by field name.

## Supported Field Types

Current supported field types:

- `int`, `uint`, `CK_ID`
- `float`
- `bool`
- `string`
- `CKGUID` through a string editor parameter; accepted text forms include `guid:0x...,0x...`, `CKGUID(...)`, `{...,...}`, and `...,...`
- `VxVector`
- `Vx2DVector`
- `VxColor`
- `VxQuaternion`
- `VxMatrix`
- `CKObject@` and common subclasses such as `CKBeObject@`, `CK3dEntity@`, `CKBehavior@`
- `XObjectArray`
- `BehaviorRef@`
- `ParamTypeInfo@`
- `BBPrototype@`
- `BBDecl@`
- `BBSlot@`
- `BBConfig@`

Value type aliases include `guid`, `vector`, `vector2`, `2dvector`, `color`, `quat`, `quaternion`, and `matrix`. Their default Virtools parameter GUIDs are `CKPGUID_STRING` for `CKGUID`, `CKPGUID_VECTOR`, `CKPGUID_2DVECTOR`, `CKPGUID_COLOR`, `CKPGUID_QUATERNION`, and `CKPGUID_MATRIX`.

Numeric defaults accept comma, semicolon, pipe, or whitespace separators. `VxColor` accepts float channels (`1,0,0,1`), byte-style channels (`255,0,0,255`), and `#RRGGBB` / `#RRGGBBAA`. `VxMatrix` accepts `identity` or 16 numeric values in row-major order.

These value conversions are implemented in the shared registry/codec layer, so Component injection, `BehaviorRef`, `BBConfig`, `BBInstance`, and low-level `BB.Call` / `BB.Spawn` parameter IO use the same CK parameter read/write rules.

When a declaration uses a real Virtools parameter type name, the Component resolves it through `CKParameterManager` before falling back to built-in aliases. This supports SDK/game/plugin registered enum and flags types without maintaining hard-coded GUID lists. Script fields still use `int` / `uint`; numeric defaults are accepted, enum/flags text defaults are passed to the SDK string parser for that parameter type, and the bridge exposes the SDK display text lazily through `ParamRef.GetText()`.

`message` / `attribute` and other SDK string-backed DWORD parameter types use the current `CKParameterTypeDesc` conversion path. If a type exposes a `StringFunction`, enum table, or flags table, defaults and string overrides are delegated to the SDK instead of a CKAngelScript hard-coded list.

`XObjectArray` maps to `CKPGUID_OBJECTARRAY`. The bridge supports minimal read/write as object IDs through `Param::ObjectArray(...)`, `ParamRef.Set(...)`, and lazy `ParamRef.Get()`.

Object metadata can use editor type aliases such as `object`, `behavior`, `3dentity`, `2dentity`, `camera`, `light`, `material`, `texture`, `mesh`, `group`, and `dataarray`.

`BehaviorRef@` input parameters use `CKPGUID_BEHAVIOR`.

`BBPrototype@` defaults to a string input containing a prototype name, `Category/Name`, or `guid:0x...,0x...`. If the declaration uses `type="behavior"` / `type=behavior`, the input parameter uses `CKPGUID_BEHAVIOR` instead; the injected `BBPrototype@` is built from the referenced behavior's prototype GUID.

`BBDecl@` uses the same string/GUID/behavior source rules as `BBPrototype@`, but exposes real prototype metadata and the v3 layout API. `BBSlot@` binds a setup-time slot from a `BBConfig@` through `from="ConfigField"` or from a standalone prototype query. Prefer `from` because the slot can register `start`, `stop`, `default`, and `value` metadata with the owning config automatically.

`BBConfig@` is the v3 runtime Building Block configuration facade. It resolves the prototype during injection, receives declarative metadata, stores pending pin/source/operation bindings, and creates a `BBInstance@` with `Spawn(ctx)`, `SpawnStarted(ctx)`, `EnsureSpawned(ctx)`, or `EnsureStarted(ctx)`. Parameters not mentioned by metadata or `BBSlot@` keep the SDK/prototype defaults.

```angelscript
class HudText {
    CK2dEntity@ Target;

    [bbconfig prototype="Interface/Text/2D Text" target="Target" lifetime="component"]
    [bbsetting "Text Properties"="Screen Proportionnal,WordWrap"]
    [bbpin "Text"="Ready"]
    BBConfig@ Text;

    BBInstance@ TextInstance;

    [bbslot from="Text" pin="Text"]
    BBSlot@ TextText;

    void Start(const CKBehaviorContext &in ctx) {
        @TextInstance = Text.EnsureStarted(ctx);
    }

    void Update(const CKBehaviorContext &in ctx) {
        TextInstance.StepSet(ctx, TextText, "Running");
    }
}
```

`BBConfig@` uses `lifetime="component"` by default. This does not autostart the BB unless `autostart=true` is declared. Component lifetime stops instances on disable/deactivate/pause and destroys them on reset/delete or script object rebuild. `Stop()` leaves the runtime behavior alive so it can be restarted; `Destroy()` releases the runtime behavior and owned source/operation links. If a `BBSlot@` marks an input with `start`, `BBInstance.Start()` uses it automatically. If a `BBSlot@` marks an input with `stop`, `BBInstance.Stop()` pulses it before deactivating the behavior. If `prototype` is omitted, the config uses the generated Component input parameter value, with the same string/GUID/behavior source rules as `BBDecl@`.

`autostart=true` calls `EnsureStarted(ctx)` before the first script `Update`. `step="eachUpdate"` steps the live instance after every script `Update`; `step="onChange"` steps only after the Component reinjects changed metadata or parameter-derived config. Manual script calls to `BBConfig.Set(...)` or `BBInstance.Set(...)` should still step explicitly.

`ParamTypeInfo@` injects the runtime CK parameter type metadata for the connected input source. It is useful when a component accepts plugin-defined or enum/flags parameters and wants to expose `Name()`, `Guid()`, `Describe()`, `Enum()`, `Flags()`, or `Struct()` without guessing the data layout.

## Enum And Flags Helpers

Scripts can resolve enum and flags values through the runtime `CKParameterManager` instead of hard-coding integers:

```angelscript
class RenderComponent {
    [param type="Render Options" default="Clear ZBuffer,Buffer Swapping"]
    uint RenderOptions;

    void Update(const CKBehaviorContext &in ctx) {
        ParamTypeInfo@ info = Param::Find(ctx, "Render Options");
        uint clearAndSwap = Param::FlagsMask(ctx, "Render Options", "Clear ZBuffer,Buffer Swapping");
        int modulate = Param::Value(ctx, "Texture Blend Mode", "Modulate");
        string text = Param::Text(ctx, "Render Options", clearAndSwap);
    }
}
```

Available helpers:

- `Param::Count(ctx)`, `Param::At(ctx, index)`, and `Param::Find(ctx, query, occurrence = 0)`
- `Param::Guid(ctx, typeName)` and `Param::Type(ctx, typeName)`
- `Param::IsEnum(ctx, typeName)` and `Param::IsFlags(ctx, typeName)`
- `Param::Value(ctx, typeName, valueName, fallback = 0)`
- `Param::Flag(ctx, typeName, flagName, fallback = 0)`
- `Param::FlagsMask(ctx, typeName, "Flag A,Flag B", fallback = 0)`
- `Param::Text(ctx, typeName, value)`
- `Param::Describe(ctx, typeName)`

All helpers also accept `CKContext@` in place of `CKBehaviorContext`. The type argument can be a type name or a parameter type `CKGUID`.

## Injection Rules

- Metadata is read from `CachedScript`, so it works for Loader-managed shared modules and for private `Source`/`File` modules.
- The Component creates missing input parameters and default local sources on first script object creation.
- On script identity or manifest change, the old script object is released, Component-created input parameters no longer declared are removed, and the object is rebuilt.
- Field injection runs before `OnLoad` and `Awake`.
- Scalar, string, math value, CK object, `BehaviorRef@`, `ParamTypeInfo@`, `BBPrototype@`, `BBDecl@`, `BBSlot@`, and `BBConfig@` fields are refreshed before later lifecycle/update calls unless `update=false` / `sync=false` is declared.
- Ref-counted handle fields are replaced through bridge-owned release helpers, so changing a behavior or BB prototype parameter no longer requires a full script object rebuild.
- CK object injection validates the actual object against the script field type before writing the handle, so a `CKMaterial@` parameter cannot silently be written into a `CK3dEntity@` field.

## Errors

Configuration errors activate the Component `Error` output:

- reserved parameter name
- duplicate declared parameter name
- missing script field
- private/protected/const field
- unsupported field type
- field type and declared type mismatch
- failure to create Virtools input/default parameters
- failure to read or inject parameter values
- object value incompatible with the declared script field type

Diagnostics include the script field name, declared manifest/metadata type, generated CK parameter type, actual AngelScript field type, and relevant source value when available. Missing-field errors also list writable public field candidates. `BBPrototype@` / `BBDecl@` failures report the unresolved BB name/GUID/behavior source, `BBSlot@` failures report prototype, slot kind, slot name, occurrence, and candidates, `BBConfig@` failures report prototype, setting values, required slots, and slot candidates, and `ParamTypeInfo@` failures report the source parameter's real CK type.

When `Output Error Message` is enabled, the error text is written to the Component error output parameters.

## Remaining Gaps

- Enum/flags values are represented as `int` / `uint` in scripts. Runtime lookup is available through `Param::*`; generated compile-time hints can be produced with `tools/generate_angelscript_catalog.py`.
- No dedicated BB prototype picker; `BBPrototype@` / `BBDecl@` / `BBConfig@` currently use string, `Category/Name`, GUID text, or SDK-driven `BB::Find/FindAll/At`. `BBSlot@` and `BBConfig@` add setup-time slot binding on top of the resolved prototype.
- A dedicated BB prototype picker is still preferable long term. `type=behavior` is a practical editor-facing fallback because Virtools already has a behavior picker.
- Managed parameter pruning is intentionally tied to manifest/metadata rebuilds, not to arbitrary runtime graph edits.
- Generic plugin-defined struct parameters are not decoded field-by-field. Use `ParamRef@` source connections or SDK string-backed defaults for unknown types.
- `BehaviorRef.ConnectOperation()` mutates the existing graph and is not automatically rolled back by Component lifecycle. Runtime `BBConfig.Operation()` and low-level `BB.Call/Spawn.SetOperation()` remain the safe scoped paths because their owned operations are released with the runtime instance/result/task.
