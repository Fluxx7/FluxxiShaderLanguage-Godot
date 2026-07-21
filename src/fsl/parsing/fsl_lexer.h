#pragma once

#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/templates/hash_map.hpp"

#include "ast_defs.h"
#include <optional>
#include <stack>



using namespace godot;


/**
 * My sorta shitty implementation of a lexer. 
 */

class FSLLexer {
protected:
public:
    FSLLexer() = default;
    LocalVector<Token> tokenize(String file_name, String lexee);
};