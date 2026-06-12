# Fluxxi Shader Language

Fluxxi Shader Language (FSL) is a semi-custom compute shader language for Godot written as a GDExtension. It is based off of GLSL, and adds support for multiple kernels in one file and shader includes. Some naming conventions are also changed from GLSL (listed below). 

The FSLComputeShaderHandling library is included as well, which abstracts compilation, resource initialization/updating, compute list construction, and shader dispatch to make using compute shaders easier. Standard `.glsl` shaders are supported as well, although they cannot use all of the features without some additional code to pass information to the compute shader handler. 

# FSL features

## Shader Language
- Core type changes
  - all `vec` types are renamed 
    - `vecn` -> `floatn`
    - `ivecn` -> `intn`
    - `uvecn` -> `uintn`
    - `bvecn` -> `booln`
    - `dvecn` -> `doublen`
  - all `mat` types are renamed
    - `matnxm` -> `floatnxm`
    - `dmatnxm` -> `doublenxm`
    - Note: due to the new syntax, the shorthand `matn` syntax has been removed 
  - literals are no longer typed, ie. `float foo = 0;` is valid
- Added directive to include definitions in other `.fsl` files
  - Syntax: `#include "<file path>"`
- Added keyword to declare a kernel function
  - Syntax: `kernel[x_threads, y_threads, z_threads] kernel_name()`
  - Push constants for a given kernel are specified as arguments to the kernel function
    - ie. `kernel[16, 1, 1] foo(int bar, float baz)` would declare `bar` and `baz` as push constants for the `foo` kernel
    - Invocation/workgroup ids can have an alias declared in the kernel declaration as well
      - Syntax: `kernel_name(id : GlobalInvocationID)`
- All other GLSL features are supported

## Godot
- Adds a new `FSLFile` class that automatically prepares valid GLSL compute shader code for Godot from a provided FSL file

# Editor support

Syntax highlighting for `.fsl` files lives in [editors/](editors/):
- [editors/fsl-vscode](editors/fsl-vscode) — VSCode extension (TextMate grammar; this is the single source of truth for the grammar)
- [editors/fsl-rider](editors/fsl-rider) — Rider/JetBrains plugin that bundles the same grammar

See each folder's README for build/install instructions, and
[editors/MAINTAINING.md](editors/MAINTAINING.md) for how to extend the grammar and
customize highlighting.

# FSLComputeShaderHandling library
## ComputeShader
- 
## ComputePlan
- 