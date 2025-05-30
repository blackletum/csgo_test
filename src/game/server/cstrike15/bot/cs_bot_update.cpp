﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#include "cbase.h"
#include "cs_gamerules.h"
#include "cs_bot.h"
#include "fmtstr.h"
#include "cs_simple_hostage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------------------------------------
float CCSBot::GetMoveSpeed( void )
{
	return 250.0f;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Lightweight maintenance, invoked frequently
 */
void CCSBot::Upkeep( void )
{
	VPROF_BUDGET( "CCSBot::Upkeep", VPROF_BUDGETGROUP_NPCS );

	if (TheNavMesh->IsGenerating() || !IsAlive())
		return;
	
	// If bot_flipout is on, then generate some random commands.
	if ( cv_bot_flipout.GetBool() )
	{
		int val = RandomInt( 0, 2 );
		if ( val == 0 )
			MoveForward();
		else if ( val == 1 )
			MoveBackward();
		
		val = RandomInt( 0, 2 );
		if ( val == 0 )
			StrafeLeft();
		else if ( val == 1 )
			StrafeRight();

		if ( RandomInt( 0, 5 ) == 0 )
			Jump( true );
		
		val = RandomInt( 0, 2 );
		if ( val == 0 )
			Crouch();
		else ( val == 1 );
			StandUp();
	
		return;
	}
	
	// BOTPORT: Remove this nasty hack
	m_eyePosition = EyePosition();

	Vector myOrigin = GetCentroid( this );

	if ( m_bIsSleeping )
		return;

	// aiming must be smooth - update often
	if ( IsAimingAtEnemy() )
	{
		if (gpGlobals->curtime >= m_aimFocusNextUpdate)
		{
			PickNewAimSpot();
		}

		UpdateAimPrediction();

		SetLookAngles( m_aimGoal[YAW], m_aimGoal[PITCH] );
	}
	else
	{
		if (m_lookAtSpotClearIfClose)
		{
			// don't look at spots just in front of our face - it causes erratic view rotation
			const float tooCloseRange = 100.0f;
			if ((m_lookAtSpot - myOrigin).IsLengthLessThan( tooCloseRange ))
				m_lookAtSpotState = NOT_LOOKING_AT_SPOT;
		}

		switch( m_lookAtSpotState )
		{
			case NOT_LOOKING_AT_SPOT:
			{
				// look ahead
				SetLookAngles( m_lookAheadAngle, 0.0f );
				break;
			}

			case LOOK_TOWARDS_SPOT:
			{
				UpdateLookAt();
				if (IsLookingAtPosition( m_lookAtSpot, m_lookAtSpotAngleTolerance ))
				{
					m_lookAtSpotState = LOOK_AT_SPOT;
					m_lookAtSpotTimestamp = gpGlobals->curtime;
				}
				break;
			}

			case LOOK_AT_SPOT:
			{
				UpdateLookAt();

				if (m_lookAtSpotDuration >= 0.0f && gpGlobals->curtime - m_lookAtSpotTimestamp > m_lookAtSpotDuration)
				{
					m_lookAtSpotState = NOT_LOOKING_AT_SPOT;
					m_lookAtSpotDuration = 0.0f;
				}
				break;
			}
		}

		// have view "drift" very slowly, so view looks "alive"
		if (!IsUsingSniperRifle())
		{
			float driftAmplitude = 2.0f;
			if (IsBlind())
			{
				driftAmplitude = 5.0f;
			}

			m_lookYaw += driftAmplitude * BotCOS( 33.0f * gpGlobals->curtime );
			m_lookPitch += driftAmplitude * BotSIN( 13.0f * gpGlobals->curtime );
		}
	}

	// view angles can change quickly
	UpdateLookAngles();
}

void CCSBot::CoopUpdateChecks()
{
		// check to see if the bot hasn't seen anyone in a long time and if so, kill them and recycle their spot
	if ( CSGameRules() && CSGameRules()->IsPlayingCoopGuardian() && 
		 IsAlive() && 
		 !CSGameRules()->IsFreezePeriod() &&
		 !CSGameRules()->IsWarmupPeriod() && 
		 !CSGameRules()->IsRoundOver() )
	{
		float flStart = CSGameRules()->GetRoundStartTime();
		if ( m_spawnedTime > flStart )
			flStart = m_spawnedTime;

		// this waits 70 seconds from the last time they spawned or the round start
		// and sees if they've seen an enemy in the last 15 seconds
		// if not, they get removed
		// TODO: put the 70 and 15 on convars
		if ( m_nextCleanupCheckTimestamp < gpGlobals->curtime &&  flStart > 0 
			 && IsEnemyVisible() == false && GetTimeSinceLastSawEnemy() > 20 )
		{
			if ( CSGameRules()->IsHostageRescueMap() && (gpGlobals->curtime - flStart) > 50 
				 && GetTask() != CCSBot::COLLECT_HOSTAGES )
			{
				CBaseEntity *hostage = static_cast<CBaseEntity*>( GetGameState()->GetNearestFreeHostage() );
				if ( hostage )
				{
					// here we want to force them to run for the hostages, no questions asked
					// go get hostages
					SetTask( CCSBot::COLLECT_HOSTAGES );
					ForceRun( 3.0 );
					SetGoalEntity( hostage );
					ResetWaitForHostagePatience();
					SetDisposition( CCSBot::ENGAGE_AND_INVESTIGATE );
					MoveTo( GetCentroid( hostage ), FASTEST_ROUTE );
					PrintIfWatched( "I'm collecting hostages\n" );
				}
			}
			else if ( (gpGlobals->curtime - flStart) > 140 )
			{
				bool bSeen = false;

				int nTeam = CSGameRules()->IsHostageRescueMap() ? TEAM_TERRORIST : TEAM_CT;
				// last check is to make sure a player can't see them
				for ( int i = 1; i <= MAX_PLAYERS; i++ )
				{
					CCSPlayer *pPlayer = ToCSPlayer( UTIL_PlayerByIndex( i ) );
					if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() == nTeam )
					{
						if ( IsVisible( pPlayer->GetAbsOrigin() + Vector( 0, 0, 64 ), false, pPlayer ) )
						{
							bSeen = true;
							break;
						}
					}
				}

				const Vector *vecGoal = GetGameState()->GetRandomFreeHostagePosition();
				if ( CSGameRules()->IsHostageRescueMap() == false )
				{
					int zoneIndex = GetGameState()->GetNextBombsiteToSearch();
					// move to bombsite - if we reach it, we'll update its cleared status, causing us to select another
					vecGoal = TheCSBots()->GetRandomPositionInZone( TheCSBots()->GetZone( zoneIndex ) );
				}

				if ( vecGoal && IsVisible( *vecGoal, false, NULL ) )
				{
					bSeen = true;
				}

				if ( bSeen == false )
					CommitSuicide();
			}

			m_nextCleanupCheckTimestamp = gpGlobals->curtime + 0.5f;
		}

		// Make sure bots know the bomb state in guardian. Don't require them to actually see it.
		if ( CBaseEntity* pBomb = TheCSBots()->GetLooseBomb() )
		{
			GetGameState()->UpdateLooseBomb( pBomb->GetAbsOrigin() );
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Heavyweight processing, invoked less often
 */
void CCSBot::Update( void )
{
	VPROF_BUDGET( "CCSBot::Update", VPROF_BUDGETGROUP_NPCS );

	// If bot_flipout is on, then we only do stuff in Upkeep().
	if ( cv_bot_flipout.GetBool() )
		return;

	Vector myOrigin = GetCentroid( this );

	// if we're sleeping and essentially dormant, check to see if there is a CT near us to wake us up
	if ( m_bIsSleeping && CSGameRules()->IsPlayingCooperativeGametype() )
	{
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CCSPlayer *pPlayer = ToCSPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer && pPlayer->GetTeamNumber() == TEAM_CT )
			{
				Vector vecPlayer = pPlayer->GetAbsOrigin() + Vector( 0, 0, 64 );
				if ( !IsBeyondBotMaxVisionDistance( vecPlayer ) && IsVisible( vecPlayer, false, pPlayer ) )
				{
					// get us going ASAP
					m_bIsSleeping = false;
					m_lookAtSpotState = NOT_LOOKING_AT_SPOT;

					// if we are a heavy, shout a taunt!
					if ( HasHeavyArmor() )
					{
						GetChatter()->DoPhoenixHeavyWakeTaunt();
					}

					break;
				}
			}
		}

		if ( m_bIsSleeping )
			return;
	}

	// if we are spectating, get into the game
	if (GetTeamNumber() == 0)
	{
		HandleCommand_JoinTeam( m_desiredTeam );
		HandleCommand_JoinClass();
		return;
	}


	// update our radio chatter
	// need to allow bots to finish their chatter even if they are dead
	GetChatter()->Update();
	
	// check if we are dead
	if (!IsAlive())
	{
		// remember that we died
		m_diedLastRound = true;

		BotDeathThink();
		return;
	}

	// the bot is alive and in the game at this point
	m_hasJoined = true;

	//
	// Debug beam rendering
	//

	if (cv_bot_debug.GetBool() && IsLocalPlayerWatchingMe())
	{
		DebugDisplay();
	}

	if (cv_bot_stop.GetBool())
		return;

	// check if we are stuck
	StuckCheck();

	// Check for physics props and other breakables in our way that we can break
	BreakablesCheck();

	// Check for useable doors in our way that we need to open
	DoorCheck();

	CoopUpdateChecks();

	// update travel distance to all players (this is an optimization)
	// EDIT: actually this is really slow and only used to detect audible events; we're using straight line dist instead
	//UpdateTravelDistanceToAllPlayers();

	// if our current 'noise' was heard a long time ago, forget it
	const float rememberNoiseDuration = 20.0f;
	if (m_noiseTimestamp > 0.0f && gpGlobals->curtime - m_noiseTimestamp > rememberNoiseDuration)
	{
		ForgetNoise();
	}

	// where are we
	if (!m_currentArea || !m_currentArea->Contains( myOrigin ))
	{
		m_currentArea = (CCSNavArea *)TheNavMesh->GetNavArea( myOrigin );
	}

	// track the last known area we were in
	if (m_currentArea && m_currentArea != m_lastKnownArea)
	{
		m_lastKnownArea = m_currentArea;

		OnEnteredNavArea( m_currentArea );
	}

	// keep track of how long we have been motionless
	const float stillSpeed = 10.0f;
	if (GetAbsVelocity().IsLengthLessThan( stillSpeed ))
	{
		m_stillTimer.Start();
	}
	else
	{
		m_stillTimer.Invalidate();
	}

	// if we're blind, retreat!
	if (IsBlind())
	{
		if (m_blindFire)
		{
			PrimaryAttack();
		}
	}

	UpdatePanicLookAround();

	//
	// Enemy acquisition and attack initiation
	//

	// take a snapshot and update our reaction time queue
	UpdateReactionQueue();

	// "threat" may be the same as our current enemy
	CCSPlayer *threat = GetRecognizedEnemy();
	if (threat)
	{
		SNPROF("(threat)");

		Vector threatOrigin = GetCentroid( threat );

		// adjust our personal "safe" time
		AdjustSafeTime();

		BecomeAlert();

		const float selfDefenseRange = 500.0f; // 750.0f;
		const float farAwayRange = 2000.0f;

		//
		// Decide if we should attack
		//
		bool doAttack = false;
		switch( GetDisposition() )
		{
			case IGNORE_ENEMIES:
			{
				// never attack
				doAttack = false;
				break;
			}

			case SELF_DEFENSE:
			{
				// attack if fired on
				doAttack = (IsPlayerLookingAtMe( threat, 0.99f ) && DidPlayerJustFireWeapon( threat ));

				// attack if enemy very close
				if (!doAttack)
				{
					doAttack = (myOrigin - threatOrigin).IsLengthLessThan( selfDefenseRange );
				}

				if ( CSGameRules()->IsPlayingCoopMission() && IsSniper() )
				{
					// snipers love far away targets
					doAttack = true;
				}

				break;
			}

			case ENGAGE_AND_INVESTIGATE:
			case OPPORTUNITY_FIRE:
			{
				if ((myOrigin - threatOrigin).IsLengthGreaterThan( farAwayRange ))
				{
					// enemy is very far away - wait to take our shot until he is closer
					// unless we are a sniper or he is shooting at us
					if (IsSniper())
					{
						// snipers love far away targets
						doAttack = true;
					}
					else
					{
						// attack if fired on
						doAttack = (IsPlayerLookingAtMe( threat, 0.99f ) && DidPlayerJustFireWeapon( threat ));					
					}
				}
				else
				{
					// normal combat range
					doAttack = true;
				}

				break;
			}
		}

		// if we aren't attacking but we are being attacked, retaliate
		if (!doAttack && !IsAttacking() && GetDisposition() != IGNORE_ENEMIES)
		{
			const float recentAttackDuration = 1.0f;
			if (GetTimeSinceAttacked() < recentAttackDuration)
			{
				// we may not be attacking our attacker, but at least we're not just taking it
				// (since m_attacker isn't reaction-time delayed, we can't directly use it)
				doAttack = true;
				PrintIfWatched( "Ouch! Retaliating!\n" );
			}
		}

		if (doAttack)
		{
			if (!IsAttacking() || threat != GetBotEnemy())
			{
				if (IsUsingKnife() && IsHiding())
				{
					// if hiding with a knife, wait until threat is close
					const float knifeAttackRange = 250.0f;
					if ((GetAbsOrigin() - threat->GetAbsOrigin()).IsLengthLessThan( knifeAttackRange ))
					{
						Attack( threat );
					}
				}
				else
				{
					Attack( threat );
				}
			}
		}
		else
		{
			// dont attack, but keep track of nearby enemies
			SetBotEnemy( threat );
			m_isEnemyVisible = true;
		}

		TheCSBots()->SetLastSeenEnemyTimestamp();
	}

	//
	// Validate existing enemy, if any
	//
	if (m_enemy != NULL)
	{
		SNPROF("Validate existing enemy, if any");

		if (IsAwareOfEnemyDeath())
		{
			// we have noticed that our enemy has died
			m_enemy = NULL;
			m_isEnemyVisible = false;
		}
		else
		{
			// check LOS to current enemy (chest & head), in case he's dead (GetNearestEnemy() only returns live players)
			// note we're not checking FOV - once we've acquired an enemy (which does check FOV), assume we know roughly where he is
			if (IsVisible( m_enemy, false, &m_visibleEnemyParts ))
			{
				m_isEnemyVisible = true;
				m_lastSawEnemyTimestamp = gpGlobals->curtime;
				m_lastEnemyPosition = GetCentroid( m_enemy );
			}
			else
			{
				m_isEnemyVisible = false;

				if ( (gpGlobals->curtime - m_lastSawEnemyTimestamp ) > 1.0f && EquipGrenade() && CSGameRules()->IsPlayingCooperativeGametype() )
				{
					ThrowGrenade( m_lastEnemyPosition );
				}
			}
				
			// check if enemy died
			if (m_enemy->IsAlive())
			{
				m_enemyDeathTimestamp = 0.0f;
				m_isLastEnemyDead = false;
			}
			else if (m_enemyDeathTimestamp == 0.0f)
			{
				// note time of death (to allow bots to overshoot for a time)
				m_enemyDeathTimestamp = gpGlobals->curtime;
				m_isLastEnemyDead = true;
			}
		}
	}
	else
	{
		m_isEnemyVisible = false;
	}


	// if we have seen an enemy recently, keep an eye on him if we can

	{

	SNPROF("1");


	if (!IsBlind() && !IsLookingAtSpot(PRIORITY_UNINTERRUPTABLE) )
	{
		const float seenRecentTime = 3.0f;
		if (m_enemy != NULL && GetTimeSinceLastSawEnemy() < seenRecentTime)
		{
			AimAtEnemy();
		}
		else
		{
			StopAiming();
		}
	}
	else if( IsAimingAtEnemy() )
	{
		StopAiming();
	}

	}


	//
	// Hack to fire while retreating
	/// @todo Encapsulate aiming and firing on enemies separately from current task
	//
	if (GetDisposition() == IGNORE_ENEMIES)
	{
		SNPROF("Fire weapon at enemy");

		FireWeaponAtEnemy();
	}

	{

		SNPROF("2");

	// toss grenades
	LookForGrenadeTargets();

	// process grenade throw state machine
	UpdateGrenadeThrow();

	// avoid enemy grenades
	AvoidEnemyGrenades();

	}


	// check if our weapon is totally out of ammo
	// or if we no longer feel "safe", equip our weapon
	if (!IsSafe() && !IsUsingGrenade() && IsActiveWeaponOutOfAmmo())
	{
		EquipBestWeapon();
	}

	/// @todo This doesn't work if we are restricted to just knives and sniper rifles because we cant use the rifle at close range
	if (!IsSafe() && !IsUsingGrenade() && IsUsingKnife() && !IsEscapingFromBomb())
	{
		SNPROF("3");
		EquipBestWeapon();
	}

	// if we haven't seen an enemy in awhile, and we switched to our pistol during combat, 
	// switch back to our primary weapon (if it still has ammo left)
	const float safeRearmTime = 5.0f;
	if (!IsReloading() && IsUsingPistol() && !IsPrimaryWeaponEmpty() && GetTimeSinceLastSawEnemy() > safeRearmTime)
	{
		SNPROF("4");
		EquipBestWeapon();
	}

	{

	SNPROF("5");
	// reload our weapon if we must
	ReloadCheck();

	// equip silencer
	SilencerCheck();

	// listen to the radio
	RespondToRadioCommands();

	}


	// make way
	const float avoidTime = 0.33f;
	if (gpGlobals->curtime - m_avoidTimestamp < avoidTime && m_avoid != NULL)
	{
		StrafeAwayFromPosition( GetCentroid( m_avoid ) );
	}
	else
	{
		m_avoid = NULL;
	}

	// if we're using a sniper rifle and are no longer attacking, stop looking thru scope
	if (!IsAtHidingSpot() && !IsAttacking() && IsUsingSniperRifle() && IsUsingScope())
	{
		SecondaryAttack();
	}

	if (!IsBlind())
	{
		SNPROF("6");

		// check encounter spots
		UpdatePeripheralVision();

		// watch for snipers
		if (CanSeeSniper() && !HasSeenSniperRecently())
		{
			SNPROF("watch for smipers");

			GetChatter()->SpottedSniper();

			const float sniperRecentInterval = 20.0f;
			m_sawEnemySniperTimer.Start( sniperRecentInterval );
		}

		//
		// Update gamestate
		//
		if (m_bomber != NULL)
			GetChatter()->SpottedBomber( GetBomber() );

		if (CanSeeLooseBomb())
			GetChatter()->SpottedLooseBomb( TheCSBots()->GetLooseBomb() );
	}

	
	// if we're burning, escape from the flames
	if ( GetTimeSinceBurnedByFlames() < 1.0f && !IsEscapingFromFlames() )
	{
		EscapeFromFlames();
		return;
	}


	//
	// Scenario interrupts
	//

	{
		SNPROF("Scenario interrupts");

	switch (TheCSBots()->GetScenario())
	{
		case CCSBotManager::SCENARIO_DEFUSE_BOMB:
		{
			// flee if the bomb is ready to blow and we aren't defusing it or attacking and we know where the bomb is
			// (aggressive players wait until its almost too late)
			float gonnaBlowTime = 8.0f - (2.0f * GetProfile()->GetAggression());

			// if we have a defuse kit, can wait longer
			if (m_bHasDefuser)
				gonnaBlowTime *= 0.66f;

			if (!IsEscapingFromBomb() &&								// we aren't already escaping the bomb
				TheCSBots()->IsBombPlanted() &&							// is the bomb planted
				GetGameState()->IsPlantedBombLocationKnown() &&			// we know where the bomb is
				TheCSBots()->GetBombTimeLeft() < gonnaBlowTime &&		// is the bomb about to explode
				!IsDefusingBomb() &&									// we aren't defusing the bomb
				!IsAttacking())											// we aren't in the midst of a firefight
			{
				EscapeFromBomb();
				break;
			}

			break;
		}

		case CCSBotManager::SCENARIO_RESCUE_HOSTAGES:
		{
			if (GetTeamNumber() == TEAM_CT)
			{
				UpdateHostageEscortCount();
			}
			else
			{
				// Terrorists have imperfect information on status of hostages
				unsigned char status = GetGameState()->ValidateHostagePositions();

				if (status & CSGameState::HOSTAGES_ALL_GONE)
				{
					GetChatter()->HostagesTaken();
					Idle();
				}
				else if (status & CSGameState::HOSTAGE_GONE)
				{
					GetGameState()->HostageWasTaken();
					Idle();
				}
			}
			break;
		}
	}

	}


	//
	// Follow nearby humans if our co-op is high and we have nothing else to do
	// If we were just following someone, don't auto-follow again for a short while to 
	// give us a chance to do something else.
	//
	const float earliestAutoFollowTime = 5.0f;
	const float minAutoFollowTeamwork = 0.4f;
	if (cv_bot_auto_follow.GetBool() &&
		TheCSBots()->GetElapsedRoundTime() > earliestAutoFollowTime &&
		GetProfile()->GetTeamwork() > minAutoFollowTeamwork && 
		CanAutoFollow() &&
		!IsBusy() && 
		!IsFollowing() && 
		!IsBlind() && 
		!GetGameState()->IsAtPlantedBombsite())
	{
		SNPROF("7");

		// chance of following is proportional to teamwork attribute
		if (GetProfile()->GetTeamwork() > RandomFloat( 0.0f, 1.0f ))
		{
			CCSPlayer *leader = GetClosestVisibleHumanFriend();
			if (leader && leader->IsAutoFollowAllowed())
			{
				// count how many bots are already following this player
				const float maxFollowCount = 2;
				if (GetBotFollowCount( leader ) < maxFollowCount)
				{
					const float autoFollowRange = 300.0f;
					Vector leaderOrigin = GetCentroid( leader );
					if ((leaderOrigin - myOrigin).IsLengthLessThan( autoFollowRange ))
					{
						CNavArea *leaderArea = TheNavMesh->GetNavArea( leaderOrigin );
						if (leaderArea)
						{
							PathCost cost( this, FASTEST_ROUTE );
							float travelRange = NavAreaTravelDistance( GetLastKnownArea(), leaderArea, cost );
							if (travelRange >= 0.0f && travelRange < autoFollowRange)
							{
								// follow this human
								Follow( leader );
								PrintIfWatched( "Auto-Following %s\n", leader->GetPlayerName() );

								if (CSGameRules()->IsCareer())
								{
									GetChatter()->Say( "FollowingCommander", 10.0f );
								}
								else
								{
									GetChatter()->Say( "FollowingSir", 10.0f );
								}
							}
						}
					}
				}
			}
		}
		else
		{	
			SNPROF("8");

			// we decided not to follow, don't re-check for a duration
			m_allowAutoFollowTime = gpGlobals->curtime + 15.0f + (1.0f - GetProfile()->GetTeamwork()) * 30.0f;
		}
	}

	if (IsFollowing())
	{
		SNPROF("9");
		// if we are following someone, make sure they are still alive
		CBaseEntity *leader = m_leader;
		if (leader == NULL || !leader->IsAlive())
		{
			StopFollowing();
		}

		// decide whether to continue following them
		const float highTeamwork = 0.85f;
		if (GetProfile()->GetTeamwork() < highTeamwork)
		{
			float minFollowDuration = 15.0f;
			if (GetFollowDuration() > minFollowDuration + 40.0f * GetProfile()->GetTeamwork())
			{
				// we are bored of following our leader
				StopFollowing();
				PrintIfWatched( "Stopping following - bored\n" );
			}
		}
	}


	//
	// Execute state machine
	//

	{
		SNPROF("9b");


	if (m_isOpeningDoor)
	{

		SNPROF("m_isOpeningDoor");
		// opening doors takes precedence over attacking because DoorCheck() will stop opening doors if
		// we don't have a knife out.
		m_openDoorState.OnUpdate( this );

		if (m_openDoorState.IsDone())
		{
			// open door behavior is finished, return to previous behavior context
			m_openDoorState.OnExit( this );
			m_isOpeningDoor = false;
		}
	}
	else if (m_isAttacking)
	{
		SNPROF("m_attackState.OnUpdate");

		m_attackState.OnUpdate( this );
	}
	else
	{
        TM_ZONE_FILTERED(TELEMETRY_LEVEL1, 50, TMZF_NONE, "%s", tmDynamicString(TELEMETRY_LEVEL0, m_state->GetName())); 
		//SNPROF(m_state->GetName());

		m_state->OnUpdate( this );
	}

	// do wait behavior
	if (!IsAttacking() && IsWaiting())
	{
		SNPROF("do wait behavior1");

		ResetStuckMonitor();
		ClearMovement();
	}

	// don't move while reloading unless we see an enemy
	if (IsReloading() && !m_isEnemyVisible)
	{
		SNPROF("do wait behavior2");
		ResetStuckMonitor();
		ClearMovement();
	}

	}

	// if we get too far ahead of the hostages we are escorting, wait for them
	if (!IsAttacking() && m_inhibitWaitingForHostageTimer.IsElapsed())
	{
		SNPROF("10");

		const float waitForHostageRange = 500.0f;
		if ((GetTask() == COLLECT_HOSTAGES || GetTask() == RESCUE_HOSTAGES) && GetRangeToFarthestEscortedHostage() > waitForHostageRange)
		{
			if (!m_isWaitingForHostage)
			{
				// just started waiting
				m_isWaitingForHostage = true;
				m_waitForHostageTimer.Start( 10.0f );
			}
			else
			{
				// we've been waiting
				if (m_waitForHostageTimer.IsElapsed())
				{
					// give up waiting for awhile
					m_isWaitingForHostage = false;
					m_inhibitWaitingForHostageTimer.Start( 3.0f );
				}
				else
				{
					// keep waiting
					ResetStuckMonitor();
					ClearMovement();
				}
			}
		}
	}

	// remember our prior safe time status
	m_wasSafe = IsSafe();
}


//--------------------------------------------------------------------------------------------------------------
class DrawTravelTime 
{
public:
	DrawTravelTime( const CCSBot *me )
	{
		m_me = me;
	}

	bool operator() ( CBasePlayer *player )
	{
		if (player->IsAlive() && !m_me->InSameTeam( player ))
		{
			CFmtStr msg;
			player->EntityText(	0,
								msg.sprintf( "%3.0f", m_me->GetTravelDistanceToPlayer( (CCSPlayer *)player ) ),
								0.1f );


			if (m_me->DidPlayerJustFireWeapon( ToCSPlayer( player ) ))
			{
				player->EntityText( 1, "BANG!", 0.1f );
			}
		}

		return true;
	}

	const CCSBot *m_me;
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Render bot debug info
 */
void CCSBot::DebugDisplay( void ) const
{
	const float duration = 0.15f;
	CFmtStr msg;
	
	NDebugOverlay::ScreenText( 0.5f, 0.34f, msg.sprintf( "Skill: %d%%", (int)(100.0f * GetProfile()->GetSkill()) ), 255, 255, 255, 150, duration );

	if ( m_pathLadder )
	{
		NDebugOverlay::ScreenText( 0.5f, 0.36f, msg.sprintf( "Ladder: %d", m_pathLadder->GetID() ), 255, 255, 255, 150, duration );
	}

	// show safe time
	float safeTime = GetSafeTimeRemaining();
	if (safeTime > 0.0f)
	{
		NDebugOverlay::ScreenText( 0.5f, 0.38f, msg.sprintf( "SafeTime: %3.2f", safeTime ), 255, 255, 255, 150, duration );
	}

	// show if blind
	if (IsBlind())
	{
		NDebugOverlay::ScreenText( 0.5f, 0.38f, msg.sprintf( "<<< BLIND >>>" ), 255, 255, 255, 255, duration );
	}

	// show if alert
	if (IsAlert())
	{
		NDebugOverlay::ScreenText( 0.5f, 0.38f, msg.sprintf( "ALERT" ), 255, 0, 0, 255, duration );
	}

	// show if panicked
	if (IsPanicking())
	{
		NDebugOverlay::ScreenText( 0.5f, 0.36f, msg.sprintf( "PANIC" ), 255, 255, 0, 255, duration );
	}

	// show behavior variables
	if (m_isAttacking)
	{
		NDebugOverlay::ScreenText( 0.5f, 0.4f, msg.sprintf( "ATTACKING: %s", GetBotEnemy()->GetPlayerName() ), 255, 0, 0, 255, duration );
	}
	else
	{
		NDebugOverlay::ScreenText( 0.5f, 0.4f, msg.sprintf( "State: %s", m_state->GetName() ), 255, 255, 0, 255, duration );
		NDebugOverlay::ScreenText( 0.5f, 0.42f, msg.sprintf( "Task: %s", GetTaskName() ), 0, 255, 0, 255, duration );
		NDebugOverlay::ScreenText( 0.5f, 0.44f, msg.sprintf( "Disposition: %s", GetDispositionName() ), 100, 100, 255, 255, duration );
		NDebugOverlay::ScreenText( 0.5f, 0.46f, msg.sprintf( "Morale: %s", GetMoraleName() ), 0, 200, 200, 255, duration );
	}

	// show look at status
	if (m_lookAtSpotState != NOT_LOOKING_AT_SPOT)
	{
		const char *string = msg.sprintf( "LookAt: %s (%s)", m_lookAtDesc, (m_lookAtSpotState == LOOK_TOWARDS_SPOT) ? "LOOK_TOWARDS_SPOT" : "LOOK_AT_SPOT" );

		NDebugOverlay::ScreenText( 0.5f, 0.60f, string, 255, 255, 0, 150, duration );
	}

	NDebugOverlay::ScreenText( 0.5f, 0.62f, msg.sprintf( "Steady view = %s", HasViewBeenSteady( 0.2f ) ? "YES" : "NO" ), 255, 255, 0, 150, duration );


	// show friend/foes I know of
	NDebugOverlay::ScreenText( 0.5f, 0.64f, msg.sprintf( "Nearby friends = %d", m_nearbyFriendCount ), 100, 255, 100, 150, duration );
	NDebugOverlay::ScreenText( 0.5f, 0.66f, msg.sprintf( "Nearby enemies = %d", m_nearbyEnemyCount ), 255, 100, 100, 150, duration );

	if ( m_lastNavArea )
	{
		NDebugOverlay::ScreenText( 0.5f, 0.68f, msg.sprintf( "Nav Area: %d (%s)", m_lastNavArea->GetID(), m_szLastPlaceName.Get() ), 255, 255, 255, 150, duration );
		/*
		if ( cv_bot_traceview.GetBool() )
		{
			if ( m_currentArea )
			{
				NDebugOverlay::Line( GetAbsOrigin(), m_currentArea->GetCenter(), 0, 255, 0, true, 0.1f );
			}
			else if ( m_lastKnownArea )
			{
				NDebugOverlay::Line( GetAbsOrigin(), m_lastKnownArea->GetCenter(), 255, 0, 0, true, 0.1f );
			}
			else if ( m_lastNavArea )
			{
				NDebugOverlay::Line( GetAbsOrigin(), m_lastNavArea->GetCenter(), 0, 0, 255, true, 0.1f );
			}
		}
		*/
	}

	// show debug message history
	float y = 0.8f;
	const float lineHeight = 0.02f;
	const float fadeAge = 7.0f;
	const float maxAge = 10.0f;
	for( int i=0; i<TheBots->GetDebugMessageCount(); ++i )
	{
		const CBotManager::DebugMessage *msg = TheBots->GetDebugMessage( i );

		if (msg->m_age.GetElapsedTime() < maxAge)
		{
			int alpha = 255;

			if (msg->m_age.GetElapsedTime() > fadeAge)
			{
				alpha *= (1.0f - (msg->m_age.GetElapsedTime() - fadeAge) / (maxAge - fadeAge));
			}

			NDebugOverlay::ScreenText( 0.5f, y, msg->m_string, 255, 255, 255, alpha, duration );
			y += lineHeight;
		}
	}

	// show noises
	const Vector *noisePos = GetNoisePosition();
	if (noisePos)
	{
		const float size = 25.0f;
		NDebugOverlay::VertArrow( *noisePos + Vector( 0, 0, size ), *noisePos, size/4.0f, 255, 255, 0, 0, true, duration );
	}

	// show aim spot
	if (IsAimingAtEnemy())
	{
		// since this is executed on the server, we don't have a way of rendering something view relative, 
		// so project out the aim vector to a distance corresponding to the target distance
		Vector toCurrent = m_targetSpot - EyePositionConst();
		float fDistance = toCurrent.Length();
		Vector aimVector;
		AngleVectors( m_aimGoal, &aimVector );
		Vector aimTarget = EyePositionConst() +  aimVector * fDistance;

		NDebugOverlay::Cross3D( aimTarget, 8.0f, 255, 255, 0, true, duration );

/*
		vgui::surface()->DrawSetColor( r, g, b, alpha );
		float fHalfFov = DEG2RAD( pPlayer->GetFOV() ) * 0.5f;
		float fSpreadDistance = ( GetInaccuracy() + GetSpread() ) * 320.0f / tanf( fHalfFov );
		int iSpreadDistance = RoundFloatToInt( YRES( fSpreadDistance ));
		vgui::surface()->DrawFilledRect( x0, y0, x1, y1 );
*/

	}

	if (IsHiding())
	{
		// show approach points
		DrawApproachPoints();
	}
	else
	{
		// show encounter spot data
		if (false && m_spotEncounter)
		{
			NDebugOverlay::Line( m_spotEncounter->path.from, m_spotEncounter->path.to, 0, 150, 150, true, 0.1f );

			Vector dir = m_spotEncounter->path.to - m_spotEncounter->path.from;
			float length = dir.NormalizeInPlace();

			const SpotOrder *order;
			Vector along;

			FOR_EACH_VEC( m_spotEncounter->spots, it )
			{
				order = &m_spotEncounter->spots[ it ];

				// ignore spots the enemy could not have possibly reached yet
				if (order->spot->GetArea())
				{
					if (TheCSBots()->GetElapsedRoundTime() < order->spot->GetArea()->GetEarliestOccupyTime( OtherTeam( GetTeamNumber() ) ))
					{
						continue;
					}
				}

				along = m_spotEncounter->path.from + order->t * length * dir;

				NDebugOverlay::Line( along, order->spot->GetPosition(), 0, 255, 255, true, 0.1f );
			}
		}
	}

	// show aim targets
	if (false)
	{
		CStudioHdr *pStudioHdr = const_cast< CCSBot *>( this )->GetModelPtr();
		if ( !pStudioHdr )
			return;

		mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( const_cast< CCSBot *>( this )->GetHitboxSet() );
		if ( !set )
			return;

		Vector position, forward, right, up;
		QAngle angles;
		char buffer[16];

		for ( int i = 0; i < set->numhitboxes; i++ )
		{
			mstudiobbox_t *pbox = set->pHitbox( i );

			const_cast< CCSBot *>( this )->GetBonePosition( pbox->bone, position, angles );
		
			AngleVectors( angles, &forward, &right, &up );

			NDebugOverlay::BoxAngles( position, pbox->bbmin, pbox->bbmax, angles, 255, 0, 0, 0, 0.1f );

			const float size = 5.0f;
			NDebugOverlay::Line( position, position + size * forward, 255, 255, 0, true, 0.1f );
			NDebugOverlay::Line( position, position + size * right, 255, 0, 0, true, 0.1f );
			NDebugOverlay::Line( position, position + size * up, 0, 255, 0, true, 0.1f );

			Q_snprintf( buffer, 16, "%d", i );
			if (i == 12)
			{
				// in local bone space
				const float headForwardOffset = 4.0f;
				const float headRightOffset = 2.0f;
				position += headForwardOffset * forward + headRightOffset * right;
			}
			NDebugOverlay::Text( position, buffer, true, 0.1f );
		}
	}


	/*
	const QAngle &angles = const_cast< CCSBot *>( this )->GetPunchAngle();
	NDebugOverlay::ScreenText( 0.3f, 0.66f, msg.sprintf( "Punch angle pitch = %3.2f", angles.x ), 255, 255, 0, 150, duration );
	*/

	DrawTravelTime drawTravelTime( this );
	ForEachPlayer( drawTravelTime );

/*
	// show line of fire
	if ((cv_bot_traceview.GetInt() == 100 && IsLocalPlayerWatchingMe()) || cv_bot_traceview.GetInt() == 101)
	{
		if (!IsFriendInLineOfFire())
		{
			Vector vecAiming = GetViewVector();
			Vector vecSrc	 = EyePositionConst();

			if (GetTeamNumber() == TEAM_TERRORIST)
				UTIL_DrawBeamPoints( vecSrc, vecSrc + 2000.0f * vecAiming, 1, 255, 0, 0 );
			else
				UTIL_DrawBeamPoints( vecSrc, vecSrc + 2000.0f * vecAiming, 1, 0, 50, 255 );
		}
	}

	// show path navigation data
	if (cv_bot_traceview.GetInt() == 1 && IsLocalPlayerWatchingMe())
	{
		Vector from = EyePositionConst();

		const float size = 50.0f;
		//Vector arrow( size * cos( m_forwardAngle * M_PI/180.0f ), size * sin( m_forwardAngle * M_PI/180.0f ), 0.0f );
		Vector arrow( size * (float)cos( m_lookAheadAngle * M_PI/180.0f ), 
					  size * (float)sin( m_lookAheadAngle * M_PI/180.0f ), 
					  0.0f );
		
		UTIL_DrawBeamPoints( from, from + arrow, 1, 0, 255, 255 );
	}

	if (cv_bot_show_nav.GetBool() && m_lastKnownArea)
	{
		m_lastKnownArea->DrawConnectedAreas();
	}
	*/


	if (IsAttacking())
	{
		const float crossSize = 2.0f;
		CCSPlayer *enemy = GetBotEnemy();
		if (enemy)
		{
			NDebugOverlay::Cross3D( GetPartPosition( enemy, GUT ), crossSize, 0, 255, 0, true, 0.1f );
			NDebugOverlay::Cross3D( GetPartPosition( enemy, HEAD ), crossSize, 0, 255, 0, true, 0.1f );
			NDebugOverlay::Cross3D( GetPartPosition( enemy, FEET ), crossSize, 0, 255, 0, true, 0.1f );
			NDebugOverlay::Cross3D( GetPartPosition( enemy, LEFT_SIDE ), crossSize, 0, 255, 0, true, 0.1f );
			NDebugOverlay::Cross3D( GetPartPosition( enemy, RIGHT_SIDE ), crossSize, 0, 255, 0, true, 0.1f );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Periodically compute shortest path distance to each player.
 * NOTE: Travel distance is NOT symmetric between players A and B.  Each much be computed separately.
 */
void CCSBot::UpdateTravelDistanceToAllPlayers( void )
{
	SNPROF("UpdateTravelDistanceToAllPlayers");

	const unsigned char numPhases = 3;

	if (m_updateTravelDistanceTimer.IsElapsed())
	{
		ShortestPathCost pathCost;

		for( int i=1; i<=gpGlobals->maxClients; ++i )
		{
			CCSPlayer *player = static_cast< CCSPlayer * >( UTIL_PlayerByIndex( i ) );

			if (player == NULL)
				continue;

			if (FNullEnt( player->edict() ))
				continue;

			if (!player->IsPlayer())
				continue;
			
			if (!player->IsAlive())
				continue;

			// skip friends for efficiency
			if ( !IsOtherEnemy( player ) )
				continue;

			int which = player->entindex() % MAX_PLAYERS;

			// if player is very far away, update every third time (on phase 0)
			const float veryFarAway = 4000.0f;
			if (m_playerTravelDistance[ which ] < 0.0f || m_playerTravelDistance[ which ] > veryFarAway)
			{
				if (m_travelDistancePhase != 0)
					continue;
			}
			else
			{
				// if player is far away, update two out of three times (on phases 1 and 2)
				const float farAway = 2000.0f;
				if (m_playerTravelDistance[ which ] > farAway && m_travelDistancePhase == 0)
					continue;
			}

			// if player is fairly close, update often
			m_playerTravelDistance[ which ] = NavAreaTravelDistance( EyePosition(), player->EyePosition(), pathCost );
		}

		// throttle the computation frequency
		const float checkInterval = 1.0f;
		m_updateTravelDistanceTimer.Start( checkInterval );

		// round-robin the phases
		++m_travelDistancePhase;
		if (m_travelDistancePhase >= numPhases)
		{
			m_travelDistancePhase = 0;
		}
	}
}
