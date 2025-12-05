#pragma once
#include "hashutils.h"
#include "utils.h"

namespace compiler {
namespace token {

const size_t MAX_OP_NAME_LEN = 10;

enum Type
{
    TYPE_OPERATOR,
    TYPE_VAR,
    TYPE_NUM,
    TYPE_FAKE
};

enum OperatorType
{
    OPERATOR_TYPE_ADD,
    OPERATOR_TYPE_SUB,
    OPERATOR_TYPE_MUL,
    OPERATOR_TYPE_DIV,
    OPERATOR_TYPE_POW,
};

enum OperatorArgnum
{
    OPERATOR_ARGNUM_NONE = 0x00,
    OPERATOR_ARGNUM_1    = 0x01,
    OPERATOR_ARGNUM_2    = 0x02,

};

enum OperatorPrecedance
{
    OPERATOR_PRECEDANCE_0 = 0x00,
    OPERATOR_PRECEDANCE_1 = 0x01,
    OPERATOR_PRECEDANCE_2 = 0x02,
    OPERATOR_PRECEDANCE_3 = 0x03,
    OPERATOR_PRECEDANCE_4 = 0x04,

};

struct Operator
{
    OperatorType type;
    OperatorArgnum argnum;
    OperatorPrecedance precedance;
    const char* str;

    utils_hash_t hash;

};

struct Identifier
{
    char* str;
    utils_hash_t hash;
    int val;
};

union Token
{
    Identifier id;
    OperatorType op_type;
    int num;
};

#define MAKE_OPERATOR(str, type, argnum, precedance) \
    { type, argnum, precedance, str, utils_djb2_hash(str, SIZEOF(str)) }

static Operator op_arr[] = 
{
    MAKE_OPERATOR("+"     ,  OPERATOR_TYPE_ADD  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_1),
    MAKE_OPERATOR("-"     ,  OPERATOR_TYPE_SUB  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_1),
    MAKE_OPERATOR("*"     ,  OPERATOR_TYPE_MUL  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_2),
    MAKE_OPERATOR("/"     ,  OPERATOR_TYPE_DIV  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_2),
    MAKE_OPERATOR("^"     ,  OPERATOR_TYPE_POW  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_3),
};

const Operator* get_operator(OperatorType op_type);

const char* type_str(Type node_type);

const char* node_op_type_str(OperatorType op_type);

} // token
} // compiler
