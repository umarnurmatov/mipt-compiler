#include "ast.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>

#include "compiler_error.h"
#include "ioutils.h"
#include "logutils.h"
#include "memutils.h"
#include "assertutils.h"
#include "token.h"
#include "symbol.h"
#include "utils.h"
#include "vector.h"

namespace compiler {
namespace ast {

ATTR_UNUSED static const char* LOG_AST = "AST";

#ifdef _DEBUG

#define AST_ASSERT_OK_(ast)                \
    {                                      \
        Err err = verify_(ast);            \
        if(err != ERR_NONE) {              \
            AST_DUMP(ast, err);            \
            utils_assert(err == ERR_NONE); \
        }                                  \
    }

#else // _DEBUG

#define AST_ASSERT_OK_(ast) 

#endif // _DEBUG

static Err fwrite_node_(AST* astree, ASTNode* node, FILE* file);

ATTR_UNUSED static void print_node_ptr_(FILE* file, void* ptr);

static int advance_buf_pos_(AST* astree);

static void skip_spaces_(AST* astree);

Err scan_token_(AST* astree, token::Token* tok);

Err fread_node_infix_(AST* astree, ASTNode** node, const char* filename);

#ifdef _DEBUG

char* dump_graphviz_(AST* ast, ASTNode* node);

void dump_node_graphviz_(AST* astree, FILE* file, ASTNode* node, int rank);

Err verify_(AST* ast);

#endif // _DEBUG

Err ctor(AST* astree)
{
    utils_assert(astree);

    astree->size = 0;

    const size_t to_delete_cap = 10;
    
    vector_ctor(&astree->to_delete, to_delete_cap, sizeof(ASTNode*));

    const size_t env_cap = 10;
    
    vector_ctor(&astree->envs, env_cap, sizeof(Env*));

    return ERR_NONE;
}

Err copy(AST* from, AST* to)
{
    AST_ASSERT_OK_(from);
    utils_assert(to);

    to->size = from->size;
    to->root = copy_subtree(from, from->root, NULL);

    const size_t to_delete_cap = 10;

    vector_ctor(&to->to_delete, to_delete_cap, sizeof(ASTNode*));

    return ERR_NONE;
}

void dtor(AST* astree)
{
    utils_assert(astree);

    free_subtree(astree->root); 

    astree->size = 0;
    astree->root = NULL;

    for(size_t i = 0; i < astree->to_delete.size; ++i)
        free_subtree(*(ASTNode**)vector_at(&astree->to_delete, i));

    NFREE(astree->buf.ptr);

    vector_dtor(&astree->to_delete);

    for(size_t i = 0; i < astree->envs.size; ++i) {
        Env* env = *(Env**)vector_at(&astree->envs, i);
        vector_dtor(&env->symbol_table);
        free(env);
    }

    vector_dtor(&astree->envs);
}

void free_subtree(ASTNode* node)
{
    if(!node) return;

    if(node->left)
        free_subtree(node->left);
    if(node->right)
        free_subtree(node->right);

    NFREE(node);
}

void mark_to_delete(AST* astree, ASTNode* node)
{
    AST_ASSERT_OK_(astree);
    utils_assert(node);

    vector_push(&astree->to_delete, &node);
}

Err fwrite_infix(AST* astree, FILE* stream)
{
    AST_ASSERT_OK_(astree);
    utils_assert(stream);
    utils_assert(astree->root);

    Err err = fwrite_node_(astree, astree->root->left, stream);
    return err;
}

Err fread_infix(AST* astree, FILE* stream, const char* filename)
{    
    utils_assert(astree);
    utils_assert(filename);

    size_t fsize = get_file_size(stream);

    astree->buf.ptr = TYPED_CALLOC(fsize, char);
    astree->buf.ptr verified(return ALLOC_FAIL);

    size_t bytes_transferred = fread(astree->buf.ptr, sizeof(astree->buf.ptr[0]), fsize, stream);

    // TODO check for errors
    astree->buf.len = (unsigned) bytes_transferred;
    
    Err err = fread_node_infix_(astree, &astree->root, filename);

    if(err != ERR_NONE) {
        AST_DUMP(astree, err);

        for(size_t i = 0; i < astree->to_delete.size; ++i)
            free(*(ASTNode**)vector_at(&astree->to_delete, i));

        vector_free(&astree->to_delete);

        astree->root = NULL;

        return err;
    }

    vector_free(&astree->to_delete);

    AST_DUMP(astree, err);

    return ERR_NONE;
}

#define FREAD_LOG_SYNTAX_ERR(expc)                                              \
    UTILS_LOGE(                                                                 \
        LOG_AST,                                                                \
        "%s:1:%ld: syntax error: unexpected symbol <%c>, expected <" expc ">",  \
        filename,                                                               \
        astree->buf.pos,                                                        \
        astree->buf.ptr[astree->buf.pos]                                        \
    );                                                                          

Err fread_node_infix_(AST* astree, ASTNode** node, const char* filename)
{
    AST_ASSERT_OK_(astree);

    Err err = ERR_NONE;

    if(astree->buf.ptr[astree->buf.pos] == '(') {

        advance_buf_pos_(astree);
        skip_spaces_(astree);

        (*node) = new_node(NULL, NULL, NULL, NULL);
        mark_to_delete(astree, *node);

        scan_token_(astree, &(*node)->token);

        skip_spaces_(astree);

        err = fread_node_infix_(astree, &(*node)->left, filename);
        err == ERR_NONE verified(return err);

        if((*node)->left) {
            (*node)->left->parent = (*node);
        }

        skip_spaces_(astree);

        err = fread_node_infix_(astree, &(*node)->right, filename);
        err == ERR_NONE verified(return err);

        if((*node)->right) {
            (*node)->right->parent = (*node);
        }

        astree->size++;

        if(astree->buf.ptr[astree->buf.pos] != ')') {
            FREAD_LOG_SYNTAX_ERR(")");
            return SYNTAX_ERR;
        }

        advance_buf_pos_(astree);
        skip_spaces_(astree);
    }
    else if(strncmp(astree->buf.ptr + astree->buf.pos, 
                    TOKEN_NIL_STR, SIZEOF(TOKEN_NIL_STR) - 1) == 0) {

        if(astree->buf.ptr[astree->buf.pos] != 'n') {
            FREAD_LOG_SYNTAX_ERR("n");
            return SYNTAX_ERR;
        }

        astree->buf.pos += SIZEOF(TOKEN_NIL_STR) - 1;
        skip_spaces_(astree);
        *node = NULL;
    }
    else {
        FREAD_LOG_SYNTAX_ERR("(");
        return SYNTAX_ERR;
    }

    return ERR_NONE;
}

#undef FREAD_LOG_SYNTAX_ERR

static int advance_buf_pos_(AST* astree)
{
    AST_ASSERT_OK_(astree);

    if(astree->buf.pos < astree->buf.len - 1) {
        astree->buf.pos++;
        return 0;
    }
    else
        return 1;
}

static void skip_spaces_(AST* astree)
{
    AST_ASSERT_OK_(astree);

    while(isspace(astree->buf.ptr[astree->buf.pos])) {
        if(advance_buf_pos_(astree))
            break;
    }
}

Err scan_token_(AST* astree, token::Token* token)
{
    AST_ASSERT_OK_(astree);

    *token = TOKEN_INITLIST;

    char* end = strchr(astree->buf.ptr + astree->buf.pos, ' ');
    ssize_t tok_str_len = end - astree->buf.ptr - astree->buf.pos;

    UTILS_LOGD(LOG_AST, "node %.*s", tok_str_len,astree->buf.ptr + astree->buf.pos);

    BEGIN {
        // Keyword, separator or operator
        bool tok_found = false;
        for(size_t i = 0; i < SIZEOF(token::TokenArr); ++i) {
            if(tok_str_len == token::TokenArr[i].str_internal_len
               && strncmp(token::TokenArr[i].str_internal, astree->buf.ptr + astree->buf.pos, (unsigned) tok_str_len) == 0) {

                token->type = token::TokenArr[i].type;
                token->val  = token::TokenArr[i].val;
                tok_found = true;
                break;
            }
        }
        if(tok_found) GOTO_END;

        if(strncmp(astree->buf.ptr + astree->buf.pos, "CALL", (unsigned) tok_str_len) == 0) {
            token->type = token::TYPE_CALL;
            GOTO_END;
        }

        // Numeric
        int val = 0;
        int sign = 1;
        ssize_t pos_cur = astree->buf.pos;
        ssize_t pos_prev = pos_cur;
        if(astree->buf.ptr[astree->buf.pos] == '-') {
            sign = -1;
            pos_cur++;
        }
        while('0' <= astree->buf.ptr[pos_cur] && astree->buf.ptr[pos_cur] <= '9') {
            int digit = astree->buf.ptr[pos_cur] - '0';
            val = val * 10 + digit;
            pos_cur++;
        }

        if(pos_prev != pos_cur) {
            token->type = token::TYPE_NUM_LITERAL;
            token->val  = { .num = sign * val };
            UTILS_LOGD(LOG_AST, "got num %d", val);
            GOTO_END;
        }

        // Identifier
        char* id_end = strchr(astree->buf.ptr + astree->buf.pos, ':');
        char* id_str_ptr = astree->buf.ptr + astree->buf.pos;
        ssize_t id_str_len = id_end - astree->buf.ptr - astree->buf.pos;

        token->type = token::TYPE_IDENTIFIER;
        token->val  = {
            .str = {
                .str = id_str_ptr,
                .len = (unsigned) id_str_len
            }
        };

        size_t symbol_str_len = (size_t)(end - id_end - 1);
        char* symbol_ptr = id_end + 1;

        if(strncmp("FUNC", symbol_ptr, symbol_str_len) == 0) {
            Env* new_env = create_env();

            int func_sym_id = add_symbol_to_env(
                new_env, 
                &token->val.str, 
                SYMBOL_TYPE_FUNCTION);

            utils_assert(func_sym_id >= 0);

            int env_id = ast::add_enviroment(astree, &new_env);

            astree->current_env    = new_env;
            astree->current_env_id = env_id;
            
        }
        else if(strncmp("VAR", symbol_ptr, symbol_str_len) == 0) {
            int sym_id = add_symbol_to_env(
                astree->current_env, 
                &token->val.str, 
                SYMBOL_TYPE_VARIABLE);

            token->scope_id       = astree->current_env_id;
            token->inner_scope_id = sym_id;
        }
        else if(strncmp("PAR", symbol_ptr, symbol_str_len) == 0) {
            int sym_id = add_symbol_to_env(
                astree->current_env, 
                &token->val.str, 
                SYMBOL_TYPE_PARAMETER);

            token->scope_id       = astree->current_env_id;
            token->inner_scope_id = sym_id;
        }

        UTILS_LOGD(LOG_AST, "got symbol %.*s %.*s", symbol_str_len, symbol_ptr, id_str_len, id_str_ptr);
        
    } END;
    
    astree->buf.pos += tok_str_len;
    return ERR_NONE;
}

static Err fwrite_node_(AST* astree, ASTNode* node, FILE* stream)
{
    utils_assert(node);
    utils_assert(stream);

    Err err = ERR_NONE;
    if(node->token.type == token::TYPE_IDENTIFIER) {
        Env* identifier_env = get_enviroment(astree, node->token.scope_id);
        Symbol* sym = symbol_at(identifier_env, node->token.inner_scope_id);
        fprintf(stream, "( %.*s:%s ", node->token.val.str.len, node->token.val.str.str, symbol_type_str(sym->type));
    }
    else
        fprintf(stream, "( %s ", token::value_str(&node->token));

    if(node->left)
        err = fwrite_node_(astree, node->left, stream);
    else
        fprintf(stream, TOKEN_NIL_STR);

    if(node->right)
        err = fwrite_node_(astree, node->right, stream);
    else
        fprintf(stream, " " TOKEN_NIL_STR " ");

   fprintf(stream, ")");

    return err;
}

static void print_node_ptr_(FILE* file, void* ptr)
{
    utils_assert(file);
    utils_assert(ptr);

    fprintf(file, "%p", *(ASTNode**)ptr);
}

void node_print(FILE* stream, void* node)
{
    utils_assert(stream);
    utils_assert(node);

    ASTNode* node_ = (ASTNode*) node;
    fprintf(stream, "[%p; l: %p; r: %p; p: %p]", node_, node_->left, node_->right, node_->parent);
}

ASTNode* new_node(token::Token* token, ASTNode *left, ASTNode *right, ASTNode *parent)
{
    ASTNode* node = TYPED_CALLOC(1, ASTNode);

    if(!node) return NULL;

    if(token)
        *node = {
            .left   = left,
            .right  = right,
            .parent = parent,
            .token  = *token
        };
    else 
        *node = {
            .left   = left,
            .right  = right,
            .parent = parent,
            .token  = TOKEN_INITLIST
        };

    if(right)
        node->right->parent = node;

    if(left)
        node->left->parent = node;

    return node;
}

ASTNode* copy_subtree(AST* astree, ASTNode* node, ASTNode* parent)
{
    AST_ASSERT_OK_(astree);
    utils_assert(node);
    
    ASTNode *new_node = ast::new_node(&node->token, NULL, NULL, NULL);

    if(node->left)
        new_node->left = copy_subtree(astree, node->left, new_node);

    if(node->right)
        new_node->right = copy_subtree(astree, node->right, new_node);

    new_node->parent = parent;

    return new_node;
}

int add_enviroment(AST* astree, Env** enviroment)
{
    vector_push(&astree->envs, enviroment);
    return (signed) astree->envs.size - 1;
}

Env* get_enviroment(AST* astree, int env_id)
{
    return *(Env**)vector_at(&astree->envs, (unsigned) env_id);
}

Env* find_enviroment(AST* astree, utils_str_t* str, SymbolType type)
{
    for(size_t i = 0; i <= (unsigned) astree->current_env_id; ++i) {
        Env* env = *(Env**)vector_at(&astree->envs, i);
        if(find_symbol(env, str, type) != -1)
            return env;
    }
    return NULL;
}

#ifdef _DEBUG

#define GRAPHVIZ_FNAME_ "graphviz"
#define GRAPHVIZ_CMD_LEN_ 100

#define CLR_RED_LIGHT_   "\"#FFB0B0\""
#define CLR_GREEN_LIGHT_ "\"#B0FFB0\""
#define CLR_BLUE_LIGHT_  "\"#B0B0FF\""

#define CLR_RED_BOLD_    "\"#FF0000\""
#define CLR_GREEN_BOLD_  "\"#03C03C\""
#define CLR_BLUE_BOLD_   "\"#0000FF\""

void dump(AST* ast, ASTNode* node, Err err, const char* msg, const char* filename, int line, const char* funcname)
{
    utils_log_fprintf("<pre>\n"); 

    time_t cur_time = time(NULL);
    struct tm* iso_time = localtime(&cur_time);
    char time_buff[100];
    strftime(time_buff, sizeof(time_buff), "%F %T", iso_time);

    if(err != ERR_NONE) {
        utils_log_fprintf("<h3 style=\"color:red;\">[ERROR] [%s] from %s:%d: %s() </h3>", time_buff, filename, line, funcname);
        utils_log_fprintf("<h4><font color=\"red\">err: %s </font></h4>", strerr(err));
    }
    else
        utils_log_fprintf("<h3>[DEBUG] [%s] from %s:%d: %s() </h3>\n", time_buff, filename, line, funcname);

    if(msg)
        utils_log_fprintf("what: %s\n", msg);

    char* img_pref = dump_graphviz_(ast, node);

    utils_log_fprintf(
        "\n<img src=" IMG_DIR "/%s.svg\n", 
        strrchr(img_pref, '/') + 1
    );

    utils_log_fprintf("</pre>\n"); 

    utils_log_fprintf("<hr color=\"black\" />\n");

    NFREE(img_pref);
}

char* dump_graphviz_(AST* ast, ASTNode* node)
{
    utils_assert(ast);

    FILE* file = open_file(LOG_DIR "/" GRAPHVIZ_FNAME_ ".txt", "w");

    if(!file)
        exit(EXIT_FAILURE);

    fprintf(file, "digraph {\n rankdir=TB;\n"); 
    fprintf(file, "nodesep=0.9;\nranksep=0.75;\n");

    dump_node_graphviz_(ast, file, node, 1);

    fprintf(file, "};");

    fclose(file);
    
    create_dir(LOG_DIR "/" IMG_DIR);
    char* img_tmpnam = tempnam(LOG_DIR "/" IMG_DIR, "img-");
    utils_assert(img_tmpnam);

    static char strbuf[GRAPHVIZ_CMD_LEN_]= "";

    snprintf(
        strbuf, 
        GRAPHVIZ_CMD_LEN_, 
        "dot -T svg -o %s.svg " LOG_DIR "/" GRAPHVIZ_FNAME_ ".txt", 
        img_tmpnam
    );

    system(strbuf);

    return img_tmpnam;
}

void dump_node_graphviz_(AST* astree, FILE* file, ASTNode* node, int rank)
{
    utils_assert(file);

    if(!node) return;

    if(node->left)
        dump_node_graphviz_(astree, file, node->left, rank + 1); 
    if(node->right) 
        dump_node_graphviz_(astree, file, node->right, rank + 1);

    if(!node->left && !node->right)
        fprintf(
            file, 
            "node_%p["
            "shape=record,"
            "label=\" { parent: %p | addr: %p | type: %s | val: %s | { L: %p | R: %p } } \","
            "style=\"filled\","
            "color=" CLR_GREEN_BOLD_ ","
            "fillcolor=" CLR_GREEN_LIGHT_ ","
            "rank=%d"
            "];\n",
            node,
            node->parent,
            node,
            token::type_str(node->token.type),
            token::value_str(&node->token),
            node->left,
            node->right,
            rank
        );
    else
        fprintf(
            file, 
            "node_%p["
            "shape=none,"
            "label=<"
            "<table cellspacing=\"0\" border=\"0\" cellborder=\"1\">"
              "<tr>"
                "<td colspan=\"2\">parent %p</td>"
              "</tr>"
              "<tr>"
                "<td colspan=\"2\">addr: %p</td>"
              "</tr>"
              "<tr>"
                "<td colspan=\"2\">type: %s</td>"
              "</tr>"
              "<tr>"
                "<td colspan=\"2\">val: %s</td>"
              "</tr>"
              "<tr>"
                "<td bgcolor=" CLR_RED_LIGHT_ ">L: %p</td>"
                "<td bgcolor=" CLR_BLUE_LIGHT_">R: %p</td>"
              "</tr>"
            "</table>>,"
            "rank=%d,"
            "];\n",
            node,
            node->parent,
            node,
            token::type_str(node->token.type),
            token::value_str(&node->token),
            node->left,
            node->right,
            rank
        );

    if(node->left) {
        if(node->left->parent == node)
            fprintf(
                file,
                "node_%p -> node_%p ["
                "dir=both," 
                "color=" CLR_RED_BOLD_ ","
                "fontcolor=" CLR_RED_BOLD_ ","
                "];\n",
                node, 
                node->left
            );
        else
            fprintf(
                file,
                "node_%p -> node_%p ["
                "color=" CLR_RED_BOLD_ ","
                "fontcolor=" CLR_RED_BOLD_ ","
                "];\n",
                node, 
                node->left
            );
    }

    if(node->right) {
        if(node->right->parent == node)
            fprintf(
                file,
                "node_%p -> node_%p ["
                "dir=both," 
                "color=" CLR_BLUE_BOLD_ ","
                "fontcolor=" CLR_BLUE_BOLD_ ","
                "];\n",
                node, 
                node->right
            );
        else
            fprintf(
                file,
                "node_%p -> node_%p ["
                "color=" CLR_BLUE_BOLD_ ","
                "fontcolor=" CLR_BLUE_BOLD_ ","
                "];\n",
                node, 
                node->right
            );
    }
}

Err verify_(AST* ast)
{
    if(!ast)
        return NULLPTR;

    return ERR_NONE;
}


#endif // _DEBUG

} // ast
} // compiler
