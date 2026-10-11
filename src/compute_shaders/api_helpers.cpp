#include "api_helpers.h"

/*
    SCALAR HANDLING
*/

PackedByteArray float_scalar_to_bytes(uint32_t num_bytes, Variant value) {
    PackedByteArray out_bytes = {};
    out_bytes.resize(num_bytes);
    switch (num_bytes) {
        case 2:
            out_bytes.encode_half(0, (double) value);
            break;
        case 4:
            out_bytes.encode_float(0, (float) value);
            break;
        case 8:
            out_bytes.encode_double(0, (double) value);
            break;
        default:
            break;
    }
    return out_bytes;
}

PackedByteArray int_scalar_to_bytes(uint32_t num_bytes, Variant value) {
    PackedByteArray out_bytes = {};
    out_bytes.resize(num_bytes);
    switch (num_bytes) {
        case 1:
            out_bytes.encode_s8(0, (int32_t) value);
            break;
        case 2:
            out_bytes.encode_s16(0, (int32_t) value);
            break;
        case 4:
            out_bytes.encode_s32(0, (int32_t) value);
            break;
        case 8:
            out_bytes.encode_s64(0, (int32_t) value);
            break;
        default:
            break;
    }
    return out_bytes;
}

PackedByteArray uint_scalar_to_bytes(uint32_t num_bytes, Variant value) {
    PackedByteArray out_bytes = {};
    out_bytes.resize(num_bytes);
    switch (num_bytes) {
        case 1:
            out_bytes.encode_u8(0, (uint32_t) value);
            break;
        case 2:
            out_bytes.encode_u16(0, (uint32_t) value);
            break;
        case 4:
            out_bytes.encode_u32(0, (uint32_t) value);
            break;
        case 8:
            out_bytes.encode_u64(0, (uint32_t) value);
            break;
        default:
            break;
    }
    return out_bytes;
}

PackedByteArray bool_scalar_to_bytes(Variant value) {
    PackedByteArray out_bytes = {};
    out_bytes.resize(4);
    out_bytes.encode_u32(0, (uint32_t) value);
    return out_bytes;
}

/*
    VECTOR HANDLING
*/
PackedByteArray float_vec_to_bytes(uint32_t num_bytes, uint32_t vec_size, Variant value) {
    if (vec_size == 1) return float_scalar_to_bytes(num_bytes, value);
    PackedByteArray out_bytes = {};
    uint32_t stride = num_bytes;
    LocalVector<Variant> vec_vals;
    auto bad_input = [&](String err_msg) -> PackedByteArray {
        // TODO: add error message for bad variant types
        return {};
    };
    switch (value.get_type()) {
        case Variant::ARRAY: {
            Array vec_array = (Array) value;
            if (vec_array.size() < vec_size) return bad_input("");
            for (int i = 0; i < (int) vec_size; i++) {
                vec_vals.push_back(vec_array[i]);
            }
        } break;
        case Variant::VECTOR4: {
            if (vec_size != 4) {
                // change all of these to be warnings if vec_size < the provided vectors size
                return bad_input("");
            }
            godot::Vector4 vec4 = (godot::Vector4) value;
            for (int i = 0; i < (int) vec_size; i++) {
                vec_vals.push_back(vec4[i]);
            }
        } break;
        case Variant::VECTOR3: {
            if (vec_size != 3) {
                return bad_input("");
            }
            godot::Vector3 vec3 = (godot::Vector3) value;
            for (int i = 0; i < (int) vec_size; i++) {
                vec_vals.push_back(vec3[i]);
            }
        } break;
        case Variant::VECTOR2: {
            if (vec_size != 2) {
                return bad_input("");
            }
            godot::Vector2 vec2 = (godot::Vector2) value;
            for (int i = 0; i < (int) vec_size; i++) {
                vec_vals.push_back(vec2[i]);
            }
        } break;

        default: 
            return bad_input("");
    }
    
    for (int i = 0; i < (int) vec_size; i++) {
        uint32_t offset = stride * i;
        out_bytes.append_array(float_scalar_to_bytes(num_bytes, vec_vals[i]));
    }
	return out_bytes;
}


/*
    MATRIX HANDLING
*/

PackedByteArray float_to_bytes(uint32_t num_bytes, uint32_t vec_size, uint32_t mat_size, Variant value, BufferFormat format) {
    if (mat_size == 1) { 
        if (vec_size == 1) 
            return float_scalar_to_bytes(num_bytes, value);
        else 
            return float_vec_to_bytes(num_bytes, vec_size, value);
    }
    PackedByteArray out_bytes = {};
    uint32_t stride = num_bytes;
    LocalVector<PackedByteArray> mat_vals;
    auto bad_input = [&](String err_msg) -> PackedByteArray {
        // TODO: add error message for bad variant types
        return {};
    };
    switch (value.get_type()) {
        case Variant::PROJECTION: {
            godot::Projection vec_proj = (godot::Projection) value;
            for (int i = 0; i < (int) mat_size; i++) {
                mat_vals.push_back(float_vec_to_bytes(num_bytes, vec_size, vec_proj[i]));
            }
        } break;

        default: 
            return bad_input("");
    }
    uint32_t curr_offset = 0;
    FSLLayout rules = get_fsl_type_layout(FSLCoreType{F32, (FSLVecSize) vec_size, ONE}, 0, format);
    for (int i = 0; i < (int) mat_size; i++) {
        if (curr_offset % rules.alignment != 0) {
            uint32_t align_size = rules.alignment - (curr_offset % rules.alignment);
            PackedByteArray align_bytes = {};
            align_bytes.resize(align_size);
            out_bytes.append_array(align_bytes);
            curr_offset += align_size;
        }
        out_bytes.append_array(mat_vals[i]);
        curr_offset += rules.size;
    }
	return out_bytes;
}

PackedByteArray fsl_core_type_to_bytes(const FSLCoreType &core_type, Variant value, BufferFormat format) {
    PackedByteArray out_bytes = {};
    uint32_t primitive_size = get_fsl_primitive_size(core_type.primitive);
    if (core_type.primitive == F16 || core_type.primitive == F32 || core_type.primitive == F64) {
        return float_to_bytes(primitive_size, core_type.vec_size, core_type.mat_size, value, format);
    }
    uint32_t core_size = primitive_size * core_type.vec_size;
    out_bytes.resize(core_size);
    if (core_type.vec_size == ONE) {
        switch (core_type.primitive) {
            case U8:
            case U16:
            case U32:
            case U64:
                out_bytes = uint_scalar_to_bytes(primitive_size, value);
                break;
            case I8:
            case I16:
            case I32:
            case I64:
                out_bytes = int_scalar_to_bytes(primitive_size, value);
                break;
            case BOOL:
                out_bytes = bool_scalar_to_bytes(value);
                break;
            default:
                break;
        }
    } else {
        uint32_t stride = get_fsl_primitive_size(core_type.primitive);
        for (int i = 0; i < (int) core_type.vec_size; i++) {
            uint32_t offset = stride * i;
            switch (core_type.primitive) {
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

PackedByteArray fsl_struct_to_bytes(const FSLStruct &fsl_struct, Variant value, BufferFormat format) {
    PackedByteArray out_bytes = {};
    for (const auto& field : fsl_struct.fields) {
                
    }
    return out_bytes;
}

PackedByteArray fsl_base_type_to_bytes(const FSLBaseType &base_type, Variant value, BufferFormat format) {
    PackedByteArray out_bytes = {};
    match(base_type, 
        [&](const FSLCoreType &core_type) {
            out_bytes = fsl_core_type_to_bytes(core_type, value, format);
        },
        [&](const FSLStruct &fsl_struct) {
            out_bytes = fsl_struct_to_bytes(fsl_struct, value, format);
        });
	return out_bytes;
}



PackedByteArray fsl_type_to_bytes(const FSLType &fsl_type, Variant value, BufferFormat format) {
    PackedByteArray out_bytes = {};
    match(fsl_type, 
        [&](const FSLBaseType &base_type) {
            out_bytes = fsl_base_type_to_bytes(base_type, value, format);
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

Variant bytes_to_variant_core(const FSLCoreType &fsl_type, PackedByteArray bytes, BufferFormat format) {
	return Variant();
}

Variant bytes_to_variant_base(const FSLBaseType &fsl_type, PackedByteArray bytes, BufferFormat format) {
	return Variant();
}

Variant bytes_to_variant(const FSLType &fsl_type, PackedByteArray bytes, BufferFormat format) {
	return Variant();
}
