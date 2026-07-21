#include "fsl_resource.h"

void FSLTexture::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_flag", "flag"), &FSLTexture::add_flag);
    ClassDB::bind_method(D_METHOD("set_flags", "flags"), &FSLTexture::set_flags);
    ClassDB::bind_method(D_METHOD("remove_flag", "flag"), &FSLTexture::remove_flag);
}

void FSLTexture::add_flag(uint64_t flag) {
    flag_long |= flag;
    flags.push_back(flag);
    _rebuild();
}

void FSLTexture::set_flags(uint64_t flags) {
    flag_long = 0;
    flag_long |= flags;
    _rebuild();
}

void FSLTexture::remove_flag(uint64_t flag) {
    flag_long &= !flag;
    _rebuild();
}
