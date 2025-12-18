#include "translator.h"

#include <stdio.h>

#include "ast.h"
#include "logutils.h"
#include "symbol.h"
#include "token.h"

namespace compiler {

static const char* LOG_TRANSLATOR = "TRANSLATOR";

static void emit_node_        (Translator* tr, ast::ASTNode* node);
static void emit_operator_    (Translator* tr, ast::ASTNode* node);
static void emit_keyword_     (Translator* tr, ast::ASTNode* node);
static void emit_while_       (Translator* tr, ast::ASTNode* node);
static void emit_if_          (Translator* tr, ast::ASTNode* node);
static void emit_while_       (Translator* tr, ast::ASTNode* node);
static void emit_return_      (Translator* tr, ast::ASTNode* node);
static void emit_num_literal_ (Translator* tr, ast::ASTNode* node);
static void emit_identifier_  (Translator* tr, ast::ASTNode* node);
static void emit_function_    (Translator* tr, ast::ASTNode* node);
static void emit_variable_    (Translator* tr, ast::ASTNode* node);
static void emit_assignment_  (Translator* tr, ast::ASTNode* node);
static void emit_in_          (Translator* tr, ast::ASTNode* node);
static void emit_out_         (Translator* tr, ast::ASTNode* node);
static void emit_call_        (Translator* tr, ast::ASTNode* node);

static const char* get_func_name_            (ast::ASTNode* node);
static void        emit_comparasion_operator_(Translator* tr, ast::ASTNode* node, const char* cmd);

void emit_program(Translator* tr)
{
    fprintf(tr->file, "CALL :func_main\n");
    fprintf(tr->file, "PUSHR A0\n");
    fprintf(tr->file, "OUT\n");
    fprintf(tr->file, "HLT\n");

    emit_node_(tr, tr->astree->root);
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
            emit_call_(tr, node);
            break;

        case token::TYPE_TERMINATOR:
        case token::TYPE_FAKE:
        case token::TYPE_NONE:
        default:
            UTILS_LOGE(LOG_TRANSLATOR, "unsupported token");
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
            // emit_node_(tr, node->left);
            // emit_node_(tr, node->right);
            // fprintf(tr->file, "MUL\n");
            break;

        case OPERATOR_TYPE_AND:
            // emit_node_(tr, node->left);
            // emit_node_(tr, node->right);
            // fprintf(tr->file, "MUL\n");
            break;

        case OPERATOR_TYPE_EQ:
            emit_comparasion_operator_(tr, node, "JE");
            break;

        case OPERATOR_TYPE_NEQ:
            emit_comparasion_operator_(tr, node, "JNE");
            break;

        case OPERATOR_TYPE_GT:
            emit_comparasion_operator_(tr, node, "JA");
            break;

        case OPERATOR_TYPE_LT:
            emit_comparasion_operator_(tr, node, "JB");
            break;

        case OPERATOR_TYPE_GEQ:
            emit_comparasion_operator_(tr, node, "JAE");
            break;

        case OPERATOR_TYPE_LEQ:
            emit_comparasion_operator_(tr, node, "JBE");
            break;

        case OPERATOR_TYPE_ASSIGN:
            emit_assignment_(tr, node);
            break;

        case OPERATOR_TYPE_SQRT:
            emit_node_(tr, node->left);
            fprintf(tr->file, "SQR\n");
            break;

        default:
            break;
    }
}

static void emit_comparasion_operator_(Translator* tr, ast::ASTNode* node, const char* cmd)
{
    emit_node_(tr, node->left);
    emit_node_(tr, node->right);
    fprintf(tr->file, "SUB\n");
    fprintf(tr->file, "PUSH 0\n");
    fprintf(tr->file, "%s :%s_true_%d\n", cmd, cmd, tr->label_id);
    fprintf(tr->file, "PUSH 0\n");
    fprintf(tr->file, "JMP :%s_false_%d\n", cmd, tr->label_id);
    fprintf(tr->file, ":%s_true_%d\n", cmd, tr->label_id);
    fprintf(tr->file, "PUSH 1\n");
    fprintf(tr->file, ":%s_false_%d\n", cmd, tr->label_id);
    tr->label_id++;
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
            emit_return_(tr, node);
            break;

        case KEYWORD_TYPE_IN:
            emit_in_(tr, node);
            break;

        case KEYWORD_TYPE_OUT:
            emit_out_(tr, node);
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

static void emit_return_(Translator* tr, ast::ASTNode* node)
{
    emit_node_(tr, node->left);

    tr->current_env = get_enviroment(tr->astree, node->token.scope_id);
    size_t stackframe_size = tr->current_env->symbol_table.size - 1;

    fprintf(tr->file, "POPR A0\n");

    fprintf(tr->file, "PUSHR SP\n");
    fprintf(tr->file, "PUSH %lu\n", stackframe_size);
    fprintf(tr->file, "SUB\n");
    fprintf(tr->file, "POPR SP\n");

    fprintf(tr->file, "RET\n");
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

    tr->current_env = get_enviroment(tr->astree, node->token.scope_id);
    size_t stackframe_size = tr->current_env->symbol_table.size - 1;

    fprintf(tr->file, "%s\n", get_func_name_(node));

    fprintf(tr->file, "PUSH %lu\n", stackframe_size);
    fprintf(tr->file, "PUSHR SP\n");
    fprintf(tr->file, "ADD\n");
    fprintf(tr->file, "POPR SP\n");

    emit_node_(tr, node->right);
}

static void emit_variable_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    utils_assert(node->token.inner_scope_id >= 0);

    // value
    fprintf(tr->file, "PUSHM [SP-%d]\n", node->token.inner_scope_id - 1);
}

static void emit_assignment_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    emit_node_(tr, node->right);

    utils_assert(node->left->token.inner_scope_id >= 0);
    fprintf(tr->file, "POPM [SP-%d]\n", node->left->token.inner_scope_id - 1);
}

static void emit_in_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;

    fprintf(tr->file, "IN\n");

    utils_assert(node->left->token.inner_scope_id >= 0);
    fprintf(tr->file, "POPM [SP-%d]\n", node->left->token.inner_scope_id - 1);
}

static void emit_out_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;
    
    emit_node_(tr, node->left);

    fprintf(tr->file, "OUT\n");
}

static void emit_call_(Translator* tr, ast::ASTNode* node)
{
    utils_assert(tr);
    utils_assert(node);

    LOG_TRACE;
    
    Env* func_env = get_enviroment(tr->astree, node->token.scope_id);
    size_t stackframe_size = func_env->symbol_table.size - 1;

    ast::ASTNode* arg = node->right;
    int argcnt = 0;
    while(arg && arg->token.type == token::TYPE_SEPARATOR) {
        emit_node_(tr, arg->left);
        fprintf(tr->file, "POPM [SP+%d]\n", stackframe_size - argcnt);
        arg = arg->right;
        argcnt++;
    }
    if(arg) {
        emit_node_(tr, arg);
        fprintf(tr->file, "POPM [SP+%d]\n", stackframe_size - argcnt);
    }

    fprintf(tr->file, "CALL %s\n", get_func_name_(node->left));
    fprintf(tr->file, "PUSHR A0\n");
}

static const char* get_func_name_(ast::ASTNode* node)
{
    utils_assert(node);

    const size_t buf_size = 100;
    static char buffer[buf_size] = "";
    
    snprintf(buffer, buf_size, ":func_%.*s", 
             node->token.val.str.len, node->token.val.str.str);

    return buffer;
}

#undef LOG_TRACE

} // compiler
