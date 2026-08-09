# Native Buffers and FFI

CKAngelScript exposes low-level native memory and DynCall helpers for advanced integrations. Prefer high-level CK/Vx bindings when possible; native FFI can crash the host if signatures or ownership are wrong.

## NativePointer

`NativePointer` wraps a raw address. It is useful when a registered CK method expects a memory pointer or when calling native functions through DynCall.

Global helpers:

```angelscript
NativePointer malloc(size_t size)
void free(NativePointer ptr)
```

Rules:

- Only `free` pointers allocated with `malloc`.
- Do not store pointers to CK objects longer than the object lifetime.
- Prefer `ObjectRef@` for durable CK object identity.
- Treat null pointers as invalid before passing them to native calls.

## NativeBuffer

`NativeBuffer@` owns a byte buffer and is safer than manual `malloc/free` for temporary data. Use it for raw parameter payloads, binary structs, or FFI argument/result storage.

Typical pattern:

```angelscript
NativeBuffer@ buffer = NativeBuffer(16);
buffer.WriteInt(123);
buffer.WriteFloat(4.5f);

buffer.Reset();
int id = 0;
float value = 0.0f;
buffer.ReadInt(id);
buffer.ReadFloat(value);
```

Raw CK parameter pattern:

```angelscript
NativeBuffer@ buffer = NativeBuffer(16);
buffer.Fill(0, buffer.Size());
ParamValue@ raw = Param::Raw(ctx, "SomeFixedStruct", buffer);
```

Use `ParamRef.GetRaw()` and `ParamRef.SetRaw(buffer)` when a CK parameter type cannot be represented by a higher-level `ParamValue`.

Useful buffer methods:

| Method | Purpose |
| --- | --- |
| `Write(?&in)` / `Read(?&out)` | Generic primitive or POD write/read. |
| `WriteInt`, `WriteUInt`, `WriteFloat`, `WriteDouble`, `WriteString` | Explicit typed writes. |
| `ReadInt`, `ReadUInt`, `ReadFloat`, `ReadDouble`, `ReadString` | Explicit typed reads. |
| `Seek`, `Skip`, `Reset`, `CursorPos`, `Size` | Cursor management. |
| `Fill`, `Merge`, `Extract`, `Compare` | Byte range utilities. |
| `ToPointer` | Pointer to the current cursor position. |
| `Load`, `Save` | File I/O into or out of the buffer. |

`NativeBuffer(uintptr_t, size_t)` and `NativeBuffer(NativePointer, size_t)` wrap existing memory. Those buffers do not own the memory; the original owner must keep it alive.

## Prefer Sized and Typed SDK Overloads

When a CK SDK method has both `NativePointer` and `NativeBuffer`/array overloads, use the sized or typed overload. The compatibility `NativePointer` form cannot verify the allocation length.

Load a complete in-memory Virtools file synchronously:

```angelscript
NativeBuffer@ bytes = NativeBuffer(fileSize);
bytes.Load(path, fileSize);

CKFile@ ckFile = ctx.CreateCKFile();
CKObjectArray@ loaded = CreateCKObjectArray();
CKERROR result = ckFile.Load(bytes, loaded);

DeleteCKObjectArray(loaded);
ctx.DeleteCKFile(ckFile);
```

`CKFile.Load(NativeBuffer@, ...)` derives the byte count from `NativeBuffer.Size()` and completes before the call returns. `CKFile.OpenMemory` intentionally remains a `NativePointer` API: it is the first half of a two-stage `OpenMemory`/`LoadFileData` operation, and the CK parser continues borrowing the memory between those calls.

`BehaviorFrame` also exposes sized views for parameter storage:

```angelscript
NativeBuffer@ input = frame.PinReadData(0);
NativeBuffer@ output = frame.PoutWriteData(0);
NativeBuffer@ local = frame.LocalReadData(0);

if (input !is null && input.Size() == 4) {
    int value = 0;
    input.Read(value);
}
```

The SDK-style aliases are `GetInputParameterReadData`,
`GetOutputParameterWriteData`, `GetLocalParameterReadData`, and
`GetLocalParameterWriteData`. These buffers are bounded but borrow live
Virtools parameter memory: consume them during the current behavior call and do
not retain them across frames or layout changes. The `*DataPtr` variants remain
only for compatibility and cannot validate an allocation length.

Use script arrays for typed lists instead of packing integers manually:

```angelscript
bool AddDependencies(CKDependenciesContext@ dependencies, CKObject@ object) {
    array<CK_ID> ids = { object.GetID() };
    return dependencies.AddObjects(ids);
}

void FindObjectsWithAttributes(CKContext@ ctx,
                               CKAttributeType first,
                               CKAttributeType second) {
    array<CKAttributeType> attributes = { first, second };
    ctx.GetAttributeManager().FillListByAttributes(attributes);
}
```

The array overloads infer their element count and validate the AngelScript element type. In generated catalog output, typedefs may appear in canonical form (`CK_ID` as `uint`, `CKAttributeType` as `int`).

## DynLoad and DynCall

DynLoad locates libraries and symbols. DynCall invokes function pointers using a call signature. The exact helpers are registered by `ScriptDynCall.cpp`; verify the required signature before calling into a library.

The low-level helper `DynGetModeFromCCSigChar(int8 sigChar)` converts DynCall calling-convention signature characters to modes.

Recommended workflow:

1. Load the library.
2. Resolve a symbol.
3. Choose the correct calling convention.
4. Marshal arguments using `NativePointer`/`NativeBuffer` and primitive values.
5. Release owned resources.

## Safety Checklist

- Confirm 32-bit library compatibility.
- Confirm `cdecl`/`stdcall`/thiscall conventions.
- Confirm struct packing and pointer ownership.
- Keep buffers alive until the native function returns.
- Never pass AngelScript object pointers to native code unless the binding explicitly supports it.
- Do not free memory owned by Virtools, AngelScript, or another DLL.

## When Not To Use FFI

Use existing SDK bindings, `Scene::*`, `Behavior`/`BB`, and `Param::*` first. FFI is intended for external libraries or unsupported native APIs, not routine scene or behavior work.
