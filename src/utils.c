#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define NUM_POSSIBLE_TOKENS 38
#define MAX_TOKEN_NAME_LEN 16

// must be in the same order as the Token codes
static const char TOKEN_NAMES[NUM_POSSIBLE_TOKENS][MAX_TOKEN_NAME_LEN] = {
	"ID",
	"TYPE_INT", "TYPE_CHAR", "TYPE_DOUBLE", "IF", "ELSE", "WHILE", "VOID", "RETURN", "STRUCT",
	"INT", "DOUBLE", "CHAR", "STRING",
	"COMMA", "SEMICOLON", "LPAR", "RPAR", "LBRACKET", "RBRACKET", "LACC", "RACC", "END",
	"ADD", "SUB", "MUL", "DIV", "DOT", "AND", "OR", "NOT", "ASSIGN", "EQUAL", "NOTEQ", "LESS", "LESSEQ", "GREATER", "GREATEREQ"
};


void err(const char *fmt, ...) {
	fprintf(stderr, "Error: ");
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

void *safeAlloc(size_t nBytes) {
	void *p=malloc(nBytes);
	if (!p) {
		err("Not enough memory");
	}
	return p;
}

char *loadFile(const char *fileName) {
	FILE *fis = fopen(fileName, "rb");
	if (!fis) {
		err("Unable to open %s", fileName);
	}
	fseek(fis, 0, SEEK_END);
	size_t n = (size_t) ftell(fis);
	fseek(fis, 0, SEEK_SET);
	char *buf = (char*) safeAlloc((size_t) n + 1);
	size_t nRead = fread(buf, sizeof(char), (size_t) n, fis);
	fclose(fis);
	if (n != nRead) {
		err("Cannot read all the content of %s", fileName);
	}
	buf[n] = '\0';
	return buf;
}

extern const char *getTokenName(int code) {
	return TOKEN_NAMES[code];
}
