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
    OPERATOR_TYPE_POW        = 0x04,
};

enum KeywordType
{
    KEYWORD_TYPE_WHILE       = 0x05,
    KEYWORD_TYPE_IF          = 0x06,
    KEYWORD_TYPE_ELSE        = 0x07,
};

enum SeparatorType {
    SEPARATOR_TYPE_PAR_OPEN  = 0x08,
    SEPARATOR_TYPE_PAR_CLOSE = 0x09
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
    MAKE_KEYWORD  ("while" , "WHILE"       , TYPE_KEYWORD   , KEYWORD_TYPE_WHILE      ),
    MAKE_KEYWORD  ("if"    , "IF"          , TYPE_KEYWORD   , KEYWORD_TYPE_IF         ),
    MAKE_KEYWORD  ("else"  , "ELSE"        , TYPE_KEYWORD   , KEYWORD_TYPE_ELSE       ),
    MAKE_SEPARATOR("("     , "PAR_OPEN"    , TYPE_SEPARATOR , SEPARATOR_TYPE_PAR_OPEN ),
    MAKE_SEPARATOR(")"     , "PAR_CLOSE"   , TYPE_SEPARATOR , SEPARATOR_TYPE_PAR_CLOSE),
};

const char* type_str(Type token_type);

const char* value_str(Token* token);

} // token
} // compiler
