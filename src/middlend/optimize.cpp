#include "optimize.h"

#include "assertutils.h"
#include "token.h"
#include "utils.h"

namespace compiler {
namespace optimizer {

ATTR_UNUSED static const char* LOG_CTG_DIFF_OPT = "DIFFTREE OPTIMIZE";

static bool treeChanged = false;

static bool ast_subtree_holds_identifier_(ast::ASTNode* node);

static ast::ASTNode* const_(int num);

static ast::ASTNode* const_fold_(ast::AST* astree, ast::ASTNode* node);

static ast::ASTNode* eliminate_neutral_(ast::AST* astree, ast::ASTNode* node);

static ast::ASTNode* eliminate_neutral_mul_(ast::AST* astree, ast::ASTNode* node, ast::ASTNode* left, ast::ASTNode* right);

static ast::ASTNode* eliminate_neutral_add_(ast::AST* astree, ast::ASTNode* node, ast::ASTNode* left, ast::ASTNode* right);

static ast::ASTNode* eliminate_neutral_pow_(ast::AST* astree, ast::ASTNode* node, ast::ASTNode* left, ast::ASTNode* right);

void optimize(ast::AST *astree)
{
    do {
        treeChanged = false;
        const_fold_(astree, astree->root->left);
        eliminate_neutral_(astree, astree->root->left);

    } while(treeChanged);
}

static bool ast_subtree_holds_identifier_(ast::ASTNode* node)
{
    if(node->token.type == token::TYPE_IDENTIFIER)
        return true;

    if(node->left)
        return ast_subtree_holds_identifier_(node->left);

    else if (node->right)
        return ast_subtree_holds_identifier_(node->right);

    return false;
}

static ast::ASTNode* const_(int num)
{
    token::Token tok_num_literal = TOKEN_INITLIST;
    tok_num_literal.type = token::TYPE_NUM_LITERAL;
    tok_num_literal.val.num = num;
    return ast::new_node(&tok_num_literal, NULL, NULL, NULL);
}

static ast::ASTNode* const_fold_(ast::AST* astree, ast::ASTNode* node)
{
    ast::ASTNode *left = NULL, *right = NULL;

    if(node->left)
        left = const_fold_(astree, node->left);

    if(node->right)
        right = const_fold_(astree, node->right);

    bool left_holds_id = true, right_holds_id = true; 
    if(left)  left_holds_id  = ast_subtree_holds_identifier_(left);
    if(right) right_holds_id = ast_subtree_holds_identifier_(right);

    if(!left_holds_id && !right_holds_id) {

        ast::ASTNode* new_node = const_(0); // FIXME
        
        if(node->parent->left == node)
            node->parent->left = new_node;

        if(node->parent->right == node)
            node->parent->right = new_node;
        
        ast::mark_to_delete(astree, node);

        treeChanged = true;

        return new_node;
    }

    return node;
}

#define IS_VALUE_(node, value) \
    ((node->token.type == token::TYPE_NUM_LITERAL) && (node->token.val.num == value))

#define cL ast::copy_subtree(astree, node->left, node)
#define cR ast::copy_subtree(astree, node->right, node)

static ast::ASTNode* eliminate_neutral_(ast::AST* astree, ast::ASTNode* node)
{
    utils_assert(astree);
    utils_assert(node);

    ast::ASTNode *left = NULL, *right = NULL, *new_node = node;
    
    if(node->token.type != token::TYPE_OPERATOR)
        return node;

    if(node->left)
        left = eliminate_neutral_(astree, node->left);

    if(node->right)
        right = eliminate_neutral_(astree, node->right);

    if(node->token.val.op_type == token::OPERATOR_TYPE_MUL)
        new_node = eliminate_neutral_mul_(astree, node, left, right);

    else if(node->token.val.op_type == token::OPERATOR_TYPE_ADD)
        new_node = eliminate_neutral_add_(astree, node, left, right);

    else if(node->token.val.op_type == token::OPERATOR_TYPE_POW)
        new_node = eliminate_neutral_pow_(astree, node, left, right);


    if(new_node != node) {
        if(node->parent->left == node)
            node->parent->left = new_node;

        else if(node->parent->right == node)
            node->parent->right = new_node;

        treeChanged = true;

        ast::mark_to_delete(astree, node);
    }

    return new_node;
}

static ast::ASTNode* eliminate_neutral_mul_(ast::AST* astree, ast::ASTNode* node, ast::ASTNode* left, ast::ASTNode* right)
{
    utils_assert(astree);
    utils_assert(node);
    utils_assert(left);
    utils_assert(right);

    utils_assert(node->token.type == token::TYPE_OPERATOR);
    utils_assert(node->token.val.op_type == token::OPERATOR_TYPE_MUL);

    ast::ASTNode* new_node = node;

    if     (IS_VALUE_(left,  0)) new_node = const_(0.f);
    else if(IS_VALUE_(left,  1)) new_node = cR;
    else if(IS_VALUE_(right, 0)) new_node = const_(0.f);
    else if(IS_VALUE_(right, 1)) new_node = cL;

    return new_node;
}


static ast::ASTNode* eliminate_neutral_add_(ast::AST* astree, ast::ASTNode* node, ast::ASTNode* left, ast::ASTNode* right)
{
    utils_assert(astree);
    utils_assert(node);
    utils_assert(left);
    utils_assert(right);

    utils_assert(node->token.type == token::TYPE_OPERATOR);
    utils_assert(node->token.val.op_type == token::OPERATOR_TYPE_MUL);

    ast::ASTNode* new_node = node;

    if     (IS_VALUE_(left, 0))  new_node = cR;
    else if(IS_VALUE_(right, 0)) new_node = cL;
    
    return new_node;
}

static ast::ASTNode* eliminate_neutral_pow_(ast::AST* astree, ast::ASTNode* node, ast::ASTNode* left, ast::ASTNode* right)
{
    utils_assert(astree);
    utils_assert(node);
    utils_assert(left);
    utils_assert(right);

    utils_assert(node->token.type == token::TYPE_OPERATOR);
    utils_assert(node->token.val.op_type == token::OPERATOR_TYPE_MUL);

    ast::ASTNode* new_node = node;

    if      (IS_VALUE_(left,  0)) new_node = const_(0.f); // 0 ^ x = 0
    else if (IS_VALUE_(left,  1)) new_node = const_(1.f); // 1 ^ x = 1
    else if (IS_VALUE_(right, 0)) new_node = const_(1.f); // x ^ 0 = 1
    else if (IS_VALUE_(right, 1)) new_node = cL;          // x ^ 1 = x

    return new_node;
}

#undef IS_VALUE_
#undef cL
#undef cR

} // optimizer
} // compiler
