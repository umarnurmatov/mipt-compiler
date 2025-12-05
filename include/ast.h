#pragma once

#include <stdlib.h>
#include <stdio.h>

#include "vector.h"
#include "token.h"

#include "error.h"

#define AST_INIT_LIST                   \
    {                                   \
        .root        = NULL,            \
        .size        = 0,               \
        .identifiers = VECTOR_INITLIST, \
        .to_delete   = VECTOR_INITLIST  \
    };                      

namespace compiler {
namespace ast {

struct ASTNode
{
    ASTNode* left;
    ASTNode* right;
    ASTNode* parent;
    
    token::Type type;
    token::Token value;

};

struct AST
{
    ASTNode* root;
    size_t size;

    Vector identifiers;
    Vector to_delete;

};

Err ctor(AST* astree);

Err copy(AST* from, AST* to);

void dtor(AST* astree);

const char* strerr(Err err);

void node_print(FILE* stream, void* node);

Err fread_tree(AST* astree, const char* filename);

ASTNode* new_node(token::Type node_type, token::Token node_value, ASTNode *left, ASTNode *right, ASTNode *parent);

ASTNode* copy_subtree(AST* astree, ASTNode* node, ASTNode* parent);

void free_subtree(ASTNode* node);

token::Identifier* find_identifier(AST* astree, utils_hash_t hash);

void mark_to_delete(AST* astree, ASTNode* node);

#ifdef _DEBUG 

void dump(AST* ast, ASTNode* node, Err err, const char* msg, const char* file, int line, const char* funcname);


#define AST_DUMP_NODE(ast, node, err) \
    dump(ast, node, err, NULL, __FILE__, __LINE__, __func__); 

#define AST_DUMP(ast, err) \
    dump(ast, (ast)->root, err, NULL, __FILE__, __LINE__, __func__); 

#define AST_DUMP_MSG(ast, err, msg) \
    dump(ast, (ast)->root, err, msg, __FILE__, __LINE__, __func__); 

#endif // _DEBUG
    
} // ast 
} // compiler
