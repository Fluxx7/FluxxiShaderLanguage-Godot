plugins {
    id("java")
    id("org.jetbrains.intellij.platform") version "2.16.0"
}

group = "io.github.fluxx7"
version = "0.2.1"

repositories {
    mavenCentral()
    intellijPlatform {
        defaultRepositories()
    }
}

dependencies {
    intellijPlatform {
        // Compile against IntelliJ Community; the plugin only uses the platform
        // TextMate plugin, which is bundled in Rider (and every other JetBrains IDE).
        create("IC", "2024.2.5")
        bundledPlugin("org.jetbrains.plugins.textmate")
    }
}

java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(21)
    }
}

intellijPlatform {
    buildSearchableOptions = false
    pluginConfiguration {
        ideaVersion {
            sinceBuild = "242"
            untilBuild = provider { null }
        }
    }
}

// The grammar lives in ../fsl-vscode (single source of truth); the JetBrains
// TextMate engine reads VSCode-format bundles directly, so bundle that folder.
tasks.processResources {
    from(layout.projectDirectory.dir("../fsl-vscode")) {
        include("package.json", "language-configuration.json", "syntaxes/**")
        into("fsl-bundle")
    }
}
