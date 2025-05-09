﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: dll-agnostic routines (no dll dependencies here)
//
// $NoKeywords: $
//=============================================================================//

// Author: Matthew D. Campbell (matt@turtlerockstudios.com), 2003

#include "cbase.h"

#include <ctype.h>
#include "shared_util.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static char s_shared_token[ 1500 ];
static char s_shared_quote = '\"';

//--------------------------------------------------------------------------------------------------------------
char * SharedVarArgs(PRINTF_FORMAT_STRING const char *format, ...)
{
	va_list argptr;
	const int BufLen = 1024;
	const int NumBuffers = 4;
	static char string[NumBuffers][BufLen];
	static int curstring = 0;
	
	curstring = ( curstring + 1 ) % NumBuffers;

	va_start (argptr, format);
	_vsnprintf( string[curstring], BufLen, format, argptr );
	va_end (argptr);

	return string[curstring];  
}

//--------------------------------------------------------------------------------------------------------------
char * BufPrintf(char *buf, int& len, PRINTF_FORMAT_STRING const char *fmt, ...)
{
	if (len <= 0)
		return NULL;

	va_list argptr;

	va_start(argptr, fmt);
	_vsnprintf(buf, len, fmt, argptr);
	va_end(argptr);
	// Make sure the buffer is null-terminated.
	buf[ len - 1 ] = 0;

	len -= strlen(buf);
	return buf + strlen(buf);
}

#ifdef _WIN32
//--------------------------------------------------------------------------------------------------------------
wchar_t * BufWPrintf(wchar_t *buf, int& len, PRINTF_FORMAT_STRING const wchar_t *fmt, ...)
{
	if (len <= 0)
		return NULL;

	va_list argptr;

	va_start(argptr, fmt);
	_vsnwprintf(buf, len, fmt, argptr);
	va_end(argptr);
	// Make sure the buffer is null-terminated.
	buf[ len - 1 ] = 0;

	len -= wcslen(buf);
	return buf + wcslen(buf);
}
#endif

//--------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
const wchar_t * NumAsWString( int val )
{
	const int BufLen = 16;
	static wchar_t buf[BufLen];
	int len = BufLen;
	BufWPrintf( buf, len, L"%d", val );
	return buf;
}
#endif
// dgoodenough - PS3 needs this guy as well.
// PS3_BUILDFIX
#ifdef _PS3
const wchar_t * NumAsWString( int val )
{
	const int BufLen = 16;
	static wchar_t buf[BufLen];
	char szBuf[BufLen];

	Q_snprintf(szBuf, BufLen, "%d", val );
	szBuf[BufLen - 1] = 0;
	for ( int i = 0; i < BufLen; ++i )
	{
		buf[i] = szBuf[i];
	}
	return buf;
}
#endif
//--------------------------------------------------------------------------------------------------------------
const char * NumAsString( int val )
{
	const int BufLen = 16;
	static char buf[BufLen];
	int len = BufLen;
	BufPrintf( buf, len, "%d", val );
	return buf;
}

//--------------------------------------------------------------------------------------------------------
/**
 * Returns the token parsed by SharedParse()
 */
char *SharedGetToken( void )
{
	return s_shared_token;
}

//--------------------------------------------------------------------------------------------------------
/**
 * Returns the token parsed by SharedParse()
 */
void SharedSetQuoteChar( char c )
{
	s_shared_quote = c;
}

//--------------------------------------------------------------------------------------------------------
/**
 * Parse a token out of a string
 */
const char *SharedParse( const char *data )
{
	int             c;
	int             len;
	
	len = 0;
	s_shared_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == s_shared_quote)
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c==s_shared_quote || !c)
			{
				s_shared_token[len] = 0;
				return data;
			}
			s_shared_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		s_shared_token[len] = c;
		len++;
		s_shared_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		s_shared_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);
	
	s_shared_token[len] = 0;
	return data;
}

//--------------------------------------------------------------------------------------------------------
/**
 * Returns true if additional data is waiting to be processed on this line
 */
bool SharedTokenWaiting( const char *buffer )
{
	const char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !V_isspace( *p ) || V_isalnum( *p ) )
			return true;

		p++;
	}

	return false;
}
