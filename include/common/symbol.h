#pragma once

#include "hashutils.h"
#include "stringutils.h"
#include "vector.h"

namespace compiler {

enum SymbolType 
{
    SYMBOL_TYPE_VARIABLE,
    SYMBOL_TYPE_PARAMETER,
    SYMBOL_TYPE_FUNCTION,
    SYMBOL_TYPE_NONE
};

struct Symbol
{
    utils_str_t    str;
    utils_hash_t   hash;
    SymbolType     type;
};

struct Env
{
    Vector symbol_table;
};

Env* create_env();

const char* symbol_type_str(SymbolType type);

Symbol* symbol_at(Env* env, int id);

int find_symbol(Env* env, utils_str_t* str, SymbolType type);

int add_symbol_to_env(Env* env, utils_str_t* str, SymbolType type);

}
