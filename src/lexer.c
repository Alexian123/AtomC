#include "lexer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define NUM_POSSIBLE_TOKENS 38
#define MAX_TOKEN_NAME_LEN 16

// valid escape characters
static const char *ESCAPE_CHARS = "nrt\\\'\"";

// must be in the same order as the Token codes
static const char TOKEN_NAMES[NUM_POSSIBLE_TOKENS][MAX_TOKEN_NAME_LEN] = {
	"ID",
	"TYPE_INT", "TYPE_CHAR", "TYPE_DOUBLE", "IF", "ELSE", "WHILE", "VOID", "RETURN", "STRUCT",
	"INT", "DOUBLE", "CHAR", "STRING",
	"COMMA", "SEMICOLON", "LPAR", "RPAR", "LBRACKET", "RBRACKET", "LACC", "RACC", "END",
	"ADD", "SUB", "MUL", "DIV", "DOT", "AND", "OR", "NOT", "ASSIGN", "EQUAL", "NOTEQ", "LESS", "LESSEQ", "GREATER", "GREATEREQ"
};

static Token *tokens;	// single linked list of tokens
static Token *lastTk;	// the last token in the list

static int line = 1;	// the current line in the input file

// adds a token to the end of the tokens list and returns it; sets its code and line
static Token *addTk(int code);

// extracts substring from begin to (end - 1)
static char *extract(const char *begin, const char *end);

// returns a new string after parsing the escape characters from the input string
static char *parseEscapeChars(const char *input);

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
				if (tk->c == '\n') printf(":\\n\n");
				else if (tk->c == '\r') printf(":\\r\n");
				else if (tk->c =='\t') printf(":\\t\n");
				else printf(":%c\n", tk->c);
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
			case '.': 
				if (isalpha(pch[1])) {
					addTk(DOT); 
					++pch; 
					break;
				}
				err("Invalid character on line %d: \'%c\' (ASCII: %d)\nExpected an identifier before & after DOT operator", line, *pch, *pch);

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
					break;
				}
				err("Invalid character on line %d: \'%c\' (ASCII: %d)\nExpected a second \'%c\'", line, *pch, *pch, *pch);

			case '|':
				if (pch[1] == '|') {
					addTk(OR);
					pch += 2;
					break;
				}
				err("Invalid character on line %d: \'%c\' (ASCII: %d)\nExpected a second \'%c\'", line, *pch, *pch, *pch);

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
				} else if (isdigit(*pch)) {		// numeric constant
					for (start = pch++; isdigit(*pch); ++pch);
					if (*pch == '.' || *pch == 'e' || *pch == 'E') {	// double 
						int num_digits = 0;
						if (*pch == '.') {
							++pch;
							for (; isdigit(*pch); ++pch, ++num_digits);	// fractional part
							if (num_digits == 0 && tolower(*pch) != 'e') {	// no digits after '.'
								text = extract(start, pch);
								err("Invalid double constant on line %d: %s", line, text);
							}
						}
						if (*pch == 'e' || *pch == 'E')	{
							++pch;
							if (*pch == '+' || *pch == '-') {
								++pch;
							}
							for (num_digits = 0; isdigit(*pch); ++pch, ++num_digits);	// exponent
							if (num_digits == 0) {	// no digits after exponent
								text = extract(start, pch);
								err("Invalid double constant on line %d: %s", line, text);
							}
						}
						text = extract(start, pch);
						tk = addTk(DOUBLE);
						tk->d = atof(text);
						free(text);
					}  else {	// integer
						text = extract(start, pch);
						tk = addTk(INT);
						tk->i = atoi(text);
						free(text);
					}
				} else if (*pch == '\'') {		// character constant
					start = pch++;
					if (*pch == '\\' && strchr(ESCAPE_CHARS, pch[1]) && pch[2] == '\'') {	// excape character
						tk = addTk(CHAR);
						switch (pch[1]) {
							case 'n': tk->c = '\n'; break;
							case 'r': tk->c = '\r'; break;
							case 't': tk->c = '\t'; break;
							default: tk->c = pch[1];
						}
						pch += 3;
					} else if (*pch != '\\' && *pch != '\'' && pch[1] == '\'')  {	// ascii character (except \ and ')
						tk = addTk(CHAR);
						tk->c = *pch;
						pch += 2;
					} else {
						for (; *pch != '\n' && *pch != '\r' && *pch != '\0'; ++pch);	// find newline
						text = extract(start, pch);
						err("Invalid character constant on line %d: %s", line, text);
					}
				} else if (*pch == '"') {		// string constant
					for (start = ++pch; *pch != '"' && *pch != '\n' && *pch != '\r' && *pch != '\0'; ++pch) {
						if (*pch == '\\' && pch[1] == '"') {	// skip escaped double quote 
							++pch;
						} else if (*pch == '\\' && !strchr(ESCAPE_CHARS, pch[1])) {	// invalid escape character
							text = extract(start - 1, pch + 2);
							err("Invalid string constant on line %d: %s\nBad escape character", line, text);
						}
					}
					if (*pch == '"') {
						tk = addTk(STRING);
						text = extract(start, pch);
						tk->text = parseEscapeChars(text);
						free(text);
						++pch;
					} else {
						text = extract(start - 1, pch);
						err("Invalid string constant on line %d: %s\nMissing end double-quote", line, text);
					}
				} else { // error
					err("Invalid character on line %d: \'%c\' (ASCII: %d)", line, *pch, *pch);
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
	int n = end - begin;
	char *substr = safeAlloc(n + 1);
	for (int i = 0; i < n; ++i) {
		substr[i] = begin[i];
	}
	substr[n] = '\0';
	return substr;
}

static char *parseEscapeChars(const char *input) {
	char *s = safeAlloc(strlen(input) + 1);
	int n = 0;
	for (; *input != '\0'; ++input) {
		if (*input == '\\') {
			switch (input[1]) {
				case 'n': s[n++] = '\n'; break;
				case 'r': s[n++] = '\r'; break;
				case 't': s[n++] = '\t'; break;
				default: s[n++] = input[1];
			}
			++input;
			continue;
		}
		s[n++] = *input;
	}
	s[n] = '\0';
	return s;
}