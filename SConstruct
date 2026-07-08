#!/usr/bin/env python
import os
import sys

from methods import print_error


libname = "fluxxishaderlang"
projectdir = "project"
# The addon folder is the distributable unit: symlink or copy
# project/addons/fluxxishaderlang into any consuming Godot project.
addondir = "{}/addons/{}".format(projectdir, libname)

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

copy = env.Install("{}/bin/{}/".format(addondir, env["platform"]), library)

# Generate C# wrapper classes for the bound GDExtension API from the manifest.
# The manifest (not ClassDB reflection) is the source of truth, so names and
# type signatures come out exactly as declared. Outputs ship inside the addon.
import json

csharp_manifest = "bindings/csharp_manifest.json"
csharp_outdir = "{}/csharp".format(addondir)
with open(csharp_manifest) as manifest_file:
    csharp_classes = list(json.load(manifest_file)["classes"].keys())
csharp_sources = env.Command(
    ["{}/{}.cs".format(csharp_outdir, cls) for cls in csharp_classes],
    [csharp_manifest, "bindings/generate_csharp_bindings.py"],
    '"{}" bindings/generate_csharp_bindings.py --manifest {} --out {}'.format(
        sys.executable, csharp_manifest, csharp_outdir
    ),
)

default_args = [library, copy, csharp_sources]

# Always regenerate compile_commands.json as part of the default build so VSCode
# IntelliSense stays current (new .cpp files get include paths automatically),
# even on a plain `scons` invocation without compiledb=yes. godot-cpp registers
# the "compiledb" target (the compilation_db tool + CompilationDatabase node) on
# our env unconditionally; here we just opt it into Default.
default_args += ["compiledb"]

Default(*default_args)
