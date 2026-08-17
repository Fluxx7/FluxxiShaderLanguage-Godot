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
| declaration       | `kernel` `layout` `buffer` `uniform`           | contextual (not reserved) |
| buffer formats    | `std140` `std430` `vertex` `index`             | contextual (not reserved) |
| texture formats   | `rgba16f` `rgba32f`                            | contextual (not reserved) |
| image types       | `image2D` `image2DArray`                       | contextual (not reserved) |

*Contextual* keywords only act as keywords in the positions the grammar
shows them (global declaration heads, inside `layout(...)`); elsewhere they
are ordinary identifiers, so `uint index;` is intended to be a legal buffer
member. The current lexer classifies these globally, which reserves them
everywhere for now.

Type names (`float`, `uint3`, …) are **not** reserved words: grammatically a
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
```

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
  (`kernel[TILE, TILE, 1]`) and array dims (`float[N * N] xs;`).

---

## 3. Program structure

A file is a sequence of global declarations. Order is free except where the
validator imposes declare-before-use.

```
translation-unit    ::= global-declaration*

global-declaration  ::= resource-decl
                      | shared-decl
                      | function-decl
                      | kernel-decl
                      | variable-decl ";"          global constants
```

---

## 4. Types

A type is written as a single self-delimiting prefix: optional specifiers,
a type name, then zero or more array dimensions. **Array dimensions attach
to the type, not the variable name** (`float2[4] xs`, never `float2 xs[4]`).

```
type            ::= specifier* identifier array-dim*
specifier       ::= "const" | "in" | "out" | "inout" | "shared"
array-dim       ::= "[" const-expression? "]"      empty brackets = unsized
```

`const-expression` is an operation list (§6) that must be constant; *how*
constant depends on context — see [Constness levels](#constness-levels).

Built-in type names (a validator concern, not a grammar one):

```
float  double  int  uint  bool 
float2 float3 float4   double2 double3 double4   int2 int3 int4   uint2 uint3 uint4   bool2 bool3 bool4
float2x2 float2x3 float2x4  float3x2 float3x3 float3x4  float4x2 float4x3 float4x4 
double2x2 double2x3 double2x4  double3x2 double3x3 double3x4  double4x2 double4x3 double4x4 
```

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

Example: `const uint fft_size : specialization_constant = 256;`

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

```
resource-decl   ::= buffer-decl | image-decl | uniform-decl

buffer-decl     ::= "layout" "(" buffer-format ")" "buffer" identifier
                    "{" buffer-member* "}" identifier? ";"
buffer-format   ::= "std140" | "std430" | "vertex" | "index"
buffer-member   ::= type identifier ";"

image-decl      ::= "layout" "(" texture-format ")" "uniform" image-type identifier ";"
texture-format  ::= "rgba16f" | "rgba32f"
image-type      ::= "image2D" | "image2DArray"

uniform-decl    ::= "uniform" type identifier ";"          (planned)
```

- If the trailing identifier after a buffer's `}` is present, members are
  accessed through it (`params.count`); otherwise members are accessed
  directly by name (`data[i]`).
- An **unsized** array member (`float[] xs;`) is only legal as the *last*
  member of a buffer block (validator).
- `vertex` / `index` layouts declare vertex- and index-buffer views;
  their member rules are **(undecided)**.

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
   namespace; duplicate kernel names and duplicate resource names are errors.
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
    operator operand types must be compatible (no `float2 + float3`).
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

### Annotations

18. Annotation names must be known; placement must be legal for that
    annotation (e.g. `specialization_constant` only on `const` global
    scalars); argument counts and types must match.

---

## 9. Known divergences of the current implementation

Tracked here so the document can describe the *intended* language above.

- Array constructor expressions (`float2[4]( ... )`) are unimplemented.
- The validator implements only a subset of §8 so far.
