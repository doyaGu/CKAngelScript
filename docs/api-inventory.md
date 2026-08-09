# API Inventory

This page inventories the public API surface currently exposed by CKAngelScript. It is a map for maintainers, not a replacement for the task-focused guides. Script registration counts come from the tracked live-engine export in `docs/script-api.json`; the native public ABI is declared in `include/CKAngelScript.h`.

## Summary

| Surface | Scope | Primary sources |
| --- | --- | --- |
| Public API | C ABI functions plus the `CKAngelScriptApi` C++ wrapper, header-only soft-load helpers for optional extension plugins, public struct initializers/status text, load/compile/unload, module import binding, module bytecode save/load, raw borrow helpers, source metadata reflection, symbol/runtime handles, typed argument/result helpers, execution handles, engine extension callbacks | `include/CKAngelScript.h`, `docs/public-api.md` |
| High-level script API | Script entries, scene refs, behavior frame, behavior bridge, BB runtime helpers, parameter conversion/catalog, messaging, async tasks | `src/ScriptRuntime.cpp`, `src/ScriptScene.cpp`, `src/ScriptBehaviorFrame.cpp`, `src/ScriptBridgeRegistration.cpp`, `src/ScriptParameterRegistry.cpp`, `src/ScriptMessage.cpp`, `src/ScriptAsync.cpp` |
| Native memory and FFI | `NativePointer`, `NativeBuffer`, DynLoad/DynCall/DynCallback helpers | `src/ScriptNativePointer.cpp`, `src/ScriptNativeBuffer.cpp`, `src/ScriptDynCall.cpp` |
| Raw CK/Vx SDK bindings | CK context/managers/objects, SDK enums/defines/structs, Vx math types, containers | `src/ScriptCK*.cpp`, `src/ScriptVxMath.cpp`, `src/ScriptX*.cpp` |
| Component/editor integration | `AngelScript Component`, `AngelScript Executor`, metadata manifest, injected fields, BB config/slot metadata, `BehaviorFrame@` lifecycle invocation | `src/ScriptComponent*.cpp`, `src/AngelScriptExecutor.cpp`, `src/ScriptInvoker.cpp`, `docs/component-metadata-manifest.md`, `docs/angelscript-executor.md` |

<!-- BEGIN GENERATED API CATALOG SUMMARY -->
## Registration Counts

Generated from the tracked live-engine export in `docs/script-api.json`:

| Registration kind | Count |
| --- | ---: |
| Global functions | 598 |
| Global properties | 301 |
| Object types | 336 |
| Object methods | 11033 |
| Object behaviours | 777 |
| Object properties | 465 |
| Funcdefs | 29 |
| Enums | 139 |
| Enum values | 1427 |
| Typedefs | 24 |
| Total | 15129 |

Large raw SDK binding files and exported cast/behaviour entries dominate the total. The high-level and integration files are still the workflow surface; raw CK/Vx bindings and SDK constants account for most exported symbols.

## Namespace Map

| Namespace | Global functions | Main responsibility |
| --- | ---: | --- |
| `Scene` | 123 | Safe scene lookup, typed refs, creation, scene membership, selection, destruction |
| `Param` | 83 | Parameter values, enum/flags/type metadata, text/raw conversion, operations |
| `Async` | 25 | Frame-based cooperative tasks, aggregates, waits for BB/graph tasks |
| `BB` | 20 | Building Block prototype discovery, declaration/config creation, runtime bridge entry points |
| `Message` | 25 | Publish/send/request/reply messaging between runtime entries and components |
| `Behavior` | 13 | Behavior graph lookup, query construction, graph bridge entry points |
| `Runtime` | 11 | Script entry manager inspection and control |
| `Global` | 298 | Raw CK/Vx helpers, constructors/operators, allocation helpers, format/info functions |

The global bucket includes raw SDK-style helpers and constructors, so new high-level workflow APIs should normally prefer a named namespace.
<!-- END GENERATED API CATALOG SUMMARY -->

## High-Level Script API Families

| Family | Key types and namespaces | Notes |
| --- | --- | --- |
| Script entries | `ScriptContext`, `RuntimeScriptInfo`, `RuntimeDependencyInfo`, `Runtime::*` | Discovered modules. `[runtime]` entries receive `ScriptContext` lifecycle; `[executor]` entries provide Executor classes. |
| Scene interop | `Scene::*`, `ObjectRef`, `SceneObjectRef`, `Entity3DRef`, `Entity2DRef`, `MaterialRef`, `TextureRef`, `MeshRef`, `SceneRef`, `LevelRef` | Revalidating object handles are the durable identity layer; raw CK pointers should be reacquired near use. |
| Script host frame | `BehaviorFrame` | Preferred Component/Executor lifecycle argument; complete current-`CKBehavior` facade for context fields, CKObject/scene inherited state, identity, owner/target, graph/layout entry points, IO, pins/pouts/settings, logical locals, child-frame derivation with `FrameFor`/`SubFrame`, generic SDK parameter value reads/writes, metadata/state, structural graph edits, graph-link activation-delay waits, frame-aware async waits/message waits, trace, one-shot BB calls, frame-aware `BBBridge` access for persistent BB discovery/configuration, SDK-style `Get*` migration aliases, physical-local `BehaviorLocal*` aliases, and raw `CKBehavior@` escape-hatch access while reacquiring the live behavior by id/stamp. `Local*` is the host logical-local API; `GetLocalParameter*`, `LocalParameter`, and `BehaviorLocal*` are physical CKBehavior local APIs. |
| Behavior graph bridge | `Behavior::*`, `BehaviorRef`, `BehaviorGraph`, `BehaviorQuery`, `BehaviorNode`, `BehaviorLayout`, `BehaviorLinkRef`, `GraphTask` | Setup-time graph search, layout handles, target-parameter source preservation, graph traversal, synchronous `BehaviorRef.Step(ctx/frame, input)` execution of existing native BB instances, and graph task observation. |
| Runtime BB bridge | `BB::*`, `BBPrototype`, `BBDecl`, `BBConfig`, `BBSlot`, `BBInstance`, `BBResult`, `BBTask`, `BBCallBuilder`, `BBTaskBuilder` | SDK-driven BB discovery, runtime BB spawning/stepping, live pin/settings updates, name-based one-shot calls, and `BehaviorFrame@` overloads for common config/instance operations. |
| Parameters | `Param::*`, `ParamRef`, `ParamValue`, `ParamStructValue`, `ParamTypeInfo`, `ParamEnumInfo`, `ParamFlagsInfo`, `ParamStructInfo`, `ParamOp`, `ParamSourceLinkRef` | Generic parameter conversion and metadata backed by `CKParameterManager`. |
| Messaging | `Message::*`, `ScriptMessage`, `ScriptMessageTarget` | Topic publish/subscribe, typed directed sends, compatible string-target adapters, generation-checked component targets, and request/reply over `AsyncTask<dictionary@>`. |
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
| Initialization/diagnostics | `CKAngelScriptInitResult`, `CKAngelScriptInitLoadOptions`, `CKAngelScriptInitImportBindOptions`, `CKAngelScriptInitBytecodeSaveOptions`, `CKAngelScriptInitBytecodeLoadOptions`, `CKAngelScriptInitModuleFingerprint`, `CKAngelScriptInitFunctionOptions`, `CKAngelScriptInitFunctionExecutionOptions`, `CKAngelScriptInitExecutionStepOptions`, `CKAngelScriptInitObjectOptions`, `CKAngelScriptInitMethodOptions`, `CKAngelScriptInitObjectMethodExecuteOptions`, `CKAngelScriptInitEngineExtension`, `CKAngelScriptGetStatusName`, `CKAngelScriptGetStatusDescription` |
| Header-only soft-load helpers | `CKAngelScriptExtensionApi`, `CKAngelScriptInitExtensionApi`, `CKAngelScriptExtensionApiIsLoaded`, `CKAngelScriptLoadExtensionApi`, `CKAngelScriptRegisterEngineExtensionWithApi`, `CKAngelScriptUnregisterEngineExtensionWithApi` |
| Modules/functions | `CKAngelScriptLoadModule`, `CKAngelScriptCompileModule`, `CKAngelScriptUnloadModule`, `CKAngelScriptHasModule`, `CKAngelScriptGetModuleGeneration`, `CKAngelScriptBorrowModule`, `CKAngelScriptBorrowFunctionByName`, `CKAngelScriptBorrowFunctionByDecl`, `CKAngelScriptEnumerateMetadata`, `CKAngelScriptGetImportedFunctionCount`, `CKAngelScriptEnumerateImportedFunctions`, `CKAngelScriptBindImportedFunction`, `CKAngelScriptBindAllImportedFunctions`, `CKAngelScriptUnbindImportedFunction`, `CKAngelScriptUnbindAllImportedFunctions`, `CKAngelScriptEnumerateBoundImportEdges`, `CKAngelScriptEnumerateModuleIncludeEdges`, `CKAngelScriptGetModuleFingerprint`, `CKAngelScriptSaveModuleBytecode`, `CKAngelScriptLoadModuleBytecode`, `CKAngelScriptFindFunction`, `CKAngelScriptReleaseFunction` |
| Objects/methods | `CKAngelScriptCreateObject`, `CKAngelScriptReleaseObject`, `CKAngelScriptFindObjectMethod`, `CKAngelScriptReleaseMethod`, `CKAngelScriptCallObjectMethod`; object creation accepts `ClassName` with optional `ClassNamespace`, or exact `TypeDecl`; method lookup is scoped to an object handle |
| Args/results | `CKAngelScriptArgSetBool`, `CKAngelScriptArgSetInt`, `CKAngelScriptArgSetFloat`, `CKAngelScriptArgSetString`, `CKAngelScriptArgSetBorrowedObject`, `CKAngelScriptArgSetObjectHandle`, `CKAngelScriptResultGetBool`, `CKAngelScriptResultGetInt`, `CKAngelScriptResultGetFloat`, `CKAngelScriptResultGetString`, `CKAngelScriptResultGetStringView` |
| Executions | `CKAngelScriptCreateFunctionExecution`, `CKAngelScriptStartExecution`, `CKAngelScriptResumeExecution`, `CKAngelScriptCancelExecution`, `CKAngelScriptReleaseExecution`, `CKAngelScriptGetExecutionState`, `CKAngelScriptBorrowExecutionResult`, `CKAngelScriptGetLastResult` |
| Extension callbacks | `CKAngelScriptRegisterEngineExtension`, `CKAngelScriptUnregisterEngineExtension`; extension registrations are wrapped in AngelScript config groups so unregister can remove current-engine symbols when they are not in use |

Supporting public structs/enums are `CKAS_STATUS`, `CKAS_FEATURE`, `CKAS_CALLFLAGS`, `CKAS_BYTECODEFLAGS`, `CKAS_EXECUTIONSTATE`, `CKAS_METADATA_TARGET`, `CKAS_MODULEKIND`, `CKAngelScriptExtensionApi`, `CKAngelScriptMetadataEntry`, `CKAngelScriptImportEntry`, `CKAngelScriptBoundImportEdge`, `CKAngelScriptIncludeEdge`, `CKAngelScriptModuleFingerprint`, `CKAngelScriptSourceSection`, `CKAngelScriptLoadOptions`, `CKAngelScriptImportBindOptions`, `CKAngelScriptBytecodeSaveOptions`, `CKAngelScriptBytecodeLoadOptions`, `CKAngelScriptFunctionOptions`, `CKAngelScriptFunctionExecutionOptions`, `CKAngelScriptExecutionStepOptions`, `CKAngelScriptObjectOptions`, `CKAngelScriptMethodOptions`, `CKAngelScriptObjectMethodExecuteOptions`, `CKAngelScriptResult`, and `CKAngelScriptEngineExtension`. The current public call flag is `CKAS_CALL_NO_SUSPEND`; feature names are exact binary surface probes, not readiness checks. Current public feature probes include module lifecycle, raw AngelScript access, function handles/execution/resume, object handles, object handle argument writing, object type namespace lookup, synchronous object method call, typed arg/result helpers, stack trace, engine extension, public struct initializers, status text, metadata reflection, script array access, active-context host exceptions, source-section loading, host-call filtering, module imports, module bytecode, module replacement transactions, module graph diagnostics, and module fingerprints. Metadata targets are `CKAS_METADATA_TYPE`, `CKAS_METADATA_TYPE_METHOD`, `CKAS_METADATA_GLOBAL_FUNCTION`, `CKAS_METADATA_GLOBAL_VARIABLE`, and `CKAS_METADATA_TYPE_PROPERTY`.

## Maintenance Checklist

- Keep user-facing workflow APIs under named namespaces such as `Scene`, `Behavior`, `BB`, `Param`, `Message`, `Async`, or `Runtime`.
- Prefer SDK-driven metadata over hard-coded game or plugin-specific tables.
- Treat bytecode as an executable module cache only. CKAngelScript metadata reflection is backed by `CScriptBuilder` source metadata, so bytecode-only module loads need a source/metadata sidecar if callers still need metadata diagnostics or discovery.
- Treat module graph/fingerprint data as CKAngelScript diagnostics. It explains public load/import API activity and source-section snapshot includes, but raw AngelScript mutations can bypass it.
- Add new public entry points only through the C ABI in `include/CKAngelScript.h`, then mirror them in the C++ wrapper and document result lifetime rules.
- For every public callback, document whether callback functions and `UserData` are call-scope, callback-scope, handle-scope, or registration-scope.
- When adding raw bindings, update `docs/sdk-bindings.md` if a new binding area appears.
- When adding high-level APIs, update the relevant task guide and this inventory.
