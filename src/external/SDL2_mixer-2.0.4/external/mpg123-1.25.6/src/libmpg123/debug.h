﻿/*
	debug.h: 
		if DEBUG defined: debugging macro fprintf wrappers
		else: macros defined to do nothing
	That saves typing #ifdef DEBUG all the time and still preserves
	lean code without debugging.
	
	public domain (or LGPL / GPL, if you like that more;-)
	generated by debugdef.pl, what was
	trivially written by Thomas Orgis <thomas@orgis.org>
*/

#include "config.h"

/*
	I could do that with variadic macros available:
	#define sdebug(me, s) fprintf(stderr, "[location] " s "\n")
	#define debug(me, s, ...) fprintf(stderr, "[location] " s "}n", __VA_ARGS__)

	Variadic macros are a C99 feature...
	Now just predefining stuff non-variadic for up to 15 arguments.
	It's cumbersome to have them all with different names, though...
*/

#ifdef ME
#define DBGPRFX ME": "
#else
#define DBGPRFX ""
#endif

#ifdef DEBUG

#include <stdio.h>
#define debug(s) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__)
#define debug1(s, a) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a)
#define debug2(s, a, b) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b)
#define debug3(s, a, b, c) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c)
#define debug4(s, a, b, c, d) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d)
#define debug5(s, a, b, c, d, e) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e)
#define debug6(s, a, b, c, d, e, f) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f)
#define debug7(s, a, b, c, d, e, f, g) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f, g)
#define debug8(s, a, b, c, d, e, f, g, h) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f, g, h)
#define debug9(s, a, b, c, d, e, f, g, h, i) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i)
#define debug10(s, a, b, c, d, e, f, g, h, i, j) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j)
#define debug11(s, a, b, c, d, e, f, g, h, i, j, k) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k)
#define debug12(s, a, b, c, d, e, f, g, h, i, j, k, l) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l)
#define debug13(s, a, b, c, d, e, f, g, h, i, j, k, l, m) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m)
#define debug14(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m, n)
#define debug15(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] debug: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)
#else
#define debug(s) 
#define debug1(s, a) 
#define debug2(s, a, b) 
#define debug3(s, a, b, c) 
#define debug4(s, a, b, c, d) 
#define debug5(s, a, b, c, d, e) 
#define debug6(s, a, b, c, d, e, f) 
#define debug7(s, a, b, c, d, e, f, g) 
#define debug8(s, a, b, c, d, e, f, g, h) 
#define debug9(s, a, b, c, d, e, f, g, h, i) 
#define debug10(s, a, b, c, d, e, f, g, h, i, j) 
#define debug11(s, a, b, c, d, e, f, g, h, i, j, k) 
#define debug12(s, a, b, c, d, e, f, g, h, i, j, k, l) 
#define debug13(s, a, b, c, d, e, f, g, h, i, j, k, l, m) 
#define debug14(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n) 
#define debug15(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) 
#endif

/* warning macros also here... */
#ifndef NO_WARNING
#define warning(s) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__)
#define warning1(s, a) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a)
#define warning2(s, a, b) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b)
#define warning3(s, a, b, c) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c)
#define warning4(s, a, b, c, d) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d)
#define warning5(s, a, b, c, d, e) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e)
#define warning6(s, a, b, c, d, e, f) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f)
#define warning7(s, a, b, c, d, e, f, g) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f, g)
#define warning8(s, a, b, c, d, e, f, g, h) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f, g, h)
#define warning9(s, a, b, c, d, e, f, g, h, i) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i)
#define warning10(s, a, b, c, d, e, f, g, h, i, j) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j)
#define warning11(s, a, b, c, d, e, f, g, h, i, j, k) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k)
#define warning12(s, a, b, c, d, e, f, g, h, i, j, k, l) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l)
#define warning13(s, a, b, c, d, e, f, g, h, i, j, k, l, m) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m)
#define warning14(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m, n)
#define warning15(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] warning: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)
#else
#define warning(s) 
#define warning1(s, a) 
#define warning2(s, a, b) 
#define warning3(s, a, b, c) 
#define warning4(s, a, b, c, d) 
#define warning5(s, a, b, c, d, e) 
#define warning6(s, a, b, c, d, e, f) 
#define warning7(s, a, b, c, d, e, f, g) 
#define warning8(s, a, b, c, d, e, f, g, h) 
#define warning9(s, a, b, c, d, e, f, g, h, i) 
#define warning10(s, a, b, c, d, e, f, g, h, i, j) 
#define warning11(s, a, b, c, d, e, f, g, h, i, j, k) 
#define warning12(s, a, b, c, d, e, f, g, h, i, j, k, l) 
#define warning13(s, a, b, c, d, e, f, g, h, i, j, k, l, m) 
#define warning14(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n) 
#define warning15(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) 
#endif

/* error macros also here... */
#ifndef NO_ERROR
#define error(s) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__)
#define error1(s, a) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a)
#define error2(s, a, b) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b)
#define error3(s, a, b, c) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c)
#define error4(s, a, b, c, d) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d)
#define error5(s, a, b, c, d, e) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e)
#define error6(s, a, b, c, d, e, f) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f)
#define error7(s, a, b, c, d, e, f, g) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f, g)
#define error8(s, a, b, c, d, e, f, g, h) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f, g, h)
#define error9(s, a, b, c, d, e, f, g, h, i) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i)
#define error10(s, a, b, c, d, e, f, g, h, i, j) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j)
#define error11(s, a, b, c, d, e, f, g, h, i, j, k) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k)
#define error12(s, a, b, c, d, e, f, g, h, i, j, k, l) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l)
#define error13(s, a, b, c, d, e, f, g, h, i, j, k, l, m) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m)
#define error14(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m, n)
#define error15(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] error: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)
#else
#define error(s) 
#define error1(s, a) 
#define error2(s, a, b) 
#define error3(s, a, b, c) 
#define error4(s, a, b, c, d) 
#define error5(s, a, b, c, d, e) 
#define error6(s, a, b, c, d, e, f) 
#define error7(s, a, b, c, d, e, f, g) 
#define error8(s, a, b, c, d, e, f, g, h) 
#define error9(s, a, b, c, d, e, f, g, h, i) 
#define error10(s, a, b, c, d, e, f, g, h, i, j) 
#define error11(s, a, b, c, d, e, f, g, h, i, j, k) 
#define error12(s, a, b, c, d, e, f, g, h, i, j, k, l) 
#define error13(s, a, b, c, d, e, f, g, h, i, j, k, l, m) 
#define error14(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n) 
#define error15(s, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) 
#endif

/* ereturn macros also here... */
#ifndef NO_ERETURN
#define ereturn(rv, s) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__); return rv; }while(0)
#define ereturn1(rv, s, a) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a); return rv; }while(0)
#define ereturn2(rv, s, a, b) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b); return rv; }while(0)
#define ereturn3(rv, s, a, b, c) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c); return rv; }while(0)
#define ereturn4(rv, s, a, b, c, d) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d); return rv; }while(0)
#define ereturn5(rv, s, a, b, c, d, e) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e); return rv; }while(0)
#define ereturn6(rv, s, a, b, c, d, e, f) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f); return rv; }while(0)
#define ereturn7(rv, s, a, b, c, d, e, f, g) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f, g); return rv; }while(0)
#define ereturn8(rv, s, a, b, c, d, e, f, g, h) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f, g, h); return rv; }while(0)
#define ereturn9(rv, s, a, b, c, d, e, f, g, h, i) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i); return rv; }while(0)
#define ereturn10(rv, s, a, b, c, d, e, f, g, h, i, j) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j); return rv; }while(0)
#define ereturn11(rv, s, a, b, c, d, e, f, g, h, i, j, k) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k); return rv; }while(0)
#define ereturn12(rv, s, a, b, c, d, e, f, g, h, i, j, k, l) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l); return rv; }while(0)
#define ereturn13(rv, s, a, b, c, d, e, f, g, h, i, j, k, l, m) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m); return rv; }while(0)
#define ereturn14(rv, s, a, b, c, d, e, f, g, h, i, j, k, l, m, n) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m, n); return rv; }while(0)
#define ereturn15(rv, s, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) do{ fprintf(stderr, DBGPRFX"[" __FILE__ ":%i] ereturn: " s "\n", __LINE__, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o); return rv; }while(0)
#else
#define ereturn(rv, s) return rv
#define ereturn1(rv, s, a) return rv
#define ereturn2(rv, s, a, b) return rv
#define ereturn3(rv, s, a, b, c) return rv
#define ereturn4(rv, s, a, b, c, d) return rv
#define ereturn5(rv, s, a, b, c, d, e) return rv
#define ereturn6(rv, s, a, b, c, d, e, f) return rv
#define ereturn7(rv, s, a, b, c, d, e, f, g) return rv
#define ereturn8(rv, s, a, b, c, d, e, f, g, h) return rv
#define ereturn9(rv, s, a, b, c, d, e, f, g, h, i) return rv
#define ereturn10(rv, s, a, b, c, d, e, f, g, h, i, j) return rv
#define ereturn11(rv, s, a, b, c, d, e, f, g, h, i, j, k) return rv
#define ereturn12(rv, s, a, b, c, d, e, f, g, h, i, j, k, l) return rv
#define ereturn13(rv, s, a, b, c, d, e, f, g, h, i, j, k, l, m) return rv
#define ereturn14(rv, s, a, b, c, d, e, f, g, h, i, j, k, l, m, n) return rv
#define ereturn15(rv, s, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) return rv
#endif
