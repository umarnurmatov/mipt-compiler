# Language

## Grammar
Language grammar in extended Backus-Naur form:
```
GENERAL            ::= FUNCTION_DECL+'\0'

FUNCTION_DECL      ::= ["ret"]? IDENTIFIER '(' PARAMETER_LIST ')' COMPOUND_STATEMENT
PARAMETER_LIST     ::= { IDENTIFIER { ',' IDENTIFIER }* } | <none>

BLOCK              ::= "{" STATEMENT* "}"
STATEMENT          ::= {  WHILE | IF | DECLARATION | ASSIGNMENT | RETURN } ';'

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
IDENTIFIER         ::= [а-яА-Я_][0-9а-яА-Я_]*
```

## AST format
