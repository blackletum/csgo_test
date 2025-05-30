﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws CSPort's death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <game_controls/baseviewport.h>
#include "clientmode_shared.h"
#include "c_baseplayer.h"
#include "c_team.h"
#include "tf_shareddefs.h"

#include "hud_basedeathnotice.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );


using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudBaseDeathNotice::CHudBaseDeathNotice( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudDeathNotice" )
{
	vgui::Panel *pParent = GetClientMode()->GetViewport();
	SetParent( pParent );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );

	CalcRoundedCorners();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::Init( void )
{
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "object_destroyed" );	
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );
	ListenForGameEvent( "teamplay_flag_event" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::VidInit( void )
{
	m_DeathNotices.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CHudBaseDeathNotice::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CHudBaseDeathNotice::GetTeamColor( int iTeamNumber )
{
	// By default, return the standard team color.  Subclasses may override this.
	return g_PR->GetTeamColor( iTeamNumber );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::Paint()
{
	// Retire any death notices that have expired
	RetireExpiredDeathNotices();

	CBaseViewport *pViewport = dynamic_cast<CBaseViewport *>( GetClientModeNormal()->GetViewport() );
	int yStart = pViewport->GetDeathMessageStartHeight();

	surface()->DrawSetTextFont( m_hTextFont );

	int xMargin = XRES( 10 );
	int xSpacing = UTIL_ComputeStringWidth( m_hTextFont, L" " );

	int iCount = m_DeathNotices.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		DeathNoticeItem &msg = m_DeathNotices[i];
		
		CHudTexture *icon = msg.iconDeath;
						
		wchar_t victim[256]=L"";
		wchar_t killer[256]=L"";

		// TEMP - print the death icon name if we don't have a material for it

		g_pVGuiLocalize->ConvertANSIToUnicode( msg.Victim.szName, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( msg.Killer.szName, killer, sizeof( killer ) );

		int iVictimTextWide = UTIL_ComputeStringWidth( m_hTextFont, victim ) + xSpacing;
		int iDeathInfoTextWide= msg.wzInfoText[0] ? UTIL_ComputeStringWidth( m_hTextFont, msg.wzInfoText ) + xSpacing : 0;
		int iKillerTextWide = killer[0] ? UTIL_ComputeStringWidth( m_hTextFont, killer ) + xSpacing : 0;
		int iLineTall = m_flLineHeight;
		int iTextTall = surface()->GetFontTall( m_hTextFont );
		int iconWide = 0, iconTall = 0, iDeathInfoOffset = 0, iVictimTextOffset = 0, iconActualWide = 0;

		// Get the local position for this notice
		if ( icon )
		{			
			iconActualWide = icon->EffectiveWidth( 1.0f );
			iconWide = iconActualWide + xSpacing;
			iconTall = icon->EffectiveHeight( 1.0f );
			
			int iconTallDesired = iLineTall-YRES(2);
			Assert( 0 != iconTallDesired );
			float flScale = (float) iconTallDesired / (float) iconTall;

			iconActualWide *= flScale;
			iconTall *= flScale;
			iconWide *= flScale;
		}
		int iTotalWide = iKillerTextWide + iconWide + iVictimTextWide + iDeathInfoTextWide + ( xMargin * 2 );
		int y = yStart + ( ( iLineTall + m_flLineSpacing ) * i );				
		int yText = y + ( ( iLineTall - iTextTall ) / 2 );
		int yIcon = y + ( ( iLineTall - iconTall ) / 2 );

		int x=0;
		if ( m_bRightJustify )
		{
			x =	GetWide() - iTotalWide;
		}

		// draw a background panel for the message
		Vertex_t vert[NUM_BACKGROUND_COORD];
		GetBackgroundPolygonVerts( x, y+1, x+iTotalWide, y+iLineTall-1, ARRAYSIZE( vert ), vert );		
		surface()->DrawSetTexture( -1 );
		surface()->DrawSetColor( msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor );
		surface()->DrawTexturedPolygon( ARRAYSIZE( vert ), vert );

		x += xMargin;
			
		if ( killer[0] )
		{
			// Draw killer's name
			DrawText( x, yText, m_hTextFont, GetTeamColor( msg.Killer.iTeam ), killer );
			x += iKillerTextWide;
		}

		// Draw death icon
		if ( icon )
		{
			icon->DrawSelf( x, yIcon, iconActualWide, iconTall, m_clrIcon );
			x += iconWide;
		}

		// Draw additional info text next to death icon 
		if ( msg.wzInfoText[0] )
		{
			if ( msg.bSelfInflicted )
			{
				iDeathInfoOffset += iVictimTextWide;
				iVictimTextOffset -= iDeathInfoTextWide;
			}

			DrawText( x + iDeathInfoOffset, yText, m_hTextFont, Color(255,255,255,255), msg.wzInfoText );
			x += iDeathInfoTextWide;
		}

		// Draw victims name
		DrawText( x + iVictimTextOffset, yText, m_hTextFont, GetTeamColor( msg.Victim.iTeam ), victim );
		x += iVictimTextWide;
	}
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::RetireExpiredDeathNotices()
{
	// Remove any expired death notices.  Loop backwards because we might remove one
	int iCount = m_DeathNotices.Count();
	for ( int i = iCount-1; i >= 0; i-- )
	{
		if ( gpGlobals->curtime > m_DeathNotices[i].GetExpiryTime() )
		{
			m_DeathNotices.Remove(i);
		}
	}

	// Do we have too many death messages in the queue?
	if ( m_DeathNotices.Count() > 0 &&
		m_DeathNotices.Count() > (int)m_flMaxDeathNotices )
	{		 
		// First, remove any notices not involving the local player, since they are lower priority.		
		iCount = m_DeathNotices.Count();
		int iNeedToRemove = iCount - (int)m_flMaxDeathNotices;
		// loop condition is iCount-1 because we won't remove the most recent death notice, otherwise
		// new non-local-player-involved messages would not appear if the queue was full of messages involving the local player
		for ( int i = 0; i < iCount-1 && iNeedToRemove > 0 ; i++ )
		{
			if ( !m_DeathNotices[i].bLocalPlayerInvolved )
			{
				m_DeathNotices.Remove( i );
				iCount--;
				iNeedToRemove--;
			}	
		}

		// Now that we've culled any non-local-player-involved messages up to the amount we needed to remove, see
		// if we've removed enough
		iCount = m_DeathNotices.Count();
		iNeedToRemove = iCount - (int)m_flMaxDeathNotices;
		if ( iNeedToRemove > 0 )		
		{
			// if we still have too many messages, then just remove however many we need, oldest first
			for ( int i = 0; i < iNeedToRemove; i++ )
			{
				m_DeathNotices.Remove( 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::FireGameEvent( IGameEvent *event )
{
	if ( !g_PR )
	{
		return;
	}

	if ( hud_deathnotice_time.GetFloat() == 0 )
	{
		return;
	}

	const char *pszEventName = event->GetName();
	
	// Add a new death message.  Note we always look it up by index rather than create a reference or pointer to it;
	// additional messages may get added during this function that cause the underlying array to get realloced, so don't
	// ever keep a pointer to memory here.
	int iMsg = AddDeathNoticeItem();
	int iLocalPlayerIndex = GetLocalPlayerIndex();

	bool bPlayerDeath = FStrEq( pszEventName, "player_death" );
	bool bObjectDeath = FStrEq( pszEventName, "object_destroyed" );

	if ( bPlayerDeath || bObjectDeath )
	{
		int victim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		int killer = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		const char *killedwith = event->GetString( "weapon" );
		const char *killedwithweaponlog = event->GetString( "weapon_logclassname" );

		if ( bObjectDeath && victim == 0 )
		{
			// for now, no death notices of map placed objects
			m_DeathNotices.Remove( iMsg );
			return;
		}

		// Get the names of the players
		const char *killer_name = g_PR->GetPlayerName( killer );
		const char *victim_name = g_PR->GetPlayerName( victim );
		if ( !killer_name )
		{
			killer_name = "";
		}

		if ( !victim_name )
		{
			victim_name = "";
		}

		// Make a new death notice
		bool bLocalPlayerInvolved = false;
		if ( iLocalPlayerIndex == killer || iLocalPlayerIndex == victim )
		{
			bLocalPlayerInvolved = true;
		}
		m_DeathNotices[iMsg].bLocalPlayerInvolved = bLocalPlayerInvolved;
		m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( killer );
		m_DeathNotices[iMsg].Victim.iTeam = g_PR->GetTeam( victim );
		Q_strncpy( m_DeathNotices[iMsg].Killer.szName, killer_name, ARRAYSIZE( m_DeathNotices[iMsg].Killer.szName ) );
		Q_strncpy( m_DeathNotices[iMsg].Victim.szName, victim_name, ARRAYSIZE( m_DeathNotices[iMsg].Victim.szName ) );
		if ( killedwith && *killedwith )
		{
			Q_snprintf( m_DeathNotices[iMsg].szIcon, sizeof(m_DeathNotices[iMsg].szIcon), "d_%s", killedwith );
		}			
		if ( !killer || killer == victim )
		{
			m_DeathNotices[iMsg].bSelfInflicted = true;
			m_DeathNotices[iMsg].Killer.szName[0] = 0;

			if ( event->GetInt( "damagebits" ) & DMG_FALL )
			{
				// special case text for falling death
				V_wcsncpy( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( "#DeathMsg_Fall" ), sizeof( m_DeathNotices[iMsg].wzInfoText ) );
			}
			else if ( ( event->GetInt( "damagebits" ) & DMG_VEHICLE ) || ( 0 == Q_stricmp( m_DeathNotices[iMsg].szIcon, "d_tracktrain" ) ) )
			{
				// special case icon for hit-by-vehicle death
				Q_strncpy( m_DeathNotices[iMsg].szIcon, "d_vehicle", ARRAYSIZE( m_DeathNotices[iMsg].szIcon ) );
			}			
		}

		char sDeathMsg[512];

		// Record the death notice in the console
		if ( m_DeathNotices[iMsg].bSelfInflicted )
		{
			if ( !strcmp( m_DeathNotices[iMsg].szIcon, "d_worldspawn" ) )
			{
				Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s died.", m_DeathNotices[iMsg].Victim.szName );
			}
			else	// d_world
			{
				Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s suicided.", m_DeathNotices[iMsg].Victim.szName );
			}
		}
		else
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s killed %s", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName );

			if ( killedwithweaponlog && killedwithweaponlog[0] && ( killedwithweaponlog[0] > 13 ) )
			{
				Q_strncat( sDeathMsg, VarArgs( " with %s.", killedwithweaponlog ), sizeof( sDeathMsg ), COPY_ALL_CHARACTERS );
			}
			else if ( m_DeathNotices[iMsg].szIcon[0] && ( m_DeathNotices[iMsg].szIcon[0] > 13 ) )
			{
				Q_strncat( sDeathMsg, VarArgs( " with %s.", &m_DeathNotices[iMsg].szIcon[2] ), sizeof( sDeathMsg ), COPY_ALL_CHARACTERS );
			}
		}

		Msg( "%s\n", sDeathMsg );

	} 
	else if ( FStrEq( "teamplay_point_captured", pszEventName ) )
	{
		GetLocalizedControlPointName( event, m_DeathNotices[iMsg].Victim.szName, ARRAYSIZE( m_DeathNotices[iMsg].Victim.szName ) );

		// Array of capper indices
		const char *cappers = event->GetString("cappers");

		char szCappers[256];
		szCappers[0] = '\0';

		int len = Q_strlen(cappers);
		for( int i=0;i<len;i++ )
		{
			int iPlayerIndex = (int)cappers[i];

			Assert( iPlayerIndex > 0 && iPlayerIndex <= gpGlobals->maxClients );

			const char *pPlayerName = g_PR->GetPlayerName( iPlayerIndex );

			if ( i == 0 )
			{
				// use first player as the team
				m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
				m_DeathNotices[iMsg].Victim.iTeam = TEAM_UNASSIGNED;
			}
			else
			{
				Q_strncat( szCappers, ", ", sizeof(szCappers), 2 );
			}

			Q_strncat( szCappers, pPlayerName, sizeof(szCappers), COPY_ALL_CHARACTERS );
			if ( iLocalPlayerIndex == iPlayerIndex )
				m_DeathNotices[iMsg].bLocalPlayerInvolved = true;
		}		

		Q_strncpy( m_DeathNotices[iMsg].Killer.szName, szCappers, sizeof(m_DeathNotices[iMsg].Killer.szName) );
		V_wcsncpy( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( "#Msg_Captured" ), sizeof( m_DeathNotices[iMsg].wzInfoText ) );

		// print a log message
		Msg( "%s captured %s for team #%d\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName, m_DeathNotices[iMsg].Killer.iTeam );
	} 
	else if ( FStrEq( "teamplay_capture_blocked", pszEventName ) )
	{
		GetLocalizedControlPointName( event, m_DeathNotices[iMsg].Victim.szName, ARRAYSIZE( m_DeathNotices[iMsg].Victim.szName ) );
		V_wcsncpy( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( "#Msg_Defended" ), sizeof( m_DeathNotices[iMsg].wzInfoText ) );

		int iPlayerIndex = event->GetInt( "blocker" );
		const char *blocker_name = g_PR->GetPlayerName( iPlayerIndex );
		Q_strncpy( m_DeathNotices[iMsg].Killer.szName, blocker_name, ARRAYSIZE( m_DeathNotices[iMsg].Killer.szName ) );
		m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
		if ( iLocalPlayerIndex == iPlayerIndex )
			m_DeathNotices[iMsg].bLocalPlayerInvolved = true;

		// print a log message
		Msg( "%s defended %s for team #%d\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName, m_DeathNotices[iMsg].Killer.iTeam );
	}
	else if ( FStrEq( "teamplay_flag_event", pszEventName ) )
	{
		const char *pszMsgKey = NULL;
		int iEventType = event->GetInt( "eventtype" );
		switch ( iEventType )
		{
		case TF_FLAGEVENT_PICKUP: 
			pszMsgKey = "#Msg_PickedUpFlag"; 
			break;
		case TF_FLAGEVENT_CAPTURE: 
			pszMsgKey = "#Msg_CapturedFlag"; 
			break;
		case TF_FLAGEVENT_DEFEND: 
			pszMsgKey = "#Msg_DefendedFlag"; 
			break;

		// Add this when we can get localization for it
		//case TF_FLAGEVENT_DROPPED: 
		//	pszMsgKey = "#Msg_DroppedFlag"; 
		//	break;

		default:
			// unsupported, don't put anything up			
			m_DeathNotices.Remove( iMsg );
			return;
		}

		wchar_t *pwzEventText = g_pVGuiLocalize->Find( pszMsgKey );
		Assert( pwzEventText );
		if ( pwzEventText )
		{
			V_wcsncpy( m_DeathNotices[iMsg].wzInfoText, pwzEventText, sizeof( m_DeathNotices[iMsg].wzInfoText ) );
		}
		else
		{
			V_memset( m_DeathNotices[iMsg].wzInfoText, 0, sizeof( m_DeathNotices[iMsg].wzInfoText ) );
		}

		int iPlayerIndex = event->GetInt( "player" );
		const char *szPlayerName = g_PR->GetPlayerName( iPlayerIndex );
		Q_strncpy( m_DeathNotices[iMsg].Killer.szName, szPlayerName, ARRAYSIZE( m_DeathNotices[iMsg].Killer.szName ) );
		m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
		if ( iLocalPlayerIndex == iPlayerIndex )
			m_DeathNotices[iMsg].bLocalPlayerInvolved = true;
	}

	OnGameEvent( event, m_DeathNotices[iMsg] );

	if ( !m_DeathNotices[iMsg].iconDeath && m_DeathNotices[iMsg].szIcon )
	{
		// Try and find the death identifier in the icon list
		// On consoles, we flip usage of the inverted icon to make it more visible
		bool bInverted = m_DeathNotices[iMsg].bLocalPlayerInvolved;
		if ( IsGameConsole() )
		{
			bInverted = !bInverted;
		}
		m_DeathNotices[iMsg].iconDeath = GetIcon( m_DeathNotices[iMsg].szIcon, bInverted );
		if ( !m_DeathNotices[iMsg].iconDeath )
		{
			// Can't find it, so use the default skull & crossbones icon
			m_DeathNotices[iMsg].iconDeath = GetIcon( "d_skull_tf", m_DeathNotices[iMsg].bLocalPlayerInvolved );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the localized name of the control point sent in the event
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::GetLocalizedControlPointName( IGameEvent *event, char *namebuf, int namelen )
{
	// Cap point name ( MATTTODO: can't we find this from the point index ? )
	const char *pName = event->GetString( "cpname", "Unnamed Control Point" );
	const wchar_t *pLocalizedName = g_pVGuiLocalize->Find( pName );

	if ( pLocalizedName )
	{
		g_pVGuiLocalize->ConvertUnicodeToANSI( pLocalizedName, namebuf, namelen );
	}
	else
	{
		Q_strncpy( namebuf, pName, namelen );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new death notice to the queue
//-----------------------------------------------------------------------------
int CHudBaseDeathNotice::AddDeathNoticeItem()
{
	int iMsg = m_DeathNotices.AddToTail();
	DeathNoticeItem &msg = m_DeathNotices[iMsg];
	msg.flCreationTime = gpGlobals->curtime;
	return iMsg;
}

//-----------------------------------------------------------------------------
// Purpose: draw text helper
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::DrawText( int x, int y, HFont hFont, Color clr, const wchar_t *szText )
{
	surface()->DrawSetTextPos( x, y );
	surface()->DrawSetTextColor( clr );
	surface()->DrawSetTextFont( hFont );	//reset the font, draw icon can change it
	surface()->DrawUnicodeString( szText, FONT_DRAW_NONADDITIVE );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a rounded-corner polygon that fits in the specified bounds
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::GetBackgroundPolygonVerts( int x0, int y0, int x1, int y1, int iVerts, vgui::Vertex_t vert[] )
{
	Assert( iVerts == NUM_BACKGROUND_COORD );
	// use the offsets we generated for one corner and apply those to the passed-in dimensions to create verts for the poly
	for ( int i = 0; i < NUM_CORNER_COORD; i++ )
	{
		int j = ( NUM_CORNER_COORD-1 ) - i;
		// upper left corner
		vert[i].Init( Vector2D( x0 + m_CornerCoord[i].x, y0 + m_CornerCoord[i].y ) );
		// upper right corner
		vert[i+NUM_CORNER_COORD].Init( Vector2D( x1 - m_CornerCoord[j].x, y0 + m_CornerCoord[j].y ) );
		// lower right corner
		vert[i+(NUM_CORNER_COORD*2)].Init( Vector2D( x1 - m_CornerCoord[i].x, y1 - m_CornerCoord[i].y ) );
		// lower left corner
		vert[i+(NUM_CORNER_COORD*3)].Init( Vector2D( x0 + m_CornerCoord[j].x, y1 - m_CornerCoord[j].y) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates the offsets for rounded corners based on current screen res
//-----------------------------------------------------------------------------
void CHudBaseDeathNotice::CalcRoundedCorners()
{
	// generate the offset geometry for upper left corner
	int iMax = ARRAYSIZE( m_CornerCoord );
	for ( int i = 0; i < iMax; i++ )
	{
		m_CornerCoord[i].x = m_flCornerRadius * ( 1 - cos( ( (float) i / (float) (iMax - 1 ) ) * ( M_PI / 2 ) ) );
		m_CornerCoord[i].y = m_flCornerRadius * ( 1 - sin( ( (float) i / (float) (iMax - 1 ) ) * ( M_PI / 2 ) ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets specified icon
//-----------------------------------------------------------------------------
CHudTexture *CHudBaseDeathNotice::GetIcon( const char *szIcon, bool bInvert )
{
	// get the inverted version if specified
	if ( bInvert && 0 == V_strncmp( "d_", szIcon, 2 ) )
	{
		// change prefix from d_ to dneg_
		char szIconTmp[255] = "dneg_";
		V_strcat( szIconTmp, szIcon+2, ARRAYSIZE( szIconTmp ) );
		CHudTexture *pIcon = HudIcons().GetIcon( szIconTmp );
		// return inverted version if found
		if ( pIcon )
			return pIcon;
		// if we didn't find the inverted version, keep going and try the normal version
	}
	return HudIcons().GetIcon( szIcon );
}

//-----------------------------------------------------------------------------
// Purpose: Gets the expiry time for this death notice item
//-----------------------------------------------------------------------------
float DeathNoticeItem::GetExpiryTime()
{
	float flDuration = hud_deathnotice_time.GetFloat();
	if ( bLocalPlayerInvolved )
	{
		// if the local player is involved, make the message last longer
		flDuration *= 2;
	}
	return flCreationTime + flDuration;
}