﻿/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    xnamath.h

Abstract:

	XNA math library for Windows and Xbox 360
--*/

#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif

#ifndef __XNAMATH_H__
#define __XNAMATH_H__

#ifdef __XBOXMATH_H__
#error XNAMATH and XBOXMATH are incompatible in the same compilation module. Use one or the other.
#endif

#define XNAMATH_VERSION 200

#if !defined(_XM_X64_) && !defined(_XM_X86_)
#if defined(_M_AMD64) || defined(_AMD64_)
#define _XM_X64_
#elif defined(_M_IX86) || defined(_X86_)
#define _XM_X86_
#endif
#endif

#if defined(_XM_X86_) || defined(_XM_X64_)
#define _XM_SSE_INTRINSICS_
#if !defined(__cplusplus) && !defined(_XM_NO_INTRINSICS_)
#error xnamath.h only supports C compliation for Xbox 360 targets and no intrinsics cases for x86/x64
#endif
#elif defined(_XBOX_VER)
#if !defined(__VMX128_SUPPORTED) && !defined(_XM_NO_INTRINSICS_)
#error xnamath.h requires VMX128 compiler support for XBOX 360
#endif // !__VMX128_SUPPORTED && !_XM_NO_INTRINSICS_
#define _XM_VMX128_INTRINSICS_
#else
#error xnamath.h only supports x86, x64, or XBox 360 targets
#endif

#if defined(_XM_SSE_INTRINSICS_)
#ifndef _XM_NO_INTRINSICS_
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
#elif defined(_XM_VMX128_INTRINSICS_)
#error This version of xnamath.h is for Windows use only
#endif

#if defined(_XM_SSE_INTRINSICS_)
#pragma warning(push)
#pragma warning(disable:4985)
#endif
#include <math.h>
#if defined(_XM_SSE_INTRINSICS_)
#pragma warning(pop)
#endif

#include <sal.h>

#if !defined(XMINLINE)
#if !defined(XM_NO_MISALIGNED_VECTOR_ACCESS)
#define XMINLINE __inline
#else
#define XMINLINE __forceinline
#endif
#endif

#if !defined(XMFINLINE)
#define XMFINLINE __forceinline
#endif

#if !defined(XMDEBUG)
#if defined(_DEBUG)
#define XMDEBUG
#endif
#endif // !XMDEBUG

#if !defined(XMASSERT)
#if defined(_PREFAST_)
#define XMASSERT(Expression) __analysis_assume((Expression))
#elif defined(XMDEBUG) // !_PREFAST_
#define XMASSERT(Expression) ((VOID)((Expression) || (XMAssert(#Expression, __FILE__, __LINE__), 0)))
#else // !XMDEBUG
#define XMASSERT(Expression) ((VOID)0)
#endif // !XMDEBUG
#endif // !XMASSERT

#if !defined(XM_NO_ALIGNMENT)
#define _DECLSPEC_ALIGN_16_   __declspec(align(16))
#else
#define _DECLSPEC_ALIGN_16_
#endif


#if defined(_MSC_VER) && (_MSC_VER<1500) && (_MSC_VER>=1400)
#define _XM_ISVS2005_
#endif

#if defined(_MSC_VER) && (_MSC_VER<1600) && (_MSC_VER>=1500)
#define _XM_ISVS2008_
#endif

/****************************************************************************
 *
 * Constant definitions
 *
 ****************************************************************************/

#define XM_PI               3.141592654f
#define XM_2PI              6.283185307f
#define XM_1DIVPI           0.318309886f
#define XM_1DIV2PI          0.159154943f
#define XM_PIDIV2           1.570796327f
#define XM_PIDIV4           0.785398163f

#define XM_SELECT_0         0x00000000
#define XM_SELECT_1         0xFFFFFFFF

#define XM_PERMUTE_0X       0x00010203
#define XM_PERMUTE_0Y       0x04050607
#define XM_PERMUTE_0Z       0x08090A0B
#define XM_PERMUTE_0W       0x0C0D0E0F
#define XM_PERMUTE_1X       0x10111213
#define XM_PERMUTE_1Y       0x14151617
#define XM_PERMUTE_1Z       0x18191A1B
#define XM_PERMUTE_1W       0x1C1D1E1F

#define XM_CRMASK_CR6       0x000000F0
#define XM_CRMASK_CR6TRUE   0x00000080
#define XM_CRMASK_CR6FALSE  0x00000020
#define XM_CRMASK_CR6BOUNDS XM_CRMASK_CR6FALSE

#define XM_CACHE_LINE_SIZE  64

/****************************************************************************
 *
 * Macros
 *
 ****************************************************************************/

// Unit conversion

XMFINLINE FLOAT XMConvertToRadians(FLOAT fDegrees) { return fDegrees * (XM_PI / 180.0f); }
XMFINLINE FLOAT XMConvertToDegrees(FLOAT fRadians) { return fRadians * (180.0f / XM_PI); }

// Condition register evaluation proceeding a recording (Rc) comparison

#define XMComparisonAllTrue(CR)            (((CR) & XM_CRMASK_CR6TRUE) == XM_CRMASK_CR6TRUE)
#define XMComparisonAnyTrue(CR)            (((CR) & XM_CRMASK_CR6FALSE) != XM_CRMASK_CR6FALSE)
#define XMComparisonAllFalse(CR)           (((CR) & XM_CRMASK_CR6FALSE) == XM_CRMASK_CR6FALSE)
#define XMComparisonAnyFalse(CR)           (((CR) & XM_CRMASK_CR6TRUE) != XM_CRMASK_CR6TRUE)
#define XMComparisonMixed(CR)              (((CR) & XM_CRMASK_CR6) == 0)
#define XMComparisonAllInBounds(CR)        (((CR) & XM_CRMASK_CR6BOUNDS) == XM_CRMASK_CR6BOUNDS)
#define XMComparisonAnyOutOfBounds(CR)     (((CR) & XM_CRMASK_CR6BOUNDS) != XM_CRMASK_CR6BOUNDS)


#define XMMin(a, b) (((a) < (b)) ? (a) : (b))
#define XMMax(a, b) (((a) > (b)) ? (a) : (b))

/****************************************************************************
 *
 * Data types
 *
 ****************************************************************************/

#pragma warning(push)
#pragma warning(disable:4201)

#if !defined (_XM_X86_) && !defined(_XM_X64_)
#pragma bitfield_order(push)
#pragma bitfield_order(lsb_to_msb)
#endif // !_XM_X86_ && !_XM_X64_

#if defined(_XM_NO_INTRINSICS_) && !defined(_XBOX_VER)
// The __vector4 structure is an intrinsic on Xbox but must be separately defined
// for x86/x64
typedef struct __vector4
{
    union
    {
        struct
        {
            FLOAT x;
            FLOAT y;
            FLOAT z;
            FLOAT w;
        };
        FLOAT v[4];
        UINT  u[4];
    };
} __vector4;
#endif // _XM_NO_INTRINSICS_

#if (defined (_XM_X86_) || defined(_XM_X64_)) && defined(_XM_NO_INTRINSICS_)
typedef UINT __vector4i[4];
#else
typedef __declspec(align(16)) UINT __vector4i[4];
#endif

// Vector intrinsic: Four 32 bit floating point components aligned on a 16 byte 
// boundary and mapped to hardware vector registers
#if defined(_XM_SSE_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)
typedef __m128 XMVECTOR;
#else
typedef __vector4 XMVECTOR;
#endif

// Conversion types for constants
typedef _DECLSPEC_ALIGN_16_ struct XMVECTORF32 {
    union {
        float f[4];
        XMVECTOR v;
    };

#if defined(__cplusplus)
    inline operator XMVECTOR() const { return v; }
#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)
    inline operator __m128i() const { return reinterpret_cast<const __m128i *>(&v)[0]; }
    inline operator __m128d() const { return reinterpret_cast<const __m128d *>(&v)[0]; }
#endif
#endif // __cplusplus
} XMVECTORF32;

typedef _DECLSPEC_ALIGN_16_ struct XMVECTORI32 {
    union {
        INT i[4];
        XMVECTOR v;
    };
#if defined(__cplusplus)
    inline operator XMVECTOR() const { return v; }
#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)
    inline operator __m128i() const { return reinterpret_cast<const __m128i *>(&v)[0]; }
    inline operator __m128d() const { return reinterpret_cast<const __m128d *>(&v)[0]; }
#endif
#endif // __cplusplus
} XMVECTORI32;

typedef _DECLSPEC_ALIGN_16_ struct XMVECTORU8 {
    union {
        BYTE u[16];
        XMVECTOR v;
    };
#if defined(__cplusplus)
    inline operator XMVECTOR() const { return v; }
#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)
    inline operator __m128i() const { return reinterpret_cast<const __m128i *>(&v)[0]; }
    inline operator __m128d() const { return reinterpret_cast<const __m128d *>(&v)[0]; }
#endif
#endif // __cplusplus
} XMVECTORU8;

typedef _DECLSPEC_ALIGN_16_ struct XMVECTORU32 {
    union {
        UINT u[4];
        XMVECTOR v;
    };
#if defined(__cplusplus)
    inline operator XMVECTOR() const { return v; }
#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)
    inline operator __m128i() const { return reinterpret_cast<const __m128i *>(&v)[0]; }
    inline operator __m128d() const { return reinterpret_cast<const __m128d *>(&v)[0]; }
#endif
#endif // __cplusplus
} XMVECTORU32;

// Fix-up for (1st-3rd) XMVECTOR parameters that are pass-in-register for x86 and Xbox 360, but not for other targets
#if defined(_XM_VMX128_INTRINSICS_) && !defined(_XM_NO_INTRINISCS_)
typedef const XMVECTOR FXMVECTOR;
#elif defined(_XM_X86_) && !defined(_XM_NO_INTRINISCS_)
typedef const XMVECTOR FXMVECTOR;
#elif defined(__cplusplus)
typedef const XMVECTOR& FXMVECTOR;
#else
typedef const XMVECTOR FXMVECTOR;
#endif

// Fix-up for (4th+) XMVECTOR parameters to pass in-register for Xbox 360 and by reference otherwise
#if defined(_XM_VMX128_INTRINSICS_) && !defined(_XM_NO_INTRINISCS_)
typedef const XMVECTOR CXMVECTOR;
#elif defined(__cplusplus)
typedef const XMVECTOR& CXMVECTOR;
#else
typedef const XMVECTOR CXMVECTOR;
#endif

// Vector operators
#if defined(__cplusplus) && !defined(XM_NO_OPERATOR_OVERLOADS)

XMVECTOR    operator+ (FXMVECTOR V);
XMVECTOR    operator- (FXMVECTOR V);

XMVECTOR&   operator+= (XMVECTOR& V1, FXMVECTOR V2);
XMVECTOR&   operator-= (XMVECTOR& V1, FXMVECTOR V2);
XMVECTOR&   operator*= (XMVECTOR& V1, FXMVECTOR V2);
XMVECTOR&   operator/= (XMVECTOR& V1, FXMVECTOR V2);
XMVECTOR&   operator*= (XMVECTOR& V, FLOAT S);
XMVECTOR&   operator/= (XMVECTOR& V, FLOAT S);

XMVECTOR    operator+ (FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR    operator- (FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR    operator* (FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR    operator/ (FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR    operator* (FXMVECTOR V, FLOAT S);
XMVECTOR    operator* (FLOAT S, FXMVECTOR V);
XMVECTOR    operator/ (FXMVECTOR V, FLOAT S);

#endif // __cplusplus && !XM_NO_OPERATOR_OVERLOADS

// Matrix type: Sixteen 32 bit floating point components aligned on a
// 16 byte boundary and mapped to four hardware vector registers
#if (defined(_XM_X86_) || defined(_XM_X64_)) && defined(_XM_NO_INTRINSICS_)
typedef struct _XMMATRIX
#else
typedef _DECLSPEC_ALIGN_16_ struct _XMMATRIX
#endif
{
    union
    {
        XMVECTOR r[4];
        struct
        {
            FLOAT _11, _12, _13, _14;
            FLOAT _21, _22, _23, _24;
            FLOAT _31, _32, _33, _34;
            FLOAT _41, _42, _43, _44;
        };
        FLOAT m[4][4];
    };

#ifdef __cplusplus

                _XMMATRIX() {};
                _XMMATRIX(FXMVECTOR R0, FXMVECTOR R1, FXMVECTOR R2, CXMVECTOR R3);
                _XMMATRIX(FLOAT m00, FLOAT m01, FLOAT m02, FLOAT m03,
                         FLOAT m10, FLOAT m11, FLOAT m12, FLOAT m13,
                         FLOAT m20, FLOAT m21, FLOAT m22, FLOAT m23,
                         FLOAT m30, FLOAT m31, FLOAT m32, FLOAT m33);
                _XMMATRIX(CONST FLOAT *pArray);

    FLOAT       operator() (UINT Row, UINT Column) CONST;
    FLOAT&      operator() (UINT Row, UINT Column);

    _XMMATRIX&  operator= (CONST _XMMATRIX&);

#ifndef XM_NO_OPERATOR_OVERLOADS
    _XMMATRIX&  operator*= (CONST _XMMATRIX&);
    _XMMATRIX   operator* (CONST _XMMATRIX&) CONST;
#endif // !XM_NO_OPERATOR_OVERLOADS

#endif // __cplusplus

} XMMATRIX;

// Fix-up for XMMATRIX parameters to pass in-register on Xbox 360, by reference otherwise
#if defined(_XM_VMX128_INTRINSICS_)
typedef const XMMATRIX CXMMATRIX;
#elif defined(__cplusplus)
typedef const XMMATRIX& CXMMATRIX;
#else
typedef const XMMATRIX CXMMATRIX;
#endif

// 16 bit floating point number consisting of a sign bit, a 5 bit biased 
// exponent, and a 10 bit mantissa
//typedef WORD HALF;
typedef USHORT HALF;

// 2D Vector; 32 bit floating point components
typedef struct _XMFLOAT2
{
    FLOAT x;
    FLOAT y;

#ifdef __cplusplus

    _XMFLOAT2() {};
    _XMFLOAT2(FLOAT _x, FLOAT _y);
    _XMFLOAT2(CONST FLOAT *pArray);

    _XMFLOAT2& operator= (CONST _XMFLOAT2& Float2);

#endif // __cplusplus

} XMFLOAT2;

// 2D Vector; 32 bit floating point components aligned on a 16 byte boundary
typedef __declspec(align(16)) XMFLOAT2 XMFLOAT2A;

// 2D Vector; 16 bit floating point components
typedef struct _XMHALF2
{
    HALF x;
    HALF y;

#ifdef __cplusplus

    _XMHALF2() {};
    _XMHALF2(HALF _x, HALF _y);
    _XMHALF2(CONST HALF *pArray);
    _XMHALF2(FLOAT _x, FLOAT _y);
    _XMHALF2(CONST FLOAT *pArray);

    _XMHALF2& operator= (CONST _XMHALF2& Half2);

#endif // __cplusplus

} XMHALF2;

// 2D Vector; 16 bit signed normalized integer components
typedef struct _XMSHORTN2
{
    SHORT x;
    SHORT y;

#ifdef __cplusplus

    _XMSHORTN2() {};
    _XMSHORTN2(SHORT _x, SHORT _y);
    _XMSHORTN2(CONST SHORT *pArray);
    _XMSHORTN2(FLOAT _x, FLOAT _y);
    _XMSHORTN2(CONST FLOAT *pArray);

    _XMSHORTN2& operator= (CONST _XMSHORTN2& ShortN2);

#endif // __cplusplus

} XMSHORTN2;

// 2D Vector; 16 bit signed integer components
typedef struct _XMSHORT2
{
    SHORT x;
    SHORT y;

#ifdef __cplusplus

    _XMSHORT2() {};
    _XMSHORT2(SHORT _x, SHORT _y);
    _XMSHORT2(CONST SHORT *pArray);
    _XMSHORT2(FLOAT _x, FLOAT _y);
    _XMSHORT2(CONST FLOAT *pArray);

    _XMSHORT2& operator= (CONST _XMSHORT2& Short2);

#endif // __cplusplus

} XMSHORT2;

// 2D Vector; 16 bit unsigned normalized integer components
typedef struct _XMUSHORTN2
{
    USHORT x;
    USHORT y;

#ifdef __cplusplus

    _XMUSHORTN2() {};
    _XMUSHORTN2(USHORT _x, USHORT _y);
    _XMUSHORTN2(CONST USHORT *pArray);
    _XMUSHORTN2(FLOAT _x, FLOAT _y);
    _XMUSHORTN2(CONST FLOAT *pArray);

    _XMUSHORTN2& operator= (CONST _XMUSHORTN2& UShortN2);

#endif // __cplusplus

} XMUSHORTN2;

// 2D Vector; 16 bit unsigned integer components
typedef struct _XMUSHORT2
{
    USHORT x;
    USHORT y;

#ifdef __cplusplus

    _XMUSHORT2() {};
    _XMUSHORT2(USHORT _x, USHORT _y);
    _XMUSHORT2(CONST USHORT *pArray);
    _XMUSHORT2(FLOAT _x, FLOAT _y);
    _XMUSHORT2(CONST FLOAT *pArray);

    _XMUSHORT2& operator= (CONST _XMUSHORT2& UShort2);

#endif // __cplusplus

} XMUSHORT2;

// 3D Vector; 32 bit floating point components
typedef struct _XMFLOAT3
{
    FLOAT x;
    FLOAT y;
    FLOAT z;

#ifdef __cplusplus

    _XMFLOAT3() {};
    _XMFLOAT3(FLOAT _x, FLOAT _y, FLOAT _z);
    _XMFLOAT3(CONST FLOAT *pArray);

    _XMFLOAT3& operator= (CONST _XMFLOAT3& Float3);

#endif // __cplusplus

} XMFLOAT3;

// 3D Vector; 32 bit floating point components aligned on a 16 byte boundary
typedef __declspec(align(16)) XMFLOAT3 XMFLOAT3A; 

// 3D Vector; 11-11-10 bit normalized components packed into a 32 bit integer
// The normalized 3D Vector is packed into 32 bits as follows: a 10 bit signed, 
// normalized integer for the z component and 11 bit signed, normalized 
// integers for the x and y components.  The z component is stored in the 
// most significant bits and the x component in the least significant bits
// (Z10Y11X11): [32] zzzzzzzz zzyyyyyy yyyyyxxx xxxxxxxx [0]
typedef struct _XMHENDN3
{
    union
    {
        struct
        {
            INT  x   : 11;    // -1023/1023 to 1023/1023
            INT  y   : 11;    // -1023/1023 to 1023/1023
            INT  z   : 10;    // -511/511 to 511/511
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMHENDN3() {};
    _XMHENDN3(UINT Packed);
    _XMHENDN3(FLOAT _x, FLOAT _y, FLOAT _z);
    _XMHENDN3(CONST FLOAT *pArray);

    operator UINT ();

    _XMHENDN3& operator= (CONST _XMHENDN3& HenDN3);
    _XMHENDN3& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMHENDN3;

// 3D Vector; 11-11-10 bit components packed into a 32 bit integer
// The 3D Vector is packed into 32 bits as follows: a 10 bit signed, 
// integer for the z component and 11 bit signed integers for the 
// x and y components.  The z component is stored in the 
// most significant bits and the x component in the least significant bits
// (Z10Y11X11): [32] zzzzzzzz zzyyyyyy yyyyyxxx xxxxxxxx [0]
typedef struct _XMHEND3
{
    union
    {
        struct
        {
            INT  x   : 11;    // -1023 to 1023
            INT  y   : 11;    // -1023 to 1023
            INT  z   : 10;    // -511 to 511
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMHEND3() {};
    _XMHEND3(UINT Packed);
    _XMHEND3(FLOAT _x, FLOAT _y, FLOAT _z);
    _XMHEND3(CONST FLOAT *pArray);

    operator UINT ();

    _XMHEND3& operator= (CONST _XMHEND3& HenD3);
    _XMHEND3& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMHEND3;

// 3D Vector; 11-11-10 bit normalized components packed into a 32 bit integer
// The normalized 3D Vector is packed into 32 bits as follows: a 10 bit unsigned, 
// normalized integer for the z component and 11 bit unsigned, normalized 
// integers for the x and y components.  The z component is stored in the 
// most significant bits and the x component in the least significant bits
// (Z10Y11X11): [32] zzzzzzzz zzyyyyyy yyyyyxxx xxxxxxxx [0]
typedef struct _XMUHENDN3
{
    union
    {
        struct
        {
            UINT  x  : 11;    // 0/2047 to 2047/2047
            UINT  y  : 11;    // 0/2047 to 2047/2047
            UINT  z  : 10;    // 0/1023 to 1023/1023
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMUHENDN3() {};
    _XMUHENDN3(UINT Packed);
    _XMUHENDN3(FLOAT _x, FLOAT _y, FLOAT _z);
    _XMUHENDN3(CONST FLOAT *pArray);

    operator UINT ();

    _XMUHENDN3& operator= (CONST _XMUHENDN3& UHenDN3);
    _XMUHENDN3& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMUHENDN3;

// 3D Vector; 11-11-10 bit components packed into a 32 bit integer
// The 3D Vector is packed into 32 bits as follows: a 10 bit unsigned
// integer for the z component and 11 bit unsigned integers 
// for the x and y components.  The z component is stored in the 
// most significant bits and the x component in the least significant bits
// (Z10Y11X11): [32] zzzzzzzz zzyyyyyy yyyyyxxx xxxxxxxx [0]
typedef struct _XMUHEND3
{
    union
    {
        struct
        {
            UINT  x  : 11;    // 0 to 2047
            UINT  y  : 11;    // 0 to 2047
            UINT  z  : 10;    // 0 to 1023
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMUHEND3() {};
    _XMUHEND3(UINT Packed);
    _XMUHEND3(FLOAT _x, FLOAT _y, FLOAT _z);
    _XMUHEND3(CONST FLOAT *pArray);

    operator UINT ();

    _XMUHEND3& operator= (CONST _XMUHEND3& UHenD3);
    _XMUHEND3& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMUHEND3;

// 3D Vector; 10-11-11 bit normalized components packed into a 32 bit integer
// The normalized 3D Vector is packed into 32 bits as follows: a 10 bit signed, 
// normalized integer for the x component and 11 bit signed, normalized 
// integers for the y and z components.  The z component is stored in the 
// most significant bits and the x component in the least significant bits
// (Z11Y11X10): [32] zzzzzzzz zzzyyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMDHENN3
{
    union
    {
        struct
        {
            INT  x   : 10;    // -511/511 to 511/511
            INT  y   : 11;    // -1023/1023 to 1023/1023
            INT  z   : 11;    // -1023/1023 to 1023/1023
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMDHENN3() {};
    _XMDHENN3(UINT Packed);
    _XMDHENN3(FLOAT _x, FLOAT _y, FLOAT _z);
    _XMDHENN3(CONST FLOAT *pArray);

    operator UINT ();

    _XMDHENN3& operator= (CONST _XMDHENN3& DHenN3);
    _XMDHENN3& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMDHENN3;

// 3D Vector; 10-11-11 bit components packed into a 32 bit integer
// The 3D Vector is packed into 32 bits as follows: a 10 bit signed, 
// integer for the x component and 11 bit signed integers for the 
// y and z components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (Z11Y11X10): [32] zzzzzzzz zzzyyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMDHEN3
{
    union
    {
        struct
        {
            INT  x   : 10;    // -511 to 511
            INT  y   : 11;    // -1023 to 1023
            INT  z   : 11;    // -1023 to 1023
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMDHEN3() {};
    _XMDHEN3(UINT Packed);
    _XMDHEN3(FLOAT _x, FLOAT _y, FLOAT _z);
    _XMDHEN3(CONST FLOAT *pArray);

    operator UINT ();

    _XMDHEN3& operator= (CONST _XMDHEN3& DHen3);
    _XMDHEN3& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMDHEN3;

// 3D Vector; 10-11-11 bit normalized components packed into a 32 bit integer
// The normalized 3D Vector is packed into 32 bits as follows: a 10 bit unsigned, 
// normalized integer for the x component and 11 bit unsigned, normalized 
// integers for the y and z components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (Z11Y11X10): [32] zzzzzzzz zzzyyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMUDHENN3
{
    union
    {
        struct
        {
            UINT  x  : 10;    // 0/1023 to 1023/1023
            UINT  y  : 11;    // 0/2047 to 2047/2047
            UINT  z  : 11;    // 0/2047 to 2047/2047
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMUDHENN3() {};
    _XMUDHENN3(UINT Packed);
    _XMUDHENN3(FLOAT _x, FLOAT _y, FLOAT _z);
    _XMUDHENN3(CONST FLOAT *pArray);

    operator UINT ();

    _XMUDHENN3& operator= (CONST _XMUDHENN3& UDHenN3);
    _XMUDHENN3& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMUDHENN3;

// 3D Vector; 10-11-11 bit components packed into a 32 bit integer
// The 3D Vector is packed into 32 bits as follows: a 10 bit unsigned, 
// integer for the x component and 11 bit unsigned integers 
// for the y and z components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (Z11Y11X10): [32] zzzzzzzz zzzyyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMUDHEN3
{
    union
    {
        struct
        {
            UINT  x  : 10;    // 0 to 1023
            UINT  y  : 11;    // 0 to 2047
            UINT  z  : 11;    // 0 to 2047
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMUDHEN3() {};
    _XMUDHEN3(UINT Packed);
    _XMUDHEN3(FLOAT _x, FLOAT _y, FLOAT _z);
    _XMUDHEN3(CONST FLOAT *pArray);

    operator UINT ();

    _XMUDHEN3& operator= (CONST _XMUDHEN3& UDHen3);
    _XMUDHEN3& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMUDHEN3;

// 4D Vector; 32 bit floating point components
typedef struct _XMFLOAT4
{
    FLOAT x;
    FLOAT y;
    FLOAT z;
    FLOAT w;

#ifdef __cplusplus

    _XMFLOAT4() {};
    _XMFLOAT4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMFLOAT4(CONST FLOAT *pArray);

    _XMFLOAT4& operator= (CONST _XMFLOAT4& Float4);

#endif // __cplusplus

} XMFLOAT4;

// 4D Vector; 32 bit floating point components aligned on a 16 byte boundary
typedef __declspec(align(16)) XMFLOAT4 XMFLOAT4A;

// 4D Vector; 16 bit floating point components
typedef struct _XMHALF4
{
    HALF x;
    HALF y;
    HALF z;
    HALF w;

#ifdef __cplusplus

    _XMHALF4() {};
    _XMHALF4(HALF _x, HALF _y, HALF _z, HALF _w);
    _XMHALF4(CONST HALF *pArray);
    _XMHALF4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMHALF4(CONST FLOAT *pArray);

    _XMHALF4& operator= (CONST _XMHALF4& Half4);

#endif // __cplusplus

} XMHALF4;

// 4D Vector; 16 bit signed normalized integer components
typedef struct _XMSHORTN4
{
    SHORT x;
    SHORT y;
    SHORT z;
    SHORT w;

#ifdef __cplusplus

    _XMSHORTN4() {};
    _XMSHORTN4(SHORT _x, SHORT _y, SHORT _z, SHORT _w);
    _XMSHORTN4(CONST SHORT *pArray);
    _XMSHORTN4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMSHORTN4(CONST FLOAT *pArray);

    _XMSHORTN4& operator= (CONST _XMSHORTN4& ShortN4);

#endif // __cplusplus

} XMSHORTN4;

// 4D Vector; 16 bit signed integer components
typedef struct _XMSHORT4
{
    SHORT x;
    SHORT y;
    SHORT z;
    SHORT w;

#ifdef __cplusplus

    _XMSHORT4() {};
    _XMSHORT4(SHORT _x, SHORT _y, SHORT _z, SHORT _w);
    _XMSHORT4(CONST SHORT *pArray);
    _XMSHORT4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMSHORT4(CONST FLOAT *pArray);

    _XMSHORT4& operator= (CONST _XMSHORT4& Short4);

#endif // __cplusplus

} XMSHORT4;

// 4D Vector; 16 bit unsigned normalized integer components
typedef struct _XMUSHORTN4
{
    USHORT x;
    USHORT y;
    USHORT z;
    USHORT w;

#ifdef __cplusplus

    _XMUSHORTN4() {};
    _XMUSHORTN4(USHORT _x, USHORT _y, USHORT _z, USHORT _w);
    _XMUSHORTN4(CONST USHORT *pArray);
    _XMUSHORTN4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMUSHORTN4(CONST FLOAT *pArray);

    _XMUSHORTN4& operator= (CONST _XMUSHORTN4& UShortN4);

#endif // __cplusplus

} XMUSHORTN4;

// 4D Vector; 16 bit unsigned integer components
typedef struct _XMUSHORT4
{
    USHORT x;
    USHORT y;
    USHORT z;
    USHORT w;

#ifdef __cplusplus

    _XMUSHORT4() {};
    _XMUSHORT4(USHORT _x, USHORT _y, USHORT _z, USHORT _w);
    _XMUSHORT4(CONST USHORT *pArray);
    _XMUSHORT4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMUSHORT4(CONST FLOAT *pArray);

    _XMUSHORT4& operator= (CONST _XMUSHORT4& UShort4);

#endif // __cplusplus

} XMUSHORT4;

// 4D Vector; 10-10-10-2 bit normalized components packed into a 32 bit integer
// The normalized 4D Vector is packed into 32 bits as follows: a 2 bit unsigned, 
// normalized integer for the w component and 10 bit signed, normalized 
// integers for the z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W2Z10Y10X10): [32] wwzzzzzz zzzzyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMXDECN4
{
    union
    {
        struct
        {
            INT  x   : 10;    // -511/511 to 511/511
            INT  y   : 10;    // -511/511 to 511/511
            INT  z   : 10;    // -511/511 to 511/511
            UINT w   : 2;     //      0/3 to     3/3
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMXDECN4() {};
    _XMXDECN4(UINT Packed);
    _XMXDECN4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMXDECN4(CONST FLOAT *pArray);

    operator UINT ();

    _XMXDECN4& operator= (CONST _XMXDECN4& XDecN4);
    _XMXDECN4& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMXDECN4;

// 4D Vector; 10-10-10-2 bit components packed into a 32 bit integer
// The normalized 4D Vector is packed into 32 bits as follows: a 2 bit unsigned
// integer for the w component and 10 bit signed integers for the 
// z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W2Z10Y10X10): [32] wwzzzzzz zzzzyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMXDEC4
{
    union
    {
        struct
        {
            INT  x   : 10;    // -511 to 511
            INT  y   : 10;    // -511 to 511
            INT  z   : 10;    // -511 to 511
            UINT w   : 2;     //    0 to   3
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMXDEC4() {};
    _XMXDEC4(UINT Packed);
    _XMXDEC4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMXDEC4(CONST FLOAT *pArray);

    operator UINT ();

    _XMXDEC4& operator= (CONST _XMXDEC4& XDec4);
    _XMXDEC4& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMXDEC4;

// 4D Vector; 10-10-10-2 bit normalized components packed into a 32 bit integer
// The normalized 4D Vector is packed into 32 bits as follows: a 2 bit signed, 
// normalized integer for the w component and 10 bit signed, normalized 
// integers for the z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W2Z10Y10X10): [32] wwzzzzzz zzzzyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMDECN4
{
    union
    {
        struct
        {
            INT  x   : 10;    // -511/511 to 511/511
            INT  y   : 10;    // -511/511 to 511/511
            INT  z   : 10;    // -511/511 to 511/511
            INT  w   : 2;     //     -1/1 to     1/1
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMDECN4() {};
    _XMDECN4(UINT Packed);
    _XMDECN4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMDECN4(CONST FLOAT *pArray);

    operator UINT ();

    _XMDECN4& operator= (CONST _XMDECN4& DecN4);
    _XMDECN4& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMDECN4;

// 4D Vector; 10-10-10-2 bit components packed into a 32 bit integer
// The 4D Vector is packed into 32 bits as follows: a 2 bit signed, 
// integer for the w component and 10 bit signed integers for the 
// z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W2Z10Y10X10): [32] wwzzzzzz zzzzyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMDEC4
{
    union
    {
        struct
        {
            INT  x   : 10;    // -511 to 511
            INT  y   : 10;    // -511 to 511
            INT  z   : 10;    // -511 to 511
            INT  w   : 2;     //   -1 to   1
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMDEC4() {};
    _XMDEC4(UINT Packed);
    _XMDEC4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMDEC4(CONST FLOAT *pArray);

    operator UINT ();

    _XMDEC4& operator= (CONST _XMDEC4& Dec4);
    _XMDEC4& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMDEC4;

// 4D Vector; 10-10-10-2 bit normalized components packed into a 32 bit integer
// The normalized 4D Vector is packed into 32 bits as follows: a 2 bit unsigned, 
// normalized integer for the w component and 10 bit unsigned, normalized 
// integers for the z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W2Z10Y10X10): [32] wwzzzzzz zzzzyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMUDECN4
{
    union
    {
        struct
        {
            UINT  x  : 10;    // 0/1023 to 1023/1023
            UINT  y  : 10;    // 0/1023 to 1023/1023
            UINT  z  : 10;    // 0/1023 to 1023/1023
            UINT  w  : 2;     //    0/3 to       3/3
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMUDECN4() {};
    _XMUDECN4(UINT Packed);
    _XMUDECN4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMUDECN4(CONST FLOAT *pArray);

    operator UINT ();

    _XMUDECN4& operator= (CONST _XMUDECN4& UDecN4);
    _XMUDECN4& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMUDECN4;

// 4D Vector; 10-10-10-2 bit components packed into a 32 bit integer
// The 4D Vector is packed into 32 bits as follows: a 2 bit unsigned, 
// integer for the w component and 10 bit unsigned integers 
// for the z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W2Z10Y10X10): [32] wwzzzzzz zzzzyyyy yyyyyyxx xxxxxxxx [0]
typedef struct _XMUDEC4
{
    union
    {
        struct
        {
            UINT  x  : 10;    // 0 to 1023
            UINT  y  : 10;    // 0 to 1023
            UINT  z  : 10;    // 0 to 1023
            UINT  w  : 2;     // 0 to    3
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMUDEC4() {};
    _XMUDEC4(UINT Packed);
    _XMUDEC4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMUDEC4(CONST FLOAT *pArray);

    operator UINT ();

    _XMUDEC4& operator= (CONST _XMUDEC4& UDec4);
    _XMUDEC4& operator= (CONST UINT Packed);

#endif // __cplusplus

} XMUDEC4;

// 4D Vector; 20-20-20-4 bit normalized components packed into a 64 bit integer
// The normalized 4D Vector is packed into 64 bits as follows: a 4 bit unsigned, 
// normalized integer for the w component and 20 bit signed, normalized 
// integers for the z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W4Z20Y20X20): [64] wwwwzzzz zzzzzzzz zzzzzzzz yyyyyyyy yyyyyyyy yyyyxxxx xxxxxxxx xxxxxxxx [0]
typedef struct _XMXICON4
{
    union
    {
        struct
        {
            INT64  x   : 20;    // -524287/524287 to 524287/524287
            INT64  y   : 20;    // -524287/524287 to 524287/524287
            INT64  z   : 20;    // -524287/524287 to 524287/524287
            UINT64 w   : 4;     //           0/15 to         15/15
        };
        UINT64 v;
    };

#ifdef __cplusplus

    _XMXICON4() {};
    _XMXICON4(UINT64 Packed);
    _XMXICON4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMXICON4(CONST FLOAT *pArray);

    operator UINT64 ();

    _XMXICON4& operator= (CONST _XMXICON4& XIcoN4);
    _XMXICON4& operator= (CONST UINT64 Packed);

#endif // __cplusplus

} XMXICON4;

// 4D Vector; 20-20-20-4 bit components packed into a 64 bit integer
// The 4D Vector is packed into 64 bits as follows: a 4 bit unsigned
// integer for the w component and 20 bit signed integers for the 
// z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W4Z20Y20X20): [64] wwwwzzzz zzzzzzzz zzzzzzzz yyyyyyyy yyyyyyyy yyyyxxxx xxxxxxxx xxxxxxxx [0]
typedef struct _XMXICO4
{
    union
    {
        struct
        {
            INT64  x   : 20;    // -524287 to 524287
            INT64  y   : 20;    // -524287 to 524287
            INT64  z   : 20;    // -524287 to 524287
            UINT64 w   : 4;     //       0 to     15
        };
        UINT64 v;
    };

#ifdef __cplusplus

    _XMXICO4() {};
    _XMXICO4(UINT64 Packed);
    _XMXICO4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMXICO4(CONST FLOAT *pArray);

    operator UINT64 ();

    _XMXICO4& operator= (CONST _XMXICO4& XIco4);
    _XMXICO4& operator= (CONST UINT64 Packed);

#endif // __cplusplus

} XMXICO4;

// 4D Vector; 20-20-20-4 bit normalized components packed into a 64 bit integer
// The normalized 4D Vector is packed into 64 bits as follows: a 4 bit signed, 
// normalized integer for the w component and 20 bit signed, normalized 
// integers for the z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W4Z20Y20X20): [64] wwwwzzzz zzzzzzzz zzzzzzzz yyyyyyyy yyyyyyyy yyyyxxxx xxxxxxxx xxxxxxxx [0]
typedef struct _XMICON4
{
    union
    {
        struct
        {
            INT64  x   : 20;    // -524287/524287 to 524287/524287
            INT64  y   : 20;    // -524287/524287 to 524287/524287
            INT64  z   : 20;    // -524287/524287 to 524287/524287
            INT64  w   : 4;     //           -7/7 to           7/7
        };
        UINT64 v;
    };

#ifdef __cplusplus

    _XMICON4() {};
    _XMICON4(UINT64 Packed);
    _XMICON4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMICON4(CONST FLOAT *pArray);

    operator UINT64 ();

    _XMICON4& operator= (CONST _XMICON4& IcoN4);
    _XMICON4& operator= (CONST UINT64 Packed);

#endif // __cplusplus

} XMICON4;

// 4D Vector; 20-20-20-4 bit components packed into a 64 bit integer
// The 4D Vector is packed into 64 bits as follows: a 4 bit signed, 
// integer for the w component and 20 bit signed integers for the 
// z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W4Z20Y20X20): [64] wwwwzzzz zzzzzzzz zzzzzzzz yyyyyyyy yyyyyyyy yyyyxxxx xxxxxxxx xxxxxxxx [0]
typedef struct _XMICO4
{
    union
    {
        struct
        {
            INT64  x   : 20;    // -524287 to 524287
            INT64  y   : 20;    // -524287 to 524287
            INT64  z   : 20;    // -524287 to 524287
            INT64  w   : 4;     //      -7 to      7
        };
        UINT64 v;
    };

#ifdef __cplusplus

    _XMICO4() {};
    _XMICO4(UINT64 Packed);
    _XMICO4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMICO4(CONST FLOAT *pArray);

    operator UINT64 ();

    _XMICO4& operator= (CONST _XMICO4& Ico4);
    _XMICO4& operator= (CONST UINT64 Packed);

#endif // __cplusplus

} XMICO4;

// 4D Vector; 20-20-20-4 bit normalized components packed into a 64 bit integer
// The normalized 4D Vector is packed into 64 bits as follows: a 4 bit unsigned, 
// normalized integer for the w component and 20 bit unsigned, normalized 
// integers for the z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W4Z20Y20X20): [64] wwwwzzzz zzzzzzzz zzzzzzzz yyyyyyyy yyyyyyyy yyyyxxxx xxxxxxxx xxxxxxxx [0]
typedef struct _XMUICON4
{
    union
    {
        struct
        {
            UINT64  x  : 20;    // 0/1048575 to 1048575/1048575
            UINT64  y  : 20;    // 0/1048575 to 1048575/1048575
            UINT64  z  : 20;    // 0/1048575 to 1048575/1048575
            UINT64  w  : 4;     //      0/15 to           15/15
        };
        UINT64 v;
    };

#ifdef __cplusplus

    _XMUICON4() {};
    _XMUICON4(UINT64 Packed);
    _XMUICON4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMUICON4(CONST FLOAT *pArray);

    operator UINT64 ();

    _XMUICON4& operator= (CONST _XMUICON4& UIcoN4);
    _XMUICON4& operator= (CONST UINT64 Packed);

#endif // __cplusplus

} XMUICON4;

// 4D Vector; 20-20-20-4 bit components packed into a 64 bit integer
// The 4D Vector is packed into 64 bits as follows: a 4 bit unsigned 
// integer for the w component and 20 bit unsigned integers for the 
// z, y, and x components.  The w component is stored in the 
// most significant bits and the x component in the least significant bits
// (W4Z20Y20X20): [64] wwwwzzzz zzzzzzzz zzzzzzzz yyyyyyyy yyyyyyyy yyyyxxxx xxxxxxxx xxxxxxxx [0]
typedef struct _XMUICO4
{
    union
    {
        struct
        {
            UINT64  x  : 20;    // 0 to 1048575
            UINT64  y  : 20;    // 0 to 1048575
            UINT64  z  : 20;    // 0 to 1048575
            UINT64  w  : 4;     // 0 to      15
        };
        UINT64 v;
    };

#ifdef __cplusplus

    _XMUICO4() {};
    _XMUICO4(UINT64 Packed);
    _XMUICO4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMUICO4(CONST FLOAT *pArray);

    operator UINT64 ();

    _XMUICO4& operator= (CONST _XMUICO4& UIco4);
    _XMUICO4& operator= (CONST UINT64 Packed);

#endif // __cplusplus

} XMUICO4;

// ARGB Color; 8-8-8-8 bit unsigned normalized integer components packed into
// a 32 bit integer.  The normalized color is packed into 32 bits using 8 bit
// unsigned, normalized integers for the alpha, red, green, and blue components.
// The alpha component is stored in the most significant bits and the blue
// component in the least significant bits (A8R8G8B8):
// [32] aaaaaaaa rrrrrrrr gggggggg bbbbbbbb [0]
typedef struct _XMCOLOR
{
    union
    {
        struct
        {
            UINT b    : 8;  // Blue:    0/255 to 255/255
            UINT g    : 8;  // Green:   0/255 to 255/255
            UINT r    : 8;  // Red:     0/255 to 255/255
            UINT a    : 8;  // Alpha:   0/255 to 255/255
        };
        UINT c;
    };

#ifdef __cplusplus

    _XMCOLOR() {};
    _XMCOLOR(UINT Color);
    _XMCOLOR(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMCOLOR(CONST FLOAT *pArray);

    operator UINT ();

    _XMCOLOR& operator= (CONST _XMCOLOR& Color);
    _XMCOLOR& operator= (CONST UINT Color);

#endif // __cplusplus

} XMCOLOR;

// 4D Vector; 8 bit signed normalized integer components
typedef struct _XMBYTEN4
{
    union
    {
        struct
        {
            CHAR x;
            CHAR y;
            CHAR z;
            CHAR w;
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMBYTEN4() {};
    _XMBYTEN4(CHAR _x, CHAR _y, CHAR _z, CHAR _w);
    _XMBYTEN4(UINT _v);
    _XMBYTEN4(CONST CHAR *pArray);
    _XMBYTEN4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMBYTEN4(CONST FLOAT *pArray);

    _XMBYTEN4& operator= (CONST _XMBYTEN4& ByteN4);

#endif // __cplusplus

} XMBYTEN4;

// 4D Vector; 8 bit signed integer components
typedef struct _XMBYTE4
{
    union
    {
        struct
        {
            CHAR x;
            CHAR y;
            CHAR z;
            CHAR w;
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMBYTE4() {};
    _XMBYTE4(CHAR _x, CHAR _y, CHAR _z, CHAR _w);
    _XMBYTE4(UINT _v);
    _XMBYTE4(CONST CHAR *pArray);
    _XMBYTE4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMBYTE4(CONST FLOAT *pArray);

    _XMBYTE4& operator= (CONST _XMBYTE4& Byte4);

#endif // __cplusplus

} XMBYTE4;

// 4D Vector; 8 bit unsigned normalized integer components
typedef struct _XMUBYTEN4
{
    union
    {
        struct
        {
            BYTE x;
            BYTE y;
            BYTE z;
            BYTE w;
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMUBYTEN4() {};
    _XMUBYTEN4(BYTE _x, BYTE _y, BYTE _z, BYTE _w);
    _XMUBYTEN4(UINT _v);
    _XMUBYTEN4(CONST BYTE *pArray);
    _XMUBYTEN4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMUBYTEN4(CONST FLOAT *pArray);

    _XMUBYTEN4& operator= (CONST _XMUBYTEN4& UByteN4);

#endif // __cplusplus

} XMUBYTEN4;

// 4D Vector; 8 bit unsigned integer components
typedef struct _XMUBYTE4
{
    union
    {
        struct
        {
            BYTE x;
            BYTE y;
            BYTE z;
            BYTE w;
        };
        UINT v;
    };

#ifdef __cplusplus

    _XMUBYTE4() {};
    _XMUBYTE4(BYTE _x, BYTE _y, BYTE _z, BYTE _w);
    _XMUBYTE4(UINT _v);
    _XMUBYTE4(CONST BYTE *pArray);
    _XMUBYTE4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w);
    _XMUBYTE4(CONST FLOAT *pArray);

    _XMUBYTE4& operator= (CONST _XMUBYTE4& UByte4);

#endif // __cplusplus

} XMUBYTE4;

// 3x3 Matrix: 32 bit floating point components
typedef struct _XMFLOAT3X3
{
    union
    {
        struct
        {
            FLOAT _11, _12, _13;
            FLOAT _21, _22, _23;
            FLOAT _31, _32, _33;
        };
        FLOAT m[3][3];
    };

#ifdef __cplusplus

                _XMFLOAT3X3() {};
                _XMFLOAT3X3(FLOAT m00, FLOAT m01, FLOAT m02,
                         FLOAT m10, FLOAT m11, FLOAT m12,
                         FLOAT m20, FLOAT m21, FLOAT m22);
                _XMFLOAT3X3(CONST FLOAT *pArray);

    FLOAT       operator() (UINT Row, UINT Column) CONST;
    FLOAT&      operator() (UINT Row, UINT Column);

    _XMFLOAT3X3& operator= (CONST _XMFLOAT3X3& Float3x3);

#endif // __cplusplus

} XMFLOAT3X3;

// 4x3 Matrix: 32 bit floating point components
typedef struct _XMFLOAT4X3
{
    union
    {
        struct
        {
            FLOAT _11, _12, _13;
            FLOAT _21, _22, _23;
            FLOAT _31, _32, _33;
            FLOAT _41, _42, _43;
        };
        FLOAT m[4][3];
    };

#ifdef __cplusplus

                _XMFLOAT4X3() {};
                _XMFLOAT4X3(FLOAT m00, FLOAT m01, FLOAT m02,
                         FLOAT m10, FLOAT m11, FLOAT m12,
                         FLOAT m20, FLOAT m21, FLOAT m22,
                         FLOAT m30, FLOAT m31, FLOAT m32);
                _XMFLOAT4X3(CONST FLOAT *pArray);

    FLOAT       operator() (UINT Row, UINT Column) CONST;
    FLOAT&      operator() (UINT Row, UINT Column);

    _XMFLOAT4X3& operator= (CONST _XMFLOAT4X3& Float4x3);

#endif // __cplusplus

} XMFLOAT4X3;

// 4x3 Matrix: 32 bit floating point components aligned on a 16 byte boundary
typedef __declspec(align(16)) XMFLOAT4X3 XMFLOAT4X3A;

// 4x4 Matrix: 32 bit floating point components
typedef struct _XMFLOAT4X4
{
    union
    {
        struct
        {
            FLOAT _11, _12, _13, _14;
            FLOAT _21, _22, _23, _24;
            FLOAT _31, _32, _33, _34;
            FLOAT _41, _42, _43, _44;
        };
        FLOAT m[4][4];
    };

#ifdef __cplusplus

                _XMFLOAT4X4() {};
                _XMFLOAT4X4(FLOAT m00, FLOAT m01, FLOAT m02, FLOAT m03,
                         FLOAT m10, FLOAT m11, FLOAT m12, FLOAT m13,
                         FLOAT m20, FLOAT m21, FLOAT m22, FLOAT m23,
                         FLOAT m30, FLOAT m31, FLOAT m32, FLOAT m33);
                _XMFLOAT4X4(CONST FLOAT *pArray);

    FLOAT       operator() (UINT Row, UINT Column) CONST;
    FLOAT&      operator() (UINT Row, UINT Column);

    _XMFLOAT4X4& operator= (CONST _XMFLOAT4X4& Float4x4);

#endif // __cplusplus

} XMFLOAT4X4;

// 4x4 Matrix: 32 bit floating point components aligned on a 16 byte boundary
typedef __declspec(align(16)) XMFLOAT4X4 XMFLOAT4X4A;

#if !defined(_XM_X86_) && !defined(_XM_X64_)
#pragma bitfield_order(pop)
#endif // !_XM_X86_ && !_XM_X64_

#pragma warning(pop)

/****************************************************************************
 *
 * Data conversion operations
 *
 ****************************************************************************/

#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_VMX128_INTRINSICS_)
#else
XMVECTOR        XMConvertVectorIntToFloat(FXMVECTOR VInt, UINT DivExponent);
XMVECTOR        XMConvertVectorFloatToInt(FXMVECTOR VFloat, UINT MulExponent);
XMVECTOR        XMConvertVectorUIntToFloat(FXMVECTOR VUInt, UINT DivExponent);
XMVECTOR        XMConvertVectorFloatToUInt(FXMVECTOR VFloat, UINT MulExponent);
#endif

FLOAT           XMConvertHalfToFloat(HALF Value);
FLOAT*          XMConvertHalfToFloatStream(__out_bcount(sizeof(FLOAT)+OutputStride*(HalfCount-1)) FLOAT* pOutputStream,
                                           __in UINT OutputStride,
                                           __in_bcount(sizeof(HALF)+InputStride*(HalfCount-1)) CONST HALF* pInputStream,
                                           __in UINT InputStride, __in UINT HalfCount);
HALF            XMConvertFloatToHalf(FLOAT Value);
HALF*           XMConvertFloatToHalfStream(__out_bcount(sizeof(HALF)+OutputStride*(FloatCount-1)) HALF* pOutputStream,
                                           __in UINT OutputStride,
                                           __in_bcount(sizeof(FLOAT)+InputStride*(FloatCount-1)) CONST FLOAT* pInputStream,
                                           __in UINT InputStride, __in UINT FloatCount);

#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_VMX128_INTRINSICS_)
#else
XMVECTOR XMVectorSetBinaryConstant(UINT C0, UINT C1, UINT C2, UINT C3);
XMVECTOR XMVectorSplatConstant(INT IntConstant, UINT DivExponent);
XMVECTOR XMVectorSplatConstantInt(INT IntConstant);
#endif

/****************************************************************************
 *
 * Load operations
 *
 ****************************************************************************/

XMVECTOR        XMLoadInt(__in CONST UINT* pSource);
XMVECTOR        XMLoadFloat(__in CONST FLOAT* pSource);

XMVECTOR        XMLoadInt2(__in CONST UINT* pSource);
XMVECTOR        XMLoadInt2A(__in CONST UINT* PSource);
XMVECTOR        XMLoadFloat2(__in CONST XMFLOAT2* pSource);
XMVECTOR        XMLoadFloat2A(__in CONST XMFLOAT2A* pSource);
XMVECTOR        XMLoadHalf2(__in CONST XMHALF2* pSource);
XMVECTOR        XMLoadShortN2(__in CONST XMSHORTN2* pSource);
XMVECTOR        XMLoadShort2(__in CONST XMSHORT2* pSource);
XMVECTOR        XMLoadUShortN2(__in CONST XMUSHORTN2* pSource);
XMVECTOR        XMLoadUShort2(__in CONST XMUSHORT2* pSource);

XMVECTOR        XMLoadInt3(__in CONST UINT* pSource);
XMVECTOR        XMLoadInt3A(__in CONST UINT* pSource);
XMVECTOR        XMLoadFloat3(__in CONST XMFLOAT3* pSource);
XMVECTOR        XMLoadFloat3A(__in CONST XMFLOAT3A* pSource);
XMVECTOR        XMLoadHenDN3(__in CONST XMHENDN3* pSource);
XMVECTOR        XMLoadHenD3(__in CONST XMHEND3* pSource);
XMVECTOR        XMLoadUHenDN3(__in CONST XMUHENDN3* pSource);
XMVECTOR        XMLoadUHenD3(__in CONST XMUHEND3* pSource);
XMVECTOR        XMLoadDHenN3(__in CONST XMDHENN3* pSource);
XMVECTOR        XMLoadDHen3(__in CONST XMDHEN3* pSource);
XMVECTOR        XMLoadUDHenN3(__in CONST XMUDHENN3* pSource);
XMVECTOR        XMLoadUDHen3(__in CONST XMUDHEN3* pSource);

XMVECTOR        XMLoadInt4(__in CONST UINT* pSource);
XMVECTOR        XMLoadInt4A(__in CONST UINT* pSource);
XMVECTOR        XMLoadFloat4(__in CONST XMFLOAT4* pSource);
XMVECTOR        XMLoadFloat4A(__in CONST XMFLOAT4A* pSource);
XMVECTOR        XMLoadHalf4(__in CONST XMHALF4* pSource);
XMVECTOR        XMLoadShortN4(__in CONST XMSHORTN4* pSource);
XMVECTOR        XMLoadShort4(__in CONST XMSHORT4* pSource);
XMVECTOR        XMLoadUShortN4(__in CONST XMUSHORTN4* pSource);
XMVECTOR        XMLoadUShort4(__in CONST XMUSHORT4* pSource);
XMVECTOR        XMLoadXIcoN4(__in CONST XMXICON4* pSource);
XMVECTOR        XMLoadXIco4(__in CONST XMXICO4* pSource);
XMVECTOR        XMLoadIcoN4(__in CONST XMICON4* pSource);
XMVECTOR        XMLoadIco4(__in CONST XMICO4* pSource);
XMVECTOR        XMLoadUIcoN4(__in CONST XMUICON4* pSource);
XMVECTOR        XMLoadUIco4(__in CONST XMUICO4* pSource);
XMVECTOR        XMLoadXDecN4(__in CONST XMXDECN4* pSource);
XMVECTOR        XMLoadXDec4(__in CONST XMXDEC4* pSource);
XMVECTOR        XMLoadDecN4(__in CONST XMDECN4* pSource);
XMVECTOR        XMLoadDec4(__in CONST XMDEC4* pSource);
XMVECTOR        XMLoadUDecN4(__in CONST XMUDECN4* pSource);
XMVECTOR        XMLoadUDec4(__in CONST XMUDEC4* pSource);
XMVECTOR        XMLoadByteN4(__in CONST XMBYTEN4* pSource);
XMVECTOR        XMLoadByte4(__in CONST XMBYTE4* pSource);
XMVECTOR        XMLoadUByteN4(__in CONST XMUBYTEN4* pSource);
XMVECTOR        XMLoadUByte4(__in CONST XMUBYTE4* pSource);
XMVECTOR        XMLoadColor(__in CONST XMCOLOR* pSource);

XMMATRIX        XMLoadFloat3x3(__in CONST XMFLOAT3X3* pSource);
XMMATRIX        XMLoadFloat4x3(__in CONST XMFLOAT4X3* pSource);
XMMATRIX        XMLoadFloat4x3A(__in CONST XMFLOAT4X3A* pSource);
XMMATRIX        XMLoadFloat4x4(__in CONST XMFLOAT4X4* pSource);
XMMATRIX        XMLoadFloat4x4A(__in CONST XMFLOAT4X4A* pSource);

/****************************************************************************
 *
 * Store operations
 *
 ****************************************************************************/

VOID            XMStoreInt(__out UINT* pDestination, FXMVECTOR V);
VOID            XMStoreFloat(__out FLOAT* pDestination, FXMVECTOR V);

VOID            XMStoreInt2(__out UINT* pDestination, FXMVECTOR V);
VOID            XMStoreInt2A(__out UINT* pDestination, FXMVECTOR V);
VOID            XMStoreFloat2(__out XMFLOAT2* pDestination, FXMVECTOR V);
VOID            XMStoreFloat2A(__out XMFLOAT2A* pDestination, FXMVECTOR V);
VOID            XMStoreHalf2(__out XMHALF2* pDestination, FXMVECTOR V);
VOID            XMStoreShortN2(__out XMSHORTN2* pDestination, FXMVECTOR V);
VOID            XMStoreShort2(__out XMSHORT2* pDestination, FXMVECTOR V);
VOID            XMStoreUShortN2(__out XMUSHORTN2* pDestination, FXMVECTOR V);
VOID            XMStoreUShort2(__out XMUSHORT2* pDestination, FXMVECTOR V);

VOID            XMStoreInt3(__out UINT* pDestination, FXMVECTOR V);
VOID            XMStoreInt3A(__out UINT* pDestination, FXMVECTOR V);
VOID            XMStoreFloat3(__out XMFLOAT3* pDestination, FXMVECTOR V);
VOID            XMStoreFloat3A(__out XMFLOAT3A* pDestination, FXMVECTOR V);
VOID            XMStoreHenDN3(__out XMHENDN3* pDestination, FXMVECTOR V);
VOID            XMStoreHenD3(__out XMHEND3* pDestination, FXMVECTOR V);
VOID            XMStoreUHenDN3(__out XMUHENDN3* pDestination, FXMVECTOR V);
VOID            XMStoreUHenD3(__out XMUHEND3* pDestination, FXMVECTOR V);
VOID            XMStoreDHenN3(__out XMDHENN3* pDestination, FXMVECTOR V);
VOID            XMStoreDHen3(__out XMDHEN3* pDestination, FXMVECTOR V);
VOID            XMStoreUDHenN3(__out XMUDHENN3* pDestination, FXMVECTOR V);
VOID            XMStoreUDHen3(__out XMUDHEN3* pDestination, FXMVECTOR V);

VOID            XMStoreInt4(__out UINT* pDestination, FXMVECTOR V);
VOID            XMStoreInt4A(__out UINT* pDestination, FXMVECTOR V);
VOID            XMStoreInt4NC(__out UINT* pDestination, FXMVECTOR V);
VOID            XMStoreFloat4(__out XMFLOAT4* pDestination, FXMVECTOR V);
VOID            XMStoreFloat4A(__out XMFLOAT4A* pDestination, FXMVECTOR V);
VOID            XMStoreFloat4NC(__out XMFLOAT4* pDestination, FXMVECTOR V);
VOID            XMStoreHalf4(__out XMHALF4* pDestination, FXMVECTOR V);
VOID            XMStoreShortN4(__out XMSHORTN4* pDestination, FXMVECTOR V);
VOID            XMStoreShort4(__out XMSHORT4* pDestination, FXMVECTOR V);
VOID            XMStoreUShortN4(__out XMUSHORTN4* pDestination, FXMVECTOR V);
VOID            XMStoreUShort4(__out XMUSHORT4* pDestination, FXMVECTOR V);
VOID            XMStoreXIcoN4(__out XMXICON4* pDestination, FXMVECTOR V);
VOID            XMStoreXIco4(__out XMXICO4* pDestination, FXMVECTOR V);
VOID            XMStoreIcoN4(__out XMICON4* pDestination, FXMVECTOR V);
VOID            XMStoreIco4(__out XMICO4* pDestination, FXMVECTOR V);
VOID            XMStoreUIcoN4(__out XMUICON4* pDestination, FXMVECTOR V);
VOID            XMStoreUIco4(__out XMUICO4* pDestination, FXMVECTOR V);
VOID            XMStoreXDecN4(__out XMXDECN4* pDestination, FXMVECTOR V);
VOID            XMStoreXDec4(__out XMXDEC4* pDestination, FXMVECTOR V);
VOID            XMStoreDecN4(__out XMDECN4* pDestination, FXMVECTOR V);
VOID            XMStoreDec4(__out XMDEC4* pDestination, FXMVECTOR V);
VOID            XMStoreUDecN4(__out XMUDECN4* pDestination, FXMVECTOR V);
VOID            XMStoreUDec4(__out XMUDEC4* pDestination, FXMVECTOR V);
VOID            XMStoreByteN4(__out XMBYTEN4* pDestination, FXMVECTOR V);
VOID            XMStoreByte4(__out XMBYTE4* pDestination, FXMVECTOR V);
VOID            XMStoreUByteN4(__out XMUBYTEN4* pDestination, FXMVECTOR V);
VOID            XMStoreUByte4(__out XMUBYTE4* pDestination, FXMVECTOR V);
VOID            XMStoreColor(__out XMCOLOR* pDestination, FXMVECTOR V);

VOID            XMStoreFloat3x3(__out XMFLOAT3X3* pDestination, CXMMATRIX M);
VOID            XMStoreFloat3x3NC(__out XMFLOAT3X3* pDestination, CXMMATRIX M);
VOID            XMStoreFloat4x3(__out XMFLOAT4X3* pDestination, CXMMATRIX M);
VOID            XMStoreFloat4x3A(__out XMFLOAT4X3A* pDestination, CXMMATRIX M);
VOID            XMStoreFloat4x3NC(__out XMFLOAT4X3* pDestination, CXMMATRIX M);
VOID            XMStoreFloat4x4(__out XMFLOAT4X4* pDestination, CXMMATRIX M);
VOID            XMStoreFloat4x4A(__out XMFLOAT4X4A* pDestination, CXMMATRIX M);
VOID            XMStoreFloat4x4NC(__out XMFLOAT4X4* pDestination, CXMMATRIX M);

/****************************************************************************
 *
 * General vector operations
 *
 ****************************************************************************/

XMVECTOR        XMVectorZero();
XMVECTOR        XMVectorSet(FLOAT x, FLOAT y, FLOAT z, FLOAT w);
XMVECTOR        XMVectorSetInt(UINT x, UINT y, UINT z, UINT w);
XMVECTOR        XMVectorReplicate(FLOAT Value);
XMVECTOR        XMVectorReplicatePtr(__in CONST FLOAT *pValue);
XMVECTOR        XMVectorReplicateInt(UINT Value);
XMVECTOR        XMVectorReplicateIntPtr(__in CONST UINT *pValue);
XMVECTOR        XMVectorTrueInt();
XMVECTOR        XMVectorFalseInt();
XMVECTOR        XMVectorSplatX(FXMVECTOR V);
XMVECTOR        XMVectorSplatY(FXMVECTOR V);
XMVECTOR        XMVectorSplatZ(FXMVECTOR V);
XMVECTOR        XMVectorSplatW(FXMVECTOR V);
XMVECTOR        XMVectorSplatOne();
XMVECTOR        XMVectorSplatInfinity();
XMVECTOR        XMVectorSplatQNaN();
XMVECTOR        XMVectorSplatEpsilon();
XMVECTOR        XMVectorSplatSignMask();

FLOAT           XMVectorGetByIndex(FXMVECTOR V,UINT i);
FLOAT           XMVectorGetX(FXMVECTOR V);
FLOAT           XMVectorGetY(FXMVECTOR V);
FLOAT           XMVectorGetZ(FXMVECTOR V);
FLOAT           XMVectorGetW(FXMVECTOR V);

VOID            XMVectorGetByIndexPtr(__out FLOAT *f, FXMVECTOR V, UINT i);
VOID            XMVectorGetXPtr(__out FLOAT *x, FXMVECTOR V);
VOID            XMVectorGetYPtr(__out FLOAT *y, FXMVECTOR V);
VOID            XMVectorGetZPtr(__out FLOAT *z, FXMVECTOR V);
VOID            XMVectorGetWPtr(__out FLOAT *w, FXMVECTOR V);

UINT            XMVectorGetIntByIndex(FXMVECTOR V,UINT i);
UINT            XMVectorGetIntX(FXMVECTOR V);
UINT            XMVectorGetIntY(FXMVECTOR V);
UINT            XMVectorGetIntZ(FXMVECTOR V);
UINT            XMVectorGetIntW(FXMVECTOR V);

VOID            XMVectorGetIntByIndexPtr(__out UINT *x,FXMVECTOR V, UINT i);
VOID            XMVectorGetIntXPtr(__out UINT *x, FXMVECTOR V);
VOID            XMVectorGetIntYPtr(__out UINT *y, FXMVECTOR V);
VOID            XMVectorGetIntZPtr(__out UINT *z, FXMVECTOR V);
VOID            XMVectorGetIntWPtr(__out UINT *w, FXMVECTOR V);

XMVECTOR        XMVectorSetByIndex(FXMVECTOR V,FLOAT f,UINT i);
XMVECTOR        XMVectorSetX(FXMVECTOR V, FLOAT x);
XMVECTOR        XMVectorSetY(FXMVECTOR V, FLOAT y);
XMVECTOR        XMVectorSetZ(FXMVECTOR V, FLOAT z);
XMVECTOR        XMVectorSetW(FXMVECTOR V, FLOAT w);

XMVECTOR        XMVectorSetByIndexPtr(FXMVECTOR V, __in CONST FLOAT *f, UINT i);
XMVECTOR        XMVectorSetXPtr(FXMVECTOR V, __in CONST FLOAT *x);
XMVECTOR        XMVectorSetYPtr(FXMVECTOR V, __in CONST FLOAT *y);
XMVECTOR        XMVectorSetZPtr(FXMVECTOR V, __in CONST FLOAT *z);
XMVECTOR        XMVectorSetWPtr(FXMVECTOR V, __in CONST FLOAT *w);

XMVECTOR        XMVectorSetIntByIndex(FXMVECTOR V, UINT x,UINT i);
XMVECTOR        XMVectorSetIntX(FXMVECTOR V, UINT x);
XMVECTOR        XMVectorSetIntY(FXMVECTOR V, UINT y);
XMVECTOR        XMVectorSetIntZ(FXMVECTOR V, UINT z);
XMVECTOR        XMVectorSetIntW(FXMVECTOR V, UINT w);

XMVECTOR        XMVectorSetIntByIndexPtr(FXMVECTOR V, __in CONST UINT *x, UINT i);
XMVECTOR        XMVectorSetIntXPtr(FXMVECTOR V, __in CONST UINT *x);
XMVECTOR        XMVectorSetIntYPtr(FXMVECTOR V, __in CONST UINT *y);
XMVECTOR        XMVectorSetIntZPtr(FXMVECTOR V, __in CONST UINT *z);
XMVECTOR        XMVectorSetIntWPtr(FXMVECTOR V, __in CONST UINT *w);

XMVECTOR        XMVectorPermuteControl(UINT ElementIndex0, UINT ElementIndex1, UINT ElementIndex2, UINT ElementIndex3);
XMVECTOR        XMVectorPermute(FXMVECTOR V1, FXMVECTOR V2, FXMVECTOR Control);
XMVECTOR        XMVectorSelectControl(UINT VectorIndex0, UINT VectorIndex1, UINT VectorIndex2, UINT VectorIndex3);
XMVECTOR        XMVectorSelect(FXMVECTOR V1, FXMVECTOR V2, FXMVECTOR Control);
XMVECTOR        XMVectorMergeXY(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorMergeZW(FXMVECTOR V1, FXMVECTOR V2);

#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_VMX128_INTRINSICS_)
#else
XMVECTOR XMVectorShiftLeft(FXMVECTOR V1, FXMVECTOR V2, UINT Elements);
XMVECTOR XMVectorRotateLeft(FXMVECTOR V, UINT Elements);
XMVECTOR XMVectorRotateRight(FXMVECTOR V, UINT Elements);
XMVECTOR XMVectorSwizzle(FXMVECTOR V, UINT E0, UINT E1, UINT E2, UINT E3);
XMVECTOR XMVectorInsert(FXMVECTOR VD, FXMVECTOR VS, UINT VSLeftRotateElements,
                        UINT Select0, UINT Select1, UINT Select2, UINT Select3);
#endif

XMVECTOR        XMVectorEqual(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorEqualR(__out UINT* pCR, FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorEqualInt(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorEqualIntR(__out UINT* pCR, FXMVECTOR V, FXMVECTOR V2);
XMVECTOR        XMVectorNearEqual(FXMVECTOR V1, FXMVECTOR V2, FXMVECTOR Epsilon);
XMVECTOR        XMVectorNotEqual(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorNotEqualInt(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorGreater(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorGreaterR(__out UINT* pCR, FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorGreaterOrEqual(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorGreaterOrEqualR(__out UINT* pCR, FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorLess(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorLessOrEqual(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorInBounds(FXMVECTOR V, FXMVECTOR Bounds);
XMVECTOR        XMVectorInBoundsR(__out UINT* pCR, FXMVECTOR V, FXMVECTOR Bounds);

XMVECTOR        XMVectorIsNaN(FXMVECTOR V);
XMVECTOR        XMVectorIsInfinite(FXMVECTOR V);

XMVECTOR        XMVectorMin(FXMVECTOR V1,FXMVECTOR V2);
XMVECTOR        XMVectorMax(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorRound(FXMVECTOR V);
XMVECTOR        XMVectorTruncate(FXMVECTOR V);
XMVECTOR        XMVectorFloor(FXMVECTOR V);
XMVECTOR        XMVectorCeiling(FXMVECTOR V);
XMVECTOR        XMVectorClamp(FXMVECTOR V, FXMVECTOR Min, FXMVECTOR Max);
XMVECTOR        XMVectorSaturate(FXMVECTOR V);

XMVECTOR        XMVectorAndInt(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorAndCInt(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorOrInt(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorNorInt(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorXorInt(FXMVECTOR V1, FXMVECTOR V2);

XMVECTOR        XMVectorNegate(FXMVECTOR V);
XMVECTOR        XMVectorAdd(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorAddAngles(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorSubtract(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorSubtractAngles(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorMultiply(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorMultiplyAdd(FXMVECTOR V1, FXMVECTOR V2, FXMVECTOR V3);
XMVECTOR        XMVectorNegativeMultiplySubtract(FXMVECTOR V1, FXMVECTOR V2, FXMVECTOR V3);
XMVECTOR        XMVectorScale(FXMVECTOR V, FLOAT ScaleFactor);
XMVECTOR        XMVectorReciprocalEst(FXMVECTOR V);
XMVECTOR        XMVectorReciprocal(FXMVECTOR V);
XMVECTOR        XMVectorSqrtEst(FXMVECTOR V);
XMVECTOR        XMVectorSqrt(FXMVECTOR V);
XMVECTOR        XMVectorReciprocalSqrtEst(FXMVECTOR V);
XMVECTOR        XMVectorReciprocalSqrt(FXMVECTOR V);
XMVECTOR        XMVectorExpEst(FXMVECTOR V);
XMVECTOR        XMVectorExp(FXMVECTOR V);
XMVECTOR        XMVectorLogEst(FXMVECTOR V);
XMVECTOR        XMVectorLog(FXMVECTOR V);
XMVECTOR        XMVectorPowEst(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorPow(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorAbs(FXMVECTOR V);
XMVECTOR        XMVectorMod(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVectorModAngles(FXMVECTOR Angles);
XMVECTOR        XMVectorSin(FXMVECTOR V);
XMVECTOR        XMVectorSinEst(FXMVECTOR V);
XMVECTOR        XMVectorCos(FXMVECTOR V);
XMVECTOR        XMVectorCosEst(FXMVECTOR V);
VOID            XMVectorSinCos(__out XMVECTOR* pSin, __out XMVECTOR* pCos, FXMVECTOR V);
VOID            XMVectorSinCosEst(__out XMVECTOR* pSin, __out XMVECTOR* pCos, FXMVECTOR V);
XMVECTOR        XMVectorTan(FXMVECTOR V);
XMVECTOR        XMVectorTanEst(FXMVECTOR V);
XMVECTOR        XMVectorSinH(FXMVECTOR V);
XMVECTOR        XMVectorSinHEst(FXMVECTOR V);
XMVECTOR        XMVectorCosH(FXMVECTOR V);
XMVECTOR        XMVectorCosHEst(FXMVECTOR V);
XMVECTOR        XMVectorTanH(FXMVECTOR V);
XMVECTOR        XMVectorTanHEst(FXMVECTOR V);
XMVECTOR        XMVectorASin(FXMVECTOR V);
XMVECTOR        XMVectorASinEst(FXMVECTOR V);
XMVECTOR        XMVectorACos(FXMVECTOR V);
XMVECTOR        XMVectorACosEst(FXMVECTOR V);
XMVECTOR        XMVectorATan(FXMVECTOR V);
XMVECTOR        XMVectorATanEst(FXMVECTOR V);
XMVECTOR        XMVectorATan2(FXMVECTOR Y, FXMVECTOR X);
XMVECTOR        XMVectorATan2Est(FXMVECTOR Y, FXMVECTOR X);
XMVECTOR        XMVectorLerp(FXMVECTOR V0, FXMVECTOR V1, FLOAT t);
XMVECTOR        XMVectorLerpV(FXMVECTOR V0, FXMVECTOR V1, FXMVECTOR T);
XMVECTOR        XMVectorHermite(FXMVECTOR Position0, FXMVECTOR Tangent0, FXMVECTOR Position1, CXMVECTOR Tangent1, FLOAT t);
XMVECTOR        XMVectorHermiteV(FXMVECTOR Position0, FXMVECTOR Tangent0, FXMVECTOR Position1, CXMVECTOR Tangent1, CXMVECTOR T);
XMVECTOR        XMVectorCatmullRom(FXMVECTOR Position0, FXMVECTOR Position1, FXMVECTOR Position2, CXMVECTOR Position3, FLOAT t);
XMVECTOR        XMVectorCatmullRomV(FXMVECTOR Position0, FXMVECTOR Position1, FXMVECTOR Position2, CXMVECTOR Position3, CXMVECTOR T);
XMVECTOR        XMVectorBaryCentric(FXMVECTOR Position0, FXMVECTOR Position1, FXMVECTOR Position2, FLOAT f, FLOAT g);
XMVECTOR        XMVectorBaryCentricV(FXMVECTOR Position0, FXMVECTOR Position1, FXMVECTOR Position2, CXMVECTOR F, CXMVECTOR G);

/****************************************************************************
 *
 * 2D vector operations
 *
 ****************************************************************************/


BOOL            XMVector2Equal(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector2EqualR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector2EqualInt(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector2EqualIntR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector2NearEqual(FXMVECTOR V1, FXMVECTOR V2, FXMVECTOR Epsilon);
BOOL            XMVector2NotEqual(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector2NotEqualInt(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector2Greater(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector2GreaterR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector2GreaterOrEqual(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector2GreaterOrEqualR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector2Less(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector2LessOrEqual(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector2InBounds(FXMVECTOR V, FXMVECTOR Bounds);
UINT            XMVector2InBoundsR(FXMVECTOR V, FXMVECTOR Bounds);

BOOL            XMVector2IsNaN(FXMVECTOR V);
BOOL            XMVector2IsInfinite(FXMVECTOR V);

XMVECTOR        XMVector2Dot(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVector2Cross(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVector2LengthSq(FXMVECTOR V);
XMVECTOR        XMVector2ReciprocalLengthEst(FXMVECTOR V);
XMVECTOR        XMVector2ReciprocalLength(FXMVECTOR V);
XMVECTOR        XMVector2LengthEst(FXMVECTOR V);
XMVECTOR        XMVector2Length(FXMVECTOR V);
XMVECTOR        XMVector2NormalizeEst(FXMVECTOR V);
XMVECTOR        XMVector2Normalize(FXMVECTOR V);
XMVECTOR        XMVector2ClampLength(FXMVECTOR V, FLOAT LengthMin, FLOAT LengthMax);
XMVECTOR        XMVector2ClampLengthV(FXMVECTOR V, FXMVECTOR LengthMin, FXMVECTOR LengthMax);
XMVECTOR        XMVector2Reflect(FXMVECTOR Incident, FXMVECTOR Normal);
XMVECTOR        XMVector2Refract(FXMVECTOR Incident, FXMVECTOR Normal, FLOAT RefractionIndex);
XMVECTOR        XMVector2RefractV(FXMVECTOR Incident, FXMVECTOR Normal, FXMVECTOR RefractionIndex);
XMVECTOR        XMVector2Orthogonal(FXMVECTOR V);
XMVECTOR        XMVector2AngleBetweenNormalsEst(FXMVECTOR N1, FXMVECTOR N2);
XMVECTOR        XMVector2AngleBetweenNormals(FXMVECTOR N1, FXMVECTOR N2);
XMVECTOR        XMVector2AngleBetweenVectors(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVector2LinePointDistance(FXMVECTOR LinePoint1, FXMVECTOR LinePoint2, FXMVECTOR Point);
XMVECTOR        XMVector2IntersectLine(FXMVECTOR Line1Point1, FXMVECTOR Line1Point2, FXMVECTOR Line2Point1, CXMVECTOR Line2Point2);
XMVECTOR        XMVector2Transform(FXMVECTOR V, CXMMATRIX M);
XMFLOAT4*       XMVector2TransformStream(__out_bcount(sizeof(XMFLOAT4)+OutputStride*(VectorCount-1)) XMFLOAT4* pOutputStream,
                                         __in UINT OutputStride,
                                         __in_bcount(sizeof(XMFLOAT2)+InputStride*(VectorCount-1)) CONST XMFLOAT2* pInputStream,
                                         __in UINT InputStride, __in UINT VectorCount, CXMMATRIX M);
XMFLOAT4*       XMVector2TransformStreamNC(__out_bcount(sizeof(XMFLOAT4)+OutputStride*(VectorCount-1)) XMFLOAT4* pOutputStream,
                                           __in UINT OutputStride,
                                           __in_bcount(sizeof(XMFLOAT2)+InputStride*(VectorCount-1)) CONST XMFLOAT2* pInputStream,
                                           __in UINT InputStride, __in UINT VectorCount, CXMMATRIX M);
XMVECTOR        XMVector2TransformCoord(FXMVECTOR V, CXMMATRIX M);
XMFLOAT2*       XMVector2TransformCoordStream(__out_bcount(sizeof(XMFLOAT2)+OutputStride*(VectorCount-1)) XMFLOAT2* pOutputStream,
                                              __in UINT OutputStride,
                                              __in_bcount(sizeof(XMFLOAT2)+InputStride*(VectorCount-1)) CONST XMFLOAT2* pInputStream,
                                              __in UINT InputStride, __in UINT VectorCount, CXMMATRIX M);
XMVECTOR        XMVector2TransformNormal(FXMVECTOR V, CXMMATRIX M);
XMFLOAT2*       XMVector2TransformNormalStream(__out_bcount(sizeof(XMFLOAT2)+OutputStride*(VectorCount-1)) XMFLOAT2* pOutputStream,
                                               __in UINT OutputStride,
                                               __in_bcount(sizeof(XMFLOAT2)+InputStride*(VectorCount-1)) CONST XMFLOAT2* pInputStream,
                                               __in UINT InputStride, __in UINT VectorCount, CXMMATRIX M);

/****************************************************************************
 *
 * 3D vector operations
 *
 ****************************************************************************/


BOOL            XMVector3Equal(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector3EqualR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector3EqualInt(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector3EqualIntR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector3NearEqual(FXMVECTOR V1, FXMVECTOR V2, FXMVECTOR Epsilon);
BOOL            XMVector3NotEqual(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector3NotEqualInt(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector3Greater(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector3GreaterR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector3GreaterOrEqual(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector3GreaterOrEqualR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector3Less(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector3LessOrEqual(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector3InBounds(FXMVECTOR V, FXMVECTOR Bounds);
UINT            XMVector3InBoundsR(FXMVECTOR V, FXMVECTOR Bounds);

BOOL            XMVector3IsNaN(FXMVECTOR V);
BOOL            XMVector3IsInfinite(FXMVECTOR V);

XMVECTOR        XMVector3Dot(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVector3Cross(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVector3LengthSq(FXMVECTOR V);
XMVECTOR        XMVector3ReciprocalLengthEst(FXMVECTOR V);
XMVECTOR        XMVector3ReciprocalLength(FXMVECTOR V);
XMVECTOR        XMVector3LengthEst(FXMVECTOR V);
XMVECTOR        XMVector3Length(FXMVECTOR V);
XMVECTOR        XMVector3NormalizeEst(FXMVECTOR V);
XMVECTOR        XMVector3Normalize(FXMVECTOR V);
XMVECTOR        XMVector3ClampLength(FXMVECTOR V, FLOAT LengthMin, FLOAT LengthMax);
XMVECTOR        XMVector3ClampLengthV(FXMVECTOR V, FXMVECTOR LengthMin, FXMVECTOR LengthMax);
XMVECTOR        XMVector3Reflect(FXMVECTOR Incident, FXMVECTOR Normal);
XMVECTOR        XMVector3Refract(FXMVECTOR Incident, FXMVECTOR Normal, FLOAT RefractionIndex);
XMVECTOR        XMVector3RefractV(FXMVECTOR Incident, FXMVECTOR Normal, FXMVECTOR RefractionIndex);
XMVECTOR        XMVector3Orthogonal(FXMVECTOR V);
XMVECTOR        XMVector3AngleBetweenNormalsEst(FXMVECTOR N1, FXMVECTOR N2);
XMVECTOR        XMVector3AngleBetweenNormals(FXMVECTOR N1, FXMVECTOR N2);
XMVECTOR        XMVector3AngleBetweenVectors(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVector3LinePointDistance(FXMVECTOR LinePoint1, FXMVECTOR LinePoint2, FXMVECTOR Point);
VOID            XMVector3ComponentsFromNormal(__out XMVECTOR* pParallel, __out XMVECTOR* pPerpendicular, FXMVECTOR V, FXMVECTOR Normal);
XMVECTOR        XMVector3Rotate(FXMVECTOR V, FXMVECTOR RotationQuaternion);
XMVECTOR        XMVector3InverseRotate(FXMVECTOR V, FXMVECTOR RotationQuaternion);
XMVECTOR        XMVector3Transform(FXMVECTOR V, CXMMATRIX M);
XMFLOAT4*       XMVector3TransformStream(__out_bcount(sizeof(XMFLOAT4)+OutputStride*(VectorCount-1)) XMFLOAT4* pOutputStream,
                                         __in UINT OutputStride,
                                         __in_bcount(sizeof(XMFLOAT3)+InputStride*(VectorCount-1)) CONST XMFLOAT3* pInputStream,
                                         __in UINT InputStride, __in UINT VectorCount, CXMMATRIX M);
XMFLOAT4*       XMVector3TransformStreamNC(__out_bcount(sizeof(XMFLOAT4)+OutputStride*(VectorCount-1)) XMFLOAT4* pOutputStream,
                                           __in UINT OutputStride,
                                           __in_bcount(sizeof(XMFLOAT3)+InputStride*(VectorCount-1)) CONST XMFLOAT3* pInputStream,
                                           __in UINT InputStride, __in UINT VectorCount, CXMMATRIX M);
XMVECTOR        XMVector3TransformCoord(FXMVECTOR V, CXMMATRIX M);
XMFLOAT3*       XMVector3TransformCoordStream(__out_bcount(sizeof(XMFLOAT3)+OutputStride*(VectorCount-1)) XMFLOAT3* pOutputStream,
                                              __in UINT OutputStride,
                                              __in_bcount(sizeof(XMFLOAT3)+InputStride*(VectorCount-1)) CONST XMFLOAT3* pInputStream,
                                              __in UINT InputStride, __in UINT VectorCount, CXMMATRIX M);
XMVECTOR        XMVector3TransformNormal(FXMVECTOR V, CXMMATRIX M);
XMFLOAT3*       XMVector3TransformNormalStream(__out_bcount(sizeof(XMFLOAT3)+OutputStride*(VectorCount-1)) XMFLOAT3* pOutputStream,
                                               __in UINT OutputStride,
                                               __in_bcount(sizeof(XMFLOAT3)+InputStride*(VectorCount-1)) CONST XMFLOAT3* pInputStream,
                                               __in UINT InputStride, __in UINT VectorCount, CXMMATRIX M);
XMVECTOR        XMVector3Project(FXMVECTOR V, FLOAT ViewportX, FLOAT ViewportY, FLOAT ViewportWidth, FLOAT ViewportHeight, FLOAT ViewportMinZ, FLOAT ViewportMaxZ, 
                    CXMMATRIX Projection, CXMMATRIX View, CXMMATRIX World);
XMFLOAT3*       XMVector3ProjectStream(__out_bcount(sizeof(XMFLOAT3)+OutputStride*(VectorCount-1)) XMFLOAT3* pOutputStream,
                                       __in UINT OutputStride,
                                       __in_bcount(sizeof(XMFLOAT3)+InputStride*(VectorCount-1)) CONST XMFLOAT3* pInputStream,
                                       __in UINT InputStride, __in UINT VectorCount, 
                    FLOAT ViewportX, FLOAT ViewportY, FLOAT ViewportWidth, FLOAT ViewportHeight, FLOAT ViewportMinZ, FLOAT ViewportMaxZ, 
                    CXMMATRIX Projection, CXMMATRIX View, CXMMATRIX World);
XMVECTOR        XMVector3Unproject(FXMVECTOR V, FLOAT ViewportX, FLOAT ViewportY, FLOAT ViewportWidth, FLOAT ViewportHeight, FLOAT ViewportMinZ, FLOAT ViewportMaxZ, 
                    CXMMATRIX Projection, CXMMATRIX View, CXMMATRIX World);
XMFLOAT3*       XMVector3UnprojectStream(__out_bcount(sizeof(XMFLOAT3)+OutputStride*(VectorCount-1)) XMFLOAT3* pOutputStream,
                                         __in UINT OutputStride,
                                         __in_bcount(sizeof(XMFLOAT3)+InputStride*(VectorCount-1)) CONST XMFLOAT3* pInputStream,
                                         __in UINT InputStride, __in UINT VectorCount, 
                    FLOAT ViewportX, FLOAT ViewportY, FLOAT ViewportWidth, FLOAT ViewportHeight, FLOAT ViewportMinZ, FLOAT ViewportMaxZ, 
                    CXMMATRIX Projection, CXMMATRIX View, CXMMATRIX World);

/****************************************************************************
 *
 * 4D vector operations
 *
 ****************************************************************************/

BOOL            XMVector4Equal(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector4EqualR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector4EqualInt(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector4EqualIntR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector4NearEqual(FXMVECTOR V1, FXMVECTOR V2, FXMVECTOR Epsilon);
BOOL            XMVector4NotEqual(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector4NotEqualInt(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector4Greater(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector4GreaterR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector4GreaterOrEqual(FXMVECTOR V1, FXMVECTOR V2);
UINT            XMVector4GreaterOrEqualR(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector4Less(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector4LessOrEqual(FXMVECTOR V1, FXMVECTOR V2);
BOOL            XMVector4InBounds(FXMVECTOR V, FXMVECTOR Bounds);
UINT            XMVector4InBoundsR(FXMVECTOR V, FXMVECTOR Bounds);

BOOL            XMVector4IsNaN(FXMVECTOR V);
BOOL            XMVector4IsInfinite(FXMVECTOR V);

XMVECTOR        XMVector4Dot(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVector4Cross(FXMVECTOR V1, FXMVECTOR V2, FXMVECTOR V3);
XMVECTOR        XMVector4LengthSq(FXMVECTOR V);
XMVECTOR        XMVector4ReciprocalLengthEst(FXMVECTOR V);
XMVECTOR        XMVector4ReciprocalLength(FXMVECTOR V);
XMVECTOR        XMVector4LengthEst(FXMVECTOR V);
XMVECTOR        XMVector4Length(FXMVECTOR V);
XMVECTOR        XMVector4NormalizeEst(FXMVECTOR V);
XMVECTOR        XMVector4Normalize(FXMVECTOR V);
XMVECTOR        XMVector4ClampLength(FXMVECTOR V, FLOAT LengthMin, FLOAT LengthMax);
XMVECTOR        XMVector4ClampLengthV(FXMVECTOR V, FXMVECTOR LengthMin, FXMVECTOR LengthMax);
XMVECTOR        XMVector4Reflect(FXMVECTOR Incident, FXMVECTOR Normal);
XMVECTOR        XMVector4Refract(FXMVECTOR Incident, FXMVECTOR Normal, FLOAT RefractionIndex);
XMVECTOR        XMVector4RefractV(FXMVECTOR Incident, FXMVECTOR Normal, FXMVECTOR RefractionIndex);
XMVECTOR        XMVector4Orthogonal(FXMVECTOR V);
XMVECTOR        XMVector4AngleBetweenNormalsEst(FXMVECTOR N1, FXMVECTOR N2);
XMVECTOR        XMVector4AngleBetweenNormals(FXMVECTOR N1, FXMVECTOR N2);
XMVECTOR        XMVector4AngleBetweenVectors(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR        XMVector4Transform(FXMVECTOR V, CXMMATRIX M);
XMFLOAT4*       XMVector4TransformStream(__out_bcount(sizeof(XMFLOAT4)+OutputStride*(VectorCount-1)) XMFLOAT4* pOutputStream,
                                         __in UINT OutputStride,
                                         __in_bcount(sizeof(XMFLOAT4)+InputStride*(VectorCount-1)) CONST XMFLOAT4* pInputStream,
                                         __in UINT InputStride, __in UINT VectorCount, CXMMATRIX M);

/****************************************************************************
 *
 * Matrix operations
 *
 ****************************************************************************/

BOOL            XMMatrixIsNaN(CXMMATRIX M);
BOOL            XMMatrixIsInfinite(CXMMATRIX M);
BOOL            XMMatrixIsIdentity(CXMMATRIX M);

XMMATRIX        XMMatrixMultiply(CXMMATRIX M1, CXMMATRIX M2);
XMMATRIX        XMMatrixMultiplyTranspose(CXMMATRIX M1, CXMMATRIX M2);
XMMATRIX        XMMatrixTranspose(CXMMATRIX M);
XMMATRIX        XMMatrixInverse(__out XMVECTOR* pDeterminant, CXMMATRIX M);
XMVECTOR        XMMatrixDeterminant(CXMMATRIX M);
BOOL            XMMatrixDecompose(__out XMVECTOR *outScale, __out XMVECTOR *outRotQuat, __out XMVECTOR *outTrans, CXMMATRIX M);

XMMATRIX        XMMatrixIdentity();
XMMATRIX        XMMatrixSet(FLOAT m00, FLOAT m01, FLOAT m02, FLOAT m03,
                         FLOAT m10, FLOAT m11, FLOAT m12, FLOAT m13,
                         FLOAT m20, FLOAT m21, FLOAT m22, FLOAT m23,
                         FLOAT m30, FLOAT m31, FLOAT m32, FLOAT m33);
XMMATRIX        XMMatrixTranslation(FLOAT OffsetX, FLOAT OffsetY, FLOAT OffsetZ);
XMMATRIX        XMMatrixTranslationFromVector(FXMVECTOR Offset);
XMMATRIX        XMMatrixScaling(FLOAT ScaleX, FLOAT ScaleY, FLOAT ScaleZ);
XMMATRIX        XMMatrixScalingFromVector(FXMVECTOR Scale);
XMMATRIX        XMMatrixRotationX(FLOAT Angle);
XMMATRIX        XMMatrixRotationY(FLOAT Angle);
XMMATRIX        XMMatrixRotationZ(FLOAT Angle);
XMMATRIX        XMMatrixRotationRollPitchYaw(FLOAT Pitch, FLOAT Yaw, FLOAT Roll);
XMMATRIX        XMMatrixRotationRollPitchYawFromVector(FXMVECTOR Angles);
XMMATRIX        XMMatrixRotationNormal(FXMVECTOR NormalAxis, FLOAT Angle);
XMMATRIX        XMMatrixRotationAxis(FXMVECTOR Axis, FLOAT Angle);
XMMATRIX        XMMatrixRotationQuaternion(FXMVECTOR Quaternion);
XMMATRIX        XMMatrixTransformation2D(FXMVECTOR ScalingOrigin, FLOAT ScalingOrientation, FXMVECTOR Scaling, 
                    FXMVECTOR RotationOrigin, FLOAT Rotation, CXMVECTOR Translation);
XMMATRIX        XMMatrixTransformation(FXMVECTOR ScalingOrigin, FXMVECTOR ScalingOrientationQuaternion, FXMVECTOR Scaling, 
                    CXMVECTOR RotationOrigin, CXMVECTOR RotationQuaternion, CXMVECTOR Translation);
XMMATRIX        XMMatrixAffineTransformation2D(FXMVECTOR Scaling, FXMVECTOR RotationOrigin, FLOAT Rotation, FXMVECTOR Translation);
XMMATRIX        XMMatrixAffineTransformation(FXMVECTOR Scaling, FXMVECTOR RotationOrigin, FXMVECTOR RotationQuaternion, CXMVECTOR Translation);
XMMATRIX        XMMatrixReflect(FXMVECTOR ReflectionPlane);
XMMATRIX        XMMatrixShadow(FXMVECTOR ShadowPlane, FXMVECTOR LightPosition);

XMMATRIX        XMMatrixLookAtLH(FXMVECTOR EyePosition, FXMVECTOR FocusPosition, FXMVECTOR UpDirection);
XMMATRIX        XMMatrixLookAtRH(FXMVECTOR EyePosition, FXMVECTOR FocusPosition, FXMVECTOR UpDirection);
XMMATRIX        XMMatrixLookToLH(FXMVECTOR EyePosition, FXMVECTOR EyeDirection, FXMVECTOR UpDirection);
XMMATRIX        XMMatrixLookToRH(FXMVECTOR EyePosition, FXMVECTOR EyeDirection, FXMVECTOR UpDirection);
XMMATRIX        XMMatrixPerspectiveLH(FLOAT ViewWidth, FLOAT ViewHeight, FLOAT NearZ, FLOAT FarZ);
XMMATRIX        XMMatrixPerspectiveRH(FLOAT ViewWidth, FLOAT ViewHeight, FLOAT NearZ, FLOAT FarZ);
XMMATRIX        XMMatrixPerspectiveFovLH(FLOAT FovAngleY, FLOAT AspectHByW, FLOAT NearZ, FLOAT FarZ);
XMMATRIX        XMMatrixPerspectiveFovRH(FLOAT FovAngleY, FLOAT AspectHByW, FLOAT NearZ, FLOAT FarZ);
XMMATRIX        XMMatrixPerspectiveOffCenterLH(FLOAT ViewLeft, FLOAT ViewRight, FLOAT ViewBottom, FLOAT ViewTop, FLOAT NearZ, FLOAT FarZ);
XMMATRIX        XMMatrixPerspectiveOffCenterRH(FLOAT ViewLeft, FLOAT ViewRight, FLOAT ViewBottom, FLOAT ViewTop, FLOAT NearZ, FLOAT FarZ);
XMMATRIX        XMMatrixOrthographicLH(FLOAT ViewWidth, FLOAT ViewHeight, FLOAT NearZ, FLOAT FarZ);
XMMATRIX        XMMatrixOrthographicRH(FLOAT ViewWidth, FLOAT ViewHeight, FLOAT NearZ, FLOAT FarZ);
XMMATRIX        XMMatrixOrthographicOffCenterLH(FLOAT ViewLeft, FLOAT ViewRight, FLOAT ViewBottom, FLOAT ViewTop, FLOAT NearZ, FLOAT FarZ);
XMMATRIX        XMMatrixOrthographicOffCenterRH(FLOAT ViewLeft, FLOAT ViewRight, FLOAT ViewBottom, FLOAT ViewTop, FLOAT NearZ, FLOAT FarZ);

/****************************************************************************
 *
 * Quaternion operations
 *
 ****************************************************************************/

BOOL            XMQuaternionEqual(FXMVECTOR Q1, FXMVECTOR Q2);
BOOL            XMQuaternionNotEqual(FXMVECTOR Q1, FXMVECTOR Q2);

BOOL            XMQuaternionIsNaN(FXMVECTOR Q);
BOOL            XMQuaternionIsInfinite(FXMVECTOR Q);
BOOL            XMQuaternionIsIdentity(FXMVECTOR Q);

XMVECTOR        XMQuaternionDot(FXMVECTOR Q1, FXMVECTOR Q2);
XMVECTOR        XMQuaternionMultiply(FXMVECTOR Q1, FXMVECTOR Q2);
XMVECTOR        XMQuaternionLengthSq(FXMVECTOR Q);
XMVECTOR        XMQuaternionReciprocalLength(FXMVECTOR Q);
XMVECTOR        XMQuaternionLength(FXMVECTOR Q);
XMVECTOR        XMQuaternionNormalizeEst(FXMVECTOR Q);
XMVECTOR        XMQuaternionNormalize(FXMVECTOR Q);
XMVECTOR        XMQuaternionConjugate(FXMVECTOR Q);
XMVECTOR        XMQuaternionInverse(FXMVECTOR Q);
XMVECTOR        XMQuaternionLn(FXMVECTOR Q);
XMVECTOR        XMQuaternionExp(FXMVECTOR Q);
XMVECTOR        XMQuaternionSlerp(FXMVECTOR Q0, FXMVECTOR Q1, FLOAT t);
XMVECTOR        XMQuaternionSlerpV(FXMVECTOR Q0, FXMVECTOR Q1, FXMVECTOR T);
XMVECTOR        XMQuaternionSquad(FXMVECTOR Q0, FXMVECTOR Q1, FXMVECTOR Q2, CXMVECTOR Q3, FLOAT t);
XMVECTOR        XMQuaternionSquadV(FXMVECTOR Q0, FXMVECTOR Q1, FXMVECTOR Q2, CXMVECTOR Q3, CXMVECTOR T);
VOID            XMQuaternionSquadSetup(__out XMVECTOR* pA, __out XMVECTOR* pB, __out XMVECTOR* pC, FXMVECTOR Q0, FXMVECTOR Q1, FXMVECTOR Q2, CXMVECTOR Q3);
XMVECTOR        XMQuaternionBaryCentric(FXMVECTOR Q0, FXMVECTOR Q1, FXMVECTOR Q2, FLOAT f, FLOAT g);
XMVECTOR        XMQuaternionBaryCentricV(FXMVECTOR Q0, FXMVECTOR Q1, FXMVECTOR Q2, CXMVECTOR F, CXMVECTOR G);

XMVECTOR        XMQuaternionIdentity();
XMVECTOR        XMQuaternionRotationRollPitchYaw(FLOAT Pitch, FLOAT Yaw, FLOAT Roll);
XMVECTOR        XMQuaternionRotationRollPitchYawFromVector(FXMVECTOR Angles);
XMVECTOR        XMQuaternionRotationNormal(FXMVECTOR NormalAxis, FLOAT Angle);
XMVECTOR        XMQuaternionRotationAxis(FXMVECTOR Axis, FLOAT Angle);
XMVECTOR        XMQuaternionRotationMatrix(CXMMATRIX M);

VOID            XMQuaternionToAxisAngle(__out XMVECTOR* pAxis, __out FLOAT* pAngle, FXMVECTOR Q);

/****************************************************************************
 *
 * Plane operations
 *
 ****************************************************************************/

BOOL            XMPlaneEqual(FXMVECTOR P1, FXMVECTOR P2);
BOOL            XMPlaneNearEqual(FXMVECTOR P1, FXMVECTOR P2, FXMVECTOR Epsilon);
BOOL            XMPlaneNotEqual(FXMVECTOR P1, FXMVECTOR P2);

BOOL            XMPlaneIsNaN(FXMVECTOR P);
BOOL            XMPlaneIsInfinite(FXMVECTOR P);

XMVECTOR        XMPlaneDot(FXMVECTOR P, FXMVECTOR V);
XMVECTOR        XMPlaneDotCoord(FXMVECTOR P, FXMVECTOR V);
XMVECTOR        XMPlaneDotNormal(FXMVECTOR P, FXMVECTOR V);
XMVECTOR        XMPlaneNormalizeEst(FXMVECTOR P);
XMVECTOR        XMPlaneNormalize(FXMVECTOR P);
XMVECTOR        XMPlaneIntersectLine(FXMVECTOR P, FXMVECTOR LinePoint1, FXMVECTOR LinePoint2);
VOID            XMPlaneIntersectPlane(__out XMVECTOR* pLinePoint1, __out XMVECTOR* pLinePoint2, FXMVECTOR P1, FXMVECTOR P2);
XMVECTOR        XMPlaneTransform(FXMVECTOR P, CXMMATRIX M);
XMFLOAT4*       XMPlaneTransformStream(__out_bcount(sizeof(XMFLOAT4)+OutputStride*(PlaneCount-1)) XMFLOAT4* pOutputStream,
                                       __in UINT OutputStride,
                                       __in_bcount(sizeof(XMFLOAT4)+InputStride*(PlaneCount-1)) CONST XMFLOAT4* pInputStream,
                                       __in UINT InputStride, __in UINT PlaneCount, CXMMATRIX M);

XMVECTOR        XMPlaneFromPointNormal(FXMVECTOR Point, FXMVECTOR Normal);
XMVECTOR        XMPlaneFromPoints(FXMVECTOR Point1, FXMVECTOR Point2, FXMVECTOR Point3);

/****************************************************************************
 *
 * Color operations
 *
 ****************************************************************************/

BOOL            XMColorEqual(FXMVECTOR C1, FXMVECTOR C2);
BOOL            XMColorNotEqual(FXMVECTOR C1, FXMVECTOR C2);
BOOL            XMColorGreater(FXMVECTOR C1, FXMVECTOR C2);
BOOL            XMColorGreaterOrEqual(FXMVECTOR C1, FXMVECTOR C2);
BOOL            XMColorLess(FXMVECTOR C1, FXMVECTOR C2);
BOOL            XMColorLessOrEqual(FXMVECTOR C1, FXMVECTOR C2);

BOOL            XMColorIsNaN(FXMVECTOR C);
BOOL            XMColorIsInfinite(FXMVECTOR C);

XMVECTOR        XMColorNegative(FXMVECTOR C);
XMVECTOR        XMColorModulate(FXMVECTOR C1, FXMVECTOR C2);
XMVECTOR        XMColorAdjustSaturation(FXMVECTOR C, FLOAT Saturation);
XMVECTOR        XMColorAdjustContrast(FXMVECTOR C, FLOAT Contrast);

/****************************************************************************
 *
 * Miscellaneous operations
 *
 ****************************************************************************/

BOOL            XMVerifyCPUSupport();

VOID            XMAssert(__in_z CONST CHAR* pExpression, __in_z CONST CHAR* pFileName, UINT LineNumber);

XMVECTOR        XMFresnelTerm(FXMVECTOR CosIncidentAngle, FXMVECTOR RefractionIndex);

BOOL            XMScalarNearEqual(FLOAT S1, FLOAT S2, FLOAT Epsilon);
FLOAT           XMScalarModAngle(FLOAT Value);
FLOAT           XMScalarSin(FLOAT Value);
FLOAT           XMScalarCos(FLOAT Value);
VOID            XMScalarSinCos(__out FLOAT* pSin, __out FLOAT* pCos, FLOAT Value);
FLOAT           XMScalarASin(FLOAT Value);
FLOAT           XMScalarACos(FLOAT Value);
FLOAT           XMScalarSinEst(FLOAT Value);
FLOAT           XMScalarCosEst(FLOAT Value);
VOID            XMScalarSinCosEst(__out FLOAT* pSin, __out FLOAT* pCos, FLOAT Value);
FLOAT           XMScalarASinEst(FLOAT Value);
FLOAT           XMScalarACosEst(FLOAT Value);

/****************************************************************************
 *
 * Globals
 *
 ****************************************************************************/

// The purpose of the following global constants is to prevent redundant 
// reloading of the constants when they are referenced by more than one
// separate inline math routine called within the same function.  Declaring
// a constant locally within a routine is sufficient to prevent redundant
// reloads of that constant when that single routine is called multiple
// times in a function, but if the constant is used (and declared) in a 
// separate math routine it would be reloaded.

#define XMGLOBALCONST extern CONST __declspec(selectany)

XMGLOBALCONST XMVECTORF32 g_XMSinCoefficients0    = {1.0f, -0.166666667f, 8.333333333e-3f, -1.984126984e-4f};
XMGLOBALCONST XMVECTORF32 g_XMSinCoefficients1    = {2.755731922e-6f, -2.505210839e-8f, 1.605904384e-10f, -7.647163732e-13f};
XMGLOBALCONST XMVECTORF32 g_XMSinCoefficients2    = {2.811457254e-15f, -8.220635247e-18f, 1.957294106e-20f, -3.868170171e-23f};
XMGLOBALCONST XMVECTORF32 g_XMCosCoefficients0    = {1.0f, -0.5f, 4.166666667e-2f, -1.388888889e-3f};
XMGLOBALCONST XMVECTORF32 g_XMCosCoefficients1    = {2.480158730e-5f, -2.755731922e-7f, 2.087675699e-9f, -1.147074560e-11f};
XMGLOBALCONST XMVECTORF32 g_XMCosCoefficients2    = {4.779477332e-14f, -1.561920697e-16f, 4.110317623e-19f, -8.896791392e-22f};
XMGLOBALCONST XMVECTORF32 g_XMTanCoefficients0    = {1.0f, 0.333333333f, 0.133333333f, 5.396825397e-2f};
XMGLOBALCONST XMVECTORF32 g_XMTanCoefficients1    = {2.186948854e-2f, 8.863235530e-3f, 3.592128167e-3f, 1.455834485e-3f};
XMGLOBALCONST XMVECTORF32 g_XMTanCoefficients2    = {5.900274264e-4f, 2.391290764e-4f, 9.691537707e-5f, 3.927832950e-5f};
XMGLOBALCONST XMVECTORF32 g_XMASinCoefficients0   = {-0.05806367563904f, -0.41861972469416f, 0.22480114791621f, 2.17337241360606f};
XMGLOBALCONST XMVECTORF32 g_XMASinCoefficients1   = {0.61657275907170f, 4.29696498283455f, -1.18942822255452f, -6.53784832094831f};
XMGLOBALCONST XMVECTORF32 g_XMASinCoefficients2   = {-1.36926553863413f, -4.48179294237210f, 1.41810672941833f, 5.48179257935713f};
XMGLOBALCONST XMVECTORF32 g_XMATanCoefficients0   = {1.0f, 0.333333334f, 0.2f, 0.142857143f};
XMGLOBALCONST XMVECTORF32 g_XMATanCoefficients1   = {1.111111111e-1f, 9.090909091e-2f, 7.692307692e-2f, 6.666666667e-2f};
XMGLOBALCONST XMVECTORF32 g_XMATanCoefficients2   = {5.882352941e-2f, 5.263157895e-2f, 4.761904762e-2f, 4.347826087e-2f};
XMGLOBALCONST XMVECTORF32 g_XMSinEstCoefficients  = {1.0f, -1.66521856991541e-1f, 8.199913018755e-3f, -1.61475937228e-4f};
XMGLOBALCONST XMVECTORF32 g_XMCosEstCoefficients  = {1.0f, -4.95348008918096e-1f, 3.878259962881e-2f, -9.24587976263e-4f};
XMGLOBALCONST XMVECTORF32 g_XMTanEstCoefficients  = {2.484f, -1.954923183e-1f, 2.467401101f, XM_1DIVPI};
XMGLOBALCONST XMVECTORF32 g_XMATanEstCoefficients = {7.689891418951e-1f, 1.104742493348f, 8.661844266006e-1f, XM_PIDIV2};
XMGLOBALCONST XMVECTORF32 g_XMASinEstCoefficients = {-1.36178272886711f, 2.37949493464538f, -8.08228565650486e-1f, 2.78440142746736e-1f};
XMGLOBALCONST XMVECTORF32 g_XMASinEstConstants    = {1.00000011921f, XM_PIDIV2, 0.0f, 0.0f};
XMGLOBALCONST XMVECTORF32 g_XMPiConstants0        = {XM_PI, XM_2PI, XM_1DIVPI, XM_1DIV2PI};
XMGLOBALCONST XMVECTORF32 g_XMIdentityR0          = {1.0f, 0.0f, 0.0f, 0.0f};
XMGLOBALCONST XMVECTORF32 g_XMIdentityR1          = {0.0f, 1.0f, 0.0f, 0.0f};
XMGLOBALCONST XMVECTORF32 g_XMIdentityR2          = {0.0f, 0.0f, 1.0f, 0.0f};
XMGLOBALCONST XMVECTORF32 g_XMIdentityR3          = {0.0f, 0.0f, 0.0f, 1.0f};
XMGLOBALCONST XMVECTORF32 g_XMNegIdentityR0       = {-1.0f,0.0f, 0.0f, 0.0f};
XMGLOBALCONST XMVECTORF32 g_XMNegIdentityR1       = {0.0f,-1.0f, 0.0f, 0.0f};
XMGLOBALCONST XMVECTORF32 g_XMNegIdentityR2       = {0.0f, 0.0f,-1.0f, 0.0f};
XMGLOBALCONST XMVECTORF32 g_XMNegIdentityR3       = {0.0f, 0.0f, 0.0f,-1.0f};

#if defined(_XM_NO_INTRINSICS_) || defined(_XM_SSE_INTRINSICS_)

XMGLOBALCONST XMVECTORI32 g_XMNegativeZero      = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
XMGLOBALCONST XMVECTORI32 g_XMNegate3           = {0x80000000, 0x80000000, 0x80000000, 0x00000000};
XMGLOBALCONST XMVECTORI32 g_XMMask3             = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000};
XMGLOBALCONST XMVECTORI32 g_XMMaskX             = {0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000};
XMGLOBALCONST XMVECTORI32 g_XMMaskY             = {0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000};
XMGLOBALCONST XMVECTORI32 g_XMMaskZ             = {0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000};
XMGLOBALCONST XMVECTORI32 g_XMMaskW             = {0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF};
XMGLOBALCONST XMVECTORF32 g_XMOne               = { 1.0f, 1.0f, 1.0f, 1.0f};
XMGLOBALCONST XMVECTORF32 g_XMOne3              = { 1.0f, 1.0f, 1.0f, 0.0f};
XMGLOBALCONST XMVECTORF32 g_XMZero              = { 0.0f, 0.0f, 0.0f, 0.0f};
XMGLOBALCONST XMVECTORF32 g_XMNegativeOne       = {-1.0f,-1.0f,-1.0f,-1.0f};
XMGLOBALCONST XMVECTORF32 g_XMOneHalf           = { 0.5f, 0.5f, 0.5f, 0.5f};
XMGLOBALCONST XMVECTORF32 g_XMNegativeOneHalf   = {-0.5f,-0.5f,-0.5f,-0.5f};
XMGLOBALCONST XMVECTORF32 g_XMNegativeTwoPi     = {-XM_2PI, -XM_2PI, -XM_2PI, -XM_2PI};
XMGLOBALCONST XMVECTORF32 g_XMNegativePi        = {-XM_PI, -XM_PI, -XM_PI, -XM_PI};
XMGLOBALCONST XMVECTORF32 g_XMHalfPi            = {XM_PIDIV2, XM_PIDIV2, XM_PIDIV2, XM_PIDIV2};
XMGLOBALCONST XMVECTORF32 g_XMPi                = {XM_PI, XM_PI, XM_PI, XM_PI};
XMGLOBALCONST XMVECTORF32 g_XMReciprocalPi      = {XM_1DIVPI, XM_1DIVPI, XM_1DIVPI, XM_1DIVPI};
XMGLOBALCONST XMVECTORF32 g_XMTwoPi             = {XM_2PI, XM_2PI, XM_2PI, XM_2PI};
XMGLOBALCONST XMVECTORF32 g_XMReciprocalTwoPi   = {XM_1DIV2PI, XM_1DIV2PI, XM_1DIV2PI, XM_1DIV2PI};
XMGLOBALCONST XMVECTORF32 g_XMEpsilon           = {1.192092896e-7f, 1.192092896e-7f, 1.192092896e-7f, 1.192092896e-7f};
XMGLOBALCONST XMVECTORI32 g_XMInfinity          = {0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000};
XMGLOBALCONST XMVECTORI32 g_XMQNaN              = {0x7FC00000, 0x7FC00000, 0x7FC00000, 0x7FC00000};
XMGLOBALCONST XMVECTORI32 g_XMQNaNTest          = {0x007FFFFF, 0x007FFFFF, 0x007FFFFF, 0x007FFFFF};
XMGLOBALCONST XMVECTORI32 g_XMAbsMask           = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
XMGLOBALCONST XMVECTORI32 g_XMFltMin            = {0x00800000, 0x00800000, 0x00800000, 0x00800000};
XMGLOBALCONST XMVECTORI32 g_XMFltMax            = {0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF};
XMGLOBALCONST XMVECTORI32 g_XMNegOneMask		= {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
XMGLOBALCONST XMVECTORI32 g_XMMaskA8R8G8B8      = {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000};
XMGLOBALCONST XMVECTORI32 g_XMFlipA8R8G8B8      = {0x00000000, 0x00000000, 0x00000000, 0x80000000};
XMGLOBALCONST XMVECTORF32 g_XMFixAA8R8G8B8      = {0.0f,0.0f,0.0f,(float)(0x80000000U)};
XMGLOBALCONST XMVECTORF32 g_XMNormalizeA8R8G8B8 = {1.0f/(255.0f*(float)(0x10000)),1.0f/(255.0f*(float)(0x100)),1.0f/255.0f,1.0f/(255.0f*(float)(0x1000000))};
XMGLOBALCONST XMVECTORI32 g_XMMaskA2B10G10R10   = {0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000};
XMGLOBALCONST XMVECTORI32 g_XMFlipA2B10G10R10   = {0x00000200, 0x00080000, 0x20000000, 0x80000000};
XMGLOBALCONST XMVECTORF32 g_XMFixAA2B10G10R10   = {-512.0f,-512.0f*(float)(0x400),-512.0f*(float)(0x100000),(float)(0x80000000U)};
XMGLOBALCONST XMVECTORF32 g_XMNormalizeA2B10G10R10 = {1.0f/511.0f,1.0f/(511.0f*(float)(0x400)),1.0f/(511.0f*(float)(0x100000)),1.0f/(3.0f*(float)(0x40000000))};
XMGLOBALCONST XMVECTORI32 g_XMMaskX16Y16        = {0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000};
XMGLOBALCONST XMVECTORI32 g_XMFlipX16Y16        = {0x00008000, 0x00000000, 0x00000000, 0x00000000};
XMGLOBALCONST XMVECTORF32 g_XMFixX16Y16         = {-32768.0f,0.0f,0.0f,0.0f};
XMGLOBALCONST XMVECTORF32 g_XMNormalizeX16Y16   = {1.0f/32767.0f,1.0f/(32767.0f*65536.0f),0.0f,0.0f};
XMGLOBALCONST XMVECTORI32 g_XMMaskX16Y16Z16W16  = {0x0000FFFF, 0x0000FFFF, 0xFFFF0000, 0xFFFF0000};
XMGLOBALCONST XMVECTORI32 g_XMFlipX16Y16Z16W16  = {0x00008000, 0x00008000, 0x00000000, 0x00000000};
XMGLOBALCONST XMVECTORF32 g_XMFixX16Y16Z16W16   = {-32768.0f,-32768.0f,0.0f,0.0f};
XMGLOBALCONST XMVECTORF32 g_XMNormalizeX16Y16Z16W16 = {1.0f/32767.0f,1.0f/32767.0f,1.0f/(32767.0f*65536.0f),1.0f/(32767.0f*65536.0f)};
XMGLOBALCONST XMVECTORF32 g_XMNoFraction        = {8388608.0f,8388608.0f,8388608.0f,8388608.0f};
XMGLOBALCONST XMVECTORI32 g_XMMaskByte          = {0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF};
XMGLOBALCONST XMVECTORF32 g_XMNegateX           = {-1.0f, 1.0f, 1.0f, 1.0f};
XMGLOBALCONST XMVECTORF32 g_XMNegateY           = { 1.0f,-1.0f, 1.0f, 1.0f};
XMGLOBALCONST XMVECTORF32 g_XMNegateZ           = { 1.0f, 1.0f,-1.0f, 1.0f};
XMGLOBALCONST XMVECTORF32 g_XMNegateW           = { 1.0f, 1.0f, 1.0f,-1.0f};
XMGLOBALCONST XMVECTORI32 g_XMSelect0101        = {XM_SELECT_0, XM_SELECT_1, XM_SELECT_0, XM_SELECT_1};
XMGLOBALCONST XMVECTORI32 g_XMSelect1010        = {XM_SELECT_1, XM_SELECT_0, XM_SELECT_1, XM_SELECT_0};
XMGLOBALCONST XMVECTORI32 g_XMOneHalfMinusEpsilon = { 0x3EFFFFFD, 0x3EFFFFFD, 0x3EFFFFFD, 0x3EFFFFFD};

#ifdef _XM_NO_INTRINSICS_
XMGLOBALCONST XMVECTORI32 g_XMSelect1000        = {XM_SELECT_1, XM_SELECT_0, XM_SELECT_0, XM_SELECT_0};
XMGLOBALCONST XMVECTORI32 g_XMSelect1100        = {XM_SELECT_1, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0};
XMGLOBALCONST XMVECTORI32 g_XMSelect1110        = {XM_SELECT_1, XM_SELECT_1, XM_SELECT_1, XM_SELECT_0};
XMGLOBALCONST XMVECTORI32 g_XMSwizzleXYXY       = {XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0X, XM_PERMUTE_0Y};
XMGLOBALCONST XMVECTORI32 g_XMSwizzleXYZX       = {XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_0X};
XMGLOBALCONST XMVECTORI32 g_XMSwizzleYXZW       = {XM_PERMUTE_0Y, XM_PERMUTE_0X, XM_PERMUTE_0Z, XM_PERMUTE_0W};
XMGLOBALCONST XMVECTORI32 g_XMSwizzleYZXW       = {XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_0X, XM_PERMUTE_0W};
XMGLOBALCONST XMVECTORI32 g_XMSwizzleZXYW       = {XM_PERMUTE_0Z, XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0W};
XMGLOBALCONST XMVECTORI32 g_XMPermute0X0Y1X1Y   = {XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_1X, XM_PERMUTE_1Y};
XMGLOBALCONST XMVECTORI32 g_XMPermute0Z0W1Z1W   = {XM_PERMUTE_0Z, XM_PERMUTE_0W, XM_PERMUTE_1Z, XM_PERMUTE_1W};

#endif // !_XM_NO_INTRINSICS_

#endif // _XM_NO_INTRINSICS_

/****************************************************************************
 *
 * Implementation
 *
 ****************************************************************************/

#pragma warning(push)
#pragma warning(disable:4214 4204 4616 6001)

#if !defined(__cplusplus) && !defined(_XBOX) && defined(_XM_ISVS2005_)

/* Work around VC 2005 bug where math.h defines logf with a semicolon at the end.
 * Note this is fixed as of Visual Studio 2005 Service Pack 1
 */

#undef logf
#define logf(x)     ((float)log((double)(x)))

#endif // !defined(__cplusplus) && !defined(_XBOX) && defined(_XM_ISVS2005_)

//------------------------------------------------------------------------------

#if defined(_XM_NO_INTRINSICS_) || defined(_XM_SSE_INTRINSICS_)

XMFINLINE XMVECTOR XMVectorSetBinaryConstant(UINT C0, UINT C1, UINT C2, UINT C3)
{
#if defined(_XM_NO_INTRINSICS_)
    XMVECTORF32 vResult = { (C0&1)*1.f, (C1&1)*1.f, (C2&1)*1.f, (C3&1)*1.f};
    return vResult.v;
#else // XM_SSE_INTRINSICS_
    return _mm_set_ps( (C3&1)*1.f, (C2&1)*1.f, (C1&1)*1.f, (C0&1)*1.f );
#endif
}

//------------------------------------------------------------------------------

XMFINLINE XMVECTOR XMVectorSplatConstant(INT IntConstant, UINT DivExponent)
{
#if defined(_XM_NO_INTRINSICS_)
    XMASSERT( IntConstant >= -16 && IntConstant <= 15 );
    {
    XMVECTORI32 V = { IntConstant, IntConstant, IntConstant, IntConstant };
    return XMConvertVectorIntToFloat( V.v, DivExponent);
    }
#else // XM_SSE_INTRINSICS_
    XMASSERT( IntConstant >= -16 && IntConstant <= 15 );
    return _mm_set_ps1((float)(IntConstant) / (float)(1 << DivExponent) );
#endif
}

//------------------------------------------------------------------------------

XMFINLINE XMVECTOR XMVectorSplatConstantInt(INT IntConstant)
{
#if defined(_XM_NO_INTRINSICS_)
    XMASSERT( IntConstant >= -16 && IntConstant <= 15 );
    {
    XMVECTORI32 V = { IntConstant, IntConstant, IntConstant, IntConstant };
    return V.v;
    }
#else // XM_SSE_INTRINSICS_
    XMASSERT( IntConstant >= -16 && IntConstant <= 15 );
    __m128i V = _mm_set1_epi32( IntConstant );
    return reinterpret_cast<__m128 *>(&V)[0];
#endif
}

//------------------------------------------------------------------------------

XMFINLINE XMVECTOR XMVectorShiftLeft(FXMVECTOR V1, FXMVECTOR V2, UINT Elements)
{
    return XMVectorPermute(V1, V2, XMVectorPermuteControl((Elements), ((Elements) + 1), ((Elements) + 2), ((Elements) + 3)));
}

//------------------------------------------------------------------------------

XMFINLINE XMVECTOR XMVectorRotateLeft(FXMVECTOR V, UINT Elements)
{
#if defined(_XM_NO_INTRINSICS_)
    XMASSERT( Elements < 4 );
    {
    XMVECTORF32 vResult = { V.v[Elements & 3], V.v[(Elements + 1) & 3], V.v[(Elements + 2) & 3], V.v[(Elements + 3) & 3] };
    return vResult.v;
    }
#else // XM_SSE_INTRINSICS_
    FLOAT fx = XMVectorGetByIndex(V,(Elements) & 3);
    FLOAT fy = XMVectorGetByIndex(V,((Elements) + 1) & 3);
    FLOAT fz = XMVectorGetByIndex(V,((Elements) + 2) & 3);
    FLOAT fw = XMVectorGetByIndex(V,((Elements) + 3) & 3);
    return _mm_set_ps( fw, fz, fy, fx );
#endif
}

//------------------------------------------------------------------------------

XMFINLINE XMVECTOR XMVectorRotateRight(FXMVECTOR V, UINT Elements)
{
#if defined(_XM_NO_INTRINSICS_)
    XMASSERT( Elements < 4 );
    {
    XMVECTORF32 vResult = { V.v[(4 - (Elements)) & 3], V.v[(5 - (Elements)) & 3], V.v[(6 - (Elements)) & 3], V.v[(7 - (Elements)) & 3] };
    return vResult.v;
    }
#else // XM_SSE_INTRINSICS_
    FLOAT fx = XMVectorGetByIndex(V,(4 - (Elements)) & 3);
    FLOAT fy = XMVectorGetByIndex(V,(5 - (Elements)) & 3);
    FLOAT fz = XMVectorGetByIndex(V,(6 - (Elements)) & 3);
    FLOAT fw = XMVectorGetByIndex(V,(7 - (Elements)) & 3);
    return _mm_set_ps( fw, fz, fy, fx );
#endif
}

//------------------------------------------------------------------------------

XMFINLINE XMVECTOR XMVectorSwizzle(FXMVECTOR V, UINT E0, UINT E1, UINT E2, UINT E3)
{
#if defined(_XM_NO_INTRINSICS_)
    XMASSERT( (E0 < 4) && (E1 < 4) && (E2 < 4) && (E3 < 4) );
    {
    XMVECTORF32 vResult = { V.v[E0], V.v[E1], V.v[E2], V.v[E3] };
    return vResult.v;
    }
#else // XM_SSE_INTRINSICS_
    FLOAT fx = XMVectorGetByIndex(V,E0);
    FLOAT fy = XMVectorGetByIndex(V,E1);
    FLOAT fz = XMVectorGetByIndex(V,E2);
    FLOAT fw = XMVectorGetByIndex(V,E3);
    return _mm_set_ps( fw, fz, fy, fx );
#endif
}

//------------------------------------------------------------------------------

XMFINLINE XMVECTOR XMVectorInsert(FXMVECTOR VD, FXMVECTOR VS, UINT VSLeftRotateElements,
                                  UINT Select0, UINT Select1, UINT Select2, UINT Select3)
{
    XMVECTOR Control = XMVectorSelectControl(Select0&1, Select1&1, Select2&1, Select3&1);
    return XMVectorSelect( VD, XMVectorRotateLeft(VS, VSLeftRotateElements), Control );
}

// Implemented for VMX128 intrinsics as #defines aboves
#endif _XM_NO_INTRINSICS_ || _XM_SSE_INTRINSICS_

//------------------------------------------------------------------------------

#include <xnamathconvert.inl>
#include <xnamathvector.inl>
#include <xnamathmatrix.inl>
#include <xnamathmisc.inl>

#pragma warning(pop)

#endif // __XNAMATH_H__

