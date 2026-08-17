#pragma once
#include "godot_imports.h"
#include "std_imports.h"


inline uint32_t is_valid_buffer_annotation(StringName annotation) {
    static const AHashMap<StringName, uint32_t> valid_annotations = {
		{"godot_vertex", 1}, // I don't feel like adding an enum for this right now
		{"godot_index", 2},
		{"flag_indirect", 3},
	};
    if (!valid_annotations.has(annotation)) {
        return 0;
    }
    return valid_annotations[annotation];
}
