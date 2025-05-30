﻿//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "dme_controls/particlesystempanel.h"
#include "dme_controls/dmepanel.h"
#include "movieobjects/dmeparticlesystemdefinition.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "vguimatsurface/imatsystemsurface.h"
#include "matsys_controls/matsyscontrols.h"
#include "vgui/iVGUI.h"
#include "vgui_controls/propertypage.h"
#include "vgui_controls/propertysheet.h"
#include "vgui_controls/textentry.h"
#include "vgui_controls/splitter.h"
#include "vgui_controls/checkbutton.h"
#include "matsys_controls/colorpickerpanel.h"
#include "particles/particles.h"
#include "tier1/KeyValues.h"
#include "tier1/utlbuffer.h"
#include "tier2/renderutils.h"


using namespace vgui;

//-----------------------------------------------------------------------------
// Enums
//-----------------------------------------------------------------------------
enum 
{
	SCROLLBAR_SIZE=18,  // the width of a scrollbar
	WINDOW_BORDER_WIDTH=2 // the width of the window's border
};

#define SPHERE_RADIUS 10.0f


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CParticleSystemPanel::CParticleSystemPanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	m_pParticleSystem = NULL;
	m_flLastTime = FLT_MAX;
	m_bRenderBounds = false;
	m_bRenderCullBounds = false;
	m_bRenderHelpers = false;
	m_bRenderControlPoints = false;
	m_bPerformNameBasedLookup = true;
	m_bTickMyself = true;
	m_bAutoView = false;
	m_bSuppressAutoView = false;
	m_ParticleSystemName = NULL;
	InvalidateUniqueId( &m_ParticleSystemId );
	InvalidateUniqueId( &m_RenderHelperId );

	LookAt( SPHERE_RADIUS );

	m_pLightmapTexture.Init( "//platform/materials/debug/defaultlightmap", "editor" );
	m_DefaultEnvCubemap.Init( "editor/cubemap", "editor", true );
	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		SetControlPointValue( i, vec3_invalid );
	}
}

CParticleSystemPanel::~CParticleSystemPanel()
{
	m_pLightmapTexture.Shutdown();
	m_DefaultEnvCubemap.Shutdown();

	delete m_pParticleSystem;
	m_pParticleSystem = NULL;
}


void CParticleSystemPanel::ResetView()
{
	BaseClass::ResetView();
	LookAt(SPHERE_RADIUS);

	m_BestViewBoundsMin = vec3_origin;
	m_BestViewBoundsMax = vec3_origin;
	m_bSuppressAutoView = false;
}

//-----------------------------------------------------------------------------
// Scheme
//-----------------------------------------------------------------------------
void CParticleSystemPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBorder( pScheme->GetBorder( "MenuBorder") );
}

//-----------------------------------------------------------------------------
// Stop System
//-----------------------------------------------------------------------------

void CParticleSystemPanel::StopEffect()
{
	if ( !m_pParticleSystem )
		return;
	m_pParticleSystem->StopEmission(false, false, false, true );
}

//-----------------------------------------------------------------------------
// Indicates that the grid should be drawn
//-----------------------------------------------------------------------------
void CParticleSystemPanel::RenderGrid( bool bEnable )
{
	m_bRenderGrid = bEnable;
}


//-----------------------------------------------------------------------------
// Indicates that bounds should be drawn
//-----------------------------------------------------------------------------
void CParticleSystemPanel::RenderBounds( bool bEnable )
{
	m_bRenderBounds = bEnable;
}


//-----------------------------------------------------------------------------
// Indicates that control points should be drawn
//-----------------------------------------------------------------------------
void CParticleSystemPanel::RenderControlPoints( bool bEnable )
{
	m_bRenderControlPoints = bEnable;
}


//-----------------------------------------------------------------------------
// Indicates that cull sphere should be drawn
//-----------------------------------------------------------------------------
void CParticleSystemPanel::RenderCullBounds( bool bEnable )
{
	m_bRenderCullBounds = bEnable;
}


//-----------------------------------------------------------------------------
// Indicates that bounds should be drawn
//-----------------------------------------------------------------------------
void CParticleSystemPanel::RenderHelpers( bool bEnable )
{
	m_bRenderHelpers = bEnable;
}


//-----------------------------------------------------------------------------
// Indicates which helper to draw
//-----------------------------------------------------------------------------
void CParticleSystemPanel::SetRenderedHelper( CDmeParticleFunction *pOp )
{
	if ( !pOp )
	{
		InvalidateUniqueId( &m_RenderHelperId );
	}
	else
	{
		CopyUniqueId( pOp->GetId(), &m_RenderHelperId );
	}
}

//-----------------------------------------------------------------------------
// Simulate the particle system
//-----------------------------------------------------------------------------
void CParticleSystemPanel::OnTick()
{
	BaseClass::OnTick();

	if( m_bTickMyself )
	{
		Simulate();
	}
}

void CParticleSystemPanel::Simulate()
{
	if ( !m_pParticleSystem )
		return;

	float flTime = Plat_FloatTime();
	if ( m_flLastTime == FLT_MAX )
	{
		m_flLastTime = flTime;
	}

	float flDt = flTime - m_flLastTime;
	m_flLastTime = flTime;

	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		if ( !m_pParticleSystem->ReadsControlPoint( i ) || m_pControlPointValue[i] == vec3_invalid )
			continue;

		m_pParticleSystem->SetControlPoint( i, m_pControlPointValue[i] );
		m_pParticleSystem->SetControlPointOrientation( i, Vector( 1, 0, 0 ), Vector( 0, -1, 0 ), Vector( 0, 0, 1 ) );
		m_pParticleSystem->SetControlPointParent( i, i );
	}

	// Restart the particle system if it's finished
	bool bIsInvalid = !m_pParticleSystem->IsFullyValid();
	if ( m_pParticleSystem->IsFinished() || bIsInvalid )
	{
		delete m_pParticleSystem;
		m_pParticleSystem = NULL;

		if ( m_bPerformNameBasedLookup )
		{
			if ( m_ParticleSystemName.Length() )
			{
				CParticleCollection *pNewParticleSystem = g_pParticleSystemMgr->CreateParticleCollection( m_ParticleSystemName );
				m_pParticleSystem = pNewParticleSystem;
			}
		}
		else
		{
			if ( IsUniqueIdValid( m_ParticleSystemId ) )
			{
				CParticleCollection *pNewParticleSystem = g_pParticleSystemMgr->CreateParticleCollection( m_ParticleSystemId );
				m_pParticleSystem = pNewParticleSystem;
			}
		}

		if ( bIsInvalid )
		{
			PostActionSignal( new KeyValues( "ParticleSystemReconstructed" ) );
		}
		m_flLastTime = FLT_MAX;
	}
	else
	{
		m_pParticleSystem->Simulate( flDt );
	}

	if( m_bAutoView && !m_bSuppressAutoView )
	{
		UseAutoView();
	}
}


//-----------------------------------------------------------------------------
// Startup, shutdown particle collection
//-----------------------------------------------------------------------------
void CParticleSystemPanel::StartupParticleCollection()
{
	if ( m_pParticleSystem && m_bTickMyself )
	{
		vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );
	}
	m_flLastTime = FLT_MAX;
}

void CParticleSystemPanel::ShutdownParticleCollection()
{
	if ( m_pParticleSystem )
	{
		if( m_bTickMyself )
		{
			vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		}

		delete m_pParticleSystem;
		m_pParticleSystem = NULL;
	}
}

void CParticleSystemPanel::SetSelfSimulation(bool bSelfSimulate )
{
	if( m_pParticleSystem && ( m_bTickMyself != bSelfSimulate ) )
	{
		if ( !bSelfSimulate )
		{
			vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		}
		else
		{
			vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );
		}
	}

	m_bTickMyself = bSelfSimulate;
}

//-----------------------------------------------------------------------------
// Set the particle system to draw
//-----------------------------------------------------------------------------
void CParticleSystemPanel::SetParticleSystem( CDmeParticleSystemDefinition *pDef )
{
	ShutdownParticleCollection();
	if ( pDef )
	{
		m_bPerformNameBasedLookup = pDef->UseNameBasedLookup();
		if ( m_bPerformNameBasedLookup )
		{
			m_ParticleSystemName = pDef->GetName();
			Assert( g_pParticleSystemMgr->IsParticleSystemDefined( m_ParticleSystemName ) );
			m_pParticleSystem = g_pParticleSystemMgr->CreateParticleCollection( m_ParticleSystemName );
		}
		else
		{
			CopyUniqueId( pDef->GetId(), &m_ParticleSystemId );
			Assert( g_pParticleSystemMgr->IsParticleSystemDefined( m_ParticleSystemId ) );
			m_pParticleSystem = g_pParticleSystemMgr->CreateParticleCollection( m_ParticleSystemId );
		}

		PostActionSignal( new KeyValues( "ParticleSystemReconstructed" ) );
	}
	StartupParticleCollection();
}

void CParticleSystemPanel::SetParticleSystem( const char* szParticleSystemName )
{
	ShutdownParticleCollection();
	if ( szParticleSystemName )
	{
		m_ParticleSystemName = szParticleSystemName;

		if ( g_pParticleSystemMgr->IsParticleSystemDefined( m_ParticleSystemName ) )
		{
			m_pParticleSystem = g_pParticleSystemMgr->CreateParticleCollection( m_ParticleSystemName );
		}
		else
		{
			m_pParticleSystem = NULL;
		}

		PostActionSignal( new KeyValues( "ParticleSystemReconstructed" ) );
	}
	StartupParticleCollection();
}

void CParticleSystemPanel::SetDmeElement( CDmeParticleSystemDefinition *pDef )
{
	SetParticleSystem( pDef );
}

CParticleCollection *CParticleSystemPanel::GetParticleSystem()
{
	return m_pParticleSystem;
}

//-----------------------------------------------------------------------------
// Auto-viewing
//-----------------------------------------------------------------------------

void CParticleSystemPanel::EnterManipulationMode( ManipulationMode_t manipMode, bool bMouseCapture, vgui::MouseCode mouseCode )
{
	BaseClass::EnterManipulationMode(manipMode,bMouseCapture,mouseCode);

	if( manipMode != CAMERA_ROTATE )
	{
		m_bSuppressAutoView = true;
	}
}

void CParticleSystemPanel::EnableAutoViewing( bool bEnable )
{
	m_bAutoView = bEnable;
	m_BestViewBoundsMin = vec3_origin;
	m_BestViewBoundsMax = vec3_origin;

	if ( m_bAutoView )
	{
		m_bSuppressAutoView = false;
	}
}

void CParticleSystemPanel::UseAutoView()
{
	Vector vecMins, vecMaxs;
	m_pParticleSystem->GetBounds( &vecMins, &vecMaxs );
	m_BestViewBoundsMin = m_BestViewBoundsMin.Min(vecMins);
	m_BestViewBoundsMax = m_BestViewBoundsMax.Max(vecMaxs);

	float flBestViewRad = MAX( 5.0f, 0.25f*(m_BestViewBoundsMax-m_BestViewBoundsMin).Length() );

	Vector viewCenter = ( m_BestViewBoundsMin + m_BestViewBoundsMax ) * 0.5f;

	LookAt( viewCenter, flBestViewRad );
}

//-----------------------------------------------------------------------------
// Draw bounds
//-----------------------------------------------------------------------------
void CParticleSystemPanel::DrawBounds()
{
	Vector vecMins, vecMaxs;
	m_pParticleSystem->GetBounds( &vecMins, &vecMaxs );
	RenderWireframeBox( vec3_origin, vec3_angle, vecMins, vecMaxs, Color( 0, 255, 255, 255 ), true );
}


//-----------------------------------------------------------------------------
// Draw cull bounds
//-----------------------------------------------------------------------------
void CParticleSystemPanel::DrawCullBounds()
{
	Vector vecCenter;
	m_pParticleSystem->GetControlPointAtTime( m_pParticleSystem->m_pDef->GetCullControlPoint(), m_pParticleSystem->m_flCurTime, &vecCenter );
	RenderWireframeSphere( vecCenter, m_pParticleSystem->m_pDef->GetCullRadius(), 32, 16, Color( 0, 255, 255, 255 ), true );
}


//-----------------------------------------------------------------------------
// paint it!
//-----------------------------------------------------------------------------
#define AXIS_SIZE 5.0f

void CParticleSystemPanel::OnPaint3D()
{
	if ( !m_pParticleSystem )
		return;

	CMatRenderContextPtr pRenderContext( MaterialSystem() );
	pRenderContext->BindLightmapTexture( m_pLightmapTexture );
	pRenderContext->BindLocalCubemap( m_DefaultEnvCubemap );
	 
	// Draw axes
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity( );

	if ( m_bRenderGrid )
	{
		DrawGrid();
	}

	if ( m_bRenderBounds )
	{
		DrawBounds();
	}

	if ( m_bRenderCullBounds )
	{
		DrawCullBounds();
	}

	if ( m_bRenderHelpers && IsUniqueIdValid( m_RenderHelperId ) )
	{
		m_pParticleSystem->VisualizeOperator( &m_RenderHelperId );
	}

	Vector4D vecDiffuseModulation( 1.0f, 1.0f, 1.0f, 1.0f );
	m_pParticleSystem->Render( 0, pRenderContext, vecDiffuseModulation );
	m_pParticleSystem->VisualizeOperator( );
	RenderAxes( vec3_origin, AXIS_SIZE, true );

	if ( m_bRenderControlPoints )
	{
		for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
		{
			if ( m_pParticleSystem->ReadsControlPoint( i ) )
			{
				Vector vP;
				m_pParticleSystem->GetControlPointAtTime( i, m_pParticleSystem->m_flCurTime, &vP );
				RenderAxes( vP, 3.0f, true );
			}
		}
	}

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PopMatrix();
}
	    

//-----------------------------------------------------------------------------
//
// Control point page
//
//-----------------------------------------------------------------------------
class CControlPointPage : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CControlPointPage, vgui::PropertyPage );

public:
	// constructor, destructor
	CControlPointPage( vgui::Panel *pParent, const char *pName, CParticleSystemPanel *pParticleSystemPanel );

	virtual void PerformLayout();

	void CreateControlPointControls( );

private:
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", params );
	MESSAGE_FUNC_PARAMS( OnNewLine, "TextNewLine", params );

	void LayoutControlPointControls();
	void CleanUpControlPointControls();

	vgui::Label *m_pControlPointName[MAX_PARTICLE_CONTROL_POINTS];
	vgui::TextEntry *m_pControlPointValue[MAX_PARTICLE_CONTROL_POINTS];
	CParticleSystemPanel *m_pParticleSystemPanel;
};

	
//-----------------------------------------------------------------------------
// Contstructor
//-----------------------------------------------------------------------------
CControlPointPage::CControlPointPage( vgui::Panel *pParent, const char *pName, CParticleSystemPanel *pParticleSystemPanel ) :
	BaseClass( pParent, pName )
{
	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		m_pControlPointName[i] = NULL;
		m_pControlPointValue[i] = NULL;
	}

	m_pParticleSystemPanel = pParticleSystemPanel;
}


//-----------------------------------------------------------------------------
// Called when the text entry for a control point is changed
//-----------------------------------------------------------------------------
void CControlPointPage::OnTextChanged( KeyValues *pParams )
{
	vgui::Panel *pPanel = (vgui::Panel *)pParams->GetPtr( "panel" );
	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		if ( pPanel != m_pControlPointValue[i] )
			continue;

		char pBuf[512];
		m_pControlPointValue[i]->GetText( pBuf, sizeof(pBuf) );

		Vector vecValue( 0, 0, 0 );
		sscanf( pBuf, "%f %f %f", &vecValue.x, &vecValue.y, &vecValue.z );
		m_pParticleSystemPanel->SetControlPointValue( i, vecValue );
		break;
	}
}


//-----------------------------------------------------------------------------
// Called when the text entry for a control point is changed
//-----------------------------------------------------------------------------
void CControlPointPage::OnNewLine( KeyValues *pParams )
{
	vgui::Panel *pPanel = (vgui::Panel *)pParams->GetPtr( "panel" );
	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		if ( pPanel != m_pControlPointValue[i] )
			continue;

		char pBuf[512];
		m_pControlPointValue[i]->GetText( pBuf, sizeof(pBuf) );

		Vector vecValue( 0, 0, 0 );
		sscanf( pBuf, "%f %f %f", &vecValue.x, &vecValue.y, &vecValue.z );
		m_pParticleSystemPanel->SetControlPointValue( i, vecValue );

		vecValue = m_pParticleSystemPanel->GetControlPointValue( i );
		Q_snprintf( pBuf, sizeof(pBuf), "%.3f %.3f %.3f", vecValue.x, vecValue.y, vecValue.z );
		m_pControlPointValue[i]->SetText( pBuf );
		break;
	}
}


//-----------------------------------------------------------------------------
// Called when the particle system changes
//-----------------------------------------------------------------------------
void CControlPointPage::PerformLayout()
{
	BaseClass::PerformLayout();
	LayoutControlPointControls();
}


//-----------------------------------------------------------------------------
// Creates controls used to modify control point values
//-----------------------------------------------------------------------------
void CControlPointPage::CreateControlPointControls()
{
	CleanUpControlPointControls();
	CParticleCollection* pParticleSystem = m_pParticleSystemPanel->GetParticleSystem();
	if ( !pParticleSystem )
		return;

	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		if ( !pParticleSystem->ReadsControlPoint( i ) )
			continue;

		char pName[512];
		Q_snprintf( pName, sizeof(pName), "Pt #%d:", i );
		m_pControlPointName[i] = new Label( this, pName, pName );

		Q_snprintf( pName, sizeof(pName), "Entry #%d:", i );
		m_pControlPointValue[i] = new TextEntry( this, pName );
		m_pControlPointValue[i]->AddActionSignalTarget( this );
		m_pControlPointValue[i]->SendNewLine( true );
		m_pControlPointValue[i]->SetMultiline( false );

		Vector vecValue = m_pParticleSystemPanel->GetControlPointValue( i );
		if ( vecValue == vec3_invalid )
			vecValue = vec3_origin;
		Q_snprintf( pName, sizeof(pName), "%.3f %.3f %.3f", vecValue.x, vecValue.y, vecValue.z );
		m_pControlPointValue[i]->SetText( pName );
	}

	LayoutControlPointControls();
}


//-----------------------------------------------------------------------------
// Lays out the controls
//-----------------------------------------------------------------------------
void CControlPointPage::LayoutControlPointControls()
{
	int nFoundControlCount = 0;
	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		if ( !m_pControlPointName[i] )
			continue;

		int yVal = 8 + nFoundControlCount * 28;
		m_pControlPointName[i]->SetBounds( 8, yVal, 48, 24 );
		m_pControlPointValue[i]->SetBounds( 64, yVal, 160, 24 );
		++nFoundControlCount;
	}
}


//-----------------------------------------------------------------------------
// Cleans up controls used to modify control point values
//-----------------------------------------------------------------------------
void CControlPointPage::CleanUpControlPointControls( )
{
	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		if ( m_pControlPointName[i] )
		{
			delete m_pControlPointName[i];
			m_pControlPointName[i] = NULL;
		}

		if ( m_pControlPointValue[i] )
		{
			delete m_pControlPointValue[i];
			m_pControlPointValue[i] = NULL;
		}
	}
}


//-----------------------------------------------------------------------------
//
// CParticleSystemPreviewPanel
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Dme panel connection
//-----------------------------------------------------------------------------
IMPLEMENT_DMEPANEL_FACTORY( CParticleSystemPreviewPanel, DmeParticleSystemDefinition, "DmeParticleSystemDefinitionViewer", "Particle System Viewer", false );


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CParticleSystemPreviewPanel::CParticleSystemPreviewPanel( vgui::Panel *pParent, const char *pName ) :
	BaseClass( pParent, pName )
{
	m_pUnlockSystem = NULL;

	m_Splitter = new vgui::Splitter( this, "Splitter", SPLITTER_MODE_VERTICAL, 1 );
	vgui::Panel *pSplitterLeftSide = m_Splitter->GetChild( 0 );
	vgui::Panel *pSplitterRightSide = m_Splitter->GetChild( 1 );

	m_pParticleSystemPanel = new CParticleSystemPanel( pSplitterRightSide, "ParticlePreview" );
	m_pParticleSystemPanel->AddActionSignalTarget( this );
	m_pParticleSystemPanel->SetBackgroundColor( 0, 0, 0 );

	m_pParticleCount = new vgui::Label( pSplitterRightSide, "ParticleCountLabel", "" );
	m_pParticleCount->SetZPos( 1 );

	m_pControlSheet = new vgui::PropertySheet( pSplitterLeftSide, "ControlSheet" );

	m_pRenderPage = new vgui::PropertyPage( m_pControlSheet, "RenderPage" );

	m_pRenderBounds = new vgui::CheckButton( m_pRenderPage, "RenderBounds", "Render Bounding Box" );
	m_pRenderBounds->AddActionSignalTarget( this );

	m_pRenderControlPoints = new vgui::CheckButton( m_pRenderPage, "RenderControlPoints", "Render Control Points" );
	m_pRenderControlPoints->AddActionSignalTarget( this );

	m_pRenderCullBounds = new vgui::CheckButton( m_pRenderPage, "RenderCullBounds", "Render Culling Bounds" );
	m_pRenderCullBounds->AddActionSignalTarget( this );

	m_pRenderHelpers = new vgui::CheckButton( m_pRenderPage, "RenderHelpers", "Render Helpers" );
	m_pRenderHelpers->AddActionSignalTarget( this );

	m_pRenderGrid = new vgui::CheckButton( m_pRenderPage, "RenderGrid", "Render Grid" );
	m_pRenderGrid->AddActionSignalTarget( this );

	m_pBackgroundColor = new CColorPickerButton( m_pRenderPage, "BackgroundColor", this );
	m_pBackgroundColor->SetColor( m_pParticleSystemPanel->GetBackgroundColor() );

	m_pLockPreview = new vgui::CheckButton( m_pRenderPage, "LockPreview", "Lock Preview System" );
	m_pLockPreview->AddActionSignalTarget( this );

	m_pStopEffect = new vgui::Button( m_pRenderPage, "StopEffect", "Stop Effect", this, "StopEffect" );
	

	m_pRenderPage->LoadControlSettingsAndUserConfig( "resource/particlesystempreviewpanel_renderpage.res" );

	m_pControlPointPage = new CControlPointPage( m_pControlSheet, "ControlPointPage", m_pParticleSystemPanel );

	// Load layout settings; has to happen before pinning occurs in code
	LoadControlSettingsAndUserConfig( "resource/particlesystempreviewpanel.res" );

	// NOTE: Page adding happens *after* LoadControlSettingsAndUserConfig
	// because the layout of the sheet is correct at this point.
	m_pControlSheet->AddPage( m_pRenderPage, "Render" );
	m_pControlSheet->AddPage( m_pControlPointPage, "Ctrl Pts" );
}

CParticleSystemPreviewPanel::~CParticleSystemPreviewPanel()
{
}


//-----------------------------------------------------------------------------
// Set the particle system to draw
//-----------------------------------------------------------------------------
void CParticleSystemPreviewPanel::OnThink()
{
	BaseClass::OnThink();
	CParticleCollection* pParticleSystem = m_pParticleSystemPanel->GetParticleSystem();
	if ( !pParticleSystem )
	{
		m_pParticleCount->SetText( "" );
	}
	else
	{
		char buf[256];
		Q_snprintf( buf, sizeof(buf), "Particle Count: %5d/%5d", 
			pParticleSystem->m_nActiveParticles, pParticleSystem->m_nAllocatedParticles );
		m_pParticleCount->SetText( buf );
	}
}


//-----------------------------------------------------------------------------
// Called when the particle system changes
//-----------------------------------------------------------------------------
void CParticleSystemPreviewPanel::OnParticleSystemReconstructed()
{
	m_pControlPointPage->CreateControlPointControls();
}


//-----------------------------------------------------------------------------
// Set the particle system to draw
//-----------------------------------------------------------------------------
void CParticleSystemPreviewPanel::SetParticleSystem( CDmeParticleSystemDefinition *pDef, bool bOverrideLock )
{
	bool bLocked = m_pLockPreview->IsSelected();

	if ( bOverrideLock || !bLocked )
	{
		m_pParticleSystemPanel->SetParticleSystem( pDef );

		if ( bOverrideLock && bLocked )
		{
			m_pLockPreview->SetSelected(false);
			m_pUnlockSystem = NULL;
		}
		else
		{
			m_pUnlockSystem = pDef;
		}
	}
	else
	{
		m_pUnlockSystem = pDef;
	}
}

void CParticleSystemPreviewPanel::ClearParticleSystemLock()
{
	m_pLockPreview->SetSelected(false);
	m_pUnlockSystem = NULL;
}


void CParticleSystemPreviewPanel::SetDmeElement( CDmeParticleSystemDefinition *pDef )
{
	SetParticleSystem( pDef, false );
}


//-----------------------------------------------------------------------------
// Indicates which helper to draw
//-----------------------------------------------------------------------------
void CParticleSystemPreviewPanel::SetParticleFunction( CDmeParticleFunction *pFunction )
{
	m_pParticleSystemPanel->SetRenderedHelper( pFunction );
}


//-----------------------------------------------------------------------------
// Called when the check button is checked
//-----------------------------------------------------------------------------
void CParticleSystemPreviewPanel::OnCheckButtonChecked( KeyValues *pParams )
{
	bool bState = pParams->GetBool( "state", false );
	vgui::Panel *pPanel = (vgui::Panel*)pParams->GetPtr( "panel" );

	if ( pPanel == m_pRenderGrid )
	{
		m_pParticleSystemPanel->RenderGrid( bState );
		return;
	}
	if ( pPanel == m_pLockPreview )
	{
		if ( !m_pLockPreview->IsSelected() )
		{
			// unchecked the lock - switch to the latest requested selection
			m_pParticleSystemPanel->SetParticleSystem( m_pUnlockSystem );
		}
		return;
	}
	if ( pPanel == m_pRenderBounds )
	{
		m_pParticleSystemPanel->RenderBounds( bState );
		return;
	}
	if ( pPanel == m_pRenderCullBounds )
	{
		m_pParticleSystemPanel->RenderCullBounds( bState );
		return;
	}
	if ( pPanel == m_pRenderHelpers )
	{
		m_pParticleSystemPanel->RenderHelpers( bState );
		return;
	}
	if ( pPanel == m_pRenderControlPoints )
	{
		m_pParticleSystemPanel->RenderControlPoints( bState );
		return;
	}
}


//-----------------------------------------------------------------------------
// Called when a new background color is picked
//-----------------------------------------------------------------------------
void CParticleSystemPreviewPanel::OnBackgroundColorChanged( KeyValues *pParams )
{
	m_pParticleSystemPanel->SetBackgroundColor( pParams->GetColor( "color" ) );
}

void CParticleSystemPreviewPanel::OnBackgroundColorPreview( KeyValues *pParams )
{
	m_pParticleSystemPanel->SetBackgroundColor( pParams->GetColor( "color" ) );
}

void CParticleSystemPreviewPanel::OnBackgroundColorCancel( KeyValues *pParams )
{
	m_pParticleSystemPanel->SetBackgroundColor( pParams->GetColor( "startingColor" ) );
}


//-----------------------------------------------------------------------------
// Called when buttons are clicked
//-----------------------------------------------------------------------------
void CParticleSystemPreviewPanel::OnCommand( const char *pCommand )
{

	if ( !Q_stricmp( pCommand, "StopEffect" ) )
	{
		m_pParticleSystemPanel->StopEffect();
		return;
	}

	BaseClass::OnCommand( pCommand );
}