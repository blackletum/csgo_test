﻿//========= Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//


#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "isys.h"
#include "conproc.h"
#include "dedicated.h"
#include "engine_hlds_api.h"
#include "checksum_md5.h"
#include "idedicatedexports.h"
#include "tier0/dbg.h"
#include "mathlib/mathlib.h"
#include "interface.h"
#include "tier1/strtools.h"
#include "tier0/stacktools.h"
#include "tier0/icommandline.h"
#include "materialsystem/imaterialsystem.h"
#include "istudiorender.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "datacache/idatacache.h"
#include "datacache/imdlcache.h"
#include "vphysics_interface.h"
#include "icvar.h"
#include "net.h"
#include "vscript/ivscript.h"

bool InitInstance( );
void ProcessConsoleInput( void );

#define stringize(a) #a
#define engine_binary(a,b,c) a stringize(b) c

static const char *g_pszengine = "bin/engine" DLL_EXT_STRING;
static const char *g_pszsoundemitter = "bin/soundemitter" DLL_EXT_STRING;

char g_szEXEName[ 256 ];

#include "console/TextConsoleUnix.h"
extern CTextConsoleUnix console;


//-----------------------------------------------------------------------------
// Purpose: Implements OS Specific layer ( loosely )
//-----------------------------------------------------------------------------
class CSys : public ISys
{
public:
	virtual		~CSys();

	virtual bool LoadModules( CDedicatedAppSystemGroup *pAppSystemGroup );

	void		Sleep( int msec );
	bool		GetExecutableName( char *out );
	void		ErrorMessage( int level, const char *msg );

	void		WriteStatusText( char *szText );
	void		UpdateStatus( int force );

	long		LoadLibrary( char *lib );
	void		FreeLibrary( long library );
	void		*GetProcAddress( long library, const char *name );

	bool		CreateConsoleWindow( void );
	void		DestroyConsoleWindow( void );

	void		ConsoleOutput ( char *string );
	char		*ConsoleInput (void);
	void		Printf(const char *fmt, ...);
};

static CSys g_Sys;
ISys *sys = &g_Sys;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSys::~CSys()
{
	sys = NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : msec
// Output : 
//-----------------------------------------------------------------------------
void CSys::Sleep( int msec )
{
    usleep(msec * 1000);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : handle, function name-
// Output : void *
//-----------------------------------------------------------------------------
void *CSys::GetProcAddress( long library, const char *name )
{
	return dlsym( library, name );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *lib -
// Output : long
//-----------------------------------------------------------------------------
long CSys::LoadLibrary( char *lib )
{
	void *hDll = NULL;

    char    cwd[1024];
    char    absolute_lib[1024];
    
    if (!getcwd(cwd, sizeof(cwd)))
        ErrorMessage(1, "Sys_LoadLibrary: Couldn't determine current directory.");
        
    if (cwd[strlen(cwd)-1] == '/')
        cwd[strlen(cwd)-1] = 0;
        
    Q_snprintf(absolute_lib, sizeof( absolute_lib ), "%s/%s", cwd, lib);
    
    hDll = dlopen( absolute_lib, RTLD_NOW );
    if ( !hDll )
    {
        ErrorMessage( 1, dlerror() );
    }
	else
	{
		StackToolsNotify_LoadedLibrary( absolute_lib );
	}
	return (long)hDll;
}

void CSys::FreeLibrary( long library )
{
	if ( !library )
		return;

	dlclose( (void *)library );
}

bool CSys::GetExecutableName( char *out )
{
	char *name = strrchr(g_szEXEName, '/' );
	if ( name )
	{
		strcpy( out, name + 1);
		return true;
	}
	else
	{
		return false;
	}
}

/*
==============
ErrorMessage

Engine is erroring out, display error in message box
==============
*/
void CSys::ErrorMessage( int level, const char *msg )
{
	Error( "%s\n", msg );
	exit( -1 );
}

void CSys::UpdateStatus( int force )
{
}

/*
================
ConsoleOutput

Print text to the dedicated console
================
*/
void CSys::ConsoleOutput (char *string)
{
	console.Print(string);
}

/*
==============
Printf

Engine is printing to console
==============
*/
void CSys::Printf(const char *fmt, ...)
{
	// Dump text to debugging console.
	va_list argptr;
	char szText[1024];

	va_start (argptr, fmt);
	Q_vsnprintf (szText, sizeof( szText ), fmt, argptr);
	va_end (argptr);

	// Get Current text and append it.
	ConsoleOutput( szText );
}

/*
================
ConsoleInput

================
*/
char *CSys::ConsoleInput( void )
{
	return console.GetLine();
}

/*
==============
WriteStatusText

==============
*/
void CSys::WriteStatusText( char *szText )
{
}

/*
==============
CreateConsoleWindow

Create console window ( overridable? )
==============
*/
bool CSys::CreateConsoleWindow( void )
{
	return true;
}

/*
==============
DestroyConsoleWindow

==============
*/
void CSys::DestroyConsoleWindow( void )
{
}

/*
================
GameInit
================
*/
bool CSys::LoadModules( CDedicatedAppSystemGroup *pAppSystemGroup )
{
	AppSystemInfo_t appSystems[] = 
	{
 		{ g_pszengine,								CVAR_QUERY_INTERFACE_VERSION },
		{ "bin/materialsystem" DLL_EXT_STRING,		MATERIAL_SYSTEM_INTERFACE_VERSION },
		{ "bin/studiorender" DLL_EXT_STRING,		STUDIO_RENDER_INTERFACE_VERSION },
		{ "bin/vphysics" DLL_EXT_STRING,			VPHYSICS_INTERFACE_VERSION },
		{ "bin/datacache" DLL_EXT_STRING,			DATACACHE_INTERFACE_VERSION },
		{ "bin/datacache" DLL_EXT_STRING,			MDLCACHE_INTERFACE_VERSION },
		{ "bin/datacache" DLL_EXT_STRING,			STUDIO_DATA_CACHE_INTERFACE_VERSION },
		{ "bin/vscript" DLL_EXT_STRING,			VSCRIPT_INTERFACE_VERSION },

		{ g_pszengine,						VENGINE_HLDS_API_VERSION },
		{ "", "" }	// Required to terminate the list
	};

	if ( !pAppSystemGroup->AddSystems( appSystems ) ) 
		return false;
	
	engine = (IDedicatedServerAPI *)pAppSystemGroup->FindSystem( VENGINE_HLDS_API_VERSION );
															// obsolete i think SetCVarIF( (ICvar*)pAppSystemGroup->FindSystem( VENGINE_CVAR_INTERFACE_VERSION ) );

	IMaterialSystem* pMaterialSystem = (IMaterialSystem*)pAppSystemGroup->FindSystem( MATERIAL_SYSTEM_INTERFACE_VERSION );
	pMaterialSystem->SetShaderAPI( "bin/shaderapiempty" DLL_EXT_STRING );


	return true;
}

bool NET_Init()
{
	return true;
}

void NET_Shutdown()
{
}

extern int main(int argc, char *argv[]);
DLL_EXPORT int DedicatedMain( int argc, char *argv[] );

int DedicatedMain( int argc, char *argv[] )
{
	return main(argc,argv);
}
