﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "cs_team.h"
#include "entitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Datatable
IMPLEMENT_SERVERCLASS_ST(CCSTeam, DT_CSTeam)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( cs_team_manager, CCSTeam );

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified TF team manager
//-----------------------------------------------------------------------------
CCSTeam *GetGlobalCSTeam( int iIndex )
{
	return (CCSTeam*)GetGlobalTeam( iIndex );
}


//-----------------------------------------------------------------------------
// Purpose: Needed because this is an entity, but should never be used
//-----------------------------------------------------------------------------
void CCSTeam::Init( const char *pName, int iNumber )
{
	BaseClass::Init( pName, iNumber );

	// Only detect changes every half-second.
	NetworkProp()->SetUpdateInterval( 0.75f );
}

CCSTeam::CCSTeam()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCSTeam::~CCSTeam( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCSTeam::Precache( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame
//-----------------------------------------------------------------------------
void CCSTeam::Think( void )
{
	BaseClass::Think();
}


void CCSTeam::OnRoundPreStart( void )
{
}

//------------------------------------------------------------------------------------------------------------------
// PLAYERS
//-----------------------------------------------------------------------------
// Purpose: Add the specified player to this team. Remove them from their current team, if any.
//-----------------------------------------------------------------------------
void CCSTeam::AddPlayer( CBasePlayer *pPlayer )
{
	BaseClass::AddPlayer( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Clean up the player's objects when they leave
//-----------------------------------------------------------------------------
void CCSTeam::RemovePlayer( CBasePlayer *pPlayer )
{
	BaseClass::RemovePlayer( pPlayer );
}

//------------------------------------------------------------------------------------------------------------------
// UTILITY FUNCS
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCSTeam* CCSTeam::GetEnemyTeam()
{
	int iMyTeam = GetTeamNumber();

	if ( iMyTeam == TEAM_CT )
		return GetGlobalCSTeam( TEAM_TERRORIST );
	else if ( iMyTeam == TEAM_TERRORIST )
		return GetGlobalCSTeam( TEAM_CT );
	else
		return NULL;
}
