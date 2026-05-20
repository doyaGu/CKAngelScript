# AngelScript Behavior Bridge v2

## Core Model

Bridge v2 follows the CK2 behavior model directly. `CKBehavior` input/output IO and `pIn/pOut/pLocal` arrays have stable order, so script code uses index as the canonical identity. Names are only for initialization-time lookup and diagnostics.

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

## Runtime BB Calls

Runtime BB creation is index-first:

```angelscript
BBPrototype@ proto = BB::Find(ctx, "Interface/Text/2D Text");
BehaviorLayout@ layout = proto.Layout();

int on = layout.FindInput("On");
int text = layout.FindPin("Text");

BBResult@ result = proto.Call()
    .Set(text, Param::String("Hello"))
    .Run(on);

if (!result.ok) {
    result.Raise(ctx);
}
```

For multi-frame BBs, use `Spawn()`:

```angelscript
BBTask@ task = proto.Spawn()
    .Set(text, Param::String("FPS: ..."))
    .Start(on);

task.Step(ctx);          // no re-pulse
task.Step(ctx, on);      // explicit pulse
```

`BBResult.Pout(index)`, `BBTask.Pout(index)`, and `GraphTask.Pout(index)` return live/lazy parameter handles. `BBResult` owns the temporary runtime behavior until the result handle is released; `BBTask.Destroy()` invalidates task-owned parameter handles.

## Catalog Discovery

BB prototype discovery is SDK-driven and uses `CKGetPrototypeDeclaration*` order:

```angelscript
int count = BB::Count(ctx);
BBPrototype@ first = BB::At(ctx, 0);
BBPrototype@ text = BB::Find(ctx, "Interface/Text/2D Text");

string category = text.GetCategory();
string qualified = text.GetQualifiedName();
CKGUID guid = text.GetGuid();
```

`BB::Find` accepts a name, `Category/Name`, or `guid:0x...,0x...` text. Use `occurrence` only when a plain name is intentionally duplicated.

`Param` helpers expose enum/flags/type metadata without hard-coded integer tables:

```angelscript
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
- `GraphTask` never owns the watched behavior; component reset/delete only cancels the watch.
- Component pause pauses bridge-owned tasks and watches; it does not mutate external graphs.

## Current Limitations

- The internal layout cache is signature-backed, but scripts should still resolve names during setup and cache indices themselves.
- Unknown variable-size plugin structs are not decoded. Use source/copy/text conversion or fixed-size raw buffers.
- Name lookup is intentionally not available on hot-path setters/getters. Resolve names through `Layout()` first.
