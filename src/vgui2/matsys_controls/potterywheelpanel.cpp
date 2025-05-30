﻿//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "matsys_controls/potterywheelpanel.h"
#include "matsys_controls/manipulator.h"
#include "vgui/ISystem.h"
#include "vgui/Cursor.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "dmxloader/dmxelement.h"
#include "vgui_controls/Frame.h"
#include "convar.h"
#include "tier0/dbg.h"
#include "matsys_controls/matsyscontrols.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "istudiorender.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "tier2/renderutils.h"
#include "tier1/KeyValues.h"
#include "materialsystem/imesh.h"
#include "shaderapi/ishaderapi.h"
#include "view_shared.h"
#include "ivrenderview.h"
#include "game/client/irendercaptureconfiguration.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


//-----------------------------------------------------------------------------
// Translation manipulator
//-----------------------------------------------------------------------------
class CTranslationManipulator : public CTransformManipulator
{
public:
	CTranslationManipulator( matrix3x4_t *pTransform );

	// Methods of IManipulator
	virtual void OnMousePressed( vgui::MouseCode code, int x, int y );
	virtual void OnCursorMoved( int x, int y );

protected:
	int m_lastx, m_lasty;
};


CTranslationManipulator::CTranslationManipulator( matrix3x4_t *pTransform ) : CTransformManipulator( pTransform )
{
	m_lastx = m_lasty = 0;
}

void CTranslationManipulator::OnMousePressed( vgui::MouseCode code, int x, int y )
{
	m_lasty = y;
	m_lastx = x;
}

void CTranslationManipulator::OnCursorMoved( int x, int y )
{
	if ( !m_pTransform )
		return;

	Vector vPosition;
	QAngle quakeEuler;
	MatrixAngles( *m_pTransform, quakeEuler, vPosition );

	Vector forward, right, up;
	AngleVectors( quakeEuler, &forward, &right, &up );

	int dy = y - m_lasty;
	int dx = x - m_lastx;

	right *= -0.2f * dx;
	up *= 0.2f * dy;
	vPosition += up + right;

	m_lastx = x;
	m_lasty = y;

	PositionMatrix( vPosition, *m_pTransform );
}


//-----------------------------------------------------------------------------
// Zoom manipulator
//-----------------------------------------------------------------------------
class CZoomManipulator : public CBaseManipulator
{
public:
	CZoomManipulator( float *pDistance );

	// Methods of IManipulator
	virtual void OnMousePressed( vgui::MouseCode code, int x, int y );
	virtual void OnCursorMoved( int x, int y );

protected:
	int m_lasty;
	float *m_pDistance;
};

CZoomManipulator::CZoomManipulator( float *pDistance )
{
	m_lasty = 0;
	m_pDistance = pDistance;
}

void CZoomManipulator::OnMousePressed( vgui::MouseCode code, int x, int y )
{
	m_lasty = y;
}

void CZoomManipulator::OnCursorMoved( int x, int y )
{
	float delta  = 0.2f * ( y - m_lasty );
	m_lasty = y;
	*m_pDistance *= pow( 1.01f, delta );
}


//-----------------------------------------------------------------------------
// Rotation manipulator
//-----------------------------------------------------------------------------
class CRotationManipulator : public CTransformManipulator
{
public:
	CRotationManipulator( matrix3x4_t *pTransform );

	// Inherited from IManipulator
	virtual void OnMousePressed( vgui::MouseCode code, int x, int y );
	virtual void OnCursorMoved( int x, int y );
	virtual void UpdateTransform();

	void SetDefaultValues();

private:
	int m_lastx, m_lasty;
	float m_altitude, m_azimuth;
};


CRotationManipulator::CRotationManipulator( matrix3x4_t *pTransform ) : CTransformManipulator( pTransform )
{
	SetDefaultValues();
}

void CRotationManipulator::SetDefaultValues()
{
	m_lastx = m_lasty = 0;
	m_altitude = M_PI/6;
	m_azimuth = -3*M_PI/4;
	UpdateTransform();
}

void CRotationManipulator::OnMousePressed( vgui::MouseCode code, int x, int y )
{
	m_lasty = y;
	m_lastx = x;
}

void CRotationManipulator::OnCursorMoved( int x, int y )
{
	m_azimuth  += 0.002f * ( m_lastx - x );
	m_altitude -= 0.002f * ( m_lasty - y );
	m_altitude = MAX( -M_PI/2, MIN( M_PI/2, m_altitude ) );

	m_lastx = x;
	m_lasty = y;

	UpdateTransform();
}

void CRotationManipulator::UpdateTransform()
{
	if ( !m_pTransform )
		return;

	QAngle angles( RAD2DEG( m_altitude ), RAD2DEG( m_azimuth ), 0 );
	Vector vecPosition;
	MatrixGetColumn( *m_pTransform, 3, vecPosition );
	AngleMatrix( angles, vecPosition, *m_pTransform );
}

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CPotteryWheelPanel::CPotteryWheelPanel( vgui::Panel *pParent, const char *pName ) :
	BaseClass( pParent, pName ), 
	m_pCameraRotate( NULL ), 
	m_pCameraTranslate( NULL ),
	m_pCameraZoom( NULL ),
	m_pLightManip( NULL ),
	m_pCurrentManip( NULL ),
	m_nCaptureMouseCode( vgui::MouseCode( -1 ) ),
	m_xoffset( 0 ), m_yoffset( 0 ),
	m_bParentMouseNotify( false )
{
	m_bHasLightProbe = false;
	m_bSetupRenderStateDelayed = false;
	m_bRender3DSupersampled = false;
	m_bInRender3dForRenderCapture = false;
	m_pvRenderingWithFlashlightConfiguration = NULL;

	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );
	m_ClearColor.SetColor( 76, 88, 68, 255 );
	m_GridColor.SetColor( 255, 255, 255, 255 );

	SetIdentityMatrix( m_CameraPivot );

	CreateDefaultLights();

	m_nManipStartX = m_nManipStartY = 0;

	m_vecCameraOffset.Init( 100.0f, 0.0f, 0.0f );
		
	m_Camera.Init( Vector( 0, 0, 0 ), QAngle( 0, 0, 0 ), 3.0f, 16384.0f * 1.73205080757f, 30.0f, 1.0f );
	
	m_pCameraRotate = new CRotationManipulator( &m_CameraPivot );
	m_pCameraRotate->SetDefaultValues();
	m_pCameraTranslate = new CTranslationManipulator( &m_CameraPivot );
	m_pCameraZoom = new CZoomManipulator( &m_vecCameraOffset.x );

	SetKeyBoardInputEnabled( true );
	UpdateCameraTransform();

	// Used to poll input
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

CPotteryWheelPanel::~CPotteryWheelPanel()
{
	delete m_pCameraRotate;
	delete m_pCameraZoom;
	delete m_pCameraTranslate;
	DestroyLights();
}

void CPotteryWheelPanel::CreateDefaultLights()
{
	memset( &m_LightingState, 0, sizeof(MaterialLightingState_t) );
	for ( int i = 0; i < 6; ++i )
	{
		m_LightingState.m_vecAmbientCube[i].Init( 0.4f, 0.4f, 0.4f );
	}

	SetIdentityMatrix( m_LightToWorld[0] );
	m_LightingState.m_pLocalLightDesc[0].m_Type = MATERIAL_LIGHT_DIRECTIONAL;
	m_LightingState.m_pLocalLightDesc[0].m_Color.Init( 1.0f, 1.0f, 1.0f );
	m_LightingState.m_pLocalLightDesc[0].m_Direction.Init( 0.0f, 0.0f, -1.0f );
	m_LightingState.m_pLocalLightDesc[0].m_Range=0.0;
	m_LightingState.m_pLocalLightDesc[0].m_Attenuation0 = 1.0;
	m_LightingState.m_pLocalLightDesc[0].m_Attenuation1 = 0;
	m_LightingState.m_pLocalLightDesc[0].m_Attenuation2 = 0;
	m_LightingState.m_pLocalLightDesc[0].RecalculateDerivedValues();
	m_LightingState.m_nLocalLightCount = 1;

	m_pLightManip = new CPotteryWheelManip( &m_LightToWorld[0] );
}

void CPotteryWheelPanel::UpdateDirectionalLight( int idx, const Color& color, const Vector& direction )
{
	if ( idx >= 0 && idx < m_LightingState.m_nLocalLightCount )
	{
		// Update the existing light
		SetIdentityMatrix( m_LightToWorld[idx] );
		m_LightingState.m_pLocalLightDesc[idx].m_Type = MATERIAL_LIGHT_DIRECTIONAL;
		m_LightingState.m_pLocalLightDesc[idx].m_Color.Init( color.r() / 255.0f, color.g() / 255.0f, color.b() / 255.0f );
		m_LightingState.m_pLocalLightDesc[idx].m_Direction.Init( direction.x, direction.y, direction.z );
		m_LightingState.m_pLocalLightDesc[idx].m_Range=0.0;
		m_LightingState.m_pLocalLightDesc[idx].m_Attenuation0 = 1.0;
		m_LightingState.m_pLocalLightDesc[idx].m_Attenuation1 = 0;
		m_LightingState.m_pLocalLightDesc[idx].m_Attenuation2 = 0;
		m_LightingState.m_pLocalLightDesc[idx].RecalculateDerivedValues();
	}
	else
	{
		AddDirectionalLight( color, direction );
	}
}

void CPotteryWheelPanel::ClearDirectionalLights()
{
	m_LightingState.m_nLocalLightCount = 0;
}

void CPotteryWheelPanel::AddDirectionalLight( const Color& color, const Vector& direction )
{
	if ( m_LightingState.m_nLocalLightCount < MATERIAL_MAX_LIGHT_COUNT )
	{
		SetIdentityMatrix( m_LightToWorld[m_LightingState.m_nLocalLightCount] );
		m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount].m_Type = MATERIAL_LIGHT_DIRECTIONAL;
		m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount].m_Color.Init( color.r() / 255.0f, color.g() / 255.0f, color.b() / 255.0f );
		m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount].m_Direction.Init( direction.x, direction.y, direction.z );
		m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount].m_Range=0.0;
		m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount].m_Attenuation0 = 1.0;
		m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount].m_Attenuation1 = 0;
		m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount].m_Attenuation2 = 0;
		m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount].RecalculateDerivedValues();
		m_LightingState.m_nLocalLightCount++;
	}
}

void CPotteryWheelPanel::SetLightAmbient( const Vector& ambient )
{
	for ( int i = 0; i < 6; ++i )
	{
		m_LightingState.m_vecAmbientCube[i].Init( ambient.x, ambient.y, ambient.z );
	}
}

void CPotteryWheelPanel::DestroyLights()
{
	if ( m_pLightManip )
	{
		delete m_pLightManip;
		m_pLightManip = NULL;
	}

	m_LightingState.m_nLocalLightCount = 0;
}


//-----------------------------------------------------------------------------
// Sets the background color
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::SetBackgroundColor( int r, int g, int b )
{
	m_ClearColor.SetColor( r, g, b, 255 );
}

void CPotteryWheelPanel::SetBackgroundColor( const Color& c )
{
	m_ClearColor = c;
}

const Color& CPotteryWheelPanel::GetBackgroundColor() const
{
	return m_ClearColor;
}

void CPotteryWheelPanel::SetGridColor( int r, int g, int b )
{
	m_GridColor.SetColor( r, g, b, 255 );
}

//-----------------------------------------------------------------------------
// Light probe
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::SetLightProbe( CDmxElement *pLightProbe )
{
	m_LightProbeBackground.Shutdown();
	m_LightProbeHDRBackground.Shutdown();
	m_LightProbeCubemap.Shutdown();
	m_LightProbeHDRCubemap.Shutdown();
	DestroyLights();

	m_bHasLightProbe = ( pLightProbe != NULL );
	if ( !m_bHasLightProbe )
	{
		CreateDefaultLights();
		return;
	}

	const char *pCubemap = pLightProbe->GetValueString( "cubemap" );
	m_LightProbeCubemap.Init( pCubemap, TEXTURE_GROUP_OTHER );

	const char *pCubemapHDR = pLightProbe->HasAttribute( "cubemapHdr" ) ? pLightProbe->GetValueString( "cubemapHdr" ) : pCubemap;
	m_LightProbeHDRCubemap.Init( pCubemapHDR, TEXTURE_GROUP_OTHER );

	KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues->SetInt( "$ignorez", 1 );
	pVMTKeyValues->SetString( "$envmap", pCubemap );
	pVMTKeyValues->SetInt( "$no_fullbright", 1 );
	pVMTKeyValues->SetInt( "$nocull", 1 );
	m_LightProbeBackground.Init( "SPWP_LightProbeBackground", pVMTKeyValues );
	m_LightProbeBackground->Refresh();

	pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues->SetInt( "$ignorez", 1 );
	pVMTKeyValues->SetString( "$envmap", pCubemapHDR );
	pVMTKeyValues->SetInt( "$no_fullbright", 1 );
	pVMTKeyValues->SetInt( "$nocull", 1 );
	m_LightProbeHDRBackground.Init( "SPWP_LightProbeBackground_HDR", pVMTKeyValues );
	m_LightProbeHDRBackground->Refresh();

	const CUtlVector< Vector >& ambientCube = pLightProbe->GetArray<Vector>( "ambientCube" );
	if ( ambientCube.Count() == 6 )
	{
		for ( int i = 0; i < 6; ++i )
		{
			m_LightingState.m_vecAmbientCube[i].Init( ambientCube[i].x, ambientCube[i].y, ambientCube[i].z );
		}
	}

	const CUtlVector< CDmxElement* >& localLights = pLightProbe->GetArray< CDmxElement* >( "localLights" );
	int nLightCount = localLights.Count();
	for ( int i = 0; i < nLightCount; ++i )
	{
		if ( m_LightingState.m_nLocalLightCount == MATERIAL_MAX_LIGHT_COUNT )
			break;

		LightDesc_t *pDesc = &m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount];
		CDmxElement *pLocalLight = localLights[ i ];
		const char *pType = pLocalLight->GetValueString( "name" );
		const Vector& vecColor = pLocalLight->GetValue<Vector>( "color" );

		if ( !Q_stricmp( pType, "directional" ) )
		{
			pDesc->InitDirectional( pLocalLight->GetValue<Vector>( "direction" ), vecColor ); 
			++m_LightingState.m_nLocalLightCount;
			continue;
		}

		if ( !Q_stricmp( pType, "point" ) )
		{
			const Vector& vecAtten = pLocalLight->GetValue<Vector>( "attenuation" );
			pDesc->InitPoint( pLocalLight->GetValue<Vector>( "origin" ), vecColor );
			pDesc->m_Attenuation0 = vecAtten.x;
			pDesc->m_Attenuation1 = vecAtten.y;
			pDesc->m_Attenuation2 = vecAtten.z;
			pDesc->m_Range = pLocalLight->GetValue<float>( "maxDistance" );
			pDesc->RecalculateDerivedValues();
			++m_LightingState.m_nLocalLightCount;
			continue;
		}

		if ( !Q_stricmp( pType, "spot" ) )
		{
			const Vector& vecAtten = pLocalLight->GetValue<Vector>( "attenuation" );
			pDesc->InitSpot( pLocalLight->GetValue<Vector>( "origin" ), vecColor, vec3_origin,
				0.5f * RAD2DEG ( pLocalLight->GetValue<float>( "theta" ) ), 
				0.5f * RAD2DEG ( pLocalLight->GetValue<float>( "phi" ) ) );

			pDesc->m_Direction = pLocalLight->GetValue<Vector>( "direction" );
			pDesc->m_Attenuation0 = vecAtten.x;
			pDesc->m_Attenuation1 = vecAtten.y;
			pDesc->m_Attenuation2 = vecAtten.z;
			pDesc->m_Range = pLocalLight->GetValue<float>( "maxDistance" );
			pDesc->m_Falloff = pLocalLight->GetValue<float>( "exponent" );
			pDesc->RecalculateDerivedValues();
			++m_LightingState.m_nLocalLightCount;
			continue;
		}
	}

	if ( nLightCount > 0 )
	{
		m_pLightManip = new CPotteryWheelManip( &m_LightToWorld[0] );
	}
}

bool CPotteryWheelPanel::HasLightProbe() const
{
	return m_bHasLightProbe;
}

ITexture *CPotteryWheelPanel::GetLightProbeCubemap( bool bHDR )
{
	if ( !m_bHasLightProbe )
		return NULL;

	return bHDR ? m_LightProbeHDRCubemap : m_LightProbeCubemap;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CPotteryWheelPanel::GetCameraFOV( void )
{
	return m_Camera.m_flFOVX;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::SetCameraFOV( float flFOV )
{
	m_Camera.m_flFOVX = flFOV;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::SetCameraOffset( const Vector &vecOffset )
{
	m_vecCameraOffset = vecOffset;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::GetCameraOffset( Vector &vecOffset )
{
	vecOffset = m_vecCameraOffset;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::SetCameraPositionAndAngles( const Vector &vecPos, const QAngle &angDir )
{
	m_Camera.m_origin = vecPos;
	m_Camera.m_angles = angDir;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::GetCameraPositionAndAngles( Vector &vecPos, QAngle &angDir )
{
	MatrixAngles( m_CameraPivot, angDir, vecPos );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::ResetCameraPivot( void )
{
	SetIdentityMatrix( m_CameraPivot );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::ResetView( void )
{
	SetIdentityMatrix( m_CameraPivot );
	m_vecCameraOffset.Init( 100.0f, 0.0f, 0.0f );
	m_pCameraRotate->SetDefaultValues();
	UpdateCameraTransform();
}

//-----------------------------------------------------------------------------
// Sets the camera to look at the the thing we're spinning around
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::LookAt( float flRadius )
{
	// Compute the distance to the camera for the object based on its
	// radius and fov.

	// since tan( fov/2 ) = f/d
	// cos( fov/2 ) = r / r' where r = sphere radius, r' = perp distance from sphere center to max extent of camera
	// d/f = r'/d' where d' is distance of camera to sphere
	// d' = r' / tan( fov/2 ) * r' = r / ( cos (fov/2) * tan( fov/2 ) ) = r / sin( fov/2 )
	float flFOVx = m_Camera.m_flFOVX;

	// Compute fov/2 in radians
	flFOVx *= M_PI / 360.0f;

	// Compute an effective fov	based on the aspect ratio 
	// if the height is smaller than the width
	int w, h;
	GetSize( w, h );
	if ( h < w )
	{
		flFOVx = atan( h * tan( flFOVx ) / w );
	}

	m_vecCameraOffset.x = -( flRadius / sin( flFOVx ) );
	UpdateCameraTransform();
}


void CPotteryWheelPanel::LookAt( const Vector &vecCenter, float flRadius )
{
	MatrixSetColumn( vecCenter, 3, m_CameraPivot );
	LookAt( flRadius );
}

	
//-----------------------------------------------------------------------------
// Sets up render state in the material system for rendering
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::SetupRenderState( int nDisplayWidth, int nDisplayHeight )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	VMatrix view, projection;
	ComputeViewMatrix( &view, m_Camera );
	ComputeProjectionMatrix( &projection, m_Camera, nDisplayWidth, nDisplayHeight );

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->LoadIdentity( );

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->LoadMatrix( view );

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->LoadMatrix( projection );

	// Transform light based on manipulator
	MaterialLightingState_t state = m_LightingState;
	for ( int i = 0; i < state.m_nLocalLightCount; ++i )
	{
		const LightDesc_t &srcDesc = m_LightingState.m_pLocalLightDesc[ i ];
		LightDesc_t &destDesc = state.m_pLocalLightDesc[ i ];
		VectorTransform( srcDesc.m_Position, m_LightToWorld[i], destDesc.m_Position );
		VectorRotate( srcDesc.m_Direction, m_LightToWorld[i], destDesc.m_Direction );
		VectorNormalize( destDesc.m_Direction );
	}

	pRenderContext->SetLightingState( state );

	// FIXME: Remove this! This should automatically happen in DrawModel
	// in studiorender.
	if ( !g_pStudioRender )
		return;

	VMatrix worldToCamera;
	MatrixInverseTR( view, worldToCamera );
	Vector vecOrigin, vecRight, vecUp, vecForward;
	MatrixGetColumn( worldToCamera, 0, &vecRight );
	MatrixGetColumn( worldToCamera, 1, &vecUp );
	MatrixGetColumn( worldToCamera, 2, &vecForward );
	MatrixGetColumn( worldToCamera, 3, &vecOrigin );
	g_pStudioRender->SetViewState( vecOrigin, vecRight, vecUp, vecForward );

	g_pStudioRender->SetLocalLights( state.m_nLocalLightCount, state.m_pLocalLightDesc );
	g_pStudioRender->SetAmbientLightColors( state.m_vecAmbientCube );
}


//-----------------------------------------------------------------------------
// Compute the camera world position
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::UpdateCameraTransform( )
{
	// Set up the render state for the camera + light
	matrix3x4_t offset, worldToCamera;
	SetIdentityMatrix( offset );
	MatrixSetColumn( m_vecCameraOffset, 3, offset );
	ConcatTransforms( m_CameraPivot, offset, worldToCamera );
	MatrixAngles( worldToCamera, m_Camera.m_angles, m_Camera.m_origin );
}

void CPotteryWheelPanel::ComputeCameraTransform( matrix3x4_t *pWorldToCamera )
{
	AngleMatrix( m_Camera.m_angles, m_Camera.m_origin, *pWorldToCamera );
}


//-----------------------------------------------------------------------------
// Computes the position in the panel of a particular 3D point
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::ComputePanelPosition( const Vector &vecPosition, Vector2D *pPanelPos )
{ 
	int w, h;
	GetSize( w, h );

	matrix3x4_t worldToCamera;
	ComputeCameraTransform( &worldToCamera );
	MatrixAngles( worldToCamera, m_Camera.m_angles, m_Camera.m_origin );
	ComputeScreenSpacePosition( pPanelPos, vecPosition, m_Camera, w, h );
}


IMaterial * CPotteryWheelPanel::GetWireframeMaterial()
{
	if ( !m_Wireframe.IsValid() )
	{
		KeyValues *pMaterialKeys = new KeyValues( "Wireframe", "$model", "1" );
		pMaterialKeys->SetString( "$vertexcolor", "1" );
		m_Wireframe.Init( "potterywheelpanelwireframe", pMaterialKeys );
	}
	return m_Wireframe;
}


//-----------------------------------------------------------------------------
// Utility method to draw a grid at the 'ground'
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::DrawGrid()
{
	matrix3x4_t transform;
	CMatRenderContextPtr pRenderContext( MaterialSystem() );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->LoadIdentity( );

	pRenderContext->Bind( GetWireframeMaterial() );

	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	int nGridDim = 10;
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 2 * nGridDim + 2 );

	float bounds = 100.0f;
	float delta = 2 * bounds / nGridDim;
	for ( int i = 0; i < nGridDim + 1; ++i )
	{
		float xy = -bounds + delta * i;

		meshBuilder.Position3f( xy, -bounds, 0 );
		meshBuilder.Color4ub( m_GridColor.r(), m_GridColor.g(), m_GridColor.b(), m_GridColor.a());
		meshBuilder.AdvanceVertex();
		meshBuilder.Position3f( xy, bounds, 0 );
		meshBuilder.Color4ub( m_GridColor.r(), m_GridColor.g(), m_GridColor.b(), m_GridColor.a());
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( -bounds, xy, 0 );
		meshBuilder.Color4ub( m_GridColor.r(), m_GridColor.g(), m_GridColor.b(), m_GridColor.a());
		meshBuilder.AdvanceVertex();
		meshBuilder.Position3f( bounds, xy, 0 );
		meshBuilder.Color4ub( m_GridColor.r(), m_GridColor.g(), m_GridColor.b(), m_GridColor.a());
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// paint it!
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::Paint()
{
	int iWidth, iHeight;
	GetSize( iWidth, iHeight );

	int screenw, screenh;
	vgui::surface()->GetScreenSize( screenw, screenh );

	int windowposx = 0, windowposy = 0;
	GetPos( windowposx, windowposy );

	int windowposright = windowposx + iWidth;
	int windowposbottom = windowposy + iHeight;

	if ( windowposright >= screenw )
	{
		iWidth -= ( windowposright - screenw );
	}
	if ( windowposbottom >= screenh )
	{
		iHeight -= ( windowposbottom - screenh );
	}

	int startx = 0, starty = 0;
	if( windowposx < 0 )
	{
		startx = -windowposx;
		iWidth -= startx;
	}
	if ( windowposy < 0 )
	{
		starty = -windowposy;
		iHeight -= starty;
	}

	if( iWidth < 0 || iHeight < 0 )
	{
		return;
	}

	if ( GetRenderingWithFlashlightConfiguration() )
	{
		RenderCapture();
	}

	int w, h;
	GetSize( w, h );
	vgui::MatSystemSurface()->Begin3DPaint( 0, 0, w, h, m_bRender3DSupersampled );

	if ( m_pCurrentManip )
	{
		m_pCurrentManip->SetViewportSize( iWidth, iHeight );
	}

	// Set up the render state for the camera + light
	m_nRenderWidth = iWidth;
	m_nRenderHeight = iHeight;
	if ( !m_bSetupRenderStateDelayed )
		SetupRenderState( iWidth, iHeight );

	CMatRenderContextPtr pRenderContext( vgui::MaterialSystem() );
	
	PIXEVENT( pRenderContext, "CPotteryWheelPanel::Paint" );

	if ( m_bUseParentBG && GetParent() )
	{
		Color bgCol = GetParent()->GetBgColor();
		pRenderContext->ClearColor4ub( bgCol.r(), bgCol.g(), bgCol.b(), bgCol.a() );
	}
	else
	{
		pRenderContext->ClearColor4ub( m_ClearColor.r(), m_ClearColor.g(), m_ClearColor.b(), m_ClearColor.a() ); 
	}
	pRenderContext->ClearBuffers( true, true );

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	if ( HasLightProbe() )
	{
		IMaterial *pMaterial = ( vgui::MaterialSystemHardwareConfig()->GetHDRType() == HDR_TYPE_NONE ) ?
			m_LightProbeBackground : m_LightProbeHDRBackground;

		RenderBox( m_Camera.m_origin, vec3_angle, Vector( -100, -100, -100 ), Vector( 100, 100, 100 ),
			Color( 255, 255, 255, 255 ), pMaterial, true );
	}

	OnPaint3D();

	pRenderContext->CullMode( MATERIAL_CULLMODE_CW );

	vgui::MatSystemSurface()->End3DPaint( );
}

void CPotteryWheelPanel::EnableRenderingWithFlashlight( void *pvConfiguration )
{
	m_pvRenderingWithFlashlightConfiguration = pvConfiguration;
}

void CPotteryWheelPanel::RenderCapture()
{
	if ( !GetRenderingWithFlashlightConfiguration() )
		return;

	CRenderCaptureConfigurationState *pCfg = reinterpret_cast< CRenderCaptureConfigurationState * >( GetRenderingWithFlashlightConfiguration() );

	m_nRenderWidth = pCfg->m_pFlashlightDepthTexture->GetActualWidth();
	m_nRenderHeight = pCfg->m_pFlashlightDepthTexture->GetActualHeight();
	if ( !m_bSetupRenderStateDelayed )
		SetupRenderState( m_nRenderWidth, m_nRenderHeight );

	CMatRenderContextPtr pRenderContext( vgui::MaterialSystem() );
	PIXEVENT( pRenderContext, "CPotteryWheelPanel::RenderCapture" );

	CViewSetup view2d;
	view2d.x				= 0;
	view2d.y				= 0;
	view2d.width			= m_nRenderWidth;
	view2d.height			= m_nRenderHeight;
	pCfg->m_pIVRenderView->Push3DView( pRenderContext, view2d, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH, pCfg->m_pDummyColorBufferTexture, NULL, pCfg->m_pFlashlightDepthTexture );

	pRenderContext->ClearColor4ub( 0, 0, 0, 0 );
	pRenderContext->ClearBuffers( true, true, false );

	m_bInRender3dForRenderCapture = true;
	pRenderContext->CullMode(MATERIAL_CULLMODE_CW);
	g_pStudioRender->ForcedMaterialOverride( NULL, OVERRIDE_DEPTH_WRITE );

	// Don't draw the 3D scene w/ stencil
	ShaderStencilState_t state;
	pRenderContext->SetStencilState( state );

	OnPaint3D();

	g_pStudioRender->ForcedMaterialOverride( NULL );
	pRenderContext->CullMode(MATERIAL_CULLMODE_CCW);
	m_bInRender3dForRenderCapture = false;

	pCfg->m_pIVRenderView->PopView( pRenderContext, NULL );
	pRenderContext->Flush();
}


//-----------------------------------------------------------------------------
// called when we're ticked...
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::OnTick()
{
	BaseClass::OnTick();
	if ( m_pCurrentManip )
	{
		m_pCurrentManip->OnTick();
		UpdateCameraTransform();
	}
}


//-----------------------------------------------------------------------------
// input
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::OnKeyCodePressed(KeyCode code)
{
	if ( m_pCurrentManip )
	{
		switch( code )
		{
		case KEY_RSHIFT:
		case KEY_LSHIFT:
			// start translate mode
			AcceptManipulation( false );
			EnterManipulationMode( CAMERA_TRANSLATE, false );
			break;

		case KEY_RCONTROL:
		case KEY_LCONTROL:
			// start light mode
			AcceptManipulation( false );
			EnterManipulationMode( LIGHT_MODE, false );
			break;
		}
	}

	BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Set whether the parent panel should be notified of mouse actions
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::SetParentMouseNotify( bool bParentMouseNotify )
{
	m_bParentMouseNotify = bParentMouseNotify;
}

//-----------------------------------------------------------------------------
// Purpose: soaks up any remaining messages
//-----------------------------------------------------------------------------
void CPotteryWheelPanel::OnKeyCodeReleased(KeyCode code)
{
	if ( m_pCurrentManip )
	{
		switch( code )
		{
		case KEY_RSHIFT:
		case KEY_LSHIFT:
		case KEY_RCONTROL:
		case KEY_LCONTROL:
			{
				// stop manipulation mode
				AcceptManipulation( false );
				switch ( m_nCaptureMouseCode )
				{
				default:
				case MOUSE_LEFT:
					EnterManipulationMode( CAMERA_ROTATE, false );
					break;

				case MOUSE_MIDDLE:
					EnterManipulationMode( CAMERA_TRANSLATE, false );
					break;

				case MOUSE_RIGHT:
					EnterManipulationMode( CAMERA_ZOOM, false );
					break;
				}
			}
			break;
		}
	}
	BaseClass::OnKeyCodeReleased( code );
}

void CPotteryWheelPanel::OnMouseDoublePressed( vgui::MouseCode code )
{
	BaseClass::OnMouseDoublePressed( code );

	if( m_bParentMouseNotify && GetParent() )
	{
		GetParent()->OnMouseDoublePressed(code);
	}
}

void CPotteryWheelPanel::OnMousePressed( vgui::MouseCode code )
{
	if ( m_pCurrentManip )
		return;

	RequestFocus();

	if ( input()->IsKeyDown( KEY_RSHIFT ) || input()->IsKeyDown( KEY_LSHIFT ) )
	{
		EnterManipulationMode( CAMERA_TRANSLATE, true, code );
	}
	else if ( input()->IsKeyDown( KEY_RCONTROL ) || input()->IsKeyDown( KEY_LCONTROL ) )
	{
		EnterManipulationMode( LIGHT_MODE, true, code );
	}
	else
	{
		switch ( code )
		{
		case MOUSE_LEFT:
			EnterManipulationMode( CAMERA_ROTATE, true, code );
			break;

		case MOUSE_MIDDLE:
			EnterManipulationMode( CAMERA_TRANSLATE, true, code );
			break;

		case MOUSE_RIGHT:
			EnterManipulationMode( CAMERA_ZOOM, true, code );
			break;
		}
	}

	BaseClass::OnMousePressed( code );

	if( m_bParentMouseNotify && GetParent() )
	{
		GetParent()->OnMousePressed(code);
	}
}

void CPotteryWheelPanel::OnMouseReleased( vgui::MouseCode code )
{
	int x, y;
	input()->GetCursorPos( x, y );
	ScreenToLocal( x, y );

	AcceptManipulation();
	
	BaseClass::OnMouseReleased( code );

	if( m_bParentMouseNotify && GetParent() )
	{
		GetParent()->OnMouseReleased(code);
	}
}

void CPotteryWheelPanel::OnCursorMoved( int x, int y )
{
	if ( m_pCurrentManip )
	{
		if ( WarpMouse( x, y ) )
		{
			m_pCurrentManip->OnCursorMoved( x, y );
		}
	}

	BaseClass::OnCursorMoved( x, y );

	if( m_bParentMouseNotify && GetParent() )
	{
		GetParent()->OnCursorMoved(x,y);
	}
}

void CPotteryWheelPanel::OnMouseWheeled( int delta )
{
	if ( m_pCurrentManip )
	{
		m_pCurrentManip->OnMouseWheeled( delta );
	}

	BaseClass::OnMouseWheeled( delta );

	if( m_bParentMouseNotify && GetParent() )
	{
		GetParent()->OnMouseWheeled(delta);
	}
}


void CPotteryWheelPanel::EnterManipulationMode( ManipulationMode_t manipMode, bool bMouseCapture, vgui::MouseCode mouseCode /* = -1 */ )
{
	switch ( manipMode )
	{
	case CAMERA_ROTATE:
		m_pCurrentManip = m_pCameraRotate;
		break;

	case CAMERA_TRANSLATE:
		m_pCurrentManip = m_pCameraTranslate;
		break;

	case CAMERA_ZOOM:
		m_pCurrentManip = m_pCameraZoom;
		break;

	case LIGHT_MODE:
		m_pCurrentManip = m_pLightManip;
		break;
	}

	if ( !m_pCurrentManip )
		return;

	m_pCurrentManip->OnBeginManipulation();

	m_xoffset = m_yoffset = 0;

	// Warp the mouse to the center of the screen
	int width, height;
	GetSize( width, height );
	int x = width / 2;
	int y = height / 2;

	if ( bMouseCapture )
	{
		input()->GetCursorPos( m_nManipStartX, m_nManipStartY );
		EnableMouseCapture( true, mouseCode );

		int xpos = x;
		int ypos = y;
		LocalToScreen( xpos, ypos );
		input()->SetCursorPos( xpos, ypos );
	}

	m_pCurrentManip->OnMousePressed( mouseCode, x, y );
}

void CPotteryWheelPanel::AcceptManipulation( bool bReleaseMouseCapture )
{
	if ( m_pCurrentManip )
	{
		m_pCurrentManip->OnAcceptManipulation();

		if ( bReleaseMouseCapture )
		{
			EnableMouseCapture( false );
			input()->SetCursorPos( m_nManipStartX, m_nManipStartY );
		}

		m_pCurrentManip = NULL;
	}
}

void CPotteryWheelPanel::CancelManipulation()
{
	if ( m_pCurrentManip )
	{
		m_pCurrentManip->OnCancelManipulation();

		EnableMouseCapture( false );
		input()->SetCursorPos( m_nManipStartX, m_nManipStartY );

		m_pCurrentManip = NULL;
	}
	
}

void CPotteryWheelPanel::OnMouseCaptureLost()
{
	SetCursor( vgui::dc_arrow );
	m_nCaptureMouseCode = vgui::MouseCode( -1 );
}

void CPotteryWheelPanel::EnableMouseCapture( bool enable, vgui::MouseCode mouseCode /* = -1 */ )
{
	if ( enable )
	{
		m_nCaptureMouseCode = mouseCode;
		SetCursor( vgui::dc_none );
		input()->SetMouseCaptureEx( GetVPanel(), m_nCaptureMouseCode );
	}
	else
	{
		m_nCaptureMouseCode = vgui::MouseCode( -1 );
		input()->SetMouseCapture( NULL );
		SetCursor( vgui::dc_arrow );
	}
}

bool CPotteryWheelPanel::WarpMouse( int &x, int &y )
{
	// Re-force capture if it was lost...
	if ( input()->GetMouseCapture() != GetVPanel() )
	{
		input()->GetCursorPos( m_nManipStartX, m_nManipStartY );
		EnableMouseCapture( true, m_nCaptureMouseCode );
	}

	int width, height;
	GetSize( width, height );

	int centerx = width / 2;
	int centery = height / 2;

	// skip this event
	if ( x == centerx && y == centery )
		return false; 

	int xpos = centerx;
	int ypos = centery;
	LocalToScreen( xpos, ypos );
	input()->SetCursorPos( xpos, ypos );

	int dx = x - centerx;
	int dy = y - centery;

	x += m_xoffset;
	y += m_yoffset;

	m_xoffset += dx;
	m_yoffset += dy;

	return true;
}

