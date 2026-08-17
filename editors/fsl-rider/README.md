# FSL — Fluxxi Shader Language (Rider / JetBrains)

A JetBrains plugin that adds syntax highlighting for `.fsl` files in Rider
(and any other JetBrains IDE). It ships the same TextMate grammar as the
VSCode extension in [`../fsl-vscode`](../fsl-vscode) — that folder is the
single source of truth and is copied into the plugin at build time.

## Build

```sh
./gradlew buildPlugin
```

The installable plugin lands in `build/distributions/fsl-rider-0.2.1.zip`.

The first build downloads the IntelliJ Platform SDK (~1 GB), so it takes a while.

## Install in Rider

1. **Settings → Plugins → ⚙ → Install Plugin from Disk…**
2. Pick `build/distributions/fsl-rider-0.2.1.zip`
3. Restart the IDE. `.fsl` files are now highlighted.

## Zero-build alternative

If you don't want to build the plugin at all, Rider can load the VSCode
extension folder directly as a TextMate bundle:

1. **Settings → Editor → TextMate Bundles**
2. Press **+** and select the `editors/fsl-vscode` directory
3. Apply — same highlighting, no plugin needed.

(The plugin is just a redistributable wrapper around exactly that mechanism,
via the `com.intellij.textmate.bundleProvider` extension point.)
