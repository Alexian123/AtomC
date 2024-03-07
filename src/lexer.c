#include "lexer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <regex.h>

// patterns for regular expressions
static const char PATTERNS[NUM_REGEX][MAX_PATTERN_LEN] = {
    "^(0|[1-9][0-9]*)",
    "^[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?",
    "^'([^'\\\\]|\\\\['nrt0\\\\])'",
    "^\"([^\"\\\\]|\\\\['\"nrt0\\\\])*\""
};

// must be in the same order as the Token codes
static const char TOKEN_NAMES[NUM_POSSIBLE_TOKENS][MAX_TOKEN_NAME_LEN] = {
	"ID",
	"TYPE_INT", "TYPE_CHAR", "TYPE_DOUBLE", "IF", "ELSE", "WHILE", "VOID", "RETURN", "STRUCT",
	"INT", "DOUBLE", "CHAR", "STRING",
	"COMMA", "SEMICOLON", "LPAR", "RPAR", "LBRACKET", "RBRACKET", "LACC", "RACC", "END",
	"ADD", "SUB", "MUL", "DIV", "DOT", "AND", "OR", "NOT", "ASSIGN", "EQUAL", "NOTEQ", "LESS", "LESSEQ", "GREATER", "GREATEREQ"
};

static regex_t regex[NUM_REGEX];

static Token *tokens;	// single linked list of tokens
static Token *lastTk;	// the last token in the list

static int line = 1;	// the current line in the input file

// initialize & deinitialize regular expressions
static void initRegex();
static void deinitRegex();

// adds a token to the end of the tokens list and returns it; sets its code and line
static Token *addTk(int code);

// extracts substring from begin to (end - 1)
static char *extract(const char *begin, const char *end);

void showTokens(const Token *tokens) {
	printf("LINE\tNAME:VALUE\n");
	for (const Token *tk = tokens; tk != NULL; tk = tk->next) {
		printf("%d\t\t%s", tk->line, TOKEN_NAMES[tk->code]);
		switch (tk->code) {
			case ID:
			case STRING:
				printf(":%s\n", tk->text);
				break;
			case INT:
				printf(":%d\n", tk->i);
				break;
			case DOUBLE:
				printf(":%.10f\n", tk->d);
				break;
			case CHAR:
				printf(":%d\n", tk->c);
				break;
			default:
				printf("\n");
		}
	}
	printf("\n");
}

Token *tokenize(const char *pch) {
	const char *start;
	char *text;
	Token *tk;
	regmatch_t match[1];
	initRegex();
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
			case '!': addTk(NOT); ++pch; break;
			case '.': addTk(DOT); ++pch; break;

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
					text = extract(start, pch);
					if (strcmp(text, "int") == 0) {
						addTk(TYPE_INT);
						free(text);
					} else if (strcmp(text, "char") == 0) {
						addTk(TYPE_CHAR);
						free(text);
					} else if (strcmp(text, "double") == 0) {
						addTk(TYPE_DOUBLE);
						free(text);
					} else if (strcmp(text, "if") == 0) {
						addTk(IF);
						free(text);
					} else if (strcmp(text, "else") == 0) {
						addTk(ELSE);
						free(text);
					} else if (strcmp(text, "while") == 0) {
						addTk(WHILE);
						free(text);
					} else if (strcmp(text, "void") == 0) {
						addTk(VOID);
						free(text);
					} else if (strcmp(text, "return") == 0) {
						addTk(RETURN);
						free(text);
					} else if (strcmp(text, "struct") == 0) {
						addTk(STRUCT);
						free(text);
					} else {	// non-keyword ID
						tk = addTk(ID);
						tk->text = text;
					}
				} else if (regexec(&regex[MATCH_INT], pch, 1, match, 0) == 0) { // numeric constant
					// constant may be a double so we must check for period or e/E
					char test_char = *(pch + match->rm_eo);
					regmatch_t temp_match[1];
					if (test_char == '.' || test_char == 'e' || test_char == 'E') {
						if (regexec(&regex[MATCH_DOUBLE], pch, 1, temp_match, 0) == 0) {	// double constant
							text = extract(pch, pch + temp_match->rm_eo);
							pch += temp_match->rm_eo;
							tk = addTk(DOUBLE);
							tk->d = atof(text);
							free(text);
						} else {
							err("Invalid DOUBLE constant on line %d\n", line);
						}
					} else {	// integer constant
						text = extract(pch, pch + match->rm_eo);
						pch += match->rm_eo;
						tk = addTk(INT);
						tk->i = atoi(text);
						free(text);
					}
				} else if (regexec(&regex[MATCH_CHAR], pch, 1, match, 0) == 0) {	// character constant
					text = extract(pch, pch + match->rm_eo);
					pch += match->rm_eo;
					tk = addTk(CHAR);
					if (strcmp(text, "'\\n'") == 0)			tk->c = '\n';
					else if (strcmp(text, "'\\r'") == 0)	tk->c = '\r';
					else if (strcmp(text, "'\\t'") == 0)	tk->c = '\t';
					else if (strcmp(text, "'\\0'") == 0)	tk->c = '\0';
					else									tk->c = text[1];
					free(text);
				} else if (regexec(&regex[MATCH_STRING], pch, 1, match, 0) == 0) {	// string constant
					text = extract(pch, pch + match->rm_eo);
					pch += match->rm_eo;
					tk = addTk(STRING);
					tk->text = extract(text + 1, text + strlen(text) - 1);
					free(text);	
				} else { // error
					err("invalid character on line %d: \'%c\' (ASCII: %d)", line, *pch, *pch);
				}
		}
	}
	deinitRegex();
}

void initRegex() {
	for (int i = 0; i < NUM_REGEX; ++i) {
		if (regcomp(&regex[i], PATTERNS[i], REG_EXTENDED) != 0) {
			err("Error compiling regex: \"%s\"\n", PATTERNS[i]);
		}
	}
}

void deinitRegex() {
	for (int i = 0; i < NUM_REGEX; ++i) {
		regfree(&regex[i]);
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
	int n = end - begin;
	char *substr = safeAlloc(n + 1);
	for (int i = 0; i < n; ++i) {
		substr[i] = begin[i];
	}
	substr[n] = '\0';
	return substr;
}