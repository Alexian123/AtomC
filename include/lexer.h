#ifndef __LEXER_H__
#define __LEXER_H__

#include <stdio.h>

// Token codes
enum 
{
	// identifiers
	ID = 0,

	// keywords
	TYPE_INT, TYPE_CHAR, TYPE_DOUBLE, IF, ELSE, WHILE, VOID, RETURN, STRUCT,

	// constants
	INT, DOUBLE, CHAR, STRING,

	// delimiters
	COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC, END,

	// operators
	ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ
};

typedef struct _Token
{
	int code;		// ID, TYPE_CHAR, ...
	int line;		// the line from the input file
	union {
		char *text;		// the chars for ID, STRING (dynamically allocated)
		int i;			// the value for INT
		char c;			// the value for CHAR
		double d;		// the value for DOUBLE
	};
	struct _Token *next;		// next token in a simple linked list
} Token;

extern Token *tokenize(const char *pch);
extern void showTokens(const Token *tokens, FILE *stream);
extern void freeTokens(Token *tokens);

#endif