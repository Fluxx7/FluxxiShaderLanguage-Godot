# Maintaining the FSL editor extensions

Everything here revolves around one file:

```
editors/fsl-vscode/syntaxes/fsl.tmLanguage.json   ← the grammar (single source of truth)
```

Both extensions are thin wrappers around it:

- **VSCode** (`fsl-vscode/`) uses the grammar directly. `package.json` registers the
  `.fsl` extension; `language-configuration.json` handles comments/brackets/auto-indent.
- **Rider** (`fsl-rider/`) copies the whole `fsl-vscode` folder into its plugin jar at
  build time (the `processResources` block in `build.gradle.kts`) and hands it to the
  JetBrains TextMate engine via one Java class. You never edit the grammar there.

So the workflow for any syntax change is always: **edit the grammar → test → rebuild
both packages.**

---

## 1. Changing or adding syntax

The grammar is a TextMate grammar: a list of regex rules that assign **scopes**
(names like `keyword.control.fsl`) to spans of text. Colors come later — themes
map scopes to colors (section 3).

Structure of the file:

- `"patterns"` (top of file) — the ordered list of rule groups. **First match wins**,
  so order matters: e.g. `#builtin-functions` is listed before `#function-call` so
  `cos(` becomes a builtin instead of a generic function call.
- `"repository"` — the named rule groups themselves. This is where you edit.

### Common edits

**Add a keyword** (e.g. the parser learns `struct`): find the relevant rule in the
repository and extend the alternation:

```json
"match": "\\b(if|else|for|while|do|switch|case|default|break|continue|return|discard)\\b"
```

**Add a type**: the `types` rule has two patterns — FSL aliases and GLSL passthrough.
The matrix aliases from the main README (`floatNxM`, `doubleNxM`) are already in there,
ahead of the parser supporting them.

**Add a builtin function**: extend the big alternation in `builtin-functions`. Note it
ends with `(?=\\()` — a lookahead so the name only counts as a builtin when called.

**Add a new construct** (something with internal structure, like `kernel[...] name`):
add a rule with `captures`, where each capture group gets its own scope. The
`kernel-declaration` rule is the template to copy:

```json
{
  "match": "\\b(kernel)\\s*(\\[)([^\\]]*)(\\])\\s*([A-Za-z_]\\w*)?",
  "captures": {
    "1": { "name": "keyword.other.kernel.fsl" },
    "3": { "patterns": [{ "include": "#numbers" }] },
    "5": { "name": "entity.name.function.kernel.fsl" }
  }
}
```

For multi-line constructs use `begin`/`end` instead of `match` (see the `comments`
or `preprocessor` rules).

### Regex gotchas

- Patterns are **Oniguruma** regexes inside JSON strings, so every backslash is
  doubled: word boundary is `\\b`, a literal `[` is `\\[`.
- Always wrap keyword alternations in `\\b...\\b`, otherwise `in` matches inside
  `inout` and `uint`.
- A `match` rule must stay on one line of source; only `begin`/`end` rules span lines.

### Scope naming

Stick to the [standard scope names](https://macromates.com/manual/en/language_grammars#naming_conventions)
(`keyword.control`, `storage.type`, `entity.name.function`, `constant.numeric`,
`support.function`, `comment.line`, …) and just append `.fsl`. Themes only know the
standard names — an invented scope like `fsl.mycoolthing` renders as plain text in
every theme.

---

## 2. Testing a grammar change

### Quick automated check

```sh
cd editors
npm install --no-save vscode-textmate vscode-oniguruma   # first time only
node test-grammar.js                          # tokenizes every project/fsl/**.fsl file
node test-grammar.js ../project/fsl/spectrums.fsl   # prints every token + its scopes
```

This runs the same engine VSCode uses, so regex errors and wrong scopes show up
immediately. The verbose mode is the fastest way to answer "what scope did my new
rule actually produce?"

### Visual check in VSCode

Open a `.fsl` file and run **Developer: Inspect Editor Tokens and Scopes** from the
command palette. Click any token to see its scope stack and which theme rule colored
it. This is the main debugging tool for "why is this the wrong color?"

### Picking up changes in each editor

| Editor | How to see your change |
|---|---|
| VSCode (symlinked/dev install) | `Developer: Reload Window` |
| VSCode (.vsix install) | repackage + reinstall (below) |
| Rider (plugin install) | rebuild + reinstall plugin (below) |
| Rider (TextMate-bundle setup) | Settings → Editor → TextMate Bundles → re-tick the bundle (or restart) |

---

## 3. Controlling the colors

The grammar never specifies colors — it assigns **scopes**, and the active color
theme decides what each scope looks like. So there are two levers:

### Lever 1: change which scope a rule emits (affects all users)

If kernel names should look like types instead of functions, change
`entity.name.function.kernel.fsl` to a different standard scope in the grammar.
Scopes currently used, and what themes typically do with them:

| Scope | Used for | Typical theme color |
|---|---|---|
| `keyword.control.fsl` | `if` `for` `return` … | purple/pink |
| `keyword.other.kernel.fsl` / `.layout.fsl` | `kernel`, `layout` | purple/pink |
| `keyword.control.directive.*` | `#include` `#define` `#ifdef` … | purple/blue |
| `storage.type.fsl` | `float2` `uint` `vec3` … | blue/teal |
| `storage.modifier.fsl` / `.resource.fsl` | `in` `out` `inout` `const`, `uniform` `buffer` | blue italic |
| `entity.name.function.kernel.fsl` | kernel names | yellow |
| `entity.name.function.fsl` | function definitions/calls | yellow |
| `entity.name.function.preprocessor.fsl` | macro names | yellow |
| `support.function.builtin.fsl` | `cos` `imageStore` … | teal/cyan |
| `support.variable.builtin.fsl` | `GlobalInvocationID`, `gl_*` | red/orange |
| `variable.parameter.fsl` | bound name in `id : GlobalInvocationID` | orange/italic |
| `constant.numeric.*` | numbers | orange/green |
| `constant.language.*` | `true` `false` `std430` `rgba32f` | orange |
| `constant.other.macro.fsl` | ALL_CAPS identifiers (`PI`) | orange |
| `comment.line.*` / `comment.block.*` | comments | gray/green |
| `string.quoted.double.include.fsl` | `#include` paths | green/orange |

### Lever 2: override colors per scope (affects just you)

**VSCode** — `settings.json`:

```json
"editor.tokenColorCustomizations": {
  "textMateRules": [
    { "scope": "entity.name.function.kernel.fsl",
      "settings": { "foreground": "#ffd700", "fontStyle": "bold" } },
    { "scope": "support.variable.builtin.fsl",
      "settings": { "foreground": "#ff6b6b" } }
  ]
}
```

A `.fsl`-suffixed scope only affects FSL; a bare scope like `storage.type` would
affect every language.

**Rider** — JetBrains maps TextMate scopes onto its own color-scheme attributes
automatically, so FSL follows whatever scheme is active. To tweak: **Settings →
Editor → Color Scheme → Language Defaults** (the TextMate mapping draws from these
entries — Keyword, String, Number, Function declaration, …). There is no per-scope
override UI like VSCode's; changing Language Defaults affects all TextMate-highlighted
languages. If you ever want truly FSL-specific color settings in Rider, that requires
graduating from the TextMate bundle to a real custom-language plugin (lexer +
SyntaxHighlighter) — much more code, only worth it if you also want completion,
go-to-definition, etc.

---

## 4. Rebuilding and reinstalling

### VSCode

```sh
cd editors/fsl-vscode
npx @vscode/vsce package                                    # → fsl-language-X.Y.Z.vsix
code --install-extension fsl-language-X.Y.Z.vsix
```

For grammar iteration, skip packaging entirely: symlink the folder once
(`ln -s "$(pwd)" ~/.vscode/extensions/fluxx7.fsl-language-dev`) and just reload the
window after each edit.

### Rider

```sh
cd editors/fsl-rider
./gradlew buildPlugin        # → build/distributions/fsl-rider-X.Y.Z.zip
```

Then **Settings → Plugins → ⚙ → Install Plugin from Disk…** and restart. The build
re-copies the grammar from `fsl-vscode` every time, so there is nothing to sync
manually.

Build-environment notes (already configured, just don't downgrade them):
- The Gradle wrapper is 9.5.1 — `org.jetbrains.intellij.platform` 2.16+ requires Gradle 9.
- The Java toolchain is 21 (required by platform 2024.2+); the foojay-resolver plugin
  in `settings.gradle.kts` auto-downloads it if missing — keep it ≥ 1.0.0 on Gradle 9.
- First build on a fresh machine downloads the IntelliJ SDK (~1 GB); later builds are fast.

### Version bumps

When releasing a new version, update the version string in **three** places:
`fsl-vscode/package.json`, `fsl-rider/build.gradle.kts` (`version = ...`), and the
install command/zip name in the READMEs if you mention them.

---

## 5. Other maintenance

- **New file extension** (say `.fsli`): add it to `contributes.languages[0].extensions`
  in `fsl-vscode/package.json`, then rebuild both.
- **Comment/bracket behavior** (auto-close, comment toggling, indent-on-brace):
  `fsl-vscode/language-configuration.json`. Rider reads the same file for comment
  toggling via the bundle.
- **Rider plugin metadata** (name, description, vendor): 
  `fsl-rider/src/main/resources/META-INF/plugin.xml`.
- **Adding files to the Rider bundle**: if `fsl-vscode` ever grows files the bundle
  needs (e.g. a snippets file), add them in two places — the `include(...)` list in
  `fsl-rider/build.gradle.kts` *and* `BUNDLE_FILES` in `FslTextMateBundleProvider.java`
  (that class extracts each listed file from the jar at runtime).
