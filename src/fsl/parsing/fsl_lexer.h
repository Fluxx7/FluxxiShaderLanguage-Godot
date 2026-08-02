#pragma once
#include "godot_imports.h"
#include "std_imports.h"

#include "ast_defs.h"


/**
 * My sorta shitty implementation of a lexer. 
 */

class FSLLexer {
protected:
    
public:
    FSLLexer() = default;
    optional<LocalVector<Token>> tokenize(String file_name, String lexee);
    optional<String> clean(const String& source);
};