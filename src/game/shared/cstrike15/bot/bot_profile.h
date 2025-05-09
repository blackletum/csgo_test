﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#ifndef _BOT_PROFILE_H_
#define _BOT_PROFILE_H_

#pragma warning( disable : 4786 )	// long STL names get truncated in browse info.

#include "bot_constants.h"
#include "bot_util.h"
#include "cs_weapon_parse.h"

enum
{
	FirstCustomSkin = 100,
	NumCustomSkins = 100,
	LastCustomSkin = FirstCustomSkin + NumCustomSkins - 1,
};

	
//--------------------------------------------------------------------------------------------------------------
/**
 * A BotProfile describes the "personality" of a given bot
 */
class BotProfile
{
public:
	BotProfile( void )
	{
		m_name = NULL;

		m_aggression = 0.0f;
		m_skill = 0.0f;
		m_teamwork = 0.0f;
		m_weaponPreferenceCount = 0;

		m_aimFocusInitial = 0.0f;
		m_aimFocusDecay = 1.0f;
		m_aimFocusOffsetScale = 0.0f;
		m_aimFocusInterval = .5f;

		m_cost = 0;
		m_skin = 0;	
		m_difficultyFlags = 0;
		m_voicePitch = 100;
		m_reactionTime = 0.3f;
		m_attackDelay = 0.0f;
		m_lookAngleMaxAccelNormal = 0.0f;
		m_lookAngleStiffnessNormal = 0.0f;
		m_lookAngleDampingNormal = 0.0f;
		m_lookAngleMaxAccelAttacking = 0.0f;
		m_lookAngleStiffnessAttacking = 0.0f;
		m_lookAngleDampingAttacking = 0.0f;

		m_teams = TEAM_UNASSIGNED;
		m_voiceBank = 0;
		m_prefersSilencer = false;

	}

	~BotProfile( void )
	{
		if ( m_name )
			delete [] m_name;
	}

	const char *GetName( void ) const { return m_name; }		///< return bot's name
	float GetAggression() const;
	float GetSkill() const;
	float GetTeamwork() const;

	float GetAimFocusInitial() const									{ return m_aimFocusInitial; }
	float GetAimFocusDecay() const										{ return m_aimFocusDecay; }
	float GetAimFocusOffsetScale() const								{ return m_aimFocusOffsetScale; }
	float GetAimFocusInterval() const									{ return m_aimFocusInterval; }

	CSWeaponID GetWeaponPreference(int i ) const;
	const char *GetWeaponPreferenceAsString( int i ) const;
	int GetWeaponPreferenceCount() const;
	bool HasPrimaryPreference() const;				///< return true if this profile has a primary weapon preference
	bool HasPistolPreference() const;				///< return true if this profile has a pistol weapon preference

	int GetCost() const;
	int GetSkin() const;
	int GetMaxDifficulty() const;					///< return maximum difficulty flag value
	bool IsDifficulty( BotDifficultyType diff ) const;	///< return true if this profile can be used for the given difficulty level
	bool IsMaxDifficulty( BotDifficultyType diff ) const;	///< return true if this profile's highest difficulty matches the incoming difficulty level
	int GetVoicePitch() const;
	float GetReactionTime() const;
	float GetAttackDelay() const;
	int GetVoiceBank() const												{ return m_voiceBank; }
	unsigned char GetDifficultyFlags() const;
	int GetTeams( void ) const												{ return m_teams; }

	float GetLookAngleMaxAccelerationNormal() const;
	float GetLookAngleStiffnessNormal() const;
	float GetLookAngleDampingNormal() const;
	float GetLookAngleMaxAccelerationAttacking() const;
	float GetLookAngleStiffnessAttacking() const;
	float GetLookAngleDampingAttacking() const;

	bool IsValidForTeam( int team ) const;

	bool PrefersSilencer() const								{ return m_prefersSilencer; }

	bool InheritsFrom( const char *name ) const;

	void Clone( const BotProfile* source_profile );
	void SetName( const char* name );
	void SetVoicePitch( int pitch );
	int GetTemplatesCount( void ) const							{ return m_templates.Count(); }
	const BotProfile* GetTemplate( int index ) const;

private:
	friend class BotProfileManager;						///< for loading profiles

	void Inherit( const BotProfile *parent, const BotProfile *baseline );	///< copy values from parent if they differ from baseline

	char *m_name;							// the bot's name
	float m_aggression;						// percentage: 0 = coward, 1 = berserker
	float m_skill;							// percentage: 0 = terrible, 1 = expert
	float m_teamwork;						// percentage: 0 = rogue, 1 = complete obeyance to team, lots of comm

	float m_aimFocusInitial;				// initial minimum aim error on first attack
	float m_aimFocusDecay;					// how quickly our focus error decays (scale/sec)
	float m_aimFocusOffsetScale;			// how much aim focus error we get based on maximum angle distance from our view angle
	float m_aimFocusInterval;				// how frequently we update our focus

	enum { MAX_WEAPON_PREFS = 16 };
	CSWeaponID m_weaponPreference[ MAX_WEAPON_PREFS ];	///< which weapons this bot likes to use, in order of priority
	int m_weaponPreferenceCount;

	int m_cost;								///< reputation point cost for career mode
	int m_skin;								///< "skin" index
	unsigned char m_difficultyFlags;		///< bits set correspond to difficulty levels this is valid for
	int m_voicePitch;						///< the pitch shift for bot chatter (100 = normal)
	float m_reactionTime;					///< our reaction time in seconds
	float m_attackDelay;					///< time in seconds from when we notice an enemy to when we open fire
	int m_teams;							///< teams for which this profile is valid

	bool m_prefersSilencer;					///< does the bot prefer to use silencers?

	int m_voiceBank;						///< Index of the BotChatter.db voice bank this profile uses (0 is the default)

	float m_lookAngleMaxAccelNormal;		// Acceleration of look angle spring under normal conditions
	float m_lookAngleStiffnessNormal;		// Stiffness of look angle spring under normal conditions
	float m_lookAngleDampingNormal;			// Damping of look angle spring under normal conditions

	float m_lookAngleMaxAccelAttacking;		// Acceleration of look angle spring under attack conditions
	float m_lookAngleStiffnessAttacking;	// Stiffness of look angle spring under attack conditions
	float m_lookAngleDampingAttacking;		// Damping of look angle spring under attack conditions

	CUtlVector< const BotProfile * > m_templates;							///< List of templates we inherit from
};
typedef CUtlLinkedList<BotProfile *> BotProfileList;


inline int BotProfile::GetMaxDifficulty() const
{
	for ( int i = NUM_DIFFICULTY_LEVELS - 1; i >= BOT_EASY; --i )
	{
		if ( m_difficultyFlags & ( 1 << i ) )
		{
			return i;
		}
	}

	return BOT_EASY;
}

inline bool BotProfile::IsDifficulty( BotDifficultyType diff ) const
{
	return (m_difficultyFlags & (1 << diff)) ? true : false;
}

inline bool BotProfile::IsMaxDifficulty( BotDifficultyType diff ) const
{
	return ( ( m_difficultyFlags >> diff ) == 1 );
}

inline float BotProfile::GetAggression() const
{
	return m_aggression;
}

inline float BotProfile::GetSkill() const
{
	return m_skill;
}

inline float BotProfile::GetTeamwork() const
{
	return m_teamwork;
}

inline CSWeaponID BotProfile::GetWeaponPreference( int i ) const
{
	return m_weaponPreference[ i ];
}

inline int BotProfile::GetWeaponPreferenceCount() const
{
	return m_weaponPreferenceCount;
}

inline int BotProfile::GetCost() const
{
	return m_cost;
}

inline int BotProfile::GetSkin() const
{
	return m_skin;
}

inline int BotProfile::GetVoicePitch() const
{
	return m_voicePitch;
}

inline float BotProfile::GetReactionTime() const
{
	return m_reactionTime;
}

inline float BotProfile::GetAttackDelay() const
{
	return m_attackDelay;
}

inline unsigned char BotProfile::GetDifficultyFlags() const
{
	return m_difficultyFlags;
}

inline float BotProfile::GetLookAngleMaxAccelerationNormal() const
{
	return m_lookAngleMaxAccelNormal;
}

inline float BotProfile::GetLookAngleStiffnessNormal( ) const
{
	return m_lookAngleStiffnessNormal;
}

inline float BotProfile::GetLookAngleDampingNormal() const
{
	return m_lookAngleDampingNormal;
}

inline float BotProfile::GetLookAngleMaxAccelerationAttacking() const
{
	return m_lookAngleMaxAccelAttacking;
}

inline float BotProfile::GetLookAngleStiffnessAttacking() const
{
	return m_lookAngleStiffnessAttacking;
}

inline float BotProfile::GetLookAngleDampingAttacking() const
{
	return m_lookAngleDampingAttacking;
}

inline void BotProfile::SetVoicePitch( int pitch )
{
	m_voicePitch = pitch;
}

/**
 * Copy in data from parent if it differs from the baseline
 */
inline void BotProfile::Inherit( const BotProfile *parent, const BotProfile *baseline )
{
	if ( parent->m_aggression != baseline->m_aggression )
		m_aggression = parent->m_aggression;

	if ( parent->m_skill != baseline->m_skill )
		m_skill = parent->m_skill;

	if ( parent->m_teamwork != baseline->m_teamwork )
		m_teamwork = parent->m_teamwork;

	if ( parent->m_aimFocusInitial != baseline->m_aimFocusInitial )
		m_aimFocusInitial = parent->m_aimFocusInitial;

	if ( parent->m_aimFocusDecay != baseline->m_aimFocusDecay )
		m_aimFocusDecay = parent->m_aimFocusDecay;

	if ( parent->m_aimFocusOffsetScale != baseline->m_aimFocusOffsetScale )
		m_aimFocusOffsetScale = parent->m_aimFocusOffsetScale;

	if ( parent->m_aimFocusInterval != baseline->m_aimFocusInterval )
		m_aimFocusInterval = parent->m_aimFocusInterval;

	if (parent->m_weaponPreferenceCount != baseline->m_weaponPreferenceCount)
	{
		m_weaponPreferenceCount = parent->m_weaponPreferenceCount;
		for( int i=0; i<parent->m_weaponPreferenceCount; ++i )
			m_weaponPreference[i] = parent->m_weaponPreference[i];
	}

	if (parent->m_cost != baseline->m_cost)
		m_cost = parent->m_cost;

	if (parent->m_skin != baseline->m_skin)
		m_skin = parent->m_skin;

	if (parent->m_difficultyFlags != baseline->m_difficultyFlags)
		m_difficultyFlags = parent->m_difficultyFlags;

	if (parent->m_voicePitch != baseline->m_voicePitch)
		m_voicePitch = parent->m_voicePitch;

	if (parent->m_reactionTime != baseline->m_reactionTime)
		m_reactionTime = parent->m_reactionTime;

	if (parent->m_attackDelay != baseline->m_attackDelay)
		m_attackDelay = parent->m_attackDelay;

	if (parent->m_teams != baseline->m_teams)
		m_teams = parent->m_teams;

	if (parent->m_voiceBank != baseline->m_voiceBank)
		m_voiceBank = parent->m_voiceBank;

	m_templates.AddToTail( parent );
}



//--------------------------------------------------------------------------------------------------------------
/**
 * The BotProfileManager defines the interface to accessing BotProfiles
 */
class BotProfileManager
{
public:
	BotProfileManager( void );
	~BotProfileManager( void );

	void Init( const char *filename, unsigned int *checksum = NULL );
	void Reset( void );

	/// given a name, return a profile
	const BotProfile *GetProfile( const char *name, int team ) const
	{
		FOR_EACH_LL( m_profileList, it )
		{
			BotProfile *profile = m_profileList[ it ];
	
			if ( !stricmp( name, profile->GetName() ) && profile->IsValidForTeam( team ) )
				return profile;
		}

		return NULL;
	}

	/// given a template name and difficulty, return a profile
	const BotProfile *GetProfileMatchingTemplate( const char *profileName, int team, BotDifficultyType difficulty, BotProfileDevice_t device, bool bAllowDupeNames = false ) const
	{
		FOR_EACH_LL( m_profileList, it )
		{
			BotProfile *profile = m_profileList[ it ];

			if ( !profile->InheritsFrom( profileName ) )
				continue;

			if ( !profile->IsValidForTeam( team ) )
				continue;

			if ( !profile->IsDifficulty( difficulty ) )
				continue;

			if ( bAllowDupeNames == false && UTIL_IsNameTaken( profile->GetName() ) )
				continue;

			return profile;
		}

		return NULL;
	}

	const BotProfileList *GetProfileList( void ) const		{ return &m_profileList; }		///< return list of all profiles

	const BotProfile *GetRandomProfile( BotDifficultyType difficulty, int team, CSWeaponType weaponType, bool forceMatchHighestDifficulty = false ) const;			///< return random unused profile that matches the given difficulty level

	const char * GetCustomSkin( int index );				///< Returns custom skin name at a particular index
	const char * GetCustomSkinModelname( int index );		///< Returns custom skin modelname at a particular index
	const char * GetCustomSkinFname( int index );			///< Returns custom skin filename at a particular index
	int GetCustomSkinIndex( const char *name, const char *filename = NULL );	///< Looks up a custom skin index by name

	typedef CUtlVector<char *> VoiceBankList;
	const VoiceBankList *GetVoiceBanks( void ) const		{ return &m_voiceBanks; }
	int FindVoiceBankIndex( const char *filename );		///< return index of the (custom) bot phrase db, inserting it if needed

protected:
	void ParseDifficultySetting( unsigned char &difficultyFlags, char* token);
	void ParseWeaponPreference( bool &isFirstWeaponPref, int &weaponPreferenceCount, CSWeaponID* weaponPreference, char* token );

protected:
	BotProfileList m_profileList;							///< the list of all bot profiles
	BotProfileList m_templateList;							///< the list of all bot templates

	VoiceBankList m_voiceBanks;

	char *m_skins[ NumCustomSkins ];						///< Custom skin names
	char *m_skinModelnames[ NumCustomSkins ];				///< Custom skin modelnames
	char *m_skinFilenames[ NumCustomSkins ];				///< Custom skin filenames
	int m_nextSkin;											///< Next custom skin to allocate
};

/// the global singleton for accessing BotProfiles
extern BotProfileManager *TheBotProfiles;


#endif // _BOT_PROFILE_H_
