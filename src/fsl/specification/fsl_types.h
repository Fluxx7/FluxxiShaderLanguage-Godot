#pragma once
#include "godot_imports.h"
#include "std_imports.h"

inline constexpr const char* fsl_primitives[] = {
    "float",
    "uint",
    "int",
    "bool",
    "double",
};

inline constexpr const char* fsl_opaques[] = {
    "image1D",
    "image2D",
    "image3D",
    "imageCubeMap",
    "image1DArray",
    "image2DArray",
    "imageCubeMapArray",
    "imageBuffer",

    "sampler1D",
    "sampler2D",
    "sampler3D",
    "samplerCubeMap",
    "sampler1DArray",
    "sampler2DArray",
    "samplerCubeMapArray",
    "samplerBuffer",
};

inline constexpr const char *fsl_specifiers[] = {
    "in",
    "out",
    "ref",
    "const",
    "shared"
};