#pragma once
#include "fsl/std_imports.h"
#include "fsl/godot_imports.h"

template<typename T>
class Stream {
protected:
    bool errored = false;
    Span<T> source;
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

    void reset() {
        index = start;
        errored = false;
    }

    uint32_t get_index() const {
        return index;
    }
    Span<T> get_source() const {
        return source;
    }

    Stream() = default;
    Stream(Stream&& stream) = default;
    Stream(const Stream& stream) = default;

    Stream(Stream&& stream, uint32_t _start) {
        source = std::move(stream.source);
        start = _start;
        end = stream.end;
        index = stream.index < _start ? _start : stream.index;
    };
    Stream(const Stream& stream, uint32_t _start) {
        source = stream.source;
        start = _start;
        end = stream.end;
        index = _start;
    };

    Stream(Span<T> _source, uint32_t _start, uint32_t _end) : start(_start), source(_source), end(_end) { index = _start; }
};