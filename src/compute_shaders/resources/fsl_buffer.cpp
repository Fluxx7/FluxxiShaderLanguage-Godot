#include "fsl_buffer.h"
#include "fsl_resource.h"

void FSLBuffer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_flag", "flag"), &FSLBuffer::add_flag);
    ClassDB::bind_method(D_METHOD("set_flags", "flags"), &FSLBuffer::set_flags);
    ClassDB::bind_method(D_METHOD("remove_flag", "flag"), &FSLBuffer::remove_flag);
    ClassDB::bind_method(D_METHOD("get_size_bytes"), &FSLBuffer::get_size_bytes);
    ClassDB::bind_method(D_METHOD("copy_to", "dest", "allow_different_size"), &FSLBuffer::copy_to, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("copy_bytes", "dest", "num_bytes", "offset_src", "offset_dest"), &FSLBuffer::copy_bytes, DEFVAL(0), DEFVAL(0));
}

void FSLBuffer::copy_to(Ref<FSLBuffer> dest, bool allow_different_size) {
    uint32_t src_size_bytes = this->get_size_bytes();
    uint32_t dest_size_bytes = dest->get_size_bytes();
    ERR_FAIL_COND_MSG((src_size_bytes != dest_size_bytes) && !allow_different_size, "Source buffer and destination buffer are not the same size");
    uint32_t bytes_to_copy = src_size_bytes > dest_size_bytes ? dest_size_bytes : src_size_bytes;
    rd->buffer_copy(this->get_rid(), dest->get_rid(), 0, 0, bytes_to_copy);
}

void FSLBuffer::copy_bytes(Ref<FSLBuffer> dest, uint32_t num_bytes, uint32_t offset_src, uint32_t offset_dest) {
    ERR_FAIL_COND_MSG(num_bytes < 1, "Buffer copy must be at least one byte in size");
    uint32_t src_size_bytes = this->get_size_bytes();
    uint32_t dest_size_bytes = dest->get_size_bytes();
    ERR_FAIL_COND_MSG((src_size_bytes + offset_src) < num_bytes, "Requested byte copy goes beyond end of source buffer");
    ERR_FAIL_COND_MSG((dest_size_bytes + offset_dest) < num_bytes, "Requested byte copy goes beyond end of destination buffer");
    rd->buffer_copy(this->get_rid(), dest->get_rid(), offset_src, offset_dest, num_bytes);
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
