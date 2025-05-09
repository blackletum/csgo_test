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
#include "fx_cs_shared.h"


#define NUM_BULLET_SEED_BITS 8


//-----------------------------------------------------------------------------
// Purpose: Display's a blood sprite
//-----------------------------------------------------------------------------
class CTEFireBullets : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEFireBullets, CBaseTempEntity );
	DECLARE_SERVERCLASS();

					CTEFireBullets( const char *name );
	virtual			~CTEFireBullets( void );

public:
	CNetworkVar( int, m_iPlayer );
	CNetworkVar( uint16, m_nItemDefIndex );
	CNetworkVector( m_vecOrigin );
	CNetworkQAngle( m_vecAngles );
	CNetworkVar( int, m_iWeaponID );
	CNetworkVar( int, m_iMode );
	CNetworkVar( int, m_iSeed );
	CNetworkVar( float, m_fInaccuracy );
	CNetworkVar( float, m_flRecoilIndex );
	CNetworkVar( float, m_fSpread );
#if defined( WEAPON_FIRE_BULLETS_ACCURACY_FISHTAIL_FEATURE )
	CNetworkVar( float, m_fAccuracyFishtail );
#endif
	CNetworkVar( int, m_iSoundType );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEFireBullets::CTEFireBullets( const char *name ) :
	CBaseTempEntity( name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEFireBullets::~CTEFireBullets( void )
{
}

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEFireBullets, DT_TEFireBullets)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD ),
	SendPropAngle( SENDINFO_VECTORELEM( m_vecAngles, 0 ), 13, 0 ),
	SendPropAngle( SENDINFO_VECTORELEM( m_vecAngles, 1 ), 13, 0 ),
	SendPropInt( SENDINFO( m_iWeaponID ), 6, SPROP_UNSIGNED ), // max 63 weapons
	SendPropInt( SENDINFO( m_iMode ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iSeed ), NUM_BULLET_SEED_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iPlayer ), 6, SPROP_UNSIGNED ), 	// max 64 players, see MAX_PLAYERS
	SendPropFloat( SENDINFO( m_fInaccuracy ), 10, 0, 0, 1 ),	
	SendPropFloat( SENDINFO( m_flRecoilIndex ), 10, 0, 0, 1000 ),
	SendPropFloat( SENDINFO( m_fSpread ), 8, 0, 0, 0.1f ),	
	SendPropInt( SENDINFO( m_nItemDefIndex ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iSoundType ), 6, SPROP_UNSIGNED ),
#if defined( WEAPON_FIRE_BULLETS_ACCURACY_FISHTAIL_FEATURE )
	SendPropFloat( SENDINFO( m_fAccuracyFishtail ), 10, 0, -10.0f, 10.0f ),	
#endif
END_SEND_TABLE()


// Singleton
static CTEFireBullets g_TEFireBullets( "Shotgun Shot" );


void TE_FireBullets( 
	int	iPlayerIndex,
	uint16 nItemDefIndex,
	const Vector &vOrigin,
	const QAngle &vAngles,
	int	iWeaponID,
	int	iMode,
	int iSeed,
	float fInaccuracy,
	float fSpread,
	float fAccuracyFishtail,
	int iSoundType,
	float flRecoilIndex
	)
{
	// Just always send gunshots to clients.
	CBroadcastRecipientFilter filter;
	filter.UsePredictionRules();

	g_TEFireBullets.m_iPlayer = iPlayerIndex-1;
	g_TEFireBullets.m_nItemDefIndex = nItemDefIndex;
	g_TEFireBullets.m_vecOrigin = vOrigin;
	g_TEFireBullets.m_vecAngles = vAngles;
	g_TEFireBullets.m_iSeed = iSeed;
	g_TEFireBullets.m_fInaccuracy = fInaccuracy;
	g_TEFireBullets.m_flRecoilIndex = flRecoilIndex;
	g_TEFireBullets.m_fSpread = fSpread;
#if defined( WEAPON_FIRE_BULLETS_ACCURACY_FISHTAIL_FEATURE )
	g_TEFireBullets.m_fAccuracyFishtail = fAccuracyFishtail;
#endif
	g_TEFireBullets.m_iMode = iMode;
	g_TEFireBullets.m_iWeaponID = iWeaponID;
	g_TEFireBullets.m_iSoundType = iSoundType;

	Assert( iSeed < (1 << NUM_BULLET_SEED_BITS) );
	
	g_TEFireBullets.Create( filter, 0 );
}




//-----------------------------------------------------------------------------
// Purpose: Displays a bomb plant animation
//-----------------------------------------------------------------------------
class CTEPlantBomb : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlantBomb, CBaseTempEntity );
	DECLARE_SERVERCLASS();

					CTEPlantBomb( const char *name );
	virtual			~CTEPlantBomb( void );

public:
	CNetworkVar( int, m_iPlayer );
	CNetworkVector( m_vecOrigin );
	CNetworkVar( PlantBombOption_t, m_option );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEPlantBomb::CTEPlantBomb( const char *name ) :
	CBaseTempEntity( name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEPlantBomb::~CTEPlantBomb( void )
{
}

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEPlantBomb, DT_TEPlantBomb)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD ),
	SendPropInt( SENDINFO( m_iPlayer ), 6, SPROP_UNSIGNED ), 	// max 64 players, see MAX_PLAYERS
	SendPropInt( SENDINFO( m_option ), 1, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton
static CTEPlantBomb g_TEPlantBomb( "Bomb Plant" );


void TE_PlantBomb( int iPlayerIndex, const Vector &vOrigin, PlantBombOption_t option )
{
	CPASFilter filter( vOrigin );
	filter.UsePredictionRules();

	g_TEPlantBomb.m_iPlayer = iPlayerIndex-1;
	g_TEPlantBomb.m_option = option;
	g_TEPlantBomb.Create( filter, 0 );
}
