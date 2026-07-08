# C# bindings

`generate_csharp_bindings.py` emits C# wrapper classes for the GDExtension
into `project/addons/fluxxishaderlang/csharp/`, so they ship with the addon.
It runs automatically as part of `scons` whenever the manifest or the
generator changes, and can be run standalone:

```sh
python3 bindings/generate_csharp_bindings.py [--manifest PATH] [--out DIR]
```

`csharp_manifest.json` is the **single source of truth**: nothing is inferred
from ClassDB reflection, so names and type signatures come out exactly as
declared here. When you bind a new method in C++ (`_bind_methods`), add a
matching entry to the manifest.

## How the wrappers work

C# cannot statically link against a C++ GDExtension, so each wrapper holds the
underlying object (`Inner`, created via `ClassDB.Instantiate`) and forwards
calls through `GodotObject.Call` with cached `StringName`s. Classes are
`partial`, so you can extend them by hand in separate files. `static` methods
use `ClassDB.ClassCallStatic`, which requires **Godot 4.4+**.

## Manifest schema

```jsonc
{
  "namespace": "FluxxiShaderLang",       // namespace for all generated classes
  "classes": {
    "ComputeKernel": {
      "base": "RefCounted",              // nearest engine class (type of Inner)
      // OR, for classes deriving from another manifest class:
      // "extends": "FSLResource",       // generates C# inheritance too
      "abstract": false,                 // true = no parameterless constructor
      "enums": {
        "BufferType": { "Storage": 0, "Uniform": 1 }   // C# name -> value
      },
      "methods": [
        {
          "name": "get_buffer",          // godot-side (snake_case) name
          "csharp": "GetBuffer",         // optional; default is PascalCase(name)
          "static": false,               // optional; static uses ClassCallStatic
          "return": "FSLBuffer",         // optional; C# type, manifest class,
                                         // or enum name; omit for void
          "args": [
            {
              "name": "buffer_name",     // godot-side name
              "csharp": "bufferName",    // optional; default is camelCase(name)
              "type": "StringName",      // C# type, manifest class, or enum
              "default": "null"          // optional; emitted verbatim into the
                                         // C# signature (must be a C# constant)
            }
          ]
        }
      ],
      "properties": [                    // optional; uses Get()/Set()
        { "name": "some_prop", "type": "float" }
      ],
      "signals": [                       // optional; emits SignalName cache only
        { "name": "finished" }
      ]
    }
  }
}
```

Types are written as literal C# type names (`uint`, `string`, `StringName`,
`Callable`, `Variant`, `Image`, `Godot.Collections.Dictionary`, ...), except:

- A **manifest class name** (`FSLBuffer`) marshals through the wrapper:
  arguments pass `.Inner`, returns re-wrap (or return `null`).
- An **enum name** (`BufferType` within its class, `ComputeKernel.BufferType`
  elsewhere) round-trips through `long`.
- `Godot.Collections.*` arguments defaulting to `null` are replaced with an
  empty instance at the call site, since the C++ side rejects nil.
