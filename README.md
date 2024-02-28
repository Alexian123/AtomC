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
INT: "^([0-9]|[1-9][0-9]*)$"
DOUBLE: "^([0-9]|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?$"
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
