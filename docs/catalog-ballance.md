# Ballance Catalog Snapshot

This snapshot is generated from the current local Ballance validation export under:

```text
build\validation\ballance
```

It is environment-specific. Treat it as a practical Ballance snapshot, not a universal Virtools SDK reference.

## Snapshot Counts

| Item | Count |
| --- | ---: |
| Parameter types | 88 |
| Operations | 0 |
| Building Blocks exported from `AngelScript.dll` | 1 |

Parameter categories:

| Category | Count |
| --- | ---: |
| `primitive` | 42 |
| `enum` | 23 |
| `object_ref` | 21 |
| `flags` | 2 |

## Exported CKAngelScript Building Block

| Field | Value |
| --- | --- |
| Name | `AngelScript Component` |
| Category | `AngelScript` |
| GUID | `0x5F5D4A84-0x3DFD4D19` |
| Inputs | `Enable`, `Disable` |
| Outputs | `Enabled`, `Disabled`, `Error` |
| Input parameters | `Script`, `Class`, `Source`, `File`, `Manifest` |
| Setting | `Output Error Message` |

## Common Parameter Types

| Name | Category | GUID |
| --- | --- | --- |
| `String` | `primitive` | `0x6BD010E2-0x115617EA` |
| `Integer` | `primitive` | `0x5A5716FD-0x44E276D7` |
| `Float` | `primitive` | `0x47884C3F-0x432C2C20` |
| `Boolean` | `primitive` | `0x1AD52A8E-0x5E741920` |
| `Vector` | `primitive` | `0x48824EAE-0x2FE47960` |
| `Vector 2D` | `primitive` | `0x4EFCB34A-0x6079E42F` |
| `Quaternion` | `primitive` | `0x06C439EE-0x45B50FC2` |
| `Matrix` | `primitive` | `0x643F046E-0x65211B71` |
| `Color` | `primitive` | `0x57D42FEE-0x7CBB3B91` |
| `Object` / object-derived types | `object_ref` | exported per CK class |

## Generate AngelScript Hints

Run exporter validation first:

```powershell
tools\Validate-Ballance.ps1 `
  -BallanceRoot C:\Users\kakut\Games\Ballance `
  -BuildDll build\src\Release\AngelScript.dll `
  -SkipPlayer
```

Then generate catalog hints:

```powershell
python tools\generate_angelscript_catalog.py --validation-dir build\validation\ballance
```

The default output is:

```text
build\validation\ballance\CKAngelScriptCatalog.as
```

You can ask the generator for selected BB wrapper helpers:

```powershell
python tools\generate_angelscript_catalog.py `
  --validation-dir build\validation\ballance `
  --selected-bb "AngelScript/AngelScript Component"
```

## Usage Pattern

Include or copy the generated `.as` catalog into a script module when you want compile-time names for exported GUIDs:

```angelscript
CKGUID stringType = CKASCatalog::ParamTypes::String();
BBDecl@ componentDecl = CKASCatalog::BBHints::AngelScript_AngelScript_Component::Decl(ctx);
```

For most scripts, direct `Param::Type(ctx, "String")` or `BB::Require(ctx, "AngelScript/AngelScript Component")` is simpler. The catalog is most useful for large scripts that want stable generated names and wrapper helpers.
