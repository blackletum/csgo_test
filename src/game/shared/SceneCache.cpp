﻿//====== Copyright © 1996-2007, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "SceneCache.h"
#include "choreoscene.h"
#include "choreoevent.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


extern ISoundEmitterSystemBase *soundemitterbase;
CChoreoScene *BlockingLoadScene( const char *filename );

CSceneCache::CSceneCache()
{
	msecs = 0;
}

//CSceneCache::CSceneCache( const CSceneCache& src )
//{
//	msecs  = src.msecs;
//	sounds = src.sounds;
//}

int	CSceneCache::GetSoundCount() const
{
	return sounds.Count();
}

char const *CSceneCache::GetSoundName( int index )
{
	return soundemitterbase->GetSoundName( sounds[ index ] );
}

void CSceneCache::Save( CUtlBuffer& buf  )
{
	buf.PutUnsignedInt( msecs );

	unsigned short c = GetSoundCount();
	buf.PutShort( c );
	
	Assert( sounds.Count() <= 65536 );

	for ( int i = 0; i < c; ++i )
	{
		buf.PutString( GetSoundName( i ) );
	}
}

void CSceneCache::Restore( CUtlBuffer& buf  )
{
	MEM_ALLOC_CREDIT();

	msecs = buf.GetUnsignedInt();

	unsigned short c = (unsigned short)buf.GetShort();

	for ( int i = 0; i < c; ++i )
	{
		char soundname[ 512 ];
		buf.GetString( soundname, sizeof( soundname ) );

		int idx = soundemitterbase->GetSoundIndex( soundname );
		if ( soundemitterbase->IsValidIndex( idx ) )
		{
			if ( sounds.Find( idx ) == sounds.InvalidIndex() )
			{
				sounds.Insert( idx );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static method
// Input  : *event - 
//			soundlist - 
//-----------------------------------------------------------------------------
void CSceneCache::PrecacheSceneEvent( CChoreoEvent *event, CUtlSortVector< int, CSceneCacheListLess > &soundlist )
{
	if ( !event || event->GetType() != CChoreoEvent::SPEAK )
		return;

	int idx = soundemitterbase->GetSoundIndex( event->GetParameters() );
	if ( soundemitterbase->IsValidIndex( idx ) )
	{
		MEM_ALLOC_CREDIT();
		soundlist.Insert( idx );
	}

	if ( event->GetCloseCaptionType() == CChoreoEvent::CC_MASTER )
	{
		char tok[ CChoreoEvent::MAX_CCTOKEN_STRING ];
		if ( event->GetPlaybackCloseCaptionToken( tok, sizeof( tok ) ) )
		{
			int idx = soundemitterbase->GetSoundIndex( tok );
			if ( soundemitterbase->IsValidIndex( idx ) && soundlist.Find( idx ) == soundlist.InvalidIndex() )
			{
				MEM_ALLOC_CREDIT();
				soundlist.Insert( idx );
			}
		}
	}
}

void CSceneCache::Rebuild( char const *filename )
{
	msecs = 0;
	sounds.RemoveAll();

	CChoreoScene *scene = BlockingLoadScene( filename );
	if ( scene )
	{
		// Walk all events looking for SPEAK events
		CChoreoEvent *event;
		int c = scene->GetNumEvents();
		for ( int i = 0; i < c; ++i )
		{
			event = scene->GetEvent( i );
			PrecacheSceneEvent( event, sounds );
		}

		// Update scene duration, too
		msecs = (int)( scene->FindStopTime() * 1000.0f + 0.5f );

		delete scene;
	}
}
