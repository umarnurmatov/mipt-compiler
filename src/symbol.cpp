#include "symbol.h"

#include <string.h>

#include "assertutils.h"
#include "memutils.h"

namespace compiler {

const char* symbol_type_str(SymbolType type)
{
    switch(type) {
        case SYMBOL_TYPE_VARIABLE:
            return "VAR";
        case SYMBOL_TYPE_PARAMETER:
            return "PAR";
        case SYMBOL_TYPE_FUNCTION:
            return "FUNC";
        default:
            return NULL;
    }
}

Env* create_env()
{
    Env* new_env = TYPED_CALLOC(1, Env);
    
    const size_t symbol_table_cap = 10;
    vector_ctor(&new_env->symbol_table, symbol_table_cap, sizeof(Symbol));
    
    return new_env;
}

Symbol* symbol_at(Env* env, int id) {
    utils_assert(env);

    return (Symbol*)vector_at(&env->symbol_table, id);
}

int find_symbol(Env* env, utils_str_t* str, SymbolType type)
{
    utils_assert(env);

    for(size_t ind = 0; ind < env->symbol_table.size; ++ind) {
        Symbol* sym = (Symbol*)vector_at(&env->symbol_table, ind);
        if(sym->type == type && str->len == sym->str.len && strncmp(sym->str.str, str->str, str->len) == 0) {
            return (signed) ind;
        }
    }

    return -1;
}

int add_symbol_to_env(Env* env, utils_str_t* str, SymbolType type)
{
    utils_assert(env);

    int id = find_symbol(env, str, type);
    if(id >= 0) return id;

    Symbol sym = {
        .str  = *str,
        .hash = 0,
        .type = type
    };

    vector_push(&env->symbol_table, &sym);

    return (signed)env->symbol_table.size - 1;
}

}
