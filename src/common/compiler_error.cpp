#include "compiler_error.h"

namespace compiler {

const char* strerr(Err err)
{
    switch(err) {
        case ERR_NONE:
            return "none";
        case NULLPTR:
            return "passed a nullptr";
        case INVALID_BUFPOS:
            return "buffer position invalid";
        case ALLOC_FAIL:
            return "memory allocation failed";
        case IO_ERR:
            return "io error";
        case SYNTAX_ERR:
            return "syntax error";
        default:
            return "unknown";
    }
}

} // compiler
