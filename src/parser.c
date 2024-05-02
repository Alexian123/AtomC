#include "parser.h"
#include "utils.h"
#include "ad.h"
#include "at.h"

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
static bool expr(Ret *r);
static bool exprAssign(Ret *r);
static bool exprOr(Ret *r);
static bool _exprOr(Ret *r);
static bool exprAnd(Ret *r);
static bool _exprAnd(Ret *r);
static bool exprEq(Ret *r);
static bool _exprEq(Ret *r);
static bool exprRel(Ret *r);
static bool _exprRel(Ret *r);
static bool exprAdd(Ret *r);
static bool _exprAdd(Ret *r);
static bool exprMul(Ret *r);
static bool _exprMul(Ret *r);
static bool exprCast(Ret *r);
static bool exprUnary(Ret *r);
static bool exprPostfix(Ret *r);
static bool _exprPostfix(Ret *r);
static bool exprPrimary(Ret *r);

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
	Ret rCond, rExpr;
	if (stmCompound(true)) {
		return true;
	}
	if (consume(IF)) {
		if (consume(LPAR)) {
			if (expr(&rCond)) {
				if (!canBeScalar(&rCond)) tkerr("The \"if\" condition must be a scalar value");
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
			if (expr(&rCond)) {
        		if (!canBeScalar(&rCond)) tkerr("The \"while\" condition must be a scalar value");
				if (consume(RPAR)) {
					if (stm()) {
						return true;
					} else tkerr("Missing \"while\" body");
				} else tkerr("Missing \")\" after \"while\" condition");
			} else tkerr("Missing \"while\" condition");
		} else tkerr("Missing \"(\" after \"while\" statement");
	}
	if (consume(RETURN)) {
		if (expr(&rExpr)) {
			if (owner->type.tb == TB_VOID) tkerr("A void function cannot return a value");
			if (!canBeScalar(&rExpr)) tkerr("The return value must be a scalar value");
			if (!convTo(&rExpr.type, &owner->type)) tkerr("Cannot convert the return expression type to the function return type");
			if (consume(SEMICOLON)) {
				return true;
			} else tkerr("Missing semicolon after \"return\" statement");
		}
        if (owner->type.tb != TB_VOID) tkerr("a non-void function must return a value");
		if (consume(SEMICOLON)) {
			return true;
		} else tkerr("Missing semicolon after \"return\" statement");
	}
	if (expr(&rExpr)) {}
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

bool expr(Ret *r) {
	return exprAssign(r);
}

bool exprAssign(Ret *r) {
	Token *tk = iTk;
	Ret rDst;
	if (exprUnary(&rDst)) {
		if (consume(ASSIGN)) {
			if (exprAssign(r)) {
				if (!rDst.lval) tkerr("The assignment destination must be a left-value");
				if (rDst.ct) tkerr("The assignment destination cannot be a constant");
				if (!canBeScalar(&rDst)) tkerr("The assignment destination must be a scalar");
				if (!canBeScalar(r)) tkerr("The assignment source must be a scalar");
				if (!convTo(&r->type, &rDst.type)) tkerr("The assignment source cannot be converted to the destination");
				r->lval = false;
				r->ct = true;
				return true;
			} else tkerr("Invalid/missing expression after \"=\" (assignment) operator");
		} 
	}
	iTk = tk;
	return exprOr(r);
}

bool exprOr(Ret *r) {
	if (exprAnd(r)) {
		return _exprOr(r);
	}
	return false;
}

bool _exprOr(Ret *r) {
	if (consume(OR)) {
		Ret right;
		if (exprAnd(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for \"||\" (LOGICAL OR)");
			*r = (Ret) { { TB_INT, NULL, -1 }, false, true };
			_exprOr(r);
			return true;
		} else tkerr("Invalid/missing expression after \"||\" (LOGICAL OR) operator");
	}
	return true;
}

bool exprAnd(Ret *r) {
	if (exprEq(r)) {
		_exprAnd(r);
		return true;
	}
	return false;
}

bool _exprAnd(Ret *r) {
	if (consume(AND)) {
		Ret right;
		if (exprEq(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for \"&&\" (LOGICAL AND)");
			*r = (Ret) { { TB_INT, NULL, -1 }, false, true};
			_exprAnd(r);
			return true;
		} else tkerr("Invalid/missing expression after \"&&\" (LOGICAL AND) operator");
	}
	return true;
}

bool exprEq(Ret *r) {
	if (exprRel(r)) {
		_exprEq(r);
		return true;
	}
	return false;
}

bool _exprEq(Ret *r) {
	if (consume(EQUAL)) {
		Ret right;
		if (exprRel(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for \"==\" (EQUAL)");
			*r = (Ret) { { TB_INT, NULL, -1 }, false, true };
			_exprEq(r);
			return true;
		} else tkerr("Invalid/missing expression after \"==\" (EQUAL) operator");
	}
	if (consume(NOTEQ)) {
		Ret right;
		if (exprRel(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for \"!=\" (NOT EQUAL)");
			*r = (Ret) { { TB_INT, NULL, -1 }, false, true };
			_exprEq(r);
			return true;
		} else tkerr("Invalid/missing expression after \"!=\" (NOT EQUAL) operator");
	}
	return true;
}

bool exprRel(Ret *r) {
	if (exprAdd(r)) {
		_exprRel(r);
		return true;
	}
	return false;
}

bool _exprRel(Ret *r) {
	if (consume(LESS)) {
		Ret right;
		if (exprAdd(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for \"<\" (LESS)");
			*r = (Ret) { { TB_INT, NULL, -1 }, false, true };
			_exprRel(r);
			return true;
		} else tkerr("Invalid/missing expression after \"<\" (LESS) operator");
	}
	if (consume(LESSEQ)) {
		Ret right;
		if (exprAdd(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for \"<=\" (LESS OR EQUAL)");
			*r = (Ret) { { TB_INT, NULL, -1 }, false, true };
			_exprRel(r);
			return true;
		} else tkerr("Invalid/missing expression after \"<=\" (LESS OR EQUAL) operator");
	}
	if (consume(GREATER)) {
		Ret right;
		if (exprAdd(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for \">\" (GREATER)");
			*r = (Ret) { { TB_INT, NULL, -1 }, false, true };
			_exprRel(r);
			return true;
		} else tkerr("Invalid/missing expression after \">\" (GREATER) operator");
	}
	if (consume(GREATEREQ)) {
		Ret right;
		if (exprAdd(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for \">=\" (GREATER OR EQUAL)");
			*r = (Ret) { { TB_INT, NULL, -1 }, false, true };
			_exprRel(r);
			return true;
		} else tkerr("Invalid/missing expression after \">=\" (GREATER OR EQUAL) operator");
	}
	return true;
}

bool exprAdd(Ret *r) {
	if (exprMul(r)) {
		_exprAdd(r);
		return true;
	}
	return false;
}

bool _exprAdd(Ret *r) {
	if (consume(ADD)) {
		Ret right;
		if (exprMul(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for \"+\" (ADDITION) ");
			*r = (Ret) { tDst, false, true };
			_exprAdd(r);
			return true;
		} else tkerr("Invalid/missing expression after \"+\" (ADDITION) operator");
	}
	if (consume(SUB)) {
		Ret right;
		if (exprMul(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for \"-\" (SUBTRACTION)");
			*r = (Ret) { tDst, false, true };
			_exprAdd(r);
			return true;
		} else tkerr("Invalid/missing expression after \"-\" (SUBTRACTION) operator");
	}
	return true;
}

bool exprMul(Ret *r) {
	if (exprCast(r)) {
		_exprMul(r);
		return true;
	}
	return false;
}

bool _exprMul(Ret *r) {
	if (consume(MUL)) {
		Ret right;
		if (exprCast(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for \"*\" (MULTIPLICATION)");
			*r = (Ret) { tDst, false, true };
			_exprMul(r);
			return true;
		} else tkerr("Invalid/missing expression after \"*\" (MULTIPLICATION) operator");
	}
	if (consume(DIV)) {
		Ret right;
		if (exprCast(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("Invalid operand type for \"/\" (DIVISION)");
			*r = (Ret) { tDst, false, true };
			_exprMul(r);
			return true;
		} else tkerr("Invalid/missing expression after \"/\" (DIVISION) operator");
	}
	return true;
}

bool exprCast(Ret *r) {
	if (consume(LPAR)) {
		Type t;
		Ret op;
		if (typeBase(&t)) {
			if (arrayDecl(&t)) {}
			if (consume(RPAR)) {
				if (exprCast(&op)) {
					if (t.tb == TB_STRUCT) tkerr("Cannot convert to a struct type");
					if (op.type.tb == TB_STRUCT) tkerr("Cannot convert a struct");
					if (op.type.n >= 0 && t.n < 0) tkerr("An array can only be converted to another array");
					if (op.type.n < 0 && t.n >= 0) tkerr("A scalar can only be converted to another scalar");
					*r = (Ret) { t, false, true };
					return true;
				} else tkerr("Invalid/missing expression to be casted");
			} else tkerr("Missing \")\" after cast expression");
		} else tkerr("Invalid/missing cast type");
	}
	return exprUnary(r);
}

bool exprUnary(Ret *r) {
	if (consume(SUB)) {
		if (exprUnary(r)) {
			if (!canBeScalar(r)) tkerr("Unary \"-\" (MINUS) must have a scalar operand");
			r->lval = false;
			r->ct = true;
			return true;
		} else tkerr("Invalid/missing expression after \"-\" (MINUS)");
	}
	if (consume(NOT)) {
		if (exprUnary(r)) {
			if (!canBeScalar(r)) tkerr("Unary \"!\" (LOGICAL NOT) must have a scalar operand");
			r->lval = false;
			r->ct = true;
			return true;
		} else tkerr("Invalid/missing expression after \"!\" (LOGICAL NOT)");
	}
	return exprPostfix(r);
}

bool exprPostfix(Ret *r) {
	if (exprPrimary(r)) {
		_exprPostfix(r);
		return true;
	}
	return false;
}

bool _exprPostfix(Ret *r) {
	if (consume(LBRACKET)) {
		Ret idx;
		if (expr(&idx)) {
			if (consume(RBRACKET)) {
				if (r->type.n < 0) tkerr("Only an array can be indexed");
				Type tInt = { TB_INT, NULL, -1 };
				if (!convTo(&idx.type, &tInt)) tkerr("The array index is not convertible to int");
				r->type.n = -1;
				r->lval = true;
				r->ct = false;
				_exprPostfix(r);
				return true;
			} else tkerr("Missing \"]\" after expression");
		} else tkerr("Invalid/missing expression after \"[\"");
	}
	if (consume(DOT)) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
            if (r->type.tb != TB_STRUCT ) tkerr("A field can only be selected from a struct");
            Symbol *s = findSymbolInList(r->type.s->structMembers, tkName->text);
            if (!s) tkerr("The struct %s does not have a field %s", r->type.s->name, tkName->text);
            *r = (Ret) { s->type, true, s->type.n >= 0 };
			_exprPostfix(r);
			return true;
		} else tkerr("Invalid/missing identifier after \".\" (dot) operator");
	}
	return true;
}

bool exprPrimary(Ret *r) {
	if (consume(ID)) {
		Token *tkName = consumedTk;
        Symbol *s = findSymbol(tkName->text);
        if (!s) { tkerr("Undefined identifier: %s", tkName->text); }
		if (consume(LPAR)) {
			if (s->kind != SK_FN) tkerr("Only a function can be called");
			Ret rArg;
			Symbol *param = s->fn.params;
			if (expr(&rArg)) {
				if (!param) tkerr("Too many arguments in function call");
				if (!convTo(&rArg.type, &param->type)) tkerr("Cannot convert the argument type to the parameter type during function call");
				param = param->next;
				while (consume(COMMA)) {
					if (expr(&rArg)) {
						if (!param) tkerr("Too many arguments in function call");
						if (!convTo(&rArg.type,&param->type)) tkerr("Cannot convert the argument type to the parameter type during function call");
						param = param->next;
					} else tkerr("Missing/invalid expression after \",\"");
				}
			}
			if (consume(RPAR)) {
				if (param) tkerr("Too few arguments in function call");
				*r = (Ret) { s->type, false, true };
				return true;
			} else tkerr("Missing \")\" after expression");
		}
        if (s->kind == SK_FN) tkerr("A function can only be called");
        *r = (Ret) { s->type, true, s->type.n >= 0 };
		return true;
	}
	if (consume(INT)) {
		*r = (Ret) { { TB_INT, NULL, -1 }, false, true };
		return true;
	}
	if (consume(DOUBLE)) {
		*r = (Ret) { { TB_DOUBLE, NULL, -1 }, false, true };
		return true;
	}
	if (consume(CHAR)) {
		*r = (Ret) { { TB_CHAR, NULL, -1 }, false, true };
		return true;
	}
	if (consume(STRING)) {
		*r = (Ret) { { TB_CHAR, NULL, 0 }, false, true };
		return true;
	}
	if (consume(LPAR)) {
		if (expr(r)) {
			if (consume(RPAR)) {
				return true;
			} else tkerr("Missing \")\" after expression");
		}
	}
	return false;
}