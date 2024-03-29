# AtomC

## Lexical Rules

### Identifiers
```
ID: ^[a-zA-Z_][a-zA-Z0-9_]*$
```

### Keywords
```
TYPE_INT: "int"
TYPE_CHAR: "char"
TYPE_DOUBLE: "double"
IF: "if"
ELSE: "else"
WHILE: "while"
VOID: "void"
RETURN: "return"
STRUCT: "struct"
```

### Constants
```
INT: [0-9]+
DOUBLE: [0-9]+ ( '.' [0-9]+ ( [eE] [+-]? [0-9]+ )? | ( '.' [0-9]+ )? [eE] [+-]? [0-9]+ )
CHAR: ['] [^'] [']
STRING: ["] [^"]* ["]
```

### Delimiters
```
COMMA: ','
SEMICOLON: ';'
LPAR: '('
RPAR: ')'
LBRACKET: '['
RBRACKET: ']'
LACC: '{'
RACC: '}'
END: '\0' | EOF
```

### Operators
```
ADD: '+'
SUB: '-'
MUL: '*'
DIV: '/'
DOT: '.'
AND: '&&'
OR: '||'
NOT: '!'
ASSIGN: '='
EQUAL: '=='
NOTEQ: '!='
LESS: '<'
LESSEQ: '<='
GREATER: '>'
GREATEREQ: '>='
```

### Whitespace (ignored)
```
SPACE: [ \n\r\t]
LINECOMMENT: '//' [^\n\r\0]*
```


## Syntax Rules

**unit**: ( **structDef** | **fnDef** | **varDef** )* END

**structDef**: STRUCT ID LACC **varDef*** RACC SEMICOLON

**varDef**: **typeBase** ID **arrayDecl**? SEMICOLON

**typeBase**: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID

**arrayDecl**: LBRACKET INT? RBRACKET

**fnDef**: ( **typeBase** | VOID ) ID<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;LPAR ( **fnParam** ( COMMA **fnParam** )* )? RPAR<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**stmCompound**

**fnParam**: **typeBase** ID **arrayDecl**?

**stm**: **stmCompound**<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| IF LPAR **expr** RPAR **stm** ( ELSE **stm** )?<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| WHILE LPAR **expr** RPAR **stm**<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| RETURN **expr**? SEMICOLON<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| **expr**? SEMICOLON

**stmCompound**: LACC ( **varDef** | **stm** )* RACC

**expr**: **exprAssign**

**exprAssign**: **exprUnary** ASSIGN **exprAssign** | **exprOr**

**exprOr**: **exprOr** OR **exprAnd** | **exprAnd**

**exprAnd**: **exprAnd** AND **exprEq** | **exprEq**

**exprEq**: **exprEq** ( EQUAL | NOTEQ ) **eexprRel** | **exprRel**

**exprRel**: **exprRel** ( LESS | LESSEQ | GREATER | GREATEREQ ) **exprAdd** | **exprAdd**

**exprAdd**: **exprAdd** ( ADD | SUB ) **exprMul** | **exprMul**

**exprMul**: **exprMul** ( MUL | DIV ) **exprCast** | **exprCast**

**exprCast**: LPAR **typeBase** **arrayDecl**? RPAR **exprCast** | **exprUnary**

**exprUnary**: ( SUB | NOT ) **exprUnary** | **exprPostfix**

**exprPostfix**: **exprPostfix** LBRACKET **expr** RBRACKET<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| **exprPostfix** DOT ID<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| **exprPrimary**

**exprPrimary**: ID ( LPAR ( **expr** ( COMMA **expr** )* )? RPAR )?<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| INT | DOUBLE | CHAR | STRING | LPAR **expr** RPAR