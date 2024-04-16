#include "parser.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

// enable debugging info
//#define DEBUG_MODE

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
		#ifdef DEBUG_MODE	// print consumed token name for debugging purposes
				printf("Consumed %s on line %d\n", getTokenName(consumedTk->code), consumedTk->line);
		#endif
		iTk = iTk->next;
		return true;
	}
	return false;
}

bool unit() {
	#ifdef DEBUG_MODE	
		printf("unit\n");
	#endif
	for (;;) {
		if (structDef()) {}
		else if (fnDef()) {}
		else if (varDef()) {}
		else break;
	}
	if (consume(END)) {
		#ifdef DEBUG_MODE	
			printf("unit returns true\n");
		#endif
		return true;
	}
	#ifdef DEBUG_MODE	
		printf("unit returns false\n");
	#endif
	return false;
}

bool structDef() {
	#ifdef DEBUG_MODE	
		printf("structDef\n");
	#endif
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
						#ifdef DEBUG_MODE	
							printf("structDef returns true\n");
						#endif
					} else tkerr("Missing semicolon after struct declaration");
				} else tkerr("Missing \"}\" in struct declaration");
			}
		}
	}
	iTk = tk;
	#ifdef DEBUG_MODE	
		printf("structDef returns false\n");
	#endif
	return false;
}

static bool varDef() {
	#ifdef DEBUG_MODE	
		printf("varDef\n");
	#endif
	Token *tk = iTk;
	if (typeBase()) {
		if (consume(ID)) {
			if (arrayDecl()) {}
			if (consume(SEMICOLON)) {
				#ifdef DEBUG_MODE	
					printf("varDef returns true\n");
				#endif
				return true;
			} else tkerr("Missing semicolon after variable declaration");
		}
	}
	iTk = tk;
	#ifdef DEBUG_MODE	
		printf("varDef returns false\n");
	#endif
	return false;
}

bool typeBase() {
	#ifdef DEBUG_MODE	
		printf("typeBase\n");
	#endif
	if (consume(TYPE_INT)) {
		#ifdef DEBUG_MODE	
			printf("typeBase returns true\n");
		#endif
		return true;
	}
	if (consume(TYPE_DOUBLE)) {
		#ifdef DEBUG_MODE	
			printf("typeBase returns true\n");
		#endif
		return true;
	}
	if(consume(TYPE_CHAR)) {
		#ifdef DEBUG_MODE	
			printf("typeBase returns true\n");
		#endif
		return true;
	}
	if (consume(STRUCT)) {
		if (consume(ID)) {
			#ifdef DEBUG_MODE	
				printf("typeBase returns true\n");
			#endif
			return true;
		} else tkerr("Missing identifier after \"struct\" keyword");
	}
	#ifdef DEBUG_MODE	
		printf("typeBase returns false\n");
	#endif
	return false;
}

bool arrayDecl() {
	#ifdef DEBUG_MODE	
		printf("arrayDecl\n");
	#endif
	if (consume(LBRACKET)) {
		if (consume(INT)) {}
		if (consume(RBRACKET)) {
			#ifdef DEBUG_MODE	
				printf("arrayDecl returns true\n");
			#endif
			return true;
		} else tkerr("Missing \"]\" for array declaration");
	}
	#ifdef DEBUG_MODE	
		printf("arrayDecl returns false\n");
	#endif
	return false;
}

bool fnDef() {
	#ifdef DEBUG_MODE	
		printf("fnDef\n");
	#endif
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
						#ifdef DEBUG_MODE	
							printf("fnDef returns true\n");
						#endif
						return true;
					} else tkerr("Missing function body in function definition");
				} else tkerr("Missing \")\" in fuction signature");
			}
		} 
	}
	iTk = tk;
	#ifdef DEBUG_MODE	
		printf("fnDef returns false\n");
	#endif
	return false;
}

bool fnParam() {
	#ifdef DEBUG_MODE	
		printf("fnParam\n");
	#endif
	Token *tk = iTk;
	if (typeBase()) {
		if (consume(ID)) {
			if (arrayDecl()) {}
				#ifdef DEBUG_MODE	
					printf("fnParam returns true\n");
				#endif
			return true;
		}
	}
	iTk = tk;
	#ifdef DEBUG_MODE	
		printf("fnParam returns false\n");
	#endif
	return false;
}

bool stm() {
	#ifdef DEBUG_MODE	
		printf("stm\n");
	#endif
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
						#ifdef DEBUG_MODE	
							printf("stm returns true\n");
						#endif
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
						#ifdef DEBUG_MODE	
							printf("stm returns true\n");
						#endif
						return true;
					} else tkerr("Missing \"while\" body");
				} else tkerr("Missing \")\" after \"while\" condition");
			} else tkerr("Missing \"while\" condition");
		} else tkerr("Missing \"(\" after \"while\" statement");
	}
	if (consume(RETURN)) {
		if (expr()) {}
		if (consume(SEMICOLON)) {
			#ifdef DEBUG_MODE	
				printf("stm returns true\n");
			#endif
			return true;
		} else tkerr("Missing semicolon after \"return\" statement");
	}
	if (expr()) {}
	if (consume(SEMICOLON)) {
		#ifdef DEBUG_MODE	
			printf("stm returns true\n");
		#endif
		return true;
	}
	iTk = tk;
	#ifdef DEBUG_MODE	
		printf("stm returns false\n");
	#endif
	return false;
}

bool stmCompound() {
	#ifdef DEBUG_MODE	
		printf("stmCompound\n");
	#endif
	if (consume(LACC)) {
		while (varDef() || stm());
		if (consume(RACC)) {
			#ifdef DEBUG_MODE	
				printf("stmCompound returns true\n");
			#endif
			return true;
		} else tkerr("Missing \"}\"");
	}
	#ifdef DEBUG_MODE	
		printf("stmCompound returns false\n");
	#endif
	return false;
}

bool expr() {
	#ifdef DEBUG_MODE	
		printf("expr\n");
	#endif
	if (exprAssign()) {
		#ifdef DEBUG_MODE	
			printf("expr returns true\n");
		#endif
		return true;
	}
	#ifdef DEBUG_MODE	
		printf("expr returns false\n");
	#endif
	return false; 
}

bool exprAssign() {
	#ifdef DEBUG_MODE	
		printf("exprAssign\n");
	#endif
	Token *tk = iTk;
	if (exprUnary()) {
		if (consume(ASSIGN)) {
			if (exprAssign()) {
				#ifdef DEBUG_MODE	
					printf("exprAssign returns true\n");
				#endif
				return true;
			} else tkerr("Invalid/missing expression after \"=\" (assignment) operator");
		} 
	}
	iTk = tk;
	if (exprOr()) {
		#ifdef DEBUG_MODE	
			printf("exprAssign returns true\n");
		#endif
		return true;
	}
	#ifdef DEBUG_MODE	
		printf("exprAssign returns false\n");
	#endif
	return false;
}

bool exprOr() {
	#ifdef DEBUG_MODE	
		printf("exprOr\n");
	#endif
	if (exprAnd()) {
		if (_exprOr()) {
			#ifdef DEBUG_MODE	
				printf("exprOr returns true\n");
			#endif
			return true;
		}
	}
	#ifdef DEBUG_MODE	
		printf("exprOr returns false\n");
	#endif
	return false;
}

bool _exprOr() {
	#ifdef DEBUG_MODE	
		printf("_exprOr\n");
	#endif
	if (consume(OR)) {
		if (exprAnd()) {
			if (_exprOr()) {
				#ifdef DEBUG_MODE	
					printf("_exprOr returns true\n");
				#endif
				return true;
			}
		} else tkerr("Invalid/missing expression after \"||\" (logical or) operator");
	}
	#ifdef DEBUG_MODE	
		printf("_exprOr returns true\n");
	#endif
	return true;
}

bool exprAnd() {
	#ifdef DEBUG_MODE	
		printf("exprAnd\n");
	#endif
	if (exprEq()) {
		if (_exprAnd()) {
			#ifdef DEBUG_MODE	
				printf("exprAnd returns true\n");
			#endif
			return true;
		}
	}
	#ifdef DEBUG_MODE	
		printf("exprAnd returns false\n");
	#endif
	return false;
}

bool _exprAnd() {
#ifdef DEBUG_MODE	
	printf("_exprAnd\n");
#endif
	if (consume(AND)) {
		if (exprEq()) {
			if (_exprAnd()) {
				#ifdef DEBUG_MODE	
					printf("_exprAnd returns true\n");
				#endif
				return true;
			}
		} else tkerr("Invalid/missing expression after \"&&\" (logical and) operator");
	}
	#ifdef DEBUG_MODE	
		printf("_exprAnd returns true\n");
	#endif
	return true;
}

bool exprEq() {
	#ifdef DEBUG_MODE	
		printf("exprEq\n");
	#endif
	if (exprRel()) {
		if (_exprEq()) {
			#ifdef DEBUG_MODE	
				printf("exprEq returns true\n");
			#endif
			return true;
		}
	}
	#ifdef DEBUG_MODE	
		printf("exprEq returns false\n");
	#endif
	return false;
}

bool _exprEq() {
	#ifdef DEBUG_MODE	
		printf("_exprEq\n");
	#endif
	if (consume(EQUAL) || consume(NOTEQ)) {
		if (exprRel()) {
			if (_exprEq()) {
				#ifdef DEBUG_MODE	
					printf("_exprEq returns true\n");
				#endif
				return true;
			}
		} else tkerr("Invalid/missing expression after equality operator");
	}
	#ifdef DEBUG_MODE	
		printf("_exprEq returns true\n");
	#endif
	return true;
}

bool exprRel() {
	#ifdef DEBUG_MODE	
		printf("exprRel\n");
	#endif
	if (exprAdd()) {
		if (_exprRel()) {
			#ifdef DEBUG_MODE	
				printf("exprRel returns true\n");
			#endif
			return true;
		}
	}
	#ifdef DEBUG_MODE	
		printf("exprRel returns false\n");
	#endif
	return false;
}

bool _exprRel() {
	#ifdef DEBUG_MODE	
		printf("_exprRel\n");
	#endif
	if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
		if (exprAdd()) {
			if (_exprRel()) {
				#ifdef DEBUG_MODE	
					printf("_exprRel returns true\n");
				#endif
				return true;
			}
		} else tkerr("Invalid/missing expression after relational operator");
	}
	#ifdef DEBUG_MODE	
		printf("_exprRel returns true\n");
	#endif
	return true;
}

bool exprAdd() {
	#ifdef DEBUG_MODE	
		printf("exprAdd\n");
	#endif
	if (exprMul()) {
		if (_exprAdd()) {
			#ifdef DEBUG_MODE	
				printf("exprAdd returns true\n");
			#endif
			return true;
		}
	}
	#ifdef DEBUG_MODE	
		printf("exprAdd returns false\n");
	#endif
	return false;
}

bool _exprAdd() {
	#ifdef DEBUG_MODE	
		printf("_exprAdd\n");
	#endif
	if (consume(ADD) || consume(SUB)) {
		if (exprMul()) {
			if (_exprAdd()) {
				#ifdef DEBUG_MODE	
					printf("_exprAdd returns true\n");
				#endif
				return true;
			}
		} else tkerr("Invalid/missing expression after addition/subtraction operator");
	}
	#ifdef DEBUG_MODE	
		printf("_exprAdd returns true\n");
	#endif
	return true;
}

bool exprMul() {
	#ifdef DEBUG_MODE	
		printf("exprMul\n");
	#endif
	if (exprCast()) {
		if (_exprMul()) {
			#ifdef DEBUG_MODE	
				printf("exprMul returns true\n");
			#endif
			return true;
		}
	}
	#ifdef DEBUG_MODE	
		printf("exprMul returns false\n");
	#endif
	return false;
}

bool _exprMul() {
	#ifdef DEBUG_MODE	
		printf("_exprMul\n");
	#endif
	if (consume(MUL) || consume(DIV)) {
		if (exprCast()) {
			if (_exprMul()) {
				#ifdef DEBUG_MODE	
					printf("_exprMul returns true\n");
				#endif
				return true;
			}
		} else tkerr("Invalid/missing expression after multiplication/division operator");
	}
	#ifdef DEBUG_MODE	
		printf("_exprMul returns true\n");
	#endif
	return true;
}

bool exprCast() {
	#ifdef DEBUG_MODE	
		printf("exprCast\n");
	#endif
	if (consume(LPAR)) {
		if (typeBase()) {
			if (arrayDecl()) {}
			if (consume(RPAR)) {
				if (exprCast()) {
					#ifdef DEBUG_MODE	
						printf("exprCast returns true\n");
					#endif
					return true;
				} else tkerr("Invalid/missing expression to be casted");
			} else tkerr("Missing \")\" after cast expression");
		} else tkerr("Invalid/missing cast type");
	}
	if (exprUnary()) {
		#ifdef DEBUG_MODE	
			printf("exprCast returns true\n");
		#endif
		return true;
	}
	#ifdef DEBUG_MODE	
		printf("exprCast returns false\n");
	#endif
	return false;
}

bool exprUnary() {
	#ifdef DEBUG_MODE	
		printf("exprUnary\n");
	#endif
	if (consume(SUB) || consume(NOT)) {
		if (exprUnary()) {
			#ifdef DEBUG_MODE	
				printf("exprUnary returns true\n");
			#endif
			return true;
		} else tkerr("Invalid/missing expression after \"-\" / \"!\"");
	}
	if (exprPostfix()) {
		#ifdef DEBUG_MODE	
			printf("exprUnary returns true\n");
		#endif
		return true;
	}
	#ifdef DEBUG_MODE	
		printf("exprUnary returns false\n");
	#endif
	return false;
}

bool exprPostfix() {
	#ifdef DEBUG_MODE	
		printf("exprPostfix\n");
	#endif
	if (exprPrimary()) {
		if (_exprPostfix()) {
			#ifdef DEBUG_MODE	
				printf("exprPostfix returns true\n");
			#endif
			return true;
		}
	}
	#ifdef DEBUG_MODE	
		printf("exprPostfix returns false\n");
	#endif
	return false;
}

bool _exprPostfix() {
	#ifdef DEBUG_MODE	
		printf("_exprPostfix\n");
	#endif
	if (consume(LBRACKET)) {
		if (expr()) {
			if (consume(RBRACKET)) {
				if (_exprPostfix()) {
					#ifdef DEBUG_MODE	
						printf("_exprPostfix returns true\n");
					#endif
					return true;
				} else tkerr("Invalid/missing expression after \"]\"");
			} else tkerr("Missing \"]\" after expression");
		} else tkerr("Invalid/missing expression after \"[\"");
	}
	if (consume(DOT)) {
		if (consume(ID)) {
			if (_exprPostfix()) {
				#ifdef DEBUG_MODE	
					printf("_exprPostfix returns true\n");
				#endif
				return true;
			}
		} else tkerr("Invalid/missing identifier after \".\" (dot) operator");
	}
	#ifdef DEBUG_MODE	
		printf("_exprPostfix returns true\n");
	#endif
	return true;
}

bool exprPrimary() {
	#ifdef DEBUG_MODE	
		printf("exprPrimary\n");
	#endif
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
		#ifdef DEBUG_MODE	
			printf("exprPrimary returns true\n");
		#endif
		return true;
	}
	if (consume(INT) || consume(DOUBLE) || consume(CHAR) || consume(STRING)) {
		#ifdef DEBUG_MODE	
			printf("exprPrimary returns true\n");
		#endif
		return true;
	}
	if (consume(LPAR)) {
		if (expr()) {
			if (consume(RPAR)) {
				#ifdef DEBUG_MODE	
					printf("exprPrimary returns true\n");
				#endif
				return true;
			} else tkerr("Missing \")\" after expression");
		} else tkerr("Invalid/missing expression after \"(\"");
	}
	#ifdef DEBUG_MODE	
		printf("exprPrimary return false\n");
	#endif
	return false;
}