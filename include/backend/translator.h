#pragma once

#include "ast.h"

#define TRANSLATOR_INILIST  \
    {                       \
        .astree = NULL,     \
        .file   = NULL,     \
        .current_env = NULL,\
        .label_id = 0       \
    }

namespace compiler {

struct Translator {
    ast::AST* astree;
    FILE* file;

    Env* current_env;

    int label_id;
};

void emit_program(Translator* tr);

} // compiler
