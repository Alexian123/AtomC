#include "parser.h"

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
		tkerr("syntax error");
	}
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
	Token *start = iTk;
	if (consume(STRUCT)) {
		if (consume(ID)) {
			if (consume(LACC)) {
				for (;;) {
					if (vardef()) {}
					else break;
				}
				if (consume(RACC)) {
					if (consume(SEMICOLON)) {
						return true;
					}
				}
			}
		}
	}
	iTk = start;
	return false;
}

static bool varDef() {
	Token *start = iTk;
	if (typeBase()) {
		if (consume(ID)) {
			if (arrayDecl()) {}
			if (consume(SEMICOLON)) {
				return true;
			}
		}
	}
	iTk = start;
	return false;
}

bool typeBase() {
	Token *start = iTk;
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
		}
	}
	iTk = start;
	return false;
}

bool arrayDecl() {
	Token *start = iTk;
	if (consume(LBRACKET)) {
		if (consume(INT)) {}
		if (consume(RBRACKET)) {
			return true;
		}
	}
	iTk = start;
	return false;
}

bool fnDef() {
	Token *start = iTk;
	if (typeBase() || consume(VOID)) {
		if (consume(ID)) {
			if (consume(LPAR)) {
				if (fnParam()) {
					while (consume(COMMA)) {
						if (fnParam()) {}
						else tkerr("Missing additional function parameter after comma");
					}
				}
				if (consume(RPAR)) {
					if (stmCompound) {
						return true;
					}
				}
			}
		}
	}
	iTk = start;
	return false;
}

bool fnParam() {
	Token *start = iTk;
	if (typeBase()) {
		if (consume(ID)) {
			if (arrayDecl()) {}
			return true;
		}
	}
	iTk = start;
	return false;
}

bool stm() {
	Token *start = iTk;
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
							else tkerr("Empty else statement");
						}
						return true;
					}
				}
			}
		}
	}
	if (consume(WHILE)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (stm()) {
						return true;
					}
				}
			}
		}
	}
	if (consume(RETURN)) {
		if (expr()) {}
		if (consume(SEMICOLON)) {
			return true;
		}
	}
	if (expr()) {}
	if (consume(SEMICOLON)) {
		return true;
	}
	iTk = start;
	return false;
}

bool stmCompound() {
	Token *start = iTk;
	if (consume(LACC)) {
		while (varDef() || stm());
		if (consume(RACC)) {
			return true;
		}
	}
	iTk = start;
	return false;
}

bool expr() {
	return exprAssign();
}

bool exprAssign() {
	Token *start = iTk;
	if (exprUnary()) {
		if (consume(ASSIGN)) {
			if (exprAssign()) {
				return true;
			}
		}	
	}
	if (exprOr()) {
		return true;
	}
	iTk = start;
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
	Token *start = iTk;
	if (consume(OR)) {
		if (exprAnd()) {
			if (_exprOr()) {
				return true;
			}
		}
	}
	iTk = start;
	return true;
}

bool exprAnd() {

	return false;
}

bool _exprAnd() {

	return false;
}

bool exprEq() {

	return false;
}

bool _exprEq() {

	return false;
}

bool exprRel() {

	return false;
}

bool _exprRel() {

	return false;
}

bool exprAdd() {

	return false;
}

bool _exprAdd() {

	return false;
}

bool exprMul() {

	return false;
}

bool _exprMul() {

	return false;
}

bool exprCast() {

	return false;
}

bool exprUnary() {

	return false;
}

bool exprPostfix() {

	return false;
}

bool _exprPostfix() {

	return false;
}

bool exprPrimary() {
	return false;
}