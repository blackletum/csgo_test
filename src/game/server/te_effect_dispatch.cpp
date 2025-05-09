﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "basetempentity.h"
#include "te_effect_dispatch.h"
#include "networkstringtable_gamedll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: This TE provides a simple interface to dispatch effects by name using DispatchEffect().
//-----------------------------------------------------------------------------
class CTEEffectDispatch : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEEffectDispatch, CBaseTempEntity );

					CTEEffectDispatch( const char *name );
	virtual			~CTEEffectDispatch( void );

	DECLARE_SERVERCLASS();

public:
	CEffectData m_EffectData;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEEffectDispatch::CTEEffectDispatch( const char *name ) :
	CBaseTempEntity( name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEEffectDispatch::~CTEEffectDispatch( void )
{
}

IMPLEMENT_SERVERCLASS_ST( CTEEffectDispatch, DT_TEEffectDispatch )
	SendPropDataTable( SENDINFO_DT( m_EffectData ), &REFERENCE_SEND_TABLE( DT_EffectData ) )
END_SEND_TABLE()


// Singleton to fire TEEffectDispatch objects
static CTEEffectDispatch g_TEEffectDispatch( "EffectDispatch" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchEffect( const char *pName, const CEffectData &data )
{
	//Broadcast effects to all players. The potential for cheating
	//seems minimal on the effects that are sent through this,
	//and the overhead is minimal enough to make it worth sending to
	//all players.
	CReliableBroadcastRecipientFilter filter;
	filter.AddAllPlayers();

	if ( data.m_fFlags & EFFECTDATA_SERVER_IGNOREPREDICTIONCULL )
	{
		// remove prediction cull so it works on listen servers for the host
		filter.SetIgnorePredictionCull( true );
	}

	if ( !te->SuppressTE( filter ) )
	{
		// Copy the supplied effect data.
		g_TEEffectDispatch.m_EffectData = data;

		// Get the entry index in the string table.
		g_TEEffectDispatch.m_EffectData.m_iEffectName = GetEffectIndex( pName );

		// Send it to anyone who can see the effect's origin.
		g_TEEffectDispatch.Create( filter, 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchEffect( IRecipientFilter& filter, float flDelay, const char *pName, const CEffectData &data )
{
	if ( !te->SuppressTE( filter ) )
	{
		// Copy the supplied effect data.
		g_TEEffectDispatch.m_EffectData = data;

		// Get the entry index in the string table.
		g_TEEffectDispatch.m_EffectData.m_iEffectName = GetEffectIndex( pName );

		// Send it to anyone who can see the effect's origin.
		g_TEEffectDispatch.Create( filter, flDelay );
	}
}
