# FSL — Fluxxi Shader Language (VSCode)

Syntax highlighting for `.fsl` files, the Fluxxi Shader Language: a compute-shader
language that transpiles to GLSL (see the
[FluxxiShaderLanguage-Godot](https://github.com/Fluxx7/FluxxiShaderLanguage-Godot) project).

## Highlights

- `kernel[x, y, z] name(...)` declarations (keyword, workgroup size, kernel name)
- `layout(std430) buffer` / `layout(rgba32f) uniform image2D` resource declarations
- FSL types: scalars (`f32`, `u32`, `i32`, `f64`, `bool`, and the other bit widths),
  vectors (`f32x2`–`f32x4`, `u32x3`, …), matrices (`f32x4x4`, …), plus opaque
  image/sampler types (`image2D`, `image2DArray`, …)
- `struct` declarations, plus a PascalCase heuristic so struct names also
  highlight at their use sites (TextMate grammars have no symbol table)
- Trailing-colon annotations: `: specialization_constant(id)` on constants,
  `: godot_vertex` / `: godot_index` / `: flag_indirect` on buffers
- Parameter specifiers `in` / `out` / `inout` / `const` / `shared` and all six
  builtin bindings (`id : GlobalInvocationID`, `LocalInvocationID`,
  `LocalInvocationIndex`, `WorkGroupID`, `WorkGroupSize`, `NumWorkGroups`)
- Control flow incl. `while` / `do` / `switch` / `break` / `continue`
- Preprocessor directives: `#include`, `#define` (incl. function-like macros),
  `#undef`, `#ifdef`, `#else`, `#endif`
- GLSL builtin functions, line/block comments, numeric literals

## Install

### From the .vsix

```sh
code --install-extension fsl-language-0.2.1.vsix
```

(Or in VSCode: Extensions panel → `…` menu → *Install from VSIX…*)

### From source

Copy or symlink this folder into your VSCode extensions directory:

```sh
ln -s "$(pwd)" ~/.vscode/extensions/fluxx7.fsl-language-0.2.1
```

Then reload VSCode.

## Packaging

```sh
npx @vscode/vsce package
```

## Note

The grammar in `syntaxes/fsl.tmLanguage.json` is the single source of truth — the
Rider plugin in `../fsl-rider` bundles this same directory at build time.
