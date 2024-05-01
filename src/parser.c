#include "parser.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

static Token *iTk;			// the iterator in the tokens list
static Token *consumedTk;	// the last consumed token

static void tkerr(const char *fmt, ...);
static bool consume(int code);

static bool unit();
static bool structDef();
static bool varDef();
static bool typeBase();
static bool arrayDecl();
static bool fnDef();
static bool fnParam();
static bool stm();
static bool stmCompound();
static bool expr();
static bool exprAssign();
static bool exprOr();
static bool _exprOr();
static bool exprAnd();
static bool _exprAnd();
static bool exprEq();
static bool _exprEq();
static bool exprRel();
static bool _exprRel();
static bool exprAdd();
static bool _exprAdd();
static bool exprMul();
static bool _exprMul();
static bool exprCast();
static bool exprUnary();
static bool exprPostfix();
static bool _exprPostfix();
static bool exprPrimary();

void parse(Token *tokens) {
	iTk = tokens;
	if (!unit()) {
		tkerr("Syntax error");
	}
	printf("Syntax ok\n");
}

void tkerr(const char *fmt, ...) {
	fprintf(stderr, "Error in line %d: ", iTk->line);
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

bool consume(int code) {
	if (iTk->code == code) {
		consumedTk = iTk;
		iTk = iTk->next;
		return true;
	}
	return false;
}

bool unit() {
	for (;;) {
		if (structDef()) {}
		else if (fnDef()) {}
		else if (varDef()) {}
		else break;
	}
	if (consume(END)) {
		return true;
	}
	return false;
}

bool structDef() {
	Token *tk = iTk;
	if (consume(STRUCT)) {
		if (consume(ID)) {
			if (consume(LACC)) {
				for (;;) {
					if (varDef()) {}
					else break;
				}
				if (consume(RACC)) {
					if (consume(SEMICOLON)) {
						return true;
					} else tkerr("Missing semicolon after struct declaration");
				} else tkerr("Missing \"}\" in struct declaration");
			}
		} else tkerr("Missing/invalid struct identifier");
	}
	iTk = tk;
	return false;
}

static bool varDef() {
	Token *tk = iTk;
	if (typeBase()) {
		if (consume(ID)) {
			if (arrayDecl()) {}
			if (consume(SEMICOLON)) {
				return true;
			} else tkerr("Missing semicolon after variable declaration");
		} else tkerr("Missing/invalid variable identifier after base type");
	}
	iTk = tk;
	return false;
}

bool typeBase() {
	if (consume(TYPE_INT)) {
		return true;
	}
	if (consume(TYPE_DOUBLE)) {
		return true;
	}
	if(consume(TYPE_CHAR)) {
		return true;
	}
	if (consume(STRUCT)) {
		if (consume(ID)) {
			return true;
		} else tkerr("Missing identifier after \"struct\" keyword");
	}
	return false;
}

bool arrayDecl() {
	if (consume(LBRACKET)) {
		if (consume(INT)) {}
		if (consume(RBRACKET)) {
			return true;
		} else tkerr("Missing \"]\" for array declaration");
	}
	return false;
}

bool fnDef() {
	Token *tk = iTk;
	if (typeBase() || consume(VOID)) {
		if (consume(ID)) {
			if (consume(LPAR)) {
				if (fnParam()) {
					while (consume(COMMA)) {
						if (fnParam()) {}
						else tkerr("Missing/invalid additional function parameter after comma");
					}
				}
				if (consume(RPAR)) {
					if (stmCompound()) {
						return true;
					} else tkerr("Missing function body in function definition");
				} else tkerr("Missing \")\" in fuction signature");
			}
		} else tkerr("Missing/invalid function identifier");
	}
	iTk = tk;
	return false;
}

bool fnParam() {
	Token *tk = iTk;
	if (typeBase()) {
		if (consume(ID)) {
			if (arrayDecl()) {}
			return true;
		}
	}
	iTk = tk;
	return false;
}

bool stm() {
	Token *tk = iTk;
	if (stmCompound()) {
		return true;
	}
	if (consume(IF)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (stm()) {
						if (consume(ELSE)) {
							if (stm()) {}
							else tkerr("Missing/invalid statement after \"else\" keyword");
						}
						return true;
					} else tkerr("Missing \"if\" statement body");
				} else tkerr("Missing \")\" after \"if\" condition");
			} else tkerr("Invalid/Missing \"if\" condition");
		} else tkerr("Missing \"(\" after \"if\" statement");
	}
	if (consume(WHILE)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (stm()) {
						return true;
					} else tkerr("Missing \"while\" body");
				} else tkerr("Missing \")\" after \"while\" condition");
			} else tkerr("Missing \"while\" condition");
		} else tkerr("Missing \"(\" after \"while\" statement");
	}
	if (consume(RETURN)) {
		if (expr()) {}
		if (consume(SEMICOLON)) {
			return true;
		} else tkerr("Missing semicolon after \"return\" statement");
	}
	if (expr()) {}
	if (consume(SEMICOLON)) {
		return true;
	}
	iTk = tk;
	return false;
}

bool stmCompound() {
	if (consume(LACC)) {
		while (varDef() || stm());
		if (consume(RACC)) {
			return true;
		} else tkerr("Missing \"}\"");
	}
	return false;
}

bool expr() {
	if (exprAssign()) {
		return true;
	}
	return false; 
}

bool exprAssign() {
	Token *tk = iTk;
	if (exprUnary()) {
		if (consume(ASSIGN)) {
			if (exprAssign()) {
				return true;
			} else tkerr("Invalid/missing expression after \"=\" (assignment) operator");
		} 
	}
	iTk = tk;
	if (exprOr()) {
		return true;
	}
	return false;
}

bool exprOr() {
	if (exprAnd()) {
		if (_exprOr()) {
			return true;
		}
	}
	return false;
}

bool _exprOr() {
	if (consume(OR)) {
		if (exprAnd()) {
			if (_exprOr()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"||\" (logical or) operator");
	}
	return true;
}

bool exprAnd() {
	if (exprEq()) {
		if (_exprAnd()) {
			return true;
		}
	}
	return false;
}

bool _exprAnd() {
	if (consume(AND)) {
		if (exprEq()) {
			if (_exprAnd()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"&&\" (logical and) operator");
	}
	return true;
}

bool exprEq() {
	if (exprRel()) {
		if (_exprEq()) {
			return true;
		}
	}
	return false;
}

bool _exprEq() {
	if (consume(EQUAL) || consume(NOTEQ)) {
		if (exprRel()) {
			if (_exprEq()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after equality operator");
	}
	return true;
}

bool exprRel() {
	if (exprAdd()) {
		if (_exprRel()) {
			return true;
		}
	}
	return false;
}

bool _exprRel() {
	if (consume(LESS)) {
		if (exprAdd()) {
			if (_exprRel()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"<\" (LESS) operator");
	}
	if (consume(LESSEQ)) {
		if (exprAdd()) {
			if (_exprRel()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"<=\" (LESS OR EQUAL) operator");
	}
	if (consume(GREATER)) {
		if (exprAdd()) {
			if (_exprRel()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \">\" (GREATER) operator");
	}
	if (consume(GREATEREQ)) {
		if (exprAdd()) {
			if (_exprRel()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \">=\" (GREATER OR EQUAL) operator");
	}
	return true;
}

bool exprAdd() {
	if (exprMul()) {
		if (_exprAdd()) {
			return true;
		}
	}
	return false;
}

bool _exprAdd() {
	if (consume(ADD)) {
		if (exprMul()) {
			if (_exprAdd()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"+\" (ADDITION) operator");
	}
	if (consume(SUB)) {
		if (exprMul()) {
			if (_exprAdd()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"-\" (SUBTRACTION) operator");
	}
	return true;
}

bool exprMul() {
	if (exprCast()) {
		if (_exprMul()) {
			return true;
		}
	}
	return false;
}

bool _exprMul() {
	if (consume(MUL)) {
		if (exprCast()) {
			if (_exprMul()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"*\" (MULTIPLICATION) operator");
	}
	if (consume(DIV)) {
		if (exprCast()) {
			if (_exprMul()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"/\" (DIVISION) operator");
	}
	return true;
}

bool exprCast() {
	if (consume(LPAR)) {
		if (typeBase()) {
			if (arrayDecl()) {}
			if (consume(RPAR)) {
				if (exprCast()) {
					return true;
				} else tkerr("Invalid/missing expression to be casted");
			} else tkerr("Missing \")\" after cast expression");
		} else tkerr("Invalid/missing cast type");
	}
	if (exprUnary()) {
		return true;
	}
	return false;
}

bool exprUnary() {
	if (consume(SUB) || consume(NOT)) {
		if (exprUnary()) {
			return true;
		} else tkerr("Invalid/missing expression after \"-\" / \"!\"");
	}
	if (exprPostfix()) {
		return true;
	}
	return false;
}

bool exprPostfix() {
	if (exprPrimary()) {
		if (_exprPostfix()) {
			return true;
		}
	}
	return false;
}

bool _exprPostfix() {
	if (consume(LBRACKET)) {
		if (expr()) {
			if (consume(RBRACKET)) {
				if (_exprPostfix()) {
					return true;
				} else tkerr("Invalid/missing expression after \"]\"");
			} else tkerr("Missing \"]\" after expression");
		} else tkerr("Invalid/missing expression after \"[\"");
	}
	if (consume(DOT)) {
		if (consume(ID)) {
			if (_exprPostfix()) {
				return true;
			}
		} else tkerr("Invalid/missing identifier after \".\" (dot) operator");
	}
	return true;
}

bool exprPrimary() {
	if (consume(ID)) {
		if (consume(LPAR)) {
			if (expr()) {
				while (consume(COMMA)) {
					if (expr()) {}
					else tkerr("Missing/invalid expression after comma");
				}
			}
			if (consume(RPAR)) {
			} else tkerr("Missing \")\" after expression");
		}
		return true;
	}
	if (consume(INT) || consume(DOUBLE) || consume(CHAR) || consume(STRING)) {
		return true;
	}
	if (consume(LPAR)) {
		if (expr()) {
			if (consume(RPAR)) {
				return true;
			} else tkerr("Missing \")\" after expression");
		} else tkerr("Invalid/missing expression after \"(\"");
	}
	return false;
}