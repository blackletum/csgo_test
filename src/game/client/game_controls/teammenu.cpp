﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>

#include "teammenu.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/RichText.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>
#include <game/client/iviewport.h>
#include <stdlib.h> // MAX_PATH define
#include <stdio.h>
#include "byteswap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

void UpdateCursorState();
// void DuckMessage(const char *str);

// helper function
const char *GetStringTeamColor( int i )
{
	switch( i )
	{
	case 0:
		return "team0";

	case 1:
		return "team1";

	case 2:
		return "team2";

	case 3:
		return "team3";

	case 4:
	default:
		return "team4";
	}
}



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamMenu::CTeamMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_TEAM )
{
	m_pViewPort = pViewPort;
	m_iJumpKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

#if !defined(CSTRIKE15)
	// info window about this map
	m_pMapInfo = new RichText( this, "MapInfo" );

#if defined( ENABLE_HTML_WINDOW )
	m_pMapInfoHTML = new HTML( this, "MapInfoHTML");
#endif
#endif

	LoadControlSettings("Resource/UI/TeamMenu.res");
	InvalidateLayout();

	m_szMapName[0] = 0;

	//=============================================================================
	// HPE_BEGIN:
	// [Forrest] 
	//=============================================================================
	ListenForGameEvent( "cs_match_end_restart" );
	//=============================================================================
	// HPE_END
	//=============================================================================
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTeamMenu::~CTeamMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CTeamMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

#if !defined(CSTRIKE15)
    m_pMapInfo->SetFgColor( pScheme->GetColor("MapDescriptionText", Color(255, 255, 255, 0)) );

	if ( *m_szMapName )
	{
		LoadMapPage( m_szMapName ); // reload the map description to pick up the color
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: makes the user choose the auto assign option
//-----------------------------------------------------------------------------
void CTeamMenu::AutoAssign()
{
	engine->ClientCmd("jointeam 0");
	OnClose();
}


//-----------------------------------------------------------------------------
// Purpose: shows the team menu
//-----------------------------------------------------------------------------
void CTeamMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();

		SetMouseInputEnabled( true );

		// get key bindings if shown

		if( m_iJumpKey == BUTTON_CODE_INVALID ) // you need to lookup the jump key AFTER the engine has loaded
		{
			m_iJumpKey = gameuifuncs->GetButtonCodeForBind( "jump" );
		}

		if ( m_iScoreBoardKey == BUTTON_CODE_INVALID ) 
		{
			m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );
		}
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}

	m_pViewPort->ShowBackGround( bShow );
}

//=============================================================================
// HPE_BEGIN:
// [Forrest] 
//=============================================================================
//-----------------------------------------------------------------------------
// Purpose: respond to game events
//-----------------------------------------------------------------------------
void CTeamMenu::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if( 0 == Q_strcmp( type, "cs_match_end_restart" ) )
	{
		// Reselect teams when restarting a new match.
		ShowPanel( true );
	}
}
//=============================================================================
// HPE_END
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CTeamMenu::Update()
{
#if !defined(CSTRIKE15)
	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof(mapname) );

	SetLabelText( "mapname", mapname );

	// DWenger - Pulled out for now - LoadMapPage( mapname );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CTeamMenu::LoadMapPage( const char *mapName )
{
#if !defined(CSTRIKE15)
	// Save off the map name so we can re-load the page in ApplySchemeSettings().
	Q_strncpy( m_szMapName, mapName, strlen( mapName ) + 1 );
	
	char mapRES[ MAX_PATH ];

	char uilanguage[ 64 ];
	engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

	Q_snprintf( mapRES, sizeof( mapRES ), "resource/maphtml/%s_%s.html", mapName, uilanguage );

	bool bFoundHTML = false;

	if ( !g_pFullFileSystem->FileExists( mapRES ) )
	{
		// try english
		Q_snprintf( mapRES, sizeof( mapRES ), "resource/maphtml/%s_english.html", mapName );
	}
	else
	{
		bFoundHTML = true;
	}

	if( bFoundHTML || g_pFullFileSystem->FileExists( mapRES ) )
	{
		// it's a local HTML file
		char localURL[ _MAX_PATH + 7 ];
		Q_strncpy( localURL, "file://", sizeof( localURL ) );

		char pPathData[ _MAX_PATH ];
		g_pFullFileSystem->GetLocalPath( mapRES, pPathData, sizeof(pPathData) );
		Q_strncat( localURL, pPathData, sizeof( localURL ), COPY_ALL_CHARACTERS );

		// force steam to dump a local copy
		g_pFullFileSystem->GetLocalCopy( pPathData );

		m_pMapInfo->SetVisible( false );

#if defined( ENABLE_HTML_WINDOW )
		m_pMapInfoHTML->SetVisible( true );
		m_pMapInfoHTML->OpenURL( localURL );
#endif
		InvalidateLayout();
		Repaint();		

		return;
	}
	else
	{
		m_pMapInfo->SetVisible( true );

#if defined( ENABLE_HTML_WINDOW )
		m_pMapInfoHTML->SetVisible( false );
#endif
	}

	Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s.txt", mapName);

	// if no map specific description exists, load default text
	if( !g_pFullFileSystem->FileExists( mapRES ) )
	{
		if ( g_pFullFileSystem->FileExists( "maps/default.txt" ) )
		{
			Q_snprintf ( mapRES, sizeof( mapRES ), "maps/default.txt");
		}
		else
		{
			m_pMapInfo->SetText( "" );
			return; 
		}
	}

	FileHandle_t f = g_pFullFileSystem->Open( mapRES, "r" );

	// read into a memory block
	int fileSize = g_pFullFileSystem->Size(f);
	int dataSize = fileSize + sizeof( wchar_t );
	if ( dataSize % 2 )
		++dataSize;
	wchar_t *memBlock = (wchar_t *)malloc(dataSize);
	memset( memBlock, 0x0, dataSize);
	int bytesRead = g_pFullFileSystem->Read(memBlock, fileSize, f);
	if ( bytesRead < fileSize )
	{
		// NULL-terminate based on the length read in, since Read() can transform \r\n to \n and
		// return fewer bytes than we were expecting.
		char *data = reinterpret_cast<char *>( memBlock );
		data[ bytesRead ] = 0;
		data[ bytesRead+1 ] = 0;
	}

	// null-terminate the stream (redundant, since we memset & then trimmed the transformed buffer already)
	memBlock[dataSize / sizeof(wchar_t) - 1] = 0x0000;

	// ensure little-endian unicode reads correctly on all platforms
	CByteswap byteSwap;
	byteSwap.SetTargetBigEndian( false );
	byteSwap.SwapBufferToTargetEndian( memBlock, memBlock, dataSize/sizeof(wchar_t) );

	// check the first character, make sure this a little-endian unicode file
	if ( memBlock[0] != 0xFEFF )
	{
		// its a ascii char file
		m_pMapInfo->SetText( reinterpret_cast<char *>( memBlock ) );
	}
	else
	{
		m_pMapInfo->SetText( memBlock+1 );
	}
	// go back to the top of the text buffer
	m_pMapInfo->GotoTextStart();

	g_pFullFileSystem->Close( f );
	free(memBlock);

	InvalidateLayout();
	Repaint();
#endif
}

/*
//-----------------------------------------------------------------------------
// Purpose: sets the text on and displays the team buttons
//-----------------------------------------------------------------------------
void CTeamMenu::MakeTeamButtons(void)
{
	int i = 0;

	for( i = 0; i< m_pTeamButtons.Count(); i++ )
	{
		m_pTeamButtons[i]->SetVisible(false);
	}

	i = 0;

	while( true )
	{
		const char *teamname = GameResources()->GetTeamName( i );

		if ( !teamname || !teamname[0] )
			return; // no more teams
	
		char buttonText[32];
		Q_snprintf( buttonText, sizeof(buttonText), "&%i %s", i +1, teamname ); 
		m_pTeamButtons[i]->SetText( buttonText );

		m_pTeamButtons[i]->SetCommand( new KeyValues("TeamButton", "team", i ) );	
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		m_pTeamButtons[i]->SetArmedColor(pScheme->GetColor(GetStringTeamColor(i), Color(255, 255, 255, 255))  ,  pScheme->GetColor("SelectionBG", Color(255, 255, 255, 0)) );
		m_pTeamButtons[i]->SetDepressedColor( pScheme->GetColor(GetStringTeamColor(i), Color(255, 255, 255, 255)), pScheme->GetColor("ButtonArmedBgColor", Color(255, 255, 255, 0)) );
		m_pTeamButtons[i]->SetDefaultColor( pScheme->GetColor(GetStringTeamColor(i), Color(255, 255, 255, 255)), pScheme->GetColor("ButtonDepressedBgColor", Color(255, 255, 255, 0)) );
		m_pTeamButtons[i]->SetVisible(true);

		i++;
	}
} 


//-----------------------------------------------------------------------------
// Purpose: When a team button is pressed it triggers this function to cause the player to join a team
//-----------------------------------------------------------------------------
void CTeamMenu::OnTeamButton( int team )
{
	char cmd[64];
	if( team >= m_iNumTeams )  // its a special button
	{
		if( team == m_iNumTeams ) // first extra team is auto assign	
		{
			Q_snprintf( cmd, sizeof( cmd ), "jointeam 5" );
		}
		else // next is spectate
		{
			// DuckMessage( "#Spec_Duck" );
			gViewPortInterface->ShowBackGround( false );
		}
	}
	else
	{
		Q_snprintf( cmd, sizeof( cmd ), "jointeam %i", team + 1 );
		//g_iTeamNumber = team + 1;
	}

	engine->ClientCmd(cmd);
	SetVisible( false );
	OnClose();
} */

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CTeamMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

void CTeamMenu::OnKeyCodePressed(KeyCode code)
{
	if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		GetViewPortInterface()->ShowPanel( PANEL_SCOREBOARD, true );
		GetViewPortInterface()->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	//=============================================================================
	// HPE_BEGIN
	// mjohnson
	// Remove the jump auto-assign code on the xbox.  The jump code is the gamepad's
	// A button which is generally expected to select the currently focused item.
	// Add in the code for processing the stick movement up/down to modify selection.
	//=============================================================================
#if !defined(CSTRIKE15)
	else if( m_iJumpKey != BUTTON_CODE_INVALID && m_iJumpKey == code )
	{
		AutoAssign();
	}
#else
	else if (KEY_XSTICK1_UP == code || KEY_XBUTTON_UP == code)
	{
		RequestFocusPrev(GetFocusNavGroup().GetCurrentFocus()->GetVPanel());
	}
	else if (KEY_XSTICK1_DOWN == code || KEY_XBUTTON_DOWN == code)
	{
		RequestFocusNext(GetFocusNavGroup().GetCurrentFocus()->GetVPanel());
	}
#endif
	//=============================================================================
	// HPE_END
	//=============================================================================
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}
