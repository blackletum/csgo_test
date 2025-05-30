﻿//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
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
//=============================================================================
#include "pch_serverbrowser.h"
#include <GameUI/IGameUI.h>

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

#if defined( _WIN32 ) && !defined( _X360 )
#define WIN32_LEAN_AND_MEAN
#include <winsock.h>
#endif

#ifdef POSIX
#include <arpa/inet.h>
#endif

using namespace vgui;

ConVar sb_quick_list_bit_field( "sb_quick_list_bit_field", "-1" );

static CServerBrowserDialog *s_InternetDlg = NULL;
extern IGameUI *pGameUI;

static const int BROWSER_WIDTH = 640;
static const int BROWSER_HEIGHT = 384;


CServerBrowserDialog &ServerBrowserDialog()
{
	return *CServerBrowserDialog::GetInstance();
}


// Returns a list of the ports that we hit when looking for 
void GetMostCommonQueryPorts( CUtlVector<uint16> &ports )
{
	for ( int i=0; i <= 5; i++ )
	{
		ports.AddToTail( 27015 + i );
		ports.AddToTail( 26900 + i );
	}

	ports.AddToTail(4242); //RDKF
	ports.AddToTail(27215); //Lost Planet
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServerBrowserDialog::CServerBrowserDialog(vgui::Panel *parent) : Frame(parent, "CServerBrowserDialog")
{
	s_InternetDlg = this;

	m_szGameName[0] = 0;
	m_szModDir[0] = 0;
	m_pSavedData = NULL;
	m_pFilterData = NULL;
	m_pFavorites = NULL;
	m_pBlacklist = NULL;
	m_pHistory = NULL;

	// Do this before LoadUserData() so it loads the blacklist file properly
	m_pBlacklist = new CBlacklistedServers(this);

	LoadUserData();

	m_pInternetGames = new CCustomGames(this);
	m_pFavorites = new CFavoriteGames(this);
	m_pHistory = new CHistoryGames(this);
	m_pSpectateGames = new CSpectateGames(this);
	m_pLanGames = new CLanGames(this);
	m_pFriendsGames = new CFriendsGames(this);

	SetMinimumSize( BROWSER_WIDTH, BROWSER_HEIGHT );
	SetSize( BROWSER_WIDTH, BROWSER_HEIGHT );

	m_pGameList = m_pInternetGames;

	m_pContextMenu =  new CServerContextMenu(this);;

	// property sheet
	m_pTabPanel = new PropertySheet(this, "GameTabs");
	m_pTabPanel->SetTabWidth(72);
	m_pTabPanel->AddPage(m_pInternetGames, "#ServerBrowser_InternetTab");
	m_pTabPanel->AddPage(m_pFavorites, "#ServerBrowser_FavoritesTab");
	m_pTabPanel->AddPage(m_pHistory, "#ServerBrowser_HistoryTab");
	m_pTabPanel->AddPage(m_pSpectateGames, "#ServerBrowser_SpectateTab");
	m_pTabPanel->AddPage(m_pLanGames, "#ServerBrowser_LanTab");
	m_pTabPanel->AddPage(m_pFriendsGames, "#ServerBrowser_FriendsTab");
	if ( m_pBlacklist )
	{
		m_pTabPanel->AddPage(m_pBlacklist, "#ServerBrowser_BlacklistTab");
	}
	m_pTabPanel->AddActionSignalTarget(this);

	m_pStatusLabel = new Label(this, "StatusLabel", "");

	LoadControlSettingsAndUserConfig("Servers/DialogServerBrowser.res");

	m_pStatusLabel->SetText("");

	// load current tab
	const char *gameList = m_pSavedData->GetString("GameList");

	if (!Q_stricmp(gameList, "spectate"))
	{
		m_pTabPanel->SetActivePage(m_pSpectateGames);
	}
	else if (!Q_stricmp(gameList, "favorites"))
	{
		m_pTabPanel->SetActivePage(m_pFavorites);
	}
	else if (!Q_stricmp(gameList, "history"))
	{
		m_pTabPanel->SetActivePage(m_pHistory);
	}
	else if (!Q_stricmp(gameList, "lan"))
	{
		m_pTabPanel->SetActivePage(m_pLanGames);
	}
	else if (!Q_stricmp(gameList, "friends"))
	{
		m_pTabPanel->SetActivePage(m_pFriendsGames);
	}
	else if (!Q_stricmp(gameList, "blacklist"))
	{
		m_pTabPanel->SetActivePage(m_pBlacklist);
	}
	else
	{
		m_pTabPanel->SetActivePage(m_pInternetGames);
	}

	m_bActive = false;

	ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServerBrowserDialog::~CServerBrowserDialog()
{
	delete m_pContextMenu;

	SaveUserData();

  	if (m_pSavedData)
  	{
  		m_pSavedData->deleteThis();
  	}
}


//-----------------------------------------------------------------------------
// Purpose: Called once to set up
//-----------------------------------------------------------------------------
void CServerBrowserDialog::Initialize()
{
	SetTitle("#ServerBrowser_Servers", true);
	SetVisible(false);
}


//-----------------------------------------------------------------------------
// Purpose: returns a server in the list
//-----------------------------------------------------------------------------
gameserveritem_t *CServerBrowserDialog::GetServer( unsigned int serverID )
{
	if (m_pGameList)
		return m_pGameList->GetServer( serverID );
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Activates and gives the tab focus
//-----------------------------------------------------------------------------
void CServerBrowserDialog::Open()
{
	BaseClass::Activate();

	if ( !m_bActive )
	{
		ConVarRef cv_vguipanel_active( "vgui_panel_active" );
		cv_vguipanel_active.SetValue( cv_vguipanel_active.GetInt() + 1 );

		ConVarRef cv_server_browser_dialog_open( "server_browser_dialog_open" );
		cv_server_browser_dialog_open.SetValue( true );

		m_bActive = true;
	}

	m_pTabPanel->RequestFocus();
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame, updates animations for this module
//-----------------------------------------------------------------------------
void CServerBrowserDialog::OnTick()
{
	BaseClass::OnTick();
	vgui::GetAnimationController()->UpdateAnimations( system()->GetFrameTime() );
}


//-----------------------------------------------------------------------------
// Purpose: Loads filter settings from disk
//-----------------------------------------------------------------------------
void CServerBrowserDialog::LoadUserData()
{
  	// free any old filters
  	if (m_pSavedData)
  	{
  		m_pSavedData->deleteThis();
  	}

	m_pSavedData = new KeyValues("Filters");
	if ( !m_pSavedData->LoadFromFile( g_pFullFileSystem, "ServerBrowser.vdf", "CONFIG" ) )
	{
		// doesn't matter if the file is not found, defaults will work successfully and file will be created on exit
	}

	KeyValues *filters = m_pSavedData->FindKey( "Filters", false );
	if ( filters )
	{
		if ( !m_pFilterData )
		{
			m_pFilterData = filters->MakeCopy();
		}

		m_pSavedData->RemoveSubKey( filters );
	}
	else
	{
		if ( !m_pFilterData )
		{
			m_pFilterData = new KeyValues( "Filters" );
		}
	}


	// reload all the page settings if necessary
	if (m_pHistory)
	{
		// history
		m_pHistory->LoadHistoryList();
		if ( IsVisible() && m_pHistory->IsVisible() )
			m_pHistory->StartRefresh();
	}

	if (m_pFavorites)
	{
		// favorites
		m_pFavorites->LoadFavoritesList();

		// filters
		ReloadFilterSettings();

		if ( IsVisible() && m_pFavorites->IsVisible() )
			m_pFavorites->StartRefresh();
	}

	if ( m_pBlacklist )
	{
		m_pBlacklist->LoadBlacklistedList();
	}

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::SaveUserData()
{
	m_pSavedData->Clear();
	m_pSavedData->LoadFromFile( g_pFullFileSystem, "ServerBrowser.vdf", "CONFIG");

	// set the current tab
	if (m_pGameList == m_pSpectateGames)
	{
		m_pSavedData->SetString("GameList", "spectate");
	}
	else if (m_pGameList == m_pFavorites)
	{
		m_pSavedData->SetString("GameList", "favorites");
	}
	else if (m_pGameList == m_pLanGames)
	{
		m_pSavedData->SetString("GameList", "lan");
	}
	else if (m_pGameList == m_pFriendsGames)
	{
		m_pSavedData->SetString("GameList", "friends");
	}
	else if (m_pGameList == m_pHistory)
	{
		m_pSavedData->SetString("GameList", "history");
	}
	else
	{
		m_pSavedData->SetString("GameList", "internet");
	}

	m_pSavedData->RemoveSubKey( m_pSavedData->FindKey( "Filters" ) ); // remove the saved subkey and add our subkey
	m_pSavedData->AddSubKey( m_pFilterData->MakeCopy() );
	m_pSavedData->SaveToFile( g_pFullFileSystem, "ServerBrowser.vdf", "CONFIG");

	if ( m_pBlacklist )
	{
		m_pBlacklist->SaveBlacklistedList();
	}

	// save per-page config
	SaveUserConfig();
}

//-----------------------------------------------------------------------------
// Purpose: refreshes the page currently visible
//-----------------------------------------------------------------------------
void CServerBrowserDialog::RefreshCurrentPage()
{
	if (m_pGameList)
	{
		m_pGameList->StartRefresh();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::BlacklistsChanged()
{
	m_pInternetGames->ApplyGameFilters();
}

//-----------------------------------------------------------------------------
// Purpose: Updates status test at bottom of window
//-----------------------------------------------------------------------------
void CServerBrowserDialog::UpdateStatusText(const char *fmt, ...)
{
	if ( !m_pStatusLabel )
		return;

	if ( fmt && strlen(fmt) > 0 )
	{
		char str[ 1024 ];
		va_list argptr;
		va_start( argptr, fmt );
		_vsnprintf( str, sizeof(str), fmt, argptr );
		va_end( argptr );

		m_pStatusLabel->SetText( str );
	}
	else
	{
		// clear
		m_pStatusLabel->SetText( "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates status test at bottom of window
// Input  : wchar_t* (unicode string) - 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::UpdateStatusText(wchar_t *unicode)
{
	if ( !m_pStatusLabel )
		return;

	if ( unicode && wcslen(unicode) > 0 )
	{
		m_pStatusLabel->SetText( unicode );
	}
	else
	{
		// clear
		m_pStatusLabel->SetText( "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::OnGameListChanged()
{
	m_pGameList = dynamic_cast<IGameList *>(m_pTabPanel->GetActivePage());

	UpdateStatusText("");

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a static instance of this dialog
//-----------------------------------------------------------------------------
CServerBrowserDialog *CServerBrowserDialog::GetInstance()
{
	return s_InternetDlg;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a server to the list of favorites
//-----------------------------------------------------------------------------
void CServerBrowserDialog::AddServerToFavorites(gameserveritem_t &server)
{
	if ( steamapicontext->SteamMatchmaking() )
	{
		steamapicontext->SteamMatchmaking()->AddFavoriteGame( 
			server.m_nAppID, 
			server.m_NetAdr.GetIP(), 
			server.m_NetAdr.GetConnectionPort(),		
			server.m_NetAdr.GetQueryPort(), 
			k_unFavoriteFlagFavorite, 
			time( NULL ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a server to our list of blacklisted servers
//-----------------------------------------------------------------------------
void CServerBrowserDialog::AddServerToBlacklist(gameserveritem_t &server)
{
	if ( m_pBlacklist )
	{
		m_pBlacklist->AddServer( server );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CServerBrowserDialog::IsServerBlacklisted(gameserveritem_t &server)
{
	if ( m_pBlacklist )
		return m_pBlacklist->IsServerBlacklisted( server );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CServerContextMenu *CServerBrowserDialog::GetContextMenu(vgui::Panel *pPanel)
{
	// create a drop down for this object's states
	if (m_pContextMenu)
		delete m_pContextMenu;
	m_pContextMenu = new CServerContextMenu(this);
	m_pContextMenu->SetAutoDelete( false );

    if (!pPanel)
    {
	    m_pContextMenu->SetParent(this);
    }
    else
    {
        m_pContextMenu->SetParent(pPanel);
    }

	m_pContextMenu->SetVisible(false);
	return m_pContextMenu;
}

//-----------------------------------------------------------------------------
// Purpose: begins the process of joining a server from a game list
//			the game info dialog it opens will also update the game list
//-----------------------------------------------------------------------------
CDialogGameInfo *CServerBrowserDialog::JoinGame(IGameList *gameList, unsigned int serverIndex, const char* szMatchmakingType )
{
	// open the game info dialog, then mark it to attempt to connect right away
	CDialogGameInfo *gameDialog = OpenGameInfoDialog(gameList, serverIndex);

	// set the dialog name to be the server name
	gameDialog->Connect( szMatchmakingType );

	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: joins a game by a specified IP, not attached to any game list
//-----------------------------------------------------------------------------
CDialogGameInfo *CServerBrowserDialog::JoinGame(int serverIP, int serverPort )
{
	// open the game info dialog, then mark it to attempt to connect right away
	CDialogGameInfo *gameDialog = OpenGameInfoDialog( serverIP, serverPort, serverPort );

	// set the dialog name to be the server name
	gameDialog->Connect( "ServerBrowserJoinByIP" );

	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog from a game list
//-----------------------------------------------------------------------------
CDialogGameInfo *CServerBrowserDialog::OpenGameInfoDialog( IGameList *gameList, unsigned int serverIndex )
{
	gameserveritem_t *pServer = gameList->GetServer( serverIndex );
	if ( !pServer )
		return NULL;

	CDialogGameInfo *gameDialog = new CDialogGameInfo( this, NULL, pServer->m_NetAdr.GetIP(), pServer->m_NetAdr.GetQueryPort(), pServer->m_NetAdr.GetConnectionPort() );
	gameDialog->SetParent(GetVParent());
	gameDialog->AddActionSignalTarget(this);
	gameDialog->Run( pServer->GetName() );
	int i = m_GameInfoDialogs.AddToTail();
	m_GameInfoDialogs[i] = gameDialog;
	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog by a specified IP, not attached to any game list
//-----------------------------------------------------------------------------
CDialogGameInfo *CServerBrowserDialog::OpenGameInfoDialog( int serverIP, uint16 connPort, uint16 queryPort )
{
	CDialogGameInfo *gameDialog = new CDialogGameInfo( this, NULL, serverIP, queryPort, connPort);
	gameDialog->AddActionSignalTarget(this);
	gameDialog->SetParent(GetVParent());
	gameDialog->Run("");
	int i = m_GameInfoDialogs.AddToTail();
	m_GameInfoDialogs[i] = gameDialog;
	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: closes all the game info dialogs
//-----------------------------------------------------------------------------
void CServerBrowserDialog::CloseAllGameInfoDialogs()
{
	for (int i = 0; i < m_GameInfoDialogs.Count(); i++)
	{
		vgui::Panel *dlg = m_GameInfoDialogs[i];
		if (dlg)
		{
			vgui::ivgui()->PostMessage(dlg->GetVPanel(), new KeyValues("Close"), NULL);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: finds a dialog
//-----------------------------------------------------------------------------
CDialogGameInfo *CServerBrowserDialog::GetDialogGameInfoForFriend( uint64 ulSteamIDFriend )
{
	FOR_EACH_VEC( m_GameInfoDialogs, i )
	{
		CDialogGameInfo *pDlg = m_GameInfoDialogs[i];
		if ( pDlg && pDlg->GetAssociatedFriend() == ulSteamIDFriend )
		{
			return pDlg;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: accessor to the filter save data
//-----------------------------------------------------------------------------
KeyValues *CServerBrowserDialog::GetFilterSaveData(const char *filterSet)
{
	return m_pFilterData->FindKey(filterSet, true);
}

//-----------------------------------------------------------------------------
// Purpose: gets the name of the mod directory we're restricted to accessing, NULL if none
//-----------------------------------------------------------------------------
const char *CServerBrowserDialog::GetActiveModName()
{
	return m_szModDir[0] ? m_szModDir : NULL;
}


//-----------------------------------------------------------------------------
// Purpose: gets the name of the mod directory we're restricted to accessing, NULL if none
//-----------------------------------------------------------------------------
const char *CServerBrowserDialog::GetActiveGameName()
{
	return m_szGameName[0] ? m_szGameName : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: return the app id to limit game queries to, set by Source/HL1 engines (NOT by filter settings, that is per page)
//-----------------------------------------------------------------------------
CGameID &CServerBrowserDialog::GetActiveAppID()
{
	return m_iLimitAppID;
}


//-----------------------------------------------------------------------------
// Purpose: receives a specified game is active, so no other game types can be displayed in server list
//-----------------------------------------------------------------------------
void CServerBrowserDialog::OnActiveGameName( KeyValues *pKV )
{
	Q_strncpy(m_szModDir, pKV->GetString( "name" ), sizeof(m_szModDir));
	Q_strncpy(m_szGameName, pKV->GetString( "game" ), sizeof(m_szGameName));
	m_iLimitAppID = CGameID( pKV->GetUint64( "appid", 0 ) );	
	// reload filter settings (since they are no forced to be game specific)
	ReloadFilterSettings();
}

//-----------------------------------------------------------------------------
// Purpose: resets all pages filter settings
//-----------------------------------------------------------------------------
void CServerBrowserDialog::ReloadFilterSettings()
{
	m_pInternetGames->LoadFilterSettings();
	m_pSpectateGames->LoadFilterSettings();
	m_pFavorites->LoadFilterSettings();
	m_pLanGames->LoadFilterSettings();
	m_pFriendsGames->LoadFilterSettings();
	m_pHistory->LoadFilterSettings();
}

//-----------------------------------------------------------------------------
// Purpose: Adds server to the history, saves as currently connected server
//-----------------------------------------------------------------------------
void CServerBrowserDialog::OnConnectToGame( KeyValues *pMessageValues )
{
	int ip = pMessageValues->GetInt( "ip" );
	int connectionPort = pMessageValues->GetInt( "connectionport" );
	int queryPort = pMessageValues->GetInt( "queryport" );

	if ( !ip || !queryPort )
		return;

	uint32 unIP = htonl( ip );

	memset( &m_CurrentConnection, 0, sizeof(gameserveritem_t) );
	m_CurrentConnection.m_NetAdr.SetIP( unIP );
	m_CurrentConnection.m_NetAdr.SetQueryPort( queryPort );
	m_CurrentConnection.m_NetAdr.SetConnectionPort( (unsigned short)connectionPort );

	if (m_pHistory && steamapicontext->SteamMatchmaking() )
	{
		steamapicontext->SteamMatchmaking()->AddFavoriteGame( 0, unIP, connectionPort, queryPort, k_unFavoriteFlagHistory, time( NULL ) );
		m_pHistory->SetRefreshOnReload();
	}

	// tell the game info dialogs, so they can cancel if we have connected
	// to a server they were auto-retrying
	for (int i = 0; i < m_GameInfoDialogs.Count(); i++)
	{
		vgui::Panel *dlg = m_GameInfoDialogs[i];
		if (dlg)
		{
			KeyValues *kv = new KeyValues("ConnectedToGame", "ip", unIP, "connectionport", connectionPort);
			kv->SetInt( "queryport", queryPort );
			vgui::ivgui()->PostMessage(dlg->GetVPanel(), kv, NULL);
		}
	}

	// forward to favorites
	m_pFavorites->OnConnectToGame();
	if ( m_pBlacklist )
	{
		m_pBlacklist->OnConnectToGame();
	}

	m_bCurrentlyConnected = true;

	// Now we want to track which tabs have the quick list button checked
	int iQuickListBitField = 0;
	if ( m_pFriendsGames && m_pFriendsGames->IsQuickListButtonChecked() )
	{
		iQuickListBitField |= ( 1 << 0 );
	}
	if ( m_pLanGames && m_pLanGames->IsQuickListButtonChecked() )
	{
		iQuickListBitField |= ( 1 << 1 );
	}
	if ( m_pSpectateGames && m_pSpectateGames->IsQuickListButtonChecked() )
	{
		iQuickListBitField |= ( 1 << 2 );
	}
	if ( m_pHistory && m_pHistory->IsQuickListButtonChecked() )
	{
		iQuickListBitField |= ( 1 << 3 );
	}
	if ( m_pFavorites && m_pFavorites->IsQuickListButtonChecked() )
	{
		iQuickListBitField |= ( 1 << 4 );
	}
	if ( m_pInternetGames && m_pInternetGames->IsQuickListButtonChecked() )
	{
		iQuickListBitField |= ( 1 << 5 );
	}

	// Set the value so that the client.dll can use it for gamestats
	sb_quick_list_bit_field.SetValue( iQuickListBitField );
}

//-----------------------------------------------------------------------------
// Purpose: Clears currently connected server
//-----------------------------------------------------------------------------
void CServerBrowserDialog::OnDisconnectFromGame( void )
{
	m_bCurrentlyConnected = false;
	memset( &m_CurrentConnection, 0, sizeof(gameserveritem_t) );

	// forward to favorites
	m_pFavorites->OnDisconnectFromGame();
	if ( m_pBlacklist )
	{
		m_pBlacklist->OnDisconnectFromGame();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Switches to specified page
//-----------------------------------------------------------------------------
void CServerBrowserDialog::ShowServerBrowserPage( KeyValues *pMessageValues )
{
	// get the page # from message
	int iPage = pMessageValues->GetInt( "page" );
	if ( iPage < 0 || iPage >= m_pTabPanel->GetNumPages() )
	{
		DevMsg( "Tried to activate invalid server browser tab %d\n", iPage );
		return;
	}
	// switch to that page
	m_pTabPanel->SetActivePage( m_pTabPanel->GetPage( iPage ) );
}

//-----------------------------------------------------------------------------
// Purpose: Passes build mode activation down into the pages
//-----------------------------------------------------------------------------
void CServerBrowserDialog::ActivateBuildMode()
{
	// no subpanel, no build mode
	EditablePanel *panel = dynamic_cast<EditablePanel *>(m_pTabPanel->GetActivePage());
	if (!panel)
		return;

	panel->ActivateBuildMode();
}

//-----------------------------------------------------------------------------
// Purpose: gets the default position and size on the screen to appear the first time
//-----------------------------------------------------------------------------
bool CServerBrowserDialog::GetDefaultScreenPosition(int &x, int &y, int &wide, int &tall)
{

	int screenWide, screenTall;
	surface()->GetScreenSize( screenWide, screenTall );

	x = ( screenWide - BROWSER_WIDTH ) / 2;
	y = ( screenTall - BROWSER_HEIGHT ) / 2;
	wide = BROWSER_WIDTH;
	tall = BROWSER_HEIGHT;
	return true;

}

//-----------------------------------------------------------------------------
// Purpose: Sets a custom scheme
//-----------------------------------------------------------------------------
void CServerBrowserDialog::SetCustomScheme( KeyValues *pMessageValues )
{
	const char *pszScheme = pMessageValues->GetString( "SchemeName", NULL );
	if ( pszScheme )
	{
		char buffer[ MAX_PATH ];
		Q_snprintf( buffer, sizeof( buffer ), "resource/%s.res", pszScheme );
		SetScheme( vgui::scheme()->LoadSchemeFromFile( buffer, pszScheme ) );
		InvalidateLayout( true, true );
	}
}

void CServerBrowserDialog::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( code == KEY_XBUTTON_B || code == KEY_XBUTTON_BACK )
	{
		PostMessage( this, new KeyValues( "Close" ) );
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}

void CServerBrowserDialog::OnKeyCodeTyped( vgui::KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		PostMessage( this, new KeyValues( "Close" ) );
		return;
	}

	BaseClass::OnKeyCodeTyped( code );
}

void CServerBrowserDialog::OnClose()
{
	if ( m_bActive )
	{
		ConVarRef cv_vguipanel_active( "vgui_panel_active" );
		cv_vguipanel_active.SetValue( cv_vguipanel_active.GetInt() - 1 );

		ConVarRef cv_server_browser_dialog_open( "server_browser_dialog_open" );
		cv_server_browser_dialog_open.SetValue( false );

		m_bActive = false;
	}

	BaseClass::OnClose();
}


void CServerBrowserDialog::RunModuleCommand( const char *command )
{
	if ( !V_stricmp( command, "Close" ) )
	{
		OnClose();
	}
}
