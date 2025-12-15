#include "syntax_analyzer.h"

#include "ast.h"
#include "logutils.h"
#include "memutils.h"
#include "symbol.h"
#include "token.h"
#include "utils.h"
#include "vector.h"
#include "compiler_error.h"

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

static ast::ASTNode* get_program_(SyntaxAnalyzer* analyzer);

static ast::ASTNode* get_parameter_list_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_func_decl_(SyntaxAnalyzer* analyzer);

static ast::ASTNode* get_argument_list_(SyntaxAnalyzer* analyzer);
static ast::ASTNode* get_func_call_(SyntaxAnalyzer* analyzer);

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
        .val  = { .num = 0 }
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

#define LOG_SYNTAX_ERR_(msg, ...)                        \
    UTILS_LOGE(LOG_SYNTAX,                               \
            "[pos:%ld] %s:%ld:%ld: syntax error: " msg,  \
            analyzer->pos,                               \
            analyzer->lex->buf.filename,                 \
            CURRENT_TOKEN_->fileline,                    \
            CURRENT_TOKEN_->filepos __VA_OPT__(,)        \
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

    ast::ASTNode* node = get_program_(analyzer);
    
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

static ast::ASTNode* get_program_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer)
                                                            
    LOG_STACKTRACE

    ast::ASTNode* node = get_func_decl_(analyzer);

    token::Token token = {
        .type = token::TYPE_SEPARATOR,
        .val = token::Value { .sep_type = token::SEPARATOR_TYPE_CURLY_OPEN }
    };

    while(node) {                                      

        ast::ASTNode* node_right = get_func_decl_(analyzer);

        if(!node_right) break;

        node = NEW_NODE(&token, node, node_right);
    }
                                                            
    return node;
}

static ast::ASTNode* get_func_decl_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer)
                                                            
    LOG_STACKTRACE
                                                            
    GET_CURRENT_TOKEN_(token_defun);

    if(token_defun->type == token::TYPE_KEYWORD
       && token_defun->val.kw_type == token::KEYWORD_TYPE_DEFUN) {

        INCREMENT_POS_;
    }
    else
        return NULL;

    ast::ASTNode* node_id = get_identifier_(analyzer);
    
    if(!node_id) {
        LOG_SYNTAX_ERR_("expected symbol name");
        return NULL;
    }

    GET_CURRENT_TOKEN_(token);
    if(token->type == token::TYPE_SEPARATOR
       && token->val.sep_type == token::SEPARATOR_TYPE_PAR_OPEN) {

        INCREMENT_POS_;
    }
    else {
        LOG_SYNTAX_ERR_("expected open paranthesis");
        return NULL;
    }

    ast::ASTNode* node_parlist = get_parameter_list_(analyzer);

    token = CURRENT_TOKEN_;
    if(token->type == token::TYPE_SEPARATOR
       && token->val.sep_type == token::SEPARATOR_TYPE_PAR_CLOSE) {

        INCREMENT_POS_;
    }
    else {
        LOG_SYNTAX_ERR_("expected open paranthesis");
        return NULL;
    }
    
    ast::ASTNode* node_body = get_block_(analyzer);
    if(!node_body) {
        LOG_SYNTAX_ERR_("expected function body");
        return NULL;
    }

    int sym_id = ast::add_symbol(
        analyzer->astree, 
        &node_id->token.val.str, 
        SYMBOL_TYPE_FUNCTION);

    utils_assert(sym_id >= 0);

    node_id->token.id = sym_id;
    node_id->left = node_parlist;
    node_id->right = node_body;

    return node_id;
}

static ast::ASTNode* get_parameter_list_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer)
                                                            
    LOG_STACKTRACE
                                                            
    ast::ASTNode* node = get_identifier_(analyzer);

    if(node) {
        int sym_id = ast::add_symbol(
            analyzer->astree, 
            &node->token.val.str, 
            SYMBOL_TYPE_PARAMETER);

        node->token.id = sym_id;
    }

    GET_CURRENT_TOKEN_(token);
                                                            
    while(token->type == token::TYPE_SEPARATOR
          && token->val.sep_type == token::SEPARATOR_TYPE_COMMA) {                                      
        INCREMENT_POS_;

        ast::ASTNode* node_right = get_identifier_(analyzer);

        int sym_id = ast::add_symbol(
            analyzer->astree, 
            &node_right->token.val.str, 
            SYMBOL_TYPE_PARAMETER);

        UTILS_LOGD(LOG_SYNTAX, "%d", sym_id);

        node_right->token.id = sym_id;

        node = NEW_NODE(token, node, node_right);

        token = CURRENT_TOKEN_;
    }
                                                            
    return node;
}

static ast::ASTNode* get_func_call_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer)
                                                            
    LOG_STACKTRACE
                                                            
    ssize_t pos_prev = analyzer->pos;

    ast::ASTNode* node_ident = get_identifier_(analyzer);
                                                            
    GET_CURRENT_TOKEN_(token);

    if(token->type == token::TYPE_SEPARATOR
       && token->val.sep_type == token::SEPARATOR_TYPE_PAR_OPEN) {

        INCREMENT_POS_;
        token = CURRENT_TOKEN_;
    }
    else {
        analyzer->pos = pos_prev;
        NFREE(node_ident);
        return NULL;
    }

    ast::ASTNode* node_arg = get_argument_list_(analyzer);
    
    token = CURRENT_TOKEN_;

    if(token->type == token::TYPE_SEPARATOR
       && token->val.sep_type == token::SEPARATOR_TYPE_PAR_CLOSE) {

        INCREMENT_POS_;

        token::Token tok = {
            .type = token::TYPE_CALL
        };

        return NEW_NODE(&tok, node_ident, node_arg);
    }

    LOG_SYNTAX_ERR_("expected closing paranthesis");
    return NULL;
}

static ast::ASTNode* get_argument_list_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer)
                                                            
    LOG_STACKTRACE
                                                            
    ast::ASTNode* node = get_expr_(analyzer);
                                                            
    GET_CURRENT_TOKEN_(token);
                                                            
    while(token->type == token::TYPE_SEPARATOR
          && token->val.sep_type == token::SEPARATOR_TYPE_COMMA) {                                      
        INCREMENT_POS_;

        ast::ASTNode* node_right = get_expr_(analyzer);

        node = NEW_NODE(token, node, node_right);

        token = CURRENT_TOKEN_;
    }
                                                            
    return node;
}

static ast::ASTNode* get_if_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    GET_CURRENT_TOKEN_(token);

    if(token->type == token::TYPE_KEYWORD 
       && token->val.kw_type == token::KEYWORD_TYPE_IF){

        INCREMENT_POS_;

        ast::ASTNode* node_condition = get_expr_(analyzer);

        if(!node_condition) {
            LOG_SYNTAX_ERR_("expected if-condition");
            return NULL;
        }

        ast::ASTNode* node_body = get_block_(analyzer);

        if(!node_condition) {
            LOG_SYNTAX_ERR_("expected if-body");
            return NULL;
        }

        ast::ASTNode* node_else = get_else_(analyzer);

        if(node_else) {
            node_else->left = node_body;
            return NEW_NODE(token, node_condition, node_else);
        }

        return NEW_NODE(token, node_condition, node_body);
    }

    return NULL;
}

static ast::ASTNode* get_while_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    GET_CURRENT_TOKEN_(token);

    if(token->type == token::TYPE_KEYWORD 
       && token->val.kw_type == token::KEYWORD_TYPE_WHILE){

        INCREMENT_POS_;

        ast::ASTNode* node_condition = get_expr_(analyzer);

        if(!node_condition) {
            LOG_SYNTAX_ERR_("expected while-condition");
            return NULL;
        }

        ast::ASTNode* node_body = get_block_(analyzer);

        if(!node_body) {
            LOG_SYNTAX_ERR_("expected while-body");
            return NULL;
        }

        return NEW_NODE(token, node_condition, node_body);
    }

    return NULL;
}

static ast::ASTNode* get_else_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    GET_CURRENT_TOKEN_(token);

    if(token->type == token::TYPE_KEYWORD 
       && token->val.kw_type == token::KEYWORD_TYPE_ELSE){

        INCREMENT_POS_;

        ast::ASTNode* node_body = get_block_(analyzer);

        if(!node_body) {
            LOG_SYNTAX_ERR_("expected else-body");
            return NULL;
        }

        return NEW_NODE(token, NULL, node_body);
    }

    return NULL;
}

static ast::ASTNode* get_block_(SyntaxAnalyzer* analyzer)
{
    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    GET_CURRENT_TOKEN_(token);

    if(token->type == token::TYPE_SEPARATOR 
       && token->val.sep_type == token::SEPARATOR_TYPE_CURLY_OPEN) {

        INCREMENT_POS_;
        token = CURRENT_TOKEN_;
    }
    else
        return NULL;

    ast::ASTNode* root = get_statement_(analyzer);

    if(!root) return NULL;

    ast::ASTNode* node = root, *right = NULL;
    token = CURRENT_TOKEN_;

    while(!(token->type == token::TYPE_SEPARATOR 
          && token->val.sep_type == token::SEPARATOR_TYPE_CURLY_CLOSE)) {

        right = get_statement_(analyzer);

        if(!right) {
            LOG_SYNTAX_ERR_("expected statement");
            return NULL;
        }

        node->right = right;
        node = right;
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
    bool semicol_needed = true;

    BEGIN {

        node = get_while_(analyzer);
        if(node) {
            semicol_needed = false;
            GOTO_END;
        }

        node = get_if_(analyzer);
        if(node) {
            semicol_needed = false;
            GOTO_END;
        }

        node = get_assignment_(analyzer);
        if(node) GOTO_END;

        node = get_expr_(analyzer);
        if(node) GOTO_END;

    } END;

    GET_CURRENT_TOKEN_(token);

    if(semicol_needed) {
        if(token->type == token::TYPE_SEPARATOR 
           && token->val.sep_type == token::SEPARATOR_TYPE_SEMICOLON) {
            INCREMENT_POS_;
            return NEW_NODE(token, node, NULL);
        }
        else {
            LOG_SYNTAX_ERR_("expected semicolon");
            return NULL;
        }
    }

    token::Token semicol = {
        .type = token::TYPE_SEPARATOR,
        .val = token::Value 
            { .sep_type = token::SEPARATOR_TYPE_SEMICOLON }
    };

    return NEW_NODE(&semicol, node, NULL);

}

static ast::ASTNode* get_assignment_(SyntaxAnalyzer* analyzer)
{

    SYNTAX_ANANLYZER_ASSERT_OK_(analyzer);

    LOG_STACKTRACE;

    GET_CURRENT_TOKEN_(token);
    
    ast::ASTNode* left = get_identifier_(analyzer);

    token = CURRENT_TOKEN_;
    if(token->type == token::TYPE_OPERATOR 
       && token->val.op_type == token::OPERATOR_TYPE_ASSIGN) {

        if(!left) {
            LOG_SYNTAX_ERR_("expected l-value");
            return NULL;
        }

        INCREMENT_POS_;

        ast::ASTNode* right = get_expr_(analyzer);

        if(!right) {
            LOG_SYNTAX_ERR_("expected expression");
            return NULL;
        }

        int sym_id = ast::add_symbol(
            analyzer->astree, 
            &left->token.val.str, 
            SYMBOL_TYPE_VARIABLE);

        utils_assert(sym_id >= 0);

        left->token.id = sym_id;

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

    node = get_func_call_(analyzer);
    if(node) return node;

    node = get_numeric_literal_(analyzer);
    if(node) return node;

    node = get_identifier_(analyzer);

    if(node) {
        int sym_id = ast::find_symbol(analyzer->astree, &node->token.val.str, SYMBOL_TYPE_VARIABLE);

        if(sym_id < 0) {
            LOG_SYNTAX_ERR_("unknown symbol %s", token::value_str(&node->token));
            NFREE(node);
            return NULL;
        }
    }

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

    if(analyzer->pos >= (signed) analyzer->lex->tokens.size)
        return INVALID_BUFPOS;

    return ERR_NONE;
}

#endif // _DEBUG

} // syntax
} // compiler
