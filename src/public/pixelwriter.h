﻿//===== Copyright 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef PIXELWRITER_H
#define PIXELWRITER_H

#ifdef _WIN32
#pragma once
#endif

#if defined( _WIN32 ) || defined( _PS3 )
#define FORCEINLINE_PIXEL FORCEINLINE
#elif POSIX
#define FORCEINLINE_PIXEL inline
#else
#error "implement me"
#endif

// This flag allows us to write to formats we we don't support direct pixel access
// (like DXT1) without spewing errors.  The only actions that are available for
// these formats are direct access to the bitstream.
#define ALLOW_UNSUPPORTED_FORMATS 1

#include "bitmap/imageformat.h"
#include "tier0/dbg.h"
#include "mathlib/compressed_vector.h"
#include "mathlib/ssemath.h"
#include "mathlib/vector4d.h"
#include "cache_hints.h"

//-----------------------------------------------------------------------------
// Color writing class 
//-----------------------------------------------------------------------------

class CPixelWriter
{
public:
	FORCEINLINE void SetPixelMemory( ImageFormat format, void* pMemory, int stride );
	FORCEINLINE void *GetPixelMemory() { return m_pBase; }

	// this is no longer used:
#if 0 // defined( _X360 )
	// set after SetPixelMemory() 
	FORCEINLINE void ActivateByteSwapping( bool bSwap );
#endif

	FORCEINLINE void Seek( int x, int y );
	FORCEINLINE void* SkipBytes( int n ) RESTRICT;
	FORCEINLINE void SkipPixels( int n );	
	FORCEINLINE void WritePixel( int r, int g, int b, int a = 255 );
	FORCEINLINE void WritePixelNoAdvance( int r, int g, int b, int a = 255 );
	FORCEINLINE void WritePixelSigned( int r, int g, int b, int a = 255 );
	FORCEINLINE void WritePixelNoAdvanceSigned( int r, int g, int b, int a = 255 );
	FORCEINLINE void ReadPixelNoAdvance( int &r, int &g, int &b, int &a );

	// Floating point formats
	FORCEINLINE void WritePixelNoAdvanceF( float r, float g, float b, float a = 1.0f );
	FORCEINLINE void WritePixelF( float r, float g, float b, float a = 1.0f );
	FORCEINLINE void WriteManyPixelF( const float * RESTRICT pSrc, const int num  ); // write a contiguous stream of 4-floats. 



	// SIMD formats
	FORCEINLINE void WritePixel( FLTX4 rgba ) RESTRICT;
	FORCEINLINE void WritePixelNoAdvance( FLTX4 rgba ) RESTRICT;

#if defined ( _X360 ) || defined  ( _PS3 )
	// here are some explicit formats so we can avoid the switch:
	FORCEINLINE void WritePixelNoAdvance_RGBA8888( FLTX4 rgba );
	FORCEINLINE void WritePixelNoAdvance_BGRA8888( FLTX4 rgba );
	// as above, but with m_pBits passed in to avoid a LHS
	FORCEINLINE void WritePixelNoAdvance_BGRA8888( FLTX4 rgba, void *pBits ) RESTRICT;
	// for writing entire SIMD registers at once when they have
	// already been packed, and when m_pBits is vector-aligned
	// (which is a requirement for write-combined memory)
	// offset is added to m_pBits (saving you from the obligatory
	// LHS of a SkipBytes)
	FORCEINLINE void WriteFourPixelsExplicitLocation_BGRA8888( FLTX4 rgba, int offset );

	FORCEINLINE void WritePixelNoAdvance_RGBA16161616( FLTX4 rgba );
#endif

	FORCEINLINE void WritePixelNoAdvance16F( float r, float g, float b, float a );


	FORCEINLINE unsigned char GetPixelSize() { return m_Size; }	
	FORCEINLINE unsigned short GetBytesPerRow() { return m_BytesPerRow; }

	FORCEINLINE bool IsUsingFloatFormat() const;
	FORCEINLINE bool IsUsing16BitFloatFormat() const;

	// We allow "unsupported" formats only if you are writing directly into the bitstream
	FORCEINLINE bool IsUsingSupportedFormat() const;

	FORCEINLINE unsigned char *GetCurrentPixel() { return m_pBits; }

private:
	// helper functions for some explicit combinations of flags and sizes -- lets us
	// do some conversions on the GPRs using bitshifts rather than a round trip to the
	// FPU and a LHS.
	FORCEINLINE void WriteManyPixelTo16BitF( const float * RESTRICT pSrc, int num  ) RESTRICT; // write a contiguous stream of 4-floats. 
	// FORCEINLINE void WriteManyPixelTo32BitF( const float * RESTRICT pSrc, const int num  ); // write a contiguous stream of 4-floats. 

	FORCEINLINE void AssertFormatIsSupported( ImageFormat format ) const;

	enum
	{
		PIXELWRITER_USING_FLOAT_FORMAT       = 0x01,
		PIXELWRITER_USING_16BIT_FLOAT_FORMAT = 0x02,
		PIXELWRITER_SWAPBYTES                = 0x04,
		PIXELWRITER_USING_UNSUPPORTED_FORMAT = 0x08,
	};

	unsigned char*	m_pBase;
	unsigned char*	m_pBits;
	unsigned short	m_BytesPerRow;
	unsigned char	m_Size;
	unsigned char	m_nFlags;
	signed short	m_RShift;
	signed short	m_GShift;
	signed short	m_BShift;
	signed short	m_AShift;
	unsigned int	m_RMask;
	unsigned int	m_GMask;
	unsigned int	m_BMask;
	unsigned int	m_AMask;

#if defined ( _X360 ) || defined  ( _PS3 )
	ImageFormat		m_Format;
public:
	inline const ImageFormat &GetFormat() { return m_Format; }
private:
#endif
};

FORCEINLINE_PIXEL bool CPixelWriter::IsUsingFloatFormat() const
{
	return (m_nFlags & PIXELWRITER_USING_FLOAT_FORMAT) != 0;
}

FORCEINLINE_PIXEL bool CPixelWriter::IsUsing16BitFloatFormat() const
{
	return (m_nFlags & PIXELWRITER_USING_16BIT_FLOAT_FORMAT) != 0;
}

FORCEINLINE_PIXEL bool CPixelWriter::IsUsingSupportedFormat() const
{
	return (m_nFlags & PIXELWRITER_USING_UNSUPPORTED_FORMAT) == 0;
}

FORCEINLINE_PIXEL void CPixelWriter::SetPixelMemory( ImageFormat format, void* pMemory, int stride )
{
	m_pBits = (unsigned char*)pMemory;
	m_pBase = m_pBits;
	m_BytesPerRow = (unsigned short)stride;
	m_nFlags = 0;
#if defined ( _X360 ) || defined  ( _PS3 )
	m_Format = format;
#endif

	switch ( format )
	{
	case IMAGE_FORMAT_R32F: // NOTE! : the low order bits are first in this naming convention.
		m_Size = 4;
		m_RShift = 0;
		m_GShift = 0;
		m_BShift = 0;
		m_AShift = 0;
		m_RMask = 0xFFFFFFFF;
		m_GMask = 0x0;
		m_BMask = 0x0;
		m_AMask = 0x0;
		m_nFlags |= PIXELWRITER_USING_FLOAT_FORMAT;
		break;

	case IMAGE_FORMAT_RGBA32323232F:
		m_Size = 16;
		m_RShift = 0;
		m_GShift = 32;
		m_BShift = 64;
		m_AShift = 96;
		m_RMask = 0xFFFFFFFF;
		m_GMask = 0xFFFFFFFF;
		m_BMask = 0xFFFFFFFF;
		m_AMask = 0xFFFFFFFF;
		m_nFlags |= PIXELWRITER_USING_FLOAT_FORMAT;
		break;

	case IMAGE_FORMAT_RGBA16161616F:
		m_Size = 8;
		m_RShift = 0;
		m_GShift = 16;
		m_BShift = 32;
		m_AShift = 48;
		m_RMask = 0xFFFF;
		m_GMask = 0xFFFF;
		m_BMask = 0xFFFF;
		m_AMask = 0xFFFF;
		m_nFlags |= PIXELWRITER_USING_FLOAT_FORMAT | PIXELWRITER_USING_16BIT_FLOAT_FORMAT;
		break;

	case IMAGE_FORMAT_RGBA8888:
#if defined( _X360 )
	case IMAGE_FORMAT_LINEAR_RGBA8888:
#endif
		m_Size = 4;
		m_RShift = 0;
		m_GShift = 8;
		m_BShift = 16;
		m_AShift = 24;
		m_RMask = 0xFF;
		m_GMask = 0xFF;
		m_BMask = 0xFF;
		m_AMask = 0xFF;
		break;

	case IMAGE_FORMAT_BGRA1010102: // NOTE! : the low order bits are first in this naming convention.
		m_Size = 4;
		m_RShift = 20;
		m_GShift = 10;
		m_BShift = 0;
		m_AShift = 30;
		m_RMask = 0x3FF;
		m_GMask = 0x3FF;
		m_BMask = 0x3FF;
		m_AMask = 0x03;
		break;

	case IMAGE_FORMAT_BGRA8888: // NOTE! : the low order bits are first in this naming convention.
#if defined( _X360 )
	case IMAGE_FORMAT_LINEAR_BGRA8888:
#endif
		m_Size = 4;
		m_RShift = 16;
		m_GShift = 8;
		m_BShift = 0;
		m_AShift = 24;
		m_RMask = 0xFF;
		m_GMask = 0xFF;
		m_BMask = 0xFF;
		m_AMask = 0xFF;
		break;

	case IMAGE_FORMAT_BGRX8888:
#if defined( _X360 )
	case IMAGE_FORMAT_LINEAR_BGRX8888:
#endif
		m_Size = 4;
		m_RShift = 16;
		m_GShift = 8;
		m_BShift = 0;
		m_AShift = 24;
		m_RMask = 0xFF;
		m_GMask = 0xFF;
		m_BMask = 0xFF;
		m_AMask = 0x00;
		break;

	case IMAGE_FORMAT_BGRA4444:
		m_Size = 2;
		m_RShift = 4;
		m_GShift = 0;
		m_BShift = -4;
		m_AShift = 8;
		m_RMask = 0xF0;
		m_GMask = 0xF0;
		m_BMask = 0xF0;
		m_AMask = 0xF0;
		break;

	case IMAGE_FORMAT_BGR888:
		m_Size = 3;
		m_RShift = 16;
		m_GShift = 8;
		m_BShift = 0;
		m_AShift = 0;
		m_RMask = 0xFF;
		m_GMask = 0xFF;
		m_BMask = 0xFF;
		m_AMask = 0x00;
		break;

	case IMAGE_FORMAT_BGR565:
		m_Size = 2;
		m_RShift = 8;
		m_GShift = 3;
		m_BShift = -3;
		m_AShift = 0;
		m_RMask = 0xF8;
		m_GMask = 0xFC;
		m_BMask = 0xF8;
		m_AMask = 0x00;
		break;

	case IMAGE_FORMAT_BGRA5551:
	case IMAGE_FORMAT_BGRX5551:
		m_Size = 2;
		m_RShift = 7;
		m_GShift = 2;
		m_BShift = -3;
		m_AShift = 8;
		m_RMask = 0xF8;
		m_GMask = 0xF8;
		m_BMask = 0xF8;
		m_AMask = 0x80;
		break;

	// GR - alpha format for HDR support
	case IMAGE_FORMAT_A8:
#if defined( _X360 )
	case IMAGE_FORMAT_LINEAR_A8:
#endif
		m_Size = 1;
		m_RShift = 0;
		m_GShift = 0;
		m_BShift = 0;
		m_AShift = 0;
		m_RMask = 0x00;
		m_GMask = 0x00;
		m_BMask = 0x00;
		m_AMask = 0xFF;
		break;

	case IMAGE_FORMAT_UVWQ8888:
		m_Size = 4;
		m_RShift = 0;
		m_GShift = 8;
		m_BShift = 16;
		m_AShift = 24;
		m_RMask = 0xFF;
		m_GMask = 0xFF;
		m_BMask = 0xFF;
		m_AMask = 0xFF;
		break;

	case IMAGE_FORMAT_RGBA16161616:
#if defined( _X360 )
	case IMAGE_FORMAT_LINEAR_RGBA16161616:
#endif		
		m_Size = 8;
		if ( !IsX360() )
		{
			m_RShift = 0;
			m_GShift = 16;
			m_BShift = 32;
			m_AShift = 48;
		}
		else
		{
			m_RShift = 48;
			m_GShift = 32;
			m_BShift = 16;
			m_AShift = 0;
		}
		m_RMask = 0xFFFF;
		m_GMask = 0xFFFF;
		m_BMask = 0xFFFF;
		m_AMask = 0xFFFF;
		break;

	case IMAGE_FORMAT_I8:
#if defined( _X360 )
	case IMAGE_FORMAT_LINEAR_I8:
#endif
		// whatever goes into R is considered the intensity.
		m_Size = 1;
		m_RShift = 0;
		m_GShift = 0;
		m_BShift = 0;
		m_AShift = 0;
		m_RMask = 0xFF;
		m_GMask = 0x00;
		m_BMask = 0x00;
		m_AMask = 0x00;
		break;
	// FIXME: Add more color formats as need arises
	default:
		{
#if ALLOW_UNSUPPORTED_FORMATS
			m_nFlags |= PIXELWRITER_USING_UNSUPPORTED_FORMAT;
#else // ALLOW_UNSUPPORTED_FORMATS
			static bool format_error_printed[NUM_IMAGE_FORMATS];
			if ( !format_error_printed[format] )
			{
				Assert( 0 );
				Msg( "CPixelWriter::SetPixelMemory:  Unsupported image format %i\n", format );
				format_error_printed[format] = true;
			}
#endif // ALLOW_UNSUPPORTED_FORMATS
			m_Size = 0; // set to zero so that we don't stomp memory for formats that we don't understand.
            m_RShift = 0;
            m_GShift = 0;
            m_BShift = 0;
            m_AShift = 0;
            m_RMask  = 0xFF;
            m_GMask  = 0x00;
            m_BMask  = 0x00;
            m_AMask  = 0x00;
		}
		break;
	}
}

#if 0 // defined( _X360 )
FORCEINLINE void CPixelWriter::ActivateByteSwapping( bool bSwap )
{
	// X360TBD: Who is trying to use this?
	// Purposely not hooked up because PixelWriter has been ported to read/write native pixels only
	Assert( 0 );

	if ( bSwap && !(m_nFlags & PIXELWRITER_SWAPBYTES ) )
	{
		m_nFlags |= PIXELWRITER_SWAPBYTES;

		// only tested with 4 byte formats
		Assert( m_Size == 4 );
	}
	else if ( !bSwap && (m_nFlags & PIXELWRITER_SWAPBYTES ) )
	{
		m_nFlags &= ~PIXELWRITER_SWAPBYTES;
	}
	else
	{
		// same state
		return;
	}

	// swap the shifts
	m_RShift = 24-m_RShift;
	m_GShift = 24-m_GShift;
	m_BShift = 24-m_BShift;
	m_AShift = 24-m_AShift;
}
#endif

//-----------------------------------------------------------------------------
// Sets where we're writing to
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::Seek( int x, int y )
{
	Assert( IsUsingSupportedFormat() );
	m_pBits = m_pBase + y * m_BytesPerRow + x * m_Size;
}


//-----------------------------------------------------------------------------
// Skips n bytes:
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void* CPixelWriter::SkipBytes( int n ) RESTRICT
{
	m_pBits += n;
	return m_pBits;
}


//-----------------------------------------------------------------------------
// Skips n pixels:
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::SkipPixels( int n )
{
	Assert( IsUsingSupportedFormat() );
	SkipBytes( n * m_Size );
}

//-----------------------------------------------------------------------------
// Writes a pixel without advancing the index		PC ONLY
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WritePixelNoAdvanceF( float r, float g, float b, float a )
{
	Assert( IsUsingSupportedFormat() );
	Assert( IsUsingFloatFormat() );

	// X360TBD: Not ported
	Assert( IsPC() || IsPS3() );

	if (PIXELWRITER_USING_16BIT_FLOAT_FORMAT & m_nFlags)
	{
		WritePixelNoAdvance16F( r,g,b,a );
	}
	else
	{
		// fp32
		int pBuf[4] = { 0, 0, 0, 0 };
		pBuf[ m_RShift >> 5 ] |= (FloatBits(r) & m_RMask) << ( m_RShift & 0x1F );
		pBuf[ m_GShift >> 5 ] |= (FloatBits(g) & m_GMask) << ( m_GShift & 0x1F );
		pBuf[ m_BShift >> 5 ] |= (FloatBits(b) & m_BMask) << ( m_BShift & 0x1F );
		pBuf[ m_AShift >> 5 ] |= (FloatBits(a) & m_AMask) << ( m_AShift & 0x1F );
		memcpy( m_pBits, pBuf, m_Size );
	}
}


FORCEINLINE void CPixelWriter::WritePixelNoAdvance16F( float r, float g, float b, float a )
{
	if ( IsPS3() )
	{
		// we know what the values of shift and mask are going to be because 
		// of the format, so we can elide them and write directly
		float16 *fp16 = reinterpret_cast<float16 *>(m_pBits);

		fp16[0].SetFloat( r );
		fp16[1].SetFloat( g );
		fp16[2].SetFloat( b );
		fp16[3].SetFloat( a );
	}
	else
	{
		float16 fp16[4];
		fp16[0].SetFloat( r );
		fp16[1].SetFloat( g );
		fp16[2].SetFloat( b );
		fp16[3].SetFloat( a );
		// fp16
		unsigned short pBuf[4] = { 0, 0, 0, 0 };
		pBuf[ m_RShift >> 4 ] |= (fp16[0].GetBits() & m_RMask) << ( m_RShift & 0xF );
		pBuf[ m_GShift >> 4 ] |= (fp16[1].GetBits() & m_GMask) << ( m_GShift & 0xF );
		pBuf[ m_BShift >> 4 ] |= (fp16[2].GetBits() & m_BMask) << ( m_BShift & 0xF );
		pBuf[ m_AShift >> 4 ] |= (fp16[3].GetBits() & m_AMask) << ( m_AShift & 0xF );
		memcpy( m_pBits, pBuf, m_Size );
	}
}

//-----------------------------------------------------------------------------
// Writes a lot of pixels, efficiently		
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WriteManyPixelTo16BitF( const float * RESTRICT pSrc, int num  ) RESTRICT
{
	Assert( IsUsingSupportedFormat() );
	const static int SIZE = 4*sizeof(unsigned short); // known precondition
	const static int MASK = 0xFFFF;
	// another known precondition:  m_RShift == 0 && m_GShift == 16 && m_BShift == 32 && m_AShift == 48 


	unsigned char *pBits = m_pBits; // compiler actually fails to hoist this onto a register properly otherwise.
	for ( int i = 0; num; --num, ++i )
	{
		/* // this actually slowed things down, for whatever perverse reason.
		// every cache line boundary, prefetch the next in bloc, so long as we've at least 128 bytes left to go.
		// the destination is in noncacheable memory.
		if ( (num > 32) && ( (reinterpret_cast<unsigned int>(pSrc) & 127) == 0 ) )
		{
			PREFETCH_128( pSrc, 128 );
		}
		*/

		float16 * RESTRICT pOut = reinterpret_cast< float16 * >(pBits);
		pOut[0].SetFloat( pSrc[0] );
		pOut[1].SetFloat( pSrc[1] );
		pOut[2].SetFloat( pSrc[2] );
		pOut[3].SetFloat( pSrc[3] );

		/*
		pAck[i+0].SetFloat( pSrc[0] );
		pAck[i+1].SetFloat( pSrc[1] );
		pAck[i+2].SetFloat( pSrc[2] );
		pAck[i+3].SetFloat( pSrc[3] );
		*/

		pSrc += 4;
		pBits += SIZE;
	}
	

	m_pBits = pBits;
}



//-----------------------------------------------------------------------------
// Writes a pixel, advances the write index 
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WritePixelF( float r, float g, float b, float a )
{
	WritePixelNoAdvanceF(r, g, b, a);
	m_pBits += m_Size;
}

//-----------------------------------------------------------------------------
// Writes an array of pixels, advancing the write index. 
// the input data is required to be a contiguous stream of Vector4Ds
// (ie, each pixel consists of four consecutive floats, and the data is 
//  consecutive in memory)
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WriteManyPixelF( const float * RESTRICT pSrc, const int num  )
{
	Assert( IsUsingSupportedFormat() );
	Assert( IsUsingFloatFormat() );
	// X360TBD: Not ported
	Assert( IsPC() || IsPS3() );

	if ( m_Size == 4*sizeof(unsigned short) && (PIXELWRITER_USING_16BIT_FLOAT_FORMAT & m_nFlags) )
	{
		Assert( m_RShift == 0 && m_GShift == 16 && m_BShift == 32 && m_AShift == 48 );
		WriteManyPixelTo16BitF( pSrc, num );
	}
	/*
	else if ( m_Size == 4*sizeof(int) && !(PIXELWRITER_USING_16BIT_FLOAT_FORMAT & m_nFlags) )
	{
		WriteManyPixelTo32BitF( pSrc, num );
	}
	*/
	else for ( const float * const sentinel = pSrc + ( num * 4 ); pSrc < sentinel; pSrc += 4 ) // naive general case
	{
		WritePixelF( pSrc[0], pSrc[1], pSrc[2], pSrc[3] );
	}
}

//-----------------------------------------------------------------------------
// Writes a pixel, advances the write index 
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WritePixel( int r, int g, int b, int a )
{
	WritePixelNoAdvance(r,g,b,a);
	m_pBits += m_Size;
}

//-----------------------------------------------------------------------------
// Writes a pixel, advances the write index 
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WritePixelSigned( int r, int g, int b, int a )
{
	WritePixelNoAdvanceSigned(r,g,b,a);
	m_pBits += m_Size;
}


//-----------------------------------------------------------------------------
// Writes a pixel without advancing the index
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WritePixelNoAdvance( int r, int g, int b, int a )
{
	Assert( IsUsingSupportedFormat() );
	Assert( !IsUsingFloatFormat() );

	if ( m_Size <= 0 )
	{
		return;
	}
	if ( m_Size < 5 )
	{
		unsigned int val = (r & m_RMask) << m_RShift;
		val |=  (g & m_GMask) << m_GShift;
		val |= (m_BShift > 0) ? ((b & m_BMask) << m_BShift) : ((b & m_BMask) >> -m_BShift);
		val |=	(a & m_AMask) << m_AShift;

		switch( m_Size )
		{
		default:
			Assert( 0 );
			return;
		case 1:
			{
				m_pBits[0] = (unsigned char)((val & 0xff));
				return;
			}
		case 2:
			{
				((unsigned short *)m_pBits)[0] = (unsigned short)((val & 0xffff));
				return;
			}
		case 3:
			{
				if ( IsPC() || IsPS3() || !IsX360() )
				{
					((unsigned short *)m_pBits)[0] = (unsigned short)((val & 0xffff));
					m_pBits[2] = (unsigned char)((val >> 16) & 0xff);
				}
				else
				{
					m_pBits[0] = (unsigned char)(((val >> 16) & 0xff));
					m_pBits[1] = (unsigned char)(((val >> 8 ) & 0xff));
					m_pBits[2] = (unsigned char)(val & 0xff);
				}
				return;
			}
		case 4:
			{
				((unsigned int *)m_pBits)[0] = val;
				return;
			}
		}
	}
	else	// RGBA32323232 or RGBA16161616 -- PC only.
	{
		AssertMsgOnce(!IsX360(), "Unsupported lightmap format used in WritePixelNoAdvance(). This is a severe performance fault.\n");
//		AssertMsg(!IsX360(), "Unsupported lightmap format used in WritePixelNoAdvance(). This is a severe performance fault.\n");

		int64 val = ( ( int64 )(r & m_RMask) ) << m_RShift;
		val |=  ( ( int64 )(g & m_GMask) ) << m_GShift;
		val |= (m_BShift > 0) ? ((( int64 )( b & m_BMask)) << m_BShift) : (((int64)( b & m_BMask)) >> -m_BShift);
		val |=	( ( int64 )(a & m_AMask) ) << m_AShift;

		switch( m_Size )
		{
		case 6:
			{
				if ( IsPC() || IsPS3() || !IsX360() )
				{
					((unsigned int *)m_pBits)[0] = val & 0xffffffff;
					((unsigned short *)m_pBits)[2] = (unsigned short)( ( val >> 32 ) & 0xffff );
				}
				else
				{
					((unsigned int *)m_pBits)[0] = (val >> 16) & 0xffffffff;
					((unsigned short *)m_pBits)[2] = (unsigned short)( val & 0xffff );
				}
				return;
			}
		case 8:
			{
				if ( IsPC() || IsPS3() || !IsX360() )
				{
					((unsigned int *)m_pBits)[0] = val & 0xffffffff;
					((unsigned int *)m_pBits)[1] = ( val >> 32 ) & 0xffffffff;
				}
				else
				{
					((unsigned int *)m_pBits)[0] = ( val >> 32 ) & 0xffffffff;
					((unsigned int *)m_pBits)[1] = val & 0xffffffff;
				}
				return;
			}
		default:
			Assert( 0 );
			return;
		}
	}
}


#ifdef _X360
// There isn't a PC port of these because of the many varied
// pixel formats the PC deals with. If you write SSE versions 
// of all the various necessary packers, then this can be made
// to work on PC.

//-----------------------------------------------------------------------------
// Writes a pixel, advances the write index 
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WritePixel( FLTX4 rgba ) RESTRICT
{
	WritePixelNoAdvance(rgba);
	m_pBits += m_Size;
}

//-----------------------------------------------------------------------------
// Writes a pixel without advancing the index
// rgba are four float values, each on the range 0..255 (though they may leak
// fractionally over 255 due to numerical errors earlier)
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WritePixelNoAdvance( FLTX4 rgba ) RESTRICT
{
	Assert( IsUsingSupportedFormat() );
	Assert( !IsUsingFloatFormat() );

	switch (m_Size)
	{
	case 0:
		return;
	case 4:
	{
		AssertMsg((reinterpret_cast<unsigned int>(m_pBits) & 0x03) == 0,"Unaligned m_pBits in WritePixelNoAdvance!");
		switch ( m_Format )
		{
			// note: format names are low-order-byte first. 
		case IMAGE_FORMAT_RGBA8888:
		case IMAGE_FORMAT_LINEAR_RGBA8888:
			WritePixelNoAdvance_RGBA8888(rgba);
			break;

		case IMAGE_FORMAT_BGRA8888: // NOTE! : the low order bits are first in this naming convention.
		case IMAGE_FORMAT_LINEAR_BGRA8888:
			WritePixelNoAdvance_BGRA8888(rgba);
			break;
			

		default:
			AssertMsg1(false, "Unknown four-byte pixel format %d in lightmap write.\n", m_Format);
		}
		break;
	}

	case 8:
	{
		switch ( m_Format )
		{
			// note: format names are low-order-byte first. 
		case IMAGE_FORMAT_RGBA16161616:
		case IMAGE_FORMAT_LINEAR_RGBA16161616:
			WritePixelNoAdvance_RGBA16161616(rgba);
			break;

		default:
			AssertMsg1(false, "Unknown eight-byte pixel format %d in lightmap write.\n", m_Format);
		}
		break;

	}
	default:
		AssertMsg1(false, "WritePixelNoAdvance on unsupported 360 %d-byte format\n", m_Size);
		break;
	}

}


// here are some explicit formats so we can avoid the switch:
FORCEINLINE void CPixelWriter::WritePixelNoAdvance_RGBA8888( FLTX4 rgba )
{
	// it's easier to do tiered convert-saturates here 
	// than  the d3d color convertor op

	// first permute
	const static fltx4 permReverse = XMVectorPermuteControl(3,2,1,0);
	fltx4 N = XMVectorPermute(rgba, rgba, permReverse);

	N = __vctuxs(N, 0); // convert to unsigned fixed point 0 w/ saturate
	N = __vpkuwus(N, N); // convert to halfword saturate
	N = __vpkuhus(N, N); // convert to byte saturate
	N = __vspltw(N, 0);  // splat w-word to all four

	__stvewx(N, m_pBits, 0); // store whatever word happens to be aligned with m_pBits to that word 
}

FORCEINLINE void CPixelWriter::WritePixelNoAdvance_BGRA8888( FLTX4 rgba )
{
	WritePixelNoAdvance_BGRA8888( rgba, m_pBits );
}

FORCEINLINE void CPixelWriter::WritePixelNoAdvance_RGBA16161616( FLTX4 rgba )
{
	// input is in 0..16 range. 
	//Multiply by 4096 to get into 0..65536 range
	static const fltx4 vMult = { 4096.0f, 4096.0f, 4096.0f, 65536.0f };
	rgba = XMVectorMultiply( rgba, vMult );
	XMStoreUShort4( (XMUSHORT4*)m_pBits, rgba );
}

FORCEINLINE void CPixelWriter::WritePixelNoAdvance_BGRA8888( FLTX4 rgba, void * RESTRICT pBits ) RESTRICT
{
	// this happens to be in an order such that we can use the handy builtin packing op
	// clamp to 0..255 (coz it might have leaked over)
	static const fltx4 vTwoFiftyFive = {255.0f, 255.0f, 255.0f, 255.0f};
	fltx4 N = MinSIMD(vTwoFiftyFive, rgba); 

	// the magic number such that when mul-accummulated against rbga,
	// gets us a representation 3.0 + (r)*2^-22 -- puts the bits at
	// the bottom of the float
	static CONST XMVECTOR   PackScale = { (1.0f / (FLOAT)(1 << 22)), (1.0f / (FLOAT)(1 << 22)), (1.0f / (FLOAT)(1 << 22)), (1.0f / (FLOAT)(1 << 22))}; // 255.0f / (FLOAT)(1 << 22)
	static const XMVECTOR   Three = {3.0f, 3.0f, 3.0f, 3.0f};

	N = __vmaddfp(N, PackScale, Three);
	N = __vpkd3d(N, N, VPACK_D3DCOLOR, VPACK_32, 3); // pack to X word
	N = __vspltw(N, 0); // splat X

	// this is a nasty thing to work around the April XDK bug in __stvewx
	{
		void * RESTRICT copyOfPBits = pBits;
		__stvewx(N, copyOfPBits, 0);
	}

}

// for writing entire SIMD registers at once
FORCEINLINE void CPixelWriter::WriteFourPixelsExplicitLocation_BGRA8888 ( FLTX4 rgba, int offset )
{
	Assert( (reinterpret_cast<unsigned int>(m_pBits) & 15) == 0 ); // assert alignment
	XMStoreVector4A( m_pBits + offset , rgba );
}

#elif defined ( _PS3 )

// There isn't a PC port of these because of the many varied
// pixel formats the PC deals with. If you write SSE versions 
// of all the various necessary packers, then this can be made
// to work on PC.

//-----------------------------------------------------------------------------
// Writes a pixel, advances the write index 
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WritePixel( FLTX4 rgba ) RESTRICT
{
	WritePixelNoAdvance(rgba);
	m_pBits += m_Size;
}

//-----------------------------------------------------------------------------
// Writes a pixel without advancing the index
// rgba are four float values, each on the range 0..255 (though they may leak
// fractionally over 255 due to numerical errors earlier)
//-----------------------------------------------------------------------------
FORCEINLINE_PIXEL void CPixelWriter::WritePixelNoAdvance( FLTX4 rgba ) RESTRICT
{
	Assert( IsUsingSupportedFormat() );
	Assert( !IsUsingFloatFormat() );

	switch (m_Size)
	{
	case 0:
		return;
	case 4:
		{
			AssertMsg((reinterpret_cast<unsigned int>(m_pBits) & 0x03) == 0,"Unaligned m_pBits in WritePixelNoAdvance!");
			switch ( m_Format )
			{
				// note: format names are low-order-byte first. 
			case IMAGE_FORMAT_RGBA8888:
			case IMAGE_FORMAT_LINEAR_RGBA8888:
				WritePixelNoAdvance_RGBA8888(rgba);
				break;

			case IMAGE_FORMAT_BGRA8888: // NOTE! : the low order bits are first in this naming convention.
			//EAPS3	case IMAGE_FORMAT_LINEAR_BGRA8888:
				WritePixelNoAdvance_BGRA8888(rgba);
				break;


			default:
				AssertMsg1(false, "Unknown four-byte pixel format %d in lightmap write.\n", m_Format);
			}
			break;
		}

	default:
		AssertMsg1(false, "WritePixelNoAdvance on unsupported 360 %d-byte format\n", m_Size);
		break;
	}

}


// here are some explicit formats so we can avoid the switch:
FORCEINLINE void CPixelWriter::WritePixelNoAdvance_RGBA8888( FLTX4 rgba )
{
	// it's easier to do tiered convert-saturates here 
	// than  the d3d color convertor op

	// first permute

	fltx4 N = vec_perm(rgba, rgba, _VEC_SWIZZLE_WZYX);

	vector unsigned int   N_ui = vec_ctu(N, 0); // convert to unsigned fixed point 0 w/ saturate
	vector unsigned short N_us = vec_packsu(N_ui, N_ui); // convert to halfword saturate
	vector unsigned char  N_uc = vec_packsu(N_us, N_us); // convert to byte saturate
// don't need to do this, should already be unpacked to all elements in the same way
//	N = vec_splat((fltx4)N_uc, 0);  // splat w-word to all four
//	vec_ste(N, 0, m_pBits); // store whatever word happens to be aligned with m_pBits to that word 

	vec_ste((vec_uint4)N_uc, 0, (unsigned int *)m_pBits); // store whatever word happens to be aligned with m_pBits to that word 
}

FORCEINLINE void CPixelWriter::WritePixelNoAdvance_BGRA8888( FLTX4 rgba )
{
	WritePixelNoAdvance_BGRA8888( rgba, m_pBits );
}

FORCEINLINE void CPixelWriter::WritePixelNoAdvance_BGRA8888( FLTX4 rgba, void * RESTRICT pBits ) RESTRICT
{
	fltx4 N;
	vector unsigned int   N_ui = vec_ctu(rgba, 0); // convert to unsigned fixed point 0 w/ saturate
	vector unsigned short N_us = vec_packsu(N_ui, N_ui); // convert to halfword saturate
	vector unsigned char  N_uc = vec_packsu(N_us, N_us); // convert to byte saturate
//	N = vec_splat((fltx4)N_uc, 0);  // splat w-word to all four
//	vec_ste(N, 0, (float*)pBits); // store whatever word happens to be aligned with m_pBits to that word 

	vec_ste((vec_uint4)N_uc, 0, (unsigned int*)pBits); // store whatever word happens to be aligned with m_pBits to that word 

}

// for writing entire SIMD registers at once
FORCEINLINE void CPixelWriter::WriteFourPixelsExplicitLocation_BGRA8888 ( FLTX4 rgba, int offset )
{
	Assert( (reinterpret_cast<unsigned int>(m_pBits) & 15) == 0 && offset == 0 ); // assert alignment
//	XMStoreVector4A( m_pBits + offset , rgba );
	vec_st( rgba, offset, (float*)m_pBits );

}

#endif

//-----------------------------------------------------------------------------
// Writes a signed pixel without advancing the index
//-----------------------------------------------------------------------------

FORCEINLINE_PIXEL void CPixelWriter::WritePixelNoAdvanceSigned( int r, int g, int b, int a )
{
	Assert( IsUsingSupportedFormat() );
	Assert( !IsUsingFloatFormat() );

	if ( m_Size <= 0 )
	{
		return;
	}

	if ( m_Size < 5 )
	{
		int val = (r & m_RMask) << m_RShift;
		val |=  (g & m_GMask) << m_GShift;
		val |= (m_BShift > 0) ? ((b & m_BMask) << m_BShift) : ((b & m_BMask) >> -m_BShift);
		val |=	(a & m_AMask) << m_AShift;
		signed char *pSignedBits = (signed char *)m_pBits;

		if ( IsPC() || IsPS3() || !IsX360() )
		{
			switch ( m_Size )
			{
			case 4:
				pSignedBits[3] = (signed char)((val >> 24) & 0xff);
				// fall through intentionally.
			case 3:
				pSignedBits[2] = (signed char)((val >> 16) & 0xff);
				// fall through intentionally.
			case 2:
				pSignedBits[1] = (signed char)((val >> 8) & 0xff);
				// fall through intentionally.
			case 1:
				pSignedBits[0] = (signed char)((val & 0xff));
				// fall through intentionally.
				return;
			}
		}
		else
		{
			switch ( m_Size )
			{
			case 4:
				pSignedBits[0] = (signed char)((val >> 24) & 0xff);
				pSignedBits[1] = (signed char)((val >> 16) & 0xff);
				pSignedBits[2] = (signed char)((val >> 8) & 0xff);
				pSignedBits[3] = (signed char)(val & 0xff);
				break;
			case 3:
				pSignedBits[0] = (signed char)((val >> 16) & 0xff);
				pSignedBits[1] = (signed char)((val >> 8) & 0xff);
				pSignedBits[2] = (signed char)(val & 0xff);
				break;
			case 2:
				pSignedBits[0] = (signed char)((val >> 8) & 0xff);
				pSignedBits[1] = (signed char)(val & 0xff);
				break;
			case 1:
				pSignedBits[0] = (signed char)(val & 0xff);
				break;
			}
		}
	}
	else
	{
		int64 val = ( ( int64 )(r & m_RMask) ) << m_RShift;
		val |=  ( ( int64 )(g & m_GMask) ) << m_GShift;
		val |= (m_BShift > 0) ? ((( int64 )( b & m_BMask)) << m_BShift) : (((int64)( b & m_BMask)) >> -m_BShift);
		val |=	( ( int64 )(a & m_AMask) ) << m_AShift;
		signed char *pSignedBits = ( signed char * )m_pBits;

		if ( IsPC() || IsPS3() || !IsX360() )
		{
			switch( m_Size )
			{
			case 8:
				pSignedBits[7] = (signed char)((val >> 56) & 0xff);
				pSignedBits[6] = (signed char)((val >> 48) & 0xff);
				// fall through intentionally.
			case 6:
				pSignedBits[5] = (signed char)((val >> 40) & 0xff);
				pSignedBits[4] = (signed char)((val >> 32) & 0xff);
				// fall through intentionally.
			case 4:
				pSignedBits[3] = (signed char)((val >> 24) & 0xff);
				// fall through intentionally.
			case 3:
				pSignedBits[2] = (signed char)((val >> 16) & 0xff);
				// fall through intentionally.
			case 2:
				pSignedBits[1] = (signed char)((val >> 8) & 0xff);
				// fall through intentionally.
			case 1:
				pSignedBits[0] = (signed char)((val & 0xff));
				break;
			default:
				Assert( 0 );
				return;
			}
		}
		else
		{
			switch( m_Size )
			{
			case 8:
				pSignedBits[0] = (signed char)((val >> 56) & 0xff);
				pSignedBits[1] = (signed char)((val >> 48) & 0xff);
				pSignedBits[2] = (signed char)((val >> 40) & 0xff);
				pSignedBits[3] = (signed char)((val >> 32) & 0xff);
				pSignedBits[4] = (signed char)((val >> 24) & 0xff);
				pSignedBits[5] = (signed char)((val >> 16) & 0xff);
				pSignedBits[6] = (signed char)((val >> 8) & 0xff);
				pSignedBits[7] = (signed char)(val & 0xff);
				break;
			case 6:
				pSignedBits[0] = (signed char)((val >> 40) & 0xff);
				pSignedBits[1] = (signed char)((val >> 32) & 0xff);
				pSignedBits[2] = (signed char)((val >> 24) & 0xff);
				pSignedBits[3] = (signed char)((val >> 16) & 0xff);
				pSignedBits[4] = (signed char)((val >> 8) & 0xff);
				pSignedBits[5] = (signed char)(val & 0xff);
				break;
			case 4:
				pSignedBits[0] = (signed char)((val >> 24) & 0xff);
				pSignedBits[1] = (signed char)((val >> 16) & 0xff);
				pSignedBits[2] = (signed char)((val >> 8) & 0xff);
				pSignedBits[3] = (signed char)(val & 0xff);
				break;
			case 3:
				pSignedBits[0] = (signed char)((val >> 16) & 0xff);
				pSignedBits[1] = (signed char)((val >> 8) & 0xff);
				pSignedBits[2] = (signed char)(val & 0xff);
				break;	
			case 2:
				pSignedBits[0] = (signed char)((val >> 8) & 0xff);
				pSignedBits[1] = (signed char)(val & 0xff);
				break;
			case 1:
				pSignedBits[0] = (signed char)(val & 0xff);
				break;
			default:
				Assert( 0 );
				return;
			}
		}
	}
}

FORCEINLINE_PIXEL void CPixelWriter::ReadPixelNoAdvance( int &r, int &g, int &b, int &a )
{
	Assert( IsUsingSupportedFormat() );
	Assert( !IsUsingFloatFormat() );

	int val = m_pBits[0];
	if ( m_Size > 1 )
	{
		if ( IsPC() || IsPS3() || !IsX360() )
		{
			val |= (int)m_pBits[1] << 8;
			if ( m_Size > 2 )
			{
				val |= (int)m_pBits[2] << 16;
				if ( m_Size > 3 )
				{
					val |= (int)m_pBits[3] << 24;
				}
			}
		}
		else
		{
			val <<= 8;
			val |= (int)m_pBits[1];
			if ( m_Size > 2 )
			{
				val <<= 8;
				val |= (int)m_pBits[2];
				if ( m_Size > 3 )
				{
					val <<= 8;
					val |= (int)m_pBits[3];
				}
			}
		}
	}

	r = (val>>m_RShift) & m_RMask;
	g = (val>>m_GShift) & m_GMask;
	b = (val>>m_BShift) & m_BMask;
	a = (val>>m_AShift) & m_AMask;
}

#endif // PIXELWRITER_H;
