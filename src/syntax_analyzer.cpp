#include "syntax_analyzer.h"

namespace compiler {
namespace syntax {

static ast::ASTNode* parse_get_expr_(ast::AST* astree);
static ast::ASTNode* parse_get_mul_div_(ast::AST* astree);
static ast::ASTNode* parse_get_pow_(ast::AST* astree);
static ast::ASTNode* parse_get_primary_(ast::AST* astree);
static ast::ASTNode* parse_get_number_(ast::AST* astree);
static ast::ASTNode* parse_get_var_(ast::AST* astree);
static ast::ASTNode* parse_get_func_(ast::AST* astree);

#define LOG_SYNTAX_ERR_(msg, ...)           \
    UTILS_LOGE(                             \
        LOG_CTG_AST,                  \
        "%s:1:%ld: syntax error: " msg,     \
        astree->buf.filename,                \
        astree->buf.pos,                     \
        __VA_ARGS__ );

#define ADD_(left, right) \
    new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_ADD }, left, right, NULL)

#define SUB_(left, right) \
    new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_SUB }, left, right, NULL)

#define MUL_(left, right) \
    new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_MUL }, left, right, NULL)

#define DIV_(left, right) \
    new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_DIV }, left, right, NULL)

#define POW_(left, right) \
    new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_POW }, left, right, NULL)

#define CONST_(num_) \
    new_node(NODE_TYPE_NUM, NodeValue { .num = num_ }, NULL, NULL, NULL)

#define VAR_(var) \
    new_node(NODE_TYPE_VAR, NodeValue { .var_hash = var->hash }, NULL, NULL, NULL)


ast::ASTNode* parse_get_number_(ast::AST* astree)
{
    AST_ASSERT_OK_(astree);

    int val = 0;
    ssize_t pos_prev = astree->buf.pos;
    ast::ASTNode* node = NULL;

    while('0' <= BUF_AT_POS_ && BUF_AT_POS_ <= '9') {
        val = (BUF_AT_POS_ - '0') + val * 10;
        INCREMENT_POS_;
    }

    if(pos_prev != POS_) {
        node = CONST_(val);
        mark_to_delete(astree, node);
    }

    return node;
}

ast::ASTNode* parse_get_var_(ast::AST* astree)
{
    AST_ASSERT_OK_(astree);

    ssize_t pos_prev = astree->buf.pos;
    Variable var = { .c = ' ', .hash = 0, .val = 0 };
    ast::ASTNode* node = NULL;

    if(isalpha(BUF_AT_POS_)) {
        var.c = BUF_AT_POS_;
        var.hash = utils_djb2_hash(astree->buf.ptr + astree->buf.pos, sizeof(char));

        add_variable_(astree, var);

        INCREMENT_POS_;

        node = new_node(
                NODE_TYPE_VAR, 
                ast::ASTNodeValue { .var_hash = var.hash },
                NULL,
                NULL,
                NULL );
        mark_to_delete(astree, node);
    }

    return node;
}

ast::ASTNode* parse_get_expr_(ast::AST* astree)
{
    AST_ASSERT_OK_(astree);

    ast::ASTNode* node = parse_get_mul_div_(astree);

    while(BUF_AT_POS_ == '+' || BUF_AT_POS_ == '-') {
        ssize_t pos_prev = POS_;
        INCREMENT_POS_;

        ast::ASTNode* node_new = parse_get_mul_div_(astree);

        if(BUF_AT_PREV_POS_ == '+')
            node = ADD_(node, node_new);
        else
            node = SUB_(node, node_new);

        mark_to_delete(astree, node);
    }

    return node;
}

ast::ASTNode* parse_get_mul_div_(ast::AST* astree)
{
    AST_ASSERT_OK_(astree);

    ast::ASTNode* node = parse_get_pow_(astree);

    while(BUF_AT_POS_ == '*' || BUF_AT_POS_ == '/') {
        ssize_t pos_prev = POS_;
        INCREMENT_POS_;
        ast::ASTNode* node_right = parse_get_pow_(astree);

        if(BUF_AT_PREV_POS_ == '*')
            node = MUL_(node, node_right);
        else
            node = DIV_(node, node_right);

        mark_to_delete(astree, node);
    }
    
    return node;
}

ast::ASTNode* parse_get_pow_(ast::AST* astree)
{
    AST_ASSERT_OK_(astree);

    ast::ASTNode* node = parse_get_primary_(astree);
    
    while(BUF_AT_POS_ == '^') {

        INCREMENT_POS_;
        ast::ASTNode* node_new = parse_get_primary_(astree);

        node = POW_(node, node_new);
        mark_to_delete(astree, node);
    }

    return node;
}

ast::ASTNode* parse_get_func_(ast::AST* astree)
{
    AST_ASSERT_OK_(astree);

    ssize_t bufpos = 0;
    char buf[MAX_OP_NAME_LEN] = "";

    ssize_t pos_prev = POS_;
    ast::ASTNode* node = NULL;

    while(POS_ < LEN_ && isalpha(BUF_AT_POS_)) {
        buf[bufpos++] = BUF_AT_POS_;
        INCREMENT_POS_;
    }
    
    const Operator* op = NULL; // FIXME
    if(!op) {
        POS_ = pos_prev;
        return NULL;
    }

    if(BUF_AT_POS_ == '(') {
        INCREMENT_POS_;

        node = parse_get_expr_(astree);

        INCREMENT_POS_;

        node = new_node(NODE_TYPE_OP, ast::ASTNodeValue { .op_type = op->type }, node, NULL, NULL);
        mark_to_delete(astree, node);
    }
    
    return node;
}

ast::ASTNode* parse_get_primary_(ast::AST* astree)
{
    AST_ASSERT_OK_(astree);

    ast::ASTNode* node = NULL;

    if(BUF_AT_POS_ == '(') {
        INCREMENT_POS_;

        node = parse_get_expr_(astree);

        INCREMENT_POS_;

        return node;
    }

    node = parse_get_number_(astree);
    if(node) return node;

    node = parse_get_func_(astree);
    if(node) return node;

    node = parse_get_var_(astree);

    return node;
}

ast::ASTNode* parse_get_general_(ast::AST* astree)
{
    AST_ASSERT_OK_(astree);

    ast::ASTNode* node = parse_get_expr_(astree);

    if(astree->buf.ptr[astree->buf.pos] != '\n') {
        LOG_SYNTAX_ERR_("expected: \\n, got: (ASCII) %d", (int)BUF_AT_POS_);
        return NULL;
    }
    INCREMENT_POS_;

    return node;
}

#undef LOG_SYNTAX_ERR_
#undef BUF_AT_POS_
#undef BUF_AT_PREV_POS_
#undef POS_
#undef LEN_
#undef INCREMENT_POS_
#undef ADD_
#undef SUB_
#undef MUL_
#undef DIV_
#undef POW_
#undef CONST_
#undef VAR_


} // syntax
} // compiler
