﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: steam state machine that handles authenticating steam users
//
//=============================================================================//

#ifndef STEAMUAUTHSERVER_H
#define STEAMUAUTHSERVER_H
#ifdef _WIN32
#pragma once
#endif

#include "baseclient.h"
#include "utlvector.h"
#include "netadr.h"
#include "utlstring.h"

#include "steam/steam_gameserver.h"

class CSteam3Server : public CSteamGameServerAPIContext
{
public:
	CSteam3Server();
	~CSteam3Server();
#if !defined(NO_STEAM)
	STEAM_GAMESERVER_CALLBACK( CSteam3Server, OnLogonSuccess, SteamServersConnected_t, m_CallbackLogonSuccess );
	STEAM_GAMESERVER_CALLBACK( CSteam3Server, OnLogonFailure, SteamServerConnectFailure_t, m_CallbackLogonFailure );
	STEAM_GAMESERVER_CALLBACK( CSteam3Server, OnLoggedOff, SteamServersDisconnected_t, m_CallbackLoggedOff );
	// game server callbacks
	STEAM_GAMESERVER_CALLBACK( CSteam3Server, OnGSPolicyResponse, GSPolicyResponse_t, m_CallbackGSPolicyResponse );
	STEAM_GAMESERVER_CALLBACK( CSteam3Server, OnValidateAuthTicketResponse, ValidateAuthTicketResponse_t, m_CallbackValidateAuthTicketResponse );
#endif

	// CSteam3 stuff
  	void Activate();
	void NotifyOfLevelChange();
	void NotifyOfServerNameChange();
	void SendUpdatedServerDetails();
	void DeactivateAndLogoff();
	void Shutdown();

	bool NotifyClientConnect( CBaseClient *client, uint32 unUserID, const ns_address & adr, const void *pvCookie, uint32 ucbCookie );
	bool NotifyLocalClientConnect( CBaseClient *client );	// Used for local player on listen server and bots.
	void NotifyClientDisconnect( CBaseClient *client );

	void RunFrame();

	bool BSecure() { return SteamGameServer() && SteamGameServer()->BSecure(); }
	bool BIsActive() { return SteamGameServer() && ( m_eServerMode >= eServerModeNoAuthentication ); }
	bool BLanOnly() const { return m_eServerMode == eServerModeNoAuthentication; }
	bool BWantsSecure() { return m_eServerMode == eServerModeAuthenticationAndSecure; }
	bool BLoggedOn() { return SteamGameServer() && SteamGameServer()->BLoggedOn(); }
	bool CompareUserID( const USERID_t & id1, const USERID_t & id2 );
	const CSteamID& GetGSSteamID() const;
	bool BHasLogonResult() const { return m_bLogOnResult; }
	
	uint16 GetQueryPort() const	{ return m_QueryPort; }

	// Fetch public IP.  Might return 0 if we don't know
	uint32 GetPublicIP() { return SteamGameServer() ? SteamGameServer()->GetPublicIP() : 0; }
	
	bool IsMasterServerUpdaterSharingGameSocket();

	/// Select Steam account name / password to use
	void SetAccount( const char *pszToken )
	{
		m_sAccountToken = pszToken;
	}

	/// What account name was selected?
	const char *GetAccountToken() const { return m_sAccountToken.String(); }

private:
	
	bool CheckForDuplicateSteamID( const CBaseClient *client );
	CBaseClient *ClientFindFromSteamID( CSteamID & steamIDFind );
	void OnValidateAuthTicketResponseHelper( CBaseClient *cl, EAuthSessionResponse eAuthSessionResponse );
	void OnInvalidSteamLogonErrorForClient( CBaseClient *cl );
	EServerMode GetCurrentServerMode();

	EServerMode	m_eServerMode;

	bool m_bMasterServerUpdaterSharingGameSocket;
	bool m_bLogOnFinished;
	bool m_bLoggedOn;
	bool		m_bLogOnResult;		// if true, show logon result
	bool		m_bHasActivePlayers;  // player stats updates are only sent if active players are available
	CSteamID m_SteamIDGS;
	CSteamID m_steamIDLanOnly;
	bool m_bActive;
    bool m_bWantsSecure;
    bool m_bInitialized;
    
    // The port that we are listening for queries on.
	uint32		m_unIP;
	uint16		m_usPort;
    uint16		m_QueryPort;
	CUtlString m_sAccountToken;
};

// singleton accessor
CSteam3Server &Steam3Server();


#endif
