#ifndef __GC_H__
#define __GC_H__

// code generation

#include "at.h"
#include "vm.h"

// inserts after the specified instruction a conversion instruction
// only if necessary
extern void insertConvIfNeeded(Instr *before, Type *srcType, Type *dstType);

// if lval is true, generates an rval from the current value from stack
extern void addRVal(Instr **code, bool lval, Type *type);

#endif