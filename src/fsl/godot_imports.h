#pragma once

#include <godot_cpp/core/defs.hpp>
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/classes/ref_counted.hpp"

#include "godot_cpp/templates/span.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/a_hash_map.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/file_access.hpp"

// Basic types
using godot::StringName;
using godot::String;
using godot::Variant;
using godot::Ref;

// Collections
using godot::HashMap;
using godot::AHashMap;
using godot::LocalVector;
using godot::Vector;
using godot::HashSet;
using godot::Span;
using godot::Array;
using godot::PackedByteArray;

// Functions
using godot::vformat;