﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#include "cbase.h"
#include "cs_bot.h"
#include "cs_shareddefs.h"
#include "mathlib/mathlib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma warning( disable : 4355 )			// warning 'this' used in base member initializer list - we're using it safely

ConVar mp_coopmission_bot_difficulty_offset(
	"mp_coopmission_bot_difficulty_offset",
	"0",
	FCVAR_REPLICATED | FCVAR_RELEASE,
	"The difficulty offset modifier for bots during coop missions." );

//--------------------------------------------------------------------------------------------------------------
static void PrefixChanged( IConVar *c, const char *oldPrefix, float flOldValue )
{
	if ( TheCSBots() && TheCSBots()->IsServerActive() )
	{
		for( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CBasePlayer *player = static_cast<CBasePlayer *>( UTIL_PlayerByIndex( i ) );

			if ( !player )
				continue;

			if ( !player->IsBot() || !IsEntityValid( player ) )
				continue;

			CCSBot *bot = dynamic_cast< CCSBot * >( player );

			if ( !bot )
				continue;

			// set the bot's name
			char botName[MAX_PLAYER_NAME_LENGTH];
			UTIL_ConstructBotNetName( botName, MAX_PLAYER_NAME_LENGTH, bot->GetProfile() );

			engine->SetFakeClientConVarValue( bot->edict(), "name", botName );
		}
	}
}


ConVar cv_bot_traceview( "bot_traceview", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "For internal testing purposes." );
ConVar cv_bot_stop( "bot_stop", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "If nonzero, immediately stops all bot processing." );
ConVar cv_bot_show_nav( "bot_show_nav", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "For internal testing purposes." );
ConVar cv_bot_walk( "bot_walk", "0", FCVAR_REPLICATED, "If nonzero, bots can only walk, not run." );
ConVar cv_bot_difficulty( "bot_difficulty", "1", FCVAR_REPLICATED | FCVAR_RELEASE, "Defines the skill of bots joining the game.  Values are: 0=easy, 1=normal, 2=hard, 3=expert." );
ConVar cv_bot_debug( "bot_debug", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "For internal testing purposes." );
ConVar cv_bot_debug_target( "bot_debug_target", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "For internal testing purposes." );
ConVar cv_bot_quota( "bot_quota", "10", FCVAR_REPLICATED | FCVAR_RELEASE, "Determines the total number of bots in the game." );
ConVar cv_bot_quota_mode( "bot_quota_mode", "normal", FCVAR_REPLICATED | FCVAR_RELEASE, "Determines the type of quota.\nAllowed values: 'normal', 'fill', and 'match'.\nIf 'fill', the server will adjust bots to keep N players in the game, where N is bot_quota.\nIf 'match', the server will maintain a 1:N ratio of humans to bots, where N is bot_quota." );
ConVar cv_bot_prefix( "bot_prefix", "", FCVAR_REPLICATED, "This string is prefixed to the name of all bots that join the game.\n<difficulty> will be replaced with the bot's difficulty.\n<weaponclass> will be replaced with the bot's desired weapon class.\n<skill> will be replaced with a 0-100 representation of the bot's skill.", PrefixChanged );
ConVar cv_bot_allow_rogues( "bot_allow_rogues", "1", FCVAR_RELEASE|FCVAR_REPLICATED, "If nonzero, bots may occasionally go 'rogue'. Rogue bots do not obey radio commands, nor pursue scenario goals." );
ConVar cv_bot_allow_pistols( "bot_allow_pistols", "1", FCVAR_RELEASE|FCVAR_REPLICATED, "If nonzero, bots may use pistols." );
ConVar cv_bot_allow_shotguns( "bot_allow_shotguns", "1", FCVAR_RELEASE|FCVAR_REPLICATED, "If nonzero, bots may use shotguns." );
ConVar cv_bot_allow_sub_machine_guns( "bot_allow_sub_machine_guns", "1", FCVAR_RELEASE|FCVAR_REPLICATED, "If nonzero, bots may use sub-machine guns." );
ConVar cv_bot_allow_rifles( "bot_allow_rifles", "1", FCVAR_RELEASE|FCVAR_REPLICATED, "If nonzero, bots may use rifles." );
ConVar cv_bot_allow_machine_guns( "bot_allow_machine_guns", "1", FCVAR_RELEASE|FCVAR_REPLICATED, "If nonzero, bots may use the machine gun." );
ConVar cv_bot_allow_grenades( "bot_allow_grenades", "1", FCVAR_RELEASE|FCVAR_REPLICATED, "If nonzero, bots may use grenades." );
ConVar cv_bot_allow_snipers( "bot_allow_snipers", "1", FCVAR_RELEASE|FCVAR_REPLICATED, "If nonzero, bots may use sniper rifles." );
#ifdef CS_SHIELD_ENABLED
ConVar cv_bot_allow_shield( "bot_allow_shield", "1", FCVAR_REPLICATED );
#endif // CS_SHIELD_ENABLED
ConVar cv_bot_join_team( "bot_join_team", "any", FCVAR_REPLICATED | FCVAR_RELEASE, "Determines the team bots will join into. Allowed values: 'any', 'T', or 'CT'." );
ConVar cv_bot_join_after_player( "bot_join_after_player", "1", FCVAR_REPLICATED | FCVAR_RELEASE, "If nonzero, bots wait until a player joins before entering the game." );
ConVar cv_bot_auto_vacate( "bot_auto_vacate", "1", FCVAR_REPLICATED, "If nonzero, bots will automatically leave to make room for human players." );
ConVar cv_bot_zombie( "bot_zombie", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "If nonzero, bots will stay in idle mode and not attack." );
ConVar cv_bot_defer_to_human_goals( "bot_defer_to_human_goals", "0", FCVAR_REPLICATED | FCVAR_RELEASE, "If nonzero and there is a human on the team, the bots will not do the scenario tasks." );
ConVar cv_bot_defer_to_human_items( "bot_defer_to_human_items", "1", FCVAR_REPLICATED | FCVAR_RELEASE, "If nonzero and there is a human on the team, the bots will not get scenario items." );
ConVar cv_bot_chatter( "bot_chatter", "normal", FCVAR_REPLICATED | FCVAR_RELEASE, "Control how bots talk. Allowed values: 'off', 'radio', 'minimal', or 'normal'." );
ConVar cv_bot_profile_db( "bot_profile_db", "BotProfile.db", FCVAR_REPLICATED, "The filename from which bot profiles will be read." );
ConVar cv_bot_dont_shoot( "bot_dont_shoot", "0", FCVAR_REPLICATED | FCVAR_RELEASE | FCVAR_CHEAT, "If nonzero, bots will not fire weapons (for debugging)." );
ConVar cv_bot_eco_limit( "bot_eco_limit", "2000", FCVAR_REPLICATED, "If nonzero, bots will not buy if their money falls below this amount." );
ConVar cv_bot_auto_follow( "bot_auto_follow", "0", FCVAR_REPLICATED, "If nonzero, bots with high co-op may automatically follow a nearby human player." );
ConVar cv_bot_flipout( "bot_flipout", "0", FCVAR_REPLICATED, "If nonzero, bots use no CPU for AI. Instead, they run around randomly." );
#if CS_CONTROLLABLE_BOTS_ENABLED
ConVar cv_bot_controllable( "bot_controllable", "1", FCVAR_REPLICATED, "Determines whether bots can be controlled by players" );
#endif

extern void FinishClientPutInServer( CCSPlayer *pPlayer );


//--------------------------------------------------------------------------------------------------------------
// Engine callback for custom server commands
void Bot_ServerCommand( void )
{
}



//--------------------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
CCSBot::CCSBot( void ) :
m_gameState( this ),
m_hasJoined( false ),
m_pLocalProfile( NULL )
{
	if ( CSGameRules()->IsPlayingCoopMission() )
	{
		m_pChatter = new BotChatterCoop( this );
	}
	else
	{
		m_pChatter = new BotChatterInterface( this );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Destructor
 */
CCSBot::~CCSBot()
{
	if ( m_pLocalProfile )
	{
		delete m_pLocalProfile;
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Prepare bot for action
 */
bool CCSBot::Initialize( const BotProfile *profile, int team )
{
	int preserved_voice_pitch = -1;
	char preserved_name[256];
	preserved_name[0] = 0;

	AssertMsg( profile != NULL, "You cannot pass in null for a bot profile." );
	if ( NULL == profile ) return false;

	if ( m_pLocalProfile )
	{
		// Preserve the voice pitch and name from the existing profile
		preserved_voice_pitch = m_pLocalProfile->GetVoicePitch();
		if ( CSGameRules() && CSGameRules()->IsPlayingCooperativeGametype() )
		{
			// change names everytime in coop
			Q_snprintf( preserved_name, 256, "%s", profile->GetName() );
		}
		else
			Q_snprintf( preserved_name, 256, "%s", m_pLocalProfile->GetName() );
		delete m_pLocalProfile;
	}

	m_pLocalProfile = new BotProfile;

	if ( m_pLocalProfile )
	{
		// Copy the profile into the local profile
		m_pLocalProfile->Clone( profile );

		// Restore the name
		if ( Q_strlen( preserved_name ) > 0 )
		{
			m_pLocalProfile->SetName( preserved_name );
		}

		// Restore the voice pitch
		if ( preserved_voice_pitch > -1 )
		{
			m_pLocalProfile->SetVoicePitch( preserved_voice_pitch );
		}
	}

	// extend
	BaseClass::Initialize( m_pLocalProfile, team );

	// CS bot initialization
	m_diedLastRound = false;

	if ( CSGameRules()->IsPlayingGunGameTRBomb() )
	{
		// in demolition, start the CT's off with terrible morale, so they try to camp the bombsite
		if ( team == TEAM_CT )
		{
			m_morale = TERRIBLE;
		}
		else
		{
			m_morale = POSITIVE;			// starting a new round makes everyone a little happy
		}
	}
	else
	{
		m_morale = POSITIVE;			// starting a new round makes everyone a little happy
	}

	m_combatRange = RandomFloat( 325.0f, 425.0f );

	// set initial safe time guess for this map
	m_safeTime = 15.0f + 5.0f * GetProfile()->GetAggression( );

	m_name[0] = '\000';

	ResetValues();

	m_desiredTeam = team;

	if (GetTeamNumber() == 0)
	{
		HandleCommand_JoinTeam( m_desiredTeam );
		// join class is already called within JoinTeam
		//HandleCommand_JoinClass();
	}

	return true;
}

void CCSBot::CoopInitialize( void )
{
	if ( CSGameRules() && CSGameRules()->IsPlayingCoopMission() && GetTeamNumber() == TEAM_TERRORIST )
	{
		// bots start asleep when they spawn and wake up when players come close to them
		SpawnPointCoopEnemy *pEnemySpawnSpot = GetLastCoopSpawnPoint();
		if ( pEnemySpawnSpot )
		{
			SetAbsAngles( pEnemySpawnSpot->GetAbsAngles() );
			int nDifficulty = pEnemySpawnSpot->GetBotDifficulty();
			int nOffset = mp_coopmission_bot_difficulty_offset.GetInt();
			nDifficulty = nDifficulty + nOffset;

			BotDifficultyType botDifficultyType = BOT_EASY;
			if ( nDifficulty >= 6 )
				botDifficultyType = BOT_EXPERT;
			else if ( nDifficulty >= 3 )
				botDifficultyType = BOT_HARD;
			else if ( nDifficulty >= 1 )
				botDifficultyType = BOT_NORMAL;

			bool bIsAgressive = pEnemySpawnSpot->IsBotAgressive(); 

			char profileName[128];
			Q_snprintf( profileName, sizeof( profileName ), "Level_%d%s", ( int )nDifficulty, bIsAgressive ? "_Agressive" : "" );

			const BotProfile* pNewProfileData = TheBotProfiles->GetProfileMatchingTemplate( profileName, TEAM_TERRORIST, botDifficultyType, CSGameRules()->GetMatchDevice(), true );

			if ( pNewProfileData )
			{
				char szHeavy[64] = {};
				int nArmor = pEnemySpawnSpot->GetArmorToSpawnWith();
				if ( nArmor == 1 )
					Q_snprintf( szHeavy, sizeof( szHeavy ), "%s", "" );
				else if ( nArmor == 2 )
					Q_snprintf( szHeavy, sizeof( szHeavy ), "%s", "Heavy " );

				char szName[128] = {};
				char szDifficulty[128] = {};

				if ( nDifficulty >= 6 )
					Q_snprintf( szDifficulty, sizeof( szDifficulty ), "%s", "Elite " );
				else if ( nDifficulty >= 4 )
					Q_snprintf( szDifficulty, sizeof( szDifficulty ), "%s", "Expert " );
				else
					Q_snprintf( szDifficulty, sizeof( szDifficulty ), "%s", "" );

				Q_snprintf( szName, sizeof( szName ), "%s%sPhoenix", szHeavy, szDifficulty );

				Initialize( pNewProfileData, GetTeamNumber() );
				SetPlayerName( szName );
				// have to inform the engine that the bot name has been updated
				engine->SetFakeClientConVarValue( edict(), "name", szName );
			}

			// sleeping needs to be set after the initialize because we reset values in the init
			m_bIsSleeping = pEnemySpawnSpot->ShouldStartAsleep();
		}	
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Reset internal data to initial state
 */
void CCSBot::ResetValues( void )
{
	m_pChatter->Reset();
	m_gameState.Reset();

	m_avoid = NULL;
	m_avoidTimestamp = 0.0f;

	m_hurryTimer.Invalidate();
	m_alertTimer.Invalidate();
	m_sneakTimer.Invalidate();
	m_noiseBendTimer.Invalidate();
	m_bendNoisePositionValid = false;

	m_isStuck = false;
	m_stuckTimestamp = 0.0f;
	m_wiggleTimer.Invalidate();
	m_stuckJumpTimer.Invalidate();
	m_nextCleanupCheckTimestamp = 0.0f;

	m_pathLength = 0;
	m_pathIndex = 0;
	m_areaEnteredTimestamp = 0.0f;
	m_currentArea = NULL;
	m_lastKnownArea = NULL;
	m_isStopping = false;

	m_avoidFriendTimer.Invalidate();
	m_isFriendInTheWay = false;
	m_isWaitingBehindFriend = false;
	m_isAvoidingGrenade.Invalidate();

	StopPanicking();

	m_disposition = ENGAGE_AND_INVESTIGATE;

	m_enemy = NULL;

	m_grenadeTossState = NOT_THROWING;
	m_initialEncounterArea = NULL;

	m_wasSafe = true;

	m_nearbyEnemyCount = 0;
	m_enemyPlace = 0;
	m_nearbyFriendCount = 0;
	m_closestVisibleFriend = NULL;
	m_closestVisibleHumanFriend = NULL;

	for( int w=0; w<MAX_PLAYERS; ++w )
	{
		m_watchInfo[w].timestamp = 0.0f;
		m_watchInfo[w].isEnemy = false;

		m_playerTravelDistance[ w ] = -1.0f;
	}

	// randomly offset each bot's timer to spread computation out
	m_updateTravelDistanceTimer.Start( RandomFloat( 0.0f, 0.9f ) );
	m_travelDistancePhase = 0;

	m_isEnemyVisible = false;
	m_visibleEnemyParts = NONE;
	m_lastSawEnemyTimestamp = -999.9f;
	m_firstSawEnemyTimestamp = 0.0f;
	m_currentEnemyAcquireTimestamp = 0.0f;
	m_isLastEnemyDead = true;
	m_attacker = NULL;
	m_attackedTimestamp = 0.0f;
	m_enemyDeathTimestamp = 0.0f;
	m_friendDeathTimestamp = 0.0f;
	m_lastVictimID = 0;
	m_isAimingAtEnemy = false;
	m_fireWeaponTimestamp = 0.0f;
	m_equipTimer.Invalidate();
	m_zoomTimer.Invalidate();

	m_isFollowing = false;
	m_leader = NULL;
	m_followTimestamp = 0.0f;
	m_allowAutoFollowTime = 0.0f;

	m_enemyQueueIndex = 0;
	m_enemyQueueCount = 0;
	m_enemyQueueAttendIndex = 0;
	m_bomber = NULL;

	m_bIsSleeping = false;

	m_isEnemySniperVisible = false;
	m_sawEnemySniperTimer.Invalidate();

	m_lookAroundStateTimestamp = 0.0f;
	m_inhibitLookAroundTimestamp = 0.0f;

	m_lookPitch = 0.0f;
	m_lookPitchVel = 0.0f;
	m_lookYaw = 0.0f;
	m_lookYawVel = 0.0f;

	m_lookAtSpotState = NOT_LOOKING_AT_SPOT;

	m_targetSpot.Zero();
	m_targetSpotVelocity.Zero();
	m_targetSpotPredicted.Zero();
	m_aimError.Init();
	m_aimGoal.Init();
	m_targetSpotTime = 0.0f;
	m_aimFocus = 0.0f;
	m_aimFocusInterval = 0.0f;
	m_aimFocusNextUpdate = 0.0f;

	for( int p=0; p<MAX_PLAYERS; ++p )
	{
		m_partInfo[p].m_validFrame = 0;
	}

	m_spotEncounter = NULL;
	m_spotCheckTimestamp = 0.0f;
	m_peripheralTimestamp = 0.0f;

	m_avgVelIndex = 0;
	m_avgVelCount = 0;

	m_lastOrigin = GetCentroid( this );

	m_lastRadioCommand = RADIO_INVALID;
	m_lastRadioRecievedTimestamp = 0.0f;
	m_lastRadioSentTimestamp = 0.0f;
	m_radioSubject = NULL;
	m_voiceEndTimestamp = 0.0f;

	m_hostageEscortCount = 0;
	m_hostageEscortCountTimestamp = 0.0f;

	m_noisePosition = Vector( 0, 0, 0 );
	m_noiseTimestamp = 0.0f;

	m_stateTimestamp = 0.0f;
	m_task = SEEK_AND_DESTROY;
	m_taskEntity = NULL;

	m_approachPointCount = 0;
	m_approachPointViewPosition.x = 99999999999.9f;
	m_approachPointViewPosition.y = 0.0f;
	m_approachPointViewPosition.z = 0.0f;

	m_checkedHidingSpotCount = 0;

	StandUp();
	Run();
	m_mustRunTimer.Invalidate();
	m_waitTimer.Invalidate();
	m_pathLadder = NULL;

	m_repathTimer.Invalidate();

	m_huntState.ClearHuntArea();
	m_hasVisitedEnemySpawn = false;
	m_stillTimer.Invalidate();

	// adjust morale - if we died, our morale decreased, 
	// but if we live, no adjustement (round win/loss also adjusts morale)
	if (m_diedLastRound)
		DecreaseMorale();

	m_diedLastRound = false;


	// IsRogue() randomly changes this
	m_isRogue = false;	

	m_surpriseTimer.Invalidate();

	// even though these are EHANDLEs, they need to be NULL-ed
	m_goalEntity = NULL;
	m_avoid = NULL;
	m_enemy = NULL;

	for ( int i=0; i<MAX_ENEMY_QUEUE; ++i )
	{
		m_enemyQueue[i].player = NULL;
		m_enemyQueue[i].isReloading = false;
		m_enemyQueue[i].isProtectedByShield = false;
	}

	m_burnedByFlamesTimer.Invalidate();

#ifdef OPT_VIS_CSGO
	V_memset( m_bVis, 0, sizeof(m_bVis) );
	V_memset( m_aVisParts, 0, sizeof(m_aVisParts) );
#endif

	// start in idle state
	m_isOpeningDoor = false;
	StopAttacking();
	Idle();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Called when bot is placed in map, and when bots are reset after a round ends.
 * NOTE: For some reason, this can be called twice when a bot is added.
 */
void CCSBot::Spawn( void )
{
	if ( CSGameRules() && CSGameRules()->IsPlayingCoopMission() && GetTeamNumber() == TEAM_TERRORIST )
		SetLastCoopSpawnPoint( NULL );

	// do the normal player spawn process
	BaseClass::Spawn();

	ResetValues();

	Buy();

	if ( CSGameRules()->IsPlayingCoopMission() )
	{
		if ( GetTeamNumber() == TEAM_CT )
		{
			SetDisposition( CCSBot::SELF_DEFENSE );

			for ( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CCSPlayer *pPlayer = ToCSPlayer( UTIL_PlayerByIndex( i ) );
				if ( pPlayer && !pPlayer->IsBot() && pPlayer->GetTeamNumber() == TEAM_CT )
				{
					//m_isFollowing = true;
					//m_leader = pPlayer;

					//SetTask( CCSBot::FOLLOW );
					Follow( pPlayer );

					m_lastRadioCommand = RADIO_FOLLOW_ME;
					m_lastRadioRecievedTimestamp = gpGlobals->curtime;
					m_radioSubject = pPlayer;
					m_radioPosition = GetCentroid( pPlayer );		
					break;
				}
			}		
		}
		else if ( GetTeamNumber() == TEAM_TERRORIST )
		{
			CoopInitialize();
		}
	}

	// set the bot name here (after we've had a chance to initialize a new profile
	V_strcpy_safe( m_name, GetPlayerName() );
}

