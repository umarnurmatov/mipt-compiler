#include "translator.h"

#include <stdio.h>

#include "ast.h"
#include "logutils.h"
#include "symbol.h"
#include "token.h"

namespace compiler {

static const char* LOG_TRANSLATOR = "TRANSLATOR";

static void emit_node_(Translator* tr, ast::ASTNode* node);
static void emit_operator_(Translator* tr, ast::ASTNode* node);
static void emit_keyword_(Translator* tr, ast::ASTNode* node);
static void emit_while_(Translator* tr, ast::ASTNode* node);
static void emit_if_(Translator* tr, ast::ASTNode* node);
static void emit_while_(Translator* tr, ast::ASTNode* node);
static void emit_num_literal_(Translator* tr, ast::ASTNode* node);
static void emit_identifier_(Translator* tr, ast::ASTNode* node);
static void emit_function_(Translator* tr, ast::ASTNode* node);
static void emit_variable_(Translator* tr, ast::ASTNode* node);
static void emit_assignment_(Translator* tr, ast::ASTNode* node);

void emit_program(Translator* tr)
{
    emit_node_(tr, tr->astree->root);

    fprintf(tr->file, "OUT\nHLT\n");
}

#define LOG_TRACE                                 \
    UTILS_LOGD(LOG_TRANSLATOR, "token %s %s",     \
               token::type_str(node->token.type), \
               token::value_str(&node->token))

void emit_node_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    switch(node->token.type) {
        case token::TYPE_OPERATOR:
            emit_operator_(tr, node);
            break;

        case token::TYPE_KEYWORD:
            emit_keyword_(tr, node);
            break;

        case token::TYPE_SEPARATOR:
            if(node->left) emit_node_(tr, node->left);
            if(node->right) emit_node_(tr, node->right);
            break;

        case token::TYPE_IDENTIFIER:
            emit_identifier_(tr, node);
            break;

        case token::TYPE_NUM_LITERAL:
            emit_num_literal_(tr, node);
            break;

        case token::TYPE_CALL:
            break;

        case token::TYPE_TERMINATOR:
            break;

        case token::TYPE_FAKE:
            emit_node_(tr, node->left);
            break;

        case token::TYPE_NONE:
        default:
            UTILS_LOGE(LOG_TRANSLATOR, "unknown token");
            // ERROR
    }

}

void emit_operator_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    using namespace token;

    LOG_TRACE;

    switch(node->token.val.op_type) {
        case OPERATOR_TYPE_ADD:
            emit_node_(tr, node->left);
            emit_node_(tr, node->right);
            fprintf(tr->file, "ADD\n");
            break;

        case OPERATOR_TYPE_SUB:
            emit_node_(tr, node->left);
            emit_node_(tr, node->right);
            fprintf(tr->file, "SUB\n");
            break;

        case OPERATOR_TYPE_MUL:
            emit_node_(tr, node->left);
            emit_node_(tr, node->right);
            fprintf(tr->file, "MUL\n");
            break;

        case OPERATOR_TYPE_DIV:
            emit_node_(tr, node->left);
            emit_node_(tr, node->right);
            fprintf(tr->file, "DIV\n");
            break;

        case OPERATOR_TYPE_POW:
            emit_node_(tr, node->left);
            emit_node_(tr, node->right);
            fprintf(tr->file, "POW\n");
            break;

        case OPERATOR_TYPE_OR:
            emit_node_(tr, node->left);
            emit_node_(tr, node->right);
            fprintf(tr->file, "MUL\n");
            break;

        case OPERATOR_TYPE_AND:
            fprintf(tr->file, "MUL\n");
            break;

        case OPERATOR_TYPE_EQ:
            fprintf(tr->file, "SUB\n");
            break;

        case OPERATOR_TYPE_NEQ:
            fprintf(tr->file, "SUB\n");
            break;

        case OPERATOR_TYPE_GT:
            fprintf(tr->file, "SUB\n");
            break;

        case OPERATOR_TYPE_LT:
            fprintf(tr->file, "SUB\n");
            break;

        case OPERATOR_TYPE_GEQ:
            fprintf(tr->file, "SUB\n");
            break;

        case OPERATOR_TYPE_LEQ:
            fprintf(tr->file, "SUB\n");
            break;

        case OPERATOR_TYPE_ASSIGN:
            emit_assignment_(tr, node);
            break;

        default:
            break;
    }
}

void emit_keyword_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    using namespace token;

    switch(node->token.val.kw_type) {
        case KEYWORD_TYPE_WHILE:
            emit_while_(tr, node);
            break;

        case KEYWORD_TYPE_IF:
            emit_if_(tr, node);
            break;

        case KEYWORD_TYPE_ELSE:
            break;
        case KEYWORD_TYPE_DEFUN:
            break;
        case KEYWORD_TYPE_RETURN:
            break;

        default:
            break;
    }
}

void emit_while_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    fprintf(tr->file, ":beginwhile_%d\n", tr->label_id);

    emit_node_(tr, node->left);

    fprintf(tr->file, "PUSH 0\n");
    fprintf(tr->file, "JE :endwhile_%d\n", tr->label_id);
    
    emit_node_(tr, node->right);

    fprintf(tr->file, "JMP :beginwhile_%d\n", tr->label_id);
    fprintf(tr->file, ":endwhile_%d\n", tr->label_id);

    tr->label_id++;
}

void emit_if_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    emit_node_(tr, node->left);
    
    fprintf(tr->file, "PUSH 0\n");

    if(node->right->token.val.kw_type == token::KEYWORD_TYPE_ELSE) {
        fprintf(tr->file, "JE :else_%d\n", tr->label_id);

        emit_node_(tr, node->right->left);

        fprintf(tr->file, "JMP :endif_%d\n", tr->label_id);
        fprintf(tr->file, ":else_%d\n", tr->label_id);

        emit_node_(tr, node->right->right);

    }
    else {
        fprintf(tr->file, "JE :endif_%d\n", tr->label_id);

        emit_node_(tr, node->right);
    }

    fprintf(tr->file, ":endif_%d\n", tr->label_id);

    tr->label_id++;
}

static void emit_num_literal_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    fprintf(tr->file, "PUSH %d\n", node->token.val.num);
}

static void emit_identifier_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    Env* identifier_env = get_enviroment(tr->astree, node->token.scope_id);
    Symbol* sym = symbol_at(identifier_env, node->token.inner_scope_id);

    switch(sym->type) {
        case SYMBOL_TYPE_VARIABLE:
            emit_variable_(tr, node); 
            break;

        case SYMBOL_TYPE_FUNCTION:
            emit_function_(tr, node);
            break;

        case SYMBOL_TYPE_PARAMETER:
            emit_variable_(tr, node); 
            break;

        case SYMBOL_TYPE_NONE:
            UTILS_LOGE(LOG_TRANSLATOR, "got symbol of type none");
            break;

        default:
            break;
    }
}

static void emit_function_(Translator* tr, ast::ASTNode* node)
{
    LOG_TRACE;

    fprintf(tr->file, ":func_%.*s\n", node->token.val.str.len, node->token.val.str.str);

    emit_node_(tr, node->right);
}

static void emit_variable_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    utils_assert(node->token.inner_scope_id >= 0);

    // value
    fprintf(tr->file, "PUSHM [SP+%d]\n", node->token.inner_scope_id);
}

static void emit_assignment_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;
    // if(node->left->token.type != token::TYPE_IDENTIFIER) {
    //     UTILS_LOGE(LOG_TRANSLATOR, "expected identifier"); 
    // }

    emit_node_(tr, node->right);

    utils_assert(node->left->token.inner_scope_id >= 0);
    fprintf(tr->file, "POPM [SP+%d]\n", node->left->token.inner_scope_id);
}

#undef LOG_TRACE

} // compiler
