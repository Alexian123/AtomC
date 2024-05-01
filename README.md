# AtomC

## Lexical Rules

### Identifiers
```
ID: ^[a-zA-Z_][a-zA-Z0-9_]*$
```

### Keywords
```c
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
```c
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
```c
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


## Syntax/Domain Rules

**unit**: ( **structDef** | **fnDef** | **varDef** )* END<br>

**structDef**
```c
// struct name must be unique in the domain
// inside the struct two or more variables cannot share the same name
structDef: STRUCT ID[tkName] LACC
    {
    Symbol *s=findSymbolInDomain(symTable,tkName->text);
    if(s)tkerr("symbol redefinition: %s",tkName->text);
    s=addSymbolToDomain(symTable,newSymbol(tkName->text,SK_STRUCT));
    s->type.tb=TB_STRUCT;
    s->type.s=s;
    s->type.n=-1;
    pushDomain();
    owner=s;
    }
    varDef* RACC SEMICOLON
    {
    owner=NULL;
    dropDomain();
    }
```
<br>

**varDef**
```c
// variable name must be unique in the domain
// arrays must have the given dimension (int v[] is not valid)
varDef: {Type t;} typeBase[&t] ID[tkName]
    ( arrayDecl[&t]
    {if(t.n==0)tkerr("An array must have a specified dimension");}
    )? SEMICOLON
    {
    Symbol *var=findSymbolInDomain(symTable,tkName->text);
    if(var)tkerr("symbol redefinition: %s",tkName->text);
    var=newSymbol(tkName->text,SK_VAR);
    var->type=t;
    var->owner=owner;
    addSymbolToDomain(symTable,var);
    if(owner){
    switch(owner->kind){
    case SK_FN:
    var->varIdx=symbolsLen(owner->fn.locals);
    addSymbolToList(&owner->fn.locals,dupSymbol(var));
    break;
    case SK_STRUCT:
    var->varIdx=typeSize(&owner->type);
    addSymbolToList(&owner->structMembers,dupSymbol(var));
    break;
    }
    }else{
    var->varMem=safeAlloc(typeSize(&t));
    }
    }
```
<br>

**typeBase**
```c
// if the base type is a struct, it must have been defined earlier 
typeBase[out Type *t]: {t->n=-1;}
    (
    INT {t->tb=TB_INT;}
    | DOUBLE {t->tb=TB_DOUBLE;}
    | CHAR {t->tb=TB_CHAR;}
    | STRUCT ID[tkName]
    {
    t->tb=TB_STRUCT;
    t->s=findSymbol(tkName->text);
    if(!t->s)tkerr("structura nedefinita: %s",tkName->text);
    }
    )
```
<br>

**arrayDecl**
```c
arrayDecl[inout Type *t]: LBRACKET
    ( INT[tkSize] {t->n=tkSize->i;} | {t->n=0;} )
    RBRACKET
```
<br>

**fnDef**
```c
// function name must be unique in the domain
// function local domain starts immediately after LPAR
// function body {...} does not define a new subdomain inside the function local domain
fnDef: {Type t;}
    ( typeBase[&t] | VOID {t.tb=TB_VOID;} ) ID[tkName] LPAR
    {
    Symbol *fn=findSymbolInDomain(symTable,tkName->text);
    if(fn)tkerr("symbol redefinition: %s",tkName->text);
    fn=newSymbol(tkName->text,SK_FN);
    fn->type=t;
    addSymbolToDomain(symTable,fn);
    owner=fn;
    pushDomain();
    }
    ( fnParam ( COMMA fnParam )* )? RPAR stmCompound[false]
    {
    dropDomain();
    owner=NULL;
    }
```
<br>

**fnParam**
```c
// parameter name must be unique in the domain
// parameters can be arrays with a given dimension, but in this case the dimension will be deleterd (int v[10] -> int v[])
fnParam: {Type t;} typeBase[&t] ID[tkName]
    (arrayDecl[&t] {t.n=0;} )?
    {
    Symbol *param=findSymbolInDomain(symTable,tkName->text);
    if(param)tkerr("symbol redefinition: %s",tkName->text);
    param=newSymbol(tkName->text,SK_PARAM);
    param->type=t;
    param->owner=owner;
    param->paramIdx=symbolsLen(owner->fn.params);
    // the parameter is added in the current domain as well as in the function parameters
    addSymbolToDomain(symTable,param);
    addSymbolToList(&owner->fn.params,dupSymbol(param));
    }
```
<br>

**stm**
```c
// common body {...} of instruction defines a new domain
stm: stmCompound[true]
    | IF LPAR expr RPAR stm ( ELSE stm )?
    | WHILE LPAR expr RPAR stm
    | RETURN expr? SEMICOLON
    | expr? SEMICOLON
```
<br>

**stmCompound**
```c
// a new domain is defined only on demand
stmCompound[in bool newDomain]: LACC
    {if(newDomain)pushDomain();}
    ( varDef | stm )* RACC
    {if(newDomain)dropDomain();}
```
<br>

**expr**: **exprAssign**<br>

**exprAssign**: **exprUnary** ASSIGN **exprAssign** | **exprOr**<br>

**exprOr**: **exprOr** OR **exprAnd** | **exprAnd**<br>

**exprAnd**: **exprAnd** AND **exprEq** | **exprEq**<br>

**exprEq**: **exprEq** ( EQUAL | NOTEQ ) **eexprRel** | **exprRel**<br>

**exprRel**: **exprRel** ( LESS | LESSEQ | GREATER | GREATEREQ ) **exprAdd** | **exprAdd**<br>

**exprAdd**: **exprAdd** ( ADD | SUB ) **exprMul** | **exprMul**<br>

**exprMul**: **exprMul** ( MUL | DIV ) **exprCast** | **exprCast**<br>

**exprCast**
```c
// an argument is added because it is required by typeBase and arrayDecl
// t will be used later
exprCast: LPAR {Type t;} typeBase[&t] arrayDecl[&t]? RPAR exprCast | exprUnary
```
<br>

**exprUnary**: ( SUB | NOT ) **exprUnary** | **exprPostfix**<br>

**exprPostfix**: **exprPostfix** LBRACKET **expr** RBRACKET<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| **exprPostfix** DOT ID<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| **exprPrimary**<br>

**exprPrimary**: ID ( LPAR ( **expr** ( COMMA **expr** )* )? RPAR )?<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| INT | DOUBLE | CHAR | STRING | LPAR **expr** RPAR<br><br>

### Handling left recursion

**Formula**<br>
```
A: A α_1 | … | A α_m | β_1 | … | βn A: β_1 A’ | … | β_n A’
A’: α_1 A’ | … | α_m A’ | ε
```

**exprOr**: **exprAnd** **exprOr_**<br>
**_exprOr**: OR **exprAnd** **_exprOr** | ε

**exprAnd**: **exprEq** **_exprAnd**<br>
**_exprAnd** = AND **exprEq** **_exprAnd** | ε

**exprEq**: **exprRel** **_exprEq**<br>
**_exprEq**: ( EQUAL | NOTEQ) **exprRel** **_exprEq** | ε

**exprRel** = **exprAdd** **_exprRel**<br>
**_exprRel**: ( LESS | LESSEQ | GREATER | GREATEREQ ) **exprAdd** **_exprRel** | ε

**exprAdd**: **exprMul** **_exprAdd**<br>
**_exprAdd**: ( ADD | SUB ) **exprMul** **_exprAdd** | ε

**exprMul**: **exprCast** **_exprMul**<br>
**_exprMul**: ( MUL | DIV ) **exprCast** **_exprMul** | ε

**exprPostfix**: **exprPrimary** **_exprPostfix**<br>
**_exprPostfix**: LBRACKET **expr** RBRACKET **_exprPostfix**<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| DOT ID **_exprPostfix**<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| ε