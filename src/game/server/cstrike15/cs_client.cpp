﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== tf_client.cpp ========================================================

  HL2 client/server game specific stuff

*/

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "shake.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "cs_player.h"
#include "cs_gamerules.h"
#include "cs_bot.h"
#include "tier0/vprof.h"
#include "teamplayroundbased_gamerules.h"
#include "usermessages.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool			g_fGameOver;

extern ConVar mp_maxrounds;

void FinishClientPutInServer( CCSPlayer *pPlayer )
{
	pPlayer->InitialSpawn();
	pPlayer->Spawn();

	if (!pPlayer->IsBot())
	{
		// When the player first joins the server, they
		pPlayer->m_iNumSpawns = 0;
		pPlayer->m_takedamage = DAMAGE_NO;
		pPlayer->pl.deadflag = true;
		pPlayer->m_lifeState = LIFE_DEAD;
		pPlayer->AddEffects( EF_NODRAW );
		pPlayer->ChangeTeam( TEAM_UNASSIGNED );
		// TICK_NEVER_TICK We don't want to force a Team select until after MOTD closes
		pPlayer->SetContextThink( &CBasePlayer::PlayerForceTeamThink, TICK_NEVER_THINK, CS_FORCE_TEAM_THINK_CONTEXT );
		pPlayer->InitializeAccount();

		// Move them to the first intro camera.
		pPlayer->MoveToNextIntroCamera();
		pPlayer->SetMoveType( MOVETYPE_NONE );
	}


	char sName[128];
	Q_strncpy( sName, pPlayer->GetPlayerName(), sizeof( sName ) );
	
	// First parse the name and remove any %'s
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		// Replace it with a space
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	if ( !pPlayer->IsBot() )
	{
		// notify other clients of player joining the game
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[ 0 ] != 0 ? sName : "<unconnected>" );
	}
}

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBaseTFPlayer for pev, and call spawn
	CCSPlayer *pPlayer = CCSPlayer::CreatePlayer( "player", pEdict );

	pPlayer->SetPlayerName( playername );
}


void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in CS!
	Assert( !bLoadGame );

	CCSPlayer *pPlayer = ToCSPlayer( CBaseEntity::Instance( pEdict ) );
	FinishClientPutInServer( pPlayer );

	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();

	// send the 4 end of match conditions.  long frag limit, long max rounds, long rounds needed won, and long time
	CCSUsrMsg_MatchEndConditions msg;
	msg.set_fraglimit( fraglimit.GetInt() );
	msg.set_mp_maxrounds( mp_maxrounds.GetInt() );
	msg.set_mp_winlimit( mp_winlimit.GetInt() );
	msg.set_mp_timelimit( mp_timelimit.GetInt() );
	SendUserMessage( user, CS_UM_MatchEndConditions, msg );

}


/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Counter-Strike: Global Offensive";
}


//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
PRECACHE_REGISTER_BEGIN( GLOBAL, ClientGamePrecache )
	// Materials used by the client effects
	PRECACHE( MODEL, "sprites/white.vmt" );
	PRECACHE( MODEL, "sprites/physbeam.vmt" );

	// Legacy temp ents sounds
	PRECACHE( GAMESOUND, "Bounce.PistolShell" );
	PRECACHE( GAMESOUND, "Bounce.RifleShell" );
	PRECACHE( GAMESOUND, "Bounce.ShotgunShell" );
PRECACHE_REGISTER_END()

void ClientGamePrecache( void )
{
	// Flashbang-related files
	engine->ForceExactFile( "sprites/white.vmt" );
	engine->ForceExactFile( "sprites/white.vtf" );
	engine->ForceExactFile( "vgui/white.vmt" );
	engine->ForceExactFile( "vgui/white.vtf" );
	engine->ForceExactFile( "effects/flashbang.vmt" );
	engine->ForceExactFile( "effects/flashbang_white.vmt" );

	// Smoke grenade-related files
	engine->ForceExactFile( "particle/particle_smokegrenade1.vmt" );
	engine->ForceExactFile( "particle/particle_smokegrenade.vtf" );

	// Sniper scope
	engine->ForceExactFile( "sprites/scope_arc.vmt" );
	engine->ForceExactFile( "sprites/scope_arc.vtf" );

	// DSP presets - don't want people avoiding the deafening + ear ring
	engine->ForceExactFile( "scripts/dsp_presets.txt" );
}


// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			dynamic_cast< CBasePlayer* >( pEdict )->CreateCorpse();
		}

		// respawn player
		pEdict->Spawn();
	}
	else
	{       // restart the entire server
		engine->ServerCommand("reload\n");
	}
}

void GameStartFrame( void )
{
	VPROF( "GameStartFrame" );

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = teamplay.GetBool();
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	CreateGameRulesObject( "CCSGameRules" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void ClientFullyConnect( edict_t *pEntity )
{

}
