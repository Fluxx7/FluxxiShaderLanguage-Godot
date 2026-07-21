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

template<typename T>
class Stream {
protected:
    bool errored = false;
    godot::Span<T> source;
    uint32_t start;
    uint32_t index = 0;
    uint32_t end;
public:
    bool ok() const { return !errored; }
    bool at_end() const { return index >= end; };

    const T* peek() const {
        return at_end() ? nullptr : &source[index];
    }
    const T* consume() {
        if (at_end()) {
            errored = true;
            return nullptr;
        }
        return &source[index++];
    }

    Stream() = default;
    Stream(godot::Span<T> _source, uint32_t _start, uint32_t _end) : start(_start), source(_source), end(_end) { index = _start; }
};