﻿//========= Copyright ¿½ 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WIN32FONT_H
#define WIN32FONT_H
#ifdef _WIN32
#pragma once
#endif

#if !defined( _X360 ) && !defined( _PS3 )
#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#include <windows.h>
#endif
#ifdef GetCharABCWidths
#undef GetCharABCWidths
#endif

#include "utlrbtree.h"
#include "tier1/utlsymbol.h"

#if defined(_PS3)
class IFont;
#endif

//-----------------------------------------------------------------------------
// Purpose: encapsulates a windows font
//-----------------------------------------------------------------------------

class CWin32Font
{
public:
	CWin32Font();
	~CWin32Font();

	// creates the font from windows.  returns false if font does not exist in the OS.
	virtual bool Create(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags);

	// writes the char into the specified 32bpp texture
	virtual void GetCharRGBA(wchar_t ch, int rgbaWide, int rgbaTall, unsigned char *rgba);

	// returns true if the font is equivalent to that specified
	virtual bool IsEqualTo(const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags);

	// returns true only if this font is valid for use
	virtual bool IsValid();

	// gets the abc widths for a character
	virtual void GetCharABCWidths(int ch, int &a, int &b, int &c);

#if !defined (_PS3)
	// set the font to be the one to currently draw with in the gdi
	virtual void SetAsActiveFont(HDC hdc);
#endif

	// returns the height of the font, in pixels
	virtual int GetHeight();

	// returns the ascent of the font, in pixels (ascent=units above the base line)
	virtual int GetAscent();

	// returns the maximum width of a character, in pixels
	virtual int GetMaxCharWidth();

	// returns the flags used to make this font
	virtual int GetFlags();

	// returns true if this font is underlined
	bool GetUnderlined() { return m_bUnderlined; }

	// gets the name of this font
	const char *GetName() { return m_szName.String(); }

	// gets the width of ch given its position around before and after chars
	virtual void GetKernedCharWidth( wchar_t ch, wchar_t chBefore, wchar_t chAfter, float &wide, float &abcA, float &abcC );

#if defined( _X360 )
	// generates texture data for a set of chars
	virtual void GetCharsRGBA( newChar_t *newChars, int numNewChars, unsigned char *pRGBA );

	virtual void CloseResource();
#endif

private:

#if !defined( _GAMECONSOLE )
	HFONT			m_hFont;
	HDC				m_hDC;
	HBITMAP			m_hDIB;
#elif defined ( _PS3 )
	IFont           *m_pFont;
#elif defined( _X360 )
	HXUIFONT		m_hFont;
	HDC				m_hDC;
#endif

	// pointer to buffer for use when generated bitmap versions of a texture
	unsigned char	*m_pBuf;

protected:
	CUtlSymbol		m_szName;

	short			m_iTall;

#ifdef _PS3
	int				m_iWeight;
#else
	unsigned short	m_iWeight;
#endif

	unsigned short	m_iFlags;
	unsigned short	m_iScanLines;
	unsigned short	m_iBlur;
	unsigned short	m_rgiBitmapSize[2];

	unsigned int	m_iHeight : 8;
	unsigned int	m_iMaxCharWidth : 8;
	unsigned int	m_iAscent : 8;
	unsigned int	m_iDropShadowOffset : 1;
	unsigned int	m_iOutlineSize : 1;
	unsigned int	m_bAntiAliased : 1;
	unsigned int	m_bRotary : 1;
	unsigned int	m_bAdditive : 1;
	unsigned int	m_bUnderlined : 1; //30

private:
	// abc widths
	struct abc_t
	{
		short b;
		char a;
		char c;
	};

	// cache for additional or asian characters (since it's too big too just store them all)
	struct abc_cache_t
	{
		wchar_t wch;
		abc_t abc;
	};
	CUtlRBTree<abc_cache_t, unsigned short> m_ExtendedABCWidthsCache;
	static bool ExtendedABCWidthsCacheLessFunc(const abc_cache_t &lhs, const abc_cache_t &rhs);

	// First range of characters are automatically cached
#if defined( _PS3 )
	enum { ABCWIDTHS_CACHE_SIZE = 128 };
	abc_t m_ABCWidthsCache[ABCWIDTHS_CACHE_SIZE];
#elif defined( _X360 )
	// 360 requires all possible characters during font init
	enum { ABCWIDTHS_CACHE_SIZE = 256 };
	abc_t m_ABCWidthsCache[ABCWIDTHS_CACHE_SIZE];
#endif
};

#endif // WIN32FONT_H
