#include "syntax_analyzer.h"

#include "ast.h"
#include "logutils.h"
#include "memutils.h"
#include "token.h"
#include "vector.h"
#include <error.h>

namespace compiler {
namespace syntax {

static const char* LOG_SYNTAX = "SYNTAX";

static ast::ASTNode* get_expr_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_mul_div_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_pow_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_primary_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_numeric_literal_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_identifier_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_func_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_general_(SyntaxAnalyzer* analyzer);

static ast::ASTNode* new_node(SyntaxAnalyzer* analyzer, token::Token *token, ast::ASTNode *left, ast::ASTNode *right, ast::ASTNode *parent);

Err ctor(SyntaxAnalyzer* analyzer)
{
    const size_t to_delete_cap = 10;
    vector_ctor(&analyzer->to_delete, to_delete_cap, sizeof(ast::ASTNode*));
    analyzer->pos = 0;

    return ERR_NONE;
}

void dtor(SyntaxAnalyzer* analyzer)
{
    vector_dtor(&analyzer->to_delete);
}

Err perform_recursive_descent(SyntaxAnalyzer* analyzer)
{
    analyzer->astree->root = get_general_(analyzer);

    return ERR_NONE;
}

static ast::ASTNode* new_node(SyntaxAnalyzer* analyzer, token::Token *token, ast::ASTNode *left, ast::ASTNode *right, ast::ASTNode *parent)
{
    ast::ASTNode* node = ast::new_node(token, left, right, parent);
    vector_push(&analyzer->to_delete, &node);

    return node;
}

#define LOG_SYNTAX_ERR_(msg, ...)

#define INCREMENT_POS_ analyzer->pos++

#define CURRENT_TOKEN_ \
    ((token::Token*)vector_at(&analyzer->lex->tokens, analyzer->pos))

#define GET_CURRENT_TOKEN_(name) \
    token::Token* name = CURRENT_TOKEN_

#define NEW_NODE(left, right, parent) \
    new_node(analyzer, token, left, right, parent)

ast::ASTNode* get_numeric_literal_(SyntaxAnalyzer* analyzer)
{
    UTILS_LOGI(LOG_SYNTAX, "numeric");

    GET_CURRENT_TOKEN_(token);

    if(token->type != token::TYPE_LITERAL)
        return NULL;

    ast::ASTNode* node = NEW_NODE(NULL, NULL, NULL);
    INCREMENT_POS_;

    return node;
}

ast::ASTNode* get_identifier_(SyntaxAnalyzer* analyzer)
{

    GET_CURRENT_TOKEN_(token);

    if(token->type != token::TYPE_IDENTIFIER)
        return NULL;

    ast::ASTNode* node = NEW_NODE(NULL, NULL, NULL);
    INCREMENT_POS_;

    return node;
}

ast::ASTNode* get_expr_(SyntaxAnalyzer* analyzer)
{
    UTILS_LOGI(LOG_SYNTAX, "expr");

    ast::ASTNode* node = get_mul_div_(analyzer);

    GET_CURRENT_TOKEN_(token);

    while(token->type == token::TYPE_OPERATOR && ( token->val.op_type == token::OPERATOR_TYPE_ADD || token->val.op_type == token::OPERATOR_TYPE_SUB)) {

        INCREMENT_POS_;

        ast::ASTNode* node_right = get_mul_div_(analyzer);

        node = NEW_NODE(node, node_right, NULL);
        
        token = CURRENT_TOKEN_;
    }

    return node;
}

ast::ASTNode* get_mul_div_(SyntaxAnalyzer* analyzer)
{

    UTILS_LOGI(LOG_SYNTAX, "mul_div");
    ast::ASTNode* node = get_pow_(analyzer);

    GET_CURRENT_TOKEN_(token);

    while(token->type == token::TYPE_OPERATOR && (token->val.op_type == token::OPERATOR_TYPE_MUL || token->val.op_type == token::OPERATOR_TYPE_DIV)) {

        INCREMENT_POS_;

        ast::ASTNode* node_right = get_pow_(analyzer);

        node = NEW_NODE(node, node_right, NULL);

        token = CURRENT_TOKEN_;
    }

    return node;
}

ast::ASTNode* get_pow_(SyntaxAnalyzer* analyzer)
{
    UTILS_LOGI(LOG_SYNTAX, "pow");
    ast::ASTNode* node = get_primary_(analyzer);

    GET_CURRENT_TOKEN_(token);

    while(token->type == token::TYPE_OPERATOR && token->val.op_type == token::OPERATOR_TYPE_POW) {

        INCREMENT_POS_;
        ast::ASTNode* node_right = get_primary_(analyzer);

        node = NEW_NODE(node, node_right, NULL);
        
        token = CURRENT_TOKEN_;
    }

    return node;
}

ast::ASTNode* get_primary_(SyntaxAnalyzer* analyzer)
{
    UTILS_LOGI(LOG_SYNTAX, "primary");
    ast::ASTNode* node = NULL;

    GET_CURRENT_TOKEN_(token);

    if(token->type == token::TYPE_SEPARATOR && token->val.sep_type == token::SEPARATOR_TYPE_PAR_OPEN) {

        INCREMENT_POS_;

        node = get_expr_(analyzer);

        INCREMENT_POS_;

        return node;
    }

    node = get_numeric_literal_(analyzer);
    if(node) return node;

    // node = get_func_(analyzer);
    // if(node) return node;

    node = get_identifier_(analyzer);

    return node;
}

ast::ASTNode* get_general_(SyntaxAnalyzer* analyzer)
{
    ast::ASTNode* node = get_expr_(analyzer);

    UTILS_LOGI(LOG_SYNTAX, "%lu", analyzer->lex->tokens.size);
    UTILS_LOGI(LOG_SYNTAX, "%lu", analyzer->pos);
    
    GET_CURRENT_TOKEN_(token);
    if(token->type != token::TYPE_TERMINATOR) {
        // LOG_SYNTAX_ERR_("expected: \\n, got: (ASCII) %d", (int)BUF_AT_POS_);
        UTILS_LOGE(LOG_SYNTAX, "error");

        for(size_t i = 0; i < analyzer->to_delete.size; ++i)
            free(*(ast::ASTNode**)vector_at(&analyzer->to_delete, i));

        return NULL;
    }

    return node;
}

#undef LOG_SYNTAX_ERR_
#undef GET_CURRENT_TOKEN_
#undef INCREMENT_POS_

} // syntax
} // compiler
