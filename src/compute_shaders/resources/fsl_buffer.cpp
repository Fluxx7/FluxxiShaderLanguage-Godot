#include "fsl_buffer.h"
#include "fsl_resource.h"

void FSLBuffer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_flag", "flag"), &FSLBuffer::add_flag);
    ClassDB::bind_method(D_METHOD("set_flags", "flags"), &FSLBuffer::set_flags);
    ClassDB::bind_method(D_METHOD("remove_flag", "flag"), &FSLBuffer::remove_flag);
}
void FSLBuffer::add_flag(uint64_t flag) {
    flag_long |= flag;
    flags.push_back(flag);
    _rebuild();
}

void FSLBuffer::set_flags(uint64_t flags) {
    flag_long = 0;
    flag_long |= flags;
    _rebuild();
}

void FSLBuffer::remove_flag(uint64_t flag) {
    flag_long &= ~flag;
    _rebuild();
}
