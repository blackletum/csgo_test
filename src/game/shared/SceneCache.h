﻿//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef SCENECACHE_H
#define SCENECACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlCachedFileData.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

class CChoreoEvent;

#define SCENECACHE_VERSION		7


class CSceneCacheListLess
{
public:
	bool Less( const int &lhs, const int &rhs, void *pCtx )
	{
		return lhs < rhs;
	}
};


#pragma pack(1)
class CSceneCache : public IBaseCacheInfo
{
public:
	unsigned int		msecs;
	CUtlSortVector< int, CSceneCacheListLess > sounds;

	CSceneCache();
//	CSceneCache( const CSceneCache& src );

	int	GetSoundCount() const;
	char const *GetSoundName( int index );

	virtual void Save( CUtlBuffer& buf  );
	virtual void Restore( CUtlBuffer& buf  );
	virtual void Rebuild( char const *filename );

	static unsigned int ComputeSoundScriptFileTimestampChecksum();
	static void PrecacheSceneEvent( CChoreoEvent *event, CUtlSortVector< int, CSceneCacheListLess > &soundlist );
};
#pragma pack()

#endif // SCENECACHE_H
