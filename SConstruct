#!/usr/bin/env python
import os
import sys

from methods import print_error


libname = "fluxxishaderlang"
projectdir = "project"

localEnv = Environment(tools=["default"], PLATFORM="")

# Build profiles can be used to decrease compile times.
# You can either specify "disabled_classes", OR
# explicitly specify "enabled_classes" which disables all other classes.
# Modify the example file as needed and uncomment the line below or
# manually specify the build_profile parameter when running SCons.

# localEnv["build_profile"] = "build_profile.json"

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()

if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

# Recursively collect source directories under src/ so headers can be included
# with paths relative to any of them, and gather all .cpp files for compilation.
# src/gen is excluded here because its generated doc_data is appended separately.
source_dirs = [root for root, _, _ in os.walk("src") if "gen" not in root.split(os.sep)]
env.Append(CPPPATH=source_dirs)

sources = []
for directory in source_dirs:
    sources += Glob("{}/*.cpp".format(directory))

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

# .dev doesn't inhibit compatibility, so we don't need to key it.
# .universal just means "compatible with all relevant arches" so we don't need to key it.
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(env.subst('$SHLIBPREFIX'), libname, suffix, env.subst('$SHLIBSUFFIX'))

library = env.SharedLibrary(
    "bin/{}/{}".format(env['platform'], lib_filename),
    source=sources,
)

copy = env.Install("{}/bin/{}/".format(projectdir, env["platform"]), library)

default_args = [library, copy]

# Always regenerate compile_commands.json as part of the default build so VSCode
# IntelliSense stays current (new .cpp files get include paths automatically),
# even on a plain `scons` invocation without compiledb=yes. godot-cpp registers
# the "compiledb" target (the compilation_db tool + CompilationDatabase node) on
# our env unconditionally; here we just opt it into Default.
default_args += ["compiledb"]

Default(*default_args)
