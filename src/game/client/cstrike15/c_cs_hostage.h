﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CHostage class
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_CHOSTAGE_H
#define C_CHOSTAGE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_ai_basenpc.h"
#include "utlvector.h"
#include "util_shared.h"
#include "cs_playeranimstate.h"
#include "c_cs_player.h"


// for shared code
#define CHostage C_CHostage

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class C_HostageCarriableProp : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_HostageCarriableProp, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_HostageCarriableProp();

	virtual void ClientThink( void );
	void OnDataChanged( DataUpdateType_t updateType );
	virtual bool					ShouldDraw();
	virtual ShadowType_t			ShadowCastType();

	bool	m_bCreatedViewmodel;
	float	m_flFadeInStartTime;
};

//----------------------------------------------------------------------------------------------
/**
 * The client-side implementation of the Hostage
 */
class C_CHostage : public C_BaseCombatCharacter, public ICSPlayerAnimStateHelpers
{
public:
	DECLARE_CLASS( C_CHostage, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_CHostage();
	virtual ~C_CHostage();

// ICSPlayerAnimState overrides.
public:
	virtual CWeaponCSBase* CSAnim_GetActiveWeapon();
	virtual bool CSAnim_CanMove();
	virtual void EstimateAbsVelocity( Vector &vel );

public:	
	virtual void Spawn( void );
	virtual void UpdateClientSideAnimation();

	const char *GetCustomHostageNameForMap( const char *szMapName );

	void OnPreDataChanged( DataUpdateType_t updateType );
	void OnDataChanged( DataUpdateType_t updateType );

	int GetHostageState( void ) { return m_nHostageState; }

	bool IsRescued( void ) { return m_isRescued; }
	bool WasRecentlyKilledOrRescued( void );

	int GetHealth( void ) const { return m_iHealth; }
	int GetMaxHealth( void ) const { return m_iMaxHealth; }

	virtual void ClientThink( void );

	C_CSPlayer *GetLeader( void ) const;			// return who we are following or NULL

	virtual C_BaseAnimating * BecomeRagdollOnClient();
	virtual bool ShouldDraw( void );

	void ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );

	void SetClientSideHolidayHatAddon( bool bEnable );
	static void SetClientSideHoldayHatAddonForAllHostagesAndTheirRagdolls( bool bEnable );

private:
	int  m_OldLifestate;
	int  m_iMaxHealth;

	IPlayerAnimState *m_PlayerAnimState;

	CNetworkVar( EHANDLE, m_leader );				// who we are following, or NULL
	
	CNetworkVar( Vector, m_vel );

	CNetworkVar( bool, m_isRescued );
	CNetworkVar( bool, m_jumpedThisFrame );

	CNetworkVar( int, m_nHostageState );

	
	CNetworkVar( float, m_flRescueStartTime );
	CNetworkVar( float, m_flGrabSuccessTime );		//What time did the grabbing succeed?
	CNetworkVar( float, m_flDropStartTime );		//What time did the grabbing succeed?

	float m_flDeadOrRescuedTime;
	static void RecvProxy_Rescued( const CRecvProxyData *pData, void *pStruct, void *pOut );
	static void RecvProxy_Jumped( const CRecvProxyData *pData, void *pStruct, void *pOut );

	CountdownTimer m_blinkTimer;

	Vector m_lookAt;		// point in space we are looking at
	void UpdateLookAt( CStudioHdr *pStudioHdr );	// orient head and eyes towards m_lookAt
	void LookAround( void );										// look around at various interesting things
	CountdownTimer m_lookAroundTimer;

	bool m_isInit;
	void Initialize( void );						// set up attachment and pose param indices

	int m_eyeAttachment;
	int m_chestAttachment;

	int m_bodyYawPoseParam;
	float m_bodyYawMin;
	float m_bodyYawMax;

	int m_headYawPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_flCurrentHeadYaw;
	float m_flLastBodyYaw;

	int m_headPitchPoseParam;
	float m_headPitchMin;
	float m_headPitchMax;
	float m_flCurrentHeadPitch;

	bool m_createdLowViolenceRagdoll;

	CHandle<C_BaseAnimating> m_hRagdollOnClient;
	CHandle<C_BaseAnimating> m_hHolidayHatAddon;
	
private:
	C_CHostage( const C_CHostage & );				// not defined, not accessible
};


inline C_CSPlayer *C_CHostage::GetLeader( void ) const
{
	return ToCSPlayer( m_leader.m_Value );
}


extern CUtlVector< C_CHostage* > g_Hostages;
extern CUtlVector< CHandle<C_BaseAnimating> > g_HostageRagdolls;


#endif // C_CHOSTAGE_H
