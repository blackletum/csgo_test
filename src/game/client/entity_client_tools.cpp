﻿//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "toolframework/itoolentity.h"
#include "tier1/KeyValues.h"
#include "Sprite.h"
#include "enginesprite.h"
#include "beamdraw.h"
#include "toolframework_client.h"
#include "particles/particles.h"
#include "particle_parse.h"
#include "rendertexture.h"
#include "model_types.h"
#include "vstdlib/IKeyValuesSystem.h"

#ifdef PORTAL
	#include "portalrender.h"
	#include "c_portal_player.h"
#endif

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


#pragma warning( disable: 4355 )  // warning C4355: 'this' : used in base member initializer list

class CClientTools;

void DrawSpriteModel( IClientEntity *baseentity, CEngineSprite *psprite,
						const Vector &origin, float fscale, float frame, 
						int rendermode, int r, int g, int b, int a,
						const Vector& forward, const Vector& right, const Vector& up, float flHDRColorScale = 1.0f );
float StandardGlowBlend( const pixelvis_queryparams_t &params, pixelvis_handle_t *queryHandle,
						int rendermode, int renderfx, int alpha, float *pscale );

void HandleGameEntityKeyValues( KeyValues *pKeyValues );


// Interface from engine to tools for manipulating entities
class CClientTools : public IClientTools, public IClientEntityListener
{
public:
	CClientTools();

	virtual HTOOLHANDLE		AttachToEntity( EntitySearchResult entityToAttach );
	virtual void			DetachFromEntity( EntitySearchResult entityToDetach );
	virtual EntitySearchResult	GetEntity( HTOOLHANDLE handle );
	virtual bool			IsValidHandle( HTOOLHANDLE handle );


	virtual int				GetNumRecordables();
	virtual HTOOLHANDLE		GetRecordable( int index );

	// Iterates through ALL entities (separate list for client vs. server)
	virtual EntitySearchResult	NextEntity( EntitySearchResult currentEnt );

	// Use this to turn on/off the presence of an underlying game entity
	virtual void			SetEnabled( HTOOLHANDLE handle, bool enabled );

	virtual void			SetRecording( HTOOLHANDLE handle, bool recording );
	virtual bool			ShouldRecord( HTOOLHANDLE handle );

	virtual int				GetModelIndex( HTOOLHANDLE handle );
	virtual const char*		GetModelName ( HTOOLHANDLE handle );
	virtual const char*		GetClassname ( HTOOLHANDLE handle );

	virtual HTOOLHANDLE		GetToolHandleForEntityByIndex( int entindex );

	virtual void			AddClientRenderable( IClientRenderable *pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType );
	virtual void			RemoveClientRenderable( IClientRenderable *pRenderable );
	virtual void			SetTranslucencyType( IClientRenderable *pRenderable, RenderableTranslucencyType_t nType );
	virtual void			MarkClientRenderableDirty( IClientRenderable *pRenderable );

	virtual bool			DrawSprite( IClientRenderable *pRenderable,
										float scale, float frame,
										int rendermode, int renderfx,
										const Color &color, float flProxyRadius, int *pVisHandle );
	virtual void DrawSprite( const Vector &vecOrigin, float flWidth, float flHeight, color32 color );


	virtual bool			GetLocalPlayerEyePosition( Vector& org, QAngle& ang, float &fov );
	virtual EntitySearchResult	GetLocalPlayer();

	virtual ClientShadowHandle_t	CreateShadow( CBaseHandle h, int nFlags );
	virtual void			DestroyShadow( ClientShadowHandle_t h );
	virtual ClientShadowHandle_t CreateFlashlight( const FlashlightState_t &lightState );
	virtual void			DestroyFlashlight( ClientShadowHandle_t h );
	virtual void			UpdateFlashlightState( ClientShadowHandle_t h, const FlashlightState_t &flashlightState );
	virtual void			AddToDirtyShadowList( ClientShadowHandle_t h, bool force = false );
	virtual void			MarkRenderToTextureShadowDirty( ClientShadowHandle_t h );
    virtual void			UpdateProjectedTexture( ClientShadowHandle_t h, bool bForce );

	// Global toggle for recording
	virtual void			EnableRecordingMode( bool bEnable );
	virtual bool			IsInRecordingMode() const;

	// Trigger a temp entity
	virtual void			TriggerTempEntity( KeyValues *pKeyValues );

	// get owning weapon (for viewmodels)
	virtual int				GetOwningWeaponEntIndex( int entindex );
	virtual int				GetEntIndex( EntitySearchResult entityToAttach );

	virtual int				FindGlobalFlexcontroller( char const *name );
	virtual char const		*GetGlobalFlexControllerName( int idx );

	// helper for traversing ownership hierarchy
	virtual EntitySearchResult	GetOwnerEntity( EntitySearchResult currentEnt );

	// common and useful types to query for hierarchically
	virtual bool			IsPlayer( EntitySearchResult entityToAttach );
	virtual bool			IsCombatCharacter( EntitySearchResult entityToAttach );
	virtual bool			IsNPC( EntitySearchResult entityToAttach );
	virtual bool			IsRagdoll( EntitySearchResult currentEnt );
	virtual bool			IsViewModel( EntitySearchResult entityToAttach );
	virtual bool			IsViewModelOrAttachment( EntitySearchResult entityToAttach );
	virtual bool			IsWeapon( EntitySearchResult entityToAttach );
	virtual bool			IsSprite( EntitySearchResult entityToAttach );
	virtual bool			IsProp( EntitySearchResult entityToAttach );
	virtual bool			IsBrush( EntitySearchResult entityToAttach );

	virtual Vector			GetAbsOrigin( HTOOLHANDLE handle );
	virtual QAngle			GetAbsAngles( HTOOLHANDLE handle );
	virtual void			ReloadParticleDefintions( const char *pFileName, const void *pBufData, int nLen );

	// ParticleSystem iteration, query, modification
	virtual ParticleSystemSearchResult	NextParticleSystem( ParticleSystemSearchResult sr );
	virtual void						SetRecording( ParticleSystemSearchResult sr, bool bRecord );

	// Sends a mesage from the tool to the client
	virtual void			PostToolMessage( KeyValues *pKeyValues );

	// Indicates whether the client should render particle systems
	virtual void			EnableParticleSystems( bool bEnable );

	// Is the game rendering in 3rd person mode?
	virtual bool			IsRenderingThirdPerson() const;

public:
	C_BaseEntity			*LookupEntity( HTOOLHANDLE handle );

	// IClientEntityListener methods
	void OnEntityDeleted( C_BaseEntity *pEntity );
	void OnEntityCreated( C_BaseEntity *pEntity );

private:
	struct HToolEntry_t
	{
		HToolEntry_t() : m_Handle( 0 ) {}
		explicit HToolEntry_t( int handle, C_BaseEntity *pEntity = NULL )
			: m_Handle( handle ), m_hEntity( pEntity )
		{
			if ( pEntity )
			{
				m_hEntity->SetToolHandle( m_Handle );
			}
		}

		int					m_Handle;
		EHANDLE				m_hEntity;
	};

	static int				s_nNextHandle;

	static bool HandleLessFunc( const HToolEntry_t& lhs, const HToolEntry_t& rhs )
	{
		return lhs.m_Handle < rhs.m_Handle;
	}

	CUtlRBTree< HToolEntry_t >	m_Handles;
	CUtlVector< int > m_ActiveHandles;
	bool m_bInRecordingMode;
};


//-----------------------------------------------------------------------------
// Statics
//-----------------------------------------------------------------------------
int CClientTools::s_nNextHandle = 1;


//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------
static CClientTools s_ClientTools;
IClientTools *clienttools = &s_ClientTools;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientTools, IClientTools, VCLIENTTOOLS_INTERFACE_VERSION, s_ClientTools );


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CClientTools::CClientTools() : m_Handles( 0, 0, HandleLessFunc )
{
	m_bInRecordingMode = false;
	cl_entitylist->AddListenerEntity( this );
}


//-----------------------------------------------------------------------------
// Global toggle for recording
//-----------------------------------------------------------------------------
void CClientTools::EnableRecordingMode( bool bEnable )
{
	m_bInRecordingMode = bEnable;
}

bool CClientTools::IsInRecordingMode() const
{
	return m_bInRecordingMode;
}


//-----------------------------------------------------------------------------
// Trigger a temp entity
//-----------------------------------------------------------------------------
void CClientTools::TriggerTempEntity( KeyValues *pKeyValues )
{
	te->TriggerTempEntity( pKeyValues );
}


//-----------------------------------------------------------------------------
// get owning weapon (for viewmodels)
//-----------------------------------------------------------------------------
int CClientTools::GetOwningWeaponEntIndex( int entindex )
{
	C_BaseEntity *pEnt = C_BaseEntity::Instance( entindex );
	C_BaseViewModel *pViewModel = ToBaseViewModel( pEnt );
	if ( pViewModel )
	{
		C_BaseCombatWeapon *pWeapon = pViewModel->GetOwningWeapon();
		if ( pWeapon )
		{
			return pWeapon->entindex();
		}
	}

	return -1;
}

int CClientTools::GetEntIndex( EntitySearchResult entityToAttach )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity * >( entityToAttach );
	return ent ? ent->entindex() : 0;
}

void CClientTools::AddClientRenderable( IClientRenderable *pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType )
{
	Assert( pRenderable );

	cl_entitylist->AddNonNetworkableEntity( pRenderable->GetIClientUnknown() );

	ClientRenderHandle_t handle = pRenderable->RenderHandle();
	if ( INVALID_CLIENT_RENDER_HANDLE == handle )
	{
		// create new renderer handle
		ClientLeafSystem()->AddRenderable( pRenderable, bRenderWithViewModels, nType, nModelType );
	}
	else
	{
		// handle already exists, just update group & origin
		ClientLeafSystem()->RenderWithViewModels( pRenderable->RenderHandle(), bRenderWithViewModels );
		ClientLeafSystem()->SetTranslucencyType( pRenderable->RenderHandle(), nType );
		ClientLeafSystem()->SetModelType( pRenderable->RenderHandle(), nModelType );
		ClientLeafSystem()->RenderableChanged( pRenderable->RenderHandle() );
	}
}

void CClientTools::RemoveClientRenderable( IClientRenderable *pRenderable )
{
	ClientRenderHandle_t handle = pRenderable->RenderHandle();
	if( handle != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->RemoveRenderable( handle );
	}
	cl_entitylist->RemoveEntity( pRenderable->GetIClientUnknown()->GetRefEHandle() );
}

void CClientTools::MarkClientRenderableDirty( IClientRenderable *pRenderable )
{
	ClientRenderHandle_t handle = pRenderable->RenderHandle();
	if ( INVALID_CLIENT_RENDER_HANDLE != handle )
	{
		// handle already exists, just update group & origin
		ClientLeafSystem()->RenderableChanged( pRenderable->RenderHandle() );
	}
}

void CClientTools::SetTranslucencyType( IClientRenderable *pRenderable, RenderableTranslucencyType_t nType )
{
	ClientLeafSystem()->SetTranslucencyType( pRenderable->RenderHandle(), nType );
}

void CClientTools::DrawSprite( const Vector &vecOrigin, float flWidth, float flHeight, color32 color )
{
	::DrawSprite( vecOrigin, flWidth, flHeight, color );
}

bool CClientTools::DrawSprite( IClientRenderable *pRenderable, float scale, float frame, int rendermode, int renderfx, const Color &color, float flProxyRadius, int *pVisHandle )
{
	Vector origin = pRenderable->GetRenderOrigin();
	QAngle angles = pRenderable->GetRenderAngles();

	// Get extra data
	CEngineSprite *psprite = (CEngineSprite *)modelinfo->GetModelExtraData( pRenderable->GetModel() );
	if ( !psprite )
		return false;

	// Get orthonormal bases for current view - re-align to current camera (vs. recorded camera)
	Vector forward, right, up;
	C_SpriteRenderer::GetSpriteAxes( ( C_SpriteRenderer::SPRITETYPE )psprite->GetOrientation(), origin, angles, forward, right, up );

	int r = color.r();
	int g = color.g();
	int b = color.b();

	float oldBlend = render->GetBlend();
	if ( rendermode != kRenderNormal )
	{
		// kRenderGlow and kRenderWorldGlow have a special blending function
		if (( rendermode == kRenderGlow ) || ( rendermode == kRenderWorldGlow ))
		{
			pixelvis_queryparams_t params;
			if ( flProxyRadius != 0.0f )
			{
				params.Init( origin, flProxyRadius );
				params.bSizeInScreenspace = true;
			}
			else
			{
				params.Init( origin );
			}
			float blend = oldBlend * StandardGlowBlend( params, ( pixelvis_handle_t* )pVisHandle, rendermode, renderfx, color.a(), &scale );

			if ( blend <= 0.0f )
				return false;

			//Fade out the sprite depending on distance from the view origin.
			r *= blend;
			g *= blend;
			b *= blend;

			render->SetBlend( blend );
		}
	}

	DrawSpriteModel( ( IClientEntity* )pRenderable, psprite, origin, scale, frame, rendermode, r, g, b, color.a(), forward, right, up );

	if (( rendermode == kRenderGlow ) || ( rendermode == kRenderWorldGlow ))
	{
		render->SetBlend( oldBlend );
	}

	return true;
}

HTOOLHANDLE CClientTools::AttachToEntity( EntitySearchResult entityToAttach )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity * >( entityToAttach );
	Assert( ent );
	if ( !ent )
		return (HTOOLHANDLE)0;

	HTOOLHANDLE curHandle = ent->GetToolHandle();
	if ( curHandle != 0 )
		return curHandle; // Already attaached

	HToolEntry_t newHandle( s_nNextHandle++, ent );

	m_Handles.Insert( newHandle );
	m_ActiveHandles.AddToTail( newHandle.m_Handle );

	return (HTOOLHANDLE)newHandle.m_Handle;
}

void CClientTools::DetachFromEntity( EntitySearchResult entityToDetach )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity * >( entityToDetach );
	Assert( ent );
	if ( !ent )
		return;

	HTOOLHANDLE handle = ent->GetToolHandle();
	ent->SetToolHandle( (HTOOLHANDLE)0 );

	if ( handle == (HTOOLHANDLE)0 ) 
	{
		Assert( 0 );
		return;
	}

	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
	{
		Assert( 0 );
		return;
	}

	m_Handles.RemoveAt( idx );
	m_ActiveHandles.FindAndRemove( handle );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handle - 
// Output : C_BaseEntity
//-----------------------------------------------------------------------------
C_BaseEntity *CClientTools::LookupEntity( HTOOLHANDLE handle )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return NULL;

	return m_Handles[ idx ].m_hEntity;
}

int	CClientTools::GetNumRecordables()
{
	return m_ActiveHandles.Count();
}

HTOOLHANDLE CClientTools::GetRecordable( int index )
{
	if ( index < 0 || index >= m_ActiveHandles.Count() )
	{
		Assert( 0 );
		return (HTOOLHANDLE)0;
	}

	return m_ActiveHandles[ index ];
}


//-----------------------------------------------------------------------------
// Iterates through ALL entities (separate list for client vs. server)
//-----------------------------------------------------------------------------
EntitySearchResult CClientTools::NextEntity( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	if ( ent == NULL )
	{
		ent = cl_entitylist->FirstBaseEntity();
	}
	else
	{
		ent = cl_entitylist->NextBaseEntity( ent );
	}
	return reinterpret_cast< EntitySearchResult >( ent );
}


//-----------------------------------------------------------------------------
// Use this to turn on/off the presence of an underlying game entity
//-----------------------------------------------------------------------------
void CClientTools::SetEnabled( HTOOLHANDLE handle, bool enabled )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return;

	HToolEntry_t *slot = &m_Handles[ idx ];
	Assert( slot );
	if ( slot == NULL )
		return;

	C_BaseEntity *ent = slot->m_hEntity.Get();
	if ( ent == NULL ||	ent->entindex() == 0 )
		return; // Don't disable/enable the "world"

	ent->EnableInToolView( enabled );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientTools::SetRecording( HTOOLHANDLE handle, bool recording )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return;

	HToolEntry_t &entry = m_Handles[ idx ];
	if ( entry.m_hEntity )
	{
		entry.m_hEntity->SetToolRecording( recording );
	}
}

bool CClientTools::ShouldRecord( HTOOLHANDLE handle )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return false;

	HToolEntry_t &entry = m_Handles[ idx ];
	return entry.m_hEntity && entry.m_hEntity->ShouldRecordInTools();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CClientTools::GetModelIndex( HTOOLHANDLE handle )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return NULL;

	HToolEntry_t &entry = m_Handles[ idx ];
	if ( entry.m_hEntity )
	{
		return entry.m_hEntity->GetModelIndex();
	}
	Assert( 0 );
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CClientTools::GetModelName( HTOOLHANDLE handle )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return NULL;

	HToolEntry_t &entry = m_Handles[ idx ];
	if ( entry.m_hEntity )
	{
		return STRING( entry.m_hEntity->GetModelName() );
	}
	Assert( 0 );
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CClientTools::GetClassname( HTOOLHANDLE handle )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return NULL;

	HToolEntry_t &entry = m_Handles[ idx ];
	if ( entry.m_hEntity )
	{
		return STRING( entry.m_hEntity->GetClassname() );
	}
	Assert( 0 );
	return NULL;
}

EntitySearchResult CClientTools::GetEntity( HTOOLHANDLE handle )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return reinterpret_cast< EntitySearchResult >( NULL );

	HToolEntry_t *slot = &m_Handles[ idx ];
	Assert( slot );
	if ( slot == NULL )
		return reinterpret_cast< EntitySearchResult >( NULL );

	C_BaseEntity *ent = slot->m_hEntity.Get();
	return reinterpret_cast< EntitySearchResult >( ent );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handle - 
//-----------------------------------------------------------------------------
bool CClientTools::IsValidHandle( HTOOLHANDLE handle )
{
	return m_Handles.Find( HToolEntry_t( handle ) ) != m_Handles.InvalidIndex();
}

void CClientTools::OnEntityDeleted( CBaseEntity *pEntity )
{
	HTOOLHANDLE handle = pEntity ? pEntity->GetToolHandle() : (HTOOLHANDLE)0;
	if ( handle == (HTOOLHANDLE)0 )
		return;

	if ( m_bInRecordingMode )
	{
		// Send deletion message to tool interface
		KeyValues *kv = new KeyValues( "deleted" );
		ToolFramework_PostToolMessage( handle, kv );
		kv->deleteThis();
	}

	DetachFromEntity( pEntity );
}

void CClientTools::OnEntityCreated( CBaseEntity *pEntity )
{
	if ( !ToolsEnabled() )
		return;

	HTOOLHANDLE h = AttachToEntity( pEntity );

	// Send deletion message to tool interface
	KeyValues *kv = new KeyValues( "created" );
	kv->SetPtr( "esr", ( void* )pEntity );
	ToolFramework_PostToolMessage( h, kv );
	kv->deleteThis();
}

HTOOLHANDLE CClientTools::GetToolHandleForEntityByIndex( int entindex )
{
	C_BaseEntity *ent = C_BaseEntity::Instance(	entindex );
	if ( !ent )
		return (HTOOLHANDLE)0;

	return ent->GetToolHandle();
}

EntitySearchResult CClientTools::GetLocalPlayer()
{
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( 0 );
	C_BasePlayer *p = C_BasePlayer::GetLocalPlayer();
	return reinterpret_cast< EntitySearchResult >( p );
}

bool CClientTools::GetLocalPlayerEyePosition( Vector& org, QAngle& ang, float &fov )
{
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( 0 );
	C_BasePlayer *pl = C_BasePlayer::GetLocalPlayer();
	if ( pl == NULL )
		return false;

	org = pl->EyePosition();
	ang = pl->EyeAngles();
	fov = pl->GetFOV();
	return true;
}

//-----------------------------------------------------------------------------
// ParticleSystem iteration, query, modification
//-----------------------------------------------------------------------------
ParticleSystemSearchResult CClientTools::NextParticleSystem( ParticleSystemSearchResult sr )
{
	CNewParticleEffect *pParticleEffect = NULL;
	if ( sr == NULL )
	{
		pParticleEffect = ParticleMgr()->FirstNewEffect();
	}
	else
	{
		pParticleEffect = ParticleMgr()->NextNewEffect( reinterpret_cast< CNewParticleEffect* >( sr ) );
	}
	return reinterpret_cast< ParticleSystemSearchResult >( pParticleEffect );
}

void CClientTools::SetRecording( ParticleSystemSearchResult sr, bool bRecord )
{
	Assert( sr );
	if ( sr == NULL )
		return;

	CNewParticleEffect *pParticleEffect = reinterpret_cast< CNewParticleEffect* >( sr );
	pParticleEffect->SetToolRecording( bRecord );
}

//-----------------------------------------------------------------------------
// Create, destroy shadow
//-----------------------------------------------------------------------------
ClientShadowHandle_t CClientTools::CreateShadow( CBaseHandle h, int nFlags )
{
	return g_pClientShadowMgr->CreateShadow( h, nFlags, NULL );
}

void CClientTools::DestroyShadow( ClientShadowHandle_t h )
{
	g_pClientShadowMgr->DestroyShadow( h );
}

ClientShadowHandle_t CClientTools::CreateFlashlight( const FlashlightState_t &lightState )
{
	return g_pClientShadowMgr->CreateFlashlight( lightState );
}

void CClientTools::DestroyFlashlight( ClientShadowHandle_t h )
{
	g_pClientShadowMgr->DestroyFlashlight( h );
}

void CClientTools::UpdateFlashlightState( ClientShadowHandle_t h, const FlashlightState_t &lightState )
{
	g_pClientShadowMgr->UpdateFlashlightState( h, lightState );
}

void CClientTools::AddToDirtyShadowList( ClientShadowHandle_t h, bool force )
{
	g_pClientShadowMgr->AddToDirtyShadowList( h, force );
}

void CClientTools::MarkRenderToTextureShadowDirty( ClientShadowHandle_t h )
{
	g_pClientShadowMgr->MarkRenderToTextureShadowDirty( h );
}

void CClientTools::UpdateProjectedTexture( ClientShadowHandle_t h, bool bForce )
{
	g_pClientShadowMgr->UpdateProjectedTexture( h, bForce );
}

int CClientTools::FindGlobalFlexcontroller( char const *name )
{
	return C_BaseFlex::AddGlobalFlexController( (char *)name );
}

char const *CClientTools::GetGlobalFlexControllerName( int idx )
{
	return C_BaseFlex::GetGlobalFlexControllerName( idx );
}

//-----------------------------------------------------------------------------
// helper for traversing ownership hierarchy
//-----------------------------------------------------------------------------
EntitySearchResult CClientTools::GetOwnerEntity( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	return ent ? ent->GetOwnerEntity() : NULL;
}
//-----------------------------------------------------------------------------
// common and useful types to query for hierarchically
//-----------------------------------------------------------------------------
bool CClientTools::IsPlayer( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	return ent ? ent->IsPlayer() : false;
}

bool CClientTools::IsCombatCharacter( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	return ent ? ent->IsBaseCombatCharacter() : false;
}

bool CClientTools::IsNPC( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	return ent ? ent->IsNPC() : false;
}

bool CClientTools::IsRagdoll( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	C_BaseAnimating *pBaseAnimating = ent ? ent->GetBaseAnimating() : NULL;
	return pBaseAnimating ? pBaseAnimating->IsClientRagdoll() : false;
}

bool CClientTools::IsViewModel( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	C_BaseAnimating *pBaseAnimating = ent ? ent->GetBaseAnimating() : NULL;
	return pBaseAnimating ? pBaseAnimating->IsViewModel() : false;
}

bool CClientTools::IsViewModelOrAttachment( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	C_BaseAnimating *pBaseAnimating = ent ? ent->GetBaseAnimating() : NULL;
	return pBaseAnimating ? pBaseAnimating->IsViewModelOrAttachment() : false;
}

bool CClientTools::IsWeapon( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	return ent ? ent->IsBaseCombatWeapon() : false;
}

bool CClientTools::IsSprite( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	return ent ? ent->IsSprite() : false;
}

bool CClientTools::IsProp( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	return ent ? ent->IsProp() : false;
}

bool CClientTools::IsBrush( EntitySearchResult currentEnt )
{
	C_BaseEntity *ent = reinterpret_cast< C_BaseEntity* >( currentEnt );
	return ent ? ent->IsBrushModel() : false;
}

Vector CClientTools::GetAbsOrigin( HTOOLHANDLE handle )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return vec3_origin;

	HToolEntry_t &entry = m_Handles[ idx ];
	if ( entry.m_hEntity )
	{
		return entry.m_hEntity->GetAbsOrigin();
	}
	Assert( 0 );
	return vec3_origin;
}

QAngle CClientTools::GetAbsAngles( HTOOLHANDLE handle )
{
	int idx = m_Handles.Find( HToolEntry_t( handle ) );
	if ( idx == m_Handles.InvalidIndex() )
		return vec3_angle;

	HToolEntry_t &entry = m_Handles[ idx ];
	if ( entry.m_hEntity )
	{
		return entry.m_hEntity->GetAbsAngles();
	}
	Assert( 0 );
	return vec3_angle;
}


//-----------------------------------------------------------------------------
// Sends a mesage from the tool to the client
//-----------------------------------------------------------------------------
void CClientTools::PostToolMessage( KeyValues *pKeyValues )
{
	if ( !Q_stricmp( pKeyValues->GetName(), "QueryParticleManifest" ) )
	{
		// NOTE: This cannot be done during particle system init because tools aren't set up at that point
		CUtlVector<CUtlString> files;
		GetParticleManifest( files );
		int nCount = files.Count();
		for ( int i = 0; i < nCount; ++i )
		{
			char pTemp[256];
			Q_snprintf( pTemp, sizeof(pTemp), "%d", i );
			KeyValues *pSubKey = pKeyValues->FindKey( pTemp, true );
			pSubKey->SetString( "file", files[i] );
		}
		return;
	}

	if ( !Q_stricmp( pKeyValues->GetName(), "QueryMonitorTexture" ) )
	{
		pKeyValues->SetPtr( "texture", GetCameraTexture() );
		return;
	}

#ifdef PORTAL
	if ( !Q_stricmp( pKeyValues->GetName(), "portals" ) )
	{
		g_pPortalRender->HandlePortalPlaybackMessage( pKeyValues );
		return;
	}
	
	if ( !Q_stricmp( pKeyValues->GetName(), "query CPortalRenderer" ) )
	{
		pKeyValues->SetInt( "IsRenderingPortal", g_pPortalRender->GetViewRecursionLevel() );
		return;
	}
#endif

	if ( !Q_strcmp( pKeyValues->GetName(), "Game Entity KeyValues" ) )
	{
		HandleGameEntityKeyValues( pKeyValues );
	}
}


//-----------------------------------------------------------------------------
// Indicates whether the client should render particle systems
//-----------------------------------------------------------------------------
void CClientTools::EnableParticleSystems( bool bEnable )
{
	ParticleMgr()->RenderParticleSystems( bEnable );
}


//-----------------------------------------------------------------------------
// Is the game rendering in 3rd person mode?
//-----------------------------------------------------------------------------
bool CClientTools::IsRenderingThirdPerson() const
{		
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( 0 );
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return false;

	return pLocalPlayer->ShouldDrawLocalPlayer();
}


//-----------------------------------------------------------------------------
// Reload particle definitions
//-----------------------------------------------------------------------------
void CClientTools::ReloadParticleDefintions( const char *pFileName, const void *pBufData, int nLen )
{
	MDLCACHE_CRITICAL_SECTION(); // Copying particle attachment control points may end up needing to evaluate skeletons

	//////////////
	// Find any systems that depend on any system in the buffer
	// slow, but necessary if we want live reloads to work - and doesn't happen too often
	CUtlVector<CUtlString> systemNamesToReload;

	CUtlBuffer bufSystemsInBuffer(pBufData, nLen, CUtlBuffer::READ_ONLY);
	g_pParticleSystemMgr->GetParticleSystemsInBuffer( bufSystemsInBuffer, &systemNamesToReload );

	CUtlVector<CNewParticleEffect*> toReplaceEffects;
	CUtlVector<CUtlString> toReplaceNames;

	for( CNewParticleEffect *pEffect = ParticleMgr()->FirstNewEffect(); pEffect; pEffect = ParticleMgr()->NextNewEffect(pEffect) )
	{
		for( int i = 0; i < systemNamesToReload.Count(); ++i )
		{
			if ( pEffect->DependsOnSystem( systemNamesToReload[i] ) )
			{
				// only reload a given effect once
				if ( -1 == toReplaceEffects.Find(pEffect) )
				{
					toReplaceNames.AddToTail( pEffect->GetName() );
					toReplaceEffects.AddToTail( pEffect );
				}
			}
		}
	}

	CUtlVector<CNonDrawingParticleSystem*> toReplaceEffectsNonDrawing;
	CUtlVector<CUtlString> toReplaceNamesNonDrawing;
	for( CNonDrawingParticleSystem *i = ParticleMgr()->m_NonDrawingParticleSystems.Head(); i ; i = i->m_pNext )
	{
		for( int j = 0; j < systemNamesToReload.Count(); ++j )
		{
			if ( i->Get()->DependsOnSystem( systemNamesToReload[j] ) )
			{
				if ( -1 == toReplaceEffectsNonDrawing.Find( i ) )
				{
					toReplaceNamesNonDrawing.AddToTail( i->Get()->GetName() );
					toReplaceEffectsNonDrawing.AddToTail( i );
				}
			}
		}
	}

	//////////////
	// Load the data and stomp the old definitions
	CUtlBuffer bufReadParticleConfigFile(pBufData, nLen, CUtlBuffer::READ_ONLY);
	g_pParticleSystemMgr->ReadParticleConfigFile( bufReadParticleConfigFile, true );

	//////////////
	// Now replace all of the systems with their new versions
	Assert( toReplaceEffects.Count() == toReplaceNames.Count() );

	for( int i = 0; i < toReplaceNames.Count(); ++i )
	{
		CNewParticleEffect *pEffect = toReplaceEffects[i];
		pEffect->ReplaceWith( toReplaceNames[i] );
	}

	// update all the non-drawings ones
	for( int i = 0; i < toReplaceNamesNonDrawing.Count(); i++ )
	{
		CNonDrawingParticleSystem *pEffect = toReplaceEffectsNonDrawing[i];
		delete pEffect->m_pSystem;
		pEffect->m_pSystem = g_pParticleSystemMgr->CreateParticleCollection( toReplaceNamesNonDrawing[i] );
	}
}


CIFM_EntityKeyValuesHandler_AutoRegister::CIFM_EntityKeyValuesHandler_AutoRegister( const char *szHandlerID )
: m_szHandlerID( szHandlerID )
{
#if defined( DBGFLAG_ASSERT )
	const CIFM_EntityKeyValuesHandler_AutoRegister *pWalk = s_pRegisteredHandlers;
	while( pWalk )
	{
		AssertMsg( strcmp( szHandlerID, pWalk->m_szHandlerID ) != 0, "Handler already registered for this ID" );
		pWalk = pWalk->m_pNext;
	}
#endif
	m_pNext = s_pRegisteredHandlers;
	s_pRegisteredHandlers = this;
}

void CIFM_EntityKeyValuesHandler_AutoRegister::AllHandlers_PreUpdate( void )
{
	CIFM_EntityKeyValuesHandler_AutoRegister *pWalk = s_pRegisteredHandlers;
	while( pWalk )
	{
		pWalk->HandleData_PreUpdate();
		pWalk = pWalk->m_pNext;
	}
}

void CIFM_EntityKeyValuesHandler_AutoRegister::FindAndCallHandler( const char *szHandlerID, KeyValues *pKeyValues )
{
	CIFM_EntityKeyValuesHandler_AutoRegister *pWalk = s_pRegisteredHandlers;
	while( pWalk )
	{
		if( strcmp( szHandlerID, pWalk->m_szHandlerID ) == 0 )
		{
			pWalk->HandleData( pKeyValues );
			return;
		}
		pWalk = pWalk->m_pNext;
	}

	AssertMsg( false, "Unhandled NonConformantData update" );
}

void CIFM_EntityKeyValuesHandler_AutoRegister::AllHandlers_PostUpdate( void )
{
	CIFM_EntityKeyValuesHandler_AutoRegister *pWalk = s_pRegisteredHandlers;
	while( pWalk )
	{
		pWalk->HandleData_PostUpdate();
		pWalk = pWalk->m_pNext;
	}
}

void CIFM_EntityKeyValuesHandler_AutoRegister::AllHandlers_RemoveAll( void )
{
	CIFM_EntityKeyValuesHandler_AutoRegister *pWalk = s_pRegisteredHandlers;
	while( pWalk )
	{
		pWalk->HandleData_RemoveAll();
		pWalk = pWalk->m_pNext;
	}
}

CIFM_EntityKeyValuesHandler_AutoRegister *CIFM_EntityKeyValuesHandler_AutoRegister::s_pRegisteredHandlers;

HKeySymbol CIFM_EntityKeyValuesHandler_AutoRegister::GetGameKeyValuesKeySymbol( void )
{
	static HKeySymbol s_hNonConformantSymbol = KeyValuesSystem()->GetSymbolForString( GetGameKeyValuesKeyString() );
	return s_hNonConformantSymbol;
}

const char *CIFM_EntityKeyValuesHandler_AutoRegister::GetGameKeyValuesKeyString( void )
{
	return "gamekeyvalues";
}

HKeySymbol CIFM_EntityKeyValuesHandler_AutoRegister::GetHandlerIDKeySymbol( void )
{
	static HKeySymbol s_hHandlerIDSymbol = KeyValuesSystem()->GetSymbolForString( GetHandlerIDKeyString() );
	return s_hHandlerIDSymbol;
}

const char *CIFM_EntityKeyValuesHandler_AutoRegister::GetHandlerIDKeyString( void )
{
	return "handlerID";
}



KeyValues *CIFM_EntityKeyValuesHandler_AutoRegister::FindOrCreateNonConformantKeyValues( KeyValues *pParentKV )
{
	KeyValues *pReturn = pParentKV->FindKey( GetGameKeyValuesKeySymbol() );
	if( !pReturn )
	{
		pReturn = pParentKV->FindKey( GetGameKeyValuesKeyString(), true );
	}
	
	return pReturn;
}

void HandleGameEntityKeyValues( KeyValues *pKeyValues )
{
	if( pKeyValues->GetBool( "RemoveAll", false ) )
	{
		CIFM_EntityKeyValuesHandler_AutoRegister::AllHandlers_RemoveAll();
	}
	else
	{
		HKeySymbol handlerIDsym = CIFM_EntityKeyValuesHandler_AutoRegister::GetHandlerIDKeySymbol();

		CIFM_EntityKeyValuesHandler_AutoRegister::AllHandlers_PreUpdate();

		for ( KeyValues *pCurr = pKeyValues->GetFirstTrueSubKey(); pCurr; pCurr = pCurr->GetNextTrueSubKey() )
		{
			const char *szHandler = pCurr->GetString( handlerIDsym, "" );
			CIFM_EntityKeyValuesHandler_AutoRegister::FindAndCallHandler( szHandler, pCurr );
		}

		CIFM_EntityKeyValuesHandler_AutoRegister::AllHandlers_PostUpdate();
	}
}








CIFM_EntityKeyValuesHandler_RecreateEntities::CIFM_EntityKeyValuesHandler_RecreateEntities( const char *szHandlerID )
: CIFM_EntityKeyValuesHandler_AutoRegister( szHandlerID )
{
}

void CIFM_EntityKeyValuesHandler_RecreateEntities::HandleData_PreUpdate( void )
{
	for( int i = 0; i != m_PlaybackEntities.Count(); ++i )
	{
		m_PlaybackEntities[i].bTouched = false;
	}
}

void CIFM_EntityKeyValuesHandler_RecreateEntities::HandleData_PostUpdate( void )
{
	for( int i = m_PlaybackEntities.Count(); --i >= 0; )
	{
		if( !m_PlaybackEntities[i].bTouched )
		{
			//not updated, clear it
			DestroyInstance( m_PlaybackEntities[i].pEntity );
			m_PlaybackEntities.FastRemove( i );
		}
	}
}

void CIFM_EntityKeyValuesHandler_RecreateEntities::HandleData( KeyValues *pKeyValues )
{
	int iEntIndex = pKeyValues->GetInt( "entIndex", -1 );
	Assert( iEntIndex != -1 );
	if( iEntIndex == -1 )
		return;

	for( int i = 0; i != m_PlaybackEntities.Count(); ++i )
	{
		if( m_PlaybackEntities[i].iEntIndex == iEntIndex )
		{
			HandleInstance( m_PlaybackEntities[i].pEntity, pKeyValues );
			m_PlaybackEntities[i].bTouched = true;
			return;
		}
	}

	//didn't exist, create it.
	RecordedEntity_t temp;
	temp.iEntIndex = iEntIndex;
	temp.pEntity = CreateInstance();
	temp.bTouched = true;

	m_PlaybackEntities.AddToTail( temp );
	HandleInstance( temp.pEntity, pKeyValues );
}


void CIFM_EntityKeyValuesHandler_RecreateEntities::HandleData_RemoveAll( void )
{
	for( int i = m_PlaybackEntities.Count(); --i >= 0; )
	{
		DestroyInstance( m_PlaybackEntities[i].pEntity );
	}
	m_PlaybackEntities.RemoveAll();
}


	




