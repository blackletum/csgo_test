﻿//=========== (C) Copyright 2009 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================

#include "vpc.h"

// Valve includes
#include "tier1/utlbuffer.h"
#include <sys/types.h>
#include <sys/stat.h>

#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// File time
//-----------------------------------------------------------------------------
void DetermineFileInfo( const char *pFileName, long *pModificationTime, long *pSize )
{
	struct _stat32 buf;
	if ( ::_stat32( pFileName, &buf ) == 0 )
	{
		*pModificationTime = buf.st_mtime;
		*pSize = buf.st_size;
	}
	else
	{
		*pModificationTime = 0;
		*pSize = 0;
	}
}


//-----------------------------------------------------------------------------
// Read file into utlbuffer
//-----------------------------------------------------------------------------
bool ReadFileIntoUtlBuffer( const char *pFileName, long nSize, CUtlBuffer &buf )
{
	Assert( buf.IsText() );
	FILE *fp = fopen( pFileName, "rt" );
	if ( fp == NULL )
	{
		Warning( "vpc [updating autoexp.dat]: Unable to open file %s!\n", pFileName );
		return false;
	}

	buf.EnsureCapacity( nSize );
	size_t nReadBytes = fread( buf.Base(), 1, nSize, fp );
	fclose( fp );

	buf.SeekPut( CUtlBuffer::SEEK_HEAD, ( int )nReadBytes );
	return true;
}


//-----------------------------------------------------------------------------
// Update autoexp.dat, foo!
//-----------------------------------------------------------------------------

#define AUTOEXP_BLOCK_START			"\n;-------------VALVE AUTOEXP AUTOGENERATED BLOCK START: %s----------------\n"
#define AUTOEXP_MODIFICATION_TIME	"; Modification Time : "
#define AUTOEXP_BLOCK_END			"\n;--------------VALVE AUTOEXP AUTOGENERATED BLOCK END: %s-----------------\n"

void InjectAutoExpSection( const char *pVPCPath, const char *pToolsPath, const char *pAutoExpPath, const char *pInjectFile, const char *pSectionToken )
{
	long nAutoExpTime, nFileSize;
	DetermineFileInfo( pAutoExpPath, &nAutoExpTime, &nFileSize );

	CUtlBuffer autoExpDatBuf( 0, 0, CUtlBuffer::TEXT_BUFFER );

	CUtlPathStringHolder injectPath;
	injectPath.ComposeFileName( pVPCPath, pInjectFile );
	injectPath.FixupPathName();

	if ( !ReadFileIntoUtlBuffer( pAutoExpPath, nFileSize, autoExpDatBuf ) )
	{
		Warning( "Failed to open autoexp.dat (%s)\n", pAutoExpPath );
		return;
	}

	long nInjectTime;
	DetermineFileInfo( injectPath, &nInjectTime, &nFileSize );

	CUtlBuffer injectBuf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	if ( !ReadFileIntoUtlBuffer( injectPath, nFileSize, injectBuf ) )
	{
		Warning( "Failed to open autoexp_autoinject.dat (%s)\n", injectPath.Get() );
		return;
	}

	// Have we patched this already?
	char pStartBlock[512];
	char pEndBlock[512];
	V_snprintf( pStartBlock, sizeof(pStartBlock), AUTOEXP_BLOCK_START, pSectionToken );
	V_snprintf( pEndBlock, sizeof(pEndBlock), AUTOEXP_BLOCK_END, pSectionToken );

	bool bCreateBlock = false;

	int nInsertPoint, nResumePoint;
	if ( autoExpDatBuf.GetToken( pStartBlock ) )
	{
		nInsertPoint = autoExpDatBuf.TellGet() - V_strlen( pStartBlock );

		const char *pAutoExpTime = (const char*)autoExpDatBuf.PeekGet();
		int nTimeMsgLen = V_strlen( AUTOEXP_MODIFICATION_TIME );
		if ( !V_strncmp( pAutoExpTime, AUTOEXP_MODIFICATION_TIME, nTimeMsgLen ) )
		{
			autoExpDatBuf.SeekGet( CUtlBuffer::SEEK_CURRENT, nTimeMsgLen );
			nAutoExpTime = autoExpDatBuf.GetInt();
			if ( nAutoExpTime >= nInjectTime )
				return; // inject is older than autoexp
		}

		Msg( "(Updating autoexp.dat with newer data from %s)\n", pInjectFile );

		// We *should* always find the end block here
		if ( !autoExpDatBuf.GetToken( pEndBlock ) )
			return;

		nResumePoint = autoExpDatBuf.TellGet();
	}
	else
	{
		Msg( "(Updating autoexp.dat with data from %s)\n", pInjectFile );

		// Can we find the right section?
		if ( autoExpDatBuf.GetToken( pSectionToken ) )
		{
			nInsertPoint = autoExpDatBuf.TellGet();
			nResumePoint = autoExpDatBuf.TellGet();
		}
		else
		{
			bCreateBlock = true;
			nInsertPoint = autoExpDatBuf.TellMaxPut();
			nResumePoint = autoExpDatBuf.TellMaxPut();
		}
	}

	CUtlBuffer autoExpDatOutBuf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	autoExpDatOutBuf.Put( autoExpDatBuf.Base(), nInsertPoint );
	if ( bCreateBlock )
	{
		autoExpDatOutBuf.PutString( "\n" );
		autoExpDatOutBuf.PutString( pSectionToken );
		autoExpDatOutBuf.PutString( "\n" );
	}
	autoExpDatOutBuf.PutString( pStartBlock );
	autoExpDatOutBuf.PutString( AUTOEXP_MODIFICATION_TIME );
	autoExpDatOutBuf.PutInt( nInjectTime );
	autoExpDatOutBuf.PutChar( '\n' );
	autoExpDatOutBuf.PutString( (char*)injectBuf.Base() );
	autoExpDatOutBuf.PutString( pEndBlock );
	autoExpDatOutBuf.Put( (char*)autoExpDatBuf.Base() + nResumePoint, autoExpDatBuf.TellMaxPut() - nResumePoint );

	FILE *fp = fopen( pAutoExpPath, "wt" );
	if ( fp == NULL )
	{
		Warning( "vpc [updating autoexp.dat]: Unable to open file %s for write!\n", pAutoExpPath );
		return;
	}

	fwrite( autoExpDatOutBuf.Base(), 1, autoExpDatOutBuf.TellPut(), fp );
	fclose( fp );
}


void UpdateAutoExpDatForTool( const char *pToolEnvVar, const char *pToolName )
{
	const char *pToolsPath = getenv( pToolEnvVar );
	if ( !pToolsPath )
		return;

	CUtlPathStringHolder autoExpPath;
	autoExpPath.ComposeFileName( pToolsPath, "..\\Packages\\Debugger\\autoexp.dat" );
	autoExpPath.FixupPathName();
	char pVPCPath[MAX_FIXED_PATH];
	Plat_GetModuleFilename( pVPCPath, sizeof(pVPCPath) );
	V_StripFilename( pVPCPath );

	// win32 and win64 are now at the same height in the tree, but if that changes again, this needs to be updated
	#define AUTOEXPDIR	"../../"

	char pInjectPath[500];
	V_snprintf( pInjectPath, sizeof(pInjectPath), AUTOEXPDIR "devstudio macros/autoexp_autoinject_autoexpand_%s.dat", pToolName );
	InjectAutoExpSection( pVPCPath, pToolsPath, autoExpPath, pInjectPath, "[AutoExpand]" );

	V_snprintf( pInjectPath, sizeof(pInjectPath), AUTOEXPDIR "devstudio macros/autoexp_autoinject_visualizer_%s.dat", pToolName );
	InjectAutoExpSection( pVPCPath, pToolsPath, autoExpPath, pInjectPath, "[Visualizer]" );

	V_snprintf( pInjectPath, sizeof(pInjectPath), AUTOEXPDIR "devstudio macros/autoexp_autoinject_executioncontrol_%s.dat",pToolName );
//	InjectAutoExpSection( pVPCPath, pToolsPath, autoExpPath, pInjectPath, "[ExecutionControl]" );
}

void UpdateAutoExpDat( )
{
	UpdateAutoExpDatForTool( "VS80COMNTOOLS", "2k5" );
	UpdateAutoExpDatForTool( "VS90COMNTOOLS", "2k8" );
}
