﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// conproc.c -- support for qhost
#include "dedicated.h"
#include <stdio.h>
#include <process.h>
#include <windows.h>
#include "conproc.h"
#include "isys.h"
#include "tier0/icommandline.h"

static HANDLE	heventDone;
static HANDLE	hfileBuffer;
static HANDLE	heventChildSend;
static HANDLE	heventParentSend;
static HANDLE	hStdout;
static HANDLE	hStdin;

/*
==============
SetConsoleCXCY

==============
*/
BOOL SetConsoleCXCY(HANDLE hStdout, int cx, int cy)
{
	CONSOLE_SCREEN_BUFFER_INFO	info;
	COORD						coordMax;
 
	coordMax = GetLargestConsoleWindowSize(hStdout);

	if (cy > coordMax.Y)
		cy = coordMax.Y;

	if (cx > coordMax.X)
		cx = coordMax.X;
 
	if (!GetConsoleScreenBufferInfo(hStdout, &info))
		return FALSE;

// height
    info.srWindow.Left = 0;         
    info.srWindow.Right = info.dwSize.X - 1;                
    info.srWindow.Top = 0;
    info.srWindow.Bottom = cy - 1;          
 
	if (cy < info.dwSize.Y)
	{
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
 
		info.dwSize.Y = cy;
 
		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
    }
    else if (cy > info.dwSize.Y)
    {
		info.dwSize.Y = cy;
 
		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
 
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
    }
 
	if (!GetConsoleScreenBufferInfo(hStdout, &info))
		return FALSE;
 
// width
	info.srWindow.Left = 0;         
	info.srWindow.Right = cx - 1;
	info.srWindow.Top = 0;
	info.srWindow.Bottom = info.dwSize.Y - 1;               
 
	if (cx < info.dwSize.X)
	{
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
 
		info.dwSize.X = cx;
    
		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
	}
	else if (cx > info.dwSize.X)
	{
		info.dwSize.X = cx;
 
		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
 
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
	}
 
	return TRUE;
}

/*
==============
GetMappedBuffer

==============
*/
LPVOID GetMappedBuffer (HANDLE hfileBuffer)
{
	LPVOID pBuffer;

	pBuffer = MapViewOfFile (hfileBuffer,
							FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	return pBuffer;
}

/*
==============
ReleaseMappedBuffer

==============
*/
void ReleaseMappedBuffer (LPVOID pBuffer)
{
	UnmapViewOfFile (pBuffer);
}

/*
==============
GetScreenBufferLines

==============
*/
BOOL GetScreenBufferLines (int *piLines)
{
	CONSOLE_SCREEN_BUFFER_INFO	info;							  
	BOOL						bRet;

	bRet = GetConsoleScreenBufferInfo (hStdout, &info);
		
	if (bRet)
		*piLines = info.dwSize.Y;

	return bRet;
}

/*
==============
SetScreenBufferLines

==============
*/
BOOL SetScreenBufferLines (int iLines)
{
	return SetConsoleCXCY (hStdout, 80, iLines);
}

/*
==============
ReadText

==============
*/
BOOL ReadText (LPTSTR pszText, int iBeginLine, int iEndLine)
{
	COORD	coord;
	DWORD	dwRead;
	BOOL	bRet;

	coord.X = 0;
	coord.Y = iBeginLine;

	bRet = ReadConsoleOutputCharacter(
		hStdout,
		pszText,
		80 * (iEndLine - iBeginLine + 1),
		coord,
		&dwRead);

	// Make sure it's null terminated.
	if (bRet)
		pszText[dwRead] = '\0';

	return bRet;
}

/*
==============
CharToCode

==============
*/
int CharToCode (char c)
{
	char upper;
		
	upper = toupper(c);

	switch (c)
	{
		case 13:
			return 28;

		default:
			break;
	}

	if (isalpha(c))
		return (30 + upper - 65); 

	if (isdigit(c))
		return (1 + upper - 47);

	return c;
}

/*
==============
WriteText

==============
*/
BOOL WriteText (LPCTSTR szText)
{
	DWORD			dwWritten;
	INPUT_RECORD	rec;
	char			upper, *sz;

	sz = (LPTSTR) szText;

	while (*sz)
	{
	// 13 is the code for a carriage return (\n) instead of 10.
		if (*sz == 10)
			*sz = 13;

		upper = toupper(*sz);

		rec.EventType = KEY_EVENT;
		rec.Event.KeyEvent.bKeyDown = TRUE;
		rec.Event.KeyEvent.wRepeatCount = 1;
		rec.Event.KeyEvent.wVirtualKeyCode = upper;
		rec.Event.KeyEvent.wVirtualScanCode = CharToCode (*sz);
		rec.Event.KeyEvent.uChar.AsciiChar = *sz;
		rec.Event.KeyEvent.uChar.UnicodeChar = *sz;
		rec.Event.KeyEvent.dwControlKeyState = isupper(*sz) ? 0x80 : 0x0; 

		WriteConsoleInput(
			hStdin,
			&rec,
			1,
			&dwWritten);

		rec.Event.KeyEvent.bKeyDown = FALSE;

		WriteConsoleInput(
			hStdin,
			&rec,
			1,
			&dwWritten);

		sz++;
	}

	return TRUE;
}

/*
==============
RequestProc

==============
*/
unsigned _stdcall RequestProc (void *arg)
{
	int		*pBuffer;
	DWORD	dwRet;
	HANDLE	heventWait[2];
	int		iBeginLine, iEndLine;
	
	heventWait[0] = heventParentSend;
	heventWait[1] = heventDone;

	while (1)
	{
		dwRet = WaitForMultipleObjects (2, heventWait, FALSE, INFINITE);

	// heventDone fired, so we're exiting.
		if (dwRet == WAIT_OBJECT_0 + 1)	
			break;

		pBuffer = (int *) GetMappedBuffer (hfileBuffer);
		
	// hfileBuffer is invalid.  Just leave.
		if (!pBuffer)
		{
			sys->Printf ("Request Proc:  Invalid -HFILE handle\n");
			break;
		}

		switch (pBuffer[0])
		{
			case CCOM_WRITE_TEXT:
			// Param1 : Text
				pBuffer[0] = WriteText ((LPCTSTR) (pBuffer + 1));
				break;

			case CCOM_GET_TEXT:
			// Param1 : Begin line
			// Param2 : End line
				iBeginLine = pBuffer[1];
				iEndLine = pBuffer[2];
				pBuffer[0] = ReadText ((LPTSTR) (pBuffer + 1), iBeginLine, 
									   iEndLine);
				break;

			case CCOM_GET_SCR_LINES:
			// No params
				pBuffer[0] = GetScreenBufferLines (&pBuffer[1]);
				break;

			case CCOM_SET_SCR_LINES:
			// Param1 : Number of lines
				pBuffer[0] = SetScreenBufferLines (pBuffer[1]);
				break;
		}

		ReleaseMappedBuffer (pBuffer);
		SetEvent (heventChildSend);
	}

	_endthreadex (0);
	return 0;
}

/*
==============
DeinitConProc

==============
*/
void DeinitConProc (void)
{
	if ( heventDone )
	{
		SetEvent ( heventDone );
	}
}
  
/*
==============
InitConProc

==============
*/   
void InitConProc ( void )
{
	unsigned	threadAddr;
	HANDLE		hFile			= (HANDLE)0;
	HANDLE		heventParent	= (HANDLE)0;
	HANDLE		heventChild		= (HANDLE)0;
	int			WantHeight = 50;
	const char		*p;

	// give external front ends a chance to hook into the console
	if ( CommandLine()->CheckParm ( "-HFILE", &p ) && p )
	{
		hFile = (HANDLE)atoi ( p );
	}

	if ( CommandLine()->CheckParm ( "-HPARENT", &p ) && p )
	{
		heventParent = (HANDLE)atoi ( p );
	}

	if ( CommandLine()->CheckParm ( "-HCHILD", &p ) && p )
	{
		heventChild = (HANDLE)atoi ( p );
	}

	// ignore if we don't have all the events.
	if ( !hFile || !heventParent || !heventChild )
	{
		//sys->Printf ("\n\nNo external front end present.\n" );
		return;
	}

	sys->Printf( "\n\nInitConProc:  Setting up external control.\n" );

	hfileBuffer			= hFile;
	heventParentSend	= heventParent;
	heventChildSend		= heventChild;

	// So we'll know when to go away.
	heventDone = CreateEvent (NULL, FALSE, FALSE, NULL);
	if (!heventDone)
	{
		sys->Printf ("InitConProc:  Couldn't create heventDone\n");
		return;
	}

	if (!_beginthreadex (NULL, 0, RequestProc, NULL, 0, &threadAddr))
	{
		CloseHandle (heventDone);
		sys->Printf ("InitConProc:  Couldn't create third party thread\n");
		return;
	}

	// save off the input/output handles.
	hStdout	= GetStdHandle (STD_OUTPUT_HANDLE);
	hStdin	= GetStdHandle (STD_INPUT_HANDLE);

	if ( CommandLine()->CheckParm( "-conheight", &p ) && p )
	{
		WantHeight = atoi( p );
	}

	// Force 80 character width, at least 25 character height
	SetConsoleCXCY( hStdout, 80, WantHeight );
}
