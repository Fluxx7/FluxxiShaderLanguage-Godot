#include "../fsl_texture.h"
#include "fsl_texture.h"

/**********************************************************/
/******************* FSLTexture2DArray ********************/
/**********************************************************/

void FSLTexture2DArray::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_texture", "tex_width", "tex_height", "index", "image"), &FSLTexture2DArray::set_texture, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("set_textures", "tex_width", "tex_height", "tex_count", "images"), &FSLTexture2DArray::set_textures, DEFVAL(TypedArray<Ref<Image>>()));
    ClassDB::bind_method(D_METHOD("get_mip_view", "mip_level"), &FSLTexture2DArray::get_mip_view);
}

void FSLTexture2DArray::_rebuild() {
    rebuilt = true;
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    Ref<RDTextureFormat> rd_tex_format = memnew(RDTextureFormat);
    rd_tex_format->set_width(width);
    rd_tex_format->set_height(height);
    rd_tex_format->set_array_layers(count);
    rd_tex_format->set_texture_type(RenderingDevice::TextureType::TEXTURE_TYPE_2D_ARRAY);
    auto format = texture_info.format == RGBA16F ? RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT : RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT;
    rd_tex_format->set_mipmaps(texture_info.mip_count);
    rd_tex_format->set_format(format);
    rd_tex_format->set_usage_bits(flag_long);
    if (data.size() != 0) {
        TypedArray<PackedByteArray> img_data = {};
        for (const Ref<Image>& image : data) {
            img_data.append(image->get_data());
        }
        rid = rd->texture_create(rd_tex_format, memnew(RDTextureView), img_data);
    } else {
        rid = rd->texture_create(rd_tex_format, memnew(RDTextureView));
    }
    rduniform = memnew(RDUniform);
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    rduniform->add_id(rid);
    emit_signal("rid_changed", rid);
}

Ref<FSLTexture2DArray> FSLTexture2DArray::new_texture(RenderingDevice *new_rd, TextureInfo tex_info) {
	Ref<FSLTexture2DArray> new_tex = memnew(FSLTexture2DArray);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLTexture2DArray()));
        }
    }
    new_tex->rd = rd;
    new_tex->texture_info = tex_info;
    new_tex->flag_long = RenderingDevice::TextureUsageBits::TEXTURE_USAGE_CAN_COPY_FROM_BIT |
        RenderingDevice::TextureUsageBits::TEXTURE_USAGE_STORAGE_BIT |
        RenderingDevice::TextureUsageBits::TEXTURE_USAGE_CAN_UPDATE_BIT |
        RenderingDevice::TextureUsageBits::TEXTURE_USAGE_SAMPLING_BIT;
    new_tex->_rebuild();
	return new_tex;
}

void FSLTexture2DArray::set_textures(uint32_t tex_width, uint32_t tex_height, uint32_t tex_count, TypedArray<Ref<Image>> images) {
	if (tex_width != width || tex_height != height || tex_count != count) {
        width = tex_width;
        height = tex_height;
        count = tex_count;
        data = images;
        _rebuild();
    } else {
        if (!(images.size() == 0)) {
            uint32_t tex_index = 0;
            for (const Ref<Image>& image : images) {
                if (!(tex_index < count)) {
                    ERR_PRINT("Too many images provided for texture, excess images will be ignored");
                    break;
                }
                rd->texture_update(rid, tex_index++, image->get_data());
            }
        }   
    }
}

void FSLTexture2DArray::set_texture(uint32_t tex_width, uint32_t tex_height, uint32_t index, Ref<Image> image) {
    if (tex_width != width || tex_height != height || !(index < count)) {
        ERR_PRINT("Invalid image size or index provided");
    } else {
        data[index] = image;
        rd->texture_update(rid, index, image->get_data());
    }
}

Ref<FSLTextureView> FSLTexture2DArray::get_mip_view(uint32_t mip_level) {
	return FSLTextureView::new_mip_view(rd, this, mip_level, RenderingDevice::TEXTURE_SLICE_2D_ARRAY);
}

Ref<FSLTextureView> FSLTexture2DArray::get_mip_layer_view(uint32_t mip_level, uint32_t layer) {
	return FSLTextureView::new_mip_layer_view(rd, this, mip_level, layer, RenderingDevice::TEXTURE_SLICE_2D);
}

Ref<RDUniform> FSLTexture2DArray::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
	needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}
FSLTexture2DArray::~FSLTexture2DArray() {
}