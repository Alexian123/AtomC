#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "utils.h"

Token *tokens;	// single linked list of tokens
Token *lastTk;		// the last token in list

int line=1;		// the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code) {
	Token *tk = safeAlloc(sizeof(Token));
	tk->code = code;
	tk->line = line;
	tk->next = NULL;
	if (lastTk) {
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

Token *tokenize(const char *pch) {
	const char *start;
	Token *tk;
	for (;;) {
		switch (*pch) {
			case ' ':

			case '\t':
				pch++;
				break;

			case '\r':		// handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
				if (pch[1] == '\n') {
					pch++;
				}
				// fallthrough to \n

			case '\n':
				line++;
				pch++;
				break;

			case '\0':
				addTk(END);
				return tokens;

			case ',':
				addTk(COMMA);
				pch++;
				break;

			case '+':
				addTk(ADD);
				++pch;
				break;

			case '-':
				addTk(SUB);
				++pch;
				break;

			case '*':
				addTk(MUL);
				++pch;
				break;

			case '/':
				addTk(DIV);
				++pch;
				break;

			case '.':
				addTk(DOT);
				++pch;
				break;

			case '&':
				if (pch[1] == '&') {
					addTk(AND);
					pch += 2;
				} else {
					err("invalid char: %c (%d)",*pch,*pch);
				}
				break;

			case '|':
				if (pch[1] == '|') {
					addTk(OR);
					pch += 2;
				} else {
					err("invalid char: %c (%d)",*pch,*pch);
				}
				break;

			case '!':
				addTk(NOT);
				++pch;
				break;

			case '=':
				if (pch[1] == '=') {
					addTk(EQUAL);
					pch+=2;
				} else {
					addTk(ASSIGN);
					pch++;
				}
				break;

			default:
				if (isalpha(*pch) || *pch == '_') {
					for (start = pch++; isalnum(*pch) || *pch == '_'; pch++) {

					}
					char *text=extract(start,pch);
					if (strcmp(text, "char") == 0) {
						addTk(TYPE_CHAR);
					} else {
						tk = addTk(ID);
						tk->text = text;
					}
				}
				else {
					err("invalid char: %c (%d)",*pch,*pch);
				}
		}
	}
}

void showTokens(const Token *tokens) {
	for (const Token *tk = tokens; tk != NULL; tk = tk->next) {
		printf("%d\n", tk->code);
	}
}
