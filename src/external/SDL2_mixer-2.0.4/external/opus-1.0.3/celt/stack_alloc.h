﻿/* Copyright (C) 2002-2003 Jean-Marc Valin
   Copyright (C) 2007-2009 Xiph.Org Foundation */
/**
   @file stack_alloc.h
   @brief Temporary memory allocation on stack
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef STACK_ALLOC_H
#define STACK_ALLOC_H

#if (!defined (VAR_ARRAYS) && !defined (USE_ALLOCA) && !defined (NONTHREADSAFE_PSEUDOSTACK))
#error "Opus requires one of VAR_ARRAYS, USE_ALLOCA, or NONTHREADSAFE_PSEUDOSTACK be defined to select the temporary allocation mode."
#endif

#ifdef USE_ALLOCA
# ifdef WIN32
#  include <malloc.h>
# else
#  ifdef HAVE_ALLOCA_H
#   include <alloca.h>
#  else
#   include <stdlib.h>
#  endif
# endif
#endif

/**
 * @def ALIGN(stack, size)
 *
 * Aligns the stack to a 'size' boundary
 *
 * @param stack Stack
 * @param size  New size boundary
 */

/**
 * @def PUSH(stack, size, type)
 *
 * Allocates 'size' elements of type 'type' on the stack
 *
 * @param stack Stack
 * @param size  Number of elements
 * @param type  Type of element
 */

/**
 * @def VARDECL(var)
 *
 * Declare variable on stack
 *
 * @param var Variable to declare
 */

/**
 * @def ALLOC(var, size, type)
 *
 * Allocate 'size' elements of 'type' on stack
 *
 * @param var  Name of variable to allocate
 * @param size Number of elements
 * @param type Type of element
 */

#if defined(VAR_ARRAYS)

#define VARDECL(type, var)
#define ALLOC(var, size, type) type var[size]
#define SAVE_STACK
#define RESTORE_STACK
#define ALLOC_STACK

#elif defined(USE_ALLOCA)

#define VARDECL(type, var) type *var

# ifdef WIN32
#  define ALLOC(var, size, type) var = ((type*)_alloca(sizeof(type)*(size)))
# else
#  define ALLOC(var, size, type) var = ((type*)alloca(sizeof(type)*(size)))
# endif

#define SAVE_STACK
#define RESTORE_STACK
#define ALLOC_STACK

#else

#ifdef CELT_C
char *global_stack=0;
#else
extern char *global_stack;
#endif /* CELT_C */

#ifdef ENABLE_VALGRIND

#include <valgrind/memcheck.h>

#ifdef CELT_C
char *global_stack_top=0;
#else
extern char *global_stack_top;
#endif /* CELT_C */

#define ALIGN(stack, size) ((stack) += ((size) - (long)(stack)) & ((size) - 1))
#define PUSH(stack, size, type) (VALGRIND_MAKE_MEM_NOACCESS(stack, global_stack_top-stack),ALIGN((stack),sizeof(type)/sizeof(char)),VALGRIND_MAKE_MEM_UNDEFINED(stack, ((size)*sizeof(type)/sizeof(char))),(stack)+=(2*(size)*sizeof(type)/sizeof(char)),(type*)((stack)-(2*(size)*sizeof(type)/sizeof(char))))
#define RESTORE_STACK ((global_stack = _saved_stack),VALGRIND_MAKE_MEM_NOACCESS(global_stack, global_stack_top-global_stack))
#define ALLOC_STACK char *_saved_stack; ((global_stack = (global_stack==0) ? ((global_stack_top=opus_alloc_scratch(GLOBAL_STACK_SIZE*2)+(GLOBAL_STACK_SIZE*2))-(GLOBAL_STACK_SIZE*2)) : global_stack),VALGRIND_MAKE_MEM_NOACCESS(global_stack, global_stack_top-global_stack)); _saved_stack = global_stack;

#else

#define ALIGN(stack, size) ((stack) += ((size) - (long)(stack)) & ((size) - 1))
#define PUSH(stack, size, type) (ALIGN((stack),sizeof(type)/sizeof(char)),(stack)+=(size)*(sizeof(type)/sizeof(char)),(type*)((stack)-(size)*(sizeof(type)/sizeof(char))))
#define RESTORE_STACK (global_stack = _saved_stack)
#define ALLOC_STACK char *_saved_stack; (global_stack = (global_stack==0) ? opus_alloc_scratch(GLOBAL_STACK_SIZE) : global_stack); _saved_stack = global_stack;

#endif /* ENABLE_VALGRIND */

#include "os_support.h"
#define VARDECL(type, var) type *var
#define ALLOC(var, size, type) var = PUSH(global_stack, size, type)
#define SAVE_STACK char *_saved_stack = global_stack;

#endif /* VAR_ARRAYS */

#endif /* STACK_ALLOC_H */
