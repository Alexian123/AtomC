#include "parser.h"
#include "utils.h"
#include "ad.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

static Token *iTk;				// the iterator in the tokens list
static Token *consumedTk;		// the last consumed token
static Symbol *owner = NULL;	// the symbol we are inside of at a given time

static void tkerr(const char *fmt, ...);
static bool consume(int code);

static bool unit();
static bool structDef();
static bool varDef();
static bool typeBase(Type *t);
static bool arrayDecl(Type *t);
static bool fnDef();
static bool fnParam();
static bool stm();
static bool stmCompound(bool newDomain);
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
	fprintf(stderr, "Error at line %d: ", iTk->line);
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
			Token *tkName = consumedTk;
			if (consume(LACC)) {
				Symbol *s = findSymbolInDomain(symTable, tkName->text);
				if (s) tkerr("Symbol redefinition: %s", tkName->text);
				s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
				s->type.tb = TB_STRUCT;
				s->type.s = s;
				s->type.n = -1;
				pushDomain();
				owner = s;
				for (;;) {
					if (varDef()) {}
					else break;
				}
				if (consume(RACC)) {
					if (consume(SEMICOLON)) {
						owner = NULL;
						dropDomain();
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
	Type t;
	if (typeBase(&t)) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
			if (arrayDecl(&t)) {
				if (t.n == 0) tkerr("An array must must have a specified dimension");
			}
			if (consume(SEMICOLON)) {
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
						default:  // not needed, stops unnecessary warnings
							break;
					}
				} else {
					var->varMem = safeAlloc(typeSize(&t));
				}
				return true;
			} else tkerr("Missing semicolon after variable declaration");
		} else tkerr("Missing/invalid identifier after base type");
	}
	iTk = tk;
	return false;
}

bool typeBase(Type *t) {
	t->n = -1;
	if (consume(TYPE_INT)) {
		t->tb = TB_INT;
		return true;
	}
	if (consume(TYPE_DOUBLE)) {
		t->tb = TB_DOUBLE;
		return true;
	}
	if(consume(TYPE_CHAR)) {
		t->tb = TB_CHAR;
		return true;
	}
	if (consume(STRUCT)) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
			t->tb = TB_STRUCT;
			t->s = findSymbol(tkName->text);
			if (!t->s) tkerr("Undefined struct: %s", tkName->text);
			return true;
		} else tkerr("Missing struct identifier");
	}
	return false;
}

bool arrayDecl(Type *t) {
	if (consume(LBRACKET)) {
		if (consume(INT)) {
			Token *tkSize = consumedTk;
			t->n = tkSize->i;
		} else {
			t->n = 0;
		}
		if (consume(RBRACKET)) {
			return true;
		} else tkerr("Missing \"]\" for array declaration");
	}
	return false;
}

bool fnDef() {
	Token *tk = iTk;
	Type t;
	bool consumedVoidTk = false;
	if (typeBase(&t) || (consumedVoidTk = consume(VOID))) {
		if (consumedVoidTk) {
			t.tb = TB_VOID;
		}
		if (consume(ID)) {
			Token *tkName = consumedTk;
			if (consume(LPAR)) {
				Symbol *fn = findSymbolInDomain(symTable, tkName->text);
				if (fn) tkerr("Symbol redefinition: %s", tkName->text);
				fn = newSymbol(tkName->text, SK_FN);
				fn->type = t;
				addSymbolToDomain(symTable, fn);
				owner = fn;
				pushDomain();
				if (fnParam()) {
					while (consume(COMMA)) {
						if (fnParam()) {}
						else tkerr("Missing/invalid additional function parameter after comma");
					}
				}
				if (consume(RPAR)) {
					if (stmCompound(false)) {
						dropDomain();
						owner = NULL;
						return true;
					} else tkerr("Missing function body in function definition");
				} else tkerr("Missing \")\" in fuction signature");
			}
		} else tkerr("Missing/invalid identifier after base type");
	}
	iTk = tk;
	return false;
}

bool fnParam() {
	Token *tk = iTk;
	Type t;
	if (typeBase(&t)) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
			if (arrayDecl(&t)) {
				t.n = 0;
			}
			Symbol *param = findSymbolInDomain(symTable, tkName->text);
			if (param) tkerr("Symbol redefinition: %s", tkName->text);
			param = newSymbol(tkName->text, SK_PARAM);
			param->type = t;
			param->owner = owner;
			param->paramIdx = symbolsLen(owner->fn.params);
			addSymbolToDomain(symTable, param);
			addSymbolToList(&owner->fn.params, dupSymbol(param));
			return true;
		} else tkerr("Missing/invalid identifier after base type");
	}
	iTk = tk;
	return false;
}

bool stm() {
	Token *tk = iTk;
	if (stmCompound(true)) {
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

bool stmCompound(bool newDomain) {
	if (consume(LACC)) {
		if (newDomain) {
			pushDomain();
		}
		while (varDef() || stm());
		if (consume(RACC)) {
			if (newDomain) {
				dropDomain();
			}
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
	if (consume(EQUAL)) {
		if (exprRel()) {
			if (_exprEq()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"==\" (EQUAL) operator");
	}
	if (consume(NOTEQ)) {
		if (exprRel()) {
			if (_exprEq()) {
				return true;
			}
		} else tkerr("Invalid/missing expression after \"!=\" (NOT EQUAL) operator");
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
		Type t;
		if (typeBase(&t)) {
			if (arrayDecl(&t)) {}
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
	if (consume(SUB)) {
		if (exprUnary()) {
			return true;
		} else tkerr("Invalid/missing expression after \"-\"");
	}
	if (consume(NOT)) {
		if (exprUnary()) {
			return true;
		} else tkerr("Invalid/missing expression after \"!\"");
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
					else tkerr("Missing/invalid expression after \",\"");
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