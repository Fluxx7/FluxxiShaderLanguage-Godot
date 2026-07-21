#include "init_helpers.h"

Ref<FSLResource> _init_buffer(const BufferInfo &buf_info, RenderingDevice *rd) {
    if (buf_info.type == UNIFORM) {
        return FSLUniformBuffer::new_buffer(rd, buf_info);
    } else {
        switch (buf_info.format) {
            case VERTEX:
                return FSLVertexBuffer::new_buffer(rd, buf_info);
            case INDEX:
                return FSLIndexBuffer::new_buffer(rd, buf_info);
            default:
                return FSLStorageBuffer::new_buffer(rd, buf_info);
        }
    }
}

Ref<FSLResource> _init_texture(const TextureInfo &tex_info, RenderingDevice *rd) {
    switch (tex_info.type) {
        case TEXTURE2D:
            return FSLTexture2D::new_texture(rd, tex_info);
        case TEXTURE2DARRAY:
            return FSLTexture2DArray::new_texture(rd, tex_info);
    }
}

Ref<FSLResource> init_resource(const ResourceInfo &res_info, RenderingDevice *rd) {
    Ref<FSLResource> new_resource = Ref<FSLResource>();
    std::visit(overload{
            [&](const BufferInfo &buf_info) {
                new_resource = _init_buffer(buf_info, rd);
            },
            [&](const TextureInfo &tex_info) {
                new_resource = _init_texture(tex_info, rd);
            },
            [&](const VariableInfo &var_info)  {
                print_error("\t\tHow the fuck");
            }
    }, res_info.type_info);
	return new_resource;
}