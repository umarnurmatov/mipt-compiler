#pragma once

namespace compiler {

enum Err
{
    ERR_NONE,
    INVALID_BUFPOS,
    NULLPTR,
    ALLOC_FAIL,
    IO_ERR,
    SYNTAX_ERR

};

const char* strerr(Err err);

} // compiler
