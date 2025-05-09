﻿//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include <stdio.h>
#include <string.h>
#include "tier0/dbg.h"

struct PhonemeMap_t
{
	const char		*string;
	int				code;
	unsigned char	byteCode;	// needs to be unique
	float			weight;
	bool			isStandard;
	const char		*desc;
};

static PhonemeMap_t g_Phonemes[] =
{
	{ "b", 'b', 0x01, 0.8f, true, "Big : voiced alveolar stop" },
	{ "m", 'm', 0x02, 1.0f, true, "Mat : voiced bilabial nasal" },
	{ "p", 'p', 0x03, 0.8f, true, "Put; voiceless alveolar stop" },
	{ "w", 'w', 0x04, 1.0f, true, "With : voiced labial-velar approximant" },
	{ "f", 'f', 0x05, 0.8f, true, "Fork : voiceless labiodental fricative" },
	{ "v", 'v', 0x06, 0.8f, true, "Val : voiced labialdental fricative" },
	{ "r", 0x0279, 0x07, 1.0f, true, "Red : voiced alveolar approximant" },
	{ "r2", 'r', 0x08, 1.0f, true, "Red : voiced alveolar trill" },
	{ "r3", 0x027b, 0x09, 1.0f, true, "Red : voiced retroflex approximant" },
	{ "er", 0x025a, 0x0A, 1.2f, true, "URn : rhotacized schwa" },
	{ "er2", 0x025d, 0x0B, 1.2f, true, "URn : rhotacized lower-mid central vowel" },
	{ "dh", 0x00f0, 0x0C, 1.0f, true, "THen : voiced dental fricative" },
	{ "th", 0x03b8, 0x0D, 1.0f, true, "THin : voiceless dental fricative" },
	{ "sh", 0x0283, 0x0E, 1.0f, true, "SHe : voiceless postalveolar fricative" },
	{ "jh", 0x02a4, 0x0F, 1.0f, true, "Joy : voiced postalveolar afficate" },
	{ "ch", 0x02a7, 0x10, 1.0f, true, "CHin : voiceless postalveolar affricate" },
	{ "s", 's', 0x11, 0.8f, true, "Sit : voiceless alveolar fricative" },
	{ "z", 'z', 0x12, 0.8f, true, "Zap : voiced alveolar fricative" },
	{ "d", 'd', 0x13, 0.8f, true, "Dig : voiced bilabial stop" },
	{ "d2", 0x027e, 0x14, 0.8f, true, "Dig : voiced alveolar flap or tap" },
	{ "l", 'l', 0x15, 0.8f, true, "Lid : voiced alveolar lateral approximant" },
	{ "l2", 0x026b, 0x16, 0.8f, true, "Lid : velarized voiced alveolar lateral approximant" },
	{ "n", 'n', 0x17, 0.8f, true, "No : voiced alveolar nasal" },
	{ "t", 't', 0x18, 0.8f, true, "Talk : voiceless bilabial stop" },
	{ "ow", 'o', 0x19, 1.2f, true, "gO : upper-mid back rounded vowel" },
	{ "uw", 'u', 0x1A, 1.2f, true, "tOO : high back rounded vowel" },
	{ "ey", 'e', 0x1B, 1.0f, true, "Ate : upper-mid front unrounded vowel" },
	{ "ae", 0x00e6, 0x1C, 1.0f, true, "cAt : semi-low front unrounded vowel" },
	{ "aa", 0x0251, 0x1D, 1.0f, true, "fAther : low back unrounded vowel" },
	{ "aa2", 'a', 0x1E, 1.0f, true, "fAther : low front unrounded vowel" },
	{ "iy", 'i', 0x1F, 1.0f, true, "fEEl : high front unrounded vowel" },
	{ "y", 'j', 0x20, 0.7f, true, "Yacht : voiced palatal approximant" },
	{ "ah", 0x028c, 0x21, 1.0f, true, "cUt : lower-mid back unrounded vowel" },
	{ "ao", 0x0254, 0x22, 1.2f, true, "dOg : lower-mid back rounded vowel" },
	{ "ax", 0x0259, 0x23, 1.0f, true, "Ago : mid-central unrounded vowel" },
	{ "ax2", 0x025c, 0x24, 1.0f, true, "Ago : lower-mid central unrounded vowel" },
	{ "eh", 0x025b, 0x25, 1.0f, true, "pEt : lower-mid front unrounded vowel"},
	{ "ih", 0x026a, 0x26, 1.0f, true, "fIll : semi-high front unrounded vowel" },
	{ "ih2", 0x0268, 0x27, 1.0f, true, "fIll : high central unrounded vowel" },
	{ "uh", 0x028a, 0x28, 1.0f, true, "bOOk : semi-high back rounded vowel" },
	{ "g", 'g', 0x29, 0.8f, true, "taG : voiced velar stop" },
	{ "g2", 0x0261, 0x2A, 1.0f, true, "taG : voiced velar stop" },
	{ "hh", 'h', 0x2B, 0.8f, true, "Help : voiceless glottal fricative" },
	{ "hh2", 0x0266, 0x2C, 0.8f, true, "Help : breathy-voiced glottal fricative" },
	{ "c", 'k', 0x2D, 0.6f, true, "Cut : voiceless velar stop" },
	{ "nx", 0x014b, 0x2E, 1.0f, true, "siNG : voiced velar nasal" },
	{ "zh", 0x0292, 0x2F, 1.0f, true, "aZure : voiced postalveolar fricative" },

	// Added
	{ "h", 'h', 0x30, 0.8f, false, "Help : voiceless glottal fricative" },
	{ "k", 'k', 0x31, 0.6f, false, "Cut : voiceless velar stop" },
	{ "ay", 0x0251, 0x32, 1.0f, false, "fAther : low back unrounded vowel" }, // or possibly +0x026a (ih)
	{ "ng", 0x014b, 0x33, 1.0f, false, "siNG : voiced velar nasal" }, // nx
	{ "aw", 0x0251, 0x34, 1.2f, false, "fAther : low back unrounded vowel" }, // // vOWel,   // aa + uh???
	{ "oy", 'u', 0x35, 1.2f, false, "tOO : high back rounded vowel" },

	// Silence
	{ "<sil>", '_', 0x00, 1.0f, true, "silence" },
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConvertPhoneme( int code )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( test->code == code )
			return test->string;
	}

	Warning( "Unrecognized phoneme code %i\n", code );
	return "<sil>";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
// Output : int
//-----------------------------------------------------------------------------
int TextToPhoneme( const char *text )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( !stricmp( test->string, text ) )
			return test->code;
	}

	Warning( "Unrecognized phoneme %s\n", text );
	return '_';
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
// Output : float
//-----------------------------------------------------------------------------
float WeightForPhonemeCode( int code )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( test->code == code )
			return test->weight;
	}

	Warning( "Unrecognized phoneme code %i\n", code );
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
// Output : float
//-----------------------------------------------------------------------------
float WeightForPhoneme( char *text )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( !stricmp( test->string, text ) )
			return test->weight;
	}

	Warning( "WeightForPhoneme:: Unrecognized phoneme %s\n", text );
	return 1.0f;
}

int NumPhonemes()
{
	return ARRAYSIZE( g_Phonemes );
}

const char *NameForPhonemeByIndex( int index )
{
	Assert( index >= 0 && index < NumPhonemes() );
	return g_Phonemes[ index ].string;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
// Output : int
//-----------------------------------------------------------------------------
int TextToPhonemeIndex( const char *text )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( !stricmp( test->string, text ) )
			return i;
	}

	return -1;
}

int CodeForPhonemeByIndex( int index )
{
	if ( index < 0 || index >= NumPhonemes() )
		return '_';
	return g_Phonemes[ index ].code;
}

bool IsStandardPhoneme( int index )
{
	if ( index < 0 || index >= NumPhonemes() )
		return false;
	return g_Phonemes[ index ].isStandard;
}

const char *DescForPhonemeByIndex( int index )
{
	if ( index < 0 || index >= NumPhonemes() )
		return NULL;
	return g_Phonemes[ index ].desc;
}

unsigned char CodeToByteCode( int code )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		if ( g_Phonemes[ i ].code == code )
		{
			return g_Phonemes[ i ].byteCode;
		}
	}

	return 0x00;
}

int ByteCodeToCode( unsigned char byteCode )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		if ( g_Phonemes[ i ].byteCode == byteCode )
		{
			return g_Phonemes[ i ].code;
		}
	}

	return '_';
}
