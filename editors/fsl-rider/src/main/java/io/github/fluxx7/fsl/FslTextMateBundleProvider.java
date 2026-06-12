package io.github.fluxx7.fsl;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.plugins.textmate.api.TextMateBundleProvider;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Collections;
import java.util.List;

/**
 * Supplies the FSL TextMate bundle (VSCode format, shared with editors/fsl-vscode)
 * to the platform TextMate plugin. Resources live inside the plugin jar, so they
 * are extracted to a temporary directory the TextMate engine can read from.
 */
public class FslTextMateBundleProvider implements TextMateBundleProvider {
    private static final List<String> BUNDLE_FILES = List.of(
            "package.json",
            "language-configuration.json",
            "syntaxes/fsl.tmLanguage.json"
    );

    @Override
    public @NotNull List<PluginBundle> getBundles() {
        try {
            Path bundleDir = Files.createTempDirectory("fsl-textmate-bundle");
            for (String relativePath : BUNDLE_FILES) {
                Path target = bundleDir.resolve(relativePath);
                Files.createDirectories(target.getParent());
                try (InputStream resource = getClass().getClassLoader()
                        .getResourceAsStream("fsl-bundle/" + relativePath)) {
                    if (resource == null) {
                        return Collections.emptyList();
                    }
                    Files.copy(resource, target, StandardCopyOption.REPLACE_EXISTING);
                }
            }
            return List.of(new PluginBundle("FSL", bundleDir));
        } catch (IOException e) {
            return Collections.emptyList();
        }
    }
}
