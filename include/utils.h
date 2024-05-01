#ifndef __UTISL_H__
#define __UTILS_H__

#include <stddef.h>
#include <stdnoreturn.h>
#include <stdio.h>

extern noreturn void err(const char *fmt, ...);

extern void *safeAlloc(size_t nBytes);

extern char *loadFile(const char *fileName);

extern FILE *createOutputStream(const char *fileName);

extern const char *getTokenName(int code);

#endif