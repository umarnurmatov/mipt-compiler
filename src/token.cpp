#include "token.h"

#include <string.h>

#include "assertutils.h"
#include "logutils.h"

namespace compiler {
namespace token {

const char* type_str(Type token_type)
{
    switch(token_type) {
        case TYPE_OPERATOR    : return "OPERATOR";
        case TYPE_KEYWORD     : return "KEYWORD";
        case TYPE_SEPARATOR   : return "SEPARATOR";
        case TYPE_IDENTIFIER  : return "IDENTIFIER";
        case TYPE_NUM_LITERAL : return "NUM_LITERAL";
        case TYPE_CALL        : return "CALL";
        case TYPE_TERMINATOR  : return "TERMINATOR";
        case TYPE_FAKE        : return "FAKE";
        default               : return "???";
    }
}

const char* value_str(token::Token* token)
{
    static const size_t buffer_len = 100;
    static char buffer[buffer_len] = "";

    switch(token->type) {
        case TYPE_OPERATOR:
        case TYPE_KEYWORD:
        case TYPE_SEPARATOR:
        {
            for(size_t ind = 0; ind < SIZEOF(TokenArr); ++ind) {
                if(token->val.enum_val == TokenArr[ind].val.enum_val)
                    return TokenArr[ind].str_internal;
            }
            break;
        }
        case TYPE_IDENTIFIER: 
        {
            // if(var) {
            //     snprintf(buffer, buffer_len, "%s", var->str);
            //     return buffer;
            // }
            break;
        }
        case TYPE_NUM_LITERAL:
            snprintf(buffer, buffer_len, "%d", token->val.num);
            return buffer;
        case TYPE_CALL:
            return "function call";
        case TYPE_TERMINATOR:
            return "terminator";
        case TYPE_FAKE:
            return "fakeval";
        default:
            return "???";
    }

    return NULL;
}

} // token
} // compiler
