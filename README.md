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




## Domain Analysis

**structDef**
```c
// struct name must be unique in the domain
// inside the struct two or more variables cannot share the same name
structDef: STRUCT ID[tkName] LACC
    {
        Symbol *s = findSymbolInDomain(symTable, tkName->text);
        if (s) tkerr("Symbol redefinition: %s", tkName->text);
        s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
        s->type.tb = TB_STRUCT;
        s->type.s = s;
        s->type.n = -1;
        pushDomain();
        owner = s;
    }
    varDef* RACC SEMICOLON
    {
        owner = NULL;
        dropDomain();
    }
```
<br>

**varDef**
```c
// variable name must be unique in the domain
// arrays must have the given dimension (int v[] is not valid)
varDef: { Type t; } typeBase[&t] ID[tkName]
    ( arrayDecl[&t]
    { if (t.n == 0) tkerr("An array must have a specified dimension"); }
    )? SEMICOLON
    {
        Symbol *var = findSymbolInDomain(symTable, tkName->text);
        if (var) tkerr("Symbol redefinition: %s", tkName->text);
        var = newSymbol(tkName->text, SK_VAR);
        var->type = t;
        var->owner = owner;
        addSymbolToDomain(symTable, var);
        if (owner) {
            switch (owner->kind) {
                case SK_FN:
                    var->varIdx = symbolsLen(owner->fn.locals);
                    addSymbolToList(&owner->fn.locals, dupSymbol(var));
                    break;
                case SK_STRUCT:
                    var->varIdx = typeSize(&owner->type);
                    addSymbolToList(&owner->structMembers, dupSymbol(var));
                    break;
            }
        } else {
            var->varMem = safeAlloc(typeSize(&t));
        }
    }
```
<br>

**typeBase**
```c
// if the base type is a struct, it must have been defined earlier 
typeBase[out Type *t]: { t->n = -1; }
    (
    INT { t->tb=TB_INT; }
    | DOUBLE { t->tb=TB_DOUBLE; }
    | CHAR { t->tb=TB_CHAR; }
    | STRUCT ID[tkName]
    {
        t->tb = TB_STRUCT;
        t->s = findSymbol(tkName->text);
        if (!t->s) tkerr("Undefined struct: %s", tkName->text);
    }
    )
```
<br>

**arrayDecl**
```c
arrayDecl[inout Type *t]: LBRACKET
    ( INT[tkSize] { t->n = tkSize->i; } | { t->n = 0; } )
    RBRACKET
```
<br>

**fnDef**
```c
// function name must be unique in the domain
// function local domain starts immediately after LPAR
// function body {...} does not define a new subdomain inside the function local domain
fnDef: { Type t; }
    ( typeBase[&t] | VOID { t.tb = TB_VOID; } ) ID[tkName] LPAR
    {
        Symbol *fn = findSymbolInDomain(symTable, tkName->text);
        if (fn) tkerr("Symbol redefinition: %s", tkName->text);
        fn = newSymbol(tkName->text, SK_FN);
        fn->type = t;
        addSymbolToDomain(symTable, fn);
        owner = fn;
        pushDomain();
    }
    ( fnParam ( COMMA fnParam )* )? RPAR stmCompound[false]
    {
        dropDomain();
        owner = NULL;
    }
```
<br>

**fnParam**
```c
// parameter name must be unique in the domain
// parameters can be arrays with a given dimension, but in this case the dimension will be deleterd (int v[10] -> int v[])
fnParam: { Type t; } typeBase[&t] ID[tkName]
    (arrayDecl[&t] { t.n = 0; } )?
    {
        Symbol *param = findSymbolInDomain(symTable, tkName->text);
        if (param) tkerr("symbol redefinition: %s", tkName->text);
        param = newSymbol(tkName->text, SK_PARAM);
        param->type = t;
        param->owner = owner;
        param->paramIdx = symbolsLen(owner->fn.params);
        // the parameter is added in the current domain as well as in the function parameters
        addSymbolToDomain(symTable, param);
        addSymbolToList(&owner->fn.params, dupSymbol(param));
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
    { if (newDomain) pushDomain(); }
    ( varDef | stm )* RACC
    { if (newDomain) dropDomain(); }
```
<br>

**exprCast**
```c
// an argument is added because it is required by typeBase and arrayDecl
// t will be used later
exprCast: LPAR { Type t; } typeBase[&t] arrayDecl[&t]? RPAR exprCast | exprUnary
```
<br>



## Type Analysis

**stm**
```c
// IF - condition must be a scalar
// WHILE - condition must be a scalar
// RETURN - expression must be a scalar
// RETURN - void functions cannot return a value
// RETURN - non-void functions must return an expression of a type which is convertible to the return type
stm: { Ret rCond,rExpr; } stmCompound[true]
    | IF LPAR expr[&rCond]
        { if (!canBeScalar(&rCond)) tkerr("The \"if\" condition must be a scalar value"); }
        RPAR stm ( ELSE stm )?
    | WHILE LPAR expr[&rCond]
        { if (!canBeScalar(&rCond)) tkerr("The \"while\" condition must be a scalar value"); }
        RPAR stm
    | RETURN ( expr[&rExpr]?
        {
            if (owner->type.tb == TB_VOID) tkerr("A void function cannot return a value");
            if (!canBeScalar(&rExpr)) tkerr("The return value must be a scalar value");
            if (!convTo(&rExpr.type, &owner->type)) tkerr("Cannot convert the return expression type to the function return type");
        }
        |
        { if (owner->type.tb != TB_VOID) tkerr("a non-void function must return a value"); }
        ) SEMICOLON
    | expr[&rExpr]? SEMICOLON
```
<br>

**expr**
```c
expr[out Ret *r]: exprAssign[r]
```
<br>

**exprAssign**
```c
// Destination must be lval
// Destination cannot be a constant
// Both operands must be scalars
// Source must be convertible to destination
// Resulted type must be the source type
exprAssign[out Ret *r]: { Ret rDst; } exprUnary[&rDst] ASSIGN exprAssign[r]
    {
        if (!rDst.lval) tkerr("The assignment destination must be a left-value");
        if (rDst.ct) tkerr("The assignment destination cannot be a constant");
        if (!canBeScalar(&rDst)) tkerr("The assignment destination must be a scalar");
        if (!canBeScalar(r)) tkerr("The assignment source must be a scalar");
        if (!convTo(&r->type, &rDst.type)) tkerr("The assignment source cannot be converted to the destination");
        r->lval = false;
        r->ct = true;
    }
    | exprOr[r]
```
<br>

**exprOr**
```c
// Both operands must be scalars and not structs
// Result must be an int
exprOr[out Ret *r]: exprOr[r] OR { Ret right; } exprAnd[&right]
    {
        Type tDst;
        if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for ||");
        *r = (Ret) { { TB_INT, NULL, -1 }, false, true };
    }
    | exprAnd[r]
```
<br>

**exprAnd**
```c
// Both operands must be scalars and not structs
// Result must be an int
exprAnd[out Ret *r]: exprAnd[r] AND { Ret right; } exprEq[&right]
    {
        Type tDst;
        if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for &&");
        *r = (Ret) { { TB_INT, NULL, -1 }, false, true};
    }
    | exprEq[r]
```
<br>


**exprEq**
```c
// Both operands must be scalars and not structs
// Result must be an int
exprEq[out Ret *r]: exprEq[r] ( EQUAL | NOTEQ ) { Ret right; } exprRel[&right]
    {
        Type tDst;
        if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for == or !=");
        *r = (Ret) { { TB_INT, NULL, -1 }, false, true };
    }
    | exprRel[r]
```
<br>

**exprRel**
```c
// Both operands must be scalars and not structs
// Result must be an int
exprRel[out Ret *r]: exprRel[r] ( LESS | LESSEQ | GREATER | GREATEREQ ) { Ret right; }
exprAdd[&right]
    {
        Type tDst;
        if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for <, <=, >, >=");
        *r = (Ret) { { TB_INT, NULL, -1 }, false, true };
    }
    | exprAdd[r]
```
<br>

**exprAdd**
```c
// Both operands must be scalars and not structs
exprAdd[out Ret *r]: exprAdd[r] ( ADD | SUB ) { Ret right; } exprMul[&right]
    {
        Type tDst;
        if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for + or -");
        *r = (Ret) { tDst, false, true };
    }
    | exprMul[r]
```
<br>

**exprMul**
```c
// Both operands must be scalars and not structs
exprMul[out Ret *r]: exprMul[r] ( MUL | DIV ) { Ret right; } exprCast[&right]
    {
        Type tDst;
        if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for * or /");
        *r = (Ret) { tDst, false, true };
    }
    | exprCast[r]
```
<br>

**exprCast**
```c
// Struct cannot be converted
// The type to convert to cannot be a struct
// An array can only be converted to another array
// A scalar can only be converted to another scalar
exprCast[out Ret *r]: LPAR { Type t;Ret op; } typeBase[&t] arrayDecl[&t]? RPAR
exprCast[&op]
    {
        if (t.tb == TB_STRUCT) tkerr("Cannot convert to a struct type");
        if (op.type.tb == TB_STRUCT) tkerr("Cannot convert a struct");
        if (op.type.n >= 0 && t.n < 0) tkerr("An array can only be converted to another array");
        if (op.type.n < 0 && t.n >= 0) tkerr("A scalar can only be converted to another scalar");
        *r = (Ret) { t, false, true };
    }
    | exprUnary[r]
```
<br>

**exprUnary**
```c
// Minus and Not must have a scalar operand
// Result of Not is an int
exprUnary[out Ret *r]: ( SUB | NOT ) exprUnary[r]
    {
        if (!canBeScalar(r)) tkerr("Unary - or ! must have a scalar operand");
        r->lval = false;
        r->ct = true;
    }
    | exprPostfix[r]
```
<br>


**exprPostfix**
```c
// Only an array can be indexed
// Array index must be convertible to int
// Dot operator is aplicable only to structs
// Struct field must exist
exprPostfix[out Ret *r]: exprPostfix[r] LBRACKET { Ret idx; } expr[&idx] RBRACKET
        {
            if (r->type.n < 0) tkerr("Only an array can be indexed");
            Type tInt = { TB_INT, NULL, -1 };
            if (!convTo(&idx.type, &tInt)) tkerr("The index is not convertible to int");
            r->type.n = -1;
            r->lval = true;
            r->ct = false;
        }
    | exprPostfix[r] DOT ID[tkName]
        {
            if (r->type.tb != TB_STRUCT ) tkerr("A field can only be selected from a struct");
            Symbol *s = findSymbolInList(r->type.s->structMembers, tkName->text);
            if (!s) tkerr("The struct %s does not have a field %s", r->type.s->name, tkName->text);
            *r = (Ret) { s->type, true, s->type.n >= 0 };
        }
    | exprPrimary[r]
```
<br>

**exprPrimary**
```c
// ID must exist in the table of symbols
// Only functions can be called
// A function can only be called
// A function call must have the same number of arguments as parameters in its definition
// Argument types from a function call must be convertible to the function's parameter types
exprPrimary[out Ret *r]: ID[tkName]
    {
        Symbol *s = findSymbol(tkName->text);
        if (!s) tkerr("Undefined identifier: %s",tkName->text);
    }
    ( LPAR
    {
        if (s->kind != SK_FN) tkerr("Only a function can be called");
        Ret rArg;
        Symbol *param = s->fn.params;
    }
    ( expr[&rArg]
    {
        if (!param) tkerr("Too many arguments in function call");
        if (!convTo(&rArg.type, &param->type)) tkerr("Cannot convert the argument type to the parameter type during function call");
        param = param->next;
    }
    ( COMMA expr[&rArg]
    {
        if (!param) tkerr("Too many arguments in function call");
        if (!convTo(&rArg.type,&param->type)) tkerr("Cannot convert the argument type to the parameter type during function call");
        param = param->next;
    }
    )* )? RPAR
    {
        if (param) tkerr("Too few arguments in function call");
        *r = (Ret) { s->type, false, true };
    }
    |
    {
        if (s->kind == SK_FN) tkerr("A function can only be called");
        *r = (Ret) { s->type, true, s->type.n >= 0 };
    }
    )
    | INT       { *r = (Ret) { { TB_INT,    NULL, -1 }, false, true }; }
    | DOUBLE    { *r = (Ret) { { TB_DOUBLE, NULL, -1 }, false, true }; }
    | CHAR      { *r = (Ret) { { TB_CHAR,   NULL, -1 }, false, true }; }
    | STRING    { *r = (Ret) { { TB_CHAR,   NULL,  0 }, false, true }; }
    | LPAR expr[r] RPAR
```
<br>