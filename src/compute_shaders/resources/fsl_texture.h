#pragma once
#include "fsl_resource.h"

class FSLTexture2D : public FSLTexture {
    GDCLASS(FSLTexture2D, FSLTexture)
private:
protected:
    uint32_t width = 1, height = 1;
    TextureInfo texture_info;
    Ref<Image> data = nullptr;
    static void _bind_methods();

    void _rebuild() override;
public:
    FSLTexture2D() = default;

    static Ref<FSLTexture2D> new_texture(RenderingDevice* new_rd, TextureInfo tex_info);

    void set_texture(uint32_t tex_width, uint32_t tex_height, Ref<Image> tex = nullptr);

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLTexture2D();
};

class FSLTexture2DArray : public FSLTexture {
    GDCLASS(FSLTexture2DArray, FSLTexture)
private:
protected:
    uint32_t width = 1, height = 1;
    uint32_t count = 1;
    TextureInfo texture_info;
    TypedArray<Ref<Image>> data;
    static void _bind_methods();

    void _rebuild() override;
public:
    FSLTexture2DArray() = default;

    static Ref<FSLTexture2DArray> new_texture(RenderingDevice* new_rd, TextureInfo tex_info);

    void set_textures(uint32_t tex_width, uint32_t tex_height, uint32_t tex_count, TypedArray<Ref<Image>> images = {});
    void set_texture(uint32_t tex_width, uint32_t tex_height, uint32_t index, Ref<Image> image = {});

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLTexture2DArray();
};