#ifndef __UTISL_H__
#define __UTILS_H__

#include <stddef.h>
#include <stdnoreturn.h>

extern noreturn void err(const char *fmt, ...);

extern void *safeAlloc(size_t nBytes);

extern char *loadFile(const char *fileName);

#endif