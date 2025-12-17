#pragma once

#include "ast.h"

#define TRANSLATOR_INILIST \
    {                      \
        .astree = NULL,    \
        .file   = NULL,    \
        .label_id = 0      \
    }

namespace compiler {

struct Translator {
    ast::AST* astree;
    FILE* file;

    int label_id;
};

void emit_program(Translator* tr);

} // compiler
