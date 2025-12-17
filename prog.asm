:func_main
PUSH 5
POPM [SP+2]
PUSH 0
POPM [SP+3]
:beginwhile_0
PUSHM [SP+2]
PUSH 0
JE :endwhile_0
PUSHM [SP+2]
PUSH 1
SUB
POPM [SP+2]
PUSHM [SP+3]
PUSH 1
ADD
POPM [SP+3]
JMP :beginwhile_0
:endwhile_0
OUT
HLT
