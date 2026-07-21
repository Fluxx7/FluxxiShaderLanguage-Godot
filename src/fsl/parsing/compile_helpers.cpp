#include "code_builder.h"

String to_original_type(const String &identifier) {
    static const char *changed_types[] = {
        "float2",
        "float3",
        "float4",
        "int2",
        "int3",
        "int4",
        "uint2",
        "uint3",
        "uint4",
        "double2",
        "double3",
        "double4",
        "bool2",
        "bool3",
        "bool4"
    }; 
    static const char *original_types[] = {
        "vec2",
        "vec3",
        "vec4",
        "ivec2",
        "ivec3",
        "ivec4",
        "uvec2",
        "uvec3",
        "uvec4",
        "dvec2",
        "dvec3",
        "dvec4",
        "bvec2",
        "bvec3",
        "bvec4"
    }; 
    uint32_t curr_index = 0;
    for (auto type_name : changed_types) {
        if (identifier == type_name) {
            return original_types[curr_index];
        }
        curr_index++;
    }
    return identifier;
}

String to_original_name(const String &name) {
    static const char *custom_names[] = {
        "GlobalInvocationID",
        "LocalInvocationID",
        "LocalInvocationIndex",
        "WorkGroupID",
        "WorkGroupSize",
        "NumWorkGroups"
    }; 
    static const char *original_names[] = {
        "gl_GlobalInvocationID", 
        "gl_LocalInvocationID", 
        "gl_LocalInvocationIndex", 
        "gl_WorkGroupID", 
        "gl_WorkGroupSize", 
        "gl_NumWorkGroups"
    }; 
    uint32_t index;
    for (int i = 0; i < 6; i++) {
        if (name == custom_names[i]) {
            return original_names[i];
        }
    }
    return name;
}

String tex_to_glsl_name(const TextureType &tex_type) {
    switch (tex_type) {
        case TEXTURE2D:
            return "image2D";
        case TEXTURE2DARRAY:
            return "image2DArray";
    }
}

Pair<BufferInfo, String> buffer_to_glsl(const String &buffer_name, const BufferDef &buffer) {
	BufferInfo buf_info;
    String res_code = "";
    buf_info.type = buffer.buftype;
    buf_info.format = buffer.layout;
    String buffer_type = buffer.buftype == UNIFORM ? "uniform" : "buffer";
    String buffer_layout = buffer.layout == STD140 ? "std140" : "std430";
    res_code += vformat("%s) %s restrict %s {\n", buffer_layout, buffer_type, buffer_name);
    uint32_t curr_offset = 0;
    for (auto field : buffer.fields) {
        BufferFieldInfo field_info;
        String type_string = "";
        String postname_string = "";
        for (const auto &token : field.type.type) {
            switch (token->category) {
                case Token::CATEGORY_SPECIFIER:
                    type_string += token->contents;
                    break;
                case Token::CATEGORY_IDENTIFIER: {
                    type_string += to_original_type(token->contents);
                } break;
                default:
                    break;
            }
        }
        for (const auto &array_dim : field.type.array_dims) {
            postname_string += tokens_to_string(array_dim);
        }
        field_info.type = typeref_to_fslType(field.type);
        if (const FSLArray* fsl_array = std::get_if<FSLArray>(&field_info.type)) {
            for (const auto& dimension : fsl_array->dimensions) {
                if (dimension == 0) {
                    buf_info.has_unsized_field = true;
                }
                field_info.dimensions.push_back(dimension);
            }
        }
        field_info.offset = curr_offset;
        curr_offset += get_fsl_type_alignment(field_info.type, 1, buffer.layout);

        buf_info.fields[field.name] = field_info;
        res_code += vformat("\t%s %s%s;\n", type_string, field.name, postname_string);
    }
    if (!(buffer.buffer_name.length() == 0)) {
        res_code += vformat("} %s;\n",  buffer.buffer_name);
    } else {
        res_code += "};\n";
    }
	return {buf_info, res_code};
}

Pair<TextureInfo, String> texture_to_glsl(const String &texture_name, const TextureDef &texture) {
    TextureInfo tex_info;
    String res_code;
    tex_info.format = texture.format;
    tex_info.type = texture.type;

    String tex_format = textureFormat_to_string(texture.format);
    String tex_type = tex_to_glsl_name(texture.type);
    res_code += vformat("%s) restrict uniform %s %s;\n", tex_format, tex_type, texture_name);
	return {tex_info, res_code};
}