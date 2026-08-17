#include "code_builder.h"

AHashMap<StringName, String> _init_type_mappings() {
    AHashMap<StringName, String> type_mappings = {
        {"boolx2", "bvec2"},
        {"boolx3", "bvec3"},
        {"boolx4", "bvec4"}
    };
    const String type_names[] = {
        "f16",
        "f32",
        "f64",
        "u8",
        "u16",
        "u32",
        "u64",
        "i8",
        "i16",
        "i32",
        "i64"
    };
    const AHashMap<char32_t, String> prefix_to_name = {
        {'f', "float"},
        {'i', "int"},
        {'u', "uint"}
    };
    for (const auto& type : type_names) { 
        if (prefix_to_name.has(type[0])) {
            type_mappings[type] = vformat("%s%s_t", prefix_to_name[type[0]], type.right(-1)); ;
            for (uint32_t vec_size : {2, 3, 4}) {
                type_mappings[vformat("%sx%d", type, vec_size)] = vformat("%svec%d", type, vec_size);
                if (type[0] == 'f') {
                    for (uint32_t mat_size : {2,3,4}) {
                        type_mappings[vformat("%sx%dx%d", type, vec_size, mat_size)] = vformat("%smat%dx%d", type, vec_size, mat_size);
                    }
                }
            }
        }   
    }
    return type_mappings;
}

AHashMap<StringName, String>& _type_mappings() {
    static AHashMap<StringName, String> type_mappings = _init_type_mappings();
    return type_mappings;
}



String to_original_type(const String &identifier) {
    const auto& type_mappings = _type_mappings();
    if (type_mappings.has(identifier)) {
        return type_mappings[identifier];
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