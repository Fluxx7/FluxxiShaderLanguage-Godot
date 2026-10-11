#pragma once
#include "fsl_resource.h"

class FSLStorageBuffer : public FSLBuffer {
    GDCLASS(FSLStorageBuffer, FSLBuffer)
private:
protected:
    uint32_t size_bytes = 1;
    HashMap<StringName, Pair<uint32_t, uint32_t>> field_offsets;

    static void _bind_methods();
    void _init_buffer();
    uint32_t _get_fields_size_bytes(uint32_t unsized_count = 1);
    void _update_size_bytes(uint32_t unsized_count, PackedByteArray data = {});
    void _push_buffer_values(uint32_t offset, PackedByteArray values);
    void _rebuild() override;

    TypedDictionary<StringName, Variant> _bytes_to_fields(PackedByteArray raw_data);

public:
    uint32_t get_size_bytes() override;
    FSLStorageBuffer() = default;

    static Ref<FSLStorageBuffer> new_buffer(RenderingDevice* new_rd, BufferInfo buf_info);

    void set_field(StringName field, Variant value);
    void set_buffer(TypedDictionary<StringName, Variant> values);
    void set_buffer_bytes(PackedByteArray data, uint32_t offset = 0, uint32_t num_bytes = 0);
    void update_buffer(TypedDictionary<StringName, Variant> values);
    void set_unsized_element_count(uint32_t num_elements);

    PackedByteArray get_buffer_values_bytes(uint32_t offset = 0, uint32_t num_bytes = 0);
    TypedDictionary<StringName, Variant> get_buffer_values();
    Variant get_field_value(StringName field_name);

    void get_buffer_values_bytes_async(Callable callback, uint32_t offset = 0, uint32_t num_bytes = 0);
    void get_buffer_values_async(Callable callback);
    void get_field_value_async(StringName field_name, Callable callback);

    void copy_fields(Ref<FSLBuffer> dest, TypedArray<StringName> fields = {}, uint32_t num_elements = 0);

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLStorageBuffer();
};

class FSLUniformBuffer : public FSLBuffer {
    GDCLASS(FSLUniformBuffer, FSLBuffer)
private:
protected:
    HashMap<StringName, Pair<uint32_t, uint32_t>> field_offsets;
    uint32_t size_bytes = 1;
    PackedByteArray data_cache;
    static void _bind_methods();
    void _init_buffer();
    uint32_t _get_fields_size_bytes();
    void _push_buffer_values(uint32_t offset, PackedByteArray values);
    void _rebuild() override;

public:
    uint32_t get_size_bytes() override;
    FSLUniformBuffer() = default;

    static Ref<FSLUniformBuffer> new_buffer(RenderingDevice* new_rd, BufferInfo buf_info);

    void set_field(StringName field, Variant value);
    void set_buffer(TypedDictionary<StringName, Variant> values);
    void update_buffer(TypedDictionary<StringName, Variant> values);

    void copy_fields(Ref<FSLBuffer> dest, TypedArray<StringName> fields = {});

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLUniformBuffer();
};


class FSLVertexBuffer : public FSLBuffer {
    GDCLASS(FSLVertexBuffer, FSLBuffer)
private:
protected:
    uint32_t vertex_count = 0;
    uint32_t bytes_per_vertex = 12;
    static void _bind_methods();
    void _init_buffer();
    void _update_vertex_count(uint32_t new_vertex_count);
    void _rebuild() override;
public:
    uint32_t get_size_bytes() override;
    FSLVertexBuffer() = default;

    static Ref<FSLVertexBuffer> new_buffer(RenderingDevice* new_rd, BufferInfo buf_info);
    void set_vertex_count(uint32_t num_vertices);
    void set_vertex_size_bytes(uint32_t vertex_size);
    void set_buffer_bytes(PackedByteArray data, uint32_t offset = 0, uint32_t num_bytes = 0);

    void copy_vertices(Ref<FSLBuffer> dest, uint32_t num_vertices = 0, uint32_t offset = 0);

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLVertexBuffer();
};

class FSLIndexBuffer : public FSLBuffer {
    GDCLASS(FSLIndexBuffer, FSLBuffer)
private:
protected:
    uint32_t index_count = 0;
    RenderingDevice::IndexBufferFormat index_format = RenderingDevice::INDEX_BUFFER_FORMAT_UINT16;
    static void _bind_methods();
    void _init_buffer();
    void _update_index_count(uint32_t index_count);
    void _rebuild() override;
public:
    uint32_t get_size_bytes() override;
    FSLIndexBuffer() = default;

    static Ref<FSLIndexBuffer> new_buffer(RenderingDevice* new_rd, BufferInfo buf_info);

    void copy_indices(Ref<FSLBuffer> dest, uint32_t num_indices = 0, uint32_t offset = 0, uint32_t dest_offset_bytes = 0);
    void set_index_count(uint32_t num_indices);
    void set_index_format(RenderingDevice::IndexBufferFormat new_format);

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLIndexBuffer();
};