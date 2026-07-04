#pragma once
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/a_hash_map.hpp"
#include "godot_cpp/classes/rd_uniform.hpp"
#include "godot_cpp/classes/rd_texture_format.hpp"
#include "godot_cpp/classes/rd_texture_view.hpp"
#include "godot_cpp/classes/texture2drd.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/variant/typed_dictionary.hpp"

#include "ast_defs.h"
#include <optional>

struct VariableInfo {
    FSLType type;
};
struct TextureInfo {
    TextureFormat format;
    TextureType type;
};
struct BufferInfo {
    BufferType type;
    BufferFormat format;
    bool has_unsized_field = false;
    HashMap<StringName, VariableInfo> fields;
};
enum ResourceType {
    RESTYPE_BUFFER,
    RESTYPE_TEXTURE
};
struct ResourceInfo {
    uint32_t set;
    uint32_t binding;
    ResourceType resource_type;
    std::variant<BufferInfo, TextureInfo, VariableInfo> type_info;
};

class FSLResource : public RefCounted {
    GDCLASS(FSLResource, RefCounted)
private:
protected:
    RID rid;
    RenderingDevice* rd;
    LocalVector<Callable> callbacks;
    Ref<RDUniform> rduniform;
    bool rebuilt = true;
    static void _bind_methods();
public:
    ResourceType resource_type;
    virtual Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) = 0;
    // virtual void bind_callback(Callable callback);
    virtual ~FSLResource();
};

class FSLBuffer : public FSLResource {
    GDCLASS(FSLBuffer, FSLResource)
private:
protected:
    uint32_t size_bytes = 1;
    BufferInfo buffer_info;
    static void _bind_methods();
    void _init_buffer();
    void _update_size_bytes(uint32_t unsized_count);
public:
    FSLBuffer() = default;

    static Ref<FSLBuffer> new_buffer(RenderingDevice* new_rd, BufferInfo buf_info);

    void set_field(StringName field, Variant value);
    void set_buffer(TypedDictionary<StringName, Variant> values);
    void update_buffer(TypedDictionary<StringName, Variant> values);
    void set_unsized_element_count(uint32_t num_elements);
    void bind_callback(Callable callback);

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLBuffer();

};

class FSLTexture : public FSLResource {
    GDCLASS(FSLTexture, FSLResource)
private:
protected:
    uint32_t width = 1, height = 1, depth = 1;
    TextureInfo texture_info;
    static void _bind_methods();
    void _init_texture();
public:
    FSLTexture() = default;

    static Ref<FSLTexture> new_texture(RenderingDevice* new_rd, TextureInfo tex_info);

    void set_2d_texture(uint32_t tex_width, uint32_t tex_height, Ref<Image> tex = nullptr);
    void set_3d_texture(uint32_t tex_width, uint32_t tex_height, uint32_t tex_depth, Ref<Image> tex = nullptr);
    void bind_callback(Callable callback);

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLTexture();
};

class FSLUniformSet {
protected:
    RenderingDevice* rd;
    bool rebuild = true;
    RID uniform_set_id = RID();
    AHashMap<uint32_t, Ref<FSLResource>> uniforms; 
public:
    FSLUniformSet() {
        rd = RenderingServer::get_singleton()->get_rendering_device();
    }
    FSLUniformSet(RenderingDevice *in_rd) {
        rd = in_rd;
    }
    ~FSLUniformSet() {
        if (uniform_set_id.is_valid() && rd->uniform_set_is_valid(uniform_set_id)) {
            rd->free_rid(uniform_set_id);
        }
    }
    void bind_resource(Ref<FSLResource> resource, uint32_t binding) {
        uniforms[binding] = resource;
        rebuild = true;
    }
    RID get_rid(RID shader_rid, uint32_t set_index = 0);
};

inline bool tex_is_2d(TextureType tex_type) {
    switch (tex_type) {
        case TEXTURE2D:
            return true;
        default:
            return false;
    }
}

inline bool tex_is_3d(TextureType tex_type) {
    switch (tex_type) {
        case TEXTURE2DARRAY:
            return true;
        default:
            return false;
    }
}