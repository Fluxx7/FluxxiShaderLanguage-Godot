#pragma once
#include "fsl_resource.h"
#include "fsl_texture.h"

class FSLSampler2DArray : public FSLSamplerType {
    GDCLASS(FSLSampler2DArray, FSLSamplerType)
private:
protected:
    Ref<FSLTexture> bound_texture;
    RID bound_texture_rid;

    static void _bind_methods();

    void _rebuild() override;
public:
    FSLSampler2DArray() = default;

    static Ref<FSLSampler2DArray> new_sampler(RenderingDevice* new_rd, SamplerInfo sampler_info, Ref<FSLTexture2DArray> texture);

    void bind_texture(Ref<FSLTexture2DArray> texture);

    void bind_texture_rid(RID texture_rid);
    void _bind_texture_rid_wrapper(RID texture_rid) {
        bind_texture_rid(texture_rid);
    }

    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    ~FSLSampler2DArray();
};