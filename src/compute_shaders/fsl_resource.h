#pragma once
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/a_hash_map.hpp"
#include "godot_cpp/classes/rd_uniform.hpp"
#include "godot_cpp/classes/rd_texture_format.hpp"
#include "godot_cpp/classes/rd_texture_view.hpp"
#include "godot_cpp/classes/texture2drd.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/classes/image.hpp"

#include "ast_defs.h"
#include <optional>

class FSLBuffer : public RefCounted {
private:
protected:
    RID buffer_rid;
public:
    ~FSLBuffer();
};

class FSLTexture : public RefCounted {
private:
protected:
public:
};