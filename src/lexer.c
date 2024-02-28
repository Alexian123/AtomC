#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "utils.h"

#define MAX_TOKEN_NAME_LEN 16

// must be in the same order as the Token codes
static const char TOKEN_NAMES[NUM_POSSIBLE_TOKENS][MAX_TOKEN_NAME_LEN] = {
	"ID",
	"TYPE_INT", "TYPE_CHAR", "TYPE_DOUBLE", "IF", "ELSE", "WHILE", "VOID", "RETURN", "STRUCT",
	"COMMA", "SEMICOLON", "LPAR", "RPAR", "LBRACKET", "RBRACKET", "LACC", "RACC", "END",
	"ADD", "SUB", "MUL", "DIV", "DOT", "AND", "OR", "NOT", "ASSIGN", "EQUAL", "NOTEQ", "LESS", "LESSEQ", "GREATER", "GREATEREQ"
};

static Token *tokens;	// single linked list of tokens
static Token *lastTk;	// the last token in the list

static int line = 1;	// the current line in the input file

// adds a token to the end of the tokens list and returns it; sets its code and line
static Token *addTk(int code);

static char *extract(const char *begin, const char *end);

void showTokens(const Token *tokens) {
	for (const Token *tk = tokens; tk != NULL; tk = tk->next) {
		printf("%s ", TOKEN_NAMES[tk->code]);
	}
	printf("\n");
}

Token *tokenize(const char *pch) {
	const char *start;
	Token *tk;
	for (;;) {
		switch (*pch) {
			// delimiters
			case ',': 	addTk(COMMA); ++pch; break;
			case ';': 	addTk(SEMICOLON); ++pch; break;
			case '(': 	addTk(LPAR); ++pch; break;
			case ')': 	addTk(RPAR); ++pch; break;
			case '[': 	addTk(LBRACKET); ++pch; break;
			case ']': 	addTk(RBRACKET); ++pch; break;
			case '{': 	addTk(LACC); ++pch; break;
			case '}': 	addTk(RACC); ++pch; break;
			case '\0':	addTk(END);	return tokens;


			// operators
			case '+': addTk(ADD); ++pch; break;
			case '-': addTk(SUB); ++pch; break;
			case '*': addTk(MUL); ++pch; break;
			case '.': addTk(DOT); ++pch; break;
			case '!': addTk(NOT); ++pch; break;

			case '/':
				if (pch[1] == '/') { // sngle-line comment
					// ignore characters until newline or EOF
					for (pch += 2; *pch != '\r' && *pch != '\n' && *pch != '\0'; ++pch); 
				} else {
					addTk(DIV);
					++pch;
				}
				break;

			case '&':
				if (pch[1] == '&') {
					addTk(AND);
					pch += 2;
				} else {
					err("invalid char on line %d: \'%c\' (ASCII: %d)", line, *pch, *pch);
				}
				break;

			case '|':
				if (pch[1] == '|') {
					addTk(OR);
					pch += 2;
				} else {
					err("invalid char on line %d: \'%c\' (ASCII: %d)", line, *pch, *pch);
				}
				break;

			case '=':
				if (pch[1] == '=') {
					addTk(EQUAL);
					pch += 2;
				} else {
					addTk(ASSIGN);
					++pch;
				}
				break;

			case '<': 
				if (pch[1] == '=') {
					addTk(LESSEQ);
					pch += 2;
				} else {
					addTk(LESS); 
					++pch; 
				}
				break;

			case '>': 
				if (pch[1] == '=') {
					addTk(GREATEREQ);
					pch += 2;
				} else {
					addTk(GREATER); 
					++pch; 
				}
				break;


			// whitespace
			case '\r': if (pch[1] == '\n') ++pch;
			case '\n': ++line;
			case '\t':
			case ' ': ++pch; break;


			default:
				if (isalpha(*pch) || *pch == '_') { // identifiers
					for (start = pch++; isalnum(*pch) || *pch == '_'; ++pch);
					char *text = extract(start, pch);
					if (strcmp(text, TOKEN_NAMES[TYPE_INT]) == 0) {
						addTk(TYPE_INT);
					} else if (strcmp(text, TOKEN_NAMES[TYPE_CHAR]) == 0) {
						addTk(TYPE_CHAR);
					} else if (strcmp(text, TOKEN_NAMES[TYPE_DOUBLE]) == 0) {
						addTk(TYPE_DOUBLE);
					} else if (strcmp(text, TOKEN_NAMES[IF]) == 0) {
						addTk(IF);
					} else if (strcmp(text, TOKEN_NAMES[ELSE]) == 0) {
						addTk(ELSE);
					} else if (strcmp(text, TOKEN_NAMES[WHILE]) == 0) {
						addTk(WHILE);
					} else if (strcmp(text, TOKEN_NAMES[VOID]) == 0) {
						addTk(VOID);
					} else if (strcmp(text, TOKEN_NAMES[RETURN]) == 0) {
						addTk(RETURN);
					} else if (strcmp(text, TOKEN_NAMES[STRUCT]) == 0) {
						addTk(STRUCT);
					} else {	// non-keyword ID
						tk = addTk(ID);
						tk->text = text;
					}
				} else if (1) { // constants
				} else { // error
					err("invalid char on line %d: \'%c\' (ASCII: %d)", line, *pch, *pch);
				}
		}
	}
}

Token *addTk(int code) {
	Token *tk = safeAlloc(sizeof(Token));
	tk->code = code;
	tk->line = line;
	tk->next = NULL;
	if (lastTk != NULL) {
		lastTk->next = tk;
	} else {
		tokens = tk;
	}
	lastTk = tk;
	return tk;
}

char *extract(const char *begin, const char *end) {
	char *substr = safeAlloc(end - begin + 1);
	int idx = 0;
	for (; *begin != *end; ++begin) {
		substr[idx++] = *begin;
	}
	substr[idx] = '\0';
	return substr;
}