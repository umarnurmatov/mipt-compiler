#include "evaluate.h"

#include <math.h>
#include <new>

#include "token.h"
#include "logutils.h"
#include "assertutils.h"

namespace compiler {

static const char* LOG_EVAL = "EVAL";

static int evaluate_(ast::ASTNode* node);

static int evaluate_(ast::ASTNode* node) 
{
    utils_assert(node);

    switch(node->token.type) {

        case token::TYPE_OPERATOR:
            return evaluate_operator(node);
            break;

        case token::TYPE_NUM_LITERAL:
            return node->token.val.num;
            break;

        default:
            UTILS_LOGW(LOG_EVAL, 
                       "node of type %s occured", 
                       token::type_str(node->token.type));
            return 0;
            break;
    }
}

int evaluate_operator(ast::ASTNode* node)
{
    utils_assert(node);
    utils_assert(node->token.type == token::TYPE_OPERATOR);

    int res = 0;

    int left = evaluate_(node->left);
    int right = evaluate_(node->right);

    switch(node->token.val.op_type) {
        case token::OPERATOR_TYPE_ADD:
            res = left + right;
            break;

        case token::OPERATOR_TYPE_SUB:
            res = left - right;
            break;

        case token::OPERATOR_TYPE_MUL:
            res = left * right;
            break;

        case token::OPERATOR_TYPE_DIV:
            res = left / right;
            break;

        case token::OPERATOR_TYPE_POW:
            res = (int)pown(left, right);
            break;

        case token::OPERATOR_TYPE_OR:
            res = left | right;
            break;

        case token::OPERATOR_TYPE_AND:
            res = left & right;
            break;

        case token::OPERATOR_TYPE_EQ:
            res = left == right;
            break;

        case token::OPERATOR_TYPE_NEQ:
            res = left != right;
            break;

        case token::OPERATOR_TYPE_GT:
            res = left > right;
            break;

        case token::OPERATOR_TYPE_LT:
            res = left < right;
            break;

        case token::OPERATOR_TYPE_GEQ:
            res = left >= right;
            break;

        case token::OPERATOR_TYPE_LEQ:
            res = left <= right;
            break;

        case token::OPERATOR_TYPE_ASSIGN:
            break;

        case token::OPERATOR_TYPE_SQRT:
            res = (int)sqrt(left);
            break;
    }

    return res;
}

} // compiler 
