#pragma once
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/templates/span.hpp"
#include <functional>
#include <optional>
#include <variant>

#include "stream.h"

template<typename T>
class Slice {
protected:
    godot::Span<T> source;
    uint32_t start; // starting index of slice
    uint32_t end; // index after end of slice
    uint32_t len; // length of slice (end - start)
public:
    const T& operator[](uint32_t index) const {
        return source[start + index];
    }
    Stream<T> get_stream() const {
        return Stream<T>(source, start, end);
    }
    uint32_t length() const { return len; };
    Slice<T> slice(uint32_t _start, uint32_t _len) const {
        return make_slice(source, start + _start, _len);
    }
    static Slice<T> make_slice(const godot::LocalVector<T>& base_vector, uint32_t start, uint32_t len) {
        Slice<T> new_slice;
        new_slice.source = base_vector.span();
        new_slice.start = start;
        new_slice.len = len;
        new_slice.end = start + len;
        return new_slice;
    }
    static Slice<T> make_slice(godot::Span<T> source, uint32_t start, uint32_t len) {
        Slice<T> new_slice;
        new_slice.source = source;
        new_slice.start = start;
        new_slice.len = len;
        new_slice.end = start + len;
        return new_slice;
    }
};


