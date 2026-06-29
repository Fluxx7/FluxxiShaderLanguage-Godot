#pragma once
#include <godot_cpp/core/defs.hpp>
#include "godot_cpp/core/binder_common.hpp"

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

inline constexpr const char* fsl_primitives[] = {
    "float",
    "uint",
    "int",
    "bool",
    "double",
};

inline constexpr const char* fsl_opaques[] = {
    "image2D",
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
    "const"
};


enum BufferType {
    STORAGE,
    UNIFORM
};

enum BufferFormat {
    STD140,
    STD430
};

enum TextureFormat {
    RGBA32F,
    RGBA16F
};

VARIANT_ENUM_CAST(BufferType);
VARIANT_ENUM_CAST(TextureFormat);

enum FSLPrimitive {
    FLOAT,
    UINT,
    INT,
    BOOL,
    DOUBLE
};

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
    godot::String struct_name;
};

typedef std::variant<FSLCoreType, FSLStruct> FSLBaseType;

godot::String fslBaseType_to_string(FSLBaseType base_type);

struct FSLArray {
    FSLBaseType base_type;
    godot::LocalVector<uint32_t> dimensions;
};


typedef std::variant<FSLBaseType, FSLArray> FSLType;

godot::String fslType_to_string(FSLType type);
godot::Variant get_fsl_basetype_default_value(const FSLBaseType &base_type);
godot::Variant get_fsl_default_value(const FSLType &fsl_type);
uint32_t get_fsl_base_type_size(const FSLBaseType &base_type);

uint32_t get_fsl_type_size(const FSLType &fsl_type);

godot::PackedByteArray fsl_type_to_bytes(const FSLType &fsl_type, godot::Variant value);

template <typename... VarArgs>
inline void printvf(const godot::String &template_string, const VarArgs... p_args) {
    print_line(vformat(template_string, p_args...));
}