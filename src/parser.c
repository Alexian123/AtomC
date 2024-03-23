#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

static Token *iTk;			// the iterator in the tokens list
static Token *consumedTk;	// the last consumed token

static void tkerr(const char *fmt, ...);
static bool consume(int code);

static bool typeBase();	// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
static bool unit();		// unit: ( structDef | fnDef | varDef )* END

void parse(Token *tokens) {
	iTk = tokens;
	if (!unit()) tkerr("syntax error");
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
		}
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