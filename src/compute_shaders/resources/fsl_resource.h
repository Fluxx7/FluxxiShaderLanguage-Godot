#pragma once
#include "godot_imports.h"
#include "std_imports.h"

#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/variant/rid.hpp"
#include "godot_cpp/classes/rd_uniform.hpp"
#include "godot_cpp/classes/rd_texture_format.hpp"
#include "godot_cpp/classes/rd_texture_view.hpp"
#include "godot_cpp/classes/texture2drd.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/variant/typed_dictionary.hpp"

#include "ast_defs.h"

struct VariableInfo {
    FSLType type;
};
struct TextureInfo {
    TextureFormat format;
    TextureType type;
    uint32_t mip_count;
};

struct BufferFieldInfo {
    FSLType type;
    LocalVector<uint32_t> dimensions;
};
struct BufferInfo {
    enum BufferSpecialization {
        NONE,
        GODOT_VERTEX,
        GODOT_INDEX
    };
    BufferType type;
    BufferFormat format;
    uint32_t base_size_bytes;
    uint32_t tail_stride = 0;
    HashMap<StringName, BufferFieldInfo> fields;
    BufferSpecialization specialization = NONE;
    LocalVector<RenderingDevice::StorageBufferUsage> usage_flags;
};

enum ResourceType {
    RESTYPE_BUFFER,
    RESTYPE_TEXTURE,
    RESTYPE_UNIFORM
};

struct ResourceInfo {
    uint32_t set;
    uint32_t binding;
    ResourceType resource_type;
    sumtype<BufferInfo, TextureInfo, VariableInfo> type_info;
};

class FSLResource : public RefCounted {
    GDCLASS(FSLResource, RefCounted)
private:
protected:
    RID rid;
    RenderingDevice* rd;
    Ref<RDUniform> rduniform;
    bool rebuilt = true;
    static void _bind_methods();
    virtual void _rebuild() = 0;
public:
    RID get_rid();
    virtual Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) = 0;
    void connect_and_call(Callable event_handler);
    virtual ~FSLResource();
};

class FSLBuffer : public FSLResource {
    GDCLASS(FSLBuffer, FSLResource)
private:
protected:
    BufferInfo buffer_info;
    static void _bind_methods();
    uint64_t flag_long = 0;
    LocalVector<uint64_t> flags;
public:
    FSLBuffer() = default;

    void add_flag(uint64_t flag);
    void set_flags(uint64_t flags);
    void remove_flag(uint64_t flag);
};


class FSLTexture : public FSLResource {
    GDCLASS(FSLTexture, FSLResource)
private:
protected:
    TextureInfo texture_info;
    uint64_t flag_long = 0;
    LocalVector<uint64_t> flags;
    static void _bind_methods();
public:
    void add_flag(uint64_t flag);
    void set_flags(uint64_t flags);
    void remove_flag(uint64_t flag);
    void set_mip_count(uint32_t _mip_count);
    uint32_t get_mip_count() { return texture_info.mip_count; }
};

/**
 * This is essentially a passthrough class for a user-created RID. 
 * There are *zero* safety guarentees and no API functions, as 
 * no reflection is possible
 */
class FSLRawResource : public FSLResource {
    GDCLASS(FSLRawResource, FSLResource)
private:
protected:
    static void _bind_methods();
    void _rebuild() override;
    RenderingDevice::UniformType resource_type;
public:
    static Ref<FSLRawResource> from_rid(RID rid, RenderingDevice::UniformType);

    void set_rid(RID rid);

    void set_uniform_type(RenderingDevice::UniformType uniform_type);
    RenderingDevice::UniformType get_uniform_type();

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
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