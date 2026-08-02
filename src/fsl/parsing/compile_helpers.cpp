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

String gen_binding_code(const String &name, const String &bound_name) {
    static const godot::AHashMap<StringName, Pair<StringName, String>> binding_map = {
        {"GlobalInvocationID", {"uvec3", "gl_GlobalInvocationID"}},
        {"LocalInvocationID", {"uvec3", "gl_LocalInvocationID"}},
        {"LocalInvocationIndex", {"uint", "gl_LocalInvocationIndex"}},
        {"WorkGroupID", {"uvec3", "gl_WorkGroupID"}},
        {"WorkGroupSize", {"uvec3", "gl_WorkGroupSize"}},
        {"NumWorkGroups", {"uvec3", "gl_NumWorkGroups"}}
    };
    static const char *original_names[] = {
        "gl_WorkGroupID", 
        "gl_WorkGroupSize", 
        "gl_NumWorkGroups"
    }; 
    const auto& binding = binding_map[bound_name];
    return vformat("const %s %s = %s;", binding.first, name, binding.second);
}

String tex_to_glsl_name(const TextureType &tex_type) {
    switch (tex_type) {
        case TEXTURE2D:
            return "image2D";
        case TEXTURE2DARRAY:
            return "image2DArray";
    }
}

Pair<TextureInfo, String> texture_to_glsl(const String &texture_name, const AST::TextureDef &texture) {
    TextureInfo tex_info;
    String res_code;
    tex_info.format = texture.format;
    tex_info.type = texture.type;
    tex_info.mip_count = 1;

    String tex_format = textureFormat_to_string(texture.format);
    String tex_type = tex_to_glsl_name(texture.type);
    res_code += vformat("%s) restrict uniform %s %s;\n", tex_format, tex_type, texture_name);
	return {tex_info, res_code};
}