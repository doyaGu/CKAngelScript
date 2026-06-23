# API Inventory

This page inventories the public API surface currently exposed by CKAngelScript. It is a map for maintainers, not a replacement for the task-focused guides. Counts come from the AngelScript registration calls in `src/*.cpp` and the public C ABI in `include/CKAngelScript.h`.

## Summary

| Surface | Scope | Primary sources |
| --- | --- | --- |
| Public API | C ABI functions plus the `CKAngelScriptApi` C++ wrapper, header-only soft-load helpers for optional extension plugins, public struct initializers/status text, load/compile/unload, raw borrow helpers, metadata reflection, symbol/runtime handles, typed argument/result helpers, execution handles, engine extension callbacks | `include/CKAngelScript.h`, `docs/public-api.md` |
| High-level script API | Runtime scripts, scene refs, behavior bridge, BB runtime helpers, parameter conversion/catalog, messaging, async tasks | `src/ScriptRuntime.cpp`, `src/ScriptScene.cpp`, `src/ScriptBridgeRegistration.cpp`, `src/ScriptParameterRegistry.cpp`, `src/ScriptMessage.cpp`, `src/ScriptAsync.cpp` |
| Native memory and FFI | `NativePointer`, `NativeBuffer`, DynLoad/DynCall/DynCallback helpers | `src/ScriptNativePointer.cpp`, `src/ScriptNativeBuffer.cpp`, `src/ScriptDynCall.cpp` |
| Raw CK/Vx SDK bindings | CK context/managers/objects, SDK enums/defines/structs, Vx math types, containers | `src/ScriptCK*.cpp`, `src/ScriptVxMath.cpp`, `src/ScriptX*.cpp` |
| Component/editor integration | `AngelScript Component`, metadata manifest, injected fields, BB config/slot metadata | `src/ScriptComponent*.cpp`, `docs/component-metadata-manifest.md` |

## Registration Counts

Current AngelScript registration scan:

| Registration kind | Count |
| --- | ---: |
| Global functions | 525 |
| Global properties | 302 |
| Object types | 208 |
| Object methods | 3788 |
| Object behaviours | 361 |
| Object properties | 440 |
| Funcdefs | 28 |
| Enums | 145 |
| Enum values | 1431 |
| Typedefs | 35 |
| Total | 7263 |

Large raw SDK binding files dominate the total. The high-level and integration files account for roughly 1400 registrations; raw CK/Vx bindings and SDK constants account for the rest.

## Namespace Map

| Namespace | Global functions | Main responsibility |
| --- | ---: | --- |
| `Scene` | 123 | Safe scene lookup, typed refs, creation, scene membership, selection, destruction |
| `Param` | 55 | Parameter values, enum/flags/type metadata, text/raw conversion, operations |
| `Async` | 23 | Frame-based cooperative tasks, aggregates, waits for BB/graph tasks |
| `BB` | 18 | Building Block prototype discovery, declaration/config creation, runtime bridge entry points |
| `Message` | 14 | Publish/send/request/reply messaging between runtime scripts and components |
| `Behavior` | 13 | Behavior graph lookup, query construction, graph bridge entry points |
| `Runtime` | 11 | Runtime script manager inspection and control |
| Global | 267 | Raw CK/Vx helpers, constructors/operators, allocation helpers, format/info functions |

The global bucket includes raw SDK-style helpers and constructors, so new high-level workflow APIs should normally prefer a named namespace.

## High-Level Script API Families

| Family | Key types and namespaces | Notes |
| --- | --- | --- |
| Runtime scripts | `ScriptContext`, `RuntimeScriptInfo`, `RuntimeDependencyInfo`, `Runtime::*` | Long-lived scripts discovered from script roots. |
| Scene interop | `Scene::*`, `ObjectRef`, `SceneObjectRef`, `Entity3DRef`, `Entity2DRef`, `MaterialRef`, `TextureRef`, `MeshRef`, `SceneRef`, `LevelRef` | Revalidating object handles are the durable identity layer; raw CK pointers should be reacquired near use. |
| Behavior graph bridge | `Behavior::*`, `BehaviorRef`, `BehaviorGraph`, `BehaviorQuery`, `BehaviorNode`, `BehaviorLayout`, `BehaviorLinkRef`, `GraphTask` | Setup-time graph search, layout handles, graph traversal, graph task observation. |
| Runtime BB bridge | `BB::*`, `BBPrototype`, `BBDecl`, `BBConfig`, `BBSlot`, `BBInstance`, `BBResult`, `BBTask`, `BBCallBuilder`, `BBTaskBuilder` | SDK-driven BB discovery, runtime BB spawning/stepping, live pin/settings updates. |
| Parameters | `Param::*`, `ParamRef`, `ParamValue`, `ParamStructValue`, `ParamTypeInfo`, `ParamEnumInfo`, `ParamFlagsInfo`, `ParamStructInfo`, `ParamOp`, `ParamSourceLinkRef` | Generic parameter conversion and metadata backed by `CKParameterManager`. |
| Messaging | `Message::*`, `ScriptMessage` | Topic publish/subscribe, directed sends, request/reply over `AsyncTask<dictionary@>`. |
| Async | `Async::*`, `AsyncTask<T>`, `Await`, `Async*Func` funcdefs | Scheduler-backed suspension, not OS threads. |
| Native/FFI | `NativePointer`, `NativeBuffer`, DynLoad/DynCall/DynCallback types | Advanced interop; signatures and ownership must be treated as unsafe native code. |

## Raw SDK Binding Areas

| Area | Registration count | Source |
| --- | ---: | --- |
| Vx math and geometry | 1449 | `src/ScriptVxMath.cpp` |
| CK defines, structs, global constants | 1416 | `src/ScriptCKDefines.cpp` |
| CK objects | 1401 | `src/ScriptCKObjects.cpp` |
| CK enums | 939 | `src/ScriptCKEnums.cpp` |
| CK managers | 355 | `src/ScriptCKManagers.cpp` |
| CK typedefs and containers | 186 | `src/ScriptCKTypes.cpp` |
| CK context | 117 | `src/ScriptCKContext.cpp` |
| Extra script containers/string helpers | 127 | `src/ScriptXString.cpp`, `src/ScriptXObjectArray.cpp`, `src/ScriptXBitArray.cpp` |

These bindings intentionally stay close to native SDK naming. Prefer high-level APIs for identity, scene lookup, behavior control, and diagnostics; use raw SDK methods for the final engine operation.

## Public API

`CKAngelScript` and the `CKAngelScript...` functions are the native integration point for other plugins. C++ users can use `CKAngelScriptApi`, which is a non-owning inline wrapper over those C functions, plus move-only RAII wrappers for public handles.

| Group | Methods |
| --- | --- |
| Engine/context | `CKAngelScriptGetApiVersion`, `CKAngelScriptHasFeature`, `CKAngelScriptGetVersion`, `CKAngelScriptGetOptions`, `CKAngelScriptBorrowEngine`, `CKAngelScriptBorrowActiveContext` |
| Initialization/diagnostics | `CKAngelScriptInitResult`, `CKAngelScriptInitLoadOptions`, `CKAngelScriptInitFunctionOptions`, `CKAngelScriptInitFunctionExecutionOptions`, `CKAngelScriptInitExecutionStepOptions`, `CKAngelScriptInitObjectOptions`, `CKAngelScriptInitMethodOptions`, `CKAngelScriptInitObjectMethodExecuteOptions`, `CKAngelScriptInitEngineExtension`, `CKAngelScriptGetStatusName`, `CKAngelScriptGetStatusDescription` |
| Header-only soft-load helpers | `CKAngelScriptExtensionApi`, `CKAngelScriptInitExtensionApi`, `CKAngelScriptExtensionApiIsLoaded`, `CKAngelScriptLoadExtensionApi`, `CKAngelScriptRegisterEngineExtensionWithApi`, `CKAngelScriptUnregisterEngineExtensionWithApi` |
| Modules/functions | `CKAngelScriptLoadModule`, `CKAngelScriptCompileModule`, `CKAngelScriptUnloadModule`, `CKAngelScriptHasModule`, `CKAngelScriptGetModuleGeneration`, `CKAngelScriptBorrowModule`, `CKAngelScriptBorrowFunctionByName`, `CKAngelScriptBorrowFunctionByDecl`, `CKAngelScriptEnumerateMetadata`, `CKAngelScriptFindFunction`, `CKAngelScriptReleaseFunction` |
| Objects/methods | `CKAngelScriptCreateObject`, `CKAngelScriptReleaseObject`, `CKAngelScriptFindObjectMethod`, `CKAngelScriptReleaseMethod`, `CKAngelScriptCallObjectMethod`; v3 object creation accepts `ClassName` with optional `ClassNamespace`, or exact `TypeDecl`; method lookup is scoped to an object handle |
| Args/results | `CKAngelScriptArgSetBool`, `CKAngelScriptArgSetInt`, `CKAngelScriptArgSetFloat`, `CKAngelScriptArgSetString`, `CKAngelScriptArgSetBorrowedObject`, `CKAngelScriptArgSetObjectHandle`, `CKAngelScriptResultGetBool`, `CKAngelScriptResultGetInt`, `CKAngelScriptResultGetFloat`, `CKAngelScriptResultGetString`, `CKAngelScriptResultGetStringView` |
| Executions | `CKAngelScriptCreateFunctionExecution`, `CKAngelScriptStartExecution`, `CKAngelScriptResumeExecution`, `CKAngelScriptCancelExecution`, `CKAngelScriptReleaseExecution`, `CKAngelScriptGetExecutionState`, `CKAngelScriptBorrowExecutionResult`, `CKAngelScriptGetLastResult` |
| Extension callbacks | `CKAngelScriptRegisterEngineExtension`, `CKAngelScriptUnregisterEngineExtension`; extension registrations are wrapped in AngelScript config groups so unregister can remove current-engine symbols when they are not in use |

Supporting public structs/enums are `CKAS_STATUS`, `CKAS_FEATURE`, `CKAS_CALLFLAGS`, `CKAS_EXECUTIONSTATE`, `CKAS_METADATA_TARGET`, `CKAngelScriptExtensionApi`, `CKAngelScriptMetadataEntry`, `CKAngelScriptSourceSection`, `CKAngelScriptLoadOptions`, `CKAngelScriptFunctionOptions`, `CKAngelScriptFunctionExecutionOptions`, `CKAngelScriptExecutionStepOptions`, `CKAngelScriptObjectOptions`, `CKAngelScriptMethodOptions`, `CKAngelScriptObjectMethodExecuteOptions`, `CKAngelScriptResult`, and `CKAngelScriptEngineExtension`. The only public call flag in v3 is `CKAS_CALL_NO_SUSPEND`; feature names are exact binary surface probes, not readiness checks. Current public feature probes include module lifecycle, raw AngelScript access, function handles/execution/resume, object handles, object handle argument writing, object type namespace lookup, synchronous object method call, typed arg/result helpers, stack trace, engine extension, public struct initializers, status text, metadata reflection, script array access, active-context host exceptions, and source-section loading. Metadata targets are `CKAS_METADATA_TYPE`, `CKAS_METADATA_TYPE_METHOD`, `CKAS_METADATA_GLOBAL_FUNCTION`, `CKAS_METADATA_GLOBAL_VARIABLE`, and `CKAS_METADATA_TYPE_PROPERTY`.

## Maintenance Checklist

- Keep user-facing workflow APIs under named namespaces such as `Scene`, `Behavior`, `BB`, `Param`, `Message`, `Async`, or `Runtime`.
- Prefer SDK-driven metadata over hard-coded Ballance or plugin-specific tables.
- Add new public entry points only through the C ABI in `include/CKAngelScript.h`, then mirror them in the C++ wrapper and document result lifetime rules.
- For every public callback, document whether callback functions and `UserData` are call-scope, callback-scope, handle-scope, or registration-scope.
- When adding raw bindings, update `docs/sdk-bindings.md` if a new binding area appears.
- When adding high-level APIs, update the relevant task guide and this inventory.
