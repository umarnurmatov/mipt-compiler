# Language

## Grammar
Language grammar in extended Backus-Naur form:
```
GENERAL            ::= FUNCTION_DECL+'\0'

FUNCTION_DECL      ::= "defun" IDENTIFIER '(' PARAMETER_LIST ')' BLOCK
PARAMETER_LIST     ::= { IDENTIFIER { ',' IDENTIFIER }* } | <none>

BLOCK              ::= "{" STATEMENT* "}"
STATEMENT          ::= {  WHILE | IF } | { { DECLARATION | ASSIGNMENT | RETURN } ';' }

WHILE              ::= "while" EXPRESSION BLOCK
IF                 ::= "if" EXPRESSION BLOCK ELSE?
ELSE               ::= "else" BLOCK
RETURN             ::= "return" EXPRESSION
DECLARATION        ::= IDENTIFIER | ASSIGNMENT
ASSIGNMENT         ::= IDENTIFIER '=' EXPRESSION

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
PRIMARY            ::= FUNCTION_CALL | IDENTIFIER | LITERAL | '(' EXPRESSION ')'

LITERAL            ::= NUMERIC_LITERAL | STRING_LITERAL
NUMERIC_LITERAL    ::= [0-9]+
STRING_LITERAL     ::= ["]([^"\\\n]|\\.)*["]
IDENTIFIER         ::= [a-zA-Z_][0-9a-zA-Z_]*
```

## AST format

Tree is written in infix format: ``` (parent left right) ```:

```
WHILE                 -> (<while> <condition> <body>)
IF [if-clause only]   -> (<if> <condition> <body>)
IF [with else-clause] -> (<if> <condition> (<else> <if-body> <else-body>))

FUNCTION_CALL         -> (<call> <identifier> <argument_list>)
ARGUMENT_LIST         -> (<comma> <expression> (<comma> <expression> (...)))

BLOCK                 -> (<cur_open> <statement> (<cur_open> <statement> (...)))
ASSIGMENT             -> (<assigment> <identifier> <expression>)

FUNCTION_DECL         -> (<identifier> <parameter_list> <body>)
PARAMETER_LIST        -> (<comma> <identifier> (<comma> <identifier> (...)))
```
