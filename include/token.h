#pragma once
#include "hashutils.h"
#include "utils.h"

#define TOKEN_NIL_STR "nil"

namespace compiler {
namespace token {

enum Type
{
    TYPE_OPERATOR,
    TYPE_KEYWORD,
    TYPE_SEPARATOR,
    TYPE_IDENTIFIER,
    TYPE_NUM_LITERAL,
    TYPE_CALL,
    TYPE_TERMINATOR,
    TYPE_FAKE
};

/* Enumerator values MUST be sequential */

enum OperatorType
{
    OPERATOR_TYPE_ADD        = 0x00,
    OPERATOR_TYPE_SUB        = 0x01,
    OPERATOR_TYPE_MUL        = 0x02,
    OPERATOR_TYPE_DIV        = 0x03,
    OPERATOR_TYPE_POW        = 0x05,
    OPERATOR_TYPE_OR         = 0x06,
    OPERATOR_TYPE_AND        = 0x07,
    OPERATOR_TYPE_EQ         = 0x08,
    OPERATOR_TYPE_NEQ        = 0x09,
    OPERATOR_TYPE_GT         = 0x0A,
    OPERATOR_TYPE_LT         = 0x0B,
    OPERATOR_TYPE_GEQ        = 0x0D,
    OPERATOR_TYPE_LEQ        = 0x0C,
    OPERATOR_TYPE_ASSIGN     = 0x0E,
};

enum KeywordType
{
    KEYWORD_TYPE_WHILE       = 0x0F,
    KEYWORD_TYPE_IF          = 0x10,
    KEYWORD_TYPE_ELSE        = 0x11,
    KEYWORD_TYPE_DEFUN       = 0x12,
    KEYWORD_TYPE_RETURN      = 0x13,
};

enum SeparatorType {
    SEPARATOR_TYPE_PAR_OPEN    = 0x14,
    SEPARATOR_TYPE_PAR_CLOSE   = 0x15,
    SEPARATOR_TYPE_COMMA       = 0x16,
    SEPARATOR_TYPE_SEMICOLON   = 0x17,
    SEPARATOR_TYPE_CURLY_OPEN  = 0x18,
    SEPARATOR_TYPE_CURLY_CLOSE = 0x19,
};

/* --------------------------------- */

struct Identifier
{
    const char*  str;
    ssize_t      str_len;
    utils_hash_t hash;
    ssize_t      id;
};

union Value
{
    ssize_t       id;

    OperatorType  op_type;
    KeywordType   kw_type;
    SeparatorType sep_type;
    int           enum_val;

    int           num;
};

struct Token
{
    Type type;
    Value val;

    ssize_t fileline;
    ssize_t filepos;
};

struct TokenInfo
{
    const char* str;
    ssize_t     str_len;

    const char* str_internal;
    ssize_t     str_internal_len;

    Type        type;
    Value       val;
};

#define MAKE_OPERATOR(str, str_internal, type, val) \
    TokenInfo { str, SIZEOF(str) - 1, str_internal, \
                SIZEOF(str_internal) - 1, type, { .op_type = val } }

#define MAKE_KEYWORD(str, str_internal, type, val) \
    TokenInfo { str, SIZEOF(str) - 1, str_internal,\
                SIZEOF(str_internal) - 1, type, { .kw_type = val } }

#define MAKE_SEPARATOR(str, str_internal, type, val) \
    TokenInfo { str, SIZEOF(str) - 1, str_internal,  \
                SIZEOF(str_internal) - 1, type, { .sep_type = val } }

ATTR_UNUSED static TokenInfo TokenArr[] = 
{
    MAKE_OPERATOR ("+"     , "ADD"         , TYPE_OPERATOR  , OPERATOR_TYPE_ADD       ),
    MAKE_OPERATOR ("-"     , "SUB"         , TYPE_OPERATOR  , OPERATOR_TYPE_SUB       ),
    MAKE_OPERATOR ("*"     , "MUL"         , TYPE_OPERATOR  , OPERATOR_TYPE_MUL       ),
    MAKE_OPERATOR ("/"     , "DIV"         , TYPE_OPERATOR  , OPERATOR_TYPE_DIV       ),
    MAKE_OPERATOR ("^"     , "POW"         , TYPE_OPERATOR  , OPERATOR_TYPE_POW       ),
    MAKE_OPERATOR ("|"     , "OR"          , TYPE_OPERATOR  , OPERATOR_TYPE_OR        ),
    MAKE_OPERATOR ("&"     , "AND"         , TYPE_OPERATOR  , OPERATOR_TYPE_AND       ),
    MAKE_OPERATOR ("!="    , "NEQ"         , TYPE_OPERATOR  , OPERATOR_TYPE_NEQ       ),
    MAKE_OPERATOR ("=="    , "EQ"          , TYPE_OPERATOR  , OPERATOR_TYPE_EQ        ),
    MAKE_OPERATOR (">"     , "GT"          , TYPE_OPERATOR  , OPERATOR_TYPE_GT        ),
    MAKE_OPERATOR ("<"     , "LT"          , TYPE_OPERATOR  , OPERATOR_TYPE_LT        ),
    MAKE_OPERATOR (">="    , "GEQ"         , TYPE_OPERATOR  , OPERATOR_TYPE_GEQ       ),
    MAKE_OPERATOR ("<="    , "LEQ"         , TYPE_OPERATOR  , OPERATOR_TYPE_NEQ       ),
    MAKE_OPERATOR ("="     , "ASSGN"       , TYPE_OPERATOR  , OPERATOR_TYPE_ASSIGN    ),
    MAKE_KEYWORD  ("while" , "WHILE"       , TYPE_KEYWORD   , KEYWORD_TYPE_WHILE      ),
    MAKE_KEYWORD  ("if"    , "IF"          , TYPE_KEYWORD   , KEYWORD_TYPE_IF         ),
    MAKE_KEYWORD  ("else"  , "ELSE"        , TYPE_KEYWORD   , KEYWORD_TYPE_ELSE       ),
    MAKE_KEYWORD  ("defun" , "DEFUN"       , TYPE_KEYWORD   , KEYWORD_TYPE_DEFUN      ),
    MAKE_KEYWORD  ("return", "RET"         , TYPE_KEYWORD   , KEYWORD_TYPE_RETURN     ),
    MAKE_SEPARATOR("("     , "PAR_OPEN"    , TYPE_SEPARATOR , SEPARATOR_TYPE_PAR_OPEN ),
    MAKE_SEPARATOR(")"     , "PAR_CLOSE"   , TYPE_SEPARATOR , SEPARATOR_TYPE_PAR_CLOSE),
    MAKE_SEPARATOR(","     , "COMMA"       , TYPE_SEPARATOR , SEPARATOR_TYPE_COMMA    ),
    MAKE_SEPARATOR(";"     , "SEMICOL"     , TYPE_SEPARATOR , SEPARATOR_TYPE_SEMICOLON),
    MAKE_SEPARATOR("{"     , "CUR_OPEN"    , TYPE_SEPARATOR , SEPARATOR_TYPE_CURLY_OPEN),
    MAKE_SEPARATOR("}"     , "CUR_CLOSE"   , TYPE_SEPARATOR , SEPARATOR_TYPE_CURLY_CLOSE),
};

const char* type_str(Type token_type);

const char* value_str(Token* token);

} // token
} // compiler
