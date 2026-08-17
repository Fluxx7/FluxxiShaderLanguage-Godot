#pragma once
#include "godot_imports.h"
#include "std_imports.h"

typedef HashMap<StringName, LocalVector<String>> FileSourceMap;

struct FSLError {
    enum ErrorStage {
        LEXER,
        PREPROCESSOR,
        PARSER,
        VALIDATOR
    };
    enum ErrorType {
        // Lexer errors
        // Preprocessor errors
        ERR_INCOMPLETE_MACRO,
        ERR_MACRO_ARG_COUNT,
        ERR_BAD_INCLUDE,
        ERR_FAILED_INCLUDE,
        ERR_MACRO_REDEFINITION,
        ERR_UNEXPECTED_DIRECTIVE,
        ERR_INVALID_DIRECTIVE,
        ERR_OPEN_IF,
        // Parser errors
        ERR_UNEXPECTED_EOF,
        ERR_UNEXPECTED_TOKEN,
        ERR_TOKEN_EXPECTED,
        ERR_EMPTY_STRUCT,
        ERR_REUSED_ANNOTATION,
        // Validation errors
        ERR_UNKNOWN_TYPE,
        ERR_UNKNOWN_VAR,
        ERR_UNKNOWN_FUNC,

        ERR_VAR_REDEFINITION,
        ERR_FIELD_REDEFINITION,
        ERR_FUNC_REDEFINITION,
        ERR_KERNEL_REDEFINITION,
        ERR_RESOURCE_REDEFINITION,

        ERR_INVALID_NAME_BINDING,
        ERR_ASSIGN_CPLACE,
        ERR_ASSIGN_NONPLACE,
        ERR_UNKNOWN_ANNOTATION,
        ERR_INVALID_ANNOTATION,
        ERR_UNSIZED_FIELD_NOT_LAST,
        ERR_RECURSIVE_STRUCT,
        ERR_RECURSIVE_FUNCTION
    };
    String file_path;

    uint32_t start_row, start_column, end_row, end_column;
    ErrorStage stage;
    ErrorType error_type;
    String error_message;
};

void print_fsl_error(const FSLError& error, const FileSourceMap* file_sources);