#pragma once

#include "ast.h"
#include "error.h"

namespace compiler {
namespace syntax {

ast::ASTNode* parse_get_general(ast::AST* astree);

} // syntax
} // compiler
