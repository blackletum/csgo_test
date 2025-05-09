﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TRIGGERS_H
#define TRIGGERS_H
#ifdef _WIN32
#pragma once
#endif

#include "basetoggle.h"
#include "entityoutput.h"

#include "triggers_shared.h"

// DVS TODO: get rid of CBaseToggle
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseTrigger : public CBaseToggle
{
	DECLARE_CLASS( CBaseTrigger, CBaseToggle );
	DECLARE_SERVERCLASS();
public:
	CBaseTrigger();
	
	void Activate( void );
	virtual void PostClientActive( void );
	void InitTrigger( void );

	virtual void Enable( void );
	virtual void Disable( void );

	void Spawn( void );
	void UpdateOnRemove( void );
	void TouchTest(  void );

	// Input handlers
	virtual void InputEnable( inputdata_t &inputdata );
	virtual void InputDisable( inputdata_t &inputdata );
	virtual void InputToggle( inputdata_t &inputdata );
	virtual void InputTouchTest ( inputdata_t &inputdata );

	virtual void InputStartTouch( inputdata_t &inputdata );
	virtual void InputEndTouch( inputdata_t &inputdata );

	virtual bool UsesFilter( void ){ return ( m_hFilter.Get() != NULL ); }
	virtual bool PassesTriggerFilters(CBaseEntity *pOther);
	virtual void StartTouch(CBaseEntity *pOther);
	virtual void EndTouch(CBaseEntity *pOther);

	virtual void OnStartTouchAll(CBaseEntity *pOther);
	virtual void OnEndTouchAll(CBaseEntity *pOther);

	bool IsTouching( CBaseEntity *pOther );

	CBaseEntity *GetTouchedEntityOfType( const char *sClassName );

	virtual int	 DrawDebugTextOverlays(void);

	// by default, triggers don't deal with TraceAttack
	void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType) {}

	CNetworkVarForDerived( bool, m_bDisabled );
	string_t	m_iFilterName;
	CHandle<class CBaseFilter>	m_hFilter;

	const CUtlVector< EHANDLE > *GetTouchingEntities( void ) const
	{
		return &m_hTouchingEntities;
	}

protected:

	// Outputs
	COutputEvent m_OnStartTouch;
	COutputEvent m_OnStartTouchAll;
	COutputEvent m_OnEndTouch;
	COutputEvent m_OnEndTouchAll;
	COutputEvent m_OnTouching;
	COutputEvent m_OnNotTouching;

	// Entities currently being touched by this trigger
	CUtlVector< EHANDLE >	m_hTouchingEntities;

	DECLARE_DATADESC();

	// True if trigger participates in client side prediction
	CNetworkVar( bool, m_bClientSidePredicted );
};

//-----------------------------------------------------------------------------
// Purpose: Variable sized repeatable trigger.  Must be targeted at one or more entities.
//			If "delay" is set, the trigger waits some time after activating before firing.
//			"wait" : Seconds between triggerings. (.2 default/minimum)
//-----------------------------------------------------------------------------
class CTriggerMultiple : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerMultiple, CBaseTrigger );
public:
	void Spawn( void );
	void MultiTouch( CBaseEntity *pOther );
	void MultiWaitOver( void );
	virtual void ActivateMultiTrigger(CBaseEntity *pActivator);

	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnTrigger;
};

// Global list of triggers that care about weapon fire
extern CUtlVector< CHandle<CTriggerMultiple> >	g_hWeaponFireTriggers;


//------------------------------------------------------------------------------
// Base VPhysics trigger implementation
// NOTE: This uses vphysics to compute touch events.  It doesn't do a per-frame Touch call, so the 
// Entity I/O is different from a regular trigger
//------------------------------------------------------------------------------
#define SF_VPHYSICS_MOTION_MOVEABLE	0x1000

class CBaseVPhysicsTrigger : public CBaseEntity
{
	DECLARE_CLASS( CBaseVPhysicsTrigger , CBaseEntity );
	DECLARE_SERVERCLASS();

public:
	DECLARE_DATADESC();

	virtual void Spawn();
	virtual void UpdateOnRemove();
	virtual bool CreateVPhysics();
	virtual void Activate( void );
	virtual bool PassesTriggerFilters(CBaseEntity *pOther);

	// UNDONE: Pass trigger event in or change Start/EndTouch.  Add ITriggerVPhysics perhaps?
	// BUGBUG: If a player touches two of these, his movement will screw up.
	// BUGBUG: If a player uses crouch/uncrouch it will generate touch events and clear the motioncontroller flag
	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

	void InputToggle( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	

protected:
	CNetworkVarForDerived( bool, m_bDisabled );
	string_t					m_iFilterName;
	CHandle<class CBaseFilter>	m_hFilter;
};

//-----------------------------------------------------------------------------
// Purpose: Hurts anything that touches it. If the trigger has a targetname,
//			firing it will toggle state.
//-----------------------------------------------------------------------------
class CTriggerHurt : public CBaseTrigger
{
public:
	CTriggerHurt()
	{
		// This field came along after levels were built so the field defaults to 20 here in the constructor.
		m_flDamageCap = 20.0f;
	}

	DECLARE_CLASS( CTriggerHurt, CBaseTrigger );

	void Spawn( void );
	void RadiationThink( void );
	void HurtThink( void );
	void Touch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );
	bool HurtEntity( CBaseEntity *pOther, float damage );
	int HurtAllTouchers( float dt );

	void NavThink( void );

	DECLARE_DATADESC();

	float	m_flOriginalDamage;	// Damage as specified by the level designer.
	float	m_flDamage;			// Damage per second.
	float	m_flDamageCap;		// Maximum damage per second.
	float	m_flLastDmgTime;	// Time that we last applied damage.
	float	m_flDmgResetTime;	// For forgiveness, the time to reset the counter that accumulates damage.
	int		m_bitsDamageInflict;	// DMG_ damage type that the door or tigger does
	int		m_damageModel;
	bool	m_bNoDmgForce;		// Should damage from this trigger impart force on what it's hurting

	enum
	{
		DAMAGEMODEL_NORMAL = 0,
		DAMAGEMODEL_DOUBLE_FORGIVENESS,
	};

	// Outputs
	COutputEvent m_OnHurt;
	COutputEvent m_OnHurtPlayer;

	CUtlVector<EHANDLE>	m_hurtEntities;
};
//
//  Callback trigger
//

class CTriggerCallback : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerCallback, CBaseTrigger );
	DECLARE_DATADESC();
	
	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *pOther );

	static CTriggerCallback *Create( const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecMins, const Vector &vecMaxs, CBaseEntity *pOwner, void (CBaseEntity::*pfnCallback)(CBaseEntity *) )
	{
		CTriggerCallback *pTrigger = (CTriggerCallback *) CreateEntityByName( "trigger_callback" );
		if ( pTrigger == NULL )
			return NULL;

		UTIL_SetOrigin( pTrigger, vecOrigin );
		pTrigger->SetAbsAngles( vecAngles );
		UTIL_SetSize( pTrigger, vecMins, vecMaxs );

		DispatchSpawn( pTrigger );

		pTrigger->SetParent( (CBaseEntity *) pOwner );

		// Save our callback function
		pTrigger->m_pfnCallback = pfnCallback;

		return pTrigger;
	}

private:
	void (CBaseEntity::*m_pfnCallback)(CBaseEntity *);
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTriggerCamera : public CBaseEntity
{
public:
	DECLARE_CLASS( CTriggerCamera, CBaseEntity );
	// script description
	DECLARE_ENT_SCRIPTDESC();


	CTriggerCamera();
	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Enable( void );
	void Disable( void );
	void SetPlayer( CBaseEntity *pPlayer );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void FindAttachment( void );
	void FollowTarget( void );
	void ReturnToEyes( void );
	void MoveViewTo( QAngle vecGoalView );
	void Move( void );
	void StartCameraShot( const char *pszShotType, CBaseEntity *pSceneEntity, CBaseEntity *pActor1, CBaseEntity *pActor2, float duration );
	int ScriptGetFov( void );
	void ScriptSetFov( int iFOV, float rate );

	// Always transmit to clients so they know where to move the view to
	virtual int UpdateTransmitState();

	DECLARE_DATADESC();

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	void InputSetTarget( inputdata_t &inputdata );
	void InputSetTargetAttachment( inputdata_t &inputdata );

	void InputReturnToEyes( inputdata_t &inputdata );
	void InputTeleportToView( inputdata_t &inputdata );
	

	void InputSetTrackSpeed( inputdata_t &inputdata );

	void InputSetPath( inputdata_t &inputdata );

private:
	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;

	// used for moving the camera along a path (rail rides)
	CBaseEntity *m_pPath;
	string_t m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int	  m_state;
	Vector m_vecMoveDir;

	float m_fov;
	float m_fovSpeed;
	float m_trackSpeed;

	string_t m_iszTargetAttachment;
	int	  m_iAttachmentIndex;
	bool  m_bSnapToGoal;

#if HL2_EPISODIC
	bool  m_bInterpolatePosition;

	// these are interpolation vars used for interpolating the camera over time
	Vector m_vStartPos, m_vEndPos;
	float m_flInterpStartTime;

	const static float kflPosInterpTime; // seconds
#endif

	int   m_nPlayerButtons;
	int m_nOldTakeDamage;

private:
	COutputEvent m_OnEndFollow;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTriggerViewProxy : public CBaseEntity
{
public:
	DECLARE_CLASS( CTriggerViewProxy, CBaseEntity );

	CTriggerViewProxy();
	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Enable( void );
	void Disable( void );
	void SetPlayer( CBaseEntity *pPlayer );
	void TranslateViewToProxy( void );
	void Move( void );

	Vector GetPlayerOffset();

	// Always transmit to clients so they know where to move the view to
	virtual int UpdateTransmitState();

	DECLARE_DATADESC();

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	void InputTeleportPlayerToProxy( inputdata_t &inputdata );


private:
	EHANDLE m_hPlayer;

	CBaseEntity *m_pProxy;
	string_t m_sProxy;
	string_t m_sProxyAttachment;
	int m_nParentAttachment;
	int	m_state;
	int	m_nOffsetType;
	Vector m_vecInitialPosition;
	Vector m_vecLastPosition;
	Vector m_vecLastVelocity;
	Vector m_vecInitialOffset;

	float m_flTiltFraction;
	float m_flStartTime;

	bool m_bUseFakeAcceleration;
	bool m_bSkewAccelerationForward;
	float m_flAccelerationScalar;

	bool m_bEaseAnglesToCamera;

	int m_nPlayerButtons;
	int m_nOldTakeDamage;
};

#endif // TRIGGERS_H
