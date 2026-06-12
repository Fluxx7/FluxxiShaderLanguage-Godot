package io.github.fluxx7.fsl;

import com.intellij.ide.FileIconProvider;
import com.intellij.openapi.project.DumbAware;
import com.intellij.openapi.project.Project;
import com.intellij.openapi.util.IconLoader;
import com.intellij.openapi.vfs.VirtualFile;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import javax.swing.Icon;

/**
 * Supplies the file icon for .fsl files. An icon-only provider is used (rather than
 * registering a FileType) so the files stay unclaimed and the TextMate bundle keeps
 * handling their syntax highlighting.
 */
public class FslFileIconProvider implements FileIconProvider, DumbAware {
    private static final Icon FSL_ICON =
            IconLoader.getIcon("/icons/fsl.svg", FslFileIconProvider.class);

    @Override
    public @Nullable Icon getIcon(@NotNull VirtualFile file, int flags, @Nullable Project project) {
        return "fsl".equalsIgnoreCase(file.getExtension()) ? FSL_ICON : null;
    }
}
