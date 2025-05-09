﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdlib.h>
#include <string.h>
#include "materialsystem/imaterialproxy.h"
#include "materialproxyfactory.h"
#include "toolframework/itoolframework.h"
#include "toolframework/itoolsystem.h"
#include "server.h"
#include "cdll_int.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IBaseClientDLL *g_ClientDLL;
#ifndef DEDICATED
extern CSysModule		*g_ClientDLLModule;
#endif

IMaterialProxy *CMaterialProxyFactory::CreateProxy( const char *proxyName )
{
#if !defined(DEDICATED)
	IMaterialProxy *materialProxy = g_ClientDLL->InstantiateMaterialProxy( proxyName );

	// If the client didn't have it and we're in tool mode, ask the tools...
	if ( toolframework->InToolMode() && !materialProxy )
	{
		materialProxy = toolframework->LookupProxy( proxyName );
	}

	if( !materialProxy && !sv.IsDedicated() )
	{
		ConDMsg( "Can't find material proxy \"%s\"\n", proxyName );
		return NULL;
	}
	return materialProxy;
#else
	return NULL;
#endif
}

void CMaterialProxyFactory::DeleteProxy( IMaterialProxy *pProxy )
{
	// how do you delete something generated by an interface.h factory?
	if( pProxy )
	{
		pProxy->Release();
	}
}

CreateInterfaceFn CMaterialProxyFactory::GetFactory()
{
#ifndef DEDICATED
	return Sys_GetFactory( g_ClientDLLModule );
#else
	return NULL;
#endif
}
