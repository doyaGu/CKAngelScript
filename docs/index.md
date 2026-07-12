# CKAngelScript Manual

This manual is written for AngelScript authors using CKAngelScript inside Virtools or Ballance. It assumes you know the host scene/behavior model, but not the CKAngelScript APIs.

## Reading Path

1. [Getting Started](getting-started.md): build, install, validate, and run your first scripts.
2. [Examples](examples.md): copyable recipes for common runtime and component tasks.
3. [Script Entry Manager v2](runtime-script-manager-v2.md): script entries, manifests, lifecycle, dependencies, and runtime inspection.
4. [Component Metadata and Manifest](component-metadata-manifest.md): editor inputs and field injection for `AngelScript Component`.
5. [Scene Interop API](scene-interop.md): safe object refs and high-level object lookup/editing.
6. [Behavior Bridge v3](behavior-bridge-design.md): behavior graph search, parameter handles, and runtime Building Blocks.
7. [Messaging](messaging.md) and [Async](async.md): script-to-script communication and cooperative tasks.
8. [Native FFI](native-ffi.md), [Public API](public-api.md), and [SDK Bindings](sdk-bindings.md): advanced integrations.
9. [CKAngelScript Design Model](ckas-design-model.md): architecture model and Ballance replacement implications.
10. [AngelScript Executor](angelscript-executor.md): graph execution BB for AngelScript-driven replacements.
11. [Ballance Root Replacement Playbook](ballance-root-replacement-playbook.md): proven exact-replacement workflow and larger experiment ladder.
12. [Ballance Script Graph Inventory](ballance-script-graph-inventory.md): scanned Ballance root scripts, nested graph CSVs, and replacement priority.

## Which Doc Do I Need?

| Goal | Read |
| --- | --- |
| Build and install the DLL | [Getting Started](getting-started.md) |
| Validate the DLL in Ballance/Player | [Validation and Troubleshooting](validation-and-troubleshooting.md) |
| Create a long-lived script loaded from `Data/Scripts` | [Script Entry Manager v2](runtime-script-manager-v2.md) |
| Attach script logic to one behavior instance | [Component Metadata and Manifest](component-metadata-manifest.md) |
| Find or create CK objects safely | [Scene Interop API](scene-interop.md) |
| Drive existing Building Blocks or behavior graphs | [Behavior Bridge v3](behavior-bridge-design.md) |
| Send events between scripts/components | [Messaging](messaging.md) |
| Suspend script execution until later frames | [Async](async.md) |
| Call native libraries | [Native FFI](native-ffi.md) |
| Use CKAngelScript from another plugin | [Public API](public-api.md) |
| Audit exposed API surface | [API Inventory](api-inventory.md) |
| Understand raw CK/Vx bindings | [SDK Bindings](sdk-bindings.md) |
| Use generated Ballance hints | [Ballance Catalog Snapshot](catalog-ballance.md) |
| Plan exact Ballance behavior replacement | [CKAngelScript Design Model](ckas-design-model.md) |
| Use a graph execution BB | [AngelScript Executor](angelscript-executor.md) |
| Repeat the proven Ballance root replacement process | [Ballance Root Replacement Playbook](ballance-root-replacement-playbook.md) |
| Inventory Ballance script roots and graph scale | [Ballance Script Graph Inventory](ballance-script-graph-inventory.md) |

## Feature Map

| Feature | Main namespace/type | Context |
| --- | --- | --- |
| Script entries | `Runtime`, `ScriptContext` | Discovered modules; `[runtime]` entries run `ScriptContext` lifecycle, `[executor]` entries back Executor BBs |
| Components | `AngelScript Component`, `CKBehaviorContext` | One script class instance per behavior |
| Scene interop | `Scene`, `ObjectRef@` | Runtime entries and components |
| Behavior graph interop | `Behavior`, `BB`, `Param` | Runtime entries and components |
| Messaging | `Message`, `ScriptMessage`, `ScriptMessageTarget` | Runtime entries and components |
| Async work | `Async`, `AsyncTask<T>` | Any script running through the CKAngelScript scheduler |
| Native memory | `NativePointer`, `NativeBuffer` | Advanced SDK/FFI work |
| Native calls | DynCall/DynLoad/DynCallback helpers | Advanced external library calls |
| API inventory | `Scene`, `Behavior`, `BB`, `Param`, `Message`, `Async`, raw CK/Vx bindings | Maintainer-facing exposed-surface audit |

## Glossary

- **Runtime entry**: A long-lived AngelScript module managed directly by `ScriptManager`, usually discovered from `Scripts` roots with `[runtime]`.
- **Component script**: A script class attached to an `AngelScript Component` Building Block instance.
- **Context**: `ScriptContext` for runtime entries, `CKBehaviorContext` for components, or `CKContext@` for low-level helpers.
- **Object ref**: A revalidating wrapper such as `ObjectRef@`, `Entity3DRef@`, or `BehaviorRef@` that stores CK object identity safely.
- **BB**: Building Block. CKAngelScript exposes both prototype metadata (`BBDecl@`) and runtime instances (`BBInstance@`).
- **Catalog**: Generated AngelScript hints for parameter types, operations, and Building Blocks exported from a host install.

## Maintenance Notes

Examples in this manual should use current registered signatures. Prefer exact context signatures:

```angelscript
void Update(const ScriptContext &in ctx) {}
void Update(const CKBehaviorContext &in ctx) {}
```

Do not document future API names as if they already exist. If a behavior is uncertain, verify it from registration code or self-tests first.
