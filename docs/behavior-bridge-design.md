# AngelScript Behavior Bridge v3

## Core Model

Bridge v3 follows the CK2 behavior model directly. `CKBehavior` input/output IO and `pIn/pOut/pLocal` arrays have stable order, so script code uses index as the canonical identity. Names are only for initialization-time lookup and diagnostics.

The bridge no longer creates a full output snapshot map. `BBResult`, `BBTask`, and `GraphTask` only keep lightweight execution state: return code, error text, current active output indices, and sticky seen output indices. Output parameter values are read lazily through `ParamRef`.

## Layout First

```angelscript
BehaviorRef@ b = Behavior::Find(ctx, "Some BB");
BehaviorLayout@ layout = b.Layout();

int in = layout.FindInput("In");
int value = layout.FindPin("Value");
int done = layout.FindOutput("Done");

b.Pin(value).Set(Param::Int(10));
b.Trigger(in);
```

`BehaviorLayout` exposes `InputCount/OutputCount/PinCount/PoutCount/LocalCount`, name lookup with occurrence, and `ParamInfo` for parameter slots. Use `Find*()` once in `Start()` or setup code, then cache the indices.

## Behavior Graph Search

Large real game graphs should be searched through a container graph instead of global name lookup. `Behavior::Graph(ctx)` returns the owner script graph for the current component. `BehaviorRef.AsGraph()` treats a behavior as a subgraph container.

Search is setup-time only. `BehaviorQuery` defaults to exact name matching when `Name()` is used; use `NameContains()` only for exploration or diagnostics. Recursive search is explicit, pre-order, and preserves CK subbehavior order. The root container is not returned unless `IncludeRoot(true)` is set.

```angelscript
BehaviorGraph@ graph = Behavior::Graph(ctx);
BehaviorQuery@ query = Behavior::Query()
    .Name("Loading_Manager")
    .Recursive(true)
    .InputCount(-1)
    .OutputCount(-1);

BehaviorNode@ node = graph.Require(query);
if (node is null || !node.valid) {
    ctx.RaiseError(graph.DescribeCandidates(query));
    return;
}

BehaviorRef@ loadingManager = node.Behavior();
BehaviorLayout@ layout = loadingManager.Layout();
int input = layout.FindInput("In");
```

`Require()` is intentionally strict: zero or multiple candidates return an invalid node and diagnostics listing behavior name, id, prototype, target, and layout counts. `Find()` returns the requested occurrence. `FindAll()` returns every candidate for custom filtering.

Graph traversal follows real `CKBehaviorLink` objects without mutating the graph:

```angelscript
BehaviorNode@ next = node.Next();
array<BehaviorNode@>@ allNext = node.NextAll();
BehaviorLinkRef@ link = node.NextLink();

if (link !is null && link.valid) {
    BehaviorRef@ source = link.SourceBehavior();
    int outputIndex = link.SourceOutputIndex();
    BehaviorRef@ target = link.TargetBehavior();
    int inputIndex = link.TargetInputIndex();
}
```

`Next/Prev` accept an optional `BehaviorQuery@` filter. `Input(index)` and `Output(index)` are link traversal helpers for a specific target input or source output. `End(maxSteps)` follows the first next link until there is no next node or a cycle/step limit is reached.

`Behavior::Find(ctx, name)` remains as a small convenience wrapper over `Behavior::Graph(ctx).Find(Behavior::Query().Name(name).Recursive(true))`; new scripts should prefer explicit graph/query/node code when the graph is large or duplicate names are possible.

## Persistent Graph Editing

Graph editing is explicit and transaction-style. The bridge mutates the live `CKBehavior` graph only when `Apply(ctx)` is called; saving the edited graph to `.cmo/.nmo` remains the host/libnmo responsibility.

```angelscript
BehaviorGraph@ graph = Behavior::Graph(ctx);
BBDecl@ decl = BB::Require(ctx, "Logics/Calculator/Identity");

BehaviorGraphEdit@ edit = graph.Edit();
GraphEditNode@ node = edit.Add(decl, "Probe Identity");

GraphEditResult@ check = edit.Validate(ctx);
if (check is null || !check.ok) {
    check.Raise(ctx);
    return;
}

GraphEditResult@ applied = edit.Apply(ctx);
if (applied is null || !applied.ok) {
    applied.Raise(ctx);
    return;
}
```

Supported persistent operations are:

- `Import(node)`: make an existing direct child behavior available inside the transaction.
- `Add(BBDecl@/BBConfig@, name)`: create a persistent BB behavior from the SDK prototype/config.
- `Link(sourceNode, outputIndex/slot, targetNode, inputIndex/slot, delay)`: create a real `CKBehaviorLink`.
- `Unlink(link)` and `Relink(link, ...)`: remove or replace a real link.
- `Remove(node, removeIncidentLinks)`: destroy a direct child behavior; incident links must be removed explicitly unless `removeIncidentLinks=true`.
- `Move(node, targetGraph)`: move a direct child behavior to another graph; existing incident links must be unlinked first.

Validation rejects stale handles, foreign graph handles, function behaviors used as graph roots, invalid IO indices, removing nodes with remaining incident links, missing managers, bad target/owner compatibility, and bad pending parameter/settings conversion. `Apply()` creates nodes and links before destructive edits, and rolls back objects it created if a creation step fails. After `Apply()` succeeds the edit is persistent in the live CK graph; component lifetime hooks do not roll it back.

## Parameter Handles

`ParamRef@` wraps `CKParameterIn`, `CKParameterOut`, `CKParameterLocal`, operation parameters, or standalone `CKParameter`.

Important methods:

- `TypeGuid()`, `TypeName()`, `DataSize()`, `Describe()`
- `RealSource()` and `DirectSource()`
- `SetSource(ParamRef@ source)`
- `Set(ParamValue@ value)`, `Get()`, `CopyFrom(ParamRef@ source)`
- `GetText()` / `SetText()`
- `GetRaw()` / `SetRaw(NativeBuffer@ data)`

`ParamValue@` is a generic value carrier. Common values use fast paths:

```angelscript
Param::Int(1);
Param::Float(1.0f);
Param::Bool(false);
Param::String("text");
Param::Vector2(Vx2DVector(0, 0));
Param::Object(obj);
Param::ObjectArray(ids);
```

Unknown or plugin-defined types use SDK-driven paths:

```angelscript
Param::Text(ctx, "Render Options", "Clear ZBuffer,Buffer Swapping");
Param::Raw(ctx, "SomeFixedStruct", buffer);
```

The bridge delegates type names, enum/flags text, object compatibility, and operations to `CKParameterManager`. It does not maintain a hard-coded operation or custom struct table.

## Runtime Building Blocks

The v3 high-level BB model has three phases:

1. `BBDecl@` discovers the real SDK prototype declaration and exposes metadata.
2. `BBConfig@` stores owner/target and pending slot-based values, sources, and operations.
3. `BBInstance@` owns the runtime `CKBehavior` and steps or destroys it.

Settings are separate from pins. `BBConfig` uses the SDK/prototype defaults for every parameter that the script does not explicitly mention. If a script only needs fixed setup values, Component metadata can put them directly on the config. If a script needs to update a value every frame, connect a source, read an output parameter, or apply a live setting, it keeps a `BBSlot@`; the slot carries the parameter metadata and the bridge applies it at the correct point in the BB lifecycle.

Runtime creation follows the CK2 callback order that real Building Blocks expect: create the dynamic `CKBehavior`, `InitFromGuid`, write pending settings, send `CKM_BEHAVIORCREATE`, attach owner/target, then write pins, sources, and operations before the first execution. Live `SetSetting()` sends `CKM_BEHAVIORSETTINGSEDITED` and refreshes the runtime layout generation.

```angelscript
BBDecl@ textDecl = BB::Require(ctx, "Interface/Text/2D Text");
BBSlot@ textPin = textDecl.Pin("Text");
BBSlot@ textProperties = textDecl.Setting("Text Properties");

BBConfig@ cfg = textDecl.Configure()
    .Target(target)
    .SetSetting(textProperties, "Screen Proportionnal,WordWrap")
    .Set(textPin, "FPS: ...");

BBInstance@ inst = cfg.SpawnStarted(ctx);
```

Live parameter updates are explicit and lazy:

```angelscript
inst.Set(textPin, "FPS: 120");
inst.Step(ctx);

// High-frequency Set + Step path.
inst.StepSet(ctx, textPin, "FPS: 121");
```

`BBConfig.EnsureSpawned(ctx)` and `EnsureStarted(ctx)` reuse the current live `BBInstance@` when possible, so setup code can be idempotent. `BBConfig.Instance()` returns the current instance or `null` with a clear error. `BBConfig.Explain()` and `BBInstance.Explain()` return multi-line diagnostics for metadata, source wiring, target, and current runtime errors.

`BBSlot` records the slot kind, index, name, parameter type, layout caps, and layout generation. Passing a `pout` slot to `Set()`, passing a pin slot to `SetSetting()`, or using a stale slot after a dynamic layout change fails with a targeted diagnostic. The lower-level `BBPrototype.Call()` / `Spawn()` and raw index API remain available for generated code and performance-sensitive scripts.

Component metadata can inject the same v3 objects:

```angelscript
class FpsOverlay {
    CK2dEntity@ target;

    [bbconfig prototype="Interface/Text/2D Text" target="target" lifetime="component"]
    [bbsetting "Text Properties"="Screen Proportionnal,WordWrap"]
    [bbpin "Text"="FPS: ..."]
    BBConfig@ text;

    BBInstance@ instance;

    [bbslot from="text" pin="Text"]
    BBSlot@ textPin;

    void Start(const CKBehaviorContext &in ctx) {
        @instance = text.EnsureStarted(ctx);
    }

    void Update(const CKBehaviorContext &in ctx) {
        if (instance !is null) {
            instance.StepSet(ctx, textPin, "FPS: 120");
        }
    }
}
```

`BBResult.Pout(index)`, `BBTask.Pout(index)`, `BBInstance.Pout(slot)`, and `GraphTask.Pout(index)` return live/lazy parameter handles. Runtime-owned handles become invalid after `Destroy()`.

## Catalog Discovery

BB prototype discovery is SDK-driven and uses `CKGetPrototypeDeclaration*` order:

```angelscript
int count = BB::Count(ctx);
BBPrototype@ first = BB::At(ctx, 0);
BBPrototype@ text = BB::Find(ctx, "Interface/Text/2D Text");
BBDecl@ requiredText = BB::Require(ctx, "Interface/Text/2D Text");
array<BBPrototype@>@ textCandidates = BB::FindAll(ctx, "2D Text");

string category = text.GetCategory();
string qualified = text.GetQualifiedName();
CKGUID guid = text.GetGuid();
string layout = text.Describe();
```

`BB::Require` accepts a name, `Category/Name`, or `guid:0x...,0x...` text and must resolve to exactly one prototype. Zero or multiple candidates produce an invalid `BBDecl@` with candidate diagnostics. `BB::FindAll` returns every matching SDK prototype declaration, so setup code can diagnose duplicate names and then cache the selected GUID and slot handles.

`Param` helpers expose enum/flags/type metadata without hard-coded integer tables:

```angelscript
int typeCount = Param::Count(ctx);
ParamTypeInfo@ firstType = Param::At(ctx, 0);
ParamTypeInfo@ renderInfo = Param::Find(ctx, "Render Options");
CKGUID renderOptions = Param::Guid(ctx, "Render Options");
uint clear = Param::Flag(ctx, renderOptions, "Clear ZBuffer");
uint clearAndSwap = Param::FlagsMask(ctx, renderOptions, "Clear ZBuffer,Buffer Swapping");
string text = Param::Text(ctx, renderOptions, clearAndSwap);
string description = Param::Describe(ctx, renderOptions);
```

For IDE hints or script constants, generate a catalog from validation exports:

```text
python tools/generate_angelscript_catalog.py --validation-dir build/validation/ballance
```

For scripts that use only a few BBs, ask the generator for selected wrappers instead of a large hand-written helper layer:

```text
python tools/generate_angelscript_catalog.py --validation-dir build/validation/ballance --selected-bb "Interface/Text/2D Text" --selected-bb "Interface/Fonts/Create System Font"
```

The generated hints keep the flat GUID functions and add ergonomic helper namespaces:

```angelscript
CKGUID textGuid = CKASCatalog::BBHints::Interface_Text_2D_Text::Guid();
BBPrototype@ text = CKASCatalog::BBHints::Interface_Text_2D_Text::Find(ctx);
BBDecl@ textDecl = CKASCatalog::BBHints::Interface_Text_2D_Text::Decl(ctx);
BBConfig@ textConfig = CKASCatalog::BBHints::Interface_Text_2D_Text::Config(ctx);
BBSlot@ textPin = CKASCatalog::BBHints::Interface_Text_2D_Text::Pin_Text(ctx);
string textPinMetadata = CKASCatalog::BBHints::Interface_Text_2D_Text::Pin_Text_Metadata("text");

uint flags = CKASCatalog::Flags::Render_Options::Mask(ctx, "Clear ZBuffer,Buffer Swapping");
string flagsText = CKASCatalog::Flags::Render_Options::Text(ctx, flags);

ParamOp@ add = CKASCatalog::OperationHints::Addition::Create(ctx);
```

Selected wrappers expose a tiny class per requested BB with `Config(ctx)`, cached slot helpers, `EnsureStarted(ctx)`, and simple typed setters. They are optional; the runtime bridge still resolves everything from SDK prototype declarations.

Generated names are sanitized deterministically; duplicate source names get numeric suffixes.

## GraphTask

`GraphTask` observes an existing `CKBehavior` or behavior graph. It does not execute, reset, clear, or destroy the target graph.

```angelscript
BehaviorRef@ graph = Behavior::Find(ctx, "OpenDoor");
BehaviorLayout@ layout = graph.Layout();
int input = layout.FindInput("In");
int done = layout.FindOutput("Done");

GraphTask@ task = graph.Start(input, false, 3.0f);

void Update(const CKBehaviorContext &in ctx) {
    if (task !is null && task.IsAlive()) {
        task.Step(ctx);
        if (task.Done(done)) {
            task.Cancel();
        } else if (task.TimedOut()) {
            task.Raise(ctx);
        }
    }
}
```

`Done(-1)` means any output has ever been observed. `OutputActive(index)` only reports the most recent `Step()` state.

## Operations

Parameter operations are generic and SDK-driven:

```angelscript
ParamOp@ op = Param::Operation(ctx, "Addition")
    .Result(target.Pin(0).TypeGuid())
    .In(0, source.Pout(0))
    .In(1, Param::Float(1.0f));

target.ConnectOperation(0, op);
```

For runtime BBs:

```angelscript
proto.Call()
    .SetOperation(targetPin, op)
    .Run(inputIndex);
```

The bridge creates a real `CKParameterOperation`, binds `In1/In2` sources or literal locals, verifies `GetOperationFunction()`, and connects the operation output to the target `CKParameterIn`.

## Lifecycle Boundaries

- Runtime `BB.Call()` behavior is held by `BBResult` and deferred-destroyed after result release.
- Runtime `BB.Spawn()` behavior is owned by `BBTask` and is destroyed by `Destroy()`, component reset, component delete, or bridge clear.
- Runtime `BBConfig.Spawn()` behavior is owned by `BBInstance`. `Stop()` only stops/deactivates it, optionally pulsing the configured stop input; `Destroy()` releases the runtime behavior and owned source/operation links. Component metadata uses `lifetime="component"` by default: disable/deactivate/pause call `Stop()`, while reset/delete/script rebuild call `Destroy()`. `lifetime="manual"` leaves cleanup to the script.
- `GraphTask` never owns the watched behavior; component reset/delete only cancels the watch.
- Component pause pauses bridge-owned tasks and watches; it does not mutate external graphs.

## Current Limitations

- The internal layout cache is signature-backed, but scripts should still resolve names during setup and cache `BBSlot@` handles themselves.
- Unknown variable-size plugin structs are not decoded. Use source/copy/text conversion or fixed-size raw buffers.
- Name lookup is intentionally not available on hot-path setters/getters. Resolve names through `Layout()` first.
