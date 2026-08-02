#include "../fsl_texture.h"


void FSLTexture2D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_texture", "tex_width", "tex_height", "image"), &FSLTexture2D::set_texture, DEFVAL(nullptr));
}

void FSLTexture2D::_rebuild() {
    rebuilt = true;
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    Ref<RDTextureFormat> rd_tex_format = memnew(RDTextureFormat);
    rd_tex_format->set_width(width);
    rd_tex_format->set_height(height);
    rd_tex_format->set_texture_type(RenderingDevice::TextureType::TEXTURE_TYPE_2D);
    auto format = texture_info.format == RGBA16F ? RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT : RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT;
    rd_tex_format->set_mipmaps(texture_info.mip_count);
    rd_tex_format->set_format(format);
    rd_tex_format->set_usage_bits(flag_long);
    if (data != nullptr) {
        rid = rd->texture_create(rd_tex_format, memnew(RDTextureView), {data->get_data()});
    } else {
        rid = rd->texture_create(rd_tex_format, memnew(RDTextureView));
    }
    rduniform = memnew(RDUniform);
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    rduniform->add_id(rid);
    emit_signal("rid_changed", rid);
}

Ref<FSLTexture2D> FSLTexture2D::new_texture(RenderingDevice *new_rd, TextureInfo tex_info) {
    Ref<FSLTexture2D> new_tex = memnew(FSLTexture2D);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLTexture2D()));
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

void FSLTexture2D::set_texture(uint32_t tex_width, uint32_t tex_height, Ref<Image> tex) {
    if (tex_width != width || tex_height != height) {
        width = tex_width;
        height = tex_height;
        data = tex;
        _rebuild();
    } else {
        if (tex != nullptr) {
            rd->texture_update(rid, 0, tex->get_data());
        }   
    }
}

Ref<RDUniform> FSLTexture2D::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
    needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}

FSLTexture2D::~FSLTexture2D() {
}

