#pragma once

#include <stdlib.h>

#include "vector.h"
#include "error.h"

namespace compiler {
namespace lexer {

#define LEXER_INITLIST            \
    {                             \
        .buf = {                  \
            .ptr = NULL,          \
            .len = 0,             \
            .pos = 0,             \
            .filename = NULL      \
        },                        \
        .tokens = VECTOR_INITLIST \
    }                             \

struct Lexer {
    struct {
        char* ptr;
        ssize_t len;
        ssize_t pos;
        const char* filename;
    } buf;

    Vector tokens;
    Vector name_table;
};

Err ctor(Lexer* lex);

void dtor(Lexer* lex);

Err lex(Lexer *lex, const char* filename);

} // compiler 
} // lexer 
