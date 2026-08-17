#include "fsl_defs.h"

String specifier_to_string(FSLSpecifier specifier) {
	switch (specifier) {
        case SPECIFIER_IN:
            return "in";
        case SPECIFIER_OUT:
            return "out";
        case SPECIFIER_INOUT:
            return "inout";
        case SPECIFIER_CONST:
            return "const";
        case SPECIFIER_SHARED:
            return "shared";
    }
}

String bufferType_to_string(BufferType buf_type) {
	switch (buf_type) {
        case UNIFORM:
            return "uniform";
        case STORAGE:
            return "storage";
    }
}

String bufferFormat_to_string(BufferFormat buf_format) {
	switch (buf_format) {
        case STD140:
            return "std140";
        case STD430:
            return "std430";
    }
}

String get_buffer_desc(BufferType buf_type, BufferFormat buf_format) {
    if (buf_type == UNIFORM) {
        if (buf_format == STD140) {
            return "Uniform buffer with format std140";
        }
        
    } else {
        switch (buf_format) {
            case STD140:
                return "Storage buffer with format std140";
            case STD430:
                return "Storage buffer with format std430";
        }
    }
	return "Invalid buffer";
}

String textureType_to_string(TextureType tex_type) {
	switch (tex_type) {
        case TEXTURE2D:
            return "Texture2D";
        case TEXTURE2DARRAY:
            return "Texture2DArray";
    }
}

String textureFormat_to_string(TextureFormat tex_format) {
	switch (tex_format) {
        case RGBA16F:
            return "rgba16f";
        case RGBA32F:
            return "rgba32f";
    }
}

String fslCoreType_to_string(FSLCoreType core_type) {
    static const AHashMap<FSLPrimitive, String> type_names = {
        {F16, "f16"},
        {F32, "f32"},
        {F64, "f64"},
        {U8, "u8"},
        {U16, "u16"},
        {U32, "u32"},
        {U64, "u64"},
        {I8, "i8"},
        {I16, "i16"},
        {I32, "i32"},
        {I64, "i64"},
        {BOOL, "bool"}
    };
    if (core_type.primitive == FSL_PRIMITIVE_MAX) {
        return "!!bad_type!!";
    }
    String out_string = type_names[core_type.primitive];
    switch (core_type.vec_size) {
        case ONE:
            if (core_type.mat_size != ONE) {
                out_string += "x1";
            }
            break;
        case TWO:
            out_string += "x2";
            break;
        case THREE:
            out_string += "x3";
            break;
        case FOUR:
            out_string += "x4";
            break;
    }
    switch (core_type.mat_size) {
        case ONE:
            break;
        case TWO:
            out_string += "x2";
            break;
        case THREE:
            out_string += "x3";
            break;
        case FOUR:
            out_string += "x4";
            break;
    }
    return out_string;
}

String fslBaseType_to_string(FSLBaseType base_type) {
	String out_string = "";
    std::visit(overload{
        [&](FSLCoreType &core_type) {
            out_string += fslCoreType_to_string(core_type);
        },
        [&](FSLStruct &fsl_struct) {
            out_string += fsl_struct.struct_name;
        }
    }, base_type);
    return out_string;
};

String fslType_to_string(FSLType type) {
    String out_string = "";
    std::visit(overload{
        [&](FSLBaseType &base_type) {
            out_string += fslBaseType_to_string(base_type);
        },
        [&](FSLArray &array) {
            out_string += fslBaseType_to_string(array.base_type);
            for (auto size : array.dimensions) {
                if (size > 0) {
                    out_string += vformat("[%d]", size);
                } else {
                    out_string += "[]";
                }
            }
        }
    }, type);
    return out_string;
}

Variant get_fsl_basetype_default_value(const FSLBaseType &base_type) {
    Variant out_defval = 0;
    std::visit(overload{
        [&](const FSLCoreType &core_type) {
            Variant primitive_defval;
            Variant vec_defval;
            switch (core_type.primitive) {
                case F16:
                case F32:
                    primitive_defval = 0.0f;
                    break;
                case F64:
                    primitive_defval = 0.0;
                    break;
                case U8:
                case U16:
                case U32:
                case U64:
                    primitive_defval = 0u;
                    break;
                case BOOL:
                    primitive_defval = false;
                    break;
                case I8:
                case I16:
                case I32:
                case I64:
                default:
                    primitive_defval = 0;
                    break;
            }
            switch (core_type.vec_size) {
                case ONE:
                    vec_defval = primitive_defval;
                    break;
                case TWO:
                    vec_defval = Array({primitive_defval, primitive_defval});
                    break;
                case THREE:
                    vec_defval = Array({primitive_defval, primitive_defval, primitive_defval});
                    break;
                case FOUR:
                    vec_defval = Array({primitive_defval, primitive_defval, primitive_defval, primitive_defval});
                    break;
            }
            switch (core_type.mat_size) {
                case ONE:
                    out_defval = vec_defval;
                    break;
                case TWO:
                    out_defval = Array({vec_defval, vec_defval});
                    break;
                case THREE:
                    out_defval = Array({vec_defval, vec_defval, vec_defval});
                    break;
                case FOUR:
                    out_defval = Array({vec_defval, vec_defval, vec_defval, vec_defval});
                    break;
            }
        },
        [&](const FSLStruct &fsl_struct) {
            
        }
    }, base_type);
    return out_defval;
}

Variant get_fsl_default_value(const FSLType &fsl_type) {
    Variant out_defval = 0;
    match(fsl_type, 
        [&](const FSLBaseType &base_type) {
            out_defval = get_fsl_basetype_default_value(base_type);
        },
        [&](const FSLArray &array) {
            Variant item_defval = get_fsl_basetype_default_value(array.base_type);
            out_defval = item_defval;
            for (auto size : array.dimensions) {
                
            }
        });
    return out_defval;
}

const AHashMap<String, FSLPrimitive>& _get_type_names_map() {
    static const AHashMap<String, FSLPrimitive> type_names = {
        {"f16", F16},
        {"f32", F32},
        {"f64", F64},
        {"u8", U8},
        {"u16", U16},
        {"u32", U32},
        {"u64", U64},
        {"i8", I8},
        {"i16", I16},
        {"i32", I32},
        {"i64", I64},
        {"bool", BOOL}
    };
    return type_names;
}

FSLPrimitive get_primitive_from_string(const String &primitive_string) {
    const auto& type_names = _get_type_names_map();
    if (type_names.has(primitive_string)) {
        return type_names[primitive_string];
    }
	return FSL_PRIMITIVE_MAX;
}

FSLPrimitive rip_primitive_from_string(const String &type_string) {
    const auto& type_names = _get_type_names_map();
    for (const auto& [type_name, primitive] : type_names) {
        if (type_string.left(type_name.length()) == type_name) {
            return primitive;
            break;
        }
    }
	return FSL_PRIMITIVE_MAX;
}

uint32_t get_fsl_primitive_size(FSLPrimitive primitive) {
	switch (primitive) {
        case I8:
        case U8:
            return 1;
        case F16:
        case I16:
        case U16:
            return 2;
        case F32:
        case I32:
        case U32:
        case BOOL:
            return 4;
        case F64:
        case I64:
        case U64:
            return 8;
        default:
            return 0;
    }
}

FSLLayout get_fsl_base_type_layout(const FSLBaseType& base_type, BufferFormat format) {
    FSLLayout layout;
    match(base_type, 
        [&](const FSLCoreType &core_type) {
            uint32_t primitive_size = get_fsl_primitive_size(core_type.primitive);
            layout.size = primitive_size * core_type.vec_size;
            uint32_t vec_coeff = core_type.vec_size == 3 ? 4 : core_type.vec_size;
            layout.alignment = primitive_size * vec_coeff;
        },
        [&](const FSLStruct &fsl_struct) {
            uint32_t curr_offset = 0;
            for (const auto& [_name, field_type] : fsl_struct.fields) {
                auto [f_size, f_alignment, _stride] = get_fsl_type_layout(field_type, 0, format);
                if (curr_offset % f_alignment != 0) {
                    curr_offset += f_alignment - (curr_offset % f_alignment);
                }
                curr_offset += f_size;
                if (f_alignment > layout.alignment) {
                    layout.alignment = f_alignment;
                }
            }
            if (format == STD140 && layout.alignment < 16) {
                layout.alignment = 16;
            }
            if (curr_offset % layout.alignment != 0) {
                curr_offset += layout.alignment - (curr_offset % layout.alignment);
            }
            layout.size = curr_offset;
        });
    layout.stride = layout.size;
	return layout;
}

FSLLayout get_fsl_type_layout(const FSLType& fsl_type, uint32_t unsized_count, BufferFormat format) {
    FSLLayout layout;
    match(fsl_type, 
        [&](const FSLBaseType &base_type) {
            layout = get_fsl_base_type_layout(base_type, format);
        },
        [&](const FSLArray &array) {
            layout = get_fsl_base_type_layout(array.base_type, format);
            if (format == STD140 && layout.alignment < 16) {
                layout.alignment = 16;
            }
            for (const auto& array_size : array.dimensions) {
                if (array_size > 0) {
                    layout.size *= array_size;
                } else {
                    layout.size *= unsized_count;
                }
            }
        });

	return layout;
}

PackedByteArray fsl_core_type_to_bytes(const FSLCoreType &core_type, Variant value) {
    PackedByteArray out_bytes = {};
    uint32_t core_size = get_fsl_primitive_size(core_type.primitive) * core_type.vec_size;
    out_bytes.resize(core_size);
    if (core_type.vec_size == ONE) {
        switch (core_type.primitive) {
            case F16:
                out_bytes.encode_half(0, (double) value);
                break;
            case F32:
                out_bytes.encode_float(0, (float) value);
                break;
            case F64:
                out_bytes.encode_double(0, (double) value);
                break;
            case U8:
                out_bytes.encode_u8(0, (uint32_t) value);
                break;
            case U16:
                out_bytes.encode_u16(0, (uint32_t) value);
                break;
            case U32:
                out_bytes.encode_u32(0, (uint32_t) value);
                break;
            case U64:
                out_bytes.encode_u64(0, (uint32_t) value);
                break;
            case I8:
                out_bytes.encode_s8(0, (int32_t) value);
                break;
            case I16:
                out_bytes.encode_s16(0, (int32_t) value);
                break;
            case I32:
                out_bytes.encode_s32(0, (int32_t) value);
                break;
            case I64:
                out_bytes.encode_s64(0, (int32_t) value);
                break;
            case BOOL:
                out_bytes.encode_u32(0, (uint32_t) value);
                break;
            default:
                break;
        }
    } else {
        uint32_t stride = get_fsl_primitive_size(core_type.primitive);
        for (int i = 0; i < (int) core_type.vec_size; i++) {
            uint32_t offset = stride * i;
            switch (core_type.primitive) {
                case F16:
                    out_bytes.encode_half(offset, (float) value);
                    break;
                case F32:
                    out_bytes.encode_float(offset, (float) value);
                    break;
                case F64:
                    out_bytes.encode_double(offset, (double) value);
                    break;
                case U8:
                    out_bytes.encode_u8(offset, (uint32_t) value);
                    break;
                case U16:
                    out_bytes.encode_u16(offset, (uint32_t) value);
                    break;
                case U32:
                    out_bytes.encode_u32(offset, (uint32_t) value);
                    break;
                case U64:
                    out_bytes.encode_u64(offset, (uint32_t) value);
                    break;
                case I8:
                    out_bytes.encode_s8(offset, (int32_t) value);
                    break;
                case I16:
                    out_bytes.encode_s16(offset, (int32_t) value);
                    break;
                case I32:
                    out_bytes.encode_s32(offset, (int32_t) value);
                    break;
                case I64:
                    out_bytes.encode_s64(offset, (int32_t) value);
                    break;
                case BOOL:
                    out_bytes.encode_u32(offset, (uint32_t) value);
                    break;
                default:
                    break;
            }
        }
    }
	return out_bytes;
}

PackedByteArray fsl_base_type_to_bytes(const FSLBaseType &base_type, Variant value) {
    PackedByteArray out_bytes = {};
    match(base_type, 
        [&](const FSLCoreType &core_type) {
            out_bytes = fsl_core_type_to_bytes(core_type, value);
        },
        [&](const FSLStruct &fsl_struct) {
            for (const auto& field : fsl_struct.fields) {
                
            }
        });
	return out_bytes;
}

PackedByteArray fsl_type_to_bytes(const FSLType &fsl_type, Variant value) {
    PackedByteArray out_bytes = {};
    match(fsl_type, 
        [&](const FSLBaseType &base_type) {
            out_bytes = fsl_base_type_to_bytes(base_type, value);
        },
        [&](const FSLArray &array) {
            Array array_val = (Array) value;
            if (array_val.size() == 0) {
                ERR_PRINT("Invalid or empty array provided");
                return;
            }
            for (const auto& item : array_val) {
            }
        });
	return out_bytes;
}
