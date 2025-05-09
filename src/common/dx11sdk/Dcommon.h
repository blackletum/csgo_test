﻿//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Abstract:
//     Public API definitions for DWrite and D2D
//
//----------------------------------------------------------------------------

#ifndef DCOMMON_H_INCLUDED
#define DCOMMON_H_INCLUDED

//
//These macros are defined in the Windows 7 SDK, however to enable development using the technical preview,
//they are included here temporarily.
//
#ifndef DEFINE_ENUM_FLAG_OPERATORS 
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
extern "C++" { \
inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) | ((int)b)); } \
inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) & ((int)b)); } \
inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((int)a)); } \
inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) ^ ((int)b)); } \
inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
}
#endif

#ifndef __field_ecount_opt
#define __field_ecount_opt(x)
#endif

#ifndef __range
#define __range(x,y) 
#endif

#ifndef __field_ecount
#define __field_ecount(x)
#endif

/// <summary>
/// The measuring method used for text layout.
/// </summary>
typedef enum DWRITE_MEASURING_MODE
{
    /// <summary>
    /// Text is measured using glyph ideal metrics whose values are independent to the current display resolution.
    /// </summary>
    DWRITE_MEASURING_MODE_NATURAL,

    /// <summary>
    /// Text is measured using glyph display compatible metrics whose values tuned for the current display resolution.
    /// </summary>
    DWRITE_MEASURING_MODE_GDI_CLASSIC,

    /// <summary>
    /// Text is measured using the same glyph display metrics as text measured by GDI using a font
    /// created with CLEARTYPE_NATURAL_QUALITY.
    /// </summary>
    DWRITE_MEASURING_MODE_GDI_NATURAL,

    // Deprecated: These aliases are only here to ease potential merge conflicts.
    // They will be removed after January 28. BUG: 394869
    DWRITE_TEXT_MEASURING_METHOD_USE_IDEAL_METRICS = DWRITE_MEASURING_MODE_NATURAL,
    DWRITE_TEXT_MEASURING_METHOD_USE_DISPLAY_COMPATIBLE_METRICS = DWRITE_MEASURING_MODE_GDI_CLASSIC,
    DWRITE_TEXT_MEASURING_METHOD_USE_DISPLAY_NATURAL_METRICS = DWRITE_MEASURING_MODE_GDI_NATURAL

} DWRITE_MEASURING_MODE;

// Deprecated: This alias is only here to ease potential merge conflicts.
// It will be removed after January 28. BUG: 394869
typedef DWRITE_MEASURING_MODE DWRITE_TEXT_MEASURING_METHOD;

#endif /* DCOMMON_H_INCLUDED */
