#pragma once
#include "hashutils.h"
#include "utils.h"

namespace compiler {
namespace token {

enum Type
{
    TYPE_OPERATOR,
    TYPE_KEYWORD,
    TYPE_SEPARATOR,
    TYPE_IDENTIFIER,
    TYPE_LITERAL,
    TYPE_TERMINATOR,
    TYPE_FAKE
};

enum KeywordType
{
    KEYWORD_TYPE_WHILE,
    KEYWORD_TYPE_IF,
    KEYWORD_TYPE_ELSE
};

enum SeparatorType {
    SEPARATOR_TYPE_PAR_OPEN,
    SEPARATOR_TYPE_PAR_CLOSE
};

enum OperatorType
{
    OPERATOR_TYPE_ADD,
    OPERATOR_TYPE_SUB,
    OPERATOR_TYPE_MUL,
    OPERATOR_TYPE_DIV,
    OPERATOR_TYPE_POW,
};

struct Identifier
{
    const char* str;
    ssize_t str_len;
    utils_hash_t hash;
    ssize_t id;
};

union Value
{
    ssize_t id;
    OperatorType op_type;
    KeywordType kw_type;
    SeparatorType sep_type;
    int num;
};

struct Token
{
    Type type;
    Value val;
};

struct TokenInfo
{
    const char* str;
    ssize_t str_len;
    Type type;
    Value val;
};

#define MAKE_OPERATOR(str, type, val) \
    TokenInfo { str, SIZEOF(str) - 1, type, { .op_type = val } }

#define MAKE_KEYWORD(str, type, val) \
    TokenInfo { str, SIZEOF(str) - 1, type, { .kw_type = val } }

#define MAKE_SEPARATOR(str, type, val) \
    TokenInfo { str, SIZEOF(str) - 1, type, { .sep_type = val } }

static TokenInfo TokenArr[] = 
{
    MAKE_KEYWORD("while",TYPE_KEYWORD, KEYWORD_TYPE_WHILE),
    MAKE_KEYWORD("if",   TYPE_KEYWORD, KEYWORD_TYPE_IF),
    MAKE_KEYWORD("else", TYPE_KEYWORD, KEYWORD_TYPE_ELSE),
    MAKE_OPERATOR("+",   TYPE_OPERATOR, OPERATOR_TYPE_ADD),
    MAKE_OPERATOR("-",  TYPE_OPERATOR, OPERATOR_TYPE_SUB),
    MAKE_OPERATOR("*",  TYPE_OPERATOR, OPERATOR_TYPE_MUL),
    MAKE_OPERATOR("/",  TYPE_OPERATOR, OPERATOR_TYPE_DIV),
    MAKE_SEPARATOR("(", TYPE_SEPARATOR, SEPARATOR_TYPE_PAR_OPEN),
    MAKE_SEPARATOR(")", TYPE_SEPARATOR, SEPARATOR_TYPE_PAR_CLOSE),
};

const char* type_str(Type node_type);

const char* node_op_type_str(OperatorType op_type);

} // token
} // compiler
