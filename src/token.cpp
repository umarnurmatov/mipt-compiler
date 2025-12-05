#include "token.h"

#include <string.h>

#include "assertutils.h"
#include "logutils.h"

namespace compiler {
namespace token {

const char* type_str(Type node_type)
{
    switch(node_type) {
        case TYPE_OPERATOR: return "OP";
        case TYPE_KEYWORD: return "OP";
        case TYPE_SEPARATOR: return "OP";
        case TYPE_IDENTIFIER: return "IDENT";
        case TYPE_LITERAL:  return "LITERAL";
        case TYPE_FAKE: return "FAKE";
        default:        return "???";
    }
}

const char* node_op_type_str(OperatorType op_type)
{
    switch(op_type) {
        case OPERATOR_TYPE_ADD:   return "ADD";
        case OPERATOR_TYPE_SUB:   return "SUB";
        case OPERATOR_TYPE_MUL:   return "MUL";
        case OPERATOR_TYPE_DIV:   return "DIV";
        case OPERATOR_TYPE_POW:   return "POW";
        default: return "???";
    }
}


} // token
} // compiler
