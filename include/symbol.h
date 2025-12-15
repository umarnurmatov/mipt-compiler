#pragma once

#include "hashutils.h"
#include "stringutils.h"

namespace compiler {

enum SymbolType 
{
    SYMBOL_TYPE_VARIABLE,
    SYMBOL_TYPE_PARAMETER,
    SYMBOL_TYPE_FUNCTION
};

struct Symbol
{
    utils_str_t    str;
    utils_hash_t   hash;
    SymbolType     type;

    size_t         next_param; // for function declarations
};

}
