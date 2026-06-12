# FSL — Fluxxi Shader Language (VSCode)

Syntax highlighting for `.fsl` files, the Fluxxi Shader Language: a compute-shader
language that transpiles to GLSL (see the
[FluxxiShaderLanguage-Godot](https://github.com/Fluxx7/FluxxiShaderLanguage-Godot) project).

## Highlights

- `kernel[x, y, z] name(...)` declarations (keyword, workgroup size, kernel name)
- `layout(std430) buffer` / `layout(rgba32f) uniform image2D` resource declarations
- FSL type aliases (`float2`–`float4`, `int2`–`int4`, `uint…`, `double…`, `bool…`) plus
  GLSL passthrough types (`vec*`, `mat*`, `image2D`, …)
- Parameter specifiers `in` / `out` / `inout` / `const` and name bindings
  (`id : GlobalInvocationID`)
- Preprocessor directives: `#include`, `#define` (incl. function-like macros),
  `#undef`, `#ifdef`, `#else`, `#endif`
- GLSL builtin functions, line/block comments, numeric literals

## Install

### From the .vsix

```sh
code --install-extension fsl-language-0.1.0.vsix
```

(Or in VSCode: Extensions panel → `…` menu → *Install from VSIX…*)

### From source

Copy or symlink this folder into your VSCode extensions directory:

```sh
ln -s "$(pwd)" ~/.vscode/extensions/fluxx7.fsl-language-0.1.0
```

Then reload VSCode.

## Packaging

```sh
npx @vscode/vsce package
```

## Note

The grammar in `syntaxes/fsl.tmLanguage.json` is the single source of truth — the
Rider plugin in `../fsl-rider` bundles this same directory at build time.
