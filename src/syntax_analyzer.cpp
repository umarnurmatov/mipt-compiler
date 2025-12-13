#include "syntax_analyzer.h"

#include "ast.h"
#include "logutils.h"
#include "memutils.h"
#include "token.h"
#include "utils.h"
#include "vector.h"
#include "compiler_error.h"
#include <cstddef>
#include <exception>

namespace compiler {
namespace syntax {

static const char* LOG_SYNTAX = "SYNTAX";

#ifdef _DEBUG

#define SYNTAX_ANANLYZER_ASSERT_OK_(ast)                         \
    {                                                            \
        Err err = verify_(ast);                                  \
        if(err != ERR_NONE) {                                    \
            UTILS_LOGE(LOG_SYNTAX, "%s", compiler::strerr(err)); \
            utils_assert(err == ERR_NONE);                       \
        }                                                        \
    }

#else

#define SYNTAX_ANANLYZER_ASSERT_OK_(ast)

#endif // _DEBUG

static ast::ASTNode* get_general_(SyntaxAnalyzer* analyzer);

static ast::ASTNode* get_func_(SyntaxAnalyzer* analyzer);

static ast::ASTNode* get_block_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_statement_(SyntaxAnalyzer* analyzer);

static ast::ASTNode* get_while_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_if_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_else_(SyntaxAnalyzer* analyzer);

static ast::ASTNode* get_assignment_(SyntaxAnalyzer* analyzer);

static ast::ASTNode* get_expr_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_or_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_and_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_eq_neq_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_gt_lt_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_add_sub_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_mul_div_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_pow_(SyntaxAnalyzer* analyzer);

static ast::ASTNode* get_primary_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_numeric_literal_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_identifier_(SyntaxAnalyzer* analyzer);

#ifdef _DEBUG

static Err verify_(SyntaxAnalyzer* analyzer);

#endif // _DEBUG

static ast::ASTNode* new_node(SyntaxAnalyzer* analyzer, token::Token *token, ast::ASTNode *left, ast::ASTNode *right, ast::ASTNode *parent);

Err ctor(SyntaxAnalyzer* analyzer)
{
    utils_assert(analyzer);

    const size_t to_delete_cap = 10;
    vector_ctor(&analyzer->to_delete, to_delete_cap, sizeof(ast::ASTNode*));
    analyzer->pos = 0;

    return ERR_NONE;
}

void dtor(SyntaxAnalyzer* analyzer)
{
    utils_assert(analyzer);

    vector_dtor(&analyzer->to_delete);
}

Err perform_recursive_descent(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    ast::ASTNode* root = get_general_(analyzer);

    if(!root)
        return SYNTAX_ERR;

    token::Token token = {
        .type = token::TYPE_FAKE,
        .val  = token::Value { .id = 0 }
    };

    analyzer->astree->root = 
        ast::new_node(&token, root, NULL, NULL);

    return ERR_NONE;
}

static ast::ASTNode* new_node(SyntaxAnalyzer* analyzer, token::Token *token, ast::ASTNode *left, ast::ASTNode *right, ast::ASTNode *parent)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    ast::ASTNode* node = ast::new_node(token, left, right, parent);
    vector_push(&analyzer->to_delete, &node);

    return node;
}

#define LOG_SYNTAX_ERR_(msg, ...) \
    UTILS_LOGE(LOG_SYNTAX, "%s:%ld:%ld: [%ld] syntax error: " msg, \
            analyzer->lex->buf.filename, \
            token->fileline, \
            token->filepos, \
            analyzer->pos __VA_OPT__(,) \
            __VA_ARGS__)

#define LOG_STACKTRACE \
    UTILS_LOGD(LOG_SYNTAX, "pos: %ld; val: %s, type: %s", analyzer->pos, token::value_str(CURRENT_TOKEN_), token::type_str(CURRENT_TOKEN_->type));

#define INCREMENT_POS_ analyzer->pos++

#define CURRENT_TOKEN_ \
    ((token::Token*)vector_at(&analyzer->lex->tokens, (size_t) analyzer->pos))

#define GET_CURRENT_TOKEN_(name) \
    token::Token* name = CURRENT_TOKEN_

#define NEW_NODE(token, left, right) \
    new_node(analyzer, token, left, right, NULL)

ast::ASTNode* get_general_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    ast::ASTNode* node = get_block_(analyzer);
    
    GET_CURRENT_TOKEN_(token);

    BEGIN {
        if(token->type != token::TYPE_TERMINATOR) {
            LOG_SYNTAX_ERR_(
                "expected terminator, got: %s", 
                token::value_str(token));

            GOTO_END;
        }
        else if (!node) GOTO_END;

        return node;

    } END;

    for (size_t i = 0; i < analyzer->to_delete.size; ++i)
        free(*(ast::ASTNode **)vector_at(&analyzer->to_delete, i));

    return NULL;
}

static ast::ASTNode* get_block_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    GET_CURRENT_TOKEN_(token);

    if(token->type == token::TYPE_SEPARATOR && token->val.sep_type == token::SEPARATOR_TYPE_CURLY_OPEN) {
        INCREMENT_POS_;
        token = CURRENT_TOKEN_;
    }
    else {
        LOG_SYNTAX_ERR_("expected curly brace");
        return NULL;
    }

    ast::ASTNode* root = get_statement_(analyzer);
    ast::ASTNode* node = root;
    token = CURRENT_TOKEN_;

    while(!(token->type == token::TYPE_SEPARATOR && token->val.sep_type == token::SEPARATOR_TYPE_CURLY_CLOSE)) {
        node->right = get_statement_(analyzer);
        node = node->right;
        token = CURRENT_TOKEN_;
    }

    INCREMENT_POS_;

    return root;
}

static ast::ASTNode* get_statement_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    ast::ASTNode* node = NULL;

    BEGIN {

        node = get_assignment_(analyzer);
        if(node) GOTO_END;

    } END;

    GET_CURRENT_TOKEN_(token);
    if(token->type == token::TYPE_SEPARATOR && token->val.sep_type == token::SEPARATOR_TYPE_SEMICOLON) {
        INCREMENT_POS_;
        return NEW_NODE(token, node, NULL);
    }

    LOG_SYNTAX_ERR_("expected semicolon");
    return NULL;
}

static ast::ASTNode* get_assignment_(SyntaxAnalyzer* analyzer)
{

    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    GET_CURRENT_TOKEN_(token);
    
    ast::ASTNode* left = get_identifier_(analyzer);

    token = CURRENT_TOKEN_;
    if(token->type == token::TYPE_OPERATOR && token->val.op_type == token::OPERATOR_TYPE_ASSIGN) {

        INCREMENT_POS_;

        ast::ASTNode* right  = get_expr_(analyzer);

        return NEW_NODE(token, left, right);
    }

    return NULL;
}

ast::ASTNode* get_expr_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    ast::ASTNode* node = get_or_(analyzer);

    return node;
}

#define OPERATOR_(suffix, condition, next)                  \
static ast::ASTNode* get_##suffix(SyntaxAnalyzer* analyzer) \
{                                                           \
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);                  \
                                                            \
    LOG_STACKTRACE;                                         \
                                                            \
    ast::ASTNode* node = next(analyzer);                    \
                                                            \
    GET_CURRENT_TOKEN_(token);                              \
                                                            \
    while(condition) {                                      \
                                                            \
        INCREMENT_POS_;                                     \
                                                            \
        ast::ASTNode* node_right = next(analyzer);          \
                                                            \
                                                            \
        node = NEW_NODE(token, node, node_right);           \
                                                            \
        token = CURRENT_TOKEN_;                             \
                                                            \
    }                                                       \
                                                            \
    return node;                                            \
}                                                    

OPERATOR_(
    or_, 
    token->type == token::TYPE_OPERATOR 
    && token->val.op_type == token::OPERATOR_TYPE_OR,
    get_and_);

OPERATOR_(
    and_, 
    token->type == token::TYPE_OPERATOR 
    && token->val.op_type == token::OPERATOR_TYPE_AND,
    get_eq_neq_);
                                                     
OPERATOR_(
    eq_neq_, 
    token->type == token::TYPE_OPERATOR 
    && (token->val.op_type == token::OPERATOR_TYPE_EQ 
        || token->val.op_type == token::OPERATOR_TYPE_NEQ), 
    get_gt_lt_);

OPERATOR_(
    gt_lt_, 
    token->type == token::TYPE_OPERATOR 
    && (token->val.op_type == token::OPERATOR_TYPE_LEQ 
        || token->val.op_type == token::OPERATOR_TYPE_GEQ 
        || token->val.op_type == token::OPERATOR_TYPE_LT 
        || token->val.op_type == token::OPERATOR_TYPE_GT), 
    get_add_sub_);

OPERATOR_(
    add_sub_, 
    token->type == token::TYPE_OPERATOR 
    && (token->val.op_type == token::OPERATOR_TYPE_ADD 
        || token->val.op_type == token::OPERATOR_TYPE_SUB), 
    get_mul_div_);

OPERATOR_(
    mul_div_, 
    token->type == token::TYPE_OPERATOR 
    && (token->val.op_type == token::OPERATOR_TYPE_MUL 
        || token->val.op_type == token::OPERATOR_TYPE_DIV), 
    get_pow_);

OPERATOR_(
    pow_, 
    token->type == token::TYPE_OPERATOR 
    && token->val.op_type == token::OPERATOR_TYPE_POW, 
    get_primary_);

ast::ASTNode* get_primary_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;
    ast::ASTNode* node = NULL;

    GET_CURRENT_TOKEN_(token);

    if(token->type == token::TYPE_SEPARATOR 
       && token->val.sep_type == token::SEPARATOR_TYPE_PAR_OPEN) {

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

ast::ASTNode* get_numeric_literal_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    GET_CURRENT_TOKEN_(token);

    if(token->type != token::TYPE_NUM_LITERAL)
        return NULL;

    ast::ASTNode* node = NEW_NODE(token, NULL, NULL);
    INCREMENT_POS_;

    return node;
}

ast::ASTNode* get_identifier_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    GET_CURRENT_TOKEN_(token);

    if(token->type != token::TYPE_IDENTIFIER)
        return NULL;

    ast::ASTNode* node = NEW_NODE(token, NULL, NULL);
    INCREMENT_POS_;

    return node;
}

#undef LOG_SYNTAX_ERR_
#undef GET_CURRENT_TOKEN_
#undef INCREMENT_POS_

#ifdef _DEBUG

static Err verify_(SyntaxAnalyzer* analyzer)
{
    if(!analyzer)
        return NULLPTR;

    if(analyzer->pos >= analyzer->lex->tokens.size)
        return INVALID_BUFPOS;

    return ERR_NONE;
}

#endif // _DEBUG

} // syntax
} // compiler
