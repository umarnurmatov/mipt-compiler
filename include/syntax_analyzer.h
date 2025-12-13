#pragma once

#include "ast.h"
#include "compiler_error.h"
#include "lexer.h"

namespace compiler {
namespace syntax {

struct SyntaxAnalyzer
{
    lexer::Lexer* lex;
    ast::AST* astree;
    ssize_t pos;

    Vector to_delete;
};

Err ctor(SyntaxAnalyzer* analyzer);

void dtor(SyntaxAnalyzer* analyzer);

Err perform_recursive_descent(SyntaxAnalyzer* analyzer);

} // syntax
} // compiler
