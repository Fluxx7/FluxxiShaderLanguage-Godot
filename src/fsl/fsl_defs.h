#pragma once
#include "godot_imports.h"
#include "std_imports.h"

// Uncomment this if/when specialization constant support for workgroup sizes is fixed for all APIs
#define SPECIALIZATION_CONSTANTS_WORKGROUPS_USABLE

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

enum FSLSpecifier {
    SPECIFIER_IN,
    SPECIFIER_OUT,
    SPECIFIER_INOUT,
    SPECIFIER_CONST,
    SPECIFIER_SHARED
};

String specifier_to_string(FSLSpecifier specifier);

enum BufferType {
    STORAGE,
    UNIFORM
};

String bufferType_to_string(BufferType buf_type);

enum BufferFormat {
    STD140,
    STD430
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
    F16,
    F32,
    F64,
    U8,
    U16,
    U32,
    U64,
    I8,
    I16,
    I32,
    I64,
    BOOL,
    FSL_PRIMITIVE_MAX
};

inline uint32_t primitive_name_length(FSLPrimitive primitive) {
    switch (primitive) {
        case BOOL: return 4;

        case F16: 
        case F32: 
        case F64:
        case U16:
        case U32:
        case U64:
        case I16:
        case I32: 
        case I64: return 3;

        case I8:
        case U8: return 2;

        default:
            return 0;
    }
}

FSLPrimitive get_primitive_from_string(const String& primitive_string);
FSLPrimitive rip_primitive_from_string(const String& type_string);

uint32_t get_fsl_primitive_size(FSLPrimitive primitive);

enum FSLVecSize {
    ONE = 1,
    TWO = 2,
    THREE = 3,
    FOUR = 4
};

struct FSLLayout {
    uint32_t size = 0;
    uint32_t alignment = 4;
    uint32_t stride = 0;
};

struct FSLStructLayout {
    FSLLayout layout;
    AHashMap<StringName, uint32_t> offset;
};

struct FSLCoreType {
    FSLPrimitive primitive = F32;
    FSLVecSize vec_size = ONE;
    FSLVecSize mat_size = ONE;
};

struct FSLStruct;
typedef sumtype<FSLCoreType, FSLStruct> FSLBaseType;

struct FSLArray;
typedef sumtype<FSLBaseType, FSLArray> FSLType;

struct FSLStruct {
    String struct_name;
    HashMap<StringName, FSLType> fields;
};


String fslBaseType_to_string(FSLBaseType base_type);

struct FSLArray {
    FSLBaseType base_type;
    LocalVector<uint32_t> dimensions;
};


String fslType_to_string(FSLType type);
Variant get_fsl_basetype_default_value(const FSLBaseType& base_type);
Variant get_fsl_default_value(const FSLType& fsl_type);

FSLLayout get_fsl_base_type_layout(const FSLBaseType& base_type, BufferFormat format);

FSLLayout get_fsl_type_layout(const FSLType& fsl_type, uint32_t unsized_count, BufferFormat format);

PackedByteArray fsl_type_to_bytes(const FSLType& fsl_type, Variant value);

// bool variant_matches_fsl_type(const FSLType &fsl_type, const godot::Variant &gvariant);

template <typename... VarArgs>
inline void printvf(const String &template_string, const VarArgs... p_args) {
    print_line(vformat(template_string, p_args...));
}