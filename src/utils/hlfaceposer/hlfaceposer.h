﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#if !defined( HLFACEPOSER_H )
#define HLFACEPOSER_H
#ifdef _WIN32
#pragma once
#endif

#include <ctype.h>
#include <float.h>
#include <windows.h>
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "Color.h"

#define CONSOLE_COLOR Color( 82, 173, 216 )

#define ERROR_COLOR Color( 255, 50, 20 )

#define FILE_COLOR Color( 0, 63, 200 )

#define MAX_FP_MODELS 16

#define SCRUBBER_HANDLE_WIDTH		40
#define SCRUBBER_HANDLE_HEIGHT		10

char *va( const char *fmt, ... );

char const *GetGameDirectory(); // e.g. u:\main\game\ep2
char const *GetGameDirectorySimple();  // e.g.  ep2

void Con_Printf( const char *fmt, ... );
void Con_ColorPrintf( const Color& clr, const char *fmt, ... );
void Con_ErrorPrintf( const char *fmt, ... );

bool FPFullpathFileExists( const char *filename );
void MakeFileWriteable( const char *filename );
bool MakeFileWriteablePrompt( const char *filename, char const *promptTitle );
bool IsFileWriteable( const char *filename );
void FPCopyFile( const char *source, const char *dest, bool bCheckOut );
class mxWindow;
void FacePoser_MakeToolWindow( mxWindow *w, bool smallcaption );
void FacePoser_LoadWindowPositions( char const *name, bool& visible, int& x, int& y, int& w, int& h, bool& locked, bool& zoomed );
void FacePoser_SaveWindowPositions( char const *name, bool visible, int x, int y, int w, int h, bool locked, bool zoomed );
void FacePoser_AddWindowStyle( mxWindow *w, int addbits );
void FacePoser_AddWindowExStyle( mxWindow *w, int addbits );
void FacePoser_RemoveWindowStyle( mxWindow *w, int removebits );
bool FacePoser_HasWindowStyle( mxWindow *w, int bits );

void FacePoser_EnsurePhonemesLoaded( void );
void FacePoser_SetPhonemeRootDir( char const *pchRootDir );

int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSize);
int ConvertUnicodeToANSI(const wchar_t *unicode, char *ansi, int ansiBufferSize);

float FacePoser_SnapTime( float t );
char const *FacePoser_DescribeSnappedTime( float t );
int FacePoser_GetSceneFPS( void );
bool FacePoser_IsSnapping( void );

class StudioModel;
char const *FacePoser_TranslateSoundName( char const *soundname, StudioModel *model = NULL );
class CChoreoEvent;

char const *FacePoser_TranslateSoundName( CChoreoEvent *event );
char const *FacePoser_TranslateSoundNameGender( char const *soundname, gender_t gender );

extern class IFileSystem *filesystem;
extern class ISceneTokenProcessor *tokenprocessor;

char *Q_stristr_slash( char const *pStr, char const *pSearch );

void				SetCloseCaptionLanguageId( int id, bool force = false ); // from sentence.h enum
int					GetCloseCaptionLanguageId();

bool FacePoser_ShowOpenFileNameDialog( char *relative, size_t bufsize, char const *subdir, char const *wildcard );
bool FacePoser_ShowSaveFileNameDialog( char *relative, size_t bufsize, char const *subdir, char const *wildcard );

// Helper for porting from windows to vgui tool framework
inline COLORREF ColorToRGB( const Color &clr )
{
	return RGB( clr.r(), clr.g(), clr.b() );
}

inline Color RGBToColor( const COLORREF &clr )
{
	return Color( GetRValue( clr ), GetGValue( clr ), GetBValue( clr ) );
}

#endif // HLFACEPOSER_H
