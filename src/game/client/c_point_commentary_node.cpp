﻿//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "c_baseentity.h"
#include "hud.h"
#include "hudelement.h"
#include "clientmode.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui_controls/AnimationController.h>
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "soundenvelope.h"
#include "convar.h"
#include "hud_closecaption.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define MAX_SPEAKER_NAME	256
#define MAX_COUNT_STRING	64


extern ConVar english;
extern ConVar closecaption;
class C_PointCommentaryNode;


CUtlVector< CHandle<C_PointCommentaryNode> >	g_CommentaryNodes;
bool IsInCommentaryMode( void )
{
	return (g_CommentaryNodes.Count() > 0);
}


static bool g_bTracingVsCommentaryNodes = false;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudCommentary : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudCommentary, vgui::Panel );
public:
	CHudCommentary( const char *name );

	virtual void Init( void );
	virtual void VidInit( void );
	virtual void LevelInit( void ) { g_CommentaryNodes.Purge(); }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void StartCommentary( C_PointCommentaryNode *pNode, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime );
	void StopCommentary( void );
	bool IsTheActiveNode( C_PointCommentaryNode *pNode ) { return (pNode == m_hActiveNode); }
	bool HasActiveNode( void ) { return m_hActiveNode.Get() != NULL; }

	// vgui overrides
	virtual void Paint( void );
	virtual bool ShouldDraw( void );

private:
	CHandle<C_PointCommentaryNode> m_hActiveNode;
	bool	m_bShouldPaint;
	float	m_flStartTime;
	float	m_flEndTime;
	wchar_t	m_szSpeakers[MAX_SPEAKER_NAME];
	wchar_t	m_szCount[MAX_COUNT_STRING];
	CMaterialReference m_matIcon;
	bool	m_bHiding;
	bool	m_bSoundStarted;

	// Painting
	CPanelAnimationVarAliasType( int, m_iBarX, "bar_xpos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBarY, "bar_ypos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBarTall, "bar_height", "16", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBarWide, "bar_width", "16", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iSpeakersX, "speaker_xpos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iSpeakersY, "speaker_ypos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iCountXFR, "count_xpos_from_right", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iCountY, "count_ypos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iIconX, "icon_xpos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iIconY, "icon_ypos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iIconWide, "icon_width", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iIconTall, "icon_height", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nIconTextureId, "icon_texture", "vgui/hud/icon_commentary", "textureid" );

	CPanelAnimationVar( bool, m_bUseScriptBGColor, "use_script_bgcolor", "0" );
	CPanelAnimationVar( Color, m_BackgroundColor, "BackgroundColor", "0 0 0 0" );
	CPanelAnimationVar( Color, m_BGOverrideColor, "BackgroundOverrideColor", "Panel.BgColor" );
};


bool IsListeningToCommentary( void )
{
	CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
	if ( !pHudCommentary )
	{
		return false;
	}

	return pHudCommentary->HasActiveNode();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PointCommentaryNode : public C_BaseAnimating
{
	DECLARE_CLASS( C_PointCommentaryNode, C_BaseAnimating );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

	virtual void OnPreDataChanged( DataUpdateType_t type );
	virtual void OnDataChanged( DataUpdateType_t type );

	void OnRestore( void )
	{
		BaseClass::OnRestore();

		if ( m_bActive )
		{
			StopLoopingSounds();
			m_bRestartAfterRestore = true;
		}

		AddAndLockCommentaryHudGroup();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	virtual void SetDormant( bool bDormant )
	{
		if ( !IsDormant() && bDormant )
		{
			RemoveAndUnlockCommentaryHudGroup();
		}

		BaseClass::SetDormant( bDormant );
	}

	//-----------------------------------------------------------------------------
	// Cleanup
	//-----------------------------------------------------------------------------
	void UpdateOnRemove( void )
	{
		RemoveAndUnlockCommentaryHudGroup();

		StopLoopingSounds();
		BaseClass::UpdateOnRemove();
	}

	void	StopLoopingSounds( void );

	virtual bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

	void AddAndLockCommentaryHudGroup( void )
	{
		if ( !g_CommentaryNodes.Count() )
		{
			HACK_GETLOCALPLAYER_GUARD( "C_PointCommentaryNode::AddAndLockCommentaryHudGroup" );
			int iRenderGroup = GetHud().RegisterForRenderGroup( "commentary" );
			GetHud().LockRenderGroup( iRenderGroup );
		}

		if ( g_CommentaryNodes.Find( this ) == g_CommentaryNodes.InvalidIndex() )
		{
			g_CommentaryNodes.AddToTail( this );
		}
	}

	void RemoveAndUnlockCommentaryHudGroup( void )
	{
		g_CommentaryNodes.FindAndRemove( this );

		if ( !g_CommentaryNodes.Count() )
		{
			HACK_GETLOCALPLAYER_GUARD( "C_PointCommentaryNode::RemoveAndUnlockCommentaryHudGroup" );
			int iRenderGroup = GetHud().RegisterForRenderGroup( "commentary" );
			GetHud().UnlockRenderGroup( iRenderGroup );
		}
	}

	CSoundPatch *GetSoundPatch() const
	{
		return m_sndCommentary;
	}

public:
	// Data received from the server
	bool		m_bActive;
	bool		m_bWasActive;
	float		m_flStartTime;
	char		m_iszCommentaryFile[MAX_PATH];
	char		m_iszCommentaryFileNoHDR[MAX_PATH];
	char		m_iszSpeakers[MAX_SPEAKER_NAME];
	int			m_iNodeNumber;
	int			m_iNodeNumberMax;
	CSoundPatch *m_sndCommentary;
	EHANDLE		m_hViewPosition;
	bool		m_bRestartAfterRestore;
};

IMPLEMENT_CLIENTCLASS_DT(C_PointCommentaryNode, DT_PointCommentaryNode, CPointCommentaryNode)
	RecvPropBool( RECVINFO( m_bActive ) ),
	RecvPropTime( RECVINFO( m_flStartTime ) ),
	RecvPropString( RECVINFO(m_iszCommentaryFile) ),
	RecvPropString( RECVINFO(m_iszCommentaryFileNoHDR) ),
	RecvPropString( RECVINFO(m_iszSpeakers) ),
	RecvPropInt( RECVINFO( m_iNodeNumber ) ),
	RecvPropInt( RECVINFO( m_iNodeNumberMax ) ),
	RecvPropEHandle( RECVINFO(m_hViewPosition) ),
END_RECV_TABLE()

BEGIN_DATADESC( C_PointCommentaryNode )
	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWasActive, FIELD_BOOLEAN ),
	DEFINE_SOUNDPATCH( m_sndCommentary ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bWasActive = m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		AddAndLockCommentaryHudGroup();
	}

	if ( m_bWasActive == m_bActive && !m_bRestartAfterRestore )
		return;

	HACK_GETLOCALPLAYER_GUARD( "C_PointCommentaryNode::OnDataChanged" );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( m_bActive && pPlayer )
	{
		// Use the HDR / Non-HDR version based on whether we're running HDR or not
		char *pszCommentaryFile;
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_NONE && m_iszCommentaryFileNoHDR && m_iszCommentaryFileNoHDR[0] )
		{
			pszCommentaryFile = m_iszCommentaryFileNoHDR;
		}
		else
		{
			pszCommentaryFile = m_iszCommentaryFile;
		}
		if ( !pszCommentaryFile || !pszCommentaryFile[0] )
		{
			engine->ServerCmd( "commentary_finishnode\n" );
			return;
		}

		EmitSound_t es;
		es.m_nChannel = CHAN_STATIC;
		es.m_pSoundName = pszCommentaryFile;
 		es.m_SoundLevel = SNDLVL_GUNFIRE;
		es.m_nFlags = SND_SHOULDPAUSE;

		CBaseEntity *pSoundEntity;
		if ( m_hViewPosition )
		{
			pSoundEntity = m_hViewPosition;
		}
		else if ( render->GetViewEntity() )
		{
			pSoundEntity = cl_entitylist->GetEnt( render->GetViewEntity() );
			es.m_SoundLevel = SNDLVL_NONE;
		}
		else
		{
			pSoundEntity = pPlayer;
		}
		CSingleUserRecipientFilter filter( pPlayer );
		m_sndCommentary = (CSoundEnvelopeController::GetController()).SoundCreate( filter, pSoundEntity->entindex(), es );
		if ( m_sndCommentary )
		{
			(CSoundEnvelopeController::GetController()).SoundSetCloseCaptionDuration( m_sndCommentary, -1 );
			(CSoundEnvelopeController::GetController()).Play( m_sndCommentary, 1.0f, 100, m_flStartTime );
		}

		// Strip the #off of the commentary file path if there is one
		char *pcCommentaryWAVPath = V_stristr( (char*)STRING( CSoundEnvelopeController::GetController().SoundGetName( m_sndCommentary ) ) , "commentary" );

		// Get the duration so we know when it finishes
		float flDuration = enginesound->GetSoundDuration( pcCommentaryWAVPath ) ;

		CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_FULLSCREEN_HUDELEMENT( CHudCloseCaption );
		if ( pHudCloseCaption )
		{
			// This is where we play the commentary close caption (and lock the other captions out).
			// Also, if close captions are off we force a caption in non-English
			if ( closecaption.GetBool() || ( !closecaption.GetBool() && !english.GetBool() ) )
			{
				// Clear the close caption element in preparation
				pHudCloseCaption->Reset();

				// Process the commentary caption
				pHudCloseCaption->ProcessCaptionDirect( pszCommentaryFile, flDuration );

				// Find the close caption hud element & lock it
				pHudCloseCaption->Lock();
			}
		}

		// Tell the HUD element
		CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
		pHudCommentary->StartCommentary( this, m_iszSpeakers, m_iNodeNumber, m_iNodeNumberMax, m_flStartTime, m_flStartTime + flDuration );
	}
	else if ( m_bWasActive )
	{
		StopLoopingSounds();

 		CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
		if ( pHudCommentary->IsTheActiveNode(this) )
		{
			pHudCommentary->StopCommentary();
		}
	}

	m_bRestartAfterRestore = false;
}

//-----------------------------------------------------------------------------
// Purpose: Shut down the commentary
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::StopLoopingSounds( void )
{
	if ( m_sndCommentary != NULL )
	{
		(CSoundEnvelopeController::GetController()).SoundDestroy( m_sndCommentary );
		m_sndCommentary = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: No client side trace collisions
//-----------------------------------------------------------------------------
bool C_PointCommentaryNode::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	if ( !g_bTracingVsCommentaryNodes )
		return false;

	return BaseClass::TestCollision( ray, mask, trace );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool IsNodeUnderCrosshair( C_BasePlayer *pPlayer )
{
	// See if the player's looking at a commentary node
	trace_t tr;
	Vector vecSrc = pPlayer->EyePosition();
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );

	g_bTracingVsCommentaryNodes = true;
	UTIL_TraceLine( vecSrc, vecSrc + vecForward * MAX_TRACE_LENGTH, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	g_bTracingVsCommentaryNodes = false;

	if ( !tr.m_pEnt )
		return false;

	return dynamic_cast<C_PointCommentaryNode*>(tr.m_pEnt) ? true : false;
}

//===================================================================================================================
// COMMENTARY HUD ELEMENT
//===================================================================================================================
DECLARE_HUDELEMENT( CHudCommentary );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudCommentary::CHudCommentary( const char *name ) : vgui::Panel( NULL, "HudCommentary" ), CHudElement( name )
{
	vgui::Panel *pParent = GetClientMode()->GetViewport();
	SetParent( pParent );

	SetPaintBorderEnabled( false );
	SetHiddenBits( HIDEHUD_PLAYERDEAD );

	m_hActiveNode = NULL;
	m_bShouldPaint = true;
	m_bSoundStarted = false;

	SetScheme( "basemodui_scheme" );
}

void CHudCommentary::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_bUseScriptBGColor )
	{
		SetBgColor( m_BGOverrideColor );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::Paint()
{
	float flDuration = (m_flEndTime - m_flStartTime);
	float flPercentage = 1.0f;

	if ( !m_hActiveNode )
	{
		if ( !m_bHiding )
		{
			m_bHiding = true;
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence( "HideCommentary" );

			CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_FULLSCREEN_HUDELEMENT( CHudCloseCaption );

			if ( pHudCloseCaption )
			{
				pHudCloseCaption->Reset();
			}
		}
	}
	else
	{
		CSoundPatch *pSoundPatch = m_hActiveNode->GetSoundPatch();
		float fElapsedTime = CSoundEnvelopeController::GetController().SoundGetElapsedTime( pSoundPatch );
		flPercentage = clamp( fElapsedTime / flDuration, 0.0f, 1.0f );

		// Detect if we started the commentary (we may not always reach 100% from the elapsed time, and then it gets back to 0% directly)
		if ( flPercentage > 0.00f )
		{
			m_bSoundStarted = true;
		}

		if ( m_bSoundStarted && ( flPercentage == 0.0f ) && m_hActiveNode )
		{
			// The sound started and is now finished (elapsed time is 0% as the sound is not present anymore), time to stop.
			m_hActiveNode = NULL;
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence( "HideCommentary" );

			engine->ServerCmd( "commentary_finishnode\n" );
			flPercentage = 1.0f;		// Set the percentage to 1, so we don't see the progress bar go back to 0 when it reached the end.
		}
	}

	if ( !m_bShouldPaint )
		return;

	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );

 	int xOffset = m_iBarX;
	int yOffset = m_iBarY;

	// Find our fade based on our time shown
	Color clr = Color( 255, 170, 0, GetAlpha() );

	// Draw the progress bar
	vgui::surface()->DrawSetColor( clr );
	vgui::surface()->DrawOutlinedRect( xOffset, yOffset, xOffset+m_iBarWide, yOffset+m_iBarTall );
	vgui::surface()->DrawSetColor( clr );
	vgui::surface()->DrawFilledRect( xOffset+2, yOffset+2, xOffset+(int)(flPercentage*m_iBarWide)-2, yOffset+m_iBarTall-2 );

	// Draw the speaker names
	// Get our scheme and font information
	vgui::HScheme scheme = GetScheme();
	vgui::HFont hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentaryDefault", true );
	if ( !hFont )
	{
		hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Default", true );
	}
	vgui::surface()->DrawSetTextFont( hFont );
	vgui::surface()->DrawSetTextColor( clr ); 
	vgui::surface()->DrawSetTextPos( m_iSpeakersX, m_iSpeakersY );
	vgui::surface()->DrawPrintText( m_szSpeakers, wcslen(m_szSpeakers) );

	if ( COMMENTARY_BUTTONS & IN_ATTACK )
	{
		int iY = m_iBarY + m_iBarTall + YRES(4);
		wchar_t wzFinal[512] = L"";

		wchar_t *pszText = g_pVGuiLocalize->Find( "#Commentary_PrimaryAttack" );
		if ( pszText )
		{
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ) );
			vgui::surface()->DrawSetTextPos( m_iSpeakersX, iY );
			vgui::surface()->DrawPrintText( wzFinal, wcslen(wzFinal) );
		}

		pszText = g_pVGuiLocalize->Find( "#Commentary_SecondaryAttack" );
		if ( pszText )
		{
			int w, h;
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ) );
			vgui::surface()->GetTextSize( hFont, wzFinal, w, h );
			vgui::surface()->DrawSetTextPos( m_iBarX + m_iBarWide - w, iY );
			vgui::surface()->DrawPrintText( wzFinal, wcslen(wzFinal) );
		}
	}

	// Draw the commentary count
	// Determine our text size, and move that far in from the right hand size (plus the offset)
	int iCountWide, iCountTall;
	vgui::surface()->GetTextSize( hFont, m_szCount, iCountWide, iCountTall );
	vgui::surface()->DrawSetTextPos( wide - m_iCountXFR - iCountWide, m_iCountY );
	vgui::surface()->DrawPrintText( m_szCount, wcslen(m_szCount) );

	// Draw the icon
 	vgui::surface()->DrawSetColor( Color(255,170,0,GetAlpha()) );
	vgui::surface()->DrawSetTexture(m_nIconTextureId);
	vgui::surface()->DrawTexturedRect( m_iIconX, m_iIconY, m_iIconWide, m_iIconTall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudCommentary::ShouldDraw()
{
	return ( m_hActiveNode || GetAlpha() > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::Init( void )
{ 
	m_matIcon.Init( "vgui/hud/icon_commentary", TEXTURE_GROUP_VGUI );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::VidInit( void )
{ 
	SetAlpha(0);
	StopCommentary();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::StartCommentary( C_PointCommentaryNode *pNode, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime )
{
	if ( (flEndTime - flStartTime) <= 0 )
		return;

	m_hActiveNode = pNode;
	m_flStartTime = flStartTime;
	m_flEndTime = flEndTime;
	m_bSoundStarted = false;
	m_bHiding = false;
	g_pVGuiLocalize->ConvertANSIToUnicode( pszSpeakers, m_szSpeakers, sizeof(m_szSpeakers) );

	// Don't draw the element itself if closecaptions are on (and captions are always on in non-english mode)
	ConVarRef pCVar( "closecaption" );
	if ( pCVar.IsValid() )
	{
		m_bShouldPaint = ( !pCVar.GetBool() && english.GetBool() );
	}
	else
	{
		m_bShouldPaint = true;
	}
	SetPaintBackgroundEnabled( m_bShouldPaint );

	char sz[MAX_COUNT_STRING];
	Q_snprintf( sz, sizeof(sz), "%d \\ %d", iNode, iNodeMax );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, m_szCount, sizeof(m_szCount) );

	// If the commentary just started, play the commentary fade in.
	if ( fabs(flStartTime - gpGlobals->curtime) < 1.0 )
	{
		GetClientMode()->GetViewportAnimationController()->StartAnimationSequence( "ShowCommentary" );
	}
	else
	{
		// We're reloading a savegame that has an active commentary going in it. Don't fade in.
		SetAlpha( 255 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::StopCommentary( void )
{
	m_hActiveNode = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CommentaryModeShouldSwallowInput( C_BasePlayer *pPlayer )
{
	if ( !IsInCommentaryMode() )	
		return false;

	if ( pPlayer->m_nButtons & COMMENTARY_BUTTONS )
	{
		// Always steal the secondary attack
		if ( pPlayer->m_nButtons & IN_ATTACK2 )
			return true;

		// See if there's any nodes ahead of us.
		if ( IsNodeUnderCrosshair( pPlayer ) )
			return true;
	}

	return false;
}
