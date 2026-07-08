#include "fsl_defs.h"



godot::String fslBaseType_to_string(FSLBaseType base_type) {
    godot::String out_string = "";
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

godot::String fslType_to_string(FSLType type) {
    godot::String out_string = "";
    std::visit(overload{
        [&](FSLBaseType &base_type) {
            out_string += fslBaseType_to_string(base_type);
        },
        [&](FSLArray &array) {
            out_string += fslBaseType_to_string(array.base_type);
            for (auto size : array.dimensions) {
                if (size > 0) {
                    out_string += godot::vformat("[%d]", size);
                } else {
                    out_string += "[]";
                }
            }
        }
    }, type);
    return out_string;
}

godot::Variant get_fsl_basetype_default_value(const FSLBaseType &base_type) {
    godot::Variant out_defval = 0;
    std::visit(overload{
        [&](const FSLCoreType &core_type) {
            godot::Variant primitive_defval;
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
                    out_defval = godot::Array({primitive_defval, primitive_defval});
                    break;
                case THREE:
                    out_defval = godot::Array({primitive_defval, primitive_defval, primitive_defval});
                    break;
                case FOUR:
                    out_defval = godot::Array({primitive_defval, primitive_defval, primitive_defval, primitive_defval});
                    break;
            }
        },
        [&](const FSLStruct &array) {
            // not my problem right now tbh
        }
    }, base_type);
    return out_defval;
}

godot::Variant get_fsl_default_value(const FSLType &fsl_type) {
    godot::Variant out_defval = 0;
    std::visit(overload{
        [&](const FSLBaseType &base_type) {
            out_defval = get_fsl_basetype_default_value(base_type);
        },
        [&](const FSLArray &array) {
            godot::Variant item_defval = get_fsl_basetype_default_value(array.base_type);
            out_defval = item_defval;
            for (auto size : array.dimensions) {
                
            }
        }
    }, fsl_type);
    return out_defval;
}

uint32_t get_fsl_base_type_size(const FSLBaseType &base_type) {
    uint32_t out_size = 0;
    std::visit(overload{
        [&](const FSLCoreType &core_type) {
            uint32_t primitive_size;
            switch (core_type.primitive) {
                case FLOAT:
                    primitive_size = 4;
                    break;
                case UINT:
                    primitive_size = 4;
                    break;
                case INT:
                    primitive_size = 4;
                    break;
                case BOOL:
                    primitive_size = 4;
                    break;
                case DOUBLE:
                    primitive_size = 8;
                    break;
            }
            out_size = primitive_size * core_type.vec_size;
        },
        [&](const FSLStruct &array) {
            // not my problem right now tbh
        }
    }, base_type);
    return out_size;
}

uint32_t get_fsl_type_size(const FSLType &fsl_type, uint32_t unsized_count) {
    uint32_t out_size = 0;
    std::visit(overload{
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
        }
    }, fsl_type);
    return out_size;
}

uint32_t get_fsl_base_type_alignment(const FSLBaseType &base_type, BufferFormat format) {
    uint32_t alignment = 0;
    std::visit(overload{
        [&](const FSLCoreType &core_type) {
            uint32_t primitive_size;
            switch (core_type.primitive) {
                case FLOAT:
                    primitive_size = 4;
                    break;
                case UINT:
                    primitive_size = 4;
                    break;
                case INT:
                    primitive_size = 4;
                    break;
                case BOOL:
                    primitive_size = 4;
                    break;
                case DOUBLE:
                    primitive_size = 8;
                    break;
            }
            uint32_t vec_coeff = core_type.vec_size == 3 ? 4 : core_type.vec_size;
            alignment = primitive_size * vec_coeff;
        },
        [&](const FSLStruct &struct_type) {
            // also not my problem rn
        }
    }, base_type);
    return alignment;
}

uint32_t get_fsl_type_alignment(const FSLType &fsl_type, uint32_t unsized_count, BufferFormat format) {
    uint32_t alignment = 0;
    std::visit(overload{
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
        }
    }, fsl_type);
    return alignment;
}

godot::PackedByteArray fsl_core_type_to_bytes(const FSLCoreType &base_type, godot::Variant value) {
    godot::PackedByteArray out_bytes = {0, 0, 0, 0};
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
            out_bytes.resize(8);
            out_bytes.encode_double(0, (double) value);
            break;
        default:
            break;
    }
	return out_bytes;
}

godot::PackedByteArray fsl_base_type_to_bytes(const FSLBaseType &base_type, godot::Variant value) {
    godot::PackedByteArray out_bytes = {};
    std::visit(overload{
        [&](const FSLCoreType &core_type) {
            out_bytes = fsl_core_type_to_bytes(core_type, value);
        },
        [&](const FSLStruct &struct_type) {
            // not my problem right now
        }
    }, base_type);
	return out_bytes;
}

godot::PackedByteArray fsl_type_to_bytes(const FSLType &fsl_type, godot::Variant value) {
    godot::PackedByteArray out_bytes = {};
    std::visit(overload{
        [&](const FSLBaseType &base_type) {
            out_bytes = fsl_base_type_to_bytes(base_type, value);
        },
        [&](const FSLArray &array) {
            godot::Array array_val = (godot::Array) value;
            if (array_val.size() == 0) {
                ERR_PRINT("Invalid or empty array provided");
                return;
            }
            for (const auto& item : array_val) {
            }
        }
    }, fsl_type);
	return out_bytes;
}

// using godot::Variant;
// bool variant_matches_fsl_type(const FSLType &fsl_type, const godot::Variant &gvariant) {
//     switch (gvariant.get_type()) {
//         case Variant::FLOAT:
//         case Variant::INT:
//         case Variant::
//         case Variant::BOOL:
//         case Variant::VECTOR2:
//     }
// 	return false;
// }
