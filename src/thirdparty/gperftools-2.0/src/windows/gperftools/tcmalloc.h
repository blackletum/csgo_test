﻿/* Copyright (c) 2003, Google Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ---
 * Author: Sanjay Ghemawat <opensource@google.com>
 *         .h.in file by Craig Silverstein <opensource@google.com>
 */

#ifndef TCMALLOC_TCMALLOC_H_
#define TCMALLOC_TCMALLOC_H_

#include <stddef.h>                     // for size_t
#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>   // where glibc defines __THROW
#endif

// __THROW is defined in glibc systems.  It means, counter-intuitively,
// "This function will never throw an exception."  It's an optional
// optimization tool, but we may need to use it to match glibc prototypes.
#ifndef __THROW    /* I guess we're not on a glibc system */
# define __THROW   /* __THROW is just an optimization, so ok to make it "" */
#endif

// Define the version number so folks can check against it
#define TC_VERSION_MAJOR  2
#define TC_VERSION_MINOR  0
#define TC_VERSION_PATCH  ""
#define TC_VERSION_STRING "gperftools 2.0"

#include <stdlib.h>   // for struct mallinfo, if it's defined

// Annoying stuff for windows -- makes sure clients can import these functions
#ifndef PERFTOOLS_DLL_DECL
# ifdef _WIN32
#   define PERFTOOLS_DLL_DECL  __declspec(dllimport)
# else
#   define PERFTOOLS_DLL_DECL
# endif
#endif

#ifdef __cplusplus
namespace std {
struct nothrow_t;
}

extern "C" {
#endif
  // Returns a human-readable version string.  If major, minor,
  // and/or patch are not NULL, they are set to the major version,
  // minor version, and patch-code (a string, usually "").
  PERFTOOLS_DLL_DECL const char* tc_version(int* major, int* minor,
                                            const char** patch) __THROW;

  PERFTOOLS_DLL_DECL void* tc_malloc(size_t size) __THROW;
  PERFTOOLS_DLL_DECL void tc_free(void* ptr) __THROW;
  PERFTOOLS_DLL_DECL void* tc_realloc(void* ptr, size_t size) __THROW;
  PERFTOOLS_DLL_DECL void* tc_calloc(size_t nmemb, size_t size) __THROW;
  PERFTOOLS_DLL_DECL void tc_cfree(void* ptr) __THROW;

  PERFTOOLS_DLL_DECL void* tc_memalign(size_t __alignment,
                                       size_t __size) __THROW;
  PERFTOOLS_DLL_DECL int tc_posix_memalign(void** ptr,
                                           size_t align, size_t size) __THROW;
  PERFTOOLS_DLL_DECL void* tc_valloc(size_t __size) __THROW;
  PERFTOOLS_DLL_DECL void* tc_pvalloc(size_t __size) __THROW;

  PERFTOOLS_DLL_DECL void tc_malloc_stats(void) __THROW;
  PERFTOOLS_DLL_DECL int tc_mallopt(int cmd, int value) __THROW;
#if 0
  PERFTOOLS_DLL_DECL struct mallinfo tc_mallinfo(void) __THROW;
#endif

  // This is an alias for MallocExtension::instance()->GetAllocatedSize().
  // It is equivalent to
  //    OS X: malloc_size()
  //    glibc: malloc_usable_size()
  //    Windows: _msize()
  PERFTOOLS_DLL_DECL size_t tc_malloc_size(void* ptr) __THROW;

#ifdef __cplusplus
  PERFTOOLS_DLL_DECL int tc_set_new_mode(int flag) __THROW;
  PERFTOOLS_DLL_DECL void* tc_new(size_t size);
  PERFTOOLS_DLL_DECL void* tc_new_nothrow(size_t size,
                                          const std::nothrow_t&) __THROW;
  PERFTOOLS_DLL_DECL void tc_delete(void* p) __THROW;
  PERFTOOLS_DLL_DECL void tc_delete_nothrow(void* p,
                                            const std::nothrow_t&) __THROW;
  PERFTOOLS_DLL_DECL void* tc_newarray(size_t size);
  PERFTOOLS_DLL_DECL void* tc_newarray_nothrow(size_t size,
                                               const std::nothrow_t&) __THROW;
  PERFTOOLS_DLL_DECL void tc_deletearray(void* p) __THROW;
  PERFTOOLS_DLL_DECL void tc_deletearray_nothrow(void* p,
                                                 const std::nothrow_t&) __THROW;
}
#endif

#endif  // #ifndef TCMALLOC_TCMALLOC_H_
