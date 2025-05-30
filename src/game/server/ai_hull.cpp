﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
//=============================================================================//

#include "cbase.h"
#include "ai_hull.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

struct ai_hull_t
{
	ai_hull_t( int bit, const char *pName, const Vector &_mins, const Vector &_maxs, const Vector &_smallMins, const Vector &_smallMaxs, const unsigned int _nAITraceMask = MASK_NPCWORLDSTATIC )
		: hullBit( bit ), mins( _mins ), maxs( _maxs ), smallMins( _smallMins ), smallMaxs( _smallMaxs ), name( pName ), nAITraceMask( _nAITraceMask ) {}
	int			hullBit;
	const char*	name;

	Vector	mins;
	Vector	maxs;

	Vector	smallMins;
	Vector	smallMaxs;

	unsigned int nAITraceMask;
};

//=================================================================================
// Create the hull types here.
//=================================================================================

#ifdef INFESTED_DLL
ai_hull_t  Human_Hull			(bits_HUMAN_HULL,			"HUMAN_HULL",			Vector(-13,-13,   0),	Vector(13, 13, 72),		Vector(-8,-8,   0),		Vector( 8,  8, 72) );			// Colonists
ai_hull_t  Small_Centered_Hull	(bits_SMALL_CENTERED_HULL,	"SMALL_CENTERED_HULL",	Vector(-20,-20, -20),	Vector(20, 20, 20),		Vector(-12,-12,-12),	Vector(12, 12, 12) );			// -
ai_hull_t  Wide_Human_Hull		(bits_WIDE_HUMAN_HULL,		"WIDE_HUMAN_HULL",		Vector(-20,-20,   0),	Vector(20, 20, 72),		Vector(-20,-20, 0),		Vector(20, 20, 72) );			// Marines
ai_hull_t  Tiny_Hull			(bits_TINY_HULL,			"TINY_HULL",			Vector(-12,-12,   0),	Vector(12, 12, 12),		Vector(-12,-12, 0),	    Vector(12, 12, 12) );			// Parasites/grubs
ai_hull_t  Wide_Short_Hull		(bits_WIDE_SHORT_HULL,		"WIDE_SHORT_HULL",		Vector(-40,-40,   0),	Vector(40, 40, 72),		Vector(-24,-24, 0),	    Vector(24, 24, 32) );			// Harvester/Bodysnatcher/Mortarbug
ai_hull_t  Medium_Hull			(bits_MEDIUM_HULL,			"MEDIUM_HULL",			Vector(-16,-16,   0),	Vector(16, 16, 64),		Vector(-8,-8, 0),	    Vector(8, 8, 64) );				// -
ai_hull_t  Tiny_Centered_Hull	(bits_TINY_CENTERED_HULL,	"TINY_CENTERED_HULL",	Vector(-8,	-8,  -4),	Vector(8, 8,  4),		Vector(-8,-8, -4),		Vector( 8, 8, 4) );				// Buzzer
ai_hull_t  Large_Hull			(bits_LARGE_HULL,			"LARGE_HULL",			Vector(-40,-40,   0),	Vector(40, 40, 110),	Vector(-40,-40, 0),		Vector(40, 40, 110) );			// Shieldbug
ai_hull_t  Large_Centered_Hull	(bits_LARGE_CENTERED_HULL,	"LARGE_CENTERED_HULL",	Vector(-80,-80, 0),		Vector(80, 80, 200),	Vector(-80,-80,0),		Vector(80, 80, 200) );			// Queen
ai_hull_t  Medium_Tall_Hull		(bits_MEDIUM_TALL_HULL,		"MEDIUM_TALL_HULL",		Vector(-22,-22,   0),	Vector(22, 22, 100),	Vector(-22,-22, 0),	    Vector(22, 22, 100) );			// Hunter/Meatbug
ai_hull_t  Tiny_Fluid_Hull		(bits_TINY_FLUID_HULL,		"TINY_FLUID_HULL",		Vector(-8,-8,   0),		Vector(8, 8, 16),		Vector(-8,-8, 0),	    Vector(8, 8, 16),	MASK_NPCWORLDSTATIC_FLUID );		// Blob?
ai_hull_t  MediumBig_Hull		(bits_MEDIUMBIG_HULL,		"MEDIUMBIG_HULL",		Vector(-20,-20,   0),	Vector(20, 20, 69),		Vector(-20,-20, 0),	Vector(20, 20, 69) );				// Drones

#else

ai_hull_t  Human_Hull			(bits_HUMAN_HULL,			"HUMAN_HULL",			Vector(-13,-13,   0),	Vector(13, 13, 72),		Vector(-8,-8,   0),		Vector( 8,  8, 72) );
ai_hull_t  Small_Centered_Hull	(bits_SMALL_CENTERED_HULL,	"SMALL_CENTERED_HULL",	Vector(-20,-20, -20),	Vector(20, 20, 20),		Vector(-12,-12,-12),	Vector(12, 12, 12) );
ai_hull_t  Wide_Human_Hull		(bits_WIDE_HUMAN_HULL,		"WIDE_HUMAN_HULL",		Vector(-15,-15,   0),	Vector(15, 15, 72),		Vector(-10,-10, 0),		Vector(10, 10, 72) );
ai_hull_t  Tiny_Hull			(bits_TINY_HULL,			"TINY_HULL",			Vector(-12,-12,   0),	Vector(12, 12, 24),		Vector(-12,-12, 0),	    Vector(12, 12, 24) );
ai_hull_t  Wide_Short_Hull		(bits_WIDE_SHORT_HULL,		"WIDE_SHORT_HULL",		Vector(-35,-35,   0),	Vector(35, 35, 32),		Vector(-20,-20, 0),	    Vector(20, 20, 32) );
ai_hull_t  Medium_Hull			(bits_MEDIUM_HULL,			"MEDIUM_HULL",			Vector(-16,-16,   0),	Vector(16, 16, 64),		Vector(-8,-8, 0),	    Vector(8, 8, 64) );
ai_hull_t  Tiny_Centered_Hull	(bits_TINY_CENTERED_HULL,	"TINY_CENTERED_HULL",	Vector(-8,	-8,  -4),	Vector(8, 8,  4),		Vector(-8,-8, -4),		Vector( 8, 8, 4) );
ai_hull_t  Large_Hull			(bits_LARGE_HULL,			"LARGE_HULL",			Vector(-40,-40,   0),	Vector(40, 40, 100),	Vector(-40,-40, 0),		Vector(40, 40, 100) );
ai_hull_t  Large_Centered_Hull	(bits_LARGE_CENTERED_HULL,	"LARGE_CENTERED_HULL",	Vector(-38,-38, -38),	Vector(38, 38, 38),		Vector(-30,-30,-30),	Vector(30, 30, 30) );
ai_hull_t  Medium_Tall_Hull		(bits_MEDIUM_TALL_HULL,		"MEDIUM_TALL_HULL",		Vector(-18,-18,   0),	Vector(18, 18, 100),	Vector(-12,-12, 0),	    Vector(12, 12, 100) );
ai_hull_t  Tiny_Fluid_Hull		(bits_TINY_FLUID_HULL,		"TINY_FLUID_HULL",		Vector(-6.5,-6.5,   0),	Vector(6.5, 6.5, 13),	Vector(-6.5,-6.5, 0),	Vector(6.5, 6.5, 13),	MASK_NPCWORLDSTATIC_FLUID );
ai_hull_t  MediumBig_Hull		(bits_MEDIUMBIG_HULL,		"MEDIUMBIG_HULL",		Vector(-17,-17,   0),	Vector(17, 17, 69),		Vector(-15,-15, 0),	Vector(15, 15, 69) );		// asw
#endif

//
// Array of hulls. These hulls must correspond with the enumerations in AI_Hull.h!
//
ai_hull_t*	hull[NUM_HULLS] =	
{
	&Human_Hull,
	&Small_Centered_Hull,
	&Wide_Human_Hull,
	&Tiny_Hull,
	&Wide_Short_Hull,
	&Medium_Hull,
	&Tiny_Centered_Hull,
	&Large_Hull,
	&Large_Centered_Hull,
	&Medium_Tall_Hull,
	&Tiny_Fluid_Hull,
	&MediumBig_Hull,
};


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
const Vector &NAI_Hull::Mins(int id)			
{ 
	return hull[id]->mins;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
const Vector &NAI_Hull::Maxs(int id)			
{ 
	return hull[id]->maxs;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
const Vector &NAI_Hull::SmallMins(int id)			
{ 
	return hull[id]->smallMins;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
const Vector &NAI_Hull::SmallMaxs(int id)			
{ 
	return hull[id]->smallMaxs;
}

//-----------------------------------------------------------------------------
// Purpose:

// Input  :
// Output :
//-----------------------------------------------------------------------------
float NAI_Hull::Length(int id)
{
	return (hull[id]->maxs.x - hull[id]->mins.x); 
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float NAI_Hull::Width(int id)
{
	return (hull[id]->maxs.y - hull[id]->mins.y);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float NAI_Hull::Height(int id)
{
	return (hull[id]->maxs.z - hull[id]->mins.z); 
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int NAI_Hull::Bits(int id)			
{ 
	return hull[id]->hullBit;		
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
const char *NAI_Hull::Name(int id)	
{ 
	return hull[id]->name;	
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
unsigned int NAI_Hull::TraceMask(int id)	
{ 
	return hull[id]->nAITraceMask;	
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Hull_t NAI_Hull::LookupId(const char *szName)	
{ 
	int i;
	if (!szName)
	{
		return HULL_HUMAN;
	}
	for (i = 0; i < NUM_HULLS; i++)
	{
		if (stricmp( szName, NAI_Hull::Name( i )) == 0)
		{
			return (Hull_t)i;
		}
	}
	return HULL_HUMAN;
}



