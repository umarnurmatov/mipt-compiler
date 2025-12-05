#include "ast.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>

#include "hashutils.h"
#include "logutils.h"
#include "memutils.h"
#include "ioutils.h"
#include "assertutils.h"
#include "logutils.h"
#include "vector.h"

namespace compiler {
namespace ast {

#define LOG_CTG_AST "DIFFTREE"
#define NIL_STR "nil"

#ifdef _DEBUG

#define AST_ASSERT_OK_(ast)                      \
    {                                                        \
        Err err = verify_(ast);  \
        if(err != ERR_NONE) {                      \
            AST_DUMP(ast, err);                  \
            utils_assert(err == ERR_NONE);         \
        }                                                    \
    }

#else // _DEBUG

#define AST_ASSERT_OK_(ast) 

#endif // _DEBUG

static Err fwrite_node_(ASTNode* node, FILE* file);

static void add_variable_(AST* astree, token::Identifier new_var);

static char* node_value_str_(AST* astree, token::Type node_type, token::Token val);

ATTR_UNUSED static void print_node_ptr_(FILE* file, void* ptr);

#ifdef _DEBUG

char* dump_graphviz_(AST* ast, ASTNode* node);

void dump_node_graphviz_(AST* astree, FILE* file, ASTNode* node, int rank);

Err verify_(AST* ast);

#endif // _DEBUG

#define DEFAULT_VAR_VECTOR_CAPACITY       10
#define DEFAULT_TO_DELETE_VECTOR_CAPACITY 10

Err ctor(AST* astree)
{
    utils_assert(astree);

    astree->size = 0;
    
    vector_ctor(&astree->identifiers, DEFAULT_VAR_VECTOR_CAPACITY, sizeof(token::Identifier));
    vector_ctor(&astree->to_delete, DEFAULT_TO_DELETE_VECTOR_CAPACITY, sizeof(ASTNode*));

    return ERR_NONE;
}

Err copy_tree(AST* from, AST* to)
{
    to->size = from->size;
    to->root = copy_subtree(from, from->root, NULL);

    vector_ctor(&to->identifiers, DEFAULT_VAR_VECTOR_CAPACITY, sizeof(token::Identifier));
    vector_ctor(&to->to_delete, DEFAULT_TO_DELETE_VECTOR_CAPACITY, sizeof(ASTNode*));

    for(size_t i = 0; i < from->identifiers.size; ++i)
        vector_push(&to->identifiers, vector_at(&from->identifiers, i));

    return ERR_NONE;
}

void dtor(AST* astree)
{
    utils_assert(astree);

    free_subtree(astree->root); 

    astree->size = 0;
    astree->root = NULL;

    vector_dtor(&astree->identifiers);

    for(size_t i = 0; i < astree->to_delete.size; ++i)
        free_subtree(*(ASTNode**)vector_at(&astree->to_delete, i));

    vector_dtor(&astree->to_delete);
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
    vector_push(&astree->to_delete, &node);
}

Err tree_fwrite(AST* astree, const char* filename)
{
    utils_assert(filename);

    FILE* file = open_file(filename, "w");
    file verified(return IO_ERR);

    Err err = fwrite_node_(astree->root, file);

    UTILS_LOGD(LOG_CTG_AST, "Writing done");

    fclose(file);

    return err;
}

Err fwrite_node_(ASTNode* node, FILE* file)
{
    utils_assert(node);
    utils_assert(file);

    Err err = ERR_NONE;
    int io_err = 0;

    io_err = fprintf(file, "(");
    io_err >= 0 verified(return IO_ERR);

    if(node->left)
        err = fwrite_node_(node->left, file);
    else
        fprintf(file, NIL_STR);

    if(node->right)
        err = fwrite_node_(node->right, file);
    else
        fprintf(file, " " NIL_STR " ");

    io_err = fprintf(file, ")");
    io_err >= 0 verified(return IO_ERR);

    return err;
}

static void print_node_ptr_(FILE* file, void* ptr)
{
    fprintf(file, "%p", *(ASTNode**)ptr);
}

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

void node_print(FILE* stream, void* node)
{
    utils_assert(stream);
    utils_assert(node);

    ASTNode* node_ = (ASTNode*) node;
    fprintf(stream, "[%p; l: %p; r: %p; p: %p]", node_, node_->left, node_->right, node_->parent);
}

ASTNode* new_node(token::Type node_type, token::Token node_value, ASTNode *left, ASTNode *right, ASTNode *parent)
{
    ASTNode* node = TYPED_CALLOC(1, ASTNode);

    if(!node) return NULL;

    *node = {
        .left = left,
        .right = right,
        .parent = parent,
        .type = node_type,
        .value = node_value,
    };

    if(right)
        node->right->parent = node;

    if(left)
        node->left->parent = node;

    return node;
}

ASTNode* copy_subtree(AST* astree, ASTNode* node, ASTNode* parent)
{
    ASTNode *new_node = ast::new_node(node->type, node->value, NULL, NULL, NULL);

    if(node->left)
        new_node->left = copy_subtree(astree, node->left, new_node);

    if(node->right)
        new_node->right = copy_subtree(astree, node->right, new_node);

    new_node->parent = parent;

    return new_node;
}

static char* node_value_str_(AST* astree, token::Type node_type, token::Token val)
{
    static const size_t buffer_len = 100;
    static char buffer[buffer_len] = "";

    switch(node_type) {
        case token::TYPE_OPERATOR:
            return const_cast<char*>(node_op_type_str(val.op_type));
        case token::TYPE_VAR: 
        {
            token::Identifier* var = find_identifier(astree, val.id);
            if(var) {
                snprintf(buffer, buffer_len, "%s", var->str);
                return buffer;
            }
            break;
        }
        case token::TYPE_NUM:
            strfromd(buffer, buffer_len, "%f", val.num);
            return buffer;
        case token::TYPE_FAKE:
            return const_cast<char*>("fakeval");
        default:
            return const_cast<char*>("???");
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
    utils_log_fprintf(
        "<style>"
        "table {"
          "border-collapse: collapse;"
          "border: 1px solid;"
          "font-size: 0.9em;"
        "}"
        "th,"
        "td {"
          "border: 1px solid rgb(160 160 160);"
          "padding: 8px 10px;"
        "}"
        "</style>\n"
    );

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


    BEGIN {
        if(!ast->buf.ptr) GOTO_END;

        utils_log_fprintf("buf.pos = %ld\n", ast->buf.pos); 
        utils_log_fprintf("buf.len = %ld\n", ast->buf.len); 
        utils_log_fprintf("buf.ptr[%p] = ", ast->buf.ptr); 

        if(err == INVALID_BUFPOS) {
            for(ssize_t i = 0; i < ast->buf.len; ++i)
                utils_log_fprintf("%c", ast->buf.ptr[i]);
            utils_log_fprintf("\n");
            GOTO_END;
        }

#define CLR_PREV "#09AB00"
#define CLR_CUR  "#C71022"
#define CLR_NEXT "#1022C7"

#define LOG_PRINTF_CHAR(ch, col) \
    if(ch == '\0') \
        utils_log_fprintf("<span style=\"border: 1px solid" col ";\">0</span>"); \
    else if(isspace(ch)) \
        utils_log_fprintf("<span style=\"border: 1px solid" col ";\"> </span>"); \
    else \
        utils_log_fprintf("%c", ch);

        utils_log_fprintf("<font color=\"" CLR_PREV "\">"); 
        for(ssize_t i = 0; i < ast->buf.pos; ++i) {
            LOG_PRINTF_CHAR(ast->buf.ptr[i], CLR_PREV);
        }
        utils_log_fprintf("</font>");


        utils_log_fprintf("<font color=\"" CLR_CUR "\"><b>");
        LOG_PRINTF_CHAR(ast->buf.ptr[ast->buf.pos], CLR_CUR);
        utils_log_fprintf("</b></font>");


        utils_log_fprintf("<font color=\"" CLR_NEXT "\">"); 
        for(ssize_t i = ast->buf.pos + 1; i < ast->buf.len; ++i) {
            LOG_PRINTF_CHAR(ast->buf.ptr[i], CLR_NEXT);
        }
        utils_log_fprintf("</font>\n");

    } END;

#undef CLR_PREV
#undef CLR_CUR
#undef CLR_NEXT
#undef LOG_PRINTF_CHAR

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
            node_type_str(node->type),
            node_value_str_(astree, node->type, node->value),
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
            node_type_str(node->type),
            node_value_str_(astree, node->type, node->value),
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

    if(ast->buf.pos > ast->buf.len)
        return INVALID_BUFPOS;
    
    return ERR_NONE;
}


#endif // _DEBUG

} // ast
} // compiler
