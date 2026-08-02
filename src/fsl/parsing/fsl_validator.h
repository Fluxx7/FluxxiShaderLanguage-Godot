#pragma once
#include "godot_imports.h"
#include "std_imports.h"
#include "fsl_lexer.h"


class FSLValidator {
protected:
public:
    FSLValidator() = default;
    static bool validate_ast(const AST::fslAST& ast);
};