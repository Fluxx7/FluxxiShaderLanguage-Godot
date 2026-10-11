# FSL — Fluxxi Shader Language

A grammar and language definition for FSL. This is a living document: rules
marked **(planned)** are designed but not yet implemented; rules marked
**(undecided)** are open questions. Everything else reflects the current
parser.

## How to read the notation

The grammar is written in EBNF (Extended Backus–Naur Form). Each rule defines
a named *nonterminal* in terms of other rules and literal tokens:

```
rule-name   ::= thing-a thing-b        sequence: a then b
```

| Notation      | Meaning                                        |
|---------------|------------------------------------------------|
| `"kernel"`    | a literal token (exactly this text)            |
| `a b`         | `a` followed by `b`                            |
| `a \| b`      | either `a` or `b`                              |
| `a?`          | zero or one `a` (optional)                     |
| `a*`          | zero or more `a`                               |
| `a+`          | one or more `a`                                |
| `( ... )`     | grouping                                       |

A deliberate property of this grammar, mirroring the implementation: **the
grammar is permissive and the validator restricts.** The parser accepts
anything matching these productions — including programs that are nonsense —
and a separate validation pass enforces the semantic rules listed at the end.
If a construct "parses but shouldn't run," that is by design; look for the
corresponding rule under [Semantic rules](#semantic-rules-the-validators-contract).

---

## 1. Lexical structure

### Character set

Source files are ASCII. Identifiers and all tokens are ASCII; the only place
other bytes may appear is inside comments **(undecided — non-ASCII characters could be allowed in the future, if there is a good reason to add support for that)**.

### Whitespace and newlines

Whitespace separates tokens. Newlines are significant **only** to the
preprocessor (directives are line-based); after preprocessing, all whitespace
and newlines are discarded and carry no meaning. Consequently the grammar
below never mentions line breaks.

### Comments

```
line-comment    ::= "//" (any character)* end-of-line
block-comment   ::= "/*" (any character)* "*/"
```

Comments are removed during preprocessing. An unterminated block comment is
an error.

### Identifiers

```
identifier ::= (letter | "_") (letter | digit | "_")*
```

Keywords fall into two intended groups **(undecided — the exact split is
not final, and the current implementation reserves all of them)**:

| Group             | Words                                          | Intent                  |
|-------------------|------------------------------------------------|-------------------------|
| control flow      | `if` `else` `for` `return`                     | reserved                |
| specifiers        | `const` `in` `out` `inout` `shared`            | reserved                |
| declaration       | `kernel` `buffer` `uniform` `struct`           | contextual (not reserved) |
| opaque types      | `image2D` `image2DArray` `sampler`             | contextual (not reserved) |

*Contextual* keywords only act as keywords in the positions the grammar
shows them (global declaration heads); elsewhere they are ordinary
identifiers, so `u32 index;` is intended to be a legal buffer member. The
current lexer classifies these globally, which reserves them everywhere for
now.

Type names (`f32`, `u32x3`, …) are **not** reserved words: grammatically a
type is just an identifier, and "is this identifier a real type" is a
validator question.

### Literals

```
literal         ::= int-literal | uint-literal | float-literal | bool-literal

int-literal     ::= digit+ | "0x" hex-digit+
uint-literal    ::= int-literal "u"
float-literal   ::= digit+ "." digit* exponent? "f"?     e.g. 1.5, 1., 2.5e-3f
                  | "." digit+ exponent? "f"?            e.g. .5
                  | digit+ exponent "f"?                 e.g. 1e-6
exponent        ::= ("e" | "E") ("+" | "-")? digit+
bool-literal    ::= "true" | "false"

string-literal  ::= '"' (any character except '"')* '"'
```

String literals are not values: they appear only as the resource key of a
buffer declaration (§5) and in `#include` paths.

### Operators and punctuation

Single-character tokens; multi-character operators (`<<`, `+=`, `&&`, …) are
formed from adjacent tokens with no intervening whitespace.

```
arithmetic   +  -  *  /  %
bitwise      ~  &  |  ^  <<  >>
logical      !  &&  ||
comparison   ==  !=  <  >  <=  >=
assignment   =  +=  -=  *=  /=  %=  <<=  >>=  &=  |=  ^=
inc/dec      ++  --
ternary      ?  :
structure    ( ) [ ] { } , ; . :
```

Operator **precedence is not encoded in the grammar** — see
[Operations](#operations-and-expressions). Semantics of every operator follow
GLSL, since FSL transpiles to GLSL compute source.

---

## 2. Preprocessor

Directives are line-based: each begins with `#` and ends at the newline
unless continued with a trailing `\`. The preprocessor runs before parsing;
its output is a flat token stream with comments, whitespace, and newlines
removed.

```
directive       ::= include-directive
                  | define-directive
                  | undef-directive
                  | ifdef-directive

include-directive ::= "#" "include" '"' path '"'
define-directive  ::= "#" "define" identifier macro-params? replacement-tokens
undef-directive   ::= "#" "undef" identifier
ifdef-directive   ::= "#" "ifdef" identifier
                      text
                      ( "#" "else" text )?
                      "#" "endif"

macro-params    ::= "(" identifier ( "," identifier )* ")"
```

Notes:

- `#include` paths are resolved relative to the including file. 
- Function-macros expand their arguments textually, C-style. Expansion is
  recursive: a macro body may use other macros.
- `#ifdef` blocks nest. The skipped branch must still lex, but is never
  parsed.
- There is no `#ifndef` or `#elif`. The include-guard idiom is:
  `#ifdef GUARD` / `#else` / `#define GUARD` … `#endif`.
- Macros may appear anywhere a token sequence may, including kernel dims
  (`kernel[TILE, TILE, 1]`) and array dims (`f32[N * N] xs;`).

---

## 3. Program structure

A file is a sequence of global declarations. Order is free except where the
validator imposes declare-before-use.

```
translation-unit    ::= global-declaration*

global-declaration  ::= resource-decl
                      | struct-decl
                      | shared-decl
                      | function-decl
                      | kernel-decl
                      | variable-decl ";"          global constants
```

---

## 4. Types

A type is written as a single self-delimiting prefix: optional specifiers,
a type name, then zero or more array dimensions. **Array dimensions attach
to the type, not the variable name** (`f32x2[4] xs`, never `f32x2 xs[4]`).

```
type            ::= specifier* identifier array-dim*
specifier       ::= "const" | "in" | "out" | "inout" | "shared"
array-dim       ::= "[" const-expression? "]"      empty brackets = unsized
```

`const-expression` is an operation list (§6) that must be constant; *how*
constant depends on context — see [Constness levels](#constness-levels).

Built-in type names are a validator concern, not a grammar one. A type name
is a primitive, optionally suffixed with a vector width and then a second
matrix dimension:

```
type-name       ::= primitive ( "x" vec-size ( "x" vec-size )? )?
primitive       ::= "f16" | "f32" | "f64"
                  | "u8" | "u16" | "u32" | "u64"
                  | "i8" | "i16" | "i32" | "i64"
                  | "bool"
vec-size        ::= "2" | "3" | "4"
```

So `f32` is a scalar, `f32x3` a 3-vector, `f32x4x4` a 4×4 matrix, `u32x2` a
uint pair. Names declared by a `struct-decl` are type names too.

The opaque types (`image2D`, `image2DArray`, `sampler`) are **not** written
with this production and are legal only in a resource declaration (§5).

---

## 5. Declarations

### Variables

```
variable-decl   ::= type identifier annotation-list? ( "=" operation-list )?

annotation-list ::= ":" annotation ( "," annotation )*
annotation      ::= identifier ( "(" operation-list ")" )?
```

Annotations are compiler hints attached with a trailing colon. Annotation
*names*, argument counts, and legal placement are validated semantically,
not grammatically. Currently defined:

| Annotation                      | Legal on               | Meaning                          |
|---------------------------------|------------------------|----------------------------------|
| `specialization_constant`       | `const` global scalars | SPIR-V specialization constant; optional arg = explicit constant id |
| `godot_vertex`                  | storage buffers        | buffer is a vertex-buffer view   |
| `godot_index`                   | storage buffers        | buffer is an index-buffer view   |
| `flag_indirect`                 | storage buffers        | buffer holds indirect dispatch/draw commands |

Example: `const u32 fft_size : specialization_constant = 256;`

The three buffer annotations are mutually exclusive and illegal on a uniform
buffer; see [Semantic rules](#kernels-resources-and-the-call-graph).

### Shared (workgroup) variables

```
shared-decl ::= "shared" type identifier ";"
```

Declared at global scope; storage is per-workgroup. Multiple kernels in one
file may reference the same shared declaration.

### Functions

```
function-decl   ::= type identifier "(" param-list? ")" annotation-list? scope-block
param-list      ::= param ( "," param )*
param           ::= type identifier
```

Parameter direction uses the `in` / `out` / `inout` specifiers on the
parameter's type (default `in`). Functions may be overloaded by parameter
types. Recursion is forbidden (validator, via call graph).

### Kernels

```
kernel-decl     ::= "kernel" "[" operation-list "," operation-list "," operation-list "]"
                    identifier "(" kernel-param-list? ")" scope-block

kernel-param-list ::= kernel-param ( "," kernel-param )*
kernel-param      ::= identifier ":" builtin-binding      builtin value binding
                    | type identifier                     push constant

builtin-binding ::= "GlobalInvocationID" | "LocalInvocationID"
                  | "LocalInvocationIndex" | "WorkGroupID"
                  | "WorkGroupSize" | "NumWorkGroups"
```

The three bracketed operation lists are the local workgroup dimensions
(x, y, z); they must be comptime or pipeline constants. Builtin bindings and
push constants may be interleaved in any order. Push-constant layout on the
GPU side follows declaration order.

### Resources

Every resource is declared with the resource *key* it is bound by on the
Godot side, and carries no explicit `layout(...)`: the memory layout follows
from the kind of resource (storage blocks are `std430`, uniform blocks are
`std140`), and image formats are written as a type argument.

```
resource-decl   ::= buffer-decl | image-decl | sampler-decl

buffer-decl     ::= "uniform"? "buffer" "[" string-literal "]" identifier?
                    annotation-list? "{" buffer-member* "}" ";"
buffer-member   ::= type identifier ";"

image-decl      ::= "uniform" image-type "<" image-format ">" identifier ";"
image-type      ::= "image2D" | "image2DArray"
image-format    ::= "f16x4" | "f32x4"

sampler-decl    ::= "uniform" "sampler" identifier ";"
```

- The string literal in brackets is the resource's **key**: the name the
  host binds against, and the name the validator uses for duplicate
  detection. It is independent of any identifier in the declaration, so two
  files may use different instance names for one bound resource.
- A leading `uniform` makes the block a uniform buffer (`std140`); without
  it the block is a storage buffer (`std430`).
- The optional identifier before the block is the **instance name**. If it
  is present, members are accessed through it (`params.count`); otherwise
  members are accessed directly by name (`data[i]`).
- An **unsized** array member (`f32[] xs;`) is only legal as the *last*
  member of a buffer block (validator).
- `godot_vertex` / `godot_index` annotations declare vertex- and
  index-buffer views; their member rules are **(undecided)** — the stress
  suite (v05) assumes a block of loose per-vertex fields is legal, and the
  sample shaders use a single unsized array of a struct type.
- An image's format argument is spelled as the vector type of one texel, so
  `image2DArray<f16x4>` is the old `layout(rgba16f) uniform image2DArray`.
  Only the two four-channel float formats exist today.
- A `sampler` is declared on its own and combined with an image at the use
  site: `mySampler(myImage)[coords]`. **(undecided)** whether a sampler may
  be constrained to one image type at its declaration — the AST carries an
  optional image type for it, but no syntax is settled.

Example:

```
uniform buffer["CameraBuffer"] cam {
    f32x3 camera_pos;
    f32 proj_scale;
};

buffer["VertexBuffer"] : godot_vertex {
    Vertex[] vertices;
};

uniform image2DArray<f16x4> heightTexture;
uniform sampler oceanSampler;
```

### Structs

```
struct-decl     ::= "struct" identifier "{" struct-member* "}" ";"
struct-member   ::= type identifier ";"
```

A struct name becomes a type name usable anywhere a type is, including as a
buffer member (`Vertex[] vertices;`).

---

## 6. Operations and expressions

This is the deliberately permissive core of the grammar. An **operation
list** is a flat, ordered sequence of operations with **no grammatical
enforcement of infix structure or precedence**:

```
operation-list  ::= operation*

operation       ::= variable-decl            only meaningful list-initially (validator)
                  | operator                 any operator token sequence
                  | function-call
                  | variable-ref
                  | field-access
                  | array-index
                  | literal
                  | "(" operation-list ")"   grouping / sub-list

function-call   ::= identifier "(" ( operation-list ( "," operation-list )* )? ")"
variable-ref    ::= identifier
field-access    ::= "." identifier           swizzles are field accesses
array-index     ::= "[" operation-list "]"
```

The parser records operations in written order and moves on. **The validator
owns the structural rules**: operands must alternate sensibly with binary
operators, a declaration may only open a list, an `=` needs an assignable
place on its left, and so on. This keeps the parser simple and lets error
messages come from one place with full context. Two consecutive binary
operators (`1.0 + * 2.0`), a literal adjacent to a declaration (the classic
missing-semicolon shape), or a bare type name used as a value all *parse*
and are then rejected in validation.

Precedence and associativity are GLSL's, applied during validation/codegen
rather than encoded in parse-tree shape.

---

## 7. Statements

```
scope-block     ::= "{" statement* "}"

statement       ::= scope-block
                  | if-statement
                  | for-statement
                  | return-statement
                  | expression-statement

if-statement    ::= "if" "(" operation-list ")" statement else-clause?
else-clause     ::= "else" statement

for-statement   ::= "for" "(" operation-list ";" operation-list ";" operation-list ")"
                    statement

return-statement     ::= "return" operation-list? ";"
expression-statement ::= operation-list ";"
```

- Branch and loop bodies may be a single statement (no braces); a dangling
  `else` binds to the nearest unmatched `if`.
- The `for` init clause opens a scope containing the body: a variable
  declared there is invisible after the loop, so sibling loops may reuse
  the same counter name.
- An `else` is only legal immediately following an `if`'s statement
  (validator — a stray `else` parses).
- **(undecided)** `while`, `do`/`while`, `break`, `continue`, `switch`. The
  stress suite (v10) assumes `while`/`break`/`continue` will exist.

---

## 8. Semantic rules (the validator's contract)

Everything in this section parses successfully and is rejected — or will be,
once the corresponding check exists — by the validation pass.

### Names and scoping

1. Every identifier used as a value must resolve to a declared variable,
   parameter, buffer member, or builtin in a visible scope.
2. No redeclaration of a name within the same scope. Shadowing in a nested
   scope is forbidden **(undecided - could be allowed in the future if it seems like a good idea)**.
3. Type names are identifiers: using a type name as a value, or an unknown
   identifier as a type, is a validation error.
4. Kernels, functions (per overload signature), and resources each occupy a
   namespace; duplicate kernel names and duplicate resource keys are errors.
   A buffer's key, its instance name, and its members are three separate
   things: an anonymous block's members join the global value namespace and
   collide with globals of the same name, while a named block's members are
   reachable only through the instance name.
5. Declare-before-use in scopes, no ordering enforced globally. **(planned —
   need to figure out how to represent this when transpiling to GLSL)**.

### Operation-list structure

6. Operands and binary operators must alternate; unary operators must prefix
   an operand; `++`/`--` attach to an adjacent assignable place.
7. A variable declaration may only appear at the start of an operation list
   (statement position), optionally followed by `=` and an initializer.
8. The left side of any assignment operator must be an assignable place
   (variable, field/swizzle of one, or array element) — not a literal,
   call result, or `const`.
9. `out`/`inout` arguments must be assignable places at the call site.

### Types

10. Expression types are synthesized bottom-up with GLSL conversion rules;
    operator operand types must be compatible (no `f32x2 + f32x3`).
11. Initializer and assignment types must match the declared type.
12. `return` expressions must match the function's return type; non-void
    functions must return on every control path; `void` calls cannot be
    used as values.
13. Calls must match exactly one visible overload.
14. Swizzle components must be valid for the vector's width.

### Constness levels

Every expression has a constness level; contexts set a minimum:

| Level      | Meaning                          | Example source                    |
|------------|----------------------------------|-----------------------------------|
| comptime   | known at compile time            | literals, macros, `const` folding |
| pipeline   | fixed at pipeline creation       | specialization constants          |
| runtime    | known only on the GPU            | push constants, buffer reads      |

| Context                      | Required level        |
|------------------------------|-----------------------|
| buffer member array dims     | comptime              |
| shared array dims            | comptime or pipeline  |
| kernel local size            | comptime or pipeline  |
| local array dims             | comptime **(undecided: allow pipeline?)** |
| annotation arguments         | comptime              |

Unsized dims (`[]`) are legal only as the final buffer member.

### Kernels, resources, and the call graph

15. The validator builds the file's call graph once; recursion (direct or
    mutual) is an error.
16. A kernel's bound resource set is its directly referenced resources plus
    everything reachable through called functions (transitive propagation).
    Bindings are per-kernel, set 0, in declaration order over that used set.
17. `barrier()` and shared-memory access are only meaningful in kernel
    context (reached-from-kernel functions included) **(undecided: error or
    ignore in never-called functions)**.
18. `godot_vertex`, `godot_index`, and `flag_indirect` are mutually
    exclusive — a buffer may carry at most one — and none of them may appear
    on a uniform buffer.

### Annotations

19. Annotation names must be known; placement must be legal for that
    annotation (e.g. `specialization_constant` only on `const` global
    scalars); argument counts and types must match.

---

## 9. Known divergences of the current implementation

Tracked here so the document can describe the *intended* language above.

- Array constructor expressions (`f32x2[4]( ... )`) are unimplemented.
- The validator implements only a subset of §8 so far.
- The resource rework of §5 is mid-flight. The parser still dispatches on
  the legacy `layout(buffer-format) buffer name { … } inst;` and
  `layout(texture-format) uniform image-type name;` forms, and reaches
  resource parsing only from a leading `layout` or `uniform`, so the
  `buffer["key"] …` head is not parsed yet. The legacy forms should be
  removed once the new ones land.
- Bare uniform variables (`uniform f32 scale;`) were part of an earlier
  design and are gone: `UniformDef` no longer exists in the AST. A scalar
  that used to be one belongs in a uniform buffer or a push constant.
- Sampler declarations parse, but `_parse_sampler` currently expects an
  image type where the name should be, so the `uniform sampler name;` form
  in the sample shaders does not round-trip yet.
- `struct`, `switch`/`case`, `while`, `break`, and `continue` have tokens
  and AST nodes ahead of the sections above that still call them
  **(undecided)**.
