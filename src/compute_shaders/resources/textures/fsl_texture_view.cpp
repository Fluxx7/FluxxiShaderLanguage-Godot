#include "../fsl_texture.h"
#include "fsl_texture.h"

/*******************************************************/
/******************* FSLTextureView ********************/
/*******************************************************/

void FSLTextureView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_rebuild_wrapper", "_"), &FSLTextureView::_rebuild_wrapper);
}

void FSLTextureView::_rebuild() {
    rebuilt = true;
    if (rid.is_valid()) {
        rid = RID();
    }
    
    rduniform = memnew(RDUniform);
    if (layer_only) {
        // TODO: implement this
    } else {
        if (parent->get_mip_count() > mip_level) {
            rid = rd->texture_create_shared_from_slice(memnew(RDTextureView), parent->get_rid(), 0, mip_level, 1, slice_type);
        } else {
            // if the texture doesn't have the targeted mip level, get the highest mip instead
            rid = rd->texture_create_shared_from_slice(memnew(RDTextureView), parent->get_rid(), 0, parent->get_mip_count() - 1, 1, slice_type);
        }
    }   
    
    
    
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    rduniform->add_id(rid);
    emit_signal("rid_changed", rid);
}


Ref<FSLTextureView> FSLTextureView::new_mip_view(RenderingDevice *new_rd, Ref<FSLTexture> _parent, uint32_t mip_level, RenderingDevice::TextureSliceType slice_type) {
	Ref<FSLTextureView> new_tex = memnew(FSLTextureView);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLTextureView()));
        }
    }
    new_tex->rd = rd;
    new_tex->parent = _parent;
    new_tex->mip_level = mip_level;
    new_tex->slice_type = slice_type;
    _parent->connect_and_call(Callable(new_tex.ptr(), "_rebuild_wrapper"));
	return new_tex;
}

Ref<FSLTextureView> FSLTextureView::new_mip_layer_view(RenderingDevice *new_rd, Ref<FSLTexture> _parent, uint32_t mip_level, uint32_t layer, RenderingDevice::TextureSliceType slice_type) {
	Ref<FSLTextureView> new_tex = memnew(FSLTextureView);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLTextureView()));
        }
    }
    new_tex->rd = rd;
    new_tex->parent = _parent;
    new_tex->mip_level = mip_level;
    new_tex->slice_type = slice_type;
    new_tex->layer = layer;
    new_tex->layer_only = true;
    _parent->connect_and_call(Callable(*new_tex, "_rebuild_wrapper"));
	return new_tex;
}

Ref<RDUniform> FSLTextureView::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
	needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}
FSLTextureView::~FSLTextureView() {
    /*  
    slice RIDs are freed automatically when the source texture is freed.
    if the reference to the parent FSLTexture was still live when freeing the slice RID,
    there would be no issues, but the FSLResource destructor runs after the FSLTextureView destructor
    for a given FSLTextureView, and the parent reference is destroyed in the FSLTextureView destructor.
    Therefore, if all FSLResources are being freed for any reason, such as the program closing,
    then the parent reference may be the only remaining reference to the parent, resulting in the parent being freed 
    after this destructor runs, but before the FSLResource destructor runs to free the RID.

    so this prevents the FSLResource destructor from double freeing the RID
     */
    if (rid.is_valid() && rd != nullptr) {
        rid = RID();
    }
}