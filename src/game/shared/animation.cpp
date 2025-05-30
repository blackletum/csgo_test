﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "studio.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"
#include "ai_activity.h"
#include "animation.h"
#include "bone_setup.h"
#include "scriptevent.h"
#include "npcevent.h"
#include "eventlist.h"
#include "tier0/vprof.h"

#if !defined( CLIENT_DLL ) && !defined( MAKEXVCD )
#include "util.h"
#include "enginecallback.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma warning( disable : 4244 )
#define iabs(i) (( (i) >= 0 ) ? (i) : -(i) )

int ExtractBbox( CStudioHdr *pstudiohdr, int sequence, Vector& mins, Vector& maxs )
{
	if (! pstudiohdr)
		return 0;

	if (!pstudiohdr->SequencesAvailable())
		return 0;

	mstudioseqdesc_t	&seqdesc = pstudiohdr->pSeqdesc( sequence );
	
	mins = seqdesc.bbmin;

	maxs = seqdesc.bbmax;

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : *pstudiohdr - 
//			iSequence - 
//
// Output : mstudioseqdesc_t
//-----------------------------------------------------------------------------

extern int g_nActivityListVersion;
extern int g_nEventListVersion;

void SetEventIndexForSequence( mstudioseqdesc_t &seqdesc )
{
	if ( &seqdesc == NULL )
		 return;

	if ( seqdesc.numevents == 0 )
		 return;

#ifndef CLIENT_DLL
	seqdesc.flags |= STUDIO_EVENT;
#else
	seqdesc.flags |= STUDIO_EVENT_CLIENT;
#endif

	for ( int index = 0; index < (int)seqdesc.numevents; index++ )
	{
		mstudioevent_t *pevent = (mstudioevent_for_client_server_t*)seqdesc.pEvent( index );

		if ( !pevent )
			 continue;

		if ( pevent->type & AE_TYPE_NEWEVENTSYSTEM )
		{
			const char *pEventName = pevent->pszEventName();
			
			int iEventIndex = EventList_IndexForName( pEventName );
				
			if ( iEventIndex == -1 )
			{
				pevent->event_newsystem = EventList_RegisterPrivateEvent( pEventName );
			}
			else
			{
				pevent->event_newsystem = iEventIndex;
				pevent->type |= EventList_GetEventType( iEventIndex );
			}
		}
	}
}

mstudioevent_for_client_server_t *GetEventIndexForSequence( mstudioseqdesc_t &seqdesc )
{
#ifndef CLIENT_DLL
	if ( !(seqdesc.flags & STUDIO_EVENT) )
#else
	if ( !(seqdesc.flags & STUDIO_EVENT_CLIENT) )
#endif
	{
		SetEventIndexForSequence( seqdesc );
	}

	return (mstudioevent_for_client_server_t*)seqdesc.pEvent( 0 );
}


void BuildAllAnimationEventIndexes( CStudioHdr *pstudiohdr )
{
	if ( !pstudiohdr )
		return;

	if( pstudiohdr->GetEventListVersion() != g_nEventListVersion )
	{
		for ( int i = 0 ; i < pstudiohdr->GetNumSeq() ; i++ )
		{
			SetEventIndexForSequence( pstudiohdr->pSeqdesc( i ) );
		}

		pstudiohdr->SetEventListVersion( g_nEventListVersion );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ensures that activity / index relationship is recalculated
// Input  :
// Output :
//-----------------------------------------------------------------------------
void ResetEventIndexes( CStudioHdr *pstudiohdr )
{	
	if (! pstudiohdr)
		return;

	pstudiohdr->SetEventListVersion( g_nEventListVersion - 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void SetActivityForSequence( CStudioHdr *pstudiohdr, int i )
{
	int iActivityIndex;
	const char *pszActivityName;
	mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( i );

	seqdesc.flags |= STUDIO_ACTIVITY;

	pszActivityName = GetSequenceActivityName( pstudiohdr, i );
	if ( pszActivityName[0] != '\0' )
	{
		iActivityIndex = ActivityList_IndexForName( pszActivityName );
		
		if ( iActivityIndex == -1 )
		{
			// Allow this now.  Animators can create custom activities that are referenced only on the client or by scripts, etc.
			//Warning( "***\nModel %s tried to reference unregistered activity: %s \n***\n", pstudiohdr->name, pszActivityName );
			//Assert(0);
			// HACK: the client and server don't share the private activity list so registering it on the client would hose the server
#ifdef CLIENT_DLL
			seqdesc.flags &= ~STUDIO_ACTIVITY;
#else
			seqdesc.activity = ActivityList_RegisterPrivateActivity( pszActivityName );
#endif
		}
		else
		{
			seqdesc.activity = iActivityIndex;
		}
	}
}

//=========================================================
// IndexModelSequences - set activity and event indexes for all model
// sequences that have them.
//=========================================================

void IndexModelSequences( CStudioHdr *pstudiohdr )
{
	VPROF_2( "IndexModelSequences",  IsServerDll() ? VPROF_BUDGETGROUP_SERVER_ANIM : VPROF_BUDGETGROUP_CLIENT_ANIMATION, false, BUDGETFLAG_ALL );
	int i;

	if (! pstudiohdr)
		return;

	if (!pstudiohdr->SequencesAvailable())
		return;

	for ( i = 0 ; i < pstudiohdr->GetNumSeq() ; i++ )
	{
		SetActivityForSequence( pstudiohdr, i );
		SetEventIndexForSequence( pstudiohdr->pSeqdesc( i ) );
	}

	pstudiohdr->SetActivityListVersion( g_nActivityListVersion );
}

//-----------------------------------------------------------------------------
// Purpose: Ensures that activity / index relationship is recalculated
// Input  :
// Output :
//-----------------------------------------------------------------------------
void ResetActivityIndexes( CStudioHdr *pstudiohdr )
{	
	if (! pstudiohdr)
		return;

	pstudiohdr->SetActivityListVersion( g_nActivityListVersion - 1 );
}

#ifdef _DEBUG
// used to test a reent change to verifysequenceindex below: can the client count on the server's
// activity numbers?
bool DebugValidateActivityIndexes( CStudioHdr *pstudiohdr ) 
{
	bool bGood = true;
	int i;

	if (! pstudiohdr)
		return bGood;

	if (!pstudiohdr->SequencesAvailable())
		return bGood;

	for ( i = 0 ; i < pstudiohdr->GetNumSeq() ; i++ )
	{
		mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( i );

		// test SetActivityForSequence( pstudiohdr, i );
		{ 
			const char *pszActivityName;

			pszActivityName = GetSequenceActivityName( pstudiohdr, i );
			if ( pszActivityName[0] != '\0' )
			{
				int iActIdx = ActivityList_IndexForName( pszActivityName );
				AssertMsg1( iActIdx == seqdesc.activity, "Client and server sequence activity index numbers differ for %s", pstudiohdr->pszName() );
				bGood = bGood && iActIdx == seqdesc.activity;
			}
		}

		// test 			SetEventIndexForSequence( pstudiohdr->pSeqdesc( i ) );
		{	
			if ( &seqdesc == NULL )
				return bGood;

			if ( seqdesc.numevents == 0 )
				return bGood;

			for ( int index = 0; index < (int)seqdesc.numevents; index++ )
			{
				mstudioevent_t *pevent = (mstudioevent_for_client_server_t*)seqdesc.pEvent( index );

				if ( !pevent )
					continue;

				if ( pevent->type & AE_TYPE_NEWEVENTSYSTEM )
				{
					const char *pEventName = pevent->pszEventName();

					int iEventIndex = EventList_IndexForName( pEventName );

					AssertMsg1( iEventIndex == pevent->event_newsystem, "Client and server sequence event index numbers differ for %s", pstudiohdr->pszName() );
					bGood = bGood && iEventIndex == pevent->event_newsystem;
				}
			}
		}
	}
	return bGood;
}
#endif

void VerifySequenceIndex( CStudioHdr *pstudiohdr )
{
	if ( !pstudiohdr )
	{
		return;
	}

	if( pstudiohdr->GetActivityListVersion( ) < g_nActivityListVersion ) // sometimes the server's numbers can get ahead of the client's if we're sharing memory between the two, so it's only necessary to reindex if a model is lagging the version number.
	{
		// this model's sequences have not yet been indexed by activity
		IndexModelSequences( pstudiohdr );
	}
#if 0
	else if ( pstudiohdr->GetActivityListVersion( ) != g_nActivityListVersion )
	{
		if ( !DebugValidateActivityIndexes( pstudiohdr ) )
		{
			Warning( "Client and server activity index numbers differ for %s\n", pstudiohdr->pszName() );
		}
	}
#endif
}



#if !defined( MAKEXVCD )
bool IsInPrediction()
{
	return CBaseEntity::GetPredictionPlayer() != NULL;
}

int SelectWeightedSequence( CStudioHdr *pstudiohdr, int activity, int curSequence )
{
	VPROF( "SelectWeightedSequence" );
#ifdef CLIENT_DLL
	VPROF_INCREMENT_COUNTER( "Client SelectWeightedSequence", 1 );
#else // ifdef GAME_DLL
	VPROF_INCREMENT_COUNTER( "Server SelectWeightedSequence", 1 );
#endif

	if (! pstudiohdr)
		return 0;

	if (!pstudiohdr->SequencesAvailable())
		return 0;

	VerifySequenceIndex( pstudiohdr );

	int numSeq = pstudiohdr->GetNumSeq();
	if ( numSeq == 1 )
	{
		return ( GetSequenceActivity( pstudiohdr, 0, NULL ) == activity ) ? 0 : ACTIVITY_NOT_AVAILABLE;
	}

	return pstudiohdr->SelectWeightedSequence( activity, curSequence );
}


// Pick a sequence for the given activity. If the current sequence is appropriate for the 
// current activity, and its stored weight is negative (whatever that means), always select
// it. Otherwise perform a weighted selection -- imagine a large roulette wheel, with each
// sequence having a number of spaces corresponding to its weight.
int CStudioHdr::CActivityToSequenceMapping::SelectWeightedSequence( CStudioHdr *pstudiohdr, int activity, int curSequence )
{
	// is the current sequence appropriate?
	if (curSequence >= 0)
	{
		mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( curSequence );

		if (seqdesc.activity == activity && seqdesc.actweight < 0)
			return curSequence;
	}

	if ( !pstudiohdr->SequencesAvailable() )
	{
		return ACTIVITY_NOT_AVAILABLE;
	}

	if ( pstudiohdr->GetNumSeq() == 1 )
	{
		AssertMsg( 0, "Expected single sequence case to be handled in ::SelectWeightedSequence()" );
		return ACTIVITY_NOT_AVAILABLE;
	}

	if (!ValidateAgainst(pstudiohdr))
	{
		AssertMsg1(false, "CStudioHdr %s has changed its vmodel pointer without reinitializing its activity mapping! Now performing emergency reinitialization.", pstudiohdr->pszName());
		ExecuteOnce(DebuggerBreakIfDebugging());
		Reinitialize(pstudiohdr);
	}

	// a null m_pSequenceTuples just means that this studio header has no activities.
	if (!m_pSequenceTuples)
		return ACTIVITY_NOT_AVAILABLE;

	// get the data for the given activity
	HashValueType dummy( activity, 0, 0, 0 );
	UtlHashHandle_t handle = m_ActToSeqHash.Find(dummy);
	if (!m_ActToSeqHash.IsValidHandle(handle))
	{
		return ACTIVITY_NOT_AVAILABLE;
	}
	const HashValueType * __restrict actData = &m_ActToSeqHash[handle];

	AssertMsg2( actData->totalWeight > 0, "Activity %d has total weight of %d!",
		activity, actData->totalWeight );
	int weighttotal = actData->totalWeight;

	// failsafe if the weight is 0: assume the artist screwed up and that the first sequence
	// for this activity should be returned.
	int randomValue = 0;
	if ( actData->totalWeight <= 0 )
	{
		Warning( "Activity %s has %d sequences with a total weight of %d!", ActivityList_NameForIndex(activity), actData->count, actData->totalWeight );
		return (m_pSequenceTuples + actData->startingIdx)->seqnum;
	}
	else if ( actData->totalWeight == 1 )
	{
		randomValue = 0;
	}
	else
	{
		// generate a random number from 0 to the total weight
		if ( IsInPrediction() )
		{
			randomValue = SharedRandomInt( "SelectWeightedSequence", 0, weighttotal - 1 );
		}
		else
		{
			randomValue = RandomInt( 0, weighttotal - 1 );
		}
	}


	// chug through the entries in the list (they are sequential therefore cache-coherent)
	// until we run out of random juice
	SequenceTuple * __restrict sequenceInfo = m_pSequenceTuples + actData->startingIdx;

	const SequenceTuple *const stopHere = sequenceInfo + actData->count; // this is a backup 
		// in case the weights are somehow miscalculated -- we don't read or write through
		// it (because it aliases the restricted pointer above); it's only here for 
		// the comparison.

	while (randomValue >= sequenceInfo->weight && sequenceInfo < stopHere)
	{
		randomValue -= sequenceInfo->weight;
		++sequenceInfo;
	}

	return sequenceInfo->seqnum;

}

int CStudioHdr::CActivityToSequenceMapping::SelectWeightedSequenceFromModifiers( CStudioHdr *pstudiohdr, int activity, CUtlSymbol *pActivityModifiers, int iModifierCount )
{
	if ( !pstudiohdr->SequencesAvailable() )
	{
		return ACTIVITY_NOT_AVAILABLE;
	}

	VerifySequenceIndex( pstudiohdr );

	if ( pstudiohdr->GetNumSeq() == 1 )
	{
		return ( ::GetSequenceActivity( pstudiohdr, 0, NULL ) == activity ) ? 0 : ACTIVITY_NOT_AVAILABLE;
	}

	if (!ValidateAgainst(pstudiohdr))
	{
		AssertMsg1(false, "CStudioHdr %s has changed its vmodel pointer without reinitializing its activity mapping! Now performing emergency reinitialization.", pstudiohdr->pszName());
		ExecuteOnce(DebuggerBreakIfDebugging());
		Reinitialize(pstudiohdr);
	}

	// a null m_pSequenceTuples just means that this studio header has no activities.
	if (!m_pSequenceTuples)
		return ACTIVITY_NOT_AVAILABLE;

	// get the data for the given activity
	HashValueType dummy( activity, 0, 0, 0 );
	UtlHashHandle_t handle = m_ActToSeqHash.Find(dummy);
	if (!m_ActToSeqHash.IsValidHandle(handle))
	{
		return ACTIVITY_NOT_AVAILABLE;
	}
	const HashValueType * __restrict actData = &m_ActToSeqHash[handle];
	
	// go through each sequence and give actmod matches preference. LACK of an actmod is a detriment
	int top_score = -1;
	CUtlVector<int> topScoring( actData->count, actData->count );	
	for ( int i = 0; i < actData->count; i++ )
	{
		SequenceTuple * __restrict sequenceInfo = m_pSequenceTuples + actData->startingIdx + i;
		int score = 0;

		int num_seq_modifiers = sequenceInfo->iNumActivityModifiers;
		for ( int k = 0; k < num_seq_modifiers; k++ )
		{
			bool bFoundMod = false;

			for ( int m = 0; m < iModifierCount; m++ )
			{
				if ( sequenceInfo->pActivityModifiers[ k ] == pActivityModifiers[ m ] )
				{
					bFoundMod = true;
				}
			}

			if ( bFoundMod )
			{
				score += 2;
			}
			else
			{
				score--;
			}

		}
		
		if ( score >= top_score )
		{
			if ( score > top_score )
			{
				topScoring.RemoveAll();
			}
		
			for ( int n=0; n<sequenceInfo->weight; n++ )
			{
				topScoring.AddToTail( sequenceInfo->seqnum );
			}
		
			top_score = score;
		}
	}

	// randomly pick between the highest scoring sequences ( NOTE: this method of selecting a sequence ignores activity weights )
	//if ( IsInPrediction() )
	//{
	//	return topScoring[ SharedRandomInt( "SelectWeightedSequence", 0, topScoring.Count() - 1 ) ];
	//}
	if ( topScoring.Count() )
	{
		return topScoring[ RandomInt( 0, topScoring.Count() - 1 ) ];
	}
	else
	{
		return -1;
	}
}


#endif

int SelectHeaviestSequence( CStudioHdr *pstudiohdr, int activity )
{
	if ( !pstudiohdr )
		return 0;

	VerifySequenceIndex( pstudiohdr );

	int maxweight = 0;
	int seq = ACTIVITY_NOT_AVAILABLE;
	int weight = 0;
	for (int i = 0; i < pstudiohdr->GetNumSeq(); i++)
	{
		int curActivity = GetSequenceActivity( pstudiohdr, i, &weight );
		if (curActivity == activity)
		{
			if ( iabs(weight) > maxweight )
			{
				maxweight = iabs(weight);
				seq = i;
			}
		}
	}

	return seq;
}

void GetEyePosition ( CStudioHdr *pstudiohdr, Vector &vecEyePosition )
{
	if ( !pstudiohdr )
	{
		Warning( "GetEyePosition() Can't get pstudiohdr ptr!\n" );
		return;
	}

	vecEyePosition = pstudiohdr->eyeposition();
}


//-----------------------------------------------------------------------------
// Purpose: Looks up an activity by name.
// Input  : label - Name of the activity to look up, ie "ACT_IDLE"
// Output : Activity index or ACT_INVALID if not found.
//-----------------------------------------------------------------------------
int LookupActivity( CStudioHdr *pstudiohdr, const char *label )
{
	VPROF( "LookupActivity" );

	if ( !pstudiohdr )
	{
		return 0;
	}

	for ( int i = 0; i < pstudiohdr->GetNumSeq(); i++ )
	{
		mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( i );
		if ( stricmp( seqdesc.pszActivityName(), label ) == 0 )
		{
			return seqdesc.activity;
		}
	}

	return ACT_INVALID;
}

#if !defined( MAKEXVCD )
//-----------------------------------------------------------------------------
// Purpose: Looks up a sequence by sequence name first, then by activity name.
// Input  : label - The sequence name or activity name to look up.
// Output : Returns the sequence index of the matching sequence, or ACT_INVALID.
//-----------------------------------------------------------------------------
int LookupSequence( CStudioHdr *pstudiohdr, const char *label )
{
	VPROF( "LookupSequence" );

	if (! pstudiohdr)
		return 0;

	if (!pstudiohdr->SequencesAvailable())
		return 0;

	//
	// Look up by sequence name.
	//
	int iSequence = pstudiohdr->LookupSequence( label );
	if ( iSequence != -1 )
		return iSequence;

	//
	// Not found, look up by activity name.
	//
	int nActivity = LookupActivity( pstudiohdr, label );
	if (nActivity != ACT_INVALID )
	{
		return SelectWeightedSequence( pstudiohdr, nActivity );
	}

	return ACT_INVALID;
}

void GetSequenceLinearMotion( CStudioHdr *pstudiohdr, int iSequence, const float poseParameter[], Vector *pVec )
{
	if (! pstudiohdr)
	{
		Msg( "Bad pstudiohdr in GetSequenceLinearMotion()!\n" );
		return;
	}

	if (!pstudiohdr->SequencesAvailable())
		return;

	if( iSequence < 0 || iSequence >= pstudiohdr->GetNumSeq() )
	{
		// Don't spam on bogus model
		if ( pstudiohdr->GetNumSeq() > 0 )
		{
			static int msgCount = 0;
			while ( ++msgCount < 10 )
			{
				DevMsg( "Bad sequence (%i out of %i max) in GetSequenceLinearMotion() for model '%s'!\n", iSequence, pstudiohdr->GetNumSeq(), pstudiohdr->pszName() );
			}
		}
		pVec->Init();
		return;
	}

	QAngle vecAngles;
	Studio_SeqMovement( pstudiohdr, iSequence, 0, 1.0, poseParameter, (*pVec), vecAngles );
}

float GetSequenceLinearMotionAndDuration( CStudioHdr *pstudiohdr, int iSequence, const float poseParameter[], Vector *pVec )
{
	pVec->Init();
	if ( !pstudiohdr )
	{
		Msg( "Bad pstudiohdr in GetSequenceLinearMotion()!\n" );
		return 0.0f;
	}

	if ( !pstudiohdr->SequencesAvailable() )
		return 0.0f;

	if ( iSequence < 0 || iSequence >= pstudiohdr->GetNumSeq() )
	{
		// Don't spam on bogus model
		if ( pstudiohdr->GetNumSeq() > 0 )
		{
			static int msgCount = 0;
			while ( ++msgCount < 10 )
			{
				DevMsg( "Bad sequence (%i out of %i max) in GetSequenceLinearMotion() for model '%s'!\n", iSequence, pstudiohdr->GetNumSeq(), pstudiohdr->pszName() );
			}
		}
		return 0.0f;
	}

	return Studio_SeqMovementAndDuration( pstudiohdr, iSequence, 0, 1.0, poseParameter, (*pVec) );
}

#endif

const char *GetSequenceName( CStudioHdr *pstudiohdr, int iSequence )
{
	if( !pstudiohdr || iSequence < 0 || iSequence >= pstudiohdr->GetNumSeq() )
	{
		if ( pstudiohdr )
		{
			DevMsg( "Bad sequence in GetSequenceName() for model '%s'!\n", pstudiohdr->pszName() );
		}
		return "Unknown";
	}

	mstudioseqdesc_t	&seqdesc = pstudiohdr->pSeqdesc( iSequence );
	return seqdesc.pszLabel();
}

const char *GetSequenceActivityName( CStudioHdr *pstudiohdr, int iSequence )
{
	if( !pstudiohdr || iSequence < 0 || iSequence >= pstudiohdr->GetNumSeq() )
	{
		if ( pstudiohdr )
		{
			DevMsg( "Bad sequence in GetSequenceActivityName() for model '%s'!\n", pstudiohdr->pszName() );
		}
		return "Unknown";
	}

	mstudioseqdesc_t	&seqdesc = pstudiohdr->pSeqdesc( iSequence );
	return seqdesc.pszActivityName( );
}

int GetSequenceFlags( CStudioHdr *pstudiohdr, int sequence )
{
	if ( !pstudiohdr || 
		 !pstudiohdr->SequencesAvailable() ||
		sequence < 0 || 
		sequence >= pstudiohdr->GetNumSeq() )
	{
		return 0;
	}

	mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( sequence );

	return seqdesc.flags;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pstudiohdr - 
//			sequence - 
//			type - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool HasAnimationEventOfType( CStudioHdr *pstudiohdr, int sequence, int type )
{
	if ( !pstudiohdr || sequence >= pstudiohdr->GetNumSeq() )
		return false;

	mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( sequence );
	if ( !&seqdesc )
		return false;

	mstudioevent_t *pevent = GetEventIndexForSequence( seqdesc );
	if ( !pevent )
		return false;

	if (seqdesc.numevents == 0 )
		return false;

	int index;
	for ( index = 0; index < (int)seqdesc.numevents; index++ )
	{
		if ( pevent[ index ].Event() == type )
		{
			return true;
		}
	}

	return false;
}

struct animtaglookup_t
{
	int nIndex;
	const char* szName;
};

#define REGISTER_ANIMTAG( _n ) { _n, #_n },
const animtaglookup_t g_AnimTagLookupTable[ANIMTAG_COUNT] = 
{
	REGISTER_ANIMTAG( ANIMTAG_UNINITIALIZED )
	REGISTER_ANIMTAG( ANIMTAG_STARTCYCLE_N )
	REGISTER_ANIMTAG( ANIMTAG_STARTCYCLE_NE )
	REGISTER_ANIMTAG( ANIMTAG_STARTCYCLE_E )
	REGISTER_ANIMTAG( ANIMTAG_STARTCYCLE_SE )
	REGISTER_ANIMTAG( ANIMTAG_STARTCYCLE_S )
	REGISTER_ANIMTAG( ANIMTAG_STARTCYCLE_SW )
	REGISTER_ANIMTAG( ANIMTAG_STARTCYCLE_W )
	REGISTER_ANIMTAG( ANIMTAG_STARTCYCLE_NW )

	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMIN_IDLE )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMAX_IDLE )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMIN_WALK )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMAX_WALK )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMIN_RUN )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMAX_RUN )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMIN_CROUCHIDLE )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMAX_CROUCHIDLE )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMIN_CROUCHWALK )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_YAWMAX_CROUCHWALK )

	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_PITCHMIN_IDLE )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_PITCHMAX_IDLE )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_PITCHMIN_WALKRUN )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_PITCHMAX_WALKRUN )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_PITCHMIN_CROUCH )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_PITCHMAX_CROUCH )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_PITCHMIN_CROUCHWALK )
	REGISTER_ANIMTAG( ANIMTAG_AIMLIMIT_PITCHMAX_CROUCHWALK )
	
	REGISTER_ANIMTAG( ANIMTAG_FLASHBANG_PASSABLE )

	REGISTER_ANIMTAG( ANIMTAG_WEAPON_POSTLAYER )
};

int IndexFromAnimTagName( const char* szName )
{
	int i;
	for ( i=1; i<ANIMTAG_COUNT; i++ )
	{
		const animtaglookup_t *pAnimTag = &g_AnimTagLookupTable[i];
		if ( !V_strcmp( szName, pAnimTag->szName ) )
		{
			return pAnimTag->nIndex;
		}
	}
	return ANIMTAG_INVALID;
}

float GetFirstSequenceAnimTag( CStudioHdr *pstudiohdr, int sequence, int nDesiredTag, float flStart, float flEnd )
{
	if ( !pstudiohdr || sequence >= pstudiohdr->GetNumSeq() )
		return flStart;

	mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( sequence );
	if (seqdesc.numanimtags == 0 )
		return flStart;

	mstudioanimtag_t *panimtag = NULL;

	int index;
	for ( index = 0; index < (int)seqdesc.numanimtags; index++ )
	{
		panimtag = seqdesc.pAnimTag(index);

		if ( panimtag->tag == ANIMTAG_INVALID )
			continue;

		if ( panimtag->tag == ANIMTAG_UNINITIALIZED )
		{
			panimtag->tag = IndexFromAnimTagName( panimtag->pszTagName() );
		}

		if ( panimtag->tag == nDesiredTag && panimtag->cycle >= flStart && panimtag->cycle < flEnd )
		{
			return panimtag->cycle;
		}
	}

	return flStart;
}

float GetAnySequenceAnimTag( CStudioHdr *pstudiohdr, int sequence, int nDesiredTag, float flDefault )
{
	if ( !pstudiohdr || sequence >= pstudiohdr->GetNumSeq() )
		return flDefault;

	mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( sequence );
	if (seqdesc.numanimtags == 0 )
		return flDefault;

	mstudioanimtag_t *panimtag = NULL;

	int index;
	for ( index = 0; index < (int)seqdesc.numanimtags; index++ )
	{
		panimtag = seqdesc.pAnimTag(index);

		if ( panimtag->tag == ANIMTAG_INVALID )
			continue;

		if ( panimtag->tag == ANIMTAG_UNINITIALIZED )
		{
			panimtag->tag = IndexFromAnimTagName( panimtag->pszTagName() );
		}

		if ( panimtag->tag == nDesiredTag )
		{
			return panimtag->cycle;
		}
	}

	return flDefault;
}

int GetAnimationEvent( CStudioHdr *pstudiohdr, int sequence, animevent_t *pNPCEvent, float flStart, float flEnd, int index )
{
	if ( !pstudiohdr || sequence >= pstudiohdr->GetNumSeq() || !pNPCEvent )
		return 0;

	mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( sequence );
	if (seqdesc.numevents == 0 || index >= (int)seqdesc.numevents )
		return 0;

	// Msg( "flStart %f flEnd %f (%d) %s\n", flStart, flEnd, seqdesc.numevents, seqdesc.label );
	mstudioevent_t *pevent = GetEventIndexForSequence( seqdesc );
	for (; index < (int)seqdesc.numevents; index++)
	{
		// Don't send client-side events to the server AI
		if ( pevent[index].type & AE_TYPE_NEWEVENTSYSTEM )
		{
			if ( !(pevent[index].type & AE_TYPE_SERVER) )
				 continue;
		}
		else if ( pevent[index].Event_OldSystem() >= EVENT_CLIENT ) //Adrian - Support the old event system
			continue;
	
		bool bOverlapEvent = false;

		if (pevent[index].cycle >= flStart && pevent[index].cycle < flEnd)
		{
			bOverlapEvent = true;
		}
		// FIXME: doesn't work with animations being played in reverse
		else if ((seqdesc.flags & STUDIO_LOOPING) && flEnd < flStart)
		{
			if (pevent[index].cycle >= flStart || pevent[index].cycle < flEnd)
			{
				bOverlapEvent = true;
			}
		}

		if (bOverlapEvent)
		{
			pNPCEvent->pSource = NULL;
			pNPCEvent->cycle = pevent[index].cycle;
#if !defined( MAKEXVCD )
			pNPCEvent->eventtime = gpGlobals->curtime;
#else
			pNPCEvent->eventtime = 0.0f;
#endif
			pNPCEvent->type	= pevent[index].type;
			pNPCEvent->Event( pevent[index].Event() );
			pNPCEvent->options = pevent[index].pszOptions();
			return index + 1;
		}
	}
	return 0;
}



int FindTransitionSequence( CStudioHdr *pstudiohdr, int iCurrentSequence, int iGoalSequence, int *piDir )
{
	if ( !pstudiohdr )
		return iGoalSequence;

	if ( !pstudiohdr->SequencesAvailable() )
		return iGoalSequence;

	if ( ( iCurrentSequence < 0 ) || ( iCurrentSequence >= pstudiohdr->GetNumSeq() ) )
		return iGoalSequence;

	if ( ( iGoalSequence < 0 ) || ( iGoalSequence >= pstudiohdr->GetNumSeq() ) )
	{
		// asking for a bogus sequence.  Punt.
		Assert( 0 );
		return iGoalSequence;
	}


	// bail if we're going to or from a node 0
	if (pstudiohdr->EntryNode( iCurrentSequence ) == 0 || pstudiohdr->EntryNode( iGoalSequence ) == 0)
	{
		*piDir = 1;
		return iGoalSequence;
	}

	int	iEndNode;

	// Msg( "from %d to %d: ", pEndNode->iEndNode, pGoalNode->iStartNode );

	// check to see if we should be going forward or backward through the graph
	if (*piDir > 0)
	{
		iEndNode = pstudiohdr->ExitNode( iCurrentSequence );
	}
	else
	{
		iEndNode = pstudiohdr->EntryNode( iCurrentSequence );
	}

	// if both sequences are on the same node, just go there
	if (iEndNode == pstudiohdr->EntryNode( iGoalSequence ))
	{
		*piDir = 1;
		return iGoalSequence;
	}

	int iInternNode = pstudiohdr->GetTransition( iEndNode, pstudiohdr->EntryNode( iGoalSequence ) );

	// if there is no transitionial node, just go to the goal sequence
	if (iInternNode == 0)
		return iGoalSequence;

	int i;

	// look for someone going from the entry node to next node it should hit
	// this may be the goal sequences node or an intermediate node
	for (i = 0; i < pstudiohdr->GetNumSeq(); i++)
	{
		mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc(i );
		if (pstudiohdr->EntryNode( i ) == iEndNode && pstudiohdr->ExitNode( i ) == iInternNode)
		{
			*piDir = 1;
			return i;
		}
		if (seqdesc.nodeflags)
		{
			if (pstudiohdr->ExitNode( i ) == iEndNode && pstudiohdr->EntryNode( i ) == iInternNode)
			{
				*piDir = -1;
				return i;
			}
		}
	}

	// this means that two parts of the node graph are not connected.
	DevMsg( 2, "error in transition graph: %s to %s\n",  pstudiohdr->pszNodeName( iEndNode ), pstudiohdr->pszNodeName( pstudiohdr->EntryNode( iGoalSequence ) ));
	// Go ahead and jump to the goal sequence
	return iGoalSequence;
}






bool GotoSequence( CStudioHdr *pstudiohdr, int iCurrentSequence, float flCurrentCycle, float flCurrentRate, int iGoalSequence, int &nNextSequence, float &flNextCycle, int &iNextDir )
{
	if ( !pstudiohdr )
		return false;

	if ( !pstudiohdr->SequencesAvailable() )
		return false;

	if ( ( iCurrentSequence < 0 ) || ( iCurrentSequence >= pstudiohdr->GetNumSeq() ) )
		return false;

	if ( ( iGoalSequence < 0 ) || ( iGoalSequence >= pstudiohdr->GetNumSeq() ) )
	{
		// asking for a bogus sequence.  Punt.
		Assert( 0 );
		return false;
	}

	// bail if we're going to or from a node 0
	if (pstudiohdr->EntryNode( iCurrentSequence ) == 0 || pstudiohdr->EntryNode( iGoalSequence ) == 0)
	{
		iNextDir = 1;
		flNextCycle = 0.0;
		nNextSequence = iGoalSequence;
		return true;
	}

	int	iEndNode = pstudiohdr->ExitNode( iCurrentSequence );
	// Msg( "from %d to %d: ", pEndNode->iEndNode, pGoalNode->iStartNode );

	// if we're in a transition sequence
	if (pstudiohdr->EntryNode( iCurrentSequence ) != pstudiohdr->ExitNode( iCurrentSequence ))
	{
		// are we done with it?
		if (flCurrentRate > 0.0 && flCurrentCycle >= 0.999)
		{
			iEndNode = pstudiohdr->ExitNode( iCurrentSequence );
		}
		else if (flCurrentRate < 0.0 && flCurrentCycle <= 0.001)
		{
			iEndNode = pstudiohdr->EntryNode( iCurrentSequence );
		}
		else
		{
			// nope, exit
			return false;
		}
	}

	// if both sequences are on the same node, just go there
	if (iEndNode == pstudiohdr->EntryNode( iGoalSequence ))
	{
		iNextDir = 1;
		flNextCycle = 0.0;
		nNextSequence = iGoalSequence;
		return true;
	}

	int iInternNode = pstudiohdr->GetTransition( iEndNode, pstudiohdr->EntryNode( iGoalSequence ) );

	// if there is no transitionial node, just go to the goal sequence
	if (iInternNode == 0)
	{
		iNextDir = 1;
		flNextCycle = 0.0;
		nNextSequence = iGoalSequence;
		return true;
	}

	int i;

	// look for someone going from the entry node to next node it should hit
	// this may be the goal sequences node or an intermediate node
	for (i = 0; i < pstudiohdr->GetNumSeq(); i++)
	{
		mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc(i );
		if (pstudiohdr->EntryNode( i ) == iEndNode && pstudiohdr->ExitNode( i ) == iInternNode)
		{
			iNextDir = 1;
			flNextCycle = 0.0;
			nNextSequence = i;
			return true;
		}
		if (seqdesc.nodeflags)
		{
			if (pstudiohdr->ExitNode( i ) == iEndNode && pstudiohdr->EntryNode( i ) == iInternNode)
			{
				iNextDir = -1;
				flNextCycle = 0.999;	
				nNextSequence = i;
				return true;
			}
		}
	}

	// this means that two parts of the node graph are not connected.
	DevMsg( 2, "error in transition graph: %s to %s\n",  pstudiohdr->pszNodeName( iEndNode ), pstudiohdr->pszNodeName( pstudiohdr->EntryNode( iGoalSequence ) ));
	return false;
}

void SetBodygroupPreset( CStudioHdr *pstudiohdr, int& body, char const *szName )
{
	if (!pstudiohdr)
		return;

	for ( int i=0; i<pstudiohdr->GetNumBodyGroupPresets(); i++ )
	{
		const mstudiobodygrouppreset_t *pBodygroupPreset = pstudiohdr->GetBodyGroupPreset( i );
		if ( !V_strcmp( szName, pBodygroupPreset->pszName() ) )
		{

			for ( int j=0; j<pstudiohdr->numbodyparts(); j++ )
			{
				mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( j );

				int iMask = (pBodygroupPreset->iMask / pbodypart->base) % pbodypart->nummodels;
				if ( iMask == 1 )
				{
					int iCurrent = (body / pbodypart->base) % pbodypart->nummodels;
					int iValCurrent = (pBodygroupPreset->iValue / pbodypart->base) % pbodypart->nummodels;

					body = (body - (iCurrent * pbodypart->base) + (iValCurrent * pbodypart->base));
				}

			}
			break;
		}
	}
}

void SetBodygroup( CStudioHdr *pstudiohdr, int& body, int iGroup, int iValue )
{
	if (! pstudiohdr)
		return;

	if (iGroup >= pstudiohdr->numbodyparts())
		return;

	mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );

	if (iValue >= pbodypart->nummodels)
		return;

	int iCurrent = (body / pbodypart->base) % pbodypart->nummodels;

	body = (body - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));
}


int GetBodygroup( CStudioHdr *pstudiohdr, int body, int iGroup )
{
	if (! pstudiohdr)
		return 0;

	if (iGroup >= pstudiohdr->numbodyparts())
		return 0;

	mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );

	if (pbodypart->nummodels <= 1)
		return 0;

	int iCurrent = (body / pbodypart->base) % pbodypart->nummodels;

	return iCurrent;
}

const char *GetBodygroupName( CStudioHdr *pstudiohdr, int iGroup )
{
	if ( !pstudiohdr)
		return "";

	if (iGroup >= pstudiohdr->numbodyparts())
		return "";

	mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );
	return pbodypart->pszName();
}

const char *GetBodygroupPartName( CStudioHdr *pstudiohdr, int iGroup, int iPart )
{
	if ( !pstudiohdr)
		return "";

	if (iGroup >= pstudiohdr->numbodyparts())
		return "";

	mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );
	if ( iPart < 0 && iPart >= pbodypart->nummodels )
		return "";

	return pbodypart->pModel( iPart )->name;
}

int FindBodygroupByName( CStudioHdr *pstudiohdr, const char *name )
{
	if ( !pstudiohdr )
		return -1;

	int group;
	for ( group = 0; group < pstudiohdr->numbodyparts(); group++ )
	{
		mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( group );
		if ( !Q_strcasecmp( name, pbodypart->pszName() ) )
		{
			return group;
		}
	}

	return -1;
}

int GetBodygroupCount( CStudioHdr *pstudiohdr, int iGroup )
{
	if ( !pstudiohdr )
		return 0;

	if (iGroup >= pstudiohdr->numbodyparts())
		return 0;

	mstudiobodyparts_t *pbodypart = pstudiohdr->pBodypart( iGroup );
	return pbodypart->nummodels;
}

int GetNumBodyGroups( CStudioHdr *pstudiohdr )
{
	if ( !pstudiohdr )
		return 0;

	return pstudiohdr->numbodyparts();
}

int GetSequenceActivity( CStudioHdr *pstudiohdr, int sequence, int *pweight )
{
	if (!pstudiohdr || !pstudiohdr->SequencesAvailable() )
	{
		if (pweight)
			*pweight = 0;
		return 0;
	}

	Assert(sequence >= 0 && sequence < pstudiohdr->GetNumSeq());
	mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( sequence );

	if (!(seqdesc.flags & STUDIO_ACTIVITY))
	{
		SetActivityForSequence( pstudiohdr, sequence );
	}
	if (pweight)
		*pweight = seqdesc.actweight;
	return seqdesc.activity;
}


void GetAttachmentLocalSpace( CStudioHdr *pstudiohdr, int attachIndex, matrix3x4_t &pLocalToWorld )
{
	if ( attachIndex >= 0 )
	{
		const mstudioattachment_t &pAttachment = pstudiohdr->pAttachment(attachIndex);
		MatrixCopy( pAttachment.local, pLocalToWorld );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pstudiohdr - 
//			*name - 
// Output : int
//-----------------------------------------------------------------------------
int FindHitboxSetByName( CStudioHdr *pstudiohdr, const char *name )
{
	if ( !pstudiohdr )
		return -1;

	for ( int i = 0; i < pstudiohdr->numhitboxsets(); i++ )
	{
		mstudiohitboxset_t *set = pstudiohdr->pHitboxSet( i );
		if ( !set )
			continue;

		if ( !stricmp( set->pszName(), name ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pstudiohdr - 
//			setnumber - 
// Output : char const
//-----------------------------------------------------------------------------
const char *GetHitboxSetName( CStudioHdr *pstudiohdr, int setnumber )
{
	if ( !pstudiohdr )
		return "";

	mstudiohitboxset_t *set = pstudiohdr->pHitboxSet( setnumber );
	if ( !set )
		return "";

	return set->pszName();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pstudiohdr - 
// Output : int
//-----------------------------------------------------------------------------
int GetHitboxSetCount( CStudioHdr *pstudiohdr )
{
	if ( !pstudiohdr )
		return 0;

	return pstudiohdr->numhitboxsets();
}
