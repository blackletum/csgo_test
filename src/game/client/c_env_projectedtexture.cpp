﻿//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

#include "c_env_projectedtexture.h"
#include "shareddefs.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "texture_group_names.h"
#include "tier0/icommandline.h"
#include "tier0/platform.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

float C_EnvProjectedTexture::m_flVisibleBBoxMinHeight = -FLT_MAX;


IMPLEMENT_CLIENTCLASS_DT( C_EnvProjectedTexture, DT_EnvProjectedTexture, CEnvProjectedTexture )
	RecvPropEHandle( RECVINFO( m_hTargetEntity )	),
	RecvPropBool(	 RECVINFO( m_bState )			),
	RecvPropBool(	 RECVINFO( m_bAlwaysUpdate )	),
	RecvPropFloat(	 RECVINFO( m_flLightFOV )		),
	RecvPropBool(	 RECVINFO( m_bEnableShadows )	),
	RecvPropBool(	 RECVINFO( m_bSimpleProjection )	),
	RecvPropBool(	 RECVINFO( m_bLightOnlyTarget ) ),
	RecvPropBool(	 RECVINFO( m_bLightWorld )		),
	RecvPropBool(	 RECVINFO( m_bCameraSpace )		),
	RecvPropFloat(	 RECVINFO( m_flBrightnessScale )	),
	RecvPropInt(	 RECVINFO( m_LightColor ), 0, RecvProxy_Int32ToColor32 ),
	RecvPropFloat(	 RECVINFO( m_flColorTransitionTime )		),
	RecvPropFloat(	 RECVINFO( m_flAmbient )		),
	RecvPropString(  RECVINFO( m_SpotlightTextureName ) ),
	RecvPropInt(	 RECVINFO( m_nSpotlightTextureFrame ) ),
	RecvPropFloat(	 RECVINFO( m_flNearZ )	),
	RecvPropFloat(	 RECVINFO( m_flFarZ )	),
	RecvPropInt(	 RECVINFO( m_nShadowQuality )	),
	RecvPropFloat(	 RECVINFO( m_flProjectionSize )	),
	RecvPropFloat(	 RECVINFO( m_flRotation )	),
	RecvPropInt(	 RECVINFO( m_iStyle ) ),
END_RECV_TABLE()

C_EnvProjectedTexture *C_EnvProjectedTexture::Create( )
{
	C_EnvProjectedTexture *pEnt = new C_EnvProjectedTexture();

	pEnt->m_flNearZ = 4.0f;
	pEnt->m_flFarZ = 2000.0f;
//	strcpy( pEnt->m_SpotlightTextureName, "particle/rj" );
	pEnt->m_bLightWorld = true;
	pEnt->m_bLightOnlyTarget = false;
	pEnt->m_bSimpleProjection = true;
	pEnt->m_nShadowQuality = 1;
	pEnt->m_flLightFOV = 10.0f;
	pEnt->m_LightColor.r = 255;
	pEnt->m_LightColor.g = 255;
	pEnt->m_LightColor.b = 255;
	pEnt->m_LightColor.a = 255;
	pEnt->m_bEnableShadows = false;
	pEnt->m_flColorTransitionTime = 1.0f;
	pEnt->m_bCameraSpace = false;
	pEnt->SetAbsAngles( QAngle( 90, 0, 0 ) );
	pEnt->m_bAlwaysUpdate = true;
	pEnt->m_bState = true;
	pEnt->m_flProjectionSize = 500.0f;
	pEnt->m_flRotation = 0.0f;

	return pEnt;
}

C_EnvProjectedTexture::C_EnvProjectedTexture( void )
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	m_bForceUpdate = true;
	m_pMaterial = NULL;
	m_bIsCurrentlyProjected = false;
	AddToEntityList( ENTITY_LIST_SIMULATE );
}

C_EnvProjectedTexture::~C_EnvProjectedTexture( void )
{
	ShutDownLightHandle();
}

void C_EnvProjectedTexture::ShutDownLightHandle( void )
{
	// Clear out the light
	if( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		if ( m_bSimpleProjection == true )
		{
			g_pClientShadowMgr->DestroyProjection( m_LightHandle );
		}
		else
		{
			g_pClientShadowMgr->DestroyFlashlight( m_LightHandle );
		}
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}

	m_bIsCurrentlyProjected = false;
}


void C_EnvProjectedTexture::SetMaterial( IMaterial *pMaterial )
{
	if ( pMaterial != m_ProjectedMaterial )
	{
		m_ProjectedMaterial.Init( pMaterial );
		pMaterial->AddRef();
	}
}


void C_EnvProjectedTexture::SetLightColor( byte r, byte g, byte b, byte a )
{
	m_LightColor.r = r;
	m_LightColor.g = g;
	m_LightColor.b = b;
	m_LightColor.a = a;
}


void C_EnvProjectedTexture::SetSize( float flSize )
{
	m_flProjectionSize = flSize;
}


void C_EnvProjectedTexture::SetRotation( float flRotation )
{
	m_flRotation = flRotation;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_EnvProjectedTexture::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		Assert( m_SpotlightTexture.IsValid() == false );
		m_SpotlightTexture.Init( m_SpotlightTextureName, TEXTURE_GROUP_OTHER, true );
		m_ProjectedMaterial.Init( m_SpotlightTextureName, TEXTURE_GROUP_OTHER );
	}


	m_bForceUpdate = true;
	UpdateLight();
	BaseClass::OnDataChanged( updateType );
}

static ConVar asw_perf_wtf("asw_perf_wtf", "0", FCVAR_DEVELOPMENTONLY, "Disable updating of projected shadow textures from UpdateLight" );
extern ConVar r_flashlightenableculling;

bool C_EnvProjectedTexture::ShouldUpdate( void )
{
	if ( !IsGameConsole() )
	{
		CPULevel_t nCPULevel = GetCPULevel();
		bool bNoDraw = ( GetMinCPULevel() && GetMinCPULevel()-1 > nCPULevel );
		bNoDraw = bNoDraw || ( GetMaxCPULevel() && GetMaxCPULevel()-1 < nCPULevel );
		if ( bNoDraw )
			return false;

		GPULevel_t nGPULevel = GetGPULevel();
		bNoDraw = ( GetMinGPULevel() && GetMinGPULevel()-1 > nGPULevel );
		bNoDraw = bNoDraw || ( GetMaxGPULevel() && GetMaxGPULevel()-1 < nGPULevel );
		if ( bNoDraw )
			return false;
	}

	return true;
}

void C_EnvProjectedTexture::UpdateLight( void )
{
	if ( !ShouldUpdate() )
	{
		if ( m_bIsCurrentlyProjected )
		{
			ShutDownLightHandle();
		}
		return;
	}

	VPROF("C_EnvProjectedTexture::UpdateLight");
	bool bVisible = true;

	Vector vLinearFloatLightColor( m_LightColor.r, m_LightColor.g, m_LightColor.b );
	float flLinearFloatLightAlpha = m_LightColor.a;

	if ( m_bAlwaysUpdate )
	{
		m_bForceUpdate = true;
	}

	if ( m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha )
	{
		float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

		m_CurrentLinearFloatLightColor.x = Approach( vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed );
		m_CurrentLinearFloatLightColor.y = Approach( vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed );
		m_CurrentLinearFloatLightColor.z = Approach( vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed );
		m_flCurrentLinearFloatLightAlpha = Approach( flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed );

		m_bForceUpdate = true;
	}
	
	if ( !m_bForceUpdate && r_flashlightenableculling.GetBool() )
	{
		bVisible = IsBBoxVisible();		
	}

	if ( m_bState == false || !bVisible )
	{
		// Spotlight's extents aren't in view
		ShutDownLightHandle();

		return;
	}

	if ( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE || m_hTargetEntity != NULL || m_bForceUpdate )
	{
		Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
		FlashlightState_t state;

		if ( m_hTargetEntity != NULL )
		{
			if ( m_bCameraSpace )
			{
				const QAngle &angles = GetLocalAngles();

				C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
				if( pPlayer )
				{
					const QAngle playerAngles = pPlayer->GetAbsAngles();

					Vector vPlayerForward, vPlayerRight, vPlayerUp;
					AngleVectors( playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp );

					matrix3x4_t	mRotMatrix;
					AngleMatrix( angles, mRotMatrix );

					VectorITransform( vPlayerForward, mRotMatrix, vForward );
					VectorITransform( vPlayerRight, mRotMatrix, vRight );
					VectorITransform( vPlayerUp, mRotMatrix, vUp );

					float dist = (m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin()).Length();
					vPos = m_hTargetEntity->GetAbsOrigin() - vForward*dist;

					VectorNormalize( vForward );
					VectorNormalize( vRight );
					VectorNormalize( vUp );
				}
			}
			else
			{
				vForward = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
				VectorNormalize( vForward );

				// JasonM - unimplemented
				Assert (0);

				//Quaternion q = DirectionToOrientation( dir );


				//
				// JasonM - set up vRight, vUp
				//

				//			VectorNormalize( vRight );
				//			VectorNormalize( vUp );
			}
		}
		else
		{
			AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );
		}

		state.m_fHorizontalFOVDegrees = m_flLightFOV;
		state.m_fVerticalFOVDegrees = m_flLightFOV;

		state.m_vecLightOrigin = vPos;
		BasisToQuaternion( vForward, vRight, vUp, state.m_quatOrientation );
		state.m_NearZ = m_flNearZ;
		state.m_FarZ = m_flFarZ;

		if ( r_flashlightenableculling.GetBool() )
		{
			// quickly check the proposed light's bbox against the view frustum to determine whether we
			// should bother to create it, if it doesn't exist, or cull it, if it does.

#ifndef LINUX
#pragma message("OPTIMIZATION: this should be made SIMD")
#endif
			// get the half-widths of the near and far planes, 
			// based on the FOV which is in degrees. Remember that
			// on planet Valve, x is forward, y left, and z up. 
			const float tanHalfAngle = tan( m_flLightFOV * ( M_PI/180.0f ) * 0.5f );
			const float halfWidthNear = tanHalfAngle * m_flNearZ;
			const float halfWidthFar = tanHalfAngle * m_flFarZ;
			// now we can build coordinates in local space: the near rectangle is eg 
			// (0, -halfWidthNear, -halfWidthNear), (0,  halfWidthNear, -halfWidthNear), 
			// (0,  halfWidthNear,  halfWidthNear), (0, -halfWidthNear,  halfWidthNear)

			VectorAligned vNearRect[4] = { 
				VectorAligned( m_flNearZ, -halfWidthNear, -halfWidthNear), VectorAligned( m_flNearZ,  halfWidthNear, -halfWidthNear),
				VectorAligned( m_flNearZ,  halfWidthNear,  halfWidthNear), VectorAligned( m_flNearZ, -halfWidthNear,  halfWidthNear) 
			};

			VectorAligned vFarRect[4] = { 
				VectorAligned( m_flFarZ, -halfWidthFar, -halfWidthFar), VectorAligned( m_flFarZ,  halfWidthFar, -halfWidthFar),
				VectorAligned( m_flFarZ,  halfWidthFar,  halfWidthFar), VectorAligned( m_flFarZ, -halfWidthFar,  halfWidthFar) 
			};

			matrix3x4_t matOrientation( vForward, -vRight, vUp, vPos );

			enum
			{
				kNEAR = 0,
				kFAR = 1,
			};
			VectorAligned vOutRects[2][4];
			
			for ( int i = 0 ; i < 4 ; ++i )
			{
				VectorTransform( vNearRect[i].Base(), matOrientation, vOutRects[0][i].Base() );
			}
			for ( int i = 0 ; i < 4 ; ++i )
			{
				VectorTransform( vFarRect[i].Base(), matOrientation, vOutRects[1][i].Base() );
			}

			// now take the min and max extents for the bbox, and see if it is visible.
			Vector mins = **vOutRects; 
			Vector maxs = **vOutRects; 
			for ( int i = 1; i < 8 ; ++i )
			{
				VectorMin( mins, *(*vOutRects+i), mins );
				VectorMax( maxs, *(*vOutRects+i), maxs );
			}

#if 0 //for debugging the visibility frustum we just calculated
			NDebugOverlay::Triangle( vOutRects[0][0], vOutRects[0][1], vOutRects[0][2], 255, 0, 0, 100, true, 0.0f ); //first tri
			NDebugOverlay::Triangle( vOutRects[0][2], vOutRects[0][1], vOutRects[0][0], 255, 0, 0, 100, true, 0.0f ); //make it double sided
			NDebugOverlay::Triangle( vOutRects[0][2], vOutRects[0][3], vOutRects[0][0], 255, 0, 0, 100, true, 0.0f ); //second tri
			NDebugOverlay::Triangle( vOutRects[0][0], vOutRects[0][3], vOutRects[0][2], 255, 0, 0, 100, true, 0.0f ); //make it double sided

			NDebugOverlay::Triangle( vOutRects[1][0], vOutRects[1][1], vOutRects[1][2], 0, 0, 255, 100, true, 0.0f ); //first tri
			NDebugOverlay::Triangle( vOutRects[1][2], vOutRects[1][1], vOutRects[1][0], 0, 0, 255, 100, true, 0.0f ); //make it double sided
			NDebugOverlay::Triangle( vOutRects[1][2], vOutRects[1][3], vOutRects[1][0], 0, 0, 255, 100, true, 0.0f ); //second tri
			NDebugOverlay::Triangle( vOutRects[1][0], vOutRects[1][3], vOutRects[1][2], 0, 0, 255, 100, true, 0.0f ); //make it double sided

			NDebugOverlay::Box( vec3_origin, mins, maxs, 0, 255, 0, 100, 0.0f );
#endif
			
			bool bVisible = IsBBoxVisible( mins, maxs );


			if (!bVisible)
			{
				// Spotlight's extents aren't in view
				if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
				{
					ShutDownLightHandle();
				}

				return;
			}
		}

		float flAlpha = m_flCurrentLinearFloatLightAlpha * ( 1.0f / 255.0f );

		// Get the current light style value to throttle the brightness by
		flAlpha *= engine->LightStyleValue( m_iStyle );

		state.m_fQuadraticAtten = 0.0;
		state.m_fLinearAtten = 100;
		state.m_fConstantAtten = 0.0f;
		state.m_FarZAtten = m_flFarZ;
		state.m_fBrightnessScale = m_flBrightnessScale;
		state.m_Color[0] = m_CurrentLinearFloatLightColor.x * ( 1.0f / 255.0f ) * flAlpha;
		state.m_Color[1] = m_CurrentLinearFloatLightColor.y * ( 1.0f / 255.0f ) * flAlpha;
		state.m_Color[2] = m_CurrentLinearFloatLightColor.z * ( 1.0f / 255.0f ) * flAlpha;
		state.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
		state.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();
		state.m_flShadowDepthBias = g_pMaterialSystemHardwareConfig->GetShadowDepthBias();
		state.m_bEnableShadows = m_bEnableShadows;
		extern ConVar r_flashlightdepthres;
		state.m_flShadowMapResolution = r_flashlightdepthres.GetFloat();

		if ( m_bSimpleProjection )
		{
			state.m_pSpotlightTexture = NULL;
			state.m_pProjectedMaterial = m_ProjectedMaterial;
		}
		else
		{
			state.m_pSpotlightTexture = m_SpotlightTexture;
			state.m_pProjectedMaterial = NULL;
		}

		state.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;
		state.m_flProjectionSize = m_flProjectionSize;
		state.m_flProjectionRotation = m_flRotation;

		state.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality
		state.m_bShareBetweenSplitscreenPlayers = true;	// projected flashlight entities should always be shared among players, because they live in a map.

		if( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE )
		{
			// Hack: env projected textures don't work like normal flashlights; they're not assigned to a given splitscreen slot,
			// but the flashlight code requires this
			HACK_GETLOCALPLAYER_GUARD( "Env projected texture" );
			if ( m_bSimpleProjection == true )
			{
				m_LightHandle = g_pClientShadowMgr->CreateProjection( state );
			}
			else
			{
				m_LightHandle = g_pClientShadowMgr->CreateFlashlight( state );
			}

			if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				m_bForceUpdate = false;
			}
		}
		else
		{
			if ( m_bSimpleProjection == true )
			{
				g_pClientShadowMgr->UpdateProjectionState( m_LightHandle, state );
			}
			else
			{
				g_pClientShadowMgr->UpdateFlashlightState( m_LightHandle, state );
			}
			m_bForceUpdate = false;
		}

		g_pClientShadowMgr->GetFrustumExtents( m_LightHandle, m_vecExtentsMin, m_vecExtentsMax );

		m_vecExtentsMin = m_vecExtentsMin - GetAbsOrigin();
		m_vecExtentsMax = m_vecExtentsMax - GetAbsOrigin();
	}

	if( m_bLightOnlyTarget )
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, m_hTargetEntity );
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, INVALID_EHANDLE );
	}

	g_pClientShadowMgr->SetFlashlightLightWorld( m_LightHandle, m_bLightWorld );

	if ( !asw_perf_wtf.GetBool() && !m_bForceUpdate )
	{
		g_pClientShadowMgr->UpdateProjectedTexture( m_LightHandle, true );
	}

	m_bIsCurrentlyProjected = true;
}

bool C_EnvProjectedTexture::Simulate( void )
{
	UpdateLight();

	BaseClass::Simulate();
	return true;
}

bool C_EnvProjectedTexture::IsBBoxVisible( Vector vecExtentsMin, Vector vecExtentsMax )
{
	// Z position clamped to the min height (but must be less than the max)
	float flVisibleBBoxMinHeight = MIN( vecExtentsMax.z - 1.0f, m_flVisibleBBoxMinHeight );
	vecExtentsMin.z = MAX( vecExtentsMin.z, flVisibleBBoxMinHeight );

	// Check if the bbox is in the view
	return !engine->CullBox( vecExtentsMin, vecExtentsMax );
}

