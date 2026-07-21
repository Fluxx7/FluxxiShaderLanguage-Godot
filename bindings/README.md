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

Every hierarchy root gets a static `Wrap(GodotObject)` factory that inspects
the live object's `get_class()` and constructs the most-derived wrapper
declared in the manifest. All class-typed returns go through it, so a method
declared to return a base class (`FSLResource`) yields a wrapper whose C# type
matches the object's real Godot class — `is`/`as` downcasts work as expected.
Objects of classes not in the manifest fall back to the declared hierarchy
root's wrapper. Note that `Wrap` constructs a fresh wrapper per call: two
lookups of the same object are distinct C# instances, so compare
`a.Inner == b.Inner`, never wrapper references.

## Manifest schema

```jsonc
{
  "namespace": "FluxxiShaderLang",       // namespace for all generated classes
  "engine_enums": [                      // engine enum types used in signatures;
    "RenderingDevice.UniformType"        // they round-trip through long like
  ],                                     // declared enums
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
  arguments pass `.Inner`, returns re-wrap via the root's `Wrap` factory to
  the object's actual class (or return `null`).
- An **enum name** (`BufferType` within its class, `ComputeKernel.BufferType`
  elsewhere) round-trips through `long`.
- `Godot.Collections.*` arguments defaulting to `null` are replaced with an
  empty instance at the call site, since the C++ side rejects nil.
