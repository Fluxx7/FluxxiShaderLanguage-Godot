#pragma once
#include "godot_imports.h"
#include "std_imports.h"

// Uncomment this if/when specialization constant support for workgroup sizes is fixed for all APIs
// #define SPECIALIZATION_CONSTANTS_WORKGROUPS_USEABLE

inline constexpr const char* fsl_primitives[] = {
    "float",
    "uint",
    "int",
    "bool",
    "double",
};

inline constexpr const char* fsl_opaques[] = {
    "image2D",
    "image2DArray"
};

inline constexpr const char* fsl_operators[] = {
    "~",
    "!",
    "^",
    "&",
    "*",
    "/",
    ":",
    "-",
    "=",
    "|",
    "+",
    "<",
    ">",
    "?",
    ".",
    "%",
};

inline constexpr const char* fsl_symbols[] = {
    "\'",
    "\"",
    ";",
    ",",
    "(",
    ")",
};

inline constexpr const char *fsl_specifiers[] = {
    "in",
    "out",
    "inout",
    "const",
    "shared"
};

enum BufferType {
    STORAGE,
    UNIFORM
};

String bufferType_to_string(BufferType buf_type);

enum BufferFormat {
    STD140,
    STD430,
    VERTEX,
    INDEX
};

String bufferFormat_to_string(BufferFormat buf_format);

String get_buffer_desc(BufferType buf_type, BufferFormat buf_format);

enum TextureType {
    TEXTURE2D,
    TEXTURE2DARRAY
};

String textureType_to_string(TextureType tex_type);


enum TextureFormat {
    RGBA32F,
    RGBA16F
};

String textureFormat_to_string(TextureFormat tex_format);

VARIANT_ENUM_CAST(BufferType);
VARIANT_ENUM_CAST(TextureFormat);

enum FSLPrimitive {
    FLOAT,
    UINT,
    INT,
    BOOL,
    DOUBLE
};

uint32_t get_fsl_primitive_size(FSLPrimitive primitive);

enum FSLVecSize {
    ONE = 1,
    TWO = 2,
    THREE = 3,
    FOUR = 4
};

struct FSLCoreType {
    FSLPrimitive primitive;
    FSLVecSize vec_size;
};

struct FSLStruct {
    String struct_name;
};

typedef sumtype<FSLCoreType, FSLStruct> FSLBaseType;

String fslBaseType_to_string(FSLBaseType base_type);

struct FSLArray {
    FSLBaseType base_type;
    LocalVector<uint32_t> dimensions;
};


typedef sumtype<FSLBaseType, FSLArray> FSLType;

String fslType_to_string(FSLType type);
Variant get_fsl_basetype_default_value(const FSLBaseType &base_type);
Variant get_fsl_default_value(const FSLType &fsl_type);
uint32_t get_fsl_base_type_size(const FSLBaseType &base_type);

uint32_t get_fsl_type_size(const FSLType &fsl_type, uint32_t unsized_count = 1);

uint32_t get_fsl_type_alignment(const FSLType &fsl_type, uint32_t unsized_count, BufferFormat format);

PackedByteArray fsl_type_to_bytes(const FSLType &fsl_type, Variant value);

// bool variant_matches_fsl_type(const FSLType &fsl_type, const godot::Variant &gvariant);

template <typename... VarArgs>
inline void printvf(const String &template_string, const VarArgs... p_args) {
    print_line(vformat(template_string, p_args...));
}