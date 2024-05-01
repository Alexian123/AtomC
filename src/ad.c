#include "utils.h"
#include "ad.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

Domain *symTable = NULL;

int typeBaseSize(Type *t) {
	switch (t->tb) {
		case TB_INT:
			return sizeof(int);
		case TB_DOUBLE:
			return sizeof(double);
		case TB_CHAR:
			return sizeof(char);
		case TB_VOID:
			return 0;
		default: {	// TB_STRUCT
			int size = 0;
			for (Symbol *m = t->s->structMembers; m; m = m->next) {
				size += typeSize(&m->type);
			}
			return size;
		}
	}
}

int typeSize(Type *t) {
	if (t->n < 0) {
		return typeBaseSize(t);
	}
	if (t->n == 0) {
		return sizeof(void*);
	}
	return t->n * typeBaseSize(t);
}

// free from memory a list of symbols
void freeSymbols(Symbol *list) {
	for (Symbol *next; list; list = next) {
		next = list->next;
		freeSymbol(list);
	}
}

Symbol *newSymbol(const char *name, SymKind kind) {
	Symbol *s = (Symbol*) safeAlloc(sizeof(Symbol));
	// sets all the fields to 0/NULL
	memset(s, 0, sizeof(Symbol));
	s->name = name;
	s->kind = kind;
	return s;
}

Symbol *dupSymbol(Symbol *symbol) {
	Symbol *s = (Symbol*) safeAlloc(sizeof(Symbol));
	*s = *symbol;
	s->next = NULL;
	return s;
}

// s->next is already NULL from newSymbol
Symbol *addSymbolToList(Symbol **list, Symbol *s) {
	Symbol *iter = *list;
	if (iter) {
		while (iter->next) {
			iter = iter->next;
		}
		iter->next = s;
	} else {
		*list = s;
	}
	return s;
}

int symbolsLen(Symbol *list) {
	int n = 0;
	for (; list; list = list->next) n++;
	return n;
}

void freeSymbol(Symbol *s) {
	switch (s->kind) {
		case SK_VAR:
			if (!s->owner)
				free(s->varMem);
			break;
		case SK_FN:
			freeSymbols(s->fn.params);
			freeSymbols(s->fn.locals);
			break;
		case SK_STRUCT:
			freeSymbols(s->structMembers);
			break;
		case SK_PARAM:
			break;
	}
	free(s);
}

Domain *pushDomain() {
	Domain *d = (Domain*) safeAlloc(sizeof(Domain));
	d->symbols = NULL;
	d->parent = symTable;
	symTable = d;
	return d;
}

void dropDomain() {
	Domain *d = symTable;
	symTable = d->parent;
	freeSymbols(d->symbols);
	free(d);
}

void showNamedType(Type *t, const char *name, FILE *stream) {
	switch (t->tb) {
		case TB_INT:			fprintf(stream, "int"); break;
		case TB_DOUBLE:			fprintf(stream, "double"); break;
		case TB_CHAR:			fprintf(stream, "char"); break;
		case TB_VOID:			fprintf(stream, "void"); break;
		default:/*TB_STRUCT*/	fprintf(stream, "struct %s",t->s->name);
	}
	if (name) {
		fprintf(stream, " %s", name);
	}
	if (t->n == 0) {
		fprintf(stream, "[]");
	}
	else if (t->n > 0) {
		fprintf(stream, "[%d]", t->n);
	}
}

void showSymbol(Symbol *s, FILE *stream) {
	switch (s->kind) {
			case SK_VAR:
				showNamedType(&s->type, s->name, stream);
				if (s->owner) {
					fprintf(stream, ";\t// size=%d, idx=%d\n", typeSize(&s->type), s->varIdx);
				} else {
					fprintf(stream, ";\t// size=%d, mem=%p\n", typeSize(&s->type), s->varMem);
				}
				break;
			case SK_PARAM: 
				{
					showNamedType(&s->type,s->name, stream);
					fprintf(stream, " /*size=%d, idx=%d*/", typeSize(&s->type), s->paramIdx);
				}
				break;
			case SK_FN: 
				{
					showNamedType(&s->type, s->name, stream);
					fprintf(stream, "(");
					bool next = false;
					for(Symbol *param = s->fn.params; param; param = param->next) {
						if (next) {
							fprintf(stream, ", ");
						}
						showSymbol(param, stream);
						next = true;
					}
					fprintf(stream, "){\n");
					for(Symbol *local = s->fn.locals; local; local = local->next) {
						fprintf(stream, "\t");
						showSymbol(local, stream);
					}
					fprintf(stream, "\t}\n");
				}
				break;
			case SK_STRUCT:
				{
					fprintf(stream, "struct %s{\n", s->name);
					for (Symbol *m = s->structMembers; m; m = m->next) {
						fprintf(stream, "\t");
						showSymbol(m, stream);
					}
					fprintf(stream, "\t};\t// size=%d\n", typeSize(&s->type));
				}
				break;
	}
}

void showDomain(Domain *d, const char *name, FILE *stream) {
	fprintf(stream, "// domain: %s\n", name);
	for (Symbol *s = d->symbols; s; s = s->next) {
		showSymbol(s, stream);
	}
	fputs("\n", stream);
}

Symbol *findSymbolInDomain(Domain *d, const char *name) {
	for(Symbol *s = d->symbols; s; s = s->next) {
		if (!strcmp(s->name, name)) {
			return s;
		}
	}
	return NULL;
}

Symbol *findSymbol(const char *name) {
	for(Domain *d = symTable; d; d = d->parent) {
		Symbol *s = findSymbolInDomain(d, name);
		if (s) {
			return s;
		}
	}
	return NULL;
}

Symbol *addSymbolToDomain(Domain *d, Symbol *s) {
	return addSymbolToList(&d->symbols, s);
}

Symbol *addExtFn(const char *name, void (*extFnPtr)(), Type ret) {
	Symbol *fn = newSymbol(name, SK_FN);
	fn->fn.extFnPtr = extFnPtr;
	fn->type = ret;
	addSymbolToDomain(symTable, fn);
	return fn;
}

Symbol *addFnParam(Symbol *fn, const char *name, Type type) {
	Symbol *param = newSymbol(name, SK_PARAM);
	param->type = type;
	param->paramIdx = symbolsLen(fn->fn.params);
	addSymbolToList(&fn->fn.params, dupSymbol(param));
	return param;
}
