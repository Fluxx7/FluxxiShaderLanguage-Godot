#pragma once

#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/templates/span.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/templates/hash_map.hpp"

#include "fsl_lexer.h"
#include <optional>
#include <stack>

using namespace godot;

class FSLValidator {
protected:

public:
    FSLValidator() = default;
    bool validate_ast(const fslAST& ast);
};