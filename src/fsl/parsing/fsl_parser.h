#pragma once

#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/templates/hash_map.hpp"

#include "ast_defs.h"
#include <optional>
#include <stack>



using namespace godot;

class FSLParser {
public:
    enum ParserContex {
        GLOBAL,
        KERNELDECL,
        RESOURCEDECL
    };
    LocalVector<Token> shared_code;
    HashMap<StringName, KernelNode> kernel_data;
    LocalVector<Token> _tokenize(String);
    std::optional<LocalVector<Token>> _preprocess(String& path);
    void _parse_file(fslAST &ast, const LocalVector<Token> &tokens, uint32_t &out_linenum, uint32_t &last_code_linenum);
    static std::optional<fslAST> get_ast(String path);
};
