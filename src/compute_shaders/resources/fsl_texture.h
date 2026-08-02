#pragma once
#include "fsl_resource.h"

class FSLTextureView : public FSLTexture {
    GDCLASS(FSLTextureView, FSLTexture)
private:
protected:
    uint32_t mip_level = 1;
    uint32_t layer = 1;
    bool layer_only = false;
    Ref<FSLTexture> parent;
    RenderingDevice::TextureSliceType slice_type;
    static void _bind_methods();

    void _rebuild() override;
    void _rebuild_wrapper(RID _) {
        _rebuild();
    };
public:
    FSLTextureView() = default;

    // static Ref<FSLTextureView> new_view(RenderingDevice* new_rd, Ref<FSLTexture> _parent);
    static Ref<FSLTextureView> new_mip_view(RenderingDevice* new_rd, Ref<FSLTexture> _parent, uint32_t mip_level, RenderingDevice::TextureSliceType slice_type);
    static Ref<FSLTextureView> new_mip_layer_view(RenderingDevice* new_rd, Ref<FSLTexture> _parent, uint32_t mip_level, uint32_t layer, RenderingDevice::TextureSliceType slice_type);

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLTextureView();
};

class FSLTexture2D : public FSLTexture {
    GDCLASS(FSLTexture2D, FSLTexture)
private:
protected:
    uint32_t width = 1, height = 1;
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
    TypedArray<Ref<Image>> data;
    static void _bind_methods();

    void _rebuild() override;
public:
    FSLTexture2DArray() = default;

    static Ref<FSLTexture2DArray> new_texture(RenderingDevice* new_rd, TextureInfo tex_info);

    void set_textures(uint32_t tex_width, uint32_t tex_height, uint32_t tex_count, TypedArray<Ref<Image>> images = {});
    void set_texture(uint32_t tex_width, uint32_t tex_height, uint32_t index, Ref<Image> image = {});

    Ref<FSLTextureView> get_mip_view(uint32_t mip_level);
    Ref<FSLTextureView> get_mip_layer_view(uint32_t mip_level, uint32_t layer);

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLTexture2DArray();
};