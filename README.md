# Language

## TODO
1. extract buffer as separate structure
2. use hash table for symbol table
    1. add normal! scopes
3. check for function params


## Grammar
Language grammar in extended Backus-Naur form:
```
GENERAL            ::= FUNCTION_DECL+'\0'

FUNCTION_DECL      ::= "defun" IDENTIFIER '(' PARAMETER_LIST ')' BLOCK
PARAMETER_LIST     ::= { IDENTIFIER { ',' IDENTIFIER }* } | <none>

BLOCK              ::= "{" STATEMENT* "}"
STATEMENT          ::= {  WHILE | IF } | { { IN | OUT | DECLARATION | ASSIGNMENT | RETURN } ';' }

WHILE              ::= "while" EXPRESSION BLOCK
IF                 ::= "if" EXPRESSION BLOCK ELSE?
ELSE               ::= "else" BLOCK
RETURN             ::= "return" EXPRESSION
DECLARATION        ::= IDENTIFIER | ASSIGMENT
ASSIGNMENT         ::= IDENTIFIER '=' EXPRESSION
IN                 ::= 'in' IDENTIFIER
OUT                ::= 'out' EXPRESSION
OUT                ::= 'ramset' EXPRESSION

FUNCTION_CALL      ::= IDENTIFIER'(' ARGUMENT_LIST ')'
ARGUMENT_LIST      ::= { EXPRESSION { ',' EXPRESSION }* } | <none>

EXPRESSION         ::= OP_OR
OP_OR              ::= OP_AND { ["|"] OP_AND }*
OP_AND             ::= OP_EQ_NEQ { ["&"] OP_EQ_NEQ }*
OP_EQ_NEQ          ::= OP_GT_LT { ["==""!="] OP_GT_LT }*
OP_GT_LT           ::= OP_ADD_SUB { ['>''<''>=''<='] OP_ADD_SUB }*
OP_ADD_SUB         ::= OP_MUL_DIV { ['+''-'] OP_MUL_DIV }*
OP_MUL_DIV         ::= OP_POW { ['*''/'] OP_POW }*
OP_POWER           ::= PRIMARY { ['^'] PRIMARY }*
OP_SQRT            ::= ['@'] OP_SQRT | OP_IN_OUT
PRIMARY            ::= FUNCTION_CALL | IDENTIFIER | LITERAL | '(' EXPRESSION ')'

LITERAL            ::= NUMERIC_LITERAL | STRING_LITERAL
NUMERIC_LITERAL    ::= [0-9]+
STRING_LITERAL     ::= ["]([^"\\\n]|\\.)*["]
IDENTIFIER         ::= [a-zA-Z_][0-9a-zA-Z_]*
```

## AST format

Tree is written in infix format: ``` (parent left right) ```:

```
WHILE                 -> (WHILE <condition> <body>)
IF [if-clause only]   -> (IF <condition> <body>)
IF [with else-clause] -> (IF <condition> (ELSE <if-body> <else-body>))

FUNCTION_CALL         -> (CALL <identifier> <argument_list>)
<argument_list>       -> (COMMA <expression> (COMMA <expression> (...)))

BLOCK                 -> (SEMICOL <statement> (SEMICOL <statement> (...)))
ASSIGMENT             -> (ASSGN <identifier> <expression>)

FUNCTION_DECL         -> (<identifier> <parameter_list> <body>)
<parameter_list>      -> (COMMA <identifier> (COMMA <identifier> (...)))

RETURN                -> (RET <expression> <function>)

<identifier> = <id_str>:["VAR""FUNC"PAR"]
```

## ASM

FUNC_DECL ->

:<func_name>
<body>

WHILE ->

:<beginwhile>
<expr>
PUSH 0
JE :<endwhile>
<body>
JMP <beginwhile>
:<endwhile>

IF (no else) ->

<expr>
PUSH 0
JE :<endif>
<body>
:<endif>

IF (with else) ->

<expr>
PUSH 0
JE :<else>
<if-body>
JMP :<endif>
:<else>
<else-body>
:<endif>


