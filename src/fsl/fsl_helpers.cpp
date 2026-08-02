#include "fsl_defs.h"


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
        case VERTEX:
            return "vertex";
        case INDEX:
            return "index";
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
            case VERTEX:
                return "Vertex buffer";
            case INDEX:
                return "Index buffer";
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

String fslBaseType_to_string(FSLBaseType base_type) {
	String out_string = "";
    std::visit(overload{
        [&](FSLCoreType &core_type) {
            switch (core_type.primitive) {
                case FLOAT:
                    out_string += "float";
                    break;
                case UINT:
                    out_string += "uint";
                    break;
                case INT:
                    out_string += "int";
                    break;
                case BOOL:
                    out_string += "bool";
                    break;
                case DOUBLE:
                    out_string += "double";
                    break;
            }
            switch (core_type.vec_size) {
                case ONE:
                    break;
                case TWO:
                    out_string += "2";
                    break;
                case THREE:
                    out_string += "3";
                    break;
                case FOUR:
                    out_string += "4";
                    break;
            }
        },
        [&](FSLStruct &fsl_struct) {

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
            switch (core_type.primitive) {
                case FLOAT:
                    primitive_defval = 0.0f;
                    break;
                case UINT:
                    primitive_defval = 0u;
                    break;
                case INT:
                    primitive_defval = 0;
                    break;
                case BOOL:
                    primitive_defval = false;
                    break;
                case DOUBLE:
                    primitive_defval = 0.0;
                    break;
            }
            switch (core_type.vec_size) {
                case ONE:
                    out_defval = primitive_defval;
                    break;
                case TWO:
                    out_defval = Array({primitive_defval, primitive_defval});
                    break;
                case THREE:
                    out_defval = Array({primitive_defval, primitive_defval, primitive_defval});
                    break;
                case FOUR:
                    out_defval = Array({primitive_defval, primitive_defval, primitive_defval, primitive_defval});
                    break;
            }
        },
        [&](const FSLStruct &array) {
            // not my problem right now tbh
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

uint32_t get_fsl_primitive_size(FSLPrimitive primitive) {
	switch (primitive) {
        case FLOAT:
        case UINT:
        case INT:
        case BOOL:
            return 4;
        case DOUBLE:
            return 8;
    }
}

uint32_t get_fsl_base_type_size(const FSLBaseType &base_type) {
    uint32_t out_size = 0;
    match(base_type, 
        [&](const FSLCoreType &core_type) {
            uint32_t primitive_size = get_fsl_primitive_size(core_type.primitive);
            out_size = primitive_size * core_type.vec_size;
        },
        [&](const FSLStruct &array) {
            // not my problem right now tbh
        });
    return out_size;
}

uint32_t get_fsl_type_size(const FSLType &fsl_type, uint32_t unsized_count) {
    uint32_t out_size = 0;
    match(fsl_type,
        [&](const FSLBaseType &base_type) {
            out_size = get_fsl_base_type_size(base_type);
        },
        [&](const FSLArray &array) {
            out_size = get_fsl_base_type_size(array.base_type);
            for (const auto& array_size : array.dimensions) {
                if (array_size > 0) {
                    out_size *= array_size;
                } else {
                    out_size *= unsized_count;
                }
            }
        });
    return out_size;
}

uint32_t get_fsl_base_type_alignment(const FSLBaseType &base_type, BufferFormat format) {
    uint32_t alignment = 0;
    match(base_type, 
        [&](const FSLCoreType &core_type) {
            uint32_t primitive_size = get_fsl_primitive_size(core_type.primitive);
            uint32_t vec_coeff = core_type.vec_size == 3 ? 4 : core_type.vec_size;
            alignment = primitive_size * vec_coeff;
        },
        [&](const FSLStruct &struct_type) {
            // also not my problem rn
        });
    return alignment;
}

uint32_t get_fsl_type_alignment(const FSLType &fsl_type, uint32_t unsized_count, BufferFormat format) {
    uint32_t alignment = 0;
    match(fsl_type, 
        [&](const FSLBaseType &base_type) {
            alignment = get_fsl_base_type_alignment(base_type, format);
        },
        [&](const FSLArray &array) {
            alignment = get_fsl_base_type_alignment(array.base_type, format);
            if (format == STD140 && alignment < 16) {
                alignment = 16;
            }
            for (const auto& array_size : array.dimensions) {
                if (array_size > 0) {
                    alignment *= array_size;
                } else {
                    alignment *= unsized_count;
                }
            }
        });
    return alignment;
}

PackedByteArray fsl_core_type_to_bytes(const FSLCoreType &base_type, Variant value) {
    PackedByteArray out_bytes = {};
    out_bytes.resize(get_fsl_base_type_size(base_type));
    if (base_type.vec_size == ONE) {
        switch (base_type.primitive) {
            case FLOAT:
                out_bytes.encode_float(0, (float) value);
                break;
            case UINT:
                out_bytes.encode_u32(0, (uint32_t) value);
                break;
            case INT:
                out_bytes.encode_s32(0, (int32_t) value);
                break;
            case DOUBLE:
                out_bytes.encode_double(0, (double) value);
                break;
            default:
                break;
        }
    } else {
        uint32_t stride = get_fsl_primitive_size(base_type.primitive);
        for (int i = 0; i < (int) base_type.vec_size; i++) {
            uint32_t offset = stride * i;
            switch (base_type.primitive) {
                case FLOAT:
                    out_bytes.encode_float(offset, (float) value);
                    break;
                case UINT:
                    out_bytes.encode_u32(offset, (uint32_t) value);
                    break;
                case INT:
                    out_bytes.encode_s32(offset, (int32_t) value);
                    break;
                case DOUBLE:
                    out_bytes.encode_double(offset, (double) value);
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
        [&](const FSLStruct &struct_type) {
            // not my problem right now
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

// Variant fsl_core_type_to_variant(const FSLCoreType &core_type) {
    
// }

// Variant fsl_base_type_to_variant(const FSLBaseType &base_type) {
//     Variant output;
//     std::visit(overload{
//         [&](const FSLCoreType &core_type) {
//             output = fsl_core_type_to_variant(core_type);
//         },
//         [&](const FSLStruct &struct_type) {
//             // not my problem right now
//         }
//     }, base_type);
//     return output;
// }

// Variant fsl_type_to_variant(const FSLType &fsl_type) {
//     Variant output;
//     std::visit(overload{
//         [&](const FSLBaseType &base_type) {
//             output = fsl_base_type_to_variant(base_type);
//         },
//         [&](const FSLArray &array) {
//             // for (const auto& item : array_val) {
//             // }
//         }
//     }, fsl_type);
//     return output;
// }
